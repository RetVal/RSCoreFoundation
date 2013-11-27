//
//  RSBasicHash.c
//  RSCoreFoundation
//
//  Created by Apple Inc.
//  Changed by RetVal on 12/9/12.
//  Copyright (c) 2012 RetVal. All rights reserved.
//

#include <stdio.h>
#include <RSCoreFoundation/RSBase.h>

#include <RSCoreFoundation/RSBasicHash.h>

#include <RSCoreFoundation/RSRuntime.h>

#ifndef RSBasicHashVersion
#define RSBasicHashVersion 2
#endif

#if (RSBasicHashVersion == 1)
#define COCOA_HASHTABLE_REHASH_END(arg0, arg1, arg2) do {} while (0)
#define COCOA_HASHTABLE_REHASH_END_ENABLED() 0
#define COCOA_HASHTABLE_REHASH_START(arg0, arg1, arg2) do {} while (0)
#define COCOA_HASHTABLE_REHASH_START_ENABLED() 0
#define COCOA_HASHTABLE_HASH_KEY(arg0, arg1, arg2) do {} while (0)
#define COCOA_HASHTABLE_HASH_KEY_ENABLED() 0
#define COCOA_HASHTABLE_PROBE_DELETED(arg0, arg1) do {} while (0)
#define COCOA_HASHTABLE_PROBE_DELETED_ENABLED() 0
#define COCOA_HASHTABLE_PROBE_EMPTY(arg0, arg1) do {} while (0)
#define COCOA_HASHTABLE_PROBE_EMPTY_ENABLED() 0
#define COCOA_HASHTABLE_PROBE_VALID(arg0, arg1) do {} while (0)
#define COCOA_HASHTABLE_PROBE_VALID_ENABLED() 0
#define COCOA_HASHTABLE_PROBING_END(arg0, arg1) do {} while (0)
#define COCOA_HASHTABLE_PROBING_END_ENABLED() 0
#define COCOA_HASHTABLE_PROBING_START(arg0, arg1) do {} while (0)
#define COCOA_HASHTABLE_PROBING_START_ENABLED() 0
#define COCOA_HASHTABLE_TEST_EQUAL(arg0, arg1, arg2) do {} while (0)
#define COCOA_HASHTABLE_TEST_EQUAL_ENABLED() 0


static const uintptr_t __RSBasicHashTableSizes[64] =
{
    0, 3, 7, 13, 23, 41, 71, 127, 191, 251, 383, 631, 1087, 1723,
    2803, 4523, 7351, 11959, 19447, 31231, 50683, 81919, 132607,
    214519, 346607, 561109, 907759, 1468927, 2376191, 3845119,
    6221311, 10066421, 16287743, 26354171, 42641881, 68996069,
    111638519, 180634607, 292272623, 472907251,
#if __LP64__
    765180413UL, 1238087663UL, 2003267557UL, 3241355263UL, 5244622819UL,
#if 0
    8485977589UL, 13730600407UL, 22216578047UL, 35947178479UL,
    58163756537UL, 94110934997UL, 152274691561UL, 246385626107UL,
    398660317687UL, 645045943807UL, 1043706260983UL, 1688752204787UL,
    2732458465769UL, 4421210670577UL, 7153669136377UL,
    11574879807461UL, 18728548943849UL, 30303428750843UL
#endif
#endif
};

static const uintptr_t __RSBasicHashTableCapacities[64] =
{
    0, 3, 6, 11, 19, 32, 52, 85, 118, 155, 237, 390, 672, 1065,
    1732, 2795, 4543, 7391, 12019, 19302, 31324, 50629, 81956,
    132580, 214215, 346784, 561026, 907847, 1468567, 2376414,
    3844982, 6221390, 10066379, 16287773, 26354132, 42641916,
    68996399, 111638327, 180634415, 292272755,
#if __LP64__
    472907503UL, 765180257UL, 1238087439UL, 2003267722UL, 3241355160UL,
#if 0
    5244622578UL, 8485977737UL, 13730600347UL, 22216578100UL,
    35947178453UL, 58163756541UL, 94110935011UL, 152274691274UL,
    246385626296UL, 398660317578UL, 645045943559UL, 1043706261135UL,
    1688752204693UL, 2732458465840UL, 4421210670552UL,
    7153669136706UL, 11574879807265UL, 18728548943682UL
#endif
#endif
};

// Primitive roots for the primes above
static const uintptr_t __RSBasicHashPrimitiveRoots[64] =
{
    0, 2, 3, 2, 5, 6, 7, 3, 19, 6, 5, 3, 3, 3,
    2, 5, 6, 3, 3, 6, 2, 3, 3,
    3, 5, 10, 3, 3, 22, 3,
    3, 3, 5, 2, 22, 2,
    11, 5, 5, 2,
#if __LP64__
    3, 10, 2, 3, 10,
    2, 3, 5, 3,
    3, 2, 7, 2,
    3, 3, 3, 2,
    3, 5, 5,
    2, 3, 2
#endif
};

RSInline void *__RSBasicHashAllocateMemory(RSConstBasicHashRef ht, RSIndex count, RSIndex elem_size, BOOL strong, BOOL compactable)
{
    RSAllocatorRef allocator = RSGetAllocator(ht);//RSAllocatorGetDefault(ht);
    void *new_mem = nil;
    new_mem = RSAllocatorAllocate(allocator, count * elem_size);
    return new_mem;
}

RSInline void *__RSBasicHashAllocateMemory2(RSAllocatorRef allocator, RSIndex count, RSIndex elem_size, BOOL strong, BOOL compactable)
{
    void *new_mem = nil;
    new_mem = RSAllocatorAllocate(allocator, count * elem_size);
    return new_mem;
}

#define __AssignWithWriteBarrier(location, value) objc_assign_strongCast((ISA)value, (ISA *)location)

#define ENABLE_DTRACE_PROBES 0
#define ENABLE_MEMORY_COUNTERS 0

#if defined(DTRACE_PROBES_DISABLED) && DTRACE_PROBES_DISABLED
#undef ENABLE_DTRACE_PROBES
#define ENABLE_DTRACE_PROBES 0
#endif


#define __RSBasicHashSubABZero 0xa7baadb1
#define __RSBasicHashSubABOne 0xa5baadb9

typedef union
{
    uintptr_t neutral;
    ISA strong;
    ISA weak;
} RSBasicHashValue;


struct __RSBasicHash
{
    RSRuntimeBase base;
    struct { // 128 bits
        uint8_t hash_style:2;
        uint8_t fast_grow:1;
        uint8_t keys_offset:1;
        uint8_t counts_offset:2;
        uint8_t counts_width:2;
        uint8_t hashes_offset:2;
        uint8_t strong_values:1;
        uint8_t strong_keys:1;
        uint8_t weak_values:1;
        uint8_t weak_keys:1;
        uint8_t int_values:1;
        uint8_t int_keys:1;
        uint8_t indirect_keys:1;
        uint8_t compactable_keys:1;
        uint8_t compactable_values:1;
        uint8_t finalized:1;
        uint8_t __2:4;
        uint8_t num_buckets_idx;  /* index to number of buckets */
        uint32_t used_buckets;    /* number of used buckets */
        uint8_t __8:8;
        uint8_t __9:8;
        uint16_t special_bits;
        uint16_t deleted;
        uint16_t mutations;
    } bits;
    __strong RSBasicHashCallbacks *callbacks;
    void *pointers[1];
};

RSPrivate BOOL RSBasicHashHasStrongValues(RSConstBasicHashRef ht)
{
#if DEPLOYMENT_TARGET_MACOSX
    return ht->bits.strong_values ? YES : NO;
#else
    return NO;
#endif
}

RSPrivate BOOL RSBasicHashHasStrongKeys(RSConstBasicHashRef ht)
{
#if DEPLOYMENT_TARGET_MACOSX
    return ht->bits.strong_keys ? YES : NO;
#else
    return NO;
#endif
}

RSInline BOOL __RSBasicHashHasCompactableKeys(RSConstBasicHashRef ht)
{
#if DEPLOYMENT_TARGET_MACOSX
    return ht->bits.compactable_keys ? YES : NO;
#else
    return NO;
#endif
}

RSInline BOOL __RSBasicHashHasCompactableValues(RSConstBasicHashRef ht)
{
#if DEPLOYMENT_TARGET_MACOSX
    return ht->bits.compactable_values ? YES : NO;
#else
    return NO;
#endif
}

RSInline BOOL __RSBasicHashHasWeakValues(RSConstBasicHashRef ht)
{
#if DEPLOYMENT_TARGET_MACOSX
    return ht->bits.weak_values ? YES : NO;
#else
    return NO;
#endif
}

RSInline BOOL __RSBasicHashHasWeakKeys(RSConstBasicHashRef ht)
{
#if DEPLOYMENT_TARGET_MACOSX
    return ht->bits.weak_keys ? YES : NO;
#else
    return NO;
#endif
}

RSInline BOOL __RSBasicHashHasHashCache(RSConstBasicHashRef ht)
{
#if DEPLOYMENT_TARGET_MACOSX
    return ht->bits.hashes_offset ? YES : NO;
#else
    return NO;
#endif
}

RSInline uintptr_t __RSBasicHashImportValue(RSConstBasicHashRef ht, uintptr_t stack_value)
{
    return ht->callbacks->retainValue(ht, stack_value);
}

RSInline uintptr_t __RSBasicHashImportKey(RSConstBasicHashRef ht, uintptr_t stack_key)
{
    return ht->callbacks->retainKey(ht, stack_key);
}

RSInline void __RSBasicHashEjectValue(RSConstBasicHashRef ht, uintptr_t stack_value)
{
    ht->callbacks->releaseValue(ht, stack_value);
}

RSInline void __RSBasicHashEjectKey(RSConstBasicHashRef ht, uintptr_t stack_key)
{
    ht->callbacks->releaseKey(ht, stack_key);
}

RSInline BOOL __RSBasicHashTestEqualValue(RSConstBasicHashRef ht, uintptr_t stack_value_a, uintptr_t stack_value_b)
{
    return ht->callbacks->equateValues(ht, stack_value_a, stack_value_b);
}

RSInline BOOL __RSBasicHashTestEqualKey(RSConstBasicHashRef ht, uintptr_t in_coll_key, uintptr_t stack_key)
{
    //COCOA_HASHTABLE_TEST_EQUAL(ht, in_coll_key, stack_key);
    return ht->callbacks->equateKeys(ht, in_coll_key, stack_key);
}

RSInline RSHashCode __RSBasicHashHashKey(RSConstBasicHashRef ht, uintptr_t stack_key)
{
    RSHashCode hash_code = (RSHashCode)ht->callbacks->hashKey(ht, stack_key);
    //COCOA_HASHTABLE_HASH_KEY(ht, stack_key, hash_code);
    return hash_code;
}

RSInline RSBasicHashValue *__RSBasicHashGetValues(RSConstBasicHashRef ht)
{
    return (RSBasicHashValue *)ht->pointers[0];
}

RSInline void __RSBasicHashSetValues(RSBasicHashRef ht, RSBasicHashValue *ptr)
{
    __AssignWithWriteBarrier(&ht->pointers[0], ptr);
}

RSInline RSBasicHashValue *__RSBasicHashGetKeys(RSConstBasicHashRef ht)
{
    return (RSBasicHashValue *)ht->pointers[ht->bits.keys_offset];
}

RSInline void __RSBasicHashSetKeys(RSBasicHashRef ht, RSBasicHashValue *ptr)
{
    __AssignWithWriteBarrier(&ht->pointers[ht->bits.keys_offset], ptr);
}

RSInline void *__RSBasicHashGetCounts(RSConstBasicHashRef ht)
{
    return (void *)ht->pointers[ht->bits.counts_offset];
}

RSInline void __RSBasicHashSetCounts(RSBasicHashRef ht, void *ptr)
{
    __AssignWithWriteBarrier(&ht->pointers[ht->bits.counts_offset], ptr);
}

RSInline uintptr_t __RSBasicHashGetValue(RSConstBasicHashRef ht, RSIndex idx)
{
    uintptr_t val = __RSBasicHashGetValues(ht)[idx].neutral;
    if (__RSBasicHashSubABZero == val) return 0UL;
    if (__RSBasicHashSubABOne == val) return ~0UL;
    return val;
}

RSInline void __RSBasicHashSetValue(RSBasicHashRef ht, RSIndex idx, uintptr_t stack_value, BOOL ignoreOld, BOOL literal)
{
    RSBasicHashValue *valuep = &(__RSBasicHashGetValues(ht)[idx]);
    uintptr_t old_value = ignoreOld ? 0 : valuep->neutral;
    if (!literal)
    {
        if (0UL == stack_value) stack_value = __RSBasicHashSubABZero;
        if (~0UL == stack_value) stack_value = __RSBasicHashSubABOne;
    }
    if (RSBasicHashHasStrongValues(ht)) valuep->strong = (ISA)stack_value; else valuep->neutral = stack_value;
    if (!ignoreOld)
    {
        if (!(old_value == 0UL || old_value == ~0UL)) {
            if (__RSBasicHashSubABZero == old_value) old_value = 0UL;
            if (__RSBasicHashSubABOne == old_value) old_value = ~0UL;
            __RSBasicHashEjectValue(ht, old_value);
        }
    }
}

RSInline uintptr_t __RSBasicHashGetKey(RSConstBasicHashRef ht, RSIndex idx)
{
    if (ht->bits.keys_offset) {
        uintptr_t key = __RSBasicHashGetKeys(ht)[idx].neutral;
        if (__RSBasicHashSubABZero == key) return 0UL;
        if (__RSBasicHashSubABOne == key) return ~0UL;
        return key;
    }
    if (ht->bits.indirect_keys) {
        uintptr_t stack_value = __RSBasicHashGetValue(ht, idx);
        return ht->callbacks->getIndirectKey(ht, stack_value);
    }
    return __RSBasicHashGetValue(ht, idx);
}

RSInline void __RSBasicHashSetKey(RSBasicHashRef ht, RSIndex idx, uintptr_t stack_key, BOOL ignoreOld, BOOL literal)
{
    if (0 == ht->bits.keys_offset) HALTWithError(RSInvalidArgumentException, "the bits.keys_offset is 0");
    RSBasicHashValue *keyp = &(__RSBasicHashGetKeys(ht)[idx]);
    uintptr_t old_key = ignoreOld ? 0 : keyp->neutral;
    if (!literal) {
        if (0UL == stack_key) stack_key = __RSBasicHashSubABZero;
        if (~0UL == stack_key) stack_key = __RSBasicHashSubABOne;
    }
    if (RSBasicHashHasStrongKeys(ht)) keyp->strong = (ISA)stack_key; else keyp->neutral = stack_key;
    if (!ignoreOld) {
        if (!(old_key == 0UL || old_key == ~0UL)) {
            if (__RSBasicHashSubABZero == old_key) old_key = 0UL;
            if (__RSBasicHashSubABOne == old_key) old_key = ~0UL;
            __RSBasicHashEjectKey(ht, old_key);
        }
    }
}

RSInline uintptr_t __RSBasicHashIsEmptyOrDeleted(RSConstBasicHashRef ht, RSIndex idx)
{
    uintptr_t stack_value = __RSBasicHashGetValues(ht)[idx].neutral;
    return (0UL == stack_value || ~0UL == stack_value);
}

RSInline uintptr_t __RSBasicHashIsDeleted(RSConstBasicHashRef ht, RSIndex idx)
{
    uintptr_t stack_value = __RSBasicHashGetValues(ht)[idx].neutral;
    return (~0UL == stack_value);
}

RSInline uintptr_t __RSBasicHashGetSlotCount(RSConstBasicHashRef ht, RSIndex idx)
{
    void *counts = __RSBasicHashGetCounts(ht);
    switch (ht->bits.counts_width)
    {
        case 0: return ((uint8_t *)counts)[idx];
        case 1: return ((uint16_t *)counts)[idx];
        case 2: return ((uint32_t *)counts)[idx];
        case 3: return ((uint64_t *)counts)[idx];
    }
    return 0;
}

RSInline void __RSBasicHashBumpCounts(RSBasicHashRef ht)
{
    void *counts = __RSBasicHashGetCounts(ht);
    //RSAllocatorRef allocator = RSAllocatorGetDefault();
    switch (ht->bits.counts_width)
    {
        case 0:
        {
            uint8_t *counts08 = (uint8_t *)counts;
            ht->bits.counts_width = 1;
            RSIndex num_buckets = __RSBasicHashTableSizes[ht->bits.num_buckets_idx];
            uint16_t *counts16 = (uint16_t *)__RSBasicHashAllocateMemory(ht, num_buckets, 2, NO, NO);
            if (!counts16) HALTWithError(RSInvalidArgumentException, "counts16 is 0");
            ////////__SetLastAllocationEventName(counts16, "RSBasicHash (count-store)");
            for (RSIndex idx2 = 0; idx2 < num_buckets; idx2++)
            {
                counts16[idx2] = counts08[idx2];
            }
            __RSBasicHashSetCounts(ht, counts16);
//            if (!RS_IS_COLLECTABLE_ALLOCATOR(allocator)) {
//                RSAllocatorDeallocate(allocator, counts08);
//            }
            break;
        }
        case 1:
        {
            uint16_t *counts16 = (uint16_t *)counts;
            ht->bits.counts_width = 2;
            RSIndex num_buckets = __RSBasicHashTableSizes[ht->bits.num_buckets_idx];
            uint32_t *counts32 = (uint32_t *)__RSBasicHashAllocateMemory(ht, num_buckets, 4, NO, NO);
            if (!counts32) HALTWithError(RSInvalidArgumentException, "counts32 is 0");
            ////////__SetLastAllocationEventName(counts32, "RSBasicHash (count-store)");
            for (RSIndex idx2 = 0; idx2 < num_buckets; idx2++)
            {
                counts32[idx2] = counts16[idx2];
            }
            __RSBasicHashSetCounts(ht, counts32);
//            if (!RS_IS_COLLECTABLE_ALLOCATOR(allocator)) {
//                RSAllocatorDeallocate(allocator, counts16);
//            }
            break;
        }
        case 2:
        {
            uint32_t *counts32 = (uint32_t *)counts;
            ht->bits.counts_width = 3;
            RSIndex num_buckets = __RSBasicHashTableSizes[ht->bits.num_buckets_idx];
            uint64_t *counts64 = (uint64_t *)__RSBasicHashAllocateMemory(ht, num_buckets, 8, NO, NO);
            if (!counts64) HALTWithError(RSInvalidArgumentException, "counts64 is 0");
            ////////__SetLastAllocationEventName(counts64, "RSBasicHash (count-store)");
            for (RSIndex idx2 = 0; idx2 < num_buckets; idx2++)
            {
                counts64[idx2] = counts32[idx2];
            }
            __RSBasicHashSetCounts(ht, counts64);
//            if (!RS_IS_COLLECTABLE_ALLOCATOR(allocator)) {
//                RSAllocatorDeallocate(allocator, counts32);
//            }
            break;
        }
        case 3:
        {
            HALTWithError(RSInvalidArgumentException, "bits.counts_witdh = 3");
            break;
        }
    }
}

static void __RSBasicHashIncSlotCount(RSBasicHashRef ht, RSIndex idx)
{
    void *counts = __RSBasicHashGetCounts(ht);
    switch (ht->bits.counts_width)
    {
        case 0:
        {
            uint8_t *counts08 = (uint8_t *)counts;
            uint8_t val = counts08[idx];
            if (val < INT8_MAX)
            {
                counts08[idx] = val + 1;
                return;
            }
            __RSBasicHashBumpCounts(ht);
            __RSBasicHashIncSlotCount(ht, idx);
            break;
        }
        case 1:
        {
            uint16_t *counts16 = (uint16_t *)counts;
            uint16_t val = counts16[idx];
            if (val < INT16_MAX)
            {
                counts16[idx] = val + 1;
                return;
            }
            __RSBasicHashBumpCounts(ht);
            __RSBasicHashIncSlotCount(ht, idx);
            break;
        }
        case 2:
        {
            uint32_t *counts32 = (uint32_t *)counts;
            uint32_t val = counts32[idx];
            if (val < INT32_MAX)
            {
                counts32[idx] = val + 1;
                return;
            }
            __RSBasicHashBumpCounts(ht);
            __RSBasicHashIncSlotCount(ht, idx);
            break;
        }
        case 3:
        {
            uint64_t *counts64 = (uint64_t *)counts;
            uint64_t val = counts64[idx];
            if (val < INT64_MAX)
            {
                counts64[idx] = val + 1;
                return;
            }
            __RSBasicHashBumpCounts(ht);
            __RSBasicHashIncSlotCount(ht, idx);
            break;
        }
    }
}

RSInline void __RSBasicHashDecSlotCount(RSBasicHashRef ht, RSIndex idx)
{
    void *counts = __RSBasicHashGetCounts(ht);
    switch (ht->bits.counts_width)
    {
        case 0: ((uint8_t  *)counts)[idx]--; return;
        case 1: ((uint16_t *)counts)[idx]--; return;
        case 2: ((uint32_t *)counts)[idx]--; return;
        case 3: ((uint64_t *)counts)[idx]--; return;
    }
}

RSInline uintptr_t *__RSBasicHashGetHashes(RSConstBasicHashRef ht)
{
    return (uintptr_t *)ht->pointers[ht->bits.hashes_offset];
}

RSInline void __RSBasicHashSetHashes(RSBasicHashRef ht, uintptr_t *ptr)
{
    __AssignWithWriteBarrier(&ht->pointers[ht->bits.hashes_offset], ptr);
}


// to expose the load factor, expose this function to customization
RSInline RSIndex __RSBasicHashGetCapacityForNumBuckets(RSConstBasicHashRef ht, RSIndex num_buckets_idx)
{
    return __RSBasicHashTableCapacities[num_buckets_idx];
}

RSInline RSIndex __RSBasicHashGetNumBucketsIndexForCapacity(RSConstBasicHashRef ht, RSIndex capacity)
{
    for (RSIndex idx = 0; idx < 64; idx++)
    {
        if (capacity <= __RSBasicHashGetCapacityForNumBuckets(ht, idx)) return idx;
    }
    HALTWithError(RSInvalidArgumentException, "number buckets index is not found");
    return 0;
}

RSPrivate RSIndex RSBasicHashGetNumBuckets(RSConstBasicHashRef ht)
{
    return __RSBasicHashTableSizes[ht->bits.num_buckets_idx];
}

RSPrivate RSIndex RSBasicHashGetCapacity(RSConstBasicHashRef ht)
{
    return __RSBasicHashGetCapacityForNumBuckets(ht, ht->bits.num_buckets_idx);
}

// In returned struct, .count is zero if the bucket is empty or deleted,
// and the .weak_key field indicates which. .idx is either the index of
// the found bucket or the index of the bucket which should be filled by
// an add operation. For a set or multiset, the .weak_key and .weak_value
// are the same.
RSPrivate RSBasicHashBucket RSBasicHashGetBucket(RSConstBasicHashRef ht, RSIndex idx)
{
    RSBasicHashBucket result;
    result.idx = idx;
    if (__RSBasicHashIsEmptyOrDeleted(ht, idx))
    {
        result.count = 0;
        result.weak_value = 0;
        result.weak_key = 0;
    }
    else
    {
        result.count = (ht->bits.counts_offset) ? __RSBasicHashGetSlotCount(ht, idx) : 1;
        result.weak_value = __RSBasicHashGetValue(ht, idx);
        result.weak_key = __RSBasicHashGetKey(ht, idx);
    }
    return result;
}

