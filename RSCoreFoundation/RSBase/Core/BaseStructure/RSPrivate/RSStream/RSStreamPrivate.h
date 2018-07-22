//
//  RSStreamPrivate.h
//  RSCoreFoundation
//
//  Created by closure on 9/28/15.
//  Copyright © 2015 RetVal. All rights reserved.
//

#ifndef RSStreamPrivate_h
#define RSStreamPrivate_h

#include <RSCoreFoundation/RSStream.h>
#include <RSCoreFoundation/RSRunLoop.h>
#include <RSCoreFoundation/RSRuntime.h>

RS_EXTERN_C_BEGIN

struct _RSStream;
struct _RSStreamClient {
    RSStreamClientContext cbContext;
    void (*cb)(struct _RSStream *, RSStreamEventType, void *);
    RSOptionFlags when;
    RSRunLoopSourceRef rlSource;
    RSMutableArrayRef runLoopsAndModes;
    RSOptionFlags whatToSignal;
};

#define RSStreamCurrentVersion 2

// A unified set of callbacks so we can use a single structure for all struct _RSStreams.
struct _RSStreamCallBacks {
    RSIndex version;
    void *(*create)(struct _RSStream *stream, void *info);
    void (*finalize)(struct _RSStream *stream, void *info);
    RSStringRef (*copyDescription)(struct _RSStream *stream, void *info);
    
    BOOL (*open)(struct _RSStream *stream, RSErrorRef *error, BOOL *openComplete, void *info);
    BOOL (*openCompleted)(struct _RSStream *stream, RSErrorRef *error, void *info);
    RSIndex (*read)(RSReadStreamRef stream, UInt8 *buffer, RSIndex bufferLength, RSErrorRef *error, BOOL *atEOF, void *info);
    const UInt8 *(*getBuffer)(RSReadStreamRef sream, RSIndex maxBytesToRead, RSIndex *numBytesRead, RSErrorRef *error, BOOL *atEOF, void *info);
    BOOL (*canRead)(RSReadStreamRef, RSErrorRef *error, void *info);
    RSIndex (*write)(RSWriteStreamRef, const UInt8 *buffer, RSIndex bufferLength, RSErrorRef *error, void *info);
    BOOL (*canWrite)(RSWriteStreamRef, RSErrorRef *error, void *info);
    void (*close)(struct _RSStream *stream, void *info);
    
    RSTypeRef (*copyProperty)(struct _RSStream *stream, RSStringRef propertyName, void *info);
    BOOL (*setProperty)(struct _RSStream *stream, RSStringRef propertyName, RSTypeRef propertyValue, void *info);
    void (*requestEvents)(struct _RSStream *stream, RSOptionFlags events, void *info);
    void (*schedule)(struct _RSStream *stream, RSRunLoopRef runLoop, RSStringRef runLoopMode, void *info);
    void (*unschedule)(struct _RSStream *stream, RSRunLoopRef runLoop, RSStringRef runLoopMode, void *info);
};

struct _RSStream;

RSExport void* _RSStreamGetInfoPointer(struct _RSStream* stream);

// cb version must be > 0
RSExport struct _RSStream *_RSStreamCreateWithConstantCallbacks(RSAllocatorRef alloc, void *info, const struct _RSStreamCallBacks *cb, BOOL isReading);

// Only available for streams created with _RSStreamCreateWithConstantCallbacks, above. cb's version must be 1
RSExport void _RSStreamSetInfoPointer(struct _RSStream *stream, void *info, const struct _RSStreamCallBacks *cb);

/*
 ** _RSStreamSourceScheduleWithRunLoop
 **
 ** Schedules the given run loop source on the given run loop and mode.  It then
 ** adds the loop and mode pair to the runLoopsAndModes list.  The list is
 ** simply a linear list of a loop reference followed by a mode reference.
 **
 ** source Run loop source to be scheduled
 **
 ** runLoopsAndModes List of run loop/mode pairs on which the source is scheduled
 **
 ** runLoop Run loop on which the source is being scheduled
 **
 ** runLoopMode Run loop mode on which the source is being scheduled
 */
