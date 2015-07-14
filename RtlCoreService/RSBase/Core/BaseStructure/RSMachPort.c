/*
 * Copyright (c) 2013 Apple Inc. All rights reserved.
 *
 * @APPLE_LICENSE_HEADER_START@
 *
 * This file contains Original Code and/or Modifications of Original Code
 * as defined in and that are subject to the Apple Public Source License
 * Version 2.0 (the 'License'). You may not use this file except in
 * compliance with the License. Please obtain a copy of the License at
 * http://www.opensource.apple.com/apsl/ and read it before using this
 * file.
 *
 * The Original Code and all software distributed under the License are
 * distributed on an 'AS IS' basis, WITHOUT WARRANTY OF ANY KIND, EITHER
 * EXPRESS OR IMPLIED, AND APPLE HEREBY DISCLAIMS ALL SUCH WARRANTIES,
 * INCLUDING WITHOUT LIMITATION, ANY WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE, QUIET ENJOYMENT OR NON-INFRINGEMENT.
 * Please see the License for the specific language governing rights and
 * limitations under the License.
 *
 * @APPLE_LICENSE_HEADER_END@
 */

/*	RSMachPort.c
	Copyright (c) 1998-2013, Apple Inc. All rights reserved.
	Responsibility: Christopher Kane
 */

#include <RSCoreFoundation/RSMachPort.h>
#include <RSCoreFoundation/RSRunLoop.h>
#include <RSCoreFoundation/RSArray.h>
#include <dispatch/dispatch.h>
//#include <dispatch/private.h>
#include <mach/mach.h>
#include <dlfcn.h>
#include <stdio.h>
#include "RSInternal.h"


#define AVOID_WEAK_COLLECTIONS 1

#if !AVOID_WEAK_COLLECTIONS
#import "RSPointerArray.h"
#endif

static dispatch_queue_t _RSMachPortQueue() {
    static volatile dispatch_queue_t __RSMachPortQueue = NULL;
    static dispatch_once_t onceToken;
    dispatch_once(&onceToken, ^{ __RSMachPortQueue = dispatch_queue_create("RSMachPort Queue", NULL); });
    return __RSMachPortQueue;
}


enum {
    RSMachPortStateReady = 0,
    RSMachPortStateInvalidating = 1,
    RSMachPortStateInvalid = 2,
    RSMachPortStateDeallocating = 3
};

struct __RSMachPort {
    RSRuntimeBase _base;
    int32_t _state;
    mach_port_t _port;                          /* immutable */
    dispatch_source_t _dsrc;                    /* protected by _lock */
    dispatch_semaphore_t _dsrc_sem;             /* protected by _lock */
    RSMachPortInvalidationCallBack _icallout;   /* protected by _lock */
    RSRunLoopSourceRef _source;                 /* immutable, once created */
    RSMachPortCallBack _callout;                /* immutable */
    RSMachPortContext _context;                 /* immutable */
    RSSpinLock _lock;
    const void *(*retain)(const void *info); // use these to store the real callbacks
    void        (*release)(const void *info);
};

/* Bit 1 in the base reserved bits is used for has-receive-ref state */
/* Bit 2 in the base reserved bits is used for has-send-ref state */
/* Bit 3 in the base reserved bits is used for has-send-ref2 state */

RSInline BOOL __RSMachPortHasReceive(RSMachPortRef mp) {
    return (BOOL)__RSBitfieldGetValue(RSRuntimeClassBaseFiled(mp), 1, 1);
}

RSInline void __RSMachPortSetHasReceive(RSMachPortRef mp) {
    __RSBitfieldSetValue(RSRuntimeClassBaseFiled(mp), 1, 1, 1);
}

RSInline BOOL __RSMachPortHasSend(RSMachPortRef mp) {
    return (BOOL)__RSBitfieldGetValue(RSRuntimeClassBaseFiled(mp), 2, 2);
}

RSInline void __RSMachPortSetHasSend(RSMachPortRef mp) {
    __RSBitfieldSetValue(RSRuntimeClassBaseFiled(mp), 2, 2, 1);
}

RSInline BOOL __RSMachPortHasSend2(RSMachPortRef mp) {
    return (BOOL)__RSBitfieldGetValue(RSRuntimeClassBaseFiled(mp), 3, 3);
}

