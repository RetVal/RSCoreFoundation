//
//  RSRunLoop.h
//  RSCoreFoundation
//
//  Created by RetVal on 11/23/12.
//  Copyright (c) 2012 RetVal. All rights reserved.
//

#ifndef RSCoreFoundation_RSRunLoop_h
#define RSCoreFoundation_RSRunLoop_h
RS_EXTERN_C_BEGIN
#include <RSCoreFoundation/RSBase.h>

typedef struct __RSRunLoop* RSRunLoopRef RS_AVAILABLE(0_0);
typedef struct __RSRunLoopSource* RSRunLoopSourceRef RS_AVAILABLE(0_0);

typedef struct {
    RSIndex	version;
    void *	info;
    const void *(*retain)(const void *info);
    void	(*release)(const void *info);
    RSStringRef	(*description)(const void *info);
    BOOL	(*equal)(const void *info1, const void *info2);
    RSHashCode	(*hash)(const void *info);
    void	(*schedule)(void *info, RSRunLoopRef runloop, RSStringRef mode); // call when source add to the RSRunLoop
    void	(*cancel)(void *info, RSRunLoopRef runloop, RSStringRef mode);   // call when source remove from the RSRunLoop
    void	(*perform)(void *info);
} RSRunLoopSourceContext RS_AVAILABLE(0_0);

enum {
    RSRunLoopRunFinished = 1,
    RSRunLoopRunStopped = 2,
    RSRunLoopRunTimedOut = 3,
    RSRunLoopRunHandledSource = 4
};

/* Run Loop Observer Activities */
typedef RS_OPTIONS(RSOptionFlags, RSRunLoopActivity) {
    RSRunLoopEntry = (1UL << 0),
    RSRunLoopBeforeTimers = (1UL << 1),
    RSRunLoopBeforeSources = (1UL << 2),
    RSRunLoopBeforeWaiting = (1UL << 5),
    RSRunLoopAfterWaiting = (1UL << 6),
    RSRunLoopExit = (1UL << 7),
    RSRunLoopAllActivities = 0x0FFFFFFFU
};

typedef RS_OPTIONS(RSOptionFlags, RSPerformBlockOptionFlags) {
    RSPerformBlockDefault = 0,
    RSPerformBlockOverCommit = 2,
};

RSExport RSTypeID RSRunLoopGetTypeID() RS_AVAILABLE(0_0);

RSExport RSRunLoopRef RSRunLoopCurrentLoop() RS_AVAILABLE(0_0);   // get the RSRunLoop for the current thread with retain
RSExport RSRunLoopRef RSRunLoopMainLoop() RS_AVAILABLE(0_0);      // get the RSRunLoop for the main thread with retain

RSExport RSStringRef RSRunLoopModeName(RSRunLoopRef rl) RS_AVAILABLE(0_0);    // get the name of the RSRunLoop with retain

RSExport void RSPerformFunctionInBackGround(void (*perform)(void *info), void *info, BOOL waitUntilDone) RS_AVAILABLE(0_3);
RSExport void RSPerformFunctionOnMainThread(void (*perform)(void *info), void *info, BOOL waitUntilDone) RS_AVAILABLE(0_3);
RSExport void RSPerformFunctionAfterDelay(void (*perform)(void *info), void *info, RSTimeInterval delayTime) RS_AVAILABLE(0_3);

#if RS_BLOCKS_AVAILABLE
RSExport void RSPerformBlockInBackGround(void (^perform)()) RS_AVAILABLE(0_3);
RSExport void RSPerformBlockOnMainThread(void (^perform)()) RS_AVAILABLE(0_3);
RSExport void RSPerformBlockInBackGroundWaitUntilDone(void (^perform)()) RS_AVAILABLE(0_3);
RSExport void RSPerformBlockOnMainThreadWaitUntilDone(void (^perform)()) RS_AVAILABLE(0_3);
RSExport void RSPerformBlockAfterDelay(RSTimeInterval delayTime, void (^perform)()) RS_AVAILABLE(0_3);
RSExport void RSPerformBlockRepeat(RSIndex performCount, void (^perform)(RSIndex idx)) RS_AVAILABLE(0_3);
RSExport void RSPerformBlockRepeatWithFlags(RSIndex performCount, RSPerformBlockOptionFlags performFlags, void (^perform)(RSIndex idx)) RS_AVAILABLE(0_3);
#endif

RSExport void RSRunLoopPerformFunctionInRunLoop(RSRunLoopRef runloop, void(*perform)(void* info), void* info, RSStringRef mode) RS_AVAILABLE(0_3);
RSExport void RSRunLoopDoSourceInRunLoop(RSRunLoopRef runloop, RSRunLoopSourceRef source, RSStringRef mode) RS_AVAILABLE(0_3);
RSExport void RSRunLoopDoSourcesInRunLoop(RSRunLoopRef runloop, RSArrayRef sources, RSStringRef mode) RS_AVAILABLE(0_3);
RSExport void RSRunLoopRun() RS_AVAILABLE(0_0);   // start the current RSRunLoop, it will block until its work finished.

RSExport BOOL RSRunLoopIsWaiting(RSRunLoopRef runloop) RS_AVAILABLE(0_0);
RSExport void RSRunLoopWakeUp(RSRunLoopRef runloop) RS_AVAILABLE(0_0);
RSExport void RSRunLoopStop(RSRunLoopRef runloop) RS_AVAILABLE(0_0);

RSExport BOOL RSRunLoopContainsSource(RSRunLoopRef runloop, RSRunLoopSourceRef source, RSStringRef mode) RS_AVAILABLE(0_0);
RSExport void RSRunLoopAddSource(RSRunLoopRef runloop, RSRunLoopSourceRef source, RSStringRef mode) RS_AVAILABLE(0_0);     // add the source to the run loop
RSExport void RSRunLoopRemoveSource(RSRunLoopRef runloop, RSRunLoopSourceRef source, RSStringRef mode) RS_AVAILABLE(0_0);

RSExport RSRunLoopSourceRef RSRunLoopGetSourceWithInformation(RSRunLoopRef runloop, RSRunLoopSourceContext* context) RS_AVAILABLE(0_0);

RSExport RSTypeID RSRunLoopSourceGetTypeID() RS_AVAILABLE(0_0);

RSExport RSRunLoopSourceRef RSRunLoopSourceCreate(RSAllocatorRef allocator, RSIndex order, RSRunLoopSourceContext* context) RS_AVAILABLE(0_0);
RSExport BOOL RSRunLoopSourceIsValid(RSRunLoopSourceRef source) RS_AVAILABLE(0_0);
RSExport void RSRunLoopSourceRemoveFromRunLoop(RSRunLoopSourceRef source, RSStringRef mode) RS_AVAILABLE(0_0);

RSExport RSStringRef const RSRunLoopDefault RS_AVAILABLE(0_0);   // one source one thread mode.
RSExport RSStringRef const RSRunLoopQueue RS_AVAILABLE(0_1);     // all sources run on one thread.
RSExport RSStringRef const RSRunLoopMain RS_AVAILABLE(0_1);

RS_EXTERN_C_END
#endif
