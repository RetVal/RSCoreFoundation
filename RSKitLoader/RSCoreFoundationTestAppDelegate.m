//
//  RSCoreFoundationTestAppDelegate.m
//  RSCoreFoundationLoader
//
//  Created by RetVal on 10/14/12.
//  Copyright (c) 2012 RetVal. All rights reserved.
//

#import "RSCoreFoundationTestAppDelegate.h"
#import <RSCoreFoundation/RSCoreFoundation.h>
#include <pthread.h>
#include <asl.h>
@interface RSCoreFoundationTestAppDelegate()

@end

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

RSExport void RSQSortArray(void **list, RSIndex count, RSIndex elementSize, RSComparisonOrder order, RSComparatorFunction comparator, void *context);

#if RS_BLOCKS_AVAILABLE
typedef RSComparisonResult (^COMPARATOR)(RSIndex, RSIndex);
#else
typedef RSComparisonResult (*COMPARATOR)(RSIndex, RSIndex);
#endif

RSExport void RSSortIndexes(RSIndex *indexBuffer, RSIndex count, RSOptionFlags opts, COMPARATOR cmp, RSComparisonOrder order);

@implementation RSCoreFoundationTestAppDelegate

- (void)applicationDidFinishLaunching:(NSNotification *)aNotification
{
    RSDictionaryWriteToFile(NULL, RSFileManagerStandardizingPath(RSSTR("~/Desktop/dict_null.plist")), RSWriteFileAutomatically);
    return;
    RSNumberRef a[] = { RSNumberWithInt(1),
                        RSNumberWithInt(9),
                        RSNumberWithInt(3),
                        RSNumberWithInt(5),
                        RSNumberWithInt(4),
                        RSNumberWithInt(8),
                        RSNumberWithInt(7),
                        RSNumberWithInt(0)};
    
    RSNumberCompare(RSNumberWithInt(1), RSNumberWithInt(2), nil);
    RSQSortArray((void **)&a, sizeof(a) / sizeof(RSNumberRef), sizeof(RSNumberRef), RSOrderedAscending, (RSComparatorFunction)RSNumberCompare, nil);
    void (^pab)(RSNumberRef a[], int n) = ^(RSNumberRef a[], int n){
        for (int i = 0; i < n; i ++) RSLog(RSSTR("%r"), a[i]);
    };
    pab(a, sizeof(a) / sizeof(RSNumberRef));
    return;
    logMem();
    RSAbsoluteTime start = RSAbsoluteTimeGetCurrent();
    RSDictionaryRef dictRSNumber = RSDictionaryCreateWithContentOfPath(RSAllocatorDefault, RSFileManagerStandardizingPath(RSSTR("~/Desktop/RSNumber.plist")));
    RSAbsoluteTime end = RSAbsoluteTimeGetCurrent();
    
    logMem();
    
    RSRelease(dictRSNumber);
    logMem();
//    RSDateRef endDate = RSDateCreate(RSAllocatorDefault, end);
//    RSDateRef startDate = RSDateCreate(RSAllocatorDefault, start);
    RSLog(RSSTR("cost is %f"), end - start);
    return;
    RSNumberRef speicific = RSNumberCreateBoolean(RSAllocatorDefault, YES);
    RSShow(speicific);
//    RSDictionarySetValue((RSMutableDictionaryRef)plistAboutInfo, _RSBundleHasSpeicificInfoKey, speicific);
    RSRelease(speicific);
    
    RSBundleRef mainBundle = RSBundleGetMainBundle();
    RSArchiverRef archiver = RSArchiverCreate(RSAllocatorDefault);
    RSDictionaryRef dict = RSBundleGetInfoDict(mainBundle);
    RSArchiverEncodeObjectForKey(archiver, RSSTR("bundle"), dict);
    RSDataWriteToFile(RSAutorelease(RSArchiverCopyData(archiver)), RSFileManagerStandardizingPath(RSSTR("~/Desktop/RSBundle.archiver.plist")), RSWriteFileAutomatically);
    RSRelease(archiver);
    return;
    RSShow(RSSTR("START"));
    for (RSUInteger idx = 0; idx < 1000000; idx++) {
        RSNumberCreate(RSAllocatorDefault, RSNumberInteger, &idx);
    }
    RSShow(RSSTR("END"));
    return ;
}

- (void)applicationWillTerminate:(NSNotification *)notification
{
}

- (BOOL)applicationShouldTerminateAfterLastWindowClosed:(NSApplication *)sender
{
    return YES;
}
@end
