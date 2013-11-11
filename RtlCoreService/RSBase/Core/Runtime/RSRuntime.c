//
//  RSRuntime.c
//  RSCoreFoundation
//
//  Created by RetVal on 10/14/12.
//  Copyright (c) 2012 RetVal. All rights reserved.
//

#include <RSCoreFoundation/RSBase.h>
#include <RSCoreFoundation/RSBasicHash.h>
#include <RSCoreFoundation/RSBaseHash.h>
#include <RSCoreFoundation/RSBaseMacro.h>
#include <RSCoreFoundation/RSBaseType.h>
#include <RSCoreFoundation/RSDate.h>
#include <RSCoreFoundation/RSException.h>
#include <RSCoreFoundation/RSNil.h>
#include <RSCoreFoundation/RSTimeZone.h>
#include <RSCoreFoundation/RSRuntime.h>
#include "RSRuntimeAtomic.h"
#include "../BaseStructure/RSPrivate/RSPrivateOperatingSystem/RSPrivateTask.h"

// define the runtime class table   (10 bits)
#define  __RSClassTableSize 1024
RSPrivate volatile RSSpinLock  __RSRuntimeClassTableSpinLock = RSSpinLockInit;
static RSRuntimeClass* __RSRuntimeClassTable[__RSClassTableSize];
static RSBit32 __RSRuntimeClassTableCount = 0;
static BOOL __RSRuntimeInitiazing = YES;

#define __RSRuntimeClassMaxCount 65535
#define __RSRuntimeClassReservedRangeEnd 256
// end define the runtime class table

// define the error base class
static RSRuntimeClass __RSRuntimeErrorClass =
{
    _RSRuntimeScannedObject,
    "Runtime Error Class",
    (void*) __HALT,
    (void*) __HALT,
    (void*) __HALT,
    (void*) __HALT,
    (void*) __HALT,
    (void*) __HALT
};
static RSTypeID __RSRuntimeErrorClassID = _RSRuntimeNotATypeID;

RSPrivate void __RSRuntimeMemoryBarrier()
{
#if DEPLOYMENT_TARGET_MACOSX
    OSMemoryBarrier();
#elif DEPLOYMENT_TARGET_WINDOWS
    
#elif DEPLOYMENT_TARGET_LINUX
    
#endif
}
static struct {
    RSCBuffer name;
    RSCBuffer value;
} __RSEnv[] = {
    {"PATH", NULL},
    {"USER", NULL},
    {"HOME", NULL},
    {"HOMEDRIVE", NULL},
    {"USERNAME", NULL},
    {"TZFILE", NULL},
    {"TZ", NULL},
    {"NEXT_ROOT", NULL},
    {"DYLD_IMAGE_SUFFIX", NULL},
    {"CFProcessPath", NULL},
    {"CFNETWORK_LIBRARY_PATH", NULL},
    {"CFUUIDVersionNumber", NULL},
    {"CFDebugNamedDataSharing", NULL},
    {"CFPropertyListAllowImmutableCollections", NULL},
    {"CFBundleUseDYLD", NULL},
    {"CFBundleDisableStringsSharing", NULL},
    {"CFCharacterSetCheckForExpandedSet", NULL},
    {"__CF_DEBUG_EXPANDED_SET", NULL},
    {"CFStringDisableROM", NULL},
    {"CF_CHARSET_PATH", NULL},
    {"__CF_USER_TEXT_ENCODING", NULL},
    {"__CFPREFERENCES_AUTOSYNC_INTERVAL", NULL},
    {"__CFPREFERENCES_LOG_FAILURES", NULL},
    {"CFNumberDisableCache", NULL},
    {"__CFPREFERENCES_AVOID_DAEMON", NULL},
    {"RSCFRunMode", NULL},
    {"TMPDIR", NULL},
    {NULL, NULL}
    // the last one is for optional "COMMAND_MODE" "legacy", do not use this slot, insert before
};

RSPrivate const char *__RSGetEnvironment(RSCBuffer n)
{
    return getenv(n);
}

RSPrivate const char *__RSRuntimeGetEnvironment(RSCBuffer n)
{
    RSIndex idx = 0;
    RSCBuffer value = nil;
    for (; idx < sizeof(__RSEnv) / sizeof(__RSEnv[0]); idx++)
    {
        if (__RSEnv[idx].name && 0 == strcmp(n, __RSEnv[idx].name))
        {
            value = __RSEnv[idx].value;
            break;
        }
    }
    __RSEnv[idx].value = value ?: __RSGetEnvironment(n);
    return __RSEnv[idx].value;
}

RSPrivate BOOL __RSRuntimeSetEnvironment(RSCBuffer name, RSCBuffer value)
{
    int error = setenv(name, value, YES);
    if (error != 0)
        return NO;
    return YES;
}

struct ____RSDebugLogContext
{
    RSSpinLock _debugLogLock;
    int _handle;
	RSBlock _logPath[RSMaxPathSize];
};

