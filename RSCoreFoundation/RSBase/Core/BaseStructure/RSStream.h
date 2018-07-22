//
//  RSStream.h
//  RSCoreFoundation
//
//  Created by closure on 9/28/15.
//  Copyright © 2015 RetVal. All rights reserved.
//

#ifndef __RSSTREAM__
#define __RSSTREAM__

#include <RSCoreFoundation/RSBase.h>
#include <RSCoreFoundation/RSString.h>
#include <RSCoreFoundation/RSDictionary.h>
#include <RSCoreFoundation/RSURL.h>
#include <RSCoreFoundation/RSRunLoop.h>
#include <RSCoreFoundation/RSSocket.h>
#include <RSCoreFoundation/RSError.h>
#include <dispatch/dispatch.h>

RS_EXTERN_C_BEGIN

typedef RSIndex RSStreamStatus;
enum {
    RSStreamStatusNotOpen = 0,
    RSStreamStatusOpening,  /* open is in-progress */
    RSStreamStatusOpen,
    RSStreamStatusReading,
    RSStreamStatusWriting,
    RSStreamStatusAtEnd,    /* no further bytes can be read/written */
    RSStreamStatusClosed,
    RSStreamStatusError
};

typedef RSOptionFlags RSStreamEventType;
enum {
    RSStreamEventNone = 0,
    RSStreamEventOpenCompleted = 1,
    RSStreamEventHasBytesAvailable = 2,
    RSStreamEventCanAcceptBytes = 4,
    RSStreamEventErrorOccurred = 8,
    RSStreamEventEndEncountered = 16
};

typedef struct {
    RSIndex version;
    void *info;
    void *(*retain)(void *info);
    void (*release)(void *info);
    RSStringRef (*copyDescription)(void *info);
} RSStreamClientContext;

typedef struct __RSReadStream * RSReadStreamRef;
typedef struct __RSWriteStream * RSWriteStreamRef;

typedef void (*RSReadStreamClientCallBack)(RSReadStreamRef stream, RSStreamEventType type, void *clientCallBackInfo);
typedef void (*RSWriteStreamClientCallBack)(RSWriteStreamRef stream, RSStreamEventType type, void *clientCallBackInfo);

RSExport RSTypeID RSReadStreamGetTypeID(void);
RSExport RSTypeID RSWriteStreamGetTypeID(void);

RSExport const RSStringRef RSStreamPropertyDataWritten;

RSExport RSReadStreamRef RSReadStreamCreateWithBytesNoCopy(RSAllocatorRef alloc, const UInt8 *bytes, RSIndex length, RSAllocatorRef bytesDeallocator);
RSExport RSWriteStreamRef RSWriteStreamCreateWithBuffer(RSAllocatorRef alloc, UInt8 *buffer, RSIndex bufferCapacity);

/* New buffers are allocated from bufferAllocator as bytes are written to the stream.  At any point, you can recover the bytes thusfar written by asking for the property RSStreamPropertyDataWritten, above */
RSExport RSWriteStreamRef RSWriteStreamCreateWithAllocatedBuffers(RSAllocatorRef alloc, RSAllocatorRef bufferAllocator);

/* File streams */
RSExport RSReadStreamRef RSReadStreamCreateWithFile(RSAllocatorRef alloc, RSURLRef fileURL);
RSExport RSWriteStreamRef RSWriteStreamCreateWithFile(RSAllocatorRef alloc, RSURLRef fileURL);

RSExport void RSStreamCreateBoundPair(RSAllocatorRef alloc, RSReadStreamRef *readStream, RSWriteStreamRef *writeStream, RSIndex transferBufferSize);

/* Property for file write streams; value should be a RSBoolean.  Set to TRUE to append to a file, rather than to replace its contents */
RSExport const RSStringRef RSStreamPropertyAppendToFile;

RSExport const RSStringRef RSStreamPropertyFileCurrentOffset;   // Value is a RSNumber


/* Socket stream properties */

/* Value will be a RSData containing the native handle */
RSExport const RSStringRef RSStreamPropertySocketNativeHandle;

/* Value will be a RSString, or NULL if unknown */
RSExport const RSStringRef RSStreamPropertySocketRemoteHostName;

/* Value will be a RSNumber, or NULL if unknown */
RSExport const RSStringRef RSStreamPropertySocketRemotePortNumber;

