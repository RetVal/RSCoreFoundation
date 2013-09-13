//
//  RSProcessInfo.c
//  RSCoreFoundation
//
//  Created by RetVal on 8/16/13.
//  Copyright (c) 2013 RetVal. All rights reserved.
//

#include "RSProcessInfo.h"

#include <RSCoreFoundation/RSRuntime.h>
#include "../RSBase/Core/BaseStructure/RSPrivate/RSPrivateOperatingSystem/RSPrivateFileSystem.h"
RS_CONST_STRING_DECL(__RSProcessInfoEnvironment, "RSEnvironemnt");
RS_CONST_STRING_DECL(__RSProcessInfoProgramName, "RSProgramName");

RS_CONST_STRING_DECL(__RSProcessInfoMachine, "RSMachine");
RS_CONST_STRING_DECL(__RSProcessInfoMachineType, "RSMachineType");
RS_CONST_STRING_DECL(__RSProcessInfoMachineSerinalNumber, "RSMachineSerinalNumber");

RS_CONST_STRING_DECL(__RSProcessInfoOperatingSystemVersion, "RSOperatingSystemVersion");
RS_CONST_STRING_DECL(__RSProcessInfoHumenReadableOperatingSystemVersion, "RSHumenReadableOperatingSystemVersion");
RS_CONST_STRING_DECL(__RSProcessInfoOperatingSystemMajorVersion, "RSOperatingSystemMajorVersion");
RS_CONST_STRING_DECL(__RSProcessInfoOperatingSystemMinorVersion, "RSOperatingSystemMinorVersion");
RS_CONST_STRING_DECL(__RSProcessInfoOperationSystemBugFixVersion, "RSOperationSystemBugFixVersion");

RS_CONST_STRING_DECL(__RSProcessInfoKernel, "RSKernel");
RS_CONST_STRING_DECL(__RSProcessInfoKernelType, "RSKernelType");
RS_CONST_STRING_DECL(__RSProcessInfoKernelUUID, "RSKernelUUID");
RS_CONST_STRING_DECL(__RSProcessInfoKernelVersion, "RSKernelVersion");
RS_CONST_STRING_DECL(__RSProcessInfoKernelRelease, "RSKernelRelease");

RS_CONST_STRING_DECL(__RSProcessInfoProcessor, "RSProcessor");
RS_CONST_STRING_DECL(__RSProcessInfoProcessorBrand, "RSProcessorBrand");
RS_CONST_STRING_DECL(__RSProcessInfoProcessorCount, "RSProcessorCount");

RS_CONST_STRING_DECL(__RSProcessInfoMemory, "RSMemory");
RS_CONST_STRING_DECL(__RSProcessInfoMemoryCapacity, "RSMemoryCapacity");
RS_CONST_STRING_DECL(__RSProcessInfoMemoryCache1, "RSMemoryCache1");
RS_CONST_STRING_DECL(__RSProcessInfoMemoryCache2, "RSMemoryCache2");
RS_CONST_STRING_DECL(__RSProcessInfoMemoryCache3, "RSMemoryCache3");

RS_CONST_STRING_DECL(__RSProcessInfoDisk, "RSDisk");

struct __RSProcessInfo
{
    RSRuntimeBase _base;
    RSMutableDictionaryRef _info;
    void *_core;
};

RSInline RSHandle __RSProcessInfoLoadLibForSerinalNumber()
{
    RSHandle handle = nil;
#if DEPLOYMENT_TARGET_MACOSX || DEPLOYMENT_TARGET_IPHONEOS
    RSBundleRef framework = RSBundleGetWithIdentifier(RSSTR("com.RetVal.RSCoreFoundation"));
    if (framework)
    {
        RSStringRef path = RSBundleCreatePathForResource(framework, RSSTR("RSProcessInfo"), RSSTR("dylib"));
        handle = _RSLoadLibrary(path);
        RSRelease(path);
    }
#elif DEPLOYMENT_TARGET_LINUX
    handle = _RSLoadLibrary(RSSTR("RSProcessInfo.so"));
#elif DEPLOYMENT_TARGET_WINDOWS
    handle = _RSLoadLibrary(RSSTR("RSProcessInfo.dll"));
#endif
    return handle;
}

