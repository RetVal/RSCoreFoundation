//
//  RSBag.c
//  RSCoreFoundation
//
//  Created by Closure on 11/10/13.
//  Copyright (c) 2013 RetVal. All rights reserved.
//

#include "RSBag.h"

#include <RSCoreFoundation/RSRuntime.h>

#include "RSInternal.h"
#include "RSBasicHash.h"
#include <RSCoreFoundation/RSString.h>


#define RSDictionary 0
#define RSSet 0
#define RSBag 0
#undef RSBag
#define RSBag 1

#if RSDictionary
const RSBagKeyCallBacks RSTypeBagKeyCallBacks = {0, __RSTypeCollectionRetain, __RSTypeCollectionRelease, RSDescription, RSEqual, RSHash};
const RSBagKeyCallBacks RSCopyStringBagKeyCallBacks = {0, __RSStringCollectionCopy, __RSTypeCollectionRelease, RSDescription, RSEqual, RSHash};
const RSBagValueCallBacks RSTypeBagValueCallBacks = {0, __RSTypeCollectionRetain, __RSTypeCollectionRelease, RSDescription, RSEqual};
RSPrivate const RSBagValueCallBacks RSTypeBagValueCompactableCallBacks = {0, __RSTypeCollectionRetain, __RSTypeCollectionRelease, RSDescription, RSEqual};
static const RSBagKeyCallBacks __RSNullBagKeyCallBacks = {0, nil, nil, nil, nil, nil};
static const RSBagValueCallBacks __RSNullBagValueCallBacks = {0, nil, nil, nil, nil};

#define RSHashRef RSDictionaryRef
#define RSMutableHashRef RSMutableDictionaryRef
#define RSHashKeyCallBacks RSBagKeyCallBacks
#define RSHashValueCallBacks RSBagValueCallBacks
#endif

#if RSSet
const RSBagCallBacks RSTypeBagCallBacks = {0, __RSTypeCollectionRetain, __RSTypeCollectionRelease, RSDescription, RSEqual, RSHash};
const RSBagCallBacks RSCopyStringBagCallBacks = {0, __RSStringCollectionCopy, __RSTypeCollectionRelease, RSDescription, RSEqual, RSHash};
static const RSBagCallBacks __RSNullBagCallBacks = {0, nil, nil, nil, nil, nil};

#define RSBagKeyCallBacks RSBagCallBacks
#define RSBagValueCallBacks RSBagCallBacks
#define RSTypeBagKeyCallBacks RSTypeBagCallBacks
#define RSTypeBagValueCallBacks RSTypeBagCallBacks
#define __RSNullBagKeyCallBacks __RSNullBagCallBacks
#define __RSNullBagValueCallBacks __RSNullBagCallBacks

#define RSHashRef RSSetRef
#define RSMutableHashRef RSMutableSetRef
#define RSHashKeyCallBacks RSBagCallBacks
#define RSHashValueCallBacks RSBagCallBacks
#endif

#if RSBag
const RSBagCallBacks RSTypeBagCallBacks = {0, __RSTypeCollectionRetain, __RSTypeCollectionRelease, RSDescription, RSEqual, RSHash};
const RSBagCallBacks RSCopyStringBagCallBacks = {0, __RSStringCollectionCopy, __RSTypeCollectionRelease, RSDescription, RSEqual, RSHash};
static const RSBagCallBacks __RSNullBagCallBacks = {0, nil, nil, nil, nil, nil};

#define RSBagKeyCallBacks RSBagCallBacks
#define RSBagValueCallBacks RSBagCallBacks
#define RSTypeBagKeyCallBacks RSTypeBagCallBacks
#define RSTypeBagValueCallBacks RSTypeBagCallBacks
#define __RSNullBagKeyCallBacks __RSNullBagCallBacks
#define __RSNullBagValueCallBacks __RSNullBagCallBacks

#define RSHashRef RSBagRef
#define RSMutableHashRef RSMutableBagRef
#define RSHashKeyCallBacks RSBagCallBacks
#define RSHashValueCallBacks RSBagCallBacks
#endif


typedef uintptr_t any_t;
typedef const void * const_any_pointer_t;
typedef void * any_pointer_t;

static BOOL __RSBagClassEqual(RSTypeRef cf1, RSTypeRef cf2) {
    return __RSBasicHashEqual((RSBasicHashRef)cf1, (RSBasicHashRef)cf2);
}

static RSHashCode __RSBagClassHash(RSTypeRef cf) {
    return __RSBasicHashHash((RSBasicHashRef)cf);
}

static RSStringRef __RSBagClassDescription(RSTypeRef cf) {
    return __RSBasicHashDescription((RSBasicHashRef)cf);
}

static void __RSBagClassDeallocate(RSTypeRef cf) {
    __RSBasicHashDeallocate((RSBasicHashRef)cf);
}

static RSTypeID __RSBagTypeID = _RSRuntimeNotATypeID;

static const RSRuntimeClass __RSBagClass = {
    _RSRuntimeScannedObject,
    "RSBag",
    nil,        // init
    nil,        // copy
    __RSBagClassDeallocate,
    __RSBagClassEqual,
    __RSBagClassHash,
    __RSBagClassDescription,
    nil,
    nil
};

RSTypeID RSBagGetTypeID(void) {
    if (_RSRuntimeNotATypeID == __RSBagTypeID)
    {
        __RSBagTypeID = __RSRuntimeRegisterClass(&__RSBagClass);
        __RSRuntimeSetClassTypeID(&__RSBagClass, __RSBagTypeID);
    }
    return __RSBagTypeID;
}

#if (RSBasicHashVersion == 1)

#define GCRETAIN(A, B) RSTypeSetCallBacks.retain(A, B)
#define GCRELEASE(A, B) RSTypeSetCallBacks.release(A, B)

static uintptr_t __RSBagStandardRetainValue(RSConstBasicHashRef ht, uintptr_t stack_value) {
    if (RSBasicHashGetSpecialBits(ht) & 0x0100) return stack_value;
    return (RSBasicHashHasStrongValues(ht)) ? (uintptr_t)GCRETAIN(RSAllocatorSystemDefault, (RSTypeRef)stack_value) : (uintptr_t)RSRetain((RSTypeRef)stack_value);
}

static uintptr_t __RSBagStandardRetainKey(RSConstBasicHashRef ht, uintptr_t stack_key) {
    if (RSBasicHashGetSpecialBits(ht) & 0x0001) return stack_key;
    return (RSBasicHashHasStrongKeys(ht)) ? (uintptr_t)GCRETAIN(RSAllocatorSystemDefault, (RSTypeRef)stack_key) : (uintptr_t)RSRetain((RSTypeRef)stack_key);
}

static void __RSBagStandardReleaseValue(RSConstBasicHashRef ht, uintptr_t stack_value) {
    if (RSBasicHashGetSpecialBits(ht) & 0x0200) return;
    if (RSBasicHashHasStrongValues(ht)) GCRELEASE(RSAllocatorSystemDefault, (RSTypeRef)stack_value); else RSRelease((RSTypeRef)stack_value);
}

static void __RSBagStandardReleaseKey(RSConstBasicHashRef ht, uintptr_t stack_key) {
    if (RSBasicHashGetSpecialBits(ht) & 0x0002) return;
    if (RSBasicHashHasStrongKeys(ht)) GCRELEASE(RSAllocatorSystemDefault, (RSTypeRef)stack_key); else RSRelease((RSTypeRef)stack_key);
}

