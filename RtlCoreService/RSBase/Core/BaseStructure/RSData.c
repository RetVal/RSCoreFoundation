//
//  RSData.c
//  RSCoreFoundation
//
//  Created by RetVal on 10/21/12.
//  Copyright (c) 2012 RetVal. All rights reserved.
//

#include <stdio.h>
#include <RSCoreFoundation/RSBase.h>
#include <RSCoreFoundation/RSBaseMacro.h>
#include <RSCoreFoundation/RSNil.h>
#include <RSCoreFoundation/RSData.h>
#include <RSCoreFoundation/RSString.h>
#include <RSCoreFoundation/RSRuntime.h>
#include "RSPrivate/CString/RSPrivateBuffer.h"
#include "RSPrivate/CString/RSPrivateStringFormat.h"

typedef struct __RSISAPayload
{
    RSRuntimeBase _base;
    RSAllocatorRef _deallocator;
    ISA _payload;
}*__RSISAPayloadRef;

RSInline BOOL __RSISAPayloadIsCopyed(RSTypeRef isa)
{
    return __RSBitfieldGetValue(RSRuntimeClassBaseFiled(isa), 3, 2);
}

RSInline void __RSISAPayloadSetCopyed(RSTypeRef isa, BOOL copyed)
{
    __RSBitfieldSetValue(RSRuntimeClassBaseFiled(isa), 3, 2, copyed);
}

static void __RSISAPayloadClassDeallocate(RSTypeRef rs)
{
    if (__RSISAPayloadIsCopyed(rs)) return;
    __RSISAPayloadRef pay = (__RSISAPayloadRef)rs;
    if (pay->_payload) RSAllocatorDeallocate(pay->_deallocator ? : RSAllocatorDefault, pay->_payload);
    pay->_payload = nil;
}

static RSStringRef __RSISAPayloadClassDecription(RSTypeRef rs)
{
    return RSStringCreateWithFormat(RSAllocatorSystemDefault, RSSTR("%s-%p"), RSClassNameFromInstance(rs), rs);
}

static RSRuntimeClass __RSISAPayloadClass = {
    _RSRuntimeScannedObject,
    "__RSISAPayload",
    nil,
    nil,
    __RSISAPayloadClassDeallocate,
    nil,
    nil,
    __RSISAPayloadClassDecription,
    nil,
    nil,
};

static RSTypeID __RSISAPayloadTypeID = _RSRuntimeNotATypeID;

RSPrivate void ___RSISAPayloadInitialize()
{
    __RSISAPayloadTypeID = __RSRuntimeRegisterClass(&__RSISAPayloadClass);
    __RSRuntimeSetClassTypeID(&__RSISAPayloadClass, __RSISAPayloadTypeID);
}

RSPrivate RSTypeRef ___RSISAPayloadCreateWithISA(RSAllocatorRef deallocator, ISA isa)
{
    if (!isa) return nil;
    __RSISAPayloadRef _isa = (__RSISAPayloadRef)__RSRuntimeCreateInstance(RSAllocatorSystemDefault, __RSISAPayloadTypeID, sizeof(struct __RSISAPayload) - sizeof(RSRuntimeBase));
    _isa->_payload = isa;
    _isa->_deallocator = deallocator;
    return _isa;
}

RSPrivate RSTypeRef ___RSISAPayloadCreateWithISACopy(RSAllocatorRef allocator, ISA isa, RSIndex size)
{
    if (!isa) return nil;
    __RSISAPayloadRef _isa = (__RSISAPayloadRef)__RSRuntimeCreateInstance(allocator, __RSISAPayloadTypeID, sizeof(struct __RSISAPayload) - sizeof(RSRuntimeBase) + size);
    __RSISAPayloadSetCopyed(_isa, YES);
    _isa->_deallocator = nil;
//    _isa->_payload = &_isa->_payload + sizeof(ISA);
    memcpy(&_isa->_payload, isa, size);
    return _isa;
}

RSPrivate ISA ___RSISAPayloadGetISA(RSTypeRef _isa)
{
    if (!_isa) return nil;
    __RSGenericValidInstance(_isa, __RSISAPayloadTypeID);
    return !__RSISAPayloadIsCopyed(_isa) ? ((__RSISAPayloadRef)_isa)->_payload : &((__RSISAPayloadRef)_isa)->_payload;
}

