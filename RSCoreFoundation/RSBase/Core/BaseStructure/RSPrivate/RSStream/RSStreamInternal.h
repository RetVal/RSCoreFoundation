//
//  RSStreamInternal.h
//  RSCoreFoundation
//
//  Created by closure on 9/28/15.
//  Copyright © 2015 RetVal. All rights reserved.
//

#ifndef RSStreamInternal_h
#define RSStreamInternal_h

#include <RSCoreFoundation/RSStreamAbstract.h>
#include <RSCoreFoundation/RSStreamPrivate.h>
#include <RSCoreFoundation/RSBase.h>
#include <RSCoreFoundation/RSRuntime.h>

RS_EXTERN_C_BEGIN
struct _RSStream {
    RSRuntimeBase _base;
    RSOptionFlags flags;
    RSErrorRef error;
    struct _RSStreamClient *client;
    void *info;
    const struct _RSStreamCallBacks *callBacks;
    RSSpinLock streamLock;
    RSArrayRef previousRunloopsAndModes;
    dispatch_queue_t queue;
};
// Older versions of the callbacks; v0 callbacks match v1 callbacks, except that create, finalize, and copyDescription are missing.
typedef BOOL (*_RSStreamCBOpenV1)(struct _RSStream *stream, RSStreamError *error, BOOL *openComplete, void *info);
typedef BOOL (*_RSStreamCBOpenCompletedV1)(struct _RSStream *stream, RSStreamError *error, void *info);
typedef RSIndex (*_RSStreamCBReadV1)(RSReadStreamRef stream, UInt8 *buffer, RSIndex bufferLength, RSStreamError *error, BOOL *atEOF, void *info);
typedef const UInt8 *(*_RSStreamCBGetBufferV1)(RSReadStreamRef sream, RSIndex maxBytesToRead, RSIndex *numBytesRead, RSStreamError *error, BOOL *atEOF, void *info);
typedef BOOL (*_RSStreamCBCanReadV1)(RSReadStreamRef, void *info);
typedef RSIndex (*_RSStreamCBWriteV1)(RSWriteStreamRef, const UInt8 *buffer, RSIndex bufferLength, RSStreamError *error, void *info);
typedef BOOL (*_RSStreamCBCanWriteV1)(RSWriteStreamRef, void *info);

struct _RSStreamCallBacksV1 {
    RSIndex version;
    void *(*create)(struct _RSStream *stream, void *info);
    void (*finalize)(struct _RSStream *stream, void *info);
    RSStringRef (*copyDescription)(struct _RSStream *stream, void *info);
    
    _RSStreamCBOpenV1 open;
    _RSStreamCBOpenCompletedV1 openCompleted;
    _RSStreamCBReadV1 read;
    _RSStreamCBGetBufferV1 getBuffer;
    _RSStreamCBCanReadV1 canRead;
    _RSStreamCBWriteV1 write;
    _RSStreamCBCanWriteV1 canWrite;
    void (*close)(struct _RSStream *stream, void *info);
    
    RSTypeRef (*copyProperty)(struct _RSStream *stream, RSStringRef propertyName, void *info);
    BOOL (*setProperty)(struct _RSStream *stream, RSStringRef propertyName, RSTypeRef propertyValue, void *info);
    void (*requestEvents)(struct _RSStream *stream, RSOptionFlags events, void *info);
    void (*schedule)(struct _RSStream *stream, RSRunLoopRef runLoop, RSStringRef runLoopMode, void *info);
    void (*unschedule)(struct _RSStream *stream, RSRunLoopRef runLoop, RSStringRef runLoopMode, void *info);
};

// These two are defined in RSSocketStream.c because that's where the glue for RSNetwork is.
RSPrivate RSErrorRef _RSErrorFromStreamError(RSAllocatorRef alloc, RSStreamError *err);
RSPrivate RSStreamError _RSStreamErrorFromError(RSErrorRef error);


RS_EXTERN_C_END

#endif /* RSStreamInternal_h */