RSInline void __RSMachPortSetHasSend2(RSMachPortRef mp) {
    __RSBitfieldSetValue(RSRuntimeClassBaseFiled(mp), 3, 3, 1);
}

RSInline BOOL __RSMachPortIsValid(RSMachPortRef mp) {
    return RSMachPortStateReady == mp->_state;
}


void _RSMachPortInstallNotifyPort(RSRunLoopRef rl, RSStringRef mode) {
}

static BOOL __RSMachPortEqual(RSTypeRef cf1, RSTypeRef cf2) {
    RSMachPortRef mp1 = (RSMachPortRef)cf1;
    RSMachPortRef mp2 = (RSMachPortRef)cf2;
    return (mp1->_port == mp2->_port);
}

static RSHashCode __RSMachPortHash(RSTypeRef cf) {
    RSMachPortRef mp = (RSMachPortRef)cf;
    return (RSHashCode)mp->_port;
}

static RSStringRef __RSMachPortCopyDescription(RSTypeRef cf) {
    RSMachPortRef mp = (RSMachPortRef)cf;
    RSStringRef contextDesc = NULL;
    if (NULL != mp->_context.info && NULL != mp->_context.copyDescription) {
        contextDesc = mp->_context.copyDescription(mp->_context.info);
    }
    if (NULL == contextDesc) {
        contextDesc = RSStringCreateWithFormat(RSAllocatorSystemDefault, NULL, RSSTR("<RSMachPort context %p>"), mp->_context.info);
    }
    Dl_info info;
    void *addr = mp->_callout;
    const char *name = (dladdr(addr, &info) && info.dli_saddr == addr && info.dli_sname) ? info.dli_sname : "???";
    RSStringRef result = RSStringCreateWithFormat(RSAllocatorSystemDefault, NULL, RSSTR("<RSMachPort %p [%p]>{valid = %s, port = %x, source = %p, callout = %s (%p), context = %@}"), cf, RSGetAllocator(mp), (__RSMachPortIsValid(mp) ? "Yes" : "No"), mp->_port, mp->_source, name, addr, contextDesc);
    if (NULL != contextDesc) {
        RSRelease(contextDesc);
    }
    return result;
}

// Only call with mp->_lock locked
RSInline void __RSMachPortInvalidateLocked(RSRunLoopSourceRef source, RSMachPortRef mp) {
    RSMachPortInvalidationCallBack cb = mp->_icallout;
    if (cb) {
        RSSpinLockUnlock(&mp->_lock);
        cb(mp, mp->_context.info);
        RSSpinLockLock(&mp->_lock);
    }
    if (NULL != source) {
        RSSpinLockUnlock(&mp->_lock);
        RSRunLoopSourceInvalidate(source);
        RSRelease(source);
        RSSpinLockLock(&mp->_lock);
    }
    void *info = mp->_context.info;
    void (*release)(const void *info) = mp->release;
    mp->_context.info = NULL;
    if (release) {
        RSSpinLockUnlock(&mp->_lock);
        release(info);
        RSSpinLockLock(&mp->_lock);
    }
    mp->_state = RSMachPortStateInvalid;
    OSMemoryBarrier();
}

static void __RSMachPortDeallocate(RSTypeRef cf) {
    CHECK_FOR_FORK_RET();
    RSMachPortRef mp = (RSMachPortRef)cf;
    
    // RSMachPortRef is invalid before we get here, except under GC
    RSSpinLockLock(&mp->_lock);
    RSRunLoopSourceRef source = NULL;
    BOOL wasReady = (mp->_state == RSMachPortStateReady);
    if (wasReady) {
        mp->_state = RSMachPortStateInvalidating;
        OSMemoryBarrier();
        if (mp->_dsrc) {
            dispatch_source_cancel(mp->_dsrc);
            mp->_dsrc = NULL;
        }
        source = mp->_source;
        mp->_source = NULL;
    }
    if (wasReady) {
        __RSMachPortInvalidateLocked(source, mp);
    }
    mp->_state = RSMachPortStateDeallocating;
    
    // hand ownership of the port and semaphores to the block below
    mach_port_t port = mp->_port;
    dispatch_semaphore_t sem1 = mp->_dsrc_sem;
    BOOL doSend2 = __RSMachPortHasSend2(mp), doSend = __RSMachPortHasSend(mp), doReceive = __RSMachPortHasReceive(mp);
    
    RSSpinLockUnlock(&mp->_lock);
    
    dispatch_async(dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_LOW, 0), ^{
        if (sem1) {
            dispatch_semaphore_wait(sem1, DISPATCH_TIME_FOREVER);
            // immediate release is only safe if dispatch_semaphore_signal() does not touch the semaphore after doing the signal bit
            dispatch_release(sem1);
        }
        
        // MUST deallocate the send right FIRST if necessary,
        // then the receive right if necessary.  Don't ask me why;
        // if it's done in the other order the port will leak.
        if (doSend2) {
            mach_port_mod_refs(mach_task_self(), port, MACH_PORT_RIGHT_SEND, -1);
        }
        if (doSend) {
            mach_port_mod_refs(mach_task_self(), port, MACH_PORT_RIGHT_SEND, -1);
        }
        if (doReceive) {
            mach_port_mod_refs(mach_task_self(), port, MACH_PORT_RIGHT_RECEIVE, -1);
        }
    });
}

