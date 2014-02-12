//
//  RSKVBucket.c
//  RSCoreFoundation
//
//  Created by closure on 2/2/14.
//  Copyright (c) 2014 RetVal. All rights reserved.
//

#include "RSKVBucket.h"

#include <RSCoreFoundation/RSRuntime.h>

struct __RSKVBucket
{
    RSRuntimeBase _base;
    RSTypeRef _k;
    RSTypeRef _v;
};

static RSTypeRef __RSKVBucketClassCopy(RSAllocatorRef allocator, RSTypeRef rs, BOOL mutableCopy)
{
    return RSRetain(rs);
}

static void __RSKVBucketClassDeallocate(RSTypeRef rs)
{
    RSKVBucketRef kvb = (RSKVBucketRef)rs;
    RSRelease(kvb->_k);
    RSRelease(kvb->_v);
}

static BOOL __RSKVBucketClassEqual(RSTypeRef rs1, RSTypeRef rs2)
{
    RSKVBucketRef RSKVBucket1 = (RSKVBucketRef)rs1;
    RSKVBucketRef RSKVBucket2 = (RSKVBucketRef)rs2;
    BOOL result = NO;
    
    result = RSEqual(RSKVBucket1->_k, RSKVBucket2->_k) && RSEqual(RSKVBucket1->_v, RSKVBucket2->_v);
    
    return result;
}

static RSHashCode __RSKVBucketClassHash(RSTypeRef rs)
{
    RSKVBucketRef kvb = (RSKVBucketRef)rs;
    return RSHash(kvb->_k) ^ RSHash(kvb->_v);
}

static RSStringRef __RSKVBucketClassDescription(RSTypeRef rs)
{
    RSKVBucketRef kvb = (RSKVBucketRef)rs;
    RSStringRef description = RSStringCreateWithFormat(RSAllocatorDefault, RSSTR("RSKVBucket %p %r -> %r"), kvb, kvb->_k, kvb->_v);
    return description;
}

static RSRuntimeClass __RSKVBucketClass =
{
    _RSRuntimeScannedObject,
    0,
    "RSKVBucket",
    nil,
    __RSKVBucketClassCopy,
    __RSKVBucketClassDeallocate,
    __RSKVBucketClassEqual,
    __RSKVBucketClassHash,
    __RSKVBucketClassDescription,
    nil,
    nil
};

static RSTypeID _RSKVBucketTypeID = _RSRuntimeNotATypeID;
static void __RSKVBucketInitialize();

RSExport RSTypeID RSKVBucketGetTypeID()
{
    static dispatch_once_t onceToken;
    dispatch_once(&onceToken, ^{
        __RSKVBucketInitialize();
    });
    return _RSKVBucketTypeID;
}

static void __RSKVBucketInitialize()
{
    _RSKVBucketTypeID = __RSRuntimeRegisterClass(&__RSKVBucketClass);
    __RSRuntimeSetClassTypeID(&__RSKVBucketClass, _RSKVBucketTypeID);
}

RSPrivate void __RSKVBucketDeallocate()
{
//    <#do your finalize operation#>
}

static RSKVBucketRef __RSKVBucketCreateInstance(RSAllocatorRef allocator, RSTypeRef key, RSTypeRef value)
{
    RSKVBucketRef instance = (RSKVBucketRef)__RSRuntimeCreateInstance(allocator, RSKVBucketGetTypeID(), sizeof(struct __RSKVBucket) - sizeof(RSRuntimeBase));
    
    instance->_k = RSRetain(key);
    instance->_v = RSRetain(value);
    
    return instance;
}

RSExport RSKVBucketRef RSKVBucketCreate(RSAllocatorRef allocator, RSTypeRef key, RSTypeRef value) {
    return __RSKVBucketCreateInstance(allocator, key, value);
}

RSExport RSTypeRef RSKVBucketGetKey(RSKVBucketRef bucket) {
    if (!bucket) return nil;
    __RSGenericValidInstance(bucket, _RSKVBucketTypeID);
    return bucket->_k;
}

RSExport RSTypeRef RSKVBucketGetValue(RSKVBucketRef bucket) {
    if (!bucket) return nil;
    __RSGenericValidInstance(bucket, _RSKVBucketTypeID);
    return bucket->_v;
}

