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
#include "../CoreFoundation/Hash/RSRawHashTable.h"

// define the runtime class table   (10 bits)
#define  __RSClassTableSize 1024
RSSpinLock __RSRuntimeClassTableSpinLock = RSSpinLockInit;
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
    0,
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

#if DEPLOYMENT_TARGET_MACOSX || DEPLOYMENT_TARGET_EMBEDDED || DEPLOYMENT_TARGET_EMBEDDED_MINI
RSPrivate uint8_t __RS120290 = NO;
RSPrivate uint8_t __RS120291 = NO;
RSPrivate uint8_t __RS120293 = NO;
RSPrivate char * __crashreporter_info__ = nil; // Keep this symbol, since it was exported and other things may be linking against it, like GraphicsServices.framework on iOS
asm(".desc ___crashreporter_info__, 0x10");


//  __01121__ AND __01123__ use for check fork
static void __01121__(void) {
    __RS120291 = pthread_is_threaded_np() ? YES : NO;
}
extern void RSCoreFoundationCrash(const char *crash);
static void __01123__(void) {
    // Ideally, child-side atfork handlers should be async-cancel-safe, as fork()
    // is async-cancel-safe and can be called from signal handlers.  See also
    // http://standards.ieee.org/reading/ieee/interp/1003-1c-95_int/pasc-1003.1c-37.html
    // This is not a problem for RS.
    if (__RS120290) {
        __RS120293 = YES;
#if DEPLOYMENT_TARGET_MACOSX
        if (__RS120291) {
            RSCoreFoundationCrash("*** multi-threaded process forked ***");
        } else {
            RSCoreFoundationCrash("*** single-threaded process forked ***");
        }
#endif
    }
}

#define EXEC_WARNING_STRING_1 "The process has forked and you cannot use this CoreFoundation functionality safely. You MUST exec().\n"
#define EXEC_WARNING_STRING_2 "Break on __THE_PROCESS_HAS_FORKED_AND_YOU_CANNOT_USE_THIS_COREFOUNDATION_FUNCTIONALITY___YOU_MUST_EXEC__() to debug.\n"

RSPrivate void __THE_PROCESS_HAS_FORKED_AND_YOU_CANNOT_USE_THIS_COREFOUNDATION_FUNCTIONALITY___YOU_MUST_EXEC__(void) {
    write(2, EXEC_WARNING_STRING_1, sizeof(EXEC_WARNING_STRING_1) - 1);
    write(2, EXEC_WARNING_STRING_2, sizeof(EXEC_WARNING_STRING_2) - 1);
    __HALT();
}
#endif

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

struct _RSRuntimeLogPreference ___RSDebugLogPreference = {
#define DEBUG_PERFERENCE(Prefix, Name, BitWidth, Default, Comment) .Prefix##Name = _##Prefix##Name,
#include <RSCoreFoundation/RSRuntimeDebugSupport.h>
};

struct ____RSDebugLogContext {
    RSSpinLock _debugLogLock;
    FILE *_file;
    int _handle;
	RSBlock _logPath[RSMaxPathSize];
};

#include <sys/fcntl.h>
#include <sys/stat.h>
static struct ____RSDebugLogContext ____RSRuntimeDebugLevelLogContext = {0};

static BOOL __RSDebugLevelPreferencesParser(const char *buffer, size_t len, struct _RSRuntimeLogPreference *preference) {
    BOOL result = NO;
    if (!buffer || !len || !preference)
        return result;
    
    
    
    return result;
}