/* Socket streams; the returned streams are paired such that they use the same socket; pass NULL if you want only the read stream or the write stream */
RSExport void RSStreamCreatePairWithSocket(RSAllocatorRef alloc, RSSocketHandle sock, RSReadStreamRef *readStream, RSWriteStreamRef *writeStream);
RSExport void RSStreamCreatePairWithSocketToHost(RSAllocatorRef alloc, RSStringRef host, UInt32 port, RSReadStreamRef *readStream, RSWriteStreamRef *writeStream);
RSExport void RSStreamCreatePairWithPeerSocketSignature(RSAllocatorRef alloc, const RSSocketSignature *signature, RSReadStreamRef *readStream, RSWriteStreamRef *writeStream);

/* Returns the current state of the stream */
RSExport RSStreamStatus RSReadStreamGetStatus(RSReadStreamRef stream);
RSExport RSStreamStatus RSWriteStreamGetStatus(RSWriteStreamRef stream);

/* Returns NULL if no error has occurred; otherwise returns the error. */
RSExport RSErrorRef RSReadStreamCopyError(RSReadStreamRef stream) RS_AVAILABLE(0_5);
RSExport RSErrorRef RSWriteStreamCopyError(RSWriteStreamRef stream) RS_AVAILABLE(0_5);

/* Returns success/failure.  Opening a stream causes it to reserve all the system
 resources it requires.  If the stream can open non-blocking, this will always
 return TRUE; listen to the run loop source to find out when the open completes
 and whether it was successful, or poll using RSRead/WriteStreamGetStatus(), waiting
 for a status of RSStreamStatusOpen or RSStreamStatusError.  */
RSExport BOOL RSReadStreamOpen(RSReadStreamRef stream);
RSExport BOOL RSWriteStreamOpen(RSWriteStreamRef stream);

/* Terminates the flow of bytes; releases any system resources required by the
 stream.  The stream may not fail to close.  You may call RSStreamClose() to
 effectively abort a stream. */
RSExport void RSReadStreamClose(RSReadStreamRef stream);
RSExport void RSWriteStreamClose(RSWriteStreamRef stream);

/* Whether there is data currently available for reading; returns TRUE if it's
 impossible to tell without trying */
RSExport BOOL RSReadStreamHasBytesAvailable(RSReadStreamRef stream);

/* Returns the number of bytes read, or -1 if an error occurs preventing any
 bytes from being read, or 0 if the stream's end was encountered.
 It is an error to try and read from a stream that hasn't been opened first.
 This call will block until at least one byte is available; it will NOT block
 until the entire buffer can be filled.  To avoid blocking, either poll using
 RSReadStreamHasBytesAvailable() or use the run loop and listen for the
 RSStreamCanRead event for notification of data available. */
RSExport RSIndex RSReadStreamRead(RSReadStreamRef stream, UInt8 *buffer, RSIndex bufferLength);

/* Returns a pointer to an internal buffer if possible (setting *numBytesRead
 to the length of the returned buffer), otherwise returns NULL; guaranteed
 to return in O(1).  Bytes returned in the buffer are considered read from
 the stream; if maxBytesToRead is greater than 0, not more than maxBytesToRead
 will be returned.  If maxBytesToRead is less than or equal to zero, as many bytes
 as are readily available will be returned.  The returned buffer is good only
 until the next stream operation called on the stream.  Caller should neither
 change the contents of the returned buffer nor attempt to deallocate the buffer;
 it is still owned by the stream. */
RSExport const UInt8 *RSReadStreamGetBuffer(RSReadStreamRef stream, RSIndex maxBytesToRead, RSIndex *numBytesRead);

/* Whether the stream can currently be written to without blocking;
 returns TRUE if it's impossible to tell without trying */
RSExport BOOL RSWriteStreamCanAcceptBytes(RSWriteStreamRef stream);

/* Returns the number of bytes successfully written, -1 if an error has
 occurred, or 0 if the stream has been filled to capacity (for fixed-length
 streams).  If the stream is not full, this call will block until at least
 one byte is written.  To avoid blocking, either poll via RSWriteStreamCanAcceptBytes
 or use the run loop and listen for the RSStreamCanWrite event. */
RSExport RSIndex RSWriteStreamWrite(RSWriteStreamRef stream, const UInt8 *buffer, RSIndex bufferLength);

/* Particular streams can name properties and assign meanings to them; you
 access these properties through the following calls.  A property is any interesting
 information about the stream other than the data being transmitted itself.
 Examples include the headers from an HTTP transmission, or the expected
 number of bytes, or permission information, etc.  Properties that can be set
 configure the behavior of the stream, and may only be settable at particular times
 (like before the stream has been opened).  See the documentation for particular
 properties to determine their get- and set-ability. */