static RSTypeRef __RSProcessInfoClassCopy(RSAllocatorRef allocator, RSTypeRef rs, BOOL mutableCopy)
{
    return RSRetain(rs);
}

static void __RSProcessInfoClassDeallocate(RSTypeRef rs)
{
    RSProcessInfoRef pi = (RSProcessInfoRef)rs;
    if (pi->_info)
    {
        __RSRuntimeSetInstanceSpecial(RSDictionaryGetValue(pi->_info, __RSProcessInfoEnvironment), NO);
        __RSRuntimeSetInstanceSpecial(RSDictionaryGetValue(pi->_info, __RSProcessInfoMachine), NO);
        __RSRuntimeSetInstanceSpecial(RSDictionaryGetValue(pi->_info, __RSProcessInfoOperatingSystemVersion), NO);
        __RSRuntimeSetInstanceSpecial(RSDictionaryGetValue(pi->_info, __RSProcessInfoKernel), NO);
        __RSRuntimeSetInstanceSpecial(RSDictionaryGetValue(pi->_info, __RSProcessInfoProcessor), NO);
        __RSRuntimeSetInstanceSpecial(RSDictionaryGetValue(pi->_info, __RSProcessInfoMemory), NO);
        __RSRuntimeSetInstanceSpecial(RSDictionaryGetValue(pi->_info, __RSProcessInfoProgramName), NO);
        __RSRuntimeSetInstanceSpecial(pi->_info, NO);
        RSRelease(pi->_info);
    }
    _RSCloseHandle(pi->_core);
}

static BOOL __RSProcessInfoClassEqual(RSTypeRef rs1, RSTypeRef rs2)
{
    RSProcessInfoRef RSProcessInfo1 = (RSProcessInfoRef)rs1;
    RSProcessInfoRef RSProcessInfo2 = (RSProcessInfoRef)rs2;
    BOOL result = NO;
    
    result = RSEqual(RSProcessInfo1->_info, RSProcessInfo2->_info);
    
    return result;
}

static RSHashCode __RSProcessInfoClassHash(RSTypeRef rs)
{
    return RSHash(((RSProcessInfoRef)rs)->_info);
}

static RSStringRef __RSProcessInfoClassDescription(RSTypeRef rs)
{
    RSStringRef description = RSStringCreateWithFormat(RSAllocatorDefault, RSSTR("RSProcessInfo %p"), rs);
    return description;
}

static RSRuntimeClass __RSProcessInfoClass =
{
    _RSRuntimeScannedObject,
    "RSProcessInfo",
    nil,
    __RSProcessInfoClassCopy,
    __RSProcessInfoClassDeallocate,
    __RSProcessInfoClassEqual,
    __RSProcessInfoClassHash,
    __RSProcessInfoClassDescription,
    nil,
    nil
};

static RSSpinLock __RSProcessInfoLock = RSSpinLockInit;
static RSProcessInfoRef __RSProcessInfoGlobal = nil;
static RSTypeID _RSProcessInfoTypeID = _RSRuntimeNotATypeID;

RSExport RSTypeID RSProcessInfoGetTypeID()
{
    return _RSProcessInfoTypeID;
}
static void __RSProcessInfoInitialize() RS_INIT_ROUTINE;
static void __RSProcessInfoInitialize()
{
    _RSProcessInfoTypeID = __RSRuntimeRegisterClass(&__RSProcessInfoClass);
    __RSRuntimeSetClassTypeID(&__RSProcessInfoClass, _RSProcessInfoTypeID);
}
static void __RSProcessInfoDeallocate() RS_FINAL_ROUTINE;
static void __RSProcessInfoDeallocate()
{
    RSSpinLockLock(&__RSProcessInfoLock);
    if (__RSProcessInfoGlobal)
    {
        __RSRuntimeSetInstanceSpecial(__RSProcessInfoGlobal, NO);
        RSRelease(__RSProcessInfoGlobal);
        __RSProcessInfoGlobal = nil;
    }
    RSSpinLockUnlock(&__RSProcessInfoLock);
}

