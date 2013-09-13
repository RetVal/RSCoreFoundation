//
//  RSSet.c
//  RSCoreFoundation
//
//  Created by RetVal on 12/8/12.
//  Copyright (c) 2012 RetVal. All rights reserved.
//

#include <stdio.h>
#include <RSCoreFoundation/RSBase.h>
#include <RSCoreFoundation/RSBaseHash.h>
#include <RSCoreFoundation/RSSet.h>

#include <RSCoreFoundation/RSRuntime.h>
#include <RSCoreFoundation/RSBasicHash.h>

#define RSDictionary 0
#define RSSet 0
#define RSBag 0
#undef RSSet
#define RSSet 1

#if RSDictionary
const RSSetKeyCallBacks RSTypeSetKeyCallBacks = {0, __RSTypeCollectionRetain, __RSTypeCollectionRelease, RSCopyDescription, RSEqual, RSHash};
const RSSetKeyCallBacks RSCopyStringSetKeyCallBacks = {0, __RSStringCollectionCopy, __RSTypeCollectionRelease, RSCopyDescription, RSEqual, RSHash};
const RSSetValueCallBacks RSTypeSetValueCallBacks = {0, __RSTypeCollectionRetain, __RSTypeCollectionRelease, RSCopyDescription, RSEqual};
RSPrivate const RSSetValueCallBacks RSTypeSetValueCompactableCallBacks = {0, __RSTypeCollectionRetain, __RSTypeCollectionRelease, RSCopyDescription, RSEqual};
static const RSSetKeyCallBacks __RSNullSetKeyCallBacks = {0, NULL, NULL, NULL, NULL, NULL};
static const RSSetValueCallBacks __RSNullSetValueCallBacks = {0, NULL, NULL, NULL, NULL};

#define RSHashRef RSDictionaryRef
#define RSMutableHashRef RSMutableDictionaryRef
#define RSHashKeyCallBacks RSSetKeyCallBacks
#define RSHashValueCallBacks RSSetValueCallBacks
#endif

#if RSSet
const RSSetCallBacks RSTypeSetCallBacks = {0, __RSTypeCollectionRetain, __RSTypeCollectionRelease, RSDescription, RSEqual, RSHash};
const RSSetCallBacks RSCopyStringSetCallBacks = {0, __RSStringCollectionCopy, __RSTypeCollectionRelease, RSDescription, RSEqual, RSHash};
static const RSSetCallBacks __RSNullSetCallBacks = {0, NULL, NULL, NULL, NULL, NULL};

#define RSSetKeyCallBacks RSSetCallBacks
#define RSSetValueCallBacks RSSetCallBacks
#define RSTypeSetKeyCallBacks RSTypeSetCallBacks
#define RSTypeSetValueCallBacks RSTypeSetCallBacks
#define __RSNullSetKeyCallBacks __RSNullSetCallBacks
#define __RSNullSetValueCallBacks __RSNullSetCallBacks

#define RSHashRef RSSetRef
#define RSMutableHashRef RSMutableSetRef
#define RSHashKeyCallBacks RSSetCallBacks
#define RSHashValueCallBacks RSSetCallBacks
#endif

typedef uintptr_t any_t;
typedef const void * const_any_pointer_t;
typedef void * any_pointer_t;

static BOOL __RSSetEqual(RSTypeRef obj1, RSTypeRef obj2) {
    return __RSBasicHashEqual((RSBasicHashRef)obj1, (RSBasicHashRef)obj2);
}

static RSHashCode __RSSetHash(RSTypeRef obj) {
    return __RSBasicHashHash((RSBasicHashRef)obj);
}

static RSStringRef __RSSetCopyDescription(RSTypeRef obj) {
    return __RSBasicHashCopyDescription((RSBasicHashRef)obj);
}

static RSTypeRef __RSSetClassCopy(RSAllocatorRef allocator, RSTypeRef rs, BOOL mutableCopy)
{
    if (mutableCopy) {
        RSMutableSetRef copy = RSSetCreateMutableCopy(allocator, RSSetGetCount(rs), rs);
        return copy;
    }
    RSSetRef copy = RSSetCreateCopy(allocator, rs);
    return copy;
}

static void __RSSetDeallocate(RSTypeRef obj) {
    __RSBasicHashDeallocate((RSBasicHashRef)obj);
}

static RSTypeID __RSSetTypeID = _RSRuntimeNotATypeID;

static const RSRuntimeClass __RSSetClass = {
    _RSRuntimeScannedObject,
    "RSSet",
    NULL,        // init
    __RSSetClassCopy,        // copy
    __RSSetDeallocate,
    __RSSetEqual,
    __RSSetHash,
    __RSSetCopyDescription,        //
    NULL,
    NULL
};

RSPrivate void __RSSetInitialize()
{
    __RSSetTypeID = __RSRuntimeRegisterClass(&__RSSetClass);
    __RSRuntimeSetClassTypeID(&__RSSetClass, __RSSetTypeID);
}

RSExport RSTypeID RSSetGetTypeID(void)
{
    return __RSSetTypeID;
}


static uintptr_t __RSSetStandardRetainValue(RSConstBasicHashRef ht, uintptr_t stack_value) {
    if (RSBasicHashGetSpecialBits(ht) & 0x0100) return stack_value;
    return (RSBasicHashHasStrongValues(ht)) ? (uintptr_t)(RSTypeRef)stack_value : (uintptr_t)RSRetain((RSTypeRef)stack_value);
}

static uintptr_t __RSSetStandardRetainKey(RSConstBasicHashRef ht, uintptr_t stack_key) {
    if (RSBasicHashGetSpecialBits(ht) & 0x0001) return stack_key;
    return (RSBasicHashHasStrongKeys(ht)) ? (uintptr_t)(RSTypeRef)stack_key : (uintptr_t)RSRetain((RSTypeRef)stack_key);
}