// This lock protects __RSAllMachPorts. Take before any instance-specific lock.
static RSSpinLock __RSAllMachPortsLock = RSSpinLockInit;

#if AVOID_WEAK_COLLECTIONS
static RSMutableArrayRef __RSAllMachPorts = NULL;
#else
static __RSPointerArray *__RSAllMachPorts = nil;
#endif

static BOOL __RSMachPortCheck(mach_port_t) __attribute__((noinline));
static BOOL __RSMachPortCheck(mach_port_t port) {
    mach_port_type_t type = 0;
    kern_return_t ret = mach_port_type(mach_task_self(), port, &type);
    return (KERN_SUCCESS != ret || (0 == (type & MACH_PORT_TYPE_PORT_RIGHTS))) ? false : true;
}


#define DISPATCH_SOURCE_TYPE_INTERVAL (&_dispatch_source_type_interval)
__OSX_AVAILABLE_STARTING(__MAC_10_9,__IPHONE_7_0)
DISPATCH_SOURCE_TYPE_DECL(interval);

static void __RSMachPortChecker(BOOL fromTimer) {
    RSSpinLockLock(&__RSAllMachPortsLock); // take this lock first before any instance-specific lock
#if AVOID_WEAK_COLLECTIONS
    for (RSIndex idx = 0, cnt = __RSAllMachPorts ? RSArrayGetCount(__RSAllMachPorts) : 0; idx < cnt; idx++) {
        RSMachPortRef mp = (RSMachPortRef)RSArrayObjectAtIndex(__RSAllMachPorts, idx);
#else
        for (RSIndex idx = 0, cnt = __RSAllMachPorts ? [__RSAllMachPorts count] : 0; idx < cnt; idx++) {
            RSMachPortRef mp = (RSMachPortRef)[__RSAllMachPorts pointerAtIndex:idx];
#endif
            if (!mp) continue;
            // second clause cleans no-longer-wanted RSMachPorts out of our strong table
            if (!__RSMachPortCheck(mp->_port) || (!RSUseCollectableAllocator && 1 == RSGetRetainCount(mp))) {
                RSRunLoopSourceRef source = NULL;
                BOOL wasReady = (mp->_state == RSMachPortStateReady);
                if (wasReady) {
                    RSSpinLockLock(&mp->_lock); // take this lock second
                    mp->_state = RSMachPortStateInvalidating;
                    OSMemoryBarrier();
                    if (mp->_dsrc) {
                        dispatch_source_cancel(mp->_dsrc);
                        mp->_dsrc = NULL;
                    }
                    source = mp->_source;
                    mp->_source = NULL;
                    RSRetain(mp);
                    RSSpinLockUnlock(&mp->_lock);
                    dispatch_async(dispatch_get_main_queue(), ^{
                        // We can grab the mach port-specific spin lock here since we're no longer on the same thread as the one taking the all mach ports spin lock.
                        // But be sure to release it during callouts
                        RSSpinLockLock(&mp->_lock);
                        __RSMachPortInvalidateLocked(source, mp);
                        RSSpinLockUnlock(&mp->_lock);
                        RSRelease(mp);
                    });
                }
#if AVOID_WEAK_COLLECTIONS
                RSArrayRemoveObjectAtIndex(__RSAllMachPorts, idx);
#else
                [__RSAllMachPorts removePointerAtIndex:idx];
#endif
                idx--;
                cnt--;
            }
        }
#if !AVOID_WEAK_COLLECTIONS
        [__RSAllMachPorts compact];
#endif
        RSSpinLockUnlock(&__RSAllMachPortsLock);
    };
    
    
    static RSTypeID __RSMachPortTypeID = _RSRuntimeNotATypeID;
    
    static const RSRuntimeClass __RSMachPortClass = {
        _RSRuntimeScannedObject,
        0,
        "RSMachPort",
        NULL,      // init
        NULL,      // copy
        __RSMachPortDeallocate,
        __RSMachPortEqual,
        __RSMachPortHash,
        __RSMachPortCopyDescription,      //
        NULL,
    };
    
    RSPrivate void __RSMachPortInitialize(void) {
        __RSMachPortTypeID = __RSRuntimeRegisterClass(&__RSMachPortClass);
    }
    
    RSTypeID RSMachPortGetTypeID(void) {
        return __RSMachPortTypeID;
    }
    
    /* Note: any receive or send rights that the port contains coming in will
     * not be cleaned up by RSMachPort; it will increment and decrement
     * references on the port if the kernel ever allows that in the future,
     * but will not cleanup any references you got when you got the port. */
    RSMachPortRef _RSMachPortCreateWithPort2(RSAllocatorRef allocator, mach_port_t port, RSMachPortCallBack callout, RSMachPortContext *context, BOOL *shouldFreeInfo, BOOL deathWatch) {
        if (shouldFreeInfo) *shouldFreeInfo = true;
        CHECK_FOR_FORK_RET(NULL);
        
        mach_port_type_t type = 0;
        kern_return_t ret = mach_port_type(mach_task_self(), port, &type);
        if (KERN_SUCCESS != ret || (0 == (type & MACH_PORT_TYPE_PORT_RIGHTS))) {
            if (type & ~MACH_PORT_TYPE_DEAD_NAME) {
                RSLog(RSLogLevelError, RSSTR("*** RSMachPortCreateWithPort(): bad Mach port parameter (0x%lx) or unsupported mysterious kind of Mach port (%d, %ld)"), (unsigned long)port, ret, (unsigned long)type);
            }
            return NULL;
        }
        
        static dispatch_source_t timerSource = NULL;
        static dispatch_once_t onceToken;
        dispatch_once(&onceToken, ^{
            timerSource = dispatch_source_create(DISPATCH_SOURCE_TYPE_INTERVAL, 60 * 1000 /* milliseconds */, 0, _RSMachPortQueue());
            dispatch_source_set_event_handler(timerSource, ^{
                __RSMachPortChecker(true);
            });
            dispatch_resume(timerSource);
        });
        
        RSMachPortRef mp = NULL;
        RSSpinLockLock(&__RSAllMachPortsLock);
#if AVOID_WEAK_COLLECTIONS
        for (RSIndex idx = 0, cnt = __RSAllMachPorts ? RSArrayGetCount(__RSAllMachPorts) : 0; idx < cnt; idx++) {
            RSMachPortRef p = (RSMachPortRef)RSArrayObjectAtIndex(__RSAllMachPorts, idx);
            if (p && p->_port == port) {
                RSRetain(p);
                mp = p;
                break;
            }
        }
#else
        for (RSIndex idx = 0, cnt = __RSAllMachPorts ? [__RSAllMachPorts count] : 0; idx < cnt; idx++) {
            RSMachPortRef p = (RSMachPortRef)[__RSAllMachPorts pointerAtIndex:idx];
            if (p && p->_port == port) {
                RSRetain(p);
                mp = p;
                break;
            }
        }
#endif
        RSSpinLockUnlock(&__RSAllMachPortsLock);
        
        if (!mp) {
            RSIndex size = sizeof(struct __RSMachPort) - sizeof(RSRuntimeBase);
            RSMachPortRef memory = (RSMachPortRef)__RSRuntimeCreateInstance(allocator, RSMachPortGetTypeID(), size);
            if (NULL == memory) {
                return NULL;
            }
            memory->_port = port;
            memory->_dsrc = NULL;
            memory->_dsrc_sem = NULL;
            memory->_icallout = NULL;
            memory->_source = NULL;
            memory->_context.info = NULL;
            memory->_context.retain = NULL;
            memory->_context.release = NULL;
            memory->_context.copyDescription = NULL;
            memory->retain = NULL;
            memory->release = NULL;
            memory->_callout = callout;
            memory->_lock = RSSpinLockInit;
            if (NULL != context) {
                memmove(&memory->_context, context, sizeof(RSMachPortContext));
                //            objc_memmove_collectable(&memory->_context, context, sizeof(RSMachPortContext));
                memory->_context.info = context->retain ? (void *)context->retain(context->info) : context->info;
                memory->retain = context->retain;
                memory->release = context->release;
                memory->_context.retain = (void *)0xAAAAAAAAAACCCAAA;
                memory->_context.release = (void *)0xAAAAAAAAAABBBAAA;
            }
            memory->_state = RSMachPortStateReady;
            RSSpinLockLock(&__RSAllMachPortsLock);
#if AVOID_WEAK_COLLECTIONS
            if (!__RSAllMachPorts) __RSAllMachPorts = RSArrayCreateMutable(RSAllocatorSystemDefault, 0);
            RSArrayAddObject(__RSAllMachPorts, memory);
#else
            if (!__RSAllMachPorts) __RSAllMachPorts = [[__RSPointerArray alloc] initWithOptions:(RSUseCollectableAllocator ? RSPointerFunctionsZeroingWeakMemory : RSPointerFunctionsStrongMemory)];
            [__RSAllMachPorts addPointer:memory];
#endif
            RSSpinLockUnlock(&__RSAllMachPortsLock);
            mp = memory;
            if (shouldFreeInfo) *shouldFreeInfo = false;
            
            if (type & MACH_PORT_TYPE_SEND_RIGHTS) {
                dispatch_source_t theSource = dispatch_source_create(DISPATCH_SOURCE_TYPE_MACH_SEND, port, DISPATCH_MACH_SEND_DEAD, _RSMachPortQueue());
                if (theSource) {
                    dispatch_semaphore_t sem = dispatch_semaphore_create(0);
                    dispatch_source_set_cancel_handler(theSource, ^{ dispatch_semaphore_signal(sem); dispatch_release(theSource); });
                    dispatch_source_set_event_handler(theSource, ^{ __RSMachPortChecker(false); });
                    memory->_dsrc_sem = sem;
                    memory->_dsrc = theSource;
                    dispatch_resume(theSource);
                }
            }
        }
        
        if (mp && !RSMachPortIsValid(mp)) { // must do this outside lock to avoid deadlock
            RSRelease(mp);
            mp = NULL;
        }
        return mp;
    }
    
    RSMachPortRef RSMachPortCreateWithPort(RSAllocatorRef allocator, mach_port_t port, RSMachPortCallBack callout, RSMachPortContext *context, BOOL *shouldFreeInfo) {
        return _RSMachPortCreateWithPort2(allocator, port, callout, context, shouldFreeInfo, true);
    }
    
    RSMachPortRef RSMachPortCreate(RSAllocatorRef allocator, RSMachPortCallBack callout, RSMachPortContext *context, BOOL *shouldFreeInfo) {
        if (shouldFreeInfo) *shouldFreeInfo = true;
        CHECK_FOR_FORK_RET(NULL);
        mach_port_t port = MACH_PORT_NULL;
        kern_return_t ret = mach_port_allocate(mach_task_self(), MACH_PORT_RIGHT_RECEIVE, &port);
        if (KERN_SUCCESS == ret) {
            ret = mach_port_insert_right(mach_task_self(), port, port, MACH_MSG_TYPE_MAKE_SEND);
        }
        if (KERN_SUCCESS != ret) {
            if (MACH_PORT_NULL != port) mach_port_destroy(mach_task_self(), port);
            return NULL;
        }
        RSMachPortRef result = _RSMachPortCreateWithPort2(allocator, port, callout, context, shouldFreeInfo, true);
        if (NULL == result) {
            if (MACH_PORT_NULL != port) mach_port_destroy(mach_task_self(), port);
            return NULL;
        }
        __RSMachPortSetHasReceive(result);
        __RSMachPortSetHasSend(result);
        return result;
    }
    
    void RSMachPortInvalidate(RSMachPortRef mp) {
        CHECK_FOR_FORK_RET();
        RS_OBJC_FUNCDISPATCHV(RSMachPortGetTypeID(), void, (NSMachPort *)mp, invalidate);
        __RSGenericValidInstance(mp, RSMachPortGetTypeID());
        RSRetain(mp);
        RSRunLoopSourceRef source = NULL;
        BOOL wasReady = false;
        RSSpinLockLock(&__RSAllMachPortsLock); // take this lock first
        RSSpinLockLock(&mp->_lock);
        wasReady = (mp->_state == RSMachPortStateReady);
        if (wasReady) {
            mp->_state = RSMachPortStateInvalidating;
            OSMemoryBarrier();
#if AVOID_WEAK_COLLECTIONS
            for (RSIndex idx = 0, cnt = __RSAllMachPorts ? RSArrayGetCount(__RSAllMachPorts) : 0; idx < cnt; idx++) {
                RSMachPortRef p = (RSMachPortRef)RSArrayObjectAtIndex(__RSAllMachPorts, idx);
                if (p == mp) {
                    RSArrayRemoveObjectAtIndex(__RSAllMachPorts, idx);
                    break;
                }
            }
#else
            for (RSIndex idx = 0, cnt = __RSAllMachPorts ? [__RSAllMachPorts count] : 0; idx < cnt; idx++) {
                RSMachPortRef p = (RSMachPortRef)[__RSAllMachPorts pointerAtIndex:idx];
                if (p == mp) {
                    [__RSAllMachPorts removePointerAtIndex:idx];
                    break;
                }
            }
#endif
            if (mp->_dsrc) {
                dispatch_source_cancel(mp->_dsrc);
                mp->_dsrc = NULL;
            }
            source = mp->_source;
            mp->_source = NULL;
        }
        RSSpinLockUnlock(&mp->_lock);
        RSSpinLockUnlock(&__RSAllMachPortsLock); // release this lock last
        if (wasReady) {
            RSSpinLockLock(&mp->_lock);
            __RSMachPortInvalidateLocked(source, mp);
            RSSpinLockUnlock(&mp->_lock);
        }
        RSRelease(mp);
    }
    
    mach_port_t RSMachPortGetPort(RSMachPortRef mp) {
        CHECK_FOR_FORK_RET(0);
        RS_OBJC_FUNCDISPATCHV(RSMachPortGetTypeID(), mach_port_t, (NSMachPort *)mp, machPort);
        __RSGenericValidInstance(mp, RSMachPortGetTypeID());
        return mp->_port;
    }
    
    void RSMachPortGetContext(RSMachPortRef mp, RSMachPortContext *context) {
        __RSGenericValidInstance(mp, RSMachPortGetTypeID());
        RSAssert1(0 == context->version, __RSLogAssertion, "%s(): context version not initialized to 0", __PRETTY_FUNCTION__);
        //    objc_memmove_collectable(context, &mp->_context, sizeof(RSMachPortContext));
        memmove(context, &mp->_context, sizeof(RSMachPortContext));
    }
    
    BOOL RSMachPortIsValid(RSMachPortRef mp) {
        RS_OBJC_FUNCDISPATCHV(RSMachPortGetTypeID(), BOOL, (NSMachPort *)mp, isValid);
        __RSGenericValidInstance(mp, RSMachPortGetTypeID());
        if (!__RSMachPortIsValid(mp)) return false;
        mach_port_type_t type = 0;
        kern_return_t ret = mach_port_type(mach_task_self(), mp->_port, &type);
        if (KERN_SUCCESS != ret || (type & ~(MACH_PORT_TYPE_SEND|MACH_PORT_TYPE_SEND_ONCE|MACH_PORT_TYPE_RECEIVE|MACH_PORT_TYPE_DNREQUEST))) {
            return false;
        }
        return true;
    }
    
    RSMachPortInvalidationCallBack RSMachPortGetInvalidationCallBack(RSMachPortRef mp) {
        __RSGenericValidInstance(mp, RSMachPortGetTypeID());
        RSSpinLockLock(&mp->_lock);
        RSMachPortInvalidationCallBack cb = mp->_icallout;
        RSSpinLockUnlock(&mp->_lock);
        return cb;
    }
    
    /* After the RSMachPort has started going invalid, or done invalid, you can't change this, and
     we'll only do the callout directly on a transition from NULL to non-NULL. */
    void RSMachPortSetInvalidationCallBack(RSMachPortRef mp, RSMachPortInvalidationCallBack callout) {
        CHECK_FOR_FORK_RET();
        __RSGenericValidInstance(mp, RSMachPortGetTypeID());
        if (callout) {
            mach_port_type_t type = 0;
            kern_return_t ret = mach_port_type(mach_task_self(), mp->_port, &type);
            if (KERN_SUCCESS != ret || 0 == (type & MACH_PORT_TYPE_SEND_RIGHTS)) {
                RSLog(RSLogLevelError, RSSTR("*** WARNING: RSMachPortSetInvalidationCallBack() called on a RSMachPort with a Mach port (0x%x) which does not have any send rights.  This is not going to work.  Callback function: %p"), mp->_port, callout);
            }
        }
        RSSpinLockLock(&mp->_lock);
        if (__RSMachPortIsValid(mp) || !callout) {
            mp->_icallout = callout;
        } else if (!mp->_icallout && callout) {
            RSSpinLockUnlock(&mp->_lock);
            callout(mp, mp->_context.info);
            RSSpinLockLock(&mp->_lock);
        } else {
            RSLog(RSLogLevelWarning, RSSTR("RSMachPortSetInvalidationCallBack(): attempt to set invalidation callback (%p) on invalid RSMachPort (%p) thwarted"), callout, mp);
        }
        RSSpinLockUnlock(&mp->_lock);
    }
    
    /* Returns the number of messages queued for a receive port. */
    RSIndex RSMachPortGetQueuedMessageCount(RSMachPortRef mp) {
        CHECK_FOR_FORK_RET(0);
        __RSGenericValidInstance(mp, RSMachPortGetTypeID());
        mach_port_status_t status;
        mach_msg_type_number_t num = MACH_PORT_RECEIVE_STATUS_COUNT;
        kern_return_t ret = mach_port_get_attributes(mach_task_self(), mp->_port, MACH_PORT_RECEIVE_STATUS, (mach_port_info_t)&status, &num);
        return (KERN_SUCCESS != ret) ? 0 : status.mps_msgcount;
    }
    
    static mach_port_t __RSMachPortGetPort(void *info) {
        RSMachPortRef mp = (RSMachPortRef)info;
        return mp->_port;
    }
    
    RSPrivate void *__RSMachPortPerform(void *msg, RSIndex size, RSAllocatorRef allocator, void *info) {
        CHECK_FOR_FORK_RET(NULL);
        RSMachPortRef mp = (RSMachPortRef)info;
        RSSpinLockLock(&mp->_lock);
        BOOL isValid = __RSMachPortIsValid(mp);
        void *context_info = NULL;
        void (*context_release)(const void *) = NULL;
        if (isValid) {
            if (mp->retain) {
                context_info = (void *)mp->retain(mp->_context.info);
                context_release = mp->release;
            } else {
                context_info = mp->_context.info;
            }
        }
        RSSpinLockUnlock(&mp->_lock);
        if (isValid) {
            mp->_callout(mp, msg, size, context_info);
            
            if (context_release) {
                context_release(context_info);
            }
            CHECK_FOR_FORK_RET(NULL);
        }
        return NULL;
    }
    
    
    
    RSRunLoopSourceRef RSMachPortCreateRunLoopSource(RSAllocatorRef allocator, RSMachPortRef mp, RSIndex order) {
        CHECK_FOR_FORK_RET(NULL);
        __RSGenericValidInstance(mp, RSMachPortGetTypeID());
        if (!RSMachPortIsValid(mp)) return NULL;
        RSRunLoopSourceRef result = NULL;
        RSSpinLockLock(&mp->_lock);
        if (__RSMachPortIsValid(mp)) {
            if (NULL != mp->_source && !RSRunLoopSourceIsValid(mp->_source)) {
                RSRelease(mp->_source);
                mp->_source = NULL;
            }
            if (NULL == mp->_source) {
                RSRunLoopSourceContext1 context;
                context.version = 1;
                context.info = (void *)mp;
                context.retain = (const void *(*)(const void *))RSRetain;
                context.release = (void (*)(const void *))RSRelease;
                context.description = (RSStringRef (*)(const void *))__RSMachPortCopyDescription;
                context.equal = (BOOL (*)(const void *, const void *))__RSMachPortEqual;
                context.hash = (RSHashCode (*)(const void *))__RSMachPortHash;
                context.getPort = __RSMachPortGetPort;
                context.perform = __RSMachPortPerform;
                mp->_source = RSRunLoopSourceCreate(allocator, order, (RSRunLoopSourceContext *)&context);
            }
            result = mp->_source ? (RSRunLoopSourceRef)RSRetain(mp->_source) : NULL;
        }
        RSSpinLockUnlock(&mp->_lock);
        return result;
    }
    
