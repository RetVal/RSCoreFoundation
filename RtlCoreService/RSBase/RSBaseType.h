//
//  RSBaseType.h
//  RSCoreFoundation
//
//  Created by RetVal on 11/17/12.
//  Copyright (c) 2012 RetVal. All rights reserved.
//

#ifndef RSCoreFoundation_RSBaseType_h
#define RSCoreFoundation_RSBaseType_h
#import <RSCoreFoundation/RSBaseMacro.h>
#include <sys/types.h>
#include <stdarg.h>
#include <assert.h>
#include <ctype.h>
#include <errno.h>
#include <float.h>
#include <limits.h>
#include <locale.h>
#include <math.h>
#include <setjmp.h>
#include <signal.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <stdint.h>
#if defined(__STDC_VERSION__) && (199901L <= __STDC_VERSION__)

#include <inttypes.h>
#include <stdbool.h>


#endif

#define RS_BITS_PER_BYTE 8
#ifndef RSBufferSize
#define RSBufferSize PATH_MAX
#endif

#if !defined(RS_EXTERN_C_BEGIN)
#if defined(__cplusplus)
#define RS_EXTERN_C_BEGIN extern "C" {
#define RS_EXTERN_C_END   }
#else
#define RS_EXTERN_C_BEGIN
#define RS_EXTERN_C_END
#endif
#endif
typedef char  RSBlock;
typedef RSBlock* RSBuffer;

typedef const char RSCBlock;
typedef RSCBlock* RSCBuffer;

typedef unsigned char RSUBlock;
typedef RSUBlock* RSUBuffer;

typedef const unsigned char RSCUBlock;
typedef RSCUBlock* RSCUBuffer;

typedef wchar_t RSWBlock;
typedef RSWBlock* RSWBuffer;

typedef const wchar_t RSCWBlock;
typedef RSCWBlock* RSCWBuffer;

typedef void* RSHandle;

typedef const void* RSCHandle;
typedef signed char BOOL;

typedef int8_t  RSBit8;    //-128 to 127
typedef int16_t RSBit16;   //-32,768 to 32,767
typedef int32_t RSBit32;   //-2,147,483,648 to 2,147,483,647
typedef int64_t RSBit64;   //-9,223,372,036,854,775,808 to 9,223,372,036,854,775,807
typedef uint8_t RSBitU8;   //0 to 255
typedef uint16_t RSBitU16; //0 to 65,535
typedef uint32_t RSBitU32; //0 to 4,294,967,295
typedef uint64_t RSBitU64; //0 to 18,446,744,073,709,551,615

#if __LP64__
typedef RSBit64 RSBGNumber;
#else
typedef RSBit32 RSBGNumber;
#endif

#if   (TARGET_OS_MAC)||(TARGET_OS_IPHONE)

#define MACOSX

#elif TARGET_OS_WIN
    #define YES 1
    #define NO  0
#endif

#if __LP64__ || (TARGET_OS_EMBEDDED && !TARGET_OS_IPHONE) || TARGET_OS_WIN32 || NS_BUILD_32_LIKE_64
typedef long RSInteger;
typedef unsigned long RSUInteger;
typedef double RSFloat;
#else
typedef int RSInteger;
typedef unsigned int RSUInteger;
typedef float RSFloat;
#endif

typedef double RSTimeInterval;
typedef float  Float32;
typedef double Float64;

typedef RSBit16 RSCode;

typedef RSCode RSErrorCode;
typedef void* ISA;

#if (DEPLOYMENT_TARGET_MACOSX || DEPLOYMENT_TARGET_IPHONE)
#include <libkern/OSAtomic.h>
typedef volatile OSSpinLock RSSpinLock;
#define RSSpinLockInit OS_SPINLOCK_INIT

RSInline void RSSpinLockLock(volatile RSSpinLock *lock) {
    OSSpinLockLock(lock);
}

RSInline void RSSpinLockUnlock(volatile RSSpinLock *lock) {
    OSSpinLockUnlock(lock);
}
    
RSInline BOOL RSSpinLockTry(volatile RSSpinLock *lock) {
    return OSSpinLockTry(lock);
}