static RSProcessInfoRef __RSProcessInfoCreateInstance(RSAllocatorRef allocator, RSDictionaryRef content)
{
    RSProcessInfoRef instance = (RSProcessInfoRef)__RSRuntimeCreateInstance(allocator, _RSProcessInfoTypeID, sizeof(struct __RSProcessInfo) - sizeof(RSRuntimeBase));
    
    if (content)
    {
        instance->_info = RSMutableCopy(allocator, content);
    }
    else
    {
        instance->_info = RSDictionaryCreateMutable(RSAllocatorDefault, 0, RSDictionaryRSTypeContext);
    }
    __RSRuntimeSetInstanceSpecial(instance->_info, YES);
    
    return instance;
}

// do lazy loading

RSInline RSTypeRef __RSProcessInfoObjectForKey(RSProcessInfoRef processInfo, RSStringRef key)
{
    return RSDictionaryGetValue(processInfo->_info, key);
}

RSInline void __RSProcessInfoSetObjectForKey(RSProcessInfoRef processInfo, RSStringRef key, RSTypeRef value)
{
    return RSDictionarySetValue(processInfo->_info, key, value);
}

RSInline RSHandle __RSProcessInfoGetHandle(RSProcessInfoRef processInfo)
{
    return processInfo->_core ? : (processInfo->_core = __RSProcessInfoLoadLibForSerinalNumber());
}

RSInline void __RSDictionaryAddEnvWithCString(RSMutableDictionaryRef dict, const char *key)
{
    const char *value = __RSGetEnvironment(key);
    if (dict && key && value)
    {
        RSStringRef k = RSStringCreateWithCString(RSAllocatorSystemDefault, key, RSStringEncodingUTF8);
        RSStringRef v = RSStringCreateWithCString(RSAllocatorSystemDefault, value, RSStringEncodingUTF8);
        RSDictionarySetValue(dict, k, v);
        RSRelease(k);
        RSRelease(v);
    }
}

RSInline RSDictionaryRef __RSProcessInfoGetEnvironment(RSProcessInfoRef processInfo)
{
    RSDictionaryRef dict = __RSProcessInfoObjectForKey(processInfo, __RSProcessInfoEnvironment);
    if (dict) return dict;
    RSMutableDictionaryRef environment = RSDictionaryCreateMutable(RSAllocatorSystemDefault, 0, RSDictionaryRSTypeContext);
#if DEPLOYMENT_TARGET_MACOSX || DEPLOYMENT_TARGET_IPHONEOS
    __RSDictionaryAddEnvWithCString(environment, "Apple_PubSub_Socket_Render");
    __RSDictionaryAddEnvWithCString(environment, "DYLD_FRAMEWORK_PATH");
    __RSDictionaryAddEnvWithCString(environment, "DYLD_LIBRARY_PATH");
#endif
    __RSDictionaryAddEnvWithCString(environment, "HOME");
    __RSDictionaryAddEnvWithCString(environment, "LOGNAME");
#if DEPLOYMENT_TARGET_MACOSX || DEPLOYMENT_TARGET_IPHONEOS
    __RSDictionaryAddEnvWithCString(environment, "NSUnbufferedIO");
#endif
    __RSDictionaryAddEnvWithCString(environment, "PATH");
    __RSDictionaryAddEnvWithCString(environment, "PWD");
    __RSDictionaryAddEnvWithCString(environment, "SHELL");
    __RSDictionaryAddEnvWithCString(environment, "SSH_AUTH_SOCK");
    __RSDictionaryAddEnvWithCString(environment, "TMPDIR");
    __RSDictionaryAddEnvWithCString(environment, "USER");
#if DEPLOYMENT_TARGET_MACOSX || DEPLOYMENT_TARGET_IPHONEOS
    __RSDictionaryAddEnvWithCString(environment, "__CF_USER_TEXT_ENCODING");
    __RSDictionaryAddEnvWithCString(environment, "__XCODE_BUILT_PRODUCTS_DIR_PATHS");
    __RSDictionaryAddEnvWithCString(environment, "__XPC_DYLD_FRAMEWORK_PATH");
    __RSDictionaryAddEnvWithCString(environment, "__XPC_DYLD_LIBRARY_PATH");
#endif
    __RSProcessInfoSetObjectForKey(processInfo, __RSProcessInfoEnvironment, environment);
    RSRelease(environment);
    __RSRuntimeSetInstanceSpecial(environment, YES);    // now environment retain count = 1
    return environment;
}

