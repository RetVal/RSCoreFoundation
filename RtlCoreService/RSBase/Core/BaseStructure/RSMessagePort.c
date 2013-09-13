//
//  RSMachPort.c
//  RSKit
//
//  Created by RetVal on 12/15/12.
//  Copyright (c) 2012 RetVal. All rights reserved.
//

#include <stdio.h>
#include <RSKit/RSMessagePort.h>
#include <RSKit/RSBase.h>
#include <servers/bootstrap.h>  // for notification center.

#define __RSMessagePortMaxInlineBytes 4096*10
#define __RSMessagePortMaxDataSize 0x60000000L

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

#if ((TARGET_OS_MAC && !(TARGET_OS_EMBEDDED || TARGET_OS_IPHONE)) || (TARGET_OS_EMBEDDED || TARGET_OS_IPHONE)) && !defined(RS_USE_OSBYTEORDER_H)
#include <libkern/OSByteOrder.h>
#define RS_USE_OSBYTEORDER_H 1
#endif

RS_EXTERN_C_BEGIN

enum __RSByteOrder {
    RSByteOrderUnknown,
    RSByteOrderLittleEndian,
    RSByteOrderBigEndian
};
typedef RSIndex RSByteOrder;

RSInline RSByteOrder RSByteOrderGetCurrent(void) {
#if RS_USE_OSBYTEORDER_H
    int32_t byteOrder = OSHostByteOrder();
    switch (byteOrder) {
        case OSLittleEndian: return RSByteOrderLittleEndian;
        case OSBigEndian: return RSByteOrderBigEndian;
        default: break;
    }
    return RSByteOrderUnknown;
#else
#if __LITTLE_ENDIAN__
    return RSByteOrderLittleEndian;
#elif __BIG_ENDIAN__
    return RSByteOrderBigEndian;
#else
    return RSByteOrderUnknown;
#endif
#endif
}

RSInline uint16_t RSSwapInt16(uint16_t arg) {
#if RS_USE_OSBYTEORDER_H
    return OSSwapInt16(arg);
#else
    uint16_t result;
    result = (uint16_t)(((arg << 8) & 0xFF00) | ((arg >> 8) & 0xFF));
    return result;
#endif
}

RSInline uint32_t RSSwapInt32(uint32_t arg) {
#if RS_USE_OSBYTEORDER_H
    return OSSwapInt32(arg);
#else
    uint32_t result;
    result = ((arg & 0xFF) << 24) | ((arg & 0xFF00) << 8) | ((arg >> 8) & 0xFF00) | ((arg >> 24) & 0xFF);
    return result;
#endif
}

RSInline uint64_t RSSwapInt64(uint64_t arg) {
#if RS_USE_OSBYTEORDER_H
    return OSSwapInt64(arg);
#else
    union RSSwap {
        uint64_t sv;
        uint32_t ul[2];
    } tmp, result;
    tmp.sv = arg;
    result.ul[0] = RSSwapInt32(tmp.ul[1]);
    result.ul[1] = RSSwapInt32(tmp.ul[0]);
    return result.sv;
#endif
}

RSInline uint16_t RSSwapInt16BigToHost(uint16_t arg) {
#if RS_USE_OSBYTEORDER_H
    return OSSwapBigToHostInt16(arg);
#elif __BIG_ENDIAN__
    return arg;
#else
    return RSSwapInt16(arg);
#endif
}

RSInline uint32_t RSSwapInt32BigToHost(uint32_t arg) {
#if RS_USE_OSBYTEORDER_H
    return OSSwapBigToHostInt32(arg);
#elif __BIG_ENDIAN__
    return arg;
#else
    return RSSwapInt32(arg);
#endif
}

RSInline uint64_t RSSwapInt64BigToHost(uint64_t arg) {
#if RS_USE_OSBYTEORDER_H
    return OSSwapBigToHostInt64(arg);
#elif __BIG_ENDIAN__
    return arg;
#else
    return RSSwapInt64(arg);
#endif
}

RSInline uint16_t RSSwapInt16HostToBig(uint16_t arg) {
#if RS_USE_OSBYTEORDER_H
    return OSSwapHostToBigInt16(arg);
#elif __BIG_ENDIAN__
    return arg;
#else
    return RSSwapInt16(arg);
#endif
}

