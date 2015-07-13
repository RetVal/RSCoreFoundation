//
//  BasicHashFindBucket.cpp
//  CoreFoundation
//
//  Created by closure on 6/14/15.
//  Copyright © 2015 RetVal. All rights reserved.
//

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

/*	BasicHashFindBucket.m
 Copyright (c) 2009-2013, Apple Inc. All rights reserved.
 Responsibility: Christopher Kane
 */

#if !defined(FIND_BUCKET_NAME) || !defined(FIND_BUCKET_HASH_STYLE) || !defined(FIND_BUCKET_FOR_REHASH) || !defined(FIND_BUCKET_FOR_INDIRECT_KEY)
#error All of FIND_BUCKET_NAME, FIND_BUCKET_HASH_STYLE, FIND_BUCKET_FOR_REHASH, and FIND_BUCKET_FOR_INDIRECT_KEY must be defined before #including this file.
#endif


// During rehashing of a mutable BasicHash, we know that there are no
// deleted slots and the keys have already been uniqued. When rehashing,
// if key_hash is non-0, we use it as the hash code.
#if FIND_BUCKET_FOR_REHASH
Index
#else
Bucket &&
#endif
FIND_BUCKET_NAME (uintptr_t stack_key
#if FIND_BUCKET_FOR_REHASH
                  , uintptr_t key_hash
#endif
) const {
    uint8_t num_buckets_idx = bits.num_buckets_idx;
    uintptr_t num_buckets = BasicHashTable::TableSizes[num_buckets_idx];
#if FIND_BUCKET_FOR_REHASH
    HashCode hash_code = key_hash ? key_hash : _HashKey(stack_key);
#else
    HashCode hash_code = _HashKey(stack_key);
#endif
    
#if FIND_BUCKET_HASH_STYLE == 1	// _LinearHashingValue
    // Linear probing, with c = 1
    // probe[0] = h1(k)
    // probe[i] = (h1(k) + i * c) mod num_buckets, i = 1 .. num_buckets - 1
    // h1(k) = k mod num_buckets
#if defined(__arm__)
    uintptr_t h1 = _Fold(hash_code, num_buckets_idx);
#else
    uintptr_t h1 = hash_code % num_buckets;
#endif
#elif FIND_BUCKET_HASH_STYLE == 2	// _DoubleHashingValue
    // Double hashing
    // probe[0] = h1(k)
    // probe[i] = (h1(k) + i * h2(k)) mod num_buckets, i = 1 .. num_buckets - 1
    // h1(k) = k mod num_buckets
    // h2(k) = floor(k / num_buckets) mod num_buckets
#if defined(__arm__)
    uintptr_t h1 = _Fold(hash_code, num_buckets_idx);
    uintptr_t h2 = _Fold(hash_code / num_buckets, num_buckets_idx);
#else
    uintptr_t h1 = hash_code % num_buckets;
    uintptr_t h2 = (hash_code / num_buckets) % num_buckets;
#endif
    if (0 == h2) h2 = num_buckets - 1;
#elif FIND_BUCKET_HASH_STYLE == 3	// _ExponentialHashingValue
    // Improved exponential hashing
    // probe[0] = h1(k)
    // probe[i] = (h1(k) + pr(k)^i * h2(k)) mod num_buckets, i = 1 .. num_buckets - 1
    // h1(k) = k mod num_buckets
    // h2(k) = floor(k / num_buckets) mod num_buckets
    // note: h2(k) has the effect of rotating the sequence if it is constant
    // note: pr(k) is any primitive root of num_buckets, varying this gives different sequences
#if defined(__arm__)
    uintptr_t h1 = _Fold(hash_code, num_buckets_idx);
    uintptr_t h2 = _Fold(hash_code / num_buckets, num_buckets_idx);
#else
    uintptr_t h1 = hash_code % num_buckets;
    uintptr_t h2 = (hash_code / num_buckets) % num_buckets;
#endif
    if (0 == h2) h2 = num_buckets - 1;
    uintptr_t pr = BasicHashTable::PrimitiveRoots[num_buckets_idx];
#endif
    
    COCOA_HASHTABLE_PROBING_START(this, num_buckets);
    Value *keys = (bits.keys_offset) ? (Value *)_GetKeys() : _GetValues();
#if !FIND_BUCKET_FOR_REHASH
    uintptr_t *hashes = (_HasHashCache()) ? _GetHashes() : nil;
#endif
    Index deleted_idx = NotFound;
    uintptr_t probe = h1;
#if FIND_BUCKET_HASH_STYLE == 3	// _ExponentialHashingValue
    uintptr_t acc = pr;
#endif
    for (Index idx = 0; idx < num_buckets; idx++) {
        uintptr_t curr_key = keys[probe].neutral;
        if (curr_key == 0UL) {
            COCOA_HASHTABLE_PROBE_EMPTY(this, probe);
#if FIND_BUCKET_FOR_REHASH
            Index result = (NotFound == deleted_idx) ? probe : deleted_idx;
#else
            Bucket result;
            result.idx = (NotFound == deleted_idx) ? probe : deleted_idx;
            result.count = 0;
#endif
            COCOA_HASHTABLE_PROBING_END(this, idx + 1);
            return MoveValue(result);
#if !FIND_BUCKET_FOR_REHASH
        } else if (curr_key == ~0UL) {
            COCOA_HASHTABLE_PROBE_DELETED(this, probe);
            if (NotFound == deleted_idx) {
                deleted_idx = probe;
            }
        } else {
            COCOA_HASHTABLE_PROBE_VALID(this, probe);
            if (SubABZero == curr_key) curr_key = 0UL;
            if (SubABOne == curr_key) curr_key = ~0UL;
#if FIND_BUCKET_FOR_INDIRECT_KEY
            // curr_key holds the value coming in here
            curr_key = _GetIndirectKey(curr_key);
#endif
            if (curr_key == stack_key || ((!hashes || hashes[probe] == hash_code) && _EqualValue(curr_key, stack_key))) {
                COCOA_HASHTABLE_PROBING_END(this, idx + 1);
#if FIND_BUCKET_FOR_REHASH
                Index result = probe;
                return result;
#else
                Bucket result;
                result.idx = probe;
                result.weak_value = _GetValue(probe);
                result.weak_key = curr_key;
                result.count = (bits.counts_offset) ? _GetSlotCount(probe) : 1;
                return MoveValue(result);
#endif
            }
#endif
        }
        
#if FIND_BUCKET_HASH_STYLE == 1	// _LinearHashingValue
        probe += 1;
        if (num_buckets <= probe) {
            probe -= num_buckets;
        }
#elif FIND_BUCKET_HASH_STYLE == 2	// _DoubleHashingValue
        probe += h2;
        if (num_buckets <= probe) {
            probe -= num_buckets;
        }
#elif FIND_BUCKET_HASH_STYLE == 3	// _ExponentialHashingValue
        probe = h1 + h2 * acc;
        if (num_buckets <= probe) {
#if defined(__arm__)
            probe = _Fold(probe, num_buckets_idx);
#else
            probe = probe % num_buckets;
#endif
        }
        acc = acc * pr;
        if (num_buckets <= acc) {
#if defined(__arm__)
            acc = _Fold(acc, num_buckets_idx);
#else
            acc = acc % num_buckets;
#endif
        }
#endif
        
    }
    COCOA_HASHTABLE_PROBING_END(this, num_buckets);
#if FIND_BUCKET_FOR_REHASH
    Index result = deleted_idx;
    return result;
#else
    Bucket result = {
        .count = 0,
        .idx = deleted_idx
    };
    return MoveValue(result);
#endif
     // all buckets full or deleted, return first deleted element which was found
}

#undef FIND_BUCKET_NAME
#undef FIND_BUCKET_HASH_STYLE
#undef FIND_BUCKET_FOR_REHASH
#undef FIND_BUCKET_FOR_INDIRECT_KEY
