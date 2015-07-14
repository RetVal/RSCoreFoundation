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

/*	RSMessagePort.c
	Copyright (c) 1998-2013, Apple Inc. All rights reserved.
	Responsibility: Christopher Kane
 */

#include <RSCoreFoundation/RSMessagePort.h>
#include <RSCoreFoundation/RSRunLoop.h>
#include <RSCoreFoundation/RSMachPort.h>
#include <RSCoreFoundation/RSDictionary.h>
#include <RSCoreFoundation/RSByteOrder.h>
#include <limits.h>
#include "RSInternal.h"
#include <mach/mach.h>
#include <mach/message.h>
#include <mach/mach_error.h>
#include <bootstrap_priv.h>
#include <math.h>
#include <mach/mach_time.h>
#include <dlfcn.h>
#include <dispatch/dispatch.h>
//#include <dispatch/private.h>

extern pid_t getpid(void);

#define __RSMessagePortMaxNameLengthMax 255

#if defined(BOOTSTRAP_MAX_NAME_LEN)
#define __RSMessagePortMaxNameLength BOOTSTRAP_MAX_NAME_LEN
#else
#define __RSMessagePortMaxNameLength 128
#endif

#if __RSMessagePortMaxNameLengthMax < __RSMessagePortMaxNameLength
#undef __RSMessagePortMaxNameLength
#define __RSMessagePortMaxNameLength __RSMessagePortMaxNameLengthMax
#endif

#define __RSMessagePortMaxDataSize 0x60000000L

static RSSpinLock __RSAllMessagePortsLock = RSSpinLockInit;
static RSMutableDictionaryRef __RSAllLocalMessagePorts = NULL;
static RSMutableDictionaryRef __RSAllRemoteMessagePorts = NULL;

struct __RSMessagePort {
    RSRuntimeBase _base;
    RSSpinLock _lock;
    RSStringRef _name;
    RSMachPortRef _port;		/* immutable; invalidated */
    RSMutableDictionaryRef _replies;
    int32_t _convCounter;
    int32_t _perPID;			/* zero if not per-pid, else pid */
    RSMachPortRef _replyPort;		/* only used by remote port; immutable once created; invalidated */
    RSRunLoopSourceRef _source;		/* only used by local port; immutable once created; invalidated */
    dispatch_source_t _dispatchSource;  /* only used by local port; invalidated */
    dispatch_queue_t _dispatchQ;	/* only used by local port */
    RSMessagePortInvalidationCallBack _icallout;
    RSMessagePortCallBack _callout;	/* only used by local port; immutable */
    RSMessagePortCallBackEx _calloutEx;	/* only used by local port; immutable */
    RSMessagePortContext _context;	/* not part of remote port; immutable; invalidated */
};

/* Bit 0 in the base reserved bits is used for invalid state */
/* Bit 1 of the base reserved bits is used for has-extra-port-refs state */
/* Bit 2 of the base reserved bits is used for is-remote state */
/* Bit 3 in the base reserved bits is used for is-deallocing state */

RSInline BOOL __RSMessagePortIsValid(RSMessagePortRef ms) {
    return (BOOL)__RSBitfieldGetValue(RSRuntimeClassBaseFiled(ms), 0, 0);
}

RSInline void __RSMessagePortSetValid(RSMessagePortRef ms) {
    __RSBitfieldSetValue(RSRuntimeClassBaseFiled(ms), 0, 0, 1);
}

RSInline void __RSMessagePortUnsetValid(RSMessagePortRef ms) {
    __RSBitfieldSetValue(RSRuntimeClassBaseFiled(ms), 0, 0, 0);
}

RSInline BOOL __RSMessagePortExtraMachRef(RSMessagePortRef ms) {
    return (BOOL)__RSBitfieldGetValue(RSRuntimeClassBaseFiled(ms), 1, 1);
}

RSInline void __RSMessagePortSetExtraMachRef(RSMessagePortRef ms) {
    __RSBitfieldSetValue(RSRuntimeClassBaseFiled(ms), 1, 1, 1);
}

RSInline void __RSMessagePortUnsetExtraMachRef(RSMessagePortRef ms) {
    __RSBitfieldSetValue(RSRuntimeClassBaseFiled(ms), 1, 1, 0);
}

RSInline BOOL __RSMessagePortIsRemote(RSMessagePortRef ms) {
    return (BOOL)__RSBitfieldGetValue(RSRuntimeClassBaseFiled(ms), 2, 2);
}

RSInline void __RSMessagePortSetRemote(RSMessagePortRef ms) {
    __RSBitfieldSetValue(RSRuntimeClassBaseFiled(ms), 2, 2, 1);
}

RSInline void __RSMessagePortUnsetRemote(RSMessagePortRef ms) {
    __RSBitfieldSetValue(RSRuntimeClassBaseFiled(ms), 2, 2, 0);
}

RSInline BOOL __RSMessagePortIsDeallocing(RSMessagePortRef ms) {
    return (BOOL)__RSBitfieldGetValue(RSRuntimeClassBaseFiled(ms), 3, 3);
}

RSInline void __RSMessagePortSetIsDeallocing(RSMessagePortRef ms) {
    __RSBitfieldSetValue(RSRuntimeClassBaseFiled(ms), 3, 3, 1);
}

RSInline void __RSMessagePortLock(RSMessagePortRef ms) {
    RSSpinLockLock(&(ms->_lock));
}

RSInline void __RSMessagePortUnlock(RSMessagePortRef ms) {
    RSSpinLockUnlock(&(ms->_lock));
}

// Just a heuristic
#define __RSMessagePortMaxInlineBytes 4096*10

struct __RSMessagePortMachMessage0 {
    mach_msg_base_t base;
    int32_t magic;
    int32_t msgid;
    int32_t byteslen;
    uint8_t bytes[0];
};

struct __RSMessagePortMachMessage1 {
    mach_msg_base_t base;
    mach_msg_ool_descriptor_t ool;
    int32_t magic;
    int32_t msgid;
    int32_t byteslen;
};

#define MAGIC 0xF1F2F3F4

#define MSGP0_FIELD(msgp, ident) ((struct __RSMessagePortMachMessage0 *)msgp)->ident
#define MSGP1_FIELD(msgp, ident) ((struct __RSMessagePortMachMessage1 *)msgp)->ident
#define MSGP_GET(msgp, ident) \
((((mach_msg_base_t *)msgp)->body.msgh_descriptor_count) ? MSGP1_FIELD(msgp, ident) : MSGP0_FIELD(msgp, ident))