#include <sys/fcntl.h>
#include <sys/stat.h>
static struct ____RSDebugLogContext ____RSRuntimeDebugLevelLogContext = {0};
static void __RSDebugLevelInitialize()
{
    RSBlock _debuglogPrefixPath[RSMaxPathSize] = {0};
    ____RSRuntimeDebugLevelLogContext._debugLogLock = RSSpinLockInit;
	strcpy(_debuglogPrefixPath, __RSRuntimeGetEnvironment("HOME"));
#if DEPLOYMENT_TARGET_MACOSX || DEPLOYMENT_TARGET_LINUX
    strcat(_debuglogPrefixPath, "/Library/Logs/RSCoreFoundation");
	mkdir(_debuglogPrefixPath, 0777);
    sprintf(____RSRuntimeDebugLevelLogContext._logPath, "%s/com.retval.RSCoreFoundation.runtime.runid%05d.log", _debuglogPrefixPath, getpid());
    ____RSRuntimeDebugLevelLogContext._handle = open(____RSRuntimeDebugLevelLogContext._logPath, O_RDWR | O_CREAT, 0555);
#elif DEPLOYMENT_TARGET_WINDOWS
    ____RSRuntimeDebugLevelLogContext._handle = open(____RSRuntimeDebugLevelLogContext._logPath, O_RDWR | O_CREAT, 0555);
#endif
}

static void __RSDebugLevelDeallocate()
{
	unsigned long long org = lseek(____RSRuntimeDebugLevelLogContext._handle, 0, SEEK_CUR);
    lseek(____RSRuntimeDebugLevelLogContext._handle, 0, SEEK_END);
    unsigned long long size = lseek(____RSRuntimeDebugLevelLogContext._handle, 0, SEEK_CUR); // get current file pointer
    lseek(____RSRuntimeDebugLevelLogContext._handle, org, SEEK_SET);
    if (size < 1 )
	{
		close(____RSRuntimeDebugLevelLogContext._handle);
		unlink(____RSRuntimeDebugLevelLogContext._logPath);
		____RSRuntimeDebugLevelLogContext._handle = 0;
		__builtin_memset(____RSRuntimeDebugLevelLogContext._logPath, 0, 2*RSMaxPathSize);
	}
}

static void ___RSDebugLevelLogWrite(RSCBuffer tolog, RSIndex length)
{
    RSSpinLockLock(&____RSRuntimeDebugLevelLogContext._debugLogLock);
    write(____RSRuntimeDebugLevelLogContext._handle, tolog, length);
    RSSpinLockUnlock(&____RSRuntimeDebugLevelLogContext._debugLogLock);
}

RSExport void __RSDebugLevelLog(RSCBuffer tolog, RSIndex length)
{
    ___RSDebugLevelLogWrite(tolog, length);
}

static void __RSRuntimeEnvironmentInitialize()
{
    for (RSIndex idx = 0; idx < sizeof(__RSEnv) / sizeof(__RSEnv[0]); idx++)
    {
        __RSEnv[idx].value = __RSEnv[idx].name ? getenv(__RSEnv[idx].name) : NULL;
    }
}

const RSHashCode RSIndexMax = ~0;
RSExport RSTypeID __RSRuntimeRegisterClass(const RSRuntimeClass* restrict const cls)
{
    if (_RSRuntimeScannedObject != cls->version) {
        return _RSRuntimeNotATypeID;
    }
    __RSRuntimeLockTable();
    if (__RSRuntimeClassMaxCount <= __RSRuntimeClassTableCount) {
        __RSRuntimeUnlockTable();
        return _RSRuntimeNotATypeID;
    }
    if (__RSClassTableSize <= __RSRuntimeClassTableCount) {
        __RSRuntimeUnlockTable();
        return _RSRuntimeNotATypeID;
    }
    __RSRuntimeScanClass(cls);
    __RSRuntimeClassTable[__RSRuntimeClassTableCount] = (RSRuntimeClass*)cls;
    RSTypeID type = __RSRuntimeClassTableCount++;
    __RSRuntimeUnlockTable();
    return type;
}

RSExport void __RSRuntimeUnregisterClass(RSTypeID classID)
{
    __RSRuntimeLockTable();
    __RSRuntimeClassTable[classID] = NULL;
    __RSRuntimeUnlockTable();
}

RSExport const RSRuntimeClass* __RSRuntimeGetClassWithTypeID(RSTypeID classID)
{
    if (classID >= __RSRuntimeClassTableCount) HALTWithError(RSInvalidArgumentException, "the size of class table in runtime is overflow");
    if (__RSRuntimeClassTable[classID] == nil) HALTWithError(RSInvalidArgumentException, "the class is not registered in runtime");
    return (const RSRuntimeClass*)__RSRuntimeClassTable[classID];
}

RSExport const char* __RSRuntimeGetClassNameWithTypeID(RSTypeID classID)
{
    return __RSRuntimeGetClassWithTypeID(classID)->className;
}

RSExport const char* __RSRuntimeGetClassNameWithInstance(RSTypeRef obj)
{
    return __RSRuntimeGetClassNameWithTypeID(RSGetTypeID(obj));
}

RSExport BOOL  __RSRuntimeScanClass(const RSRuntimeClass* cls)
{
    RSRuntimeClass* __cls = (RSRuntimeClass*)cls;
    if (__cls) {
        RSRuntimeClassVersion version = __RSRuntimeGetClassVersionInformation(cls);
        if (version.reftype == _RSRuntimeScannedObject) {
            if (cls->refcount) {
                version.reftype = _RSRuntimeCustomRefCount;
                __RSRuntimeSetClassVersionInformation(__cls, version);
                return YES;
            }
            version.reftype = _RSRuntimeBasedObject;
            __RSRuntimeSetClassVersionInformation(__cls, version);
            return YES;
        }
    }
    return NO;
}

