//
//  Array.h
//  RSCoreFoundation
//
//  Created by closure on 7/23/15.
//  Copyright (c) 2015 RetVal. All rights reserved.
//

#ifndef __RSCoreFoundation__Array__
#define __RSCoreFoundation__Array__

#include <RSFoundation/Object.hpp>
#include <RSFoundation/type_traits.hpp>
#include <iterator>
#include <vector>

namespace RSCF {
    template <typename T, size_t N>
    inline size_t array_lengthof(T (&)[N]) {
        return N;
    }
    /// Array - Represent a constant reference to an array (0 or more elements
    /// consecutively in memory), i.e. a start pointer and a length.  It allows
    /// various APIs to take consecutive elements easily and conveniently.
    ///
    /// This class does not own the underlying data, it is expected to be used in
    /// situations where the data resides in some other buffer, whose lifetime
    /// extends past that of the Array. For this reason, it is not in general
    /// safe to store an Array.
    ///
    /// This is intended to be trivially copyable, so it should be passed by
    /// value.
    template<typename T>
    class Array {
    public:
        typedef const T *iterator;
        typedef const T *const_iterator;
        typedef size_t size_type;
        
        typedef std::reverse_iterator<iterator> reverse_iterator;
        
    private:
        /// The start of the array, in an external buffer.
        const T *Data;
        
        /// The number of elements.
        size_type Length;
        
        /// \brief A dummy "optional" type that is only created by implicit
        /// conversion from a reference to T.
        ///
        /// This type must *only* be used in a function argument or as a copy of
        /// a function argument, as otherwise it will hold a pointer to a temporary
        /// past that temporaries' lifetime.
        struct TRefOrNothing {
            const T *TPtr;
            
            TRefOrNothing() : TPtr(nullptr) {}
            TRefOrNothing(const T &TRef) : TPtr(&TRef) {}
        };
        
    public:
        /// @name Constructors
        /// @{
        
        /// Construct an empty Array.
        /*implicit*/ Array() : Data(nullptr), Length(0) {}
        
        /// Construct an empty Array from None.
        /*implicit*/ Array(decltype(nullptr)) : Data(nullptr), Length(0) {}
        
        /// Construct an Array from a single element.
        /*implicit*/ Array(const T &OneElt)
        : Data(&OneElt), Length(1) {}
        
        /// Construct an Array from a pointer and length.
        /*implicit*/ Array(const T *data, size_t length)
        : Data(data), Length(length) {}
        
        /// Construct an Array from a range.
        Array(const T *begin, const T *end)
        : Data(begin), Length(end - begin) {}
        
        /// Construct an Array from a std::vector.
        template<typename A>
        /*implicit*/ Array(const std::vector<T, A> &Vec)
        : Data(Vec.data()), Length(Vec.size()) {}
        
        /// Construct an Array from a C array.
        template <size_t N>
        /*implicit*/ constexpr Array(const T (&Arr)[N])
        : Data(Arr), Length(N) {}
        
#if LLVM_HAS_INITIALIZER_LISTS
        /// Construct an Array from a std::initializer_list.
        /*implicit*/ Array(const std::initializer_list<T> &Vec)
        : Data(Vec.begin() == Vec.end() ? (T*)0 : Vec.begin()),
        Length(Vec.size()) {}
#endif
        
        /// Construct an Array<const T*> from Array<T*>. This uses SFINAE to
        /// ensure that only Arrays of pointers can be converted.
        template <typename U>
        Array(const Array<U *> &A,
              typename std::enable_if<
              std::is_convertible<U *const *, T const *>::value>::type* = 0)
        : Data(A.data()), Length(A.size()) {}
        
        /// @}
        /// @name Simple Operations
        /// @{
        
        iterator begin() const { return Data; }
        iterator end() const { return Data + Length; }
        
        reverse_iterator rbegin() const { return reverse_iterator(end()); }
        reverse_iterator rend() const { return reverse_iterator(begin()); }
        
        /// empty - Check if the array is empty.
        bool empty() const { return Length == 0; }
        
        const T *data() const { return Data; }
        
        /// size - Get the array size.
        size_t size() const { return Length; }
        
        /// front - Get the first element.
        const T &front() const {
            assert(!empty());
            return Data[0];
        }
        
        /// back - Get the last element.
        const T &back() const {
            assert(!empty());
            return Data[Length-1];
        }
        