RSPrivate ISA __RSAutoReleaseISA(RSAllocatorRef deallocator, ISA isa)
{
    RSAutorelease(___RSISAPayloadCreateWithISA(deallocator, isa));
    return isa;
}


struct __RSData
{
    RSRuntimeBase _base;
    RSIndex _length;
    RSIndex _capacity;
    RSAllocatorRef _allocator;        // reserved.
    void* _bytes;
};
enum {
    _RSDataMutable = 1 << 0,_RSDataMutableMask = _RSDataMutable,
    _RSDataStrongMemory = 1 << 1, _RSDataStrongMemoryMask = _RSDataStrongMemory,
};
/*=
 rsinfo1 bits field (data always has strong memory)
 bit 1 : is mutable
 bit 2 : is strong
 bit 3 :
 bit 4 :
 bit 5 :
 bit 6 :
 bit 7 :
 bit 8 :
 
 */

RSInline BOOL   isMutable(RSTypeRef obj) {
    return ((((RSDataRef)obj)->_base._rsinfo._rsinfo1 & _RSDataMutable) == _RSDataMutableMask);
}
RSInline void   markMutable(RSDataRef obj) {
    ((RSMutableDataRef)obj)->_base._rsinfo._rsinfo1 |= _RSDataMutable;
}

RSInline void   unMarkMutable(RSDataRef obj) {
    ((RSMutableDataRef)obj)->_base._rsinfo._rsinfo1 &= ~_RSDataMutable;
}

RSInline BOOL   isStrong(RSTypeRef obj) {
    return ((((RSDataRef)obj)->_base._rsinfo._rsinfo1 & _RSDataStrongMemory) == _RSDataStrongMemoryMask);
}
RSInline void   markStrong(RSDataRef obj) {
    ((RSMutableDataRef)obj)->_base._rsinfo._rsinfo1 |= _RSDataStrongMemory;
}

RSInline void   unMarkStrong(RSDataRef obj) {
    ((RSMutableDataRef)obj)->_base._rsinfo._rsinfo1 &= ~_RSDataStrongMemory;
}


RSInline RSIndex __RSDataLength(RSDataRef obj) {
    return obj->_length;
}

RSInline RSIndex __RSDataCapacity(RSDataRef obj) {
    return obj->_capacity;
}

RSInline void __RSDataSetLength(RSMutableDataRef data, RSIndex length) {
    data->_length = length;
}

RSInline void __RSDataSetCapacity(RSMutableDataRef data, RSIndex capacity) {
    data->_capacity = capacity;
}
RSInline void* __RSDataGetBytesPtr(RSDataRef data) {
    return data->_bytes;
}
RSInline void __RSDataSetBytesPtr(RSMutableDataRef data, const void* bytes) {
    data->_bytes = (void*)bytes;
}

RSInline void* __RSDataAvailable(RSDataRef data)
{
    if (data == nil) return nil;
    __RSGenericValidInstance(data, RSDataGetTypeID());
    if (data == nil) return nil;
    return (void*)0xfff00000;
}

RSInline BOOL __RSDataAvailableRange(RSMutableDataRef data, RSRange range)
{
    if (data->_length < range.location + range.length) {
        return NO;
    }
    return YES;
}

static void __RSDataSetStrongBytesPtr(RSMutableDataRef data, const void* bytes, RSIndex length, RSIndex capacity)
{
    if (data) {
        data->_bytes = (void*)bytes;
        __RSDataSetLength(data, length);
        __RSDataSetCapacity(data, capacity);
    }
}

static BOOL __RSDataTestDataWithLength(void* bytes, RSIndex length)
{
    RSBitU8* _b = (RSBitU8*)bytes;
    length = length / sizeof(void);
    RSBitU8 b = ((RSBitU8*)bytes)[length - 1];
    _b[length - 1] = 1;
    _b[length - 1] = b;
    return YES;
}