static mach_msg_base_t *__RSMessagePortCreateMessage(bool reply, mach_port_t port, mach_port_t replyPort, int32_t convid, int32_t msgid, const uint8_t *bytes, int32_t byteslen) {
    if (__RSMessagePortMaxDataSize < byteslen) return NULL;
    int32_t rounded_byteslen = ((byteslen + 3) & ~0x3);
    if (rounded_byteslen <= __RSMessagePortMaxInlineBytes) {
        int32_t size = sizeof(struct __RSMessagePortMachMessage0) + rounded_byteslen;
        struct __RSMessagePortMachMessage0 *msg = RSAllocatorAllocate(RSAllocatorSystemDefault, size);
        if (!msg) return NULL;
        memset(msg, 0, size);
        msg->base.header.msgh_id = convid;
        msg->base.header.msgh_size = size;
        msg->base.header.msgh_remote_port = port;
        msg->base.header.msgh_local_port = replyPort;
        msg->base.header.msgh_reserved = 0;
        msg->base.header.msgh_bits = MACH_MSGH_BITS((reply ? MACH_MSG_TYPE_MOVE_SEND_ONCE : MACH_MSG_TYPE_COPY_SEND), (MACH_PORT_NULL != replyPort ? MACH_MSG_TYPE_MAKE_SEND_ONCE : 0));
        msg->base.body.msgh_descriptor_count = 0;
        msg->magic = MAGIC;
        msg->msgid = RSSwapInt32HostToLittle(msgid);
        msg->byteslen = RSSwapInt32HostToLittle(byteslen);
        if (NULL != bytes && 0 < byteslen) {
            memmove(msg->bytes, bytes, byteslen);
        }
        return (mach_msg_base_t *)msg;
    } else {
        int32_t size = sizeof(struct __RSMessagePortMachMessage1);
        struct __RSMessagePortMachMessage1 *msg = RSAllocatorAllocate(RSAllocatorSystemDefault, size);
        if (!msg) return NULL;
        memset(msg, 0, size);
        msg->base.header.msgh_id = convid;
        msg->base.header.msgh_size = size;
        msg->base.header.msgh_remote_port = port;
        msg->base.header.msgh_local_port = replyPort;
        msg->base.header.msgh_reserved = 0;
        msg->base.header.msgh_bits = MACH_MSGH_BITS((reply ? MACH_MSG_TYPE_MOVE_SEND_ONCE : MACH_MSG_TYPE_COPY_SEND), (MACH_PORT_NULL != replyPort ? MACH_MSG_TYPE_MAKE_SEND_ONCE : 0));
        msg->base.header.msgh_bits |= MACH_MSGH_BITS_COMPLEX;
        msg->base.body.msgh_descriptor_count = 1;
        msg->magic = MAGIC;
        msg->msgid = RSSwapInt32HostToLittle(msgid);
        msg->byteslen = RSSwapInt32HostToLittle(byteslen);
        msg->ool.deallocate = false;
        msg->ool.copy = MACH_MSG_VIRTUAL_COPY;
        msg->ool.address = (void *)bytes;
        msg->ool.size = byteslen;
        msg->ool.type = MACH_MSG_OOL_DESCRIPTOR;
        return (mach_msg_base_t *)msg;
    }
}

static RSStringRef __RSMessagePortCopyDescription(RSTypeRef cf) {
    RSMessagePortRef ms = (RSMessagePortRef)cf;
    RSStringRef result;
    const char *locked;
    RSStringRef contextDesc = NULL;
    locked = ms->_lock ? "Yes" : "No";
    if (__RSMessagePortIsRemote(ms)) {
        result = RSStringCreateWithFormat(RSAllocatorSystemDefault, RSSTR("<RSMessagePort %p [%p]>{locked = %s, valid = %s, remote = %s, name = %@}"), cf, RSGetAllocator(ms), locked, (__RSMessagePortIsValid(ms) ? "Yes" : "No"), (__RSMessagePortIsRemote(ms) ? "Yes" : "No"), ms->_name);
    } else {
        if (NULL != ms->_context.info && NULL != ms->_context.copyDescription) {
            contextDesc = ms->_context.copyDescription(ms->_context.info);
        }
        if (NULL == contextDesc) {
            contextDesc = RSStringCreateWithFormat(RSAllocatorSystemDefault, RSSTR("<RSMessagePort context %p>"), ms->_context.info);
        }
        void *addr = ms->_callout ? (void *)ms->_callout : (void *)ms->_calloutEx;
        Dl_info info;
        const char *name = (dladdr(addr, &info) && info.dli_saddr == addr && info.dli_sname) ? info.dli_sname : "???";
        result = RSStringCreateWithFormat(RSAllocatorSystemDefault, RSSTR("<RSMessagePort %p [%p]>{locked = %s, valid = %s, remote = %s, name = %@, source = %p, callout = %s (%p), context = %@}"), cf, RSGetAllocator(ms), locked, (__RSMessagePortIsValid(ms) ? "Yes" : "No"), (__RSMessagePortIsRemote(ms) ? "Yes" : "No"), ms->_name, ms->_source, name, addr, (NULL != contextDesc ? contextDesc : RSSTR("<no description>")));
    }
    if (NULL != contextDesc) {
        RSRelease(contextDesc);
    }
    return result;
}

static void __RSMessagePortDeallocate(RSTypeRef cf) {
    RSMessagePortRef ms = (RSMessagePortRef)cf;
    __RSMessagePortSetIsDeallocing(ms);
    RSMessagePortInvalidate(ms);
    // Delay cleanup of _replies until here so that invalidation during
    // SendRequest does not cause _replies to disappear out from under that function.
    if (NULL != ms->_replies) {
        RSRelease(ms->_replies);
    }
    if (NULL != ms->_name) {
        RSRelease(ms->_name);
    }
    if (NULL != ms->_port) {
        if (__RSMessagePortExtraMachRef(ms)) {
            mach_port_mod_refs(mach_task_self(), RSMachPortGetPort(ms->_port), MACH_PORT_RIGHT_SEND, -1);
            mach_port_mod_refs(mach_task_self(), RSMachPortGetPort(ms->_port), MACH_PORT_RIGHT_RECEIVE, -1);
        }
        RSMachPortInvalidate(ms->_port);
        RSRelease(ms->_port);
    }
    
    // A remote message port for a local message port in the same process will get the
    // same mach port, and the remote port will keep the mach port from being torn down,
    // thus keeping the remote port from getting any sort of death notification and
    // auto-invalidating; so we manually implement the 'auto-invalidation' here by
    // tickling each remote port to check its state after any message port is destroyed,
    // but most importantly after local message ports are destroyed.
    RSSpinLockLock(&__RSAllMessagePortsLock);
    RSMessagePortRef *remotePorts = NULL;
    RSIndex cnt = 0;
    if (NULL != __RSAllRemoteMessagePorts) {
        cnt = RSDictionaryGetCount(__RSAllRemoteMessagePorts);
        remotePorts = RSAllocatorAllocate(RSAllocatorSystemDefault, cnt * sizeof(RSMessagePortRef));
        RSDictionaryGetKeysAndValues(__RSAllRemoteMessagePorts, NULL, (const void **)remotePorts);
        for (RSIndex idx = 0; idx < cnt; idx++) {
            RSRetain(remotePorts[idx]);
        }
    }
    RSSpinLockUnlock(&__RSAllMessagePortsLock);
    if (remotePorts) {
        for (RSIndex idx = 0; idx < cnt; idx++) {
            // as a side-effect, this will auto-invalidate the RSMessagePort if the RSMachPort is invalid
            RSMessagePortIsValid(remotePorts[idx]);
            RSRelease(remotePorts[idx]);
        }
        RSAllocatorDeallocate(RSAllocatorSystemDefault, remotePorts);
    }
}

static RSTypeID __RSMessagePortTypeID = _RSRuntimeNotATypeID;

static const RSRuntimeClass __RSMessagePortClass = {
    _RSRuntimeScannedObject,
    0,
    "RSMessagePort",
    NULL,      // init
    NULL,      // copy
    __RSMessagePortDeallocate,
    NULL,
    NULL,
    __RSMessagePortCopyDescription,      //
    NULL,
};

RSPrivate void __RSMessagePortInitialize(void) {
    __RSMessagePortTypeID = __RSRuntimeRegisterClass(&__RSMessagePortClass);
}

RSTypeID RSMessagePortGetTypeID(void) {
    return __RSMessagePortTypeID;
}

static RSStringRef __RSMessagePortSanitizeStringName(RSStringRef name, uint8_t **utfnamep, RSIndex *utfnamelenp) {
    uint8_t *utfname;
    RSIndex utflen;
    RSStringRef result = NULL;
    utfname = RSAllocatorAllocate(RSAllocatorSystemDefault, __RSMessagePortMaxNameLength + 1);
    RSStringGetBytes(name, RSMakeRange(0, RSStringGetLength(name)), RSStringEncodingUTF8, 0, false, utfname, __RSMessagePortMaxNameLength, &utflen);
    utfname[utflen] = '\0';
    if (strlen((const char *)utfname) != utflen) {
        /* PCA 9194709: refuse to sanitize a string with an embedded nul character */
        RSAllocatorDeallocate(RSAllocatorSystemDefault, utfname);
        utfname = NULL;
        utfnamelenp = 0;
    } else {
        /* A new string is created, because the original string may have been
         truncated to the max length, and we want the string name to definitely
         match the raw UTF-8 chunk that has been created. Also, this is useful
         to get a constant string in case the original name string was mutable. */
        result = RSStringCreateWithBytes(RSAllocatorSystemDefault, utfname, utflen, RSStringEncodingUTF8, false);
    }
    if (NULL != utfnamep) {
        *utfnamep = utfname;
    } else if (NULL != utfname) {
        RSAllocatorDeallocate(RSAllocatorSystemDefault, utfname);
    }
    if (NULL != utfnamelenp) {
        *utfnamelenp = utflen;
    }
    return result;
}

