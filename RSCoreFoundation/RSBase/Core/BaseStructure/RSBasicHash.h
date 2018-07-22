//
//  RSBasicHash.h
//  RSCoreFoundation
//
//  Created by Apple Inc.
//  Changed by RetVal on 12/9/12.
//  Copyright (c) 2012 RetVal. All rights reserved.
//

#ifndef RSCoreFoundation_RSBasicHash_h
#define RSCoreFoundation_RSBasicHash_h
RS_EXTERN_C_BEGIN

#ifndef RSBasicHashVersion
#define RSBasicHashVersion 2
#endif

struct __RSFastEnumerationStateEquivalent2 {
    unsigned long state;
    //  Arbitrary state information used by the iterator. Typically this is set to 0 at the beginning of the iteration.
    unsigned long *itemsPtr;
    //  A C array of objects.
    unsigned long *mutationsPtr;
    //  Arbitrary state information used to detect whether the collection has been mutated.
    unsigned long extra[5];
    //  A C array that you can use to hold returned values.
};


enum {
    __RSBasicHashLinearHashingValue = 1,
    __RSBasicHashDoubleHashingValue = 2,
    __RSBasicHashExponentialHashingValue = 3,
};

enum {
    RSBasicHashHasKeys = (1UL << 0),
    RSBasicHashHasCounts = (1UL << 1),
    RSBasicHashHasHashCache = (1UL << 2),
    
    RSBasicHashIntegerValues = (1UL << 6),
    RSBasicHashIntegerKeys = (1UL << 7),
    
    RSBasicHashStrongValues = (1UL << 8),
    RSBasicHashStrongKeys = (1UL << 9),
    
    RSBasicHashWeakValues = (1UL << 10),
    RSBasicHashWeakKeys = (1UL << 11),
    
    RSBasicHashIndirectKeys = (1UL << 12),
    
    RSBasicHashLinearHashing = (__RSBasicHashLinearHashingValue << 13), // bits 13-14
    RSBasicHashDoubleHashing = (__RSBasicHashDoubleHashingValue << 13),
    RSBasicHashExponentialHashing = (__RSBasicHashExponentialHashingValue << 13),
    
    RSBasicHashAggressiveGrowth = (1UL << 15),
    
    RSBasicHashCompactableValues = (1UL << 16),
    RSBasicHashCompactableKeys = (1UL << 17),
};

// Note that for a hash table without keys, the value is treated as the key,
// and the value should be passed in as the key for operations which take a key.

typedef struct {
    RSIndex idx;
    uintptr_t weak_key;
    uintptr_t weak_value;
    uintptr_t count;
} RSBasicHashBucket;

typedef struct __RSBasicHash *RSBasicHashRef;
typedef const struct __RSBasicHash *RSConstBasicHashRef;

RSInline BOOL RSBasicHashIsMutable(RSConstBasicHashRef ht)
{
    return ((1<<6) == (((RSRuntimeBase*)ht)->_rsinfo._rsinfo1 & (1<<6))) ? NO : YES;//__RSBitfieldGetValue(((RSRuntimeBase *)ht)->_cfinfo[RS_INFO_BITS], 6, 6) ? NO : YES;
}

RSInline void RSBasicHashMakeImmutable(RSBasicHashRef ht)
{
    ((RSRuntimeBase*)ht)->_rsinfo._rsinfo1 |= (1 << 6);
    //__RSBitfieldSetValue(((RSRuntimeBase *)ht)->_cfinfo[RS_INFO_BITS], 6, 6, 1);
}

typedef struct __RSBasicHashCallbacks RSBasicHashCallbacks;