RSInline uint32_t RSSwapInt32HostToBig(uint32_t arg) {
#if RS_USE_OSBYTEORDER_H
    return OSSwapHostToBigInt32(arg);
#elif __BIG_ENDIAN__
    return arg;
#else
    return RSSwapInt32(arg);
#endif
}

RSInline uint64_t RSSwapInt64HostToBig(uint64_t arg) {
#if RS_USE_OSBYTEORDER_H
    return OSSwapHostToBigInt64(arg);
#elif __BIG_ENDIAN__
    return arg;
#else
    return RSSwapInt64(arg);
#endif
}

RSInline uint16_t RSSwapInt16LittleToHost(uint16_t arg) {
#if RS_USE_OSBYTEORDER_H
    return OSSwapLittleToHostInt16(arg);
#elif __LITTLE_ENDIAN__
    return arg;
#else
    return RSSwapInt16(arg);
#endif
}

RSInline uint32_t RSSwapInt32LittleToHost(uint32_t arg) {
#if RS_USE_OSBYTEORDER_H
    return OSSwapLittleToHostInt32(arg);
#elif __LITTLE_ENDIAN__
    return arg;
#else
    return RSSwapInt32(arg);
#endif
}

RSInline uint64_t RSSwapInt64LittleToHost(uint64_t arg) {
#if RS_USE_OSBYTEORDER_H
    return OSSwapLittleToHostInt64(arg);
#elif __LITTLE_ENDIAN__
    return arg;
#else
    return RSSwapInt64(arg);
#endif
}

RSInline uint16_t RSSwapInt16HostToLittle(uint16_t arg) {
#if RS_USE_OSBYTEORDER_H
    return OSSwapHostToLittleInt16(arg);
#elif __LITTLE_ENDIAN__
    return arg;
#else
    return RSSwapInt16(arg);
#endif
}

RSInline uint32_t RSSwapInt32HostToLittle(uint32_t arg) {
#if RS_USE_OSBYTEORDER_H
    return OSSwapHostToLittleInt32(arg);
#elif __LITTLE_ENDIAN__
    return arg;
#else
    return RSSwapInt32(arg);
#endif
}

RSInline uint64_t RSSwapInt64HostToLittle(uint64_t arg) {
#if RS_USE_OSBYTEORDER_H
    return OSSwapHostToLittleInt64(arg);
#elif __LITTLE_ENDIAN__
    return arg;
#else
    return RSSwapInt64(arg);
#endif
}

typedef struct {uint32_t v;} RSSwappedFloat32;
typedef struct {uint64_t v;} RSSwappedFloat64;

RSInline RSSwappedFloat32 RSConvertFloat32HostToSwapped(Float32 arg) {
    union RSSwap {
        Float32 v;
        RSSwappedFloat32 sv;
    } result;
    result.v = arg;
#if __LITTLE_ENDIAN__
    result.sv.v = RSSwapInt32(result.sv.v);
#endif
    return result.sv;
}

RSInline Float32 RSConvertFloat32SwappedToHost(RSSwappedFloat32 arg) {
    union RSSwap {
        Float32 v;
        RSSwappedFloat32 sv;
    } result;
    result.sv = arg;
#if __LITTLE_ENDIAN__
    result.sv.v = RSSwapInt32(result.sv.v);
#endif
    return result.v;
}

RSInline RSSwappedFloat64 RSConvertFloat64HostToSwapped(Float64 arg) {
    union RSSwap {
        Float64 v;
        RSSwappedFloat64 sv;
    } result;
    result.v = arg;
#if __LITTLE_ENDIAN__
    result.sv.v = RSSwapInt64(result.sv.v);
#endif
    return result.sv;
}

RSInline Float64 RSConvertFloat64SwappedToHost(RSSwappedFloat64 arg) {
    union RSSwap {
        Float64 v;
        RSSwappedFloat64 sv;
    } result;
    result.sv = arg;
#if __LITTLE_ENDIAN__
    result.sv.v = RSSwapInt64(result.sv.v);
#endif
    return result.v;
}

RSInline RSSwappedFloat32 RSConvertFloatHostToSwapped(float arg) {
    union RSSwap {
        float v;
        RSSwappedFloat32 sv;
    } result;
    result.v = arg;
#if __LITTLE_ENDIAN__
    result.sv.v = RSSwapInt32(result.sv.v);
#endif
    return result.sv;
}

