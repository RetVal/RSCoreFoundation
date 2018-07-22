//
//  RSBaseInstance.h
//  RSCoreFoundation
//
//  Created by RetVal on 10/14/12.
//  Copyright (c) 2012 RetVal. All rights reserved.
//
#include <pthread.h>
#include <unistd.h>
#include <RSCoreFoundation/RSAllocator.h>
#include <RSCoreFoundation/RSDictionary.h>
#include <RSCoreFoundation/RSException.h>
#include <RSCoreFoundation/RSString.h>

#ifndef _OBJC_OBJC_H_
//typedef void* id;
#endif
#ifndef RSCoreFoundation_RSRuntime_h
#define RSCoreFoundation_RSRuntime_h

#define RSInitializePriority    2000

#define __RSRuntimeDebugPreference              1

#define __RSRuntimeInstanceBZeroBeforeDie       1
#define __RSRuntimeISABaseOnEmptyField          0

#define __RSRuntimeInstanceManageWatcher        0
#define __RSRuntimeInstanceRefWatcher           0
#define __RSRuntimeInstanceAllocFreeWatcher     0
#define __RSRuntimeInstanceARC                  0
#define __RSRuntimeCheckAutoreleaseFlag         0
#define __RSRuntimeLogSave                      0

#define __RSStringNoticeWhenConstantStringAddToTable    0
#define __RSPropertyListWarningWhenParseNullKey         0
#define __RSPropertyListWarningWhenParseNullValue       0
#define __RSLogDebugLevelShouldFallThrough              0

RS_EXTERN_C_BEGIN
enum {
    RuntimeBaseInfoMask = 1 << (0L),
    RuntimeBaseResourcefMask = 1 << (2L),
    RuntimeBaseCustomerRefMask = 1 << (3L),
};//RSInfoBlock
typedef struct __RSRuntimeBaseInfo
{
#if __LP64__
    RSBitU32 _rsinfo    :8;   // reserved for runtime (at least 5 bits)
    RSBitU32 _objId     :10;  // reserved for runtime
    RSBitU32 _special   :1;   // never retain/releases
    RSBitU32 _reserved2 :1;
    RSBitU32 _rsinfo1   :8;   // class self
    RSBitU32 _reserved  :3;   // class self (at least 3 bits)
    RSBitU32 _reserved3 :1;
#else
    RSBitU32 _rc        :7;   // reference count in 32 bits.
    RSBitU32 _special   :1;   // never retain/releases
    RSBitU32 _objId     :10;  // reserved for runtime
    RSBitU32 _rsinfo    :6;   // reserved for runtime
    RSBitU32 _rsinfo1   :8;   // class self
#endif
    
}RSRuntimeBaseInfo;

typedef struct __RSRuntimeBase {
    RSPointer _rsisa;     // this field may be used for RSCreateInstance.
    RSRuntimeBaseInfo _rsinfo;
#if __LP64__
    RSBitU32 _rc;
#endif
} RSRuntimeBase;

RSInline void __RSRuntimeSetInstanceAsStackValue(RSTypeRef base)
{
    ((RSRuntimeBase*)base)->_rsinfo._rsinfo |= (1 << 1);
}

RSInline BOOL __RSRuntimeInstanceIsStackValue(RSTypeRef base)
{
    return (((RSRuntimeBase*)base)->_rsinfo._rsinfo & (1 << 1)) == 2;
}

RSInline void __RSSetObjC(RSTypeRef base)
{
    ((RSRuntimeBase*)base)->_rsinfo._rsinfo |= (1<<2);
}

RSInline BOOL __RSIsObjC(RSTypeRef base)
{
    return ((RSRuntimeBase*)base)->_rsinfo._rsinfo & (1<<2);
}

RSInline void __RSSetValid(RSTypeRef base)
{
#if !__RSRuntimeISABaseOnEmptyField
    ((RSRuntimeBase*)base)->_rsisa = (RSPointer)base;
#endif
    ((RSRuntimeBase*)base)->_rsinfo._rsinfo |= (1<<3);
}

