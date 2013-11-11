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

#if (RSBasicHashVersion == 1)
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
#elif (RSBasicHashVersion == 2)
/*
 * Copyright (c) 2013 Apple Inc. All rights reserved.
 *
 * @APPLE_LICENSE_HEADER_START@
 *
 * This file contains Original Code and/or Modifications of Original Code
 * as defined in and that are subject to the Apple Public Source License
 * Version 2.0 (the 'License'). You may not use this file except in
 * compliance with the License. Please obtain a copy of the License at
 * http://www.opensource.apple.com/apsl/ and read it before using this
 * file.
 *
 * The Original Code and all software distributed under the License are
 * distributed on an 'AS IS' basis, WITHOUT WARRANTY OF ANY KIND, EITHER
 * EXPRESS OR IMPLIED, AND APPLE HEREBY DISCLAIMS ALL SUCH WARRANTIES,
 * INCLUDING WITHOUT LIMITATION, ANY WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE, QUIET ENJOYMENT OR NON-INFRINGEMENT.
 * Please see the License for the specific language governing rights and
 * limitations under the License.
 *
 * @APPLE_LICENSE_HEADER_END@
 */

/*	RSBasicHashFindBucket.m
 Copyright (c) 2009-2013, Apple Inc. All rights reserved.
 Responsibility: Christopher Kane
 */


#if !defined(FIND_BUCKET_NAME) || !defined(FIND_BUCKET_HASH_STYLE) || !defined(FIND_BUCKET_FOR_REHASH) || !defined(FIND_BUCKET_FOR_INDIRECT_KEY)
#error All of FIND_BUCKET_NAME, FIND_BUCKET_HASH_STYLE, FIND_BUCKET_FOR_REHASH, and FIND_BUCKET_FOR_INDIRECT_KEY must be defined before #including this file.
#endif


// During rehashing of a mutable RSBasicHash, we know that there are no
// deleted slots and the keys have already been uniqued. When rehashing,
// if key_hash is non-0, we use it as the hash code.
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
            curr_key = __RSBasicHashGetIndirectKey(ht, curr_key);
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

#endif