RSInline RSStringRef __RSProcessInfoGetProgramName(RSProcessInfoRef processInfo)
{
    RSStringRef name = __RSProcessInfoObjectForKey(processInfo, __RSProcessInfoProgramName);
    if (name) return name;
    
    name = RSStringCreateWithCString(RSAllocatorSystemDefault, getprogname(), RSStringEncodingUTF8);
    if (name)
    {
        __RSRuntimeSetInstanceSpecial(name, YES);
        __RSProcessInfoSetObjectForKey(processInfo, __RSProcessInfoProgramName, name);
    }
    return name;
}

static BOOL __RSProcessInfoUpdateMachineType(RSProcessInfoRef processInfo, RSStringRef key);
static BOOL __RSProcessInfoUpdateMachineUUID(RSProcessInfoRef processInfo, RSStringRef key);

static BOOL __RSProcessInfoUpdateVersion(RSProcessInfoRef processInfo, RSStringRef key);
static BOOL __RSProcessInfoUpdateMemory(RSProcessInfoRef processInfo, RSStringRef key);
static BOOL __RSProcessInfoUpdateProcessor(RSProcessInfoRef processInfo, RSStringRef key);
static BOOL __RSProcessInfoUpdateDisk(RSProcessInfoRef processInfo);

static BOOL __RSProcessInfoUpdate(RSMutableDictionaryRef processInfo, RSStringRef lookupKey, RSStringRef key);
static BOOL __RSProcessInfoUpdateKeys(RSProcessInfoRef processInfo, RSArrayRef lookupKeys, RSArrayRef description, RSStringRef key);

#if DEPLOYMENT_TARGET_MACOSX
#include <sys/sysctl.h>
#include "RSProcessInfo/Darwin/RSProcessInfoKey.h"
#include <dlfcn.h>
#include <sys/utsname.h>
RSInline int ____RSProcessInfoUpdate(const char *key, char buf[], size_t *size)
{
    return sysctlbyname(key, buf, size, NULL, 0);
}

RSInline RSTypeRef ___RSProcessInfoCreateUpdate(RSStringRef key)
{
    RSUInteger size = 0;
    RSCBuffer ckey = nil;
    RSTypeRef value = nil;
    if (!key || key == (RSStringRef)RSNil) return nil;
    if (kSuccess == ____RSProcessInfoUpdate(ckey = RSStringGetCStringPtr(key, RSStringEncodingMacRoman), nil, &size))
    {
        void *buf = RSAllocatorAllocate(RSAllocatorSystemDefault, size);
        if (kSuccess == ____RSProcessInfoUpdate(ckey, buf, &size))
        {
            if (!RSStringHasSuffix(key, RSSTR("size")))
                value = RSStringCreateWithCString(RSAllocatorSystemDefault, buf, RSStringEncodingUTF8);
            else
            {
                if (size == sizeof(unsigned long long))
                    value = RSStringCreateWithFormat(RSAllocatorSystemDefault, RSSTR("%lld"), *(unsigned long long*)buf);
                else if (size == sizeof(unsigned long))
                    value = RSStringCreateWithFormat(RSAllocatorSystemDefault, RSSTR("%ld"), *(unsigned long*)buf);
                else if (size == sizeof(double))
                    value = RSStringCreateWithFormat(RSAllocatorSystemDefault, RSSTR("%f"), *(float*)buf);
            }
        }
        RSAllocatorDeallocate(RSAllocatorSystemDefault, buf);
    }
    return value;
}

