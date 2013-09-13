//
//  RSBitVector.c
//  RSCoreFoundation
//
//  Created by RetVal on 6/30/13.
//  Copyright (c) 2013 RetVal. All rights reserved.
//

#include "RSBitVector.h"

#include <RSCoreFoundation/RSRuntime.h>
typedef RSByte __RSBitVectorBucket;

enum {
    __RS_BITS_PER_BYTE_MASK = 7,
    __RS_BITS_PER_BYTE = 8,
    __RS_BITS_PER_BUCKET_MASK = 7,
    __RS_BITS_PER_BUCKET = (__RS_BITS_PER_BYTE * sizeof(__RSBitVectorBucket)),
};

RSInline RSIndex __RSBitVectorRoundUpCapacity(RSIndex capacity) {
    if (0 == capacity) capacity = 1;
    return ((capacity + 63) / 64) * 64;
}

struct __RSBitVector
{
    RSRuntimeBase _base;
    RSIndex _count;
    RSIndex _capacity;
    __RSBitVectorBucket *_buckets;
};

RSInline RSBitU32 __RSBitVectorMutableVariety(const void *rs) {
    return __RSBitfieldGetValue(RSRuntimeClassBaseFiled(rs), 3, 2);
}

RSInline void __RSBitVectorSetMutableVariety(void *rs, RSBitU32 v) {
    __RSBitfieldSetValue(RSRuntimeClassBaseFiled(rs), 3, 2, v);
}

RSInline RSIndex __RSBitVectorCount(RSBitVectorRef bv) {
    return bv->_count;
}

RSInline void __RSBitVectorSetCount(RSMutableBitVectorRef bv, RSIndex v) {
    bv->_count = v;
}

RSInline RSIndex __RSBitVectorCapacity(RSBitVectorRef bv) {
    return bv->_capacity;
}

RSInline void __RSBitVectorSetCapacity(RSMutableBitVectorRef bv, RSIndex v) {
    bv->_capacity = v;
}

RSInline RSIndex __RSBitVectorNumBucketsUsed(RSBitVectorRef bv) {
    return bv->_count / __RS_BITS_PER_BUCKET + 1;
}

RSInline void __RSBitVectorSetNumBucketsUsed(RSMutableBitVectorRef bv, RSIndex v) {
    /* for a RSBitVector, _bucketsUsed == _count / __RS_BITS_PER_BUCKET + 1 */
}

RSInline RSIndex __RSBitVectorNumBuckets(RSBitVectorRef bv) {
    return bv->_capacity / __RS_BITS_PER_BUCKET + 1;
}

RSInline __RSBitVectorBucket *__RSBitVectorBuckets(RSBitVectorRef bv) {
    return bv->_buckets;
}

RSInline void __RSBitVectorSetBuckets(RSMutableBitVectorRef bv, __RSBitVectorBucket *buckets) {
    bv->_buckets = buckets;
}


RSInline RSBit __RSBitVectorBit(__RSBitVectorBucket *buckets, RSIndex idx) {
    RSIndex bucketIdx = idx / __RS_BITS_PER_BUCKET;
    RSIndex bitOfBucket = idx & (__RS_BITS_PER_BUCKET - 1);
    return (buckets[bucketIdx] >> (__RS_BITS_PER_BUCKET - 1 - bitOfBucket)) & 0x1;
}

RSInline void __RSSetBitVectorBit(__RSBitVectorBucket *buckets, RSIndex idx, RSBit value) {
    RSIndex bucketIdx = idx / __RS_BITS_PER_BUCKET;
    RSIndex bitOfBucket = idx & (__RS_BITS_PER_BUCKET - 1);
    if (value) {
        buckets[bucketIdx] |= (1 << (__RS_BITS_PER_BUCKET - 1 - bitOfBucket));
    } else {
        buckets[bucketIdx] &= ~(1 << (__RS_BITS_PER_BUCKET - 1 - bitOfBucket));
    }
}

RSInline void __RSFlipBitVectorBit(__RSBitVectorBucket *buckets, RSIndex idx) {
    RSIndex bucketIdx = idx / __RS_BITS_PER_BUCKET;
    RSIndex bitOfBucket = idx & (__RS_BITS_PER_BUCKET - 1);
    buckets[bucketIdx] ^= (1 << (__RS_BITS_PER_BUCKET - 1 - bitOfBucket));
}


