//
//  RSUUID.c
//  RSCoreFoundation
//
//  Created by RetVal on 11/17/12.
//  Copyright (c) 2012 RetVal. All rights reserved.
//

#include <stdio.h>
#include <RSCoreFoundation/RSUUID.h>
#include <RSCoreFoundation/RSRuntime.h>

struct __RSUUID {
    RSRuntimeBase _base;
    RSUUIDBytes _bytes;
};

static BOOL __RSUUIDEqual(RSTypeRef obj1, RSTypeRef obj2)
{
    RSUUIDRef uuid1 = (RSUUIDRef)obj1;
    RSUUIDRef uuid2 = (RSUUIDRef)obj2;
    RSIndex cnt = 16;
    for (RSIndex idx = 0; idx < cnt; idx++)
    {
        if (uuid1->_bytes.byte[idx] != uuid2->_bytes.byte[idx])
            return NO;
    }
    return YES;
}

static RSTypeRef __RSUUIDCopy(RSAllocatorRef allocator, RSTypeRef rs, BOOL mutableCopy)
{
    return RSUUIDCreateWithUUIDBytes(allocator, RSUUIDGetBytes(rs));
}

static RSStringRef __RSUUIDDescription(RSTypeRef obj)
{
    RSUUIDRef uuid = (RSUUIDRef)obj;
    RSStringRef description = RSUUIDCreateString(RSAllocatorSystemDefault, uuid);
    return description;
}

static const RSRuntimeClass __RSUUIDClass = {
    _RSRuntimeScannedObject,
    "RSUUID",
    nil,
    __RSUUIDCopy,
    nil,
    __RSUUIDEqual,
    nil,
    __RSUUIDDescription,
    nil,
    nil,
};

static RSTypeID __RSUUIDTypeID = _RSRuntimeNotATypeID;

RSPrivate void __RSUUIDInitialize()
{
    __RSUUIDTypeID = __RSRuntimeRegisterClass(&__RSUUIDClass);
    __RSRuntimeSetClassTypeID(&__RSUUIDClass, __RSUUIDTypeID);
}

RSExport RSTypeID RSUUIDGetTypeID()
{
    return __RSUUIDTypeID;
}
static void _intToHexChars(RSBitU32 in, UniChar* out, int digits)
{
    int shift;
    RSBitU32 d;
    
    while (--digits >= 0) {
        shift = digits << 2;
        d = 0x0FL & (in >> shift);
        if (d <= 9) {
            *out++ = (UniChar)'0' + d;
        } else {
            *out++ = (UniChar)'A' + (d - 10);
        }
    }
}

static RSBitU8 _byteFromHexChars(UniChar* in) {
    RSBitU8 result = 0;
    UniChar c;
    RSBitU8 d;
    RSIndex i;
    
    for (i=0; i<2; i++) {
        c = in[i];
        if ((c >= (UniChar)'0') && (c <= (UniChar)'9')) {
            d = c - (UniChar)'0';
        } else if ((c >= (UniChar)'a') && (c <= (UniChar)'f')) {
            d = c - ((UniChar)'a' - 10);
        } else if ((c >= (UniChar)'A') && (c <= (UniChar)'F')) {
            d = c - ((UniChar)'A' - 10);
        } else {
            return 0;
        }
        result = (result << 4) | d;
    }
    
    return result;
}

RSInline BOOL _isHexChar(UniChar c) {
    return ((((c >= (UniChar)'0') && (c <= (UniChar)'9')) || ((c >= (UniChar)'a') && (c <= (UniChar)'f')) || ((c >= (UniChar)'A') && (c <= (UniChar)'F'))) ? YES : NO);
}

#define READ_A_BYTE(into) if (i+1 < len) { \
(into) = _byteFromHexChars(&(chars[i])); \
i+=2; \
}

#if DEPLOYMENT_TARGET_WINDOWS
#include <Rpc.h>
#elif DEPLOYMENT_TARGET_MACOSX || DEPLOYMENT_TARGET_EMBEDDED
#include <uuid/uuid.h>
#endif