/***********************  Runtime for base private API  **********************************************************/
RSExport RSTypeRef __RSRuntimeCreateInstance(RSAllocatorRef allocator, RSTypeID typeID, RSIndex size)
{
//    if (size > 20480000 - sizeof(struct __RSRuntimeBase)) HALTWithError(RSInvalidArgumentException, "the instance size is too huge!");
    RSRuntimeClass* cls = (RSRuntimeClass*)__RSRuntimeGetClassWithTypeID(typeID);
    RSTypeRef obj = nil;
    if (cls)
    {
        if (allocator == nil) allocator = RSAllocatorSystemDefault;
        if ((obj = RSAllocatorAllocate(allocator, size + sizeof(struct __RSRuntimeBase))))
        {
            __RSSetValid(obj);
            __RSRuntimeSetInstanceTypeID(obj, typeID);
            RSRetain(obj);
            if (cls->init)
            {
                RSRuntimeClassInit init = cls->init;
                init(obj);
            }
            
#if __RSRuntimeInstanceManageWatcher
            __RSCLog(RSLogLevelDebug, "%s alloc - <%p>\n", cls->className, obj);
#endif
            if (cls->refcount) ((RSRuntimeBase*)obj)->_rsinfo._customRef = 1;
            else ((RSRuntimeBase*)obj)->_rsinfo._customRef = 0;
        }
    }
#if defined(RSAutoMemoryLog)
    RSMmLogUnitNumber();
#endif
    return obj ?:((void*)RSNil);
}

#if DEPLOYMENT_TARGET_MACOSX
#define NUM_EXTERN_TABLES 8
#define EXTERN_TABLE_IDX(O) (((uintptr_t)(O) >> 8) & 0x7)
#elif DEPLOYMENT_TARGET_EMBEDDED || DEPLOYMENT_TARGET_EMBEDDED_MINI || DEPLOYMENT_TARGET_WINDOWS || DEPLOYMENT_TARGET_LINUX
#define NUM_EXTERN_TABLES 1
#define EXTERN_TABLE_IDX(O) 0
#else
#error
#endif

// we disguise pointers so that programs like 'leaks' forget about these references
#define DISGUISE(O) (~(uintptr_t)(O))

static struct {
    RSSpinLock lock;
    RSBasicHashRef table;
    uint8_t padding[64 - sizeof(RSBasicHashRef) - sizeof(RSSpinLock)];
} __RSRetainCounters[NUM_EXTERN_TABLES];
//#include <objc/runtime.h>
RSExport uintptr_t __RSDoExternRefOperation(uintptr_t op, void* obj)
{
    if (nil == obj) HALTWithError(RSInvalidArgumentException, "the object is nil");
    uintptr_t idx = EXTERN_TABLE_IDX(obj);
    uintptr_t disguised = DISGUISE(obj);
    RSSpinLock *lock = &__RSRetainCounters[idx].lock;
    RSBasicHashRef table = __RSRetainCounters[idx].table;
    uintptr_t count;
    switch (op)
    {
        case 300:   // increment
            //if (__CFOASafe) __CFRecordAllocationEvent(__kCFObjectRetainedEvent, obj, 0, 0, NULL);
        case _RSRuntimeOperationRetain:   // increment, no event
            RSSpinLockLock(lock);
            RSBasicHashAddValue(table, disguised, disguised);
            RSSpinLockUnlock(lock);
            return (uintptr_t)obj;
        case 400:   // decrement
            //if (__CFOASafe) __CFRecordAllocationEvent(__kCFObjectReleasedEvent, obj, 0, 0, NULL);
        case _RSRuntimeOperationRelease:   // decrement, no event
            RSSpinLockLock(lock);
            count = (uintptr_t)RSBasicHashRemoveValue(table, disguised);
            RSSpinLockUnlock(lock);
            return 0 == count;
        case _RSRuntimeOperationRetainCount:
            RSSpinLockLock(lock);
            count = (uintptr_t)RSBasicHashGetCountOfKey(table, disguised);
            RSSpinLockUnlock(lock);
            return count;
    }
    return 0;
}

#if !defined(RSBasicHashVersion) || (RSBasicHashVersion == 1)
static void __RSExternRefNullFreeCallbacks(RSConstBasicHashRef ht, RSAllocatorRef allocator, RSBasicHashCallbacks *cb) {
}

static uintptr_t __RSExternRefNullRetainValue(RSConstBasicHashRef ht, uintptr_t stack_value) {
    return stack_value;
}

static uintptr_t __RSExternRefNullRetainKey(RSConstBasicHashRef ht, uintptr_t stack_key) {
    return stack_key;
}

static void __RSExternRefNullReleaseValue(RSConstBasicHashRef ht, uintptr_t stack_value) {
}

static void __RSExternRefNullReleaseKey(RSConstBasicHashRef ht, uintptr_t stack_key) {
}

static BOOL __RSExternRefNullEquateValues(RSConstBasicHashRef ht, uintptr_t coll_value1, uintptr_t stack_value2)
{
    return coll_value1 == stack_value2;
}

static BOOL __RSExternRefNullEquateKeys(RSConstBasicHashRef ht, uintptr_t coll_key1, uintptr_t stack_key2)
{
    return coll_key1 == stack_key2;
}

