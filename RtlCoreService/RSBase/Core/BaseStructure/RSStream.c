//
//  RSStream.c
//  RSCoreFoundation
//
//  Created by closure on 9/28/15.
//  Copyright © 2015 RetVal. All rights reserved.
//

#include <RSCoreFoundation/RSStream.h>
#include <RSCoreFoundation/RSRuntime.h>

#include <string.h>
#include "RSStreamInternal.h"
#include "RSInternal.h"

enum {
    MIN_STATUS_CODE_BIT	= 0,
    // ..status bits...
    MAX_STATUS_CODE_BIT	= 4,
    
    CONSTANT_CALLBACKS	= 5,
    CALLING_CLIENT		= 6,	// MUST remain 6 since it's value is used elsewhere.
    
    HAVE_CLOSED			= 7,
    
    // Values above used to be defined and others may rely on their values
    
    // Values below should not matter if they are re-ordered or shift
    
    SHARED_SOURCE
};

static RSSpinLock sSourceLock = RSSpinLockInit;
static RSMutableDictionaryRef sSharedSources = nil;

static RSTypeID __RSReadStreamTypeID = _RSRuntimeNotATypeID;
static RSTypeID __RSWriteStreamTypeID = _RSRuntimeNotATypeID;

RSInline RSBitU8 __RSStreamGetStatus(struct _RSStream *stream) {
    return __RSBitfieldGetValue(stream->flags, MAX_STATUS_CODE_BIT, MIN_STATUS_CODE_BIT);
}

RSPrivate RSStreamStatus _RSStreamGetStatus(struct _RSStream *stream);
static BOOL _RSStreamRemoveRunLoopAndModeFromArray(RSMutableArrayRef runLoopsAndModes, RSRunLoopRef rl, RSStringRef mode);
static void _wakeUpRunLoop(struct _RSStream *stream);

RSInline void checkRLMArray(RSArrayRef arr) {
#ifdef DEBUG
    assert(arr == nil || (RSArrayGetCount(arr) % 2) == 0);
#endif
}

RSInline void _RSStreamLock(struct _RSStream *stream) {
    RSSpinLockLock(&stream->streamLock);
}

RSInline void _RSStreamUnlock(struct _RSStream *stream) {
    RSSpinLockUnlock(&stream->streamLock);
}

RSInline RSRunLoopSourceRef _RSStreamCopySource(struct _RSStream *stream) {
    RSRunLoopSourceRef source = nil;
    if (stream) {
        _RSStreamLock(stream);
        if (stream->client) {
            source = stream->client->rlSource;
        }
        if (source) {
            RSRetain(source);
        }
        _RSStreamUnlock(stream);
    }
    return source;
}

RSInline void _RSStreamSetSource(struct _RSStream *stream, RSRunLoopSourceRef source, BOOL invalidateOldSource) {
    RSRunLoopSourceRef oldSource = nil;
    if (stream) {
        _RSStreamLock(stream);
        if (stream->client) {
            oldSource = stream->client->rlSource;
            if (oldSource != nil) {
                RSRetain(oldSource);
            }
            stream->client->rlSource = source;
            if (source != nil) {
                RSRetain(source);
            }
        }
        _RSStreamUnlock(stream);
    }
    
    if (oldSource) {
        RSRelease(oldSource);
        if (invalidateOldSource) {
            RSRunLoopSourceInvalidate(oldSource);
        }
        RSRelease(oldSource);
    }
}

RSInline const struct _RSStreamCallBacks *_RSStreamGetCallBackPtr(struct _RSStream *stream) {
    return stream->callBacks;
}

RSInline void _RSStreamSetStatusCode(struct _RSStream *stream, RSStreamStatus newStatus) {
    RSStreamStatus status = __RSStreamGetStatus(stream);
    if (((status != RSStreamStatusClosed) && (status != RSStreamStatusError)) ||
        ((status == RSStreamStatusClosed) && (newStatus == RSStreamStatusError))) {
        __RSBitfieldSetValue(stream->flags, MAX_STATUS_CODE_BIT, MIN_STATUS_CODE_BIT, newStatus);
    }
}

RSInline void _RSStreamScheduleEvent(struct _RSStream *stream, RSStreamEventType event) {
    if (stream->client && (stream->client->when & event)) {
        RSRunLoopSourceRef source = _RSStreamCopySource(stream);
        if (source) {
            stream->client->whatToSignal |= event;
            RSRunLoopSourceSignal(source);
            RSRelease(source);
            _wakeUpRunLoop(stream);
        }
    }
}

RSInline void _RSStreamSetStreamError(struct _RSStream *stream, RSStreamError *err) {
    if (!stream->error) {
        stream->error = (RSErrorRef)RSAllocatorAllocate(RSGetAllocator(stream), sizeof(RSStreamError));
    }
    memmove(stream->error, err, sizeof(RSStreamError));
}

static RSStringRef __RSStreamDescription(RSTypeRef rs) {
    struct _RSStream *stream = (struct _RSStream *)rs;
    const struct _RSStreamCallBacks *cb = _RSStreamGetCallBackPtr(stream);
    RSStringRef contextDescription;
    RSStringRef desc;
    if (cb->copyDescription) {
        if (cb->version == 0) {
            contextDescription = ((RSStringRef(*)(void *))cb->copyDescription)(_RSStreamGetInfoPointer(stream));
        } else {
            contextDescription = cb->copyDescription(stream, _RSStreamGetInfoPointer(stream));
        }
    } else {
        contextDescription = RSStringCreateWithFormat(RSGetAllocator(stream), RSSTR("info = %p"), _RSStreamGetInfoPointer(stream));
    }
    if (RSGetTypeID(rs) == __RSReadStreamTypeID) {
        desc = RSStringCreateWithFormat(RSGetAllocator(stream), RSSTR("<RSReadStream %p>{%R}"), stream, contextDescription);
    } else {
        desc = RSStringCreateWithFormat(RSGetAllocator(stream), RSSTR("<RSWriteStream %p>{%R}"), stream, contextDescription);
    }
    RSRelease(contextDescription);
    return desc;
}

static void _RSStreamDetachSource(struct _RSStream *stream) {
    if (stream && stream->client && stream->client->rlSource) {
        if (!__RSBitIsSet(stream->flags, SHARED_SOURCE)) {
            _RSStreamSetSource(stream, nil, YES);
        } else {
            RSArrayRef runLoopAndSourceKey;
            RSMutableArrayRef list;
            RSIndex count;
            RSIndex i;
            RSSpinLockLock(&sSourceLock);
            
            runLoopAndSourceKey = (RSArrayRef)RSDictionaryGetValue(sSharedSources, stream);
            list = (RSMutableArrayRef)RSDictionaryGetValue(sSharedSources, runLoopAndSourceKey);
            count = RSArrayGetCount(list);
            i = RSArrayIndexOfObject(list, stream);
            if (i != RSNotFound) {
                RSArrayRemoveObjectAtIndex(list, i);
                count--;
            }
            
            assert(RSArrayIndexOfObject(list, stream) == RSNotFound && "RSStreamClose: stream found twice in its shared source's list");
            if (count == 0) {
                RSRunLoopSourceRef source = _RSStreamCopySource(stream);
                if (source) {
                    RSRunLoopRemoveSource((RSRunLoopRef)RSArrayObjectAtIndex(runLoopAndSourceKey, 0), source, RSArrayObjectAtIndex(runLoopAndSourceKey, 1));
                    RSRelease(source);
                }
                RSDictionaryRemoveValue(sSharedSources, runLoopAndSourceKey);
            }
            RSDictionaryRemoveValue(sSharedSources, stream);
            _RSStreamSetSource(stream, nil, count == 0);
            __RSBitClear(stream->flags, SHARED_SOURCE);
            
            RSSpinLockUnlock(&sSourceLock);
        }
    }
}

RSPrivate void _RSStreamClose(struct _RSStream *stream) {
    RSStreamStatus status = _RSStreamGetStatus(stream);
    const struct _RSStreamCallBacks *cb = _RSStreamGetCallBackPtr(stream);
    if (status == RSStreamStatusNotOpen || status == RSStreamStatusClosed || (status == RSStreamStatusError && __RSBitIsSet(stream->flags, HAVE_CLOSED))) {
        return;
    }
    if (!__RSBitIsSet(stream->flags, HAVE_CLOSED)) {
        __RSBitSet(stream->flags, HAVE_CLOSED);
        __RSBitSet(stream->flags, CALLING_CLIENT);
        if (cb->close) {
            cb->close(stream, _RSStreamGetInfoPointer(stream));
        }
        if (stream->client) {
            _RSStreamDetachSource(stream);
        }
        _RSStreamSetStatusCode(stream, RSStreamStatusClosed);
        __RSBitClear(stream->flags, CALLING_CLIENT);
    }
}

static void __RSStreamDeallocate(RSTypeRef rs) {
    struct _RSStream *stream = (struct _RSStream *)rs;
    const struct _RSStreamCallBacks *cb = _RSStreamGetCallBackPtr(stream);
    RSAllocatorRef alloc = RSGetAllocator(stream);
    _RSStreamClose(stream);
    if (stream->client) {
        RSStreamClientContext *cbContext = &(stream->client->cbContext);
        if (cbContext->info && cbContext->release) {
            cbContext->release(cbContext->info);
        }
        if (stream->client->runLoopsAndModes) {
            RSRelease(stream->client->runLoopsAndModes);
        }
        RSAllocatorDeallocate(alloc, stream->client);
        stream->client = nil;
    }
    if (cb->finalize) {
        if (cb->version == 0) {
            ((void (*)(void *))cb->finalize)(_RSStreamGetInfoPointer(stream));
        } else {
            cb->finalize(stream, _RSStreamGetInfoPointer(stream));
        }
    }
    if (stream->error) {
        if (cb->version < 2) {
            RSAllocatorDeallocate(alloc, stream->error);
        } else {
            RSRelease(stream->error);
        }
    }
    if (!__RSBitIsSet(stream->flags, CONSTANT_CALLBACKS)) {
        RSAllocatorDeallocate(alloc, (void *)stream->callBacks);
    }
    if (stream->previousRunloopsAndModes) {
        RSRelease(stream->previousRunloopsAndModes);
        stream->previousRunloopsAndModes = nil;
    }
    if (stream->queue) {
        dispatch_release(stream->queue);
        stream->queue = nil;
    }
}

static const RSRuntimeClass __RSReadStreamClass = {
    _RSRuntimeScannedObject,
    0,
    "RSReadStream",
    nil,
    nil,
    __RSStreamDeallocate,
    nil,
    nil,
    __RSStreamDescription,
    nil,
};

static const RSRuntimeClass __RSWriteStreamClass = {
    _RSRuntimeScannedObject,
    0,
    "RSWriteStream",
    nil,
    nil,
    __RSStreamDeallocate,
    nil,
    nil,
    __RSStreamDescription,
    nil,
};