RSInline float RSConvertFloatSwappedToHost(RSSwappedFloat32 arg) {
    union RSSwap {
        float v;
        RSSwappedFloat32 sv;
    } result;
    result.sv = arg;
#if __LITTLE_ENDIAN__
    result.sv.v = RSSwapInt32(result.sv.v);
#endif
    return result.v;
}

RSInline RSSwappedFloat64 RSConvertDoubleHostToSwapped(double arg) {
    union RSSwap {
        double v;
        RSSwappedFloat64 sv;
    } result;
    result.v = arg;
#if __LITTLE_ENDIAN__
    result.sv.v = RSSwapInt64(result.sv.v);
#endif
    return result.sv;
}

RSInline double RSConvertDoubleSwappedToHost(RSSwappedFloat64 arg) {
    union RSSwap {
        double v;
        RSSwappedFloat64 sv;
    } result;
    result.sv = arg;
#if __LITTLE_ENDIAN__
    result.sv.v = RSSwapInt64(result.sv.v);
#endif
    return result.v;
}

RS_EXTERN_C_END

#define MAGIC 0xF1F2F3F4

#define MSGP0_FIELD(msgp, ident) ((struct __RSMessagePortMachMessage0 *)msgp)->ident
#define MSGP1_FIELD(msgp, ident) ((struct __RSMessagePortMachMessage1 *)msgp)->ident
#define MSGP_GET(msgp, ident) \
((((mach_msg_base_t *)msgp)->body.msgh_descriptor_count) ? MSGP1_FIELD(msgp, ident) : MSGP0_FIELD(msgp, ident))

