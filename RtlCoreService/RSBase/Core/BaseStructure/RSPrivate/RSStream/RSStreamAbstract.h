//
//  RSStreamAbstract.h
//  RSCoreFoundation
//
//  Created by closure on 9/28/15.
//  Copyright © 2015 RetVal. All rights reserved.
//

#ifndef RSStreamAbstract_h
#define RSStreamAbstract_h

#include <RSCoreFoundation/RSStream.h>

RS_EXTERN_C_BEGIN

/*  During a stream's lifetime, the open callback will be called once, followed by any number of openCompleted calls (until openCompleted returns TRUE).  Then any number of read/canRead or write/canWrite calls, then a single close call.  copyProperty can be called at any time.  prepareAsynch will be called exactly once when the stream's client is first configured.
 
 Expected semantics:
 - open reserves any system resources that are needed.  The stream may start the process of opening, returning TRUE immediately and setting openComplete to FALSE.  When the open completes, _RSStreamSignalEvent should be called passing kRSStreamOpenCompletedEvent.  openComplete should be set to TRUE only if the open operation completed in its entirety.
 - openCompleted will only be called after open has been called, but before any kRSStreamOpenCompletedEvent has been received.  Return TRUE, setting error.code to 0, if the open operation has completed.  Return TRUE, setting error to the correct error code and domain if the open operation completed, but failed.  Return FALSE if the open operation is still in-progress.  If your open ever fails to complete (i.e. sets openComplete to FALSE), you must be implement the openCompleted callback.
 - read should read into the given buffer, returning the number of bytes successfully read.  read must block until at least one byte is available, but should not block until the entire buffer is filled; zero should only be returned if end-of-stream is encountered. atEOF should be set to true if the EOF is encountered, false otherwise.  error.code should be set to zero if no error occurs; otherwise, error should be set to the appropriate values.
 - getBuffer is an optimization to return an internal buffer of bytes read from the stream, and may return NULL.  getBuffer itself may be NULL if the concrete implementation does not wish to provide an internal buffer.  If implemented, it should set numBytesRead to the number of bytes available in the internal buffer (but should not exceed maxBytesToRead) and return a pointer to the base of the bytes.
 - canRead will only be called once openCompleted reports that the stream has been successfully opened (or the initial open call succeeded).  It should return whether there are bytes that can be read without blocking.
 - write should write the bytes in the given buffer to the device, returning the number of bytes successfully written.  write must block until at least one byte is written.  error.code should be set to zero if no error occurs; otherwise, error should be set to the appropriate values.
 - close should close the device, releasing any reserved system resources.  close cannot fail (it may be called to abort the stream), and may be called at any time after open has been called.  It will only be called once.
 - copyProperty should return the value for the given property, or NULL if none exists.  Composite streams (streams built on top of other streams) should take care to call RSStreamCopyProperty on the base stream if they do not recognize the property given, to give the underlying stream a chance to respond.
 
 In all cases, errors returned by reference will be initialized to NULL by the caller, and if they are set to non-NULL, will
 be released by the caller
 */

typedef struct {
    RSIndex version; /* == 2 */
    
    void *(*create)(RSReadStreamRef stream, void *info);
    void (*finalize)(RSReadStreamRef stream, void *info);
    RSStringRef (*copyDescription)(RSReadStreamRef stream, void *info);
    
    BOOL (*open)(RSReadStreamRef stream, RSErrorRef *error, BOOL *openComplete, void *info);
    BOOL (*openCompleted)(RSReadStreamRef stream, RSErrorRef *error, void *info);
    RSIndex (*read)(RSReadStreamRef stream, UInt8 *buffer, RSIndex bufferLength, RSErrorRef *error, BOOL *atEOF, void *info);
    const UInt8 *(*getBuffer)(RSReadStreamRef stream, RSIndex maxBytesToRead, RSIndex *numBytesRead, RSErrorRef *error, BOOL *atEOF, void *info);
    BOOL (*canRead)(RSReadStreamRef stream, RSErrorRef *error, void *info);
    void (*close)(RSReadStreamRef stream, void *info);
    
    RSTypeRef (*copyProperty)(RSReadStreamRef stream, RSStringRef propertyName, void *info);
    BOOL (*setProperty)(RSReadStreamRef stream, RSStringRef propertyName, RSTypeRef propertyValue, void *info);
    
    void (*requestEvents)(RSReadStreamRef stream, RSOptionFlags streamEvents, void *info);
    void (*schedule)(RSReadStreamRef stream, RSRunLoopRef runLoop, RSStringRef runLoopMode, void *info);
    void (*unschedule)(RSReadStreamRef stream, RSRunLoopRef runLoop, RSStringRef runLoopMode, void *info);
} RSReadStreamCallBacks;