RS_PUBLIC_CONST_STRING_DECL(RSStreamPropertySocketNativeHandle, "RSStreamPropertySocketNativeHandle")
RS_PUBLIC_CONST_STRING_DECL(RSStreamPropertySocketRemoteHostName, "RSStreamPropertySocketRemoteHostName")
RS_PUBLIC_CONST_STRING_DECL(RSStreamPropertySocketRemotePortNumber, "RSStreamPropertySocketRemotePortNumber")
RS_PUBLIC_CONST_STRING_DECL(RSStreamPropertyDataWritten, "RSStreamPropertyDataWritten")
RS_PUBLIC_CONST_STRING_DECL(RSStreamPropertyAppendToFile, "RSStreamPropertyAppendToFile")

RSPrivate void __RSStreamInitialize() {
    __RSReadStreamTypeID = __RSRuntimeRegisterClass(&__RSReadStreamClass);
    __RSWriteStreamTypeID = __RSRuntimeRegisterClass(&__RSWriteStreamClass);
}

RSExport RSTypeID RSReadStreamGetTypeID() {
    return __RSReadStreamTypeID;
}

RSExport RSTypeID RSWriteStreamGetTypeID() {
    return __RSWriteStreamTypeID;
}

static struct _RSStream *_RSStreamCreate(RSAllocatorRef allocator, BOOL isReadStream) {
    struct _RSStream *newStream = (struct _RSStream *)__RSRuntimeCreateInstance(allocator, isReadStream ? __RSReadStreamTypeID : __RSWriteStreamTypeID, sizeof(struct _RSStream) - sizeof(RSRuntimeBase));
    if (newStream) {
        //        numStreamInstances ++;
        newStream->flags = 0;
        _RSStreamSetStatusCode(newStream, RSStreamStatusNotOpen);
        newStream->error = nil;
        newStream->client = nil;
        newStream->info = nil;
        newStream->callBacks = nil;
        
        newStream->streamLock = RSSpinLockInit;
        newStream->previousRunloopsAndModes = nil;
        newStream->queue = nil;
    }

    return newStream;
}

RSExport void *_RSStreamGetInfoPointer(struct _RSStream *stream) {
    return stream == nil ? nil : stream->info;
}

RSPrivate struct _RSStream *_RSStreamCreateWithConstantCallbacks(RSAllocatorRef allocator, void *info, const struct _RSStreamCallBacks *cb, BOOL isReading) {
    if (cb->version != 1) return nil;
    struct _RSStream *newStream = _RSStreamCreate(allocator, isReading);
    if (newStream) {
        __RSBitSet(newStream->flags, CONSTANT_CALLBACKS);
        newStream->callBacks = cb;
        if (cb->create) {
            newStream->info = cb->create(newStream, info);
        } else {
            newStream->info = info;
        }
    }
    return newStream;
}

RSExport void _RSStreamSetInfoPointer(struct _RSStream *stream, void *info, const struct _RSStreamCallBacks *cb) {
    if (info != stream->info) {
        if (stream->callBacks->finalize) {
            stream->callBacks->finalize(stream, stream->info);
        }
        if (cb->create) {
            stream->info = cb->create(stream, info);
        } else {
            stream->info = info;
        }
    }
    stream->callBacks = cb;
}

RSExport RSReadStreamRef RSReadStreamCreate(RSAllocatorRef allocator, const RSReadStreamCallBacks *callbacks, void *info) {
    struct _RSStream *newStream = _RSStreamCreate(allocator, YES);
    if (!newStream) return nil;
    struct _RSStreamCallBacks *cb = (struct _RSStreamCallBacks *)RSAllocatorAllocate(allocator, sizeof(struct _RSStreamCallBacks));
    if (!cb) {
        RSRelease(newStream);
        return nil;
    }
    if (callbacks->version == 0) {
        RSReadStreamCallBacksV0 *cbV0 = (RSReadStreamCallBacksV0 *)callbacks;
        RSStreamClientContext *ctxt = (RSStreamClientContext *)info;
        newStream->info = ctxt->release ? (void *)ctxt->retain(ctxt->info) : ctxt->info;
        newStream->info = ctxt->retain ? (void *)ctxt->retain(ctxt->info) : ctxt->info;
        cb->version = 0;
        cb->create = (void *(*)(struct _RSStream *, void *))ctxt->retain;
        cb->finalize = (void(*)(struct _RSStream *, void *))ctxt->release;
        cb->copyDescription = (RSStringRef(*)(struct _RSStream *, void *))ctxt->copyDescription;
        cb->open = (BOOL(*)(struct _RSStream *, RSErrorRef *, BOOL *, void *))cbV0->open;
        cb->openCompleted = (BOOL (*)(struct _RSStream *, RSErrorRef *, void *))cbV0->openCompleted;
        cb->read = (RSIndex (*)(RSReadStreamRef, UInt8 *, RSIndex, RSErrorRef *, BOOL *, void *))cbV0->read;
        cb->getBuffer = (const UInt8 *(*)(RSReadStreamRef, RSIndex, RSIndex *, RSErrorRef *, BOOL *, void *))cbV0->getBuffer;
        cb->canRead = (BOOL (*)(RSReadStreamRef, RSErrorRef*, void*))cbV0->canRead;
        cb->write = nil;
        cb->canWrite = nil;
        cb->close = (void (*)(struct _RSStream *, void *))cbV0->close;
        cb->copyProperty = (RSTypeRef (*)(struct _RSStream *, RSStringRef, void *))cbV0->copyProperty;
        cb->setProperty = nil;
        cb->requestEvents = nil;
        cb->schedule = (void (*)(struct _RSStream *, RSRunLoopRef, RSStringRef, void *))cbV0->schedule;
        cb->unschedule = (void (*)(struct _RSStream *, RSRunLoopRef, RSStringRef, void *))cbV0->unschedule;
    }else if (callbacks->version == 1) {
        RSReadStreamCallBacksV1 *cbV1 = (RSReadStreamCallBacksV1 *)callbacks;
        newStream->info = cbV1->create ? cbV1->create((RSReadStreamRef)newStream, info) : info;
        cb->version = 1;
        cb->create = (void *(*)(struct _RSStream *, void *))cbV1->create;
        cb->finalize = (void(*)(struct _RSStream *, void *))cbV1->finalize;
        cb->copyDescription = (RSStringRef(*)(struct _RSStream *, void *))cbV1->copyDescription;
        cb->open = (BOOL(*)(struct _RSStream *, RSErrorRef *, BOOL *, void *))cbV1->open;
        cb->openCompleted = (BOOL(*)(struct _RSStream *, RSErrorRef *, void *))cbV1->openCompleted;
        cb->read = (RSIndex (*)(RSReadStreamRef, UInt8 *, RSIndex, RSErrorRef *, BOOL *, void *))cbV1->read;
        cb->getBuffer = (const UInt8 *(*)(RSReadStreamRef, RSIndex, RSIndex *, RSErrorRef *, BOOL *, void *))cbV1->getBuffer;
        cb->canRead = (BOOL (*)(RSReadStreamRef, RSErrorRef*, void*))cbV1->canRead;
        cb->write = nil;
        cb->canWrite = nil;
        cb->close = (void (*)(struct _RSStream *, void *))cbV1->close;
        cb->copyProperty = (RSTypeRef (*)(struct _RSStream *, RSStringRef, void *))cbV1->copyProperty;
        cb->setProperty = (BOOL(*)(struct _RSStream *, RSStringRef, RSTypeRef, void *))cbV1->setProperty;
        cb->requestEvents = (void(*)(struct _RSStream *, RSOptionFlags, void *))cbV1->requestEvents;
        cb->schedule = (void (*)(struct _RSStream *, RSRunLoopRef, RSStringRef, void *))cbV1->schedule;
        cb->unschedule = (void (*)(struct _RSStream *, RSRunLoopRef, RSStringRef, void *))cbV1->unschedule;
    } else {
        newStream->info = callbacks->create ? callbacks->create((RSReadStreamRef)newStream, info) : info;
        cb->version = 2;
        cb->create = (void *(*)(struct _RSStream *, void *))callbacks->create;
        cb->finalize = (void(*)(struct _RSStream *, void *))callbacks->finalize;
        cb->copyDescription = (RSStringRef(*)(struct _RSStream *, void *))callbacks->copyDescription;
        cb->open = (BOOL(*)(struct _RSStream *, RSErrorRef *, BOOL *, void *))callbacks->open;
        cb->openCompleted = (BOOL (*)(struct _RSStream *, RSErrorRef *, void *))callbacks->openCompleted;
        cb->read = callbacks->read;
        cb->getBuffer = callbacks->getBuffer;
        cb->canRead = callbacks->canRead;
        cb->write = nil;
        cb->canWrite = nil;
        cb->close = (void (*)(struct _RSStream *, void *))callbacks->close;
        cb->copyProperty = (RSTypeRef (*)(struct _RSStream *, RSStringRef, void *))callbacks->copyProperty;
        cb->setProperty = (BOOL(*)(struct _RSStream *, RSStringRef, RSTypeRef, void *))callbacks->setProperty;
        cb->requestEvents = (void(*)(struct _RSStream *, RSOptionFlags, void *))callbacks->requestEvents;
        cb->schedule = (void (*)(struct _RSStream *, RSRunLoopRef, RSStringRef, void *))callbacks->schedule;
        cb->unschedule = (void (*)(struct _RSStream *, RSRunLoopRef, RSStringRef, void *))callbacks->unschedule;
    }
    
    newStream->callBacks = cb;
    return (RSReadStreamRef)newStream;
}