RSInline void __RSSetInValid(RSTypeRef base)
{
    ((RSRuntimeBase*)base)->_rsinfo._rsinfo &= ~(1<<3);
}

RSInline BOOL __RSIsValid(RSTypeRef base)
{
    return YES;
    BOOL valid = (((RSRuntimeBase*)base)->_rsinfo._rsinfo & (1<<3)) == (1<<3);
    if (NO == ((RSRuntimeBase*)base)->_rsinfo._special)
        return __RSRuntimeInstanceIsStackValue(base) || (valid && ((RSIndex)base == (RSIndex)((RSRuntimeBase*)base)->_rsisa));
    return valid;
}

#if __RSRuntimeDebugPreference
RSInline void __RSSetAutorelease(RSTypeRef base)
{
    ((RSRuntimeBase*)base)->_rsinfo._rsinfo |= (1<<4);
}

RSInline void __RSSetUnAutorelease(RSTypeRef base)
{
    ((RSRuntimeBase*)base)->_rsinfo._rsinfo &= ~(1<<4);
}

RSInline BOOL __RSIsAutorelease(RSTypeRef base)
{
    return (((RSRuntimeBase*)base)->_rsinfo._rsinfo & (1<<4)) == (1 << 4);
}
#endif

RSInline void __RSAllocatorSetSystemDefault(RSTypeRef base)
{
    ((RSRuntimeBase*)base)->_rsinfo._rsinfo |= (1<<5);
}

RSInline void __RSAllocatorNotSystemDefault(RSTypeRef base)
{
    ((RSRuntimeBase*)base)->_rsinfo._rsinfo &= ~(1<<5);
}

RSInline BOOL __RSAllocatorIsSystemDefault(RSTypeRef base)
{
    return (((RSRuntimeBase*)base)->_rsinfo._rsinfo & (1<<5)) == (1 << 5);
}

RSInline void __RSRuntimeSetInstanceAsClass(RSTypeRef base) {
    ((RSRuntimeBase *)base)->_rsinfo._rsinfo |= (1 << 6);
}

RSInline void __RSRuntimeUnsetInstanceAsClass(RSTypeRef base) {
    ((RSRuntimeBase *)base)->_rsinfo._rsinfo &= ~(1 << 6);
}

RSInline BOOL __RSRuntimeInstanceIsClass(RSTypeRef base) {
    return !((uintptr_t)(base) & 0x1) && (((RSRuntimeBase *)base)->_rsinfo._rsinfo & (1 << 6)) == (1 << 6);
}

#if __LP64__
#define RSRuntimeBaseDefault(...)\
    ._base._rsisa = 0,\
    ._base._rc = 1,\
    ._base._rsinfo._reserved = 0,\
    ._base._rsinfo._special = 1,\
    ._base._rsinfo._objId = 0,\
    ._base._rsinfo._rsinfo1 = 0,\
    ._base._rsinfo._rsinfo = (1 << 1) | (1 << 3)

#else
#define RSRuntimeBaseDefault(...)\
    ._base._rsisa = 0,\
    ._base._rc = 1,\
    ._base._rsinfo._reserved = 0,\
    ._base._rsinfo._special = 1,\
    ._base._rsinfo._objId = 7,\
    ._base._rsinfo._rsinfo1 = 0,\
    ._base._rsinfo._rsinfo = (1 << 1) | (1 << 3)
#endif

#define __CONSTANT_RSSTRINGS__
//#if __BIG_ENDIAN__
//#if __LP64__
//#define RSRuntimeBaseDefault(...) {0, {0, 0, 0, 1, 0},0}
//#else
//
//#endif
//#else
//#if __LP64__
//#define RSRuntimeBaseDefault(...) {0, {0, 0, 0, 1, 0},0}
//#else
//#endif
//#endif