static BOOL __RSProcessInfoUpdateMachineType(RSProcessInfoRef processInfo, RSStringRef key /*__RSProcessInfoMachine*/)
{
    BOOL result = NO;
    if (__RSProcessInfoObjectForKey(processInfo, key)) return result = YES;
    RSMutableArrayRef keys = RSArrayCreateMutable(RSAllocatorSystemDefault, 0);
    RSMutableArrayRef descriptions = RSArrayCreateMutable(RSAllocatorSystemDefault, 0);
    RSRetain(key);
    RSArrayAddObject(keys, __RSProcessInfo_hw_model);
    RSArrayAddObject(descriptions, __RSProcessInfoMachineType);
    
//    RSArrayAddObject(keys, nil);
//    RSArrayAddObject(descriptions, __RSProcessInfoMachineSerinalNumber);
    
    result = __RSProcessInfoUpdateKeys(processInfo, keys, descriptions, key);
    RSMutableDictionaryRef dict = (RSMutableDictionaryRef)__RSProcessInfoObjectForKey(processInfo, key);
    __RSRuntimeSetInstanceSpecial(dict, YES);
    // machine Serinal Number
    
    void *handle = __RSProcessInfoGetHandle(processInfo);
    if (handle)
    {
        void *func = _RSGetImplementAddress(handle, "__RSProcessInfoGetSerinalNumber");
        RSStringRef serinal = func ? ((RSStringRef (*)())func)() : RSSTR("Null");
        RSDictionarySetValue(dict, __RSProcessInfoMachineSerinalNumber, serinal);
        RSRelease(serinal);
    }
    
    RSRelease(key);
    RSRelease(descriptions);
    RSRelease(keys);
    return result;
}

static BOOL __RSProcessInfoUpdateVersion(RSProcessInfoRef processInfo, RSStringRef key /*__RSProcessInfoOperatingSystemVersion*/)
{
    BOOL result = NO;
    if (__RSProcessInfoObjectForKey(processInfo, key)) return result = YES;
    RSMutableArrayRef keys = RSArrayCreateMutable(RSAllocatorSystemDefault, 0);
    RSMutableArrayRef descriptions = RSArrayCreateMutable(RSAllocatorSystemDefault, 0);
    RSRetain(key);
    RSArrayAddObject(keys, __RSProcessInfo_kern_ostype);
    RSArrayAddObject(descriptions, __RSProcessInfoKernelType);
    
    RSArrayAddObject(keys, __RSProcessInfo_kern_osversion);
    RSArrayAddObject(descriptions, __RSProcessInfoKernelVersion);
    
    RSArrayAddObject(keys, __RSProcessInfo_kern_osrelease);
    RSArrayAddObject(descriptions, __RSProcessInfoKernelRelease);
    
    RSArrayAddObject(keys, __RSProcessInfo_kern_uuid);
    RSArrayAddObject(descriptions, __RSProcessInfoKernelUUID);
    
    result = __RSProcessInfoUpdateKeys(processInfo, keys, descriptions, key);
    RSMutableDictionaryRef dict = (RSMutableDictionaryRef)__RSProcessInfoObjectForKey(processInfo, key);
    __RSRuntimeSetInstanceSpecial(dict, YES);
    
    RS_CONST_STRING_DECL(__RSSystemVersionInfoPlistPath, "/System/Library/CoreServices/SystemVersion.plist");
    RSDictionaryRef sysVer = RSDictionaryCreateWithContentOfPath(RSAllocatorSystemDefault, __RSSystemVersionInfoPlistPath);
    RS_CONST_STRING_DECL(ProductUserVisibleVersion, "ProductUserVisibleVersion");
    RSStringRef userVisibleVersion = RSDictionaryGetValue(sysVer, ProductUserVisibleVersion);
    RSDictionarySetValue(dict, __RSProcessInfoHumenReadableOperatingSystemVersion, userVisibleVersion);
    RSRelease(sysVer);
    
    RSRelease(key);
    RSRelease(descriptions);
    RSRelease(keys);
    return result;
}