RSExport RSWriteStreamRef RSWriteStreamCreate(RSAllocatorRef alloc, const RSWriteStreamCallBacks *callbacks, void *info) {
    struct _RSStream *newStream = _RSStreamCreate(alloc, FALSE);
    struct _RSStreamCallBacks *cb;
    if (!newStream) return nil;
    cb = (struct _RSStreamCallBacks *)RSAllocatorAllocate(alloc, sizeof(struct _RSStreamCallBacks));
    if (!cb) {
        RSRelease(newStream);
        return nil;
    }
    if (callbacks->version == 0) {
        RSWriteStreamCallBacksV0 *cbV0 = (RSWriteStreamCallBacksV0 *)callbacks;
        RSStreamClientContext *ctxt = (RSStreamClientContext *)info;
        newStream->info = ctxt->retain ? (void *)ctxt->retain(ctxt->info) : ctxt->info;
        cb->version = 0;
        cb->create = (void *(*)(struct _RSStream *, void *))ctxt->retain;
        cb->finalize = (void(*)(struct _RSStream *, void *))ctxt->release;
        cb->copyDescription = (RSStringRef(*)(struct _RSStream *, void *))ctxt->copyDescription;
        cb->open = (BOOL(*)(struct _RSStream *, RSErrorRef *, BOOL *, void *))cbV0->open;
        cb->openCompleted = (BOOL (*)(struct _RSStream *, RSErrorRef *, void *))cbV0->openCompleted;
        cb->read = nil;
        cb->getBuffer = nil;
        cb->canRead = nil;
        cb->write = (RSIndex(*)(RSWriteStreamRef stream, const UInt8 *buffer, RSIndex bufferLength, RSErrorRef *error, void *info))cbV0->write;
        cb->canWrite = (BOOL(*)(RSWriteStreamRef stream, RSErrorRef *error, void *info))cbV0->canWrite;
        cb->close = (void (*)(struct _RSStream *, void *))cbV0->close;
        cb->copyProperty = (RSTypeRef (*)(struct _RSStream *, RSStringRef, void *))cbV0->copyProperty;
        cb->setProperty = nil;
        cb->requestEvents = nil;
        cb->schedule = (void (*)(struct _RSStream *, RSRunLoopRef, RSStringRef, void *))cbV0->schedule;
        cb->unschedule = (void (*)(struct _RSStream *, RSRunLoopRef, RSStringRef, void *))cbV0->unschedule;
    } else if (callbacks->version == 1) {
        RSWriteStreamCallBacksV1 *cbV1 = (RSWriteStreamCallBacksV1 *)callbacks;
        cb->version = 1;
        newStream->info = cbV1->create ? cbV1->create((RSWriteStreamRef)newStream, info) : info;
        cb->create = (void *(*)(struct _RSStream *, void *))cbV1->create;
        cb->finalize = (void(*)(struct _RSStream *, void *))cbV1->finalize;
        cb->copyDescription = (RSStringRef(*)(struct _RSStream *, void *))cbV1->copyDescription;
        cb->open = (BOOL(*)(struct _RSStream *, RSErrorRef *, BOOL *, void *))cbV1->open;
        cb->openCompleted = (BOOL (*)(struct _RSStream *, RSErrorRef *, void *))cbV1->openCompleted;
        cb->read = nil;
        cb->getBuffer = nil;
        cb->canRead = nil;
        cb->write = (RSIndex(*)(RSWriteStreamRef stream, const UInt8 *buffer, RSIndex bufferLength, RSErrorRef *error, void *info))cbV1->write;
        cb->canWrite = (BOOL(*)(RSWriteStreamRef stream, RSErrorRef *error, void *info))cbV1->canWrite;
        cb->close = (void (*)(struct _RSStream *, void *))cbV1->close;
        cb->copyProperty = (RSTypeRef (*)(struct _RSStream *, RSStringRef, void *))cbV1->copyProperty;
        cb->setProperty = (BOOL (*)(struct _RSStream *, RSStringRef, RSTypeRef, void *))cbV1->setProperty;
        cb->requestEvents = (void(*)(struct _RSStream *, RSOptionFlags, void *))cbV1->requestEvents;
        cb->schedule = (void (*)(struct _RSStream *, RSRunLoopRef, RSStringRef, void *))cbV1->schedule;
        cb->unschedule = (void (*)(struct _RSStream *, RSRunLoopRef, RSStringRef, void *))cbV1->unschedule;
    } else {
        cb->version = callbacks->version;
        newStream->info = callbacks->create ? callbacks->create((RSWriteStreamRef)newStream, info) : info;
        cb->create = (void *(*)(struct _RSStream *, void *))callbacks->create;
        cb->finalize = (void(*)(struct _RSStream *, void *))callbacks->finalize;
        cb->copyDescription = (RSStringRef(*)(struct _RSStream *, void *))callbacks->copyDescription;
        cb->open = (BOOL(*)(struct _RSStream *, RSErrorRef *, BOOL *, void *))callbacks->open;
        cb->openCompleted = (BOOL (*)(struct _RSStream *, RSErrorRef *, void *))callbacks->openCompleted;
        cb->read = nil;
        cb->getBuffer = nil;
        cb->canRead = nil;
        cb->write = callbacks->write;
        cb->canWrite = callbacks->canWrite;
        cb->close = (void (*)(struct _RSStream *, void *))callbacks->close;
        cb->copyProperty = (RSTypeRef (*)(struct _RSStream *, RSStringRef, void *))callbacks->copyProperty;
        cb->setProperty = (BOOL (*)(struct _RSStream *, RSStringRef, RSTypeRef, void *))callbacks->setProperty;
        cb->requestEvents = (void(*)(struct _RSStream *, RSOptionFlags, void *))callbacks->requestEvents;
        cb->schedule = (void (*)(struct _RSStream *, RSRunLoopRef, RSStringRef, void *))callbacks->schedule;
        cb->unschedule = (void (*)(struct _RSStream *, RSRunLoopRef, RSStringRef, void *))callbacks->unschedule;
    }
    newStream->callBacks = cb;
    return (RSWriteStreamRef)newStream;
}

static void _signalEventSync(struct _RSStream *stream, RSOptionFlags whatToSignal) {
    RSOptionFlags eventMask;
    __RSBitSet(stream->flags, CALLING_CLIENT);
    _RSStreamLock(stream);
    struct _RSStreamClient *client = stream->client;
    if (client == nil) {
        _RSStreamUnlock(stream);
    } else {
        void *info = nil;
        void (*release)(void *) = nil;
        void (*cb)(struct _RSStream *, RSStreamEventType, void *) = client == nil ? nil : client->cb;
        if (stream->client->cbContext.retain == nil) {
            info = stream->client->cbContext.info;
        } else {
            info = stream->client->cbContext.retain(stream->client->cbContext.info);
            release = stream->client->cbContext.release;
        }
    
        _RSStreamUnlock(stream);
        for (eventMask = 1; eventMask <= whatToSignal; eventMask = eventMask << 1) {
            _RSStreamLock(stream);
            BOOL shouldSingal = ((eventMask & whatToSignal) && stream->client && (stream->client->when & eventMask));
            _RSStreamUnlock(stream);
            if (shouldSingal && client) {
                cb(stream, eventMask, info);
            }
        }
        if (release) {
            (*release)(info);
        }
    }
    __RSBitClear(stream->flags, CALLING_CLIENT);
}

static void _signalEventQueue(dispatch_queue_t q, struct _RSStream *stream, RSOptionFlags whatToSignal) {
    RSRetain(stream);
    dispatch_async(q, ^{
        _signalEventSync(stream, whatToSignal);
        RSRelease(stream);
    });
}

static void _rsstream_solo_signalEventSync(void *info) {
    RSTypeID typeID = RSGetTypeID((RSTypeRef)info);
    if (typeID != RSReadStreamGetTypeID() && typeID != RSWriteStreamGetTypeID()) {
        RSLog(RSSTR("Excepted a read or write stream for %p"), info);
#if defined(DEBUG)
        abort();
#endif
    } else {
        struct _RSStream *stream = (struct _RSStream *)info;
        _RSStreamLock(stream);
        RSOptionFlags whatToSignal = stream->client->whatToSignal;
        stream->client->whatToSignal = 0;
        dispatch_queue_t queue = stream->queue;
        if (queue) dispatch_retain(queue);
        RSRetain(stream);
        _RSStreamUnlock(stream);
        if (queue == 0) {
            _signalEventSync(stream, whatToSignal);
        } else {
            _signalEventQueue(queue, stream, whatToSignal);
            dispatch_release(queue);
        }
        RSRelease(stream);
    }
}

static void _rsstream_shared_signalEventSync(void *info) {
    RSTypeID typeID = RSGetTypeID((RSTypeRef)info);
    if (typeID != RSArrayGetTypeID()) {
        RSLog(RSSTR("Excepted an array for %p"), info);
#if defined(DEBUG)
        abort();
#endif
    } else {
        RSMutableArrayRef list = (RSMutableArrayRef)info;
        RSIndex c, i;
        RSOptionFlags whatToSignal = 0;
        dispatch_queue_t queue = 0;
        struct _RSStream *stream = nil;
        RSSpinLockLock(&sSourceLock);
        c = RSArrayGetCount(list);
        for (i = 0; i < c; ++i) {
            struct _RSStream *s = (struct _RSStream *)RSArrayObjectAtIndex(list, i);
            if (s->client->whatToSignal) {
                stream = s;
                RSRetain(stream);
                whatToSignal = stream->client->whatToSignal;
                s->client->whatToSignal = 0;
                queue = stream->queue;
                if (queue) dispatch_retain(queue);
                break;
            }
        }
        
        for (; i < c; ++i) {
            struct _RSStream *s = (struct _RSStream *)RSArrayObjectAtIndex(list, i);
            if (s->client->whatToSignal) {
                RSRunLoopSourceRef source = _RSStreamCopySource(stream);
                if (source) {
                    RSRunLoopSourceSignal(source);
                    RSRelease(source);
                }
                break;
            }
        }
        RSSpinLockUnlock(&sSourceLock);
        if (stream) {
            if (queue == 0) {
                _signalEventSync(stream, whatToSignal);
            } else {
                _signalEventQueue(queue, stream, whatToSignal);
            }
            RSRelease(stream);
        }
    }
}

static RSArrayRef _RSStreamGetRunLoopsAndModes(struct _RSStream *stream) {
    RSArrayRef result = nil;
    if (stream && stream->client) {
        _RSStreamLock(stream);
        if (stream->previousRunloopsAndModes) {
            RSRelease(stream->previousRunloopsAndModes);
            stream->previousRunloopsAndModes = nil;
        }
        if (stream->client->runLoopsAndModes) {
            stream->previousRunloopsAndModes = RSArrayCreateCopy(RSGetAllocator(stream), stream->client->runLoopsAndModes);
        }
        result = stream->previousRunloopsAndModes;
        checkRLMArray(result);
        _RSStreamUnlock(stream);
    }
    return result;
}

static RSArrayRef _RSStreamCopyRunLoopsAndModes(struct _RSStream *stream) {
    RSArrayRef result = nil;
    if (stream && stream->client) {
        _RSStreamLock(stream);
        if (stream->client->runLoopsAndModes) {
            result = RSArrayCreateCopy(RSGetAllocator(stream), stream->client->runLoopsAndModes);
        }
        checkRLMArray(result);
        _RSStreamUnlock(stream);
    }
    return result;
}

static void _wakeUpRunLoop(struct _RSStream *stream) {
    RSArrayRef rlArray = _RSStreamCopyRunLoopsAndModes(stream);
    if (rlArray) {
        RSIndex cnt = RSArrayGetCount(rlArray);
        RSRunLoopRef rl = nil;
        if (cnt == 2) {
            rl = (RSRunLoopRef)RSArrayObjectAtIndex(rlArray, 0);
        } else if (cnt > 2) {
            RSIndex idx;
            rl = (RSRunLoopRef)RSArrayObjectAtIndex(rlArray, 0);
            for (idx = 2; nil != rlArray && idx < cnt; idx += 2) {
                RSRunLoopRef value = (RSRunLoopRef)RSArrayObjectAtIndex(rlArray, idx);
                if (value != rl) rl = nil;
            }
            if (nil == rl) {
                for (idx = 0; idx < cnt; idx += 2) {
                    RSRunLoopRef value = (RSRunLoopRef)RSArrayObjectAtIndex(rlArray, idx);
                    RSStringRef currentMode = RSRunLoopCopyCurrentMode(value);
                    if (nil != currentMode && RSEqual(currentMode, RSArrayObjectAtIndex(rlArray, idx + 1) && RSRunLoopIsWaiting(value))) {
                        RSRelease(currentMode);
                        rl = value;
                        break;
                    }
                    if (nil != currentMode) {
                        RSRelease(currentMode);
                    }
                }
                if (nil == rl) {
                    rl = (RSRunLoopRef)RSArrayObjectAtIndex(rlArray, 0);
                }
            }
        }
        if (nil != rl && RSRunLoopIsWaiting(rl)) {
            RSRunLoopWakeUp(rl);
        }
        RSRelease(rlArray);
    }
}

