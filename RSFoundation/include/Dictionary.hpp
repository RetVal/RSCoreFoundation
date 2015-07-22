//
//  Dictionary.h
//  RSFoundation
//
//  Created by closure on 7/21/15.
//  Copyright (c) 2015 RetVal. All rights reserved.
//

#ifndef __RSCoreFoundation__Dictionary__
#define __RSCoreFoundation__Dictionary__

#include <RSFoundation/Object.hpp>
#include <RSFoundation/type_traits.hpp>
#include <RSFoundation/AlignOf.hpp>

#include <algorithm>
#include <iterator>
#include <new>
#include <utility>
#include <cassert>
#include <cstddef>
#include <cstring>
#include <RSFoundation/MathExtras.hpp>
#include <RSFoundation/DictionaryInfo.hpp>

namespace RSCF {
    // Provide DictionaryInfo for all pointers.
    template<typename T>
    struct DictionaryInfo<T*> {
        static inline T* getEmptyKey() {
            intptr_t Val = -1;
            return reinterpret_cast<T*>(Val);
        }
        static inline T* getTombstoneKey() {
            intptr_t Val = -2;
            return reinterpret_cast<T*>(Val);
        }
        static RSHashCode getHashValue(const T *PtrVal) {
            return (RSHashCode((uintptr_t)PtrVal) >> 4) ^
            (RSHashCode((uintptr_t)PtrVal) >> 9);
        }
        static bool isEqual(const T *LHS, const T *RHS) { return LHS == RHS; }
    };
    
    // Provide DictionaryInfo for chars.
    template<> struct DictionaryInfo<char> {
        static inline char getEmptyKey() { return ~0; }
        static inline char getTombstoneKey() { return ~0 - 1; }
        static RSHashCode getHashValue(const char& Val) { return Val * 37; }
        static bool isEqual(const char &LHS, const char &RHS) {
            return LHS == RHS;
        }
    };
    
    // Provide DictionaryInfo for size_t ints.
    template<> struct DictionaryInfo<unsigned> {
        static inline unsigned getEmptyKey() { return ~0; }
        static inline unsigned getTombstoneKey() { return ~0U - 1; }
        static RSHashCode getHashValue(const unsigned& Val) { return Val * 37; }
        static bool isEqual(const unsigned& LHS, const unsigned& RHS) {
            return LHS == RHS;
        }
    };
    
    // Provide DictionaryInfo for unsigned longs.
    template<> struct DictionaryInfo<unsigned long> {
        static inline unsigned long getEmptyKey() { return ~0UL; }
        static inline unsigned long getTombstoneKey() { return ~0UL - 1L; }
        static RSHashCode getHashValue(const unsigned long& Val) {
            return (RSHashCode)(Val * 37UL);
        }
        static bool isEqual(const unsigned long& LHS, const unsigned long& RHS) {
            return LHS == RHS;
        }
    };
    
    // Provide DictionaryInfo for unsigned long longs.
    template<> struct DictionaryInfo<unsigned long long> {
        static inline unsigned long long getEmptyKey() { return ~0ULL; }
        static inline unsigned long long getTombstoneKey() { return ~0ULL - 1ULL; }
        static RSHashCode getHashValue(const unsigned long long& Val) {
            return (RSHashCode)(Val * 37ULL);
        }
        static bool isEqual(const unsigned long long& LHS,
                            const unsigned long long& RHS) {
            return LHS == RHS;
        }
    };
    
    // Provide DictionaryInfo for ints.
    template<> struct DictionaryInfo<int> {
        static inline int getEmptyKey() { return 0x7fffffff; }
        static inline int getTombstoneKey() { return -0x7fffffff - 1; }
        static RSHashCode getHashValue(const int& Val) { return (RSHashCode)(Val * 37); }
        static bool isEqual(const int& LHS, const int& RHS) {
            return LHS == RHS;
        }
    };
    
    // Provide DictionaryInfo for longs.
    template<> struct DictionaryInfo<long> {
        static inline long getEmptyKey() {
            return (1UL << (sizeof(long) * 8 - 1)) - 1L;
        }
        static inline long getTombstoneKey() { return getEmptyKey() - 1L; }
        static RSHashCode getHashValue(const long& Val) {
            return (RSHashCode)(Val * 37L);
        }
        static bool isEqual(const long& LHS, const long& RHS) {
            return LHS == RHS;
        }
    };
    
    // Provide DictionaryInfo for long longs.
    template<> struct DictionaryInfo<long long> {
        static inline long long getEmptyKey() { return 0x7fffffffffffffffLL; }
        static inline long long getTombstoneKey() { return -0x7fffffffffffffffLL-1; }
        static RSHashCode getHashValue(const long long& Val) {
            return (RSHashCode)(Val * 37LL);
        }
        static bool isEqual(const long long& LHS,
                            const long long& RHS) {
            return LHS == RHS;
        }
    };
    
    // Provide DictionaryInfo for all pairs whose members have info.
    template<typename T, typename U>
    struct DictionaryInfo<std::pair<T, U> > {
        typedef std::pair<T, U> Pair;
        typedef DictionaryInfo<T> FirstInfo;
        typedef DictionaryInfo<U> SecondInfo;
        
