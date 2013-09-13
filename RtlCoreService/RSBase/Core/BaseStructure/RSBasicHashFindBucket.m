//
//  RSBasicHashFindBucket.c
//  RSCoreFoundation
//
//  Created by Apple Inc.
//  Changed by RetVal on 12/9/12.
//  Copyright (c) 2012 RetVal. All rights reserved.
//

// MurmurHash2 32 bit ? djb

#include <RSCoreFoundation/RSBasicHash.h>
static
#if FIND_BUCKET_FOR_REHASH
RSIndex
#else
RSBasicHashBucket
#endif
FIND_BUCKET_NAME (RSConstBasicHashRef ht, uintptr_t stack_key
#if FIND_BUCKET_FOR_REHASH
                  , uintptr_t key_hash
#endif
                  ) {
    uint8_t num_buckets_idx = ht->bits.num_buckets_idx;
    uintptr_t num_buckets = __RSBasicHashTableSizes[num_buckets_idx];
#if FIND_BUCKET_FOR_REHASH
    RSHashCode hash_code = key_hash ? key_hash : __RSBasicHashHashKey(ht, stack_key);
#else
    RSHashCode hash_code = __RSBasicHashHashKey(ht, stack_key);
#endif
    
#if FIND_BUCKET_HASH_STYLE == 1	// __RSBasicHashLinearHashingValue
    // Linear probing, with c = 1
    // probe[0] = h1(k)
    // probe[i] = (h1(k) + i * c) mod num_buckets, i = 1 .. num_buckets - 1
    // h1(k) = k mod num_buckets
#if defined(__arm__)
    uintptr_t h1 = __RSBasicHashFold(hash_code, num_buckets_idx);
#else
    uintptr_t h1 = hash_code % num_buckets;
#endif
#elif FIND_BUCKET_HASH_STYLE == 2	// __RSBasicHashDoubleHashingValue
    // Double hashing
    // probe[0] = h1(k)
    // probe[i] = (h1(k) + i * h2(k)) mod num_buckets, i = 1 .. num_buckets - 1
    // h1(k) = k mod num_buckets
    // h2(k) = floor(k / num_buckets) mod num_buckets
#if defined(__arm__)
    uintptr_t h1 = __RSBasicHashFold(hash_code, num_buckets_idx);
    uintptr_t h2 = __RSBasicHashFold(hash_code / num_buckets, num_buckets_idx);
#else
    uintptr_t h1 = hash_code % num_buckets;
    uintptr_t h2 = (hash_code / num_buckets) % num_buckets;
#endif
    if (0 == h2) h2 = num_buckets - 1;
#elif FIND_BUCKET_HASH_STYLE == 3	// __RSBasicHashExponentialHashingValue
    // Improved exponential hashing
    // probe[0] = h1(k)
    // probe[i] = (h1(k) + pr(k)^i * h2(k)) mod num_buckets, i = 1 .. num_buckets - 1
    // h1(k) = k mod num_buckets
    // h2(k) = floor(k / num_buckets) mod num_buckets
    // note: h2(k) has the effect of rotating the sequence if it is constant
    // note: pr(k) is any primitive root of num_buckets, varying this gives different sequences
#if defined(__arm__)
    uintptr_t h1 = __RSBasicHashFold(hash_code, num_buckets_idx);
    uintptr_t h2 = __RSBasicHashFold(hash_code / num_buckets, num_buckets_idx);
#else
    uintptr_t h1 = hash_code % num_buckets;
    uintptr_t h2 = (hash_code / num_buckets) % num_buckets;
#endif
    if (0 == h2) h2 = num_buckets - 1;
    uintptr_t pr = __RSBasicHashPrimitiveRoots[num_buckets_idx];
#endif
    
    COCOA_HASHTABLE_PROBING_START(ht, num_buckets);
    RSBasicHashValue *keys = (ht->bits.keys_offset) ? __RSBasicHashGetKeys(ht) : __RSBasicHashGetValues(ht);
#if !FIND_BUCKET_FOR_REHASH
    uintptr_t *hashes = (__RSBasicHashHasHashCache(ht)) ? __RSBasicHashGetHashes(ht) : NULL;
#endif
    RSIndex deleted_idx = RSNotFound;
    uintptr_t probe = h1;
#if FIND_BUCKET_HASH_STYLE == 3	// __RSBasicHashExponentialHashingValue
    uintptr_t acc = pr;
#endif
    for (RSIndex idx = 0; idx < num_buckets; idx++) {
        uintptr_t curr_key = keys[probe].neutral;
        if (curr_key == 0UL) {
            COCOA_HASHTABLE_PROBE_EMPTY(ht, probe);
#if FIND_BUCKET_FOR_REHASH
            RSIndex result = (RSNotFound == deleted_idx) ? probe : deleted_idx;
#else
            RSBasicHashBucket result;
            result.idx = (RSNotFound == deleted_idx) ? probe : deleted_idx;
            result.count = 0;
#endif
            COCOA_HASHTABLE_PROBING_END(ht, idx + 1);
            return result;
#if !FIND_BUCKET_FOR_REHASH
        } else if (curr_key == ~0UL) {
            COCOA_HASHTABLE_PROBE_DELETED(ht, probe);
            if (RSNotFound == deleted_idx) {
                deleted_idx = probe;
            }
        } else {
            COCOA_HASHTABLE_PROBE_VALID(ht, probe);
            if (__RSBasicHashSubABZero == curr_key) curr_key = 0UL;
            if (__RSBasicHashSubABOne == curr_key) curr_key = ~0UL;
#if FIND_BUCKET_FOR_INDIRECT_KEY
            // curr_key holds the value coming in here
            curr_key = ht->callbacks->getIndirectKey(ht, curr_key);
#endif
            if (curr_key == stack_key || ((!hashes || hashes[probe] == hash_code) && __RSBasicHashTestEqualKey(ht, curr_key, stack_key))) {
                COCOA_HASHTABLE_PROBING_END(ht, idx + 1);
#if FIND_BUCKET_FOR_REHASH
                RSIndex result = probe;
#else
                RSBasicHashBucket result;
                result.idx = probe;
                result.weak_value = __RSBasicHashGetValue(ht, probe);
                result.weak_key = curr_key;
                result.count = (ht->bits.counts_offset) ? __RSBasicHashGetSlotCount(ht, probe) : 1;
#endif
                return result;
            }
#endif
        }
        
#if FIND_BUCKET_HASH_STYLE == 1	// __RSBasicHashLinearHashingValue
        probe += 1;
        if (num_buckets <= probe) {
            probe -= num_buckets;
        }
#elif FIND_BUCKET_HASH_STYLE == 2	// __RSBasicHashDoubleHashingValue
        probe += h2;
        if (num_buckets <= probe) {
            probe -= num_buckets;
        }
#elif FIND_BUCKET_HASH_STYLE == 3	// __RSBasicHashExponentialHashingValue
        probe = h1 + h2 * acc;
        if (num_buckets <= probe) {
#if defined(__arm__)
            probe = __RSBasicHashFold(probe, num_buckets_idx);
#else
            probe = probe % num_buckets;
#endif
        }
        acc = acc * pr;
        if (num_buckets <= acc) {
#if defined(__arm__)
            acc = __RSBasicHashFold(acc, num_buckets_idx);
#else
            acc = acc % num_buckets;
#endif
        }
#endif
        
    }
    COCOA_HASHTABLE_PROBING_END(ht, num_buckets);
#if FIND_BUCKET_FOR_REHASH
    RSIndex result = deleted_idx;
#else
    RSBasicHashBucket result;
    result.idx = deleted_idx;
    result.count = 0;
#endif
    return result; // all buckets full or deleted, return first deleted element which was found
}
#undef FIND_BUCKET_NAME
#undef FIND_BUCKET_HASH_STYLE
#undef FIND_BUCKET_FOR_REHASH
#undef FIND_BUCKET_FOR_INDIRECT_KEY