static void __RSSetStandardReleaseValue(RSConstBasicHashRef ht, uintptr_t stack_value) {
    if (RSBasicHashGetSpecialBits(ht) & 0x0200) return;
    if (RSBasicHashHasStrongValues(ht)) ;//GCRELEASE(RSAllocatorSystemDefault, (RSTypeRef)stack_value);
    else RSRelease((RSTypeRef)stack_value);
}

static void __RSSetStandardReleaseKey(RSConstBasicHashRef ht, uintptr_t stack_key) {
    if (RSBasicHashGetSpecialBits(ht) & 0x0002) return;
    if (RSBasicHashHasStrongKeys(ht)) ;//GCRELEASE(RSAllocatorSystemDefault, (RSTypeRef)stack_key);
    else RSRelease((RSTypeRef)stack_key);
}

static BOOL __RSSetStandardEquateValues(RSConstBasicHashRef ht, uintptr_t coll_value1, uintptr_t stack_value2) {
    if (RSBasicHashGetSpecialBits(ht) & 0x0400) return coll_value1 == stack_value2;
    return RSEqual((RSTypeRef)coll_value1, (RSTypeRef)stack_value2);
}

static BOOL __RSSetStandardEquateKeys(RSConstBasicHashRef ht, uintptr_t coll_key1, uintptr_t stack_key2) {
    if (RSBasicHashGetSpecialBits(ht) & 0x0004) return coll_key1 == stack_key2;
    return RSEqual((RSTypeRef)coll_key1, (RSTypeRef)stack_key2);
}

static uintptr_t __RSSetStandardHashKey(RSConstBasicHashRef ht, uintptr_t stack_key) {
    if (RSBasicHashGetSpecialBits(ht) & 0x0008) return stack_key;
    return (uintptr_t)RSHash((RSTypeRef)stack_key);
}

static uintptr_t __RSSetStandardGetIndirectKey(RSConstBasicHashRef ht, uintptr_t coll_value) {
    return 0;
}

static RSStringRef __RSSetStandardCopyValueDescription(RSConstBasicHashRef ht, uintptr_t stack_value) {
    if (RSBasicHashGetSpecialBits(ht) & 0x0800) return RSStringCreateWithFormat(RSAllocatorSystemDefault, RSSTR("<%p>"), (void *)stack_value);
    return RSDescription((RSTypeRef)stack_value);
}

static RSStringRef __RSSetStandardCopyKeyDescription(RSConstBasicHashRef ht, uintptr_t stack_key) {
    if (RSBasicHashGetSpecialBits(ht) & 0x0010) return RSStringCreateWithFormat(RSAllocatorSystemDefault, RSSTR("<%p>"), (void *)stack_key);
    return RSDescription((RSTypeRef)stack_key);
}

static RSBasicHashCallbacks *__RSSetStandardCopyCallbacks(RSConstBasicHashRef ht, RSAllocatorRef allocator, RSBasicHashCallbacks *cb);
static void __RSSetStandardFreeCallbacks(RSConstBasicHashRef ht, RSAllocatorRef allocator, RSBasicHashCallbacks *cb);

static const RSBasicHashCallbacks RSSetStandardCallbacks = {
    __RSSetStandardCopyCallbacks,
    __RSSetStandardFreeCallbacks,
    __RSSetStandardRetainValue,
    __RSSetStandardRetainKey,
    __RSSetStandardReleaseValue,
    __RSSetStandardReleaseKey,
    __RSSetStandardEquateValues,
    __RSSetStandardEquateKeys,
    __RSSetStandardHashKey,
    __RSSetStandardGetIndirectKey,
    __RSSetStandardCopyValueDescription,
    __RSSetStandardCopyKeyDescription
};

static RSBasicHashCallbacks *__RSSetStandardCopyCallbacks(RSConstBasicHashRef ht, RSAllocatorRef allocator, RSBasicHashCallbacks *cb) {
    return (RSBasicHashCallbacks *)&RSSetStandardCallbacks;
}

static void __RSSetStandardFreeCallbacks(RSConstBasicHashRef ht, RSAllocatorRef allocator, RSBasicHashCallbacks *cb) {
}


static RSBasicHashCallbacks *__RSSetCopyCallbacks(RSConstBasicHashRef ht, RSAllocatorRef allocator, RSBasicHashCallbacks *cb) {
    RSBasicHashCallbacks *newcb = NULL;
//    if (RS_IS_COLLECTABLE_ALLOCATOR(allocator)) {
//        newcb = (RSBasicHashCallbacks *)auto_zone_allocate_object(objc_collectableZone(), sizeof(RSBasicHashCallbacks) + 10 * sizeof(void *), AUTO_MEMORY_UNSCANNED, NO, NO);
//    } else {
    newcb = RSAllocatorAllocate(allocator, sizeof(RSBasicHashCallbacks) + 10 * sizeof(void *));
    //}
    if (!newcb) return NULL;
    memmove(newcb, (void *)cb, sizeof(RSBasicHashCallbacks) + 10 * sizeof(void *));
    return newcb;
}

static void __RSSetFreeCallbacks(RSConstBasicHashRef ht, RSAllocatorRef allocator, RSBasicHashCallbacks *cb) {
    if (RS_IS_COLLECTABLE_ALLOCATOR(allocator)) {
    } else {
        RSAllocatorDeallocate(allocator, cb);
    }
}

static uintptr_t __RSSetRetainValue(RSConstBasicHashRef ht, uintptr_t stack_value) {
    const RSBasicHashCallbacks *cb = RSBasicHashGetCallbacks(ht);
    const_any_pointer_t (*value_retain)(RSAllocatorRef, const_any_pointer_t) = (const_any_pointer_t (*)(RSAllocatorRef, const_any_pointer_t))cb->context[0];
    if (NULL == value_retain) return stack_value;
    return (uintptr_t)INVOKE_CALLBACK2(value_retain, RSGetAllocator(ht), (const_any_pointer_t)stack_value);
}