static void __RSMessagePortDummyCallback(RSMachPortRef port, void *msg, RSIndex size, void *info) {
    // not supposed to be implemented
}

static void __RSMessagePortInvalidationCallBack(RSMachPortRef port, void *info) {
    // info has been setup as the RSMessagePort owning the RSMachPort
    if (info) RSMessagePortInvalidate(info);
}

static RSMessagePortRef __RSMessagePortCreateLocal(RSAllocatorRef allocator, RSStringRef name, RSMessagePortCallBack callout, RSMessagePortContext *context, BOOL *shouldFreeInfo, BOOL perPID, RSMessagePortCallBackEx calloutEx) {
    RSMessagePortRef memory;
    uint8_t *utfname = NULL;
    
    if (shouldFreeInfo) *shouldFreeInfo = true;
    if (NULL != name) {
        name = __RSMessagePortSanitizeStringName(name, &utfname, NULL);
    }
    RSSpinLockLock(&__RSAllMessagePortsLock);
    if (!perPID && NULL != name) {
        RSMessagePortRef existing;
        if (NULL != __RSAllLocalMessagePorts && (RSDictionaryGetValueIfPresent(__RSAllRemoteMessagePorts, name, (const void **)&existing))) {
            RSRetain(existing);
            RSSpinLockUnlock(&__RSAllMessagePortsLock);
            RSRelease(name);
            RSAllocatorDeallocate(RSAllocatorSystemDefault, utfname);
            if (!RSMessagePortIsValid(existing)) { // must do this outside lock to avoid deadlock
                RSRelease(existing);
                existing = NULL;
            }
            return (RSMessagePortRef)(existing);
        }
    }
    RSSpinLockUnlock(&__RSAllMessagePortsLock);
    RSIndex size = sizeof(struct __RSMessagePort) - sizeof(RSRuntimeBase);
    memory = (RSMessagePortRef)__RSRuntimeCreateInstance(allocator, __RSMessagePortTypeID, size);
    if (NULL == memory) {
        if (NULL != name) {
            RSRelease(name);
        }
        RSAllocatorDeallocate(RSAllocatorSystemDefault, utfname);
        return NULL;
    }
    __RSMessagePortUnsetValid(memory);
    __RSMessagePortUnsetExtraMachRef(memory);
    __RSMessagePortUnsetRemote(memory);
    memory->_lock = RSSpinLockInit;
    memory->_name = name;
    memory->_port = NULL;
    memory->_replies = NULL;
    memory->_convCounter = 0;
    memory->_perPID = perPID ? getpid() : 0;	// actual value not terribly useful for local ports
    memory->_replyPort = NULL;
    memory->_source = NULL;
    memory->_dispatchSource = NULL;
    memory->_dispatchQ = NULL;
    memory->_icallout = NULL;
    memory->_callout = callout;
    memory->_calloutEx = calloutEx;
    memory->_context.info = NULL;
    memory->_context.retain = NULL;
    memory->_context.release = NULL;
    memory->_context.copyDescription = NULL;
    
    if (NULL != name) {
        RSMachPortRef native = NULL;
        kern_return_t ret;
        mach_port_t bs, mp;
        task_get_bootstrap_port(mach_task_self(), &bs);
        if (!perPID) {
            ret = bootstrap_check_in(bs, (char *)utfname, &mp); /* If we're started by launchd or the old mach_init */
            if (ret == KERN_SUCCESS) {
                ret = mach_port_insert_right(mach_task_self(), mp, mp, MACH_MSG_TYPE_MAKE_SEND);
                if (KERN_SUCCESS == ret) {
                    RSMachPortContext ctx = {0, memory, NULL, NULL, NULL};
                    native = RSMachPortCreateWithPort(allocator, mp, __RSMessagePortDummyCallback, &ctx, NULL);
                    __RSMessagePortSetExtraMachRef(memory);
                } else {
                    RSLog(RSLogLevelDebug, RSSTR("*** RSMessagePort: mach_port_insert_member() after bootstrap_check_in(): failed %d (0x%x) '%s', port = 0x%x, name = '%s'"), ret, ret, bootstrap_strerror(ret), mp, utfname);
                    mach_port_destroy(mach_task_self(), mp);
                    RSAllocatorDeallocate(RSAllocatorSystemDefault, utfname);
                    // name is released by deallocation
                    RSRelease(memory);
                    return NULL;
                }
            }
        }
        if (!native) {
            RSMachPortContext ctx = {0, memory, NULL, NULL, NULL};
            native = RSMachPortCreate(allocator, __RSMessagePortDummyCallback, &ctx, NULL);
            if (!native) {
                RSAllocatorDeallocate(RSAllocatorSystemDefault, utfname);
                // name is released by deallocation
                RSRelease(memory);
                return NULL;
            }
            mp = RSMachPortGetPort(native);
            ret = bootstrap_register2(bs, (char *)utfname, mp, perPID ? BOOTSTRAP_PER_PID_SERVICE : 0);
            if (ret != KERN_SUCCESS) {
                RSLog(RSLogLevelDebug, RSSTR("*** RSMessagePort: bootstrap_register(): failed %d (0x%x) '%s', port = 0x%x, name = '%s'\nSee /usr/include/servers/bootstrap_defs.h for the error codes."), ret, ret, bootstrap_strerror(ret), mp, utfname);
                RSMachPortInvalidate(native);
                RSRelease(native);
                RSAllocatorDeallocate(RSAllocatorSystemDefault, utfname);
                // name is released by deallocation
                RSRelease(memory);
                return NULL;
            }
        }
        RSMachPortSetInvalidationCallBack(native, __RSMessagePortInvalidationCallBack);
        memory->_port = native;
    }
    
    RSAllocatorDeallocate(RSAllocatorSystemDefault, utfname);
    __RSMessagePortSetValid(memory);
    if (NULL != context) {
        memmove(&memory->_context, context, sizeof(RSMessagePortContext));
        memory->_context.info = context->retain ? (void *)context->retain(context->info) : context->info;
    }
    RSSpinLockLock(&__RSAllMessagePortsLock);
    if (!perPID && NULL != name) {
        RSMessagePortRef existing;
        if (NULL != __RSAllLocalMessagePorts && (RSDictionaryGetValueIfPresent(__RSAllRemoteMessagePorts, name, (const void **)&existing))) {
            RSRetain(existing);
            RSSpinLockUnlock(&__RSAllMessagePortsLock);
            RSRelease(memory);
            return (RSMessagePortRef)(existing);
        }
        if (NULL == __RSAllLocalMessagePorts) {
            __RSAllLocalMessagePorts = RSDictionaryCreateMutable(RSAllocatorSystemDefault, 0, RSDictionaryNilValueContext);
        }
        RSDictionarySetValue(__RSAllLocalMessagePorts, name, memory);
    }
    RSSpinLockUnlock(&__RSAllMessagePortsLock);
    if (shouldFreeInfo) *shouldFreeInfo = false;
    return memory;
}

RSMessagePortRef RSMessagePortCreateLocal(RSAllocatorRef allocator, RSStringRef name, RSMessagePortCallBack callout, RSMessagePortContext *context, BOOL *shouldFreeInfo) {
    return __RSMessagePortCreateLocal(allocator, name, callout, context, shouldFreeInfo, false, NULL);
}