static BOOL __RSBagStandardEquateValues(RSConstBasicHashRef ht, uintptr_t coll_value1, uintptr_t stack_value2) {
    if (RSBasicHashGetSpecialBits(ht) & 0x0400) return coll_value1 == stack_value2;
    return RSEqual((RSTypeRef)coll_value1, (RSTypeRef)stack_value2);
}

static BOOL __RSBagStandardEquateKeys(RSConstBasicHashRef ht, uintptr_t coll_key1, uintptr_t stack_key2) {
    if (RSBasicHashGetSpecialBits(ht) & 0x0004) return coll_key1 == stack_key2;
    return RSEqual((RSTypeRef)coll_key1, (RSTypeRef)stack_key2);
}

static uintptr_t __RSBagStandardHashKey(RSConstBasicHashRef ht, uintptr_t stack_key) {
    if (RSBasicHashGetSpecialBits(ht) & 0x0008) return stack_key;
    return (uintptr_t)RSHash((RSTypeRef)stack_key);
}

static uintptr_t __RSBagStandardGetIndirectKey(RSConstBasicHashRef ht, uintptr_t coll_value) {
    return 0;
}

static RSStringRef __RSBagStandardCopyValueDescription(RSConstBasicHashRef ht, uintptr_t stack_value) {
    if (RSBasicHashGetSpecialBits(ht) & 0x0800) return RSStringCreateWithFormat(RSAllocatorSystemDefault, nil, RSSTR("<%p>"), (void *)stack_value);
    return RSDescription((RSTypeRef)stack_value);
}

static RSStringRef __RSBagStandardCopyKeyDescription(RSConstBasicHashRef ht, uintptr_t stack_key) {
    if (RSBasicHashGetSpecialBits(ht) & 0x0010) return RSStringCreateWithFormat(RSAllocatorSystemDefault, nil, RSSTR("<%p>"), (void *)stack_key);
    return RSDescription((RSTypeRef)stack_key);
}

static RSBasicHashCallbacks *__RSBagStandardCopyCallbacks(RSConstBasicHashRef ht, RSAllocatorRef allocator, RSBasicHashCallbacks *cb);
static void __RSBagStandardFreeCallbacks(RSConstBasicHashRef ht, RSAllocatorRef allocator, RSBasicHashCallbacks *cb);

static const RSBasicHashCallbacks RSBagStandardCallbacks = {
    __RSBagStandardCopyCallbacks,
    __RSBagStandardFreeCallbacks,
    __RSBagStandardRetainValue,
    __RSBagStandardRetainKey,
    __RSBagStandardReleaseValue,
    __RSBagStandardReleaseKey,
    __RSBagStandardEquateValues,
    __RSBagStandardEquateKeys,
    __RSBagStandardHashKey,
    __RSBagStandardGetIndirectKey,
    __RSBagStandardCopyValueDescription,
    __RSBagStandardCopyKeyDescription
};

static RSBasicHashCallbacks *__RSBagStandardCopyCallbacks(RSConstBasicHashRef ht, RSAllocatorRef allocator, RSBasicHashCallbacks *cb) {
    return (RSBasicHashCallbacks *)&RSBagStandardCallbacks;
}

static void __RSBagStandardFreeCallbacks(RSConstBasicHashRef ht, RSAllocatorRef allocator, RSBasicHashCallbacks *cb) {
}


static RSBasicHashCallbacks *__RSBagCopyCallbacks(RSConstBasicHashRef ht, RSAllocatorRef allocator, RSBasicHashCallbacks *cb) {
    RSBasicHashCallbacks *newcb = nil;
    newcb = (RSBasicHashCallbacks *)RSAllocatorAllocate(allocator, sizeof(RSBasicHashCallbacks) + 10 * sizeof(void *));
    if (!newcb) return nil;
    memmove(newcb, (void *)cb, sizeof(RSBasicHashCallbacks) + 10 * sizeof(void *));
    return newcb;
}

static void __RSBagFreeCallbacks(RSConstBasicHashRef ht, RSAllocatorRef allocator, RSBasicHashCallbacks *cb) {
    if (RS_IS_COLLECTABLE_ALLOCATOR(allocator)) {
    } else {
        RSAllocatorDeallocate(allocator, cb);
    }
}

static uintptr_t __RSBagRetainValue(RSConstBasicHashRef ht, uintptr_t stack_value) {
    const RSBasicHashCallbacks *cb = RSBasicHashGetCallbacks(ht);
    const_any_pointer_t (*value_retain)(RSAllocatorRef, const_any_pointer_t) = (const_any_pointer_t (*)(RSAllocatorRef, const_any_pointer_t))cb->context[0];
    if (nil == value_retain) return stack_value;
    return (uintptr_t)INVOKE_CALLBACK2(value_retain, RSGetAllocator(ht), (const_any_pointer_t)stack_value);
}

static uintptr_t __RSBagRetainKey(RSConstBasicHashRef ht, uintptr_t stack_key) {
    const RSBasicHashCallbacks *cb = RSBasicHashGetCallbacks(ht);
    const_any_pointer_t (*key_retain)(RSAllocatorRef, const_any_pointer_t) = (const_any_pointer_t (*)(RSAllocatorRef, const_any_pointer_t))cb->context[1];
    if (nil == key_retain) return stack_key;
    return (uintptr_t)INVOKE_CALLBACK2(key_retain, RSGetAllocator(ht), (const_any_pointer_t)stack_key);
}

static void __RSBagReleaseValue(RSConstBasicHashRef ht, uintptr_t stack_value) {
    const RSBasicHashCallbacks *cb = RSBasicHashGetCallbacks(ht);
    void (*value_release)(RSAllocatorRef, const_any_pointer_t) = (void (*)(RSAllocatorRef, const_any_pointer_t))cb->context[2];
    if (nil != value_release) INVOKE_CALLBACK2(value_release, RSGetAllocator(ht), (const_any_pointer_t) stack_value);
}

static void __RSBagReleaseKey(RSConstBasicHashRef ht, uintptr_t stack_key) {
    const RSBasicHashCallbacks *cb = RSBasicHashGetCallbacks(ht);
    void (*key_release)(RSAllocatorRef, const_any_pointer_t) = (void (*)(RSAllocatorRef, const_any_pointer_t))cb->context[3];
    if (nil != key_release) INVOKE_CALLBACK2(key_release, RSGetAllocator(ht), (const_any_pointer_t) stack_key);
}

static BOOL __RSBagEquateValues(RSConstBasicHashRef ht, uintptr_t coll_value1, uintptr_t stack_value2) {
    const RSBasicHashCallbacks *cb = RSBasicHashGetCallbacks(ht);
    BOOL (*value_equal)(const_any_pointer_t, const_any_pointer_t) = (BOOL (*)(const_any_pointer_t, const_any_pointer_t))cb->context[4];
    if (nil == value_equal) return (coll_value1 == stack_value2);
    return INVOKE_CALLBACK2(value_equal, (const_any_pointer_t) coll_value1, (const_any_pointer_t) stack_value2) ? 1 : 0;
}

static BOOL __RSBagEquateKeys(RSConstBasicHashRef ht, uintptr_t coll_key1, uintptr_t stack_key2) {
    const RSBasicHashCallbacks *cb = RSBasicHashGetCallbacks(ht);
    BOOL (*key_equal)(const_any_pointer_t, const_any_pointer_t) = (BOOL (*)(const_any_pointer_t, const_any_pointer_t))cb->context[5];
    if (nil == key_equal) return (coll_key1 == stack_key2);
    return INVOKE_CALLBACK2(key_equal, (const_any_pointer_t) coll_key1, (const_any_pointer_t) stack_key2) ? 1 : 0;
}