#if DEPLOYMENT_TARGET_WINDOWS
#include <Windows.h>
typedef RSBit32 RSSpinLock;
#define RSSpinLockInit 0
#define RS_SPINLOCK_INIT_FOR_STRUCTS(X) (X = RSSpinLockInit)

RSInline void RSSpinLock(volatile RSSpinLock *lock) {
    while (InterlockedCompareExchange((LONG volatile *)lock, ~0, 0) != 0) {
        Sleep(0);
    }
}

RSInline void RSSpinUnlock(volatile RSSpinLock_t *lock) {
    MemoryBarrier();
    *lock = 0;
}

RSInline BOOL RSSpinLockTry(volatile RSSpinLock_t *lock) {
    return (InterlockedCompareExchange((LONG volatile *)lock, ~0, 0) == 0);
}

#elif DEPLOYMENT_TARGET_LINUX

typedef RSBit32 RSSpinLock_t;
#define RSSpinLockInit 0
#define RS_SPINLOCK_INIT_FOR_STRUCTS(X) (X = RSSpinLockInit)

RSInline void RSSpinLock(volatile RSSpinLock_t *lock) {
    while (__sync_val_compare_and_swap(lock, 0, ~0) != 0) {
        sleep(0);
    }
}

RSInline void RSSpinUnlock(volatile RSSpinLock_t *lock) {
    __sync_synchronize();
    *lock = 0;
}

RSInline BOOL RSSpinLockTry(volatile RSSpinLock_t *lock) {
    return (__sync_val_compare_and_swap(lock, 0, ~0) == 0);
}
#endif


#if RS_BLOCKS_AVAILABLE

void RSSyncUpdateBlock(volatile RSSpinLock *lock, void (^block)(void));

#endif
#endif

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif
#ifndef YES
#define YES (BOOL)1
#endif
#ifndef NO
#define NO (BOOL)0
#endif
#ifndef nill
#define nill nil
#endif
#endif

#ifndef RSCoreFoundation_RSBaseObject
#define RSCoreFoundation_RSBaseObject
#if __LP64__
typedef unsigned long long RSTypeID;
typedef unsigned long long RSOptionFlags;
typedef unsigned long long RSHashCode;
typedef signed long long RSIndex;
#else
typedef unsigned long RSTypeID;
typedef unsigned long RSOptionFlags;
typedef unsigned long RSHashCode;
typedef signed long RSIndex;
#endif
typedef const void* RSTypeRef;
typedef void* RSMutableTypeRef;

typedef RSTypeRef RSClassRef;

typedef unsigned char                   UInt8;
typedef signed char                     SInt8;
typedef unsigned short                  UInt16;
typedef signed short                    SInt16;

#if __LP64__
typedef unsigned int                    UInt32;
typedef signed int                      SInt32;
#else
typedef unsigned long                   UInt32;
typedef signed long                     SInt32;
#endif

typedef UInt32                          UnicodeScalarValue;
typedef UInt32                          UTF32Char;
typedef UInt16                          UniChar;
typedef UInt16                          UTF16Char;
typedef UInt8                           UTF8Char;
typedef UniChar *                       UniCharPtr;
typedef unsigned long                   UniCharCount;
typedef UniCharCount *                  UniCharCountPtr;

typedef RSTimeInterval RSAbsoluteTime;

typedef uintptr_t RSPointer;

typedef enum {
    RSWriteFileDefault = 0,
    RSWriteFileAutomatically = 1
}RSWriteFileMode;

enum {
    RSNotFound = -1
};

enum {
    RSCompareLessThan = -1,
    RSCompareEqualTo = 0,
    RSCompareGreaterThan = 1
};
typedef RSInteger RSComparisonResult;

typedef RSComparisonResult (*RSComparatorFunction)(const void *val1, const void *val2, void *context);

typedef struct {
    RSIndex location;
    RSIndex length;
}RSRange;

RSInline RSRange RSMakeRange(RSIndex location, RSIndex length) {
    RSRange range;
    range.location = location;
    range.length = length;
    return range;
}

typedef RS_ENUM(RSInteger, RSComparisonOrder) {RSOrderedAscending = -1L, RSOrderedSame, RSOrderedDescending};
#endif