RSInline RSMutableBitVectorRef __RSBitVectorIncrementCapacity(RSMutableBitVectorRef bv, RSIndex incrementSize)
{
    if (!bv || !incrementSize) return nil;
    if (nil == bv->_buckets) bv->_buckets = RSAllocatorAllocate(RSAllocatorSystemDefault, incrementSize = __RSBitVectorRoundUpCapacity(incrementSize));
    else bv->_buckets = RSAllocatorReallocate(RSAllocatorSystemDefault, bv->_buckets, incrementSize = __RSBitVectorRoundUpCapacity(incrementSize + __RSBitVectorCapacity(bv)));
    __RSBitVectorSetCapacity(bv, incrementSize);
    return bv;
}

static void __RSBitVectorClassInit(RSTypeRef rs)
{
    
}

static RSTypeRef __RSBitVectorClassCopy(RSAllocatorRef allocator, RSTypeRef rs, BOOL mutableCopy)
{
    if (mutableCopy) return RSBitVectorCreateMutableCopy(allocator, rs);
    return RSBitVectorCreateCopy(allocator, rs);
}

static void __RSBitVectorClassDeallocate(RSTypeRef rs)
{
    RSMutableBitVectorRef bv = (RSMutableBitVectorRef)rs;
    if (bv->_buckets)
    {
        RSAllocatorDeallocate(RSAllocatorSystemDefault, bv->_buckets);
        bv->_buckets = nil;
    }
}

static BOOL __RSBitVectorClassEqual(RSTypeRef rs1, RSTypeRef rs2)
{
    RSBitVectorRef RSBitVector1 = (RSBitVectorRef)rs1;
    RSBitVectorRef RSBitVector2 = (RSBitVectorRef)rs2;
    BOOL result = NO;
    if (RSBitVector1->_count != RSBitVector2->_count) return result;
    result = __builtin_memcmp(RSBitVector1->_buckets, RSBitVector2->_buckets, RSBitVector1->_count) == 0 ? YES : NO;
    
    return result;
}

static RSStringRef __RSBitVectorClassDescription(RSTypeRef rs)
{
    RSStringRef description = RSStringCreateWithFormat(RSAllocatorSystemDefault, RSSTR("RSBitVector %p"), rs);
    return description;
}

static RSRuntimeClass __RSBitVectorClass =
{
    _RSRuntimeScannedObject,
    "RSBitVector",
    __RSBitVectorClassInit,
    __RSBitVectorClassCopy,
    __RSBitVectorClassDeallocate,
    __RSBitVectorClassEqual,
    nil,
    __RSBitVectorClassDescription,
    nil,
    nil
};

static RSTypeID _RSBitVectorTypeID = _RSRuntimeNotATypeID;

RSExport RSTypeID RSBitVectorGetTypeID()
{
    return _RSBitVectorTypeID;
}

RSPrivate void __RSBitVectorInitialize()
{
    _RSBitVectorTypeID = __RSRuntimeRegisterClass(&__RSBitVectorClass);
    __RSRuntimeSetClassTypeID(&__RSBitVectorClass, _RSBitVectorTypeID);
}

RSPrivate void __RSBitVectorDeallocate()
{
    
}

static RSMutableBitVectorRef __RSBitVectorCreateInstance(RSAllocatorRef allocator, const RSByte *bytes, RSIndex countOfBytes, BOOL mutable)
{
    if (!mutable)
    {
        if (bytes == nil || countOfBytes <= 0) return nil;
    }
    else
    {
        if (bytes == nil && countOfBytes == 0)
            countOfBytes = __RSBitVectorRoundUpCapacity(countOfBytes);
    }
    RSMutableBitVectorRef instance = (RSMutableBitVectorRef)__RSRuntimeCreateInstance(allocator, _RSBitVectorTypeID, sizeof(struct __RSBitVector) - sizeof(RSRuntimeBase));
    __RSBitVectorIncrementCapacity(instance, countOfBytes);
    if (!instance->_buckets)
    {
        RSRelease(instance);
        return nil;
    }
    __RSBitVectorSetNumBucketsUsed(instance, countOfBytes / __RS_BITS_PER_BUCKET + 1);
    __RSBitVectorSetCount(instance, countOfBytes);
    if (bytes)  __builtin_memcmp(instance->_buckets, bytes, countOfBytes);
    if (mutable)__RSBitVectorSetMutableVariety(instance, YES);
    return instance;
}