static uintptr_t __RSBagHashKey(RSConstBasicHashRef ht, uintptr_t stack_key) {
    const RSBasicHashCallbacks *cb = RSBasicHashGetCallbacks(ht);
    RSHashCode (*hash)(const_any_pointer_t) = (RSHashCode (*)(const_any_pointer_t))cb->context[6];
    if (nil == hash) return stack_key;
    return (uintptr_t)INVOKE_CALLBACK1(hash, (const_any_pointer_t) stack_key);
}

static uintptr_t __RSBagGetIndirectKey(RSConstBasicHashRef ht, uintptr_t coll_value) {
    return 0;
}

static RSStringRef __RSBagCopyValueDescription(RSConstBasicHashRef ht, uintptr_t stack_value) {
    const RSBasicHashCallbacks *cb = RSBasicHashGetCallbacks(ht);
    RSStringRef (*value_describe)(const_any_pointer_t) = (RSStringRef (*)(const_any_pointer_t))cb->context[8];
    if (nil == value_describe) return RSStringCreateWithFormat(RSAllocatorSystemDefault, nil, RSSTR("<%p>"), (const_any_pointer_t) stack_value);
    return (RSStringRef)INVOKE_CALLBACK1(value_describe, (const_any_pointer_t) stack_value);
}

static RSStringRef __RSBagCopyKeyDescription(RSConstBasicHashRef ht, uintptr_t stack_key) {
    const RSBasicHashCallbacks *cb = RSBasicHashGetCallbacks(ht);
    RSStringRef (*key_describe)(const_any_pointer_t) = (RSStringRef (*)(const_any_pointer_t))cb->context[9];
    if (nil == key_describe) return RSStringCreateWithFormat(RSAllocatorSystemDefault, nil, RSSTR("<%p>"), (const_any_pointer_t) stack_key);
    return (RSStringRef)INVOKE_CALLBACK1(key_describe, (const_any_pointer_t) stack_key);
}

