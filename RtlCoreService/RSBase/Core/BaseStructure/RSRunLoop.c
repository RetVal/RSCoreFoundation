//
//  RSRunLoop.c
//  RSCoreFoundation
//
//  Created by RetVal on 11/23/12.
//  Copyright (c) 2012 RetVal. All rights reserved.
//

#include <stdio.h>
#include <RSCoreFoundation/RSRunLoop.h>

#include <RSCoreFoundation/RSArray.h>
#include <RSCoreFoundation/RSData.h>
#include <RSCoreFoundation/RSDictionary.h>
#include <RSCoreFoundation/RSString.h>

#include <pthread.h>

#include <RSCoreFoundation/RSRuntime.h>
#include <RSCoreFoundation/RSTimer.h>
#include <dispatch/dispatch.h>

#include "RSPrivate/RSPrivateOperatingSystem/RSPrivateTask.h"

#pragma mark -
#pragma mark RSRunLoop Private methods inside

RS_PUBLIC_CONST_STRING_DECL(RSRunLoopDefault, "RSRunLoopDefault");
RS_PUBLIC_CONST_STRING_DECL(RSRunLoopQueue, "RSRunLoopQueue");
RS_PUBLIC_CONST_STRING_DECL(RSRunLoopMain, "RSRunLoopMain");

static pthread_key_t const __RSRunLoopThreadKey = RSRunLoopKey;
static RSRunLoopRef _RSRunLoopMain = nil;

RSPrivate RSStringRef __RSRunLoopThreadToString(pthread_t t)
{
    RSStringRef thread = nil;
#if DEPLOYMENT_TARGET_MACOSX
    RSBlock name[256] = {0};
    RSIndex i = pthread_getname_np(t, name, 256);
    if (i == 0)
    {
        i = strlen(name);
        if (i != 0) thread = RSStringCreateWithCString(RSAllocatorSystemDefault, name, RSStringEncodingUTF8);
        else goto HERE;
    }
    else
    {
    HERE:
        thread = RSStringCreateWithFormat(RSAllocatorSystemDefault, RSSTR("%ld"), __RSGetTidWithThread(t));
    }
#elif DEPLOYMENT_TARGET_LINUX
    thread = RSStringCreateWithFormat(RSAllocatorSystemDefault, RSSTR("%ld"), t);
#elif DEPLOYMENT_TARGET_WINDOWS
    thread = RSStringCreateWithFormat(RSAllocatorSystemDefault, RSSTR("%p"),t->p);
#endif
    return thread;
}

#pragma mark -
#pragma mark RSRunLoopMode API

static const int ____kRSRunLoopModeDefault  = 0b00000;
static const int ____kRSRunLoopModeQueue    = 0b00001;
static const int ____kRSRunLoopModeMain     = 0b00010;
static const int ____kRSRunLoopModeUserDef  = 0b00011;

static const int ____kRSRunLoopZone0_default_000 = 0b000;
static const int ____kRSRunLoopZone0_default_001 = 0b001;
static const int ____kRSRunLoopZone0_default_010 = 0b010;
static const int ____kRSRunLoopZone0_default_011 = 0b011;
static const int ____kRSRunLoopZone0_default_100 = 0b100;
static const int ____kRSRunLoopZone0_default_101 = 0b101;
static const int ____kRSRunLoopZone0_default_110 = 0b110;
static const int ____kRSRunLoopZone0_default_111 = 0b111;

static const int ____kRSRunLoopModeDefaultZone0_reserved0   = ____kRSRunLoopZone0_default_000;
static const int ____kRSRunLoopModeDefaultZone0_reserved1   = ____kRSRunLoopZone0_default_001;
static const int ____kRSRunLoopModeDefaultZone0_reserved2   = ____kRSRunLoopZone0_default_010;
static const int ____kRSRunLoopModeDefaultZone0_reserved3   = ____kRSRunLoopZone0_default_011;

static const int ____kRSRunLoopModeQueueZone0_low           = ____kRSRunLoopZone0_default_000;
static const int ____kRSRunLoopModeQueueZone0_middle        = ____kRSRunLoopZone0_default_001;
static const int ____kRSRunLoopModeQueueZone0_high          = ____kRSRunLoopZone0_default_010;

static const int ____kRSRunLoopModeMainZone0_fshedule       = ____kRSRunLoopZone0_default_001;
static const int ____kRSRunLoopModeMainZone0_active         = ____kRSRunLoopZone0_default_010;
static const int ____kRSRunLoopModeMainZone0_sshedule       = ____kRSRunLoopZone0_default_100;
static const int ____kRSRunLoopModeMainZone0_all            = ____kRSRunLoopZone0_default_111;
static const int ____kRSRunLoopModeMainZone0_fa             = ____kRSRunLoopZone0_default_011;
static const int ____kRSRunLoopModeMainZone0_fs             = ____kRSRunLoopZone0_default_101;
static const int ____kRSRunLoopModeMainZone0_as             = ____kRSRunLoopZone0_default_110;

static const int ____kRSRunLoopModeUserDefZone0_compute     = ____kRSRunLoopZone0_default_000;
static const int ____kRSRunLoopModeUserDefZone0_DiskIO      = ____kRSRunLoopZone0_default_001;
static const int ____kRSRunLoopModeUserDefZone0_NetIO       = ____kRSRunLoopZone0_default_010;
static const int ____kRSRunLoopModeUserDefZone0_memory      = ____kRSRunLoopZone0_default_100;
static const int ____kRSRunLoopModeUserDefZone0_dn          = ____kRSRunLoopZone0_default_011;
static const int ____kRSRunLoopModeUserDefZone0_dm          = ____kRSRunLoopZone0_default_101;
static const int ____kRSRunLoopModeUserDefZone0_nm          = ____kRSRunLoopZone0_default_110;
static const int ____kRSRunLoopModeUserDefZone0_all         = ____kRSRunLoopZone0_default_111;


