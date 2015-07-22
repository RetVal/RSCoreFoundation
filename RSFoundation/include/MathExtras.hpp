//
//  MathExtras.h
//  RSCoreFoundation
//
//  Created by closure on 7/22/15.
//  Copyright (c) 2015 RetVal. All rights reserved.
//

#ifndef __RSCoreFoundation__MathExtras__
#define __RSCoreFoundation__MathExtras__

#include <cassert>
#include <cstring>
#include <type_traits>
#include <limits>

#ifdef _MSC_VER
#include <intrin.h>
#include <limits>
#endif

#include <RSFoundation/Object.hpp>

namespace RSCF {
#ifndef __GNUC_PREREQ
# if defined(__GNUC__) && defined(__GNUC_MINOR__)
#  define __GNUC_PREREQ(maj, min) \
((__GNUC__ << 16) + __GNUC_MINOR__ >= ((maj) << 16) + (min))
# else
#  define __GNUC_PREREQ(maj, min) 0
# endif
#endif
    
    enum ZeroBehavior {
        /// \brief The returned value is undefined.
        ZB_Undefined,
        /// \brief The returned value is numeric_limits<T>::max()
        ZB_Max,
        /// \brief The returned value is numeric_limits<T>::digits
        ZB_Width
    };
    
    /// \brief Count number of 0's from the least significant bit to the most
    ///   stopping at the first 1.
    ///
    /// Only size_t integral types are allowed.
    ///
    /// \param ZB the behavior on an input of 0. Only ZB_Width and ZB_Undefined are
    ///   valid arguments.
    template <typename T>
    typename std::enable_if<std::numeric_limits<T>::is_integer &&
    !std::numeric_limits<T>::is_signed, std::size_t>::type
    countTrailingZeros(T Val, ZeroBehavior ZB = ZB_Width) {
        (void)ZB;
        
        if (!Val)
            return std::numeric_limits<T>::digits;
        if (Val & 0x1)
            return 0;
        
        // Bisection method.
        std::size_t ZeroBits = 0;
        T Shift = std::numeric_limits<T>::digits >> 1;
        T Mask = std::numeric_limits<T>::max() >> Shift;
        while (Shift) {
            if ((Val & Mask) == 0) {
                Val >>= Shift;
                ZeroBits |= Shift;
            }
            Shift >>= 1;
            Mask >>= Shift;
        }
        return ZeroBits;
    }
    
    // Disable signed.
    template <typename T>
    typename std::enable_if<std::numeric_limits<T>::is_integer &&
    std::numeric_limits<T>::is_signed, std::size_t>::type
    countTrailingZeros(T Val, ZeroBehavior ZB = ZB_Width);
    
#if __GNUC__ >= 4 || _MSC_VER
    template <>
    inline std::size_t countTrailingZeros<uint32_t>(uint32_t Val, ZeroBehavior ZB) {
        if (ZB != ZB_Undefined && Val == 0)
            return 32;
        
#if __has_builtin(__builtin_ctz) || __GNUC_PREREQ(4, 0)
        return __builtin_ctz(Val);
#elif _MSC_VER
        size_t long Index;
        _BitScanForward(&Index, Val);
        return Index;
#endif
    }
    
#if !defined(_MSC_VER) || defined(_M_X64)
    template <>
    inline std::size_t countTrailingZeros<uint64_t>(uint64_t Val, ZeroBehavior ZB) {
        if (ZB != ZB_Undefined && Val == 0)
            return 64;
        
#if __has_builtin(__builtin_ctzll) || __GNUC_PREREQ(4, 0)
        return __builtin_ctzll(Val);
#elif _MSC_VER
        size_t long Index;
        _BitScanForward64(&Index, Val);
        return Index;
#endif
    }
#endif
#endif
    
    /// \brief Count number of 0's from the most significant bit to the least
    ///   stopping at the first 1.
    ///
    /// Only size_t integral types are allowed.
    ///
    /// \param ZB the behavior on an input of 0. Only ZB_Width and ZB_Undefined are
    ///   valid arguments.
    template <typename T>
    typename std::enable_if<std::numeric_limits<T>::is_integer &&
    !std::numeric_limits<T>::is_signed, std::size_t>::type
    countLeadingZeros(T Val, ZeroBehavior ZB = ZB_Width) {
        (void)ZB;
        
        if (!Val)
            return std::numeric_limits<T>::digits;
        
        // Bisection method.
        std::size_t ZeroBits = 0;
        for (T Shift = std::numeric_limits<T>::digits >> 1; Shift; Shift >>= 1) {
            T Tmp = Val >> Shift;
            if (Tmp)
                Val = Tmp;
            else
                ZeroBits |= Shift;
        }
        return ZeroBits;
    }
    
