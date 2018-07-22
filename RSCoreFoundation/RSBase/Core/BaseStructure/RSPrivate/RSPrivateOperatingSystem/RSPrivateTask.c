//
//  RSPrivateTask.c
//  RSCoreFoundation
//
//  Created by RetVal on 5/15/13.
//  Copyright (c) 2013 RetVal. All rights reserved.
//

#include "RSPrivateTask.h"
#include <mach/mach.h>
pid_t __RSGetPid()
{
#if DEPLOYMENT_TARGET_MACOSX || DEPLOYMENT_TARGET_LINUX
    return getpid();
#elif DEPLOYMENT_TARGET_WINDOWS
    return GetCurrentProcessId();
#endif
    return 0;
}

tid_t __RSGetTidWithThread(pthread_t thread)
{
#if DEPLOYMENT_TARGET_MACOSX
    return pthread_mach_thread_np(thread);
#elif DEPLOYMENT_TARGET_LINUX
    return syscall(SYS_gettid);
#elif DEPLOYMENT_TARGET_WINDOWS
    return GetCurrentThreadId();
#endif
    return 0;
}

tid_t __RSGetTid()
{
#if DEPLOYMENT_TARGET_MACOSX
    return __RSGetTidWithThread(pthread_self());
#elif DEPLOYMENT_TARGET_LINUX
    return syscall(SYS_gettid);
#elif DEPLOYMENT_TARGET_WINDOWS
    return GetCurrentThreadId();
#endif
    return 0;
}