        // copy - Allocate copy in Allocator and return Array<T> to it.
        template <typename Allocator> Array<T> copy(Allocator &A) {
            T *Buff = A.template Allocate<T>(Length);
            std::copy(begin(), end(), Buff);
            return Array<T>(Buff, Length);
        }
        
        /// equals - Check for element-wise equality.
        bool equals(Array RHS) const {
            if (Length != RHS.Length)
                return false;
            // Don't use std::equal(), since it asserts in MSVC on nullptr iterators.
            for (auto L = begin(), LE = end(), R = RHS.begin(); L != LE; ++L, ++R)
                // Match std::equal() in using == (instead of !=) to minimize API
                // requirements of Array'ed types.
                if (!(*L == *R))
                    return false;
            return true;
        }
        
        /// slice(n) - Chop off the first N elements of the array.
        Array<T> slice(unsigned N) const {
            assert(N <= size() && "Invalid specifier");
            return Array<T>(data()+N, size()-N);
        }
        
        /// slice(n, m) - Chop off the first N elements of the array, and keep M
        /// elements in the array.
        Array<T> slice(unsigned N, unsigned M) const {
            assert(N+M <= size() && "Invalid specifier");
            return Array<T>(data()+N, M);
        }
        
        // \brief Drop the last \p N elements of the array.
        Array<T> drop_back(unsigned N = 1) const {
            assert(size() >= N && "Dropping more elements than exist");
            return slice(0, size() - N);
        }
        
        /// @}
        /// @name Operator Overloads
        /// @{
        const T &operator[](size_t Index) const {
            assert(Index < Length && "Invalid index!");
            return Data[Index];
        }
        
        /// @}
        /// @name Expensive Operations
        /// @{
        std::vector<T> vec() const {
            return std::vector<T>(Data, Data+Length);
        }
        
        /// @}
        /// @name Conversion operators
        /// @{
        operator std::vector<T>() const {
            return std::vector<T>(Data, Data+Length);
        }
        
        /// @}
        /// @{
        /// @name Convenience methods
        
        /// @brief Predicate for testing that the array equals the exact sequence of
        /// arguments.
        ///
        /// Will return false if the size is not equal to the exact number of
        /// arguments given or if the array elements don't equal the argument
        /// elements in order. Currently supports up to 16 arguments, but can
        /// easily be extended.
        bool equals(TRefOrNothing Arg0 = TRefOrNothing(),
                    TRefOrNothing Arg1 = TRefOrNothing(),
                    TRefOrNothing Arg2 = TRefOrNothing(),
                    TRefOrNothing Arg3 = TRefOrNothing(),
                    TRefOrNothing Arg4 = TRefOrNothing(),
                    TRefOrNothing Arg5 = TRefOrNothing(),
                    TRefOrNothing Arg6 = TRefOrNothing(),
                    TRefOrNothing Arg7 = TRefOrNothing(),
                    TRefOrNothing Arg8 = TRefOrNothing(),
                    TRefOrNothing Arg9 = TRefOrNothing(),
                    TRefOrNothing Arg10 = TRefOrNothing(),
                    TRefOrNothing Arg11 = TRefOrNothing(),
                    TRefOrNothing Arg12 = TRefOrNothing(),
                    TRefOrNothing Arg13 = TRefOrNothing(),
                    TRefOrNothing Arg14 = TRefOrNothing(),
                    TRefOrNothing Arg15 = TRefOrNothing()) {
            TRefOrNothing Args[] = {Arg0,  Arg1,  Arg2,  Arg3, Arg4,  Arg5,
                Arg6,  Arg7,  Arg8,  Arg9, Arg10, Arg11,
                Arg12, Arg13, Arg14, Arg15};
            if (size() > array_lengthof(Args))
                return false;
            
            for (unsigned i = 0, e = size(); i != e; ++i)
                if (Args[i].TPtr == nullptr || (*this)[i] != *Args[i].TPtr)
                    return false;
            
            // Either the size is exactly as many args, or the next arg must be null.
            return size() == array_lengthof(Args) || Args[size()].TPtr == nullptr;
        }
        
        /// @}
    };
    