static BOOL __RSProcessInfoUpdateMemory(RSProcessInfoRef processInfo, RSStringRef key /*__RSProcessInfoMemory*/)
{
    BOOL result = NO;
    if (__RSProcessInfoObjectForKey(processInfo, key)) return YES;
    
    RSMutableArrayRef keys = RSArrayCreateMutable(RSAllocatorSystemDefault, 0);
    RSMutableArrayRef descriptions = RSArrayCreateMutable(RSAllocatorSystemDefault, 0);
    RSRetain(key);
    
    RSArrayAddObject(keys, __RSProcessInfo_hw_memsize);
    RSArrayAddObject(descriptions, __RSProcessInfoMemoryCapacity);
    RSArrayAddObject(keys, __RSProcessInfo_hw_l1dcachesize);
    RSArrayAddObject(descriptions, __RSProcessInfoMemoryCache1);
    RSArrayAddObject(keys, __RSProcessInfo_hw_l2cachesize);
    RSArrayAddObject(descriptions, __RSProcessInfoMemoryCache2);
    RSArrayAddObject(keys, __RSProcessInfo_hw_l3cachesize);
    RSArrayAddObject(descriptions, __RSProcessInfoMemoryCache3);
    
    result = __RSProcessInfoUpdateKeys(processInfo, keys, descriptions, key);
    RSMutableDictionaryRef dict = (RSMutableDictionaryRef)__RSProcessInfoObjectForKey(processInfo, key);
    __RSRuntimeSetInstanceSpecial(dict, YES);
    
    RSRelease(key);
    RSRelease(descriptions);
    RSRelease(keys);
    
    return result;
}

static BOOL __RSProcessInfoUpdateProcessor(RSProcessInfoRef processInfo, RSStringRef key)
{
    BOOL result = NO;
    if (__RSProcessInfoObjectForKey(processInfo, key)) return YES;
    
    RSMutableArrayRef keys = RSArrayCreateMutable(RSAllocatorSystemDefault, 0);
    RSMutableArrayRef descriptions = RSArrayCreateMutable(RSAllocatorSystemDefault, 0);
    RSRetain(key);
    
    RSArrayAddObject(keys, __RSProcessInfo_hw_activecpu);
    RSArrayAddObject(descriptions, __RSProcessInfoProcessorCount);
    RSArrayAddObject(keys, __RSProcessInfo_machdep_cpu_brand_string);
    RSArrayAddObject(descriptions, __RSProcessInfoProcessorBrand);
    
    result = __RSProcessInfoUpdateKeys(processInfo, keys, descriptions, key);
    RSMutableDictionaryRef dict = (RSMutableDictionaryRef)__RSProcessInfoObjectForKey(processInfo, key);
    __RSRuntimeSetInstanceSpecial(dict, YES);
    
    RSRelease(key);
    RSRelease(descriptions);
    RSRelease(keys);
    
    return result;
}
#endif

static BOOL __RSProcessInfoUpdate(RSMutableDictionaryRef processInfo, RSStringRef lookupKey, RSStringRef key)
{
    RSStringRef value = nil;
    value = ___RSProcessInfoCreateUpdate(lookupKey);
    if (value)
    {
        RSDictionarySetValue(processInfo, key, value);
        RSRelease(value);
    }
    return value ? YES : NO;
}

static BOOL __RSProcessInfoUpdateKeys(RSProcessInfoRef processInfo, RSArrayRef lookupKeys, RSArrayRef description, RSStringRef key)
{
    if (key == nil || lookupKeys == nil) return NO;
    if (description == nil) description = lookupKeys;
    const RSUInteger keyCount = RSArrayGetCount(lookupKeys);
    if (keyCount > RSArrayGetCount(description)) return NO;
    RSUInteger sucessCount = 0;
    RSMutableDictionaryRef dict = RSDictionaryCreateMutable(RSAllocatorSystemDefault, 0, RSDictionaryRSTypeContext);
    if (dict)
    {
        for (RSUInteger idx = 0; idx < keyCount; idx++)
        {
            sucessCount += __RSProcessInfoUpdate(dict, RSArrayObjectAtIndex(lookupKeys, idx), RSArrayObjectAtIndex(description, idx));
        }
        __RSProcessInfoSetObjectForKey(processInfo, key, dict);
        RSRelease(dict);
    }
    return sucessCount == keyCount;
}