RSExport RSBitVectorRef RSBitVectorCreate(RSAllocatorRef allocator, RSByte *bytes, RSIndex countOfBytes)
{
    if (!bytes || 0 >= countOfBytes) return nil;
    return __RSBitVectorCreateInstance(allocator, bytes, countOfBytes, NO);
}

RSExport RSMutableBitVectorRef RSBitVectorCreateMutable(RSAllocatorRef allocator, RSIndex capacity)
{
    return __RSBitVectorCreateInstance(allocator, nil, capacity, YES);
}

RSExport RSBitVectorRef RSBitVectorCreateCopy(RSAllocatorRef allocator, RSBitVectorRef bv)
{
    __RSGenericValidInstance(bv, _RSBitVectorTypeID);
    return __RSBitVectorCreateInstance(allocator, (const RSByte *)bv->_buckets, __RSBitVectorCount(bv), NO);
}

RSExport RSMutableBitVectorRef RSBitVectorCreateMutableCopy(RSAllocatorRef allocator, RSBitVectorRef bv)
{
    __RSGenericValidInstance(bv, _RSBitVectorTypeID);
    return __RSBitVectorCreateInstance(allocator, (const RSByte *)bv->_buckets, __RSBitVectorCount(bv), YES);
}

RSExport RSBit RSBitVectorBitAtIndex(RSBitVectorRef bv, RSIndex byteIndex)
{
    __RSGenericValidInstance(bv, _RSBitVectorTypeID);
    return __RSBitVectorBit(__RSBitVectorBuckets(bv), byteIndex);
}

RSExport RSIndex RSBitVectorGetCount(RSBitVectorRef bv)
{
    __RSGenericValidInstance(bv, _RSBitVectorTypeID);
    return __RSBitVectorCount(bv);
}

struct _occursContext {
    RSBit value;
    RSIndex count;
};

static __RSBitVectorBucket __RSBitVectorCountBits(__RSBitVectorBucket bucketValue, __RSBitVectorBucket bucketValueMask, struct _occursContext *context)
{
    static const __RSBitVectorBucket __RSNibbleBitCount[16] = {0, 1, 1, 2, 1, 2, 2, 3, 1, 2, 2, 3, 2, 3, 3, 4};
    __RSBitVectorBucket val;
    val = (context->value) ? (bucketValue & bucketValueMask) : (~bucketValue & bucketValueMask);
    for (RSIndex idx = 0; idx < (RSIndex)sizeof(__RSBitVectorBucket) * 2; idx++)
    {
        context->count += __RSNibbleBitCount[val & 0xF];
        val = val >> 4;
    }
    return bucketValue;
}

static __RSBitVectorBucket __RSBitBucketMask(RSIndex bottomBit, RSIndex topBit) {
    RSIndex shiftL = __RS_BITS_PER_BUCKET - topBit + bottomBit - 1;
    __RSBitVectorBucket result = ~(__RSBitVectorBucket)0;
    result = (result << shiftL);
    result = (result >> bottomBit);
    return result;
}

typedef __RSBitVectorBucket (*__RSInternalMapper)(__RSBitVectorBucket bucketValue, __RSBitVectorBucket bucketValueMask, void *context);