RSMessagePortRef RSMessagePortCreatePerProcessLocal(RSAllocatorRef allocator, RSStringRef name, RSMessagePortCallBack callout, RSMessagePortContext *context, BOOL *shouldFreeInfo) {
    return __RSMessagePortCreateLocal(allocator, name, callout, context, shouldFreeInfo, true, NULL);
}

RSMessagePortRef _RSMessagePortCreateLocalEx(RSAllocatorRef allocator, RSStringRef name, BOOL perPID, uintptr_t unused, RSMessagePortCallBackEx calloutEx, RSMessagePortContext *context, BOOL *shouldFreeInfo) {
    return __RSMessagePortCreateLocal(allocator, name, NULL, context, shouldFreeInfo, perPID, calloutEx);
}

static RSMessagePortRef __RSMessagePortCreateRemote(RSAllocatorRef allocator, RSStringRef name, BOOL perPID, RSIndex pid) {
    RSMessagePortRef memory;
    RSMachPortRef native;
    RSMachPortContext ctx;
    uint8_t *utfname = NULL;
    RSIndex size;
    mach_port_t bp, port;
    kern_return_t ret;
    
    name = __RSMessagePortSanitizeStringName(name, &utfname, NULL);
    if (NULL == name) {
        return NULL;
    }
    RSSpinLockLock(&__RSAllMessagePortsLock);
    if (!perPID && NULL != name) {
        RSMessagePortRef existing;
        if (NULL != __RSAllRemoteMessagePorts && (RSDictionaryGetValueIfPresent(__RSAllRemoteMessagePorts, name, (const void **)&existing))) {
            RSRetain(existing);
            RSSpinLockUnlock(&__RSAllMessagePortsLock);
            RSRelease(name);
            RSAllocatorDeallocate(RSAllocatorSystemDefault, utfname);
            if (!RSMessagePortIsValid(existing)) { // must do this outside lock to avoid deadlock
                RSRelease(existing);
                existing = NULL;
            }
            return (RSMessagePortRef)(existing);
        }
    }
    RSSpinLockUnlock(&__RSAllMessagePortsLock);
    size = sizeof(struct __RSMessagePort) - sizeof(RSMessagePortContext) - sizeof(RSRuntimeBase);
    memory = (RSMessagePortRef)__RSRuntimeCreateInstance(allocator, __RSMessagePortTypeID, size);
    if (NULL == memory) {
        if (NULL != name) {
            RSRelease(name);
        }
        RSAllocatorDeallocate(RSAllocatorSystemDefault, utfname);
        return NULL;
    }
    __RSMessagePortUnsetValid(memory);
    __RSMessagePortUnsetExtraMachRef(memory);
    __RSMessagePortSetRemote(memory);
    memory->_lock = RSSpinLockInit;
    memory->_name = name;
    memory->_port = NULL;
    memory->_replies = RSDictionaryCreateMutable(RSAllocatorSystemDefault, 0, RSDictionaryNilContext);
    memory->_convCounter = 0;
    memory->_perPID = perPID ? pid : 0;
    memory->_replyPort = NULL;
    memory->_source = NULL;
    memory->_dispatchSource = NULL;
    memory->_dispatchQ = NULL;
    memory->_icallout = NULL;
    memory->_callout = NULL;
    memory->_calloutEx = NULL;
    ctx.version = 0;
    ctx.info = memory;
    ctx.retain = NULL;
    ctx.release = NULL;
    ctx.copyDescription = NULL;
    task_get_bootstrap_port(mach_task_self(), &bp);
    ret = bootstrap_look_up2(bp, (char *)utfname, &port, perPID ? (pid_t)pid : 0, perPID ? BOOTSTRAP_PER_PID_SERVICE : 0);
    native = (KERN_SUCCESS == ret) ? RSMachPortCreateWithPort(allocator, port, __RSMessagePortDummyCallback, &ctx, NULL) : NULL;
    RSAllocatorDeallocate(RSAllocatorSystemDefault, utfname);
    if (NULL == native) {
        // name is released by deallocation
        RSRelease(memory);
        return NULL;
    }
    memory->_port = native;
    __RSMessagePortSetValid(memory);
    RSSpinLockLock(&__RSAllMessagePortsLock);
    if (!perPID && NULL != name) {
        RSMessagePortRef existing;
        if (NULL != __RSAllRemoteMessagePorts && (RSDictionaryGetValueIfPresent(__RSAllRemoteMessagePorts, name, (const void **)&existing))) {
            RSRetain(existing);
            RSSpinLockUnlock(&__RSAllMessagePortsLock);
            RSRelease(memory);
            return (RSMessagePortRef)(existing);
        }
        if (NULL == __RSAllRemoteMessagePorts) {
            __RSAllRemoteMessagePorts = RSDictionaryCreateMutable(RSAllocatorSystemDefault, 0, RSDictionaryNilValueContext);
        }
        RSDictionarySetValue(__RSAllLocalMessagePorts, name, memory);
    }
    RSRetain(native);
    RSSpinLockUnlock(&__RSAllMessagePortsLock);
    RSMachPortSetInvalidationCallBack(native, __RSMessagePortInvalidationCallBack);
    // that set-invalidation-callback might have called back into us
    // if the RSMachPort is already bad, but that was a no-op since
    // there was no callback setup at the (previous) time the RSMachPort
    // went invalid; so check for validity manually and react
    if (!RSMachPortIsValid(native)) {
        RSRelease(memory); // does the invalidate
        RSRelease(native);
        return NULL;
    }
    RSRelease(native);
    return (RSMessagePortRef)memory;
}

RSMessagePortRef RSMessagePortCreateRemote(RSAllocatorRef allocator, RSStringRef name) {
    return __RSMessagePortCreateRemote(allocator, name, false, 0);
}

RSMessagePortRef RSMessagePortCreatePerProcessRemote(RSAllocatorRef allocator, RSStringRef name, RSIndex pid) {
    return __RSMessagePortCreateRemote(allocator, name, true, pid);
}

BOOL RSMessagePortIsRemote(RSMessagePortRef ms) {
    __RSGenericValidInstance(ms, __RSMessagePortTypeID);
    return __RSMessagePortIsRemote(ms);
}

RSStringRef RSMessagePortGetName(RSMessagePortRef ms) {
    __RSGenericValidInstance(ms, __RSMessagePortTypeID);
    return ms->_name;
}