static uintptr_t __RSExternRefNullHashKey(RSConstBasicHashRef ht, uintptr_t stack_key)
{
    return stack_key;
}

static uintptr_t __RSExternRefNullGetIndirectKey(RSConstBasicHashRef ht, uintptr_t coll_value)
{
    return 0;
}

static RSStringRef __RSExternRefNullCopyValueDescription(RSConstBasicHashRef ht, uintptr_t stack_value)
{
    return RSStringCreateWithFormat(RSAllocatorSystemDefault, RSSTR("<%p>"), (void *)stack_value);
}

static RSStringRef __RSExternRefNullCopyKeyDescription(RSConstBasicHashRef ht, uintptr_t stack_key)
{
    return RSStringCreateWithFormat(RSAllocatorSystemDefault, RSSTR("<%p>"), (void *)stack_key);
}

static RSBasicHashCallbacks *__RSExternRefNullCopyCallbacks(RSConstBasicHashRef ht, RSAllocatorRef allocator, RSBasicHashCallbacks *cb);

static const RSBasicHashCallbacks RSExternRefCallbacks =
{
    __RSExternRefNullCopyCallbacks,
    __RSExternRefNullFreeCallbacks,
    __RSExternRefNullRetainValue,
    __RSExternRefNullRetainKey,
    __RSExternRefNullReleaseValue,
    __RSExternRefNullReleaseKey,
    __RSExternRefNullEquateValues,
    __RSExternRefNullEquateKeys,
    __RSExternRefNullHashKey,
    __RSExternRefNullGetIndirectKey,
    __RSExternRefNullCopyValueDescription,
    __RSExternRefNullCopyKeyDescription
};

static RSBasicHashCallbacks *__RSExternRefNullCopyCallbacks(RSConstBasicHashRef ht, RSAllocatorRef allocator, RSBasicHashCallbacks *cb)
{
    return (RSBasicHashCallbacks *)&RSExternRefCallbacks;
}
#elif (RSBasicHashVersion == 2)
static RSBasicHashCallbacks RSExternRefCallbacks = {NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL};

#endif

RSExport RSTypeRef  __RSRuntimeInstanceRetain(RSTypeRef obj, BOOL try)
{
    RSTypeID typeID = RSGetTypeID(obj);
    if (typeID != _RSRuntimeNotATypeID) {
        if (try) return nil;
        const RSRuntimeClass*class = __RSRuntimeGetClassWithTypeID(typeID);
        if (__RSRuntimeVersionRetainConstomer(class) == YES)
        {
            RSRuntimeClassRefcount ref = class->refcount;
            RSUInteger refCount = ref(1,obj);
            if (refCount == RSIndexMax) {
                // = = fuck overflow.
                HALTWithError(RSGenericException, "retain count is overflow");
            }
        }
        else
        {
            HALTWithError(RSInvalidArgumentException, "the class should be supported customer reference function.");
        }
    }
    return obj;
}

RSExport void  __RSRuntimeInstanceRelease(RSTypeRef obj, BOOL flag)
{
    RSTypeID typeID = RSGetTypeID(obj);
    if (typeID != _RSRuntimeNotATypeID) {
        if (flag) return;   // just try;
        if (((RSRuntimeBase*)obj)->_rsinfo._special) return;
        const RSRuntimeClass* class = __RSRuntimeGetClassWithTypeID(typeID);
        if (__RSRuntimeVersionRetainConstomer(class) == YES) { // check the class is support customer refcount
            RSRuntimeClassRefcount ref = class->refcount;
            RSUInteger refCount = ref(-1,obj);
            if (refCount == 0) {
                // call deallocate
                RSDeallocateInstance(obj);
            }
        }
    }
    return ;
}

RSExport RSIndex __RSRuntimeInstanceGetRetainCount(RSTypeRef obj)
{
    RSTypeID typeID = RSGetTypeID(obj);
    if (typeID != _RSRuntimeNotATypeID) {
        RSUInteger refCount = ~0;
        const RSRuntimeClass* class = __RSRuntimeGetClassWithTypeID(typeID);
        if (__RSRuntimeVersionRetainConstomer(class) == YES) {
            RSRuntimeClassRefcount ref = class->refcount;
            refCount = ref(0,obj);
            return refCount;
        }
    }
    return 0;
}

RSExport void __RSRuntimeInstanceDeallocate(RSTypeRef obj)
{
#if defined(RSAutoMemoryLog)
    RSMmLogUnitNumber();
#endif
    RSRuntimeClass* cls = (RSRuntimeClass*)__RSRuntimeGetClassWithTypeID(RSGetTypeID(obj));
    return cls->deallocate(obj);
}

RSExport RSTypeRef __RSRuntimeRetain(RSTypeRef obj, BOOL try)
{
    RSTypeID typeID = RSGetTypeID(obj);
    if (typeID != _RSRuntimeNotATypeID)
    {
        //RSUInteger refCount = ~0;
        if (try) return obj;
        RSIndex ref = 0;
        RSIndex before = 0;
#if __LP64__
        if (((RSRuntimeBase*)obj)->_rsinfo._special)
            return obj;
        do
        {
            before = ref = ((RSRuntimeBase*)obj)->_rc;
        } while (before == OSAtomicIncrement64Barrier((int64_t*)&((RSRuntimeBase *)obj)->_rc));
//#else
//        if (((RSRuntimeBase*)obj)->_rsinfo._special) return obj;
//        do {
//            ref = ((RSRuntimeBase*)obj)->_rsnfo._rc;
//        } while (!RSRuntimeAtomicAdd(ref, ref+1, (int32_t*)&(((RSRuntimeBase*)obj)->_rsnfo._rc));
#endif
    }
    else 
        HALTWithError(RSInvalidArgumentException, "the instance is not available");
    return obj;
}