        static inline Pair getEmptyKey() {
            return std::make_pair(FirstInfo::getEmptyKey(),
                                  SecondInfo::getEmptyKey());
        }
        static inline Pair getTombstoneKey() {
            return std::make_pair(FirstInfo::getTombstoneKey(),
                                  SecondInfo::getEmptyKey());
        }
        static RSHashCode getHashValue(const Pair& PairVal) {
            uint64_t key = (uint64_t)FirstInfo::getHashValue(PairVal.first) << 32
            | (uint64_t)SecondInfo::getHashValue(PairVal.second);
            key += ~(key << 32);
            key ^= (key >> 22);
            key += ~(key << 13);
            key ^= (key >> 8);
            key += (key << 3);
            key ^= (key >> 15);
            key += ~(key << 27);
            key ^= (key >> 31);
            return (RSHashCode)key;
        }
        static bool isEqual(const Pair& LHS, const Pair& RHS) { return LHS == RHS; }
    };
    
} // end namespace RSCF



namespace RSCF {
    
    template<typename KeyT, typename ValueT,
    typename KeyInfoT = DictionaryInfo<KeyT>,
    bool IsConst = false>
    class DictionaryIterator;
    
    template<typename DerivedT,
    typename KeyT, typename ValueT, typename KeyInfoT>
    class DictionaryBase {
    protected:
        typedef std::pair<KeyT, ValueT> BucketT;
        
    public:
        typedef KeyT key_type;
        typedef ValueT mapped_type;
        typedef BucketT value_type;
        
        typedef DictionaryIterator<KeyT, ValueT, KeyInfoT> iterator;
        typedef DictionaryIterator<KeyT, ValueT,
        KeyInfoT, true> const_iterator;
        inline iterator begin() {
            // When the map is empty, avoid the overhead of AdvancePastEmptyBuckets().
            return empty() ? end() : iterator(getBuckets(), getBucketsEnd());
        }
        inline iterator end() {
            return iterator(getBucketsEnd(), getBucketsEnd(), true);
        }
        inline const_iterator begin() const {
            return empty() ? end() : const_iterator(getBuckets(), getBucketsEnd());
        }
        inline const_iterator end() const {
            return const_iterator(getBucketsEnd(), getBucketsEnd(), true);
        }
        
        bool empty() const {
            return getNumEntries() == 0;
        }
        size_t size() const { return getNumEntries(); }
        
        /// Grow the Dictionary so that it has at least Size buckets. Does not shrink
        void resize(size_t Size) {
            if (Size > getNumBuckets())
                grow(Size);
        }
        
        void clear() {
            if (getNumEntries() == 0 && getNumTombstones() == 0) return;
            
            // If the capacity of the array is huge, and the # elements used is small,
            // shrink the array.
            if (getNumEntries() * 4 < getNumBuckets() && getNumBuckets() > 64) {
                shrink_and_clear();
                return;
            }
            
            const KeyT EmptyKey = getEmptyKey(), TombstoneKey = getTombstoneKey();
            for (BucketT *P = getBuckets(), *E = getBucketsEnd(); P != E; ++P) {
                if (!KeyInfoT::isEqual(P->first, EmptyKey)) {
                    if (!KeyInfoT::isEqual(P->first, TombstoneKey)) {
                        P->second.~ValueT();
                        decrementNumEntries();
                    }
                    P->first = EmptyKey;
                }
            }
            assert(getNumEntries() == 0 && "Node count imbalance!");
            setNumTombstones(0);
        }
        
        /// count - Return true if the specified key is in the map.
        bool count(const KeyT &Val) const {
            const BucketT *TheBucket;
            return LookupBucketFor(Val, TheBucket);
        }
        
        iterator find(const KeyT &Val) {
            BucketT *TheBucket;
            if (LookupBucketFor(Val, TheBucket))
                return iterator(TheBucket, getBucketsEnd(), true);
            return end();
        }
        const_iterator find(const KeyT &Val) const {
            const BucketT *TheBucket;
            if (LookupBucketFor(Val, TheBucket))
                return const_iterator(TheBucket, getBucketsEnd(), true);
            return end();
        }
        
        /// Alternate version of find() which allows a different, and possibly
        /// less expensive, key type.
        /// The DictionaryInfo is responsible for supplying methods
        /// getHashValue(LookupKeyT) and isEqual(LookupKeyT, KeyT) for each key
        /// type used.
        template<class LookupKeyT>
        iterator find_as(const LookupKeyT &Val) {
            BucketT *TheBucket;
            if (LookupBucketFor(Val, TheBucket))
                return iterator(TheBucket, getBucketsEnd(), true);
            return end();
        }
        template<class LookupKeyT>
        const_iterator find_as(const LookupKeyT &Val) const {
            const BucketT *TheBucket;
            if (LookupBucketFor(Val, TheBucket))
                return const_iterator(TheBucket, getBucketsEnd(), true);
            return end();
        }
        
        /// lookup - Return the entry for the specified key, or a default
        /// constructed value if no such entry exists.
        ValueT lookup(const KeyT &Val) const {
            const BucketT *TheBucket;
            if (LookupBucketFor(Val, TheBucket))
                return TheBucket->second;
            return ValueT();
        }
        
        // Inserts key,value pair into the map if the key isn't already in the map.
        // If the key is already in the map, it returns false and doesn't update the
        // value.
        std::pair<iterator, bool> insert(const std::pair<KeyT, ValueT> &KV) {
            BucketT *TheBucket;
            if (LookupBucketFor(KV.first, TheBucket))
                return std::make_pair(iterator(TheBucket, getBucketsEnd(), true),
                                      false); // Already in map.
            
            // Otherwise, insert the new element.
            TheBucket = InsertIntoBucket(KV.first, KV.second, TheBucket);
            return std::make_pair(iterator(TheBucket, getBucketsEnd(), true), true);
        }
        
