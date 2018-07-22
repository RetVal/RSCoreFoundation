//
//  RSPrivateOSCpu.c
//  RSCoreFoundation
//
//  Created by RetVal on 11/6/12.
//  Copyright (c) 2012 RetVal. All rights reserved.
//

#include <sys/sysctl.h>

#if DEPLOYMENT_TARGET_MACOSX || DEPLOYMENT_TARGET_EMBEDDED || DEPLOYMENT_TARGET_EMBEDDED_MINI
#include <unistd.h>
#include <sys/uio.h>
#include <mach/mach.h>
#include <pthread.h>
#include <mach-o/loader.h>
#include <mach-o/dyld.h>
#include <crt_externs.h>
#include <dlfcn.h>
#include <vproc.h>
#include <sys/sysctl.h>
#include <sys/stat.h>
#include <mach/mach.h>
#include <mach/mach_vm.h>
#include <sys/mman.h>
#include <stdio.h>
#include <sys/errno.h>
#include <mach/mach_time.h>
#include <Block.h>
#endif
#if DEPLOYMENT_TARGET_LINUX || DEPLOYMENT_TARGET_FREEBSD
#include <string.h>
#include <pthread.h>
#include <sys/mman.h>
#endif

RSPrivate RSUInteger __RSActiveProcessorCount()
{
    int32_t pcnt;
#if DEPLOYMENT_TARGET_WINDOWS
    SYSTEM_INFO sysInfo;
    GetSystemInfo(&sysInfo);
    DWORD_PTR activeProcessorMask = sysInfo.dwActiveProcessorMask;
    // assumes sizeof(DWORD_PTR) is 64 bits or less
    uint64_t v = activeProcessorMask;
    v = v - ((v >> 1) & 0x5555555555555555ULL);
    v = (v & 0x3333333333333333ULL) + ((v >> 2) & 0x3333333333333333ULL);
    v = (v + (v >> 4)) & 0xf0f0f0f0f0f0f0fULL;
    pcnt = (v * 0x0101010101010101ULL) >> ((sizeof(v) - 1) * 8);
#elif DEPLOYMENT_TARGET_MACOSX || DEPLOYMENT_TARGET_EMBEDDED || DEPLOYMENT_TARGET_EMBEDDED_MINI
    int32_t mib[] = {CTL_HW, HW_AVAILCPU};
    size_t len = sizeof(pcnt);
    int32_t result = sysctl(mib, sizeof(mib) / sizeof(int32_t), &pcnt, &len, nil, 0);
    if (result != 0) {
        pcnt = 0;
    }
#else
    // Assume the worst
    pcnt = 1;
#endif
    return pcnt;
}

RSPrivate RSIndex __RSActiveProcessorFrequency()
{
    int32_t pcnt = 1;
#if DEPLOYMENT_TARGET_WINDOWS
    
#elif DEPLOYMENT_TARGET_MACOSX || DEPLOYMENT_TARGET_EMBEDDED || DEPLOYMENT_TARGET_EMBEDDED_MINI
    int32_t mib[] = {CTL_HW, HW_CPU_FREQ};
    size_t len = sizeof(pcnt);
    int32_t result = sysctl(mib, sizeof(mib) / sizeof(int32_t), &pcnt, &len, nil, 0);
    if (result != 0) {
        pcnt = 0;
    }
#else
    // Assume the worst
    pcnt = 1;
#endif
    return pcnt;
}

/*
 */

RSPrivate RSIndex __RSPhysicalMemory()
{
    int32_t pmc;
#if DEPLOYMENT_TARGET_WINDOWS
    
#elif DEPLOYMENT_TARGET_MACOSX || DEPLOYMENT_TARGET_EMBEDDED || DEPLOYMENT_TARGET_EMBEDDED_MINI
    size_t size = sizeof(int);
    //int32_t results;
    int mib[2] = {CTL_HW, HW_PHYSMEM};
    sysctl(mib, 2, &pmc, &size, nil, 0);
    
#else
    // Assume the worst
#endif
    return pmc;
}

BOOL _RSExecutableLinkedOnOrAfter(RSUInteger v)
{
    return YES;
}

#if DEPLOYMENT_TARGET_MACOSX
RSPrivate void *__RSLookupCarbonCoreFunction(const char *name) {
    static void *image = NULL;
    if (NULL == image) {
        image = dlopen("/System/Library/Frameworks/CoreServices.framework/Versions/A/Frameworks/CarbonCore.framework/Versions/A/CarbonCore", RTLD_LAZY | RTLD_LOCAL);
    }
    void *dyfunc = NULL;
    if (image) {
        dyfunc = dlsym(image, name);
    }
    return dyfunc;
}
#endif

#if DEPLOYMENT_TARGET_MACOSX || DEPLOYMENT_TARGET_EMBEDDED || DEPLOYMENT_TARGET_EMBEDDED_MINI
RSPrivate uintptr_t __RSFindPointer(uintptr_t ptr, uintptr_t start) {
    vm_map_t task = mach_task_self();
    mach_vm_address_t address = start;
    for (;;) {
        mach_vm_size_t size = 0;
        vm_region_basic_info_data_64_t info;
        mach_msg_type_number_t count = VM_REGION_BASIC_INFO_COUNT_64;
        mach_port_t object_name;
        kern_return_t ret = mach_vm_region(task, &address, &size, VM_REGION_BASIC_INFO_64, (vm_region_info_t)&info, &count, &object_name);
        if (KERN_SUCCESS != ret) break;
        boolean_t scan = (info.protection & VM_PROT_WRITE) ? 1 : 0;
        if (scan) {
            uintptr_t *addr = (uintptr_t *)((uintptr_t)address);
            uintptr_t *end = (uintptr_t *)((uintptr_t)address + (uintptr_t)size);
            while (addr < end) {
                if ((uintptr_t *)start <= addr && *addr == ptr) {
                    return (uintptr_t)addr;
                }
                addr++;
            }
        }
        address += size;
    }
    return 0;
}

RSExport void __RSDumpAllPointerLocations(uintptr_t ptr) {
    uintptr_t addr = 0;
    do {
        addr = __RSFindPointer(ptr, sizeof(void *) + addr);
        printf("%p\n", (void *)addr);
    } while (addr != 0);
}
#endif

RSPrivate BOOL __RSProcessIsRestricted() {
    return issetugid();
}

#if DEPLOYMENT_TARGET_MACOSX || DEPLOYMENT_TARGET_EMBEDDED
RSPrivate void *__RSLookupCoreServicesInternalFunction(const char *name) {
    static void *image = NULL;
    if (NULL == image) {
        image = dlopen("/System/Library/PrivateFrameworks/CoreServicesInternal.framework/CoreServicesInternal", RTLD_LAZY | RTLD_LOCAL);
    }
    void *dyfunc = NULL;
    if (image) {
        dyfunc = dlsym(image, name);
    }
    return dyfunc;
}

RSPrivate void *__RSLookupRSNetworkFunction(const char *name) {
    static void *image = NULL;
    if (NULL == image) {
        const char *path = NULL;
        if (!__RSProcessIsRestricted()) {
            path = __RSGetEnvironment("RSNETWORK_LIBRARY_PATH");
        }
        if (!path) {
            path = "/System/Library/Frameworks/RSNetwork.framework/RSNetwork";
        }
        image = dlopen(path, RTLD_LAZY | RTLD_LOCAL);
    }
    void *dyfunc = NULL;
    if (image) {
        dyfunc = dlsym(image, name);
    }
    return dyfunc;
}
#endif