#if defined(__arm__)
static uintptr_t __RSBasicHashFold(uintptr_t dividend, uint8_t idx)
{
    switch (idx)
    {
        case 1: return dividend % 3;
        case 2: return dividend % 7;
        case 3: return dividend % 13;
        case 4: return dividend % 23;
        case 5: return dividend % 41;
        case 6: return dividend % 71;
        case 7: return dividend % 127;
        case 8: return dividend % 191;
        case 9: return dividend % 251;
        case 10: return dividend % 383;
        case 11: return dividend % 631;
        case 12: return dividend % 1087;
        case 13: return dividend % 1723;
        case 14: return dividend % 2803;
        case 15: return dividend % 4523;
        case 16: return dividend % 7351;
        case 17: return dividend % 11959;
        case 18: return dividend % 19447;
        case 19: return dividend % 31231;
        case 20: return dividend % 50683;
        case 21: return dividend % 81919;
        case 22: return dividend % 132607;
        case 23: return dividend % 214519;
        case 24: return dividend % 346607;
        case 25: return dividend % 561109;
        case 26: return dividend % 907759;
        case 27: return dividend % 1468927;
        case 28: return dividend % 2376191;
        case 29: return dividend % 3845119;
        case 30: return dividend % 6221311;
        case 31: return dividend % 10066421;
        case 32: return dividend % 16287743;
        case 33: return dividend % 26354171;
        case 34: return dividend % 42641881;
        case 35: return dividend % 68996069;
        case 36: return dividend % 111638519;
        case 37: return dividend % 180634607;
        case 38: return dividend % 292272623;
        case 39: return dividend % 472907251;
    }
    HALTWithError(RSInvalidArgumentException, "fold is not found");
    return ~0;
}
#endif

#define FIND_BUCKET_NAME		___RSBasicHashFindBucket_Linear
#define FIND_BUCKET_HASH_STYLE		1
#define FIND_BUCKET_FOR_REHASH		0
#define FIND_BUCKET_FOR_INDIRECT_KEY	0
#include "RSBasicHashFindBucket.m"

#define FIND_BUCKET_NAME		___RSBasicHashFindBucket_Linear_NoCollision
#define FIND_BUCKET_HASH_STYLE		1
#define FIND_BUCKET_FOR_REHASH		1
#define FIND_BUCKET_FOR_INDIRECT_KEY	0
#include "RSBasicHashFindBucket.m"

#define FIND_BUCKET_NAME		___RSBasicHashFindBucket_Linear_Indirect
#define FIND_BUCKET_HASH_STYLE		1
#define FIND_BUCKET_FOR_REHASH		0
#define FIND_BUCKET_FOR_INDIRECT_KEY	1
#include "RSBasicHashFindBucket.m"

#define FIND_BUCKET_NAME		___RSBasicHashFindBucket_Linear_Indirect_NoCollision
#define FIND_BUCKET_HASH_STYLE		1
#define FIND_BUCKET_FOR_REHASH		1
#define FIND_BUCKET_FOR_INDIRECT_KEY	1
#include "RSBasicHashFindBucket.m"

#define FIND_BUCKET_NAME		___RSBasicHashFindBucket_Double
#define FIND_BUCKET_HASH_STYLE		2
#define FIND_BUCKET_FOR_REHASH		0
#define FIND_BUCKET_FOR_INDIRECT_KEY	0
#include "RSBasicHashFindBucket.m"

#define FIND_BUCKET_NAME		___RSBasicHashFindBucket_Double_NoCollision
#define FIND_BUCKET_HASH_STYLE		2
#define FIND_BUCKET_FOR_REHASH		1
#define FIND_BUCKET_FOR_INDIRECT_KEY	0
#include "RSBasicHashFindBucket.m"

#define FIND_BUCKET_NAME		___RSBasicHashFindBucket_Double_Indirect
#define FIND_BUCKET_HASH_STYLE		2
#define FIND_BUCKET_FOR_REHASH		0
#define FIND_BUCKET_FOR_INDIRECT_KEY	1
#include "RSBasicHashFindBucket.m"

#define FIND_BUCKET_NAME		___RSBasicHashFindBucket_Double_Indirect_NoCollision
#define FIND_BUCKET_HASH_STYLE		2
#define FIND_BUCKET_FOR_REHASH		1
#define FIND_BUCKET_FOR_INDIRECT_KEY	1
#include "RSBasicHashFindBucket.m"

#define FIND_BUCKET_NAME		___RSBasicHashFindBucket_Exponential
#define FIND_BUCKET_HASH_STYLE		3
#define FIND_BUCKET_FOR_REHASH		0
#define FIND_BUCKET_FOR_INDIRECT_KEY	0
#include "RSBasicHashFindBucket.m"

#define FIND_BUCKET_NAME		___RSBasicHashFindBucket_Exponential_NoCollision
#define FIND_BUCKET_HASH_STYLE		3
#define FIND_BUCKET_FOR_REHASH		1
#define FIND_BUCKET_FOR_INDIRECT_KEY	0
#include "RSBasicHashFindBucket.m"

#define FIND_BUCKET_NAME		___RSBasicHashFindBucket_Exponential_Indirect
#define FIND_BUCKET_HASH_STYLE		3
#define FIND_BUCKET_FOR_REHASH		0
#define FIND_BUCKET_FOR_INDIRECT_KEY	1
#include "RSBasicHashFindBucket.m"

#define FIND_BUCKET_NAME		___RSBasicHashFindBucket_Exponential_Indirect_NoCollision
#define FIND_BUCKET_HASH_STYLE		3
#define FIND_BUCKET_FOR_REHASH		1
#define FIND_BUCKET_FOR_INDIRECT_KEY	1
#include "RSBasicHashFindBucket.m"




RSInline RSBasicHashBucket __RSBasicHashFindBucket(RSConstBasicHashRef ht, uintptr_t stack_key)
{
    if (0 == ht->bits.num_buckets_idx)
    {
        RSBasicHashBucket result = {RSNotFound, 0UL, 0UL, 0};
        return result;
    }
    if (ht->bits.indirect_keys)
    {
        switch (ht->bits.hash_style)
        {
            case __RSBasicHashLinearHashingValue: return ___RSBasicHashFindBucket_Linear_Indirect(ht, stack_key);
            case __RSBasicHashDoubleHashingValue: return ___RSBasicHashFindBucket_Double_Indirect(ht, stack_key);
            case __RSBasicHashExponentialHashingValue: return ___RSBasicHashFindBucket_Exponential_Indirect(ht, stack_key);
        }
    }
    else
    {
        switch (ht->bits.hash_style)
        {
            case __RSBasicHashLinearHashingValue: return ___RSBasicHashFindBucket_Linear(ht, stack_key);
            case __RSBasicHashDoubleHashingValue: return ___RSBasicHashFindBucket_Double(ht, stack_key);
            case __RSBasicHashExponentialHashingValue: return ___RSBasicHashFindBucket_Exponential(ht, stack_key);
        }
    }
    HALTWithError(RSInvalidArgumentException, "the hash style of bucket is not found");
    RSBasicHashBucket result = {RSNotFound, 0UL, 0UL, 0};
    return result;
}

RSInline RSIndex __RSBasicHashFindBucket_NoCollision(RSConstBasicHashRef ht, uintptr_t stack_key, uintptr_t key_hash)
{
    if (0 == ht->bits.num_buckets_idx)
    {
        return RSNotFound;
    }
    if (ht->bits.indirect_keys)
    {
        switch (ht->bits.hash_style)
        {
            case __RSBasicHashLinearHashingValue: return ___RSBasicHashFindBucket_Linear_Indirect_NoCollision(ht, stack_key, key_hash);
            case __RSBasicHashDoubleHashingValue: return ___RSBasicHashFindBucket_Double_Indirect_NoCollision(ht, stack_key, key_hash);
            case __RSBasicHashExponentialHashingValue: return ___RSBasicHashFindBucket_Exponential_Indirect_NoCollision(ht, stack_key, key_hash);
        }
    }
    else
    {
        switch (ht->bits.hash_style)
        {
            case __RSBasicHashLinearHashingValue: return ___RSBasicHashFindBucket_Linear_NoCollision(ht, stack_key, key_hash);
            case __RSBasicHashDoubleHashingValue: return ___RSBasicHashFindBucket_Double_NoCollision(ht, stack_key, key_hash);
            case __RSBasicHashExponentialHashingValue: return ___RSBasicHashFindBucket_Exponential_NoCollision(ht, stack_key, key_hash);
        }
    }
    HALTWithError(RSInvalidArgumentException, "the hash style of bound is not found");
    return RSNotFound;
}

RSPrivate RSBasicHashBucket RSBasicHashFindBucket(RSConstBasicHashRef ht, uintptr_t stack_key)
{
    if (__RSBasicHashSubABZero == stack_key || __RSBasicHashSubABOne == stack_key)
    {
        RSBasicHashBucket result = {RSNotFound, 0UL, 0UL, 0};
        return result;
    }
    return __RSBasicHashFindBucket(ht, stack_key);
}

RSPrivate uint16_t RSBasicHashGetSpecialBits(RSConstBasicHashRef ht)
{
    return ht->bits.special_bits;
}

RSPrivate uint16_t RSBasicHashSetSpecialBits(RSBasicHashRef ht, uint16_t bits)
{
    uint16_t old =  ht->bits.special_bits;
    ht->bits.special_bits = bits;
    return old;
}

RSPrivate RSOptionFlags RSBasicHashGetFlags(RSConstBasicHashRef ht)
{
    RSOptionFlags flags = (ht->bits.hash_style << 13);
    if (RSBasicHashHasStrongValues(ht)) flags |= RSBasicHashStrongValues;
    if (RSBasicHashHasStrongKeys(ht)) flags |= RSBasicHashStrongKeys;
    if (__RSBasicHashHasCompactableKeys(ht)) flags |= RSBasicHashCompactableKeys;
    if (__RSBasicHashHasCompactableValues(ht)) flags |= RSBasicHashCompactableValues;
    if (ht->bits.fast_grow) flags |= RSBasicHashAggressiveGrowth;
    if (ht->bits.keys_offset) flags |= RSBasicHashHasKeys;
    if (ht->bits.counts_offset) flags |= RSBasicHashHasCounts;
    if (__RSBasicHashHasHashCache(ht)) flags |= RSBasicHashHasHashCache;
    return flags;
}

RSPrivate const RSBasicHashCallbacks *RSBasicHashGetCallbacks(RSConstBasicHashRef ht)
{
    return ht->callbacks;
}

RSPrivate RSIndex RSBasicHashGetCount(RSConstBasicHashRef ht)
{
    if (ht->bits.counts_offset) {
        RSIndex total = 0L;
        RSIndex cnt = (RSIndex)__RSBasicHashTableSizes[ht->bits.num_buckets_idx];
        for (RSIndex idx = 0; idx < cnt; idx++) {
            total += __RSBasicHashGetSlotCount(ht, idx);
        }
        return total;
    }
    return (RSIndex)ht->bits.used_buckets;
}

RSPrivate RSIndex RSBasicHashGetCountOfKey(RSConstBasicHashRef ht, uintptr_t stack_key)
{
    if (__RSBasicHashSubABZero == stack_key || __RSBasicHashSubABOne == stack_key) {
        return 0L;
    }
    if (0L == ht->bits.used_buckets) {
        return 0L;
    }
    return __RSBasicHashFindBucket(ht, stack_key).count;
}

RSPrivate RSIndex RSBasicHashGetCountOfValue(RSConstBasicHashRef ht, uintptr_t stack_value)
{
    if (__RSBasicHashSubABZero == stack_value) {
        return 0L;
    }
    if (0L == ht->bits.used_buckets) {
        return 0L;
    }
    if (!(ht->bits.keys_offset)) {
        return __RSBasicHashFindBucket(ht, stack_value).count;
    }
    __block RSIndex total = 0L;
    RSBasicHashApply(ht, ^(RSBasicHashBucket bkt) {
        if ((stack_value == bkt.weak_value) || __RSBasicHashTestEqualValue(ht, bkt.weak_value, stack_value)) total += bkt.count;
        return (BOOL)YES;
    });
    return total;
}

RSPrivate BOOL RSBasicHashesAreEqual(RSConstBasicHashRef ht1, RSConstBasicHashRef ht2)
{
    RSIndex cnt1 = RSBasicHashGetCount(ht1);
    if (cnt1 != RSBasicHashGetCount(ht2)) return NO;
    if (0 == cnt1) return YES;
    __block BOOL equal = YES;
    RSBasicHashApply(ht1, ^(RSBasicHashBucket bkt1) {
        RSBasicHashBucket bkt2 = __RSBasicHashFindBucket(ht2, bkt1.weak_key);
        if (bkt1.count != bkt2.count) {
            equal = NO;
            return (BOOL)NO;
        }
        if ((ht1->bits.keys_offset) && (bkt1.weak_value != bkt2.weak_value) && !__RSBasicHashTestEqualValue(ht1, bkt1.weak_value, bkt2.weak_value)) {
            equal = NO;
            return (BOOL)NO;
        }
        return (BOOL)YES;
    });
    return equal;
}

RSPrivate void RSBasicHashApply(RSConstBasicHashRef ht, BOOL (^block)(RSBasicHashBucket))
{
    RSIndex used = (RSIndex)ht->bits.used_buckets, cnt = (RSIndex)__RSBasicHashTableSizes[ht->bits.num_buckets_idx];
    for (RSIndex idx = 0; 0 < used && idx < cnt; idx++) {
        RSBasicHashBucket bkt = RSBasicHashGetBucket(ht, idx);
        if (0 < bkt.count) {
            if (!block(bkt)) {
                return;
            }
            used--;
        }
    }
}

RSPrivate void RSBasicHashApplyIndexed(RSConstBasicHashRef ht, RSRange range, BOOL (^block)(RSBasicHashBucket))
{
    if (range.length < 0) HALTWithError(RSInvalidArgumentException, "the length of range is not available");
    if (range.length == 0) return;
    RSIndex cnt = (RSIndex)__RSBasicHashTableSizes[ht->bits.num_buckets_idx];
    if (cnt < range.location + range.length) HALTWithError(RSRangeException, "the range is overflow");
    for (RSIndex idx = 0; idx < range.length; idx++) {
        RSBasicHashBucket bkt = RSBasicHashGetBucket(ht, range.location + idx);
        if (0 < bkt.count) {
            if (!block(bkt)) {
                return;
            }
        }
    }
}

RSPrivate void RSBasicHashGetElements(RSConstBasicHashRef ht, RSIndex bufferslen, uintptr_t *weak_values, uintptr_t *weak_keys)
{
    RSIndex used = (RSIndex)ht->bits.used_buckets, cnt = (RSIndex)__RSBasicHashTableSizes[ht->bits.num_buckets_idx];
    RSIndex offset = 0;
    for (RSIndex idx = 0; 0 < used && idx < cnt && offset < bufferslen; idx++) {
        RSBasicHashBucket bkt = RSBasicHashGetBucket(ht, idx);
        if (0 < bkt.count) {
            used--;
            for (RSIndex cnt = bkt.count; cnt-- && offset < bufferslen;) {
                if (weak_values) { weak_values[offset] = bkt.weak_value; }
                if (weak_keys) { weak_keys[offset] = bkt.weak_key; }
                offset++;
            }
        }
    }
}

RSPrivate unsigned long __RSBasicHashFastEnumeration(RSConstBasicHashRef ht, struct __RSFastEnumerationStateEquivalent2 *state, void *stackbuffer, unsigned long count)
{
    /* copy as many as count items over */
    if (0 == state->state) {        /* first time */
        state->mutationsPtr = (unsigned long *)&ht->bits + (__LP64__ ? 1 : 3);
    }
    state->itemsPtr = (unsigned long *)stackbuffer;
    RSIndex cntx = 0;
    RSIndex used = (RSIndex)ht->bits.used_buckets, cnt = (RSIndex)__RSBasicHashTableSizes[ht->bits.num_buckets_idx];
    for (RSIndex idx = (RSIndex)state->state; 0 < used && idx < cnt && cntx < (RSIndex)count; idx++) {
        RSBasicHashBucket bkt = RSBasicHashGetBucket(ht, idx);
        if (0 < bkt.count) {
            state->itemsPtr[cntx++] = (unsigned long)bkt.weak_key;
            used--;
        }
        state->state++;
    }
    return cntx;
}

#if ENABLE_MEMORY_COUNTERS
static volatile int64_t __RSBasicHashTotalCount = 0ULL;
static volatile int64_t __RSBasicHashTotalSize = 0ULL;
static volatile int64_t __RSBasicHashPeakCount = 0ULL;
static volatile int64_t __RSBasicHashPeakSize = 0ULL;
static volatile int32_t __RSBasicHashSizes[64] = {0};
#endif

static void __RSBasicHashDrain(RSBasicHashRef ht, BOOL forFinalization) {
#if ENABLE_MEMORY_COUNTERS
    OSAtomicAdd64Barrier(-1 * (int64_t) RSBasicHashGetSize(ht, YES), & __RSBasicHashTotalSize);
#endif
    
    RSIndex old_num_buckets = __RSBasicHashTableSizes[ht->bits.num_buckets_idx];
    
    RSAllocatorRef allocator = RSGetAllocator(ht);
    BOOL nullify = (!forFinalization || !0);//RS_IS_COLLECTABLE_ALLOCATOR(allocator));
    
    RSBasicHashValue *old_values = nil, *old_keys = nil;
    void *old_counts = nil;
    uintptr_t *old_hashes = nil;
    
    old_values = __RSBasicHashGetValues(ht);
    if (nullify) __RSBasicHashSetValues(ht, nil);
    if (ht->bits.keys_offset) {
        old_keys = __RSBasicHashGetKeys(ht);
        if (nullify) __RSBasicHashSetKeys(ht, nil);
    }
    if (ht->bits.counts_offset) {
        old_counts = __RSBasicHashGetCounts(ht);
        if (nullify) __RSBasicHashSetCounts(ht, nil);
    }
    if (__RSBasicHashHasHashCache(ht)) {
        old_hashes = __RSBasicHashGetHashes(ht);
        if (nullify) __RSBasicHashSetHashes(ht, nil);
    }
    
    if (nullify) {
        ht->bits.mutations++;
        ht->bits.num_buckets_idx = 0;
        ht->bits.used_buckets = 0;
        ht->bits.deleted = 0;
    }
    
    for (RSIndex idx = 0; idx < old_num_buckets; idx++) {
        uintptr_t stack_value = old_values[idx].neutral;
        if (stack_value != 0UL && stack_value != ~0UL) {
            uintptr_t old_value = stack_value;
            if (__RSBasicHashSubABZero == old_value) old_value = 0UL;
            if (__RSBasicHashSubABOne == old_value) old_value = ~0UL;
            __RSBasicHashEjectValue(ht, old_value);
            if (old_keys) {
                uintptr_t old_key = old_keys[idx].neutral;
                if (__RSBasicHashSubABZero == old_key) old_key = 0UL;
                if (__RSBasicHashSubABOne == old_key) old_key = ~0UL;
                __RSBasicHashEjectKey(ht, old_key);
            }
        }
    }
    
    if (forFinalization) {
        ht->callbacks->freeCallbacks(ht, allocator, ht->callbacks);
    }
    
    if (!0)//RS_IS_COLLECTABLE_ALLOCATOR(allocator)) {
    {
        
    
        RSAllocatorDeallocate(allocator, old_values);
        RSAllocatorDeallocate(allocator, old_keys);
        RSAllocatorDeallocate(allocator, old_counts);
        RSAllocatorDeallocate(allocator, old_hashes);
    }
    
#if ENABLE_MEMORY_COUNTERS
    int64_t size_now = OSAtomicAdd64Barrier((int64_t) RSBasicHashGetSize(ht, YES), & __RSBasicHashTotalSize);
    while (__RSBasicHashPeakSize < size_now && !OSAtomicCompareAndSwap64Barrier(__RSBasicHashPeakSize, size_now, & __RSBasicHashPeakSize));
#endif
}