static void __RSBitVectorInternalMap(RSMutableBitVectorRef bv, RSRange range, __RSInternalMapper mapper, void *context) {
    RSIndex bucketIdx, bitOfBucket;
    RSIndex nBuckets;
    __RSBitVectorBucket bucketValMask, newBucketVal;
    if (0 == range.length) return;
    bucketIdx = range.location / __RS_BITS_PER_BUCKET;
    bitOfBucket = range.location & (__RS_BITS_PER_BUCKET - 1);
    /* Follow usual pattern of ramping up to a bit bucket boundary ...*/
    if (bitOfBucket + range.length < __RS_BITS_PER_BUCKET) {
        bucketValMask = __RSBitBucketMask(bitOfBucket, bitOfBucket + range.length - 1);
        range.length = 0;
    } else {
        bucketValMask = __RSBitBucketMask(bitOfBucket, __RS_BITS_PER_BUCKET - 1);
        range.length -= __RS_BITS_PER_BUCKET - bitOfBucket;
    }
    newBucketVal = mapper(bv->_buckets[bucketIdx], bucketValMask, context);
    bv->_buckets[bucketIdx] = (bv->_buckets[bucketIdx] & ~bucketValMask) | (newBucketVal & bucketValMask);
    bucketIdx++;
    /* ... clipping along with entire bit buckets ... */
    nBuckets = range.length / __RS_BITS_PER_BUCKET;
    range.length -= nBuckets * __RS_BITS_PER_BUCKET;
    while (nBuckets--) {
        newBucketVal = mapper(bv->_buckets[bucketIdx], ~0, context);
        bv->_buckets[bucketIdx] = newBucketVal;
        bucketIdx++;
    }
    /* ... and ramping down with the last fragmentary bit bucket. */
    if (0 != range.length) {
        bucketValMask = __RSBitBucketMask(0, range.length - 1);
        newBucketVal = mapper(bv->_buckets[bucketIdx], bucketValMask, context);
        bv->_buckets[bucketIdx] = (bv->_buckets[bucketIdx] & ~bucketValMask) | (newBucketVal & bucketValMask);
    }
}

#if defined(DEBUG)
//RSInline void __RSBitVectorValidateRange(RSBitVectorRef bv, RSRange range, const char *func) {
//    RSAssert2(0 <= range.location && range.location < __RSBitVectorCount(bv), __kRSLogAssertion, "%s(): range.location index (%d) out of bounds", func, range.location);
//    RSAssert2(0 <= range.length, __kRSLogAssertion, "%s(): range.length (%d) cannot be less than zero", func, range.length);
//    RSAssert2(range.location + range.length <= __RSBitVectorCount(bv), __kRSLogAssertion, "%s(): ending index (%d) out of bounds", func, range.location + range.length);
//}
#define __RSBitVectorValidateRange(bf,r,f)
#else
#define __RSBitVectorValidateRange(bf,r,f)
#endif

RSExport RSIndex RSBitVectorGetCountOfBit(RSBitVectorRef bv, RSRange range, RSBit value)
{
    __RSGenericValidInstance(bv, _RSBitVectorTypeID);
    struct _occursContext context;
    __RSBitVectorValidateRange(bv, range, __PRETTY_FUNCTION__);
    if (0 == range.length) return 0;
    context.value = value;
    context.count = 0;
    __RSBitVectorInternalMap((RSMutableBitVectorRef)bv, range, (__RSInternalMapper)__RSBitVectorCountBits, &context);
    return context.count;
}

RSExport BOOL RSBitVectorContainsBit(RSBitVectorRef bv, RSRange range, RSBit value)
{
    __RSGenericValidInstance(bv, _RSBitVectorTypeID);
    __RSBitVectorValidateRange(bv, range, __PRETTY_FUNCTION__);
    return (RSBitVectorGetCountOfBit(bv, range, value) != 0) ? YES : NO;
}

RSExport RSBit RSBitVectorGetBitAtIndex(RSBitVectorRef bv, RSIndex idx)
{
    __RSGenericValidInstance(bv, _RSBitVectorTypeID);
//    RSAssert2(0 <= idx && idx < __RSBitVectorCount(bv), __kRSLogAssertion, "%s(): index (%d) out of bounds", __PRETTY_FUNCTION__, idx);
    return __RSBitVectorBit(bv->_buckets, idx);
}

struct _getBitsContext {
    uint8_t *curByte;
    RSIndex initBits;	/* Bits to extract off the front for the prev. byte */
    RSIndex totalBits;	/* This is for stopping at the end */
    bool ignoreFirstInitBits;
};