BOOL RSMessagePortSetName(RSMessagePortRef ms, RSStringRef name) {
    RSAllocatorRef allocator = RSGetAllocator(ms);
    uint8_t *utfname = NULL;
    
    __RSGenericValidInstance(ms, __RSMessagePortTypeID);
    if (ms->_perPID || __RSMessagePortIsRemote(ms)) return false;
    name = __RSMessagePortSanitizeStringName(name, &utfname, NULL);
    if (NULL == name) {
        return false;
    }
    RSSpinLockLock(&__RSAllMessagePortsLock);
    if (NULL != name) {
        RSMessagePortRef existing;
        if (NULL != __RSAllLocalMessagePorts && RSDictionaryGetValueIfPresent(__RSAllLocalMessagePorts, name, (const void **)&existing)) {
            RSSpinLockUnlock(&__RSAllMessagePortsLock);
            RSRelease(name);
            RSAllocatorDeallocate(RSAllocatorSystemDefault, utfname);
            return false;
        }
    }
    RSSpinLockUnlock(&__RSAllMessagePortsLock);
    
    if (NULL != name && (NULL == ms->_name || !RSEqual(ms->_name, name))) {
        RSMachPortRef oldPort = ms->_port;
        RSMachPortRef native = NULL;
        kern_return_t ret;
        mach_port_t bs, mp;
        task_get_bootstrap_port(mach_task_self(), &bs);
        ret = bootstrap_check_in(bs, (char *)utfname, &mp); /* If we're started by launchd or the old mach_init */
        if (ret == KERN_SUCCESS) {
            ret = mach_port_insert_right(mach_task_self(), mp, mp, MACH_MSG_TYPE_MAKE_SEND);
            if (KERN_SUCCESS == ret) {
                RSMachPortContext ctx = {0, ms, NULL, NULL, NULL};
                native = RSMachPortCreateWithPort(allocator, mp, __RSMessagePortDummyCallback, &ctx, NULL);
                __RSMessagePortSetExtraMachRef(ms);
            } else {
                mach_port_destroy(mach_task_self(), mp);
                RSAllocatorDeallocate(RSAllocatorSystemDefault, utfname);
                RSRelease(name);
                return false;
            }
        }
        if (!native) {
            RSMachPortContext ctx = {0, ms, NULL, NULL, NULL};
            native = RSMachPortCreate(allocator, __RSMessagePortDummyCallback, &ctx, NULL);
            if (!native) {
                RSAllocatorDeallocate(RSAllocatorSystemDefault, utfname);
                RSRelease(name);
                return false;
            }
            mp = RSMachPortGetPort(native);
            ret = bootstrap_register2(bs, (char *)utfname, mp, 0);
            if (ret != KERN_SUCCESS) {
                RSLog(RSLogLevelDebug, RSSTR("*** RSMessagePort: bootstrap_register(): failed %d (0x%x) '%s', port = 0x%x, name = '%s'\nSee /usr/include/servers/bootstrap_defs.h for the error codes."), ret, ret, bootstrap_strerror(ret), mp, utfname);
                RSMachPortInvalidate(native);
                RSRelease(native);
                RSAllocatorDeallocate(RSAllocatorSystemDefault, utfname);
                RSRelease(name);
                return false;
            }
        }
        RSMachPortSetInvalidationCallBack(native, __RSMessagePortInvalidationCallBack);
        ms->_port = native;
        if (NULL != oldPort && oldPort != native) {
            if (__RSMessagePortExtraMachRef(ms)) {
                mach_port_mod_refs(mach_task_self(), RSMachPortGetPort(oldPort), MACH_PORT_RIGHT_SEND, -1);
                mach_port_mod_refs(mach_task_self(), RSMachPortGetPort(oldPort), MACH_PORT_RIGHT_RECEIVE, -1);
            }
            RSMachPortInvalidate(oldPort);
            RSRelease(oldPort);
        }
        RSSpinLockLock(&__RSAllMessagePortsLock);
        // This relocking without checking to see if something else has grabbed
        // that name in the cache is rather suspect, but what would that even
        // mean has happened?  We'd expect the bootstrap_* calls above to have
        // failed for this one and not gotten this far, or failed for all of the
        // other simultaneous attempts to get the name (and having succeeded for
        // this one, gotten here).  So we're not going to try very hard here
        // with the thread-safety.
        if (NULL == __RSAllLocalMessagePorts) {
            __RSAllLocalMessagePorts = RSDictionaryCreateMutable(RSAllocatorSystemDefault, 0, RSDictionaryNilValueContext);
        }
        if (NULL != ms->_name) {
            RSDictionaryRemoveValue(__RSAllLocalMessagePorts, ms->_name);
            RSRelease(ms->_name);
        }
        ms->_name = name;
        RSDictionarySetValue(__RSAllLocalMessagePorts, name, ms);
        RSSpinLockUnlock(&__RSAllMessagePortsLock);
    }
    
    RSAllocatorDeallocate(RSAllocatorSystemDefault, utfname);
    return true;
}

void RSMessagePortGetContext(RSMessagePortRef ms, RSMessagePortContext *context) {
    __RSGenericValidInstance(ms, __RSMessagePortTypeID);
    //#warning RS: assert that this is a local port
    RSAssert1(0 == context->version, __RSLogAssertion, "%s(): context version not initialized to 0", __PRETTY_FUNCTION__);
    memmove(context, &ms->_context, sizeof(RSMessagePortContext));
}

void RSMessagePortInvalidate(RSMessagePortRef ms) {
    __RSGenericValidInstance(ms, __RSMessagePortTypeID);
    if (!__RSMessagePortIsDeallocing(ms)) {
        RSRetain(ms);
    }
    __RSMessagePortLock(ms);
    if (__RSMessagePortIsValid(ms)) {
        if (ms->_dispatchSource) {
            dispatch_source_cancel(ms->_dispatchSource);
            ms->_dispatchSource = NULL;
            ms->_dispatchQ = NULL;
        }
        
        RSMessagePortInvalidationCallBack callout = ms->_icallout;
        RSRunLoopSourceRef source = ms->_source;
        RSMachPortRef replyPort = ms->_replyPort;
        RSMachPortRef port = ms->_port;
        RSStringRef name = ms->_name;
        void *info = NULL;
        
        __RSMessagePortUnsetValid(ms);
        if (!__RSMessagePortIsRemote(ms)) {
            info = ms->_context.info;
            ms->_context.info = NULL;
        }
        ms->_source = NULL;
        ms->_replyPort = NULL;
        ms->_port = NULL;
        __RSMessagePortUnlock(ms);
        
        RSSpinLockLock(&__RSAllMessagePortsLock);
        if (0 == ms->_perPID && NULL != name && NULL != (__RSMessagePortIsRemote(ms) ? __RSAllRemoteMessagePorts : __RSAllLocalMessagePorts)) {
            RSDictionaryRemoveValue(__RSMessagePortIsRemote(ms) ? __RSAllRemoteMessagePorts : __RSAllLocalMessagePorts, name);
        }
        RSSpinLockUnlock(&__RSAllMessagePortsLock);
        if (NULL != callout) {
            callout(ms, info);
        }
        if (!__RSMessagePortIsRemote(ms) && NULL != ms->_context.release) {
            ms->_context.release(info);
        }
        if (NULL != source) {
            RSRunLoopSourceInvalidate(source);
            RSRelease(source);
        }
        if (NULL != replyPort) {
            RSMachPortInvalidate(replyPort);
            RSRelease(replyPort);
        }
        if (__RSMessagePortIsRemote(ms)) {
            // Get rid of our extra ref on the Mach port gotten from bs server
            mach_port_deallocate(mach_task_self(), RSMachPortGetPort(port));
        }
        if (NULL != port) {
            // We already know we're going invalid, don't need this callback
            // anymore; plus, this solves a reentrancy deadlock; also, this
            // must be done before the deallocate of the Mach port, to
            // avoid a race between the notification message which could be
            // handled in another thread, and this NULL'ing out.
            RSMachPortSetInvalidationCallBack(port, NULL);
            if (__RSMessagePortExtraMachRef(ms)) {
                mach_port_mod_refs(mach_task_self(), RSMachPortGetPort(port), MACH_PORT_RIGHT_SEND, -1);
                mach_port_mod_refs(mach_task_self(), RSMachPortGetPort(port), MACH_PORT_RIGHT_RECEIVE, -1);
            }
            RSMachPortInvalidate(port);
            RSRelease(port);
        }
    } else {
        __RSMessagePortUnlock(ms);
    }
    if (!__RSMessagePortIsDeallocing(ms)) {
        RSRelease(ms);
    }
}

BOOL RSMessagePortIsValid(RSMessagePortRef ms) {
    __RSGenericValidInstance(ms, __RSMessagePortTypeID);
    if (!__RSMessagePortIsValid(ms)) return false;
    RSRetain(ms);
    if (NULL != ms->_port && !RSMachPortIsValid(ms->_port)) {
        RSMessagePortInvalidate(ms);
        RSRelease(ms);
        return false;
    }
    if (NULL != ms->_replyPort && !RSMachPortIsValid(ms->_replyPort)) {
        RSMessagePortInvalidate(ms);
        RSRelease(ms);
        return false;
    }
    RSRelease(ms);
    return true;
}

RSMessagePortInvalidationCallBack RSMessagePortGetInvalidationCallBack(RSMessagePortRef ms) {
    __RSGenericValidInstance(ms, __RSMessagePortTypeID);
    return ms->_icallout;
}