static void __RSBasicHashRehash(RSBasicHashRef ht, RSIndex newItemCount) {
#if ENABLE_MEMORY_COUNTERS
    OSAtomicAdd64Barrier(-1 * (int64_t) RSBasicHashGetSize(ht, YES), & __RSBasicHashTotalSize);
    OSAtomicAdd32Barrier(-1, &__RSBasicHashSizes[ht->bits.num_buckets_idx]);
#endif
    
    if (COCOA_HASHTABLE_REHASH_START_ENABLED()) COCOA_HASHTABLE_REHASH_START(ht, RSBasicHashGetNumBuckets(ht), RSBasicHashGetSize(ht, YES));
    
    RSIndex new_num_buckets_idx = ht->bits.num_buckets_idx;
    if (0 != newItemCount) {
        if (newItemCount < 0) newItemCount = 0;
        RSIndex new_capacity_req = ht->bits.used_buckets + newItemCount;
        new_num_buckets_idx = __RSBasicHashGetNumBucketsIndexForCapacity(ht, new_capacity_req);
        if (1 == newItemCount && ht->bits.fast_grow) {
            new_num_buckets_idx++;
        }
    }
    
    RSIndex new_num_buckets = __RSBasicHashTableSizes[new_num_buckets_idx];
    RSIndex old_num_buckets = __RSBasicHashTableSizes[ht->bits.num_buckets_idx];
    
    RSBasicHashValue *new_values = nil, *new_keys = nil;
    void *new_counts = nil;
    uintptr_t *new_hashes = nil;
    
    if (0 < new_num_buckets) {
        new_values = (RSBasicHashValue *)__RSBasicHashAllocateMemory(ht, new_num_buckets, sizeof(RSBasicHashValue), RSBasicHashHasStrongValues(ht), __RSBasicHashHasCompactableValues(ht));
        if (!new_values) HALTWithError(RSMallocException, "new value is nil");
        ////__SetLastAllocationEventName(new_values, "RSBasicHash (value-store)");
        memset(new_values, 0, new_num_buckets * sizeof(RSBasicHashValue));
        if (ht->bits.keys_offset) {
            new_keys = (RSBasicHashValue *)__RSBasicHashAllocateMemory(ht, new_num_buckets, sizeof(RSBasicHashValue), RSBasicHashHasStrongKeys(ht), __RSBasicHashHasCompactableKeys(ht));
            if (!new_keys) HALTWithError(RSMallocException, "new keys is nil");
            ////__SetLastAllocationEventName(new_keys, "RSBasicHash (key-store)");
            memset(new_keys, 0, new_num_buckets * sizeof(RSBasicHashValue));
        }
        if (ht->bits.counts_offset) {
            new_counts = (uintptr_t *)__RSBasicHashAllocateMemory(ht, new_num_buckets, (1 << ht->bits.counts_width), NO, NO);
            if (!new_counts) HALTWithError(RSMallocException, "mew counts is 0");
            ////__SetLastAllocationEventName(new_counts, "RSBasicHash (count-store)");
            memset(new_counts, 0, new_num_buckets * (1 << ht->bits.counts_width));
        }
        if (__RSBasicHashHasHashCache(ht)) {
            new_hashes = (uintptr_t *)__RSBasicHashAllocateMemory(ht, new_num_buckets, sizeof(uintptr_t), NO, NO);
            if (!new_hashes) HALTWithError(RSMallocException, "new hash cache is nil");
            ////__SetLastAllocationEventName(new_hashes, "RSBasicHash (hash-store)");
            memset(new_hashes, 0, new_num_buckets * sizeof(uintptr_t));
        }
    }
    
    ht->bits.num_buckets_idx = new_num_buckets_idx;
    ht->bits.deleted = 0;
    
    RSBasicHashValue *old_values = nil, *old_keys = nil;
    void *old_counts = nil;
    uintptr_t *old_hashes = nil;
    
    old_values = __RSBasicHashGetValues(ht);
    __RSBasicHashSetValues(ht, new_values);
    if (ht->bits.keys_offset) {
        old_keys = __RSBasicHashGetKeys(ht);
        __RSBasicHashSetKeys(ht, new_keys);
    }
    if (ht->bits.counts_offset) {
        old_counts = __RSBasicHashGetCounts(ht);
        __RSBasicHashSetCounts(ht, new_counts);
    }
    if (__RSBasicHashHasHashCache(ht)) {
        old_hashes = __RSBasicHashGetHashes(ht);
        __RSBasicHashSetHashes(ht, new_hashes);
    }
    
    if (0 < old_num_buckets) {
        for (RSIndex idx = 0; idx < old_num_buckets; idx++) {
            uintptr_t stack_value = old_values[idx].neutral;
            if (stack_value != 0UL && stack_value != ~0UL) {
                if (__RSBasicHashSubABZero == stack_value) stack_value = 0UL;
                if (__RSBasicHashSubABOne == stack_value) stack_value = ~0UL;
                uintptr_t stack_key = stack_value;
                if (ht->bits.keys_offset && old_keys) {
                    stack_key = old_keys[idx].neutral;
                    if (__RSBasicHashSubABZero == stack_key) stack_key = 0UL;
                    if (__RSBasicHashSubABOne == stack_key) stack_key = ~0UL;
                }
                if (ht->bits.indirect_keys) {
                    stack_key = ht->callbacks->getIndirectKey(ht, stack_value);
                }
                RSIndex bkt_idx = __RSBasicHashFindBucket_NoCollision(ht, stack_key, old_hashes ? old_hashes[idx] : 0UL);
                __RSBasicHashSetValue(ht, bkt_idx, stack_value, NO, NO);
                if (old_keys) {
                    __RSBasicHashSetKey(ht, bkt_idx, stack_key, NO, NO);
                }
                if (old_counts) {
                    switch (ht->bits.counts_width) {
                        case 0: ((uint8_t *)new_counts)[bkt_idx] = ((uint8_t *)old_counts)[idx]; break;
                        case 1: ((uint16_t *)new_counts)[bkt_idx] = ((uint16_t *)old_counts)[idx]; break;
                        case 2: ((uint32_t *)new_counts)[bkt_idx] = ((uint32_t *)old_counts)[idx]; break;
                        case 3: ((uint64_t *)new_counts)[bkt_idx] = ((uint64_t *)old_counts)[idx]; break;
                    }
                }
                if (old_hashes) {
                    new_hashes[bkt_idx] = old_hashes[idx];
                }
            }
        }
    }
    
    RSAllocatorRef allocator = RSAllocatorGetDefault(ht);
    if (!0)//RS_IS_COLLECTABLE_ALLOCATOR(allocator)) {
    {
        RSAllocatorDeallocate(allocator, old_values);
        RSAllocatorDeallocate(allocator, old_keys);
        RSAllocatorDeallocate(allocator, old_counts);
        RSAllocatorDeallocate(allocator, old_hashes);
    }
    
    if (COCOA_HASHTABLE_REHASH_END_ENABLED()) COCOA_HASHTABLE_REHASH_END(ht, RSBasicHashGetNumBuckets(ht), RSBasicHashGetSize(ht, YES));
    
#if ENABLE_MEMORY_COUNTERS
    int64_t size_now = OSAtomicAdd64Barrier((int64_t) RSBasicHashGetSize(ht, YES), &__RSBasicHashTotalSize);
    while (__RSBasicHashPeakSize < size_now && !OSAtomicCompareAndSwap64Barrier(__RSBasicHashPeakSize, size_now, & __RSBasicHashPeakSize));
    OSAtomicAdd32Barrier(1, &__RSBasicHashSizes[ht->bits.num_buckets_idx]);
#endif
}

RSPrivate void RSBasicHashSetCapacity(RSBasicHashRef ht, RSIndex capacity) {
    if (!RSBasicHashIsMutable(ht)) HALTWithError(RSInvalidArgumentException, "hash table is not mutable");
    if (ht->bits.used_buckets < capacity) {
        ht->bits.mutations++;
        __RSBasicHashRehash(ht, capacity - ht->bits.used_buckets);
    }
}

static void __RSBasicHashAddValue(RSBasicHashRef ht, RSIndex bkt_idx, uintptr_t stack_key, uintptr_t stack_value) {
    ht->bits.mutations++;
    if (RSBasicHashGetCapacity(ht) < ht->bits.used_buckets + 1) {
        __RSBasicHashRehash(ht, 1);
        bkt_idx = __RSBasicHashFindBucket_NoCollision(ht, stack_key, 0);
    } else if (__RSBasicHashIsDeleted(ht, bkt_idx)) {
        ht->bits.deleted--;
    }
    uintptr_t key_hash = 0;
    if (__RSBasicHashHasHashCache(ht)) {
        key_hash = __RSBasicHashHashKey(ht, stack_key);
    }
    stack_value = __RSBasicHashImportValue(ht, stack_value);
    if (ht->bits.keys_offset) {
        stack_key = __RSBasicHashImportKey(ht, stack_key);
    }
    __RSBasicHashSetValue(ht, bkt_idx, stack_value, NO, NO);
    if (ht->bits.keys_offset) {
        __RSBasicHashSetKey(ht, bkt_idx, stack_key, NO, NO);
    }
    if (ht->bits.counts_offset) {
        __RSBasicHashIncSlotCount(ht, bkt_idx);
    }
    if (__RSBasicHashHasHashCache(ht)) {
        __RSBasicHashGetHashes(ht)[bkt_idx] = key_hash;
    }
    ht->bits.used_buckets++;
}

static void __RSBasicHashReplaceValue(RSBasicHashRef ht, RSIndex bkt_idx, uintptr_t stack_key, uintptr_t stack_value) {
    ht->bits.mutations++;
    stack_value = __RSBasicHashImportValue(ht, stack_value);
    if (ht->bits.keys_offset) {
        stack_key = __RSBasicHashImportKey(ht, stack_key);
    }
    __RSBasicHashSetValue(ht, bkt_idx, stack_value, NO, NO);
    if (ht->bits.keys_offset) {
        __RSBasicHashSetKey(ht, bkt_idx, stack_key, NO, NO);
    }
}

static void __RSBasicHashRemoveValue(RSBasicHashRef ht, RSIndex bkt_idx) {
    ht->bits.mutations++;
    __RSBasicHashSetValue(ht, bkt_idx, ~0UL, NO, YES);
    if (ht->bits.keys_offset) {
        __RSBasicHashSetKey(ht, bkt_idx, ~0UL, NO, YES);
    }
    if (ht->bits.counts_offset) {
        __RSBasicHashDecSlotCount(ht, bkt_idx);
    }
    if (__RSBasicHashHasHashCache(ht)) {
        __RSBasicHashGetHashes(ht)[bkt_idx] = 0;
    }
    ht->bits.used_buckets--;
    ht->bits.deleted++;
    BOOL do_shrink = NO;
    if (ht->bits.fast_grow) { // == slow shrink
        do_shrink = (5 < ht->bits.num_buckets_idx && ht->bits.used_buckets < __RSBasicHashGetCapacityForNumBuckets(ht, ht->bits.num_buckets_idx - 5));
    } else {
        do_shrink = (2 < ht->bits.num_buckets_idx && ht->bits.used_buckets < __RSBasicHashGetCapacityForNumBuckets(ht, ht->bits.num_buckets_idx - 2));
    }
    if (do_shrink) {
        __RSBasicHashRehash(ht, -1);
        return;
    }
    do_shrink = (0 == ht->bits.deleted); // .deleted roll-over
    RSIndex num_buckets = __RSBasicHashTableSizes[ht->bits.num_buckets_idx];
    do_shrink = do_shrink || ((20 <= num_buckets) && (num_buckets / 4 <= ht->bits.deleted));
    if (do_shrink) {
        __RSBasicHashRehash(ht, 0);
    }
}

RSPrivate BOOL RSBasicHashAddValue(RSBasicHashRef ht, uintptr_t stack_key, uintptr_t stack_value) {
    if (!RSBasicHashIsMutable(ht)) HALTWithError(RSInvalidArgumentException, "hash table is not mutable");
    if (__RSBasicHashSubABZero == stack_key) HALTWithError(RSInvalidArgumentException, "stack key is panic one");
    if (__RSBasicHashSubABOne == stack_key) HALTWithError(RSInvalidArgumentException, "stack key is panic two");
    if (__RSBasicHashSubABZero == stack_value) HALTWithError(RSInvalidArgumentException, "stack value is panic one");
    if (__RSBasicHashSubABOne == stack_value) HALTWithError(RSInvalidArgumentException, "stack value is panic two");
    RSBasicHashBucket bkt = __RSBasicHashFindBucket(ht, stack_key);
    if (0 < bkt.count) {
        ht->bits.mutations++;
        if (ht->bits.counts_offset && bkt.count < LONG_MAX) { // if not yet as large as a RSIndex can be... otherwise clamp and do nothing
            __RSBasicHashIncSlotCount(ht, bkt.idx);
            return YES;
        }
    } else {
        __RSBasicHashAddValue(ht, bkt.idx, stack_key, stack_value);
        return YES;
    }
    return NO;
}

RSPrivate void RSBasicHashReplaceValue(RSBasicHashRef ht, uintptr_t stack_key, uintptr_t stack_value) {
    if (!RSBasicHashIsMutable(ht)) HALTWithError(RSInvalidArgumentException, "hash table is not mutable");
    if (__RSBasicHashSubABZero == stack_key) HALTWithError(RSInvalidArgumentException, "stack key is panic one");
    if (__RSBasicHashSubABOne == stack_key) HALTWithError(RSInvalidArgumentException, "stack key is panic two");
    if (__RSBasicHashSubABZero == stack_value) HALTWithError(RSInvalidArgumentException, "stack value is panic one");
    if (__RSBasicHashSubABOne == stack_value) HALTWithError(RSInvalidArgumentException, "stack value is panic two");
    RSBasicHashBucket bkt = __RSBasicHashFindBucket(ht, stack_key);
    if (0 < bkt.count) {
        __RSBasicHashReplaceValue(ht, bkt.idx, stack_key, stack_value);
    }
}

RSPrivate void RSBasicHashSetValue(RSBasicHashRef ht, uintptr_t stack_key, uintptr_t stack_value) {
    if (!RSBasicHashIsMutable(ht)) HALTWithError(RSInvalidArgumentException, "hash table is not mutable");
    if (__RSBasicHashSubABZero == stack_key) HALTWithError(RSInvalidArgumentException, "stack key is panic one");
    if (__RSBasicHashSubABOne == stack_key) HALTWithError(RSInvalidArgumentException, "stack key is panic two");
    if (__RSBasicHashSubABZero == stack_value) HALTWithError(RSInvalidArgumentException, "stack value is panic one");
    if (__RSBasicHashSubABOne == stack_value) HALTWithError(RSInvalidArgumentException, "stack value is panic two");
    RSBasicHashBucket bkt = __RSBasicHashFindBucket(ht, stack_key);
    if (0 < bkt.count) {
        __RSBasicHashReplaceValue(ht, bkt.idx, stack_key, stack_value);
    } else {
        __RSBasicHashAddValue(ht, bkt.idx, stack_key, stack_value);
    }
}

RSPrivate RSIndex RSBasicHashRemoveValue(RSBasicHashRef ht, uintptr_t stack_key) {
    if (!RSBasicHashIsMutable(ht)) HALTWithError(RSInvalidArgumentException, "hash table is not mutable");
    if (__RSBasicHashSubABZero == stack_key || __RSBasicHashSubABOne == stack_key) return 0;
    RSBasicHashBucket bkt = __RSBasicHashFindBucket(ht, stack_key);
    if (1 < bkt.count) {
        ht->bits.mutations++;
        if (ht->bits.counts_offset && bkt.count < LONG_MAX) { // if not as large as a RSIndex can be... otherwise clamp and do nothing
            __RSBasicHashDecSlotCount(ht, bkt.idx);
        }
    } else if (0 < bkt.count) {
        __RSBasicHashRemoveValue(ht, bkt.idx);
    }
    return bkt.count;
}

RSPrivate RSIndex RSBasicHashRemoveValueAtIndex(RSBasicHashRef ht, RSIndex idx) {
    if (!RSBasicHashIsMutable(ht)) HALTWithError(RSInvalidArgumentException, "hash table is not mutable");
    RSBasicHashBucket bkt = RSBasicHashGetBucket(ht, idx);
    if (1 < bkt.count) {
        ht->bits.mutations++;
        if (ht->bits.counts_offset && bkt.count < LONG_MAX) { // if not as large as a RSIndex can be... otherwise clamp and do nothing
            __RSBasicHashDecSlotCount(ht, bkt.idx);
        }
    } else if (0 < bkt.count) {
        __RSBasicHashRemoveValue(ht, bkt.idx);
    }
    return bkt.count;
}

RSPrivate void RSBasicHashRemoveAllValues(RSBasicHashRef ht) {
    if (!RSBasicHashIsMutable(ht)) HALTWithError(RSInvalidArgumentException, "hash table is not mutable");
    if (0 == ht->bits.num_buckets_idx) return;
    __RSBasicHashDrain(ht, NO);
}

BOOL RSBasicHashAddIntValueAndInc(RSBasicHashRef ht, uintptr_t stack_key, uintptr_t int_value) {
    if (!RSBasicHashIsMutable(ht)) HALTWithError(RSInvalidArgumentException, "hash table is not mutable");
    if (__RSBasicHashSubABZero == stack_key) HALTWithError(RSInvalidArgumentException, "stack key is panic one");
    if (__RSBasicHashSubABOne == stack_key) HALTWithError(RSInvalidArgumentException, "stack key is panic two");
    if (__RSBasicHashSubABZero == int_value) HALTWithError(RSInvalidArgumentException, "int value is panic one");
    if (__RSBasicHashSubABOne == int_value) HALTWithError(RSInvalidArgumentException, "int value is panic two");
    RSBasicHashBucket bkt = __RSBasicHashFindBucket(ht, stack_key);
    if (0 < bkt.count) {
        ht->bits.mutations++;
    } else {
        // must rehash before renumbering
        if (RSBasicHashGetCapacity(ht) < ht->bits.used_buckets + 1) {
            __RSBasicHashRehash(ht, 1);
            bkt.idx = __RSBasicHashFindBucket_NoCollision(ht, stack_key, 0);
        }
        RSIndex cnt = (RSIndex)__RSBasicHashTableSizes[ht->bits.num_buckets_idx];
        for (RSIndex idx = 0; idx < cnt; idx++) {
            if (!__RSBasicHashIsEmptyOrDeleted(ht, idx)) {
                uintptr_t stack_value = __RSBasicHashGetValue(ht, idx);
                if (int_value <= stack_value) {
                    stack_value++;
                    __RSBasicHashSetValue(ht, idx, stack_value, YES, NO);
                    ht->bits.mutations++;
                }
            }
        }
        __RSBasicHashAddValue(ht, bkt.idx, stack_key, int_value);
        return YES;
    }
    return NO;
}

void RSBasicHashRemoveIntValueAndDec(RSBasicHashRef ht, uintptr_t int_value) {
    if (!RSBasicHashIsMutable(ht)) HALTWithError(RSInvalidArgumentException, "hash table is not mutable");
    if (__RSBasicHashSubABZero == int_value) HALTWithError(RSInvalidArgumentException, "int value is panic one");
    if (__RSBasicHashSubABOne == int_value) HALTWithError(RSInvalidArgumentException, "int value is panic two");
    uintptr_t bkt_idx = ~0UL;
    RSIndex cnt = (RSIndex)__RSBasicHashTableSizes[ht->bits.num_buckets_idx];
    for (RSIndex idx = 0; idx < cnt; idx++) {
        if (!__RSBasicHashIsEmptyOrDeleted(ht, idx)) {
            uintptr_t stack_value = __RSBasicHashGetValue(ht, idx);
            if (int_value == stack_value) {
                bkt_idx = idx;
            }
            if (int_value < stack_value) {
                stack_value--;
                __RSBasicHashSetValue(ht, idx, stack_value, YES, NO);
                ht->bits.mutations++;
            }
        }
    }
    __RSBasicHashRemoveValue(ht, bkt_idx);
}

RSPrivate size_t RSBasicHashGetSize(RSConstBasicHashRef ht, BOOL total) {
    size_t size = sizeof(struct __RSBasicHash);
    if (ht->bits.keys_offset) size += sizeof(RSBasicHashValue *);
    if (ht->bits.counts_offset) size += sizeof(void *);
    if (__RSBasicHashHasHashCache(ht)) size += sizeof(uintptr_t *);
    if (total) {
        RSIndex num_buckets = __RSBasicHashTableSizes[ht->bits.num_buckets_idx];
        if (0 < num_buckets)
        {
            size += RSGetInstanceSize(__RSBasicHashGetValues(ht));
            if (ht->bits.keys_offset) size += RSGetInstanceSize(__RSBasicHashGetKeys(ht));
            if (ht->bits.counts_offset) size += RSGetInstanceSize(__RSBasicHashGetCounts(ht));
            if (__RSBasicHashHasHashCache(ht)) size += RSGetInstanceSize(__RSBasicHashGetHashes(ht));
            size += RSGetInstanceSize((void *)ht->callbacks);
        }
    }
    return size;
}

RSPrivate RSStringRef RSBasicHashDescription(RSConstBasicHashRef ht, BOOL detailed, RSStringRef prefix, RSStringRef entryPrefix, BOOL describeElements) {
    RSMutableStringRef result = RSStringCreateMutable(RSAllocatorSystemDefault, 0);
    RSStringAppendStringWithFormat(result, RSSTR("%R{type = %s %s%s, count = %ld,\n"), prefix, (RSBasicHashIsMutable(ht) ? "mutable" : "immutable"), ((ht->bits.counts_offset) ? "multi" : ""), ((ht->bits.keys_offset) ? "dict" : "set"), RSBasicHashGetCount(ht));
    if (detailed) {
        const char *cb_type = "custom";
        RSStringAppendStringWithFormat(result, RSSTR("%Rhash cache = %s, strong values = %s, strong keys = %s, cb = %s,\n"), prefix, (__RSBasicHashHasHashCache(ht) ? "yes" : "no"), (RSBasicHashHasStrongValues(ht) ? "yes" : "no"), (RSBasicHashHasStrongKeys(ht) ? "yes" : "no"), cb_type);
        RSStringAppendStringWithFormat(result, RSSTR("%Rnum bucket index = %d, num buckets = %ld, capacity = %d, num buckets used = %u,\n"), prefix, ht->bits.num_buckets_idx, RSBasicHashGetNumBuckets(ht), RSBasicHashGetCapacity(ht), ht->bits.used_buckets);
        RSStringAppendStringWithFormat(result, RSSTR("%Rcounts width = %d, finalized = %s,\n"), prefix,((ht->bits.counts_offset) ? (1 << ht->bits.counts_width) : 0), (ht->bits.finalized ? "yes" : "no"));
        RSStringAppendStringWithFormat(result, RSSTR("%Rnum mutations = %ld, num deleted = %ld, size = %ld, total size = %ld,\n"), prefix, ht->bits.mutations, ht->bits.deleted, RSBasicHashGetSize(ht, NO), RSBasicHashGetSize(ht, YES));
        RSStringAppendStringWithFormat(result, RSSTR("%Rvalues ptr = %p, keys ptr = %p, counts ptr = %p, hashes ptr = %p,\n"), prefix, __RSBasicHashGetValues(ht), ((ht->bits.keys_offset) ? __RSBasicHashGetKeys(ht) : nil), ((ht->bits.counts_offset) ? __RSBasicHashGetCounts(ht) : nil), (__RSBasicHashHasHashCache(ht) ? __RSBasicHashGetHashes(ht) : nil));
    }
    RSStringAppendStringWithFormat(result, RSSTR("%Rentries =>\n"), prefix);
    RSBasicHashApply(ht, ^(RSBasicHashBucket bkt) {
        RSStringRef vDesc = nil, kDesc = nil;
        if (!describeElements) {
            vDesc = RSStringCreateWithFormat(RSAllocatorSystemDefault, RSSTR("<%p>"), (void *)bkt.weak_value);
            if (ht->bits.keys_offset) {
                kDesc = RSStringCreateWithFormat(RSAllocatorSystemDefault, RSSTR("<%p>"), (void *)bkt.weak_key);
            }
        } else {
            vDesc = ht->callbacks->copyValueDescription(ht, bkt.weak_value);
            if (ht->bits.keys_offset) {
                kDesc = ht->callbacks->copyKeyDescription(ht, bkt.weak_key);
            }
        }
        if (ht->bits.keys_offset && ht->bits.counts_offset) {
            RSStringAppendStringWithFormat(result, RSSTR("%R%ld : %R = %R (%ld)\n"), entryPrefix, bkt.idx, kDesc, vDesc, bkt.count);
        } else if (ht->bits.keys_offset) {
            RSStringAppendStringWithFormat(result, RSSTR("%R%ld : %R = %R\n"), entryPrefix, bkt.idx, kDesc, vDesc);
        } else if (ht->bits.counts_offset) {
            RSStringAppendStringWithFormat(result, RSSTR("%R%ld : %R (%ld)\n"), entryPrefix, bkt.idx, vDesc, bkt.count);
        } else {
            RSStringAppendStringWithFormat(result, RSSTR("%R%ld : %R\n"), entryPrefix, bkt.idx, vDesc);
        }
        if (kDesc) RSRelease(kDesc);
        if (vDesc) RSRelease(vDesc);
        return (BOOL)YES;
    });
    RSStringAppendStringWithFormat(result, RSSTR("%R}\n"), prefix);
    return result;
}

RSPrivate void RSBasicHashShow(RSConstBasicHashRef ht) {
    RSStringRef str = RSBasicHashDescription(ht, YES, RSSTR(""), RSSTR("\t"), NO);
#if defined(DEBUG)
    RSShow(str);
#endif
    RSRelease(str);
}

