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
typedef struct __RSRunLoopTimer* RSRunLoopTimerRef RS_AVAILABLE(0_3);
typedef struct __RSRunLoopObserver * RSRunLoopObserverRef RS_AVAILABLE(0_3);


typedef struct {
    RSIndex	version;
    void *	info;
    const void *(*retain)(const void *info);
    void	(*release)(const void *info);
    RSStringRef	(*description)(const void *info);
    BOOL	(*equal)(const void *info1, const void *info2);
    RSHashCode	(*hash)(const void *info);
    void	(*schedule)(void *info, RSRunLoopRef rl, RSStringRef mode);
    void	(*cancel)(void *info, RSRunLoopRef rl, RSStringRef mode);
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

RSExport RSStringRef RSRunLoopModeName(RSRunLoopRef rl) RS_AVAILABLE(0_0);    // get the name of the RSRunLoop with retain

#if RS_BLOCKS_AVAILABLE
RSExport void RSPerformBlockInBackGround(void (^perform)()) RS_AVAILABLE(0_3);
RSExport void RSPerformBlockOnMainThread(void (^perform)()) RS_AVAILABLE(0_3);
RSExport void RSPerformBlockInBackGroundWaitUntilDone(void (^perform)()) RS_AVAILABLE(0_3);
RSExport void RSPerformBlockOnMainThreadWaitUntilDone(void (^perform)()) RS_AVAILABLE(0_3);
RSExport void RSPerformBlockAfterDelay(RSTimeInterval delayTime, void (^perform)()) RS_AVAILABLE(0_3);
RSExport void RSPerformBlockRepeat(RSIndex performCount, void (^perform)(RSIndex idx)) RS_AVAILABLE(0_3);
RSExport void RSPerformBlockRepeatWithFlags(RSIndex performCount, RSPerformBlockOptionFlags performFlags, void (^perform)(RSIndex idx)) RS_AVAILABLE(0_3);
#endif
RSExport void RSRunLoopRun() RS_AVAILABLE(0_0);   // start the current RSRunLoop, it will block until its work finished.

RSExport RSRunLoopSourceRef RSRunLoopGetSourceWithInformation(RSRunLoopRef runloop, RSRunLoopSourceContext* context) RS_AVAILABLE(0_0);

RSExport RSTypeID RSRunLoopSourceGetTypeID() RS_AVAILABLE(0_0);

RSExport RSRunLoopSourceRef RSRunLoopSourceCreate(RSAllocatorRef allocator, RSIndex order, RSRunLoopSourceContext* context) RS_AVAILABLE(0_0);
RSExport BOOL RSRunLoopSourceIsValid(RSRunLoopSourceRef source) RS_AVAILABLE(0_0);

#include <RSCoreFoundation/RSBase.h>
#include <RSCoreFoundation/RSArray.h>
#include <RSCoreFoundation/RSDate.h>
#include <RSCoreFoundation/RSString.h>
#if (TARGET_OS_MAC && !(TARGET_OS_EMBEDDED || TARGET_OS_IPHONE)) || (TARGET_OS_EMBEDDED || TARGET_OS_IPHONE)
#include <mach/port.h>
#endif
    
RSExport RSTypeID RSRunLoopGetTypeID() RS_AVAILABLE(0_0);

RSExport RSRunLoopRef RSRunLoopGetCurrent() RS_AVAILABLE(0_0);   // get the RSRunLoop for the current thread with retain
RSExport RSRunLoopRef RSRunLoopGetMain() RS_AVAILABLE(0_0);      // get the RSRunLoop for the main thread with retain

RSExport RSStringRef RSRunLoopCopyCurrentMode(RSRunLoopRef rl) RS_AVAILABLE(0_4);

RSExport RSArrayRef RSRunLoopCopyAllModes(RSRunLoopRef rl) RS_AVAILABLE(0_4);

RSExport void RSRunLoopAddCommonMode(RSRunLoopRef rl, RSStringRef mode) RS_AVAILABLE(0_4);

RSExport RSAbsoluteTime RSRunLoopGetNextTimerFireDate(RSRunLoopRef rl, RSStringRef mode) RS_AVAILABLE(0_4);

RSExport void RSRunLoopRun(void) RS_AVAILABLE(0_4);
RSExport RSBit32 RSRunLoopRunInMode(RSStringRef mode, RSTimeInterval seconds, BOOL returnAfterSourceHandled) RS_AVAILABLE(0_4);
RSExport BOOL RSRunLoopIsWaiting(RSRunLoopRef rl) RS_AVAILABLE(0_0);
RSExport void RSRunLoopWakeUp(RSRunLoopRef rl) RS_AVAILABLE(0_0);
RSExport void RSRunLoopStop(RSRunLoopRef rl) RS_AVAILABLE(0_0);
    
#if __BLOCKS__
RSExport void RSRunLoopPerformBlock(RSRunLoopRef rl, RSTypeRef mode, void (^block)(void)) RS_AVAILABLE(0_4);
#endif

RSExport BOOL RSRunLoopContainsSource(RSRunLoopRef runloop, RSRunLoopSourceRef source, RSStringRef mode) RS_AVAILABLE(0_0);
RSExport void RSRunLoopAddSource(RSRunLoopRef runloop, RSRunLoopSourceRef source, RSStringRef mode) RS_AVAILABLE(0_0);     // add the source to the run loop
RSExport void RSRunLoopRemoveSource(RSRunLoopRef runloop, RSRunLoopSourceRef source, RSStringRef mode) RS_AVAILABLE(0_0);
    
RSExport BOOL RSRunLoopContainsObserver(RSRunLoopRef rl, RSRunLoopObserverRef observer, RSStringRef mode) RS_AVAILABLE(0_4);
RSExport void RSRunLoopAddObserver(RSRunLoopRef rl, RSRunLoopObserverRef observer, RSStringRef mode) RS_AVAILABLE(0_4);
RSExport void RSRunLoopRemoveObserver(RSRunLoopRef rl, RSRunLoopObserverRef observer, RSStringRef mode) RS_AVAILABLE(0_4);

RSExport BOOL RSRunLoopContainsTimer(RSRunLoopRef rl, RSRunLoopTimerRef timer, RSStringRef mode) RS_AVAILABLE(0_4);
RSExport void RSRunLoopAddTimer(RSRunLoopRef rl, RSRunLoopTimerRef timer, RSStringRef mode) RS_AVAILABLE(0_4);
RSExport void RSRunLoopRemoveTimer(RSRunLoopRef rl, RSRunLoopTimerRef timer, RSStringRef mode) RS_AVAILABLE(0_4);
    
typedef struct {
    RSIndex	version;
    void *	info;
    const void *(*retain)(const void *info);
    void	(*release)(const void *info);
    RSStringRef	(*description)(const void *info);
    BOOL	(*equal)(const void *info1, const void *info2);
    RSHashCode	(*hash)(const void *info);
#if (TARGET_OS_MAC && !(TARGET_OS_EMBEDDED || TARGET_OS_IPHONE)) || (TARGET_OS_EMBEDDED || TARGET_OS_IPHONE)
    mach_port_t	(*getPort)(void *info);
    void *	(*perform)(void *msg, RSIndex size, RSAllocatorRef allocator, void *info);
#else
    void *	(*getPort)(void *info);
    void	(*perform)(void *info);
#endif
} RSRunLoopSourceContext1 RS_AVAILABLE(0_4);
    
RSExport RSTypeID RSRunLoopSourceGetTypeID(void) RS_AVAILABLE(0_4);

RSExport RSRunLoopSourceRef RSRunLoopSourceCreate(RSAllocatorRef allocator, RSIndex order, RSRunLoopSourceContext *context) RS_AVAILABLE(0_4);

RSExport RSIndex RSRunLoopSourceGetOrder(RSRunLoopSourceRef source) RS_AVAILABLE(0_4);
RSExport void RSRunLoopSourceInvalidate(RSRunLoopSourceRef source) RS_AVAILABLE(0_4);
RSExport BOOL RSRunLoopSourceIsValid(RSRunLoopSourceRef source) RS_AVAILABLE(0_4);
RSExport void RSRunLoopSourceGetContext(RSRunLoopSourceRef source, RSRunLoopSourceContext *context) RS_AVAILABLE(0_4);
RSExport void RSRunLoopSourceSignal(RSRunLoopSourceRef source) RS_AVAILABLE(0_4);

typedef struct {
    RSIndex	version;
    void *	info;
    const void *(*retain)(const void *info);
    void	(*release)(const void *info);
    RSStringRef	(*description)(const void *info);
} RSRunLoopObserverContext RS_AVAILABLE(0_4);

typedef void (*RSRunLoopObserverCallBack)(RSRunLoopObserverRef observer, RSRunLoopActivity activity, void *info) RS_AVAILABLE(0_4);

RSExport RSTypeID RSRunLoopObserverGetTypeID(void) RS_AVAILABLE(0_4);

RSExport RSRunLoopObserverRef RSRunLoopObserverCreate(RSAllocatorRef allocator, RSOptionFlags activities, BOOL repeats, RSIndex order, RSRunLoopObserverCallBack callout, RSRunLoopObserverContext *context) RS_AVAILABLE(0_4);
#if __BLOCKS__
RSExport RSRunLoopObserverRef RSRunLoopObserverCreateWithHandler(RSAllocatorRef allocator, RSOptionFlags activities, BOOL repeats, RSIndex order, void (^block) (RSRunLoopObserverRef observer, RSRunLoopActivity activity)) RS_AVAILABLE(0_4);
#endif

RSExport RSOptionFlags RSRunLoopObserverGetActivities(RSRunLoopObserverRef observer) RS_AVAILABLE(0_4);
RSExport BOOL RSRunLoopObserverDoesRepeat(RSRunLoopObserverRef observer) RS_AVAILABLE(0_4);
RSExport RSIndex RSRunLoopObserverGetOrder(RSRunLoopObserverRef observer) RS_AVAILABLE(0_4);
RSExport void RSRunLoopObserverInvalidate(RSRunLoopObserverRef observer) RS_AVAILABLE(0_4);
RSExport BOOL RSRunLoopObserverIsValid(RSRunLoopObserverRef observer) RS_AVAILABLE(0_4);
RSExport void RSRunLoopObserverGetContext(RSRunLoopObserverRef observer, RSRunLoopObserverContext *context) RS_AVAILABLE(0_4);

typedef struct {
    RSIndex	version;
    void *	info;
    const void *(*retain)(const void *info);
    void	(*release)(const void *info);
    RSStringRef	(*description)(const void *info);
} RSRunLoopTimerContext RS_AVAILABLE(0_4);

typedef void (*RSRunLoopTimerCallBack)(RSRunLoopTimerRef timer, void *info) RS_AVAILABLE(0_4);

RSExport RSTypeID RSRunLoopTimerGetTypeID(void) RS_AVAILABLE(0_4);

RSExport RSRunLoopTimerRef RSRunLoopTimerCreate(RSAllocatorRef allocator, RSAbsoluteTime fireDate, RSTimeInterval interval, RSOptionFlags flags, RSIndex order, RSRunLoopTimerCallBack callout, RSRunLoopTimerContext *context) RS_AVAILABLE(0_4);
#if __BLOCKS__
RSExport RSRunLoopTimerRef RSRunLoopTimerCreateWithHandler(RSAllocatorRef allocator, RSAbsoluteTime fireDate, RSTimeInterval interval, RSOptionFlags flags, RSIndex order, void (^block) (RSRunLoopTimerRef timer)) RS_AVAILABLE(0_4);
#endif

RSExport RSAbsoluteTime RSRunLoopTimerGetNextFireDate(RSRunLoopTimerRef timer) RS_AVAILABLE(0_4);
RSExport void RSRunLoopTimerSetNextFireDate(RSRunLoopTimerRef timer, RSAbsoluteTime fireDate) RS_AVAILABLE(0_4);
RSExport RSTimeInterval RSRunLoopTimerGetInterval(RSRunLoopTimerRef timer) RS_AVAILABLE(0_4);
RSExport BOOL RSRunLoopTimerDoesRepeat(RSRunLoopTimerRef timer) RS_AVAILABLE(0_4);
RSExport RSIndex RSRunLoopTimerGetOrder(RSRunLoopTimerRef timer) RS_AVAILABLE(0_4);
RSExport void RSRunLoopTimerInvalidate(RSRunLoopTimerRef timer) RS_AVAILABLE(0_4);
RSExport BOOL RSRunLoopTimerIsValid(RSRunLoopTimerRef timer) RS_AVAILABLE(0_4);
RSExport void RSRunLoopTimerGetContext(RSRunLoopTimerRef timer, RSRunLoopTimerContext *context) RS_AVAILABLE(0_4);

// Setting a tolerance for a timer allows it to fire later than the scheduled fire date, improving the ability of the system to optimize for increased power savings and responsiveness. The timer may fire at any time between its scheduled fire date and the scheduled fire date plus the tolerance. The timer will not fire before the scheduled fire date. For repeating timers, the next fire date is calculated from the original fire date regardless of tolerance applied at individual fire times, to avoid drift. The default value is zero, which means no additional tolerance is applied. The system reserves the right to apply a small amount of tolerance to certain timers regardless of the value of this property.
// As the user of the timer, you will have the best idea of what an appropriate tolerance for a timer may be. A general rule of thumb, though, is to set the tolerance to at least 10% of the interval, for a repeating timer. Even a small amount of tolerance will have a significant positive impact on the power usage of your application. The system may put a maximum value of the tolerance.
RSExport RSTimeInterval RSRunLoopTimerGetTolerance(RSRunLoopTimerRef timer) RS_AVAILABLE(0_4);
RSExport void RSRunLoopTimerSetTolerance(RSRunLoopTimerRef timer, RSTimeInterval tolerance) RS_AVAILABLE(0_4);

    


RSExport RSStringRef const RSRunLoopDefault RS_AVAILABLE(0_0);   // one source one thread mode.
RSExport RSStringRef const RSRunLoopQueue RS_AVAILABLE(0_1);     // all sources run on one thread.
RSExport RSStringRef const RSRunLoopMain RS_AVAILABLE(0_1);
RSExport RSStringRef const RSRunLoopDefaultMode RS_AVAILABLE(0_3);
RSExport RSStringRef const RSRunLoopCommonModes RS_AVAILABLE(0_3);
RS_EXTERN_C_END
#endif