struct mode
{
    // 00000 = RSRunLoopDefault
    // 00001 = RSRunLoopQueue
    // 00010 = RSRunLoopMain
    // 00011 = RSRunLoopUserDefine
    int _modeField : 5;
    
    // _modeField = 00000
    // 000 = reserved
    // 001 = reserved
    // 010 = reserved
    // 011 = reserved
    
    // _modeField = 00001
    // 000 = low
    // 001 = default
    // 010 = high
    // 011 = reserved
    
    // _modeField = 00010
    // USE OR OPERATOR
    // 001 = first schedule will run in main queue
    // 010 = active will run in main queue
    // 100 = second schedule will run in main queue
    // 111 = all part of source should do in main queue
    
    // _modeField = 00011
    // USE OR OPERATOR
    // 000 = compute
    // 001 = Disk IO
    // 010 = Network IO
    // 100 = memory block
    
    int _zone0 : 3;
    
    // 0 async
    // 1 sync
    int _sync : 1;
    
    int _autorelease : 1;
    
    int _autoremove : 1;
    
    int _reserved : sizeof(int)*8 - 9;
};

struct __RSRunLoopMode
{
    struct mode _mode;
};

typedef struct __RSRunLoopMode RSRunLoopMode;

static RSRunLoopMode __RSRunLoopModeCreateInstance(RSAllocatorRef allocator, struct mode mode)
{
    RSRunLoopMode _rlm = {mode};
    return _rlm;
}

static BOOL __RSRunLoopModeIsSync(RSRunLoopMode mode)
{
    return mode._mode._sync;
}

static void __RSRunLoopModeMarkSync(RSRunLoopMode mode)
{
    mode._mode._sync = YES;
}

static void __RSRunLoopModeMarkAsync(RSRunLoopMode mode)
{
    mode._mode._sync = NO;
}

static RSRunLoopMode __RSRunLoopModeTranslateNameToMode(RSAllocatorRef allocator, RSStringRef modeName)
{
    struct mode __cmode = {0};
    __cmode._autorelease = NO;
    __cmode._autoremove = NO;
    if (modeName == RSRunLoopDefault ||
        RSEqual(modeName, RSRunLoopDefault))
    {
        __cmode._sync = NO;
        __cmode._modeField = ____kRSRunLoopModeDefault;
        __cmode._zone0 = 0b000;
        __cmode._reserved = 0;
    }
    else if (modeName == RSRunLoopQueue ||
             RSEqual(modeName, RSRunLoopQueue))
    {
        __cmode._sync = YES;
        __cmode._modeField = ____kRSRunLoopModeQueue;
        __cmode._zone0 = 0b001;
        __cmode._reserved = 0;
    }
    else if (modeName == RSRunLoopMain ||
             RSEqual(modeName, RSRunLoopMain))
    {
        __cmode._sync = YES;
        __cmode._modeField = ____kRSRunLoopModeMain;
        __cmode._zone0 = 0b111;
        __cmode._reserved = 0;
    }
    RSRunLoopMode _mode = __RSRunLoopModeCreateInstance(allocator, __cmode);
    return _mode;
}

#pragma mark -
#pragma mark RSRunLoopSource API

struct __RSRunLoopSource
{
    RSRuntimeBase _base;
    RSSpinLock _lock;
    RSRunLoopRef _rl;
    RSIndex _order;
    RSRunLoopMode _mode;
    union
    {
        RSRunLoopSourceContext context1;
    }_context;
    
};

RSInline void __RSRunLoopSourceLock(RSRunLoopSourceRef rls)
{
    RSSpinLockLock(&rls->_lock);
}

RSInline void __RSRunLoopSourceUnLock(RSRunLoopSourceRef rls)
{
    RSSpinLockUnlock(&rls->_lock);
}

RSInline BOOL __RSRunLoopSourceIsValid(RSRunLoopSourceRef rls)
{
    return __RSBitfieldGetValue(RSRuntimeClassBaseFiled(rls), 1, 1);
}

RSInline void __RSRunLoopSourceValid(RSRunLoopSourceRef rls)
{
    __RSBitfieldSetValue(RSRuntimeClassBaseFiled(rls), 1, 1, 1);
}

RSInline void __RSRunLoopSourceInvalid(RSRunLoopSourceRef rls)
{
    __RSBitfieldSetValue(RSRuntimeClassBaseFiled(rls), 1, 1, 0);
}

RSInline BOOL __RSRunLoopSourceIsRunning(RSRunLoopSourceRef rls)
{
    return __RSBitfieldGetValue(RSRuntimeClassBaseFiled(rls), 2, 2);
}

RSInline void __RSRunLoopSourceMarkRunning(RSRunLoopSourceRef rls)
{
    __RSBitfieldSetValue(RSRuntimeClassBaseFiled(rls), 2, 2, 1);
}

RSInline void __RSRunLoopSourceMarkWaiting(RSRunLoopSourceRef rls)
{
    __RSBitfieldSetValue(RSRuntimeClassBaseFiled(rls), 2, 2, 0);
}

RSInline BOOL __RSRunLoopSourceIsFinished(RSRunLoopSourceRef rls)
{
    return __RSBitfieldGetValue(RSRuntimeClassBaseFiled(rls), 3, 3);
}

RSInline void __RSRunLoopSourceMarkFinished(RSRunLoopSourceRef rls)
{
    __RSBitfieldSetValue(RSRuntimeClassBaseFiled(rls), 3, 3, 1);
}

RSInline void __RSRunLoopSourceUnmarkFinished(RSRunLoopSourceRef rls)
{
    __RSBitfieldSetValue(RSRuntimeClassBaseFiled(rls), 3, 3, 0);
}

static RSTypeRef __RSRunLoopSourceClassCopy(RSAllocatorRef allocator, RSTypeRef rs, BOOL mutableCopy)
{
    RSRunLoopSourceRef source = (RSRunLoopSourceRef)rs;
    return RSRetain(source);
}