RSPrivate BOOL __RSBasicHashEqual(RSTypeRef obj1, RSTypeRef obj2) {
    RSBasicHashRef ht1 = (RSBasicHashRef)obj1;
    RSBasicHashRef ht2 = (RSBasicHashRef)obj2;
    //#warning this used to require that the key and value equal callbacks were pointer identical
    return RSBasicHashesAreEqual(ht1, ht2);
}

RSPrivate RSHashCode __RSBasicHashHash(RSTypeRef obj) {
    RSBasicHashRef ht = (RSBasicHashRef)obj;
    return RSBasicHashGetCount(ht);
}

RSPrivate RSStringRef __RSBasicHashDescription(RSTypeRef obj) {
    RSBasicHashRef ht = (RSBasicHashRef)obj;
    RSStringRef desc = RSBasicHashDescription(ht, NO, RSSTR(""), RSSTR("\t"), YES);
    RSStringRef result = RSStringCreateWithFormat(RSAllocatorSystemDefault, RSSTR("<RSBasicHash %p [%p]>%r"), obj, RSAllocatorSystemDefault, desc);
    RSRelease(desc);
    return result;
}

RSPrivate void __RSBasicHashDeallocate(RSTypeRef obj) {
    RSBasicHashRef ht = (RSBasicHashRef)obj;
    if (ht->bits.finalized) HALTWithError(RSInvalidArgumentException, "hash table is finilized");
    ht->bits.finalized = 1;
    __RSBasicHashDrain(ht, YES);
#if ENABLE_MEMORY_COUNTERS
    OSAtomicAdd64Barrier(-1, &__RSBasicHashTotalCount);
    OSAtomicAdd32Barrier(-1, &__RSBasicHashSizes[ht->bits.num_buckets_idx]);
#endif
}

static RSTypeID __RSBasicHashTypeID = _RSRuntimeNotATypeID;

static const RSRuntimeClass __RSBasicHashClass = {
    _RSRuntimeScannedObject,
    "RSBasicHash",
    nil,        // init
    nil,        // copy
    __RSBasicHashDeallocate,
    __RSBasicHashEqual,
    __RSBasicHashHash,
    __RSBasicHashDescription,        //
    nil,
    nil,
};

RSPrivate void __RSBasicHashInitialize()
{
    __RSBasicHashTypeID = __RSRuntimeRegisterClass(&__RSBasicHashClass);
    __RSRuntimeSetClassTypeID(&__RSBasicHashClass, __RSBasicHashTypeID);
}

RSPrivate RSTypeID RSBasicHashGetTypeID(void) {
    //if (_RSRuntimeNotATypeID == __RSBasicHashTypeID) __RSBasicHashTypeID = __RSRuntimeRegisterClass(&__RSBasicHashClass);
    return __RSBasicHashTypeID;
}

RSBasicHashRef RSBasicHashCreate(RSAllocatorRef allocator, RSOptionFlags flags, const RSBasicHashCallbacks *cb) {
    size_t size = sizeof(struct __RSBasicHash) - sizeof(RSRuntimeBase);
    if (flags & RSBasicHashHasKeys) size += sizeof(RSBasicHashValue *); // keys
    if (flags & RSBasicHashHasCounts) size += sizeof(void *); // counts
    if (flags & RSBasicHashHasHashCache) size += sizeof(uintptr_t *); // hashes
    RSBasicHashRef ht = (RSBasicHashRef)__RSRuntimeCreateInstance(allocator, RSBasicHashGetTypeID(), size);
    if (nil == ht) return nil;
    
    ht->bits.finalized = 0;
    ht->bits.hash_style = (flags >> 13) & 0x3;
    ht->bits.fast_grow = (flags & RSBasicHashAggressiveGrowth) ? 1 : 0;
    ht->bits.counts_width = 0;
    ht->bits.strong_values = (flags & RSBasicHashStrongValues) ? 1 : 0;
    ht->bits.strong_keys = (flags & RSBasicHashStrongKeys) ? 1 : 0;
    ht->bits.weak_values = (flags & RSBasicHashWeakValues) ? 1 : 0;
    ht->bits.weak_keys = (flags & RSBasicHashWeakKeys) ? 1 : 0;
    ht->bits.int_values = (flags & RSBasicHashIntegerValues) ? 1 : 0;
    ht->bits.int_keys = (flags & RSBasicHashIntegerKeys) ? 1 : 0;
    ht->bits.indirect_keys = (flags & RSBasicHashIndirectKeys) ? 1 : 0;
    ht->bits.compactable_keys = (flags & RSBasicHashCompactableKeys) ? 1 : 0;
    ht->bits.compactable_values = (flags & RSBasicHashCompactableValues) ? 1 : 0;
    ht->bits.__2 = 0;
    ht->bits.__8 = 0;
    ht->bits.__9 = 0;
    ht->bits.num_buckets_idx = 0;
    ht->bits.used_buckets = 0;
    ht->bits.deleted = 0;
    ht->bits.mutations = 1;
    
    if (ht->bits.strong_values && ht->bits.weak_values) HALTWithError(RSInvalidArgumentException, "flags are conflict");
    if (ht->bits.strong_values && ht->bits.int_values) HALTWithError(RSInvalidArgumentException, "flags are conflict");
    if (ht->bits.strong_keys && ht->bits.weak_keys) HALTWithError(RSInvalidArgumentException, "flags are conflict");
    if (ht->bits.strong_keys && ht->bits.int_keys) HALTWithError(RSInvalidArgumentException, "flags are conflict");
    if (ht->bits.weak_values && ht->bits.int_values) HALTWithError(RSInvalidArgumentException, "flags are conflict");
    if (ht->bits.weak_keys && ht->bits.int_keys) HALTWithError(RSInvalidArgumentException, "flags are conflict");
    if (ht->bits.indirect_keys && ht->bits.strong_keys) HALTWithError(RSInvalidArgumentException, "flags are conflict");
    if (ht->bits.indirect_keys && ht->bits.weak_keys) HALTWithError(RSInvalidArgumentException, "flags are conflict");
    if (ht->bits.indirect_keys && ht->bits.int_keys) HALTWithError(RSInvalidArgumentException, "flags are conflict");
    
    uint64_t offset = 1;
    ht->bits.keys_offset = (flags & RSBasicHashHasKeys) ? offset++ : 0;
    ht->bits.counts_offset = (flags & RSBasicHashHasCounts) ? offset++ : 0;
    ht->bits.hashes_offset = (flags & RSBasicHashHasHashCache) ? offset++ : 0;
    
#if DEPLOYMENT_TARGET_EMBEDDED || DEPLOYMENT_TARGET_EMBEDDED_MINI
    ht->bits.hashes_offset = 0;
    ht->bits.strong_values = 0;
    ht->bits.strong_keys = 0;
    ht->bits.weak_values = 0;
    ht->bits.weak_keys = 0;
#endif
    
    __AssignWithWriteBarrier(&ht->callbacks, cb);
    for (RSIndex idx = 0; idx < offset; idx++) {
        ht->pointers[idx] = nil;
    }
    
#if ENABLE_MEMORY_COUNTERS
    int64_t size_now = OSAtomicAdd64Barrier((int64_t) RSBasicHashGetSize(ht, YES), & __RSBasicHashTotalSize);
    while (__RSBasicHashPeakSize < size_now && !OSAtomicCompareAndSwap64Barrier(__RSBasicHashPeakSize, size_now, & __RSBasicHashPeakSize));
    int64_t count_now = OSAtomicAdd64Barrier(1, & __RSBasicHashTotalCount);
    while (__RSBasicHashPeakCount < count_now && !OSAtomicCompareAndSwap64Barrier(__RSBasicHashPeakCount, count_now, & __RSBasicHashPeakCount));
    OSAtomicAdd32Barrier(1, &__RSBasicHashSizes[ht->bits.num_buckets_idx]);
#endif
    
    return ht;
}
//#define RS_IS_COLLECTABLE_ALLOCATOR(allocator) (nil && (nil == (allocator) || RSAllocatorSystemDefault == (allocator) || NO))
RSBasicHashRef RSBasicHashCreateCopy(RSAllocatorRef allocator, RSConstBasicHashRef src_ht) {
    size_t size = RSBasicHashGetSize(src_ht, NO) - sizeof(RSRuntimeBase);
    RSBasicHashCallbacks *newcb = src_ht->callbacks->copyCallbacks(src_ht, allocator, src_ht->callbacks);
    if (nil == newcb) {
        return nil;
    }
    
    RSIndex new_num_buckets = __RSBasicHashTableSizes[src_ht->bits.num_buckets_idx];
    RSBasicHashValue *new_values = nil, *new_keys = nil;
    void *new_counts = nil;
    uintptr_t *new_hashes = nil;
    
    if (0 < new_num_buckets) {
        BOOL strongValues = RSBasicHashHasStrongValues(src_ht) && !(nil && !RS_IS_COLLECTABLE_ALLOCATOR(allocator));
        BOOL strongKeys = RSBasicHashHasStrongKeys(src_ht) && !(nil && !RS_IS_COLLECTABLE_ALLOCATOR(allocator));
        BOOL compactableValues = __RSBasicHashHasCompactableValues(src_ht) && !(nil && !RS_IS_COLLECTABLE_ALLOCATOR(allocator));
        new_values = (RSBasicHashValue *)__RSBasicHashAllocateMemory2(allocator, new_num_buckets, sizeof(RSBasicHashValue), strongValues, compactableValues);
        if (!new_values) return nil; // in this unusual circumstance, leak previously allocated blocks for now
        //__SetLastAllocationEventName(new_values, "RSBasicHash (value-store)");
        if (src_ht->bits.keys_offset) {
            new_keys = (RSBasicHashValue *)__RSBasicHashAllocateMemory2(allocator, new_num_buckets, sizeof(RSBasicHashValue), strongKeys, NO);
            if (!new_keys) return nil; // in this unusual circumstance, leak previously allocated blocks for now
            //__SetLastAllocationEventName(new_keys, "RSBasicHash (key-store)");
        }
        if (src_ht->bits.counts_offset) {
            new_counts = (uintptr_t *)__RSBasicHashAllocateMemory2(allocator, new_num_buckets, (1 << src_ht->bits.counts_width), NO, NO);
            if (!new_counts) return nil; // in this unusual circumstance, leak previously allocated blocks for now
            //__SetLastAllocationEventName(new_counts, "RSBasicHash (count-store)");
        }
        if (__RSBasicHashHasHashCache(src_ht)) {
            new_hashes = (uintptr_t *)__RSBasicHashAllocateMemory2(allocator, new_num_buckets, sizeof(uintptr_t), NO, NO);
            if (!new_hashes) return nil; // in this unusual circumstance, leak previously allocated blocks for now
            //__SetLastAllocationEventName(new_hashes, "RSBasicHash (hash-store)");
        }
    }
    
    RSBasicHashRef ht = (RSBasicHashRef)__RSRuntimeCreateInstance(allocator, RSBasicHashGetTypeID(), size);
    if (nil == ht) return nil; // in this unusual circumstance, leak previously allocated blocks for now
    
    memmove((uint8_t *)ht + sizeof(RSRuntimeBase), (uint8_t *)src_ht + sizeof(RSRuntimeBase), sizeof(ht->bits));
    if (nil && !RS_IS_COLLECTABLE_ALLOCATOR(allocator)) {
        ht->bits.strong_values = 0;
        ht->bits.strong_keys = 0;
        ht->bits.weak_values = 0;
        ht->bits.weak_keys = 0;
    }
    ht->bits.finalized = 0;
    ht->bits.mutations = 1;
    __AssignWithWriteBarrier(&ht->callbacks, newcb);
    
    if (0 == new_num_buckets) {
#if ENABLE_MEMORY_COUNTERS
        int64_t size_now = OSAtomicAdd64Barrier((int64_t) RSBasicHashGetSize(ht, YES), & __RSBasicHashTotalSize);
        while (__RSBasicHashPeakSize < size_now && !OSAtomicCompareAndSwap64Barrier(__RSBasicHashPeakSize, size_now, & __RSBasicHashPeakSize));
        int64_t count_now = OSAtomicAdd64Barrier(1, & __RSBasicHashTotalCount);
        while (__RSBasicHashPeakCount < count_now && !OSAtomicCompareAndSwap64Barrier(__RSBasicHashPeakCount, count_now, & __RSBasicHashPeakCount));
        OSAtomicAdd32Barrier(1, &__RSBasicHashSizes[ht->bits.num_buckets_idx]);
#endif
        return ht;
    }
    
    RSBasicHashValue *old_values = nil, *old_keys = nil;
    void *old_counts = nil;
    uintptr_t *old_hashes = nil;
    
    old_values = __RSBasicHashGetValues(src_ht);
    if (src_ht->bits.keys_offset) {
        old_keys = __RSBasicHashGetKeys(src_ht);
    }
    if (src_ht->bits.counts_offset) {
        old_counts = __RSBasicHashGetCounts(src_ht);
    }
    if (__RSBasicHashHasHashCache(src_ht)) {
        old_hashes = __RSBasicHashGetHashes(src_ht);
    }
    
    __RSBasicHashSetValues(ht, new_values);
    if (new_keys) {
        __RSBasicHashSetKeys(ht, new_keys);
    }
    if (new_counts) {
        __RSBasicHashSetCounts(ht, new_counts);
    }
    if (new_hashes) {
        __RSBasicHashSetHashes(ht, new_hashes);
    }
    
    for (RSIndex idx = 0; idx < new_num_buckets; idx++) {
        uintptr_t stack_value = old_values[idx].neutral;
        if (stack_value != 0UL && stack_value != ~0UL) {
            uintptr_t old_value = stack_value;
            if (__RSBasicHashSubABZero == old_value) old_value = 0UL;
            if (__RSBasicHashSubABOne == old_value) old_value = ~0UL;
            __RSBasicHashSetValue(ht, idx, __RSBasicHashImportValue(ht, old_value), YES, NO);
            if (new_keys) {
                uintptr_t old_key = old_keys[idx].neutral;
                if (__RSBasicHashSubABZero == old_key) old_key = 0UL;
                if (__RSBasicHashSubABOne == old_key) old_key = ~0UL;
                __RSBasicHashSetKey(ht, idx, __RSBasicHashImportKey(ht, old_key), YES, NO);
            }
        } else {
            __RSBasicHashSetValue(ht, idx, stack_value, YES, YES);
            if (new_keys) {
                __RSBasicHashSetKey(ht, idx, stack_value, YES, YES);
            }
        }
    }
    if (new_counts) memmove(new_counts, old_counts, new_num_buckets * (1 << ht->bits.counts_width));
    if (new_hashes) memmove(new_hashes, old_hashes, new_num_buckets * sizeof(uintptr_t));
    
#if ENABLE_MEMORY_COUNTERS
    int64_t size_now = OSAtomicAdd64Barrier((int64_t) RSBasicHashGetSize(ht, YES), & __RSBasicHashTotalSize);
    while (__RSBasicHashPeakSize < size_now && !OSAtomicCompareAndSwap64Barrier(__RSBasicHashPeakSize, size_now, & __RSBasicHashPeakSize));
    int64_t count_now = OSAtomicAdd64Barrier(1, & __RSBasicHashTotalCount);
    while (__RSBasicHashPeakCount < count_now && !OSAtomicCompareAndSwap64Barrier(__RSBasicHashPeakCount, count_now, & __RSBasicHashPeakCount));
    OSAtomicAdd32Barrier(1, &__RSBasicHashSizes[ht->bits.num_buckets_idx]);
#endif
    
    return ht;
}

RSPrivate void __RSBasicHashSetCallbacks(RSBasicHashRef ht, const RSBasicHashCallbacks *cb) {
    __AssignWithWriteBarrier(&ht->callbacks, cb);
}

void _RSbhx588461(RSBasicHashRef ht, BOOL growth) {
    if (!RSBasicHashIsMutable(ht)) HALTWithError(RSInvalidArgumentException, "hast table is not mutable");
    if (ht->bits.finalized) HALTWithError(RSInvalidArgumentException, "hash table is deallocte");
    ht->bits.fast_grow = growth ? 1 : 0;
}

#elif (RSBasicHashVersion == 2)

#ifndef HALT
#define HALT    __HALT()
#endif

#include "RSBasicHash.h"
#include <RSCoreFoundation/RSRuntime.h>
#include <RSCoreFoundation/RSSet.h>
#include <Block.h>
#include <math.h>

#if DEPLOYMENT_TARGET_MACOSX || DEPLOYMENT_TARGET_EMBEDDED
#include <dispatch/dispatch.h>
#endif

#if DEPLOYMENT_TARGET_MACOSX || DEPLOYMENT_TARGET_EMBEDDED
#define __SetLastAllocationEventName(A, B) do { } while (0)
// do { if (__RSOASafe && (A)) __RSSetLastAllocationEventName(A, B); } while (0)
#else
#define __SetLastAllocationEventName(A, B) do { } while (0)
#endif

#define __AssignWithWriteBarrier(location, value) objc_assign_strongCast((void*)value, (void* *)location)

#define ENABLE_DTRACE_PROBES 0
#define ENABLE_MEMORY_COUNTERS 0

#if defined(DTRACE_PROBES_DISABLED) && DTRACE_PROBES_DISABLED
#undef ENABLE_DTRACE_PROBES
#define ENABLE_DTRACE_PROBES 0
#endif

/*
 // dtrace -h -s foo.d
 // Note: output then changed by casts of the arguments
 // dtrace macros last generated 2010-09-08 on 10.7 prerelease (11A259)
 
 provider Cocoa_HashTable {
 probe hash_key(unsigned long table, unsigned long key, unsigned long hash);
 probe test_equal(unsigned long table, unsigned long key1, unsigned long key2);
 probe probing_start(unsigned long table, unsigned long num_buckets);
 probe probe_empty(unsigned long table, unsigned long idx);
 probe probe_deleted(unsigned long table, unsigned long idx);
 probe probe_valid(unsigned long table, unsigned long idx);
 probe probing_end(unsigned long table, unsigned long num_probes);
 probe rehash_start(unsigned long table, unsigned long num_buckets, unsigned long total_size);
 probe rehash_end(unsigned long table, unsigned long num_buckets, unsigned long total_size);
 };
 
 #pragma D attributes Unstable/Unstable/Common provider Cocoa_HashTable provider
 #pragma D attributes Private/Private/Unknown provider Cocoa_HashTable module
 #pragma D attributes Private/Private/Unknown provider Cocoa_HashTable function
 #pragma D attributes Unstable/Unstable/Common provider Cocoa_HashTable name
 #pragma D attributes Unstable/Unstable/Common provider Cocoa_HashTable args
 */

#if ENABLE_DTRACE_PROBES

#define COCOA_HASHTABLE_STABILITY "___dtrace_stability$Cocoa_HashTable$v1$4_4_5_1_1_0_1_1_0_4_4_5_4_4_5"

#define COCOA_HASHTABLE_TYPEDEFS "___dtrace_typedefs$Cocoa_HashTable$v2"

#define COCOA_HASHTABLE_REHASH_END(arg0, arg1, arg2) \
    do { \
        __asm__ volatile(".reference " COCOA_HASHTABLE_TYPEDEFS); \
        __dtrace_probe$Cocoa_HashTable$rehash_end$v1$756e7369676e6564206c6f6e67$756e7369676e6564206c6f6e67$756e7369676e6564206c6f6e67((unsigned long)(arg0), (unsigned long)(arg1), (unsigned long)(arg2)); \
        __asm__ volatile(".reference " COCOA_HASHTABLE_STABILITY); \
    } while (0)
#define	COCOA_HASHTABLE_REHASH_END_ENABLED() \
    ({ int _r = __dtrace_isenabled$Cocoa_HashTable$rehash_end$v1(); \
    __asm__ volatile(""); \
    _r; })
#define COCOA_HASHTABLE_REHASH_START(arg0, arg1, arg2) \
    do { \
        __asm__ volatile(".reference " COCOA_HASHTABLE_TYPEDEFS); \
        __dtrace_probe$Cocoa_HashTable$rehash_start$v1$756e7369676e6564206c6f6e67$756e7369676e6564206c6f6e67$756e7369676e6564206c6f6e67((unsigned long)(arg0), (unsigned long)(arg1), (unsigned long)(arg2)); \
        __asm__ volatile(".reference " COCOA_HASHTABLE_STABILITY); \
    } while (0)
#define	COCOA_HASHTABLE_REHASH_START_ENABLED() \
    ({ int _r = __dtrace_isenabled$Cocoa_HashTable$rehash_start$v1(); \
    __asm__ volatile(""); \
    _r; })
#define COCOA_HASHTABLE_HASH_KEY(arg0, arg1, arg2) \
    do { \
        __asm__ volatile(".reference " COCOA_HASHTABLE_TYPEDEFS); \
        __dtrace_probe$Cocoa_HashTable$hash_key$v1$756e7369676e6564206c6f6e67$756e7369676e6564206c6f6e67$756e7369676e6564206c6f6e67((unsigned long)(arg0), (unsigned long)(arg1), (unsigned long)(arg2)); \
        __asm__ volatile(".reference " COCOA_HASHTABLE_STABILITY); \
    } while (0)
#define	COCOA_HASHTABLE_HASH_KEY_ENABLED() \
    ({ int _r = __dtrace_isenabled$Cocoa_HashTable$hash_key$v1(); \
    __asm__ volatile(""); \
    _r; })
#define COCOA_HASHTABLE_PROBE_DELETED(arg0, arg1) \
    do { \
        __asm__ volatile(".reference " COCOA_HASHTABLE_TYPEDEFS); \
        __dtrace_probe$Cocoa_HashTable$probe_deleted$v1$756e7369676e6564206c6f6e67$756e7369676e6564206c6f6e67((unsigned long)(arg0), (unsigned long)(arg1)); \
        __asm__ volatile(".reference " COCOA_HASHTABLE_STABILITY); \
    } while (0)
#define	COCOA_HASHTABLE_PROBE_DELETED_ENABLED() \
    ({ int _r = __dtrace_isenabled$Cocoa_HashTable$probe_deleted$v1(); \
    __asm__ volatile(""); \
    _r; })
#define COCOA_HASHTABLE_PROBE_EMPTY(arg0, arg1) \
    do { \
        __asm__ volatile(".reference " COCOA_HASHTABLE_TYPEDEFS); \
        __dtrace_probe$Cocoa_HashTable$probe_empty$v1$756e7369676e6564206c6f6e67$756e7369676e6564206c6f6e67((unsigned long)(arg0), (unsigned long)(arg1)); \
        __asm__ volatile(".reference " COCOA_HASHTABLE_STABILITY); \
    } while (0)
#define	COCOA_HASHTABLE_PROBE_EMPTY_ENABLED() \
    ({ int _r = __dtrace_isenabled$Cocoa_HashTable$probe_empty$v1(); \
    __asm__ volatile(""); \
    _r; })