        // Inserts key,value pair into the map if the key isn't already in the map.
        // If the key is already in the map, it returns false and doesn't update the
        // value.
        std::pair<iterator, bool> insert(std::pair<KeyT, ValueT> &&KV) {
            BucketT *TheBucket;
            if (LookupBucketFor(KV.first, TheBucket))
                return std::make_pair(iterator(TheBucket, getBucketsEnd(), true),
                                      false); // Already in map.
            
            // Otherwise, insert the new element.
            TheBucket = InsertIntoBucket(std::move(KV.first),
                                         std::move(KV.second),
                                         TheBucket);
            return std::make_pair(iterator(TheBucket, getBucketsEnd(), true), true);
        }
        
        /// insert - Range insertion of pairs.
        template<typename InputIt>
        void insert(InputIt I, InputIt E) {
            for (; I != E; ++I)
                insert(*I);
        }
        
        
        bool erase(const KeyT &Val) {
            BucketT *TheBucket;
            if (!LookupBucketFor(Val, TheBucket))
                return false; // not in map.
            
            TheBucket->second.~ValueT();
            TheBucket->first = getTombstoneKey();
            decrementNumEntries();
            incrementNumTombstones();
            return true;
        }
        void erase(iterator I) {
            BucketT *TheBucket = &*I;
            TheBucket->second.~ValueT();
            TheBucket->first = getTombstoneKey();
            decrementNumEntries();
            incrementNumTombstones();
        }
        
        value_type& FindAndConstruct(const KeyT &Key) {
            BucketT *TheBucket;
            if (LookupBucketFor(Key, TheBucket))
                return *TheBucket;
            
            return *InsertIntoBucket(Key, ValueT(), TheBucket);
        }
        
        ValueT &operator[](const KeyT &Key) {
            return FindAndConstruct(Key).second;
        }
        
        value_type& FindAndConstruct(KeyT &&Key) {
            BucketT *TheBucket;
            if (LookupBucketFor(Key, TheBucket))
                return *TheBucket;
            
            return *InsertIntoBucket(std::move(Key), ValueT(), TheBucket);
        }
        
        ValueT &operator[](KeyT &&Key) {
            return FindAndConstruct(std::move(Key)).second;
        }
        
        /// isPointerIntoBucketsArray - Return true if the specified pointer points
        /// somewhere into the Dictionary's array of buckets (i.e. either to a key or
        /// value in the Dictionary).
        bool isPointerIntoBucketsArray(const void *Ptr) const {
            return Ptr >= getBuckets() && Ptr < getBucketsEnd();
        }
        
        /// getPointerIntoBucketsArray() - Return an opaque pointer into the buckets
        /// array.  In conjunction with the previous method, this can be used to
        /// determine whether an insertion caused the Dictionary to reallocate.
        const void *getPointerIntoBucketsArray() const { return getBuckets(); }
        
    protected:
        DictionaryBase() {}
        
        void destroyAll() {
            if (getNumBuckets() == 0) // Nothing to do.
                return;
            
            const KeyT EmptyKey = getEmptyKey(), TombstoneKey = getTombstoneKey();
            for (BucketT *P = getBuckets(), *E = getBucketsEnd(); P != E; ++P) {
                if (!KeyInfoT::isEqual(P->first, EmptyKey) &&
                    !KeyInfoT::isEqual(P->first, TombstoneKey))
                    P->second.~ValueT();
                P->first.~KeyT();
            }
            
#ifndef NDEBUG
            memset((void*)getBuckets(), 0x5a, sizeof(BucketT)*getNumBuckets());
#endif
        }
        
        void initEmpty() {
            setNumEntries(0);
            setNumTombstones(0);
            
            assert((getNumBuckets() & (getNumBuckets()-1)) == 0 &&
                   "# initial buckets must be a power of two!");
            const KeyT EmptyKey = getEmptyKey();
            for (BucketT *B = getBuckets(), *E = getBucketsEnd(); B != E; ++B)
                new (&B->first) KeyT(EmptyKey);
        }
        
        void moveFromOldBuckets(BucketT *OldBucketsBegin, BucketT *OldBucketsEnd) {
            initEmpty();
            
            // Insert all the old elements.
            const KeyT EmptyKey = getEmptyKey();
            const KeyT TombstoneKey = getTombstoneKey();
            for (BucketT *B = OldBucketsBegin, *E = OldBucketsEnd; B != E; ++B) {
                if (!KeyInfoT::isEqual(B->first, EmptyKey) &&
                    !KeyInfoT::isEqual(B->first, TombstoneKey)) {
                    // Insert the key/value into the new table.
                    BucketT *DestBucket;
                    bool FoundVal = LookupBucketFor(B->first, DestBucket);
                    (void)FoundVal; // silence warning.
                    assert(!FoundVal && "Key already in new map?");
                    DestBucket->first = std::move(B->first);
                    new (&DestBucket->second) ValueT(std::move(B->second));
                    incrementNumEntries();
                    
                    // Free the value.
                    B->second.~ValueT();
                }
                B->first.~KeyT();
            }
            
#ifndef NDEBUG
            if (OldBucketsBegin != OldBucketsEnd)
                memset((void*)OldBucketsBegin, 0x5a,
                       sizeof(BucketT) * (OldBucketsEnd - OldBucketsBegin));
#endif
        }
        
