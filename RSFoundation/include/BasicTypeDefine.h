//
//  RSBasicTypeDefine.h
//  RSCoreFoundation
//
//  Created by closure on 5/26/15.
//  Copyright (c) 2015 RetVal. All rights reserved.
//

#ifndef __RSCoreFoundation_RSBasicTypeDefine__
#define __RSCoreFoundation_RSBasicTypeDefine__

#include <sys/types.h>
#include <stdarg.h>
#include <assert.h>
#include <string.h>
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
#include <time.h>
#include <stdint.h>
#if defined(__STDC_VERSION__) && (199901L <= __STDC_VERSION__)

#include <inttypes.h>
#include <stdbool.h>

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

#if defined(__cplusplus)
#define RSFoundationExtern extern "C"
#else
#define RSFoundationExtern extern
#endif

#ifndef RSFoundationExport
#if TARGET_OS_WIN32
#if !defined(RSFOUNDATION_EXPORT)
#if defined(RS_BUILDING_RS) && defined(__cplusplus)
#define RSFOUNDATION_EXPORT extern "C" __declspec(dllexport)
#elif defined(RS_BUILDING_RS) && !defined(__cplusplus)
#define RSFOUNDATION_EXPORT extern __declspec(dllexport)
#elif defined(__cplusplus)
#define RSFOUNDATION_EXPORT extern "C" __declspec(dllimport)
#else
#define RSFOUNDATION_EXPORT extern __declspec(dllimport)
#endif
#endif
#else
#define RSFOUNDATION_EXPORT RSFoundationExtern
#endif
#define RSFoundationExport RSFOUNDATION_EXPORT
#endif

#ifndef RSFoundationInside
#define RSFoundationInside static
#endif

#if !defined(RSFoundationInline)
#if defined(__GNUC__)
#define RSFoundationInline static __inline__ __attribute__((always_inline))
#elif defined(__MWERKS__) || defined(__cplusplus)
#define RSFoundationInline static inline
#elif defined(_MSC_VER)
#define RSFoundationInline static __inline
#elif TARGET_OS_WIN32
#define RSFoundationInline static __inline__
#endif
#endif

#define Private

#if (__cplusplus && __cplusplus >= 201103L && (__has_extension(cxx_strong_enums) || __has_feature(objc_fixed_enum))) || (!__cplusplus && __has_feature(objc_fixed_enum))
#define RS_FOUNDATION_ENUM(_type, _name) enum _name : _type _name; enum _name : _type
#if (__cplusplus)
#define RS_FOUNDATION_OPTIONS(_type, _name) _type _name; enum : _type
#else
#define RS_FOUNDATION_OPTIONS(_type, _name) enum _name : _type _name; enum _name : _type
#endif
#else
#define RS_FOUNDATION_ENUM(_type, _name) _type _name; enum
#define RS_FOUNDATION_OPTIONS(_type, _name) _type _name; enum
#endif

#ifndef RSFoundationReserved
#define RSFoundationReserved __attribute__((visibility("hidden")))
#endif

#ifndef RSFoundationPrivate
#define RSFoundationPrivate  __private_extern__
#endif

#if TARGET_OS_WIN32
#define nil ((void*)0)
#else
#include <MacTypes.h>
#ifndef __MACTYPES__
#define nil ((void*)0)
#endif
#endif

#define __RS_COMPILE_YEAR__	(__DATE__[7] * 1000 + __DATE__[8] * 100 + __DATE__[9] * 10 + __DATE__[10] - 53328)
#define __RS_COMPILE_MONTH__	((__DATE__[1] + __DATE__[2] == 207) ? 1 : \
(__DATE__[1] + __DATE__[2] == 199) ? 2 : \
(__DATE__[1] + __DATE__[2] == 211) ? 3 : \
(__DATE__[1] + __DATE__[2] == 226) ? 4 : \
(__DATE__[1] + __DATE__[2] == 218) ? 5 : \
(__DATE__[1] + __DATE__[2] == 227) ? 6 : \
(__DATE__[1] + __DATE__[2] == 225) ? 7 : \
(__DATE__[1] + __DATE__[2] == 220) ? 8 : \
(__DATE__[1] + __DATE__[2] == 213) ? 9 : \
(__DATE__[1] + __DATE__[2] == 215) ? 10 : \
(__DATE__[1] + __DATE__[2] == 229) ? 11 : \
(__DATE__[1] + __DATE__[2] == 200) ? 12 : 0)
#define __RS_COMPILE_DAY__	(__DATE__[4] * 10 + __DATE__[5] - (__DATE__[4] == ' ' ? 368 : 528))
#define __RS_COMPILE_DATE__	(__RS_COMPILE_YEAR__ * 10000 + __RS_COMPILE_MONTH__ * 100 + __RS_COMPILE_DAY__)

#define __RS_COMPILE_HOUR__	(__TIME__[0] * 10 + __TIME__[1] - 528)
#define __RS_COMPILE_MINUTE__	(__TIME__[3] * 10 + __TIME__[4] - 528)
#define __RS_COMPILE_SECOND__	(__TIME__[6] * 10 + __TIME__[7] - 528)
#define __RS_COMPILE_TIME__	(__RS_COMPILE_HOUR__ * 10000 + __RS_COMPILE_MINUTE__ * 100 + __RS_COMPILE_SECOND__)

#define __RS_COMPILE_SECOND_OF_DAY__	(__RS_COMPILE_HOUR__ * 3600 + __RS_COMPILE_MINUTE__ * 60 + __RS_COMPILE_SECOND__)