RSPrivate void _RSStreamSignalEvent(struct _RSStream *stream, RSStreamEventType event, RSErrorRef error, BOOL synchronousAllowed) {
    RSStreamStatus status = __RSStreamGetStatus(stream);
    if (status == RSStreamStatusNotOpen) {
        RSLog(RSSTR("Stream %p is sending an event before being opened"), stream);
        event = 0;
    } else if (status == RSStreamStatusClosed || status == RSStreamStatusError) {
        event = 0;
    } else if (status == RSStreamStatusAtEnd) {
        event &= RSStreamEventErrorOccurred;
    } else if (status != RSStreamStatusOpening) {
        event &= ~RSStreamEventOpenCompleted;
    }
    
    if (event & RSStreamEventOpenCompleted && status == RSStreamStatusOpening) {
        _RSStreamSetStatusCode(stream, RSStreamStatusOpen);
    }
    if (event & RSStreamEventEndEncountered && status < RSStreamStatusAtEnd) {
        _RSStreamSetStatusCode(stream, RSStreamStatusAtEnd);
    }
    if (event & RSStreamEventErrorOccurred) {
        if (_RSStreamGetCallBackPtr(stream)->version < 2) {
            _RSStreamSetStreamError(stream, (RSStreamError *)error);
        } else {
            assert(error && "RSStream: RSSTreamEventErrorOccurred signalled, but error is nil!");
            RSRetain(error);
            if (stream->error) {
                RSRelease(stream->error);
            }
            stream->error = error;
        }
        _RSStreamSetStatusCode(stream, RSStreamStatusError);
    }
    if (stream->client && (stream->client->when & event) != 0) {
        RSRunLoopSourceRef source = _RSStreamCopySource(stream);
        if (source) {
            BOOL signalNow = NO;
            stream->client->whatToSignal |= event;
            if (synchronousAllowed && !__RSBitIsSet(stream->flags, CALLING_CLIENT)) {
                RSRunLoopRef rl = RSRunLoopGetCurrent();
                RSStringRef mode = RSRunLoopCopyCurrentMode(rl);
                if (mode) {
                    if (RSRunLoopContainsSource(rl, source, mode)) {
                        signalNow = YES;
                    }
                }
                if (mode) {
                    RSRelease(mode);
                }
            }
            if (signalNow) {
                _rsstream_solo_signalEventSync(stream);
            } else {
                if (source) {
                    RSRunLoopSourceSignal(source);
                }
                _wakeUpRunLoop(stream);
            }
            RSRelease(source);
        }
    }
}

RSPrivate RSStreamStatus _RSStreamGetStatus(struct _RSStream *stream) {
    RSStreamStatus status = __RSStreamGetStatus(stream);
    __RSBitSet(stream->flags, CALLING_CLIENT);
    if (status == RSStreamStatusOpening) {
        const struct _RSStreamCallBacks *cb = _RSStreamGetCallBackPtr(stream);
        if (cb->openCompleted) {
            BOOL isComplete;
            if (cb->version < 2) {
                RSStreamError err = {0};
                isComplete = ((_RSStreamCBOpenCompletedV1)cb->openCompleted)(stream, &err, _RSStreamGetInfoPointer(stream));
                if (err.error != 0) {
                    _RSStreamSetStreamError(stream, &err);
                } else {
                    isComplete = cb->openCompleted(stream, &(stream->error), _RSStreamGetInfoPointer(stream));
                }
                if (isComplete) {
                    if (!stream->error) {
                        status = RSStreamStatusOpen;
                    } else {
                        status =RSStreamStatusError;
                    }
                    _RSStreamSetStatusCode(stream, status);
                    if (status == RSStreamStatusOpen) {
                        _RSStreamScheduleEvent(status, RSStreamEventOpenCompleted);
                    } else {
                        _RSStreamScheduleEvent(stream, RSStreamEventErrorOccurred);
                    }
                }
            }
        }
    }
    __RSBitClear(stream->flags, CALLING_CLIENT);
    return status;
}

RSExport RSStreamStatus RSReadStreamGetStatus(RSReadStreamRef stream) {
    RS_OBJC_FUNCDISPATCHV(__RSReadStreamTypeID, RSStreamStatus, (NSInputStream *)stream, streamStatus);
    return _RSStreamGetStatus((struct _RSStream *)stream);
}

RSExport RSStreamStatus RSWriteStreamGetStatus(RSWriteStreamRef stream) {
    RS_OBJC_FUNCDISPATCHV(__RSWriteStreamTypeID, RSStreamStatus, (NSOutputStream *)stream, streamStatus);
    return _RSStreamGetStatus((struct _RSStream *)stream);
}

static RSStreamError _RSStreamGetStreamError(struct _RSStream *stream) {
    RSStreamError result;
    if (!stream->error) {
        result.error = 0;
        result.domain = 0;
    } else if (_RSStreamGetCallBackPtr(stream)->version < 2) {
        RSStreamError *streamError = (RSStreamError *)(stream->error);
        result.error = streamError->error;
        result.domain = streamError->domain;
    } else {
        result = _RSStreamErrorFromError(stream->error);
    }
    return result;
}

RSExport RSStreamError RSReadStreamGetError(RSReadStreamRef stream) {
    RS_OBJC_FUNCDISPATCHV(__RSReadStreamTypeID, RSStreamError, (NSInputStream *)stream, _cfStreamError);
    return _RSStreamGetStreamError((struct _RSStream *)stream);
}

RSExport RSStreamError RSWriteStreamGetError(RSWriteStreamRef stream) {
    RS_OBJC_FUNCDISPATCHV(__RSWriteStreamTypeID, RSStreamError, (NSOutputStream *)stream, _cfStreamError);
    return _RSStreamGetStreamError((struct _RSStream *)stream);
}

static RSErrorRef _RSStreamCopyError(struct _RSStream *stream) {
    if (!stream->error) {
        return nil;
    } else if (_RSStreamGetCallBackPtr(stream)->version < 2) {
        return _RSErrorFromStreamError(RSGetAllocator(stream), (RSStreamError *)stream->error);
    } else {
        RSRetain(stream->error);
        return stream->error;
    }
}

RSExport RSErrorRef RSReadStreamCopyError(RSReadStreamRef stream) {
    RS_OBJC_FUNCDISPATCHV(__RSReadStreamTypeID, RSErrorRef, (NSInputStream *)stream, streamError);
    return _RSStreamCopyError((struct _RSStream *)stream);
}

RSExport RSErrorRef RSWriteStreamCopyError(RSWriteStreamRef stream) {
    return _RSStreamCopyError((struct _RSStream *)stream);
    RS_OBJC_FUNCDISPATCHV(__RSWriteStreamTypeID, RSErrorRef, (NSOutputStream *)stream, streamError);
}

RSPrivate BOOL _RSStreamOpen(struct _RSStream *stream) {
    const struct _RSStreamCallBacks *cb = _RSStreamGetCallBackPtr(stream);
    BOOL success, openComplete;
    if (_RSStreamGetStatus(stream) != RSStreamStatusNotOpen) {
        return FALSE;
    }
    __RSBitSet(stream->flags, CALLING_CLIENT);
    _RSStreamSetStatusCode(stream, RSStreamStatusOpening);
    if (cb->open) {
        if (cb->version < 2) {
            RSStreamError err = {0, 0};
            success = ((_RSStreamCBOpenV1)(cb->open))(stream, &err, &openComplete, _RSStreamGetInfoPointer(stream));
            if (err.error != 0) _RSStreamSetStreamError(stream, &err);
        } else {
            success = cb->open(stream, &(stream->error), &openComplete, _RSStreamGetInfoPointer(stream));
        }
    } else {
        success = TRUE;
        openComplete = TRUE;
    }
    if (openComplete) {
        if (success) {
            // 2957690 - Guard against the possibility that the stream has already signalled itself in to a later state (like AtEnd)
            if (__RSStreamGetStatus(stream) == RSStreamStatusOpening) {
                _RSStreamSetStatusCode(stream, RSStreamStatusOpen);
            }
            _RSStreamScheduleEvent(stream, RSStreamEventOpenCompleted);
        } else {
#if DEPLOYMENT_TARGET_WINDOWS
            _RSStreamClose(stream);
#endif
            _RSStreamSetStatusCode(stream, RSStreamStatusError);
            _RSStreamScheduleEvent(stream, RSStreamEventErrorOccurred);
        }
    }
    __RSBitClear(stream->flags, CALLING_CLIENT);
    return success;
}

RSExport BOOL RSReadStreamOpen(RSReadStreamRef stream) {
    if(RS_IS_OBJC(__RSReadStreamTypeID, stream)) {
        (void)RS_OBJC_CALLV((NSInputStream *)stream, open);
        return TRUE;
    }
    return _RSStreamOpen((struct _RSStream *)stream);
}

RSExport BOOL RSWriteStreamOpen(RSWriteStreamRef stream) {
    if(RS_IS_OBJC(__RSWriteStreamTypeID, stream)) {
        (void)RS_OBJC_CALLV((NSOutputStream *)stream, open);
        return TRUE;
    }
    return _RSStreamOpen((struct _RSStream *)stream);
}

RSExport void RSReadStreamClose(RSReadStreamRef stream) {
    RS_OBJC_FUNCDISPATCHV(__RSReadStreamTypeID, void, (NSInputStream *)stream, close);
    _RSStreamClose((struct _RSStream *)stream);
}

RSExport void RSWriteStreamClose(RSWriteStreamRef stream) {
    RS_OBJC_FUNCDISPATCHV(__RSWriteStreamTypeID, void, (NSOutputStream *)stream, close);
    _RSStreamClose((struct _RSStream *)stream);
}

RSExport BOOL RSReadStreamHasBytesAvailable(RSReadStreamRef readStream) {
    RS_OBJC_FUNCDISPATCHV(__RSReadStreamTypeID, BOOL, (NSInputStream *)readStream, hasBytesAvailable);
    struct _RSStream *stream = (struct _RSStream *)readStream;
    RSStreamStatus status = _RSStreamGetStatus(stream);
    const struct _RSStreamCallBacks *cb;
    if (status != RSStreamStatusOpen && status != RSStreamStatusReading) {
        return FALSE;
    }
    cb  = _RSStreamGetCallBackPtr(stream);
    if (cb->canRead == NULL) {
        return TRUE;  // No way to know without trying....
    } else {
        BOOL result;
        __RSBitSet(stream->flags, CALLING_CLIENT);
        if (cb->version < 2) {
            result = ((_RSStreamCBCanReadV1)(cb->canRead))((RSReadStreamRef)stream, _RSStreamGetInfoPointer(stream));
        } else {
            result = cb->canRead((RSReadStreamRef)stream, &(stream->error), _RSStreamGetInfoPointer(stream));
            if (stream->error) {
                _RSStreamSetStatusCode(stream, RSStreamStatusError);
                _RSStreamScheduleEvent(stream, RSStreamEventErrorOccurred);
            }
        }
        __RSBitClear(stream->flags, CALLING_CLIENT);
        return result;
    }
}