static void __RSRunLoopSourceClassDeallocate(RSTypeRef rs)
{
    RSRunLoopSourceRef source = (RSRunLoopSourceRef)rs;
	//    RSSpinLockLock(&source->_lock);
	//    RSSpinLockUnlock(&source->_lock);
    if (source->_context.context1.release && source->_context.context1.info)
    {
        source->_context.context1.release(source->_context.context1.info);
    }
    //RSRunLoopRemoveSource(source->_rl, source, RSRunLoopDefault);
    source->_rl = nil;
    
}

static BOOL __RSRunLoopSourceClassEqual(RSTypeRef rs1, RSTypeRef rs2)
{
    RSRunLoopSourceRef rls1 = (RSRunLoopSourceRef)rs1;
    RSRunLoopSourceRef rls2 = (RSRunLoopSourceRef)rs2;
    if (0 != __builtin_memcmp(&rls1->_context.context1,
                              &rls2->_context.context1,
                              sizeof(RSRunLoopSourceContext))) return NO;
    if (rls1->_context.context1.equal &&
		(YES == rls1->_context.context1.equal(rls1->_context.context1.info,
											  rls2->_context.context1.info)))
        return YES;
    return NO;
}

static RSStringRef __RSRunLoopSourceClassDescription(RSTypeRef rs)
{
    RSRunLoopSourceRef rls = (RSRunLoopSourceRef)rs;
    RSStringRef description = RSStringCreateWithFormat(RSAllocatorSystemDefault, RSSTR("RSRunLoopSource <%p>"), rls);
    return description;
}

static RSTypeID _RSRunLoopSourceTypeID = _RSRuntimeNotATypeID;

static RSRuntimeClass __RSRunLoopSourceClass =
{
    _RSRuntimeScannedObject,
    "RSRunLoopSource",
    nil,
    __RSRunLoopSourceClassCopy,
    __RSRunLoopSourceClassDeallocate,
    __RSRunLoopSourceClassEqual,
    nil,
    __RSRunLoopSourceClassDescription,
    nil,
    nil
};

RSPrivate void __RSRunLoopSourceInitialize()
{
    _RSRunLoopSourceTypeID = __RSRuntimeRegisterClass(&__RSRunLoopSourceClass);
    __RSRuntimeSetClassTypeID(&__RSRunLoopSourceClass, _RSRunLoopSourceTypeID);
}

RSExport RSTypeID RSRunLoopSourceGetTypeID()
{
    return _RSRunLoopSourceTypeID;
}

static BOOL __RSRunLoopSourceContext0Avaialable(RSRunLoopSourceContext* context)
{
    if (context == nil) return NO;
    if (context->version) return NO;
//	context->schedule == nil ||
//  context->cancel == nil ||
//    if (context->perform == nil)
//        return NO;
    return YES;
    
}

static void __RSRunLoopSourceAvailable(RSRunLoopSourceRef rls)
{
    __RSGenericValidInstance(rls, _RSRunLoopSourceTypeID);
    if (__RSRunLoopSourceContext0Avaialable(&rls->_context.context1)) return;
    HALTWithError(RSInvalidArgumentException, "the RSRunLoopSource is not available");
}

static RSRunLoopSourceRef __RSRunLoopSourceCreateInstance(RSAllocatorRef allocator, RSIndex order, RSRunLoopSourceContext* context)
{
    RSRunLoopSourceRef memory = (RSRunLoopSourceRef)__RSRuntimeCreateInstance(allocator, _RSRunLoopSourceTypeID, sizeof(struct __RSRunLoopSource) - sizeof(RSRuntimeBase));
    memory->_order = order;
    memcpy(&memory->_context.context1, context, sizeof(RSRunLoopSourceContext));
    if (likely(memory->_context.context1.retain && memory->_context.context1.info))
        memory->_context.context1.retain(memory->_context.context1.info);
    return memory;
}

RSExport RSRunLoopSourceRef RSRunLoopSourceCreate(RSAllocatorRef allocator, RSIndex order, RSRunLoopSourceContext* context)
{
    if (NO == __RSRunLoopSourceContext0Avaialable(context)) return nil;
    return __RSRunLoopSourceCreateInstance(allocator, order, context);
}

RSExport BOOL RSRunLoopSourceIsValid(RSRunLoopSourceRef source)
{
	if (source == nil) return NO;
    __RSRunLoopSourceAvailable(source);
    return __RSRunLoopSourceIsValid(source);
}

RSExport void RSRunLoopSourceRemoveFromRunLoop(RSRunLoopSourceRef source, RSStringRef mode)
{
    __RSRunLoopSourceAvailable(source);
    if (mode == nil) mode = RSRunLoopDefault;
    if (source->_rl)
		RSRunLoopRemoveSource(source->_rl, source, mode);
}

#pragma mark -
#pragma mark RSRunLoop API

struct __RSRunLoop
{
    RSRuntimeBase _base;
    pthread_t _selfThread;
    dispatch_queue_t _queue;
	dispatch_semaphore_t _semaphore;
    RSMutableArrayRef _sources;
    RSStringRef _mode;
    //RSSpinLock _lock;
    pthread_mutex_t _lock;
	RSSpinLock _lockForSources;
};

RSPrivate void *__RSRunLoopGetQueue(RSRunLoopRef rl)
{
    return rl ? rl->_queue : nil;
}

RSInline void __RSRunLoopLock(RSRunLoopRef rl)
{
//    RSSpinLockLock(&rl->_lock);
    pthread_mutex_lock(&rl->_lock);
}

RSInline void __RSRunLoopUnLock(RSRunLoopRef rl)
{
//    RSSpinLockUnlock(&rl->_lock);
    pthread_mutex_unlock(&rl->_lock);
}