static RSMutableDataRef __RSDataCreateInstance(RSAllocatorRef allocator, const void* bytes, RSIndex length, BOOL strong, BOOL mutable)
{
    //__RSDataTestDataWithLength(bytes, length);
    RSMutableDataRef data = (RSMutableDataRef)__RSRuntimeCreateInstance(allocator, RSDataGetTypeID(), sizeof(struct __RSData) - sizeof(struct __RSRuntimeBase));
    __RSDataSetCapacity(data, length);
    if (bytes)
    {
        if (strong)
        {
            data->_allocator = RSAllocatorSystemDefault;
            data->_bytes = RSAllocatorAllocate(RSAllocatorSystemDefault, length);
            __RSDataSetCapacity(data, RSAllocatorSize(RSAllocatorSystemDefault, length));
            memcpy(data->_bytes, bytes, length);
            __RSDataSetLength(data, length);
        }
        else
        {
            __RSDataSetCapacity(data, length);
            __RSDataSetLength(data, length);
            __RSDataSetBytesPtr(data, bytes);
            unMarkStrong(data);
        }
    }
    else if (length)
    {
        data->_allocator = RSAllocatorSystemDefault;
        data->_bytes = RSAllocatorAllocate(RSAllocatorSystemDefault, length);
        __RSDataSetLength(data, 0);
        //__RSDataSetStrongBytesPtr(data, __RSDataCreateBytesBlock(RSAllocatorSystemDefault, nil, length), 0, length);
    }
    if (mutable) markMutable(data);
    return data;
}

static void __RSDataClassInit(RSTypeRef data)
{
    if (data == nil) HALTWithError(RSInvalidArgumentException, "data is nil");
    //__RSRuntimeSetInstanceTypeID(_data, RSDataGetTypeID());
    markStrong(data);
    //__RSCLog(RSLogLevelDebug, "RSDataClassInit");RSMmLogUnitNumber();
}

static RSTypeRef __RSDataClassCopy(RSAllocatorRef allocator, RSTypeRef rs, BOOL mutableCopy)
{
    if (mutableCopy)
    {
        RSMutableDataRef copy = RSDataCreateMutableCopy(allocator, rs);
        return copy;
    }
    else
    {
        if (!isMutable(rs)) return RSRetain(rs);
        RSDataRef copy = RSDataCreateCopy(allocator, rs);
        return copy;
    }
    return nil;
}

static void __RSDataClassDeallocate(RSTypeRef obj)
{
    if (obj == nil) HALTWithError(RSInvalidArgumentException, "data is nil");
    if (obj == RSNil) return; // = = maybe need not this line, if the obj is RSNil, the runtime has error.
    //__RSCLog(RSLogLevelDebug, "obj(%p) call %s\n",obj,__PRETTY_FUNCTION__);
    RSMutableDataRef _data = (RSMutableDataRef)obj;
    RSAllocatorRef allocator = _data->_allocator ?: RSAllocatorSystemDefault;
    if (isStrong(_data)) {
        RSAllocatorDeallocate(allocator, _data->_bytes);
    }else {
        _data->_bytes = nil;
    }
}

static BOOL __RSDataClassEqual(RSTypeRef data1, RSTypeRef data2)
{
    if (data1 == nil || data2 == nil) HALTWithError(RSInvalidArgumentException, "data1 or data2 is nil");
    if (data1 == data2) return YES;
    RSIndex length = __RSDataLength(data1);
    if (length != __RSDataLength(data2)) return NO;
    return 0 == __builtin_memcmp(__RSDataGetBytesPtr(data1), __RSDataGetBytesPtr(data2), length);
}

static  RSStringRef __RSDataClassDecription(RSTypeRef obj)
{
    RSDataRef data = (RSDataRef)obj;
    RSIndex length = __RSDataLength(data);
    RSBitU8* bytes = __RSDataGetBytesPtr(data);
    RSMutableStringRef desc = RSStringCreateMutable(RSAllocatorSystemDefault, 0);
    RSStringAppendStringWithFormat(desc, RSSTR("<RSData %p [%p]>{length = %u, capacity = %u, bytes = "), data, RSGetAllocator(data), length, __RSDataCapacity(data));

    for (RSIndex idx = 0; idx < length; ++idx)
        RSStringAppendStringWithFormat(desc, RSSTR("\\x%02x"),bytes[idx]);
    RSStringAppendCString(desc, "}", RSStringEncodingASCII);
    return desc;
}

static RSRuntimeClass __RSDataClass = {
    _RSRuntimeScannedObject,
    "RSData",
    __RSDataClassInit,
    __RSDataClassCopy,
    __RSDataClassDeallocate,
    __RSDataClassEqual,
    nil,
    __RSDataClassDecription,
    nil,
    nil,
};