RSExport void __RSRuntimeRelease(RSTypeRef obj, BOOL try)
{
    RSTypeID typeID = RSGetTypeID(obj);
    if (typeID != _RSRuntimeNotATypeID)
    {
        //RSUInteger refCount = ~0;
        if (try) return;
        RSIndex ref = 0;
        RSIndex before = 0;
#if __LP64__
        if (((RSRuntimeBase*)obj)->_rsinfo._special) return;
        ref = ((RSRuntimeBase*)obj)->_rc;
        if (ref == 1) {
            //OSAtomicDecrement64Barrier((int64_t*)&((RSRuntimeBase *)obj)->_rc);
            return RSDeallocateInstance(obj);
        }
        do {
            before = ref = ((RSRuntimeBase*)obj)->_rc;
        } while (before == OSAtomicDecrement64Barrier((int64_t*)&((RSRuntimeBase *)obj)->_rc));
//#else
//        if (((RSRuntimeBase*)obj)->_rsinfo._special) return;
//        ref = ((RSRuntimeBase*)obj)->_rsnfo._rc;
//        if (ref == 1) return RSDeallocateInstance(obj);
//        do {
//            ref = ((RSRuntimeBase*)obj)->_rsnfo._rc;
//        } while (!RSRuntimeAtomicAdd(ref, ref-1, (int32_t*)&(((RSRuntimeBase*)obj)->_rsinfo._rc));
#endif
    }
    else
        HALTWithError(RSInvalidArgumentException, "the instance is not available");
    return;
}

RSExport RSIndex __RSRuntimeGetRetainCount(RSTypeRef obj)
{
    RSTypeID typeID = RSGetTypeID(obj);
    if (typeID != _RSRuntimeNotATypeID) {
        //RSUInteger refCount = ~0;
        RSIndex ref = 0;
#if __LP64__
        
        ref = ((RSRuntimeBase*)obj)->_rc;
#else
        
        ref = ((RSRuntimeBase*)obj)->_rsnfo._rc;
#endif
        return ref;
    }
    else
        HALTWithError(RSInvalidArgumentException, "the instance is not available");
    return 0;
}

RSExport void __RSRuntimeSetInstanceSpecial(RSTypeRef obj, BOOL special)
{
    if (!obj) return;
    RSRuntimeBase* base = (RSRuntimeBase*)obj;
    base->_rsinfo._special = special;
}

RSExport BOOL __RSRuntimeIsInstanceSpecial(RSTypeRef rs)
{
    return ((RSRuntimeBase*)rs)->_rsinfo._special;
}
                 
RSExport void      __RSRuntimeDeallocate(RSTypeRef obj)
{
    if (obj == nil) HALTWithError(RSInvalidArgumentException, "the object is nil");
#if defined(RSAutoMemoryLog)
    RSMmLogUnitNumber();
#endif
    RSAllocatorRef allocator = RSGetAllocator(obj);
#if __RSRuntimeInstanceBZeroBeforeDie
    memset((RSMutableTypeRef)obj, 0, RSAllocatorInstanceSize(allocator, obj));
#endif
    RSAllocatorDeallocate(allocator, obj);
}
RSExport ISA __RSRuntimeRSObject(RSTypeRef obj)
{
    return (ISA)obj;
}

RSExport void __RSRuntimeCheckInstanceType(RSTypeRef obj, RSTypeID ID, const char* function)
{
    __RSRuntimeCheckInstanceAvailable(obj);
    if (unlikely(RSGetTypeID(obj) != ID))
    {
        HALTWithError(RSObjectNotAvailableException, function);
    }
    return;
}

RSPrivate const void *__RSStringCollectionCopy(RSAllocatorRef allocator, const void *ptr)
{
    return RSStringCreateCopy(allocator, ptr);
}
RSPrivate const void *__RSTypeCollectionRetain(RSAllocatorRef allocator, const void *ptr)
{
    return RSRetain(ptr);
}
RSPrivate void __RSTypeCollectionRelease(RSAllocatorRef allocator, const void *ptr)
{
    return RSRelease(ptr);
}

/****************************************************************************************************************/
// the __RSCPrint is just a printf.
static RSSpinLock __RSLogSpinlock = RSSpinLockInit;
void __RSCPrint(int fd, RSCBuffer cStr)
{
    //pthread_mutex_lock(&__RSLogMutexLock);
    RSSpinLockLock(&__RSLogSpinlock);
    printf("%s", cStr);
    RSSpinLockUnlock(&__RSLogSpinlock);
}

#ifndef ___RS_C_LOG_PREFIX
#define ___RS_C_LOG_PREFIX

#define ___RS_C_LOG_WARNING_PREFIX "<RSWarning> :"
#define ___RS_C_LOG_WARNING_PREFIX_LENGTH 13