#define COCOA_HASHTABLE_PROBE_VALID(arg0, arg1) \
    do { \
        __asm__ volatile(".reference " COCOA_HASHTABLE_TYPEDEFS); \
        __dtrace_probe$Cocoa_HashTable$probe_valid$v1$756e7369676e6564206c6f6e67$756e7369676e6564206c6f6e67((unsigned long)(arg0), (unsigned long)(arg1)); \
        __asm__ volatile(".reference " COCOA_HASHTABLE_STABILITY); \
    } while (0)
#define	COCOA_HASHTABLE_PROBE_VALID_ENABLED() \
    ({ int _r = __dtrace_isenabled$Cocoa_HashTable$probe_valid$v1(); \
    __asm__ volatile(""); \
    _r; })
#define COCOA_HASHTABLE_PROBING_END(arg0, arg1) \
    do { \
        __asm__ volatile(".reference " COCOA_HASHTABLE_TYPEDEFS); \
        __dtrace_probe$Cocoa_HashTable$probing_end$v1$756e7369676e6564206c6f6e67$756e7369676e6564206c6f6e67((unsigned long)(arg0), (unsigned long)(arg1)); \
        __asm__ volatile(".reference " COCOA_HASHTABLE_STABILITY); \
    } while (0)
#define	COCOA_HASHTABLE_PROBING_END_ENABLED() \
    ({ int _r = __dtrace_isenabled$Cocoa_HashTable$probing_end$v1(); \
    __asm__ volatile(""); \
    _r; })
#define COCOA_HASHTABLE_PROBING_START(arg0, arg1) \
    do { \
        __asm__ volatile(".reference " COCOA_HASHTABLE_TYPEDEFS); \
        __dtrace_probe$Cocoa_HashTable$probing_start$v1$756e7369676e6564206c6f6e67$756e7369676e6564206c6f6e67((unsigned long)(arg0), (unsigned long)(arg1)); \
        __asm__ volatile(".reference " COCOA_HASHTABLE_STABILITY); \
    } while (0)
#define	COCOA_HASHTABLE_PROBING_START_ENABLED() \
    ({ int _r = __dtrace_isenabled$Cocoa_HashTable$probing_start$v1(); \
    __asm__ volatile(""); \
    _r; })
#define COCOA_HASHTABLE_TEST_EQUAL(arg0, arg1, arg2) \
    do { \
        __asm__ volatile(".reference " COCOA_HASHTABLE_TYPEDEFS); \
        __dtrace_probe$Cocoa_HashTable$test_equal$v1$756e7369676e6564206c6f6e67$756e7369676e6564206c6f6e67$756e7369676e6564206c6f6e67((unsigned long)(arg0), (unsigned long)(arg1), (unsigned long)(arg2)); \
        __asm__ volatile(".reference " COCOA_HASHTABLE_STABILITY); \
    } while (0)
#define	COCOA_HASHTABLE_TEST_EQUAL_ENABLED() \
    ({ int _r = __dtrace_isenabled$Cocoa_HashTable$test_equal$v1(); \
    __asm__ volatile(""); \
    _r; })

extern void __dtrace_probe$Cocoa_HashTable$hash_key$v1$756e7369676e6564206c6f6e67$756e7369676e6564206c6f6e67$756e7369676e6564206c6f6e67(unsigned long, unsigned long, unsigned long);
extern int __dtrace_isenabled$Cocoa_HashTable$hash_key$v1(void);
extern void __dtrace_probe$Cocoa_HashTable$probe_deleted$v1$756e7369676e6564206c6f6e67$756e7369676e6564206c6f6e67(unsigned long, unsigned long);
extern int __dtrace_isenabled$Cocoa_HashTable$probe_deleted$v1(void);
extern void __dtrace_probe$Cocoa_HashTable$probe_empty$v1$756e7369676e6564206c6f6e67$756e7369676e6564206c6f6e67(unsigned long, unsigned long);
extern int __dtrace_isenabled$Cocoa_HashTable$probe_empty$v1(void);
extern void __dtrace_probe$Cocoa_HashTable$probe_valid$v1$756e7369676e6564206c6f6e67$756e7369676e6564206c6f6e67(unsigned long, unsigned long);
extern int __dtrace_isenabled$Cocoa_HashTable$probe_valid$v1(void);
extern void __dtrace_probe$Cocoa_HashTable$probing_end$v1$756e7369676e6564206c6f6e67$756e7369676e6564206c6f6e67(unsigned long, unsigned long);
extern int __dtrace_isenabled$Cocoa_HashTable$probing_end$v1(void);
extern void __dtrace_probe$Cocoa_HashTable$probing_start$v1$756e7369676e6564206c6f6e67$756e7369676e6564206c6f6e67(unsigned long, unsigned long);
extern int __dtrace_isenabled$Cocoa_HashTable$probing_start$v1(void);
extern void __dtrace_probe$Cocoa_HashTable$rehash_end$v1$756e7369676e6564206c6f6e67$756e7369676e6564206c6f6e67$756e7369676e6564206c6f6e67(unsigned long, unsigned long, unsigned long);
extern int __dtrace_isenabled$Cocoa_HashTable$rehash_end$v1(void);
extern void __dtrace_probe$Cocoa_HashTable$rehash_start$v1$756e7369676e6564206c6f6e67$756e7369676e6564206c6f6e67$756e7369676e6564206c6f6e67(unsigned long, unsigned long, unsigned long);
extern int __dtrace_isenabled$Cocoa_HashTable$rehash_start$v1(void);
extern void __dtrace_probe$Cocoa_HashTable$test_equal$v1$756e7369676e6564206c6f6e67$756e7369676e6564206c6f6e67$756e7369676e6564206c6f6e67(unsigned long, unsigned long, unsigned long);
extern int __dtrace_isenabled$Cocoa_HashTable$test_equal$v1(void);

#else

#define COCOA_HASHTABLE_REHASH_END(arg0, arg1, arg2) do {} while (0)
#define COCOA_HASHTABLE_REHASH_END_ENABLED() 0
#define COCOA_HASHTABLE_REHASH_START(arg0, arg1, arg2) do {} while (0)
#define COCOA_HASHTABLE_REHASH_START_ENABLED() 0
#define COCOA_HASHTABLE_HASH_KEY(arg0, arg1, arg2) do {} while (0)
#define COCOA_HASHTABLE_HASH_KEY_ENABLED() 0
#define COCOA_HASHTABLE_PROBE_DELETED(arg0, arg1) do {} while (0)
#define COCOA_HASHTABLE_PROBE_DELETED_ENABLED() 0
#define COCOA_HASHTABLE_PROBE_EMPTY(arg0, arg1) do {} while (0)
#define COCOA_HASHTABLE_PROBE_EMPTY_ENABLED() 0
#define COCOA_HASHTABLE_PROBE_VALID(arg0, arg1) do {} while (0)
#define COCOA_HASHTABLE_PROBE_VALID_ENABLED() 0
#define COCOA_HASHTABLE_PROBING_END(arg0, arg1) do {} while (0)
#define COCOA_HASHTABLE_PROBING_END_ENABLED() 0
#define COCOA_HASHTABLE_PROBING_START(arg0, arg1) do {} while (0)
#define COCOA_HASHTABLE_PROBING_START_ENABLED() 0
#define COCOA_HASHTABLE_TEST_EQUAL(arg0, arg1, arg2) do {} while (0)
#define COCOA_HASHTABLE_TEST_EQUAL_ENABLED() 0

#endif


#if !defined(__LP64__)
#define __LP64__ 0
#endif

// Prime numbers. Values above 100 have been adjusted up so that the
// malloced block size will be just below a multiple of 512; values
// above 1200 have been adjusted up to just below a multiple of 4096.
static const uintptr_t __RSBasicHashTableSizes[64] = {
    0, 3, 7, 13, 23, 41, 71, 127, 191, 251, 383, 631, 1087, 1723,
    2803, 4523, 7351, 11959, 19447, 31231, 50683, 81919, 132607,
    214519, 346607, 561109, 907759, 1468927, 2376191, 3845119,
    6221311, 10066421, 16287743, 26354171, 42641881, 68996069,
    111638519, 180634607, 292272623, 472907251,
#if __LP64__
    765180413UL, 1238087663UL, 2003267557UL, 3241355263UL, 5244622819UL,
#if 0
    8485977589UL, 13730600407UL, 22216578047UL, 35947178479UL,
    58163756537UL, 94110934997UL, 152274691561UL, 246385626107UL,
    398660317687UL, 645045943807UL, 1043706260983UL, 1688752204787UL,
    2732458465769UL, 4421210670577UL, 7153669136377UL,
    11574879807461UL, 18728548943849UL, 30303428750843UL
#endif
#endif
};

static const uintptr_t __RSBasicHashTableCapacities[64] = {
    0, 3, 6, 11, 19, 32, 52, 85, 118, 155, 237, 390, 672, 1065,
    1732, 2795, 4543, 7391, 12019, 19302, 31324, 50629, 81956,
    132580, 214215, 346784, 561026, 907847, 1468567, 2376414,
    3844982, 6221390, 10066379, 16287773, 26354132, 42641916,
    68996399, 111638327, 180634415, 292272755,
#if __LP64__
    472907503UL, 765180257UL, 1238087439UL, 2003267722UL, 3241355160UL,
#if 0
    5244622578UL, 8485977737UL, 13730600347UL, 22216578100UL,
    35947178453UL, 58163756541UL, 94110935011UL, 152274691274UL,
    246385626296UL, 398660317578UL, 645045943559UL, 1043706261135UL,
    1688752204693UL, 2732458465840UL, 4421210670552UL,
    7153669136706UL, 11574879807265UL, 18728548943682UL
#endif
#endif
};

// Primitive roots for the primes above
static const uintptr_t __RSBasicHashPrimitiveRoots[64] = {
    0, 2, 3, 2, 5, 6, 7, 3, 19, 6, 5, 3, 3, 3,
    2, 5, 6, 3, 3, 6, 2, 3, 3,
    3, 5, 10, 3, 3, 22, 3,
    3, 3, 5, 2, 22, 2,
    11, 5, 5, 2,
#if __LP64__
    3, 10, 2, 3, 10,
    2, 3, 5, 3,
    3, 2, 7, 2,
    3, 3, 3, 2,
    3, 5, 5,
    2, 3, 2
#endif
};

RSInline void *__RSBasicHashAllocateMemory(RSConstBasicHashRef ht, RSIndex count, RSIndex elem_size, BOOL strong, BOOL compactable) {
    RSAllocatorRef allocator = RSGetAllocator(ht);
    void *new_mem = nil;
    new_mem = RSAllocatorAllocate(allocator, count * elem_size);
    return new_mem;
}

RSInline void *__RSBasicHashAllocateMemory2(RSAllocatorRef allocator, RSIndex count, RSIndex elem_size, BOOL strong, BOOL compactable) {
    void *new_mem = nil;
    new_mem = RSAllocatorAllocate(allocator, count * elem_size);
    return new_mem;
}

#define __RSBasicHashSubABZero 0xa7baadb1
#define __RSBasicHashSubABOne 0xa5baadb9

typedef union {
    uintptr_t neutral;
    void* strong;
    void* weak;
} RSBasicHashValue;

struct __RSBasicHash {
    RSRuntimeBase base;
    struct { // 192 bits
        uint16_t mutations;
        uint8_t hash_style:2;
        uint8_t keys_offset:1;
        uint8_t counts_offset:2;
        uint8_t counts_width:2;
        uint8_t hashes_offset:2;
        uint8_t strong_values:1;
        uint8_t strong_keys:1;
        uint8_t weak_values:1;
        uint8_t weak_keys:1;
        uint8_t int_values:1;
        uint8_t int_keys:1;
        uint8_t indirect_keys:1;
        uint32_t used_buckets;      /* number of used buckets */
        uint64_t deleted:16;
        uint64_t num_buckets_idx:8; /* index to number of buckets */
        uint64_t __kret:10;
        uint64_t __vret:10;
        uint64_t __krel:10;
        uint64_t __vrel:10;
        uint64_t __:1;
        uint64_t null_rc:1;
        uint64_t fast_grow:1;
        uint64_t finalized:1;
        uint64_t __kdes:10;
        uint64_t __vdes:10;
        uint64_t __kequ:10;
        uint64_t __vequ:10;
        uint64_t __khas:10;
        uint64_t __kget:10;
    } bits;
    void *pointers[1];
};

static void *RSBasicHashCallBackPtrs[(1UL << 10)];
static int32_t RSBasicHashCallBackPtrsCount = 0;

static int32_t RSBasicHashGetPtrIndex(void *ptr) {
    static dispatch_once_t once;
    dispatch_once(&once, ^{
        RSBasicHashCallBackPtrs[0] = nil;
        RSBasicHashCallBackPtrs[1] = (void *)RSDescription;
        RSBasicHashCallBackPtrs[2] = (void *)__RSTypeCollectionRelease;
        RSBasicHashCallBackPtrs[3] = (void *)__RSTypeCollectionRetain;
        RSBasicHashCallBackPtrs[4] = (void *)RSEqual;
        RSBasicHashCallBackPtrs[5] = (void *)RSHash;
        RSBasicHashCallBackPtrs[6] = (void *)__RSStringCollectionCopy;
        RSBasicHashCallBackPtrs[7] = nil;
        RSBasicHashCallBackPtrsCount = 8;
    });
    
    // The uniquing here is done locklessly for best performance, and in
    // a way that will keep multiple threads from stomping each other's
    // newly registered values, but may result in multiple slots
    // containing the same pointer value.
    
    int32_t idx;
    for (idx = 0; idx < RSBasicHashCallBackPtrsCount; idx++) {
        if (RSBasicHashCallBackPtrs[idx] == ptr) return idx;
    }
    
    if (1000 < RSBasicHashCallBackPtrsCount) HALT;
    idx = OSAtomicIncrement32(&RSBasicHashCallBackPtrsCount); // returns new value
    RSBasicHashCallBackPtrs[idx - 1] = ptr;
    return idx - 1;
}

RSPrivate BOOL RSBasicHashHasStrongValues(RSConstBasicHashRef ht) {
#if DEPLOYMENT_TARGET_MACOSX
    return ht->bits.strong_values ? true : false;
#else
    return false;
#endif
}

RSPrivate BOOL RSBasicHashHasStrongKeys(RSConstBasicHashRef ht) {
#if DEPLOYMENT_TARGET_MACOSX
    return ht->bits.strong_keys ? true : false;
#else
    return false;
#endif
}

RSInline BOOL __RSBasicHashHasWeakValues(RSConstBasicHashRef ht) {
#if DEPLOYMENT_TARGET_MACOSX
    return ht->bits.weak_values ? true : false;
#else
    return false;
#endif
}

RSInline BOOL __RSBasicHashHasWeakKeys(RSConstBasicHashRef ht) {
#if DEPLOYMENT_TARGET_MACOSX
    return ht->bits.weak_keys ? true : false;
#else
    return false;
#endif
}

RSInline BOOL __RSBasicHashHasHashCache(RSConstBasicHashRef ht) {
#if DEPLOYMENT_TARGET_MACOSX
    return ht->bits.hashes_offset ? true : false;
#else
    return false;
#endif
}

RSInline uintptr_t __RSBasicHashImportValue(RSConstBasicHashRef ht, uintptr_t stack_value) {
    uintptr_t (*func)(RSAllocatorRef, uintptr_t) = (uintptr_t (*)(RSAllocatorRef, uintptr_t))RSBasicHashCallBackPtrs[ht->bits.__vret];
    if (!func || ht->bits.null_rc) return stack_value;
    RSAllocatorRef alloc = RSGetAllocator(ht);
    return func(alloc, stack_value);
}

RSInline uintptr_t __RSBasicHashImportKey(RSConstBasicHashRef ht, uintptr_t stack_key) {
    uintptr_t (*func)(RSAllocatorRef, uintptr_t) = (uintptr_t (*)(RSAllocatorRef, uintptr_t))RSBasicHashCallBackPtrs[ht->bits.__kret];
    if (!func || ht->bits.null_rc) return stack_key;
    RSAllocatorRef alloc = RSGetAllocator(ht);
    return func(alloc, stack_key);
}

RSInline void __RSBasicHashEjectValue(RSConstBasicHashRef ht, uintptr_t stack_value) {
    void (*func)(RSAllocatorRef, uintptr_t) = (void (*)(RSAllocatorRef, uintptr_t))RSBasicHashCallBackPtrs[ht->bits.__vrel];
    if (!func || ht->bits.null_rc) return;
    RSAllocatorRef alloc = RSGetAllocator(ht);
    func(alloc, stack_value);
}

RSInline void __RSBasicHashEjectKey(RSConstBasicHashRef ht, uintptr_t stack_key) {
    void (*func)(RSAllocatorRef, uintptr_t) = (void (*)(RSAllocatorRef, uintptr_t))RSBasicHashCallBackPtrs[ht->bits.__krel];
    if (!func || ht->bits.null_rc) return;
    RSAllocatorRef alloc = RSGetAllocator(ht);
    func(alloc, stack_key);
}

RSInline RSStringRef __RSBasicHashDescValue(RSConstBasicHashRef ht, uintptr_t stack_value) {
    RSStringRef (*func)(uintptr_t) = (RSStringRef (*)(uintptr_t))RSBasicHashCallBackPtrs[ht->bits.__vdes];
    if (!func) return RSStringCreateWithFormat(RSAllocatorSystemDefault, RSSTR("<%p>"), (void *)stack_value);
    return func(stack_value);
}

RSInline RSStringRef __RSBasicHashDescKey(RSConstBasicHashRef ht, uintptr_t stack_key) {
    RSStringRef (*func)(uintptr_t) = (RSStringRef (*)(uintptr_t))RSBasicHashCallBackPtrs[ht->bits.__kdes];
    if (!func) return RSStringCreateWithFormat(RSAllocatorSystemDefault, RSSTR("<%p>"), (void *)stack_key);
    return func(stack_key);
}

RSInline BOOL __RSBasicHashTestEqualValue(RSConstBasicHashRef ht, uintptr_t stack_value_a, uintptr_t stack_value_b) {
    BOOL (*func)(uintptr_t, uintptr_t) = (BOOL (*)(uintptr_t, uintptr_t))RSBasicHashCallBackPtrs[ht->bits.__vequ];
    if (!func) return (stack_value_a == stack_value_b);
    return func(stack_value_a, stack_value_b);
}

RSInline BOOL __RSBasicHashTestEqualKey(RSConstBasicHashRef ht, uintptr_t in_coll_key, uintptr_t stack_key) {
    COCOA_HASHTABLE_TEST_EQUAL(ht, in_coll_key, stack_key);
    BOOL (*func)(uintptr_t, uintptr_t) = (BOOL (*)(uintptr_t, uintptr_t))RSBasicHashCallBackPtrs[ht->bits.__kequ];
    if (!func) return (in_coll_key == stack_key);
    return func(in_coll_key, stack_key);
}

RSInline RSHashCode __RSBasicHashHashKey(RSConstBasicHashRef ht, uintptr_t stack_key) {
    RSHashCode (*func)(uintptr_t) = (RSHashCode (*)(uintptr_t))RSBasicHashCallBackPtrs[ht->bits.__khas];
    RSHashCode hash_code = func ? func(stack_key) : stack_key;
    COCOA_HASHTABLE_HASH_KEY(ht, stack_key, hash_code);
    return hash_code;
}

RSInline uintptr_t __RSBasicHashGetIndirectKey(RSConstBasicHashRef ht, uintptr_t coll_key) {
    uintptr_t (*func)(uintptr_t) = (uintptr_t (*)(uintptr_t))RSBasicHashCallBackPtrs[ht->bits.__kget];
    if (!func) return coll_key;
    return func(coll_key);
}

RSInline RSBasicHashValue *__RSBasicHashGetValues(RSConstBasicHashRef ht) {
    return (RSBasicHashValue *)ht->pointers[0];
}

RSInline void __RSBasicHashSetValues(RSBasicHashRef ht, RSBasicHashValue *ptr) {
    __AssignWithWriteBarrier(&ht->pointers[0], ptr);
}

RSInline RSBasicHashValue *__RSBasicHashGetKeys(RSConstBasicHashRef ht) {
    return (RSBasicHashValue *)ht->pointers[ht->bits.keys_offset];
}

RSInline void __RSBasicHashSetKeys(RSBasicHashRef ht, RSBasicHashValue *ptr) {
    __AssignWithWriteBarrier(&ht->pointers[ht->bits.keys_offset], ptr);
}

RSInline void *__RSBasicHashGetCounts(RSConstBasicHashRef ht) {
    return (void *)ht->pointers[ht->bits.counts_offset];
}

RSInline void __RSBasicHashSetCounts(RSBasicHashRef ht, void *ptr) {
    __AssignWithWriteBarrier(&ht->pointers[ht->bits.counts_offset], ptr);
}

RSInline uintptr_t __RSBasicHashGetValue(RSConstBasicHashRef ht, RSIndex idx) {
    uintptr_t val = __RSBasicHashGetValues(ht)[idx].neutral;
    if (__RSBasicHashSubABZero == val) return 0UL;
    if (__RSBasicHashSubABOne == val) return ~0UL;
    return val;
}

RSInline void __RSBasicHashSetValue(RSBasicHashRef ht, RSIndex idx, uintptr_t stack_value, BOOL ignoreOld, BOOL literal) {
    RSBasicHashValue *valuep = &(__RSBasicHashGetValues(ht)[idx]);
    uintptr_t old_value = ignoreOld ? 0 : valuep->neutral;
    if (!literal) {
        if (0UL == stack_value) stack_value = __RSBasicHashSubABZero;
        if (~0UL == stack_value) stack_value = __RSBasicHashSubABOne;
    }
    if (RSBasicHashHasStrongValues(ht)) valuep->strong = (void*)stack_value; else valuep->neutral = stack_value;
    if (!ignoreOld) {
        if (!(old_value == 0UL || old_value == ~0UL)) {
            if (__RSBasicHashSubABZero == old_value) old_value = 0UL;
            if (__RSBasicHashSubABOne == old_value) old_value = ~0UL;
            __RSBasicHashEjectValue(ht, old_value);
        }
    }
}

RSInline uintptr_t __RSBasicHashGetKey(RSConstBasicHashRef ht, RSIndex idx) {
    if (ht->bits.keys_offset) {
        uintptr_t key = __RSBasicHashGetKeys(ht)[idx].neutral;
        if (__RSBasicHashSubABZero == key) return 0UL;
        if (__RSBasicHashSubABOne == key) return ~0UL;
        return key;
    }
    if (ht->bits.indirect_keys) {
        uintptr_t stack_value = __RSBasicHashGetValue(ht, idx);
        return __RSBasicHashGetIndirectKey(ht, stack_value);
    }
    return __RSBasicHashGetValue(ht, idx);
}