static void __RSDebugLevelPreferencesInitialize() {
    RSBlock _preferencesPath[RSMaxPathSize] = {0};
    strcpy(_preferencesPath, __RSRuntimeGetEnvironment("HOME") ? : "");
#if DEPLOYMENT_TARGET_MACOSX || DEPLOYMENT_TARGET_LINUX
    strcat(_preferencesPath, ".RSCoreFoundationPreferences");
    int handle = open(_preferencesPath, O_RDONLY, 0555);
    if (handle >= 0) {
        size_t fileSize = lseek(handle, 0, SEEK_END);
        if (fileSize) {
            lseek(handle, 0, SEEK_SET);
            const char *buffer = nil;
            if (fileSize < RSMaxPathSize) {
                __builtin_memset(_preferencesPath, 0, RSMaxPathSize);
                buffer = _preferencesPath;
            } else {
                buffer = (const char *)calloc(1, fileSize + 1);
            }
            ssize_t readLength = read(handle, (void *)buffer, fileSize);
            if (readLength) {
                struct _RSRuntimeLogPreference preference = ___RSDebugLogPreference;
                if (__RSDebugLevelPreferencesParser(buffer, readLength, &preference))
                    ___RSDebugLogPreference = preference;
            }
            if (buffer != _preferencesPath)
                free((void *)buffer);
        }
        close(handle);
    }
#elif DEPLOYMENT_TARGET_WINDOWS

#endif
    
    const char * Value = nil;
#define DEBUG_PERFERENCE(Prefix, Name, BitWidth, Default, Comment) \
    Value = __RSGetEnvironment(""#Prefix#Name); \
    if (Value) { \
        if (atoi(Value) == 1) {\
            __RSCLog(RSLogLevelNotice, "----- " #Prefix#Name " is enabled -----\n");\
            ___RSDebugLogPreference.Prefix##Name = 1;\
        }\
        Value = nil;\
    }\

#include <RSCoreFoundation/RSRuntimeDebugSupport.h>
    
    if (___RSDebugLogPreference._RSRuntimeInstanceManageWatcher | ___RSDebugLogPreference._RSRuntimeInstanceRefWatcher | ___RSDebugLogPreference._RSRuntimeInstanceAllocFreeWatcher | ___RSDebugLogPreference._RSStringNoticeWhenConstantStringAddToTable | ___RSDebugLogPreference._RSRuntimeCheckAutoreleaseFlag) {
        ___RSDebugLogPreference._RSRuntimeLogSave = 1;
    } else {
        ___RSDebugLogPreference._RSRuntimeLogSave = 0;
    }
}

static void __RSDebugLevelInitialize()
{
    RSBlock _debuglogPrefixPath[RSMaxPathSize] = {0};
    ____RSRuntimeDebugLevelLogContext._debugLogLock = RSSpinLockInit;
	strcpy(_debuglogPrefixPath, __RSRuntimeGetEnvironment("HOME") ? : "");
    __RSDebugLevelPreferencesInitialize();
#if DEPLOYMENT_TARGET_MACOSX || DEPLOYMENT_TARGET_LINUX
    strcat(_debuglogPrefixPath, "/Library/Logs/RSCoreFoundation");
	mkdir(_debuglogPrefixPath, 0777);
    sprintf(____RSRuntimeDebugLevelLogContext._logPath, "%s/com.retval.RSCoreFoundation.runtime.runid%05d.log", _debuglogPrefixPath, getpid());
    ____RSRuntimeDebugLevelLogContext._handle = fileno(____RSRuntimeDebugLevelLogContext._file = tmpfile());
#elif DEPLOYMENT_TARGET_WINDOWS
    ____RSRuntimeDebugLevelLogContext._handle = open(____RSRuntimeDebugLevelLogContext._logPath, O_RDWR | O_CREAT, 0555);
#endif
}

RSPrivate void __RSDebugLevelDeallocate()
{
	unsigned long long org = lseek(____RSRuntimeDebugLevelLogContext._handle, 0, SEEK_CUR);
    lseek(____RSRuntimeDebugLevelLogContext._handle, 0, SEEK_END);
    unsigned long long size __unused = lseek(____RSRuntimeDebugLevelLogContext._handle, 0, SEEK_CUR); // get current file pointer
    lseek(____RSRuntimeDebugLevelLogContext._handle, org, SEEK_SET);
    if (___RSDebugLogPreference._RSRuntimeLogSave)
	{
        int fd = open(____RSRuntimeDebugLevelLogContext._logPath, O_RDWR | O_CREAT, 0555);
        if (fd > 0) {
            char *buf = calloc(1, size);
            lseek(____RSRuntimeDebugLevelLogContext._handle, 0, SEEK_SET);
            read(____RSRuntimeDebugLevelLogContext._handle, buf, size);
            write(fd, buf, size);
            free(buf);
            close(fd);
        }
	}
    
    close(____RSRuntimeDebugLevelLogContext._handle);
    fclose(____RSRuntimeDebugLevelLogContext._file);
    ____RSRuntimeDebugLevelLogContext._handle = 0;
    __builtin_memset(____RSRuntimeDebugLevelLogContext._logPath, 0, 2*RSMaxPathSize);
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

static void __RSRuntimeSetClassTypeIDWithNameToCache(const char *name, RSTypeID ID);
static void __RSRuntimeRemoveClassTypeIDWithNameFromCache(const char *name);

const RSHashCode RSIndexMax = ~0;
RSExport RSTypeID __RSRuntimeRegisterClass(const RSRuntimeClass* restrict const cls) {
    if (!cls->className)
        return _RSRuntimeNotATypeID;
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
    __RSRuntimeSetClassTypeIDWithNameToCache(cls->className, type);
    __RSRuntimeUnlockTable();
    return type;
}

RSExport void __RSRuntimeUnregisterClass(RSTypeID classID)
{
    __RSRuntimeLockTable();
    __RSRuntimeClassTable[classID] = NULL;
    __RSRuntimeUnlockTable();
}

RSExport const RSRuntimeClass* __RSRuntimeGetClassWithTypeID(RSTypeID classID) {
    __RSRuntimeLockTable();
    if (classID >= __RSRuntimeClassTableCount) {
        __RSRuntimeUnlockTable();
        HALTWithError(RSInvalidArgumentException, "the size of class table in runtime is overflow");
    }
    if (__RSRuntimeClassTable[classID] == nil) {
        __RSRuntimeUnlockTable();
        HALTWithError(RSInvalidArgumentException, "the class is not registered in runtime");
    }
    const RSRuntimeClass *cls = (const RSRuntimeClass*)__RSRuntimeClassTable[classID];
    __RSRuntimeUnlockTable();
    return cls;
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
        unsigned char reftype = version._rsinfo;
        __builtin_memset(&version, 0, sizeof(RSRuntimeClassVersion));
        version.reftype = reftype;
        if (version.reftype == _RSRuntimeScannedObject) {
            if (cls->refcount) {
                version.reftype = _RSRuntimeCustomRefCount;
                __RSRuntimeSetClassVersionInformation(__cls, version);
                __RSRuntimeSetInstanceAsClass(__cls);
                __RSRuntimeSetInstanceSpecial(__cls, YES);
                __RSSetValid(__cls);
                return YES;
            }
            version.reftype = _RSRuntimeBasedObject;
            __RSRuntimeSetClassVersionInformation(__cls, version);
            __RSRuntimeSetInstanceAsClass(__cls);
            __RSRuntimeSetInstanceSpecial(__cls, YES);
            __RSSetValid(__cls);
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
            
            if (___RSDebugLogPreference._RSRuntimeInstanceManageWatcher)
                __RSCLog(RSLogLevelDebug, "%s alloc - <%p>\n", cls->className, obj);

        }
    }
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
    uint8_t padding[64 - sizeof(RSBasicHashRef)];
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
        case _RSRuntimeOperationRetain:   // increment, no event
            RSSpinLockLock(lock);
            RSBasicHashAddValue(table, disguised, disguised);
            RSSpinLockUnlock(lock);
            return (uintptr_t)obj;
        case 400:   // decrement
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

RSExport void __RSRuntimeInstanceDeallocate(RSTypeRef obj) {
    RSRuntimeClass* cls = (RSRuntimeClass*)__RSRuntimeGetClassWithTypeID(RSGetTypeID(obj));
    return cls->deallocate(obj);
}

RSExport RSTypeRef __RSRuntimeRetain(RSTypeRef obj, BOOL try) {
    RSTypeID typeID = RSGetTypeID(obj);
    if (typeID != _RSRuntimeNotATypeID) {
        //RSUInteger refCount = ~0;
        if (try) return obj;
        RSIndex ref = 0;
        RSIndex before = 0;
#if __LP64__
        if (__RSRuntimeInstanceIsSpecial(obj))
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

RSExport BOOL __RSRuntimeInstanceIsSpecial(RSTypeRef rs)
{
    return ((RSRuntimeBase*)rs)->_rsinfo._special;
}
                 
RSExport void      __RSRuntimeDeallocate(RSTypeRef obj)
{
    if (obj == nil) HALTWithError(RSInvalidArgumentException, "the object is nil");
    RSAllocatorRef allocator = RSGetAllocator(obj);
    if (___RSDebugLogPreference._RSRuntimeInstanceBZeroBeforeDie)
        memset((RSMutableTypeRef)obj, 0, RSAllocatorInstanceSize(allocator, obj));
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
    RSSyncUpdateBlock(&__RSLogSpinlock, ^{
        printf("%s", cStr);
    });
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
                    // do not fall through
                    if (!___RSDebugLogPreference._RSLogDebugLevelShouldFallThrough) {
                        break;
                    }
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

RSExport void __RSLog(RSIndex logLevel, RSStringRef format, ...) {
    RSRetain(format);
    va_list args ;
    va_start(args, format);
    
    __RSLogArgs(logLevel, format, args);
    
    va_end(args);
    RSRelease(format);
}

RSExport void __RSLogArgs(RSIndex logLevel, RSStringRef format, va_list args) {
    RSStringRef formatString = RSStringCreateWithFormatAndArguments(RSAllocatorSystemDefault, 0, format, args);
    
    static RSTimeZoneRef tz = nil;
    if (tz == nil) tz = RSTimeZoneCopySystem();
    RSGregorianDate ret = RSAbsoluteTimeGetGregorianDate(RSAbsoluteTimeGetCurrent(), tz);
    RSMutableStringRef logString = RSStringCreateMutable(RSAllocatorSystemDefault, 0);
    RSStringAppendStringWithFormat(logString,
                                   RSSTR("%04d-%02d-%02d %02d:%02d:%02.03f [%05d:%03x] %r"),
                                   ret.year, ret.month, ret.day, ret.hour, ret.minute, ret.second,
                                   __RSGetPid(), __RSGetTid(),
                                   formatString);
    RSRelease(formatString);
    char *buf = (char *)RSStringGetCStringPtr(logString, __RSDefaultEightBitStringEncoding); // always nil
    RSIndex converted = - 1;
    RSIndex length = logString ? (buf ? RSStringGetLength(logString) : RSStringGetMaximumSizeForEncoding(RSStringGetLength(logString), RSStringEncodingUTF8) + 1) : 0;
    BOOL shouldReleaseBuf = NO;
    char BUF[256] = {0};
    if (!buf) {
        buf = logString ? (length > 254 ? (void)(shouldReleaseBuf = YES), (char *)RSAllocatorAllocate(RSAllocatorSystemDefault, length) : BUF) : nil;
        if (buf) converted = RSStringGetCString(logString, (char *)buf, length, RSStringEncodingUTF8);
    } else {
        converted = length;
    }
    if (converted && logString && buf) {
        RSBitU64 len = length;//(logString);
        // silently ignore 0-length or really large messages, and levels outside the valid range
        if (!((1 << 31) < len)) {
            RSBlock* cStrFormat = "%s\n";
            if ((length) && ((('\r' == (UTF8Char)*(buf + length - 1)) && '\n' == (UTF8Char)*(buf + length)) ||
                             ('\n' == (UTF8Char)*(buf + length)))) {
                cStrFormat = "%s";
            }
            __RSCLog(logLevel, cStrFormat, buf);
        }
    }
    if (buf && shouldReleaseBuf) RSAllocatorDeallocate(RSAllocatorSystemDefault, buf);
    
    RSRelease(logString);
}

RSExport void __RSLogShowWarning(RSTypeRef rs) {
    return __RSLog(RSLogLevelWarning, RSSTR("%R"), rs);
}


RSExport void __RSTraceLog(RSCBuffer format,...) {
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



#pragma mark -
#pragma mark Runtime Class Name Cache (Name - TypeID)

static raw_hash_table __RSRuntimeRawHashTable = {0};
static RSSpinLock __RSRuntimeRawHashTableLock = RSSpinLockInit;

static void __RSRuntimeClassCacheInitize() {
    raw_ht_init(&__RSRuntimeRawHashTable, HT_KEY_CONST, 0.05);
}

static void __RSRuntimeClassCacheDeallocate() {
    raw_ht_destroy(&__RSRuntimeRawHashTable);
}

static RSTypeID __RSRuntimeGetClassTypeIDWithNameInCache(const char *name) {
    if (!name) return _RSRuntimeNotATypeID;
    __block RSTypeID ID = _RSRuntimeNotATypeID;
    RSSyncUpdateBlock(&__RSRuntimeRawHashTableLock, ^{
        size_t value_size = 0;
        void *value = raw_ht_get(&__RSRuntimeRawHashTable, (void *)name, strlen(name) + 1, &value_size);
        if (value)
            __builtin_memcpy(&ID, value, value_size);
    });
    return ID;
}

static void __RSRuntimeSetClassTypeIDWithNameToCache(const char *name, RSTypeID ID) {
    if (!name || ID == _RSRuntimeNotATypeID)
        return ;
    RSSyncUpdateBlock(&__RSRuntimeRawHashTableLock, ^{
        RSTypeID typeID = ID;
        raw_ht_insert(&__RSRuntimeRawHashTable, (void *)name, strlen(name) + 1, (void *)&typeID, sizeof(RSTypeID));
    });
    return ;
}

static void __RSRuntimeRemoveClassTypeIDWithNameFromCache(const char *name) {
    if (!name)
        return;
    RSSyncUpdateBlock(&__RSRuntimeRawHashTableLock, ^{
        raw_ht_remove(&__RSRuntimeRawHashTable, (void *)name, strlen(name) + 1);
    });
}

RSExport RSTypeID __RSRuntimeGetClassTypeIDWithName(const char *name) {
    RSTypeID ID = _RSRuntimeNotATypeID;
    if (!name) return ID;
    if (_RSRuntimeNotAType != (ID = __RSRuntimeGetClassTypeIDWithNameInCache(name)))
        return ID;
    __RSRuntimeLockTable();
    for (RSTypeID idx = 0; idx < __RSRuntimeClassTableCount; idx++) {
        if (__RSRuntimeClassTable[idx] && __RSRuntimeClassTable[idx]->className && 0 == strcmp(name, __RSRuntimeClassTable[idx]->className)) {
            ID = idx;
            break;
        }
    }
    if (_RSRuntimeNotATypeID != ID) {
        __RSRuntimeSetClassTypeIDWithNameToCache(name, ID);
    }
    __RSRuntimeUnlockTable();
    return ID;
}

RSPrivate pthread_t _RSMainPThread = kNilPthreadT;

extern void __RSNilInitialize(void);
extern void __RSNumberInitialize(void);
extern void __RSAllocatorInitialize(void);
extern void __RSDataInitialize(void);
extern void __RSStringInitialize(void);
extern void __RSArrayInitialize(void);
extern void __RSDictionaryInitialize(void);
extern void __RSTimeZoneInitialize(void);
extern void __RSDateInitialize(void);
extern void __RSCalendarInitialize(void);
extern void __RSCharacterSetInitialize(void);
//extern void __RSBaseHeapInitialize();
extern void __RSUUIDInitialize(void);
extern void __RSRunLoopInitialize(void);
extern void __RSRunLoopSourceInitialize(void);
extern void __RSRunLoopObserverInitialize(void);
extern void __RSRunLoopTimerInitialize(void);

//extern void __RSSocketInitialize();
extern void __RSBasicHashInitialize(void);
extern void __RSSetInitialize(void);
extern void __RSErrorInitialize(void);
extern void __RSAutoreleasePoolInitialize(void);

extern void __RSPropertyListInitializeInitStatics(void);
extern void __RSBinaryPropertyListInitStatics(void);
extern void __RSMessagePortInitialize(void);
extern void __RSExceptionInitialize(void);

extern void __RSNotificationCenterInitialize(void);
extern void __RSNotificationInitialize(void);
extern void __RSDistributedNotificationCenterInitialize(void);
extern void __RSObserverInitialize(void);

extern void __RSPropertyListInitialize(void);
extern void __RSQueueInitialize(void);

extern void __RSJSONSerializationInitialize(void);
extern void __RSThreadInitialize(void);

extern void ___RSISAPayloadInitialize(void);
extern void __RSBundleInitialize(void);

extern void __RSProtocolInitialize(void);
extern void __RSDelegateInitialize(void);

extern void __RSXMLParserInitialize(void);
extern void __RSArchiverInitialize(void);

extern void __RSURLInitialize(void);
extern void __RSURLRequestInitialize(void);
extern void __RSURLResponseInitialize(void);
extern void __RSURLConnectionInitialize(void);
extern void __RSHTTPCookieInitialize(void);

extern void __RSMachPortInitialize(void);
extern void __RSMessagePortInitialize(void);

#include <RSCoreFoundation/RSNotificationCenter.h>
RS_PUBLIC_CONST_STRING_DECL(RSCoreFoundationWillDeallocateNotification, "RSCoreFoundationWillDeallocateNotification")
RS_PUBLIC_CONST_STRING_DECL(RSCoreFoundationDidFinishLoadingNotification, "RSCoreFoundationDidFinishLoadingNotification")

RSExport RS_INIT_ROUTINE void RSCoreFoundationInitialize() {
#if DEBUG
    //__RSCLog(RSLogLevelDebug, "%s\n","RSRuntime initialize");//trace on.
#endif
    if (!pthread_main_np()) HALTWithError(RSGenericException, "RSCoreFoundation must be initialized on main thread");   //check initialize on the main thread.
    _RSMainPThread = pthread_self();
    
    __RSRuntimeEnvironmentInitialize();
	__RSDebugLevelInitialize();
    __RSRuntimeClassCacheInitize();
    
    pthread_atfork(__01121__, nil, __01123__);
    
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
    __RSCharacterSetInitialize();
    
    __RSRuntimeClassTableCount = 22;
    
    __RSSetInitialize();        //  23

    __RSTimeZoneInitialize();   //  24  rely on RSString and RSArray (Known time zones information).
    __RSDateInitialize();       //  25  rely on Calendar
    __RSCalendarInitialize();   //  26  rely on RSTimeZone, RSDate, RSString.
    __RSJSONSerializationInitialize(); // not register class
//    __RSBaseHeapInitialize();   //  27
    
    __RSRuntimeClassTableCount = 40;
    __RSRunLoopInitialize();
    __RSRunLoopSourceInitialize();
    __RSRunLoopObserverInitialize();
    __RSRunLoopTimerInitialize();
    
    __RSNotificationCenterInitialize();
    __RSNotificationInitialize();
    __RSObserverInitialize();
    
    __RSThreadInitialize();
    
    __RSPropertyListInitializeInitStatics();
    __RSBinaryPropertyListInitStatics();
    __RSPropertyListInitialize();
    
    __RSArchiverInitialize();
    
//    __RSSocketInitialize();
    
    __RSURLInitialize();
    __RSURLRequestInitialize();
    __RSURLResponseInitialize();
    __RSURLConnectionInitialize();
    __RSHTTPCookieInitialize();
    
    __RSQueueInitialize();
    
    __RSBundleInitialize();
    __RSXMLParserInitialize();
    
    __RSMachPortInitialize();
    __RSMessagePortInitialize();
    
    //__RSDistributedNotificationCenterInitialize();
    if (__RSRuntimeClassTableCount < __RSRuntimeClassReservedRangeEnd) __RSRuntimeClassTableCount = __RSRuntimeClassReservedRangeEnd;
    __RSRuntimeInitiazing = NO;
    
    __RSProtocolInitialize();
    __RSDelegateInitialize();
    
    RSNotificationCenterPostImmediately(RSNotificationCenterGetDefault(), RSCoreFoundationDidFinishLoadingNotification, nil, nil);
    return;
}

extern void __RSTimeZoneDeallocate(void);
extern void __RSRunLoopDeallocate(void);
extern void __RSErrorDeallocate(void);
extern void __RSAutoreleasePoolDeallocate(void);
extern void __RSNotificationCenterDeallocate(void);
extern void __RSFileManagerDeallocate(void);
extern void __RSSocketDeallocate(void);
extern void __RSBundleDeallocate(void);
extern void __RSArchiverDeallocate(void);
extern void __RSCharacterSetDeallocate(void);

void RSCoreFoundationDeallocate(void) __RS_FINAL_ROUTINE(RSRuntimePriority);

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
    __RSCharacterSetDeallocate();
    RSStringCacheRelease();     // deallocate RSString automatically memory.
    
    for (RSIndex idx = 0; idx < NUM_EXTERN_TABLES; idx++)
    {
	    __RSRuntimeRelease(__RSRetainCounters[idx].table, NO);
	}
#if DEBUG
    RSAllocatorLogUnitCount();        // check the Memory unit, it should be 0 or memory leaks.
#endif
    __RSRuntimeClassCacheDeallocate();
    // check the Memory unit, it should be 0 or memory leaks.
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

void __HALT() {
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
}