static RSTypeID _RSDataTypeID = _RSRuntimeNotATypeID;
RSPrivate void __RSDataInitialize()
{
    _RSDataTypeID = __RSRuntimeRegisterClass(&__RSDataClass);
    __RSRuntimeSetClassTypeID(&__RSDataClass, _RSDataTypeID);
    
}
RSExport RSTypeID RSDataGetTypeID()
{
    return _RSDataTypeID;
}

RSHashCode RSDataHash(RSDataRef data)
{
    return 0;
}

RSExport RSAllocatorRef RSDataGetAllocator(RSDataRef data)
{
    if(__RSDataAvailable(data))
    {
        return data->_allocator ? : RSAllocatorSystemDefault;
    }
    return RSAllocatorSystemDefault;
}

RSExport const void* RSDataCopyBytes(RSDataRef data)
{
    if(__RSDataAvailable(data))
    {
        void* ptr = RSAllocatorAllocate(RSDataGetAllocator(data), __RSDataLength(data));
        memcpy(ptr, __RSDataGetBytesPtr(data), __RSDataLength(data));
        return (ptr);
    }
    return nil;
}

RSExport const void* RSDataGetBytesPtr(RSDataRef data)
{
    if (__RSDataAvailable(data)) {
        return __RSDataGetBytesPtr(data);
    }
    return nil;
}

RSExport __autorelease void* RSDataMutableBytes(RSDataRef data)
{
    if(__RSDataAvailable(data))
    {
        void* ptr = RSAllocatorAllocate(RSDataGetAllocator(data), __RSDataLength(data));
        memcpy(ptr, __RSDataGetBytesPtr(data), __RSDataLength(data));
        return __RSAutoReleaseISA(RSDataGetAllocator(data), ptr);
    }
    return nil;
}

RSExport void* RSDataGetMutableBytesPtr(RSDataRef data)
{
    if (__RSDataAvailable(data)) {
        return __RSDataGetBytesPtr(data);
    }
    return nil;
}

static void __RSDataResizeStore(RSMutableDataRef data, RSIndex newSize);

RSExport void RSDataSetLength(RSMutableDataRef data, RSIndex extraLength)
{
	if (isMutable(data) == NO) return;
    if (extraLength < 0) __HALT(); // Avoid integer overflow.
    if (__RSDataCapacity(data) < extraLength)
    {
        __RSDataResizeStore(data, extraLength);
    }
}

RSExport void RSDataIncreaseLength(RSMutableDataRef data, RSIndex extraLength)
{
    if (isMutable(data) == NO) return;
    if (extraLength < 0) __HALT(); // Avoid integer overflow.
    if (__RSDataCapacity(data) < __RSDataLength(data) + extraLength)
    {
        __RSDataResizeStore(data, __RSDataLength(data) + extraLength);
    }
}

RSExport RSIndex RSDataGetLength(RSDataRef data)
{
    if (__RSDataAvailable(data)) {
        return __RSDataLength(data);
    }
    return 0;
}

RSExport RSIndex RSDataGetCapacity(RSDataRef data) {
    return __RSDataAvailable(data) ? __RSDataCapacity(data) : 0;
}

RSExport RSDataRef RSDataCreate(RSAllocatorRef allocator, const void* bytes, RSIndex length)
{
    if (bytes == nil || length == 0) return nil;
    RSMutableDataRef data = __RSDataCreateInstance(allocator, bytes, length, YES, NO);
    return data;
}

RSExport RSDataRef RSDataCreateCopy(RSAllocatorRef allocator, RSDataRef data)
{
    if (allocator && __RSDataAvailable(data))
    {
        RSDataRef new = RSDataCreate(allocator, __RSDataGetBytesPtr(data), __RSDataLength(data));
        return new;
    }
    return nil;
}

RSExport RSDataRef RSDataCreateWithNoCopy(RSAllocatorRef allocator, const void* bytes, RSIndex length, BOOL freeWhenDone, RSAllocatorRef bytesDeallocator)
{
    if (bytes == nil || length == 0) return nil;
    //freeWhenDone = YES, the data have the strong pointer to the bytes.
    RSMutableDataRef  data = __RSDataCreateInstance(allocator, bytes, length, NO, NO);
    if (freeWhenDone)
        markStrong(data);
    if (bytesDeallocator)
    {
        data->_allocator = bytesDeallocator;
    }
    return data;
}

