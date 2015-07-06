//
//  BasicHash.hpp
//  RSCoreFoundation
//
//  Created by closure on 6/16/15.
//  Copyright © 2015 RetVal. All rights reserved.
//

#ifndef BasicHash_cpp
#define BasicHash_cpp

#include <RSFoundation/String.hpp>
#include <RSFoundation/Allocator.hpp>

namespace RSFoundation {
    namespace Basic {
        using namespace Collection;
        class BasicHashTable : public Object {
        public:
            typedef uintptr_t TableElementType;
            static const int TableSize = 64;
            static const TableElementType TableSizes[TableSize];
            static const TableElementType TableCapacities[TableSize];
            static const TableElementType PrimitiveRoots[TableSize];
        };
        
        class BasicHash : public Object {
        public:
            typedef struct {
                Index idx;
                uintptr_t weak_key;
                uintptr_t weak_value;
                uintptr_t count;
            } Bucket;
            
            enum {
                LinearHashingValue = 1,
                DoubleHashingValue = 2,
                ExponentialHashingValue = 3,
            };
            
            enum {
                HasKeys = (1UL << 0),
                HasCounts = (1UL << 1),
                HasHashCache = (1UL << 2),
                
                IntegerValues = (1UL << 6),
                IntegerKeys = (1UL << 7),
                
                StrongValues = (1UL << 8),
                StrongKeys = (1UL << 9),
                
                WeakValues = (1UL << 10),
                WeakKeys = (1UL << 11),
                
                IndirectKeys = (1UL << 12),
                
                LinearHashing = (LinearHashingValue << 13), // bits 13-14
                DoubleHashing = (DoubleHashingValue << 13),
                ExponentialHashing = (ExponentialHashingValue << 13),
                
                AggressiveGrowth = (1UL << 15),
                
                CompactableValues = (1UL << 16),
                CompactableKeys = (1UL << 17),
            };
    
        private:
            BasicHash() {
                
            }
            
            typedef union {
                uintptr_t neutral;
                void *strong;
                void *weak;
            } Value ;
            
        private:
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
            
        private:
            inline bool _HasStrongValues() const { return bits.strong_values ? true : false; }
            inline bool _HasStrongKeys() const { return bits.strong_keys ? true : false; }
            
            inline bool _HasWeakValues() const { return bits.weak_values ? true : false; }
            inline bool _HasWeakKeys() const { return bits.weak_keys ? true : false; }
            
            inline bool _HasHashCache() const { return bits.hashes_offset ? true : false; }
            
            inline uintptr_t _ImportValue(uintptr_t stack_value) const {
                assert(bits.null_rc);
                return stack_value;
            }
            
            inline uintptr_t _ImportKey(uintptr_t stack_key) const {
                assert(bits.null_rc);
                return stack_key;
            }
            
            inline void _EjectValue(uintptr_t stack_value) const {
                assert(bits.null_rc);
                return;
            }
            
            inline void _EjectKey(uintptr_t stack_key) const {
                assert(bits.null_rc);
                return;
            }
            
            inline const String* _DescValue(uintptr_t stack_value) const {
                return &String::Empty;
            }
            
            inline const String* _DescKey(uintptr_t stack_key) const {
                return &String::Empty;
            }
            
            inline bool _EqualValue(uintptr_t stack_value_a, uintptr_t stack_value_b) const {
                return stack_value_a == stack_value_b;
            }
            
            inline HashCode _HashKey(uintptr_t stack_key) const {
                //                HashCode (*func)(uintptr_t) = (RSHashCode (*)(uintptr_t))RSBasicHashCallBackPtrs[bits.__khas];
                //                HashCode hash_code = func ? func(stack_key) : stack_key;
                HashCode hash_code = stack_key;
                //                COCOA_HASHTABLE_HASH_KEY(this, stack_key, hash_code);
                return hash_code;
            }
            
            inline uintptr_t _GetIndirectKey(uintptr_t coll_key) const {
                //                uintptr_t (*func)(uintptr_t) = (uintptr_t (*)(uintptr_t))BasicHashCallBackPtrs[bits.__kget];
                return coll_key;
            }
            
            inline Value *_GetValues() const {
                return (Value *)(pointers[0]);
            }
            
            inline void _SetValues(const Value *ptr) {
                pointers[0] = (void *)ptr;
            }
            
            inline Value *_GetKeys() const {
                return (Value *)(pointers[bits.keys_offset]);
            }
            
            inline void _SetKeys(const Value *ptr) {
                *(&pointers[bits.keys_offset]) = (void *)ptr;
            }
            
            inline void *_GetCounts() const {
                return static_cast<void *>(pointers[bits.counts_offset]);
            }
            
            inline void _SetCounts(void *ptr) {
                *(&pointers[bits.counts_offset]) = ptr;
            }
            
            inline uintptr_t _GetValue(Index idx) const {
                uintptr_t val = _GetValues()[idx].neutral;
                if (SubABZero == val) { return 0UL; }
                else if (SubABOne == val) { return ~0UL; }
                return val;
            }
            
            inline void _SetValue(Index idx, uintptr_t stack_value, bool ignoreOld, bool literal) {
                Value *valuep = &(_GetValues()[idx]);
                uintptr_t old_value = ignoreOld ? 0 : valuep->neutral;
                if (literal) {
                    if (0UL == stack_value) { stack_value = SubABZero; }
                    else if (~0UL == stack_value) { stack_value = SubABOne; }
                }
                if (_HasStrongValues()) {
                    valuep->strong = (void *)stack_value;
                } else {
                    valuep->neutral = stack_value;
                }
                
                if (!ignoreOld) {
                    if (!(old_value == 0UL || old_value == ~0UL)) {
                        if (SubABZero == old_value) {
                            old_value = 0UL;
                        } else if (SubABOne == old_value) {
                            old_value = ~0UL;
                        }
                        _EjectValue(old_value);
                    }
                }
            }
            
            inline uintptr_t _GetKey(Index idx) const {
                if (bits.keys_offset) {
                    uintptr_t key = _GetKeys()[idx].neutral;
                    if (0UL == key) { key = SubABZero; }
                    else if (~0UL == key) { key = SubABOne; }
                    return key;
                }
                if (bits.indirect_keys) {
                    uintptr_t stack_value = _GetValue(idx);
                    return _GetIndirectKey(stack_value);
                }
                return _GetValue(idx);
            }
            
