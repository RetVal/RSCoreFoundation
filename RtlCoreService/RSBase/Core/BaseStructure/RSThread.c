//
//  RSThread.c
//  RSCoreFoundation
//
//  Created by RetVal on 4/9/13.
//  Copyright (c) 2013 RetVal. All rights reserved.
//

#include <RSCoreFoundation/RSDictionary.h>
#include <RSCoreFoundation/RSException.h>
#include <RSCoreFoundation/RSNumber+Extension.h>
#include <RSCoreFoundation/RSThread.h>
#include "RSPrivate/RSException/RSPrivateExceptionHandler.h"


RSPrivate void *__RSStartSimpleThread(void *func, void *arg)
{
#if DEPLOYMENT_TARGET_MACOSX || DEPLOYMENT_TARGET_EMBEDDED || DEPLOYMENT_TARGET_EMBEDDED_MINI || DEPLOYMENT_TARGET_LINUX || DEPLOYMENT_TARGET_FREEBSD
    pthread_attr_t attr;
    pthread_t tid = 0;
    pthread_attr_init(&attr);
    pthread_attr_setscope(&attr, PTHREAD_SCOPE_SYSTEM);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
//    pthread_attr_setstacksize(&attr, 60 * 1024);	// 60K stack for our internal threads is sufficient
    __RSRuntimeMemoryBarrier(); // ensure arg is fully initialized and set in memory
    pthread_create(&tid, &attr, func, arg);
    pthread_attr_destroy(&attr);
	//warning RS: we dont actually know that a pthread_t is the same size as void *
    return (void *)tid;
#elif DEPLOYMENT_TARGET_WINDOWS
    unsigned tid;
    struct _args *args = (struct _args*)RSAllocatorAllocate(RSAllocatorSystemDefault, sizeof(struct _args));
    HANDLE handle;
    args->func = func;
    args->arg = arg;
    /* The thread is created suspended, because otherwise there would be a race between the assignment below of the handle field, and it's possible use in the thread func above. */
    args->handle = (HANDLE)_beginthreadex(nil, 0, func, args, CREATE_SUSPENDED, &tid);
    handle = args->handle;
    ResumeThread(handle);
    return handle;
#endif
}

static struct ___RSExceptionHandlerPool *__tls_get_excption_pool()
{
    return nil;
}

static void __e_push_exception_handler(_exception_handler_block handler)
{
    
}

static void __e_pop_exception_handler(_exception_handler_block handler)
{
    
}

#pragma mark -
#pragma mark Thread Local Data

// If slot >= RS_TSD_MAX_SLOTS, the SPI functions will crash at nil + slot address.
// If thread data has been torn down, these functions should crash on RS_TSD_BAD_PTR + slot address.
#define RS_TSD_MAX_SLOTS 70

#if DEPLOYMENT_TARGET_MACOSX || DEPLOYMENT_TARGET_EMBEDDED || DEPLOYMENT_TARGET_EMBEDDED_MINI
#define RS_TSD_KEY 206
//#define RS_TSD_KEY 55
#endif

// Windows and Linux, not sure how many times the destructor could get called; RS_TSD_MAX_DESTRUCTOR_CALLS could be 1

#define RS_TSD_BAD_PTR ((void *)0x1000)

// Data structure to hold TSD data, cleanup functions for each
typedef struct __RSTSDTable {
    uint32_t destructorCount;
    uintptr_t data[RS_TSD_MAX_SLOTS];
    tsdDestructor destructors[RS_TSD_MAX_SLOTS];
} __RSTSDTable;

static void __RSTSDFinalize(void *arg);

static void __RSTSDSetSpecific(void *arg) {
#if DEPLOYMENT_TARGET_MACOSX || DEPLOYMENT_TARGET_EMBEDDED || DEPLOYMENT_TARGET_EMBEDDED_MINI
    pthread_setspecific(RS_TSD_KEY, arg);
#elif DEPLOYMENT_TARGET_LINUX
    pthread_setspecific(__RSTSDIndexKey, arg);
#elif DEPLOYMENT_TARGET_WINDOWS
    TlsSetValue(__RSTSDIndexKey, arg);
#endif
}