static void waitForOpen(struct _RSStream *stream);
RSIndex RSReadStreamRead(RSReadStreamRef readStream, UInt8 *buffer, RSIndex bufferLength) {
    RS_OBJC_FUNCDISPATCHV(__RSReadStreamTypeID, RSIndex, (NSInputStream *)readStream, read:(uint8_t *)buffer maxLength:(NSUInteger)bufferLength);
    struct _RSStream *stream = (struct _RSStream *)readStream;
    RSStreamStatus status = _RSStreamGetStatus(stream);
    const struct _RSStreamCallBacks *cb = _RSStreamGetCallBackPtr(stream);
    if (status == RSStreamStatusOpening) {
        __RSBitSet(stream->flags, CALLING_CLIENT);
        waitForOpen(stream);
        __RSBitClear(stream->flags, CALLING_CLIENT);
        status = _RSStreamGetStatus(stream);
    }
    
    if (status != RSStreamStatusOpen && status != RSStreamStatusReading && status != RSStreamStatusAtEnd) {
        return -1;
    } else  if (status == RSStreamStatusAtEnd) {
        return 0;
    } else {
        BOOL atEOF;
        RSIndex bytesRead;
        __RSBitSet(stream->flags, CALLING_CLIENT);
        if (stream->client) {
            stream->client->whatToSignal &= ~RSStreamEventHasBytesAvailable;
        }
        _RSStreamSetStatusCode(stream, RSStreamStatusReading);
        if (cb->version < 2) {
            RSStreamError err = {0, 0};
            bytesRead = ((_RSStreamCBReadV1)(cb->read))((RSReadStreamRef)stream, buffer, bufferLength, &err, &atEOF, _RSStreamGetInfoPointer(stream));
            if (err.error != 0) _RSStreamSetStreamError(stream, &err);
        } else {
            bytesRead = cb->read((RSReadStreamRef)stream, buffer, bufferLength, &(stream->error), &atEOF, _RSStreamGetInfoPointer(stream));
        }
        if (stream->error) {
            bytesRead = -1;
            _RSStreamSetStatusCode(stream, RSStreamStatusError);
            _RSStreamScheduleEvent(stream, RSStreamEventErrorOccurred);
        } else if (atEOF) {
            _RSStreamSetStatusCode(stream, RSStreamStatusAtEnd);
            _RSStreamScheduleEvent(stream, RSStreamEventEndEncountered);
        } else {
            _RSStreamSetStatusCode(stream, RSStreamStatusOpen);
        }
        __RSBitClear(stream->flags, CALLING_CLIENT);
        return bytesRead;
    }
}

RSExport const UInt8 *RSReadStreamGetBuffer(RSReadStreamRef readStream, RSIndex maxBytesToRead, RSIndex *numBytesRead) {
    if (RS_IS_OBJC(__RSReadStreamTypeID, readStream)) {
        uint8_t *bufPtr = NULL;
        BOOL gotBytes = (BOOL) RS_OBJC_CALLV((NSInputStream *)readStream, getBuffer:&bufPtr length:(NSUInteger *)numBytesRead);
        if(gotBytes) {
            return (const UInt8 *)bufPtr;
        } else {
            return NULL;
        }
    }
    struct _RSStream *stream = (struct _RSStream *)readStream;
    RSStreamStatus status = _RSStreamGetStatus(stream);
    const struct _RSStreamCallBacks *cb = _RSStreamGetCallBackPtr(stream);
    const UInt8 *buffer;
    if (status == RSStreamStatusOpening) {
        __RSBitSet(stream->flags, CALLING_CLIENT);
        waitForOpen(stream);
        __RSBitClear(stream->flags, CALLING_CLIENT);
        status = _RSStreamGetStatus(stream);
    }
    if (status != RSStreamStatusOpen && status != RSStreamStatusReading && status != RSStreamStatusAtEnd) {
        *numBytesRead = -1;
        buffer = NULL;
    } else  if (status == RSStreamStatusAtEnd || cb->getBuffer == NULL) {
        *numBytesRead = 0;
        buffer = NULL;
    } else {
        BOOL atEOF;
        BOOL hadBytes = stream->client && (stream->client->whatToSignal & RSStreamEventHasBytesAvailable);
        __RSBitSet(stream->flags, CALLING_CLIENT);
        if (hadBytes) {
            stream->client->whatToSignal &= ~RSStreamEventHasBytesAvailable;
        }
        _RSStreamSetStatusCode(stream, RSStreamStatusReading);
        if (cb->version < 2) {
            RSStreamError err = {0, 0};
            buffer = ((_RSStreamCBGetBufferV1)(cb->getBuffer))((RSReadStreamRef)stream, maxBytesToRead, numBytesRead, &err, &atEOF, _RSStreamGetInfoPointer(stream));
            if (err.error != 0) _RSStreamSetStreamError(stream, &err);
        } else {
            buffer = cb->getBuffer((RSReadStreamRef)stream, maxBytesToRead, numBytesRead, &(stream->error), &atEOF, _RSStreamGetInfoPointer(stream));
        }
        if (stream->error) {
            *numBytesRead = -1;
            _RSStreamSetStatusCode(stream, RSStreamStatusError);
            buffer = NULL;
            _RSStreamScheduleEvent(stream, RSStreamEventErrorOccurred);
        } else if (atEOF) {
            _RSStreamSetStatusCode(stream, RSStreamStatusAtEnd);
            _RSStreamScheduleEvent(stream, RSStreamEventEndEncountered);
        } else {
            if (!buffer && hadBytes) {
                stream->client->whatToSignal |= RSStreamEventHasBytesAvailable;
            }
            _RSStreamSetStatusCode(stream, RSStreamStatusOpen);
        }
        __RSBitClear(stream->flags, CALLING_CLIENT);
    }
    return buffer;
}

RSExport BOOL RSWriteStreamCanAcceptBytes(RSWriteStreamRef writeStream) {
    RS_OBJC_FUNCDISPATCHV(__RSWriteStreamTypeID, BOOL, (NSOutputStream *)writeStream, hasSpaceAvailable);
    struct _RSStream *stream = (struct _RSStream *)writeStream;
    RSStreamStatus status = _RSStreamGetStatus(stream);
    const struct _RSStreamCallBacks *cb;
    if (status != RSStreamStatusOpen && status != RSStreamStatusWriting) {
        return FALSE;
    }
    cb  = _RSStreamGetCallBackPtr(stream);
    if (cb->canWrite == NULL) {
        return TRUE;  // No way to know without trying....
    } else {
        BOOL result;
        __RSBitSet(stream->flags, CALLING_CLIENT);
        if (cb->version < 2) {
            result = ((_RSStreamCBCanWriteV1)(cb->canWrite))((RSWriteStreamRef)stream, _RSStreamGetInfoPointer(stream));
        } else {
            result = cb->canWrite((RSWriteStreamRef)stream, &(stream->error), _RSStreamGetInfoPointer(stream));
            if (stream->error) {
                _RSStreamSetStatusCode(stream, RSStreamStatusError);
                _RSStreamScheduleEvent(stream, RSStreamEventErrorOccurred);
            }
        }
        __RSBitClear(stream->flags, CALLING_CLIENT);
        return result;
    }
}

RSExport RSIndex RSWriteStreamWrite(RSWriteStreamRef writeStream, const UInt8 *buffer, RSIndex bufferLength) {
    RS_OBJC_FUNCDISPATCHV(__RSWriteStreamTypeID, RSIndex, (NSOutputStream *)writeStream, write:(const uint8_t *)buffer maxLength:(NSUInteger)bufferLength);
    struct _RSStream *stream = (struct _RSStream *)writeStream;
    RSStreamStatus status = _RSStreamGetStatus(stream);
    const struct _RSStreamCallBacks *cb = _RSStreamGetCallBackPtr(stream);
    if (status == RSStreamStatusOpening) {
        __RSBitSet(stream->flags, CALLING_CLIENT);
        waitForOpen(stream);
        __RSBitClear(stream->flags, CALLING_CLIENT);
        status = _RSStreamGetStatus(stream);
    }
    if (status != RSStreamStatusOpen && status != RSStreamStatusWriting) {
        return -1;
    } else {
        RSIndex result;
        __RSBitSet(stream->flags, CALLING_CLIENT);
        _RSStreamSetStatusCode(stream, RSStreamStatusWriting);
        if (stream->client) {
            stream->client->whatToSignal &= ~RSStreamEventCanAcceptBytes;
        }
        if (cb->version < 2) {
            RSStreamError err = {0, 0};
            result = ((_RSStreamCBWriteV1)(cb->write))((RSWriteStreamRef)stream, buffer, bufferLength, &err, _RSStreamGetInfoPointer(stream));
            if (err.error) _RSStreamSetStreamError(stream, &err);
        } else {
            result = cb->write((RSWriteStreamRef)stream, buffer, bufferLength, &(stream->error), _RSStreamGetInfoPointer(stream));
        }
        if (stream->error) {
            _RSStreamSetStatusCode(stream, RSStreamStatusError);
            _RSStreamScheduleEvent(stream, RSStreamEventErrorOccurred);
        } else if (result == 0) {
            _RSStreamSetStatusCode(stream, RSStreamStatusAtEnd);
            _RSStreamScheduleEvent(stream, RSStreamEventEndEncountered);
        } else {
            _RSStreamSetStatusCode(stream, RSStreamStatusOpen);
        }
        __RSBitClear(stream->flags, CALLING_CLIENT);
        return result;
    }
}

RSPrivate RSTypeRef _RSStreamCopyProperty(struct _RSStream *stream, RSStringRef propertyName) {
    const struct _RSStreamCallBacks *cb = _RSStreamGetCallBackPtr(stream);
    if (cb->copyProperty == NULL) {
        return NULL;
    } else {
        RSTypeRef result;
        __RSBitSet(stream->flags, CALLING_CLIENT);
        result = cb->copyProperty(stream, propertyName, _RSStreamGetInfoPointer(stream));
        __RSBitClear(stream->flags, CALLING_CLIENT);
        return result;
    }
}

RSExport RSTypeRef RSReadStreamCopyProperty(RSReadStreamRef stream, RSStringRef propertyName) {
    RS_OBJC_FUNCDISPATCHV(__RSReadStreamTypeID, RSTypeRef, (NSInputStream *)stream, propertyForKey:(NSString *)propertyName);
    return _RSStreamCopyProperty((struct _RSStream *)stream, propertyName);
}

RSExport RSTypeRef RSWriteStreamCopyProperty(RSWriteStreamRef stream, RSStringRef propertyName) {
    RS_OBJC_FUNCDISPATCHV(__RSWriteStreamTypeID, RSTypeRef, (NSOutputStream *)stream, propertyForKey:(NSString *)propertyName);
    return _RSStreamCopyProperty((struct _RSStream *)stream, propertyName);
}

RSPrivate BOOL _RSStreamSetProperty(struct _RSStream *stream, RSStringRef prop, RSTypeRef val) {
    const struct _RSStreamCallBacks *cb = _RSStreamGetCallBackPtr(stream);
    if (cb->setProperty == NULL) {
        return FALSE;
    } else {
        BOOL result;
        __RSBitSet(stream->flags, CALLING_CLIENT);
        result = cb->setProperty(stream, prop, val, _RSStreamGetInfoPointer(stream));
        __RSBitClear(stream->flags, CALLING_CLIENT);
        return result;
    }
}