        template <typename OtherBaseT>
        void copyFrom(const DictionaryBase<OtherBaseT, KeyT, ValueT, KeyInfoT>& other) {
            assert(getNumBuckets() == other.getNumBuckets());
            
            setNumEntries(other.getNumEntries());
            setNumTombstones(other.getNumTombstones());
            
            if (isPodLike<KeyT>::value && isPodLike<ValueT>::value)
                memcpy(getBuckets(), other.getBuckets(),
                       getNumBuckets() * sizeof(BucketT));
            else
                for (size_t i = 0; i < getNumBuckets(); ++i) {
                    new (&getBuckets()[i].first) KeyT(other.getBuckets()[i].first);
                    if (!KeyInfoT::isEqual(getBuckets()[i].first, getEmptyKey()) &&
                        !KeyInfoT::isEqual(getBuckets()[i].first, getTombstoneKey()))
                        new (&getBuckets()[i].second) ValueT(other.getBuckets()[i].second);
                }
        }
        
        void swap(DictionaryBase& RHS) {
            std::swap(getNumEntries(), RHS.getNumEntries());
            std::swap(getNumTombstones(), RHS.getNumTombstones());
        }
        
        static size_t getHashValue(const KeyT &Val) {
            return KeyInfoT::getHashValue(Val);
        }
        template<typename LookupKeyT>
        static size_t getHashValue(const LookupKeyT &Val) {
            return KeyInfoT::getHashValue(Val);
        }
        static const KeyT getEmptyKey() {
            return KeyInfoT::getEmptyKey();
        }
        static const KeyT getTombstoneKey() {
            return KeyInfoT::getTombstoneKey();
        }
        
    private:
        size_t getNumEntries() const {
            return static_cast<const DerivedT *>(this)->getNumEntries();
        }
        void setNumEntries(size_t Num) {
            static_cast<DerivedT *>(this)->setNumEntries(Num);
        }
        void incrementNumEntries() {
            setNumEntries(getNumEntries() + 1);
        }
        void decrementNumEntries() {
            setNumEntries(getNumEntries() - 1);
        }
        size_t getNumTombstones() const {
            return static_cast<const DerivedT *>(this)->getNumTombstones();
        }
        void setNumTombstones(size_t Num) {
            static_cast<DerivedT *>(this)->setNumTombstones(Num);
        }
        void incrementNumTombstones() {
            setNumTombstones(getNumTombstones() + 1);
        }
        void decrementNumTombstones() {
            setNumTombstones(getNumTombstones() - 1);
        }
        const BucketT *getBuckets() const {
            return static_cast<const DerivedT *>(this)->getBuckets();
        }
        BucketT *getBuckets() {
            return static_cast<DerivedT *>(this)->getBuckets();
        }
        size_t getNumBuckets() const {
            return static_cast<const DerivedT *>(this)->getNumBuckets();
        }
        BucketT *getBucketsEnd() {
            return getBuckets() + getNumBuckets();
        }
        const BucketT *getBucketsEnd() const {
            return getBuckets() + getNumBuckets();
        }
        
        void grow(size_t AtLeast) {
            static_cast<DerivedT *>(this)->grow(AtLeast);
        }
        
        void shrink_and_clear() {
            static_cast<DerivedT *>(this)->shrink_and_clear();
        }
        
        
        BucketT *InsertIntoBucket(const KeyT &Key, const ValueT &Value,
                                  BucketT *TheBucket) {
            TheBucket = InsertIntoBucketImpl(Key, TheBucket);
            
            TheBucket->first = Key;
            new (&TheBucket->second) ValueT(Value);
            return TheBucket;
        }
        
        BucketT *InsertIntoBucket(const KeyT &Key, ValueT &&Value,
                                  BucketT *TheBucket) {
            TheBucket = InsertIntoBucketImpl(Key, TheBucket);
            
            TheBucket->first = Key;
            new (&TheBucket->second) ValueT(std::move(Value));
            return TheBucket;
        }
        
        BucketT *InsertIntoBucket(KeyT &&Key, ValueT &&Value, BucketT *TheBucket) {
            TheBucket = InsertIntoBucketImpl(Key, TheBucket);
            
            TheBucket->first = std::move(Key);
            new (&TheBucket->second) ValueT(std::move(Value));
            return TheBucket;
        }
        
        BucketT *InsertIntoBucketImpl(const KeyT &Key, BucketT *TheBucket) {
            // If the load of the hash table is more than 3/4, or if fewer than 1/8 of
            // the buckets are empty (meaning that many are filled with tombstones),
            // grow the table.
            //
            // The later case is tricky.  For example, if we had one empty bucket with
            // tons of tombstones, failing lookups (e.g. for insertion) would have to
            // probe almost the entire table until it found the empty bucket.  If the
            // table completely filled with tombstones, no lookup would ever succeed,
            // causing infinite loops in lookup.
            size_t NewNumEntries = getNumEntries() + 1;
            size_t NumBuckets = getNumBuckets();
            if (NewNumEntries*4 >= NumBuckets*3) {
                this->grow(NumBuckets * 2);
                LookupBucketFor(Key, TheBucket);
                NumBuckets = getNumBuckets();
            } else if (NumBuckets-(NewNumEntries+getNumTombstones()) <= NumBuckets/8) {
                this->grow(NumBuckets);
                LookupBucketFor(Key, TheBucket);
            }
            assert(TheBucket);
            
            // Only update the state after we've grown our bucket space appropriately
            // so that when growing buckets we have self-consistent entry count.
            incrementNumEntries();
            
            // If we are writing over a tombstone, remember this.
            const KeyT EmptyKey = getEmptyKey();
            if (!KeyInfoT::isEqual(TheBucket->first, EmptyKey))
                decrementNumTombstones();
            
            return TheBucket;
        }
        