static RSBasicHashRef __RSBagCreateGeneric(RSAllocatorRef allocator, const RSHashKeyCallBacks *keyCallBacks, const RSHashValueCallBacks *valueCallBacks, BOOL useValueCB) {
    RSOptionFlags flags = RSBasicHashLinearHashing; // RSBasicHashExponentialHashing
    flags |= (RSDictionary ? RSBasicHashHasKeys : 0) | (RSBag ? RSBasicHashHasCounts : 0);
    
    RSBasicHashCallbacks *cb = nil;
    BOOL std_cb = false;
    uint16_t specialBits = 0;
    const_any_pointer_t (*key_retain)(RSAllocatorRef, const_any_pointer_t) = nil;
    void (*key_release)(RSAllocatorRef, const_any_pointer_t) = nil;
    const_any_pointer_t (*value_retain)(RSAllocatorRef, const_any_pointer_t) = nil;
    void (*value_release)(RSAllocatorRef, const_any_pointer_t) = nil;
    
    if ((nil == keyCallBacks || 0 == keyCallBacks->version) && (!useValueCB || nil == valueCallBacks || 0 == valueCallBacks->version)) {
        BOOL keyRetainNull = nil == keyCallBacks || nil == keyCallBacks->retain;
        BOOL keyReleaseNull = nil == keyCallBacks || nil == keyCallBacks->release;
        BOOL keyEquateNull = nil == keyCallBacks || nil == keyCallBacks->equal;
        BOOL keyHashNull = nil == keyCallBacks || nil == keyCallBacks->hash;
        BOOL keyDescribeNull = nil == keyCallBacks || nil == keyCallBacks->copyDescription;
        
        BOOL valueRetainNull = (useValueCB && (nil == valueCallBacks || nil == valueCallBacks->retain)) || (!useValueCB && keyRetainNull);
        BOOL valueReleaseNull = (useValueCB && (nil == valueCallBacks || nil == valueCallBacks->release)) || (!useValueCB && keyReleaseNull);
        BOOL valueEquateNull = (useValueCB && (nil == valueCallBacks || nil == valueCallBacks->equal)) || (!useValueCB && keyEquateNull);
        BOOL valueDescribeNull = (useValueCB && (nil == valueCallBacks || nil == valueCallBacks->copyDescription)) || (!useValueCB && keyDescribeNull);
        
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
            cb = (RSBasicHashCallbacks *)&RSBagStandardCallbacks;
            if (!(keyRetainNull || keyReleaseNull || keyEquateNull || keyHashNull || keyDescribeNull || valueRetainNull || valueReleaseNull || valueEquateNull || valueDescribeNull)) {
                std_cb = true;
            } else {
                // just set these to tickle the GC Strong logic below in a way that mimics past practice
                key_retain = keyCallBacks ? keyCallBacks->retain : nil;
                key_release = keyCallBacks ? keyCallBacks->release : nil;
                if (useValueCB) {
                    value_retain = valueCallBacks ? valueCallBacks->retain : nil;
                    value_release = valueCallBacks ? valueCallBacks->release : nil;
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
        BOOL (*key_equal)(const_any_pointer_t, const_any_pointer_t) = nil;
        BOOL (*value_equal)(const_any_pointer_t, const_any_pointer_t) = nil;
        RSStringRef (*key_describe)(const_any_pointer_t) = nil;
        RSStringRef (*value_describe)(const_any_pointer_t) = nil;
        RSHashCode (*hash_key)(const_any_pointer_t) = nil;
        key_retain = keyCallBacks ? keyCallBacks->retain : nil;
        key_release = keyCallBacks ? keyCallBacks->release : nil;
        key_equal = keyCallBacks ? keyCallBacks->equal : nil;
        key_describe = keyCallBacks ? keyCallBacks->copyDescription : nil;
        if (useValueCB) {
            value_retain = valueCallBacks ? valueCallBacks->retain : nil;
            value_release = valueCallBacks ? valueCallBacks->release : nil;
            value_equal = valueCallBacks ? valueCallBacks->equal : nil;
            value_describe = valueCallBacks ? valueCallBacks->copyDescription : nil;
        } else {
            value_retain = key_retain;
            value_release = key_release;
            value_equal = key_equal;
            value_describe = key_describe;
        }
        hash_key = keyCallBacks ? keyCallBacks->hash : nil;
        
        RSBasicHashCallbacks *newcb = nil;
        newcb = (RSBasicHashCallbacks *)RSAllocatorAllocate(allocator, sizeof(RSBasicHashCallbacks) + 10 * sizeof(void *));
        if (!newcb) return nil;
        newcb->copyCallbacks = __RSBagCopyCallbacks;
        newcb->freeCallbacks = __RSBagFreeCallbacks;
        newcb->retainValue = __RSBagRetainValue;
        newcb->retainKey = __RSBagRetainKey;
        newcb->releaseValue = __RSBagReleaseValue;
        newcb->releaseKey = __RSBagReleaseKey;
        newcb->equateValues = __RSBagEquateValues;
        newcb->equateKeys = __RSBagEquateKeys;
        newcb->hashKey = __RSBagHashKey;
        newcb->getIndirectKey = __RSBagGetIndirectKey;
        newcb->copyValueDescription = __RSBagCopyValueDescription;
        newcb->copyKeyDescription = __RSBagCopyKeyDescription;
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
        if (std_cb || value_retain != nil || value_release != nil) {
            flags |= RSBasicHashStrongValues;
        }
        if (std_cb || key_retain != nil || key_release != nil) {
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

#if RSDictionary
RSPrivate RSHashRef __RSBagCreateTransfer(RSAllocatorRef allocator, const_any_pointer_t *klist, const_any_pointer_t *vlist, RSIndex numValues)
#endif
#if RSSet || RSBag
RSPrivate RSHashRef __RSBagCreateTransfer(RSAllocatorRef allocator, const_any_pointer_t *klist, RSIndex numValues)

#endif
{
#if RSSet || RSBag
    const_any_pointer_t *vlist = klist;
#endif
    RSTypeID typeID = RSBagGetTypeID();
    RSAssert2(0 <= numValues, __RSLogAssertion, "%s(): numValues (%ld) cannot be less than zero", __PRETTY_FUNCTION__, numValues);
    RSOptionFlags flags = RSBasicHashLinearHashing; // RSBasicHashExponentialHashing
    flags |= (RSDictionary ? RSBasicHashHasKeys : 0) | (RSBag ? RSBasicHashHasCounts : 0);
    RSBasicHashCallbacks *cb = (RSBasicHashCallbacks *)&RSBagStandardCallbacks;
    RSBasicHashRef ht = RSBasicHashCreate(allocator, flags, cb);
    RSBasicHashSetSpecialBits(ht, 0x0303);
    if (0 < numValues) RSBasicHashSetCapacity(ht, numValues);
    for (RSIndex idx = 0; idx < numValues; idx++) {
        RSBasicHashAddValue(ht, (uintptr_t)klist[idx], (uintptr_t)vlist[idx]);
    }
    RSBasicHashSetSpecialBits(ht, 0x0000);
    RSBasicHashMakeImmutable(ht);
    *(uintptr_t *)ht = __RSISAForTypeID(typeID);
    __RSRuntimeSetInstanceTypeID(ht, typeID);
    //        if (__RSOASafe) __RSSetLastAllocationEventName(ht, "RSBag (immutable)");
    return (RSHashRef)ht;
}

#elif (RSBasicHashVersion == 2)
static RSBasicHashRef __RSBagCreateGeneric(RSAllocatorRef allocator, const RSHashKeyCallBacks *keyCallBacks, const RSHashValueCallBacks *valueCallBacks, BOOL useValueCB) {
    RSOptionFlags flags = RSBasicHashLinearHashing; // RSBasicHashExponentialHashing
    flags |= (RSDictionary ? RSBasicHashHasKeys : 0) | (RSBag ? RSBasicHashHasCounts : 0);
    
    if (RS_IS_COLLECTABLE_ALLOCATOR(allocator)) { // all this crap is just for figuring out two flags for GC in the way done historically; it probably simplifies down to three lines, but we let the compiler worry about that
        BOOL set_cb = false;
        BOOL std_cb = false;
        const_any_pointer_t (*key_retain)(RSAllocatorRef, const_any_pointer_t) = nil;
        void (*key_release)(RSAllocatorRef, const_any_pointer_t) = nil;
        const_any_pointer_t (*value_retain)(RSAllocatorRef, const_any_pointer_t) = nil;
        void (*value_release)(RSAllocatorRef, const_any_pointer_t) = nil;
        
        if ((nil == keyCallBacks || 0 == keyCallBacks->version) && (!useValueCB || nil == valueCallBacks || 0 == valueCallBacks->version)) {
            BOOL keyRetainNull = nil == keyCallBacks || nil == keyCallBacks->retain;
            BOOL keyReleaseNull = nil == keyCallBacks || nil == keyCallBacks->release;
            BOOL keyEquateNull = nil == keyCallBacks || nil == keyCallBacks->equal;
            BOOL keyHashNull = nil == keyCallBacks || nil == keyCallBacks->hash;
            BOOL keyDescribeNull = nil == keyCallBacks || nil == keyCallBacks->copyDescription;
            
            BOOL valueRetainNull = (useValueCB && (nil == valueCallBacks || nil == valueCallBacks->retain)) || (!useValueCB && keyRetainNull);
            BOOL valueReleaseNull = (useValueCB && (nil == valueCallBacks || nil == valueCallBacks->release)) || (!useValueCB && keyReleaseNull);
            BOOL valueEquateNull = (useValueCB && (nil == valueCallBacks || nil == valueCallBacks->equal)) || (!useValueCB && keyEquateNull);
            BOOL valueDescribeNull = (useValueCB && (nil == valueCallBacks || nil == valueCallBacks->copyDescription)) || (!useValueCB && keyDescribeNull);
            
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
                set_cb = true;
                if (!(keyRetainNull || keyReleaseNull || keyEquateNull || keyHashNull || keyDescribeNull || valueRetainNull || valueReleaseNull || valueEquateNull || valueDescribeNull)) {
                    std_cb = true;
                } else {
                    // just set these to tickle the GC Strong logic below in a way that mimics past practice
                    key_retain = keyCallBacks ? keyCallBacks->retain : nil;
                    key_release = keyCallBacks ? keyCallBacks->release : nil;
                    if (useValueCB) {
                        value_retain = valueCallBacks ? valueCallBacks->retain : nil;
                        value_release = valueCallBacks ? valueCallBacks->release : nil;
                    } else {
                        value_retain = key_retain;
                        value_release = key_release;
                    }
                }
            }
        }
        
        if (!set_cb) {
            key_retain = keyCallBacks ? keyCallBacks->retain : nil;
            key_release = keyCallBacks ? keyCallBacks->release : nil;
            if (useValueCB) {
                value_retain = valueCallBacks ? valueCallBacks->retain : nil;
                value_release = valueCallBacks ? valueCallBacks->release : nil;
            } else {
                value_retain = key_retain;
                value_release = key_release;
            }
        }
        
        if (std_cb || value_retain != nil || value_release != nil) {
            flags |= RSBasicHashStrongValues;
        }
        if (std_cb || key_retain != nil || key_release != nil) {
            flags |= RSBasicHashStrongKeys;
        }
    }
    
    
    RSBasicHashCallbacks callbacks;
    callbacks.retainKey = keyCallBacks ? (uintptr_t (*)(RSAllocatorRef, uintptr_t))keyCallBacks->retain : nil;
    callbacks.releaseKey = keyCallBacks ? (void (*)(RSAllocatorRef, uintptr_t))keyCallBacks->release : nil;
    callbacks.equateKeys = keyCallBacks ? (BOOL (*)(uintptr_t, uintptr_t))keyCallBacks->equal : nil;
    callbacks.hashKey = keyCallBacks ? (RSHashCode (*)(uintptr_t))keyCallBacks->hash : nil;
    callbacks.getIndirectKey = nil;
    callbacks.copyKeyDescription = keyCallBacks ? (RSStringRef (*)(uintptr_t))keyCallBacks->copyDescription : nil;
    callbacks.retainValue = useValueCB ? (valueCallBacks ? (uintptr_t (*)(RSAllocatorRef, uintptr_t))valueCallBacks->retain : nil) : (callbacks.retainKey);
    callbacks.releaseValue = useValueCB ? (valueCallBacks ? (void (*)(RSAllocatorRef, uintptr_t))valueCallBacks->release : nil) : (callbacks.releaseKey);
    callbacks.equateValues = useValueCB ? (valueCallBacks ? (BOOL (*)(uintptr_t, uintptr_t))valueCallBacks->equal : nil) : (callbacks.equateKeys);
    callbacks.copyValueDescription = useValueCB ? (valueCallBacks ? (RSStringRef (*)(uintptr_t))valueCallBacks->copyDescription : nil) : (callbacks.copyKeyDescription);
    
    RSBasicHashRef ht = RSBasicHashCreate(allocator, flags, &callbacks);
    return ht;
}

#if RSDictionary
RSPrivate RSHashRef __RSBagCreateTransfer(RSAllocatorRef allocator, const_any_pointer_t *klist, const_any_pointer_t *vlist, RSIndex numValues)
#endif
#if RSSet || RSBag
RSPrivate RSHashRef __RSBagCreateTransfer(RSAllocatorRef allocator, const_any_pointer_t *klist, RSIndex numValues)

#endif
{
#if RSSet || RSBag
    const_any_pointer_t *vlist = klist;
#endif
    RSTypeID typeID = RSBagGetTypeID();
    RSAssert2(0 <= numValues, __RSLogAssertion, "%s(): numValues (%ld) cannot be less than zero", __PRETTY_FUNCTION__, numValues);
    RSOptionFlags flags = RSBasicHashLinearHashing; // RSBasicHashExponentialHashing
    flags |= (RSDictionary ? RSBasicHashHasKeys : 0) | (RSBag ? RSBasicHashHasCounts : 0);
    
    RSBasicHashCallbacks callbacks;
    callbacks.retainKey = (uintptr_t (*)(RSAllocatorRef, uintptr_t))RSTypeBagKeyCallBacks.retain;
    callbacks.releaseKey = (void (*)(RSAllocatorRef, uintptr_t))RSTypeBagKeyCallBacks.release;
    callbacks.equateKeys = (BOOL (*)(uintptr_t, uintptr_t))RSTypeBagKeyCallBacks.equal;
    callbacks.hashKey = (RSHashCode (*)(uintptr_t))RSTypeBagKeyCallBacks.hash;
    callbacks.getIndirectKey = nil;
    callbacks.copyKeyDescription = (RSStringRef (*)(uintptr_t))RSTypeBagKeyCallBacks.copyDescription;
    callbacks.retainValue = RSDictionary ? (uintptr_t (*)(RSAllocatorRef, uintptr_t))RSTypeBagValueCallBacks.retain : callbacks.retainKey;
    callbacks.releaseValue = RSDictionary ? (void (*)(RSAllocatorRef, uintptr_t))RSTypeBagValueCallBacks.release : callbacks.releaseKey;
    callbacks.equateValues = RSDictionary ? (BOOL (*)(uintptr_t, uintptr_t))RSTypeBagValueCallBacks.equal : callbacks.equateKeys;
    callbacks.copyValueDescription = RSDictionary ? (RSStringRef (*)(uintptr_t))RSTypeBagValueCallBacks.copyDescription : callbacks.copyKeyDescription;
    
    RSBasicHashRef ht = RSBasicHashCreate(allocator, flags, &callbacks);
    RSBasicHashSuppressRC(ht);
    if (0 < numValues) RSBasicHashSetCapacity(ht, numValues);
    for (RSIndex idx = 0; idx < numValues; idx++) {
        RSBasicHashAddValue(ht, (uintptr_t)klist[idx], (uintptr_t)vlist[idx]);
    }
    RSBasicHashUnsuppressRC(ht);
    RSBasicHashMakeImmutable(ht);
    __RSRuntimeSetInstanceTypeID(ht, typeID);
//    if (__RSOASafe) __RSSetLastAllocationEventName(ht, "RSBag (immutable)");
    return (RSHashRef)ht;
}
#endif

#if RSDictionary
RSHashRef RSBagCreate(RSAllocatorRef allocator, const_any_pointer_t *klist, const_any_pointer_t *vlist, RSIndex numValues, const RSBagKeyCallBacks *keyCallBacks, const RSBagValueCallBacks *valueCallBacks)
#endif
#if RSSet || RSBag
RSHashRef RSBagCreate(RSAllocatorRef allocator, const_any_pointer_t *klist, RSIndex numValues, const RSBagKeyCallBacks *keyCallBacks)
#endif
{
#if RSSet || RSBag
    const_any_pointer_t *vlist = klist;
    const RSBagValueCallBacks *valueCallBacks = 0;
#endif
    RSTypeID typeID = RSBagGetTypeID();
    RSAssert2(0 <= numValues, __RSLogAssertion, "%s(): numValues (%ld) cannot be less than zero", __PRETTY_FUNCTION__, numValues);
    RSBasicHashRef ht = __RSBagCreateGeneric(allocator, keyCallBacks, valueCallBacks, RSDictionary);
    if (!ht) return nil;
    if (0 < numValues) RSBasicHashSetCapacity(ht, numValues);
    for (RSIndex idx = 0; idx < numValues; idx++) {
        RSBasicHashAddValue(ht, (uintptr_t)klist[idx], (uintptr_t)vlist[idx]);
    }
    RSBasicHashMakeImmutable(ht);
    *(uintptr_t *)ht = __RSISAForTypeID(typeID);
    __RSRuntimeSetInstanceTypeID(ht, typeID);
    //    if (__RSOASafe) __RSSetLastAllocationEventName(ht, "RSBag (immutable)");
    return (RSHashRef)ht;
}

#if RSDictionary
RSMutableHashRef RSBagCreateMutable(RSAllocatorRef allocator, RSIndex capacity, const RSBagKeyCallBacks *keyCallBacks, const RSBagValueCallBacks *valueCallBacks)
#endif
#if RSSet || RSBag
RSMutableHashRef RSBagCreateMutable(RSAllocatorRef allocator, RSIndex capacity, const RSBagKeyCallBacks *keyCallBacks)
#endif
{
#if RSSet || RSBag
    const RSBagValueCallBacks *valueCallBacks = 0;
#endif
    
    RSTypeID typeID = RSBagGetTypeID();
    RSAssert2(0 <= capacity, __RSLogAssertion, "%s(): capacity (%ld) cannot be less than zero", __PRETTY_FUNCTION__, capacity);
    RSBasicHashRef ht = __RSBagCreateGeneric(allocator, keyCallBacks, valueCallBacks, RSDictionary);
    if (!ht) return nil;
    *(uintptr_t *)ht = __RSISAForTypeID(typeID);
    __RSRuntimeSetInstanceTypeID(ht, typeID);
    //        if (__RSOASafe) __RSSetLastAllocationEventName(ht, "RSBag (mutable)");
    return (RSMutableHashRef)ht;
}

RSHashRef RSBagCreateCopy(RSAllocatorRef allocator, RSHashRef other) {
    RSTypeID typeID = RSBagGetTypeID();
    RSAssert1(other, __RSLogAssertion, "%s(): other RSBag cannot be nil", __PRETTY_FUNCTION__);
    __RSGenericValidInstance(other, typeID);
    RSBasicHashRef ht = nil;
    if (RS_IS_OBJC(typeID, other)) {
        RSIndex numValues = RSBagGetCount(other);
        const_any_pointer_t vbuffer[256], kbuffer[256];
        const_any_pointer_t *vlist = (numValues <= 256) ? vbuffer : (const_any_pointer_t *)RSAllocatorAllocate(RSAllocatorSystemDefault, numValues * sizeof(const_any_pointer_t));
#if RSSet || RSBag
        const_any_pointer_t *klist = vlist;
        RSBagGetValues(other, vlist);
#endif
#if RSDictionary
        const_any_pointer_t *klist = (numValues <= 256) ? kbuffer : (const_any_pointer_t *)RSAllocatorAllocate(RSAllocatorSystemDefault, numValues * sizeof(const_any_pointer_t), 0);
        RSDictionaryGetKeysAndValues(other, klist, vlist);
#endif
        ht = __RSBagCreateGeneric(allocator, & RSTypeBagKeyCallBacks, RSDictionary ? & RSTypeBagValueCallBacks : nil, RSDictionary);
        if (ht && 0 < numValues) RSBasicHashSetCapacity(ht, numValues);
        for (RSIndex idx = 0; ht && idx < numValues; idx++) {
            RSBasicHashAddValue(ht, (uintptr_t)klist[idx], (uintptr_t)vlist[idx]);
        }
        if (klist != kbuffer && klist != vlist) RSAllocatorDeallocate(RSAllocatorSystemDefault, klist);
        if (vlist != vbuffer) RSAllocatorDeallocate(RSAllocatorSystemDefault, vlist);
    } else {
        ht = RSBasicHashCreateCopy(allocator, (RSBasicHashRef)other);
    }
    if (!ht) return nil;
    RSBasicHashMakeImmutable(ht);
    *(uintptr_t *)ht = __RSISAForTypeID(typeID);
    __RSRuntimeSetInstanceTypeID(ht, typeID);
    //        if (__RSOASafe) __RSSetLastAllocationEventName(ht, "RSBag (immutable)");
    return (RSHashRef)ht;
}

RSMutableHashRef RSBagCreateMutableCopy(RSAllocatorRef allocator, RSIndex capacity, RSHashRef other) {
    RSTypeID typeID = RSBagGetTypeID();
    RSAssert1(other, __RSLogAssertion, "%s(): other RSBag cannot be nil", __PRETTY_FUNCTION__);
    __RSGenericValidInstance(other, typeID);
    RSAssert2(0 <= capacity, __RSLogAssertion, "%s(): capacity (%ld) cannot be less than zero", __PRETTY_FUNCTION__, capacity);
    RSBasicHashRef ht = nil;
    if (RS_IS_OBJC(typeID, other)) {
        RSIndex numValues = RSBagGetCount(other);
        const_any_pointer_t vbuffer[256], kbuffer[256];
        const_any_pointer_t *vlist = (numValues <= 256) ? vbuffer : (const_any_pointer_t *)RSAllocatorAllocate(RSAllocatorSystemDefault, numValues * sizeof(const_any_pointer_t));
#if RSSet || RSBag
        const_any_pointer_t *klist = vlist;
        RSBagGetValues(other, vlist);
#endif
#if RSDictionary
        const_any_pointer_t *klist = (numValues <= 256) ? kbuffer : (const_any_pointer_t *)RSAllocatorAllocate(RSAllocatorSystemDefault, numValues * sizeof(const_any_pointer_t), 0);
        RSDictionaryGetKeysAndValues(other, klist, vlist);
#endif
        ht = __RSBagCreateGeneric(allocator, & RSTypeBagKeyCallBacks, RSDictionary ? & RSTypeBagValueCallBacks : nil, RSDictionary);
        if (ht && 0 < numValues) RSBasicHashSetCapacity(ht, numValues);
        for (RSIndex idx = 0; ht && idx < numValues; idx++) {
            RSBasicHashAddValue(ht, (uintptr_t)klist[idx], (uintptr_t)vlist[idx]);
        }
        if (klist != kbuffer && klist != vlist) RSAllocatorDeallocate(RSAllocatorSystemDefault, klist);
        if (vlist != vbuffer) RSAllocatorDeallocate(RSAllocatorSystemDefault, vlist);
    } else {
        ht = RSBasicHashCreateCopy(allocator, (RSBasicHashRef)other);
    }
    if (!ht) return nil;
    *(uintptr_t *)ht = __RSISAForTypeID(typeID);
    __RSRuntimeSetInstanceTypeID(ht, typeID);
    //        if (__RSOASafe) __RSSetLastAllocationEventName(ht, "RSBag (mutable)");
    return (RSMutableHashRef)ht;
}

RSIndex RSBagGetCount(RSHashRef hc) {
    if (RSDictionary) RS_OBJC_FUNCDISPATCHV(__RSBagTypeID, RSIndex, (NSDictionary *)hc, count);
    if (RSSet) RS_OBJC_FUNCDISPATCHV(__RSBagTypeID, RSIndex, (NSSet *)hc, count);
    __RSGenericValidInstance(hc, __RSBagTypeID);
    return RSBasicHashGetCount((RSBasicHashRef)hc);
}

#if RSDictionary
RSIndex RSBagGetCountOfKey(RSHashRef hc, const_any_pointer_t key)
#endif
#if RSSet || RSBag
RSIndex RSBagGetCountOfValue(RSHashRef hc, const_any_pointer_t key)
#endif
{
    if (RSDictionary) RS_OBJC_FUNCDISPATCHV(__RSBagTypeID, RSIndex, (NSDictionary *)hc, countForKey:(id)key);
    if (RSSet) RS_OBJC_FUNCDISPATCHV(__RSBagTypeID, RSIndex, (NSSet *)hc, countForObject:(id)key);
    __RSGenericValidInstance(hc, __RSBagTypeID);
    return RSBasicHashGetCountOfKey((RSBasicHashRef)hc, (uintptr_t)key);
}

#if RSDictionary
BOOL RSBagContainsKey(RSHashRef hc, const_any_pointer_t key)
#endif
#if RSSet || RSBag
BOOL RSBagContainsValue(RSHashRef hc, const_any_pointer_t key)
#endif
{
    if (RSDictionary) RS_OBJC_FUNCDISPATCHV(__RSBagTypeID, char, (NSDictionary *)hc, containsKey:(id)key);
    if (RSSet) RS_OBJC_FUNCDISPATCHV(__RSBagTypeID, char, (NSSet *)hc, containsObject:(id)key);
    __RSGenericValidInstance(hc, __RSBagTypeID);
    return (0 < RSBasicHashGetCountOfKey((RSBasicHashRef)hc, (uintptr_t)key));
}

const_any_pointer_t RSBagGetValue(RSHashRef hc, const_any_pointer_t key) {
    if (RSDictionary) RS_OBJC_FUNCDISPATCHV(__RSBagTypeID, const_any_pointer_t, (NSDictionary *)hc, objectForKey:(id)key);
    if (RSSet) RS_OBJC_FUNCDISPATCHV(__RSBagTypeID, const_any_pointer_t, (NSSet *)hc, member:(id)key);
    __RSGenericValidInstance(hc, __RSBagTypeID);
    RSBasicHashBucket bkt = RSBasicHashFindBucket((RSBasicHashRef)hc, (uintptr_t)key);
    return (0 < bkt.count ? (const_any_pointer_t)bkt.weak_value : 0);
}

BOOL RSBagGetValueIfPresent(RSHashRef hc, const_any_pointer_t key, const_any_pointer_t *value) {
    if (RSDictionary) RS_OBJC_FUNCDISPATCHV(__RSBagTypeID, BOOL, (NSDictionary *)hc, __getValue:(id *)value forKey:(id)key);
    if (RSSet) RS_OBJC_FUNCDISPATCHV(__RSBagTypeID, BOOL, (NSSet *)hc, __getValue:(id *)value forObj:(id)key);
    __RSGenericValidInstance(hc, __RSBagTypeID);
    RSBasicHashBucket bkt = RSBasicHashFindBucket((RSBasicHashRef)hc, (uintptr_t)key);
    if (0 < bkt.count) {
        if (value) {
            if (RSUseCollectableAllocator && (RSBasicHashGetFlags((RSBasicHashRef)hc) & RSBasicHashStrongValues)) {
                __RSAssignWithWriteBarrier((void **)value, (void *)bkt.weak_value);
            } else {
                *value = (const_any_pointer_t)bkt.weak_value;
            }
        }
        return true;
    }
    return false;
}

#if RSDictionary
RSIndex RSDictionaryGetCountOfValue(RSHashRef hc, const_any_pointer_t value) {
    RS_OBJC_FUNCDISPATCHV(__RSBagTypeID, RSIndex, (NSDictionary *)hc, countForObject:(id)value);
    __RSGenericValidInstance(hc, __RSBagTypeID);
    return RSBasicHashGetCountOfValue((RSBasicHashRef)hc, (uintptr_t)value);
}

BOOL RSDictionaryContainsValue(RSHashRef hc, const_any_pointer_t value) {
    RS_OBJC_FUNCDISPATCHV(__RSBagTypeID, char, (NSDictionary *)hc, containsObject:(id)value);
    __RSGenericValidInstance(hc, __RSBagTypeID);
    return (0 < RSBasicHashGetCountOfValue((RSBasicHashRef)hc, (uintptr_t)value));
}

RS_EXPORT BOOL RSDictionaryGetKeyIfPresent(RSHashRef hc, const_any_pointer_t key, const_any_pointer_t *actualkey) {
    __RSGenericValidInstance(hc, __RSBagTypeID);
    RSBasicHashBucket bkt = RSBasicHashFindBucket((RSBasicHashRef)hc, (uintptr_t)key);
    if (0 < bkt.count) {
        if (actualkey) {
            if (RSUseCollectableAllocator && (RSBasicHashGetFlags((RSBasicHashRef)hc) & RSBasicHashStrongKeys)) {
                __RSAssignWithWriteBarrier((void **)actualkey, (void *)bkt.weak_key);
            } else {
                *actualkey = (const_any_pointer_t)bkt.weak_key;
            }
        }
        return true;
    }
    return false;
}
#endif

#if RSDictionary
void RSBagGetKeysAndValues(RSHashRef hc, const_any_pointer_t *keybuf, const_any_pointer_t *valuebuf)
#endif
#if RSSet || RSBag
void RSBagGetValues(RSHashRef hc, const_any_pointer_t *keybuf)
#endif
{
#if RSSet || RSBag
    const_any_pointer_t *valuebuf = 0;
#endif
    if (RSDictionary) RS_OBJC_FUNCDISPATCHV(__RSBagTypeID, void, (NSDictionary *)hc, getObjects:(id *)valuebuf andKeys:(id *)keybuf);
    if (RSSet) RS_OBJC_FUNCDISPATCHV(__RSBagTypeID, void, (NSSet *)hc, getObjects:(id *)keybuf);
    __RSGenericValidInstance(hc, __RSBagTypeID);
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
            return (BOOL)true;
        });
    } else {
        RSBasicHashGetElements((RSBasicHashRef)hc, RSBagGetCount(hc), (uintptr_t *)valuebuf, (uintptr_t *)keybuf);
    }
}

void RSBagApplyFunction(RSHashRef hc, RSBagApplierFunction applier, any_pointer_t context) {
    FAULT_CALLBACK((void **)&(applier));
    if (RSDictionary) RS_OBJC_FUNCDISPATCHV(__RSBagTypeID, void, (NSDictionary *)hc, __apply:(void (*)(const void *, const void *, void *))applier context:(void *)context);
    if (RSSet) RS_OBJC_FUNCDISPATCHV(__RSBagTypeID, void, (NSSet *)hc, __applyValues:(void (*)(const void *, void *))applier context:(void *)context);
    __RSGenericValidInstance(hc, __RSBagTypeID);
    RSBasicHashApply((RSBasicHashRef)hc, ^(RSBasicHashBucket bkt) {
#if RSDictionary
        INVOKE_CALLBACK3(applier, (const_any_pointer_t)bkt.weak_key, (const_any_pointer_t)bkt.weak_value, context);
#endif
#if RSSet
        INVOKE_CALLBACK2(applier, (const_any_pointer_t)bkt.weak_value, context);
#endif
#if RSBag
        for (RSIndex cnt = bkt.count; cnt--;) {
            INVOKE_CALLBACK2(applier, (const_any_pointer_t)bkt.weak_value, context);
        }
#endif
        return (BOOL)true;
    });
}

#if RS_BLOCKS_AVAILABLE
RSExport void RSBagApplyBlock(RSHashRef hc, void (^block)(const void* value, BOOL *stop)) {
    FAULT_CALLBACK((void **)&(applier));
    if (RSDictionary) RS_OBJC_FUNCDISPATCHV(__RSBagTypeID, void, (NSDictionary *)hc, __apply:(void (*)(const void *, const void *, void *))applier context:(void *)context);
    if (RSSet) RS_OBJC_FUNCDISPATCHV(__RSBagTypeID, void, (NSSet *)hc, __applyValues:(void (*)(const void *, void *))applier context:(void *)context);
    __RSGenericValidInstance(hc, __RSBagTypeID);
    RSBasicHashApplyBlock((RSBasicHashRef)hc, ^BOOL(RSBasicHashBucket bkt, BOOL *stop) {
        INVOKE_CALLBACK2(block, (const_any_pointer_t)bkt.weak_value, stop);
        return (BOOL)true;
    });
}
#endif

// This function is for Foundation's benefit; no one else should use it.
RS_EXPORT unsigned long _RSBagFastEnumeration(RSHashRef hc, struct __RSFastEnumerationStateEquivalent2 *state, void *stackbuffer, unsigned long count) {
    if (RS_IS_OBJC(__RSBagTypeID, hc)) return 0;
    __RSGenericValidInstance(hc, __RSBagTypeID);
    return __RSBasicHashFastEnumeration((RSBasicHashRef)hc, (struct __RSFastEnumerationStateEquivalent2 *)state, stackbuffer, count);
}

// This function is for Foundation's benefit; no one else should use it.
RS_EXPORT BOOL _RSBagIsMutable(RSHashRef hc) {
    if (RS_IS_OBJC(__RSBagTypeID, hc)) return false;
    __RSGenericValidInstance(hc, __RSBagTypeID);
    return RSBasicHashIsMutable((RSBasicHashRef)hc);
}

// This function is for Foundation's benefit; no one else should use it.
RS_EXPORT void _RSBagSetCapacity(RSMutableHashRef hc, RSIndex cap) {
    if (RS_IS_OBJC(__RSBagTypeID, hc)) return;
    __RSGenericValidInstance(hc, __RSBagTypeID);
    RSAssert2(RSBasicHashIsMutable((RSBasicHashRef)hc), __RSLogAssertion, "%s(): immutable collection %p passed to mutating operation", __PRETTY_FUNCTION__, hc);
    RSAssert3(RSBagGetCount(hc) <= cap, __RSLogAssertion, "%s(): desired capacity (%ld) is less than count (%ld)", __PRETTY_FUNCTION__, cap, RSBagGetCount(hc));
    RSBasicHashSetCapacity((RSBasicHashRef)hc, cap);
}

RSInline RSIndex __RSBagGetKVOBit(RSHashRef hc) {
    return __RSBitfieldGetValue(RSRuntimeClassBaseFiled(hc), 0, 0);
}

RSInline void __RSBagSetKVOBit(RSHashRef hc, RSIndex bit) {
    __RSBitfieldSetValue(RSRuntimeClassBaseFiled(hc), 0, 0, ((uintptr_t)bit & 0x1));
}

// This function is for Foundation's benefit; no one else should use it.
RS_EXPORT RSIndex _RSBagGetKVOBit(RSHashRef hc) {
    return __RSBagGetKVOBit(hc);
}

// This function is for Foundation's benefit; no one else should use it.
RS_EXPORT void _RSBagSetKVOBit(RSHashRef hc, RSIndex bit) {
    __RSBagSetKVOBit(hc, bit);
}


#if !defined(RS_OBJC_KVO_WILLCHANGE)
#define RS_OBJC_KVO_WILLCHANGE(obj, key)
#define RS_OBJC_KVO_DIDCHANGE(obj, key)
#define RS_OBJC_KVO_WILLCHANGEALL(obj)
#define RS_OBJC_KVO_DIDCHANGEALL(obj)
#endif

#if RSDictionary
void RSBagAddValue(RSMutableHashRef hc, const_any_pointer_t key, const_any_pointer_t value)
#endif
#if RSSet || RSBag
void RSBagAddValue(RSMutableHashRef hc, const_any_pointer_t key)
#endif
{
#if RSSet || RSBag
    const_any_pointer_t value = key;
#endif
    if (RSDictionary) RS_OBJC_FUNCDISPATCHV(__RSBagTypeID, void, (NSMutableDictionary *)hc, __addObject:(id)value forKey:(id)key);
    if (RSSet) RS_OBJC_FUNCDISPATCHV(__RSBagTypeID, void, (NSMutableSet *)hc, addObject:(id)key);
    __RSGenericValidInstance(hc, __RSBagTypeID);
    RSAssert2(RSBasicHashIsMutable((RSBasicHashRef)hc), __RSLogAssertion, "%s(): immutable collection %p passed to mutating operation", __PRETTY_FUNCTION__, hc);
    if (!RSBasicHashIsMutable((RSBasicHashRef)hc)) {
        RSLog(3, RSSTR("%s(): immutable collection %p given to mutating function"), __PRETTY_FUNCTION__, hc);
    }
    RS_OBJC_KVO_WILLCHANGE(hc, key);
    RSBasicHashAddValue((RSBasicHashRef)hc, (uintptr_t)key, (uintptr_t)value);
    RS_OBJC_KVO_DIDCHANGE(hc, key);
}

#if RSDictionary
void RSBagReplaceValue(RSMutableHashRef hc, const_any_pointer_t key, const_any_pointer_t value)
#endif
#if RSSet || RSBag
void RSBagReplaceValue(RSMutableHashRef hc, const_any_pointer_t key)
#endif
{
#if RSSet || RSBag
    const_any_pointer_t value = key;
#endif
    if (RSDictionary) RS_OBJC_FUNCDISPATCHV(__RSBagTypeID, void, (NSMutableDictionary *)hc, replaceObject:(id)value forKey:(id)key);
    if (RSSet) RS_OBJC_FUNCDISPATCHV(__RSBagTypeID, void, (NSMutableSet *)hc, replaceObject:(id)key);
    __RSGenericValidInstance(hc, __RSBagTypeID);
    RSAssert2(RSBasicHashIsMutable((RSBasicHashRef)hc), __RSLogAssertion, "%s(): immutable collection %p passed to mutating operation", __PRETTY_FUNCTION__, hc);
    if (!RSBasicHashIsMutable((RSBasicHashRef)hc)) {
        RSLog(3, RSSTR("%s(): immutable collection %p given to mutating function"), __PRETTY_FUNCTION__, hc);
    }
    RS_OBJC_KVO_WILLCHANGE(hc, key);
    RSBasicHashReplaceValue((RSBasicHashRef)hc, (uintptr_t)key, (uintptr_t)value);
    RS_OBJC_KVO_DIDCHANGE(hc, key);
}

#if RSDictionary
void RSBagSetValue(RSMutableHashRef hc, const_any_pointer_t key, const_any_pointer_t value)
#endif
#if RSSet || RSBag
void RSBagSetValue(RSMutableHashRef hc, const_any_pointer_t key)
#endif
{
#if RSSet || RSBag
    const_any_pointer_t value = key;
#endif
    if (RSDictionary) RS_OBJC_FUNCDISPATCHV(__RSBagTypeID, void, (NSMutableDictionary *)hc, setObject:(id)value forKey:(id)key);
    if (RSSet) RS_OBJC_FUNCDISPATCHV(__RSBagTypeID, void, (NSMutableSet *)hc, setObject:(id)key);
    __RSGenericValidInstance(hc, __RSBagTypeID);
    RSAssert2(RSBasicHashIsMutable((RSBasicHashRef)hc), __RSLogAssertion, "%s(): immutable collection %p passed to mutating operation", __PRETTY_FUNCTION__, hc);
    if (!RSBasicHashIsMutable((RSBasicHashRef)hc)) {
        RSLog(3, RSSTR("%s(): immutable collection %p given to mutating function"), __PRETTY_FUNCTION__, hc);
    }
    RS_OBJC_KVO_WILLCHANGE(hc, key);
    //#warning this for a dictionary used to not replace the key
    RSBasicHashSetValue((RSBasicHashRef)hc, (uintptr_t)key, (uintptr_t)value);
    RS_OBJC_KVO_DIDCHANGE(hc, key);
}

void RSBagRemoveValue(RSMutableHashRef hc, const_any_pointer_t key) {
    if (RSDictionary) RS_OBJC_FUNCDISPATCHV(__RSBagTypeID, void, (NSMutableDictionary *)hc, removeObjectForKey:(id)key);
    if (RSSet) RS_OBJC_FUNCDISPATCHV(__RSBagTypeID, void, (NSMutableSet *)hc, removeObject:(id)key);
    __RSGenericValidInstance(hc, __RSBagTypeID);
    RSAssert2(RSBasicHashIsMutable((RSBasicHashRef)hc), __RSLogAssertion, "%s(): immutable collection %p passed to mutating operation", __PRETTY_FUNCTION__, hc);
    if (!RSBasicHashIsMutable((RSBasicHashRef)hc)) {
        RSLog(3, RSSTR("%s(): immutable collection %p given to mutating function"), __PRETTY_FUNCTION__, hc);
    }
    RS_OBJC_KVO_WILLCHANGE(hc, key);
    RSBasicHashRemoveValue((RSBasicHashRef)hc, (uintptr_t)key);
    RS_OBJC_KVO_DIDCHANGE(hc, key);
}

void RSBagRemoveAllValues(RSMutableHashRef hc) {
    if (RSDictionary) RS_OBJC_FUNCDISPATCHV(__RSBagTypeID, void, (NSMutableDictionary *)hc, removeAllObjects);
    if (RSSet) RS_OBJC_FUNCDISPATCHV(__RSBagTypeID, void, (NSMutableSet *)hc, removeAllObjects);
    __RSGenericValidInstance(hc, __RSBagTypeID);
    RSAssert2(RSBasicHashIsMutable((RSBasicHashRef)hc), __RSLogAssertion, "%s(): immutable collection %p passed to mutating operation", __PRETTY_FUNCTION__, hc);
    if (!RSBasicHashIsMutable((RSBasicHashRef)hc)) {
        RSLog(3, RSSTR("%s(): immutable collection %p given to mutating function"), __PRETTY_FUNCTION__, hc);
    }
    RS_OBJC_KVO_WILLCHANGEALL(hc);
    RSBasicHashRemoveAllValues((RSBasicHashRef)hc);
    RS_OBJC_KVO_DIDCHANGEALL(hc);
}

#undef RS_OBJC_KVO_WILLCHANGE
#undef RS_OBJC_KVO_DIDCHANGE
#undef RS_OBJC_KVO_WILLCHANGEALL
#undef RS_OBJC_KVO_DIDCHANGEALL

