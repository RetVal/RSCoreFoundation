//
//  main.c
//  RSCoreFoundation-Test-Memory
//
//  Created by RetVal on 8/13/13.
//  Copyright (c) 2013 RetVal. All rights reserved.
//

#include <RSCoreFoundation/RSCoreFoundation.h>

#include <sys/types.h>
#include <sys/sysctl.h>
#include <sys/vmmeter.h>
#include <mach/mach_init.h>
#include <mach/mach_host.h>
#include <mach/mach_port.h>
#include <mach/mach_traps.h>
#include <mach/task_info.h>
#include <mach/thread_info.h>
#include <mach/thread_act.h>
#include <mach/vm_region.h>
#include <mach/vm_map.h>
#include <mach/task.h>

static int mem(unsigned long *resident_size, unsigned long *virtual_size)
{
    task_t task = mach_task_self();
    struct mach_task_basic_info info;
    mach_msg_type_number_t info_count = MACH_TASK_BASIC_INFO_COUNT;
    if (KERN_SUCCESS != task_info(task,
                                  MACH_TASK_BASIC_INFO, (task_info_t)&info, &info_count))
    {
        return -1;
    }
    *resident_size = info.resident_size;
    *virtual_size  = info.virtual_size;
    return 0;
}

static void logMem()
{
    unsigned long residentSize = 0, virtualSize = 0;
    if (0 == mem(&residentSize, &virtualSize))
    {
        RSLog(RSSTR("memory resident size = %8ld KB, virutal memory size = %8ld KB"), residentSize / 1024, virtualSize / 1024);
    }
    else RSShow(RSSTR("get memory info failed"));
    return;
}
extern void RSAutoreleaseShowCurrentPool();
extern void RSAllocatorLogUnitCount();
int main(int argc, const char * argv[])
{
    __block struct rusage rusage = {0};
    logMem();
    getrusage( RUSAGE_SELF, &rusage );
    RSLog(RSSTR("ru_maxrss %ld KB"), (size_t)rusage.ru_maxrss / 1024);
    RSPerformBlockInBackGroundWaitUntilDone(^{
        RSAllocatorLogUnitCount();
        RSMutableArrayRef array = RSAutorelease(RSArrayCreateMutable(RSAllocatorDefault, 0));
        for (RSUInteger idx = 0; idx < 1; idx++) {
            for (RSUInteger idx = 0; idx < 1000000; idx++) {
                RSArrayAddObject(array, RSStringWithFormat(RSSTR("%ld"), rand()));
            }
            logMem();
            getrusage( RUSAGE_SELF, &rusage );
            RSLog(RSSTR("ru_maxrss %ld KB"), (size_t)rusage.ru_maxrss / 1024);
        }
        logMem();
        RSAllocatorLogUnitCount();
        RSAutoreleaseShowCurrentPool();
        return;
    });
    RSAllocatorLogUnitCount();
    RSAutoreleaseShowCurrentPool();
    logMem();
    sleep(20);
    return 0;
}