void RSMessagePortSetInvalidationCallBack(RSMessagePortRef ms, RSMessagePortInvalidationCallBack callout) {
    __RSGenericValidInstance(ms, __RSMessagePortTypeID);
    if (!__RSMessagePortIsValid(ms) && NULL != callout) {
        callout(ms, ms->_context.info);
    } else {
        ms->_icallout = callout;
    }
}

static void __RSMessagePortReplyCallBack(RSMachPortRef port, void *msg, RSIndex size, void *info) {
    RSMessagePortRef ms = info;
    mach_msg_base_t *msgp = msg;
    mach_msg_base_t *replymsg;
    __RSMessagePortLock(ms);
    if (!__RSMessagePortIsValid(ms)) {
        __RSMessagePortUnlock(ms);
        return;
    }
    
    int32_t byteslen = 0;
    
    BOOL invalidMagic = (MSGP_GET(msgp, magic) != MAGIC) && (RSSwapInt32(MSGP_GET(msgp, magic)) != MAGIC);
    BOOL invalidComplex = (0 != msgp->body.msgh_descriptor_count) && !(msgp->header.msgh_bits & MACH_MSGH_BITS_COMPLEX);
    invalidComplex = invalidComplex || ((msgp->header.msgh_bits & MACH_MSGH_BITS_COMPLEX) && (0 == msgp->body.msgh_descriptor_count));
    BOOL wayTooBig = ((msgp->body.msgh_descriptor_count) ? sizeof(struct __RSMessagePortMachMessage1) : sizeof(struct __RSMessagePortMachMessage0) + __RSMessagePortMaxInlineBytes) < msgp->header.msgh_size;
    BOOL wayTooSmall = msgp->header.msgh_size < sizeof(struct __RSMessagePortMachMessage0);
    BOOL wrongSize = false;
    if (!(invalidComplex || wayTooBig || wayTooSmall)) {
        byteslen = RSSwapInt32LittleToHost(MSGP_GET(msgp, byteslen));
        wrongSize = (byteslen < 0) || (__RSMessagePortMaxDataSize < byteslen);
        if (0 != msgp->body.msgh_descriptor_count) {
            wrongSize = wrongSize || (MSGP1_FIELD(msgp, ool).size != byteslen);
        } else {
            wrongSize = wrongSize || (msgp->header.msgh_size - sizeof(struct __RSMessagePortMachMessage0) < byteslen);
        }
    }
    BOOL invalidMsgID = (0 <= msgp->header.msgh_id) && (msgp->header.msgh_id <= INT32_MAX); // conversation id
    if (invalidMagic || invalidComplex || wayTooBig || wayTooSmall || wrongSize || invalidMsgID) {
        RSLog(RSSTR("*** RSMessagePort: dropping corrupt reply Mach message (0b%d%d%d%d%d%d)"), invalidMagic, invalidComplex, wayTooBig, wayTooSmall, wrongSize, invalidMsgID);
        mach_msg_destroy((mach_msg_header_t *)msgp);
        __RSMessagePortUnlock(ms);
        return;
    }
    
    if (RSDictionaryContainsKey(ms->_replies, (void *)(uintptr_t)msgp->header.msgh_id)) {
        RSDataRef reply = NULL;
        replymsg = (mach_msg_base_t *)msg;
        if (0 == replymsg->body.msgh_descriptor_count) {
            uintptr_t msgp_extent = (uintptr_t)((uint8_t *)msgp + msgp->header.msgh_size);
            uintptr_t data_extent = (uintptr_t)((uint8_t *)&(MSGP0_FIELD(replymsg, bytes)) + byteslen);
            if (0 <= byteslen && data_extent <= msgp_extent) {
                reply = RSDataCreate(RSAllocatorSystemDefault, MSGP0_FIELD(replymsg, bytes), byteslen);
            } else {
                reply = (void *)~0;	// means NULL data
            }
        } else {
            //#warning RS: should create a no-copy data here that has a custom VM-freeing allocator, and not vm_dealloc here
            reply = RSDataCreate(RSAllocatorSystemDefault, MSGP1_FIELD(replymsg, ool).address, MSGP1_FIELD(replymsg, ool).size);
            vm_deallocate(mach_task_self(), (vm_address_t)MSGP1_FIELD(replymsg, ool).address, MSGP1_FIELD(replymsg, ool).size);
        }
        RSDictionarySetValue(ms->_replies, (void *)(uintptr_t)msgp->header.msgh_id, (void *)reply);
    } else {	/* discard message */
        if (1 == msgp->body.msgh_descriptor_count) {
            vm_deallocate(mach_task_self(), (vm_address_t)MSGP1_FIELD(msgp, ool).address, MSGP1_FIELD(msgp, ool).size);
        }
    }
    __RSMessagePortUnlock(ms);
}

SInt32 RSMessagePortSendRequest(RSMessagePortRef remote, SInt32 msgid, RSDataRef data, RSTimeInterval sendTimeout, RSTimeInterval rcvTimeout, RSStringRef replyMode, RSDataRef *returnDatap) {
    mach_msg_base_t *sendmsg;
    RSRunLoopRef currentRL = RSRunLoopGetCurrent();
    RSRunLoopSourceRef source = NULL;
    RSDataRef reply = NULL;
    uint64_t termTSR;
    uint32_t sendOpts = 0, sendTimeOut = 0;
    int32_t desiredReply;
    BOOL didRegister = false;
    kern_return_t ret;
    
    //#warning RS: This should be an assert
    // if (!__RSMessagePortIsRemote(remote)) return -999;
    if (data && __RSMessagePortMaxDataSize < RSDataGetLength(data)) {
        RSLog(RSSTR("*** RSMessagePortSendRequest: RSMessagePort cannot send more than %lu bytes of data"), __RSMessagePortMaxDataSize);
        return RSMessagePortTransportError;
    }
    __RSMessagePortLock(remote);
    if (!__RSMessagePortIsValid(remote)) {
        __RSMessagePortUnlock(remote);
        return RSMessagePortIsInvalid;
    }
    RSRetain(remote); // retain during run loop to avoid invalidation causing freeing
    if (NULL == remote->_replyPort) {
        RSMachPortContext context;
        context.version = 0;
        context.info = remote;
        context.retain = (const void *(*)(const void *))RSRetain;
        context.release = (void (*)(const void *))RSRelease;
        context.copyDescription = (RSStringRef (*)(const void *))__RSMessagePortCopyDescription;
        remote->_replyPort = RSMachPortCreate(RSGetAllocator(remote), __RSMessagePortReplyCallBack, &context, NULL);
    }
    remote->_convCounter++;
    desiredReply = -remote->_convCounter;
    sendmsg = __RSMessagePortCreateMessage(false, RSMachPortGetPort(remote->_port), (replyMode != NULL ? RSMachPortGetPort(remote->_replyPort) : MACH_PORT_NULL), -desiredReply, msgid, (data ? RSDataGetBytesPtr(data) : NULL), (data ? RSDataGetLength(data) : 0));
    if (!sendmsg) {
        __RSMessagePortUnlock(remote);
        RSRelease(remote);
        return RSMessagePortTransportError;
    }
    if (replyMode != NULL) {
        RSDictionarySetValue(remote->_replies, (void *)(uintptr_t)desiredReply, NULL);
        source = RSMachPortCreateRunLoopSource(RSGetAllocator(remote), remote->_replyPort, -100);
        didRegister = !RSRunLoopContainsSource(currentRL, source, replyMode);
        if (didRegister) {
            RSRunLoopAddSource(currentRL, source, replyMode);
        }
    }
    if (sendTimeout < 10.0*86400) {
        // anything more than 10 days is no timeout!
        sendOpts = MACH_SEND_TIMEOUT;
        sendTimeout *= 1000.0;
        if (sendTimeout < 1.0) sendTimeout = 0.0;
        sendTimeOut = floor(sendTimeout);
    }
    __RSMessagePortUnlock(remote);
    ret = mach_msg((mach_msg_header_t *)sendmsg, MACH_SEND_MSG|sendOpts, sendmsg->header.msgh_size, 0, MACH_PORT_NULL, sendTimeOut, MACH_PORT_NULL);
    __RSMessagePortLock(remote);
    if (KERN_SUCCESS != ret) {
        // need to deallocate the send-once right that might have been created
        if (replyMode != NULL) mach_port_deallocate(mach_task_self(), ((mach_msg_header_t *)sendmsg)->msgh_local_port);
        if (didRegister) {
            RSRunLoopRemoveSource(currentRL, source, replyMode);
        }
        if (source) RSRelease(source);
        __RSMessagePortUnlock(remote);
        RSAllocatorDeallocate(RSAllocatorSystemDefault, sendmsg);
        RSRelease(remote);
        return (MACH_SEND_TIMED_OUT == ret) ? RSMessagePortSendTimeout : RSMessagePortTransportError;
    }
    __RSMessagePortUnlock(remote);
    RSAllocatorDeallocate(RSAllocatorSystemDefault, sendmsg);
    if (replyMode == NULL) {
        RSRelease(remote);
        return RSMessagePortSuccess;
    }
    _RSMachPortInstallNotifyPort(currentRL, replyMode);
    
    termTSR = mach_absolute_time() + __RSTimeIntervalToTSR(rcvTimeout);
    for (;;) {
        RSRunLoopRunInMode(replyMode, __RSTimeIntervalUntilTSR(termTSR), true);
        // warning: what, if anything, should be done if remote is now invalid?
        reply = RSDictionaryGetValue(remote->_replies, (void *)(uintptr_t)desiredReply);
        if (NULL != reply || termTSR < mach_absolute_time()) {
            break;
        }
        if (!RSMessagePortIsValid(remote)) {
            // no reason that reply port alone should go invalid so we don't check for that
            break;
        }
    }
    // Should we uninstall the notify port?  A complex question...
    if (didRegister) {
        RSRunLoopRemoveSource(currentRL, source, replyMode);
    }
    if (source) RSRelease(source);
    if (NULL == reply) {
        RSDictionaryRemoveValue(remote->_replies, (void *)(uintptr_t)desiredReply);
        RSRelease(remote);
        return RSMessagePortIsValid(remote) ? RSMessagePortReceiveTimeout : RSMessagePortBecameInvalidError;
    }
    if (NULL != returnDatap) {
        *returnDatap = ((void *)~0 == reply) ? NULL : reply;
    } else if ((void *)~0 != reply) {
        RSRelease(reply);
    }
    RSDictionaryRemoveValue(remote->_replies, (void *)(uintptr_t)desiredReply);
    RSRelease(remote);
    return RSMessagePortSuccess;
}