typedef struct __RSRuntimeClassVersion
{
    RSPointer _rsinfo;
#if __LP64__
    RSIndex info       :8;   // reserved for runtime (at least 7 bits)
    RSIndex Id         :10;  // reserved for runtime
    RSIndex _special   :1;   // never retain/releases
    RSIndex _reserved2 :1;
    RSIndex _rsinfo1   :8;   // class self
    RSIndex reftype    :4;   // class self (at least 3 bits)
    RSIndex reserved : sizeof(RSIndex)*8 - 32;
#else
    RSIndex _rc        :7;   // reference count in 32 bits.
    RSIndex _special   :1;   // never retain/releases
    RSIndex Id         :10;  // reserved for runtime
    RSIndex info       :7;   // reserved for runtime
    RSIndex reserved   :3;
    RSIndex reftype    :4;   // class self
    RSIndex reserved : sizeof(RSIndex)*8 - 32;
#endif
}RSRuntimeClassVersion;

typedef void (*RSRuntimeClassInit)(RSTypeRef obj);
typedef RSTypeRef (*RSRuntimeClassCopy)(RSAllocatorRef allocator, RSTypeRef obj, BOOL mutableCopy);
typedef void (*RSRuntimeClassDeallocate)(RSTypeRef obj);
typedef BOOL (*RSRuntimeClassEqual)(RSTypeRef obj1, RSTypeRef obj2);
typedef RSStringRef (*RSRuntimeClassDescription)(RSTypeRef obj);
typedef RSHashCode (*RSRuntimeClassHash)(RSTypeRef obj);
typedef void (*RSRuntimeClassReclaim)(RSTypeRef obj);
typedef RSUInteger (*RSRuntimeClassRefcount)(intptr_t op, RSTypeRef obj);
typedef struct __RSRuntimeClass
{
    RSIndex version;                //RSRuntimeClassVersion version;
    RSIndex reserved;
    const char *className;          // must be a pure ASCII string, nul-terminated
    RSRuntimeClassInit init;           // to support RSCreateInstance, can not be nil
    RSRuntimeClassCopy copy;           // may be nil
    RSRuntimeClassDeallocate deallocate;   // when the reference count is falled to 0, this method will be called.
    RSRuntimeClassEqual equal;
    RSRuntimeClassHash hash;
    
    RSRuntimeClassDescription description;	// return str with retain
    //RSStringRef (*copyDebugDesc)(RSTypeRef obj);	// return str with retain
    
#define RS_RECLAIM_AVAILABLE 1
     // Set _RSRuntimeResourcefulInstance in the .version to indicate this field should be used
    RSRuntimeClassReclaim reclaim;
#define RS_REFCOUNT_AVAILABLE 1
    // Set _RSRuntimeCustomRefCount in the .version to indicate this field should be used
    // this field must be non-NULL when _RSRuntimeCustomRefCount is in the .version field
    // - if the callback is passed 1 in 'op' it should increment the 'obj's reference count and return 0
    // - if the callback is passed 0 in 'op' it should return the 'obj's reference count, up to 32 bits
    // - if the callback is passed -1 in 'op' it should decrement the 'obj's reference count; if it is now zero, 'obj' should be cleaned up and deallocated (the deallocate callback above will NOT be called unless the process is running under GC, and RS does not deallocate the memory for you; if running under GC, deallocate should do the object tear-down and free the object memory); then return 0
    // remember to use saturation arithmetic logic and stop incrementing and decrementing when the ref count hits UINT32_MAX, or you will have a security bug
    // remember that reference count incrementing/decrementing must be done thread-safely/atomically
    // objects should be created/initialized with a custom ref-count of 1 by the class creation functions
    // do not attempt to use any bits within the RSRuntimeBase for your reference count; store that in some additional field in your RS object
    RSRuntimeClassRefcount refcount;
    
} RSRuntimeClass;

enum { // Version field constants
    _RSRuntimeScannedObject =     (1UL << 0),
    _RSRuntimeBasedObject =       (1UL << 1),  // tells RSRuntime to make use of the base field
    _RSRuntimeResourcefulObject = (1UL << 2),  // tells RSRuntime to make use of the reclaim field
    _RSRuntimeCustomRefCount =    (1UL << 3),  // tells RSRuntime to make use of the refcount field
};

