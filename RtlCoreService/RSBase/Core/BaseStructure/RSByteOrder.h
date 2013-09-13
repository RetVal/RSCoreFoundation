//
//  RSByteOrder.h
//  RSCoreFoundation
//
//  Created by Apple Inc.
//  Changed by RetVal on 2/27/13.
//  Copyright (c) 2013 RetVal. All rights reserved.
//

#ifndef RSCoreFoundation_RSByteOrder_h
#define RSCoreFoundation_RSByteOrder_h

#include <RSCoreFoundation/RSBase.h>
#if ((TARGET_OS_MAC && !(TARGET_OS_EMBEDDED || TARGET_OS_IPHONE)) || (TARGET_OS_EMBEDDED || TARGET_OS_IPHONE)) && !defined(RS_USE_OSBYTEORDER_H)
#include <libkern/OSByteOrder.h>
#define RS_USE_OSBYTEORDER_H 1
#endif
RS_EXTERN_C_BEGIN
enum __RSByteOrder
{
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
#endif