static RSUUIDRef __RSUUIDCreateWithBytesPrimitive(RSAllocatorRef allocator, RSUUIDBytes bytes, BOOL isConst)
{
    struct __RSUUID* uuid = (struct __RSUUID*)__RSRuntimeCreateInstance(allocator, RSUUIDGetTypeID(), sizeof(struct __RSUUID) - sizeof(struct __RSRuntimeBase));
    uuid->_bytes = bytes;
    return uuid;
}

RSExport RSUUIDRef RSUUIDCreate(RSAllocatorRef allocator)
{
    /* Create a new bytes struct and then call the primitive. */
     RSUUIDBytes bytes;
     uint32_t retval = 0;
    
#if DEPLOYMENT_TARGET_WINDOWS
        UUID u;
        long rStatus = UuidCreate(&u);
        if (RPC_S_OK != rStatus && RPC_S_UUID_LOCAL_ONLY != rStatus) retval = 1;
        memmove(&bytes, &u, sizeof(bytes));
#elif DEPLOYMENT_TARGET_MACOSX || DEPLOYMENT_TARGET_EMBEDDED
        static BOOL useV1UUIDs = NO, checked = NO;
        uuid_t uuid;
        if (!checked)
        {
            const char *value = getenv("CFUUIDVersionNumber");
            if (value) {
                if (1 == strtoul_l(value, NULL, 0, NULL)) useV1UUIDs = YES;
            }
            checked = YES;
        }
        if (useV1UUIDs) uuid_generate_time(uuid); else uuid_generate_random(uuid);
        memcpy((void *)&bytes, uuid, sizeof(uuid));
#else
        retval = 1;
#endif
    return (retval == 0) ? __RSUUIDCreateWithBytesPrimitive(allocator, bytes, NO) : NULL;
}

RSExport RSUUIDRef RSUUIDCreateWithBytes(RSAllocatorRef allocator,
                                         RSBitU8 b1,  RSBitU8 b2,  RSBitU8 b3,  RSBitU8 b4,
                                         RSBitU8 b5,  RSBitU8 b6,  RSBitU8 b7,  RSBitU8 b8,
                                         RSBitU8 b9,  RSBitU8 b10, RSBitU8 b11, RSBitU8 b12,
                                         RSBitU8 b13, RSBitU8 b14, RSBitU8 b15, RSBitU8 b16)
{
    RSUUIDBytes bytes = {0};
    bytes.byte[0] = b1;
    bytes.byte[1] = b2;
    bytes.byte[2] = b3;
    bytes.byte[3] = b4;
    bytes.byte[4] = b5;
    bytes.byte[5] = b6;
    bytes.byte[6] = b7;
    bytes.byte[7] = b8;
    bytes.byte[8] = b9;
    bytes.byte[9] = b10;
    bytes.byte[10] = b11;
    bytes.byte[11] = b12;
    bytes.byte[12] = b13;
    bytes.byte[13] = b14;
    bytes.byte[14] = b15;
    bytes.byte[15] = b16;
    return __RSUUIDCreateWithBytesPrimitive(allocator, bytes, NO);
}
RSExport RSUUIDRef RSUUIDCreateWithString(RSAllocatorRef allocator, RSStringRef uuidString)
{
    if (uuidString == nil) return nil;
    RSUUIDBytes bytes = {0};
    UniChar chars[100] = {0};
    RSIndex len = RSStringGetLength(uuidString), i = 0;
    if (len > 100)
    {
        len = 100;
    }
    else if (len == 0)
    {
        return NULL;
    }
    RSStringGetCharacters(uuidString, RSMakeRange(0, len), chars);
    memset((void *)&bytes, 0, sizeof(bytes));
    
    /* Skip initial random stuff */
    while (!_isHexChar(chars[i]) && i < len) i++;
    
    READ_A_BYTE(bytes.byte[0]);
    READ_A_BYTE(bytes.byte[1]);
    READ_A_BYTE(bytes.byte[2]);
    READ_A_BYTE(bytes.byte[3]);
    i++;
    
    READ_A_BYTE(bytes.byte[4]);
    READ_A_BYTE(bytes.byte[5]);
    i++;
    
    READ_A_BYTE(bytes.byte[6]);
    READ_A_BYTE(bytes.byte[7]);
    i++;
    
    READ_A_BYTE(bytes.byte[8]);
    READ_A_BYTE(bytes.byte[9]);
    i++;
    
    READ_A_BYTE(bytes.byte[10]);
    READ_A_BYTE(bytes.byte[11]);
    READ_A_BYTE(bytes.byte[12]);
    READ_A_BYTE(bytes.byte[13]);
    READ_A_BYTE(bytes.byte[14]);
    READ_A_BYTE(bytes.byte[15]);
    
    return __RSUUIDCreateWithBytesPrimitive(allocator, bytes, NO);
}