static __RSBitVectorBucket __RSBitVectorGetBits(__RSBitVectorBucket bucketValue, __RSBitVectorBucket bucketValueMask, void *ctx) {
    struct _getBitsContext *context = (struct _getBitsContext *)ctx;
    __RSBitVectorBucket val;
    RSIndex nBits;
    val = bucketValue & bucketValueMask;
    nBits = __RSMin(__RS_BITS_PER_BUCKET - context->initBits, context->totalBits);
    /* First initBits bits go in *curByte ... */
    if (0 < context->initBits) {
        if (!context->ignoreFirstInitBits) {
            *context->curByte |= (uint8_t)(val >> (__RS_BITS_PER_BUCKET - context->initBits));
            context->curByte++;
            context->totalBits -= context->initBits;
            context->ignoreFirstInitBits = NO;
        }
        val <<= context->initBits;
    }
    /* ... then next groups of __RS_BITS_PER_BYTE go in *curByte ... */
    while (__RS_BITS_PER_BYTE <= nBits) {
        *context->curByte = (uint8_t)(val >> (__RS_BITS_PER_BUCKET - __RS_BITS_PER_BYTE));
        context->curByte++;
        context->totalBits -= context->initBits;
        nBits -= __RS_BITS_PER_BYTE;
#if __RSBITVECTORBUCKET_SIZE > __RS_BITS_PER_BYTE
        val <<= __RS_BITS_PER_BYTE;
#else
        val = 0;
#endif
    }
    /* ... then remaining bits go in *curByte */
    if (0 < nBits) {
        *context->curByte = (uint8_t)(val >> (__RS_BITS_PER_BUCKET - __RS_BITS_PER_BYTE));
        context->totalBits -= nBits;
    }
    return bucketValue;
}


RSExport void RSBitVectorGetBits(RSBitVectorRef bv, RSRange range, RSByte *bytes)
{
    struct _getBitsContext context;
    __RSGenericValidInstance(bv, _RSBitVectorTypeID);
    __RSBitVectorValidateRange(bv, range, __PRETTY_FUNCTION__);
    if (0 == range.length) return;
    context.curByte = bytes;
    context.initBits = range.location & (__RS_BITS_PER_BUCKET - 1);
    context.totalBits = range.length;
    context.ignoreFirstInitBits = YES;
    __RSBitVectorInternalMap((RSMutableBitVectorRef)bv, range, __RSBitVectorGetBits, &context);
}

static void __RSBitVectorGrow(RSMutableBitVectorRef bv, RSIndex numNewValues) {
    RSIndex oldCount = __RSBitVectorCount(bv);
    RSIndex capacity = __RSBitVectorRoundUpCapacity(oldCount + numNewValues);
    RSAllocatorRef allocator = RSGetAllocator(bv);
    __RSBitVectorSetCapacity(bv, capacity);
//    __RSBitVectorSetNumBuckets(bv, __RSBitVectorNumBucketsForCapacity(capacity));
    __RSAssignWithWriteBarrier((void **)&bv->_buckets, RSAllocatorReallocate(allocator, bv->_buckets, __RSBitVectorNumBuckets(bv) * sizeof(__RSBitVectorBucket)));
//    if (__RSOASafe) __RSSetLastAllocationEventName(bv->_buckets, "RSBitVector (store)");
    if (NULL == bv->_buckets) HALTWithError(RSGenericException, "");
}

static __RSBitVectorBucket __RSBitVectorZeroBits(__RSBitVectorBucket bucketValue, __RSBitVectorBucket bucketValueMask, void *context) {
    return 0;
}

static __RSBitVectorBucket __RSBitVectorOneBits(__RSBitVectorBucket bucketValue, __RSBitVectorBucket bucketValueMask, void *context) {
    return ~(__RSBitVectorBucket)0;
}

enum {
    RSBitVectorMutable = YES,
    RSBitVectorImmutable = NO
};

RSExport void RSBitVectorSetCount(RSMutableBitVectorRef bv, RSIndex count)
{
    RSIndex cnt;
//    RSAssert1(__RSBitVectorMutableVariety(bv) == RSBitVectorMutable, __kRSLogAssertion, "%s(): bit vector is immutable", __PRETTY_FUNCTION__);
    cnt = __RSBitVectorCount(bv);
    switch (__RSBitVectorMutableVariety(bv)) {
        case RSBitVectorMutable:
            if (cnt < count) {
                __RSBitVectorGrow(bv, count - cnt);
            }
            break;
    }
    if (cnt < count) {
        RSRange range = RSMakeRange(cnt, count - cnt);
        __RSBitVectorInternalMap(bv, range, __RSBitVectorZeroBits, NULL);
    }
    __RSBitVectorSetNumBucketsUsed(bv, count / __RS_BITS_PER_BUCKET + 1);
    __RSBitVectorSetCount(bv, count);
}