static mach_port_t __RSMessagePortGetPort(void *info) {
    RSMessagePortRef ms = info;
    if (!ms->_port && __RSMessagePortIsValid(ms)) RSLog(RSSTR("*** Warning: A local RSMessagePort (%p) is being put in a run loop or dispatch queue, but it has not been named yet, so this will be a no-op and no messages are going to be received, even if named later."), info);
    return ms->_port ? RSMachPortGetPort(ms->_port) : MACH_PORT_NULL;
}


static void *__RSMessagePortPerform(void *msg, RSIndex size, RSAllocatorRef allocator, void *info) {
    RSMessagePortRef ms = info;
    mach_msg_base_t *msgp = msg;
    mach_msg_base_t *replymsg;
    void *context_info;
    void (*context_release)(const void *);
    RSDataRef returnData, data = NULL;
    void *return_bytes = NULL;
    RSIndex return_len = 0;
    int32_t msgid;
    
    __RSMessagePortLock(ms);
    if (!__RSMessagePortIsValid(ms)) {
        __RSMessagePortUnlock(ms);
        return NULL;
    }
    if (NULL != ms->_context.retain) {
        context_info = (void *)ms->_context.retain(ms->_context.info);
        context_release = ms->_context.release;
    } else {
        context_info = ms->_context.info;
        context_release = NULL;
    }
    __RSMessagePortUnlock(ms);
    
    
    int32_t byteslen = 0;
    
    BOOL invalidMagic = (MSGP_GET(msgp, magic) != MAGIC) && (RSSwapInt32(MSGP_GET(msgp, magic)) != MAGIC);
    BOOL invalidComplex = (0 != msgp->body.msgh_descriptor_count) && !(msgp->header.msgh_bits & MACH_MSGH_BITS_COMPLEX);
    invalidComplex = invalidComplex || ((msgp->header.msgh_bits & MACH_MSGH_BITS_COMPLEX) && (0 == msgp->body.msgh_descriptor_count));
    BOOL wayTooBig = ((msgp->body.msgh_descriptor_count) ? sizeof(struct __RSMessagePortMachMessage1) : sizeof(struct __RSMessagePortMachMessage0) + __RSMessagePortMaxInlineBytes) < msgp->header.msgh_size;
    BOOL wayTooSmall = msgp->header.msgh_size < sizeof(struct __RSMessagePortMachMessage0);
    BOOL wrongSize = false;
    if (!(invalidComplex || wayTooBig || wayTooSmall)) {
        byteslen = RSSwapInt32LittleToHost(MSGP_GET(msgp, byteslen));
        wrongSize = (byteslen < 0) || (__RSMessagePortMaxDataSize < byteslen);
        if (0 != msgp->body.msgh_descriptor_count) {
            wrongSize = wrongSize || (MSGP1_FIELD(msgp, ool).size != byteslen);
        } else {
            wrongSize = wrongSize || (msgp->header.msgh_size - sizeof(struct __RSMessagePortMachMessage0) < byteslen);
        }
    }
    BOOL invalidMsgID = (msgp->header.msgh_id <= 0) || (INT32_MAX < msgp->header.msgh_id); // conversation id
    if (invalidMagic || invalidComplex || wayTooBig || wayTooSmall || wrongSize || invalidMsgID) {
        RSLog(RSSTR("*** RSMessagePort: dropping corrupt request Mach message (0b%d%d%d%d%d%d)"), invalidMagic, invalidComplex, wayTooBig, wayTooSmall, wrongSize, invalidMsgID);
        mach_msg_destroy((mach_msg_header_t *)msgp);
        return NULL;
    }
    
    /* Create no-copy, no-free-bytes wrapper RSData */
    if (0 == msgp->body.msgh_descriptor_count) {
        uintptr_t msgp_extent = (uintptr_t)((uint8_t *)msgp + msgp->header.msgh_size);
        uintptr_t data_extent = (uintptr_t)((uint8_t *)&(MSGP0_FIELD(msgp, bytes)) + byteslen);
        msgid = RSSwapInt32LittleToHost(MSGP_GET(msgp, msgid));
        if (0 <= byteslen && data_extent <= msgp_extent) {
            data = RSDataCreateWithNoCopy(allocator, MSGP0_FIELD(msgp, bytes), byteslen, NO, RSAllocatorNull);
        }
    } else {
        msgid = RSSwapInt32LittleToHost(MSGP_GET(msgp, msgid));
        data = RSDataCreateWithNoCopy(allocator, MSGP1_FIELD(msgp, ool).address, MSGP1_FIELD(msgp, ool).size, NO, RSAllocatorNull);
    }
    if (ms->_callout) {
        returnData = ms->_callout(ms, msgid, data, context_info);
    } else {
        mach_msg_trailer_t *trailer = (mach_msg_trailer_t *)(((uintptr_t)&(msgp->header) + msgp->header.msgh_size + sizeof(natural_t) - 1) & ~(sizeof(natural_t) - 1));
        returnData = ms->_calloutEx(ms, msgid, data, context_info, trailer, 0);
    }
    /* Now, returnData could be (1) NULL, (2) an ordinary data < MAX_INLINE,
     (3) ordinary data >= MAX_INLINE, (4) a no-copy data < MAX_INLINE,
     (5) a no-copy data >= MAX_INLINE. In cases (2) and (4), we send the return
     bytes inline in the Mach message, so can release the returnData object
     here. In cases (3) and (5), we'll send the data out-of-line, we need to
     create a copy of the memory, which we'll have the kernel autodeallocate
     for us on send. In case (4) also, the bytes in the return data may be part
     of the bytes in "data" that we sent into the callout, so if the incoming
     data was received out of line, we wouldn't be able to clean up the out-of-line
     wad until the message was sent either, if we didn't make the copy. */
    if (NULL != returnData) {
        return_len = RSDataGetLength(returnData);
        if (__RSMessagePortMaxDataSize < return_len) {
            RSLog(RSSTR("*** RSMessagePort reply: RSMessagePort cannot send more than %lu bytes of data"), __RSMessagePortMaxDataSize);
            return_len = 0;
            returnData = NULL;
        }
        if (returnData && return_len < __RSMessagePortMaxInlineBytes) {
            return_bytes = (void *)RSDataGetBytesPtr(returnData);
        } else if (returnData) {
            return_bytes = NULL;
            vm_allocate(mach_task_self(), (vm_address_t *)&return_bytes, return_len, VM_FLAGS_ANYWHERE | VM_MAKE_TAG(VM_MEMORY_MACH_MSG));
            /* vm_copy would only be a win here if the source address
             is page aligned; it is a lose in all other cases, since
             the kernel will just do the memmove for us (but not in
             as simple a way). */
            memmove(return_bytes, RSDataGetBytesPtr(returnData), return_len);
        }
    }
    replymsg = __RSMessagePortCreateMessage(true, msgp->header.msgh_remote_port, MACH_PORT_NULL, -1 * (int32_t)msgp->header.msgh_id, msgid, return_bytes, return_len);
    if (1 == replymsg->body.msgh_descriptor_count) {
        MSGP1_FIELD(replymsg, ool).deallocate = true;
    }
    if (data) RSRelease(data);
    if (1 == msgp->body.msgh_descriptor_count) {
        vm_deallocate(mach_task_self(), (vm_address_t)MSGP1_FIELD(msgp, ool).address, MSGP1_FIELD(msgp, ool).size);
    }
    if (returnData) RSRelease(returnData);
    if (context_release) {
        context_release(context_info);
    }
    return replymsg;
}