typedef struct {
    RSIndex version; /* == 2 */
    
    void *(*create)(RSWriteStreamRef stream, void *info);
    void (*finalize)(RSWriteStreamRef stream, void *info);
    RSStringRef (*copyDescription)(RSWriteStreamRef stream, void *info);
    
    BOOL (*open)(RSWriteStreamRef stream, RSErrorRef *error, BOOL *openComplete, void *info);
    BOOL (*openCompleted)(RSWriteStreamRef stream, RSErrorRef *error, void *info);
    RSIndex (*write)(RSWriteStreamRef stream, const UInt8 *buffer, RSIndex bufferLength, RSErrorRef *error, void *info);
    BOOL (*canWrite)(RSWriteStreamRef stream, RSErrorRef *error, void *info);
    void (*close)(RSWriteStreamRef stream, void *info);
    
    RSTypeRef (*copyProperty)(RSWriteStreamRef stream, RSStringRef propertyName, void *info);
    BOOL (*setProperty)(RSWriteStreamRef stream, RSStringRef propertyName, RSTypeRef propertyValue, void *info);
    
    void (*requestEvents)(RSWriteStreamRef stream, RSOptionFlags streamEvents, void *info);
    void (*schedule)(RSWriteStreamRef stream, RSRunLoopRef runLoop, RSStringRef runLoopMode, void *info);
    void (*unschedule)(RSWriteStreamRef stream, RSRunLoopRef runLoop, RSStringRef runLoopMode, void *info);
} RSWriteStreamCallBacks;

// Primitive creation mechanisms.
RSExport RSReadStreamRef RSReadStreamCreate(RSAllocatorRef alloc, const RSReadStreamCallBacks *callbacks, void *info);
RSExport RSWriteStreamRef RSWriteStreamCreate(RSAllocatorRef alloc, const RSWriteStreamCallBacks *callbacks, void *info);

/* All the functions below can only be called when you are sure the stream in question was created via
 RSReadStreamCreate() or RSWriteStreamCreate(), above.  They are NOT safe for toll-free bridged objects,
 so the caller must be sure the argument passed is not such an object. */

// To be called by the concrete stream implementation (the callbacks) when an event occurs. error may be NULL if event != kRSStreamEventErrorOccurred
// error should be a RSErrorRef if the callbacks are version 2 or later; otherwise it should be a (RSStreamError *).
RSExport void RSReadStreamSignalEvent(RSReadStreamRef stream, RSStreamEventType event, const void *error);
RSExport void RSWriteStreamSignalEvent(RSWriteStreamRef stream, RSStreamEventType event, const void *error);

// These require that the stream allow the run loop to run once before delivering the event to its client.
// See the comment above RSRead/WriteStreamSignalEvent for interpretation of the error argument.
RSExport void _RSReadStreamSignalEventDelayed(RSReadStreamRef stream, RSStreamEventType event, const void *error);
RSExport void _RSWriteStreamSignalEventDelayed(RSWriteStreamRef stream, RSStreamEventType event, const void *error);

RSExport void _RSReadStreamClearEvent(RSReadStreamRef stream, RSStreamEventType event);
// Write variant not currently needed
//RSExport //void _RSWriteStreamClearEvent(RSWriteStreamRef stream, RSStreamEventType event);

// Convenience for concrete implementations to extract the info pointer given the stream.
RSExport void *RSReadStreamGetInfoPointer(RSReadStreamRef stream);
RSExport void *RSWriteStreamGetInfoPointer(RSWriteStreamRef stream);

// Returns the client info pointer currently set on the stream.  These should probably be made public one day.
RSExport void *_RSReadStreamGetClient(RSReadStreamRef readStream);
RSExport void *_RSWriteStreamGetClient(RSWriteStreamRef writeStream);

// Returns an array of the runloops and modes on which the stream is currently scheduled
// Note that these are unretained mutable arrays - use the copy variant instead.
RSExport RSArrayRef _RSReadStreamGetRunLoopsAndModes(RSReadStreamRef readStream);
RSExport RSArrayRef _RSWriteStreamGetRunLoopsAndModes(RSWriteStreamRef writeStream);

// Returns an array of the runloops and modes on which the stream is currently scheduled
RSExport RSArrayRef _RSReadStreamCopyRunLoopsAndModes(RSReadStreamRef readStream);
RSExport RSArrayRef _RSWriteStreamCopyRunLoopsAndModes(RSWriteStreamRef writeStream);