RSExport void RSBitVectorFlipBitAtIndex(RSMutableBitVectorRef bv, RSIndex idx)
{
    __RSGenericValidInstance(bv, _RSBitVectorTypeID);
//    RSAssert2(0 <= idx && idx < __RSBitVectorCount(bv), __kRSLogAssertion, "%s(): index (%d) out of bounds", __PRETTY_FUNCTION__, idx);
//    RSAssert1(__RSBitVectorMutableVariety(bv) == kRSBitVectorMutable, __kRSLogAssertion, "%s(): bit vector is immutable", __PRETTY_FUNCTION__);
    __RSFlipBitVectorBit(bv->_buckets, idx);
}


static __RSBitVectorBucket __RSBitVectorFlipBits(__RSBitVectorBucket bucketValue, __RSBitVectorBucket bucketValueMask, void *context) {
    return (~(__RSBitVectorBucket)0) ^ bucketValue;
}

RSExport void RSBitVectorFlipBits(RSMutableBitVectorRef bv, RSRange range)
{
    __RSGenericValidInstance(bv, _RSBitVectorTypeID);
    __RSBitVectorValidateRange(bv, range, __PRETTY_FUNCTION__);
//    RSAssert1(__RSBitVectorMutableVariety(bv) == RSBitVectorMutable, __kRSLogAssertion, "%s(): bit vector is immutable", __PRETTY_FUNCTION__);
    if (0 == range.length) return;
    __RSBitVectorInternalMap(bv, range, __RSBitVectorFlipBits, NULL);
}

RSExport void RSBitVectorSetBitAtIndex(RSMutableBitVectorRef bv, RSIndex idx, RSBit value)
{
    __RSGenericValidInstance(bv, _RSBitVectorTypeID);
//    RSAssert2(0 <= idx && idx < __RSBitVectorCount(bv), __kRSLogAssertion, "%s(): index (%d) out of bounds", __PRETTY_FUNCTION__, idx);
//    RSAssert1(__RSBitVectorMutableVariety(bv) == RSBitVectorMutable, __kRSLogAssertion, "%s(): bit vector is immutable", __PRETTY_FUNCTION__);
    __RSSetBitVectorBit(bv->_buckets, idx, value);
}

RSExport void RSBitVectorSetBits(RSMutableBitVectorRef bv, RSRange range, RSBit value)
{
    __RSGenericValidInstance(bv, _RSBitVectorTypeID);
    __RSBitVectorValidateRange(bv, range, __PRETTY_FUNCTION__);
//    RSAssert1(__RSBitVectorMutableVariety(bv) == kRSBitVectorMutable , __kRSLogAssertion, "%s(): bit vector is immutable", __PRETTY_FUNCTION__);
    if (0 == range.length) return;
    if (value)
    {
        __RSBitVectorInternalMap(bv, range, __RSBitVectorOneBits, NULL);
    }
    else
    {
        __RSBitVectorInternalMap(bv, range, __RSBitVectorZeroBits, NULL);
    }
}

RSExport void RSBitVectorSetAllBits(RSMutableBitVectorRef bv, RSBit value)
{
    RSIndex nBuckets, leftover;
    __RSGenericValidInstance(bv, _RSBitVectorTypeID);
//    RSAssert1(__RSBitVectorMutableVariety(bv) == kRSBitVectorMutable , __kRSLogAssertion, "%s(): bit vector is immutable", __PRETTY_FUNCTION__);
    nBuckets = __RSBitVectorCount(bv) / __RS_BITS_PER_BUCKET;
    leftover = __RSBitVectorCount(bv) - nBuckets * __RS_BITS_PER_BUCKET;
    if (0 < leftover)
    {
        RSRange range = RSMakeRange(nBuckets * __RS_BITS_PER_BUCKET, leftover);
        if (value)
        {
            __RSBitVectorInternalMap(bv, range, __RSBitVectorOneBits, NULL);
        }
        else
        {
            __RSBitVectorInternalMap(bv, range, __RSBitVectorZeroBits, NULL);
        }
    }
    memset(bv->_buckets, (value ? ~0 : 0), nBuckets);
}