static void *__RSTSDGetSpecific() {
#if DEPLOYMENT_TARGET_MACOSX || DEPLOYMENT_TARGET_EMBEDDED || DEPLOYMENT_TARGET_EMBEDDED_MINI
    return pthread_getspecific(RS_TSD_KEY);
#elif DEPLOYMENT_TARGET_LINUX
    return pthread_getspecific(__RSTSDIndexKey);
#elif DEPLOYMENT_TARGET_WINDOWS
    return TlsGetValue(__RSTSDIndexKey);
#endif
}

static void __RSTSDFinalize(void *arg) {
    // Set our TSD so we're called again by pthreads. It will call the destructor PTHREAD_DESTRUCTOR_ITERATIONS times as long as a value is set in the thread specific data. We handle each case below.
    __RSTSDSetSpecific(arg);
    
    if (!arg || arg == RS_TSD_BAD_PTR) {
        // We've already been destroyed. The call above set the bad pointer again. Now we just return.
        return;
    }
    
    __RSTSDTable *table = (__RSTSDTable *)arg;
    table->destructorCount++;
    
    // On first calls invoke destructor. Later we destroy the data.
    // Note that invocation of the destructor may cause a value to be set again in the per-thread data slots. The destructor count and destructors are preserved.
    // This logic is basically the same as what pthreads does. We just skip the 'created' flag.
    for (int32_t i = 0; i < RS_TSD_MAX_SLOTS; i++) {
        if (table->data[i] && table->destructors[i]) {
            uintptr_t old = table->data[i];
            table->data[i] = (uintptr_t)nil;
            table->destructors[i]((void *)(old));
        }
    }
    
    if (table->destructorCount == PTHREAD_DESTRUCTOR_ITERATIONS - 1) {    // On PTHREAD_DESTRUCTOR_ITERATIONS-1 call, destroy our data
//        __RSCLog(RSLogLevelNotice, "__RSTSDGetTable dealloc table\n");
//        RSAllocatorDeallocate(RSAllocatorSystemDefault, table);
        free(table);
        // Now if the destructor is called again we will take the shortcut at the beginning of this function.
        __RSTSDSetSpecific(RS_TSD_BAD_PTR);
        return;
    }
    else if (pthread_main_np()) {
        __RSCLog(RSLogLevelNotice, "main thread dealloc");
    }
}

#if DEPLOYMENT_TARGET_MACOSX || DEPLOYMENT_TARGET_EMBEDDED || DEPLOYMENT_TARGET_EMBEDDED_MINI
extern int pthread_key_init_np(int, void (*)(void *));
#endif

// Get or initialize a thread local storage. It is created on demand.
static __RSTSDTable *__RSTSDGetTable() {
    __RSTSDTable *table = (__RSTSDTable *)__RSTSDGetSpecific();
    // Make sure we're not setting data again after destruction.
    if (table == RS_TSD_BAD_PTR) {
        return nil;
    }
    // Create table on demand
    if (!table) {
        // This memory is freed in the finalize function
//        __RSCLog(RSLogLevelNotice, "__RSTSDGetTable calloc table\n");
//        table = (__RSTSDTable *)RSAllocatorCallocate(RSAllocatorSystemDefault, 1, sizeof(__RSTSDTable));
        table = (__RSTSDTable *)calloc(1, sizeof(__RSTSDTable));   // main thread will leak this memory block(freed by system)
        // Windows and Linux have created the table already, we need to initialize it here for other platforms. On Windows, the cleanup function is called by DllMain when a thread exits. On Linux the destructor is set at init time.
#if DEPLOYMENT_TARGET_MACOSX || DEPLOYMENT_TARGET_EMBEDDED || DEPLOYMENT_TARGET_EMBEDDED_MINI
        pthread_key_init_np(RS_TSD_KEY, __RSTSDFinalize);
#endif
        __RSTSDSetSpecific(table);
    }
    
    return table;
}