        /// LookupBucketFor - Lookup the appropriate bucket for Val, returning it in
        /// FoundBucket.  If the bucket contains the key and a value, this returns
        /// true, otherwise it returns a bucket with an empty marker or tombstone and
        /// returns false.
        template<typename LookupKeyT>
        bool LookupBucketFor(const LookupKeyT &Val,
                             const BucketT *&FoundBucket) const {
            const BucketT *BucketsPtr = getBuckets();
            const size_t NumBuckets = getNumBuckets();
            
            if (NumBuckets == 0) {
                FoundBucket = nullptr;
                return false;
            }
            
            // FoundTombstone - Keep track of whether we find a tombstone while probing.
            const BucketT *FoundTombstone = nullptr;
            const KeyT EmptyKey = getEmptyKey();
            const KeyT TombstoneKey = getTombstoneKey();
            assert(!KeyInfoT::isEqual(Val, EmptyKey) &&
                   !KeyInfoT::isEqual(Val, TombstoneKey) &&
                   "Empty/Tombstone value shouldn't be inserted into map!");
            
            size_t BucketNo = getHashValue(Val) & (NumBuckets-1);
            size_t ProbeAmt = 1;
            while (1) {
                const BucketT *ThisBucket = BucketsPtr + BucketNo;
                // Found Val's bucket?  If so, return it.
                if (KeyInfoT::isEqual(Val, ThisBucket->first)) {
                    FoundBucket = ThisBucket;
                    return true;
                }
                
                // If we found an empty bucket, the key doesn't exist in the set.
                // Insert it and return the default value.
                if (KeyInfoT::isEqual(ThisBucket->first, EmptyKey)) {
                    // If we've already seen a tombstone while probing, fill it in instead
                    // of the empty bucket we eventually probed to.
                    FoundBucket = FoundTombstone ? FoundTombstone : ThisBucket;
                    return false;
                }
                
                // If this is a tombstone, remember it.  If Val ends up not in the map, we
                // prefer to return it than something that would require more probing.
                if (KeyInfoT::isEqual(ThisBucket->first, TombstoneKey) && !FoundTombstone)
                    FoundTombstone = ThisBucket;  // Remember the first tombstone found.
                
                // Otherwise, it's a hash collision or a tombstone, continue quadratic
                // probing.
                BucketNo += ProbeAmt++;
                BucketNo &= (NumBuckets-1);
            }
        }
        
        template <typename LookupKeyT>
        bool LookupBucketFor(const LookupKeyT &Val, BucketT *&FoundBucket) {
            const BucketT *ConstFoundBucket;
            bool Result = const_cast<const DictionaryBase *>(this)
            ->LookupBucketFor(Val, ConstFoundBucket);
            FoundBucket = const_cast<BucketT *>(ConstFoundBucket);
            return Result;
        }
        
    public:
        /// Return the approximate size (in bytes) of the actual map.
        /// This is just the raw memory used by Dictionary.
        /// If entries are pointers to objects, the size of the referenced objects
        /// are not included.
        size_t getMemorySize() const {
            return getNumBuckets() * sizeof(BucketT);
        }
    };
    