RSExport RSUUIDRef RSUUIDCreateWithUUIDBytes(RSAllocatorRef allocator, RSUUIDBytes uuidBytes)
{
    return __RSUUIDCreateWithBytesPrimitive(allocator, uuidBytes, NO);
}
RSExport RSStringRef RSUUIDCreateString(RSAllocatorRef allocator, RSUUIDRef uuid)
{
    if (uuid == nil) return nil;
    RSMutableStringRef str = RSStringCreateMutable(allocator, 0);
    UniChar buff[12];
    
    // First segment (4 bytes, 8 digits + 1 dash)
    _intToHexChars(uuid->_bytes.byte[0], buff, 2);
    _intToHexChars(uuid->_bytes.byte[1], &(buff[2]), 2);
    _intToHexChars(uuid->_bytes.byte[2], &(buff[4]), 2);
    _intToHexChars(uuid->_bytes.byte[3], &(buff[6]), 2);
    buff[8] = (UniChar)'-';
    RSStringAppendCharacters(str, buff, 9);
    
    // Second segment (2 bytes, 4 digits + 1 dash)
    _intToHexChars(uuid->_bytes.byte[4], buff, 2);
    _intToHexChars(uuid->_bytes.byte[5], &(buff[2]), 2);
    buff[4] = (UniChar)'-';
    RSStringAppendCharacters(str, buff, 5);
    
    // Third segment (2 bytes, 4 digits + 1 dash)
    _intToHexChars(uuid->_bytes.byte[6], buff, 2);
    _intToHexChars(uuid->_bytes.byte[7], &(buff[2]), 2);
    buff[4] = (UniChar)'-';
    RSStringAppendCharacters(str, buff, 5);
    
    // Fourth segment (2 bytes, 4 digits + 1 dash)
    _intToHexChars(uuid->_bytes.byte[8], buff, 2);
    _intToHexChars(uuid->_bytes.byte[9], &(buff[2]), 2);
    buff[4] = (UniChar)'-';
    RSStringAppendCharacters(str, buff, 5);
    
    // Fifth segment (6 bytes, 12 digits)
    _intToHexChars(uuid->_bytes.byte[10], buff, 2);
    _intToHexChars(uuid->_bytes.byte[11], &(buff[2]), 2);
    _intToHexChars(uuid->_bytes.byte[12], &(buff[4]), 2);
    _intToHexChars(uuid->_bytes.byte[13], &(buff[6]), 2);
    _intToHexChars(uuid->_bytes.byte[14], &(buff[8]), 2);
    _intToHexChars(uuid->_bytes.byte[15], &(buff[10]), 2);
    RSStringAppendCharacters(str, buff, 12);
    
    return str;
}

RSExport RSUUIDBytes RSUUIDGetBytes(RSUUIDRef uuid)
{
    return uuid->_bytes;
}

RSExport RSComparisonResult RSUUIDCompare(RSUUIDRef uuid1, RSUUIDRef uuid2)
{
    if ((RSGetTypeID(uuid1) != RSUUIDGetTypeID()) && (RSGetTypeID(uuid2) != RSUUIDGetTypeID())) HALTWithError(RSInvalidArgumentException, "RSTypeRef is not a RSUUIDRef.");
    for (int idx = 0; idx < 16; idx++)
    {
        int result = uuid1->_bytes.byte[idx] - uuid2->_bytes.byte[idx];
        if (result > 0) return RSCompareGreaterThan;
        if (result < 0) return RSCompareLessThan;
        continue;
    }
    return RSCompareEqualTo;
}