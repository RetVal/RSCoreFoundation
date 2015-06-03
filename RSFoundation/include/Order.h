//
//  Order.h
//  RSFoundation
//
//  Created by closure on 6/2/15.
//  Copyright (c) 2015 RetVal. All rights reserved.
//

#ifndef RSFoundation_Order_h
#define RSFoundation_Order_h

#include <RSFoundation/BasicTypeDefine.h>

namespace RSFoundation {
    namespace Basic {
#if ((TARGET_OS_MAC && !(TARGET_OS_EMBEDDED || TARGET_OS_IPHONE)) || (TARGET_OS_EMBEDDED || TARGET_OS_IPHONE)) && !defined(_USE_OSBYTEORDER_H)
#include <libkern/OSByteOrder.h>
#define _USE_OSBYTEORDER_H 1
#endif
        enum class ByteOrder : Index {
            Unknown,
            LittleEndian,
            BigEndian
        };
        
        inline ByteOrder ByteOrderGetCurrent(void) {
#if _USE_OSBYTEORDER_H
            int32_t byteOrder = OSHostByteOrder();
            switch (byteOrder) {
                case OSLittleEndian: return ByteOrder::LittleEndian;
                case OSBigEndian: return ByteOrder::BigEndian;
                default: break;
            }
            return ByteOrder::Unknown;
#else
#if __LITTLE_ENDIAN__
            return ByteOrder::LittleEndian;
#elif __BIG_ENDIAN__
            return ByteOrder::BigEndian;
#else
            return ByteOrder::Unknown;
#endif
#endif
        }
        
        inline uint16_t SwapInt16(uint16_t arg) {
#if _USE_OSBYTEORDER_H
            return OSSwapInt16(arg);
#else
            uint16_t result;
            result = (uint16_t)(((arg << 8) & 0xFF00) | ((arg >> 8) & 0xFF));
            return result;
#endif
        }
        
        inline uint32_t SwapInt32(uint32_t arg) {
#if _USE_OSBYTEORDER_H
            return OSSwapInt32(arg);
#else
            uint32_t result;
            result = ((arg & 0xFF) << 24) | ((arg & 0xFF00) << 8) | ((arg >> 8) & 0xFF00) | ((arg >> 24) & 0xFF);
            return result;
#endif
        }
        
        inline uint64_t SwapInt64(uint64_t arg) {
#if _USE_OSBYTEORDER_H
            return OSSwapInt64(arg);
#else
            union Swap {
                uint64_t sv;
                uint32_t ul[2];
            } tmp, result;
            tmp.sv = arg;
            result.ul[0] = SwapInt32(tmp.ul[1]);
            result.ul[1] = SwapInt32(tmp.ul[0]);
            return result.sv;
#endif
        }
        
        inline uint16_t SwapInt16BigToHost(uint16_t arg) {
#if _USE_OSBYTEORDER_H
            return OSSwapBigToHostInt16(arg);
#elif __BIG_ENDIAN__
            return arg;
#else
            return SwapInt16(arg);
#endif
        }
        
        inline uint32_t SwapInt32BigToHost(uint32_t arg) {
#if _USE_OSBYTEORDER_H
            return OSSwapBigToHostInt32(arg);
#elif __BIG_ENDIAN__
            return arg;
#else
            return SwapInt32(arg);
#endif
        }
        
        inline uint64_t SwapInt64BigToHost(uint64_t arg) {
#if _USE_OSBYTEORDER_H
            return OSSwapBigToHostInt64(arg);
#elif __BIG_ENDIAN__
            return arg;
#else
            return SwapInt64(arg);
#endif
        }
        
        inline uint16_t SwapInt16HostToBig(uint16_t arg) {
#if _USE_OSBYTEORDER_H
            return OSSwapHostToBigInt16(arg);
#elif __BIG_ENDIAN__
            return arg;
#else
            return SwapInt16(arg);
#endif
        }
        
        inline uint32_t SwapInt32HostToBig(uint32_t arg) {
#if _USE_OSBYTEORDER_H
            return OSSwapHostToBigInt32(arg);
#elif __BIG_ENDIAN__
            return arg;
#else
            return SwapInt32(arg);
#endif
        }
        
        inline uint64_t SwapInt64HostToBig(uint64_t arg) {
#if _USE_OSBYTEORDER_H
            return OSSwapHostToBigInt64(arg);
#elif __BIG_ENDIAN__
            return arg;
#else
            return SwapInt64(arg);
#endif
        }
        