enum {
    _RSRuntimeNotAType = 0,
    _RSRuntimeNotATypeID = 0,
};

enum {
    _RSRuntimeOperationRetain = 350,
    _RSRuntimeOperationRelease = 450,
    _RSRuntimeOperationRetainCount = 500
};
/****************************************************************************************************************/
RSExport const RSHashCode RSIndexMax;
RSExport RSSpinLock __RSRuntimeClassTableSpinLock;
RSInline void __RSRuntimeLockTable(){RSSpinLockLock(&__RSRuntimeClassTableSpinLock);}
RSInline void __RSRuntimeUnlockTable(){RSSpinLockUnlock(&__RSRuntimeClassTableSpinLock);}
RSInline bool __RSRuntimeTryLockTable(){return RSSpinLockTry(&__RSRuntimeClassTableSpinLock);}

RSInline RSRuntimeClassVersion __RSRuntimeGetClassVersionInformation(const RSRuntimeClass* cls) {RSRuntimeClassVersion v = *(RSRuntimeClassVersion*)&(cls->version); return v;}
RSInline void __RSRuntimeSetClassVersionInformation(RSRuntimeClass* cls, RSRuntimeClassVersion version){ memcpy(&cls->version, &version, sizeof(version));}

RSExport int       __RSRuntimeVersionRetainConstomer(const RSRuntimeClass* const cls) RS_AVAILABLE(0_0);
RSExport void      __RSRuntimeSetClassTypeID(const RSRuntimeClass* const cls, RSTypeID id) RS_AVAILABLE(0_0);
RSExport RSTypeID  __RSRuntimeGetClassTypeID(const RSRuntimeClass* const cls) RS_AVAILABLE(0_0);
RSExport void      __RSRuntimeSetInstanceTypeID(RSTypeRef obj, RSTypeID id) RS_AVAILABLE(0_0);

RSExport RSTypeID  __RSRuntimeRegisterClass(const RSRuntimeClass* const cls) RS_AVAILABLE(0_0);
RSExport void      __RSRuntimeUnregisterClass(RSTypeID classID) RS_AVAILABLE(0_0);

RSExport const RSRuntimeClass* __RSRuntimeGetClassWithTypeID(RSTypeID classID) RS_AVAILABLE(0_0);
RSExport const char* __RSRuntimeGetClassNameWithTypeID(RSTypeID classID) RS_AVAILABLE(0_0);
RSExport const char* __RSRuntimeGetClassNameWithInstance(RSTypeRef obj) RS_AVAILABLE(0_0);
RSExport RSTypeID __RSRuntimeGetClassTypeIDWithName(const char * name) RS_AVAILABLE(0_4);
RSExport BOOL __RSRuntimeScanClass(const RSRuntimeClass* cls) RS_AVAILABLE(0_0);

/****************************************************************************************************************/

RSExport RSTypeRef __RSRuntimeCreateInstance(RSAllocatorRef allocator, RSTypeID typeID, RSIndex size) RS_AVAILABLE(0_0);
extern void __RSRelease(RSTypeRef obj) RS_AVAILABLE(0_0);
extern RSTypeRef __RSAutorelease(RSTypeRef obj)  RS_AVAILABLE(0_1);
extern const char *__RSGetEnvironment(RSCBuffer n) RS_AVAILABLE(0_4);
extern const char *__RSRuntimeGetEnvironment(const char *n) RS_AVAILABLE(0_1);
extern BOOL __RSRuntimeSetEnvironment(RSCBuffer name, RSCBuffer value) RS_AVAILABLE(0_1);
/* all __RSRuntime functions below are depend on the object itself. */
RSExport RSTypeRef __RSRuntimeInstanceRetain(RSTypeRef obj, BOOL flag)  RS_AVAILABLE(0_0);
RSExport void      __RSRuntimeInstanceRelease(RSTypeRef obj, BOOL flag)  RS_AVAILABLE(0_0);
RSExport RSIndex   __RSRuntimeInstanceGetRetainCount(RSTypeRef obj)  RS_AVAILABLE(0_0);
RSExport void      __RSRuntimeInstanceDeallocate(RSTypeRef obj)  RS_AVAILABLE(0_0);
/* end object itself*/