RSInline void __RSBasicHashSetKey(RSBasicHashRef ht, RSIndex idx, uintptr_t stack_key, BOOL ignoreOld, BOOL literal) {
    if (0 == ht->bits.keys_offset) HALT;
    RSBasicHashValue *keyp = &(__RSBasicHashGetKeys(ht)[idx]);
    uintptr_t old_key = ignoreOld ? 0 : keyp->neutral;
    if (!literal) {
        if (0UL == stack_key) stack_key = __RSBasicHashSubABZero;
        if (~0UL == stack_key) stack_key = __RSBasicHashSubABOne;
    }
    if (RSBasicHashHasStrongKeys(ht)) keyp->strong = (void*)stack_key; else keyp->neutral = stack_key;
    if (!ignoreOld) {
        if (!(old_key == 0UL || old_key == ~0UL)) {
            if (__RSBasicHashSubABZero == old_key) old_key = 0UL;
            if (__RSBasicHashSubABOne == old_key) old_key = ~0UL;
            __RSBasicHashEjectKey(ht, old_key);
        }
    }
}

RSInline uintptr_t __RSBasicHashIsEmptyOrDeleted(RSConstBasicHashRef ht, RSIndex idx) {
    uintptr_t stack_value = __RSBasicHashGetValues(ht)[idx].neutral;
    return (0UL == stack_value || ~0UL == stack_value);
}

RSInline uintptr_t __RSBasicHashIsDeleted(RSConstBasicHashRef ht, RSIndex idx) {
    uintptr_t stack_value = __RSBasicHashGetValues(ht)[idx].neutral;
    return (~0UL == stack_value);
}

RSInline uintptr_t __RSBasicHashGetSlotCount(RSConstBasicHashRef ht, RSIndex idx) {
    void *counts = __RSBasicHashGetCounts(ht);
    switch (ht->bits.counts_width) {
        case 0: return ((uint8_t *)counts)[idx];
        case 1: return ((uint16_t *)counts)[idx];
        case 2: return ((uint32_t *)counts)[idx];
        case 3: return ((uint64_t *)counts)[idx];
    }
    return 0;
}

RSInline void __RSBasicHashBumpCounts(RSBasicHashRef ht) {
    void *counts = __RSBasicHashGetCounts(ht);
    RSAllocatorRef allocator = RSGetAllocator(ht);
    switch (ht->bits.counts_width) {
        case 0: {
            uint8_t *counts08 = (uint8_t *)counts;
            ht->bits.counts_width = 1;
            RSIndex num_buckets = __RSBasicHashTableSizes[ht->bits.num_buckets_idx];
            uint16_t *counts16 = (uint16_t *)__RSBasicHashAllocateMemory(ht, num_buckets, 2, false, false);
            if (!counts16) HALT;
            __SetLastAllocationEventName(counts16, "RSBasicHash (count-store)");
            for (RSIndex idx2 = 0; idx2 < num_buckets; idx2++) {
                counts16[idx2] = counts08[idx2];
            }
            __RSBasicHashSetCounts(ht, counts16);
            if (!RS_IS_COLLECTABLE_ALLOCATOR(allocator)) {
                RSAllocatorDeallocate(allocator, counts08);
            }
            break;
        }
        case 1: {
            uint16_t *counts16 = (uint16_t *)counts;
            ht->bits.counts_width = 2;
            RSIndex num_buckets = __RSBasicHashTableSizes[ht->bits.num_buckets_idx];
            uint32_t *counts32 = (uint32_t *)__RSBasicHashAllocateMemory(ht, num_buckets, 4, false, false);
            if (!counts32) HALT;
            __SetLastAllocationEventName(counts32, "RSBasicHash (count-store)");
            for (RSIndex idx2 = 0; idx2 < num_buckets; idx2++) {
                counts32[idx2] = counts16[idx2];
            }
            __RSBasicHashSetCounts(ht, counts32);
            if (!RS_IS_COLLECTABLE_ALLOCATOR(allocator)) {
                RSAllocatorDeallocate(allocator, counts16);
            }
            break;
        }
        case 2: {
            uint32_t *counts32 = (uint32_t *)counts;
            ht->bits.counts_width = 3;
            RSIndex num_buckets = __RSBasicHashTableSizes[ht->bits.num_buckets_idx];
            uint64_t *counts64 = (uint64_t *)__RSBasicHashAllocateMemory(ht, num_buckets, 8, false, false);
            if (!counts64) HALT;
            __SetLastAllocationEventName(counts64, "RSBasicHash (count-store)");
            for (RSIndex idx2 = 0; idx2 < num_buckets; idx2++) {
                counts64[idx2] = counts32[idx2];
            }
            __RSBasicHashSetCounts(ht, counts64);
            if (!RS_IS_COLLECTABLE_ALLOCATOR(allocator)) {
                RSAllocatorDeallocate(allocator, counts32);
            }
            break;
        }
        case 3: {
            HALT;
            break;
        }
    }
}

static void __RSBasicHashIncSlotCount(RSBasicHashRef ht, RSIndex idx) {
    void *counts = __RSBasicHashGetCounts(ht);
    switch (ht->bits.counts_width) {
        case 0: {
            uint8_t *counts08 = (uint8_t *)counts;
            uint8_t val = counts08[idx];
            if (val < INT8_MAX) {
                counts08[idx] = val + 1;
                return;
            }
            __RSBasicHashBumpCounts(ht);
            __RSBasicHashIncSlotCount(ht, idx);
            break;
        }
        case 1: {
            uint16_t *counts16 = (uint16_t *)counts;
            uint16_t val = counts16[idx];
            if (val < INT16_MAX) {
                counts16[idx] = val + 1;
                return;
            }
            __RSBasicHashBumpCounts(ht);
            __RSBasicHashIncSlotCount(ht, idx);
            break;
        }
        case 2: {
            uint32_t *counts32 = (uint32_t *)counts;
            uint32_t val = counts32[idx];
            if (val < INT32_MAX) {
                counts32[idx] = val + 1;
                return;
            }
            __RSBasicHashBumpCounts(ht);
            __RSBasicHashIncSlotCount(ht, idx);
            break;
        }
        case 3: {
            uint64_t *counts64 = (uint64_t *)counts;
            uint64_t val = counts64[idx];
            if (val < INT64_MAX) {
                counts64[idx] = val + 1;
                return;
            }
            __RSBasicHashBumpCounts(ht);
            __RSBasicHashIncSlotCount(ht, idx);
            break;
        }
    }
}

RSInline void __RSBasicHashDecSlotCount(RSBasicHashRef ht, RSIndex idx) {
    void *counts = __RSBasicHashGetCounts(ht);
    switch (ht->bits.counts_width) {
        case 0: ((uint8_t  *)counts)[idx]--; return;
        case 1: ((uint16_t *)counts)[idx]--; return;
        case 2: ((uint32_t *)counts)[idx]--; return;
        case 3: ((uint64_t *)counts)[idx]--; return;
    }
}

RSInline uintptr_t *__RSBasicHashGetHashes(RSConstBasicHashRef ht) {
    return (uintptr_t *)ht->pointers[ht->bits.hashes_offset];
}

RSInline void __RSBasicHashSetHashes(RSBasicHashRef ht, uintptr_t *ptr) {
    __AssignWithWriteBarrier(&ht->pointers[ht->bits.hashes_offset], ptr);
}


// to expose the load factor, expose this function to customization
RSInline RSIndex __RSBasicHashGetCapacityForNumBuckets(RSConstBasicHashRef ht, RSIndex num_buckets_idx) {
    return __RSBasicHashTableCapacities[num_buckets_idx];
}

RSInline RSIndex __RSBasicHashGetNumBucketsIndexForCapacity(RSConstBasicHashRef ht, RSIndex capacity) {
    for (RSIndex idx = 0; idx < 64; idx++) {
        if (capacity <= __RSBasicHashGetCapacityForNumBuckets(ht, idx)) return idx;
    }
    HALT;
    return 0;
}

RSPrivate RSIndex RSBasicHashGetNumBuckets(RSConstBasicHashRef ht) {
    return __RSBasicHashTableSizes[ht->bits.num_buckets_idx];
}

RSPrivate RSIndex RSBasicHashGetCapacity(RSConstBasicHashRef ht) {
    return __RSBasicHashGetCapacityForNumBuckets(ht, ht->bits.num_buckets_idx);
}

// In returned struct, .count is zero if the bucket is empty or deleted,
// and the .weak_key field indicates which. .idx is either the index of
// the found bucket or the index of the bucket which should be filled by
// an add operation. For a set or multiset, the .weak_key and .weak_value
// are the same.
RSPrivate RSBasicHashBucket RSBasicHashGetBucket(RSConstBasicHashRef ht, RSIndex idx) {
    RSBasicHashBucket result;
    result.idx = idx;
    if (__RSBasicHashIsEmptyOrDeleted(ht, idx)) {
        result.count = 0;
        result.weak_value = 0;
        result.weak_key = 0;
    } else {
        result.count = (ht->bits.counts_offset) ? __RSBasicHashGetSlotCount(ht, idx) : 1;
        result.weak_value = __RSBasicHashGetValue(ht, idx);
        result.weak_key = __RSBasicHashGetKey(ht, idx);
    }
    return result;
}

#if defined(__arm__)
static uintptr_t __RSBasicHashFold(uintptr_t dividend, uint8_t idx) {
    switch (idx) {
        case 1: return dividend % 3;
        case 2: return dividend % 7;
        case 3: return dividend % 13;
        case 4: return dividend % 23;
        case 5: return dividend % 41;
        case 6: return dividend % 71;
        case 7: return dividend % 127;
        case 8: return dividend % 191;
        case 9: return dividend % 251;
        case 10: return dividend % 383;
        case 11: return dividend % 631;
        case 12: return dividend % 1087;
        case 13: return dividend % 1723;
        case 14: return dividend % 2803;
        case 15: return dividend % 4523;
        case 16: return dividend % 7351;
        case 17: return dividend % 11959;
        case 18: return dividend % 19447;
        case 19: return dividend % 31231;
        case 20: return dividend % 50683;
        case 21: return dividend % 81919;
        case 22: return dividend % 132607;
        case 23: return dividend % 214519;
        case 24: return dividend % 346607;
        case 25: return dividend % 561109;
        case 26: return dividend % 907759;
        case 27: return dividend % 1468927;
        case 28: return dividend % 2376191;
        case 29: return dividend % 3845119;
        case 30: return dividend % 6221311;
        case 31: return dividend % 10066421;
        case 32: return dividend % 16287743;
        case 33: return dividend % 26354171;
        case 34: return dividend % 42641881;
        case 35: return dividend % 68996069;
        case 36: return dividend % 111638519;
        case 37: return dividend % 180634607;
        case 38: return dividend % 292272623;
        case 39: return dividend % 472907251;
    }
    HALT;
    return ~0;
}
#endif


#define FIND_BUCKET_NAME		___RSBasicHashFindBucket_Linear
#define FIND_BUCKET_HASH_STYLE		1
#define FIND_BUCKET_FOR_REHASH		0
#define FIND_BUCKET_FOR_INDIRECT_KEY	0
#include "RSBasicHashFindBucket.m"

#define FIND_BUCKET_NAME		___RSBasicHashFindBucket_Linear_NoCollision
#define FIND_BUCKET_HASH_STYLE		1
#define FIND_BUCKET_FOR_REHASH		1
#define FIND_BUCKET_FOR_INDIRECT_KEY	0
#include "RSBasicHashFindBucket.m"

#define FIND_BUCKET_NAME		___RSBasicHashFindBucket_Linear_Indirect
#define FIND_BUCKET_HASH_STYLE		1
#define FIND_BUCKET_FOR_REHASH		0
#define FIND_BUCKET_FOR_INDIRECT_KEY	1
#include "RSBasicHashFindBucket.m"

#define FIND_BUCKET_NAME		___RSBasicHashFindBucket_Linear_Indirect_NoCollision
#define FIND_BUCKET_HASH_STYLE		1
#define FIND_BUCKET_FOR_REHASH		1
#define FIND_BUCKET_FOR_INDIRECT_KEY	1
#include "RSBasicHashFindBucket.m"

#define FIND_BUCKET_NAME		___RSBasicHashFindBucket_Double
#define FIND_BUCKET_HASH_STYLE		2
#define FIND_BUCKET_FOR_REHASH		0
#define FIND_BUCKET_FOR_INDIRECT_KEY	0
#include "RSBasicHashFindBucket.m"

#define FIND_BUCKET_NAME		___RSBasicHashFindBucket_Double_NoCollision
#define FIND_BUCKET_HASH_STYLE		2
#define FIND_BUCKET_FOR_REHASH		1
#define FIND_BUCKET_FOR_INDIRECT_KEY	0
#include "RSBasicHashFindBucket.m"

#define FIND_BUCKET_NAME		___RSBasicHashFindBucket_Double_Indirect
#define FIND_BUCKET_HASH_STYLE		2
#define FIND_BUCKET_FOR_REHASH		0
#define FIND_BUCKET_FOR_INDIRECT_KEY	1
#include "RSBasicHashFindBucket.m"

#define FIND_BUCKET_NAME		___RSBasicHashFindBucket_Double_Indirect_NoCollision
#define FIND_BUCKET_HASH_STYLE		2
#define FIND_BUCKET_FOR_REHASH		1
#define FIND_BUCKET_FOR_INDIRECT_KEY	1
#include "RSBasicHashFindBucket.m"

#define FIND_BUCKET_NAME		___RSBasicHashFindBucket_Exponential
#define FIND_BUCKET_HASH_STYLE		3
#define FIND_BUCKET_FOR_REHASH		0
#define FIND_BUCKET_FOR_INDIRECT_KEY	0
#include "RSBasicHashFindBucket.m"

#define FIND_BUCKET_NAME		___RSBasicHashFindBucket_Exponential_NoCollision
#define FIND_BUCKET_HASH_STYLE		3
#define FIND_BUCKET_FOR_REHASH		1
#define FIND_BUCKET_FOR_INDIRECT_KEY	0
#include "RSBasicHashFindBucket.m"

#define FIND_BUCKET_NAME		___RSBasicHashFindBucket_Exponential_Indirect
#define FIND_BUCKET_HASH_STYLE		3
#define FIND_BUCKET_FOR_REHASH		0
#define FIND_BUCKET_FOR_INDIRECT_KEY	1
#include "RSBasicHashFindBucket.m"

#define FIND_BUCKET_NAME		___RSBasicHashFindBucket_Exponential_Indirect_NoCollision
#define FIND_BUCKET_HASH_STYLE		3
#define FIND_BUCKET_FOR_REHASH		1
#define FIND_BUCKET_FOR_INDIRECT_KEY	1
#include "RSBasicHashFindBucket.m"


RSInline RSBasicHashBucket __RSBasicHashFindBucket(RSConstBasicHashRef ht, uintptr_t stack_key) {
    if (0 == ht->bits.num_buckets_idx) {
        RSBasicHashBucket result = {RSNotFound, 0UL, 0UL, 0};
        return result;
    }
    if (ht->bits.indirect_keys) {
        switch (ht->bits.hash_style) {
            case __RSBasicHashLinearHashingValue: return ___RSBasicHashFindBucket_Linear_Indirect(ht, stack_key);
            case __RSBasicHashDoubleHashingValue: return ___RSBasicHashFindBucket_Double_Indirect(ht, stack_key);
            case __RSBasicHashExponentialHashingValue: return ___RSBasicHashFindBucket_Exponential_Indirect(ht, stack_key);
        }
    } else {
        switch (ht->bits.hash_style) {
            case __RSBasicHashLinearHashingValue: return ___RSBasicHashFindBucket_Linear(ht, stack_key);
            case __RSBasicHashDoubleHashingValue: return ___RSBasicHashFindBucket_Double(ht, stack_key);
            case __RSBasicHashExponentialHashingValue: return ___RSBasicHashFindBucket_Exponential(ht, stack_key);
        }
    }
    HALT;
    RSBasicHashBucket result = {RSNotFound, 0UL, 0UL, 0};
    return result;
}

RSInline RSIndex __RSBasicHashFindBucket_NoCollision(RSConstBasicHashRef ht, uintptr_t stack_key, uintptr_t key_hash) {
    if (0 == ht->bits.num_buckets_idx) {
        return RSNotFound;
    }
    if (ht->bits.indirect_keys) {
        switch (ht->bits.hash_style) {
            case __RSBasicHashLinearHashingValue: return ___RSBasicHashFindBucket_Linear_Indirect_NoCollision(ht, stack_key, key_hash);
            case __RSBasicHashDoubleHashingValue: return ___RSBasicHashFindBucket_Double_Indirect_NoCollision(ht, stack_key, key_hash);
            case __RSBasicHashExponentialHashingValue: return ___RSBasicHashFindBucket_Exponential_Indirect_NoCollision(ht, stack_key, key_hash);
        }
    } else {
        switch (ht->bits.hash_style) {
            case __RSBasicHashLinearHashingValue: return ___RSBasicHashFindBucket_Linear_NoCollision(ht, stack_key, key_hash);
            case __RSBasicHashDoubleHashingValue: return ___RSBasicHashFindBucket_Double_NoCollision(ht, stack_key, key_hash);
            case __RSBasicHashExponentialHashingValue: return ___RSBasicHashFindBucket_Exponential_NoCollision(ht, stack_key, key_hash);
        }
    }
    HALT;
    return RSNotFound;
}

RSPrivate RSBasicHashBucket RSBasicHashFindBucket(RSConstBasicHashRef ht, uintptr_t stack_key) {
    if (__RSBasicHashSubABZero == stack_key || __RSBasicHashSubABOne == stack_key) {
        RSBasicHashBucket result = {RSNotFound, 0UL, 0UL, 0};
        return result;
    }
    return __RSBasicHashFindBucket(ht, stack_key);
}

RSPrivate void RSBasicHashSuppressRC(RSBasicHashRef ht) {
    ht->bits.null_rc = 1;
}

RSPrivate void RSBasicHashUnsuppressRC(RSBasicHashRef ht) {
    ht->bits.null_rc = 0;
}

RSPrivate RSOptionFlags RSBasicHashGetFlags(RSConstBasicHashRef ht) {
    RSOptionFlags flags = (ht->bits.hash_style << 13);
    if (RSBasicHashHasStrongValues(ht)) flags |= RSBasicHashStrongValues;
    if (RSBasicHashHasStrongKeys(ht)) flags |= RSBasicHashStrongKeys;
    if (ht->bits.fast_grow) flags |= RSBasicHashAggressiveGrowth;
    if (ht->bits.keys_offset) flags |= RSBasicHashHasKeys;
    if (ht->bits.counts_offset) flags |= RSBasicHashHasCounts;
    if (__RSBasicHashHasHashCache(ht)) flags |= RSBasicHashHasHashCache;
    return flags;
}

RSPrivate RSIndex RSBasicHashGetCount(RSConstBasicHashRef ht) {
    if (ht->bits.counts_offset) {
        RSIndex total = 0L;
        RSIndex cnt = (RSIndex)__RSBasicHashTableSizes[ht->bits.num_buckets_idx];
        for (RSIndex idx = 0; idx < cnt; idx++) {
            total += __RSBasicHashGetSlotCount(ht, idx);
        }
        return total;
    }
    return (RSIndex)ht->bits.used_buckets;
}

RSPrivate RSIndex RSBasicHashGetCountOfKey(RSConstBasicHashRef ht, uintptr_t stack_key) {
    if (__RSBasicHashSubABZero == stack_key || __RSBasicHashSubABOne == stack_key) {
        return 0L;
    }
    if (0L == ht->bits.used_buckets) {
        return 0L;
    }
    return __RSBasicHashFindBucket(ht, stack_key).count;
}

RSPrivate RSIndex RSBasicHashGetCountOfValue(RSConstBasicHashRef ht, uintptr_t stack_value) {
    if (__RSBasicHashSubABZero == stack_value) {
        return 0L;
    }
    if (0L == ht->bits.used_buckets) {
        return 0L;
    }
    if (!(ht->bits.keys_offset)) {
        return __RSBasicHashFindBucket(ht, stack_value).count;
    }
    __block RSIndex total = 0L;
    RSBasicHashApply(ht, ^(RSBasicHashBucket bkt) {
        if ((stack_value == bkt.weak_value) || __RSBasicHashTestEqualValue(ht, bkt.weak_value, stack_value)) total += bkt.count;
        return (BOOL)true;
    });
    return total;
}

RSPrivate BOOL RSBasicHashesAreEqual(RSConstBasicHashRef ht1, RSConstBasicHashRef ht2) {
    RSIndex cnt1 = RSBasicHashGetCount(ht1);
    if (cnt1 != RSBasicHashGetCount(ht2)) return false;
    if (0 == cnt1) return true;
    __block BOOL equal = true;
    RSBasicHashApply(ht1, ^(RSBasicHashBucket bkt1) {
        RSBasicHashBucket bkt2 = __RSBasicHashFindBucket(ht2, bkt1.weak_key);
        if (bkt1.count != bkt2.count) {
            equal = false;
            return (BOOL)false;
        }
        if ((ht1->bits.keys_offset) && (bkt1.weak_value != bkt2.weak_value) && !__RSBasicHashTestEqualValue(ht1, bkt1.weak_value, bkt2.weak_value)) {
            equal = false;
            return (BOOL)false;
        }
        return (BOOL)true;
    });
    return equal;
}

RSPrivate void RSBasicHashApply(RSConstBasicHashRef ht, BOOL (^block)(RSBasicHashBucket)) {
    RSIndex used = (RSIndex)ht->bits.used_buckets, cnt = (RSIndex)__RSBasicHashTableSizes[ht->bits.num_buckets_idx];
    for (RSIndex idx = 0; 0 < used && idx < cnt; idx++) {
        RSBasicHashBucket bkt = RSBasicHashGetBucket(ht, idx);
        if (0 < bkt.count) {
            if (!block(bkt)) {
                return;
            }
            used--;
        }
    }
}

RSPrivate void RSBasicHashApplyBlock(RSConstBasicHashRef ht, BOOL (^block)(RSBasicHashBucket btk, BOOL *stop)) {
    RSIndex used = (RSIndex)ht->bits.used_buckets, cnt = (RSIndex)__RSBasicHashTableSizes[ht->bits.num_buckets_idx];
    __block BOOL stop = NO;
    for (RSIndex idx = 0; !stop && 0 < used && idx < cnt; idx++) {
        RSBasicHashBucket bkt = RSBasicHashGetBucket(ht, idx);
        if (0 < bkt.count) {
            if (!block(bkt, stop)) {
                return;
            }
            used--;
        }
    }
}

RSPrivate void RSBasicHashApplyIndexed(RSConstBasicHashRef ht, RSRange range, BOOL (^block)(RSBasicHashBucket)) {
    if (range.length < 0) HALT;
    if (range.length == 0) return;
    RSIndex cnt = (RSIndex)__RSBasicHashTableSizes[ht->bits.num_buckets_idx];
    if (cnt < range.location + range.length) HALT;
    for (RSIndex idx = 0; idx < range.length; idx++) {
        RSBasicHashBucket bkt = RSBasicHashGetBucket(ht, range.location + idx);
        if (0 < bkt.count) {
            if (!block(bkt)) {
                return;
            }
        }
    }
}

