//
//  RSBaseMacro.h
//  RSCoreFoundation
//
//  Created by RetVal on 10/13/12.
//  Copyright (c) 2012 RetVal. All rights reserved.
//
#import <RSCoreFoundation/RSErrorCode.h>
#include <RSCoreFoundation/RSAvailability.h>
#ifndef RSCoreFoundation_RSBaseMacro_h
#define RSCoreFoundation_RSBaseMacro_h

#if __has_feature(objc_arc)
#ifndef BWI
#define BWI(code)   if(((code) != (0))) break
#endif
#else
#ifndef BWI
#define BWI(code)   if(((code) != (0))) break
#endif
#endif

#ifndef RS_RETURNS_RETAINED
#if __has_feature(attribute_cf_returns_retained)
#define RS_RETURNS_RETAINED __attribute__((cf_returns_retained))
#else
#define RS_RETURNS_RETAINED
#endif
#endif

// Marks functions which return a RS type that may need to be retained by the caller but whose names are not consistent with CoreFoundation naming rules. The recommended fix to this is to rename the functions, but this macro can be used to let the clang static analyzer know of any exceptions that cannot be fixed.
// This macro is ONLY to be used in exceptional circumstances, not to annotate functions which conform to the CoreFoundation naming rules.
#ifndef RS_RETURNS_NOT_RETAINED
#if __has_feature(attribute_cf_returns_not_retained)
#define RS_RETURNS_NOT_RETAINED __attribute__((cf_returns_not_retained))
#else
#define RS_RETURNS_NOT_RETAINED
#endif
#endif

#ifndef RS_RELEASES_ARGUMENT
#if __has_feature(attribute_cf_consumed)
#define RS_RELEASES_ARGUMENT __attribute__((cf_consumed))
#else
#define RS_RELEASES_ARGUMENT
#endif
#endif

#if __has_feature(objc_arc)
#ifndef RCM
#define RCM(code,ret,r1,r2)         BWI((ret) = (((int*)(code) != nil)?(r1):(r2)))
#define RCO(obj,ret)                BWI((ret) = ((obj)?(kSuccess):(kErrVerify)))
#define RCMP(__pointer,RSError)		RCM(__pointer,RSError,kSuccess,kErrNil)
#define RCPP(__ptr_pointer,RSError)	RCM(!(RSUInteger)((*(RSUInteger*)(__ptr_pointer))),RSError,kSuccess,kErrNNil)
#endif
#else
#ifndef RCM
#define RCM(code,ret,r1,r2)         BWI((ret) = (((int*)(code) != nil)?(r1):(r2)))
#define RCO(obj,ret)                BWI((ret) = ((obj)?(kSuccess):(kErrVerify)))
#define RCMP(__pointer,RSError)		RCM(__pointer,RSError,kSuccess,kErrNil)
#define RCPP(__ptr_pointer,RSError)	RCM(!(RSUInteger)((*(RSUInteger*)(__ptr_pointer))),RSError,kSuccess,kErrNNil)
#endif
#endif

#if defined(__cplusplus)
#define RSExtern extern "C"
#else
#define RSExtern extern
#endif

/*
#ifndef RSExport
	#if TARGET_OS_WIN32

		#if !defined(RS_EXPORT)
			#if defined(RS_BUILDING_RS) && defined(__cplusplus)
				#define RS_EXPORT extern "C" __declspec(dllexport)
			#elif defined(RS_BUILDING_RS) && !defined(__cplusplus)
				#define RS_EXPORT extern __declspec(dllexport)
			#elif defined(__cplusplus)
				#define RS_EXPORT extern "C" __declspec(dllimport)
			#else
				#define RS_EXPORT extern __declspec(dllimport)
			#endif
		#endif
	#else
		#if defined(__cplusplus)
			#define RS_EXPORT extern "C" __attribute__((visibility("default")))
		#else
			#define RS_EXPORT __attribute__((visibility("default")))
		#endif
	#endif
	#define RSExport RS_EXPORT
#endif
*/

#ifndef RSExport
	#if TARGET_OS_WIN32
		#if !defined(RS_EXPORT)
			#if defined(RS_BUILDING_RS) && defined(__cplusplus)
				#define RS_EXPORT extern "C" __declspec(dllexport)
			#elif defined(RS_BUILDING_RS) && !defined(__cplusplus)
				#define RS_EXPORT extern __declspec(dllexport)
			#elif defined(__cplusplus)
				#define RS_EXPORT extern "C" __declspec(dllimport)
			#else
				#define RS_EXPORT extern __declspec(dllimport)
			#endif
		#endif
	#else
		#define RS_EXPORT RSExtern
	#endif
#define RSExport RS_EXPORT
#endif

#ifndef RSInside
#define RSInside static
#endif

#if !defined(RSInline)
    #if defined(__GNUC__)
        #define RSInline static __inline__ __attribute__((always_inline))
    #elif defined(__MWERKS__) || defined(__cplusplus)
        #define RSInline static inline
    #elif defined(_MSC_VER)
        #define RSInline static __inline
    #elif TARGET_OS_WIN32
        #define RSInline static __inline__
    #endif