RSExport RSMutableDataRef RSDataCreateMutable(RSAllocatorRef allocator, RSIndex capacity)
{
    if (capacity == 0) capacity = 1;
    RSMutableDataRef  data = __RSDataCreateInstance(allocator, nil, capacity, YES, YES);
    return data;
}

RSExport RSMutableDataRef RSDataCreateMutableCopy(RSAllocatorRef allocator, RSDataRef data)
{
    if (data == nil) return nil;
    RSMutableDataRef  mutable = __RSDataCreateInstance(allocator, __RSDataGetBytesPtr(data), __RSDataLength(data), YES, YES);
    return mutable;
}

RSExport RSMutableDataRef RSDataCreateMutableWithNoCopy(RSAllocatorRef allocator, const void* bytes, RSIndex length, BOOL freeWhenDone)
{
    if (bytes == nil || length == 0) return nil;
    RSMutableDataRef  data = __RSDataCreateInstance(allocator, bytes, length, NO, YES);
    if (freeWhenDone) {
        markStrong(data);
    }
    return data;
}

RSExport void RSDataLog(RSDataRef data)
{
    if (__RSDataAvailable(data)) {
        __RSCLog(RSLogLevelDebug, "data(%p) is available\n",data);
        const void* bytes = RSDataGetBytesPtr(data);
        if (bytes) __RSCLog(RSLogLevelDebug, "bytes : %s\n",bytes);
        else __RSCLog(RSLogLevelDebug, "bytes : nil\n");
        __RSCLog(RSLogLevelDebug, "length   : %lld\n",__RSDataLength(data));
        __RSCLog(RSLogLevelDebug, "capacity : %lld\n",__RSDataCapacity(data));
        if (isStrong(data)) __RSCLog(RSLogLevelDebug, "strong memory.\n");
        else __RSCLog(RSLogLevelDebug, "weak memory inline.\n");
        if (isMutable(data)) __RSCLog(RSLogLevelDebug, "mutable data\n");
        else __RSCLog(RSLogLevelDebug, "constant data\n");
    }
}

#if __LP64__
#define RSDATA_MAX_SIZE	    ((1ULL << 42) - 1)
#else
#define RSDATA_MAX_SIZE	    ((1ULL << 31) - 1)
#endif

#if __LP64__
#define CHUNK_SIZE (1ULL << 29)
#define LOW_THRESHOLD (1ULL << 20)
#define HIGH_THRESHOLD (1ULL << 32)
#else
#define CHUNK_SIZE (1ULL << 26)
#define LOW_THRESHOLD (1ULL << 20)
#define HIGH_THRESHOLD (1ULL << 29)
#endif

RSInline RSIndex __RSDataRoundUpCapacity(RSIndex capacity) {
    if (capacity < 16) {
        return 16;
    } else if (capacity < LOW_THRESHOLD) {
        /* Up to 4x */
        long idx = flsl(capacity);
        return (1L << (long)(idx + ((idx % 2 == 0) ? 0 : 1)));
    } else if (capacity < HIGH_THRESHOLD) {
        /* Up to 2x */
        return (1L << (long)flsl(capacity));
    } else {
        /* Round up to next multiple of CHUNK_SIZE */
        unsigned long newCapacity = CHUNK_SIZE * (1+(capacity >> ((long)flsl(CHUNK_SIZE)-1)));
        return min(newCapacity, RSDATA_MAX_SIZE);
    }
}

RSInline void __RSDataResizeStore(RSMutableDataRef data, RSIndex newSize)
{
    RSIndex capacity = __RSDataCapacity(data);
    RSAllocatorRef allocator = RSAllocatorSystemDefault;
    if (capacity < newSize)
    {
        capacity = RSAllocatorSize(allocator, __RSDataRoundUpCapacity(newSize));
        void* store = __RSDataGetBytesPtr(data);
        void* newStore = store;
        if (newStore) {
            newStore = RSAllocatorReallocate(allocator, newStore, capacity);
        }else {
            newStore = RSAllocatorAllocate(allocator, capacity);
        }
        __RSDataSetStrongBytesPtr(data, newStore, __RSDataLength(data), capacity);
//        __RSDataCreateAndSetStrongBytesPtr(data, allocator, newStore, __RSDataLength(data), capacity);
    }
    else
    {
        // capacity > newSize
        
    }
    return;
}