RSExport
BOOL RSReadStreamSetProperty(RSReadStreamRef stream, RSStringRef propertyName, RSTypeRef propertyValue) {
    RS_OBJC_FUNCDISPATCHV(__RSReadStreamTypeID, BOOL, (NSInputStream *)stream, setProperty:(id)propertyValue forKey:(NSString *)propertyName);
    return _RSStreamSetProperty((struct _RSStream *)stream, propertyName, propertyValue);
}

RSExport
BOOL RSWriteStreamSetProperty(RSWriteStreamRef stream, RSStringRef propertyName, RSTypeRef propertyValue) {
    RS_OBJC_FUNCDISPATCHV(__RSWriteStreamTypeID, BOOL, (NSOutputStream *)stream, setProperty:(id)propertyValue forKey:(NSString *)propertyName);
    return _RSStreamSetProperty((struct _RSStream *)stream, propertyName, propertyValue);
}

static void _initializeClient(struct _RSStream *stream) {
    const struct _RSStreamCallBacks *cb = _RSStreamGetCallBackPtr(stream);
    if (!cb->schedule) return; // Do we wish to allow this?
    stream->client = (struct _RSStreamClient *)RSAllocatorAllocate(RSGetAllocator(stream), sizeof(struct _RSStreamClient));
    memset(stream->client, 0, sizeof(struct _RSStreamClient));
}

/* If we add a setClient callback to the concrete stream callbacks, we must set/clear CALLING_CLIENT around it */
RSPrivate BOOL _RSStreamSetClient(struct _RSStream *stream, RSOptionFlags streamEvents, void (*clientCB)(struct _RSStream *, RSStreamEventType, void *), RSStreamClientContext *clientCallBackContext) {
    
    BOOL removingClient = (streamEvents == RSStreamEventNone || clientCB == NULL || clientCallBackContext == NULL);
    
    if (removingClient) {
        clientCB = NULL;
        streamEvents = RSStreamEventNone;
        clientCallBackContext = NULL;
    }
    if (!stream->client) {
        if (removingClient) {
            // We have no client now, and we've been asked to add none???
            return TRUE;
        }
        _initializeClient(stream);
        if (!stream->client) {
            // Asynch not supported
            return FALSE;
        }
    }
    if (stream->client->cb && stream->client->cbContext.release) {
        stream->client->cbContext.release(stream->client->cbContext.info);
    }
    stream->client->cb = clientCB;
    if (clientCallBackContext) {
        stream->client->cbContext.version = clientCallBackContext->version;
        stream->client->cbContext.retain = clientCallBackContext->retain;
        stream->client->cbContext.release = clientCallBackContext->release;
        stream->client->cbContext.copyDescription = clientCallBackContext->copyDescription;
        stream->client->cbContext.info = (clientCallBackContext->retain && clientCallBackContext->info) ? clientCallBackContext->retain(clientCallBackContext->info) : clientCallBackContext->info;
    } else {
        stream->client->cbContext.retain = NULL;
        stream->client->cbContext.release = NULL;
        stream->client->cbContext.copyDescription = NULL;
        stream->client->cbContext.info = NULL;
    }
    if (stream->client->when != streamEvents) {
        const struct _RSStreamCallBacks *cb = _RSStreamGetCallBackPtr(stream);
        stream->client->when = streamEvents;
        if (cb->requestEvents) {
            cb->requestEvents(stream, streamEvents, _RSStreamGetInfoPointer(stream));
        }
    }
    return TRUE;
}

RSExport BOOL RSReadStreamSetClient(RSReadStreamRef readStream, RSOptionFlags streamEvents, RSReadStreamClientCallBack clientCB, RSStreamClientContext *clientContext) {
#if defined(RSSTREAM_SUPPORTS_BRIDGING)
    if (RS_IS_OBJC(__RSReadStreamTypeID, (const void *)(NSInputStream *)readStream)) {
        NSInputStream* is = (NSInputStream*) readStream;
        
        if ([is respondsToSelector:@selector(_setRSClientFlags:callback:context:)])
            return [is _setRSClientFlags:streamEvents callback:clientCB context:clientContext];
        else {
            if (clientCB == NULL)
                [is setDelegate:nil];
            else {
                _RSStreamDelegate* d = [[_RSStreamDelegate alloc] initWithStreamEvents:streamEvents callback:(void*) clientCB context:clientContext];
                [is setDelegate:d];
            }
            return true;
        }
    }
#endif
    
    streamEvents &= ~RSStreamEventCanAcceptBytes;
    return _RSStreamSetClient((struct _RSStream *)readStream, streamEvents, (void (*)(struct _RSStream *, RSStreamEventType, void *))clientCB, clientContext);
}

RSExport BOOL RSWriteStreamSetClient(RSWriteStreamRef writeStream, RSOptionFlags streamEvents, RSWriteStreamClientCallBack clientCB, RSStreamClientContext *clientContext) {
#if defined(RSSTREAM_SUPPORTS_BRIDGING)
    if (RS_IS_OBJC(__RSWriteStreamTypeID, (const void *)(NSInputStream *)writeStream)) {
        NSOutputStream* os = (NSOutputStream*) writeStream;
        
        if ([os respondsToSelector:@selector(_setRSClientFlags:callback:context:)])
            return [os _setRSClientFlags:streamEvents callback:clientCB context:clientContext];
        else {
            if (clientCB == NULL)
                [os setDelegate:nil];
            else {
                _RSStreamDelegate* d = [[_RSStreamDelegate alloc] initWithStreamEvents:streamEvents callback:(void*) clientCB context:clientContext];
                [os setDelegate:d];
            }
            return true;
        }
    }
#endif
    
    streamEvents &= ~RSStreamEventHasBytesAvailable;
    return _RSStreamSetClient((struct _RSStream *)writeStream, streamEvents, (void (*)(struct _RSStream *, RSStreamEventType, void *))clientCB, clientContext);
}

RSInline void *_RSStreamGetClient(struct _RSStream *stream) {
    if (stream->client) return stream->client->cbContext.info;
    else return NULL;
}

RSExport void *_RSReadStreamGetClient(RSReadStreamRef readStream) {
    return _RSStreamGetClient((struct _RSStream *)readStream);
}

RSExport void *_RSWriteStreamGetClient(RSWriteStreamRef writeStream) {
    return _RSStreamGetClient((struct _RSStream *)writeStream);
}


RSPrivate void _RSStreamScheduleWithRunLoop(struct _RSStream *stream, RSRunLoopRef runLoop, RSStringRef runLoopMode) {
    const struct _RSStreamCallBacks *cb = _RSStreamGetCallBackPtr(stream);
    
    if (! stream->client) {
        _initializeClient(stream);
        if (!stream->client) return; // we don't support asynch.
    }
    
    if (! stream->client->rlSource) {
        /* No source, so we join the shared source group */
        RSTypeRef a[] = { runLoop, runLoopMode };
        
        RSArrayRef runLoopAndSourceKey = RSArrayCreateWithObjects(RSAllocatorSystemDefault, a, sizeof(a) / sizeof(a[0]));
        
        RSSpinLockLock(&sSourceLock);
        
        if (!sSharedSources)
            sSharedSources = RSDictionaryCreateMutable(RSAllocatorSystemDefault, 0, RSDictionaryRSTypeContext);
        RSMutableArrayRef listOfStreamsSharingASource = (RSMutableArrayRef)RSDictionaryGetValue(sSharedSources, runLoopAndSourceKey);
        if (listOfStreamsSharingASource) {
            struct _RSStream* aStream = (struct _RSStream*) RSArrayObjectAtIndex(listOfStreamsSharingASource, 0);
            RSRunLoopSourceRef source = _RSStreamCopySource(aStream);
            if (source) {
                _RSStreamSetSource(stream, source, FALSE);
                RSRelease(source);
            }
            RSRetain(listOfStreamsSharingASource);
        }
        else {
            RSRunLoopSourceContext ctxt = {
                0,
                NULL,
                RSRetain,
                RSRelease,
                (RSStringRef(*)(const void *))RSDescription,
                NULL,
                NULL,
                NULL,
                NULL,
                (void(*)(void *))_rsstream_shared_signalEventSync
            };
            
            listOfStreamsSharingASource = RSArrayCreateMutable(RSAllocatorSystemDefault, 0);
            RSDictionarySetValue(sSharedSources, runLoopAndSourceKey, listOfStreamsSharingASource);
            ctxt.info = listOfStreamsSharingASource;
            
            RSRunLoopSourceRef source = RSRunLoopSourceCreate(RSAllocatorSystemDefault, 0, &ctxt);
            _RSStreamSetSource(stream, source, FALSE);
            RSRunLoopAddSource(runLoop, source, runLoopMode);
            RSRelease(source);
        }
        
        RSArrayAddObject(listOfStreamsSharingASource, stream);
        RSDictionarySetValue(sSharedSources, stream, runLoopAndSourceKey);
        
        RSRelease(runLoopAndSourceKey);
        RSRelease(listOfStreamsSharingASource);
        
        __RSBitSet(stream->flags, SHARED_SOURCE);
        
        RSSpinLockUnlock(&sSourceLock);
    }
    else if (__RSBitIsSet(stream->flags, SHARED_SOURCE)) {
        /* We were sharing, but now we'll get our own source */
        
        RSArrayRef runLoopAndSourceKey;
        RSMutableArrayRef listOfStreamsSharingASource;
        RSIndex count, i;
        
        RSAllocatorRef alloc = RSGetAllocator(stream);
        RSRunLoopSourceContext ctxt = {
            0,
            (void *)stream,
            NULL,														// Do not use RSRetain/RSRelease callbacks here; that will cause a retain loop
            NULL,														// Do not use RSRetain/RSRelease callbacks here; that will cause a retain loop
            (RSStringRef(*)(const void *))RSDescription,
            NULL,
            NULL,
            NULL,
            NULL,
            (void(*)(void *))_rsstream_solo_signalEventSync
        };
        
        RSSpinLockLock(&sSourceLock);
        
        runLoopAndSourceKey = (RSArrayRef)RSRetain((RSTypeRef)RSDictionaryGetValue(sSharedSources, stream));
        listOfStreamsSharingASource = (RSMutableArrayRef)RSDictionaryGetValue(sSharedSources, runLoopAndSourceKey);
        
        count = RSArrayGetCount(listOfStreamsSharingASource);
        i = RSArrayIndexOfObject(listOfStreamsSharingASource, stream);
        if (i != RSNotFound) {
            RSArrayRemoveObjectAtIndex(listOfStreamsSharingASource, i);
            count--;
        }
        
        if (count == 0) {
            RSRunLoopSourceRef source = _RSStreamCopySource(stream);
            if (source) {
                RSRunLoopRemoveSource((RSRunLoopRef)RSArrayObjectAtIndex(runLoopAndSourceKey, 0), source, (RSStringRef)RSArrayObjectAtIndex(runLoopAndSourceKey, 1));
                RSRelease(source);
            }
            RSDictionaryRemoveValue(sSharedSources, runLoopAndSourceKey);
        }
        
        RSDictionaryRemoveValue(sSharedSources, stream);
        
        _RSStreamSetSource(stream, NULL, count == 0);
        
        __RSBitClear(stream->flags, SHARED_SOURCE);
        
        RSSpinLockUnlock(&sSourceLock);
        
        RSRunLoopSourceRef source = RSRunLoopSourceCreate(alloc, 0, &ctxt);
        _RSStreamSetSource(stream, source, FALSE);
        RSRunLoopAddSource((RSRunLoopRef)RSArrayObjectAtIndex(runLoopAndSourceKey, 0), source, (RSStringRef)RSArrayObjectAtIndex(runLoopAndSourceKey, 1));
        RSRelease(runLoopAndSourceKey);
        
        RSRunLoopAddSource(runLoop, source, runLoopMode);
        
        RSRelease(source);
    } else {
        /* We're not sharing, so just add the source to the rl & mode */
        RSRunLoopSourceRef source = _RSStreamCopySource(stream);
        if (source) {
            RSRunLoopAddSource(runLoop, source, runLoopMode);
            RSRelease(source);
        }
    }
    
    _RSStreamLock(stream);
    if (!stream->client->runLoopsAndModes) {
        stream->client->runLoopsAndModes = RSArrayCreateMutable(RSGetAllocator(stream), 0);
    }
    RSArrayAddObject(stream->client->runLoopsAndModes, runLoop);
    RSArrayAddObject(stream->client->runLoopsAndModes, runLoopMode);
    checkRLMArray(stream->client->runLoopsAndModes);
    _RSStreamUnlock(stream);
    
    if (cb->schedule) {
        __RSBitSet(stream->flags, CALLING_CLIENT);
        cb->schedule(stream, runLoop, runLoopMode, _RSStreamGetInfoPointer(stream));
        __RSBitClear(stream->flags, CALLING_CLIENT);
    }
    
    /*
     * If we've got events pending, we need to wake up and signal
     */
    if (stream->client && stream->client->whatToSignal != 0) {
        RSRunLoopSourceRef source = _RSStreamCopySource(stream);
        if (source) {
            RSRunLoopSourceSignal(source);
            RSRelease(source);
            _wakeUpRunLoop(stream);
        }
    }
}