    // Disable signed.
//    template <typename T>
//    typename std::enable_if<std::numeric_limits<T>::is_integer &&
//    std::numeric_limits<T>::is_signed, std::size_t>::type
//    countLeadingZeros(T Val, ZeroBehavior ZB = ZB_Width);
    
#if __GNUC__ >= 4 || _MSC_VER
    template <>
    inline std::size_t countLeadingZeros<uint32_t>(uint32_t Val, ZeroBehavior ZB) {
        if (ZB != ZB_Undefined && Val == 0)
            return 32;
        
#if __has_builtin(__builtin_clz) || __GNUC_PREREQ(4, 0)
        return __builtin_clz(Val);
#elif _MSC_VER
        size_t long Index;
        _BitScanReverse(&Index, Val);
        return Index ^ 31;
#endif
    }
    
#if !defined(_MSC_VER) || defined(_M_X64)
    template <>
    inline std::size_t countLeadingZeros<uint64_t>(uint64_t Val, ZeroBehavior ZB) {
        if (ZB != ZB_Undefined && Val == 0)
            return 64;
        
#if __has_builtin(__builtin_clzll) || __GNUC_PREREQ(4, 0)
        return __builtin_clzll(Val);
#elif _MSC_VER
        size_t long Index;
        _BitScanReverse64(&Index, Val);
        return Index ^ 63;
#endif
    }
#endif
#endif
    
    /// isPowerOf2_32 - This function returns true if the argument is a power of
    /// two > 0. Ex. isPowerOf2_32(0x00100000U) == true (32 bit edition.)
    inline bool isPowerOf2_32(uint32_t Value) {
        return Value && !(Value & (Value - 1));
    }
    
    /// isPowerOf2_64 - This function returns true if the argument is a power of two
    /// > 0 (64 bit edition.)
    inline bool isPowerOf2_64(uint64_t Value) {
        return Value && !(Value & (Value - int64_t(1L)));
    }
    
    /// CountLeadingOnes_32 - this function performs the operation of
    /// counting the number of ones from the most significant bit to the first zero
    /// bit.  Ex. CountLeadingOnes_32(0xFF0FFF00) == 8.
    /// Returns 32 if the word is all ones.
    inline size_t CountLeadingOnes_32(uint32_t Value) {
        return countLeadingZeros(~Value);
    }
    
    /// CountLeadingOnes_64 - This function performs the operation
    /// of counting the number of ones from the most significant bit to the first
    /// zero bit (64 bit edition.)
    /// Returns 64 if the word is all ones.
    inline size_t CountLeadingOnes_64(uint64_t Value) {
        return countLeadingZeros(~Value);
    }
    
    /// CountTrailingOnes_32 - this function performs the operation of
    /// counting the number of ones from the least significant bit to the first zero
    /// bit.  Ex. CountTrailingOnes_32(0x00FF00FF) == 8.
    /// Returns 32 if the word is all ones.
    inline size_t CountTrailingOnes_32(uint32_t Value) {
        return countTrailingZeros(~Value);
    }
    
    /// CountTrailingOnes_64 - This function performs the operation
    /// of counting the number of ones from the least significant bit to the first
    /// zero bit (64 bit edition.)
    /// Returns 64 if the word is all ones.
    inline size_t CountTrailingOnes_64(uint64_t Value) {
        return countTrailingZeros(~Value);
    }
    
    /// CountPopulation_32 - this function counts the number of set bits in a value.
    /// Ex. CountPopulation(0xF000F000) = 8
    /// Returns 0 if the word is zero.
    inline size_t CountPopulation_32(uint32_t Value) {
#if __GNUC__ >= 4
        return __builtin_popcount(Value);
#else
        uint32_t v = Value - ((Value >> 1) & 0x55555555);
        v = (v & 0x33333333) + ((v >> 2) & 0x33333333);
        return ((v + (v >> 4) & 0xF0F0F0F) * 0x1010101) >> 24;
#endif
    }
    
    /// CountPopulation_64 - this function counts the number of set bits in a value,
    /// (64 bit edition.)
    inline size_t CountPopulation_64(uint64_t Value) {
#if __GNUC__ >= 4
        return __builtin_popcountll(Value);
#else
        uint64_t v = Value - ((Value >> 1) & 0x5555555555555555ULL);
        v = (v & 0x3333333333333333ULL) + ((v >> 2) & 0x3333333333333333ULL);
        v = (v + (v >> 4)) & 0x0F0F0F0F0F0F0F0FULL;
        return size_t((uint64_t)(v * 0x0101010101010101ULL) >> 56);
#endif
    }

    /// MinAlign - A and B are either alignments or offsets.  Return the minimum
    /// alignment that may be assumed after adding the two together.
    inline uint64_t MinAlign(uint64_t A, uint64_t B) {
        // The largest power of 2 that divides both A and B.
        //
        // Replace "-Value" by "1+~Value" in the following commented code to avoid
        // MSVC warning C4146
        //    return (A | B) & -(A | B);
        return (A | B) & (1 + ~(A | B));
    }
    
    /// \brief Aligns \c Ptr to \c Alignment bytes, rounding up.
    ///
    /// Alignment should be a power of two.  This method rounds up, so
    /// AlignPtr(7, 4) == 8 and AlignPtr(8, 4) == 8.
    inline char *alignPtr(char *Ptr, size_t Alignment) {
        assert(Alignment && isPowerOf2_64((uint64_t)Alignment) &&
               "Alignment is not a power of two!");
        
        return (char *)(((uintptr_t)Ptr + Alignment - 1) &
                        ~(uintptr_t)(Alignment - 1));
    }
    