static uintptr_t __RSSetRetainKey(RSConstBasicHashRef ht, uintptr_t stack_key) {
    const RSBasicHashCallbacks *cb = RSBasicHashGetCallbacks(ht);
    const_any_pointer_t (*key_retain)(RSAllocatorRef, const_any_pointer_t) = (const_any_pointer_t (*)(RSAllocatorRef, const_any_pointer_t))cb->context[1];
    if (NULL == key_retain) return stack_key;
    return (uintptr_t)INVOKE_CALLBACK2(key_retain, RSGetAllocator(ht), (const_any_pointer_t)stack_key);
}

static void __RSSetReleaseValue(RSConstBasicHashRef ht, uintptr_t stack_value) {
    const RSBasicHashCallbacks *cb = RSBasicHashGetCallbacks(ht);
    void (*value_release)(RSAllocatorRef, const_any_pointer_t) = (void (*)(RSAllocatorRef, const_any_pointer_t))cb->context[2];
    if (NULL != value_release) INVOKE_CALLBACK2(value_release, RSGetAllocator(ht), (const_any_pointer_t) stack_value);
}

static void __RSSetReleaseKey(RSConstBasicHashRef ht, uintptr_t stack_key) {
    const RSBasicHashCallbacks *cb = RSBasicHashGetCallbacks(ht);
    void (*key_release)(RSAllocatorRef, const_any_pointer_t) = (void (*)(RSAllocatorRef, const_any_pointer_t))cb->context[3];
    if (NULL != key_release) INVOKE_CALLBACK2(key_release, RSGetAllocator(ht), (const_any_pointer_t) stack_key);
}

static BOOL __RSSetEquateValues(RSConstBasicHashRef ht, uintptr_t coll_value1, uintptr_t stack_value2) {
    const RSBasicHashCallbacks *cb = RSBasicHashGetCallbacks(ht);
    BOOL (*value_equal)(const_any_pointer_t, const_any_pointer_t) = (BOOL (*)(const_any_pointer_t, const_any_pointer_t))cb->context[4];
    if (NULL == value_equal) return (coll_value1 == stack_value2);
    return INVOKE_CALLBACK2(value_equal, (const_any_pointer_t) coll_value1, (const_any_pointer_t) stack_value2) ? 1 : 0;
}

static BOOL __RSSetEquateKeys(RSConstBasicHashRef ht, uintptr_t coll_key1, uintptr_t stack_key2) {
    const RSBasicHashCallbacks *cb = RSBasicHashGetCallbacks(ht);
    BOOL (*key_equal)(const_any_pointer_t, const_any_pointer_t) = (BOOL (*)(const_any_pointer_t, const_any_pointer_t))cb->context[5];
    if (NULL == key_equal) return (coll_key1 == stack_key2);
    return INVOKE_CALLBACK2(key_equal, (const_any_pointer_t) coll_key1, (const_any_pointer_t) stack_key2) ? 1 : 0;
}

static uintptr_t __RSSetHashKey(RSConstBasicHashRef ht, uintptr_t stack_key) {
    const RSBasicHashCallbacks *cb = RSBasicHashGetCallbacks(ht);
    RSHashCode (*hash)(const_any_pointer_t) = (RSHashCode (*)(const_any_pointer_t))cb->context[6];
    if (NULL == hash) return stack_key;
    return (uintptr_t)INVOKE_CALLBACK1(hash, (const_any_pointer_t) stack_key);
}

static uintptr_t __RSSetGetIndirectKey(RSConstBasicHashRef ht, uintptr_t coll_value) {
    return 0;
}

static RSStringRef __RSSetCopyValueDescription(RSConstBasicHashRef ht, uintptr_t stack_value) {
    const RSBasicHashCallbacks *cb = RSBasicHashGetCallbacks(ht);
    RSStringRef (*value_describe)(const_any_pointer_t) = (RSStringRef (*)(const_any_pointer_t))cb->context[8];
    if (NULL == value_describe) return RSStringCreateWithFormat(RSAllocatorSystemDefault, RSSTR("<%p>"), (const_any_pointer_t) stack_value);
    return (RSStringRef)INVOKE_CALLBACK1(value_describe, (const_any_pointer_t) stack_value);
}

static RSStringRef __RSSetCopyKeyDescription(RSConstBasicHashRef ht, uintptr_t stack_key) {
    const RSBasicHashCallbacks *cb = RSBasicHashGetCallbacks(ht);
    RSStringRef (*key_describe)(const_any_pointer_t) = (RSStringRef (*)(const_any_pointer_t))cb->context[9];
    if (NULL == key_describe) return RSStringCreateWithFormat(RSAllocatorSystemDefault, RSSTR("<%p>"), (const_any_pointer_t) stack_key);
    return (RSStringRef)INVOKE_CALLBACK1(key_describe, (const_any_pointer_t) stack_key);
}
#if RSSet
#define RSHashRef RSSetRef
#define RSMutableHashRef RSMutableSetRef
#define RSHashKeyCallBacks RSSetCallBacks
#define RSHashValueCallBacks RSSetCallBacks

#define __RSTypeCollectionRetain nil
#define __RSTypeCollectionRelease nil