            inline void _SetKey(Index idx, uintptr_t stack_key, bool ignoreOld, bool literal) {
                Value *keyp = &(_GetKeys()[idx]);
                uintptr_t old_key = ignoreOld ? 0 : keyp->neutral;
                if (!literal) {
                    if (0UL == stack_key) {
                        stack_key = SubABZero;
                    } else if (~0UL == stack_key) {
                        stack_key = SubABOne;
                    }
                }
                if (_HasStrongValues()) {
                    keyp->strong = (void *)stack_key;
                } else {
                    keyp->neutral = stack_key;
                }
                if (!ignoreOld) {
                    if (!(old_key == 0UL || old_key == ~0UL)) {
                        if (SubABZero == old_key) {
                            old_key = 0UL;
                        } else if (SubABOne == old_key) {
                            old_key = ~0UL;
                        }
                        _EjectValue(old_key);
                    }
                }
            }
            
            inline uintptr_t _IsEmptyOrDeleted(Index idx) const {
                uintptr_t stack_value = _GetValues()[idx].neutral;
                return (0UL == stack_value || ~0UL == stack_value);
            }
            
            inline uintptr_t _IsDeleted(Index idx) const {
                uintptr_t stack_value = _GetValues()[idx].neutral;
                return (0UL == stack_value);
            }
            
            inline uintptr_t _GetSlotCount(Index idx) const {
                void *counts = _GetCounts();
                switch (bits.counts_width) {
                    case 0: return ((uint8_t *)counts)[idx];
                    case 1: return ((uint16_t *)counts)[idx];
                    case 2: return ((uint32_t *)counts)[idx];
                    case 3: return ((uint64_t *)counts)[idx];
                }
                return 0;
            }
            
#define HALT
            
            inline void _BumpCounts() {
                void *counts = _GetCounts();
                switch (bits.counts_width) {
                    case 0: {
                        uint8_t *counts08 = (uint8_t *)counts;
                        bits.counts_width = 1;
                        Index num_buckets = BasicHashTable::TableSizes[bits.num_buckets_idx];
                        uint16_t *counts16 = (uint16_t *)__AllocateMemory(num_buckets, 2, false, false);
                        if (!counts16) HALT;
                        //                        __SetLastAllocationEventName(counts16, "BasicHash (count-store)");
                        for (Index idx2 = 0; idx2 < num_buckets; idx2++) {
                            counts16[idx2] = counts08[idx2];
                        }
                        _SetCounts(counts16);
                        auto allocator = &Allocator<uint8_t*>::SystemDefault;
                        if (!allocator->IsGC()) {
                            allocator->Deallocate(counts08);
                        }
                        break;
                    }
                    case 1: {
                        uint16_t *counts16 = (uint16_t *)counts;
                        bits.counts_width = 2;
                        Index num_buckets = BasicHashTable::TableSizes[bits.num_buckets_idx];
                        uint32_t *counts32 = (uint32_t *)__AllocateMemory(num_buckets, 4, false, false);
                        if (!counts32) HALT;
                        //                        __SetLastAllocationEventName(counts32, "BasicHash (count-store)");
                        for (Index idx2 = 0; idx2 < num_buckets; idx2++) {
                            counts32[idx2] = counts16[idx2];
                        }
                        _SetCounts(counts32);
                        auto allocator = &Allocator<uint16_t*>::SystemDefault;
                        if (!allocator->IsGC()) {
                            allocator->Deallocate(counts16);
                        }
                        break;
                    }
                    case 2: {
                        uint32_t *counts32 = (uint32_t *)counts;
                        bits.counts_width = 3;
                        Index num_buckets = BasicHashTable::TableSizes[bits.num_buckets_idx];
                        uint64_t *counts64 = (uint64_t *)__AllocateMemory(num_buckets, 8, false, false);
                        if (!counts64) HALT;
                        //                        __SetLastAllocationEventName(counts64, "BasicHash (count-store)");
                        for (Index idx2 = 0; idx2 < num_buckets; idx2++) {
                            counts64[idx2] = counts32[idx2];
                        }
                        _SetCounts(counts64);
                        auto allocator = &Allocator<uint32_t*>::SystemDefault;
                        if (!allocator->IsGC()) {
                            allocator->Deallocate(counts32);
                        }
                        break;
                    }
                    case 3: {
                        HALT;
                        break;
                    }
                }
            }
            
            void _IncSlotCount(Index idx) {
                void *counts = _GetCounts();
                switch (bits.counts_width) {
                    case 0: {
                        uint8_t *counts08 = (uint8_t *)counts;
                        uint8_t val = counts08[idx];
                        if (val < INT8_MAX) {
                            counts08[idx] = val + 1;
                            return;
                        }
                        _BumpCounts();
                        _IncSlotCount(idx);
                        break;
                    }
                    case 1: {
                        uint16_t *counts16 = (uint16_t *)counts;
                        uint16_t val = counts16[idx];
                        if (val < INT16_MAX) {
                            counts16[idx] = val + 1;
                            return;
                        }
                        _BumpCounts();
                        _IncSlotCount(idx);
                        break;
                    }
                    case 2: {
                        uint32_t *counts32 = (uint32_t *)counts;
                        uint32_t val = counts32[idx];
                        if (val < INT32_MAX) {
                            counts32[idx] = val + 1;
                            return;
                        }
                        _BumpCounts();
                        _IncSlotCount(idx);
                        break;
                    }
                    case 3: {
                        uint64_t *counts64 = (uint64_t *)counts;
                        uint64_t val = counts64[idx];
                        if (val < INT64_MAX) {
                            counts64[idx] = val + 1;
                            return;
                        }
                        _BumpCounts();
                        _IncSlotCount(idx);
                        break;
                    }
                }
            }
            
            inline void _DecSlotCount(Index idx) {
                void *counts = _GetCounts();
                switch (bits.counts_width) {
                    case 0: ((uint8_t  *)counts)[idx]--; return;
                    case 1: ((uint16_t *)counts)[idx]--; return;
                    case 2: ((uint32_t *)counts)[idx]--; return;
                    case 3: ((uint64_t *)counts)[idx]--; return;
                }
            }
            