static void tls_runloop_deallocate(void* context)
{
    if (context == nil) return;
    RSRunLoopRef rl = (RSRunLoopRef)context;
    __RSRuntimeSetInstanceSpecial(rl, NO);
//    __RSRunLoopLock(rl);
//    __RSRunLoopUnLock(rl);
    RSRelease(rl);
}

static void __RSRunLoopClassInit(RSTypeRef rs)
{
    RSRunLoopRef rl = (RSRunLoopRef)rs;
    rl->_queue = dispatch_get_current_queue();
    rl->_sources = RSArrayCreateMutable(RSAllocatorSystemDefault, 32);
    rl->_selfThread = pthread_self();
//    pthread_key_init_np(__RSRunLoopThreadKey, tls_runloop_deallocate);
    _RSSetTSD(__RSTSDKeyRunLoop, (void *)rs, tls_runloop_deallocate);
	rl->_lockForSources = RSSpinLockInit;
//    rl->_lock = RSSpinLockInit;
    pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
    __builtin_memcpy(&rl->_lock, &mutex, sizeof(pthread_mutex_t));
}

static RSTypeRef __RSRunLoopClassCopy(RSAllocatorRef allocator, RSTypeRef rs, BOOL mutableCopy)
{
    return nil;
}

static void __RSRunLoopClassDeallocate(RSTypeRef rs)
{
    RSRunLoopRef rl = (RSRunLoopRef)rs;
    __RSRunLoopLock(rl);
    
    /*
     clear the should removed source first.
     */
    
    
    
    if (rl->_sources)
    {
//        RSIndex cnt = RSArrayGetCount(rl->_sources);
//        for (RSIndex idx = 0; idx < cnt; ++idx)
//        {
//            RSRunLoopSourceRef source = (RSRunLoopSourceRef)RSArrayObjectAtIndex(rl->_sources, idx);
//            RSRelease(source);
//        }
        RSRelease(rl->_sources);
        rl->_sources = nil;
    }
    
    if (rl->_queue)
    {
        dispatch_release(rl->_queue);
        rl->_queue = nil;
    }
	
	if (rl->_semaphore)
	{
		dispatch_release(rl->_semaphore);
		rl->_semaphore = nil;
	}
    
    if (rl->_mode)
    {
        RSRelease(rl->_mode);
        rl->_mode = nil;
    }
    __RSRunLoopUnLock(rl);
}

static BOOL __RSRunLoopClassEqual(RSTypeRef rs1, RSTypeRef rs2)
{
    RSRunLoopRef rl1 = (RSRunLoopRef)rs1;
    RSRunLoopRef rl2 = (RSRunLoopRef)rs2;
    return pthread_equal(rl1->_selfThread, rl2->_selfThread);
}

static RSStringRef __RSRunLoopClassDescription(RSTypeRef rs)
{
    RSRunLoopRef rl = (RSRunLoopRef)rs;
    RSMutableStringRef description = nil;
    description = RSStringCreateMutable(RSAllocatorSystemDefault, 30);
    RSStringAppendStringWithFormat(description, RSSTR("%s(%lld)<%r>"), __RSRuntimeGetClassNameWithInstance(rs), RSArrayGetCount(rl->_sources), __RSRunLoopThreadToString(rl->_selfThread));
    return description;
}

static RSRuntimeClass __RSRunLoopClass =
{
    _RSRuntimeScannedObject,
    "RSRunLoop",
    __RSRunLoopClassInit,
    __RSRunLoopClassCopy,
    __RSRunLoopClassDeallocate,
    __RSRunLoopClassEqual,
    nil,
    __RSRunLoopClassDescription,
    nil,
    nil
};

RSInline BOOL __RSRunLoopIsWaiting(RSRunLoopRef rl)
{
	return __RSBitfieldGetValue(RSRuntimeClassBaseFiled(rl), 1, 1);
}

RSInline void __RSRunLoopWakeUp(RSRunLoopRef rl)
{
	__RSBitfieldSetValue(RSRuntimeClassBaseFiled(rl), 1, 1, 0);
}

RSInline void __RSRunLoopStop(RSRunLoopRef rl)
{
	__RSBitfieldSetValue(RSRuntimeClassBaseFiled(rl), 1, 1, 1);
}

static RSTypeID _RSRunLoopTypeID = _RSRuntimeNotATypeID;

RSPrivate void __RSRunLoopInitialize()
{
    _RSRunLoopTypeID = __RSRuntimeRegisterClass(&__RSRunLoopClass);
    __RSRuntimeSetClassTypeID(&__RSRunLoopClass, _RSRunLoopTypeID);
    
    _RSRunLoopMain = RSRunLoopCurrentLoop();
}

RSExport RSTypeID RSRunLoopGetTypeID()
{
    return _RSRunLoopTypeID;
}

static RSRunLoopRef __RSRunLoopCreateInstance(RSAllocatorRef allocator)
{
    RSRunLoopRef rl = (RSRunLoopRef)__RSRuntimeCreateInstance(allocator, _RSRunLoopTypeID, sizeof(struct __RSRunLoop) - sizeof(RSRuntimeBase));
    __RSRuntimeSetInstanceSpecial(rl, YES);
//    pthread_setspecific(__RSRunLoopThreadKey, rl);
    _RSSetTSD(__RSTSDKeyRunLoop, rl, tls_runloop_deallocate);
    rl->_mode = RSRunLoopDefault;
    return rl;
}

static RSRunLoopRef __RSRunLoopGetRunLoopForCurrentThread()
{
//    RSRunLoopRef rl = (RSRunLoopRef)pthread_getspecific(__RSRunLoopThreadKey);
    RSRunLoopRef rl = (RSRunLoopRef)_RSGetTSD(__RSTSDKeyRunLoop);
    if (rl) return rl;
    return __RSRunLoopCreateInstance(RSAllocatorSystemDefault);
}