#endif
static RSBasicHashRef __RSSetCreateInstance(RSAllocatorRef allocator, const RSHashKeyCallBacks *keyCallBacks, const RSHashValueCallBacks *valueCallBacks, BOOL useValueCB)
{
    RSOptionFlags flags = RSBasicHashLinearHashing; // RSBasicHashExponentialHashing
    flags |= (RSDictionary ? RSBasicHashHasKeys : 0) | (RSBag ? RSBasicHashHasCounts : 0);
    
    RSBasicHashCallbacks *cb = NULL;
    BOOL std_cb = NO;
    uint16_t specialBits = 0;
    const_any_pointer_t (*key_retain)(RSAllocatorRef, const_any_pointer_t) = NULL;
    void (*key_release)(RSAllocatorRef, const_any_pointer_t) = NULL;
    const_any_pointer_t (*value_retain)(RSAllocatorRef, const_any_pointer_t) = NULL;
    void (*value_release)(RSAllocatorRef, const_any_pointer_t) = NULL;
    
    if ((NULL == keyCallBacks || 0 == keyCallBacks->version) && (!useValueCB || NULL == valueCallBacks || 0 == valueCallBacks->version)) {
        BOOL keyRetainNull = NULL == keyCallBacks || NULL == keyCallBacks->retain;
        BOOL keyReleaseNull = NULL == keyCallBacks || NULL == keyCallBacks->release;
        BOOL keyEquateNull = NULL == keyCallBacks || NULL == keyCallBacks->equal;
        BOOL keyHashNull = NULL == keyCallBacks || NULL == keyCallBacks->hash;
        BOOL keyDescribeNull = NULL == keyCallBacks || NULL == keyCallBacks->copyDescription;
        
        BOOL valueRetainNull = (useValueCB && (NULL == valueCallBacks || NULL == valueCallBacks->retain)) || (!useValueCB && keyRetainNull);
        BOOL valueReleaseNull = (useValueCB && (NULL == valueCallBacks || NULL == valueCallBacks->release)) || (!useValueCB && keyReleaseNull);
        BOOL valueEquateNull = (useValueCB && (NULL == valueCallBacks || NULL == valueCallBacks->equal)) || (!useValueCB && keyEquateNull);
        BOOL valueDescribeNull = (useValueCB && (NULL == valueCallBacks || NULL == valueCallBacks->copyDescription)) || (!useValueCB && keyDescribeNull);
        
        BOOL keyRetainStd = keyRetainNull || __RSTypeCollectionRetain == keyCallBacks->retain;
        BOOL keyReleaseStd = keyReleaseNull || __RSTypeCollectionRelease == keyCallBacks->release;
        BOOL keyEquateStd = keyEquateNull || RSEqual == keyCallBacks->equal;
        BOOL keyHashStd = keyHashNull || RSHash == keyCallBacks->hash;
        BOOL keyDescribeStd = keyDescribeNull || RSDescription == keyCallBacks->copyDescription;
        
        BOOL valueRetainStd = (useValueCB && (valueRetainNull || __RSTypeCollectionRetain == valueCallBacks->retain)) || (!useValueCB && keyRetainStd);
        BOOL valueReleaseStd = (useValueCB && (valueReleaseNull || __RSTypeCollectionRelease == valueCallBacks->release)) || (!useValueCB && keyReleaseStd);
        BOOL valueEquateStd = (useValueCB && (valueEquateNull || RSEqual == valueCallBacks->equal)) || (!useValueCB && keyEquateStd);
        BOOL valueDescribeStd = (useValueCB && (valueDescribeNull || RSDescription == valueCallBacks->copyDescription)) || (!useValueCB && keyDescribeStd);
        
        if (keyRetainStd && keyReleaseStd && keyEquateStd && keyHashStd && keyDescribeStd && valueRetainStd && valueReleaseStd && valueEquateStd && valueDescribeStd) {
            cb = (RSBasicHashCallbacks *)&RSSetStandardCallbacks;
            if (!(keyRetainNull || keyReleaseNull || keyEquateNull || keyHashNull || keyDescribeNull || valueRetainNull || valueReleaseNull || valueEquateNull || valueDescribeNull)) {
                std_cb = YES;
            } else {
                // just set these to tickle the GC Strong logic below in a way that mimics past practice
                key_retain = keyCallBacks ? keyCallBacks->retain : NULL;
                key_release = keyCallBacks ? keyCallBacks->release : NULL;
                if (useValueCB) {
                    value_retain = valueCallBacks ? valueCallBacks->retain : NULL;
                    value_release = valueCallBacks ? valueCallBacks->release : NULL;
                } else {
                    value_retain = key_retain;
                    value_release = key_release;
                }
            }
            if (keyRetainNull) specialBits |= 0x0001;
            if (keyReleaseNull) specialBits |= 0x0002;
            if (keyEquateNull) specialBits |= 0x0004;
            if (keyHashNull) specialBits |= 0x0008;
            if (keyDescribeNull) specialBits |= 0x0010;
            if (valueRetainNull) specialBits |= 0x0100;
            if (valueReleaseNull) specialBits |= 0x0200;
            if (valueEquateNull) specialBits |= 0x0400;
            if (valueDescribeNull) specialBits |= 0x0800;
        }
    }
    
    if (!cb) {
        BOOL (*key_equal)(const_any_pointer_t, const_any_pointer_t) = NULL;
        BOOL (*value_equal)(const_any_pointer_t, const_any_pointer_t) = NULL;
        RSStringRef (*key_describe)(const_any_pointer_t) = NULL;
        RSStringRef (*value_describe)(const_any_pointer_t) = NULL;
        RSHashCode (*hash_key)(const_any_pointer_t) = NULL;
        key_retain = keyCallBacks ? keyCallBacks->retain : NULL;
        key_release = keyCallBacks ? keyCallBacks->release : NULL;
        key_equal = keyCallBacks ? keyCallBacks->equal : NULL;
        key_describe = keyCallBacks ? keyCallBacks->copyDescription : NULL;
        if (useValueCB) {
            value_retain = valueCallBacks ? valueCallBacks->retain : NULL;
            value_release = valueCallBacks ? valueCallBacks->release : NULL;
            value_equal = valueCallBacks ? valueCallBacks->equal : NULL;
            value_describe = valueCallBacks ? valueCallBacks->copyDescription : NULL;
        } else {
            value_retain = key_retain;
            value_release = key_release;
            value_equal = key_equal;
            value_describe = key_describe;
        }
        hash_key = keyCallBacks ? keyCallBacks->hash : NULL;
        
        RSBasicHashCallbacks *newcb = NULL;
        if (RS_IS_COLLECTABLE_ALLOCATOR(allocator)) {
            //newcb = (RSBasicHashCallbacks *)auto_zone_allocate_object(objc_collectableZone(), sizeof(RSBasicHashCallbacks) + 10 * sizeof(void *), AUTO_MEMORY_UNSCANNED, NO, NO);
        } else {
            newcb = RSAllocatorAllocate(allocator, sizeof(RSBasicHashCallbacks) + 10 * sizeof(void *));
        }
        if (!newcb) return NULL;
        newcb->copyCallbacks = __RSSetCopyCallbacks;
        newcb->freeCallbacks = __RSSetFreeCallbacks;
        newcb->retainValue = __RSSetRetainValue;
        newcb->retainKey = __RSSetRetainKey;
        newcb->releaseValue = __RSSetReleaseValue;
        newcb->releaseKey = __RSSetReleaseKey;
        newcb->equateValues = __RSSetEquateValues;
        newcb->equateKeys = __RSSetEquateKeys;
        newcb->hashKey = __RSSetHashKey;
        newcb->getIndirectKey = __RSSetGetIndirectKey;
        newcb->copyValueDescription = __RSSetCopyValueDescription;
        newcb->copyKeyDescription = __RSSetCopyKeyDescription;
        newcb->context[0] = (uintptr_t)value_retain;
        newcb->context[1] = (uintptr_t)key_retain;
        newcb->context[2] = (uintptr_t)value_release;
        newcb->context[3] = (uintptr_t)key_release;
        newcb->context[4] = (uintptr_t)value_equal;
        newcb->context[5] = (uintptr_t)key_equal;
        newcb->context[6] = (uintptr_t)hash_key;
        newcb->context[8] = (uintptr_t)value_describe;
        newcb->context[9] = (uintptr_t)key_describe;
        cb = newcb;
    }
    
    if (RS_IS_COLLECTABLE_ALLOCATOR(allocator)) {
        if (std_cb || value_retain != NULL || value_release != NULL) {
            flags |= RSBasicHashStrongValues;
        }
        if (std_cb || key_retain != NULL || key_release != NULL) {
            flags |= RSBasicHashStrongKeys;
        }
#if RSDictionary
        if (valueCallBacks == &RSTypeDictionaryValueCompactableCallBacks) {
            // Foundation allocated collections will have compactable values
            flags |= RSBasicHashCompactableValues;
        }
#endif
    }
    
    RSBasicHashRef ht = RSBasicHashCreate(allocator, flags, cb);
    if (ht) RSBasicHashSetSpecialBits(ht, specialBits);
    if (!ht && !RS_IS_COLLECTABLE_ALLOCATOR(allocator)) RSAllocatorDeallocate(allocator, cb);
    return ht;
}