static mach_msg_base_t *__RSMessagePortCreateMessage(bool reply, mach_port_t port, mach_port_t replyPort, int32_t convid, int32_t msgid, const uint8_t *bytes, int32_t byteslen)
{
    if (__RSMessagePortMaxDataSize < byteslen) return NULL;
    int32_t rounded_byteslen = ((byteslen + 3) & ~0x3);
    if (rounded_byteslen <= __RSMessagePortMaxInlineBytes)
    {
        int32_t size = sizeof(struct __RSMessagePortMachMessage0) + rounded_byteslen;
        struct __RSMessagePortMachMessage0 *msg = nil;
        RSAllocatorAllocate(kRSAllocatorSystemDefault, (RSZone)&msg, size);
        if (!msg) return NULL;
        //memset(msg, 0, size); //kRSAllocatorSystemDefault do it automatically.
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
    }
    else
    {
        int32_t size = sizeof(struct __RSMessagePortMachMessage1);
        struct __RSMessagePortMachMessage1 *msg = nil;
        RSAllocatorAllocate(kRSAllocatorSystemDefault, (RSZone)&msg, size);
        if (!msg) return NULL;
        //memset(msg, 0, size); //kRSAllocatorSystemDefault do it automatically.
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

enum {
    kRSMessagePortSend = 1 << 0,    // send right
    kRSMessagePortSend1 = 1 << 1,   // send once
    kRSMessagePortRcve = 1 << 3,    // recv right
    kRSMessagePortFreeContext = 1 << 4,
    kRSMessagePortLocal = 1 << 5,   // only support local IPC right now, remote message please use RSSocket-API.
    kRSMessagePortValid = 1 << 6,
};

struct __RSMessagePort
{
    RSRuntimeBase _base;
    RSSpinLock _lock;
    RSRunLoopSourceRef _source;
    
    RSStringRef _name;
    mach_port_t _port;
    
    mach_port_t _replyPort;
    
    RSMessagePortCallBack _callback;
    RSMessagePortContext _context;
    union
    {
        mach_msg_base_t* _msg;
        struct __RSMessagePortMachMessage0* _msg0;
        struct __RSMessagePortMachMessage1* _msg1;
    };
};

RSInline void __RSMessagePortLock(RSMessagePortRef port)
{
    RSSpinLockLock(&((struct __RSMessagePort*)port)->_lock);
}

RSInline void __RSMessagePortUnLock(RSMessagePortRef port)
{
    RSSpinLockUnLock(&((struct __RSMessagePort*)port)->_lock);
}

RSInline BOOL isMessagePortLocal(RSMessagePortRef port)
{
    return (port->_base._rsinfo._rsinfo1 & kRSMessagePortLocal) == kRSMessagePortLocal;
}

RSInline BOOL isMessagePortRemote(RSMessagePortRef port)
{
    return !isMessagePortLocal(port);
}

RSInline void setMessagePortLocal(RSMessagePortRef port)
{
    ((struct __RSMessagePort*)port)->_base._rsinfo._rsinfo1 |= kRSMessagePortLocal;
}

RSInline void setMessagePortRemote(RSMessagePortRef port)
{
    ((struct __RSMessagePort*)port)->_base._rsinfo._rsinfo1 &= ~kRSMessagePortLocal;;
}

RSInline void setMessagePortValid(RSMessagePortRef port)
{
    ((struct __RSMessagePort*)port)->_base._rsinfo._rsinfo1 |= kRSMessagePortValid;
}

RSInline void setMessagePortInvalid(RSMessagePortRef port)
{
    ((struct __RSMessagePort*)port)->_base._rsinfo._rsinfo1 &= ~kRSMessagePortValid;
}

RSInline BOOL isMessagePortValid(RSMessagePortRef port)
{
    return ((port->_base._rsinfo._rsinfo1 & kRSMessagePortValid) == kRSMessagePortValid);
}

RSInline BOOL shouldFreeContext(RSMessagePortRef port)
{
    return ((port->_base._rsinfo._rsinfo & kRSMessagePortFreeContext) == kRSMessagePortFreeContext);
}

RSInline void setShouldFreeContext(RSMessagePortRef port)
{
    __RSBitfieldSetValue(RSRuntimeClassBaseFiled(port), kRSMessagePortFreeContext, kRSMessagePortFreeContext, 1);
}

RSInline void unsetShouldFreeContext(RSMessagePortRef port)
{
    __RSBitfieldSetValue(RSRuntimeClassBaseFiled(port), kRSMessagePortFreeContext, kRSMessagePortFreeContext, 0);
}

static void __RSMessagePortClassInit(RSTypeRef rs)
{
    struct __RSMessagePort* port = (struct __RSMessagePort*)rs;
    setMessagePortLocal(port);
    setMessagePortInvalid(port);
    port->_name = nil;
    port->_lock = RSSpinLockInit;
    port->_msg  = nil;
    port->_port = MACH_PORT_NULL;
    unsetShouldFreeContext(port);
    return;
}

static void __RSMessagePortClassDeallocate(RSTypeRef rs)
{
    struct __RSMessagePort* port = (struct __RSMessagePort*)rs;
    __RSMessagePortLock(port);
    if (isMessagePortValid(port))
    {
        mach_port_deallocate(current_task(), port->_port);
    }
    if (port->_name)
    {
        RSRelease(port->_name);
    }
    __RSMessagePortUnLock(port);
    return;
}

static RSHashCode __RSMessagePortClassHash(RSTypeRef rs)
{
    return ((RSMessagePortRef)rs)->_port;
}

static RSStringRef __RSMessagePortClassDescription(RSTypeRef rs)
{
    RSMutableStringRef description = RSStringCreateMutable(kRSAllocatorDefault, 128);
    RSMessagePortRef port = (RSMessagePortRef)rs;
    RSStringAppendStringWithFormat(description, RSSTR("Message Port = %d <%s>\n Named = %R"),
                                   port->_port,
                                   (isMessagePortValid(port) ? "Valid" : "Invalid"),
                                   port->_name ?: RSSTR(""));
    return description;
}

static BOOL __RSMessagePortClassEqual(RSTypeRef rs1, RSTypeRef rs2)
{
    RSMessagePortRef port1 = (RSMessagePortRef)rs1, port2 = (RSMessagePortRef)rs2;
    BOOL result = NO;
    __RSMessagePortLock(port1);__RSMessagePortLock(port2);
    do
    {
        if (port1->_port != port2->_port) BWI(1);
        if (port1->_replyPort != port2->_replyPort) BWI(1);
        if (RSEqual(port1->_name, port2->_name) == NO) BWI(1);
        BWI(result = YES);
    } while (0);
    __RSMessagePortUnLock(port1);__RSMessagePortUnLock(port2);
    return result;
}

static const RSRuntimeClass __RSMessagePortClass =
{
    _kRSRuntimeScannedObject,
    "RSMessagePort",
    __RSMessagePortClassInit,
    nil,
    __RSMessagePortClassDeallocate,
    __RSMessagePortClassEqual,
    __RSMessagePortClassHash,
    __RSMessagePortClassDescription,
    nil,
    nil,
};

static RSTypeID _kRSMessagePortTypeID = _kRSRuntimeNotATypeID;

RSPrivate void __RSMessagePortInitialize()
{
    _kRSMessagePortTypeID = __RSRuntimeRegisterClass(&__RSMessagePortClass);
    __RSRuntimeSetClassTypeID(&__RSMessagePortClass, _kRSMessagePortTypeID);
}

RSExport RSTypeID RSMessagePortTypeID()
{
    return _kRSMessagePortTypeID;
}

RSInline void __RSMessagePortAvailable(RSMessagePortRef port)
{
    __RSGenericValidInstance(port, _kRSMessagePortTypeID);
}

enum
{
    __kRSMessagePortCreateLocal = 0,
    __kRSMessagePortCreateRemote = 1,
    __kRSMessagePortCreateWithMachPort = 2,
    __kRSMessagePortCreateWithMachPortAndReplyPort = 3,
};

RSPrivate RSMessagePortRef __RSMessagePortGeneralCreator(RSAllocatorRef allocator, mach_port_t port, mach_port_t replyPort, RSStringRef name, int flag, RSMessagePortCallBack cb, RSMessagePortContext* context, BOOL shouldFreeContext)
{
    struct __RSMessagePort* memory = (struct __RSMessagePort*)__RSRuntimeCreateInstance(allocator, _kRSMessagePortTypeID, sizeof(struct __RSMessagePort) - sizeof(struct __RSRuntimeBase));
//    memory->_name = RSRetain(name);
//    if (port)
//    {
//        memory->_port = port;
//        setMessagePortValid(memory);
//    }
//    memory->_replyPort = replyPort;
//    if (cb) memory->_callback = cb;
//    if (context) memcpy(&memory->_context, context, sizeof(RSMessagePortContext));
//    if (shouldFreeContext) setShouldFreeContext(memory);
    if ((flag & __kRSMessagePortCreateLocal) && (flag & __kRSMessagePortCreateRemote)) HALTWithError("");
    if (flag & __kRSMessagePortCreateLocal)
    {
        kern_return_t result = bootstrap_check_in(bootstrap_port, __RSNotificationIPCName, &memory->_port);
    
    }
    else if (flag & __kRSMessagePortCreateRemote)
    {
        
    }
    return memory;
}

RSExport mach_port_t RSMessagePortGetMachPort(RSMessagePortRef port)
{
    __RSMessagePortAvailable(port);
    return port->_port;
}

RSExport BOOL RSMessagePortIsValid(RSMessagePortRef port)
{
    __RSMessagePortAvailable(port);
    return isMessagePortValid(port);
}

RSExport void RSMessagePortInvalid(RSMessagePortRef port)
{
    if (RSMessagePortIsValid(port))
    {
        setMessagePortInvalid(port);
        mach_port_deallocate(current_task(), port->_port);
    }
    return;
}

RSExport RSMessagePortRef RSMessagePortCreateWithMachPort(RSAllocatorRef allocator, mach_port_t port, RSStringRef name, RSMessagePortCallBack callback, RSMessagePortContext* context, BOOL shouldFreeContext)
{
    return __RSMessagePortGeneralCreator(allocator, port, nil, name, YES, callback, context, shouldFreeContext);
}

RSExport RSMessagePortRef RSMessagePortCreateLocal(RSAllocatorRef allocator, RSStringRef name, RSMessagePortCallBack cb, RSMessagePortContext* context, BOOL shouldFreeContext)
{
    if (name == nil) HALTWithError("the name must be non-nil.");
    if (cb == nil) HALTWithError("the call is no meaning.");
    
    return nil;
}

RSInside RSErrorCode __RSMessagePortSendTo(RSMessagePortRef from, RSMessagePortRef to)
{
    //task_set_bootstrap_port(task_self_trap(), to->_port);

}

RSExport BOOL RSMessagePortSendRequest(RSMessagePortRef remote, RSDataRef request, int32_t convid, int32_t msgid, RSTimeInterval timeout)
{
    __RSMessagePortAvailable(remote);
    if (likely(request != nil))
    {
        RSIndex length = RSDataGetLength(request);
        return YES;
    }
    return NO;
}