struct __RSBasicHashCallbacks {
#if !defined(RSBasicHashVersion) || RSBasicHashVersion == 1
    RSBasicHashCallbacks *(*copyCallbacks)(RSConstBasicHashRef ht, RSAllocatorRef allocator, RSBasicHashCallbacks *cb);	// Return new value
    void (*freeCallbacks)(RSConstBasicHashRef ht, RSAllocatorRef allocator, RSBasicHashCallbacks *cb);
    uintptr_t (*retainValue)(RSConstBasicHashRef ht, uintptr_t stack_value);	// Return 2nd arg or new value
    uintptr_t (*retainKey)(RSConstBasicHashRef ht, uintptr_t stack_key);	// Return 2nd arg or new key
    void (*releaseValue)(RSConstBasicHashRef ht, uintptr_t stack_value);
    void (*releaseKey)(RSConstBasicHashRef ht, uintptr_t stack_key);
    BOOL (*equateValues)(RSConstBasicHashRef ht, uintptr_t coll_value1, uintptr_t stack_value2); // 2nd arg is in-collection value, 3rd arg is probe parameter OR in-collection value for a second collection
    BOOL (*equateKeys)(RSConstBasicHashRef ht, uintptr_t coll_key1, uintptr_t stack_key2); // 2nd arg is in-collection key, 3rd arg is probe parameter
    uintptr_t (*hashKey)(RSConstBasicHashRef ht, uintptr_t stack_key);
    uintptr_t (*getIndirectKey)(RSConstBasicHashRef ht, uintptr_t coll_value);	// Return key; 2nd arg is in-collection value
    RSStringRef (*copyValueDescription)(RSConstBasicHashRef ht, uintptr_t stack_value);
    RSStringRef (*copyKeyDescription)(RSConstBasicHashRef ht, uintptr_t stack_key);
    uintptr_t context[0]; // variable size; any pointers in here must remain valid as long as the RSBasicHash
#elif RSBasicHashVersion == 2
    uintptr_t (*retainValue)(RSAllocatorRef alloc, uintptr_t stack_value);	// Return 2nd arg or new value
    uintptr_t (*retainKey)(RSAllocatorRef alloc, uintptr_t stack_key);	// Return 2nd arg or new key
    void (*releaseValue)(RSAllocatorRef alloc, uintptr_t stack_value);
    void (*releaseKey)(RSAllocatorRef alloc, uintptr_t stack_key);
    BOOL (*equateValues)(uintptr_t coll_value1, uintptr_t stack_value2); // 1st arg is in-collection value, 2nd arg is probe parameter OR in-collection value for a second collection
    BOOL (*equateKeys)(uintptr_t coll_key1, uintptr_t stack_key2); // 1st arg is in-collection key, 2nd arg is probe parameter
    RSHashCode (*hashKey)(uintptr_t stack_key);
    uintptr_t (*getIndirectKey)(uintptr_t coll_value);	// Return key; 1st arg is in-collection value
    RSStringRef (*copyValueDescription)(uintptr_t stack_value);
    RSStringRef (*copyKeyDescription)(uintptr_t stack_key);
#endif
};
BOOL RSBasicHashHasStrongValues(RSConstBasicHashRef ht) RS_AVAILABLE(0_0);
BOOL RSBasicHashHasStrongKeys(RSConstBasicHashRef ht) RS_AVAILABLE(0_0);

#if !defined(RSBasicHashVersion) || (RSBasicHashVersion == 1)
uint16_t RSBasicHashGetSpecialBits(RSConstBasicHashRef ht) RS_AVAILABLE(0_0);
uint16_t RSBasicHashSetSpecialBits(RSBasicHashRef ht, uint16_t bits) RS_AVAILABLE(0_0);
const RSBasicHashCallbacks *RSBasicHashGetCallbacks(RSConstBasicHashRef ht) RS_AVAILABLE(0_0);
#elif (RSBasicHashVersion == 2)
void RSBasicHashSuppressRC(RSBasicHashRef ht) RS_AVAILABLE(0_3);
void RSBasicHashUnsuppressRC(RSBasicHashRef ht) RS_AVAILABLE(0_3);
#endif

RSOptionFlags RSBasicHashGetFlags(RSConstBasicHashRef ht) RS_AVAILABLE(0_0);
RSIndex RSBasicHashGetNumBuckets(RSConstBasicHashRef ht) RS_AVAILABLE(0_0);
RSIndex RSBasicHashGetCapacity(RSConstBasicHashRef ht) RS_AVAILABLE(0_0);
void RSBasicHashSetCapacity(RSBasicHashRef ht, RSIndex capacity) RS_AVAILABLE(0_0);