#if RSSet || RSBag
RSPrivate RSHashRef __RSSetCreateTransfer(RSAllocatorRef allocator, const_any_pointer_t *klist, RSIndex numValues)
{
    const_any_pointer_t *vlist = klist;
#endif
    RSTypeID typeID = RSSetGetTypeID();
    //RSAssert2(0 <= numValues, __RSLogAssertion, "%s(): numValues (%ld) cannot be less than zero", __PRETTY_FUNCTION__, numValues);
    RSOptionFlags flags = RSBasicHashLinearHashing; // RSBasicHashExponentialHashing
    flags |= (RSDictionary ? RSBasicHashHasKeys : 0) | (RSBag ? RSBasicHashHasCounts : 0);
    RSBasicHashCallbacks *cb = (RSBasicHashCallbacks *)&RSSetStandardCallbacks;
    RSBasicHashRef ht = RSBasicHashCreate(allocator, flags, cb);
    RSBasicHashSetSpecialBits(ht, 0x0303);
    if (0 < numValues) RSBasicHashSetCapacity(ht, numValues);
    for (RSIndex idx = 0; idx < numValues; idx++) {
        RSBasicHashAddValue(ht, (uintptr_t)klist[idx], (uintptr_t)vlist[idx]);
    }
    RSBasicHashSetSpecialBits(ht, 0x0000);
    RSBasicHashMakeImmutable(ht);
    //*(uintptr_t *)ht = __RSISAForTypeID(typeID);
    __RSRuntimeSetInstanceTypeID(ht, typeID);
    ///if (__RSOASafe) __RSSetLastAllocationEventName(ht, "RSSet (immutable)");
    return (RSHashRef)ht;
}

#if RSSet || RSBag
RSHashRef RSSetCreate(RSAllocatorRef allocator, const_any_pointer_t *klist, RSIndex numValues, const RSSetKeyCallBacks *keyCallBacks)
{
    const_any_pointer_t *vlist = klist;
    const RSSetValueCallBacks *valueCallBacks = 0;
#endif
    RSTypeID typeID = RSSetGetTypeID();
    //RSAssert2(0 <= numValues, __RSLogAssertion, "%s(): numValues (%ld) cannot be less than zero", __PRETTY_FUNCTION__, numValues);
    RSBasicHashRef ht = __RSSetCreateInstance(allocator, keyCallBacks, valueCallBacks, RSDictionary);
    if (!ht) return NULL;
    if (0 < numValues) RSBasicHashSetCapacity(ht, numValues);
    for (RSIndex idx = 0; idx < numValues; idx++) {
        RSBasicHashAddValue(ht, (uintptr_t)klist[idx], (uintptr_t)vlist[idx]);
    }
    RSBasicHashMakeImmutable(ht);
    //*(uintptr_t *)ht = __RSISAForTypeID(typeID);
    __RSRuntimeSetInstanceTypeID(ht, typeID);
    //if (__RSOASafe) __RSSetLastAllocationEventName(ht, "RSSet (immutable)");
    return (RSHashRef)ht;
}

#if RSSet || RSBag
RSMutableHashRef RSSetCreateMutable(RSAllocatorRef allocator, RSIndex capacity, const RSSetKeyCallBacks *keyCallBacks)
{
    const RSSetValueCallBacks *valueCallBacks = 0;
#endif
    RSTypeID typeID = RSSetGetTypeID();
    //RSAssert2(0 <= capacity, __RSLogAssertion, "%s(): capacity (%ld) cannot be less than zero", __PRETTY_FUNCTION__, capacity);
    RSBasicHashRef ht = __RSSetCreateInstance(allocator, keyCallBacks, valueCallBacks, RSDictionary);
    if (!ht) return NULL;
    //*(uintptr_t *)ht = __RSISAForTypeID(typeID);
    __RSRuntimeSetInstanceTypeID(ht, typeID);
    //if (__RSOASafe) __RSSetLastAllocationEventName(ht, "RSSet (mutable)");
    return (RSMutableHashRef)ht;
}