// For the use of RS and Foundation only
RSExport void *_RSGetTSD(uint32_t slot) {
    if (slot > RS_TSD_MAX_SLOTS) {
//        _RSLogSimple(kRSLogLevelError, "Error: TSD slot %d out of range (get)", slot);
        __HALT();
    }
    __RSTSDTable *table = __RSTSDGetTable();
    if (!table) {
        // Someone is getting TSD during thread destruction. The table is gone, so we can't get any data anymore.
//        _RSLogSimple(kRSLogLevelWarning, "Warning: TSD slot %d retrieved but the thread data has already been torn down.", slot);
        return nil;
    }
    uintptr_t *slots = (uintptr_t *)(table->data);
    return (void *)slots[slot];
}

// For the use of RS and Foundation only
RSExport void *_RSSetTSD(uint32_t slot, void *newVal, tsdDestructor destructor) {
    if (slot > RS_TSD_MAX_SLOTS) {
//        _RSLogSimple(kRSLogLevelError, "Error: TSD slot %d out of range (set)", slot);
        __HALT();
    }
    __RSTSDTable *table = __RSTSDGetTable();
    if (!table) {
        // Someone is setting TSD during thread destruction. The table is gone, so we can't get any data anymore.
//        _RSLogSimple(kRSLogLevelWarning, "Warning: TSD slot %d set but the thread data has already been torn down.", slot);
        return nil;
    }
    
    void *oldVal = (void *)table->data[slot];
    
    table->data[slot] = (uintptr_t)newVal;
    table->destructors[slot] = destructor;
    
    return oldVal;
}

#include <dispatch/dispatch.h>

struct __RSThread {
    RSRuntimeBase _base;
    pthread_t _thread;
    pid_t _tid;
    pthread_mutex_t _mutex;
    pthread_cond_t _cond;
    const void *_entry;
    void *_param;
};

typedef struct __RSThread *RSThreadRef;

#include <Block.h>
RSInline void __RSThreadSetEntryAsBlock(RSThreadRef thread, BOOL isBlock) {
    __RSBitfieldSetValue(RSRuntimeClassBaseFiled(thread), 2, 1, isBlock ? YES : NO);
}

RSInline BOOL __RSThreadEntryIsBlock(RSThreadRef thread) {
    return __RSBitfieldGetValue(RSRuntimeClassBaseFiled(thread), 2, 1);
}

RSInline void __RSThreadSetShouldTerminal(RSThreadRef thread, BOOL shouldTerminal) {
    __RSBitfieldSetValue(RSRuntimeClassBaseFiled(thread), 3, 2, shouldTerminal ? YES : NO);
}

RSInline BOOL __RSThreadShouldBeTerminal(RSThreadRef thread) {
    return __RSBitfieldGetValue(RSRuntimeClassBaseFiled(thread), 3, 2);
}

RSInline int __RSThreadLock(RSThreadRef thread) {
    return pthread_mutex_lock(&thread->_mutex);
}

RSInline int __RSThreadUnlock(RSThreadRef thread) {
    return pthread_mutex_unlock(&thread->_mutex);
}

RSInline int __RSThreadWait(RSThreadRef thread) {
    return pthread_cond_wait(&thread->_cond, &thread->_mutex);
}

RSInline int __RSThreadSingal(RSThreadRef thread) {
    return pthread_cond_signal(&thread->_cond);
}

RSInline int __RSThreadBroadcast(RSThreadRef thread) {
    return pthread_cond_broadcast(&thread->_cond);
}

static void __RSThreadClassInit(RSTypeRef rs)
{
    RSThreadRef thread = (RSThreadRef)rs;
    pthread_cond_init(&thread->_cond, nil);
    pthread_mutex_init(&thread->_mutex, nil);
    __RSRuntimeSetInstanceSpecial(thread, YES);
}

static void __RSThreadClassDeallocate(RSTypeRef rs)
{
    RSThreadRef thread = (RSThreadRef)rs;
    if (__RSThreadEntryIsBlock(thread))
        Block_release(thread->_entry);
    __RSThreadLock(thread);
    pthread_cond_destroy(&thread->_cond);
    pthread_mutex_destroy(&thread->_mutex);
}

static RSHashCode __RSThreadClassHash(RSTypeRef rs) {
    return 0;
}