RSExport RSRunLoopRef RSRunLoopCurrentLoop()
{
    return __RSRunLoopGetRunLoopForCurrentThread();
}

RSExport RSRunLoopRef RSRunLoopMainLoop()
{
    return _RSRunLoopMain;
}

RSExport RSStringRef RSRunLoopModeName(RSRunLoopRef rl)
{
    if (rl == nil) rl = RSRunLoopCurrentLoop();
    else __RSGenericValidInstance(rl, _RSRunLoopTypeID);
    return RSRetain(rl->_mode);
}

RSExport BOOL RSRunLoopIsWaiting(RSRunLoopRef rl)
{
    if (rl == nil) rl = RSRunLoopCurrentLoop();
    else __RSGenericValidInstance(rl, _RSRunLoopTypeID);
    return __RSRunLoopIsWaiting(rl);
}

RSExport void RSRunLoopWakeUp(RSRunLoopRef rl)
{
    if (rl == nil) rl = RSRunLoopCurrentLoop();
    else __RSGenericValidInstance(rl, _RSRunLoopTypeID);
	if (__RSRunLoopIsWaiting(rl) && rl->_semaphore)
	{
		__RSRunLoopWakeUp(rl);
		dispatch_semaphore_signal(rl->_semaphore);
	}
}

RSExport void RSRunLoopStop(RSRunLoopRef rl)
{
    if (rl == nil) rl = RSRunLoopCurrentLoop();
    else __RSGenericValidInstance(rl, _RSRunLoopTypeID);
	if (rl->_semaphore == nil) rl->_semaphore = dispatch_semaphore_create(0);
	__RSRunLoopStop(rl);
}


RSExport BOOL RSRunLoopContainsSource(RSRunLoopRef rl, RSRunLoopSourceRef source, RSStringRef mode)
{
    if (rl == nil) rl = RSRunLoopCurrentLoop();
    else __RSGenericValidInstance(rl, _RSRunLoopTypeID);
    if (likely(rl->_sources))
    {
        if (RSNotFound == RSArrayIndexOfObject(rl->_sources, source))
            return NO;
        return YES;
    }
    return NO;
}

RSExport void RSRunLoopAddSource(RSRunLoopRef rl, RSRunLoopSourceRef rls, RSStringRef mode)
{
    if (rl == nil) rl = RSRunLoopCurrentLoop();
    else __RSGenericValidInstance(rl, _RSRunLoopTypeID);
	mode = mode != nil ? mode : RSRunLoopDefault;
    if (likely(rl->_sources && rls))
    {
		__RSRunLoopLock(rl);
		RSSpinLockLock(&rl->_lockForSources);
		if (RSNotFound == RSArrayIndexOfObject(rl->_sources, rls))
			RSArrayAddObject(rl->_sources, rls);
		rls->_rl = rl;
		__RSRunLoopSourceMarkWaiting(rls);
        rls->_mode = __RSRunLoopModeTranslateNameToMode(RSAllocatorSystemDefault, mode);
		if (rls->_context.context1.schedule) rls->_context.context1.schedule(rls->_context.context1.info, rls->_rl, rls->_rl->_mode);
		RSSpinLockUnlock(&rl->_lockForSources);
		__RSRunLoopUnLock(rl);
    }
}

static void __RSRunLoopRemoveSource(RSRunLoopRef rl, RSRunLoopSourceRef source, RSStringRef mode)
{
	if (likely(rl->_sources))
    {
		RSSpinLockLock(&rl->_lockForSources);
		RSIndex idx = RSArrayIndexOfObject(rl->_sources, source);
		if (idx == RSNotFound)
		{
			RSSpinLockUnlock(&rl->_lockForSources);
			return;
		}
		if (source->_context.context1.cancel) source->_context.context1.cancel(source->_context.context1.info, rl, mode);
		RSArrayRemoveObjectAtIndex(rl->_sources, idx);
        source->_rl = nil;
		RSSpinLockUnlock(&rl->_lockForSources);
    }
}

RSExport void RSRunLoopRemoveSource(RSRunLoopRef rl, RSRunLoopSourceRef source, RSStringRef mode)
{
    if (rl == nil) rl = RSRunLoopCurrentLoop();
    else __RSGenericValidInstance(rl, _RSRunLoopTypeID);
    __RSRunLoopLock(rl);
    __RSRunLoopRemoveSource(rl, source, mode);
    __RSRunLoopUnLock(rl);
}

#pragma mark -
#pragma mark RSRunLoop Run mode

static void __RSRunLoopActiveSource0(RSRunLoopSourceRef rls, void* callContext)
{
    if (rls == nil) return;
    if (__RSRunLoopSourceIsFinished(rls))
        return;
    
	RSRetain(rls);
    __RSRunLoopSourceLock(rls);
    __RSRunLoopSourceMarkRunning(rls);
    if (rls->_context.context1.perform) rls->_context.context1.perform(rls->_context.context1.info);
    __RSRunLoopSourceMarkFinished(rls);
    __RSRunLoopSourceUnLock(rls);
	RSRelease(rls);
}