    template<typename KeyT, typename ValueT,
    typename KeyInfoT = DictionaryInfo<KeyT> >
    class Dictionary
    : public DictionaryBase<Dictionary<KeyT, ValueT, KeyInfoT>,
    KeyT, ValueT, KeyInfoT> {
        // Lift some types from the dependent base class into this class for
        // simplicity of referring to them.
        typedef DictionaryBase<Dictionary, KeyT, ValueT, KeyInfoT> BaseT;
        typedef typename BaseT::BucketT BucketT;
        friend class DictionaryBase<Dictionary, KeyT, ValueT, KeyInfoT>;
        
        BucketT *Buckets;
        size_t NumEntries;
        size_t NumTombstones;
        size_t NumBuckets;
        
    public:
        explicit Dictionary(size_t NumInitBuckets = 0) {
            init(NumInitBuckets);
        }
        
        Dictionary(const Dictionary &other) : BaseT() {
            init(0);
            copyFrom(other);
        }
        
        Dictionary(Dictionary &&other) : BaseT() {
            init(0);
            swap(other);
        }
        
        template<typename InputIt>
        Dictionary(const InputIt &I, const InputIt &E) {
            init(NextPowerOf2(std::distance(I, E)));
            this->insert(I, E);
        }
        
        ~Dictionary() {
            this->destroyAll();
            operator delete(Buckets);
        }
        
        void swap(Dictionary& RHS) {
            std::swap(Buckets, RHS.Buckets);
            std::swap(NumEntries, RHS.NumEntries);
            std::swap(NumTombstones, RHS.NumTombstones);
            std::swap(NumBuckets, RHS.NumBuckets);
        }
        
        Dictionary& operator=(const Dictionary& other) {
            copyFrom(other);
            return *this;
        }
        
        Dictionary& operator=(Dictionary &&other) {
            this->destroyAll();
            operator delete(Buckets);
            init(0);
            swap(other);
            return *this;
        }
        
        void copyFrom(const Dictionary& other) {
            this->destroyAll();
            operator delete(Buckets);
            if (allocateBuckets(other.NumBuckets)) {
                this->BaseT::copyFrom(other);
            } else {
                NumEntries = 0;
                NumTombstones = 0;
            }
        }
        
        void init(size_t InitBuckets) {
            if (allocateBuckets(InitBuckets)) {
                this->BaseT::initEmpty();
            } else {
                NumEntries = 0;
                NumTombstones = 0;
            }
        }
        
        void grow(size_t AtLeast) {
            size_t OldNumBuckets = NumBuckets;
            BucketT *OldBuckets = Buckets;
            
            allocateBuckets(std::max<size_t>(64, static_cast<size_t>(NextPowerOf2(AtLeast-1))));
            assert(Buckets);
            if (!OldBuckets) {
                this->BaseT::initEmpty();
                return;
            }
            
            this->moveFromOldBuckets(OldBuckets, OldBuckets+OldNumBuckets);
            
            // Free the old table.
            operator delete(OldBuckets);
        }
        
        void shrink_and_clear() {
            size_t OldNumEntries = NumEntries;
            this->destroyAll();
            
            // Reduce the number of buckets.
            size_t NewNumBuckets = 0;
            if (OldNumEntries)
                NewNumBuckets = std::max(64, 1 << (Log2_32_Ceil(OldNumEntries) + 1));
            if (NewNumBuckets == NumBuckets) {
                this->BaseT::initEmpty();
                return;
            }
            
            operator delete(Buckets);
            init(NewNumBuckets);
        }
        
    private:
        size_t getNumEntries() const {
            return NumEntries;
        }
        void setNumEntries(size_t Num) {
            NumEntries = Num;
        }
        
        size_t getNumTombstones() const {
            return NumTombstones;
        }
        void setNumTombstones(size_t Num) {
            NumTombstones = Num;
        }
        
        BucketT *getBuckets() const {
            return Buckets;
        }
        
        size_t getNumBuckets() const {
            return NumBuckets;
        }
        
        bool allocateBuckets(size_t Num) {
            NumBuckets = Num;
            if (NumBuckets == 0) {
                Buckets = nullptr;
                return false;
            }
            
            Buckets = static_cast<BucketT*>(operator new(sizeof(BucketT) * NumBuckets));
            return true;
        }
    };
    