            inline uintptr_t *_GetHashes() const {
                return (uintptr_t *)pointers[bits.hashes_offset];
            }
            
            inline void * objc_assign_strongCast(void * val, void **dest)
            { return (*dest = val); }
            
#define __AssignWithWriteBarrier(location, value) objc_assign_strongCast((void*)value, (void* *)location)
            
            inline void _SetHashes(uintptr_t *ptr) {
                __AssignWithWriteBarrier(&pointers[bits.hashes_offset], ptr);
            }
            
            
            // to expose the load factor, expose this function to customization
            inline Index _GetCapacityForNumBuckets(Index num_buckets_idx) {
                return BasicHashTable::TableCapacities[num_buckets_idx];
            }
            
            inline Index _GetNumBucketsIndexForCapacity(Index capacity) {
                for (Index idx = 0; idx < 64; idx++) {
                    if (capacity <= _GetCapacityForNumBuckets(idx)) return idx;
                }
                HALT;
                return 0;
            }
            
            inline Index GetNumBuckets() const {
                return BasicHashTable::TableSizes[bits.num_buckets_idx];
            }
            
            inline Index GetCapacity() {
                return _GetCapacityForNumBuckets(bits.num_buckets_idx);
            }
            
            inline Bucket&& GetBucket(Index idx) const {
                Bucket result;
                result.idx = idx;
                if (_IsEmptyOrDeleted(idx)) {
                    result.count = 0;
                    result.weak_value = 0;
                    result.weak_key = 0;
                } else {
                    result.count = bits.counts_offset;
                    result.weak_value = _GetValue(idx);
                    result.weak_key = _GetKey(idx);
                }
                return MoveValue(result);
            }
            
#if defined(__arm__)
            static uintptr_t _HashFold(uintptr_t dividend, uint8_t idx) {
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
            
        private:
            
#define FIND_BUCKET_NAME		___BasicHashFindBucket_Linear
#define FIND_BUCKET_HASH_STYLE		1
#define FIND_BUCKET_FOR_REHASH		0
#define FIND_BUCKET_FOR_INDIRECT_KEY	0
#include "BasicHashFindBucket.cpp"
            
#define FIND_BUCKET_NAME		___BasicHashFindBucket_Linear_NoCollision
#define FIND_BUCKET_HASH_STYLE		1
#define FIND_BUCKET_FOR_REHASH		1
#define FIND_BUCKET_FOR_INDIRECT_KEY	0
#include "BasicHashFindBucket.cpp"
            
#define FIND_BUCKET_NAME		___BasicHashFindBucket_Linear_Indirect
#define FIND_BUCKET_HASH_STYLE		1
#define FIND_BUCKET_FOR_REHASH		0
#define FIND_BUCKET_FOR_INDIRECT_KEY	1
#include "BasicHashFindBucket.cpp"
            
#define FIND_BUCKET_NAME		___BasicHashFindBucket_Linear_Indirect_NoCollision
#define FIND_BUCKET_HASH_STYLE		1
#define FIND_BUCKET_FOR_REHASH		1
#define FIND_BUCKET_FOR_INDIRECT_KEY	1
#include "BasicHashFindBucket.cpp"
            
#define FIND_BUCKET_NAME		___BasicHashFindBucket_Double
#define FIND_BUCKET_HASH_STYLE		2
#define FIND_BUCKET_FOR_REHASH		0
#define FIND_BUCKET_FOR_INDIRECT_KEY	0
#include "BasicHashFindBucket.cpp"
            
#define FIND_BUCKET_NAME		___BasicHashFindBucket_Double_NoCollision
#define FIND_BUCKET_HASH_STYLE		2
#define FIND_BUCKET_FOR_REHASH		1
#define FIND_BUCKET_FOR_INDIRECT_KEY	0
#include "BasicHashFindBucket.cpp"
            
#define FIND_BUCKET_NAME		___BasicHashFindBucket_Double_Indirect
#define FIND_BUCKET_HASH_STYLE		2
#define FIND_BUCKET_FOR_REHASH		0
#define FIND_BUCKET_FOR_INDIRECT_KEY	1
#include "BasicHashFindBucket.cpp"
            
#define FIND_BUCKET_NAME		___BasicHashFindBucket_Double_Indirect_NoCollision
#define FIND_BUCKET_HASH_STYLE		2
#define FIND_BUCKET_FOR_REHASH		1
#define FIND_BUCKET_FOR_INDIRECT_KEY	1
#include "BasicHashFindBucket.cpp"
            
#define FIND_BUCKET_NAME		___BasicHashFindBucket_Exponential
#define FIND_BUCKET_HASH_STYLE		3
#define FIND_BUCKET_FOR_REHASH		0
#define FIND_BUCKET_FOR_INDIRECT_KEY	0
#include "BasicHashFindBucket.cpp"
            
#define FIND_BUCKET_NAME		___BasicHashFindBucket_Exponential_NoCollision
#define FIND_BUCKET_HASH_STYLE		3
#define FIND_BUCKET_FOR_REHASH		1
#define FIND_BUCKET_FOR_INDIRECT_KEY	0
#include "BasicHashFindBucket.cpp"
            
#define FIND_BUCKET_NAME		___BasicHashFindBucket_Exponential_Indirect
#define FIND_BUCKET_HASH_STYLE		3
#define FIND_BUCKET_FOR_REHASH		0
#define FIND_BUCKET_FOR_INDIRECT_KEY	1
#include "BasicHashFindBucket.cpp"
            
#define FIND_BUCKET_NAME		___BasicHashFindBucket_Exponential_Indirect_NoCollision
#define FIND_BUCKET_HASH_STYLE		3
#define FIND_BUCKET_FOR_REHASH		1
#define FIND_BUCKET_FOR_INDIRECT_KEY	1
#include "BasicHashFindBucket.cpp"
            
        private:
            inline Bucket &&_HashFindBucket(uintptr_t stack_key) const {
                if (0 == bits.num_buckets_idx) {
                    Bucket result = {NotFound, 0UL, 0UL, 0};
                    return MoveValue(result);
                }
                if (bits.indirect_keys) {
                    switch (bits.hash_style) {
                        case LinearHashingValue: return MoveValue(___BasicHashFindBucket_Linear_Indirect(stack_key));
                        case DoubleHashingValue: return MoveValue(___BasicHashFindBucket_Double_Indirect(stack_key));
                        case ExponentialHashingValue: return MoveValue(___BasicHashFindBucket_Exponential_Indirect(stack_key));
                    }
                } else {
                    switch (bits.hash_style) {
                        case LinearHashingValue: return MoveValue(___BasicHashFindBucket_Linear(stack_key));
                        case DoubleHashingValue: return MoveValue(___BasicHashFindBucket_Double(stack_key));
                        case ExponentialHashingValue: return MoveValue(___BasicHashFindBucket_Exponential(stack_key));
                    }
                }
                HALT;
                Bucket result = {NotFound, 0UL, 0UL, 0};
                return MoveValue(result);
            }
            
            inline Index _FindBucket_NoCollision(uintptr_t stack_key, uintptr_t key_hash) const {
                if (0 == bits.num_buckets_idx) {
                    return NotFound;
                }
                if (bits.indirect_keys) {
                    switch (bits.hash_style) {
                        case LinearHashingValue: return ___BasicHashFindBucket_Linear_Indirect_NoCollision(stack_key, key_hash);
                        case DoubleHashingValue: return ___BasicHashFindBucket_Double_Indirect_NoCollision(stack_key, key_hash);
                        case ExponentialHashingValue: return ___BasicHashFindBucket_Exponential_Indirect_NoCollision(stack_key, key_hash);
                    }
                } else {
                    switch (bits.hash_style) {
                        case LinearHashingValue: return ___BasicHashFindBucket_Linear_NoCollision(stack_key, key_hash);
                        case DoubleHashingValue: return ___BasicHashFindBucket_Double_NoCollision(stack_key, key_hash);
                        case ExponentialHashingValue: return ___BasicHashFindBucket_Exponential_NoCollision(stack_key, key_hash);
                    }
                }
                HALT;
                return NotFound;
            }
            
#if ENABLE_MEMORY_COUNTERS
            static volatile Int64 TotalCount = 0ULL;
            static volatile Int64 TotalSize = 0ULL;
            static volatile Int64 PeakCount = 0ULL;
            static volatile Int64 PeakSize = 0ULL;
            static volatile Int32 Sizes[64] = {0};
#endif
            
            
            void _Drain(bool forFinalization) {
#if ENABLE_MEMORY_COUNTERS
                OSAtomicAdd64Barrier(-1 * (int64_t) RSBasicHashGetSize(ht, true), & TotalSize);
#endif
                
                Index old_num_buckets = BasicHashTable::TableSizes[bits.num_buckets_idx];
                
                //                RSAllocatorRef allocator = RSGetAllocator(ht);
                bool nullify = (!forFinalization || true);
                
                Value *old_values = nullptr, *old_keys = nullptr;
                void *old_counts = nullptr;
                uintptr_t *old_hashes = nullptr;
                
                old_values = _GetValues();
                if (nullify) _SetValues(nullptr);
                if (bits.keys_offset) {
                    old_keys = _GetKeys();
                    if (nullify) _SetKeys(nullptr);
                }
                if (bits.counts_offset) {
                    old_counts = _GetCounts();
                    if (nullify) _SetCounts(nullptr);
                }
                if (_HasHashCache()) {
                    old_hashes = _GetHashes();
                    if (nullify) _SetHashes(nullptr);
                }
                
                if (nullify) {
                    bits.mutations++;
                    bits.num_buckets_idx = 0;
                    bits.used_buckets = 0;
                    bits.deleted = 0;
                }
                
                for (Index idx = 0; idx < old_num_buckets; idx++) {
                    uintptr_t stack_value = old_values[idx].neutral;
                    if (stack_value != 0UL && stack_value != ~0UL) {
                        uintptr_t old_value = stack_value;
                        if (SubABZero == old_value) old_value = 0UL;
                        if (SubABOne == old_value) old_value = ~0UL;
                        _EjectValue(old_value);
                        if (old_keys) {
                            uintptr_t old_key = old_keys[idx].neutral;
                            if (SubABZero == old_key) old_key = 0UL;
                            if (SubABOne == old_key) old_key = ~0UL;
                            _EjectKey(old_key);
                        }
                    }
                }
                
                auto allocator = &Allocator<Value *>::SystemDefault;
                if (!allocator->IsGC()) {
                    allocator->Deallocate(old_values);
                    allocator->Deallocate(old_keys);
                    allocator->Deallocate(old_counts);
                    allocator->Deallocate(old_hashes);
                }
                
                
#if ENABLE_MEMORY_COUNTERS
                int64_t size_now = OSAtomicAdd64Barrier((int64_t) RSBasicHashGetSize(true), & TotalSize);
                while (PeakSize < size_now && !OSAtomicCompareAndSwap64Barrier(PeakSize, size_now, & PeakSize));
#endif
            }
            
            void _Rehash(Index newItemCount) {
#if ENABLE_MEMORY_COUNTERS
                OSAtomicAdd64Barrier(-1 * (int64_t) RSBasicHashGetSize(true), & TotalSize);
                OSAtomicAdd32Barrier(-1, &Sizes[bits.num_buckets_idx]);
#endif
                
                if (COCOA_HASHTABLE_REHASH_START_ENABLED()) COCOA_HASHTABLE_REHASH_START(this, GetNumBuckets(), GetSize(true));
                
                Index new_num_buckets_idx = bits.num_buckets_idx;
                if (0 != newItemCount) {
                    if (newItemCount < 0) newItemCount = 0;
                    Index new_capacity_req = bits.used_buckets + newItemCount;
                    new_num_buckets_idx = _GetNumBucketsIndexForCapacity(new_capacity_req);
                    if (1 == newItemCount && bits.fast_grow) {
                        new_num_buckets_idx++;
                    }
                }
                
                Index new_num_buckets = BasicHashTable::TableSizes[new_num_buckets_idx];
                Index old_num_buckets = BasicHashTable::TableSizes[bits.num_buckets_idx];
                
                Value *new_values = nullptr, *new_keys = nullptr;
                void *new_counts = nullptr;
                uintptr_t *new_hashes = nullptr;
                
                if (0 < new_num_buckets) {
                    new_values = (Value *)__AllocateMemory(new_num_buckets, sizeof(Value), _HasStrongValues(), 0);
                    if (!new_values) HALT;
                    //                    __SetLastAllocationEventName(new_values, "BasicHash (value-store)");
                    memset(new_values, 0, new_num_buckets * sizeof(Value));
                    if (bits.keys_offset) {
                        new_keys = (Value *)__AllocateMemory(new_num_buckets, sizeof(Value), _HasStrongValues(), 0);
                        if (!new_keys) HALT;
                        //                        __SetLastAllocationEventName(new_keys, "RSBasicHash (key-store)");
                        memset(new_keys, 0, new_num_buckets * sizeof(Value));
                    }
                    if (bits.counts_offset) {
                        new_counts = (uintptr_t *)__AllocateMemory(new_num_buckets, (1 << bits.counts_width), false, false);
                        if (!new_counts) HALT;
                        //                        __SetLastAllocationEventName(new_counts, "RSBasicHash (count-store)");
                        memset(new_counts, 0, new_num_buckets * (1 << bits.counts_width));
                    }
                    if (_HasHashCache()) {
                        new_hashes = (uintptr_t *)__AllocateMemory(new_num_buckets, sizeof(uintptr_t), false, false);
                        if (!new_hashes) HALT;
                        //                        __SetLastAllocationEventName(new_hashes, "RSBasicHash (hash-store)");
                        memset(new_hashes, 0, new_num_buckets * sizeof(uintptr_t));
                    }
                }
                
                bits.num_buckets_idx = new_num_buckets_idx;
                bits.deleted = 0;
                
                Value *old_values = nullptr, *old_keys = nullptr;
                void *old_counts = nullptr;
                uintptr_t *old_hashes = nullptr;
                
                old_values = _GetValues();
                _SetValues(new_values);
                if (bits.keys_offset) {
                    old_keys = _GetKeys();
                    _SetKeys(new_keys);
                }
                if (bits.counts_offset) {
                    old_counts = _GetCounts();
                    _SetCounts(new_counts);
                }
                if (_HasHashCache()) {
                    old_hashes = _GetHashes();
                    _SetHashes(new_hashes);
                }
                
                if (0 < old_num_buckets) {
                    for (Index idx = 0; idx < old_num_buckets; idx++) {
                        uintptr_t stack_value = old_values[idx].neutral;
                        if (stack_value != 0UL && stack_value != ~0UL) {
                            if (SubABZero == stack_value) stack_value = 0UL;
                            if (SubABOne == stack_value) stack_value = ~0UL;
                            uintptr_t stack_key = stack_value;
                            if (bits.keys_offset) {
                                stack_key = old_keys[idx].neutral;
                                if (SubABZero == stack_key) stack_key = 0UL;
                                if (SubABOne == stack_key) stack_key = ~0UL;
                            }
                            if (bits.indirect_keys) {
                                stack_key = _GetIndirectKey(stack_value);
                            }
                            Index bkt_idx = _FindBucket_NoCollision(stack_key, old_hashes ? old_hashes[idx] : 0UL);
                            _SetValue(bkt_idx, stack_value, false, false);
                            if (old_keys) {
                                _SetKey(bkt_idx, stack_key, false, false);
                            }
                            if (old_counts) {
                                switch (bits.counts_width) {
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
                
                auto allocator = &Allocator<BasicHash *>::SystemDefault;
                if (!allocator->IsGC()) {
                    allocator->Deallocate(old_values);
                    allocator->Deallocate(old_keys);
                    allocator->Deallocate(old_counts);
                    allocator->Deallocate(old_hashes);
                }
                
                if (COCOA_HASHTABLE_REHASH_END_ENABLED()) COCOA_HASHTABLE_REHASH_END(this, GetNumBuckets(), GetSize(true));
                
#if ENABLE_MEMORY_COUNTERS
                int64_t size_now = OSAtomicAdd64Barrier((int64_t) GetSize(true), &TotalSize);
                while (PeakSize < size_now && !OSAtomicCompareAndSwap64Barrier(PeakSize, size_now, & PeakSize));
                OSAtomicAdd32Barrier(1, &Sizes[bits.num_buckets_idx]);
#endif
            }
            
            inline bool IsMutable() const {
                return ((1<<6) == (info & (1<<6))) ? false : true;
            }
            
            void MakeImmutable() {
                info |= 1 << 6;
            }
            
            void _AddValue(Index bkt_idx, uintptr_t stack_key, uintptr_t stack_value) {
                bits.mutations++;
                if (GetCapacity() < bits.used_buckets + 1) {
                    _Rehash(1);
                    bkt_idx = _FindBucket_NoCollision(stack_key, 0);
                } else if (_IsDeleted(bkt_idx)) {
                    bits.deleted--;
                }
                uintptr_t key_hash = 0;
                if (_HasHashCache()) {
                    key_hash = _HashKey(stack_key);
                }
                stack_value = _ImportValue(stack_value);
                if (bits.keys_offset) {
                    stack_key = _ImportKey(stack_key);
                }
                _SetValue(bkt_idx, stack_value, false, false);
                if (bits.keys_offset) {
                    _SetKey(bkt_idx, stack_key, false, false);
                }
                if (bits.counts_offset) {
                    _IncSlotCount(bkt_idx);
                }
                if (_HasHashCache()) {
                    _GetHashes()[bkt_idx] = key_hash;
                }
                bits.used_buckets++;
            }
            
            void _ReplaceValue(Index bkt_idx, uintptr_t stack_key, uintptr_t stack_value) {
                bits.mutations++;
                stack_value = _ImportValue(stack_value);
                if (bits.keys_offset) {
                    stack_key = _ImportKey(stack_key);
                }
                _SetValue(bkt_idx, stack_key, false, false);
                if (bits.keys_offset) {
                    _SetKey(bkt_idx, stack_key, false, false);
                }
            }
            
            void _RemoveValue(Index bkt_idx) {
                bits.mutations++;
                _SetValue(bkt_idx, ~0UL, false, true);
                if (bits.keys_offset) {
                    _SetKey(bkt_idx, ~0UL, false, true);
                }
                if (bits.counts_offset) {
                    _DecSlotCount(bkt_idx);
                }
                if (_HasHashCache()) {
                    _GetHashes()[bkt_idx] = 0;
                }
                
                bits.used_buckets--;
                bits.deleted++;
                bool do_shrink = false;
                if (bits.fast_grow) { // == slow shrink
                    do_shrink = (5 < bits.num_buckets_idx && bits.used_buckets < _GetCapacityForNumBuckets(bits.num_buckets_idx - 5));
                } else {
                    do_shrink = (2 < bits.num_buckets_idx && bits.used_buckets < _GetCapacityForNumBuckets(bits.num_buckets_idx - 2));
                }
                if (do_shrink) {
                    _Rehash(-1);
                    return;
                }
                do_shrink = (0 == bits.deleted); // .deleted roll-over
                Index num_buckets = BasicHashTable::TableSizes[bits.num_buckets_idx];
                do_shrink = do_shrink || ((20 <= num_buckets) && (num_buckets / 4 <= bits.deleted));
                if (do_shrink) {
                    _Rehash(0);
                }
            }
            
        public:
//            friend Hash *Allocator<Hash>::_AllocateImpl<Hash, false>::_Allocate(size_t size);
            
            static BasicHash *Create(OptionFlags flags) {
                size_t size = sizeof(BasicHash);
                if (flags & HasKeys) size += sizeof(Value *); // keys
                if (flags & HasCounts) size += sizeof(void *); // counts
                if (flags & HasHashCache) size += sizeof(uintptr_t *); // hashes
                auto allocator = &Allocator<BasicHash>::SystemDefault;
                BasicHash *ht = (BasicHash *)allocator->Allocate<char *>(size);
                if (nullptr == ht) return nullptr;
                ht->bits.finalized = 0;
                ht->bits.hash_style = (flags >> 13) & 0x3;
                ht->bits.fast_grow = (flags & AggressiveGrowth) ? 1 : 0;
                ht->bits.counts_width = 0;
                ht->bits.strong_values = (flags & StrongValues) ? 1 : 0;
                ht->bits.strong_keys = (flags & StrongKeys) ? 1 : 0;
                ht->bits.weak_values = (flags & WeakValues) ? 1 : 0;
                ht->bits.weak_keys = (flags & WeakKeys) ? 1 : 0;
                ht->bits.int_values = (flags & IntegerValues) ? 1 : 0;
                ht->bits.int_keys = (flags & IntegerKeys) ? 1 : 0;
                ht->bits.indirect_keys = (flags & IndirectKeys) ? 1 : 0;
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
                ht->bits.keys_offset = (flags & HasKeys) ? offset++ : 0;
                ht->bits.counts_offset = (flags & HasCounts) ? offset++ : 0;
                ht->bits.hashes_offset = (flags & HasHashCache) ? offset++ : 0;
                
#if DEPLOYMENT_TARGET_EMBEDDED || DEPLOYMENT_TARGET_EMBEDDED_MINI
                ht->bits.hashes_offset = 0;
                ht->bits.strong_values = 0;
                ht->bits.strong_keys = 0;
                ht->bits.weak_values = 0;
                ht->bits.weak_keys = 0;
#endif
                
//                ht->bits.__kret = RSBasicHashGetPtrIndex((void *)cb->retainKey);
//                ht->bits.__vret = RSBasicHashGetPtrIndex((void *)cb->retainValue);
//                ht->bits.__krel = RSBasicHashGetPtrIndex((void *)cb->releaseKey);
//                ht->bits.__vrel = RSBasicHashGetPtrIndex((void *)cb->releaseValue);
//                ht->bits.__kdes = RSBasicHashGetPtrIndex((void *)cb->copyKeyDescription);
//                ht->bits.__vdes = RSBasicHashGetPtrIndex((void *)cb->copyValueDescription);
//                ht->bits.__kequ = RSBasicHashGetPtrIndex((void *)cb->equateKeys);
//                ht->bits.__vequ = RSBasicHashGetPtrIndex((void *)cb->equateValues);
//                ht->bits.__khas = RSBasicHashGetPtrIndex((void *)cb->hashKey);
//                ht->bits.__kget = RSBasicHashGetPtrIndex((void *)cb->getIndirectKey);
                
                for (Index idx = 0; idx < offset; idx++) {
                    ht->pointers[idx] = nullptr;
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
            
            Bucket &&FindBucket(uintptr_t stack_key) const {
                if (SubABZero == stack_key || SubABOne == stack_key) {
                    Bucket result = {NotFound, 0UL, 0UL, 0};
                    return MoveValue(result);
                }
                return _HashFindBucket(stack_key);
            }
            
            void SuppressRC() {
                bits.null_rc = 1;
            }
            
            void UnsuppressRC() {
                bits.null_rc = 0;
            }
            
            OptionFlags GetFlags() const {
                OptionFlags flags = (bits.hash_style << 13);
                if (_HasStrongValues()) flags |= StrongValues;
                if (_HasStrongKeys()) flags |= StrongKeys;
                if (bits.fast_grow) flags |= AggressiveGrowth;
                if (bits.keys_offset) flags |= HasKeys;
                if (bits.counts_offset) flags |= HasCounts;
                if (_HasHashCache()) flags |= HasHashCache;
                return flags;
            }
            
            Index GetCount() const {
                if (bits.counts_offset) {
                    Index total = 0L;
                    Index cnt = (Index)BasicHashTable::TableSizes[bits.num_buckets_idx];
                    for (Index idx = 0; idx < cnt; idx++) {
                        total += _GetSlotCount(idx);
                    }
                    return total;
                }
                return (Index)bits.used_buckets;
            }
            
            Index GetCountOfKey(uintptr_t stack_key) const {
                if (SubABZero == stack_key || SubABOne == stack_key) {
                    return 0L;
                }
                if (0L == bits.used_buckets) {
                    return 0L;
                }
                return FindBucket(stack_key).count;
            }
            
            static bool IsEqualTo(uintptr_t a, uintptr_t b) {
                return a == b;
            }
            
            Index GetCountOfValue(uintptr_t stack_value) const {
                if (SubABZero == stack_value) {
                    return 0UL;
                }
                if (0UL == bits.used_buckets) {
                    return 0UL;
                }
                if (!bits.keys_offset) {
                    return _HashFindBucket(stack_value).count;
                }
                Index total = 0;
                Apply([&total, stack_value](const Bucket& bkt) mutable {
                    if ((stack_value == bkt.weak_value) ||
                        (IsEqualTo(bkt.weak_value, stack_value))) {
                        total += bkt.count;
                    }
                    return true;
                });
                return total;
            }
            
            bool operator==(const BasicHash &ht) const {
                Index cnt = GetCount();
                if (cnt != ht.GetCount()) return false;
                if (0 == cnt) return true;
                bool equal = true;
                Apply([&ht, equal, this](const Bucket &bkt) mutable {
                    const Bucket &bkt2 = ht._HashFindBucket(bkt.weak_value);
                    if (bkt.count != bkt2.count) {
                        equal = false;
                        return false;
                    }
                    if (bits.keys_offset && (bkt.weak_value != bkt2.weak_value) &&
                        !IsEqualTo(bkt.weak_value, bkt2.weak_value)) {
                        equal = false;
                        return false;
                    }
                    return true;
                });
                return equal;
            }
            
            void Apply(std::function<bool (const Bucket &)> func) const {
                Index used = (Index)bits.used_buckets, cnt = (Index)BasicHashTable::TableSizes[bits.num_buckets_idx];
                for (Index idx = 0; 0 < used && idx < cnt; idx++) {
                    const Bucket &bkt = GetBucket(idx);
                    if (0 < bkt.count) {
                        if (!func(bkt)) {
                            return;
                        }
                        used--;
                    }
                }
            }
            
            void Apply(std::function<bool (const Bucket &, bool &)> func) const {
                Index used = (Index)bits.used_buckets, cnt = (Index)BasicHashTable::TableSizes[bits.num_buckets_idx];
                bool stop = false;
                for (Index idx = 0; !stop && 0 < used && idx < cnt; idx++) {
                    Bucket bkt = GetBucket(idx);
                    if (0 < bkt.count) {
                        if (!func(bkt, stop)) {
                            return;
                        }
                        used--;
                    }
                }
            }
            
            void Apply(Range range, std::function<bool (const Bucket &)> func) const {
                if (range.length < 0) HALT;
                if (range.length == 0) return;
                Index cnt = (Index)BasicHashTable::TableSizes[bits.num_buckets_idx];
                if (cnt < range.location + range.length) HALT;
                for (Index idx = 0; idx < range.length; idx++) {
                    Bucket bkt = GetBucket(range.location + idx);
                    if (0 < bkt.count) {
                        if (!func(bkt)) {
                            return;
                        }
                    }
                }
            }
            
            void Apply(bool (^block)(Bucket)) const {
                Index used = (Index)bits.used_buckets, cnt = (Index)BasicHashTable::TableSizes[bits.num_buckets_idx];
                for (Index idx = 0; 0 < used && idx < cnt; idx++) {
                    const Bucket &bkt = GetBucket(idx);
                    if (0 < bkt.count) {
                        if (!block(bkt)) {
                            return;
                        }
                        used--;
                    }
                }
            }
            
            void Apply(bool (^block)(Bucket, bool *)) const {
                Index used = (Index)bits.used_buckets, cnt = (Index)BasicHashTable::TableSizes[bits.num_buckets_idx];
                bool stop = false;
                for (Index idx = 0; !stop && 0 < used && idx < cnt; idx++) {
                    Bucket bkt = GetBucket(idx);
                    if (0 < bkt.count) {
                        if (!block(bkt, &stop)) {
                            return;
                        }
                        used--;
                    }
                }
            }
            
            void Apply(Range range, bool (^block)(Bucket)) const {
                if (range.length < 0) HALT;
                if (range.length == 0) return;
                Index cnt = (Index)BasicHashTable::TableSizes[bits.num_buckets_idx];
                if (cnt < range.location + range.length) HALT;
                for (Index idx = 0; idx < range.length; idx++) {
                    Bucket bkt = GetBucket(range.location + idx);
                    if (0 < bkt.count) {
                        if (!block(bkt)) {
                            return;
                        }
                    }
                }
            }
            
            void GetElements(Index buffersLen, uintptr_t *weak_values, uintptr_t *weak_keys) const {
                Index used = bits.used_buckets, cnt = BasicHashTable::TableSizes[bits.num_buckets_idx];
                Index offset = 0;
                for (Index idx = 0; 0 < used && idx < cnt && offset < buffersLen; idx++) {
                    const Bucket &bkt = GetBucket(idx);
                    if (0 < bkt.count) {
                        used--;
                        for (Index cnt = bkt.count; cnt-- && offset < buffersLen;) {
                            if (weak_values) {
                                weak_values[offset] = bkt.weak_value;
                            }
                            if (weak_keys) {
                                weak_keys[offset] = bkt.weak_key;
                            }
                            offset++;
                        }
                    }
                }
            }
            
            void SetCapacity(Index capacity) {
                if (!IsMutable()) {
                    HALT;
                }
                if (bits.used_buckets < capacity) {
                    bits.mutations++;
                    _Rehash(capacity - bits.used_buckets);
                }
            }
            
            bool AddValue(uintptr_t stack_key, uintptr_t stack_value) {
                if (!IsMutable()) {
                    HALT;
                }
                if (SubABZero == stack_key) HALT;
                if (SubABOne == stack_key) HALT;
                if (SubABZero == stack_value) HALT;
                if (SubABOne == stack_value) HALT;
                Bucket bkt = _HashFindBucket(stack_key);
                if (0 < bkt.count) {
                    bits.mutations++;
                    if (bits.counts_offset && bkt.count < LONG_MAX) { // if not yet as large as a Index can be... otherwise clamp and do nothing
                        _IncSlotCount(bkt.idx);
                        return true;
                    }
                } else {
                    _AddValue(bkt.idx, stack_key, stack_value);
                    return true;
                }
                return false;
            }
            
            void ReplaceValue(uintptr_t stack_key, uintptr_t stack_value) {
                if (!IsMutable()) HALT;
                if (SubABZero == stack_key) HALT;
                if (SubABOne == stack_key) HALT;
                if (SubABZero == stack_value) HALT;
                if (SubABOne == stack_value) HALT;
                Bucket bkt = _HashFindBucket(stack_key);
                if (0 < bkt.count) {
                    _ReplaceValue(bkt.idx, stack_key, stack_value);
                }
            }
            
            void SetValue(uintptr_t stack_key, uintptr_t stack_value) {
                if (!IsMutable()) HALT;
                if (SubABZero == stack_key) HALT;
                if (SubABOne == stack_key) HALT;
                if (SubABZero == stack_value) HALT;
                if (SubABOne == stack_value) HALT;
                Bucket bkt = _HashFindBucket(stack_key);
                if (0 < bkt.count) {
                    _ReplaceValue(bkt.idx, stack_key, stack_value);
                } else {
                    _AddValue(bkt.idx, stack_key, stack_value);
                }
            }
            
            Index RemoveValue(uintptr_t stack_key) {
                if (!IsMutable()) HALT;
                if (SubABZero == stack_key || SubABOne == stack_key) return 0;
                Bucket bkt = _HashFindBucket(stack_key);
                if (1 < bkt.count) {
                    bits.mutations++;
                    if (bits.counts_offset && bkt.count < LONG_MAX) { // if not as large as a Index can be... otherwise clamp and do nothing
                        _DecSlotCount(bkt.idx);
                    }
                } else if (0 < bkt.count) {
                    _RemoveValue(bkt.idx);
                }
                return bkt.count;
            }
            
            Index RemoveValueAtIndex(Index idx) {
                if (!IsMutable()) HALT;
                Bucket bkt = GetBucket(idx);
                if (1 < bkt.count) {
                    bits.mutations++;
                    if (bits.counts_offset && bkt.count < LONG_MAX) { // if not as large as a Index can be... otherwise clamp and do nothing
                        _DecSlotCount(bkt.idx);
                    }
                } else if (0 < bkt.count) {
                    _RemoveValue(bkt.idx);
                }
                return bkt.count;
            }
            
            void RemoveAllValues() {
                if (!IsMutable()) HALT;
                if (0 == bits.num_buckets_idx) return;
                _Drain(false);
            }
            
            bool AddIntValueAndInc(uintptr_t stack_key, uintptr_t int_value) {
                if (!IsMutable()) HALT;
                if (SubABZero == stack_key) HALT;
                if (SubABOne == stack_key) HALT;
                if (SubABZero == int_value) HALT;
                if (SubABOne == int_value) HALT;
                Bucket bkt = _HashFindBucket(stack_key);
                if (0 < bkt.count) {
                    bits.mutations++;
                } else {
                    // must rehash before renumbering
                    if (GetCapacity() < bits.used_buckets + 1) {
                        _Rehash(1);
                        bkt.idx = _FindBucket_NoCollision(stack_key, 0);
                    }
                    Index cnt = (Index)BasicHashTable::TableSizes[bits.num_buckets_idx];
                    for (Index idx = 0; idx < cnt; idx++) {
                        if (!_IsEmptyOrDeleted(idx)) {
                            uintptr_t stack_value = _GetValue(idx);
                            if (int_value <= stack_value) {
                                stack_value++;
                                _SetValue(idx, stack_value, true, false);
                                bits.mutations++;
                            }
                        }
                    }
                    _AddValue(bkt.idx, stack_key, int_value);
                    return true;
                }
                return false;
            }
            
            void RemoveIntValueAndDec(uintptr_t int_value) {
                if (!IsMutable()) HALT;
                if (SubABZero == int_value) HALT;
                if (SubABOne == int_value) HALT;
                uintptr_t bkt_idx = ~0UL;
                Index cnt = (Index)BasicHashTable::TableSizes[bits.num_buckets_idx];
                for (Index idx = 0; idx < cnt; idx++) {
                    if (!_IsEmptyOrDeleted(idx)) {
                        uintptr_t stack_value = _GetValue(idx);
                        if (int_value == stack_value) {
                            bkt_idx = idx;
                        }
                        if (int_value < stack_value) {
                            stack_value--;
                            _SetValue(idx, stack_value, true, false);
                            bits.mutations++;
                        }
                    }
                }
                _RemoveValue(bkt_idx);
            }
            
            size_t GetSize(bool total = true) {
                size_t size = sizeof(class BasicHash);
                if (bits.keys_offset) size += sizeof(Value *);
                if (bits.counts_offset) size += sizeof(void *);
                if (_HasHashCache()) size += sizeof(uintptr_t *);
                if (total) {
                    Index num_buckets = BasicHashTable::TableSizes[bits.num_buckets_idx];
                    if (0 < num_buckets) {
                        auto allocator = &Allocator<Value *>::SystemDefault;
                        
                        size += allocator->GetSize(_GetValues());
                        if (bits.keys_offset) size += allocator->GetSize(_GetKeys());
                        if (bits.counts_offset) size += allocator->GetSize(_GetCounts());
                        if (_HasHashCache()) size += allocator->GetSize(_GetHashes());
                    }
                }
                return size;
            }
            
            ~BasicHash() {
                if (bits.finalized) HALT;
                bits.finalized = 1;
                _Drain(true);
            }
        private:
            static void* __AllocateMemory(Index count, Index elem_size, bool strong = false, bool compactable = false) {
                void *new_mem = nullptr;
                new_mem = Allocator<char*>::SystemDefault.Allocate<char>(count * elem_size);
                return new_mem;
            }
            
            static void *__AllocateMemory2(Index count, Index elem_size, bool strong = false, bool compactable = false) {
                return __AllocateMemory(count, elem_size, strong, compactable);
            }
            
            constexpr static const Index SubABZero = 0xa7baadb1;
            constexpr static const Index SubABOne = 0xa5baadb9;
        private:
            Index info;
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
    }
}

#endif /* BasicHash_cpp */