RSExport void _RSStreamSourceScheduleWithRunLoop(RSRunLoopSourceRef source, RSMutableArrayRef runLoopsAndModes, RSRunLoopRef runLoop, RSStringRef runLoopMode);


/*
 ** _RSStreamSourceUnscheduleFromRunLoop
 **
 ** Unschedule the given source from the given run loop and mode.  It then will
 ** guarantee that the source remains scheduled on the list of run loop and mode
 ** pairs in the runLoopsAndModes list.  The list is simply a linear list of a
 ** loop reference followed by a mode reference.
 **
 ** source Run loop source to be unscheduled
 **
 ** runLoopsAndModes List of run loop/mode pairs on which the source is scheduled
 **
 ** runLoop Run loop from which the source is being unscheduled
 **
 ** runLoopMode Run loop mode from which the source is being unscheduled
 */
RSExport void _RSStreamSourceUnscheduleFromRunLoop(RSRunLoopSourceRef source, RSMutableArrayRef runLoopsAndModes, RSRunLoopRef runLoop, RSStringRef runLoopMode);


/*
 ** _RSStreamSourceScheduleWithAllRunLoops
 **
 ** Schedules the given run loop source on all the run loops and modes in the list.
 ** The list is simply a linear list of a loop reference followed by a mode reference.
 **
 ** source Run loop source to be unscheduled
 **
 ** runLoopsAndModes List of run loop/mode pairs on which the source is scheduled
 */
RSExport void _RSStreamSourceScheduleWithAllRunLoops(RSRunLoopSourceRef source, RSArrayRef runLoopsAndModes);


/*
 ** _RSStreamSourceUnscheduleFromRunLoop
 **
 ** Unschedule the given source from all the run loops and modes in the list.
 ** The list is simply a linear list of a loop reference followed by a mode
 ** reference.
 **
 ** source Run loop source to be unscheduled
 **
 ** runLoopsAndModes List of run loop/mode pairs on which the source is scheduled
 */
RSExport void _RSStreamSourceUncheduleFromAllRunLoops(RSRunLoopSourceRef source, RSArrayRef runLoopsAndModes);

RSExport RSReadStreamRef _RSReadStreamCreateFromFileDescriptor(RSAllocatorRef alloc, int fd);

RSExport RSWriteStreamRef _RSWriteStreamCreateFromFileDescriptor(RSAllocatorRef alloc, int fd);



#define SECURITY_NONE   (0)
#define SECURITY_SSLv2  (1)
#define SECURITY_SSLv3  (2)
#define SECURITY_SSLv32 (3)
#define SECURITY_TLS    (4)

#if (TARGET_OS_MAC && !(TARGET_OS_EMBEDDED || TARGET_OS_IPHONE)) || (TARGET_OS_EMBEDDED || TARGET_OS_IPHONE)
// This symbol is exported from RSNetwork (see RSSocketStream.i).  Only __MACH__ systems will
// get this symbol from CoreFoundation.
extern const int RSStreamErrorDomainSSL;
#endif

/*
 * Additional SPI for RSNetwork for select side read buffering
 */
RSExport BOOL __RSSocketGetBytesAvailable(RSSocketRef s, RSIndex* ctBytesAvailable);

RSExport RSIndex __RSSocketRead(RSSocketRef s, UInt8* buffer, RSIndex length, int* error);

/*
 * This define can be removed once 6030579 is removed
 */
#define RSNETWORK_6030579	1

RSExport void __RSSocketSetSocketReadBufferAttrs(RSSocketRef s, RSTimeInterval timeout, RSIndex length);

RS_EXTERN_C_END

/*
 * for RS{Read/Write}StreamCopyProperty created from a file.  The
 * result is a RSDataRef containing sizeof(int) bytes in machine byte
 * ordering representing the file descriptor of the underlying open
 * file.  If the underlying file descriptor is not open, the property
 * value will be NULL (as opposed to containing ((int) -1)).
 */
RSExport const RSStringRef _RSStreamPropertyFileNativeHandle RS_AVAILABLE(0_5);


#endif /* RSStreamPrivate_h */