static RSStringRef __RSThreadClassDescription(RSTypeRef rs)
{
    RSStringRef description = RSStringCreateWithFormat(RSAllocatorDefault, RSSTR("RSRepeatTaskThread %p"), rs);
    return description;
}

static RSRuntimeClass __RSThreadClass =
{
    _RSRuntimeScannedObject,
    0,
    "RSRepeatTaskThread",
    __RSThreadClassInit,
    nil,
    __RSThreadClassDeallocate,
    nil,
    __RSThreadClassHash,
    __RSThreadClassDescription,
    nil,
    nil
};

RSInline BOOL __RSTypeIsKindOfThread(RSTypeRef type);
#define __RSThreadCheckValid(thread) if (NO == (__RSTypeIsKindOfThread((thread)))) __HALT();

static RSTypeID _RSThreadTypeID = _RSRuntimeNotATypeID;
static void __RSRepeatTaskThreadInitialize();
static void __RSThreadPoolInitialize();

RSPrivate void __RSThreadInitialize() {
    _RSThreadTypeID = __RSRuntimeRegisterClass(&__RSThreadClass);
    __RSRuntimeSetClassTypeID(&__RSThreadClass, _RSThreadTypeID);
    __RSRepeatTaskThreadInitialize();
    __RSThreadPoolInitialize();
}

RSExport RSTypeID RSThreadGetTypeID() {
    return _RSThreadTypeID;
}

static void *__RSThreadEntry(void *context) {
    RSThreadRef thread = context;
    if (__RSTypeIsKindOfThread(thread) == NO)
        __HALT();
    __block void *result = nil;
    for (; ;) {
        __RSThreadLock(thread);
        __RSThreadWait(thread); // wait
        if (thread->_entry) {
            RSAutoreleaseBlock(^{
                if (__RSThreadEntryIsBlock(thread)) {
                    void (^perform)() = (void(^)())thread->_entry;
                    perform();
                } else {
                    void *(*perform)(void *ctx) = (void * (*)(void *))thread->_entry;
                    result = perform(thread->_param);
                }
            });
        }
        __RSThreadUnlock(thread);
        if (__RSThreadShouldBeTerminal(thread))
            break;
    }
    pthread_exit(result);
    return result;
}
#if DEPLOYMENT_TARGET_MACOSX || DEPLOYMENT_TARGET_IPHONEOS
#include <mach/mach.h>
#endif

RSInline pthread_t __RSThreadPthread(RSThreadRef thread) {
    return thread->_thread;
}

RSExport void RSThreadSuspend(RSThreadRef thread) {
    if (!thread) return;
    __RSThreadCheckValid(thread);
#if DEPLOYMENT_TARGET_MACOSX || DEPLOYMENT_TARGET_IPHONEOS
    thread_suspend(pthread_mach_thread_np(__RSThreadPthread(thread)));
#endif
}

RSExport void RSThreadResume(RSThreadRef thread) {
    if (!thread) return;
    __RSThreadCheckValid(thread);
#if DEPLOYMENT_TARGET_MACOSX || DEPLOYMENT_TARGET_IPHONEOS
    thread_resume(pthread_mach_thread_np(__RSThreadPthread(thread)));
#endif
}

RSExport BOOL RSThreadIsRunning(RSThreadRef thread) {
    if (!thread) return NO;
    __RSThreadCheckValid(thread);
#if DEPLOYMENT_TARGET_MACOSX || DEPLOYMENT_TARGET_IPHONEOS
    thread_info_data_t info;
    thread_basic_info_t basic_info = {0};
    mach_msg_type_number_t outCnt = {0};
    kern_return_t kr = thread_info(pthread_mach_thread_np(__RSThreadPthread(thread)), THREAD_BASIC_INFO, (thread_info_t)info, &outCnt);
    if (kr == KERN_SUCCESS) {
        if (outCnt == 1) {
            basic_info = (thread_basic_info_t)info;
            return basic_info->run_state & TH_STATE_RUNNING;
        }
    }
#endif
    return NO;
}

#define __RSThreadCreateNonDetach   ((void *)0x12344321)