    template<typename KeyT, typename ValueT,
    size_t InlineBuckets = 4,
    typename KeyInfoT = DictionaryInfo<KeyT> >
    class SmallDictionary
    : public DictionaryBase<SmallDictionary<KeyT, ValueT, InlineBuckets, KeyInfoT>,
    KeyT, ValueT, KeyInfoT> {
        // Lift some types from the dependent base class into this class for
        // simplicity of referring to them.
        typedef DictionaryBase<SmallDictionary, KeyT, ValueT, KeyInfoT> BaseT;
        typedef typename BaseT::BucketT BucketT;
        friend class DictionaryBase<SmallDictionary, KeyT, ValueT, KeyInfoT>;
        
        size_t Small : 1;
        size_t NumEntries : 31;
        size_t NumTombstones;
        
        struct LargeRep {
            BucketT *Buckets;
            size_t NumBuckets;
        };
        
        /// A "union" of an inline bucket array and the struct representing
        /// a large bucket. This union will be discriminated by the 'Small' bit.
        AlignedCharArrayUnion<BucketT[InlineBuckets], LargeRep> storage;
        
    public:
        explicit SmallDictionary(size_t NumInitBuckets = 0) {
            init(NumInitBuckets);
        }
        
        SmallDictionary(const SmallDictionary &other) : BaseT() {
            init(0);
            copyFrom(other);
        }
        
        SmallDictionary(SmallDictionary &&other) : BaseT() {
            init(0);
            swap(other);
        }
        
        template<typename InputIt>
        SmallDictionary(const InputIt &I, const InputIt &E) {
            init(NextPowerOf2(std::distance(I, E)));
            this->insert(I, E);
        }
        
        ~SmallDictionary() {
            this->destroyAll();
            deallocateBuckets();
        }
        
        void swap(SmallDictionary& RHS) {
            size_t TmpNumEntries = RHS.NumEntries;
            RHS.NumEntries = NumEntries;
            NumEntries = TmpNumEntries;
            std::swap(NumTombstones, RHS.NumTombstones);
            
            const KeyT EmptyKey = this->getEmptyKey();
            const KeyT TombstoneKey = this->getTombstoneKey();
            if (Small && RHS.Small) {
                // If we're swapping inline bucket arrays, we have to cope with some of
                // the tricky bits of Dictionary's storage system: the buckets are not
                // fully initialized. Thus we swap every key, but we may have
                // a one-directional move of the value.
                for (size_t i = 0, e = InlineBuckets; i != e; ++i) {
                    BucketT *LHSB = &getInlineBuckets()[i],
                    *RHSB = &RHS.getInlineBuckets()[i];
                    bool hasLHSValue = (!KeyInfoT::isEqual(LHSB->first, EmptyKey) &&
                                        !KeyInfoT::isEqual(LHSB->first, TombstoneKey));
                    bool hasRHSValue = (!KeyInfoT::isEqual(RHSB->first, EmptyKey) &&
                                        !KeyInfoT::isEqual(RHSB->first, TombstoneKey));
                    if (hasLHSValue && hasRHSValue) {
                        // Swap together if we can...
                        std::swap(*LHSB, *RHSB);
                        continue;
                    }
                    // Swap separately and handle any assymetry.
                    std::swap(LHSB->first, RHSB->first);
                    if (hasLHSValue) {
                        new (&RHSB->second) ValueT(std::move(LHSB->second));
                        LHSB->second.~ValueT();
                    } else if (hasRHSValue) {
                        new (&LHSB->second) ValueT(std::move(RHSB->second));
                        RHSB->second.~ValueT();
                    }
                }
                return;
            }
            if (!Small && !RHS.Small) {
                std::swap(getLargeRep()->Buckets, RHS.getLargeRep()->Buckets);
                std::swap(getLargeRep()->NumBuckets, RHS.getLargeRep()->NumBuckets);
                return;
            }
            
            SmallDictionary &SmallSide = Small ? *this : RHS;
            SmallDictionary &LargeSide = Small ? RHS : *this;
            
            // First stash the large side's rep and move the small side across.
            LargeRep TmpRep = std::move(*LargeSide.getLargeRep());
            LargeSide.getLargeRep()->~LargeRep();
            LargeSide.Small = true;
            // This is similar to the standard move-from-old-buckets, but the bucket
            // count hasn't actually rotated in this case. So we have to carefully
            // move construct the keys and values into their new locations, but there
            // is no need to re-hash things.
            for (size_t i = 0, e = InlineBuckets; i != e; ++i) {
                BucketT *NewB = &LargeSide.getInlineBuckets()[i],
                *OldB = &SmallSide.getInlineBuckets()[i];
                new (&NewB->first) KeyT(std::move(OldB->first));
                OldB->first.~KeyT();
                if (!KeyInfoT::isEqual(NewB->first, EmptyKey) &&
                    !KeyInfoT::isEqual(NewB->first, TombstoneKey)) {
                    new (&NewB->second) ValueT(std::move(OldB->second));
                    OldB->second.~ValueT();
                }
            }
            
            // The hard part of moving the small buckets across is done, just move
            // the TmpRep into its new home.
            SmallSide.Small = false;
            new (SmallSide.getLargeRep()) LargeRep(std::move(TmpRep));
        }
        
        SmallDictionary& operator=(const SmallDictionary& other) {
            copyFrom(other);
            return *this;
        }
        
        SmallDictionary& operator=(SmallDictionary &&other) {
            this->destroyAll();
            deallocateBuckets();
            init(0);
            swap(other);
            return *this;
        }
        
        void copyFrom(const SmallDictionary& other) {
            this->destroyAll();
            deallocateBuckets();
            Small = true;
            if (other.getNumBuckets() > InlineBuckets) {
                Small = false;
                new (getLargeRep()) LargeRep(allocateBuckets(other.getNumBuckets()));
            }
            this->BaseT::copyFrom(other);
        }
        
        void init(size_t InitBuckets) {
            Small = true;
            if (InitBuckets > InlineBuckets) {
                Small = false;
                new (getLargeRep()) LargeRep(allocateBuckets(InitBuckets));
            }
            this->BaseT::initEmpty();
        }
        
        void grow(size_t AtLeast) {
            if (AtLeast >= InlineBuckets)
                AtLeast = std::max<size_t>(64, NextPowerOf2(AtLeast-1));
            
            if (Small) {
                if (AtLeast < InlineBuckets)
                    return; // Nothing to do.
                
                // First move the inline buckets into a temporary storage.
                AlignedCharArrayUnion<BucketT[InlineBuckets]> TmpStorage;
                BucketT *TmpBegin = reinterpret_cast<BucketT *>(TmpStorage.buffer);
                BucketT *TmpEnd = TmpBegin;
                
                // Loop over the buckets, moving non-empty, non-tombstones into the
                // temporary storage. Have the loop move the TmpEnd forward as it goes.
                const KeyT EmptyKey = this->getEmptyKey();
                const KeyT TombstoneKey = this->getTombstoneKey();
                for (BucketT *P = getBuckets(), *E = P + InlineBuckets; P != E; ++P) {
                    if (!KeyInfoT::isEqual(P->first, EmptyKey) &&
                        !KeyInfoT::isEqual(P->first, TombstoneKey)) {
                        assert(size_t(TmpEnd - TmpBegin) < InlineBuckets &&
                               "Too many inline buckets!");
                        new (&TmpEnd->first) KeyT(std::move(P->first));
                        new (&TmpEnd->second) ValueT(std::move(P->second));
                        ++TmpEnd;
                        P->second.~ValueT();
                    }
                    P->first.~KeyT();
                }
                
                // Now make this map use the large rep, and move all the entries back
                // into it.
                Small = false;
                new (getLargeRep()) LargeRep(allocateBuckets(AtLeast));
                this->moveFromOldBuckets(TmpBegin, TmpEnd);
                return;
            }
            
            LargeRep OldRep = std::move(*getLargeRep());
            getLargeRep()->~LargeRep();
            if (AtLeast <= InlineBuckets) {
                Small = true;
            } else {
                new (getLargeRep()) LargeRep(allocateBuckets(AtLeast));
            }
            
            this->moveFromOldBuckets(OldRep.Buckets, OldRep.Buckets+OldRep.NumBuckets);
            
            // Free the old table.
            operator delete(OldRep.Buckets);
        }
        
        void shrink_and_clear() {
            size_t OldSize = this->size();
            this->destroyAll();
            
            // Reduce the number of buckets.
            size_t NewNumBuckets = 0;
            if (OldSize) {
                NewNumBuckets = 1 << (Log2_32_Ceil(OldSize) + 1);
                if (NewNumBuckets > InlineBuckets && NewNumBuckets < 64u)
                    NewNumBuckets = 64;
            }
            if ((Small && NewNumBuckets <= InlineBuckets) ||
                (!Small && NewNumBuckets == getLargeRep()->NumBuckets)) {
                this->BaseT::initEmpty();
                return;
            }
            
            deallocateBuckets();
            init(NewNumBuckets);
        }
        
    private:
        size_t getNumEntries() const {
            return NumEntries;
        }
        void setNumEntries(size_t Num) {
            assert(Num < INT_MAX && "Cannot support more than INT_MAX entries");
            NumEntries = Num;
        }
        
        size_t getNumTombstones() const {
            return NumTombstones;
        }
        void setNumTombstones(size_t Num) {
            NumTombstones = Num;
        }
        
        const BucketT *getInlineBuckets() const {
            assert(Small);
            // Note that this cast does not violate aliasing rules as we assert that
            // the memory's dynamic type is the small, inline bucket buffer, and the
            // 'storage.buffer' static type is 'char *'.
            return reinterpret_cast<const BucketT *>(storage.buffer);
        }
        BucketT *getInlineBuckets() {
            return const_cast<BucketT *>(
                                         const_cast<const SmallDictionary *>(this)->getInlineBuckets());
        }
        const LargeRep *getLargeRep() const {
            assert(!Small);
            // Note, same rule about aliasing as with getInlineBuckets.
            return reinterpret_cast<const LargeRep *>(storage.buffer);
        }
        LargeRep *getLargeRep() {
            return const_cast<LargeRep *>(
                                          const_cast<const SmallDictionary *>(this)->getLargeRep());
        }
        
        const BucketT *getBuckets() const {
            return Small ? getInlineBuckets() : getLargeRep()->Buckets;
        }
        BucketT *getBuckets() {
            return const_cast<BucketT *>(
                                         const_cast<const SmallDictionary *>(this)->getBuckets());
        }
        size_t getNumBuckets() const {
            return Small ? InlineBuckets : getLargeRep()->NumBuckets;
        }
        
        void deallocateBuckets() {
            if (Small)
                return;
            
            operator delete(getLargeRep()->Buckets);
            getLargeRep()->~LargeRep();
        }
        
        LargeRep allocateBuckets(size_t Num) {
            assert(Num > InlineBuckets && "Must allocate more buckets than are inline");
            LargeRep Rep = {
                static_cast<BucketT*>(operator new(sizeof(BucketT) * Num)), Num
            };
            return Rep;
        }
    };
    