static void __RSRunLoopDoSourceWithMode(RSRunLoopRef rl, RSRunLoopSourceRef rls, RSRunLoopMode mode)
{
    dispatch_queue_t queue = nil;
    switch (mode._mode._modeField)
    {
        case ____kRSRunLoopModeQueue:   // RSRunLoopQueue
        {
            switch (mode._mode._zone0)
            {
                case ____kRSRunLoopModeQueueZone0_middle:
                    queue = dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, 0);
                    break;
                case ____kRSRunLoopModeQueueZone0_high:
                    queue = dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_HIGH, 0);
                    break;
                case ____kRSRunLoopModeQueueZone0_low:
                default:
                    queue = dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_LOW, 0);
                    break;
            }
            dispatch_sync(queue, ^{
                __RSRunLoopActiveSource0(rls, nil);
            });
            break;
        }
        case ____kRSRunLoopModeMain:   // RSRunLoopMain
        {
            queue = dispatch_get_main_queue();
            switch (mode._mode._zone0)
            {
                case ____kRSRunLoopModeMainZone0_fshedule:
                    break;
                case ____kRSRunLoopModeMainZone0_active:
                    break;
                case ____kRSRunLoopModeMainZone0_sshedule:
                    break;
                case ____kRSRunLoopModeMainZone0_all:
                    break;
                case ____kRSRunLoopModeMainZone0_fs:
                    break;
                case ____kRSRunLoopModeMainZone0_fa:
                    break;
                case ____kRSRunLoopModeMainZone0_as:
                    break;
                default:
                    break;
            }
            if (pthread_main_np())
            {
                __RSRunLoopActiveSource0(rls, nil);
            }
            else
            {
                dispatch_sync(queue, ^{
                    __RSRunLoopActiveSource0(rls, nil);
                });
            }
            break;
        }
        case ____kRSRunLoopModeUserDef: // RSRunLoopUserDefine
        {
            break;
        }
        case ____kRSRunLoopModeDefault:   // RSRunLoopDefault
        {
            switch (mode._mode._zone0)
            {
                case ____kRSRunLoopModeDefaultZone0_reserved0:
                case ____kRSRunLoopModeDefaultZone0_reserved1:
                case ____kRSRunLoopModeDefaultZone0_reserved2:
                case ____kRSRunLoopModeDefaultZone0_reserved3:
                default:
                    if (pthread_main_np())
                    {
                        dispatch_sync(((RSRunLoopRef)RSRunLoopCurrentLoop())->_queue, ^{
                            __RSRunLoopActiveSource0(rls, nil);
                        });
                    }
                    else
                    {
                        dispatch_sync(dispatch_get_current_queue(), ^{
                            __RSRunLoopActiveSource0(rls, nil);
                        });
                    }
//                    {
//                        dispatch_sync(((RSRunLoopRef)rl)->_queue, ^{
//                            __RSRunLoopActiveSource0(rls, nil);
//                        });
//                    }
                    break;
                    
            }
        }
        default:
        {
            break;
        }
    }
}

static void ___RSRunLoopSourceRemoveFromRunLoop(const void* value, void* context)
{
	RSRunLoopSourceRef source = (RSRunLoopSourceRef)value;
	RSRunLoopRef rl = (RSRunLoopRef)context;
	if (source->_context.context1.cancel) source->_context.context1.cancel(source->_context.context1.info, rl, rl->_mode);
}

static void __RSRunLoopDoSource0(RSRunLoopRef rl, RSMutableArrayRef sources, dispatch_queue_t queue, BOOL shouldWait)
{
    RSIndex cnt = RSArrayGetCount(sources);
    
    if (shouldWait)
    {
        dispatch_group_t grp = nil;
        grp = dispatch_group_create();
		RSSpinLockLock(&rl->_lockForSources);
        for (RSIndex idx = 0; idx < cnt; idx++)
        {
            dispatch_group_async(grp, queue, ^{
				if (__RSRunLoopIsWaiting(rl) && rl->_semaphore) dispatch_semaphore_wait(rl->_semaphore, DISPATCH_TIME_FOREVER);
                RSRunLoopSourceRef source = (RSRunLoopSourceRef)RSArrayObjectAtIndex(sources, idx);
                __RSRunLoopDoSourceWithMode(rl, source, source->_mode);
                return;
            });
        }
		RSSpinLockUnlock(&rl->_lockForSources);
        long error = dispatch_group_wait(grp, DISPATCH_TIME_FOREVER);
        if (error) printf("runloop has an error");
        dispatch_release(grp);
		RSSpinLockLock(&rl->_lockForSources);
		RSArrayApplyFunction(sources, RSMakeRange(0, cnt), ___RSRunLoopSourceRemoveFromRunLoop, rl);
        RSArrayRemoveAllObjects(sources);
		RSSpinLockUnlock(&rl->_lockForSources);
    }
    else
    {
		RSSpinLockLock(&rl->_lockForSources);
        __block RSIndex finished = 0;
        for (RSIndex idx = 0; idx < cnt; idx++)
        {
            dispatch_async(queue, ^{
				if (__RSRunLoopIsWaiting(rl) && rl->_semaphore) dispatch_semaphore_wait(rl->_semaphore, DISPATCH_TIME_FOREVER);
                RSRunLoopSourceRef source = (RSRunLoopSourceRef)RSArrayObjectAtIndex(sources, idx);
                __RSRunLoopDoSourceWithMode(rl, source, source->_mode);	// lock source
                finished++;
               
                if (finished == cnt)
                {   
                    RSSpinLockUnlock(&rl->_lockForSources);
                    __RSRunLoopUnLock(rl);
                }
                return;
            });
        }
    }
}
static void __RSRunLoopDoSourceGPriorityDefault0(RSRunLoopRef rl, RSMutableArrayRef sources, BOOL shouldWait);
static void __RSRunLoopDoSource1(RSRunLoopRef rl, RSMutableArrayRef sources, dispatch_queue_t queue, BOOL shouldWait)
{
    // __RSRunLoopDoSource1 only called when the source run in the main thread
    // if the source need update the UI, should be put RSRunLoopMain mode,
    // and __RSRunLoopDoSourceWithMode will re-push the source to the main queue to active it.
    RSIndex cnt __unused = RSArrayGetCount(sources);
    __RSRunLoopDoSourceGPriorityDefault0(rl, sources, shouldWait);
    return;
    for (RSIndex idx = 0; idx < cnt; idx++)
    {
        dispatch_async(queue, ^{
            RSRunLoopSourceRef source = (RSRunLoopSourceRef)RSArrayObjectAtIndex(sources, idx);
            __RSRunLoopDoSourceWithMode(rl, source, source->_mode);
        });
    }
}