    /// MutableArray - Represent a mutable reference to an array (0 or more
    /// elements consecutively in memory), i.e. a start pointer and a length.  It
    /// allows various APIs to take and modify consecutive elements easily and
    /// conveniently.
    ///
    /// This class does not own the underlying data, it is expected to be used in
    /// situations where the data resides in some other buffer, whose lifetime
    /// extends past that of the MutableArray. For this reason, it is not in
    /// general safe to store a MutableArray.
    ///
    /// This is intended to be trivially copyable, so it should be passed by
    /// value.
    template<typename T>
    class MutableArray : public Array<T> {
    public:
        typedef T *iterator;
        
        typedef std::reverse_iterator<iterator> reverse_iterator;
        
        /// Construct an empty MutableArray.
        /*implicit*/ MutableArray() : Array<T>() {}
        
        /// Construct an empty MutableArray from None.
        /*implicit*/ MutableArray(decltype(nullptr)) : Array<T>() {}
        
        /// Construct an MutableArray from a single element.
        /*implicit*/ MutableArray(T &OneElt) : Array<T>(OneElt) {}
        
        /// Construct an MutableArray from a pointer and length.
        /*implicit*/ MutableArray(T *data, size_t length)
        : Array<T>(data, length) {}
        
        /// Construct an MutableArray from a range.
        MutableArray(T *begin, T *end) : Array<T>(begin, end) {}
        
        /// Construct a MutableArray from a std::vector.
        /*implicit*/ MutableArray(std::vector<T> &Vec)
        : Array<T>(Vec) {}
        
        /// Construct an MutableArray from a C array.
        template <size_t N>
        /*implicit*/ constexpr MutableArray(T (&Arr)[N])
        : Array<T>(Arr) {}
        
        T *data() const { return const_cast<T*>(Array<T>::data()); }
        
        iterator begin() const { return data(); }
        iterator end() const { return data() + this->size(); }
        
        reverse_iterator rbegin() const { return reverse_iterator(end()); }
        reverse_iterator rend() const { return reverse_iterator(begin()); }
        
        /// front - Get the first element.
        T &front() const {
            assert(!this->empty());
            return data()[0];
        }
        
        /// back - Get the last element.
        T &back() const {
            assert(!this->empty());
            return data()[this->size()-1];
        }
        
        /// slice(n) - Chop off the first N elements of the array.
        MutableArray<T> slice(unsigned N) const {
            assert(N <= this->size() && "Invalid specifier");
            return MutableArray<T>(data()+N, this->size()-N);
        }
        
        /// slice(n, m) - Chop off the first N elements of the array, and keep M
        /// elements in the array.
        MutableArray<T> slice(unsigned N, unsigned M) const {
            assert(N+M <= this->size() && "Invalid specifier");
            return MutableArray<T>(data()+N, M);
        }
        
        /// @}
        /// @name Operator Overloads
        /// @{
        T &operator[](size_t Index) const {
            assert(Index < this->size() && "Invalid index!");
            return data()[Index];
        }
    };
    
    /// @name Array Convenience constructors
    /// @{
    
    /// Construct an Array from a single element.
    template<typename T>
    Array<T> makeArray(const T &OneElt) {
        return OneElt;
    }
    
    /// Construct an Array from a pointer and length.
    template<typename T>
    Array<T> makeArray(const T *data, size_t length) {
        return Array<T>(data, length);
    }
    
    /// Construct an Array from a range.
    template<typename T>
    Array<T> makeArray(const T *begin, const T *end) {
        return Array<T>(begin, end);
    }
    
    /// Construct an Array from a std::vector.
    template<typename T>
    Array<T> makeArray(const std::vector<T> &Vec) {
        return Vec;
    }
    
    /// Construct an Array from a C array.
    template<typename T, size_t N>
    Array<T> makeArray(const T (&Arr)[N]) {
        return Array<T>(Arr);
    }
    
    /// @}
    /// @name Array Comparison Operators
    /// @{
    
    template<typename T>
    inline bool operator==(Array<T> LHS, Array<T> RHS) {
        return LHS.equals(RHS);
    }
    
    template<typename T>
    inline bool operator!=(Array<T> LHS, Array<T> RHS) {
        return !(LHS == RHS);
    }
    
    /// @}
    
    // Arrays can be treated like a POD type.
    template <typename T> struct isPodLike;
    template <typename T> struct isPodLike<Array<T> > {
        static const bool value = true;
    };
}

#endif /* defined(__RSCoreFoundation__Array__) */