#endif

#if !defined(RS_INLINE)
    #if defined(__GNUC__)
        #define RS_INLINE __inline__ __attribute__((always_inline))
    #elif defined(__MWERKS__) || defined(__cplusplus)
        #define RS_INLINE inline
    #elif defined(_MSC_VER)
        #define RS_INLINE  __inline
    #elif TARGET_OS_WIN32
        #define RS_INLINE __inline__
    #endif
#endif

#ifndef __autorelease
    #define __autorelease
#endif

#ifndef RS_OVER_LOADABLE
    #if defined(__clang__)
        #if __has_extension(attribute_overloadable)
            #define RS_OVER_LOADABLE __attribute__((overloadable))
        #else
            #define RS_OVER_LOADABLE
        #endif
    #elif defined(_MSC_VER)
        #define RS_OVER_LOADABLE RS_UNAVAILABLE(0_0)
    #endif
#endif

#ifndef RS_BLOCKS_AVAILABLE
    #if __has_extension(blocks)
        #define RS_BLOCKS_AVAILABLE 1
    #else
        #define RS_BLOCKS_AVAILABLE 0
    #endif
#endif

#if !defined(RS_REQUIRES_NIL_TERMINATION)
    #if TARGET_OS_WIN32
        #define RS_REQUIRES_NIL_TERMINATION
    #else
        #if defined(__APPLE_CC__) && (__APPLE_CC__ >= 5549)
            #define RS_REQUIRES_NIL_TERMINATION __attribute__((sentinel(0,1)))
        #else
            #define RS_REQUIRES_NIL_TERMINATION __attribute__((sentinel))
        #endif
    #endif
#endif
//__attribute__((sentinel(0,1)))
#if (__cplusplus && __cplusplus >= 201103L && (__has_extension(cxx_strong_enums) || __has_feature(objc_fixed_enum))) || (!__cplusplus && __has_feature(objc_fixed_enum))
#define RS_ENUM(_type, _name) enum _name : _type _name; enum _name : _type
#if (__cplusplus)
#define RS_OPTIONS(_type, _name) _type _name; enum : _type
#else
#define RS_OPTIONS(_type, _name) enum _name : _type _name; enum _name : _type
#endif
#else
#define RS_ENUM(_type, _name) _type _name; enum
#define RS_OPTIONS(_type, _name) _type _name; enum
#endif

#ifdef DEBUG
    #define RSDLog(format, ...) RSLog(format, ## __VA_ARGS__)
#else
    #define RSDLog(format, ...)
#endif

#ifndef RSUpTo8
#define RSUpTo8(size) (((size)+7) &~ 7)
#endif
#if !defined(__cplusplus)
#ifndef min//(a,b)
#define min(a,b) (((a)>(b)) ? (b):(a))
#endif

#ifndef max//(a,b)
#define max(a,b) (((a)>(b)) ? (a):(b))
#endif
#endif

#ifndef RSZeroMemory
#define RSZeroMemory(memblock,len)    memset((void*)(memblock),0,(size_t)(len))
#endif

#ifndef RSCopyMemory
#define RSCopyMemory(memblock,mem2block,len) memcpy((void*)(memblock),(void*)(mem2block),(size_t)(len))
#endif

#ifndef RSReserved
#define RSReserved __attribute__((visibility("hidden")))
#endif

#ifndef RSPrivate
#define RSPrivate  __private_extern__
#endif

#if TARGET_OS_MAC
#define kRtlInstallOSMainVersion        10
#define kRtlInstallOSSubVersion         7
#define kRtlInstallOSBugFixVersion      0
#elif TARGET_OS_IPHONE
#define kRtlInstallOSMainVersion        6
#define kRtlInstallOSSubVersion         0
#define kRtlInstallOSBugFixVersion      0
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

#define RSTraceLogDefault __PRETTY_FUNCTION__,__FILE__,__LINE__,NULL
#define RSTraceLogWithReason __PRETTY_FUNCTION__,__FILE__,__LINE__
RSExport void RSTraceLog(const char*, const char*,unsigned long,const char* );
#define RSTraceService 1
#if defined(RSTraceService)
#define RSTraceDbg  RSTraceLog(RSTraceLogDefault);
#define RSTraceWithError(reason) RSTraceLog(RSTraceLogWithReason,(reason))
#else
#define RSTraceDbg
#endif

#ifndef  __RS_INIT_ROUTINE
#define  __RS_INIT_ROUTINE(s) __attribute__ ((constructor((s))))
#endif

#ifndef  __RS_FINAL_ROUTINE
#define  __RS_FINAL_ROUTINE(s) __attribute__ ((destructor((s))))
#endif

#ifndef  RS_INIT_ROUTINE
#define  RS_INIT_ROUTINE __attribute__ ((constructor))
#endif

#ifndef  RS_FINAL_ROUTINE
#define  RS_FINAL_ROUTINE __attribute__ ((destructor))
#endif

#endif