static void __RSRunLoopDoSourceGPriorityDefault0(RSRunLoopRef rl, RSMutableArrayRef sources, BOOL shouldWait)
{
    dispatch_queue_t queue = dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, 0);
    __RSRunLoopDoSource0(rl, sources, queue, shouldWait);
}

static void __RSRunLoopDoSourceGPriorityHigh0(RSRunLoopRef rl, RSMutableArrayRef sources, BOOL shouldWait)
{
    dispatch_queue_t queue = dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_HIGH, 0);
    __RSRunLoopDoSource0(rl, sources, queue, shouldWait);
}

static void __RSRunLoopDoSourceGPriorityLow0(RSRunLoopRef rl, RSMutableArrayRef sources, BOOL shouldWait)
{
    dispatch_queue_t queue = dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_LOW, 0);
    __RSRunLoopDoSource0(rl, sources, queue, shouldWait);
}

// __RSRunLoopDoSourceWithRunLoopMode CALLED BY RSRunLoopRun().
// rl WILL BE LOCKED.
// if shouldWait is YES, __RSRunLoopDoSourceWithRunLoopMode will keep the runloop lock until task is over.
static void __RSRunLoopDoSourceWithRunLoopMode(RSRunLoopRef rl, RSRunLoopMode mode, BOOL shouldWait)
{
    switch (mode._mode._modeField)
    {
        case ____kRSRunLoopModeQueue:   // RSRunLoopQueue
        {
            switch (mode._mode._zone0)
            {
                case ____kRSRunLoopModeQueueZone0_middle:
                    __RSRunLoopDoSourceGPriorityDefault0(rl, rl->_sources, shouldWait);
                    break;
                case ____kRSRunLoopModeQueueZone0_high:
                    __RSRunLoopDoSourceGPriorityHigh0(rl, rl->_sources, shouldWait);
                    break;
                case ____kRSRunLoopModeQueueZone0_low:
                    __RSRunLoopDoSourceGPriorityLow0(rl, rl->_sources, shouldWait);
                    break;
            }
            break;
        }
        case ____kRSRunLoopModeMain:   // RSRunLoopMain
        {
            switch (mode._mode._zone0)
            {
                case ____kRSRunLoopModeMainZone0_fshedule:
                    break;
                case ____kRSRunLoopModeMainZone0_active:
                    break;
                case ____kRSRunLoopModeMainZone0_sshedule:
                    break;
                case ____kRSRunLoopModeMainZone0_all:
                    break;
                case ____kRSRunLoopModeMainZone0_fs:
                    break;
                case ____kRSRunLoopModeMainZone0_fa:
                    break;
                case ____kRSRunLoopModeMainZone0_as:
                    break;
                default:
                    break;
            }
            break;
        }
        case ____kRSRunLoopModeUserDef: // RSRunLoopUserDefine
        {
            break;
        }
        case ____kRSRunLoopModeDefault:   // RSRunLoopDefault
        {
            switch (mode._mode._zone0)
            {
                case ____kRSRunLoopModeDefaultZone0_reserved0:
                case ____kRSRunLoopModeDefaultZone0_reserved1:
                case ____kRSRunLoopModeDefaultZone0_reserved2:
                case ____kRSRunLoopModeDefaultZone0_reserved3:
                default:
                    if (rl == RSRunLoopMainLoop())
                    {
                        __RSRunLoopDoSource1(rl, rl->_sources, rl->_queue, shouldWait);
                    }
                    else __RSRunLoopDoSource0(rl, rl->_sources, rl->_queue, shouldWait);
                    break;
					
            }
        }
        default:
        {
            break;
        }
    }
}

static void __RSRunLoopRunCoreEntry(RSRunLoopRef rl, BOOL shouldWait)
{
	if (__RSRunLoopIsWaiting(rl))
	{
		RSRunLoopWakeUp(rl);
	}
    __RSRunLoopLock(rl);
    RSArrayRef sources = rl->_sources;
    RSIndex cnt = RSArrayGetCount(sources);
    if (cnt < 1)
    {
        __RSRunLoopUnLock(rl);
        return;
    }
    RSRunLoopMode _mode = __RSRunLoopModeTranslateNameToMode(RSAllocatorSystemDefault, rl->_mode);
    if (!shouldWait) _mode._mode._autoremove = YES;
    __RSRunLoopDoSourceWithRunLoopMode(rl, _mode, shouldWait); // if nbg is NO,<asyc> should keep lock until done.
    if (shouldWait) __RSRunLoopUnLock(rl);
}

RSExport void RSRunLoopRun()
{
    return __RSRunLoopRunCoreEntry(RSRunLoopCurrentLoop(), YES);
}

RSExport void _RSRunLoopRunBackGround()
{
    return __RSRunLoopRunCoreEntry(RSRunLoopCurrentLoop(), NO);
}

RSExport void RSPerformFunctionInBackGround(void (*perform)(void *info), void *info, BOOL waitUntilDone)
{
    if (waitUntilDone == NO)
    {
        RSPerformBlockInBackGround(^{
            perform(info);
        });
    }
    else
    {
        RSPerformBlockInBackGroundWaitUntilDone(^{
            perform(info);
        });
    }
}

RSExport void RSPerformFunctionOnMainThread(void (*perform)(void *info), void *info, BOOL waitUntilDone)
{
    if (waitUntilDone == NO)
    {
        RSPerformBlockOnMainThread(^{
            perform(info);
        });
    }
    else
    {
        RSPerformBlockOnMainThreadWaitUntilDone(^{
            perform(info);
        });
    }
}

RSExport void RSPerformFunctionAfterDelay(void (*perform)(void *info), void *info, RSTimeInterval delayTime)
{
    return RSPerformBlockAfterDelay(delayTime, ^{
        perform(info);
    });
}

#if RS_BLOCKS_AVAILABLE
typedef void (^_performBlock) (void);
struct __block__perform_info
{
    _performBlock perform;
};
#include <Block.h>