RSHashRef RSSetCreateCopy(RSAllocatorRef allocator, RSHashRef other)
{
    RSTypeID typeID = RSSetGetTypeID();
    //RSAssert1(other, __RSLogAssertion, "%s(): other RSSet cannot be NULL", __PRETTY_FUNCTION__);
    __RSGenericValidInstance(other, typeID);
    RSBasicHashRef ht = NULL;
    if (RS_IS_OBJC(typeID, other))
    {
        RSIndex numValues = RSSetGetCount(other);
        const_any_pointer_t vbuffer[256], kbuffer[256];
        const_any_pointer_t *vlist = nil;
        if (numValues <= 256) {
            vlist = vbuffer;
        }
        else
        {
            vlist = RSAllocatorAllocate(RSAllocatorSystemDefault, numValues * sizeof(const_any_pointer_t));
        }

#if RSSet || RSBag
        const_any_pointer_t *klist = vlist;
        RSSetGetValues(other, vlist);
#endif
        
#if RSDictionary
        const_any_pointer_t *klist = (numValues <= 256) ? kbuffer : (const_any_pointer_t *)RSAllocatorAllocate(RSAllocatorSystemDefault, numValues * sizeof(const_any_pointer_t), 0);
        RSDictionaryGetKeysAndValues(other, klist, vlist);
#endif
        ht = __RSSetCreateInstance(allocator, & RSTypeSetKeyCallBacks, RSDictionary ? & RSTypeSetValueCallBacks : NULL, RSDictionary);
        if (ht && 0 < numValues) RSBasicHashSetCapacity(ht, numValues);
        for (RSIndex idx = 0; ht && idx < numValues; idx++) {
            RSBasicHashAddValue(ht, (uintptr_t)klist[idx], (uintptr_t)vlist[idx]);
        }
        if (klist != kbuffer && klist != vlist) RSAllocatorDeallocate(RSAllocatorSystemDefault, klist);
        if (vlist != vbuffer) RSAllocatorDeallocate(RSAllocatorSystemDefault, vlist);
    } else {
        ht = RSBasicHashCreateCopy(allocator, (RSBasicHashRef)other);
    }
    if (!ht) return NULL;
    RSBasicHashMakeImmutable(ht);
    //*(uintptr_t *)ht = __RSISAForTypeID(typeID);
    __RSRuntimeSetInstanceTypeID(ht, typeID);
    //if (__RSOASafe) __RSSetLastAllocationEventName(ht, "RSSet (immutable)");
    return (RSHashRef)ht;
}

RSMutableHashRef RSSetCreateMutableCopy(RSAllocatorRef allocator, RSIndex capacity, RSHashRef other) {
    RSTypeID typeID = RSSetGetTypeID();
    //RSAssert1(other, __RSLogAssertion, "%s(): other RSSet cannot be NULL", __PRETTY_FUNCTION__);
    __RSGenericValidInstance(other, typeID);
    //RSAssert2(0 <= capacity, __RSLogAssertion, "%s(): capacity (%ld) cannot be less than zero", __PRETTY_FUNCTION__, capacity);
    RSBasicHashRef ht = NULL;
    if (RS_IS_OBJC(typeID, other)) {
        RSIndex numValues = RSSetGetCount(other);
        const_any_pointer_t vbuffer[256], kbuffer[256];
        const_any_pointer_t *vlist = nil;//(numValues <= 256) ? vbuffer : (const_any_pointer_t *)RSAllocatorAllocate(RSAllocatorSystemDefault, numValues * sizeof(const_any_pointer_t), 0);
        if (numValues <= 256) {
            vlist = vbuffer;
        }
        else
        {
            vlist = RSAllocatorAllocate(RSAllocatorSystemDefault, numValues * sizeof(const_any_pointer_t));
        }
#if RSSet || RSBag
        const_any_pointer_t *klist = vlist;
        RSSetGetValues(other, vlist);
#endif
#if RSDictionary
        const_any_pointer_t *klist = (numValues <= 256) ? kbuffer : (const_any_pointer_t *)RSAllocatorAllocate(RSAllocatorSystemDefault, numValues * sizeof(const_any_pointer_t), 0);
        RSDictionaryGetKeysAndValues(other, klist, vlist);
#endif
        ht = __RSSetCreateInstance(allocator, & RSTypeSetKeyCallBacks, RSDictionary ? & RSTypeSetValueCallBacks : NULL, RSDictionary);
        if (ht && 0 < numValues) RSBasicHashSetCapacity(ht, numValues);
        for (RSIndex idx = 0; ht && idx < numValues; idx++) {
            RSBasicHashAddValue(ht, (uintptr_t)klist[idx], (uintptr_t)vlist[idx]);
        }
        if (klist != kbuffer && klist != vlist) RSAllocatorDeallocate(RSAllocatorSystemDefault, klist);
        if (vlist != vbuffer) RSAllocatorDeallocate(RSAllocatorSystemDefault, vlist);
    } else {
        ht = RSBasicHashCreateCopy(allocator, (RSBasicHashRef)other);
    }
    if (!ht) return NULL;
    //*(uintptr_t *)ht = __RSISAForTypeID(typeID);
    __RSRuntimeSetInstanceTypeID(ht, typeID);
    //if (__RSOASafe) __RSSetLastAllocationEventName(ht, "RSSet (mutable)");
    return (RSMutableHashRef)ht;
}

RSIndex RSSetGetCount(RSHashRef hc) {
    __RSGenericValidInstance(hc, __RSSetTypeID);
    return RSBasicHashGetCount((RSBasicHashRef)hc);
}


#if RSSet || RSBag
RSIndex RSSetGetCountOfValue(RSHashRef hc, const_any_pointer_t key) {
#endif

    __RSGenericValidInstance(hc, __RSSetTypeID);
    return RSBasicHashGetCountOfKey((RSBasicHashRef)hc, (uintptr_t)key);
}