static BOOL __RSDataReplaceBytes(RSMutableDataRef data, RSRange range, const void* replacement, RSIndex replaceLength)
{
    if (data == nil) return NO;
    
    RSIndex capcity = __RSDataCapacity(data);
    RSIndex length = __RSDataLength(data);
    if (replacement == nil)
    {
        if (range.location + range.length > length)
            return NO;
    }
    RSIndex overWriteLength = 0;
    void* __mutableBuffer = __RSDataGetBytesPtr(data);
    if (__mutableBuffer == nil) HALTWithError(RSInvalidArgumentException, "__mutableBuffer is nil");
    if (length < range.location + range.length)
    {
        if (range.location == 0 && length == 0)
        {
            //__RSCLog(RSLogLevelNotice, "%s", "__RSStringRepalceString : the range is out of the mutable string.\n");
            overWriteLength = range.location + range.length - length;
        }
        else overWriteLength = range.location + range.length - length;
    }
    
    RSIndex newLength = length - (range.length) + replaceLength;   // 1 is the size of the end flag.
    newLength = newLength ? newLength + overWriteLength: overWriteLength;
    if (newLength > capcity) {
        __RSDataResizeStore(data, newLength + 1);
        capcity = __RSDataCapacity(data);
    }
    BOOL moveOrCopy = YES;   //YES = MOVE, NO = COPY
    if (replacement < __mutableBuffer ||
        replacement > __mutableBuffer + capcity)
        moveOrCopy = NO;
    // update the mutable string buffer, and it has the enough capacity to contain the content
    
    if (YES == moveOrCopy)
    {
        if (range.length == replaceLength)
        {
            if (replacement) memmove(__mutableBuffer + range.location, replacement, replaceLength);
            else memset(__mutableBuffer + range.location, 0, replaceLength);
        }
        else if (range.length < replaceLength)
        {
            RSIndex offset = replaceLength - range.length;
            memmove( __mutableBuffer + range.location + range.length + offset, __mutableBuffer + range.location + range.length, length - range.location - range.length);
            if (replacement) memmove(__mutableBuffer + range.location, replacement, replaceLength);
            else memset(__mutableBuffer + range.location, 0, replaceLength);
        }
        else //if (range.length > replaceLength)
        {
            //RSIndex offset = range.length - replaceLength;
            memset(__mutableBuffer + range.location, 0, range.length);
            if (replacement) memcpy(__mutableBuffer + range.location, replacement, replaceLength);
            else memset(__mutableBuffer + range.location, 0, replaceLength);
            strncat((char*)__mutableBuffer + range.location + replaceLength,
                    (char*)__mutableBuffer + range.location + range.length,
                    newLength - replaceLength);
        }
    }
    else
    {
        if (range.length == replaceLength)
        {
            if (replacement) memcpy(__mutableBuffer + range.location, replacement, replaceLength);
            else memset(__mutableBuffer + range.location, 0, replaceLength);
        }
        else if (range.length < replaceLength)
        {
            RSIndex offset = replaceLength - range.length;
            memmove(__mutableBuffer + range.location + range.length + offset,
                    __mutableBuffer + range.location + range.length,
                    length - range.location - range.length);
            
            if (replacement) memmove(__mutableBuffer + range.location,
                                     replacement,
                                     replaceLength);
            else memset(__mutableBuffer + range.location, 0, replaceLength);
            
        }
        else //if (range.length > replaceLength)
        {
            //RSIndex offset = range.length - replaceLength;
            memset(__mutableBuffer + range.location, 0, range.length);
            if (replacement) memcpy(__mutableBuffer + range.location, replacement, replaceLength);
            else memset(__mutableBuffer + range.location, 0, replaceLength);
            strncat((char*)__mutableBuffer + range.location + replaceLength,
                    (char*)__mutableBuffer + range.location + range.length,
                    newLength - replaceLength);
        }
    }
    
    if (replacement) __RSDataSetLength(data, length - range.length + replaceLength + overWriteLength);
    else __RSDataSetLength(data, length - range.length + overWriteLength);
    return NO;
}