/* all __RSRuntime funcations below are depend on the real runtime system. */
RSExport RSTypeRef __RSRuntimeRetain(RSTypeRef obj, BOOL flag)  RS_AVAILABLE(0_0);
RSExport void      __RSRuntimeRelease(RSTypeRef obj, BOOL flag)  RS_AVAILABLE(0_0);
RSExport RSIndex   __RSRuntimeGetRetainCount(RSTypeRef obj)  RS_AVAILABLE(0_0);
RSExport void      __RSRuntimeDeallocate(RSTypeRef obj)  RS_AVAILABLE(0_0);

RSExport ISA __RSRuntimeRSObject(RSTypeRef obj)  RS_AVAILABLE(0_0);
RSExport void __RSRuntimeSetInstanceSpecial(RSTypeRef obj, BOOL special) RS_AVAILABLE(0_0);
RSExport BOOL __RSRuntimeInstanceIsSpecial(RSTypeRef rs) RS_AVAILABLE(0_3);
extern void RSDeallocateInstance(RSTypeRef obj)  RS_AVAILABLE(0_0);

RSExport ISA       __RSRuntimeRetainAutorelease(RSTypeRef obj) RS_AVAILABLE(0_2);
#ifndef likely
#define  likely(x)        __builtin_expect(!!(x), 1)
#endif

#ifndef unlikely
#define  unlikely(x)      __builtin_expect(!!(x), 0)
#endif

#ifndef __RSGenericValidInstance
#define __RSGenericValidInstance(obj, typeID) __RSRuntimeCheckInstanceType(obj, typeID, __PRETTY_FUNCTION__)
#endif

#ifndef RSRuntimeClassBaseFiled
#define RSRuntimeClassBaseFiled(rs) (((RSRuntimeBase*)(rs))->_rsinfo._rsinfo1)
#endif

#ifndef __RSBitfieldMask
#define __RSBitfieldMask(N1, N2)	((((RSBitU32)~0UL) << (31UL - (N1) + (N2))) >> (31UL - N1))
#endif

#ifndef __RSBitfieldGetValue
#define __RSBitfieldGetValue(V, N1, N2)	(((V) & __RSBitfieldMask(N1, N2)) >> (N2))
#endif

#ifndef __RSBitfieldSetValue
#define __RSBitfieldSetValue(V, N1, N2, X)	((V) = ((V) & ~__RSBitfieldMask(N1, N2)) | (((X) << (N2)) & __RSBitfieldMask(N1, N2)))
#endif

#ifndef __RSBitIsSet
#define __RSBitIsSet(V, N)  (((V) & (1UL << (N))) != 0)
#endif

#ifndef __RSBitSet
#define __RSBitSet(V, N)  ((V) |= (1UL << (N)))
#endif

#ifndef __RSBitClear
#define __RSBitClear(V, N)  ((V) &= ~(1UL << (N)))
#endif