#define ___RS_C_LOG_DEBUG_PREFIX "RSRuntime debug notice : "
#define ___RS_C_LOG_DEBUG_PREFIX_LENGTH 25
#endif

static RSCBuffer ___RSCLogGetPrefix(RSIndex loglevel, RSIndex *length)
{
    RSCBuffer prx = nil;
    if (length == nil) return prx;
    switch (loglevel) {
        case RSLogLevelDebug:
            prx = ___RS_C_LOG_DEBUG_PREFIX;
            *length = ___RS_C_LOG_DEBUG_PREFIX_LENGTH;
            break;
        case RSLogLevelWarning:
            prx = ___RS_C_LOG_WARNING_PREFIX;
            *length = ___RS_C_LOG_WARNING_PREFIX_LENGTH;
            break;
        default:
            break;
    }
    return prx;
}

void __RSCLog(RSIndex logLevel,RSCBuffer format,...)
{
    RSBuffer fitCache = nil;
    RSCBuffer prefix = nil;
    RSIndex lengthOfPrefix = 0;
    RSIndex size = 0;
    
    prefix = ___RSCLogGetPrefix(logLevel, &lengthOfPrefix);
retry:
    
    if (format)
    {
        RSBlock traceBuf[RSBufferSize] = {0};
        va_list args;
        
        int result = 0;
        va_start(args, format);
        if (fitCache == nil)
        {
            if (prefix)
            {
                result = snprintf(traceBuf, RSBufferSize, "%s", prefix);
                result = vsnprintf(traceBuf + lengthOfPrefix, RSBufferSize - lengthOfPrefix, format, args);
            }
            else
            {
                result = vsnprintf(traceBuf, RSBufferSize, format, args);
            }
        }
        else
        {
            if (prefix)
            {
                snprintf(fitCache, size, "%s", prefix);
                vsnprintf(fitCache + lengthOfPrefix, size - lengthOfPrefix, format, args);
            }
            else
            {
                vsnprintf(fitCache, size, format, args);
            }
            goto Print;//__RSCPrint(fitCache);
        }
        
        if (result < RSBufferSize)
        {
            size = result + lengthOfPrefix;
            goto Print;//__RSCPrint(traceBuf);
        }
        else
        {
            if (fitCache == nil)
            {
                fitCache = RSAllocatorAllocate(RSAllocatorSystemDefault, size = result+1+lengthOfPrefix);
                goto retry;
            }
        }
        
    Print:
        {
            RSBuffer pptr = fitCache ?:traceBuf;
            switch (logLevel)
            {
                case RSLogLevelDebug:
                    __RSDebugLevelLog(pptr, max(result, size));
                case RSLogLevelNotice:
                    __RSCPrint(STDOUT_FILENO, fitCache?:traceBuf);
                    break;
                case RSLogLevelWarning:
                    __RSCPrint(STDERR_FILENO, fitCache?:traceBuf);
                    break;
                case RSLogLevelError:
                case RSLogLevelCritical:
                case RSLogLevelEmergency:
                    __RSCPrint(STDERR_FILENO, fitCache?:traceBuf);
                    __HALT();
                default:
                    break;
            }
        }
        va_end(args);
    }
    if (fitCache) RSAllocatorDeallocate(RSAllocatorSystemDefault, fitCache);
}
#define __RSTraceLogDefaultFormat "RSRuntime Trace Error :\nCurrent Call:%s\nFrom:%s - (line : %u)\nError:%s\n"
RSExport void RSTraceLog(const char* f, const char* file, RSUInteger line, const char* error) RS_AVAILABLE(0_0)
{
    __RSTraceLog(__RSTraceLogDefaultFormat,f,file,line,error);
    return;
}

RSExport void __RSLog(RSIndex logLevel, RSStringRef format, ...)
{
    RSRetain(format);
    va_list args ;
    va_start(args, format);
    
    __RSLogArgs(logLevel, format, args);
    
    va_end(args);
    RSRelease(format);
}

RSExport void __RSLogArgs(RSIndex logLevel, RSStringRef format, va_list args)
{
    RSStringRef formatString = RSStringCreateWithFormatAndArguments(RSAllocatorSystemDefault, 0, format, args);
    
    static RSTimeZoneRef tz = nil;
    if (tz == nil) tz = RSTimeZoneCopySystem();
    RSGregorianDate ret = RSAbsoluteTimeGetGregorianDate(RSAbsoluteTimeGetCurrent(), tz);
    RSMutableStringRef logString = RSStringCreateMutable(RSAllocatorSystemDefault, 0);
    RSStringAppendStringWithFormat(logString,
                                   RSSTR("%04d-%02d-%02d %02d:%02d:%02.03f [%05d:%03x]%r"),
                                   ret.year, ret.month, ret.day, ret.hour, ret.minute, ret.second,
                                   __RSGetPid(), __RSGetTid(),
                                   formatString);
    RSRelease(formatString);
    char *buf = (char *)RSStringGetCStringPtr(logString, __RSDefaultEightBitStringEncoding); // always nil
    RSIndex converted = - 1;
    RSIndex length = logString ? (buf ? RSStringGetLength(logString) : RSStringGetMaximumSizeForEncoding(RSStringGetLength(logString), RSStringEncodingUTF8) + 1) : 0;
    BOOL shouldReleaseBuf = NO;
    char BUF[256] = {0};
    if (!buf)
    {
        buf = logString ? (length < 254 ? shouldReleaseBuf = YES, (char *)RSAllocatorAllocate(RSAllocatorSystemDefault, length) : BUF) : nil;
        if (buf) converted = RSStringGetCString(logString, (char *)buf, length, RSStringEncodingUTF8);
    }
    else converted = length;
    if (converted && logString && buf)
    {
        size_t len = length;//(logString);
        // silently ignore 0-length or really large messages, and levels outside the valid range
        if (!((1 << 24) < len))
        {
            RSBlock* cStrFormat = "%s\n";
            if ((length) && ((('\r' == (UTF8Char)*(buf + length - 1)) && '\n' == (UTF8Char)*(buf + length)) ||
                             ('\n' == (UTF8Char)*(buf + length))))
            {
                cStrFormat = "%s";
            }
            __RSCLog(logLevel, cStrFormat, buf);
        }
    }
    if (buf && shouldReleaseBuf) RSAllocatorDeallocate(RSAllocatorSystemDefault, buf);
    
    RSRelease(logString);
}