RSExport void RSReadStreamScheduleWithRunLoop(RSReadStreamRef stream, RSRunLoopRef runLoop, RSStringRef runLoopMode) {
#if defined(RSSTREAM_SUPPORTS_BRIDGING)
    if (RS_IS_OBJC(__RSReadStreamTypeID, (const void *)(NSInputStream *)stream)) {
        NSInputStream* is  = (NSInputStream*) stream;
        if ([is respondsToSelector:@selector(_scheduleInRSRunLoop:forMode:)])
            [is _scheduleInRSRunLoop:runLoop forMode:runLoopMode];
        else
            [is scheduleInRunLoop:cfToNSRL(runLoop) forMode:(id) runLoopMode];
        return;
    }
#endif
    
    _RSStreamScheduleWithRunLoop((struct _RSStream *)stream, runLoop, runLoopMode);
}

RSExport void RSWriteStreamScheduleWithRunLoop(RSWriteStreamRef stream, RSRunLoopRef runLoop, RSStringRef runLoopMode) {
#if defined(RSSTREAM_SUPPORTS_BRIDGING)
    if (RS_IS_OBJC(__RSWriteStreamTypeID, (const void *)(NSOutputStream *)stream)) {
        NSOutputStream* os  = (NSOutputStream*) stream;
        if ([os respondsToSelector:@selector(_scheduleInRSRunLoop:forMode:)])
            [os _scheduleInRSRunLoop:runLoop forMode:runLoopMode];
        else
            [os scheduleInRunLoop:cfToNSRL(runLoop) forMode:(id) runLoopMode];
        return;
    }
#endif
    
    _RSStreamScheduleWithRunLoop((struct _RSStream *)stream, runLoop, runLoopMode);
}


RSPrivate void _RSStreamUnscheduleFromRunLoop(struct _RSStream *stream, RSRunLoopRef runLoop, RSStringRef runLoopMode) {
    const struct _RSStreamCallBacks *cb = _RSStreamGetCallBackPtr(stream);
    if (!stream->client) return;
    if (!stream->client->rlSource) return;
    
    if (!__RSBitIsSet(stream->flags, SHARED_SOURCE)) {
        RSRunLoopSourceRef source = _RSStreamCopySource(stream);
        if (source) {
            RSRunLoopRemoveSource(runLoop, source, runLoopMode);
            RSRelease(source);
        }
    } else {
        RSArrayRef runLoopAndSourceKey;
        RSMutableArrayRef list;
        RSIndex count, i;
        
        RSSpinLockLock(&sSourceLock);
        
        runLoopAndSourceKey = (RSArrayRef)RSDictionaryGetValue(sSharedSources, stream);
        list = (RSMutableArrayRef)RSDictionaryGetValue(sSharedSources, runLoopAndSourceKey);
        
        count = RSArrayGetCount(list);
        i = RSArrayIndexOfObject(list, stream);
        if (i != RSNotFound) {
            RSArrayRemoveObjectAtIndex(list, i);
            count--;
        }
        
        if (count == 0) {
            RSRunLoopSourceRef source = _RSStreamCopySource(stream);
            if (source) {
                RSRunLoopRemoveSource(runLoop, source, runLoopMode);
                RSRelease(source);
            }
            RSDictionaryRemoveValue(sSharedSources, runLoopAndSourceKey);
        }
        
        RSDictionaryRemoveValue(sSharedSources, stream);
        
        _RSStreamSetSource(stream, NULL, count == 0);
        
        __RSBitClear(stream->flags, SHARED_SOURCE);
        
        RSSpinLockUnlock(&sSourceLock);
    }
    
    _RSStreamLock(stream);
    _RSStreamRemoveRunLoopAndModeFromArray(stream->client->runLoopsAndModes, runLoop, runLoopMode);
    checkRLMArray(stream->client->runLoopsAndModes);
    _RSStreamUnlock(stream);
    
    if (cb->unschedule) {
        cb->unschedule(stream, runLoop, runLoopMode, _RSStreamGetInfoPointer(stream));
    }
}

RSExport void RSReadStreamUnscheduleFromRunLoop(RSReadStreamRef stream, RSRunLoopRef runLoop, RSStringRef runLoopMode) {
#if defined(RSSTREAM_SUPPORTS_BRIDGING)
    if (RS_IS_OBJC(__RSReadStreamTypeID, (const void *)(NSInputStream *)stream)) {
        NSInputStream* is  = (NSInputStream*) stream;
        if ([is respondsToSelector:@selector(_unscheduleFromRSRunLoop:forMode:)])
            [is _unscheduleFromRSRunLoop:runLoop forMode:runLoopMode];
        else
            [is removeFromRunLoop:cfToNSRL(runLoop) forMode:(id) runLoopMode];
        return;
    }
#endif
    
    _RSStreamUnscheduleFromRunLoop((struct _RSStream *)stream, runLoop, runLoopMode);
}

void RSWriteStreamUnscheduleFromRunLoop(RSWriteStreamRef stream, RSRunLoopRef runLoop, RSStringRef runLoopMode) {
#if defined(RSSTREAM_SUPPORTS_BRIDGING)
    if (RS_IS_OBJC(__RSWriteStreamTypeID, (const void *)(NSOutputStream *)stream)) {
        NSOutputStream* os  = (NSOutputStream*) stream;
        if ([os respondsToSelector:@selector(_unscheduleFromRSRunLoop:forMode:)])
            [os _unscheduleFromRSRunLoop:runLoop forMode:runLoopMode];
        else
            [os removeFromRunLoop:cfToNSRL(runLoop) forMode:(id) runLoopMode];
        return;
    }
#endif
    
    _RSStreamUnscheduleFromRunLoop((struct _RSStream *)stream, runLoop, runLoopMode);
}

static RSRunLoopRef sLegacyRL = NULL;

static void _perform(void* info)
{
}

static void* _legacyStreamRunLoop_workThread(void* arg)
{
    sLegacyRL = RSRunLoopGetCurrent();
    
#if defined(LOG_STREAM)
    fprintf(stderr, "Creating Schedulingset emulation thread.  Runloop: %p\n", sLegacyRL);
#endif
    
    RSStringRef s = RSStringCreateWithFormat(RSAllocatorDefault, NULL, RSSTR("<< RSStreamLegacySource for Runloop %p >>"), sLegacyRL);
    
    RSRunLoopSourceContext ctxt = {
        0,
        (void*) s,
        RSRetain,
        RSRelease,
        RSDescription,
        RSEqual,
        RSHash,
        NULL,
        NULL,
        _perform
    };
    
    RSRunLoopSourceRef rls = RSRunLoopSourceCreate(RSAllocatorDefault, 0, &ctxt);
    RSRelease(s);
    
    RSRunLoopAddSource(sLegacyRL, rls, RSRunLoopDefaultMode);
    RSRelease(rls);
    
    dispatch_semaphore_signal(*(dispatch_semaphore_t*) arg);
    arg = NULL;
    
    while (true) {
        RSUInteger why = RSRunLoopRunInMode(RSRunLoopDefaultMode, 1E30, true);
        
        (void) why;
#if defined(LOG_STREAM)
        switch (why) {
            case RSRunLoopRunFinished:
                fprintf(stderr, "WOKE: RSRunLoopRunFinished\n");
                break;
            case RSRunLoopRunStopped:
                fprintf(stderr, "WOKE: RSRunLoopRunStopped\n");
                break;
            case RSRunLoopRunTimedOut:
                fprintf(stderr, "WOKE: RSRunLoopRunTimedOut\n");
                break;
            case RSRunLoopRunHandledSource:
                fprintf(stderr, "WOKE: RSRunLoopRunHandledSource\n");
                break;
        }
#endif
    }
    
    return NULL;
}
static RSRunLoopRef _legacyStreamRunLoop()
{
    static dispatch_once_t sOnce = 0;
    
    dispatch_once(&sOnce, ^{
        
        if (sLegacyRL == NULL) {
            dispatch_semaphore_t sem = dispatch_semaphore_create(0);
            
            pthread_attr_t attr;
            pthread_attr_init(&attr);
            pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
            pthread_t workThread;
            (void) pthread_create(&workThread, &attr, _legacyStreamRunLoop_workThread, &sem);
            pthread_attr_destroy(&attr);
            
            dispatch_semaphore_wait(sem, DISPATCH_TIME_FOREVER);
            dispatch_release(sem);
        }
    });
    
    return sLegacyRL;
}

static dispatch_queue_t _RSStreamCopyDispatchQueue(struct _RSStream* stream)
{
    dispatch_queue_t result = NULL;
    
    _RSStreamLock(stream);
    if (stream->client) {
        result = stream->queue;
        if (result)
            dispatch_retain(result);
    }
    _RSStreamUnlock(stream);
    
    return result;
}