// __RS_COMPILE_DAY_OF_EPOCH__ works within Gregorian years 2001 - 2099; the epoch is of course CF's epoch
#define __RS_COMPILE_DAY_OF_EPOCH__	((__RS_COMPILE_YEAR__ - 2001) * 365 + (__RS_COMPILE_YEAR__ - 2001) / 4 \
+ ((__DATE__[1] + __DATE__[2] == 207) ? 0 : \
(__DATE__[1] + __DATE__[2] == 199) ? 31 : \
(__DATE__[1] + __DATE__[2] == 211) ? 59 + (__RS_COMPILE_YEAR__ % 4 == 0) : \
(__DATE__[1] + __DATE__[2] == 226) ? 90 + (__RS_COMPILE_YEAR__ % 4 == 0) : \
(__DATE__[1] + __DATE__[2] == 218) ? 120 + (__RS_COMPILE_YEAR__ % 4 == 0) : \
(__DATE__[1] + __DATE__[2] == 227) ? 151 + (__RS_COMPILE_YEAR__ % 4 == 0) : \
(__DATE__[1] + __DATE__[2] == 225) ? 181 + (__RS_COMPILE_YEAR__ % 4 == 0) : \
(__DATE__[1] + __DATE__[2] == 220) ? 212 + (__RS_COMPILE_YEAR__ % 4 == 0) : \
(__DATE__[1] + __DATE__[2] == 213) ? 243 + (__RS_COMPILE_YEAR__ % 4 == 0) : \
(__DATE__[1] + __DATE__[2] == 215) ? 273 + (__RS_COMPILE_YEAR__ % 4 == 0) : \
(__DATE__[1] + __DATE__[2] == 229) ? 304 + (__RS_COMPILE_YEAR__ % 4 == 0) : \
(__DATE__[1] + __DATE__[2] == 200) ? 334 + (__RS_COMPILE_YEAR__ % 4 == 0) : \
365 + (__RS_COMPILE_YEAR__ % 4 == 0)) \
+ __RS_COMPILE_DAY__)

#ifndef  __RS_FOUNDATION_INIT_ROUTINE
#define  __RS_FOUNDATION_INIT_ROUTINE(s) __attribute__ ((constructor((s))))
#endif

#ifndef  __RS_FOUNDATION_FINAL_ROUTINE
#define  __RS_FOUNDATION_FINAL_ROUTINE(s) __attribute__ ((destructor((s))))
#endif

#ifndef  RS_FOUNDATION_INIT_ROUTINE
#define  RS_FOUNDATION_INIT_ROUTINE __attribute__ ((constructor))
#endif

#ifndef  RS_FOUNDATION_FINAL_ROUTINE
#define  RS_FOUNDATION_FINAL_ROUTINE __attribute__ ((destructor))
#endif

typedef signed char BOOL;

namespace RSFoundation {
    typedef decltype(nullptr) nullptr_t;
    
    typedef int8_t  Int8;    //-128 to 127
    typedef int16_t Int16;   //-32,768 to 32,767
    typedef int32_t Int32;   //-2,147,483,648 to 2,147,483,647
    typedef int64_t Int64;   //-9,223,372,036,854,775,808 to 9,223,372,036,854,775,807
    typedef uint8_t UInt8;   //0 to 255
    typedef uint16_t UInt16; //0 to 65,535
    typedef uint32_t UInt32; //0 to 4,294,967,295
    typedef uint64_t UInt64; //0 to 18,446,744,073,709,551,615
    
#if __LP64__
    typedef UInt64 BGNumber;
#else
    typedef UInt32 BGNumber;
#endif
    
#if   (TARGET_OS_MAC)||(TARGET_OS_IPHONE)
    
#define MACOSX
    
#elif TARGET_OS_WIN
#define YES 1
#define NO  0
#endif
    
#if __LP64__ || (TARGET_OS_EMBEDDED && !TARGET_OS_IPHONE) || TARGET_OS_WIN32 || NS_BUILD_32_LIKE_64
    typedef long Integer;
    typedef unsigned long UInteger;
    typedef double Float;
#else
    typedef int Integer;
    typedef unsigned int UInteger;
    typedef float Float;
#endif
    
    typedef double TimeInterval;
    typedef float  Float32;
    typedef double Float64;
    
    typedef Int16 Code;
    
    typedef Code ErrorCode;
    
#if __LP64__
    typedef unsigned long long TypeID;
    typedef unsigned long long OptionFlags;
    typedef unsigned long long HashCode;
    typedef signed long long Index;
#else
    typedef unsigned long TypeID;
    typedef unsigned long OptionFlags;
    typedef unsigned long HashCode;
    typedef signed long Index;
#endif
    
    enum ComparisonResult {
        LessThan    = -1,
        EqualTo     = 0,
        GreaterThan = 1
    };
    
    enum FoundResult {
        NotFound = -1,
        Found = 0
    };
    
    typedef ComparisonResult (*ComparatorFunction)(const void *val1, const void *val2, void *context);
    
    typedef struct {
        Index location;
        Index length;
    } Range;
    
    RSFoundationInline Range RSMakeRange(Index location, Index length) {
        Range range;
        range.location = location;
        range.length = length;
        return range;
    }
    
    typedef RS_FOUNDATION_ENUM(Integer, ComparisonOrder) {Ascending = -1L, Same, Descending};
    }
    
#include <RSFoundation/BasicAlgorithm.h>
#define DEPLOYMENT_TARGET_MACOSX 1
#define DEPLOYMENT_TARGET_LINUX 0
#define DEPLOYMENT_TARGET_WINDOWS 0
#define DEPLOYMENT_TARGET_IPHONEOS 0
#endif