RSPrivate void RSBasicHashGetElements(RSConstBasicHashRef ht, RSIndex bufferslen, uintptr_t *weak_values, uintptr_t *weak_keys) {
    RSIndex used = (RSIndex)ht->bits.used_buckets, cnt = (RSIndex)__RSBasicHashTableSizes[ht->bits.num_buckets_idx];
    RSIndex offset = 0;
    for (RSIndex idx = 0; 0 < used && idx < cnt && offset < bufferslen; idx++) {
        RSBasicHashBucket bkt = RSBasicHashGetBucket(ht, idx);
        if (0 < bkt.count) {
            used--;
            for (RSIndex cnt = bkt.count; cnt-- && offset < bufferslen;) {
                if (weak_values) { weak_values[offset] = bkt.weak_value; }
                if (weak_keys) { weak_keys[offset] = bkt.weak_key; }
                offset++;
            }
        }
    }
}

RSPrivate unsigned long __RSBasicHashFastEnumeration(RSConstBasicHashRef ht, struct __RSFastEnumerationStateEquivalent2 *state, void *stackbuffer, unsigned long count) {
    /* copy as many as count items over */
    if (0 == state->state) {        /* first time */
        state->mutationsPtr = (unsigned long *)&ht->bits;
    }
    state->itemsPtr = (unsigned long *)stackbuffer;
    RSIndex cntx = 0;
    RSIndex used = (RSIndex)ht->bits.used_buckets, cnt = (RSIndex)__RSBasicHashTableSizes[ht->bits.num_buckets_idx];
    for (RSIndex idx = (RSIndex)state->state; 0 < used && idx < cnt && cntx < (RSIndex)count; idx++) {
        RSBasicHashBucket bkt = RSBasicHashGetBucket(ht, idx);
        if (0 < bkt.count) {
            state->itemsPtr[cntx++] = (unsigned long)bkt.weak_key;
            used--;
        }
        state->state++;
    }
    return cntx;
}

#if ENABLE_MEMORY_COUNTERS
static volatile int64_t __RSBasicHashTotalCount = 0ULL;
static volatile int64_t __RSBasicHashTotalSize = 0ULL;
static volatile int64_t __RSBasicHashPeakCount = 0ULL;
static volatile int64_t __RSBasicHashPeakSize = 0ULL;
static volatile int32_t __RSBasicHashSizes[64] = {0};
#endif

static void __RSBasicHashDrain(RSBasicHashRef ht, BOOL forFinalization) {
#if ENABLE_MEMORY_COUNTERS
    OSAtomicAdd64Barrier(-1 * (int64_t) RSBasicHashGetSize(ht, true), & __RSBasicHashTotalSize);
#endif
    
    RSIndex old_num_buckets = __RSBasicHashTableSizes[ht->bits.num_buckets_idx];
    
    RSAllocatorRef allocator = RSGetAllocator(ht);
    BOOL nullify = (!forFinalization || !RS_IS_COLLECTABLE_ALLOCATOR(allocator));
    
    RSBasicHashValue *old_values = nil, *old_keys = nil;
    void *old_counts = nil;
    uintptr_t *old_hashes = nil;
    
    old_values = __RSBasicHashGetValues(ht);
    if (nullify) __RSBasicHashSetValues(ht, nil);
    if (ht->bits.keys_offset) {
        old_keys = __RSBasicHashGetKeys(ht);
        if (nullify) __RSBasicHashSetKeys(ht, nil);
    }
    if (ht->bits.counts_offset) {
        old_counts = __RSBasicHashGetCounts(ht);
        if (nullify) __RSBasicHashSetCounts(ht, nil);
    }
    if (__RSBasicHashHasHashCache(ht)) {
        old_hashes = __RSBasicHashGetHashes(ht);
        if (nullify) __RSBasicHashSetHashes(ht, nil);
    }
    
    if (nullify) {
        ht->bits.mutations++;
        ht->bits.num_buckets_idx = 0;
        ht->bits.used_buckets = 0;
        ht->bits.deleted = 0;
    }
    
    for (RSIndex idx = 0; idx < old_num_buckets; idx++) {
        uintptr_t stack_value = old_values[idx].neutral;
        if (stack_value != 0UL && stack_value != ~0UL) {
            uintptr_t old_value = stack_value;
            if (__RSBasicHashSubABZero == old_value) old_value = 0UL;
            if (__RSBasicHashSubABOne == old_value) old_value = ~0UL;
            __RSBasicHashEjectValue(ht, old_value);
            if (old_keys) {
                uintptr_t old_key = old_keys[idx].neutral;
                if (__RSBasicHashSubABZero == old_key) old_key = 0UL;
                if (__RSBasicHashSubABOne == old_key) old_key = ~0UL;
                __RSBasicHashEjectKey(ht, old_key);
            }
        }
    }
    
    if (!RS_IS_COLLECTABLE_ALLOCATOR(allocator)) {
        RSAllocatorDeallocate(allocator, old_values);
        RSAllocatorDeallocate(allocator, old_keys);
        RSAllocatorDeallocate(allocator, old_counts);
        RSAllocatorDeallocate(allocator, old_hashes);
    }
    
#if ENABLE_MEMORY_COUNTERS
    int64_t size_now = OSAtomicAdd64Barrier((int64_t) RSBasicHashGetSize(ht, true), & __RSBasicHashTotalSize);
    while (__RSBasicHashPeakSize < size_now && !OSAtomicCompareAndSwap64Barrier(__RSBasicHashPeakSize, size_now, & __RSBasicHashPeakSize));
#endif
}

static void __RSBasicHashRehash(RSBasicHashRef ht, RSIndex newItemCount) {
#if ENABLE_MEMORY_COUNTERS
    OSAtomicAdd64Barrier(-1 * (int64_t) RSBasicHashGetSize(ht, true), & __RSBasicHashTotalSize);
    OSAtomicAdd32Barrier(-1, &__RSBasicHashSizes[ht->bits.num_buckets_idx]);
#endif
    
    if (COCOA_HASHTABLE_REHASH_START_ENABLED()) COCOA_HASHTABLE_REHASH_START(ht, RSBasicHashGetNumBuckets(ht), RSBasicHashGetSize(ht, true));
    
    RSIndex new_num_buckets_idx = ht->bits.num_buckets_idx;
    if (0 != newItemCount) {
        if (newItemCount < 0) newItemCount = 0;
        RSIndex new_capacity_req = ht->bits.used_buckets + newItemCount;
        new_num_buckets_idx = __RSBasicHashGetNumBucketsIndexForCapacity(ht, new_capacity_req);
        if (1 == newItemCount && ht->bits.fast_grow) {
            new_num_buckets_idx++;
        }
    }
    
    RSIndex new_num_buckets = __RSBasicHashTableSizes[new_num_buckets_idx];
    RSIndex old_num_buckets = __RSBasicHashTableSizes[ht->bits.num_buckets_idx];
    
    RSBasicHashValue *new_values = nil, *new_keys = nil;
    void *new_counts = nil;
    uintptr_t *new_hashes = nil;
    
    if (0 < new_num_buckets) {
        new_values = (RSBasicHashValue *)__RSBasicHashAllocateMemory(ht, new_num_buckets, sizeof(RSBasicHashValue), RSBasicHashHasStrongValues(ht), 0);
        if (!new_values) HALT;
        __SetLastAllocationEventName(new_values, "RSBasicHash (value-store)");
        memset(new_values, 0, new_num_buckets * sizeof(RSBasicHashValue));
        if (ht->bits.keys_offset) {
            new_keys = (RSBasicHashValue *)__RSBasicHashAllocateMemory(ht, new_num_buckets, sizeof(RSBasicHashValue), RSBasicHashHasStrongKeys(ht), 0);
            if (!new_keys) HALT;
            __SetLastAllocationEventName(new_keys, "RSBasicHash (key-store)");
            memset(new_keys, 0, new_num_buckets * sizeof(RSBasicHashValue));
        }
        if (ht->bits.counts_offset) {
            new_counts = (uintptr_t *)__RSBasicHashAllocateMemory(ht, new_num_buckets, (1 << ht->bits.counts_width), false, false);
            if (!new_counts) HALT;
            __SetLastAllocationEventName(new_counts, "RSBasicHash (count-store)");
            memset(new_counts, 0, new_num_buckets * (1 << ht->bits.counts_width));
        }
        if (__RSBasicHashHasHashCache(ht)) {
            new_hashes = (uintptr_t *)__RSBasicHashAllocateMemory(ht, new_num_buckets, sizeof(uintptr_t), false, false);
            if (!new_hashes) HALT;
            __SetLastAllocationEventName(new_hashes, "RSBasicHash (hash-store)");
            memset(new_hashes, 0, new_num_buckets * sizeof(uintptr_t));
        }
    }
    
    ht->bits.num_buckets_idx = new_num_buckets_idx;
    ht->bits.deleted = 0;
    
    RSBasicHashValue *old_values = nil, *old_keys = nil;
    void *old_counts = nil;
    uintptr_t *old_hashes = nil;
    
    old_values = __RSBasicHashGetValues(ht);
    __RSBasicHashSetValues(ht, new_values);
    if (ht->bits.keys_offset) {
        old_keys = __RSBasicHashGetKeys(ht);
        __RSBasicHashSetKeys(ht, new_keys);
    }
    if (ht->bits.counts_offset) {
        old_counts = __RSBasicHashGetCounts(ht);
        __RSBasicHashSetCounts(ht, new_counts);
    }
    if (__RSBasicHashHasHashCache(ht)) {
        old_hashes = __RSBasicHashGetHashes(ht);
        __RSBasicHashSetHashes(ht, new_hashes);
    }
    
    if (0 < old_num_buckets) {
        for (RSIndex idx = 0; idx < old_num_buckets; idx++) {
            uintptr_t stack_value = old_values[idx].neutral;
            if (stack_value != 0UL && stack_value != ~0UL) {
                if (__RSBasicHashSubABZero == stack_value) stack_value = 0UL;
                if (__RSBasicHashSubABOne == stack_value) stack_value = ~0UL;
                uintptr_t stack_key = stack_value;
                if (ht->bits.keys_offset) {
                    stack_key = old_keys[idx].neutral;
                    if (__RSBasicHashSubABZero == stack_key) stack_key = 0UL;
                    if (__RSBasicHashSubABOne == stack_key) stack_key = ~0UL;
                }
                if (ht->bits.indirect_keys) {
                    stack_key = __RSBasicHashGetIndirectKey(ht, stack_value);
                }
                RSIndex bkt_idx = __RSBasicHashFindBucket_NoCollision(ht, stack_key, old_hashes ? old_hashes[idx] : 0UL);
                __RSBasicHashSetValue(ht, bkt_idx, stack_value, false, false);
                if (old_keys) {
                    __RSBasicHashSetKey(ht, bkt_idx, stack_key, false, false);
                }
                if (old_counts) {
                    switch (ht->bits.counts_width) {
                        case 0: ((uint8_t *)new_counts)[bkt_idx] = ((uint8_t *)old_counts)[idx]; break;
                        case 1: ((uint16_t *)new_counts)[bkt_idx] = ((uint16_t *)old_counts)[idx]; break;
                        case 2: ((uint32_t *)new_counts)[bkt_idx] = ((uint32_t *)old_counts)[idx]; break;
                        case 3: ((uint64_t *)new_counts)[bkt_idx] = ((uint64_t *)old_counts)[idx]; break;
                    }
                }
                if (old_hashes) {
                    new_hashes[bkt_idx] = old_hashes[idx];
                }
            }
        }
    }
    
    RSAllocatorRef allocator = RSGetAllocator(ht);
    if (!RS_IS_COLLECTABLE_ALLOCATOR(allocator)) {
        RSAllocatorDeallocate(allocator, old_values);
        RSAllocatorDeallocate(allocator, old_keys);
        RSAllocatorDeallocate(allocator, old_counts);
        RSAllocatorDeallocate(allocator, old_hashes);
    }
    
    if (COCOA_HASHTABLE_REHASH_END_ENABLED()) COCOA_HASHTABLE_REHASH_END(ht, RSBasicHashGetNumBuckets(ht), RSBasicHashGetSize(ht, true));
    
#if ENABLE_MEMORY_COUNTERS
    int64_t size_now = OSAtomicAdd64Barrier((int64_t) RSBasicHashGetSize(ht, true), &__RSBasicHashTotalSize);
    while (__RSBasicHashPeakSize < size_now && !OSAtomicCompareAndSwap64Barrier(__RSBasicHashPeakSize, size_now, & __RSBasicHashPeakSize));
    OSAtomicAdd32Barrier(1, &__RSBasicHashSizes[ht->bits.num_buckets_idx]);
#endif
}

RSPrivate void RSBasicHashSetCapacity(RSBasicHashRef ht, RSIndex capacity) {
    if (!RSBasicHashIsMutable(ht)) HALT;
    if (ht->bits.used_buckets < capacity) {
        ht->bits.mutations++;
        __RSBasicHashRehash(ht, capacity - ht->bits.used_buckets);
    }
}

static void __RSBasicHashAddValue(RSBasicHashRef ht, RSIndex bkt_idx, uintptr_t stack_key, uintptr_t stack_value) {
    ht->bits.mutations++;
    if (RSBasicHashGetCapacity(ht) < ht->bits.used_buckets + 1) {
        __RSBasicHashRehash(ht, 1);
        bkt_idx = __RSBasicHashFindBucket_NoCollision(ht, stack_key, 0);
    } else if (__RSBasicHashIsDeleted(ht, bkt_idx)) {
        ht->bits.deleted--;
    }
    uintptr_t key_hash = 0;
    if (__RSBasicHashHasHashCache(ht)) {
        key_hash = __RSBasicHashHashKey(ht, stack_key);
    }
    stack_value = __RSBasicHashImportValue(ht, stack_value);
    if (ht->bits.keys_offset) {
        stack_key = __RSBasicHashImportKey(ht, stack_key);
    }
    __RSBasicHashSetValue(ht, bkt_idx, stack_value, false, false);
    if (ht->bits.keys_offset) {
        __RSBasicHashSetKey(ht, bkt_idx, stack_key, false, false);
    }
    if (ht->bits.counts_offset) {
        __RSBasicHashIncSlotCount(ht, bkt_idx);
    }
    if (__RSBasicHashHasHashCache(ht)) {
        __RSBasicHashGetHashes(ht)[bkt_idx] = key_hash;
    }
    ht->bits.used_buckets++;
}

static void __RSBasicHashReplaceValue(RSBasicHashRef ht, RSIndex bkt_idx, uintptr_t stack_key, uintptr_t stack_value) {
    ht->bits.mutations++;
    stack_value = __RSBasicHashImportValue(ht, stack_value);
    if (ht->bits.keys_offset) {
        stack_key = __RSBasicHashImportKey(ht, stack_key);
    }
    __RSBasicHashSetValue(ht, bkt_idx, stack_value, false, false);
    if (ht->bits.keys_offset) {
        __RSBasicHashSetKey(ht, bkt_idx, stack_key, false, false);
    }
}

static void __RSBasicHashRemoveValue(RSBasicHashRef ht, RSIndex bkt_idx) {
    ht->bits.mutations++;
    __RSBasicHashSetValue(ht, bkt_idx, ~0UL, false, true);
    if (ht->bits.keys_offset) {
        __RSBasicHashSetKey(ht, bkt_idx, ~0UL, false, true);
    }
    if (ht->bits.counts_offset) {
        __RSBasicHashDecSlotCount(ht, bkt_idx);
    }
    if (__RSBasicHashHasHashCache(ht)) {
        __RSBasicHashGetHashes(ht)[bkt_idx] = 0;
    }
    ht->bits.used_buckets--;
    ht->bits.deleted++;
    BOOL do_shrink = false;
    if (ht->bits.fast_grow) { // == slow shrink
        do_shrink = (5 < ht->bits.num_buckets_idx && ht->bits.used_buckets < __RSBasicHashGetCapacityForNumBuckets(ht, ht->bits.num_buckets_idx - 5));
    } else {
        do_shrink = (2 < ht->bits.num_buckets_idx && ht->bits.used_buckets < __RSBasicHashGetCapacityForNumBuckets(ht, ht->bits.num_buckets_idx - 2));
    }
    if (do_shrink) {
        __RSBasicHashRehash(ht, -1);
        return;
    }
    do_shrink = (0 == ht->bits.deleted); // .deleted roll-over
    RSIndex num_buckets = __RSBasicHashTableSizes[ht->bits.num_buckets_idx];
    do_shrink = do_shrink || ((20 <= num_buckets) && (num_buckets / 4 <= ht->bits.deleted));
    if (do_shrink) {
        __RSBasicHashRehash(ht, 0);
    }
}

RSPrivate BOOL RSBasicHashAddValue(RSBasicHashRef ht, uintptr_t stack_key, uintptr_t stack_value) {
    if (!RSBasicHashIsMutable(ht)) HALT;
    if (__RSBasicHashSubABZero == stack_key) HALT;
    if (__RSBasicHashSubABOne == stack_key) HALT;
    if (__RSBasicHashSubABZero == stack_value) HALT;
    if (__RSBasicHashSubABOne == stack_value) HALT;
    RSBasicHashBucket bkt = __RSBasicHashFindBucket(ht, stack_key);
    if (0 < bkt.count) {
        ht->bits.mutations++;
        if (ht->bits.counts_offset && bkt.count < LONG_MAX) { // if not yet as large as a RSIndex can be... otherwise clamp and do nothing
            __RSBasicHashIncSlotCount(ht, bkt.idx);
            return true;
        }
    } else {
        __RSBasicHashAddValue(ht, bkt.idx, stack_key, stack_value);
        return true;
    }
    return false;
}

RSPrivate void RSBasicHashReplaceValue(RSBasicHashRef ht, uintptr_t stack_key, uintptr_t stack_value) {
    if (!RSBasicHashIsMutable(ht)) HALT;
    if (__RSBasicHashSubABZero == stack_key) HALT;
    if (__RSBasicHashSubABOne == stack_key) HALT;
    if (__RSBasicHashSubABZero == stack_value) HALT;
    if (__RSBasicHashSubABOne == stack_value) HALT;
    RSBasicHashBucket bkt = __RSBasicHashFindBucket(ht, stack_key);
    if (0 < bkt.count) {
        __RSBasicHashReplaceValue(ht, bkt.idx, stack_key, stack_value);
    }
}

RSPrivate void RSBasicHashSetValue(RSBasicHashRef ht, uintptr_t stack_key, uintptr_t stack_value) {
    if (!RSBasicHashIsMutable(ht)) HALT;
    if (__RSBasicHashSubABZero == stack_key) HALT;
    if (__RSBasicHashSubABOne == stack_key) HALT;
    if (__RSBasicHashSubABZero == stack_value) HALT;
    if (__RSBasicHashSubABOne == stack_value) HALT;
    RSBasicHashBucket bkt = __RSBasicHashFindBucket(ht, stack_key);
    if (0 < bkt.count) {
        __RSBasicHashReplaceValue(ht, bkt.idx, stack_key, stack_value);
    } else {
        __RSBasicHashAddValue(ht, bkt.idx, stack_key, stack_value);
    }
}

RSPrivate RSIndex RSBasicHashRemoveValue(RSBasicHashRef ht, uintptr_t stack_key) {
    if (!RSBasicHashIsMutable(ht)) HALT;
    if (__RSBasicHashSubABZero == stack_key || __RSBasicHashSubABOne == stack_key) return 0;
    RSBasicHashBucket bkt = __RSBasicHashFindBucket(ht, stack_key);
    if (1 < bkt.count) {
        ht->bits.mutations++;
        if (ht->bits.counts_offset && bkt.count < LONG_MAX) { // if not as large as a RSIndex can be... otherwise clamp and do nothing
            __RSBasicHashDecSlotCount(ht, bkt.idx);
        }
    } else if (0 < bkt.count) {
        __RSBasicHashRemoveValue(ht, bkt.idx);
    }
    return bkt.count;
}

RSPrivate RSIndex RSBasicHashRemoveValueAtIndex(RSBasicHashRef ht, RSIndex idx) {
    if (!RSBasicHashIsMutable(ht)) HALT;
    RSBasicHashBucket bkt = RSBasicHashGetBucket(ht, idx);
    if (1 < bkt.count) {
        ht->bits.mutations++;
        if (ht->bits.counts_offset && bkt.count < LONG_MAX) { // if not as large as a RSIndex can be... otherwise clamp and do nothing
            __RSBasicHashDecSlotCount(ht, bkt.idx);
        }
    } else if (0 < bkt.count) {
        __RSBasicHashRemoveValue(ht, bkt.idx);
    }
    return bkt.count;
}

RSPrivate void RSBasicHashRemoveAllValues(RSBasicHashRef ht) {
    if (!RSBasicHashIsMutable(ht)) HALT;
    if (0 == ht->bits.num_buckets_idx) return;
    __RSBasicHashDrain(ht, false);
}

RSPrivate BOOL RSBasicHashAddIntValueAndInc(RSBasicHashRef ht, uintptr_t stack_key, uintptr_t int_value) {
    if (!RSBasicHashIsMutable(ht)) HALT;
    if (__RSBasicHashSubABZero == stack_key) HALT;
    if (__RSBasicHashSubABOne == stack_key) HALT;
    if (__RSBasicHashSubABZero == int_value) HALT;
    if (__RSBasicHashSubABOne == int_value) HALT;
    RSBasicHashBucket bkt = __RSBasicHashFindBucket(ht, stack_key);
    if (0 < bkt.count) {
        ht->bits.mutations++;
    } else {
        // must rehash before renumbering
        if (RSBasicHashGetCapacity(ht) < ht->bits.used_buckets + 1) {
            __RSBasicHashRehash(ht, 1);
            bkt.idx = __RSBasicHashFindBucket_NoCollision(ht, stack_key, 0);
        }
        RSIndex cnt = (RSIndex)__RSBasicHashTableSizes[ht->bits.num_buckets_idx];
        for (RSIndex idx = 0; idx < cnt; idx++) {
            if (!__RSBasicHashIsEmptyOrDeleted(ht, idx)) {
                uintptr_t stack_value = __RSBasicHashGetValue(ht, idx);
                if (int_value <= stack_value) {
                    stack_value++;
                    __RSBasicHashSetValue(ht, idx, stack_value, true, false);
                    ht->bits.mutations++;
                }
            }
        }
        __RSBasicHashAddValue(ht, bkt.idx, stack_key, int_value);
        return true;
    }
    return false;
}

RSPrivate void RSBasicHashRemoveIntValueAndDec(RSBasicHashRef ht, uintptr_t int_value) {
    if (!RSBasicHashIsMutable(ht)) HALT;
    if (__RSBasicHashSubABZero == int_value) HALT;
    if (__RSBasicHashSubABOne == int_value) HALT;
    uintptr_t bkt_idx = ~0UL;
    RSIndex cnt = (RSIndex)__RSBasicHashTableSizes[ht->bits.num_buckets_idx];
    for (RSIndex idx = 0; idx < cnt; idx++) {
        if (!__RSBasicHashIsEmptyOrDeleted(ht, idx)) {
            uintptr_t stack_value = __RSBasicHashGetValue(ht, idx);
            if (int_value == stack_value) {
                bkt_idx = idx;
            }
            if (int_value < stack_value) {
                stack_value--;
                __RSBasicHashSetValue(ht, idx, stack_value, true, false);
                ht->bits.mutations++;
            }
        }
    }
    __RSBasicHashRemoveValue(ht, bkt_idx);
}