RSExport void      __RSLogShowWarning(RSTypeRef rs)
{
    return __RSLog(RSLogLevelWarning, RSSTR("%R"), rs);
}


RSExport void __RSTraceLog(RSCBuffer format,...)
{
    RSBlock traceBuf[RSBufferSize] = {0};
    if (strcmp(format,__RSTraceLogDefaultFormat) == 0)
    {
        va_list args ;
        va_start(args, format);
        if (vsnprintf(traceBuf, RSBufferSize, format, args) > -1)
        {
            __RSCPrint(STDERR_FILENO, traceBuf);
        }
        
        va_end(args);
    }
}

RSPrivate pthread_t _RSMainPThread = kNilPthreadT;

extern void __RSNilInitialize();
extern void __RSNumberInitialize();
extern void __RSAllocatorInitialize();
extern void __RSDataInitialize();
extern void __RSStringInitialize();
extern void __RSArrayInitialize();
extern void __RSDictionaryInitialize();
extern void __RSTimeZoneInitialize();
extern void __RSDateInitialize();
extern void __RSCalendarInitialize();
//extern void __RSBaseHeapInitialize();
extern void __RSUUIDInitialize();
extern void __RSRunLoopInitialize();
extern void __RSSocketInitialize();
extern void __RSRunLoopSourceInitialize();
extern void __RSBasicHashInitialize();
extern void __RSSetInitialize();
extern void __RSErrorInitialize();
extern void __RSAutoreleasePoolInitialize();

extern void __RSPropertyListInitializeInitStatics();
extern void __RSBinaryPropertyListInitStatics();
extern void __RSMessagePortInitialize();
extern void __RSExceptionInitialize();

extern void __RSNotificationCenterInitialize();
extern void __RSNotificationInitialize();
extern void __RSDistributedNotificationCenterInitialize();
extern void __RSObserverInitialize();

extern void __RSPropertyListInitialize();
extern void __RSQueueInitialize();

extern void __RSJSONSerializationInitialize();

extern void ___RSISAPayloadInitialize();
extern void __RSBundleInitialize();

extern void __RSProtocolInitialize();
extern void __RSDelegateInitialize();

extern void __RSXMLParserInitialize();
extern void __RSArchiverInitialize();

extern void __RSURLInitialize();
#include <RSCoreFoundation/RSNotificationCenter.h>
RS_PUBLIC_CONST_STRING_DECL(RSCoreFoundationWillDeallocateNotification, "RSCoreFoundationWillDeallocateNotification")
RS_PUBLIC_CONST_STRING_DECL(RSCoreFoundationDidFinishLoadingNotification, "RSCoreFoundationDidFinishLoadingNotification")