RSExport RSTypeRef RSReadStreamCopyProperty(RSReadStreamRef stream, RSStringRef propertyName);
RSExport RSTypeRef RSWriteStreamCopyProperty(RSWriteStreamRef stream, RSStringRef propertyName);

/* Returns TRUE if the stream recognizes and accepts the given property-value pair;
 FALSE otherwise. */
RSExport BOOL RSReadStreamSetProperty(RSReadStreamRef stream, RSStringRef propertyName, RSTypeRef propertyValue);
RSExport BOOL RSWriteStreamSetProperty(RSWriteStreamRef stream, RSStringRef propertyName, RSTypeRef propertyValue);

/* Asynchronous processing - If you wish to neither poll nor block, you may register
 a client to hear about interesting events that occur on a stream.  Only one client
 per stream is allowed; registering a new client replaces the previous one.
 
 Once you have set a client, the stream must be scheduled to provide the context in
 which the client will be called.  Streams may be scheduled on a single dispatch queue
 or on one or more run loops.  If scheduled on a run loop, it is the caller's responsibility
 to ensure that at least one of the scheduled run loops is being run.
 
 NOTE: Unlike other CoreFoundation APIs, pasing a NULL clientContext here will remove
 the client.  If you do not care about the client context (i.e. your only concern
 is that your callback be called), you should pass in a valid context where every
 entry is 0 or NULL.
 
 */

RSExport BOOL RSReadStreamSetClient(RSReadStreamRef stream, RSOptionFlags streamEvents, RSReadStreamClientCallBack clientCB, RSStreamClientContext *clientContext);
RSExport BOOL RSWriteStreamSetClient(RSWriteStreamRef stream, RSOptionFlags streamEvents, RSWriteStreamClientCallBack clientCB, RSStreamClientContext *clientContext);

RSExport void RSReadStreamScheduleWithRunLoop(RSReadStreamRef stream, RSRunLoopRef runLoop, RSStringRef runLoopMode);
RSExport void RSWriteStreamScheduleWithRunLoop(RSWriteStreamRef stream, RSRunLoopRef runLoop, RSStringRef runLoopMode);

RSExport void RSReadStreamUnscheduleFromRunLoop(RSReadStreamRef stream, RSRunLoopRef runLoop, RSStringRef runLoopMode);
RSExport void RSWriteStreamUnscheduleFromRunLoop(RSWriteStreamRef stream, RSRunLoopRef runLoop, RSStringRef runLoopMode);

/*
 * Specify the dispatch queue upon which the client callbacks will be invoked.
 * Passing NULL for the queue will prevent future callbacks from being invoked.
 * Specifying a dispatch queue using this API will unschedule the stream from
 * any run loops it had previously been scheduled upon - similarly, scheduling
 * with a runloop will disassociate the stream from any existing dispatch queue.
 */
RSExport void RSReadStreamSetDispatchQueue(RSReadStreamRef stream, dispatch_queue_t q) RS_AVAILABLE(0_5);

RSExport void RSWriteStreamSetDispatchQueue(RSWriteStreamRef stream, dispatch_queue_t q) RS_AVAILABLE(0_5);

/*
 * Returns the previously set dispatch queue with an incremented retain count.
 * Note that the stream's queue may have been set to NULL if the stream was
 * scheduled on a runloop subsequent to it having had a dispatch queue set.
 */
RSExport dispatch_queue_t RSReadStreamCopyDispatchQueue(RSReadStreamRef stream) RS_AVAILABLE(0_5);

RSExport dispatch_queue_t RSWriteStreamCopyDispatchQueue(RSWriteStreamRef stream) RS_AVAILABLE(0_5);

/* The following API is deprecated starting in 10.5; please use RSRead/WriteStreamCopyError(), above, instead */
typedef RSIndex RSStreamErrorDomain;
enum {
    RSStreamErrorDomainCustom = -1L,      /* custom to the kind of stream in question */
    RSStreamErrorDomainPOSIX = 1,        /* POSIX errno; interpret using <sys/errno.h> */
    RSStreamErrorDomainMacOSStatus      /* OSStatus type from Carbon APIs; interpret using <MacTypes.h> */
};

typedef struct {
    RSIndex domain;
    RSIndex error;
} RSStreamError;
RSExport RSStreamError RSReadStreamGetError(RSReadStreamRef stream);
RSExport RSStreamError RSWriteStreamGetError(RSWriteStreamRef stream);

RS_EXTERN_C_END
#endif