RSRunLoopSourceRef RSMessagePortCreateRunLoopSource(RSAllocatorRef allocator, RSMessagePortRef ms, RSIndex order) {
    RSRunLoopSourceRef result = NULL;
    __RSGenericValidInstance(ms, __RSMessagePortTypeID);
    if (!RSMessagePortIsValid(ms)) return NULL;
    if (__RSMessagePortIsRemote(ms)) return NULL;
    __RSMessagePortLock(ms);
    if (NULL != ms->_source && !RSRunLoopSourceIsValid(ms->_source)) {
        RSRelease(ms->_source);
        ms->_source = NULL;
    }
    if (NULL == ms->_source && NULL == ms->_dispatchSource && __RSMessagePortIsValid(ms)) {
        RSRunLoopSourceContext1 context;
        context.version = 1;
        context.info = (void *)ms;
        context.retain = (const void *(*)(const void *))RSRetain;
        context.release = (void (*)(const void *))RSRelease;
        context.description = (RSStringRef (*)(const void *))__RSMessagePortCopyDescription;
        context.equal = NULL;
        context.hash = NULL;
        context.getPort = __RSMessagePortGetPort;
        context.perform = __RSMessagePortPerform;
        ms->_source = RSRunLoopSourceCreate(allocator, order, (RSRunLoopSourceContext *)&context);
    }
    if (NULL != ms->_source) {
        result = (RSRunLoopSourceRef)RSRetain(ms->_source);
    }
    __RSMessagePortUnlock(ms);
    return result;
}

void RSMessagePortSetDispatchQueue(RSMessagePortRef ms, dispatch_queue_t queue) {
    __RSGenericValidInstance(ms, __RSMessagePortTypeID);
    __RSMessagePortLock(ms);
    if (!__RSMessagePortIsValid(ms)) {
        __RSMessagePortUnlock(ms);
        RSLog(RSSTR("*** RSMessagePortSetDispatchQueue(): RSMessagePort is invalid"));
        return;
    }
    if (__RSMessagePortIsRemote(ms)) {
        __RSMessagePortUnlock(ms);
        RSLog(RSSTR("*** RSMessagePortSetDispatchQueue(): RSMessagePort is not a local port, queue cannot be set"));
        return;
    }
    if (NULL != ms->_source) {
        __RSMessagePortUnlock(ms);
        RSLog(RSSTR("*** RSMessagePortSetDispatchQueue(): RSMessagePort already has a RSRunLoopSourceRef, queue cannot be set"));
        return;
    }
    
    if (ms->_dispatchSource) {
        dispatch_source_cancel(ms->_dispatchSource);
        ms->_dispatchSource = NULL;
        ms->_dispatchQ = NULL;
    }
    
    if (queue) {
        mach_port_t port = __RSMessagePortGetPort(ms);
        if (MACH_PORT_NULL != port) {
            static dispatch_queue_t mportQueue = NULL;
            static dispatch_once_t once;
            dispatch_once(&once, ^{
                mportQueue = dispatch_queue_create("RSMessagePort Queue", NULL);
            });
            dispatch_source_t theSource = dispatch_source_create(DISPATCH_SOURCE_TYPE_MACH_RECV, port, 0, mportQueue);
            dispatch_source_set_cancel_handler(theSource, ^{
                dispatch_release(queue);
                dispatch_release(theSource);
            });
            dispatch_source_set_event_handler(theSource, ^{
                RSRetain(ms);
                mach_msg_header_t *msg = (mach_msg_header_t *)RSAllocatorAllocate(RSAllocatorSystemDefault, 2048);
                msg->msgh_size = 2048;
                
                for (;;) {
                    msg->msgh_bits = 0;
                    msg->msgh_local_port = port;
                    msg->msgh_remote_port = MACH_PORT_NULL;
                    msg->msgh_id = 0;
                    
                    kern_return_t ret = mach_msg(msg, MACH_RCV_MSG|MACH_RCV_LARGE|MACH_RCV_TRAILER_TYPE(MACH_MSG_TRAILER_FORMAT_0)|MACH_RCV_TRAILER_ELEMENTS(MACH_RCV_TRAILER_AV), 0, msg->msgh_size, port, 0, MACH_PORT_NULL);
                    if (MACH_MSG_SUCCESS == ret) break;
                    if (MACH_RCV_TOO_LARGE != ret) __HALT();
                    
                    uint32_t newSize = round_msg(msg->msgh_size + MAX_TRAILER_SIZE);
                    msg = RSAllocatorReallocate(RSAllocatorSystemDefault, msg, newSize);
                    msg->msgh_size = newSize;
                }
                
                dispatch_async(queue, ^{
                    mach_msg_header_t *reply = __RSMessagePortPerform(msg, msg->msgh_size, RSAllocatorSystemDefault, ms);
                    if (NULL != reply) {
                        kern_return_t ret = mach_msg(reply, MACH_SEND_MSG, reply->msgh_size, 0, MACH_PORT_NULL, 0, MACH_PORT_NULL);
                        if (KERN_SUCCESS != ret) mach_msg_destroy(reply);
                        RSAllocatorDeallocate(RSAllocatorSystemDefault, reply);
                    }
                    RSAllocatorDeallocate(RSAllocatorSystemDefault, msg);
                    RSRelease(ms);
                });
            });
            ms->_dispatchSource = theSource;
        }
        if (ms->_dispatchSource) {
            dispatch_retain(queue);
            ms->_dispatchQ = queue;
            dispatch_resume(ms->_dispatchSource);
        } else {
            RSLog(RSSTR("*** RSMessagePortSetDispatchQueue(): dispatch source could not be created"));
        }
    }
    __RSMessagePortUnlock(ms);
}