static RSThreadRef __RSThreadInit(RSThreadRef thread, BOOL isBlock, void *entry, void *args, BOOL suspend, void *attr) {
    thread->_entry = isBlock ? Block_copy(entry) : entry;
    __RSThreadSetEntryAsBlock(thread, isBlock);
    thread->_param = args;
    pthread_create(&thread->_thread, (attr == __RSThreadCreateNonDetach) ? nil : attr, __RSThreadEntry, thread);
//    pthread_create_suspended_np();
    if (attr == nil)
        pthread_detach(thread->_thread); //detach by default
    if (!suspend)
        RSThreadResume(thread);
    return thread;
}

static RSThreadRef __RSThreadCreateInstance(RSAllocatorRef allocator, RSTypeRef ctx __unused, BOOL isBlock, void *entry, void *args, BOOL suspend, void *attr) {
    RSThreadRef thread = (RSThreadRef)__RSRuntimeCreateInstance(allocator, _RSThreadTypeID, sizeof(struct __RSThread) - sizeof(RSRuntimeBase));
    return __RSThreadInit(thread, isBlock, entry, args, suspend, attr);
}

RSExport RSThreadRef RSThreadCreate(RSAllocatorRef allocator, void (*func)(void *context), void *context, BOOL suspend) {
    return __RSThreadCreateInstance(allocator, nil, NO, func, context, suspend, nil);
}

RSExport RSThreadRef RSThreadCreateWithBlock(RSAllocatorRef allocator, void (^block)(), BOOL suspend) {
    return __RSThreadCreateInstance(allocator, nil, YES, block, nil, suspend, nil);
}

RSExport RSThreadRef RSThreadSetBlock(RSThreadRef thread, void (^block)()) {
    if (!thread) return nil;
    if (RSThreadIsRunning(thread)) return nil;
    __RSThreadLock(thread);
    __RSThreadSetEntryAsBlock(thread, YES);
    thread->_entry = block;
    __RSThreadUnlock(thread);
    return thread;
}

RSExport RSThreadRef RSThreadWaitForSingal(RSThreadRef thread) {
    if (!thread) return nil;
    __RSThreadCheckValid(thread);
    __RSThreadWait(thread);
    return thread;
}

RSExport RSThreadRef RSThreadSignal(RSThreadRef thread) {
    if (!thread) return nil;
    __RSThreadCheckValid(thread);
    __RSThreadSingal(thread);
    return thread;
}

#include <RSCoreFoundation/RSQueue.h>

struct __RSWorkQueueThread {
    struct __RSThread _thread;
    RSQueueRef _taskQueue;
};

typedef struct __RSRepeatTaskThread *RSRepeatTaskThreadRef;

struct __RSRepeatTaskThread {
    struct __RSThread _thread;
    union {
        RSTypeRef (^_perform)(RSIndex idx);
        RSTypeRef (*_performFunc)(RSIndex idx);
    };
    RSRange _repeatRange;
    RSMutableArrayRef _repeatResults;
};

static void __RSRepeatTaskThreadClassInit(RSTypeRef rs)
{
    __RSThreadClassInit(rs);
    RSRepeatTaskThreadRef thread = (RSRepeatTaskThreadRef)rs;
    thread->_repeatResults = RSArrayCreateMutable(RSAllocatorSystemDefault, 0);
}

static void __RSRepeatTaskThreadClassDeallocate(RSTypeRef rs)
{
    RSRepeatTaskThreadRef thread = (RSRepeatTaskThreadRef)rs;
    RSRelease(thread->_repeatResults);
    __RSThreadClassDeallocate(rs);
}

static RSStringRef __RSRepeatTaskThreadClassDescription(RSTypeRef rs)
{
    RSStringRef description = RSStringCreateWithFormat(RSAllocatorDefault, RSSTR("RSRepeatTaskThread %p"), rs);
    return description;
}

static RSRuntimeClass __RSRepeatTaskThreadClass =
{
    _RSRuntimeScannedObject,
    0,
    "RSRepeatTaskThread",
    __RSRepeatTaskThreadClassInit,
    nil,
    __RSRepeatTaskThreadClassDeallocate,
    nil,
    __RSThreadClassHash,
    __RSRepeatTaskThreadClassDescription,
    nil,
    nil
};