#if RSSet || RSBag
BOOL RSSetContainsValue(RSHashRef hc, const_any_pointer_t key) {
#endif
    __RSGenericValidInstance(hc, __RSSetTypeID);
    return (0 < RSBasicHashGetCountOfKey((RSBasicHashRef)hc, (uintptr_t)key));
}

const_any_pointer_t RSSetGetValue(RSHashRef hc, const_any_pointer_t key)
{
    __RSGenericValidInstance(hc, __RSSetTypeID);
    RSBasicHashBucket bkt = RSBasicHashFindBucket((RSBasicHashRef)hc, (uintptr_t)key);
    return (0 < bkt.count ? (const_any_pointer_t)bkt.weak_value : 0);
}

BOOL RSSetGetValueIfPresent(RSHashRef hc, const_any_pointer_t key, const_any_pointer_t *value) {

    __RSGenericValidInstance(hc, __RSSetTypeID);
    RSBasicHashBucket bkt = RSBasicHashFindBucket((RSBasicHashRef)hc, (uintptr_t)key);
    if (0 < bkt.count)
    {
        if (value)
        {
            if (RSUseCollectableAllocator && (RSBasicHashGetFlags((RSBasicHashRef)hc) & RSBasicHashStrongValues))
            {
                __RSAssignWithWriteBarrier((void **)value, (void *)bkt.weak_value);
            }
            else
            {
                *value = (const_any_pointer_t)bkt.weak_value;
            }
        }
        return YES;
    }
    return NO;
}

#if RSSet || RSBag
void RSSetGetValues(RSHashRef hc, const_any_pointer_t *keybuf) {
    const_any_pointer_t *valuebuf = 0;
#endif
    __RSGenericValidInstance(hc, __RSSetTypeID);
    if (RSUseCollectableAllocator) {
    RSOptionFlags flags = RSBasicHashGetFlags((RSBasicHashRef)hc);
    __block const_any_pointer_t *keys = keybuf;
    __block const_any_pointer_t *values = valuebuf;
    RSBasicHashApply((RSBasicHashRef)hc, ^(RSBasicHashBucket bkt) {
        for (RSIndex cnt = bkt.count; cnt--;) {
            if (keybuf && (flags & RSBasicHashStrongKeys)) { __RSAssignWithWriteBarrier((void **)keys, (void *)bkt.weak_key); keys++; }
            if (keybuf && !(flags & RSBasicHashStrongKeys)) { *keys++ = (const_any_pointer_t)bkt.weak_key; }
            if (valuebuf && (flags & RSBasicHashStrongValues)) { __RSAssignWithWriteBarrier((void **)values, (void *)bkt.weak_value); values++; }
            if (valuebuf && !(flags & RSBasicHashStrongValues)) { *values++ = (const_any_pointer_t)bkt.weak_value; }
        }
        return (BOOL)YES;
    });
    } else {
        RSBasicHashGetElements((RSBasicHashRef)hc, RSSetGetCount(hc), (uintptr_t *)valuebuf, (uintptr_t *)keybuf);
    }
}

void RSSetApplyFunction(RSHashRef hc, RSSetApplierFunction applier, any_pointer_t context)
{
    FAULT_CALLBACK((void **)&(applier));

    __RSGenericValidInstance(hc, __RSSetTypeID);
    RSBasicHashApply((RSBasicHashRef)hc, ^(RSBasicHashBucket bkt) {
    
    #if RSSet
        INVOKE_CALLBACK2(applier, (const_any_pointer_t)bkt.weak_value, context);
    #endif

        return (BOOL)YES;
    });
}

// This function is for Foundation's benefit; no one else should use it.
//RSExport unsigned long _RSSetFastEnumeration(RSHashRef hc, struct __objcFastEnumerationStateEquivalent *state, void *stackbuffer, unsigned long count)
//{
//    if (RS_IS_OBJC(__RSSetTypeID, hc)) return 0;
//    __RSGenericValidInstance(hc, __RSSetTypeID);
//    return __RSBasicHashFastEnumeration((RSBasicHashRef)hc, (struct __RSFastEnumerationStateEquivalent2 *)state, stackbuffer, count);
//}

// This function is for Foundation's benefit; no one else should use it.
RSExport BOOL _RSSetIsMutable(RSHashRef hc)
{
    if (RS_IS_OBJC(__RSSetTypeID, hc)) return NO;
    __RSGenericValidInstance(hc, __RSSetTypeID);
    return RSBasicHashIsMutable((RSBasicHashRef)hc);
}

// This function is for Foundation's benefit; no one else should use it.
RSExport void _RSSetSetCapacity(RSMutableHashRef hc, RSIndex cap) {
    if (RS_IS_OBJC(__RSSetTypeID, hc)) return;
    __RSGenericValidInstance(hc, __RSSetTypeID);
    //RSAssert2(RSBasicHashIsMutable((RSBasicHashRef)hc), __RSLogAssertion, "%s(): immutable collection %p passed to mutating operation", __PRETTY_FUNCTION__, hc);
    //RSAssert3(RSSetGetCount(hc) <= cap, __RSLogAssertion, "%s(): desired capacity (%ld) is less than count (%ld)", __PRETTY_FUNCTION__, cap, RSSetGetCount(hc));
    RSBasicHashSetCapacity((RSBasicHashRef)hc, cap);
}

RSInline RSIndex __RSSetGetKVOBit(RSHashRef hc) {
    return __RSBitfieldGetValue(((RSRuntimeBase *)hc)->_rsinfo._rsinfo1, 0, 0);
}

RSInline void __RSSetSetKVOBit(RSHashRef hc, RSIndex bit) {
    __RSBitfieldSetValue(((RSRuntimeBase *)hc)->_rsinfo._rsinfo1, 0, 0, ((uintptr_t)bit & 0x1));
}

// This function is for Foundation's benefit; no one else should use it.
RSExport RSIndex _RSSetGetKVOBit(RSHashRef hc) {
    return __RSSetGetKVOBit(hc);
}