static void _RSStreamSetDispatchQueue(struct _RSStream* stream, dispatch_queue_t q)
{
    RSArrayRef rlm = _RSStreamCopyRunLoopsAndModes(stream);
    if (rlm) {
        RSIndex count = RSArrayGetCount(rlm);
        for (RSIndex i = 0;  i < count;  i += 2) {
            RSRunLoopRef rl = (RSRunLoopRef) RSArrayObjectAtIndex(rlm, i);
            RSStringRef mode = (RSStringRef) RSArrayObjectAtIndex(rlm, i + 1);
            _RSStreamUnscheduleFromRunLoop(stream, rl, mode);
        }
        RSRelease(rlm);
    }
    
    if (q == NULL) {
        _RSStreamLock(stream);
        if (stream->client) {
            if (stream->queue)
                dispatch_release(stream->queue);
            stream->queue = NULL;
        }
        _RSStreamUnlock(stream);
    } else {
        _RSStreamScheduleWithRunLoop(stream, _legacyStreamRunLoop(), RSRunLoopDefaultMode);
        
        _RSStreamLock(stream);
        if (stream->client) {
            if (stream->queue != q) {
                if (stream->queue)
                    dispatch_release(stream->queue);
                stream->queue = q;
                if (stream->queue)
                    dispatch_retain(stream->queue);
            }
        }
        
        _RSStreamUnlock(stream);
    }
}

void RSReadStreamSetDispatchQueue(RSReadStreamRef stream, dispatch_queue_t q)
{
    _RSStreamSetDispatchQueue((struct _RSStream*) stream, q);
}

void RSWriteStreamSetDispatchQueue(RSWriteStreamRef stream, dispatch_queue_t q)
{
    _RSStreamSetDispatchQueue((struct _RSStream*) stream, q);
}

dispatch_queue_t RSReadStreamCopyDispatchQueue(RSReadStreamRef stream)
{
    return _RSStreamCopyDispatchQueue((struct _RSStream*) stream);
}

dispatch_queue_t RSWriteStreamCopyDispatchQueue(RSWriteStreamRef stream)
{
    return _RSStreamCopyDispatchQueue((struct _RSStream*) stream);
}


static void waitForOpen(struct _RSStream *stream) {
    RSRunLoopRef runLoop = RSRunLoopGetCurrent();
    RSStringRef privateMode = RSSTR("_RSStreamBlockingOpenMode");
    _RSStreamScheduleWithRunLoop(stream, runLoop, privateMode);
    // We cannot call _RSStreamGetStatus, because that tries to set/clear CALLING_CLIENT, which should be set around this entire call (we're within a call from the client).  This should be o.k., because we're running the run loop, so our status code should be being updated in a timely fashion....
    while (__RSStreamGetStatus(stream) == RSStreamStatusOpening) {
        RSRunLoopRunInMode(privateMode, 1e+20, TRUE);
    }
    _RSStreamUnscheduleFromRunLoop(stream, runLoop, privateMode);
}

RSExport RSArrayRef _RSReadStreamGetRunLoopsAndModes(RSReadStreamRef readStream) {
    return _RSStreamGetRunLoopsAndModes((struct _RSStream *)readStream);
}

RSExport RSArrayRef _RSWriteStreamGetRunLoopsAndModes(RSWriteStreamRef writeStream) {
    return _RSStreamGetRunLoopsAndModes((struct _RSStream *)writeStream);
}

RSExport RSArrayRef _RSReadStreamCopyRunLoopsAndModes(RSReadStreamRef readStream) {
    return _RSStreamCopyRunLoopsAndModes((struct _RSStream *)readStream);
}

RSExport RSArrayRef _RSWriteStreamCopyRunLoopsAndModes(RSWriteStreamRef writeStream) {
    return _RSStreamCopyRunLoopsAndModes((struct _RSStream *)writeStream);
}

RSExport void RSReadStreamSignalEvent(RSReadStreamRef stream, RSStreamEventType event, const void *error) {
    _RSStreamSignalEvent((struct _RSStream *)stream, event, (RSErrorRef)error, TRUE);
}

RSExport void RSWriteStreamSignalEvent(RSWriteStreamRef stream, RSStreamEventType event, const void *error) {
    _RSStreamSignalEvent((struct _RSStream *)stream, event, (RSErrorRef)error, TRUE);
}

RSExport void _RSReadStreamSignalEventDelayed(RSReadStreamRef stream, RSStreamEventType event, const void *error) {
    _RSStreamSignalEvent((struct _RSStream *)stream, event, (RSErrorRef)error, FALSE);
}

RSExport void _RSReadStreamClearEvent(RSReadStreamRef readStream, RSStreamEventType event) {
    struct _RSStream *stream = (struct _RSStream *)readStream;
    if (stream->client) {
        stream->client->whatToSignal &= ~event;
    }
}

RSExport void _RSWriteStreamSignalEventDelayed(RSWriteStreamRef stream, RSStreamEventType event, const void *error) {
    _RSStreamSignalEvent((struct _RSStream *)stream, event, (RSErrorRef)error, FALSE);
}

RSExport void *RSReadStreamGetInfoPointer(RSReadStreamRef stream) {
    return _RSStreamGetInfoPointer((struct _RSStream *)stream);
}

RSExport void *RSWriteStreamGetInfoPointer(RSWriteStreamRef stream) {
    return _RSStreamGetInfoPointer((struct _RSStream *)stream);
}

/* RSExport */
void _RSStreamSourceScheduleWithRunLoop(RSRunLoopSourceRef source, RSMutableArrayRef runLoopsAndModes, RSRunLoopRef runLoop, RSStringRef runLoopMode)
{
    RSIndex count;
    RSRange range;
    
    checkRLMArray(runLoopsAndModes);
    
    count = RSArrayGetCount(runLoopsAndModes);
    range = RSMakeRange(0, count);
    
    while (range.length) {
        
        RSIndex i = RSArrayIndexOfObject(runLoopsAndModes, runLoop);
        
        if (i == RSNotFound)
            break;
        
        if (RSEqual(RSArrayObjectAtIndex(runLoopsAndModes, i + 1), runLoopMode))
            return;
        
        range.location = i + 2;
        range.length = count - range.location;
    }
    
    // Add the new values.
    RSArrayAddObject(runLoopsAndModes, runLoop);
    RSArrayAddObject(runLoopsAndModes, runLoopMode);
    
    // Schedule the source on the new loop and mode.
    if (source)
        RSRunLoopAddSource(runLoop, source, runLoopMode);
}


/* RSExport */
void _RSStreamSourceUnscheduleFromRunLoop(RSRunLoopSourceRef source, RSMutableArrayRef runLoopsAndModes, RSRunLoopRef runLoop, RSStringRef runLoopMode)
{
    RSIndex count;
    RSRange range;
    
    count = RSArrayGetCount(runLoopsAndModes);
    range = RSMakeRange(0, count);
    
    checkRLMArray(runLoopsAndModes);
    
    while (range.length) {
        
        RSIndex i = RSArrayIndexOfObject(runLoopsAndModes, runLoop);
        
        // If not found, it's not scheduled on it.
        if (i == RSNotFound)
            return;
        
        // Make sure it is scheduled in this mode.
        if (RSEqual(RSArrayObjectAtIndex(runLoopsAndModes, i + 1), runLoopMode)) {
            
            // Remove mode and runloop from the list.
            RSArrayReplaceObject(runLoopsAndModes, RSMakeRange(i, 2), nil, 0);
            // Remove it from the runloop.
            if (source)
                RSRunLoopRemoveSource(runLoop, source, runLoopMode);
            
            return;
        }
        
        range.location = i + 2;
        range.length = count - range.location;
    }
}


/* RSExport */
void _RSStreamSourceScheduleWithAllRunLoops(RSRunLoopSourceRef source, RSArrayRef runLoopsAndModes)
{
    RSIndex i, count = RSArrayGetCount(runLoopsAndModes);
    
    if (!source)
        return;
    
    checkRLMArray(runLoopsAndModes);
    
    for (i = 0; i < count; i += 2) {
        
        // Make sure it's scheduled on all the right loops and modes.
        // Go through the array adding the source to all loops and modes.
        RSRunLoopAddSource((RSRunLoopRef)RSArrayObjectAtIndex(runLoopsAndModes, i),
                           source,
                           (RSStringRef)RSArrayObjectAtIndex(runLoopsAndModes, i + 1));
    }
}


/* RSExport */
void _RSStreamSourceUncheduleFromAllRunLoops(RSRunLoopSourceRef source, RSArrayRef runLoopsAndModes)
{
    RSIndex i, count = RSArrayGetCount(runLoopsAndModes);
    
    if (!source)
        return;
    
    checkRLMArray(runLoopsAndModes);
    
    for (i = 0; i < count; i += 2) {
        
        // Go through the array removing the source from all loops and modes.
        RSRunLoopRemoveSource((RSRunLoopRef)RSArrayObjectAtIndex(runLoopsAndModes, i),
                              source,
                              (RSStringRef)RSArrayObjectAtIndex(runLoopsAndModes, i + 1));
    }
}

BOOL _RSStreamRemoveRunLoopAndModeFromArray(RSMutableArrayRef runLoopsAndModes, RSRunLoopRef rl, RSStringRef mode) {
    RSIndex idx, cnt;
    BOOL found = FALSE;
    
    if (!runLoopsAndModes) return FALSE;
    
    checkRLMArray(runLoopsAndModes);
    
    cnt = RSArrayGetCount(runLoopsAndModes);
    for (idx = 0; idx + 1 < cnt; idx += 2) {
        if (RSEqual(RSArrayObjectAtIndex(runLoopsAndModes, idx), rl) && RSEqual(RSArrayObjectAtIndex(runLoopsAndModes, idx + 1), mode)) {
            RSArrayRemoveObjectAtIndex(runLoopsAndModes, idx);
            RSArrayRemoveObjectAtIndex(runLoopsAndModes, idx);
            found = TRUE;
            break;
        }
    }
    return found;
}

// Used by NSStream to properly allocate the bridged objects
RSExport RSIndex _RSStreamInstanceSize(void) {
    return sizeof(struct _RSStream);
}

#if DEPLOYMENT_TARGET_WINDOWS
void __RSStreamCleanup(void) {
    RSSpinLockLock(&sSourceLock);
    if (sSharedSources) {
        RSIndex count = RSDictionaryGetCount(sSharedSources);
        if (count == 0) {
            // Only release if empty.  If it's still holding streams (which would be a client
            // bug leak), freeing this dict would free the streams, which then need to access the
            // dict to remove themselves, which leads to a deadlock.
            RSRelease(sSharedSources);
            sSharedSources = NULL;
        } else {
            const void ** keys = (const void **)malloc(sizeof(const void *) * count);
#if defined(DEBUG)
            int i;
#endif
            RSDictionaryGetKeysAndValues(sSharedSources, keys, NULL);
            fprintf(stderr, "*** RSNetwork is shutting down, but %ld streams are still scheduled.\n", count);
#if defined(DEBUG)
            for (i = 0; i < count;i ++) {
                if ((RSGetTypeID(keys[i]) == __RSReadStreamTypeID) || (RSGetTypeID(keys[i]) == __RSWriteStreamTypeID)) {
                    RSShow(keys[i]);
                }
            }
#endif
        }
    }
    RSSpinLockUnlock(&sSourceLock);
}
#endif