    /// Log2_32 - This function returns the floor log base 2 of the specified value,
    /// -1 if the value is zero. (32 bit edition.)
    /// Ex. Log2_32(32) == 5, Log2_32(1) == 0, Log2_32(0) == -1, Log2_32(6) == 2
    inline size_t Log2_32(uint32_t Value) {
        return 31 - countLeadingZeros(Value);
    }
    
    /// Log2_64 - This function returns the floor log base 2 of the specified value,
    /// -1 if the value is zero. (64 bit edition.)
    inline size_t Log2_64(uint64_t Value) {
        return 63 - countLeadingZeros(Value);
    }
    
    /// Log2_32_Ceil - This function returns the ceil log base 2 of the specified
    /// value, 32 if the value is zero. (32 bit edition).
    /// Ex. Log2_32_Ceil(32) == 5, Log2_32_Ceil(1) == 0, Log2_32_Ceil(6) == 3
    inline size_t Log2_32_Ceil(uint32_t Value) {
        return 32 - countLeadingZeros(Value - 1);
    }
    
    /// Log2_64_Ceil - This function returns the ceil log base 2 of the specified
    /// value, 64 if the value is zero. (64 bit edition.)
    inline size_t Log2_64_Ceil(uint64_t Value) {
        return 64 - countLeadingZeros(Value - 1);
    }
    
    /// NextPowerOf2 - Returns the next power of two (in 64-bits)
    /// that is strictly greater than A.  Returns zero on overflow.
    inline uint64_t NextPowerOf2(uint64_t A) {
        A |= (A >> 1);
        A |= (A >> 2);
        A |= (A >> 4);
        A |= (A >> 8);
        A |= (A >> 16);
        A |= (A >> 32);
        return A + 1;
    }
    
    /// Returns the power of two which is less than or equal to the given value.
    /// Essentially, it is a floor operation across the domain of powers of two.
    inline uint64_t PowerOf2Floor(uint64_t A) {
        if (!A) return 0;
        return 1ull << (63 - countLeadingZeros(A, ZB_Undefined));
    }
    
    /// Returns the next integer (mod 2**64) that is greater than or equal to
    /// \p Value and is a multiple of \p Align. \p Align must be non-zero.
    ///
    /// Examples:
    /// \code
    ///   RoundUpToAlignment(5, 8) = 8
    ///   RoundUpToAlignment(17, 8) = 24
    ///   RoundUpToAlignment(~0LL, 8) = 0
    /// \endcode
    inline uint64_t RoundUpToAlignment(uint64_t Value, uint64_t Align) {
        return ((Value + Align - 1) / Align) * Align;
    }
    
    /// Returns the offset to the next integer (mod 2**64) that is greater than
    /// or equal to \p Value and is a multiple of \p Align. \p Align must be
    /// non-zero.
    inline uint64_t OffsetToAlignment(uint64_t Value, uint64_t Align) {
        return RoundUpToAlignment(Value, Align) - Value;
    }
    
    /// abs64 - absolute value of a 64-bit int.  Not all environments support
    /// "abs" on whatever their name for the 64-bit int type is.  The absolute
    /// value of the largest negative number is undefined, as with "abs".
    inline int64_t abs64(int64_t x) {
        return (x < 0) ? -x : x;
    }
    
    /// SignExtend32 - Sign extend B-bit number x to 32-bit int.
    /// Usage int32_t r = SignExtend32<5>(x);
    template <size_t B> inline int32_t SignExtend32(uint32_t x) {
        return int32_t(x << (32 - B)) >> (32 - B);
    }
    
    /// \brief Sign extend number in the bottom B bits of X to a 32-bit int.
    /// Requires 0 < B <= 32.
    inline int32_t SignExtend32(uint32_t X, size_t B) {
        return int32_t(X << (32 - B)) >> (32 - B);
    }
    
    /// SignExtend64 - Sign extend B-bit number x to 64-bit int.
    /// Usage int64_t r = SignExtend64<5>(x);
    template <size_t B> inline int64_t SignExtend64(uint64_t x) {
        return int64_t(x << (64 - B)) >> (64 - B);
    }
    
    /// \brief Sign extend number in the bottom B bits of X to a 64-bit int.
    /// Requires 0 < B <= 64.
    inline int64_t SignExtend64(uint64_t X, size_t B) {
        return int64_t(X << (64 - B)) >> (64 - B);
    }
    
#if defined(_MSC_VER)
    // Visual Studio defines the HUGE_VAL class of macros using purposeful
    // constant arithmetic overflow, which it then warns on when encountered.
    const float huge_valf = std::numeric_limits<float>::infinity();
#else
    const float huge_valf = HUGE_VALF;
#endif
}

#endif /* defined(__RSCoreFoundation__MathExtras__) */