RSIndex RSBasicHashGetCount(RSConstBasicHashRef ht) RS_AVAILABLE(0_0);
RSBasicHashBucket RSBasicHashGetBucket(RSConstBasicHashRef ht, RSIndex idx) RS_AVAILABLE(0_0);
RSBasicHashBucket RSBasicHashFindBucket(RSConstBasicHashRef ht, uintptr_t stack_key) RS_AVAILABLE(0_0);
RSIndex RSBasicHashGetCountOfKey(RSConstBasicHashRef ht, uintptr_t stack_key) RS_AVAILABLE(0_0);
RSIndex RSBasicHashGetCountOfValue(RSConstBasicHashRef ht, uintptr_t stack_value) RS_AVAILABLE(0_0);
BOOL RSBasicHashesAreEqual(RSConstBasicHashRef ht1, RSConstBasicHashRef ht2) RS_AVAILABLE(0_0);
void RSBasicHashApply(RSConstBasicHashRef ht, BOOL (^block)(RSBasicHashBucket)) RS_AVAILABLE(0_0);
void RSBasicHashApplyBlock(RSConstBasicHashRef ht, BOOL (^block)(RSBasicHashBucket btk, BOOL *stop)) RS_AVAILABLE(0_4);
void RSBasicHashApplyIndexed(RSConstBasicHashRef ht, RSRange range, BOOL (^block)(RSBasicHashBucket)) RS_AVAILABLE(0_0);
void RSBasicHashGetElements(RSConstBasicHashRef ht, RSIndex bufferslen, uintptr_t *weak_values, uintptr_t *weak_keys) RS_AVAILABLE(0_0);

BOOL RSBasicHashAddValue(RSBasicHashRef ht, uintptr_t stack_key, uintptr_t stack_value) RS_AVAILABLE(0_0);
void RSBasicHashReplaceValue(RSBasicHashRef ht, uintptr_t stack_key, uintptr_t stack_value) RS_AVAILABLE(0_0);
void RSBasicHashSetValue(RSBasicHashRef ht, uintptr_t stack_key, uintptr_t stack_value) RS_AVAILABLE(0_0);
RSIndex RSBasicHashRemoveValue(RSBasicHashRef ht, uintptr_t stack_key) RS_AVAILABLE(0_0);
RSIndex RSBasicHashRemoveValueAtIndex(RSBasicHashRef ht, RSIndex idx) RS_AVAILABLE(0_0);
void RSBasicHashRemoveAllValues(RSBasicHashRef ht) RS_AVAILABLE(0_0);

BOOL RSBasicHashAddIntValueAndInc(RSBasicHashRef ht, uintptr_t stack_key, uintptr_t int_value) RS_AVAILABLE(0_0);
void RSBasicHashRemoveIntValueAndDec(RSBasicHashRef ht, uintptr_t int_value) RS_AVAILABLE(0_0);

size_t RSBasicHashGetSize(RSConstBasicHashRef ht, BOOL total) RS_AVAILABLE(0_0);

RSStringRef RSBasicHashDescription(RSConstBasicHashRef ht, BOOL detailed, RSStringRef linePrefix, RSStringRef entryLinePrefix, BOOL describeElements) RS_AVAILABLE(0_0);

RSTypeID RSBasicHashGetTypeID(void) RS_AVAILABLE(0_0);

extern BOOL __RSBasicHashEqual(RSTypeRef obj1, RSTypeRef obj2) RS_AVAILABLE(0_0);
extern RSHashCode __RSBasicHashHash(RSTypeRef obj) RS_AVAILABLE(0_0);
extern RSStringRef __RSBasicHashDescription(RSTypeRef obj) RS_AVAILABLE(0_0);
extern void __RSBasicHashDeallocate(RSTypeRef obj) RS_AVAILABLE(0_0);
extern unsigned long __RSBasicHashFastEnumeration(RSConstBasicHashRef ht, struct __RSFastEnumerationStateEquivalent2 *state, void *stackbuffer, unsigned long count) RS_AVAILABLE(0_0);

// creation functions create mutable RSBasicHashRefs
RSBasicHashRef RSBasicHashCreate(RSAllocatorRef allocator, RSOptionFlags flags, const RSBasicHashCallbacks *cb) RS_AVAILABLE(0_0);
RSBasicHashRef RSBasicHashCreateCopy(RSAllocatorRef allocator, RSConstBasicHashRef ht) RS_AVAILABLE(0_0);
void __RSBasicHashSetCallbacks(RSBasicHashRef ht, const RSBasicHashCallbacks *cb) RS_AVAILABLE(0_0);

RS_EXTERN_C_END
#endif
