//
//  RSPrivateOSCpu.c
//  RSCoreFoundation
//
//  Created by RetVal on 11/6/12.
//  Copyright (c) 2012 RetVal. All rights reserved.
//

#include <sys/sysctl.h>

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