static void __RSDataAppendBytes(RSMutableDataRef data, const void* appendBytes, RSIndex appendLength)
{
    if (appendBytes == nil || appendLength == 0) return;
    RSIndex capacity = __RSDataCapacity(data);
    RSIndex length = __RSDataLength(data);
    RSIndex overLength = 0;
    if (capacity == (length + appendLength)) overLength = 1;
    if (capacity < (length + appendLength + overLength))
    {
        __RSDataResizeStore(data, length + appendLength + overLength);
    }
    memcpy(data->_bytes + length, appendBytes, appendLength);
    __RSDataSetLength(data, length + appendLength);
}

RSExport void RSDataAppendBytes(RSMutableDataRef data, const void* appendBytes, RSIndex length)
{
    if (data && __RSDataAvailable(data) && appendBytes && length && isMutable(data))
    {
        __RSDataAppendBytes(data, appendBytes, length);
    }
    else if (data && length)
    {
        if (isMutable(data)) __RSLog(RSLogLevelWarning, RSSTR("RSDataAppendBytes.ismutable\n"));
        if (appendBytes) __RSLog(RSLogLevelWarning, RSSTR("RSDataAppendBytes.appendBytes\n"));
    }
    else
    {
        if (length == 0) __RSLog(RSLogLevelWarning, RSSTR("RSDataAppendBytes.length = 0\n"));
    }
    return;
}

RSExport void RSDataReplaceBytes(RSMutableDataRef data, RSRange rangeToReplace, const void* replaceBytes, RSIndex length)
{
    if (data && __RSDataAvailable(data) && replaceBytes && length && isMutable(data) && __RSDataAvailableRange(data, rangeToReplace))
    {
        __RSDataReplaceBytes(data, rangeToReplace, replaceBytes, length);
    }
    return;
}

RSExport void RSDataDeleteBytes(RSMutableDataRef data, RSRange rangeToDelete,  RSIndex length)
{
    if (data && __RSDataAvailable(data) && isMutable(data) && length && __RSDataAvailableRange(data, rangeToDelete))
    {
        __RSDataReplaceBytes(data, rangeToDelete, nil, length);
    }
}

RSExport void RSDataAppend(RSMutableDataRef data, RSDataRef appendData)
{
    if (data && __RSDataAvailable(data) && isMutable(data) && appendData && __RSDataAvailable(appendData))
    {
        __RSDataAppendBytes(data, __RSDataGetBytesPtr(appendData), __RSDataLength(appendData));
    }
}
RSExport void RSDataReplace(RSMutableDataRef data, RSRange rangeToReplace, RSDataRef replaceData)
{
    if (data && __RSDataAvailable(data) && isMutable(data) && __RSDataAvailable(replaceData) && __RSDataAvailableRange(data, rangeToReplace) && __RSDataLength(replaceData))
    {
        __RSDataReplaceBytes(data, rangeToReplace, __RSDataGetBytesPtr(replaceData), __RSDataLength(replaceData));
    }
}
RSExport void RSDataDelete(RSMutableDataRef data, RSRange rangeToDelete)
{
    if (data && __RSDataAvailable(data) && isMutable(data) && __RSDataAvailableRange(data, rangeToDelete))
    {
        __RSDataReplaceBytes(data, rangeToDelete, nil, rangeToDelete.length);
    }
}

RSExport RSComparisonResult RSDataCompare(RSDataRef data1, RSDataRef data2)
{
    if (data1 && data2 && __RSDataAvailable(data1) && __RSDataAvailable(data2))
    {
        if (data1 == data2) return RSCompareEqualTo;
        RSIndex minsize = min(__RSDataLength(data1), __RSDataLength(data2));
        RSBitU8* s1 = __RSDataGetBytesPtr(data1);
        RSBitU8* s2 = __RSDataGetBytesPtr(data2);
        for (RSIndex idx = 0; idx < minsize; idx++)
        {
            if (s1[idx] > s2[idx])  return RSCompareGreaterThan;
            if (s2[idx] < s2[idx])  return RSCompareLessThan;
            continue;
        }
        return RSCompareEqualTo;
    }
    HALTWithError(RSInvalidArgumentException, "The data1 or data2 is not available.");
    return RSCompareEqualTo;
}