RSPrivate size_t RSBasicHashGetSize(RSConstBasicHashRef ht, BOOL total) {
    size_t size = sizeof(struct __RSBasicHash);
    if (ht->bits.keys_offset) size += sizeof(RSBasicHashValue *);
    if (ht->bits.counts_offset) size += sizeof(void *);
    if (__RSBasicHashHasHashCache(ht)) size += sizeof(uintptr_t *);
    if (total) {
        RSIndex num_buckets = __RSBasicHashTableSizes[ht->bits.num_buckets_idx];
        if (0 < num_buckets) {
            size += RSAllocatorInstanceSize(RSAllocatorSystemDefault, __RSBasicHashGetValues(ht));
            if (ht->bits.keys_offset) size += RSAllocatorInstanceSize(RSAllocatorSystemDefault, __RSBasicHashGetKeys(ht));
            if (ht->bits.counts_offset) size += RSAllocatorInstanceSize(RSAllocatorSystemDefault, __RSBasicHashGetCounts(ht));
            if (__RSBasicHashHasHashCache(ht)) size += RSAllocatorInstanceSize(RSAllocatorSystemDefault, __RSBasicHashGetHashes(ht));
        }
    }
    return size;
}

RSPrivate RSStringRef RSBasicHashDescription(RSConstBasicHashRef ht, BOOL detailed, RSStringRef prefix, RSStringRef entryPrefix, BOOL describeElements) {
    RSMutableStringRef result = RSStringCreateMutable(RSAllocatorSystemDefault, 0);
    RSStringAppendStringWithFormat(result, RSSTR("%r{\n"), prefix);
//    RSStringAppendStringWithFormat(result, RSSTR("%r{type = %s %s%s, count = %ld,\n"), prefix, (RSBasicHashIsMutable(ht) ? "mutable" : "immutable"), ((ht->bits.counts_offset) ? "multi" : ""), ((ht->bits.keys_offset) ? "dict" : "set"), RSBasicHashGetCount(ht));
    if (detailed) {
        const char *cb_type = "custom";
        RSStringAppendStringWithFormat(result, RSSTR("%rhash cache = %s, strong values = %s, strong keys = %s, cb = %s,\n"), prefix, (__RSBasicHashHasHashCache(ht) ? "yes" : "no"), (RSBasicHashHasStrongValues(ht) ? "yes" : "no"), (RSBasicHashHasStrongKeys(ht) ? "yes" : "no"), cb_type);
        RSStringAppendStringWithFormat(result, RSSTR("%rnum bucket index = %d, num buckets = %ld, capacity = %ld, num buckets used = %u,\n"), prefix, ht->bits.num_buckets_idx, RSBasicHashGetNumBuckets(ht), (long)RSBasicHashGetCapacity(ht), ht->bits.used_buckets);
        RSStringAppendStringWithFormat(result, RSSTR("%rcounts width = %d, finalized = %s,\n"), prefix,((ht->bits.counts_offset) ? (1 << ht->bits.counts_width) : 0), (ht->bits.finalized ? "yes" : "no"));
        RSStringAppendStringWithFormat(result, RSSTR("%rnum mutations = %ld, num deleted = %ld, size = %ld, total size = %ld,\n"), prefix, (long)ht->bits.mutations, (long)ht->bits.deleted, RSBasicHashGetSize(ht, false), RSBasicHashGetSize(ht, true));
        RSStringAppendStringWithFormat(result, RSSTR("%rvalues ptr = %p, keys ptr = %p, counts ptr = %p, hashes ptr = %p,\n"), prefix, __RSBasicHashGetValues(ht), ((ht->bits.keys_offset) ? __RSBasicHashGetKeys(ht) : nil), ((ht->bits.counts_offset) ? __RSBasicHashGetCounts(ht) : nil), (__RSBasicHashHasHashCache(ht) ? __RSBasicHashGetHashes(ht) : nil));
    }
//    RSStringAppendStringWithFormat(result, RSSTR("%rentries =>\n"), prefix);
    
    __block RSMutableArrayRef keys = RSArrayCreateMutable(RSAllocatorSystemDefault, 0);
    __block RSMutableArrayRef values = RSArrayCreateMutable(RSAllocatorSystemDefault, 0);
    
    RSBasicHashApplyBlock(ht, ^BOOL(RSBasicHashBucket bkt, BOOL *stop) {
        RSStringRef kDesc = nil;
        if (!describeElements) {
            if (ht->bits.keys_offset) {
                kDesc = RSStringCreateWithFormat(RSAllocatorSystemDefault, RSSTR("<%p>"), (void *)bkt.weak_key);
            }
        } else {
            if (ht->bits.keys_offset) {
                kDesc = __RSBasicHashDescKey(ht, bkt.weak_key);
            }
        }
        
        RSArrayAddObject(keys, kDesc);
        
        if (kDesc) RSRelease(kDesc);
        return YES;
    });
    
    RSUInteger cnt = RSArrayGetCount(keys);

    RSArraySort(keys, RSOrderedAscending, (RSComparatorFunction)RSStringCompare, nil);
    
    RSArrayApplyBlock(keys, RSMakeRange(0, cnt), ^(const void *value, RSUInteger idx, BOOL *isStop) {
        RSBasicHashBucket bkt = RSBasicHashFindBucket(ht, (uintptr_t)value);
        RSTypeRef v = (0 < bkt.count ? (RSTypeRef)bkt.weak_value : 0);
        v = v ? __RSBasicHashDescValue(ht, bkt.weak_value) : RSSTR("Null");
        RSArrayAddObject(values, v);
        if (v) RSRelease(v);
    });
    
    for (RSUInteger idx = 0; idx < cnt; idx++) {
        RSStringAppendStringWithFormat(result, RSSTR("%r%r = %r\n"), entryPrefix, RSArrayObjectAtIndex(keys, idx), RSArrayObjectAtIndex(values, idx));
    }
    RSRelease(keys);
    RSRelease(values);
    
    RSStringAppendStringWithFormat(result, RSSTR("%r}"), prefix);
    return result;
    RSBasicHashApply(ht, ^(RSBasicHashBucket bkt) {
        RSStringRef vDesc = nil, kDesc = nil;
        if (!describeElements) {
            vDesc = RSStringCreateWithFormat(RSAllocatorSystemDefault, RSSTR("<%p>"), (void *)bkt.weak_value);
            if (ht->bits.keys_offset) {
                kDesc = RSStringCreateWithFormat(RSAllocatorSystemDefault, RSSTR("<%p>"), (void *)bkt.weak_key);
            }
        } else {
            vDesc = __RSBasicHashDescValue(ht, bkt.weak_value);
            if (ht->bits.keys_offset) {
                kDesc = __RSBasicHashDescKey(ht, bkt.weak_key);
            }
        }
        if (ht->bits.keys_offset && ht->bits.counts_offset) {
            RSStringAppendStringWithFormat(result, RSSTR("%r%ld : %r = %r (%ld)\n"), entryPrefix, bkt.idx, kDesc, vDesc, bkt.count);
        } else if (ht->bits.keys_offset) {
            RSStringAppendStringWithFormat(result, RSSTR("%r%ld : %r = %r\n"), entryPrefix, bkt.idx, kDesc, vDesc);
        } else if (ht->bits.counts_offset) {
            RSStringAppendStringWithFormat(result, RSSTR("%r%ld : %r (%ld)\n"), entryPrefix, bkt.idx, vDesc, bkt.count);
        } else {
            RSStringAppendStringWithFormat(result, RSSTR("%r%ld : %r\n"), entryPrefix, bkt.idx, vDesc);
        }
        if (kDesc) RSRelease(kDesc);
        if (vDesc) RSRelease(vDesc);
        return (BOOL)true;
    });
    RSStringAppendStringWithFormat(result, RSSTR("%r}\n"), prefix);
    return result;
}

RSPrivate void RSBasicHashShow(RSConstBasicHashRef ht) {
    RSStringRef str = RSBasicHashDescription(ht, true, RSSTR(""), RSSTR("\t"), false);
    RSShow(str);
    RSRelease(str);
}

RSPrivate BOOL __RSBasicHashEqual(RSTypeRef cf1, RSTypeRef cf2) {
    RSBasicHashRef ht1 = (RSBasicHashRef)cf1;
    RSBasicHashRef ht2 = (RSBasicHashRef)cf2;
    //#warning this used to require that the key and value equal callbacks were pointer identical
    return RSBasicHashesAreEqual(ht1, ht2);
}

RSPrivate RSHashCode __RSBasicHashHash(RSTypeRef cf) {
    RSBasicHashRef ht = (RSBasicHashRef)cf;
    return RSBasicHashGetCount(ht);
}

RSPrivate RSStringRef __RSBasicHashDescription(RSTypeRef cf) {
    RSBasicHashRef ht = (RSBasicHashRef)cf;
    RSStringRef desc = RSBasicHashDescription(ht, false, RSSTR(""), RSSTR("\t"), true);
    RSStringRef result = RSStringCreateWithFormat(RSAllocatorSystemDefault, RSSTR("<RSBasicHash %p [%p]>%r"), cf, RSGetAllocator(cf), desc);
    RSRelease(desc);
    return result;
}

RSPrivate void __RSBasicHashDeallocate(RSTypeRef cf) {
    RSBasicHashRef ht = (RSBasicHashRef)cf;
    if (ht->bits.finalized) HALT;
    ht->bits.finalized = 1;
    __RSBasicHashDrain(ht, true);
#if ENABLE_MEMORY_COUNTERS
    OSAtomicAdd64Barrier(-1, &__RSBasicHashTotalCount);
    OSAtomicAdd32Barrier(-1, &__RSBasicHashSizes[ht->bits.num_buckets_idx]);
#endif
}

static RSTypeID __RSBasicHashTypeID = _RSRuntimeNotATypeID;

static const RSRuntimeClass __RSBasicHashClass = {
    _RSRuntimeScannedObject,
    "RSBasicHash",
    nil,        // init
    nil,        // copy
    __RSBasicHashDeallocate,
    __RSBasicHashEqual,
    __RSBasicHashHash,
    __RSBasicHashDescription,
    nil,
    nil
};

RSPrivate void __RSBasicHashInitialize()
{
    __RSBasicHashTypeID = __RSRuntimeRegisterClass(&__RSBasicHashClass);
    __RSRuntimeSetClassTypeID(&__RSBasicHashClass, __RSBasicHashTypeID);
}

RSPrivate RSTypeID RSBasicHashGetTypeID(void) {
    //if (_RSRuntimeNotATypeID == __RSBasicHashTypeID) __RSBasicHashTypeID = __RSRuntimeRegisterClass(&__RSBasicHashClass);
    return __RSBasicHashTypeID;
}

RSPrivate RSBasicHashRef RSBasicHashCreate(RSAllocatorRef allocator, RSOptionFlags flags, const RSBasicHashCallbacks *cb) {
    size_t size = sizeof(struct __RSBasicHash) - sizeof(RSRuntimeBase);
    if (flags & RSBasicHashHasKeys) size += sizeof(RSBasicHashValue *); // keys
    if (flags & RSBasicHashHasCounts) size += sizeof(void *); // counts
    if (flags & RSBasicHashHasHashCache) size += sizeof(uintptr_t *); // hashes
    RSBasicHashRef ht = (RSBasicHashRef)__RSRuntimeCreateInstance(allocator, RSBasicHashGetTypeID(), size);
    if (nil == ht) return nil;
    
    ht->bits.finalized = 0;
    ht->bits.hash_style = (flags >> 13) & 0x3;
    ht->bits.fast_grow = (flags & RSBasicHashAggressiveGrowth) ? 1 : 0;
    ht->bits.counts_width = 0;
    ht->bits.strong_values = (flags & RSBasicHashStrongValues) ? 1 : 0;
    ht->bits.strong_keys = (flags & RSBasicHashStrongKeys) ? 1 : 0;
    ht->bits.weak_values = (flags & RSBasicHashWeakValues) ? 1 : 0;
    ht->bits.weak_keys = (flags & RSBasicHashWeakKeys) ? 1 : 0;
    ht->bits.int_values = (flags & RSBasicHashIntegerValues) ? 1 : 0;
    ht->bits.int_keys = (flags & RSBasicHashIntegerKeys) ? 1 : 0;
    ht->bits.indirect_keys = (flags & RSBasicHashIndirectKeys) ? 1 : 0;
    ht->bits.num_buckets_idx = 0;
    ht->bits.used_buckets = 0;
    ht->bits.deleted = 0;
    ht->bits.mutations = 1;
    
    if (ht->bits.strong_values && ht->bits.weak_values) HALT;
    if (ht->bits.strong_values && ht->bits.int_values) HALT;
    if (ht->bits.strong_keys && ht->bits.weak_keys) HALT;
    if (ht->bits.strong_keys && ht->bits.int_keys) HALT;
    if (ht->bits.weak_values && ht->bits.int_values) HALT;
    if (ht->bits.weak_keys && ht->bits.int_keys) HALT;
    if (ht->bits.indirect_keys && ht->bits.strong_keys) HALT;
    if (ht->bits.indirect_keys && ht->bits.weak_keys) HALT;
    if (ht->bits.indirect_keys && ht->bits.int_keys) HALT;
    
    uint64_t offset = 1;
    ht->bits.keys_offset = (flags & RSBasicHashHasKeys) ? offset++ : 0;
    ht->bits.counts_offset = (flags & RSBasicHashHasCounts) ? offset++ : 0;
    ht->bits.hashes_offset = (flags & RSBasicHashHasHashCache) ? offset++ : 0;
    
#if DEPLOYMENT_TARGET_EMBEDDED || DEPLOYMENT_TARGET_EMBEDDED_MINI
    ht->bits.hashes_offset = 0;
    ht->bits.strong_values = 0;
    ht->bits.strong_keys = 0;
    ht->bits.weak_values = 0;
    ht->bits.weak_keys = 0;
#endif
    
    ht->bits.__kret = RSBasicHashGetPtrIndex((void *)cb->retainKey);
    ht->bits.__vret = RSBasicHashGetPtrIndex((void *)cb->retainValue);
    ht->bits.__krel = RSBasicHashGetPtrIndex((void *)cb->releaseKey);
    ht->bits.__vrel = RSBasicHashGetPtrIndex((void *)cb->releaseValue);
    ht->bits.__kdes = RSBasicHashGetPtrIndex((void *)cb->copyKeyDescription);
    ht->bits.__vdes = RSBasicHashGetPtrIndex((void *)cb->copyValueDescription);
    ht->bits.__kequ = RSBasicHashGetPtrIndex((void *)cb->equateKeys);
    ht->bits.__vequ = RSBasicHashGetPtrIndex((void *)cb->equateValues);
    ht->bits.__khas = RSBasicHashGetPtrIndex((void *)cb->hashKey);
    ht->bits.__kget = RSBasicHashGetPtrIndex((void *)cb->getIndirectKey);
    
    for (RSIndex idx = 0; idx < offset; idx++) {
        ht->pointers[idx] = nil;
    }
    
#if ENABLE_MEMORY_COUNTERS
    int64_t size_now = OSAtomicAdd64Barrier((int64_t) RSBasicHashGetSize(ht, true), & __RSBasicHashTotalSize);
    while (__RSBasicHashPeakSize < size_now && !OSAtomicCompareAndSwap64Barrier(__RSBasicHashPeakSize, size_now, & __RSBasicHashPeakSize));
    int64_t count_now = OSAtomicAdd64Barrier(1, & __RSBasicHashTotalCount);
    while (__RSBasicHashPeakCount < count_now && !OSAtomicCompareAndSwap64Barrier(__RSBasicHashPeakCount, count_now, & __RSBasicHashPeakCount));
    OSAtomicAdd32Barrier(1, &__RSBasicHashSizes[ht->bits.num_buckets_idx]);
#endif
    
    return ht;
}

RSPrivate RSBasicHashRef RSBasicHashCreateCopy(RSAllocatorRef allocator, RSConstBasicHashRef src_ht) {
    size_t size = RSBasicHashGetSize(src_ht, false) - sizeof(RSRuntimeBase);
    RSIndex new_num_buckets = __RSBasicHashTableSizes[src_ht->bits.num_buckets_idx];
    RSBasicHashValue *new_values = nil, *new_keys = nil;
    void *new_counts = nil;
    uintptr_t *new_hashes = nil;
    
    if (0 < new_num_buckets) {
        BOOL strongValues = RSBasicHashHasStrongValues(src_ht) && !(RSUseCollectableAllocator && !RS_IS_COLLECTABLE_ALLOCATOR(allocator));
        BOOL strongKeys = RSBasicHashHasStrongKeys(src_ht) && !(RSUseCollectableAllocator && !RS_IS_COLLECTABLE_ALLOCATOR(allocator));
        new_values = (RSBasicHashValue *)__RSBasicHashAllocateMemory2(allocator, new_num_buckets, sizeof(RSBasicHashValue), strongValues, 0);
        if (!new_values) return nil; // in this unusual circumstance, leak previously allocated blocks for now
        __SetLastAllocationEventName(new_values, "RSBasicHash (value-store)");
        if (src_ht->bits.keys_offset) {
            new_keys = (RSBasicHashValue *)__RSBasicHashAllocateMemory2(allocator, new_num_buckets, sizeof(RSBasicHashValue), strongKeys, false);
            if (!new_keys) return nil; // in this unusual circumstance, leak previously allocated blocks for now
            __SetLastAllocationEventName(new_keys, "RSBasicHash (key-store)");
        }
        if (src_ht->bits.counts_offset) {
            new_counts = (uintptr_t *)__RSBasicHashAllocateMemory2(allocator, new_num_buckets, (1 << src_ht->bits.counts_width), false, false);
            if (!new_counts) return nil; // in this unusual circumstance, leak previously allocated blocks for now
            __SetLastAllocationEventName(new_counts, "RSBasicHash (count-store)");
        }
        if (__RSBasicHashHasHashCache(src_ht)) {
            new_hashes = (uintptr_t *)__RSBasicHashAllocateMemory2(allocator, new_num_buckets, sizeof(uintptr_t), false, false);
            if (!new_hashes) return nil; // in this unusual circumstance, leak previously allocated blocks for now
            __SetLastAllocationEventName(new_hashes, "RSBasicHash (hash-store)");
        }
    }
    
    RSBasicHashRef ht = (RSBasicHashRef)__RSRuntimeCreateInstance(allocator, RSBasicHashGetTypeID(), size);
    if (nil == ht) return nil; // in this unusual circumstance, leak previously allocated blocks for now
    
    memmove((uint8_t *)ht + sizeof(RSRuntimeBase), (uint8_t *)src_ht + sizeof(RSRuntimeBase), sizeof(ht->bits));
    if (RSUseCollectableAllocator && !RS_IS_COLLECTABLE_ALLOCATOR(allocator)) {
        ht->bits.strong_values = 0;
        ht->bits.strong_keys = 0;
        ht->bits.weak_values = 0;
        ht->bits.weak_keys = 0;
    }
    ht->bits.finalized = 0;
    ht->bits.mutations = 1;
    
    if (0 == new_num_buckets) {
#if ENABLE_MEMORY_COUNTERS
        int64_t size_now = OSAtomicAdd64Barrier((int64_t) RSBasicHashGetSize(ht, true), & __RSBasicHashTotalSize);
        while (__RSBasicHashPeakSize < size_now && !OSAtomicCompareAndSwap64Barrier(__RSBasicHashPeakSize, size_now, & __RSBasicHashPeakSize));
        int64_t count_now = OSAtomicAdd64Barrier(1, & __RSBasicHashTotalCount);
        while (__RSBasicHashPeakCount < count_now && !OSAtomicCompareAndSwap64Barrier(__RSBasicHashPeakCount, count_now, & __RSBasicHashPeakCount));
        OSAtomicAdd32Barrier(1, &__RSBasicHashSizes[ht->bits.num_buckets_idx]);
#endif
        return ht;
    }
    
    RSBasicHashValue *old_values = nil, *old_keys = nil;
    void *old_counts = nil;
    uintptr_t *old_hashes = nil;
    
    old_values = __RSBasicHashGetValues(src_ht);
    if (src_ht->bits.keys_offset) {
        old_keys = __RSBasicHashGetKeys(src_ht);
    }
    if (src_ht->bits.counts_offset) {
        old_counts = __RSBasicHashGetCounts(src_ht);
    }
    if (__RSBasicHashHasHashCache(src_ht)) {
        old_hashes = __RSBasicHashGetHashes(src_ht);
    }
    
    __RSBasicHashSetValues(ht, new_values);
    if (new_keys) {
        __RSBasicHashSetKeys(ht, new_keys);
    }
    if (new_counts) {
        __RSBasicHashSetCounts(ht, new_counts);
    }
    if (new_hashes) {
        __RSBasicHashSetHashes(ht, new_hashes);
    }
    
    for (RSIndex idx = 0; idx < new_num_buckets; idx++) {
        uintptr_t stack_value = old_values[idx].neutral;
        if (stack_value != 0UL && stack_value != ~0UL) {
            uintptr_t old_value = stack_value;
            if (__RSBasicHashSubABZero == old_value) old_value = 0UL;
            if (__RSBasicHashSubABOne == old_value) old_value = ~0UL;
            __RSBasicHashSetValue(ht, idx, __RSBasicHashImportValue(ht, old_value), true, false);
            if (new_keys) {
                uintptr_t old_key = old_keys[idx].neutral;
                if (__RSBasicHashSubABZero == old_key) old_key = 0UL;
                if (__RSBasicHashSubABOne == old_key) old_key = ~0UL;
                __RSBasicHashSetKey(ht, idx, __RSBasicHashImportKey(ht, old_key), true, false);
            }
        } else {
            __RSBasicHashSetValue(ht, idx, stack_value, true, true);
            if (new_keys) {
                __RSBasicHashSetKey(ht, idx, stack_value, true, true);
            }
        }
    }
    if (new_counts) memmove(new_counts, old_counts, new_num_buckets * (1 << ht->bits.counts_width));
    if (new_hashes) memmove(new_hashes, old_hashes, new_num_buckets * sizeof(uintptr_t));
    
#if ENABLE_MEMORY_COUNTERS
    int64_t size_now = OSAtomicAdd64Barrier((int64_t) RSBasicHashGetSize(ht, true), & __RSBasicHashTotalSize);
    while (__RSBasicHashPeakSize < size_now && !OSAtomicCompareAndSwap64Barrier(__RSBasicHashPeakSize, size_now, & __RSBasicHashPeakSize));
    int64_t count_now = OSAtomicAdd64Barrier(1, & __RSBasicHashTotalCount);
    while (__RSBasicHashPeakCount < count_now && !OSAtomicCompareAndSwap64Barrier(__RSBasicHashPeakCount, count_now, & __RSBasicHashPeakCount));
    OSAtomicAdd32Barrier(1, &__RSBasicHashSizes[ht->bits.num_buckets_idx]);
#endif
    
    return ht;
}

RSPrivate const RSBasicHashCallbacks *RSBasicHashGetCallbacks(RSConstBasicHashRef ht)
{
    return nil;
}
#endif