RSExport __RS_INIT_ROUTINE(RSRuntimePriority) void RSCoreFoundationInitialize()
{
#if DEBUG
    //__RSCLog(RSLogLevelDebug, "%s\n","RSRuntime initialize");//trace on.
#endif
    if (!pthread_main_np()) HALTWithError(RSGenericException, "RSCoreFoundation must be initialized on main thread");   //check initialize on the main thread.
    _RSMainPThread = pthread_self();
    
    __RSRuntimeEnvironmentInitialize();
	__RSDebugLevelInitialize();
    
    RSZeroMemory(__RSRuntimeClassTable, sizeof(__RSRuntimeClassTable));
    
    __RSRuntimeErrorClassID = __RSRuntimeRegisterClass(&__RSRuntimeErrorClass);
    __RSRuntimeSetClassTypeID(&__RSRuntimeErrorClass, __RSRuntimeErrorClassID);
    //__RSRuntimeErrorClassInitialize = 0
    __RSNilInitialize();        //  1   should be supportted by all RSTypeRef because of it is special one.
    __RSAllocatorInitialize();  //  2   referenced by everything without RSNil.
    __RSBasicHashInitialize();  //  3
    __RSDictionaryInitialize(); //  4   the dictionary use for RSString automatically memory.
    for (RSIndex idx = 0; idx < NUM_EXTERN_TABLES; idx++)
    {
	    __RSRetainCounters[idx].table = RSBasicHashCreate(RSAllocatorSystemDefault, RSBasicHashHasCounts | RSBasicHashLinearHashing | RSBasicHashAggressiveGrowth, &RSExternRefCallbacks);
	    RSBasicHashSetCapacity(__RSRetainCounters[idx].table, 40);
	    __RSRetainCounters[idx].lock = RSSpinLockInit;
	}
    
    __RSNumberInitialize();     //  5   number
    

    __RSArrayInitialize();      //  6   array
    __RSStringInitialize();     //  7   rely on RSDictionary. (RSSTR)
    
    __RSAutoreleasePoolInitialize();// 8
    ___RSISAPayloadInitialize(); //  9
    __RSExceptionInitialize();  //  10
    
    __RSRuntimeClassTableCount = 16;
    
    __RSDataInitialize();       //  17   data
    __RSUUIDInitialize();       //  18
    __RSErrorInitialize();      //  19
    
    __RSRuntimeClassTableCount = 22;
    
    __RSSetInitialize();        //  23

    __RSTimeZoneInitialize();   //  24  rely on RSString and RSArray (Known time zones information).
    __RSDateInitialize();       //  25  rely on Calendar
    __RSCalendarInitialize();   //  26  rely on RSTimeZone, RSDate, RSString.
    __RSJSONSerializationInitialize(); // not register class
//    __RSBaseHeapInitialize();   //  27
    
    __RSRunLoopInitialize();    //  27  rely on RSDictionary, RSArray, RSString, RSUUID.
    __RSRunLoopSourceInitialize();//28  do for the RSRunLoop.
    __RSURLInitialize();        // 29
    
    
    __RSSocketInitialize();     //  30  run in the RunLoop by RunLoopSource.
    
    
    
    __RSPropertyListInitializeInitStatics();
    __RSBinaryPropertyListInitStatics();
    __RSPropertyListInitialize();
    __RSQueueInitialize();
    //__RSMessagePortInitialize();
    
    __RSNotificationCenterInitialize();
    __RSNotificationInitialize();
    __RSObserverInitialize();
    
    __RSBundleInitialize();
    __RSXMLParserInitialize();
    __RSArchiverInitialize();
    
    //__RSDistributedNotificationCenterInitialize();
    if (__RSRuntimeClassTableCount < __RSRuntimeClassReservedRangeEnd) __RSRuntimeClassTableCount = __RSRuntimeClassReservedRangeEnd;
    __RSRuntimeInitiazing = NO;
    
    __RSProtocolInitialize();
    __RSDelegateInitialize();
    RSNotificationCenterPostImmediately(RSNotificationCenterGetDefault(), RSCoreFoundationDidFinishLoadingNotification, nil, nil);
    return;
}

extern void __RSTimeZoneDeallocate();
extern void __RSRunLoopDeallocate();
extern void __RSErrorDeallocate();
extern void __RSAutoreleasePoolDeallocate();
extern void __RSNotificationCenterDeallocate();
extern void __RSFileManagerDeallocate();
extern void __RSSocketDeallocate();
extern void __RSBundleDeallocate();
extern void __RSArchiverDeallocate();

void RSCoreFoundationDeallocate() __RS_FINAL_ROUTINE(RSRuntimePriority);

void RSCoreFoundationDeallocate()
{
    //__RSCLog(RSLogLevelNotice, "%s\n", "RSCoreFoundationDeallocate");
    RSNotificationCenterPostImmediately(RSNotificationCenterGetDefault(), RSCoreFoundationWillDeallocateNotification, nil, nil);
    __RSArchiverDeallocate();
    __RSSocketDeallocate();
    __RSNotificationCenterDeallocate();
    __RSRunLoopDeallocate();
    __RSBundleDeallocate();
    __RSFileManagerDeallocate();
    __RSErrorDeallocate();
    __RSTimeZoneDeallocate();   // deallocate RSTimeZone (known time zones information)
    __RSAutoreleasePoolDeallocate();
    RSStringCacheRelease();     // deallocate RSString automatically memory.
    
    for (RSIndex idx = 0; idx < NUM_EXTERN_TABLES; idx++)
    {
	    __RSRuntimeRelease(__RSRetainCounters[idx].table, NO);
	}
#if DEBUG
//    RSAllocatorLogUnitCount();        // check the Memory unit, it should be 0 or memory leaks.
#endif
    RSAllocatorLogUnitCount();        // check the Memory unit, it should be 0 or memory leaks.
	__RSDebugLevelDeallocate();
}

#if defined(__ppc__)
#define HALT do { asm __volatile__("trap"); kill(getpid(), 9); } while (0)
#elif defined(__i386__) || defined(__x86_64__)
#if defined(__GNUC__)
#define HALT do { asm __volatile__("int3"); kill(getpid(), 9); } while (0)
#elif defined(_MSC_VER)
#define HALT do {DebugBreak(); abort(); } while (0)
#else
#error Compiler not supported
#endif
#endif
#if defined(__arm__)
#define HALT do {asm __volatile__("bkpt 0xCF"); kill(getpid(), 9); } while (0)
#endif

void __HALT(){
    HALT;
#if __has_builtin(__builtin_unreachable)
    __builtin_unreachable();
#endif
//#if defined(__ppc__)
//    __asm__("trap");
//#elif defined(__i386__) || defined(__x86_64__)
//#if defined(_MSC_VER)
//    __asm int 3;
//#else
//    __asm__("int3");
//#endif
//#endif
    exit(0);
}