static RSTypeID _RSRepeatTaskThreadTypeID = _RSRuntimeNotATypeID;

RSExport RSTypeID RSRepeatTaskThreadGetTypeID()
{
    return _RSRepeatTaskThreadTypeID;
}

static void __RSRepeatTaskThreadInitialize()
{
    _RSRepeatTaskThreadTypeID = __RSRuntimeRegisterClass(&__RSRepeatTaskThreadClass);
    __RSRuntimeSetClassTypeID(&__RSRepeatTaskThreadClass, _RSRepeatTaskThreadTypeID);
}

RSInline void __RSRepeatTaskThreadSetBlock(RSRepeatTaskThreadRef thread, RSTypeRef (^block)(RSIndex idx)) {
    thread->_perform = block;
    return;
}

RSPrivate RSThreadRef __RSRepeatTaskThreadCreateInstance(RSAllocatorRef allocator, RSTypeRef ctx, BOOL isBlock, void *entry, void *args, BOOL suspend, void *attr)
{
    RSRepeatTaskThreadRef thread = (RSRepeatTaskThreadRef)__RSRuntimeCreateInstance(allocator, _RSRepeatTaskThreadTypeID, sizeof(struct __RSRepeatTaskThread) - sizeof(RSRuntimeBase));
    thread->_repeatRange = RSNumberRangeValue(ctx);
    void(^block)(void) = ^void(void) {
        RSTypeRef result = nil;
        if (isBlock) {
            for (RSIndex idx = thread->_repeatRange.location; idx < thread->_repeatRange.location + thread->_repeatRange.length; idx++) {
//                RSLog(RSSTR("perform %ld"), idx);
                result = ((RSRepeatTaskThreadRef)thread)->_perform(idx);
                RSArrayAddObject(((RSRepeatTaskThreadRef)thread)->_repeatResults, result);
            }
        } else {
            for (RSIndex idx = ((RSRepeatTaskThreadRef)thread)->_repeatRange.location; idx < ((RSRepeatTaskThreadRef)thread)->_repeatRange.length; idx++) {
                result = ((RSRepeatTaskThreadRef)thread)->_performFunc(idx);
                RSArrayAddObject(((RSRepeatTaskThreadRef)thread)->_repeatResults, result);
            }
        }
        return;
    };
    ((RSRepeatTaskThreadRef)thread)->_performFunc = entry;
    __RSThreadInit((RSThreadRef)thread, YES, block, nil, YES, attr);
    if (!suspend)
        RSThreadResume((RSThreadRef)thread);
    return (RSThreadRef)thread;
}

struct __RSThreadPool {
    RSRuntimeBase _base;
    RSSpinLock _lock;           // used for modify thead pool
    pthread_cond_t _cond;
    pthread_mutex_t _mutex;
    dispatch_group_t _group;
    RSMutableArrayRef _threads; // __RSWorkQueueThread
};

typedef struct __RSThreadPool *RSThreadPoolRef;

RSInline void __RSThreadPoolLock(RSThreadPoolRef threadPool) {
    RSSpinLockLock(&threadPool->_lock);
}

RSInline void __RSThreadPoolUnlock(RSThreadPoolRef threadPool) {
    RSSpinLockUnlock(&threadPool->_lock);
}

static void __RSThreadPoolClassInit(RSTypeRef rs)
{
    RSThreadPoolRef threadPool = (RSThreadPoolRef)rs;
    threadPool->_threads = RSArrayCreateMutable(RSAllocatorSystemDefault, 0);
    pthread_cond_init(&threadPool->_cond, nil);
    pthread_mutex_init(&threadPool->_mutex, nil);
    threadPool->_group = dispatch_group_create();
}