/* Deprecated versions; here for backwards compatibility. */
typedef struct {
    RSIndex version; /* == 1 */
    void *(*create)(RSReadStreamRef stream, void *info);
    void (*finalize)(RSReadStreamRef stream, void *info);
    RSStringRef (*copyDescription)(RSReadStreamRef stream, void *info);
    BOOL (*open)(RSReadStreamRef stream, RSStreamError *error, BOOL *openComplete, void *info);
    BOOL (*openCompleted)(RSReadStreamRef stream, RSStreamError *error, void *info);
    RSIndex (*read)(RSReadStreamRef stream, UInt8 *buffer, RSIndex bufferLength, RSStreamError *error, BOOL *atEOF, void *info);
    const UInt8 *(*getBuffer)(RSReadStreamRef stream, RSIndex maxBytesToRead, RSIndex *numBytesRead, RSStreamError *error, BOOL *atEOF, void *info);
    BOOL (*canRead)(RSReadStreamRef stream, void *info);
    void (*close)(RSReadStreamRef stream, void *info);
    RSTypeRef (*copyProperty)(RSReadStreamRef stream, RSStringRef propertyName, void *info);
    BOOL (*setProperty)(RSReadStreamRef stream, RSStringRef propertyName, RSTypeRef propertyValue, void *info);
    void (*requestEvents)(RSReadStreamRef stream, RSOptionFlags streamEvents, void *info);
    void (*schedule)(RSReadStreamRef stream, RSRunLoopRef runLoop, RSStringRef runLoopMode, void *info);
    void (*unschedule)(RSReadStreamRef stream, RSRunLoopRef runLoop, RSStringRef runLoopMode, void *info);
} RSReadStreamCallBacksV1;

typedef struct {
    RSIndex version; /* == 1 */
    void *(*create)(RSWriteStreamRef stream, void *info);
    void (*finalize)(RSWriteStreamRef stream, void *info);
    RSStringRef (*copyDescription)(RSWriteStreamRef stream, void *info);
    BOOL (*open)(RSWriteStreamRef stream, RSStreamError *error, BOOL *openComplete, void *info);
    BOOL (*openCompleted)(RSWriteStreamRef stream, RSStreamError *error, void *info);
    RSIndex (*write)(RSWriteStreamRef stream, const UInt8 *buffer, RSIndex bufferLength, RSStreamError *error, void *info);
    BOOL (*canWrite)(RSWriteStreamRef stream, void *info);
    void (*close)(RSWriteStreamRef stream, void *info);
    RSTypeRef (*copyProperty)(RSWriteStreamRef stream, RSStringRef propertyName, void *info);
    BOOL (*setProperty)(RSWriteStreamRef stream, RSStringRef propertyName, RSTypeRef propertyValue, void *info);
    void (*requestEvents)(RSWriteStreamRef stream, RSOptionFlags streamEvents, void *info);
    void (*schedule)(RSWriteStreamRef stream, RSRunLoopRef runLoop, RSStringRef runLoopMode, void *info);
    void (*unschedule)(RSWriteStreamRef stream, RSRunLoopRef runLoop, RSStringRef runLoopMode, void *info);
} RSWriteStreamCallBacksV1;

typedef struct {
    RSIndex version; /* == 0 */
    BOOL (*open)(RSReadStreamRef stream, RSStreamError *error, BOOL *openComplete, void *info);
    BOOL (*openCompleted)(RSReadStreamRef stream, RSStreamError *error, void *info);
    RSIndex (*read)(RSReadStreamRef stream, UInt8 *buffer, RSIndex bufferLength, RSStreamError *error, BOOL *atEOF, void *info);
    const UInt8 *(*getBuffer)(RSReadStreamRef stream, RSIndex maxBytesToRead, RSIndex *numBytesRead, RSStreamError *error, BOOL *atEOF, void *info);
    BOOL (*canRead)(RSReadStreamRef stream, void *info);
    void (*close)(RSReadStreamRef stream, void *info);
    RSTypeRef (*copyProperty)(RSReadStreamRef stream, RSStringRef propertyName, void *info);
    void (*schedule)(RSReadStreamRef stream, RSRunLoopRef runLoop, RSStringRef runLoopMode, void *info);
    void (*unschedule)(RSReadStreamRef stream, RSRunLoopRef runLoop, RSStringRef runLoopMode, void *info);
} RSReadStreamCallBacksV0;

typedef struct {
    RSIndex version; /* == 0 */
    BOOL (*open)(RSWriteStreamRef stream, RSStreamError *error, BOOL *openComplete, void *info);
    BOOL (*openCompleted)(RSWriteStreamRef stream, RSStreamError *error, void *info);
    RSIndex (*write)(RSWriteStreamRef stream, const UInt8 *buffer, RSIndex bufferLength, RSStreamError *error, void *info);
    BOOL (*canWrite)(RSWriteStreamRef stream, void *info);
    void (*close)(RSWriteStreamRef stream, void *info);
    RSTypeRef (*copyProperty)(RSWriteStreamRef stream, RSStringRef propertyName, void *info);
    void (*schedule)(RSWriteStreamRef stream, RSRunLoopRef runLoop, RSStringRef runLoopMode, void *info);
    void (*unschedule)(RSWriteStreamRef stream, RSRunLoopRef runLoop, RSStringRef runLoopMode, void *info);
} RSWriteStreamCallBacksV0;

RS_EXTERN_C_END

#endif /* RSStreamAbstract_h */