RSExport RSProcessInfoRef RSProcessInfoGetDefault()
{
    if (__RSProcessInfoGlobal) return __RSProcessInfoGlobal;
    RSProcessInfoRef pi = __RSProcessInfoCreateInstance(RSAllocatorSystemDefault, nil);
    RSSpinLockLock(&__RSProcessInfoLock);
    __RSProcessInfoGlobal = pi;
    __RSRuntimeSetInstanceSpecial(__RSProcessInfoGlobal, YES);
    RSSpinLockUnlock(&__RSProcessInfoLock);
    return __RSProcessInfoGlobal;
}

RSExport RSDictionaryRef RSProcessInfoGetEnvironment(RSProcessInfoRef processInfo)
{
    __RSGenericValidInstance(processInfo, _RSProcessInfoTypeID);
    return __RSProcessInfoGetEnvironment(processInfo);
}

RSExport RSStringRef RSProcessInfoGetProgramName(RSProcessInfoRef processInfo)
{
    __RSGenericValidInstance(processInfo, _RSProcessInfoTypeID);
    return __RSProcessInfoGetProgramName(processInfo);
}

RSExport RSStringRef RSProcessInfoGetMachineType(RSProcessInfoRef processInfo)
{
    __RSGenericValidInstance(processInfo, _RSProcessInfoTypeID);
    __RSProcessInfoUpdateMachineType(processInfo, __RSProcessInfoMachine);
    return RSDictionaryGetValue(__RSProcessInfoObjectForKey(processInfo, __RSProcessInfoMachine), __RSProcessInfoMachineType);
}

RSExport RSStringRef RSProcessInfoGetMachineSerinalNumber(RSProcessInfoRef processInfo)
{
    __RSGenericValidInstance(processInfo, _RSProcessInfoTypeID);
    __RSProcessInfoUpdateMachineType(processInfo, __RSProcessInfoMachine);
    return RSDictionaryGetValue(__RSProcessInfoObjectForKey(processInfo, __RSProcessInfoMachine), __RSProcessInfoMachineSerinalNumber);
}

RSExport RSStringRef RSProcessInfoGetOperatingSystemVersion(RSProcessInfoRef processInfo)
{
    __RSGenericValidInstance(processInfo, _RSProcessInfoTypeID);
    __RSProcessInfoUpdateVersion(processInfo, __RSProcessInfoOperatingSystemVersion);
    return RSDictionaryGetValue(__RSProcessInfoObjectForKey(processInfo, __RSProcessInfoOperatingSystemVersion), __RSProcessInfoHumenReadableOperatingSystemVersion);
}

RSExport RSStringRef RSProcessInfoGetKernelBuildVersion(RSProcessInfoRef processInfo)
{
    __RSGenericValidInstance(processInfo, _RSProcessInfoTypeID);
    __RSProcessInfoUpdateVersion(processInfo, __RSProcessInfoOperatingSystemVersion);
    return RSDictionaryGetValue(__RSProcessInfoObjectForKey(processInfo, __RSProcessInfoOperatingSystemVersion), __RSProcessInfoKernelVersion);
}

RSExport RSStringRef RSProcessInfoGetKernelTypeName(RSProcessInfoRef processInfo)
{
    __RSGenericValidInstance(processInfo, _RSProcessInfoTypeID);
    __RSProcessInfoUpdateVersion(processInfo, __RSProcessInfoOperatingSystemVersion);
    return RSDictionaryGetValue(__RSProcessInfoObjectForKey(processInfo, __RSProcessInfoOperatingSystemVersion), __RSProcessInfoKernelType);
}