/* end runtime system*/
/****************************************************************************************************************/
#include <RSCoreFoundation/RSArray.h>
extern RSAllocatorRef RSAllocatorSystemDefault;
extern void RSArrayDebugLogSelf(RSArrayRef array);
extern uintptr_t __RSDoExternRefOperation(uintptr_t op, void* obj);
/****************************************************************************************************************/
typedef RSComparisonResult (*RSKeyCompare)(RSTypeRef s1, RSTypeRef s2);
/****************************************************************************************************************/
#define RSRuntimePriority       1031
extern ISA __RSAutoReleaseISA(RSAllocatorRef deallocator, ISA isa);
//extern RSStringRef  __RSStringCreateWithFormat(RSAllocatorRef allocator, RSIndex size, RSStringRef format, ...)  RS_AVAILABLE(0_0);
RSExport void __RSTraceLog(RSCBuffer,...)  RS_AVAILABLE(0_0);
RSExport void __RSLog(RSIndex, RSStringRef, ...) RS_AVAILABLE(0_3);
RSExport void __RSLogArgs(RSIndex, RSStringRef, va_list) RS_AVAILABLE(0_3);
RSExport void __RSLogShowWarning(RSTypeRef) RS_AVAILABLE(0_3);
RSExport void RSCoreFoundationInitialize(void) RS_INIT_ROUTINE;//__RS_INIT_ROUTINE(RSRuntimePriority);
enum {
    RSLogLevelEmergency = 0,
    RSLogLevelAlert = 1,
    RSLogLevelCritical = 2,
    RSLogLevelError = 3,
    RSLogLevelWarning = 4,
    RSLogLevelNotice = 5,
    RSLogLevelInfo = 6,
    RSLogLevelDebug = 7,
};
void __RSCPrint(int fd, RSCBuffer cStr);
void __RSCLog(RSIndex logLevel,RSCBuffer format,...) __attribute__((format(printf,2,3)));
void __HALT(void) __attribute__((noreturn));

typedef RS_ENUM(RSIndex, RSSystemVersion) {
    RSSystemVersionCheetah = 0,         /* 10.0 */
    RSSystemVersionPuma = 1,            /* 10.1 */
    RSSystemVersionJaguar = 2,          /* 10.2 */
    RSSystemVersionPanther = 3,         /* 10.3 */
    RSSystemVersionTiger = 4,           /* 10.4 */
    RSSystemVersionLeopard = 5,         /* 10.5 */
    RSSystemVersionSnowLeopard = 6,	/* 10.6 */
    RSSystemVersionLion = 7,		/* 10.7 */
    RSSystemVersionMountainLion = 8,    /* 10.8 */
    RSSystemVersionMaverick = 9,    /* 10.9 */
    RSSystemVersionMax,                 /* This should bump up when new entries are added */
};

#ifdef DEBUG
#define __RSRuntimeShowErrorMessage
#endif

#ifdef __RSRuntimeShowErrorMessage
#define HALTWithError(exception, error) do { RSExceptionCreateAndRaise(RSAllocatorSystemDefault, exception ?: RSGenericException, RSSTR(#error), nil); __HALT();} while (0)
#else
#define HALTWithError(exception, error) __HALT()
#endif
RSInline void __RSRuntimeCheckInstanceAvailable(RSTypeRef obj)
{
    if ((uintptr_t)(obj) & 0x1) return;
    if (unlikely(obj == nil) || NO == __RSIsValid(obj))
        __RSCLog(RSLogLevelCritical, "instance is not available %p", obj);
}
RSExport void __RSRuntimeCheckInstanceType(RSTypeRef obj, RSTypeID ID, const char* function);
    
    
#define __RSLogAssertion    RSLogLevelAlert
#if defined(DEBUG)
#define __RSAssert(cond, prio, desc, a1, a2, a3, a4, a5)	\
do {			\
if (!(cond)) {	\
RSLog(RSSTR(desc), a1, a2, a3, a4, a5); \
/* HALT; */		\
}			\
} while (0)
#else
#define __RSAssert(cond, prio, desc, a1, a2, a3, a4, a5)	\
do {} while (0)
#endif
    
#define RSAssert(condition, priority, description)			\
__RSAssert((condition), (priority), description, 0, 0, 0, 0, 0)
#define RSAssert1(condition, priority, description, a1)			\
__RSAssert((condition), (priority), description, (a1), 0, 0, 0, 0)
#define RSAssert2(condition, priority, description, a1, a2)		\
__RSAssert((condition), (priority), description, (a1), (a2), 0, 0, 0)
#define RSAssert3(condition, priority, description, a1, a2, a3)		\
__RSAssert((condition), (priority), description, (a1), (a2), (a3), 0, 0)
#define RSAssert4(condition, priority, description, a1, a2, a3, a4)	\
__RSAssert((condition), (priority), description, (a1), (a2), (a3), (a4), 0)

RS_EXTERN_C_END
#endif