// This function is for Foundation's benefit; no one else should use it.
RSExport void _RSSetSetKVOBit(RSHashRef hc, RSIndex bit) {
    __RSSetSetKVOBit(hc, bit);
}


#if !defined(RS_OBJC_KVO_WILLCHANGE)
#define RS_OBJC_KVO_WILLCHANGE(obj, key)
#define RS_OBJC_KVO_DIDCHANGE(obj, key)
#define RS_OBJC_KVO_WILLCHANGEALL(obj)
#define RS_OBJC_KVO_DIDCHANGEALL(obj)
#endif


#if RSSet || RSBag
void RSSetAddValue(RSMutableHashRef hc, const_any_pointer_t key)
{
    const_any_pointer_t value = key;
#endif
    __RSGenericValidInstance(hc, __RSSetTypeID);
    //RSAssert2(RSBasicHashIsMutable((RSBasicHashRef)hc), __RSLogAssertion, "%s(): immutable collection %p passed to mutating operation", __PRETTY_FUNCTION__, hc);
    if (!RSBasicHashIsMutable((RSBasicHashRef)hc)) {
        RSLog(RSSTR("%s(): immutable collection %p given to mutating function"), __PRETTY_FUNCTION__, hc);
    }
    RS_OBJC_KVO_WILLCHANGE(hc, key);
    RSBasicHashAddValue((RSBasicHashRef)hc, (uintptr_t)key, (uintptr_t)value);
    RS_OBJC_KVO_DIDCHANGE(hc, key);
}

#if RSSet || RSBag
void RSSetReplaceValue(RSMutableHashRef hc, const_any_pointer_t key)
{
    const_any_pointer_t value = key;
#endif
    __RSGenericValidInstance(hc, __RSSetTypeID);
    //RSAssert2(RSBasicHashIsMutable((RSBasicHashRef)hc), __RSLogAssertion, "%s(): immutable collection %p passed to mutating operation", __PRETTY_FUNCTION__, hc);
    if (!RSBasicHashIsMutable((RSBasicHashRef)hc)) {
        RSLog(RSSTR("%s(): immutable collection %p given to mutating function"), __PRETTY_FUNCTION__, hc);
    }
    RS_OBJC_KVO_WILLCHANGE(hc, key);
    RSBasicHashReplaceValue((RSBasicHashRef)hc, (uintptr_t)key, (uintptr_t)value);
    RS_OBJC_KVO_DIDCHANGE(hc, key);
}

#if RSSet || RSBag
void RSSetSetValue(RSMutableHashRef hc, const_any_pointer_t key)
{
    const_any_pointer_t value = key;
#endif
    __RSGenericValidInstance(hc, __RSSetTypeID);
    //RSAssert2(RSBasicHashIsMutable((RSBasicHashRef)hc), __RSLogAssertion, "%s(): immutable collection %p passed to mutating operation", __PRETTY_FUNCTION__, hc);
    if (!RSBasicHashIsMutable((RSBasicHashRef)hc)) {
        RSLog(RSSTR("%s(): immutable collection %p given to mutating function"), __PRETTY_FUNCTION__, hc);
    }
    RS_OBJC_KVO_WILLCHANGE(hc, key);
    //#warning this for a dictionary used to not replace the key
    RSBasicHashSetValue((RSBasicHashRef)hc, (uintptr_t)key, (uintptr_t)value);
    RS_OBJC_KVO_DIDCHANGE(hc, key);
}

void RSSetRemoveValue(RSMutableHashRef hc, const_any_pointer_t key)
{
    if (RSDictionary) RS_OBJC_FUNCDISPATCHV(__RSSetTypeID, void, (NSMutableDictionary *)hc, removeObjectForKey:(id)key);
    if (RSSet) RS_OBJC_FUNCDISPATCHV(__RSSetTypeID, void, (NSMutableSet *)hc, removeObject:(id)key);
    __RSGenericValidInstance(hc, __RSSetTypeID);
    //RSAssert2(RSBasicHashIsMutable((RSBasicHashRef)hc), __RSLogAssertion, "%s(): immutable collection %p passed to mutating operation", __PRETTY_FUNCTION__, hc);
    if (!RSBasicHashIsMutable((RSBasicHashRef)hc)) {
        RSLog(RSSTR("%s(): immutable collection %p given to mutating function"), __PRETTY_FUNCTION__, hc);
    }
    RS_OBJC_KVO_WILLCHANGE(hc, key);
    RSBasicHashRemoveValue((RSBasicHashRef)hc, (uintptr_t)key);
    RS_OBJC_KVO_DIDCHANGE(hc, key);
}

void RSSetRemoveAllValues(RSMutableHashRef hc) {
    if (RSDictionary) RS_OBJC_FUNCDISPATCHV(__RSSetTypeID, void, (NSMutableDictionary *)hc, removeAllObjects);
    if (RSSet) RS_OBJC_FUNCDISPATCHV(__RSSetTypeID, void, (NSMutableSet *)hc, removeAllObjects);
    __RSGenericValidInstance(hc, __RSSetTypeID);
    //RSAssert2(RSBasicHashIsMutable((RSBasicHashRef)hc), __RSLogAssertion, "%s(): immutable collection %p passed to mutating operation", __PRETTY_FUNCTION__, hc);
    if (!RSBasicHashIsMutable((RSBasicHashRef)hc)) {
        RSLog(RSSTR("%s(): immutable collection %p given to mutating function"), __PRETTY_FUNCTION__, hc);
    }
    RS_OBJC_KVO_WILLCHANGEALL(hc);
    RSBasicHashRemoveAllValues((RSBasicHashRef)hc);
    RS_OBJC_KVO_DIDCHANGEALL(hc);
}
    
#undef RS_OBJC_KVO_WILLCHANGE
#undef RS_OBJC_KVO_DIDCHANGE
#undef RS_OBJC_KVO_WILLCHANGEALL
#undef RS_OBJC_KVO_DIDCHANGEALL