    template<typename KeyT, typename ValueT,
    typename KeyInfoT, bool IsConst>
    class DictionaryIterator {
        typedef std::pair<KeyT, ValueT> Bucket;
        typedef DictionaryIterator<KeyT, ValueT,
        KeyInfoT, true> ConstIterator;
        friend class DictionaryIterator<KeyT, ValueT, KeyInfoT, true>;
    public:
        typedef ptrdiff_t difference_type;
        typedef typename std::conditional<IsConst, const Bucket, Bucket>::type
        value_type;
        typedef value_type *pointer;
        typedef value_type &reference;
        typedef std::forward_iterator_tag iterator_category;
    private:
        pointer Ptr, End;
    public:
        DictionaryIterator() : Ptr(nullptr), End(nullptr) {}
        
        DictionaryIterator(pointer Pos, pointer E, bool NoAdvance = false)
        : Ptr(Pos), End(E) {
            if (!NoAdvance) AdvancePastEmptyBuckets();
        }
        
        // If IsConst is true this is a converting constructor from iterator to
        // const_iterator and the default copy constructor is used.
        // Otherwise this is a copy constructor for iterator.
        DictionaryIterator(const DictionaryIterator<KeyT, ValueT,
                         KeyInfoT, false>& I)
        : Ptr(I.Ptr), End(I.End) {}
        
        reference operator*() const {
            return *Ptr;
        }
        pointer operator->() const {
            return Ptr;
        }
        
        bool operator==(const ConstIterator &RHS) const {
            return Ptr == RHS.operator->();
        }
        bool operator!=(const ConstIterator &RHS) const {
            return Ptr != RHS.operator->();
        }
        
        inline DictionaryIterator& operator++() {  // Preincrement
            ++Ptr;
            AdvancePastEmptyBuckets();
            return *this;
        }
        DictionaryIterator operator++(int) {  // Postincrement
            DictionaryIterator tmp = *this; ++*this; return tmp;
        }
        
    private:
        void AdvancePastEmptyBuckets() {
            const KeyT Empty = KeyInfoT::getEmptyKey();
            const KeyT Tombstone = KeyInfoT::getTombstoneKey();
            
            while (Ptr != End &&
                   (KeyInfoT::isEqual(Ptr->first, Empty) ||
                    KeyInfoT::isEqual(Ptr->first, Tombstone)))
                ++Ptr;
        }
    };
    
    template<typename KeyT, typename ValueT, typename KeyInfoT>
    static inline size_t
    capacity_in_bytes(const Dictionary<KeyT, ValueT, KeyInfoT> &X) {
        return X.getMemorySize();
    }
}

#endif /* defined(__RSCoreFoundation__Dictionary__) */