static void __block__perform(void *info)
{
    struct __block__perform_info *pbpi = (struct __block__perform_info *)info;
    pbpi->perform();
    _Block_release(pbpi->perform);
    RSAllocatorDeallocate(RSAllocatorSystemDefault, pbpi);
}

RSExport void RSPerformBlockInBackGround(void (^perform)())
{
//    dispatch_async(dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_HIGH, 0), ^{
//        RSAutoreleaseBlock(^{
//            perform();
//        });
//    });
//    return;
    struct __block__perform_info *pbpi = RSAllocatorAllocate(RSAllocatorSystemDefault, sizeof(struct __block__perform_info));
    pbpi->perform = (_performBlock)_Block_copy(^(){
        RSAutoreleaseBlock(^{
            perform();
        });
    });
    __RSStartSimpleThread(__block__perform, pbpi);
}

RSExport void RSPerformBlockInBackGroundWaitUntilDone(void (^perform)())
{
    dispatch_semaphore_t semaphore = dispatch_semaphore_create(0);
    dispatch_async(dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_BACKGROUND, 0), ^{
        RSAutoreleaseBlock(^{
            perform();
        });
        dispatch_semaphore_signal(semaphore);
    });
    dispatch_semaphore_wait(semaphore, DISPATCH_TIME_FOREVER);
    dispatch_release(semaphore);
}

RSExport void RSPerformBlockOnMainThread(void (^perform)())
{
    if (pthread_main_np())
    {
        RSAutoreleaseBlock(^{
            perform();
        });
    }
    else
    {
        dispatch_async(dispatch_get_main_queue(), ^{
            RSAutoreleaseBlock(^{
                perform();
            });
        });
    }
}

RSExport void RSPerformBlockOnMainThreadWaitUntilDone(void (^perform)())
{
    if (pthread_main_np())
    {
        RSAutoreleaseBlock(^{
            perform();
        });
    }
    else if (1)
    {
        dispatch_semaphore_t semaphore = dispatch_semaphore_create(0);
        dispatch_async(dispatch_get_main_queue(), ^{
            RSAutoreleaseBlock(^{
                perform();
            });
            dispatch_semaphore_signal(semaphore);
        });
        dispatch_semaphore_wait(semaphore, DISPATCH_TIME_FOREVER);
        dispatch_release(semaphore);
    }
}

RSExport void RSPerformBlockAfterDelay(RSTimeInterval timeInterval, void (^perform)())
{
    RSTimerRef timer = RSTimerCreateSchedule(RSAllocatorSystemDefault, RSAbsoluteTimeGetCurrent() + timeInterval, 0, NO, nil, ^(RSTimerRef timer) {
        RSAutoreleaseBlock(^{
            perform();
        });
        RSTimerInvalidate(timer);
    });
    RSTimerFire(timer);
    return;
    dispatch_after(dispatch_time(0, timeInterval), dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_HIGH, 0), ^{
        perform();
    });
}

RSExport void RSPerformBlockRepeat(RSIndex performCount, void (^perform)(RSIndex idx))
{
    RSPerformBlockRepeatWithFlags(performCount, RSPerformBlockDefault, perform);
}

RSExport void RSPerformBlockRepeatWithFlags(RSIndex performCount, RSOptionFlags performFlags, void (^perform)(RSIndex idx))
{
    if (performCount > 0 && perform)
    {
        dispatch_apply(performCount, dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_HIGH, (unsigned long)performFlags), ^(size_t idx) {
            RSAutoreleaseBlock(^{
                perform(idx);
            });
        });
    }

}
#endif

RSExport void RSRunLoopPerformFunctionInRunLoop(RSRunLoopRef runloop, void(*perform)(void* info), void* info, RSStringRef mode)
{
    if (runloop == nil) RSRunLoopPerformFunctionInRunLoop(RSRunLoopCurrentLoop(), perform, info, mode);
    else __RSGenericValidInstance(runloop, _RSRunLoopTypeID);
    
    if (perform)
    {
        RSRunLoopSourceContext context = {0, info, nil, nil, nil, nil, nil, nil, nil, perform};
        RSRunLoopSourceRef source = RSRunLoopSourceCreate(RSAllocatorSystemDefault, 0, &context);
        RSRunLoopDoSourceInRunLoop(runloop, source, mode);
        RSRelease(source);
    }
}

RSExport void RSRunLoopDoSourceInRunLoop(RSRunLoopRef runloop, RSRunLoopSourceRef source, RSStringRef mode)
{
    if (source == nil) return;
    if (runloop == nil) //RSRunLoopDoSourceInRunLoop(RSRunLoopCurrentLoop(), source, mode);
        runloop = RSRunLoopCurrentLoop();
    else __RSGenericValidInstance(runloop, _RSRunLoopTypeID);
    
    RSRunLoopAddSource(runloop, source, mode);
    
    __RSRunLoopRunCoreEntry(runloop, NO);
}
RSExport void RSRunLoopDoSourcesInRunLoop(RSRunLoopRef runloop, RSArrayRef sources, RSStringRef mode)
{
    if (sources == nil) return;
    RSIndex cnt = RSArrayGetCount(sources);
    if (cnt < 1) return;
    if (runloop == nil) return RSRunLoopDoSourcesInRunLoop(RSRunLoopCurrentLoop(), sources, mode);
    else __RSGenericValidInstance(runloop, _RSRunLoopTypeID);
    
    for (RSIndex idx = 0; idx < cnt; ++idx)
    {
        RSRunLoopSourceRef source = (RSRunLoopSourceRef)RSArrayObjectAtIndex(sources, idx);
        RSRunLoopAddSource(runloop, source, mode);
    }
    __RSRunLoopRunCoreEntry(runloop, NO);
}

RSPrivate void __RSRunLoopDeallocate()
{
    RSDeallocateInstance(_RSRunLoopMain);
}