        inline uint16_t SwapInt16LittleToHost(uint16_t arg) {
#if _USE_OSBYTEORDER_H
            return OSSwapLittleToHostInt16(arg);
#elif __LITTLE_ENDIAN__
            return arg;
#else
            return SwapInt16(arg);
#endif
        }
        
        inline uint32_t SwapInt32LittleToHost(uint32_t arg) {
#if _USE_OSBYTEORDER_H
            return OSSwapLittleToHostInt32(arg);
#elif __LITTLE_ENDIAN__
            return arg;
#else
            return SwapInt32(arg);
#endif
        }
        
        inline uint64_t SwapInt64LittleToHost(uint64_t arg) {
#if _USE_OSBYTEORDER_H
            return OSSwapLittleToHostInt64(arg);
#elif __LITTLE_ENDIAN__
            return arg;
#else
            return SwapInt64(arg);
#endif
        }
        
        inline uint16_t SwapInt16HostToLittle(uint16_t arg) {
#if _USE_OSBYTEORDER_H
            return OSSwapHostToLittleInt16(arg);
#elif __LITTLE_ENDIAN__
            return arg;
#else
            return SwapInt16(arg);
#endif
        }
        
        inline uint32_t SwapInt32HostToLittle(uint32_t arg) {
#if _USE_OSBYTEORDER_H
            return OSSwapHostToLittleInt32(arg);
#elif __LITTLE_ENDIAN__
            return arg;
#else
            return SwapInt32(arg);
#endif
        }
        
        inline uint64_t SwapInt64HostToLittle(uint64_t arg) {
#if _USE_OSBYTEORDER_H
            return OSSwapHostToLittleInt64(arg);
#elif __LITTLE_ENDIAN__
            return arg;
#else
            return SwapInt64(arg);
#endif
        }
        
        typedef struct {uint32_t v;} SwappedFloat32;
        typedef struct {uint64_t v;} SwappedFloat64;
        
        inline SwappedFloat32 ConvertFloat32HostToSwapped(Float32 arg) {
            union Swap {
                Float32 v;
                SwappedFloat32 sv;
            } result;
            result.v = arg;
#if __LITTLE_ENDIAN__
            result.sv.v = SwapInt32(result.sv.v);
#endif
            return result.sv;
        }
        
        inline Float32 ConvertFloat32SwappedToHost(SwappedFloat32 arg) {
            union Swap {
                Float32 v;
                SwappedFloat32 sv;
            } result;
            result.sv = arg;
#if __LITTLE_ENDIAN__
            result.sv.v = SwapInt32(result.sv.v);
#endif
            return result.v;
        }
        
        inline SwappedFloat64 ConvertFloat64HostToSwapped(Float64 arg) {
            union Swap {
                Float64 v;
                SwappedFloat64 sv;
            } result;
            result.v = arg;
#if __LITTLE_ENDIAN__
            result.sv.v = SwapInt64(result.sv.v);
#endif
            return result.sv;
        }
        
        inline Float64 ConvertFloat64SwappedToHost(SwappedFloat64 arg) {
            union Swap {
                Float64 v;
                SwappedFloat64 sv;
            } result;
            result.sv = arg;
#if __LITTLE_ENDIAN__
            result.sv.v = SwapInt64(result.sv.v);
#endif
            return result.v;
        }
        
        inline SwappedFloat32 ConvertFloatHostToSwapped(float arg) {
            union Swap {
                float v;
                SwappedFloat32 sv;
            } result;
            result.v = arg;
#if __LITTLE_ENDIAN__
            result.sv.v = SwapInt32(result.sv.v);
#endif
            return result.sv;
        }
        
        inline float ConvertFloatSwappedToHost(SwappedFloat32 arg) {
            union Swap {
                float v;
                SwappedFloat32 sv;
            } result;
            result.sv = arg;
#if __LITTLE_ENDIAN__
            result.sv.v = SwapInt32(result.sv.v);
#endif
            return result.v;
        }
        
        inline SwappedFloat64 ConvertDoubleHostToSwapped(double arg) {
            union Swap {
                double v;
                SwappedFloat64 sv;
            } result;
            result.v = arg;
#if __LITTLE_ENDIAN__
            result.sv.v = SwapInt64(result.sv.v);
#endif
            return result.sv;
        }
        
        inline double ConvertDoubleSwappedToHost(SwappedFloat64 arg) {
            union Swap {
                double v;
                SwappedFloat64 sv;
            } result;
            result.sv = arg;
#if __LITTLE_ENDIAN__
            result.sv.v = SwapInt64(result.sv.v);
#endif
            return result.v;
        }
    }
}

#endif