static void __RSThreadPoolClassDeallocate(RSTypeRef rs)
{
    RSThreadPoolRef threadPool = (RSThreadPoolRef)rs;
    pthread_mutex_lock(&threadPool->_mutex);
    RSArrayApplyBlock(threadPool->_threads, RSMakeRange(0, RSArrayGetCount(threadPool->_threads)), ^(const void *value, RSUInteger idx, BOOL *isStop) {
        __RSThreadSetShouldTerminal((RSThreadRef)value, YES);
        __RSThreadSingal((RSThreadRef)value);
        __RSRuntimeSetInstanceSpecial(value, NO);
        pthread_detach(__RSThreadPthread((RSThreadRef)value));
    });
    RSRelease(threadPool->_threads);
    pthread_mutex_destroy(&threadPool->_mutex);
    pthread_cond_destroy(&threadPool->_cond);
    
    dispatch_release(threadPool->_group);
}

static RSStringRef __RSThreadPoolClassDescription(RSTypeRef rs)
{
    RSStringRef description = RSStringCreateWithFormat(RSAllocatorDefault, RSSTR("RSRepeatTaskThread %p"), rs);
    return description;
}

static RSRuntimeClass __RSThreadPoolClass =
{
    _RSRuntimeScannedObject,
    0,
    "RSThreadPool",
    __RSThreadPoolClassInit,
    nil,
    __RSThreadPoolClassDeallocate,
    nil,
    nil,
    __RSThreadPoolClassDescription,
    nil,
    nil
};

static RSTypeID _RSThreadPoolTypeID = _RSRuntimeNotATypeID;

RSExport RSTypeID RSThreadPoolGetTypeID() {
    return _RSThreadPoolTypeID;
}

static void __RSThreadPoolInitialize() {
    _RSThreadPoolTypeID = __RSRuntimeRegisterClass(&__RSThreadPoolClass);
    __RSRuntimeSetClassTypeID(&__RSThreadPoolClass, _RSThreadPoolTypeID);
}

RSInline RSArrayRef __RSThreadPoolThreads(RSThreadPoolRef threadPool) {
    return threadPool->_threads;
}

extern void _RSArraySetCapacity(RSMutableArrayRef array, RSIndex cap);
static RSThreadPoolRef __RSThreadPoolCreateInstance(RSAllocatorRef allocator, RSUInteger numberOfThread, void *attr, RSThreadRef (*threadCreator)(RSAllocatorRef allocator, RSTypeRef ctx, BOOL isBlock, void *entry, void *args, BOOL suspend, void *attr)) {
    RSThreadPoolRef threadPool = (RSThreadPoolRef)__RSRuntimeCreateInstance(allocator, _RSThreadPoolTypeID, sizeof(struct __RSThreadPool) - sizeof(RSRuntimeBase));
    _RSArraySetCapacity(threadPool->_threads, numberOfThread);
    for (RSUInteger idx = 0; idx < numberOfThread; idx++) {
        RSThreadRef thread = threadCreator(allocator, nil, YES, nil, nil, YES, attr);
        RSArrayAddObject(threadPool->_threads, thread);
        RSRelease(thread);
    }
    return threadPool;
}

#include "RSPrivate/RSPrivateOperatingSystem/RSPrivateOSCpu.h"
#include "RSPrivate/RSPrivateOperatingSystem/RSPrivateTask.h"

RSInline RSUInteger __RSPerformConcurrentThreadCount(RSUInteger activeProssorCount, RSIndex performCount) {
    if (performCount < activeProssorCount) return performCount;
    return activeProssorCount;
}