RSExport RSStringRef RSProcessInfoGetKernelUUID(RSProcessInfoRef processInfo)
{
    __RSGenericValidInstance(processInfo, _RSProcessInfoTypeID);
    __RSProcessInfoUpdateVersion(processInfo, __RSProcessInfoOperatingSystemVersion);
    return RSDictionaryGetValue(__RSProcessInfoObjectForKey(processInfo, __RSProcessInfoOperatingSystemVersion), __RSProcessInfoKernelUUID);
}

RSExport RSStringRef RSProcessInfoGetKernelReleaseVersion(RSProcessInfoRef processInfo)
{
    __RSGenericValidInstance(processInfo, _RSProcessInfoTypeID);
    __RSProcessInfoUpdateVersion(processInfo, __RSProcessInfoOperatingSystemVersion);
    return RSDictionaryGetValue(__RSProcessInfoObjectForKey(processInfo, __RSProcessInfoOperatingSystemVersion), __RSProcessInfoKernelRelease);
}

RSExport unsigned long long RSProcessInfoGetMemorySize(RSProcessInfoRef processInfo)
{
    __RSGenericValidInstance(processInfo, _RSProcessInfoTypeID);
    __RSProcessInfoUpdateMemory(processInfo, __RSProcessInfoMemory);
    return RSStringUnsignedLongLongValue(RSDictionaryGetValue(__RSProcessInfoObjectForKey(processInfo, __RSProcessInfoMemory), __RSProcessInfoMemoryCapacity));
}

RSExport unsigned long long RSProcessInfoGetCache1Size(RSProcessInfoRef processInfo)
{
    __RSGenericValidInstance(processInfo, _RSProcessInfoTypeID);
    __RSProcessInfoUpdateMemory(processInfo, __RSProcessInfoMemory);
    return RSStringUnsignedLongLongValue(RSDictionaryGetValue(__RSProcessInfoObjectForKey(processInfo, __RSProcessInfoMemory), __RSProcessInfoMemoryCache1));
}

RSExport unsigned long long RSProcessInfoGetCache2Size(RSProcessInfoRef processInfo)
{
    __RSGenericValidInstance(processInfo, _RSProcessInfoTypeID);
    __RSProcessInfoUpdateMemory(processInfo, __RSProcessInfoMemory);
    return RSStringUnsignedLongLongValue(RSDictionaryGetValue(__RSProcessInfoObjectForKey(processInfo, __RSProcessInfoMemory), __RSProcessInfoMemoryCache2));
}

RSExport unsigned long long RSProcessInfoGetCache3Size(RSProcessInfoRef processInfo)
{
    __RSGenericValidInstance(processInfo, _RSProcessInfoTypeID);
    __RSProcessInfoUpdateMemory(processInfo, __RSProcessInfoMemory);
    return RSStringUnsignedLongLongValue(RSDictionaryGetValue(__RSProcessInfoObjectForKey(processInfo, __RSProcessInfoMemory), __RSProcessInfoMemoryCache3));
}

RSExport unsigned long long RSProcessInfoGetProcessorCount(RSProcessInfoRef processInfo)
{
    __RSGenericValidInstance(processInfo, _RSProcessInfoTypeID);
    __RSProcessInfoUpdateProcessor(processInfo, __RSProcessInfoProcessor);
    return RSStringUnsignedLongLongValue(RSDictionaryGetValue(__RSProcessInfoObjectForKey(processInfo, __RSProcessInfoProcessor), __RSProcessInfoProcessorCount));
}

RSExport RSStringRef RSProcessInfoGetProcessorDescription(RSProcessInfoRef processInfo)
{
    __RSGenericValidInstance(processInfo, _RSProcessInfoTypeID);
    __RSProcessInfoUpdateProcessor(processInfo, __RSProcessInfoProcessor);
    return RSDictionaryGetValue(__RSProcessInfoObjectForKey(processInfo, __RSProcessInfoProcessor), __RSProcessInfoProcessorBrand);
}