RSExport RSArrayRef __RSPerformBlockRepeatCopyResults(RSIndex performCount, RSTypeRef (^perform)(RSIndex idx)) {
    RSMutableArrayRef returnResults = RSArrayCreateMutable(RSAllocatorSystemDefault, 0);
    dispatch_queue_t queue = dispatch_queue_create(nil, DISPATCH_QUEUE_CONCURRENT);
    dispatch_group_t grounp = dispatch_group_create();
    RSArrayRef (^block)(RSRange range) = ^RSArrayRef (RSRange range) {
        RSMutableArrayRef results = RSArrayCreateMutable(RSAllocatorSystemDefault, range.length);
        for (RSIndex idx = 0; idx < range.length; idx++) {
            RSArrayAddObject(results, perform(idx + range.location));
        }
        return (results);
    };
    RSUInteger activeProssorCount = __RSActiveProcessorCount();
    RSUInteger threadCount = __RSPerformConcurrentThreadCount(activeProssorCount, performCount);
    RSUInteger workCount = performCount / threadCount;
    RSUInteger lastPayload = performCount - threadCount * workCount;
    RSTypeRef *tmpResults = RSAllocatorAllocate(RSAllocatorSystemDefault, sizeof(RSTypeRef) * threadCount);
    for (RSIndex idx = 0; idx < threadCount; idx++) {
        dispatch_group_async(grounp, queue, ^{
            tmpResults[idx] = block(RSMakeRange(idx * workCount, workCount + ((idx == (threadCount - 1)) ? lastPayload : 0)));
        });
    }
    dispatch_group_wait(grounp, DISPATCH_TIME_FOREVER);
    for (RSIndex idx = 0; idx < threadCount; idx++) {
        RSArrayAddObjects(returnResults, tmpResults[idx]);
        RSRelease(tmpResults[idx]);
    }
    RSAllocatorDeallocate(RSAllocatorSystemDefault, tmpResults);
    dispatch_release(grounp);
    
    return returnResults;
}

RSExport RSArrayRef RSPerformBlockConcurrentCopyResults(RSIndex performCount, RSTypeRef (^perform)(RSIndex idx)) {
    RSQueueRef queue = RSQueueCreate(RSAllocatorDefault, 0, RSQueueAtom);
    RSPerformBlockRepeatWithFlags(performCount, RSPerformBlockOverCommit, ^(RSIndex idx) {
        RSQueueEnqueue(queue, perform(idx));
    });
    RSArrayRef results = RSQueueCopyCoreQueueUnsafe(queue);
    RSRelease(queue);
    return results;
//    return RSAutorelease(__RSPerformBlockRepeatCopyResults(performCount, perform));
//    RSUInteger activeProssorCount = __RSActiveProcessorCount();
//    RSUInteger threadCount = __RSPerformConcurrentThreadCount(activeProssorCount, performCount);
//    RSUInteger workCount = performCount / threadCount;
//    RSUInteger lastPayload = performCount - threadCount * workCount;
//    RSThreadPoolRef threadPool = __RSThreadPoolCreateInstance(RSAllocatorSystemDefault, threadCount, __RSThreadCreateNonDetach, __RSRepeatTaskThreadCreateInstance);
//    RSArrayApplyBlock(__RSThreadPoolThreads(threadPool), RSMakeRange(0, threadCount), ^(const void *value, RSUInteger idx, BOOL *isStop) {
//        RSRepeatTaskThreadRef repeat = (RSRepeatTaskThreadRef)value;
//        __RSRepeatTaskThreadSetBlock(repeat, perform);
//        repeat->_repeatRange = RSMakeRange(idx * workCount, workCount + ((idx == (threadCount - 1)) ? lastPayload : 0));
//        RSThreadResume((RSThreadRef)repeat);
//        RSThreadSignal((RSThreadRef)repeat);
//        __RSThreadSetShouldTerminal((RSThreadRef)repeat, YES);
//    });
//    
//    RSArrayApplyBlock(__RSThreadPoolThreads(threadPool), RSMakeRange(0, threadCount), ^(const void *value, RSUInteger idx, BOOL *isStop) {
//        void *result = nil;
//        pthread_join(__RSThreadPthread((RSThreadRef)value), &result);
//    });
//    
//    RSMutableArrayRef returnResults = RSArrayCreateMutable(RSAllocatorSystemDefault, 0);
//    RSArrayApplyBlock(__RSThreadPoolThreads(threadPool), RSMakeRange(0, threadCount), ^(const void *value, RSUInteger idx, BOOL *isStop) {
//        RSRepeatTaskThreadRef repeat = (RSRepeatTaskThreadRef)value;
//        RSArrayAddObjects(returnResults, repeat->_repeatResults);
//    });
//    RSRelease(threadPool);
//    return RSAutorelease(returnResults);
}



RSInline BOOL __RSTypeIsKindOfThread(RSTypeRef type) {
    RSTypeID ID = RSGetTypeID(type);
    if (ID == _RSThreadTypeID || ID == _RSRepeatTaskThreadTypeID)
        return YES;
    return NO;
}
