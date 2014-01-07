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
#include "RSPrivate/RSPrivateOperatingSystem/RSPrivateFileSystem.h"

#pragma mark -
#pragma mark RSRunLoop Private methods inside

RS_PUBLIC_CONST_STRING_DECL(RSRunLoopDefault, "RSRunLoopDefaultMode");
RS_PUBLIC_CONST_STRING_DECL(RSRunLoopQueue, "RSRunLoopQueue");
RS_PUBLIC_CONST_STRING_DECL(RSRunLoopMain, "RSRunLoopMain");

RS_PUBLIC_CONST_STRING_DECL(RSRunLoopDefaultMode, "RSRunLoopDefaultMode");
RS_PUBLIC_CONST_STRING_DECL(RSRunLoopCommonModes, "RSRunLoopCommonModes");

static pthread_key_t const __RSRunLoopThreadKey = RSRunLoopKey;

#define RSRunLoopVersion    3

#define HALT    __HALT
extern int64_t __RSTimeIntervalToTSR(RSTimeInterval ti);
extern RSTimeInterval __RSTSRToTimeInterval(int64_t tsr);
#ifndef CHECK_FOR_FORK
#define CHECK_FOR_FORK(...)
#endif

#if (DEPLOYMENT_TARGET_MACOSX || DEPLOYMENT_TARGET_IPHONEOS) && RSCoreFoundationVersionNumber0_4
#include <malloc/malloc.h>
// VM Pressure source for malloc <rdar://problem/7805121>
static dispatch_source_t __runloop_malloc_vm_pressure_source;

RSExport void __runloop_vm_pressure_handler(void *context __unused)
{
	malloc_zone_pressure_relief(0,0);
}

enum {
	DISPATCH_VM_PRESSURE = 0x80000000,
};

enum {
	DISPATCH_MEMORYSTATUS_PRESSURE_NORMAL = 0x01,
	DISPATCH_MEMORYSTATUS_PRESSURE_WARN = 0x02,
#if !TARGET_OS_EMBEDDED
	DISPATCH_MEMORYSTATUS_PRESSURE_CRITICAL = 0x04,
#endif
};

static void __runloop_malloc_vm_pressure_setup(void)
{
	__runloop_malloc_vm_pressure_source = dispatch_source_create(DISPATCH_SOURCE_TYPE_MEMORYPRESSURE, 0, DISPATCH_MEMORYSTATUS_PRESSURE_NORMAL | DISPATCH_MEMORYSTATUS_PRESSURE_WARN, dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, 2));
	dispatch_source_set_event_handler_f(__runloop_malloc_vm_pressure_source,
                                        __runloop_vm_pressure_handler);
	dispatch_resume(__runloop_malloc_vm_pressure_source);
}
#else
#define __runloop_malloc_vm_pressure_setup()
#endif

RSPrivate RSStringRef __RSRunLoopThreadToString(pthread_t t)
{
    RSStringRef thread = nil;
#if DEPLOYMENT_TARGET_MACOSX || DEPLOYMENT_TARGET_IPHONEOS
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

#if (RSRunLoopVersion == 1)
static RSRunLoopRef _RSRunLoopMain = nil;

#pragma mark -
#pragma mark RSRunLoopMode API

static const int ____RSRunLoopModeDefault  = 0b00000;
static const int ____RSRunLoopModeQueue    = 0b00001;
static const int ____RSRunLoopModeMain     = 0b00010;
static const int ____RSRunLoopModeUserDef  = 0b00011;

static const int ____RSRunLoopZone0_default_000 = 0b000;
static const int ____RSRunLoopZone0_default_001 = 0b001;
static const int ____RSRunLoopZone0_default_010 = 0b010;
static const int ____RSRunLoopZone0_default_011 = 0b011;
static const int ____RSRunLoopZone0_default_100 = 0b100;
static const int ____RSRunLoopZone0_default_101 = 0b101;
static const int ____RSRunLoopZone0_default_110 = 0b110;
static const int ____RSRunLoopZone0_default_111 = 0b111;

static const int ____RSRunLoopModeDefaultZone0_reserved0   = ____RSRunLoopZone0_default_000;
static const int ____RSRunLoopModeDefaultZone0_reserved1   = ____RSRunLoopZone0_default_001;
static const int ____RSRunLoopModeDefaultZone0_reserved2   = ____RSRunLoopZone0_default_010;
static const int ____RSRunLoopModeDefaultZone0_reserved3   = ____RSRunLoopZone0_default_011;

static const int ____RSRunLoopModeQueueZone0_low           = ____RSRunLoopZone0_default_000;
static const int ____RSRunLoopModeQueueZone0_middle        = ____RSRunLoopZone0_default_001;
static const int ____RSRunLoopModeQueueZone0_high          = ____RSRunLoopZone0_default_010;

static const int ____RSRunLoopModeMainZone0_fshedule       = ____RSRunLoopZone0_default_001;
static const int ____RSRunLoopModeMainZone0_active         = ____RSRunLoopZone0_default_010;
static const int ____RSRunLoopModeMainZone0_sshedule       = ____RSRunLoopZone0_default_100;
static const int ____RSRunLoopModeMainZone0_all            = ____RSRunLoopZone0_default_111;
static const int ____RSRunLoopModeMainZone0_fa             = ____RSRunLoopZone0_default_011;
static const int ____RSRunLoopModeMainZone0_fs             = ____RSRunLoopZone0_default_101;
static const int ____RSRunLoopModeMainZone0_as             = ____RSRunLoopZone0_default_110;

static const int ____RSRunLoopModeUserDefZone0_compute     = ____RSRunLoopZone0_default_000;
static const int ____RSRunLoopModeUserDefZone0_DiskIO      = ____RSRunLoopZone0_default_001;
static const int ____RSRunLoopModeUserDefZone0_NetIO       = ____RSRunLoopZone0_default_010;
static const int ____RSRunLoopModeUserDefZone0_memory      = ____RSRunLoopZone0_default_100;
static const int ____RSRunLoopModeUserDefZone0_dn          = ____RSRunLoopZone0_default_011;
static const int ____RSRunLoopModeUserDefZone0_dm          = ____RSRunLoopZone0_default_101;
static const int ____RSRunLoopModeUserDefZone0_nm          = ____RSRunLoopZone0_default_110;
static const int ____RSRunLoopModeUserDefZone0_all         = ____RSRunLoopZone0_default_111;


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
        __cmode._modeField = ____RSRunLoopModeDefault;
        __cmode._zone0 = 0b000;
        __cmode._reserved = 0;
    }
    else if (modeName == RSRunLoopQueue ||
             RSEqual(modeName, RSRunLoopQueue))
    {
        __cmode._sync = YES;
        __cmode._modeField = ____RSRunLoopModeQueue;
        __cmode._zone0 = 0b001;
        __cmode._reserved = 0;
    }
    else if (modeName == RSRunLoopMain ||
             RSEqual(modeName, RSRunLoopMain))
    {
        __cmode._sync = YES;
        __cmode._modeField = ____RSRunLoopModeMain;
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

RSExport void RSRunLoopRemoveSource(RSRunLoopSourceRef source, RSStringRef mode)
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

#if DEPLOYMENT_TARGET_MACOSX || DEPLOYMENT_TARGET_IPHONEOS
#include <malloc/malloc.h>
// VM Pressure source for malloc <rdar://problem/7805121>
static dispatch_source_t __runtime_malloc_vm_pressure_source;

RSExport void __runtime_vm_pressure_handler(void *context __unused)
{
	malloc_zone_pressure_relief(0,0);
    __RSCLog(RSLogLevelWarning, "%s\n", "RSRuntime reduce memory alloc to handle the memory pressure...");
}

enum {
	DISPATCH_VM_PRESSURE = 0x80000000,
};

enum {
	DISPATCH_MEMORYSTATUS_PRESSURE_NORMAL = 0x01,
	DISPATCH_MEMORYSTATUS_PRESSURE_WARN = 0x02,
#if !TARGET_OS_EMBEDDED
	DISPATCH_MEMORYSTATUS_PRESSURE_CRITICAL = 0x04,
#endif
};


#include <sys/event.h>

struct dispatch_source_type_s {
	struct kevent ke;
	uint64_t mask;
	void (*init)(dispatch_source_t ds, dispatch_source_type_t type,
                 uintptr_t handle, unsigned long mask, dispatch_queue_t q);
};

#define _OS_OBJECT_HEADER(isa, ref_cnt, xref_cnt) \
isa; /* must be pointer-sized */ \
int volatile ref_cnt; \
int volatile xref_cnt

#define DISPATCH_STRUCT_HEADER(x) \
_OS_OBJECT_HEADER( \
const struct dispatch_##x##_vtable_s *do_vtable, \
do_ref_cnt, \
do_xref_cnt); \
struct dispatch_##x##_s *volatile do_next; \
struct dispatch_queue_s *do_targetq; \
void *do_ctxt; \
void *do_finalizer; \
unsigned int do_suspend_cnt;

#define DISPATCH_QUEUE_HEADER \
uint32_t volatile dq_running; \
uint32_t dq_width; \
struct dispatch_object_s *volatile dq_items_tail; \
struct dispatch_object_s *volatile dq_items_head; \
unsigned long dq_serialnum; \
dispatch_queue_t dq_specific_q;

#define DISPATCH_QUEUE_MIN_LABEL_SIZE 64

struct dispatch_kevent_s {
	TAILQ_ENTRY(dispatch_kevent_s) dk_list;
	TAILQ_HEAD(, dispatch_source_refs_s) dk_sources;
	struct kevent dk_kevent;
};

typedef struct dispatch_kevent_s *dispatch_kevent_t;

typedef struct dispatch_source_refs_s *dispatch_source_refs_t;

struct dispatch_timer_source_s {
	uint64_t target;
	uint64_t last_fire;
	uint64_t interval;
	uint64_t leeway;
	uint64_t flags; // dispatch_timer_flags_t
	unsigned long missed;
};

struct dispatch_source_refs_s {
	TAILQ_ENTRY(dispatch_source_refs_s) dr_list;
	uintptr_t dr_source_wref; // "weak" backref to dispatch_source_t
	dispatch_function_t ds_handler_func;
	void *ds_handler_ctxt;
	void *ds_cancel_handler;
	void *ds_registration_handler;
};

struct dispatch_timer_source_refs_s {
	struct dispatch_source_refs_s _ds_refs;
	struct dispatch_timer_source_s _ds_timer;
};

struct dispatch_source_s {
	DISPATCH_STRUCT_HEADER(source);
	DISPATCH_QUEUE_HEADER;
	// Instruments always copies DISPATCH_QUEUE_MIN_LABEL_SIZE, which is 64,
	// so the remainder of the structure must be big enough
	union {
		char _ds_pad[DISPATCH_QUEUE_MIN_LABEL_SIZE];
		struct {
			char dq_label[8];
			dispatch_kevent_t ds_dkev;
			dispatch_source_refs_t ds_refs;
			unsigned int ds_atomic_flags;
			unsigned int
        ds_is_level:1,
        ds_is_adder:1,
        ds_is_installed:1,
        ds_needs_rearm:1,
        ds_is_timer:1,
        ds_cancel_is_block:1,
        ds_handler_is_block:1,
        ds_registration_is_block:1;
			unsigned long ds_data;
			unsigned long ds_pending_data;
			unsigned long ds_pending_data_mask;
			unsigned long ds_ident_hack;
		};
	};
};

static void
dispatch_source_type_vm_init(dispatch_source_t ds,
                             dispatch_source_type_t type __unused,
                             uintptr_t handle __unused,
                             unsigned long mask __unused,
                             dispatch_queue_t q __unused)
{
	ds->ds_is_level = NO;
}

const struct dispatch_source_type_s my_dispatch_source_type_vm = {
	.ke = {
		.filter = EVFILT_VM,
		.flags = EV_DISPATCH,
	},
	.mask = NOTE_VM_PRESSURE,
	.init = dispatch_source_type_vm_init,
};

#define DISPATCH_SOURCE_TYPE_VM (&my_dispatch_source_type_vm)

RSExport void __runtime_malloc_vm_pressure_setup(void)
{
    extern const struct dispatch_source_type_s _dispatch_source_type_vm;
    printf("_dispatch_source_type_vm address = %p, mask = %lld\nmy_dispatch_source_type_vm address = %p, mask = %lld\n",
           &_dispatch_source_type_vm, _dispatch_source_type_vm.mask,
           &my_dispatch_source_type_vm, my_dispatch_source_type_vm.mask);
//    dispatch_source_create(<#dispatch_source_type_t type#>, <#uintptr_t handle#>, <#unsigned long mask#>, <#dispatch_queue_t queue#>)
    //    DISPATCH_MEMORYSTATUS_PRESSURE_NORMAL | DISPATCH_MEMORYSTATUS_PRESSURE_WARN
	__runtime_malloc_vm_pressure_source = dispatch_source_create(&_dispatch_source_type_vm, 0, DISPATCH_VM_PRESSURE, dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, 2));
	dispatch_source_set_event_handler_f(__runtime_malloc_vm_pressure_source,
                                        __runtime_vm_pressure_handler);
	dispatch_resume(__runtime_malloc_vm_pressure_source);
}
#else
#define __runtime_malloc_vm_pressure_setup()
#endif

#define DISPATCH_UNUSED __unused

RSPrivate void __RSRunLoopInitialize()
{
    _RSRunLoopTypeID = __RSRuntimeRegisterClass(&__RSRunLoopClass);
    __RSRuntimeSetClassTypeID(&__RSRunLoopClass, _RSRunLoopTypeID);
    
    _RSRunLoopMain = RSRunLoopGetCurrent();
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

RSExport RSRunLoopRef RSRunLoopGetCurrent()
{
    return __RSRunLoopGetRunLoopForCurrentThread();
}

RSExport RSRunLoopRef RSRunLoopGetMain()
{
    return _RSRunLoopMain;
}

RSExport RSStringRef RSRunLoopModeName(RSRunLoopRef rl)
{
    if (rl == nil) rl = RSRunLoopGetCurrent();
    else __RSGenericValidInstance(rl, _RSRunLoopTypeID);
    return RSRetain(rl->_mode);
}

RSExport BOOL RSRunLoopIsWaiting(RSRunLoopRef rl)
{
    if (rl == nil) rl = RSRunLoopGetCurrent();
    else __RSGenericValidInstance(rl, _RSRunLoopTypeID);
    return __RSRunLoopIsWaiting(rl);
}

RSExport void RSRunLoopWakeUp(RSRunLoopRef rl)
{
    if (rl == nil) rl = RSRunLoopGetCurrent();
    else __RSGenericValidInstance(rl, _RSRunLoopTypeID);
	if (__RSRunLoopIsWaiting(rl) && rl->_semaphore)
	{
		__RSRunLoopWakeUp(rl);
		dispatch_semaphore_signal(rl->_semaphore);
	}
}

RSExport void RSRunLoopStop(RSRunLoopRef rl)
{
    if (rl == nil) rl = RSRunLoopGetCurrent();
    else __RSGenericValidInstance(rl, _RSRunLoopTypeID);
	if (rl->_semaphore == nil) rl->_semaphore = dispatch_semaphore_create(0);
	__RSRunLoopStop(rl);
}


RSExport BOOL RSRunLoopContainsSource(RSRunLoopRef rl, RSRunLoopSourceRef source, RSStringRef mode)
{
    if (rl == nil) rl = RSRunLoopGetCurrent();
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
    if (rl == nil) rl = RSRunLoopGetCurrent();
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
    if (rl == nil) rl = RSRunLoopGetCurrent();
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
        case ____RSRunLoopModeQueue:   // RSRunLoopQueue
        {
            switch (mode._mode._zone0)
            {
                case ____RSRunLoopModeQueueZone0_middle:
                    queue = dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, 0);
                    break;
                case ____RSRunLoopModeQueueZone0_high:
                    queue = dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_HIGH, 0);
                    break;
                case ____RSRunLoopModeQueueZone0_low:
                default:
                    queue = dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_LOW, 0);
                    break;
            }
            dispatch_sync(queue, ^{
                __RSRunLoopActiveSource0(rls, nil);
            });
            break;
        }
        case ____RSRunLoopModeMain:   // RSRunLoopMain
        {
            queue = dispatch_get_main_queue();
            switch (mode._mode._zone0)
            {
                case ____RSRunLoopModeMainZone0_fshedule:
                    break;
                case ____RSRunLoopModeMainZone0_active:
                    break;
                case ____RSRunLoopModeMainZone0_sshedule:
                    break;
                case ____RSRunLoopModeMainZone0_all:
                    break;
                case ____RSRunLoopModeMainZone0_fs:
                    break;
                case ____RSRunLoopModeMainZone0_fa:
                    break;
                case ____RSRunLoopModeMainZone0_as:
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
        case ____RSRunLoopModeUserDef: // RSRunLoopUserDefine
        {
            break;
        }
        case ____RSRunLoopModeDefault:   // RSRunLoopDefault
        {
            switch (mode._mode._zone0)
            {
                case ____RSRunLoopModeDefaultZone0_reserved0:
                case ____RSRunLoopModeDefaultZone0_reserved1:
                case ____RSRunLoopModeDefaultZone0_reserved2:
                case ____RSRunLoopModeDefaultZone0_reserved3:
                default:
                    if (pthread_main_np())
                    {
                        dispatch_sync(((RSRunLoopRef)RSRunLoopGetCurrent())->_queue, ^{
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
        case ____RSRunLoopModeQueue:   // RSRunLoopQueue
        {
            switch (mode._mode._zone0)
            {
                case ____RSRunLoopModeQueueZone0_middle:
                    __RSRunLoopDoSourceGPriorityDefault0(rl, rl->_sources, shouldWait);
                    break;
                case ____RSRunLoopModeQueueZone0_high:
                    __RSRunLoopDoSourceGPriorityHigh0(rl, rl->_sources, shouldWait);
                    break;
                case ____RSRunLoopModeQueueZone0_low:
                    __RSRunLoopDoSourceGPriorityLow0(rl, rl->_sources, shouldWait);
                    break;
            }
            break;
        }
        case ____RSRunLoopModeMain:   // RSRunLoopMain
        {
            switch (mode._mode._zone0)
            {
                case ____RSRunLoopModeMainZone0_fshedule:
                    break;
                case ____RSRunLoopModeMainZone0_active:
                    break;
                case ____RSRunLoopModeMainZone0_sshedule:
                    break;
                case ____RSRunLoopModeMainZone0_all:
                    break;
                case ____RSRunLoopModeMainZone0_fs:
                    break;
                case ____RSRunLoopModeMainZone0_fa:
                    break;
                case ____RSRunLoopModeMainZone0_as:
                    break;
                default:
                    break;
            }
            break;
        }
        case ____RSRunLoopModeUserDef: // RSRunLoopUserDefine
        {
            break;
        }
        case ____RSRunLoopModeDefault:   // RSRunLoopDefault
        {
            switch (mode._mode._zone0)
            {
                case ____RSRunLoopModeDefaultZone0_reserved0:
                case ____RSRunLoopModeDefaultZone0_reserved1:
                case ____RSRunLoopModeDefaultZone0_reserved2:
                case ____RSRunLoopModeDefaultZone0_reserved3:
                default:
                    if (rl == RSRunLoopGetMain())
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
    return __RSRunLoopRunCoreEntry(RSRunLoopGetCurrent(), YES);
}

RSExport void _RSRunLoopRunBackGround()
{
    return __RSRunLoopRunCoreEntry(RSRunLoopGetCurrent(), NO);
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

RSExport void RSRunLoopPerformFunctionInRunLoop(RSRunLoopRef runloop, void(*perform)(void* info), void* info, RSStringRef mode)
{
    if (runloop == nil) RSRunLoopPerformFunctionInRunLoop(RSRunLoopGetCurrent(), perform, info, mode);
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
    if (runloop == nil) //RSRunLoopDoSourceInRunLoop(RSRunLoopGetCurrent(), source, mode);
        runloop = RSRunLoopGetCurrent();
    else __RSGenericValidInstance(runloop, _RSRunLoopTypeID);
    
    RSRunLoopAddSource(runloop, source, mode);
    
    __RSRunLoopRunCoreEntry(runloop, NO);
}
RSExport void RSRunLoopDoSourcesInRunLoop(RSRunLoopRef runloop, RSArrayRef sources, RSStringRef mode)
{
    if (sources == nil) return;
    RSIndex cnt = RSArrayGetCount(sources);
    if (cnt < 1) return;
    if (runloop == nil) return RSRunLoopDoSourcesInRunLoop(RSRunLoopGetCurrent(), sources, mode);
    else __RSGenericValidInstance(runloop, _RSRunLoopTypeID);
    
    for (RSIndex idx = 0; idx < cnt; ++idx)
    {
        RSRunLoopSourceRef source = (RSRunLoopSourceRef)RSArrayObjectAtIndex(sources, idx);
        RSRunLoopAddSource(runloop, source, mode);
    }
    __RSRunLoopRunCoreEntry(runloop, NO);
}

#elif (RSRunLoopVersion == 2)

#pragma mark - RSRunLoop Apple Version
/*
 * Copyright (c) 2012 Apple Inc. All rights reserved.
 *
 * @APPLE_LICENSE_HEADER_START@
 *
 * This file contains Original Code and/or Modifications of Original Code
 * as defined in and that are subject to the Apple Public Source License
 * Version 2.0 (the 'License'). You may not use this file except in
 * compliance with the License. Please obtain a copy of the License at
 * http://www.opensource.apple.com/apsl/ and read it before using this
 * file.
 *
 * The Original Code and all software distributed under the License are
 * distributed on an 'AS IS' basis, WITHOUT WARRANTY OF ANY KIND, EITHER
 * EXPRESS OR IMPLIED, AND APPLE HEREBY DISCLAIMS ALL SUCH WARRANTIES,
 * INCLUDING WITHOUT LIMITATION, ANY WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE, QUIET ENJOYMENT OR NON-INFRINGEMENT.
 * Please see the License for the specific language governing rights and
 * limitations under the License.
 *
 * @APPLE_LICENSE_HEADER_END@
 */

/*	RSRunLoop.c
 Copyright (c) 1998-2012, Apple Inc. All rights reserved.
 Responsibility: Tony Parker
 */

#include <RSCoreFoundation/RSArray.h>
#include <RSCoreFoundation/RSRunLoop.h>
#include <RSCoreFoundation/RSSet.h>
//#include <RSCoreFoundation/RSBag.h>
#include "RSInternal.h"
#include <math.h>
#include <stdio.h>
#include <limits.h>
#include <pthread.h>
#include <dispatch/dispatch.h>
#if DEPLOYMENT_TARGET_MACOSX || DEPLOYMENT_TARGET_EMBEDDED || DEPLOYMENT_TARGET_EMBEDDED_MINI
//#include <dispatch/../private/private.h>
//#include <RSCoreFoundation/RSUserNotification.h>
#include <mach/mach.h>
#include <mach/clock_types.h>
#include <mach/clock.h>
#include <unistd.h>
#include <dlfcn.h>
extern mach_port_t _dispatch_get_main_queue_port_4RS(void);
extern mach_port_t _dispatch_runloop_root_queue_get_port_4RS(void);
extern void _dispatch_main_queue_callback_4RS(mach_msg_header_t *msg);

#elif DEPLOYMENT_TARGET_WINDOWS
#include <process.h>
DISPATCH_EXPORT HANDLE _dispatch_get_main_queue_handle_4RS(void);
DISPATCH_EXPORT void _dispatch_main_queue_callback_4RS(void);

#define MACH_PORT_NULL 0
#define mach_port_name_t HANDLE
#define _dispatch_get_main_queue_port_4RS _dispatch_get_main_queue_handle_4RS
#define _dispatch_main_queue_callback_4RS(x) _dispatch_main_queue_callback_4RS()

#define AbsoluteTime LARGE_INTEGER

#endif
#include <Block.h>

static int _LogRSRunLoop = 0;

// for conservative arithmetic safety, such that (TIMER_DATE_LIMIT + TIMER_INTERVAL_LIMIT + RSAbsoluteTimeIntervalSince1970) * 10^9 < 2^63
#define TIMER_DATE_LIMIT	4039289856.0
#define TIMER_INTERVAL_LIMIT	504911232.0

#ifndef DISPATCH_QUEUE_OVERCOMMIT
#define DISPATCH_QUEUE_OVERCOMMIT RSPerformBlockOverCommit
#endif

#define HANDLE_DISPATCH_ON_BASE_INVOCATION_ONLY 0

#if DEPLOYMENT_TARGET_WINDOWS

static pthread_t NilPthreadT = { nil, nil };
#define pthreadPointer(a) a.p
typedef	int kern_return_t;
#define KERN_SUCCESS 0

#else

static pthread_t NilPthreadT = (pthread_t)0;
#define pthreadPointer(a) a
#define lockCount(a) a
#endif


RSExport bool RSDictionaryGetKeyIfPresent(RSDictionaryRef dict, const void *key, const void **actualkey);

// In order to reuse most of the code across Mach and Windows v1 RunLoopSources, we define a
// simple abstraction layer spanning Mach ports and Windows HANDLES
#if DEPLOYMENT_TARGET_MACOSX || DEPLOYMENT_TARGET_EMBEDDED || DEPLOYMENT_TARGET_EMBEDDED_MINI

RSExport uint32_t __RSGetProcessPortCount(void) {
    ipc_info_space_t info;
    ipc_info_name_array_t table = 0;
    mach_msg_type_number_t tableCount = 0;
    ipc_info_tree_name_array_t tree = 0;
    mach_msg_type_number_t treeCount = 0;
    
    kern_return_t ret = mach_port_space_info(mach_task_self(), &info, &table, &tableCount, &tree, &treeCount);
    if (ret != KERN_SUCCESS) {
        return (uint32_t)0;
    }
    if (table != nil) {
        ret = vm_deallocate(mach_task_self(), (vm_address_t)table, tableCount * sizeof(*table));
    }
    if (tree != nil) {
        ret = vm_deallocate(mach_task_self(), (vm_address_t)tree, treeCount * sizeof(*tree));
    }
    return (uint32_t)tableCount;
}

extern RSMutableArrayRef __RSArrayCreateMutable0(RSAllocatorRef allocator, RSIndex capacity, const RSArrayCallBacks *callBacks);

RSExport RSArrayRef __RSStopAllThreads(void) {
    RSMutableArrayRef suspended_list = __RSArrayCreateMutable0(RSAllocatorSystemDefault, 0, nil);
    mach_port_t my_task = mach_task_self();
    mach_port_t my_thread = mach_thread_self();
    thread_act_array_t thr_list = 0;
    mach_msg_type_number_t thr_cnt = 0;
    
    // really, should loop doing the stopping until no more threads get added to the list N times in a row
    kern_return_t ret = task_threads(my_task, &thr_list, &thr_cnt);
    if (ret == KERN_SUCCESS) {
        for (RSIndex idx = 0; idx < thr_cnt; idx++) {
            thread_act_t thread = thr_list[idx];
            if (thread == my_thread) continue;
            if (RSArrayContainsObject(suspended_list, RSMakeRange(0, RSArrayGetCount(suspended_list)), (const void *)(uintptr_t)thread)) continue;
            ret = thread_suspend(thread);
            if (ret == KERN_SUCCESS) {
                RSArrayAddObject(suspended_list, (const void *)(uintptr_t)thread);
            } else {
                mach_port_deallocate(my_task, thread);
            }
        }
        vm_deallocate(my_task, (vm_address_t)thr_list, sizeof(thread_t) * thr_cnt);
    }
    mach_port_deallocate(my_task, my_thread);
    return suspended_list;
}

RSExport void __RSRestartAllThreads(RSArrayRef threads) {
    for (RSIndex idx = 0; idx < RSArrayGetCount(threads); idx++) {
        thread_act_t thread = (thread_act_t)(uintptr_t)RSArrayObjectAtIndex(threads, idx);
        kern_return_t ret = thread_resume(thread);
        if (ret != KERN_SUCCESS) {
            char msg[256];
            snprintf(msg, 256, "*** Failure from thread_resume (%d) ***", ret);
            //CRSetCrashLogMessage(msg);
            HALT;
        }
        mach_port_deallocate(mach_task_self(), thread);
    }
}

static uint32_t __RS_last_warned_port_count = 0;

static void foo() __attribute__((unused));
static void foo() {
    uint32_t pcnt = __RSGetProcessPortCount();
    if (__RS_last_warned_port_count + 1000 < pcnt) {
        RSArrayRef threads = __RSStopAllThreads();
        
        
        // do stuff here
//        RSOptionFlags responseFlags = 0;
//        SInt32 result = RSUserNotificationDisplayAlert(0.0, RSUserNotificationCautionAlertLevel, nil, nil, nil, RSSTR("High Mach Port Usage"), RSSTR("This application is using a lot of Mach ports."), RSSTR("Default"), RSSTR("Altern"), RSSTR("Other b"), &responseFlags);
//        if (0 != result) {
//            RSLog(3, RSSTR("ERROR"));
//        } else {
//            switch (responseFlags) {
//                case RSUserNotificationDefaultResponse: RSLog(3, RSSTR("DefaultR")); break;
//                case RSUserNotificationAlternateResponse: RSLog(3, RSSTR("AltR")); break;
//                case RSUserNotificationOtherResponse: RSLog(3, RSSTR("OtherR")); break;
//                case RSUserNotificationCancelResponse: RSLog(3, RSSTR("CancelR")); break;
//            }
//        }
        
        
        __RSRestartAllThreads(threads);
        RSRelease(threads);
        __RS_last_warned_port_count = pcnt;
    }
}


typedef mach_port_t __RSPort;
#define RSPORT_nil MACH_PORT_NULL
typedef mach_port_t __RSPortSet;

static void __THE_SYSTEM_HAS_NO_PORTS_AVAILABLE__(kern_return_t ret) __attribute__((noinline));
static void __THE_SYSTEM_HAS_NO_PORTS_AVAILABLE__(kern_return_t ret) { HALT; };

static __RSPort __RSPortAllocate(void) {
    __RSPort result = RSPORT_nil;
    kern_return_t ret = mach_port_allocate(mach_task_self(), MACH_PORT_RIGHT_RECEIVE, &result);
    if (KERN_SUCCESS != ret) {
        char msg[256];
        snprintf(msg, 256, "*** The system has no mach ports available. You may be able to diagnose which application(s) are using ports by using 'top' or Activity Monitor. (%d) ***", ret);
        //CRSetCrashLogMessage(msg);
        __THE_SYSTEM_HAS_NO_PORTS_AVAILABLE__(ret);
        return RSPORT_nil;
    }
    
    ret = mach_port_insert_right(mach_task_self(), result, result, MACH_MSG_TYPE_MAKE_SEND);
    if (KERN_SUCCESS != ret) {
        char msg[256];
        snprintf(msg, 256, "*** Unable to set send right on mach port. (%d) ***", ret);
        //CRSetCrashLogMessage(msg);
        HALT;
    }
    
    mach_port_limits_t limits;
    limits.mpl_qlimit = 1;
    ret = mach_port_set_attributes(mach_task_self(), result, MACH_PORT_LIMITS_INFO, (mach_port_info_t)&limits, MACH_PORT_LIMITS_INFO_COUNT);
    if (KERN_SUCCESS != ret) {
        char msg[256];
        snprintf(msg, 256, "*** Unable to set attributes on mach port. (%d) ***", ret);
        //CRSetCrashLogMessage(msg);
        mach_port_destroy(mach_task_self(), result);
        HALT;
    }
    
    return result;
}

RSInline void __RSPortFree(__RSPort port) {
    mach_port_destroy(mach_task_self(), port);
}

static void __THE_SYSTEM_HAS_NO_PORT_SETS_AVAILABLE__(kern_return_t ret) __attribute__((noinline));
static void __THE_SYSTEM_HAS_NO_PORT_SETS_AVAILABLE__(kern_return_t ret) { HALT; };

RSInline __RSPortSet __RSPortSetAllocate(void) {
    __RSPortSet result;
    kern_return_t ret = mach_port_allocate(mach_task_self(), MACH_PORT_RIGHT_PORT_SET, &result);
    if (KERN_SUCCESS != ret) { __THE_SYSTEM_HAS_NO_PORT_SETS_AVAILABLE__(ret); }
    return (KERN_SUCCESS == ret) ? result : RSPORT_nil;
}

RSInline kern_return_t __RSPortSetInsert(__RSPort port, __RSPortSet portSet) {
    if (MACH_PORT_NULL == port) {
        return -1;
    }
    return mach_port_insert_member(mach_task_self(), port, portSet);
}

RSInline kern_return_t __RSPortSetRemove(__RSPort port, __RSPortSet portSet) {
    if (MACH_PORT_NULL == port) {
        return -1;
    }
    return mach_port_extract_member(mach_task_self(), port, portSet);
}

RSInline void __RSPortSetFree(__RSPortSet portSet) {
    kern_return_t ret;
    mach_port_name_array_t array;
    mach_msg_type_number_t idx, number;
    
    ret = mach_port_get_set_status(mach_task_self(), portSet, &array, &number);
    if (KERN_SUCCESS == ret) {
        for (idx = 0; idx < number; idx++) {
            mach_port_extract_member(mach_task_self(), array[idx], portSet);
        }
        vm_deallocate(mach_task_self(), (vm_address_t)array, number * sizeof(mach_port_name_t));
    }
    mach_port_destroy(mach_task_self(), portSet);
}

#elif DEPLOYMENT_TARGET_WINDOWS

typedef HANDLE __RSPort;
#define RSPORT_nil nil

// A simple dynamic array of HANDLEs, which grows to a high-water mark
typedef struct ___RSPortSet {
    uint16_t	used;
    uint16_t	size;
    HANDLE	*handles;
    RSSpinLock_t lock;		// insert and remove must be thread safe, like the Mach calls
} *__RSPortSet;

RSInline __RSPort __RSPortAllocate(void) {
    return CreateEventA(nil, YES, NO, nil);
}

RSInline void __RSPortFree(__RSPort port) {
    CloseHandle(port);
}

static __RSPortSet __RSPortSetAllocate(void) {
    __RSPortSet result = (__RSPortSet)RSAllocatorAllocate(RSAllocatorSystemDefault, sizeof(struct ___RSPortSet), 0);
    result->used = 0;
    result->size = 4;
    result->handles = (HANDLE *)RSAllocatorAllocate(RSAllocatorSystemDefault, result->size * sizeof(HANDLE), 0);
    RS_SPINLOCK_INIT_FOR_STRUCTS(result->lock);
    return result;
}

static void __RSPortSetFree(__RSPortSet portSet) {
    RSAllocatorDeallocate(RSAllocatorSystemDefault, portSet->handles);
    RSAllocatorDeallocate(RSAllocatorSystemDefault, portSet);
}

// Returns portBuf if ports fit in that space, else returns another ptr that must be freed
static __RSPort *__RSPortSetGetPorts(__RSPortSet portSet, __RSPort *portBuf, uint32_t bufSize, uint32_t *portsUsed) {
    RSSpinLockLock(&(portSet->lock));
    __RSPort *result = portBuf;
    if (bufSize < portSet->used)
        result = (__RSPort *)RSAllocatorAllocate(RSAllocatorSystemDefault, portSet->used * sizeof(HANDLE), 0);
    if (portSet->used > 1) {
        // rotate the ports to vaguely simulate round-robin behaviour
        uint16_t lastPort = portSet->used - 1;
        HANDLE swapHandle = portSet->handles[0];
        memmove(portSet->handles, &portSet->handles[1], lastPort * sizeof(HANDLE));
        portSet->handles[lastPort] = swapHandle;
    }
    memmove(result, portSet->handles, portSet->used * sizeof(HANDLE));
    *portsUsed = portSet->used;
    RSSpinLockUnlock(&(portSet->lock));
    return result;
}

static kern_return_t __RSPortSetInsert(__RSPort port, __RSPortSet portSet) {
    if (nil == port) {
        return -1;
    }
    RSSpinLockLock(&(portSet->lock));
    if (portSet->used >= portSet->size) {
        portSet->size += 4;
        portSet->handles = (HANDLE *)RSAllocatorReallocate(RSAllocatorSystemDefault, portSet->handles, portSet->size * sizeof(HANDLE), 0);
    }
    if (portSet->used >= MAXIMUM_WAIT_OBJECTS) {
        RSLog(RSLogLevelWarning, RSSTR("*** More than MAXIMUM_WAIT_OBJECTS (%d) ports add to a port set.  The last ones will be ignored."), MAXIMUM_WAIT_OBJECTS);
    }
    portSet->handles[portSet->used++] = port;
    RSSpinLockUnlock(&(portSet->lock));
    return KERN_SUCCESS;
}

static kern_return_t __RSPortSetRemove(__RSPort port, __RSPortSet portSet) {
    int i, j;
    if (nil == port) {
        return -1;
    }
    RSSpinLockLock(&(portSet->lock));
    for (i = 0; i < portSet->used; i++) {
        if (portSet->handles[i] == port) {
            for (j = i+1; j < portSet->used; j++) {
                portSet->handles[j-1] = portSet->handles[j];
            }
            portSet->used--;
            RSSpinLockUnlock(&(portSet->lock));
            return YES;
        }
    }
    RSSpinLockUnlock(&(portSet->lock));
    return KERN_SUCCESS;
}

#endif

#if !defined(__MACTYPES__) && !defined(_OS_OSTYPES_H)
#if defined(__BIG_ENDIAN__)
typedef	struct UnsignedWide {
    UInt32		hi;
    UInt32		lo;
} UnsignedWide;
#elif defined(__LITTLE_ENDIAN__)
typedef	struct UnsignedWide {
    UInt32		lo;
    UInt32		hi;
} UnsignedWide;
#endif
typedef UnsignedWide		AbsoluteTime;
#endif

#if DEPLOYMENT_TARGET_MACOSX || DEPLOYMENT_TARGET_IPHONEOS || DEPLOYMENT_TARGET_EMBEDDED || DEPLOYMENT_TARGET_EMBEDDED_MINI
#include <mach/mach_time.h>
#endif

#if DEPLOYMENT_TARGET_MACOSX || DEPLOYMENT_TARGET_EMBEDDED || DEPLOYMENT_TARGET_EMBEDDED_MINI
extern mach_port_name_t mk_timer_create(void);
extern kern_return_t mk_timer_destroy(mach_port_name_t name);
extern kern_return_t mk_timer_arm(mach_port_name_t name, AbsoluteTime expire_time);
extern kern_return_t mk_timer_cancel(mach_port_name_t name, AbsoluteTime *result_time);

RSInline AbsoluteTime __RSUInt64ToAbsoluteTime(int64_t x) {
    AbsoluteTime a;
    a.hi = x >> 32;
    a.lo = x & (int64_t)0xFFFFFFFF;
    return a;
}

static uint32_t __RSSendTrivialMachMessage(mach_port_t port, uint32_t msg_id, RSOptionFlags options, uint32_t timeout) {
    kern_return_t result;
    mach_msg_header_t header;
    header.msgh_bits = MACH_MSGH_BITS(MACH_MSG_TYPE_COPY_SEND, 0);
    header.msgh_size = sizeof(mach_msg_header_t);
    header.msgh_remote_port = port;
    header.msgh_local_port = MACH_PORT_NULL;
    header.msgh_id = msg_id;
    result = mach_msg(&header, MACH_SEND_MSG|(mach_msg_option_t)options, header.msgh_size, 0, MACH_PORT_NULL, timeout, MACH_PORT_NULL);
    if (result == MACH_SEND_TIMED_OUT) mach_msg_destroy(&header);
    return result;
}
#elif DEPLOYMENT_TARGET_WINDOWS

static HANDLE mk_timer_create(void) {
    return CreateWaitableTimer(nil, NO, nil);
}

static kern_return_t mk_timer_destroy(HANDLE name) {
    BOOL res = CloseHandle(name);
    if (!res) {
        DWORD err = GetLastError();
        RSLog(RSLogLevelError, RSSTR("RSRunLoop: Unable to destroy timer: %d"), err);
    }
    return (int)res;
}

static kern_return_t mk_timer_arm(HANDLE name, LARGE_INTEGER expire_time) {
    BOOL res = SetWaitableTimer(name, &expire_time, 0, nil, nil, NO);
    if (!res) {
        DWORD err = GetLastError();
        RSLog(RSLogLevelError, RSSTR("RSRunLoop: Unable to set timer: %d"), err);
    }
    return (int)res;
}

static kern_return_t mk_timer_cancel(HANDLE name, LARGE_INTEGER *result_time) {
    BOOL res = CancelWaitableTimer(name);
    if (!res) {
        DWORD err = GetLastError();
        RSLog(RSLogLevelError, RSSTR("RSRunLoop: Unable to cancel timer: %d"), err);
    }
    return (int)res;
}

// The name of this function is a lie on Windows. The return value matches the argument of the fake mk_timer functions above. Note that the Windows timers expect to be given "system time". We have to do some calculations to get the right value, which is a FILETIME-like value.
RSInline LARGE_INTEGER __RSUInt64ToAbsoluteTime(int64_t desiredFireTime) {
    LARGE_INTEGER result;
    // There is a race we know about here, (timer fire time calculated -> thread suspended -> timer armed == late timer fire), but we don't have a way to avoid it at this time, since the only way to specify an absolute value to the timer is to calculate the relative time first. Fixing that would probably require not using the TSR for timers on Windows.
    int64_t timeDiff = desiredFireTime - (int64_t)mach_absolute_time();
    if (timeDiff < 0) {
        result.QuadPart = 0;
    } else {
        RSTimeInterval amountOfTimeToWait = __RSTSRToTimeInterval(timeDiff);
        // Result is in 100 ns (10**-7 sec) units to be consistent with a FILETIME.
        // RSTimeInterval is in seconds.
        result.QuadPart = -(amountOfTimeToWait * 10000000);
    }
    return result;
}

#endif

/* unlock a run loop and modes before doing callouts/sleeping */
/* never try to take the run loop lock with a mode locked */
/* be very careful of common subexpression elimination and compacting code, particular across locks and unlocks! */
/* run loop mode structures should never be deallocated, even if they become empty */

static RSTypeID __RSRunLoopModeTypeID = _RSRuntimeNotATypeID;
static RSTypeID __RSRunLoopTypeID = _RSRuntimeNotATypeID;
static RSTypeID __RSRunLoopSourceTypeID = _RSRuntimeNotATypeID;
static RSTypeID __RSRunLoopObserverTypeID = _RSRuntimeNotATypeID;
static RSTypeID __RSRunLoopTimerTypeID = _RSRuntimeNotATypeID;

typedef struct __RSRunLoopMode *RSRunLoopModeRef;

struct __RSRunLoopMode {
    RSRuntimeBase _base;
    pthread_mutex_t _lock;	/* must have the run loop locked before locking this */
    RSStringRef _name;
    BOOL _stopped;
    char _padding[3];
    RSMutableSetRef _sources0;
    RSMutableSetRef _sources1;
    RSMutableArrayRef _observers;
    RSMutableArrayRef _timers;
    RSMutableDictionaryRef _portToV1SourceMap;
    __RSPortSet _portSet;
    RSIndex _observerMask;
#if DEPLOYMENT_TARGET_MACOSX || DEPLOYMENT_TARGET_EMBEDDED || DEPLOYMENT_TARGET_EMBEDDED_MINI
    mach_port_t _timerPort;
#endif
#if DEPLOYMENT_TARGET_WINDOWS
    HANDLE _timerPort;
    DWORD _msgQMask;
    void (*_msgPump)(void);
#endif
};

RSInline void __RSRunLoopModeLock(RSRunLoopModeRef rlm) {
    pthread_mutex_lock(&(rlm->_lock));
    //    RSLog(6, RSSTR("__RSRunLoopModeLock locked %p"), rlm);
}

RSInline void __RSRunLoopModeUnlock(RSRunLoopModeRef rlm) {
    //    RSLog(6, RSSTR("__RSRunLoopModeLock unlocking %p"), rlm);
    pthread_mutex_unlock(&(rlm->_lock));
}

static BOOL __RSRunLoopModeClassEqual(RSTypeRef rs1, RSTypeRef rs2) {
    RSRunLoopModeRef rlm1 = (RSRunLoopModeRef)rs1;
    RSRunLoopModeRef rlm2 = (RSRunLoopModeRef)rs2;
    return RSEqual(rlm1->_name, rlm2->_name);
}

static RSHashCode __RSRunLoopModeClassHash(RSTypeRef rs) {
    RSRunLoopModeRef rlm = (RSRunLoopModeRef)rs;
    return RSHash(rlm->_name);
}

static RSStringRef __RSRunLoopModeClassCopyDescription(RSTypeRef rs) {
    RSRunLoopModeRef rlm = (RSRunLoopModeRef)rs;
    RSMutableStringRef result;
    result = RSStringCreateMutable(RSAllocatorSystemDefault, 0);
    RSStringAppendStringWithFormat(result, RSSTR("<RSRunLoopMode %p [%p]>{name = %r, "), rlm, RSGetAllocator(rlm), rlm->_name);
    RSStringAppendStringWithFormat(result, RSSTR("port set = %p, "), rlm->_portSet);
    RSStringAppendStringWithFormat(result, RSSTR("timer port = %p, "), rlm->_timerPort);
#if DEPLOYMENT_TARGET_WINDOWS
    RSStringAppendStringWithFormat(result, RSSTR("MSGQ mask = %p, "), rlm->_msgQMask);
#endif
    RSStringAppendStringWithFormat(result, RSSTR("\n\tsources0 = %r,\n\tsources1 = %r,\n\tobservers = %r,\n\ttimers = %r\n},\n"), rlm->_sources0, rlm->_sources1, rlm->_observers, rlm->_timers);
    return result;
}

static void __RSRunLoopModeClassDeallocate(RSTypeRef rs) {
    RSRunLoopModeRef rlm = (RSRunLoopModeRef)rs;
    if (nil != rlm->_sources0) RSRelease(rlm->_sources0);
    if (nil != rlm->_sources1) RSRelease(rlm->_sources1);
    if (nil != rlm->_observers) RSRelease(rlm->_observers);
    if (nil != rlm->_timers) RSRelease(rlm->_timers);
    if (nil != rlm->_portToV1SourceMap) RSRelease(rlm->_portToV1SourceMap);
    RSRelease(rlm->_name);
    __RSPortSetFree(rlm->_portSet);
    if (MACH_PORT_NULL != rlm->_timerPort) mk_timer_destroy(rlm->_timerPort);
    pthread_mutex_destroy(&rlm->_lock);
    memset((char *)rs + sizeof(RSRuntimeBase), 0x7C, sizeof(struct __RSRunLoopMode) - sizeof(RSRuntimeBase));
}

struct _block_item {
    struct _block_item *_next;
    RSTypeRef _mode;	// RSString or RSSet
    void (^_block)(void);
};

typedef struct _per_run_data {
    uint32_t a;
    uint32_t b;
    uint32_t stopped;
    uint32_t ignoreWakeUps;
} _per_run_data;

struct __RSRunLoop {
    RSRuntimeBase _base;
    pthread_mutex_t _lock;			/* locked for accessing mode list */
    __RSPort _wakeUpPort;			// used for RSRunLoopWakeUp
    BOOL _unused;
    volatile _per_run_data *_perRunData;              // reset for runs of the run loop
    pthread_t _pthread;
    uint32_t _winthread;
    RSMutableSetRef _commonModes;
    RSMutableSetRef _commonModeItems;
    RSRunLoopModeRef _currentMode;
    RSMutableSetRef _modes;
    struct _block_item *_blocks_head;
    struct _block_item *_blocks_tail;
    RSTypeRef _counterpart;
};

/* Bit 0 of the base reserved bits is used for stopped state */
/* Bit 1 of the base reserved bits is used for sleeping state */
/* Bit 2 of the base reserved bits is used for deallocating state */

RSInline volatile _per_run_data *__RSRunLoopPushPerRunData(RSRunLoopRef rl) {
    volatile _per_run_data *previous = rl->_perRunData;
    rl->_perRunData = (volatile _per_run_data *)RSAllocatorAllocate(RSAllocatorSystemDefault, sizeof(_per_run_data));
    rl->_perRunData->a = 0x8283524C;
    rl->_perRunData->b = 0x8283524C; // 'RSRL'
    rl->_perRunData->stopped = 0x00000000;
    rl->_perRunData->ignoreWakeUps = 0x00000000;
    return previous;
}

RSInline void __RSRunLoopPopPerRunData(RSRunLoopRef rl, volatile _per_run_data *previous) {
    if (rl->_perRunData) RSAllocatorDeallocate(RSAllocatorSystemDefault, (void *)rl->_perRunData);
    rl->_perRunData = previous;
}

RSInline BOOL __RSRunLoopIsStopped(RSRunLoopRef rl) {
    return (rl->_perRunData->stopped) ? YES : NO;
}

RSInline void __RSRunLoopSetStopped(RSRunLoopRef rl) {
    rl->_perRunData->stopped = 0x53544F50;	// 'STOP'
}

RSInline void __RSRunLoopUnsetStopped(RSRunLoopRef rl) {
    rl->_perRunData->stopped = 0x0;
}

RSInline BOOL __RSRunLoopIsIgnoringWakeUps(RSRunLoopRef rl) {
    return (rl->_perRunData->ignoreWakeUps) ? YES : NO;
}

RSInline void __RSRunLoopSetIgnoreWakeUps(RSRunLoopRef rl) {
    rl->_perRunData->ignoreWakeUps = 0x57414B45; // 'WAKE'
}

RSInline void __RSRunLoopUnsetIgnoreWakeUps(RSRunLoopRef rl) {
    rl->_perRunData->ignoreWakeUps = 0x0;
}

RSInline BOOL __RSRunLoopIsSleeping(RSRunLoopRef rl) {
    return (BOOL)__RSBitfieldGetValue(RSRuntimeClassBaseFiled(rl), 1, 1);
}

RSInline void __RSRunLoopSetSleeping(RSRunLoopRef rl) {
    __RSBitfieldSetValue(RSRuntimeClassBaseFiled(rl), 1, 1, 1);
}

RSInline void __RSRunLoopUnsetSleeping(RSRunLoopRef rl) {
    __RSBitfieldSetValue(RSRuntimeClassBaseFiled(rl), 1, 1, 0);
}

RSInline BOOL __RSRunLoopIsDeallocating(RSRunLoopRef rl) {
    return (BOOL)__RSBitfieldGetValue(RSRuntimeClassBaseFiled(rl), 2, 2);
}

RSInline void __RSRunLoopSetDeallocating(RSRunLoopRef rl) {
    __RSBitfieldSetValue(RSRuntimeClassBaseFiled(rl), 2, 2, 1);
}

RSInline void __RSRunLoopLock(RSRunLoopRef rl) {
    pthread_mutex_lock(&(((RSRunLoopRef)rl)->_lock));
    //    RSLog(6, RSSTR("__RSRunLoopLock locked %p"), rl);
}

RSInline void __RSRunLoopUnlock(RSRunLoopRef rl) {
    //    RSLog(6, RSSTR("__RSRunLoopLock unlocking %p"), rl);
    pthread_mutex_unlock(&(((RSRunLoopRef)rl)->_lock));
}

static RSStringRef __RSRunLoopClassDescription(RSTypeRef rs) {
    RSRunLoopRef rl = (RSRunLoopRef)rs;
    RSMutableStringRef result;
    result = RSStringCreateMutable(RSAllocatorSystemDefault, 0);
#if DEPLOYMENT_TARGET_WINDOWS
    RSStringAppendFormat(result, nil, RSSTR("<RSRunLoop %p [%p]>{wakeup port = 0x%x, stopped = %s, ignoreWakeUps = %s, \ncurrent mode = %r,\n"), rs, RSGetAllocator(rs), rl->_wakeUpPort, __RSRunLoopIsStopped(rl) ? "YES" : "NO", __RSRunLoopIsIgnoringWakeUps(rl) ? "YES" : "NO", rl->_currentMode ? rl->_currentMode->_name : RSSTR("(none)"));
#else
    RSStringAppendStringWithFormat(result, nil, RSSTR("<RSRunLoop %p [%p]>{wakeup port = 0x%x, stopped = %s, ignoreWakeUps = %s, \ncurrent mode = %r,\n"), rs, RSGetAllocator(rs), rl->_wakeUpPort, __RSRunLoopIsStopped(rl) ? "YES" : "NO", __RSRunLoopIsIgnoringWakeUps(rl) ? "YES" : "NO", rl->_currentMode ? rl->_currentMode->_name : RSSTR("(none)"));
#endif
    RSStringAppendStringWithFormat(result, nil, RSSTR("common modes = %r,\ncommon mode items = %r,\nmodes = %r}\n"), rl->_commonModes, rl->_commonModeItems, rl->_modes);
    return result;
}

RSExport void __RSRunLoopDump() { // RSExport to keep the compiler from discarding it
    RSShow(RSDescription(RSRunLoopGetCurrent()));
}

RSInline void __RSRunLoopLockInit(pthread_mutex_t *lock) {
    pthread_mutexattr_t mattr;
    pthread_mutexattr_init(&mattr);
    pthread_mutexattr_settype(&mattr, PTHREAD_MUTEX_RECURSIVE);
    int32_t mret = pthread_mutex_init(lock, &mattr);
    pthread_mutexattr_destroy(&mattr);
    if (0 != mret) {
    }
}

/* call with rl locked */
static RSRunLoopModeRef __RSRunLoopFindMode(RSRunLoopRef rl, RSStringRef modeName, BOOL create) {
    CHECK_FOR_FORK();
    RSRunLoopModeRef rlm;
    struct __RSRunLoopMode srlm;
    memset(&srlm, 0, sizeof(srlm));
//    srlm._base._rsisa = __RSISAForTypeID(__RSRunLoopModeTypeID);
    __RSRuntimeSetInstanceTypeID(&srlm, __RSRunLoopModeTypeID);
    __RSRuntimeSetInstanceAsStackValue(&srlm);
    srlm._name = modeName;
    rlm = (RSRunLoopModeRef)RSSetGetValue(rl->_modes, &srlm);
    if (nil != rlm) {
        __RSRunLoopModeLock(rlm);
        return rlm;
    }
    if (!create) {
        return nil;
    }
    rlm = (RSRunLoopModeRef)__RSRuntimeCreateInstance(RSAllocatorSystemDefault, __RSRunLoopModeTypeID, sizeof(struct __RSRunLoopMode) - sizeof(RSRuntimeBase));
    if (nil == rlm) {
        return nil;
    }
    __RSRunLoopLockInit(&rlm->_lock);
    rlm->_name = RSStringCreateCopy(RSAllocatorSystemDefault, modeName);
    rlm->_stopped = NO;
    rlm->_portToV1SourceMap = nil;
    rlm->_sources0 = nil;
    rlm->_sources1 = nil;
    rlm->_observers = nil;
    rlm->_timers = nil;
    rlm->_observerMask = 0;
    rlm->_portSet = __RSPortSetAllocate();
    rlm->_timerPort = mk_timer_create();
    kern_return_t ret = __RSPortSetInsert(rlm->_timerPort, rlm->_portSet);
    if (KERN_SUCCESS != ret) {
        char msg[256];
        snprintf(msg, 256, "*** Unable to insert timer port into port set. (%d) ***", ret);
        //CRSetCrashLogMessage(msg);
        HALT;
    }
    ret = __RSPortSetInsert(rl->_wakeUpPort, rlm->_portSet);
    if (KERN_SUCCESS != ret) {
        char msg[256];
        snprintf(msg, 256, "*** Unable to insert wake up port into port set. (%d) ***", ret);
        ////CRSetCrashLogMessage(msg);
        HALT;
    }
#if DEPLOYMENT_TARGET_WINDOWS
    rlm->_msgQMask = 0;
    rlm->_msgPump = nil;
#endif
    RSSetAddValue(rl->_modes, rlm);
    RSRelease(rlm);
    __RSRunLoopModeLock(rlm);	/* return mode locked */
    return rlm;
}


// expects rl and rlm locked
static BOOL __RSRunLoopModeIsEmpty(RSRunLoopRef rl, RSRunLoopModeRef rlm, RSRunLoopModeRef previousMode) {
    CHECK_FOR_FORK();
    if (nil == rlm) return YES;
#if DEPLOYMENT_TARGET_WINDOWS
    if (0 != rlm->_msgQMask) return NO;
#endif
    BOOL libdispatchQSafe = pthread_main_np() && ((HANDLE_DISPATCH_ON_BASE_INVOCATION_ONLY && nil == previousMode) || (!HANDLE_DISPATCH_ON_BASE_INVOCATION_ONLY && 0 == _RSGetTSD(__RSTSDKeyIsInGCDMainQ)));
    if (libdispatchQSafe && (RSRunLoopGetMain() == rl) && RSSetContainsValue(rl->_commonModes, rlm->_name)) return NO; // represents the libdispatch main queue
    if (nil != rlm->_sources0 && 0 < RSSetGetCount(rlm->_sources0)) return NO;
    if (nil != rlm->_sources1 && 0 < RSSetGetCount(rlm->_sources1)) return NO;
    if (nil != rlm->_timers && 0 < RSArrayGetCount(rlm->_timers)) return NO;
    struct _block_item *item = rl->_blocks_head;
    while (item) {
        struct _block_item *curr = item;
        item = item->_next;
        BOOL doit = NO;
        if (RSStringGetTypeID() == RSGetTypeID(curr->_mode)) {
            doit = RSEqual(curr->_mode, rlm->_name) || (RSEqual(curr->_mode, RSRunLoopCommonModes) && RSSetContainsValue(rl->_commonModes, rlm->_name));
        } else {
            doit = RSSetContainsValue((RSSetRef)curr->_mode, rlm->_name) || (RSSetContainsValue((RSSetRef)curr->_mode, RSRunLoopCommonModes) && RSSetContainsValue(rl->_commonModes, rlm->_name));
        }
        if (doit) return NO;
    }
    return YES;
}

#if DEPLOYMENT_TARGET_WINDOWS

uint32_t _RSRunLoopGetWindowsMessageQueueMask(RSRunLoopRef rl, RSStringRef modeName) {
    if (modeName == RSRunLoopCommonModes) {
        RSLog(RSLogLevelError, RSSTR("_RSRunLoopGetWindowsMessageQueueMask: RSRunLoopCommonModes unsupported"));
        HALT;
    }
    DWORD result = 0;
    __RSRunLoopLock(rl);
    RSRunLoopModeRef rlm = __RSRunLoopFindMode(rl, modeName, NO);
    if (rlm) {
        result = rlm->_msgQMask;
        __RSRunLoopModeUnlock(rlm);
    }
    __RSRunLoopUnlock(rl);
    return (uint32_t)result;
}

void _RSRunLoopSetWindowsMessageQueueMask(RSRunLoopRef rl, uint32_t mask, RSStringRef modeName) {
    if (modeName == RSRunLoopCommonModes) {
        RSLog(RSLogLevelError, RSSTR("_RSRunLoopSetWindowsMessageQueueMask: RSRunLoopCommonModes unsupported"));
        HALT;
    }
    __RSRunLoopLock(rl);
    RSRunLoopModeRef rlm = __RSRunLoopFindMode(rl, modeName, YES);
    rlm->_msgQMask = (DWORD)mask;
    __RSRunLoopModeUnlock(rlm);
    __RSRunLoopUnlock(rl);
}

uint32_t _RSRunLoopGetWindowsThreadID(RSRunLoopRef rl) {
    return rl->_winthread;
}

RSWindowsMessageQueueHandler _RSRunLoopGetWindowsMessageQueueHandler(RSRunLoopRef rl, RSStringRef modeName) {
    if (modeName == RSRunLoopCommonModes) {
        RSLog(RSLogLevelError, RSSTR("_RSRunLoopGetWindowsMessageQueueMask: RSRunLoopCommonModes unsupported"));
        HALT;
    }
    if (rl != RSRunLoopGetCurrent()) {
        RSLog(RSLogLevelError, RSSTR("_RSRunLoopGetWindowsMessageQueueHandler: run loop parameter must be the current run loop"));
        HALT;
    }
    void (*result)(void) = nil;
    __RSRunLoopLock(rl);
    RSRunLoopModeRef rlm = __RSRunLoopFindMode(rl, modeName, NO);
    if (rlm) {
        result = rlm->_msgPump;
        __RSRunLoopModeUnlock(rlm);
    }
    __RSRunLoopUnlock(rl);
    return result;
}

void _RSRunLoopSetWindowsMessageQueueHandler(RSRunLoopRef rl, RSStringRef modeName, RSWindowsMessageQueueHandler func) {
    if (modeName == RSRunLoopCommonModes) {
        RSLog(RSLogLevelError, RSSTR("_RSRunLoopGetWindowsMessageQueueMask: RSRunLoopCommonModes unsupported"));
        HALT;
    }
    if (rl != RSRunLoopGetCurrent()) {
        RSLog(RSLogLevelError, RSSTR("_RSRunLoopGetWindowsMessageQueueHandler: run loop parameter must be the current run loop"));
        HALT;
    }
    __RSRunLoopLock(rl);
    RSRunLoopModeRef rlm = __RSRunLoopFindMode(rl, modeName, YES);
    rlm->_msgPump = func;
    __RSRunLoopModeUnlock(rlm);
    __RSRunLoopUnlock(rl);
}

#endif

/* Bit 3 in the base reserved bits is used for invalid state in run loop objects */

RSInline BOOL __RSRunLoopIsValid(const void *rs) {
    return (BOOL)__RSBitfieldGetValue(RSRuntimeClassBaseFiled(rs), 3, 3);
}

RSInline void __RSRunLoopSetValid(void *rs) {
    __RSBitfieldSetValue(RSRuntimeClassBaseFiled(rs), 3, 3, 1);
}

RSInline void __RSRunLoopUnsetValid(void *rs) {
    __RSBitfieldSetValue(RSRuntimeClassBaseFiled(rs), 3, 3, 0);
}

typedef struct {
    RSIndex	version;
    void *	info;
    const void *(*retain)(const void *info);
    void	(*release)(const void *info);
    RSStringRef	(*copyDescription)(const void *info);
    BOOL	(*equal)(const void *info1, const void *info2);
    RSHashCode	(*hash)(const void *info);
#if (TARGET_OS_MAC && !(TARGET_OS_EMBEDDED || TARGET_OS_IPHONE)) || (TARGET_OS_EMBEDDED || TARGET_OS_IPHONE)
    mach_port_t	(*getPort)(void *info);
    void *	(*perform)(void *msg, RSIndex size, RSAllocatorRef allocator, void *info);
#else
    void *	(*getPort)(void *info);
    void	(*perform)(void *info);
#endif
} RSRunLoopSourceContext1;

struct __RSRunLoopSource {
    RSRuntimeBase _base;
    uint32_t _bits;
    pthread_mutex_t _lock;
    RSIndex _order;			/* immutable */
    RSMutableBagRef _runLoops;
    union {
        RSRunLoopSourceContext version0;	/* immutable, except invalidation */
        RSRunLoopSourceContext1 version1;	/* immutable, except invalidation */
    } _context;
};

/* Bit 1 of the base reserved bits is used for signalled state */

RSInline BOOL __RSRunLoopSourceIsSignaled(RSRunLoopSourceRef rls) {
    return (BOOL)__RSBitfieldGetValue(rls->_bits, 1, 1);
}

RSInline void __RSRunLoopSourceSetSignaled(RSRunLoopSourceRef rls) {
    __RSBitfieldSetValue(rls->_bits, 1, 1, 1);
}

RSInline void __RSRunLoopSourceUnsetSignaled(RSRunLoopSourceRef rls) {
    __RSBitfieldSetValue(rls->_bits, 1, 1, 0);
}

RSInline void __RSRunLoopSourceLock(RSRunLoopSourceRef rls) {
    pthread_mutex_lock(&(rls->_lock));
    //    RSLog(6, RSSTR("__RSRunLoopSourceLock locked %p"), rls);
}

RSInline void __RSRunLoopSourceUnlock(RSRunLoopSourceRef rls) {
    //    RSLog(6, RSSTR("__RSRunLoopSourceLock unlocking %p"), rls);
    pthread_mutex_unlock(&(rls->_lock));
}

typedef struct {
    RSIndex	version;
    void *	info;
    const void *(*retain)(const void *info);
    void	(*release)(const void *info);
    RSStringRef	(*copyDescription)(const void *info);
} RSRunLoopObserverContext;

typedef struct __RSRunLoopObserver* RSRunLoopObserverRef;
typedef void (*RSRunLoopObserverCallBack)(RSRunLoopObserverRef observer, RSRunLoopActivity activity, void *info);

struct __RSRunLoopObserver {
    RSRuntimeBase _base;
    pthread_mutex_t _lock;
    RSRunLoopRef _runLoop;
    RSIndex _rlCount;
    RSOptionFlags _activities;		/* immutable */
    RSIndex _order;			/* immutable */
    RSRunLoopObserverCallBack _callout;	/* immutable */
    RSRunLoopObserverContext _context;	/* immutable, except invalidation */
};

/* Bit 0 of the base reserved bits is used for firing state */
/* Bit 1 of the base reserved bits is used for repeats state */

RSInline BOOL __RSRunLoopObserverIsFiring(RSRunLoopObserverRef rlo) {
    return (BOOL)__RSBitfieldGetValue(RSRuntimeClassBaseFiled(rlo), 0, 0);
}

RSInline void __RSRunLoopObserverSetFiring(RSRunLoopObserverRef rlo) {
    __RSBitfieldSetValue(RSRuntimeClassBaseFiled(rlo), 0, 0, 1);
}

RSInline void __RSRunLoopObserverUnsetFiring(RSRunLoopObserverRef rlo) {
    __RSBitfieldSetValue(RSRuntimeClassBaseFiled(rlo), 0, 0, 0);
}

RSInline BOOL __RSRunLoopObserverRepeats(RSRunLoopObserverRef rlo) {
    return (BOOL)__RSBitfieldGetValue(RSRuntimeClassBaseFiled(rlo), 1, 1);
}

RSInline void __RSRunLoopObserverSetRepeats(RSRunLoopObserverRef rlo) {
    __RSBitfieldSetValue(RSRuntimeClassBaseFiled(rlo), 1, 1, 1);
}

RSInline void __RSRunLoopObserverUnsetRepeats(RSRunLoopObserverRef rlo) {
    __RSBitfieldSetValue(RSRuntimeClassBaseFiled(rlo), 1, 1, 0);
}

RSInline void __RSRunLoopObserverLock(RSRunLoopObserverRef rlo) {
    pthread_mutex_lock(&(rlo->_lock));
    //    RSLog(6, RSSTR("__RSRunLoopObserverLock locked %p"), rlo);
}

RSInline void __RSRunLoopObserverUnlock(RSRunLoopObserverRef rlo) {
    //    RSLog(6, RSSTR("__RSRunLoopObserverLock unlocking %p"), rlo);
    pthread_mutex_unlock(&(rlo->_lock));
}

static void __RSRunLoopObserverSchedule(RSRunLoopObserverRef rlo, RSRunLoopRef rl, RSRunLoopModeRef rlm) {
    __RSRunLoopObserverLock(rlo);
    if (0 == rlo->_rlCount) {
        rlo->_runLoop = rl;
    }
    rlo->_rlCount++;
    __RSRunLoopObserverUnlock(rlo);
}

static void __RSRunLoopObserverCancel(RSRunLoopObserverRef rlo, RSRunLoopRef rl, RSRunLoopModeRef rlm) {
    __RSRunLoopObserverLock(rlo);
    rlo->_rlCount--;
    if (0 == rlo->_rlCount) {
        rlo->_runLoop = nil;
    }
    __RSRunLoopObserverUnlock(rlo);
}

typedef struct __RSRunLoopTimer* RSRunLoopTimerRef;
typedef struct {
    RSIndex	version;
    void *	info;
    const void *(*retain)(const void *info);
    void	(*release)(const void *info);
    RSStringRef	(*copyDescription)(const void *info);
} RSRunLoopTimerContext;

typedef void (*RSRunLoopTimerCallBack)(RSRunLoopTimerRef timer, void *info);

struct __RSRunLoopTimer {
    RSRuntimeBase _base;
    uint16_t _bits;
    pthread_mutex_t _lock;
    RSRunLoopRef _runLoop;
    RSMutableSetRef _rlModes;
    RSAbsoluteTime _nextFireDate;
    RSTimeInterval _interval;		/* immutable */
    int64_t _fireTSR;			/* TSR units */
    RSIndex _order;			/* immutable */
    RSRunLoopTimerCallBack _callout;	/* immutable */
    RSRunLoopTimerContext _context;	/* immutable, except invalidation */
};

/* Bit 0 of the base reserved bits is used for firing state */
/* Bit 1 of the base reserved bits is used for fired-during-callout state */

RSInline BOOL __RSRunLoopTimerIsFiring(RSRunLoopTimerRef rlt) {
    return (BOOL)__RSBitfieldGetValue(rlt->_bits, 0, 0);
}

RSInline void __RSRunLoopTimerSetFiring(RSRunLoopTimerRef rlt) {
    __RSBitfieldSetValue(rlt->_bits, 0, 0, 1);
}

RSInline void __RSRunLoopTimerUnsetFiring(RSRunLoopTimerRef rlt) {
    __RSBitfieldSetValue(rlt->_bits, 0, 0, 0);
}

RSInline BOOL __RSRunLoopTimerIsDeallocating(RSRunLoopTimerRef rlt) {
    return (BOOL)__RSBitfieldGetValue(rlt->_bits, 2, 2);
}

RSInline void __RSRunLoopTimerSetDeallocating(RSRunLoopTimerRef rlt) {
    __RSBitfieldSetValue(rlt->_bits, 2, 2, 1);
}

RSInline void __RSRunLoopTimerLock(RSRunLoopTimerRef rlt) {
    pthread_mutex_lock(&(rlt->_lock));
    //    RSLog(6, RSSTR("__RSRunLoopTimerLock locked %p"), rlt);
}

RSInline void __RSRunLoopTimerUnlock(RSRunLoopTimerRef rlt) {
    //    RSLog(6, RSSTR("__RSRunLoopTimerLock unlocking %p"), rlt);
    pthread_mutex_unlock(&(rlt->_lock));
}

static RSSpinLock __RSRLTFireTSRLock = RSSpinLockInit;

RSInline void __RSRunLoopTimerFireTSRLock(void) {
    RSSpinLockLock(&__RSRLTFireTSRLock);
}

RSInline void __RSRunLoopTimerFireTSRUnlock(void) {
    RSSpinLockUnlock(&__RSRLTFireTSRLock);
}

#if DEPLOYMENT_TARGET_WINDOWS

struct _collectTimersContext {
    RSMutableArrayRef results;
    int64_t cutoffTSR;
};

static void __RSRunLoopCollectTimers(const void *value, void *ctx) {
    RSRunLoopTimerRef rlt = (RSRunLoopTimerRef)value;
    struct _collectTimersContext *context = (struct _collectTimersContext *)ctx;
    if (rlt->_fireTSR <= context->cutoffTSR) {
        if (nil == context->results)
            context->results = RSArrayCreateMutable(RSAllocatorSystemDefault, 0, &RSTypeArrayCallBacks);
        RSArrayAppendValue(context->results, rlt);
    }
}

// RunLoop and RunLoopMode must be locked
static void __RSRunLoopTimersToFireRecursive(RSRunLoopRef rl, RSRunLoopModeRef rlm, struct _collectTimersContext *ctxt) {
    __RSRunLoopTimerFireTSRLock();
    if (nil != rlm->_timers && 0 < RSArrayGetCount(rlm->_timers)) {
        RSArrayApplyFunction(rlm->_timers, RSMakeRange(0, RSArrayGetCount(rlm->_timers)),  __RSRunLoopCollectTimers, ctxt);
    }
    __RSRunLoopTimerFireTSRUnlock();
}

// RunLoop and RunLoopMode must be locked
static RSArrayRef __RSRunLoopTimersToFire(RSRunLoopRef rl, RSRunLoopModeRef rlm) {
    struct _collectTimersContext ctxt = {nil, mach_absolute_time()};
    __RSRunLoopTimersToFireRecursive(rl, rlm, &ctxt);
    return ctxt.results;
}

#endif

// call with rl and rlm locked
static RSRunLoopSourceRef __RSRunLoopModeFindSourceForMachPort(RSRunLoopRef rl, RSRunLoopModeRef rlm, __RSPort port) {	/* DOES CALLOUT */
    CHECK_FOR_FORK();
    RSRunLoopSourceRef found = rlm->_portToV1SourceMap ? (RSRunLoopSourceRef)RSDictionaryGetValue(rlm->_portToV1SourceMap, (const void *)(uintptr_t)port) : nil;
    return found;
}

// Remove backreferences the mode's sources have to the rl (context);
// the primary purpose of rls->_runLoops is so that Invalidation can remove
// the source from the run loops it is in, but during deallocation of a
// run loop, we already know that the sources are going to be punted
// from it, so invalidation of sources does not need to remove from a
// deallocating run loop.
static void __RSRunLoopCleanseSources(const void *value, void *context) {
    RSRunLoopModeRef rlm = (RSRunLoopModeRef)value;
    RSRunLoopRef rl = (RSRunLoopRef)context;
    RSIndex idx, cnt;
    const void **list, *buffer[256];
    if (nil == rlm->_sources0 && nil == rlm->_sources1) return;
    
    cnt = (rlm->_sources0 ? RSSetGetCount(rlm->_sources0) : 0) + (rlm->_sources1 ? RSSetGetCount(rlm->_sources1) : 0);
    list = (const void **)((cnt <= 256) ? buffer : RSAllocatorAllocate(RSAllocatorSystemDefault, cnt * sizeof(void *)));
    if (rlm->_sources0) RSSetGetValues(rlm->_sources0, list);
    if (rlm->_sources1) RSSetGetValues(rlm->_sources1, list + (rlm->_sources0 ? RSSetGetCount(rlm->_sources0) : 0));
    for (idx = 0; idx < cnt; idx++) {
        RSRunLoopSourceRef rls = (RSRunLoopSourceRef)list[idx];
        __RSRunLoopSourceLock(rls);
        if (nil != rls->_runLoops) {
            RSBagRemoveValue(rls->_runLoops, rl);
        }
        __RSRunLoopSourceUnlock(rls);
    }
    if (list != buffer) RSAllocatorDeallocate(RSAllocatorSystemDefault, list);
}

static void __RSRunLoopDeallocateSources(const void *value, void *context) {
    RSRunLoopModeRef rlm = (RSRunLoopModeRef)value;
    RSRunLoopRef rl = (RSRunLoopRef)context;
    RSIndex idx, cnt;
    const void **list, *buffer[256];
    if (nil == rlm->_sources0 && nil == rlm->_sources1) return;
    
    cnt = (rlm->_sources0 ? RSSetGetCount(rlm->_sources0) : 0) + (rlm->_sources1 ? RSSetGetCount(rlm->_sources1) : 0);
    list = (const void **)((cnt <= 256) ? buffer : RSAllocatorAllocate(RSAllocatorSystemDefault, cnt * sizeof(void *)));
    if (rlm->_sources0) RSSetGetValues(rlm->_sources0, list);
    if (rlm->_sources1) RSSetGetValues(rlm->_sources1, list + (rlm->_sources0 ? RSSetGetCount(rlm->_sources0) : 0));
    for (idx = 0; idx < cnt; idx++) {
        RSRetain(list[idx]);
    }
    if (rlm->_sources0) RSSetRemoveAllValues(rlm->_sources0);
    if (rlm->_sources1) RSSetRemoveAllValues(rlm->_sources1);
    for (idx = 0; idx < cnt; idx++) {
        RSRunLoopSourceRef rls = (RSRunLoopSourceRef)list[idx];
        __RSRunLoopSourceLock(rls);
        if (nil != rls->_runLoops) {
            RSBagRemoveValue(rls->_runLoops, rl);
        }
        __RSRunLoopSourceUnlock(rls);
        if (0 == rls->_context.version0.version) {
            if (nil != rls->_context.version0.cancel) {
                rls->_context.version0.cancel(rls->_context.version0.info, rl, rlm->_name);	/* CALLOUT */
            }
        } else if (1 == rls->_context.version0.version) {
            __RSPort port = rls->_context.version1.getPort(rls->_context.version1.info);	/* CALLOUT */
            if (RSPORT_nil != port) {
                __RSPortSetRemove(port, rlm->_portSet);
            }
        }
        RSRelease(rls);
    }
    if (list != buffer) RSAllocatorDeallocate(RSAllocatorSystemDefault, list);
}

static void __RSRunLoopDeallocateObservers(const void *value, void *context) {
    RSRunLoopModeRef rlm = (RSRunLoopModeRef)value;
    RSRunLoopRef rl = (RSRunLoopRef)context;
    RSIndex idx, cnt;
    const void **list, *buffer[256];
    if (nil == rlm->_observers) return;
    cnt = RSArrayGetCount(rlm->_observers);
    list = (const void **)((cnt <= 256) ? buffer : RSAllocatorAllocate(RSAllocatorSystemDefault, cnt * sizeof(void *)));
    RSArrayGetObjects(rlm->_observers, RSMakeRange(0, cnt), list);
    for (idx = 0; idx < cnt; idx++) {
        RSRetain(list[idx]);
    }
    RSArrayRemoveAllObjects(rlm->_observers);
    for (idx = 0; idx < cnt; idx++) {
        __RSRunLoopObserverCancel((RSRunLoopObserverRef)list[idx], rl, rlm);
        RSRelease(list[idx]);
    }
    if (list != buffer) RSAllocatorDeallocate(RSAllocatorSystemDefault, list);
}

static void __RSRunLoopDeallocateTimers(const void *value, void *context) {
    RSRunLoopModeRef rlm = (RSRunLoopModeRef)value;
    RSIndex idx, cnt;
    const void **list, *buffer[256];
    if (nil == rlm->_timers) return;
    cnt = RSArrayGetCount(rlm->_timers);
    list = (const void **)((cnt <= 256) ? buffer : RSAllocatorAllocate(RSAllocatorSystemDefault, cnt * sizeof(void *)));
    RSArrayGetObjects(rlm->_timers, RSMakeRange(0, RSArrayGetCount(rlm->_timers)), list);
    for (idx = 0; idx < cnt; idx++) {
        RSRetain(list[idx]);
    }
    RSArrayRemoveAllObjects(rlm->_timers);
    for (idx = 0; idx < cnt; idx++) {
        RSRunLoopTimerRef rlt = (RSRunLoopTimerRef)list[idx];
        __RSRunLoopTimerLock(rlt);
        // if the run loop is deallocating, and since a timer can only be in one
        // run loop, we're going to be removing the timer from all modes, so be
        // a little heavy-handed and direct
        RSSetRemoveAllValues(rlt->_rlModes);
        rlt->_runLoop = nil;
        __RSRunLoopTimerUnlock(rlt);
        RSRelease(list[idx]);
    }
    if (list != buffer) RSAllocatorDeallocate(RSAllocatorSystemDefault, list);
}

RSExport pthread_t _RSMainPThread;
RSExport RSRunLoopRef _RSRunLoopGet0b(pthread_t t);

static void __RSRunLoopClassDeallocate(RSTypeRef rs) {
    RSRunLoopRef rl = (RSRunLoopRef)rs;
    
    if (_RSRunLoopGet0b(_RSMainPThread) == rs) HALT;
    
    /* We try to keep the run loop in a valid state as long as possible,
     since sources may have non-retained references to the run loop.
     Another reason is that we don't want to lock the run loop for
     callback reasons, if we can get away without that.  We start by
     eliminating the sources, since they are the most likely to call
     back into the run loop during their "cancellation". Common mode
     items will be removed from the mode indirectly by the following
     three lines. */
    __RSRunLoopSetDeallocating(rl);
    if (nil != rl->_modes) {
        RSSetApplyFunction(rl->_modes, (__RSRunLoopCleanseSources), rl); // remove references to rl
        RSSetApplyFunction(rl->_modes, (__RSRunLoopDeallocateSources), rl);
        RSSetApplyFunction(rl->_modes, (__RSRunLoopDeallocateObservers), rl);
        RSSetApplyFunction(rl->_modes, (__RSRunLoopDeallocateTimers), rl);
    }
    __RSRunLoopLock(rl);
    struct _block_item *item = rl->_blocks_head;
    while (item) {
        struct _block_item *curr = item;
        item = item->_next;
        RSRelease(curr->_mode);
        Block_release(curr->_block);
        free(curr);
    }
    if (nil != rl->_commonModeItems) {
        RSRelease(rl->_commonModeItems);
    }
    if (nil != rl->_commonModes) {
        RSRelease(rl->_commonModes);
    }
    if (nil != rl->_modes) {
        RSRelease(rl->_modes);
    }
    __RSPortFree(rl->_wakeUpPort);
    rl->_wakeUpPort = RSPORT_nil;
    __RSRunLoopPopPerRunData(rl, nil);
    __RSRunLoopUnlock(rl);
    pthread_mutex_destroy(&rl->_lock);
    memset((char *)rs + sizeof(RSRuntimeBase), 0x8C, sizeof(struct __RSRunLoop) - sizeof(RSRuntimeBase));
}

static const RSRuntimeClass __RSRunLoopModeClass = {
    _RSRuntimeScannedObject,
    "RSRunLoopMode",
    nil,      // init
    nil,      // copy
    __RSRunLoopModeClassDeallocate,
    __RSRunLoopModeClassEqual,
    __RSRunLoopModeClassHash,
    __RSRunLoopModeClassCopyDescription,
    nil,
    nil
};

static const RSRuntimeClass __RSRunLoopClass = {
    _RSRuntimeScannedObject,
    "RSRunLoop",
    nil,      // init
    nil,      // copy
    __RSRunLoopClassDeallocate,
    nil,
    nil,
    __RSRunLoopClassDescription,
    nil,
    nil
};

RSExport void __RSFinalizeRunLoop(uintptr_t data);

static int64_t tenus = 0LL;

RSPrivate void __RSRunLoopInitialize(void) {
    tenus = __RSTimeIntervalToTSR(0.000010000);
    __RSRunLoopTypeID = __RSRuntimeRegisterClass(&__RSRunLoopClass);
    __RSRuntimeSetClassTypeID(&__RSRunLoopClass, __RSRunLoopTypeID);
    __RSRunLoopModeTypeID = __RSRuntimeRegisterClass(&__RSRunLoopModeClass);
    __RSRuntimeSetClassTypeID(&__RSRunLoopModeClass, __RSRunLoopModeTypeID);
}

RSTypeID RSRunLoopGetTypeID(void) {
    return __RSRunLoopTypeID;
}

static RSRunLoopRef __RSRunLoopCreate(pthread_t t) {
    RSRunLoopRef loop = nil;
    RSRunLoopModeRef rlm;
    uint32_t size = sizeof(struct __RSRunLoop) - sizeof(RSRuntimeBase);
    loop = (RSRunLoopRef)__RSRuntimeCreateInstance(RSAllocatorSystemDefault, __RSRunLoopTypeID, size);
    if (nil == loop) {
        return nil;
    }
    (void)__RSRunLoopPushPerRunData(loop);
    __RSRunLoopLockInit(&loop->_lock);
    loop->_wakeUpPort = __RSPortAllocate();
    if (RSPORT_nil == loop->_wakeUpPort) HALT;
    __RSRunLoopSetIgnoreWakeUps(loop);
    loop->_commonModes = RSSetCreateMutable(RSAllocatorSystemDefault, 0, &RSTypeSetCallBacks);
    RSSetAddValue(loop->_commonModes, RSRunLoopDefaultMode);
    loop->_commonModeItems = nil;
    loop->_currentMode = nil;
    loop->_modes = RSSetCreateMutable(RSAllocatorSystemDefault, 0, &RSTypeSetCallBacks);
    loop->_blocks_head = nil;
    loop->_blocks_tail = nil;
    loop->_counterpart = nil;
    loop->_pthread = t;
#if DEPLOYMENT_TARGET_WINDOWS
    loop->_winthread = GetCurrentThreadId();
#else
    loop->_winthread = 0;
#endif
    rlm = __RSRunLoopFindMode(loop, RSRunLoopDefaultMode, YES);
    if (nil != rlm) __RSRunLoopModeUnlock(rlm);
    return loop;
}

static RSMutableDictionaryRef __RSRunLoops = nil;
static RSSpinLock loopsLock = RSSpinLockInit;

// should only be called by Foundation
// t==0 is a synonym for "main thread" that always works
RSExport RSRunLoopRef _RSRunLoopGet0(pthread_t t) {
    if (pthread_equal(t, NilPthreadT))
    {
        t = _RSMainPThread;
    }
    RSSpinLockLock(&loopsLock);
    if (!__RSRunLoops)
    {
        RSSpinLockUnlock(&loopsLock);
        RSDictionaryContext context = {0, nil, RSDictionaryRSTypeContext->valueContext};
        RSMutableDictionaryRef dict = RSDictionaryCreateMutable(RSAllocatorSystemDefault, 0, &context);
        RSRunLoopRef mainLoop = __RSRunLoopCreate(_RSMainPThread);
        RSDictionarySetValue(dict, pthreadPointer(_RSMainPThread), mainLoop);
        if (!OSAtomicCompareAndSwapPtrBarrier(nil, dict, (void * volatile *)&__RSRunLoops)) {
            RSRelease(dict);
        }
        RSRelease(mainLoop);
        RSSpinLockLock(&loopsLock);
    }
    RSRunLoopRef loop = (RSRunLoopRef)RSDictionaryGetValue(__RSRunLoops, pthreadPointer(t));
    RSSpinLockUnlock(&loopsLock);
    if (!loop)
    {
        RSRunLoopRef newLoop = __RSRunLoopCreate(t);
        RSSpinLockLock(&loopsLock);
        loop = (RSRunLoopRef)RSDictionaryGetValue(__RSRunLoops, pthreadPointer(t));
        if (!loop)
        {
            RSDictionarySetValue(__RSRunLoops, pthreadPointer(t), newLoop);
            loop = newLoop;
        }
        // don't release run loops inside the loopsLock, because RSRunLoopDeallocate may end up taking it
        RSSpinLockUnlock(&loopsLock);
        RSRelease(newLoop);
    }
    if (pthread_equal(t, pthread_self()))
    {
        _RSSetTSD(__RSTSDKeyRunLoop, (void *)loop, nil);
        if (0 == _RSGetTSD(__RSTSDKeyRunLoopCntr))
        {
            _RSSetTSD(__RSTSDKeyRunLoopCntr, (void *)(PTHREAD_DESTRUCTOR_ITERATIONS-1), (void (*)(void *))__RSFinalizeRunLoop);
        }
    }
    return loop;
}

// should only be called by Foundation
RSRunLoopRef _RSRunLoopGet0b(pthread_t t) {
    if (pthread_equal(t, NilPthreadT)) {
        t = _RSMainPThread;
    }
    RSSpinLockLock(&loopsLock);
    RSRunLoopRef loop = nil;
    if (__RSRunLoops) {
        loop = (RSRunLoopRef)RSDictionaryGetValue(__RSRunLoops, pthreadPointer(t));
    }
    RSSpinLockUnlock(&loopsLock);
    return loop;
}

static void __RSRunLoopRemoveAllSources(RSRunLoopRef rl, RSStringRef modeName);

RSArrayRef RSRunLoopCopyAllModes(RSRunLoopRef rl);
// Called for each thread as it exits
RSExport void __RSFinalizeRunLoop(uintptr_t data) {
    RSRunLoopRef rl = nil;
    if (data <= 1) {
        RSSpinLockLock(&loopsLock);
        if (__RSRunLoops) {
            rl = (RSRunLoopRef)RSDictionaryGetValue(__RSRunLoops, pthreadPointer(pthread_self()));
            if (rl) RSRetain(rl);
            RSDictionaryRemoveValue(__RSRunLoops, pthreadPointer(pthread_self()));
        }
        RSSpinLockUnlock(&loopsLock);
    } else {
        _RSSetTSD(__RSTSDKeyRunLoopCntr, (void *)(data - 1), (void (*)(void *))__RSFinalizeRunLoop);
    }
    if (rl && RSRunLoopGetMain() != rl) { // protect against cooperative threads
        if (nil != rl->_counterpart) {
            RSRelease(rl->_counterpart);
            rl->_counterpart = nil;
        }
        // purge all sources before deallocation
        RSArrayRef array = RSRunLoopCopyAllModes(rl);
        for (RSIndex idx = RSArrayGetCount(array); idx--;) {
            RSStringRef modeName = (RSStringRef)RSArrayObjectAtIndex(array, idx);
            __RSRunLoopRemoveAllSources(rl, modeName);
        }
        __RSRunLoopRemoveAllSources(rl, RSRunLoopCommonModes);
        RSRelease(array);
    }
    if (rl) RSRelease(rl);
}

pthread_t _RSRunLoopGet1(RSRunLoopRef rl) {
    return rl->_pthread;
}

// should only be called by Foundation
RSExport RSTypeRef _RSRunLoopGet2(RSRunLoopRef rl) {
    RSTypeRef ret = nil;
    RSSpinLockLock(&loopsLock);
    ret = rl->_counterpart;
    RSSpinLockUnlock(&loopsLock);
    return ret;
}

// should only be called by Foundation
RSExport RSTypeRef _RSRunLoopGet2b(RSRunLoopRef rl) {
    return rl->_counterpart;
}

#if DEPLOYMENT_TARGET_MACOSX
void _RSRunLoopSetCurrent(RSRunLoopRef rl) {
    if (pthread_main_np()) return;
    RSRunLoopRef currentLoop = RSRunLoopGetCurrent();
    if (rl != currentLoop) {
        RSRetain(currentLoop); // avoid a deallocation of the currentLoop inside the lock
        RSSpinLockLock(&loopsLock);
        if (rl) {
            RSDictionarySetValue(__RSRunLoops, pthreadPointer(pthread_self()), rl);
        } else {
            RSDictionaryRemoveValue(__RSRunLoops, pthreadPointer(pthread_self()));
        }
        RSSpinLockUnlock(&loopsLock);
        RSRelease(currentLoop);
        _RSSetTSD(__RSTSDKeyRunLoop, nil, nil);
        _RSSetTSD(__RSTSDKeyRunLoopCntr, 0, (void (*)(void *))__RSFinalizeRunLoop);
    }
}
#endif

RSRunLoopRef RSRunLoopGetMain(void) {
    CHECK_FOR_FORK();
    static RSRunLoopRef __main = nil; // no retain needed
    if (!__main) __main = _RSRunLoopGet0(_RSMainPThread); // no CAS needed
    return __main;
}

RSExport RSRunLoopRef RSRunLoopGetCurrent(void) {
    CHECK_FOR_FORK();
    RSRunLoopRef rl = (RSRunLoopRef)_RSGetTSD(__RSTSDKeyRunLoop);
    if (rl) return rl;
    return _RSRunLoopGet0(pthread_self());
}

RSStringRef RSRunLoopCopyCurrentMode(RSRunLoopRef rl) {
    CHECK_FOR_FORK();
    RSStringRef result = nil;
    __RSRunLoopLock(rl);
    if (nil != rl->_currentMode) {
        result = (RSStringRef)RSRetain(rl->_currentMode->_name);
    }
    __RSRunLoopUnlock(rl);
    return result;
}

static void __RSRunLoopGetModeName(const void *value, void *context) {
    RSRunLoopModeRef rlm = (RSRunLoopModeRef)value;
    RSMutableArrayRef array = (RSMutableArrayRef)context;
    RSArrayAddObject(array, rlm->_name);
}

RSArrayRef RSRunLoopCopyAllModes(RSRunLoopRef rl) {
    CHECK_FOR_FORK();
    RSMutableArrayRef array;
    __RSRunLoopLock(rl);
    array = RSArrayCreateMutable(RSAllocatorSystemDefault, RSSetGetCount(rl->_modes));
    RSSetApplyFunction(rl->_modes, (__RSRunLoopGetModeName), array);
    __RSRunLoopUnlock(rl);
    return array;
}

void RSRunLoopAddObserver(RSRunLoopRef rl, RSRunLoopObserverRef rlo, RSStringRef modeName);
void RSRunLoopRemoveObserver(RSRunLoopRef rl, RSRunLoopObserverRef rlo, RSStringRef modeName);

void RSRunLoopAddTimer(RSRunLoopRef rl, RSRunLoopTimerRef rlt, RSStringRef modeName);
void RSRunLoopRemoveTimer(RSRunLoopRef rl, RSRunLoopTimerRef rlt, RSStringRef modeName);

static void __RSRunLoopAddItemsToCommonMode(const void *value, void *ctx) {
    RSTypeRef item = (RSTypeRef)value;
    RSRunLoopRef rl = (RSRunLoopRef)(((RSTypeRef *)ctx)[0]);
    RSStringRef modeName = (RSStringRef)(((RSTypeRef *)ctx)[1]);
    if (RSGetTypeID(item) == __RSRunLoopSourceTypeID) {
        RSRunLoopAddSource(rl, (RSRunLoopSourceRef)item, modeName);
    } else if (RSGetTypeID(item) == __RSRunLoopObserverTypeID) {
        RSRunLoopAddObserver(rl, (RSRunLoopObserverRef)item, modeName);
    } else if (RSGetTypeID(item) == __RSRunLoopTimerTypeID) {
        RSRunLoopAddTimer(rl, (RSRunLoopTimerRef)item, modeName);
    }
}

static void __RSRunLoopAddItemToCommonModes(const void *value, void *ctx) {
    RSStringRef modeName = (RSStringRef)value;
    RSRunLoopRef rl = (RSRunLoopRef)(((RSTypeRef *)ctx)[0]);
    RSTypeRef item = (RSTypeRef)(((RSTypeRef *)ctx)[1]);
    if (RSGetTypeID(item) == __RSRunLoopSourceTypeID) {
        RSRunLoopAddSource(rl, (RSRunLoopSourceRef)item, modeName);
    } else if (RSGetTypeID(item) == __RSRunLoopObserverTypeID) {
        RSRunLoopAddObserver(rl, (RSRunLoopObserverRef)item, modeName);
    } else if (RSGetTypeID(item) == __RSRunLoopTimerTypeID) {
        RSRunLoopAddTimer(rl, (RSRunLoopTimerRef)item, modeName);
    }
}

static void __RSRunLoopRemoveItemFromCommonModes(const void *value, void *ctx) {
    RSStringRef modeName = (RSStringRef)value;
    RSRunLoopRef rl = (RSRunLoopRef)(((RSTypeRef *)ctx)[0]);
    RSTypeRef item = (RSTypeRef)(((RSTypeRef *)ctx)[1]);
    if (RSGetTypeID(item) == __RSRunLoopSourceTypeID) {
        RSRunLoopRemoveSource(rl, (RSRunLoopSourceRef)item, modeName);
    } else if (RSGetTypeID(item) == __RSRunLoopObserverTypeID) {
        RSRunLoopRemoveObserver(rl, (RSRunLoopObserverRef)item, modeName);
    } else if (RSGetTypeID(item) == __RSRunLoopTimerTypeID) {
        RSRunLoopRemoveTimer(rl, (RSRunLoopTimerRef)item, modeName);
    }
}

RSExport BOOL _RSRunLoop01(RSRunLoopRef rl, RSStringRef modeName) {
    __RSRunLoopLock(rl);
    BOOL present = RSSetContainsValue(rl->_commonModes, modeName);
    __RSRunLoopUnlock(rl);
    return present;
}

void RSRunLoopAddCommonMode(RSRunLoopRef rl, RSStringRef modeName) {
    CHECK_FOR_FORK();
    if (__RSRunLoopIsDeallocating(rl)) return;
    __RSRunLoopLock(rl);
    if (!RSSetContainsValue(rl->_commonModes, modeName)) {
        RSSetRef set = rl->_commonModeItems ? RSSetCreateCopy(RSAllocatorSystemDefault, rl->_commonModeItems) : nil;
        RSSetAddValue(rl->_commonModes, modeName);
        if (nil != set) {
            RSTypeRef context[2] = {rl, modeName};
            /* add all common-modes items to new mode */
            RSSetApplyFunction(set, (__RSRunLoopAddItemsToCommonMode), (void *)context);
            RSRelease(set);
        }
    } else {
    }
    __RSRunLoopUnlock(rl);
}


static void __RSRUNLOOP_IS_CALLING_OUT_TO_AN_OBSERVER_CALLBACK_FUNCTION__() __attribute__((noinline));
static void __RSRUNLOOP_IS_CALLING_OUT_TO_AN_OBSERVER_CALLBACK_FUNCTION__(RSRunLoopObserverCallBack func, RSRunLoopObserverRef observer, RSRunLoopActivity activity, void *info) {
    if (func) {
        func(observer, activity, info);
    }
    getpid(); // thwart tail-call optimization
}

static void __RSRUNLOOP_IS_CALLING_OUT_TO_A_TIMER_CALLBACK_FUNCTION__() __attribute__((noinline));
static void __RSRUNLOOP_IS_CALLING_OUT_TO_A_TIMER_CALLBACK_FUNCTION__(RSRunLoopTimerCallBack func, RSRunLoopTimerRef timer, void *info) {
    if (func) {
        func(timer, info);
    }
    getpid(); // thwart tail-call optimization
}

static void __RSRUNLOOP_IS_CALLING_OUT_TO_A_BLOCK__() __attribute__((noinline));
static void __RSRUNLOOP_IS_CALLING_OUT_TO_A_BLOCK__(void (^block)(void)) {
    if (block) {
        block();
    }
    getpid(); // thwart tail-call optimization
}

static BOOL __RSRunLoopDoBlocks(RSRunLoopRef rl, RSRunLoopModeRef rlm) { // Call with rl and rlm locked
    if (!rl->_blocks_head) return NO;
    if (!rlm || !rlm->_name) return NO;
    BOOL did = NO;
    struct _block_item *head = rl->_blocks_head;
    struct _block_item *tail = rl->_blocks_tail;
    rl->_blocks_head = nil;
    rl->_blocks_tail = nil;
    RSSetRef commonModes = rl->_commonModes;
    RSStringRef curMode = rlm->_name;
    __RSRunLoopModeUnlock(rlm);
    __RSRunLoopUnlock(rl);
    struct _block_item *prev = nil;
    struct _block_item *item = head;
    while (item) {
        struct _block_item *curr = item;
        item = item->_next;
        BOOL doit = NO;
        if (RSStringGetTypeID() == RSGetTypeID(curr->_mode)) {
            doit = RSEqual(curr->_mode, curMode) || (RSEqual(curr->_mode, RSRunLoopCommonModes) && RSSetContainsValue(commonModes, curMode));
        } else {
            doit = RSSetContainsValue((RSSetRef)curr->_mode, curMode) || (RSSetContainsValue((RSSetRef)curr->_mode, RSRunLoopCommonModes) && RSSetContainsValue(commonModes, curMode));
        }
        if (!doit) prev = curr;
        if (doit) {
            if (prev) prev->_next = item;
            if (curr == head) head = item;
            if (curr == tail) tail = prev;
            void (^block)(void) = curr->_block;
            RSRelease(curr->_mode);
            free(curr);
            if (doit) {
                __RSRUNLOOP_IS_CALLING_OUT_TO_A_BLOCK__(block);
                did = YES;
            }
            Block_release(block); // do this before relocking to prevent deadlocks where some yahoo wants to run the run loop reentrantly from their dealloc
        }
    }
    __RSRunLoopLock(rl);
    __RSRunLoopModeLock(rlm);
    if (head) {
        tail->_next = rl->_blocks_head;
        rl->_blocks_head = head;
        if (!rl->_blocks_tail) rl->_blocks_tail = tail;
    }
    return did;
}

/* rl is locked, rlm is locked on entrance and exit */
static void __RSRunLoopDoObservers() __attribute__((noinline));

void RSRunLoopObserverInvalidate(RSRunLoopObserverRef rlo);

static void __RSRunLoopDoObservers(RSRunLoopRef rl, RSRunLoopModeRef rlm, RSRunLoopActivity activity) {	/* DOES CALLOUT */
    CHECK_FOR_FORK();
    
    RSIndex cnt = rlm->_observers ? RSArrayGetCount(rlm->_observers) : 0;
    if (cnt < 1) return;
    
    /* Fire the observers */
    STACK_BUFFER_DECL(RSRunLoopObserverRef, buffer, (cnt <= 1024) ? cnt : 1);
    RSRunLoopObserverRef *collectedObservers = (cnt <= 1024) ? buffer : (RSRunLoopObserverRef *)RSAllocatorAllocate(RSAllocatorSystemDefault, cnt * sizeof(RSRunLoopObserverRef));
    RSIndex obs_cnt = 0;
    for (RSIndex idx = 0; idx < cnt; idx++) {
        RSRunLoopObserverRef rlo = (RSRunLoopObserverRef)RSArrayObjectAtIndex(rlm->_observers, idx);
        if (0 != (rlo->_activities & activity) && __RSRunLoopIsValid(rlo) && !__RSRunLoopObserverIsFiring(rlo)) {
            collectedObservers[obs_cnt++] = (RSRunLoopObserverRef)RSRetain(rlo);
        }
    }
    __RSRunLoopModeUnlock(rlm);
    __RSRunLoopUnlock(rl);
    for (RSIndex idx = 0; idx < obs_cnt; idx++) {
        RSRunLoopObserverRef rlo = collectedObservers[idx];
        __RSRunLoopObserverLock(rlo);
        if (__RSRunLoopIsValid(rlo)) {
            BOOL doInvalidate = !__RSRunLoopObserverRepeats(rlo);
            __RSRunLoopObserverSetFiring(rlo);
            __RSRunLoopObserverUnlock(rlo);
            __RSRUNLOOP_IS_CALLING_OUT_TO_AN_OBSERVER_CALLBACK_FUNCTION__(rlo->_callout, rlo, activity, rlo->_context.info);
            if (doInvalidate) {
                RSRunLoopObserverInvalidate(rlo);
            }
            __RSRunLoopObserverUnsetFiring(rlo);
        } else {
            __RSRunLoopObserverUnlock(rlo);
        }
        RSRelease(rlo);
    }
    __RSRunLoopLock(rl);
    __RSRunLoopModeLock(rlm);
    
    if (collectedObservers != buffer) free(collectedObservers);
}

static RSComparisonResult __RSRunLoopSourceComparator(const void *val1, const void *val2, void *context) {
    RSRunLoopSourceRef o1 = (RSRunLoopSourceRef)val1;
    RSRunLoopSourceRef o2 = (RSRunLoopSourceRef)val2;
    if (o1->_order < o2->_order) return RSCompareLessThan;
    if (o2->_order < o1->_order) return RSCompareGreaterThan;
    return RSCompareEqualTo;
}

static void __RSRunLoopCollectSources0(const void *value, void *context) {
    RSRunLoopSourceRef rls = (RSRunLoopSourceRef)value;
    RSTypeRef *sources = (RSTypeRef *)context;
    if (0 == rls->_context.version0.version && __RSRunLoopIsValid(rls) && __RSRunLoopSourceIsSignaled(rls)) {
        if (nil == *sources) {
            *sources = RSRetain(rls);
        } else if (RSGetTypeID(*sources) == __RSRunLoopSourceTypeID) {
            RSTypeRef oldrls = *sources;
            *sources = RSArrayCreateMutable(RSAllocatorSystemDefault, 0);
            RSArrayAddObject((RSMutableArrayRef)*sources, oldrls);
            RSArrayAddObject((RSMutableArrayRef)*sources, rls);
            RSRelease(oldrls);
        } else {
            RSArrayAddObject((RSMutableArrayRef)*sources, rls);
        }
    }
}

static void __RSRUNLOOP_IS_CALLING_OUT_TO_A_SOURCE0_PERFORM_FUNCTION__() __attribute__((noinline));
static void __RSRUNLOOP_IS_CALLING_OUT_TO_A_SOURCE0_PERFORM_FUNCTION__(void (*perform)(void *), void *info) {
    if (perform) {
        perform(info);
    }
    getpid(); // thwart tail-call optimization
}

static void __RSRUNLOOP_IS_CALLING_OUT_TO_A_SOURCE1_PERFORM_FUNCTION__() __attribute__((noinline));
static void __RSRUNLOOP_IS_CALLING_OUT_TO_A_SOURCE1_PERFORM_FUNCTION__(
#if DEPLOYMENT_TARGET_MACOSX || DEPLOYMENT_TARGET_EMBEDDED || DEPLOYMENT_TARGET_EMBEDDED_MINI
                                                                       void *(*perform)(void *msg, RSIndex size, RSAllocatorRef allocator, void *info),
                                                                       mach_msg_header_t *msg, RSIndex size, mach_msg_header_t **reply,
#else
                                                                       void (*perform)(void *),
#endif
                                                                       void *info) {
    if (perform) {
#if DEPLOYMENT_TARGET_MACOSX || DEPLOYMENT_TARGET_EMBEDDED || DEPLOYMENT_TARGET_EMBEDDED_MINI
        *reply = perform(msg, size, RSAllocatorSystemDefault, info);
#else
        perform(info);
#endif
    }
    getpid(); // thwart tail-call optimization
}

/* rl is locked, rlm is locked on entrance and exit */
static BOOL __RSRunLoopDoSources0(RSRunLoopRef rl, RSRunLoopModeRef rlm, BOOL stopAfterHandle) __attribute__((noinline));
static BOOL __RSRunLoopDoSources0(RSRunLoopRef rl, RSRunLoopModeRef rlm, BOOL stopAfterHandle)
{	/* DOES CALLOUT */
    CHECK_FOR_FORK();
    RSTypeRef sources = nil;
    BOOL sourceHandled = NO;
    
    /* Fire the version 0 sources */
    if (nil != rlm->_sources0 && 0 < RSSetGetCount(rlm->_sources0)) {
        RSSetApplyFunction(rlm->_sources0, (__RSRunLoopCollectSources0), &sources);
    }
    if (nil != sources)
    {
        __RSRunLoopModeUnlock(rlm);
        __RSRunLoopUnlock(rl);
        // sources is either a single (retained) RSRunLoopSourceRef or an array of (retained) RSRunLoopSourceRef
        if (RSGetTypeID(sources) == __RSRunLoopSourceTypeID)
        {
            RSRunLoopSourceRef rls = (RSRunLoopSourceRef)sources;
            __RSRunLoopSourceLock(rls);
            if (__RSRunLoopSourceIsSignaled(rls))
            {
                __RSRunLoopSourceUnsetSignaled(rls);
                if (__RSRunLoopIsValid(rls))
                {
                    __RSRunLoopSourceUnlock(rls);
                    __RSRUNLOOP_IS_CALLING_OUT_TO_A_SOURCE0_PERFORM_FUNCTION__(rls->_context.version0.perform, rls->_context.version0.info);
                    CHECK_FOR_FORK();
                    sourceHandled = YES;
                }
                else {
                    __RSRunLoopSourceUnlock(rls);
                }
            }
            else {
                __RSRunLoopSourceUnlock(rls);
            }
        }
        else
        {
            RSIndex cnt = RSArrayGetCount((RSArrayRef)sources);
            RSArraySort((RSMutableArrayRef)sources, RSOrderedAscending, (__RSRunLoopSourceComparator), nil);
            for (RSIndex idx = 0; idx < cnt; idx++)
            {
                RSRunLoopSourceRef rls = (RSRunLoopSourceRef)RSArrayObjectAtIndex((RSArrayRef)sources, idx);
                __RSRunLoopSourceLock(rls);
                if (__RSRunLoopSourceIsSignaled(rls))
                {
                    __RSRunLoopSourceUnsetSignaled(rls);
                    if (__RSRunLoopIsValid(rls))
                    {
                        __RSRunLoopSourceUnlock(rls);
                        __RSRUNLOOP_IS_CALLING_OUT_TO_A_SOURCE0_PERFORM_FUNCTION__(rls->_context.version0.perform, rls->_context.version0.info);
                        CHECK_FOR_FORK();
                        sourceHandled = YES;
                    } else
                    {
                        __RSRunLoopSourceUnlock(rls);
                    }
                } else
                {
                    __RSRunLoopSourceUnlock(rls);
                }
                if (stopAfterHandle && sourceHandled)
                {
                    break;
                }
            }
        }
        RSRelease(sources);
        __RSRunLoopLock(rl);
        __RSRunLoopModeLock(rlm);
    }
    return sourceHandled;
}

// msg, size and reply are unused on Windows

static BOOL __RSRunLoopDoSource1() __attribute__((noinline));
static BOOL __RSRunLoopDoSource1(RSRunLoopRef rl, RSRunLoopModeRef rlm, RSRunLoopSourceRef rls
#if DEPLOYMENT_TARGET_MACOSX || DEPLOYMENT_TARGET_EMBEDDED || DEPLOYMENT_TARGET_EMBEDDED_MINI
                                    , mach_msg_header_t *msg, RSIndex size, mach_msg_header_t **reply
#endif
) {	/* DOES CALLOUT */
    CHECK_FOR_FORK();
    BOOL sourceHandled = NO;
    
    /* Fire a version 1 source */
    RSRetain(rls);
    __RSRunLoopModeUnlock(rlm);
    __RSRunLoopUnlock(rl);
    __RSRunLoopSourceLock(rls);
    if (__RSRunLoopIsValid(rls))
    {
        __RSRunLoopSourceUnsetSignaled(rls);
        __RSRunLoopSourceUnlock(rls);
        __RSRUNLOOP_IS_CALLING_OUT_TO_A_SOURCE1_PERFORM_FUNCTION__(rls->_context.version1.perform,
#if DEPLOYMENT_TARGET_MACOSX || DEPLOYMENT_TARGET_EMBEDDED || DEPLOYMENT_TARGET_EMBEDDED_MINI
                                                                   msg, size, reply,
#endif
                                                                   rls->_context.version1.info);
        CHECK_FOR_FORK();
        sourceHandled = YES;
    } else {
        if (_LogRSRunLoop) { __RSLog(RSLogLevelDebug, RSSTR("%p (%s) __RSRunLoopDoSource1 rls %p is invalid"), RSRunLoopGetCurrent(), *_RSGetProgname(), rls); }
        __RSRunLoopSourceUnlock(rls);
    }
    RSRelease(rls);
    __RSRunLoopLock(rl);
    __RSRunLoopModeLock(rlm);
    return sourceHandled;
}

static RSIndex __RSRunLoopInsertionIndexInTimerArray(RSArrayRef array, RSRunLoopTimerRef rlt) __attribute__((noinline));
static RSIndex __RSRunLoopInsertionIndexInTimerArray(RSArrayRef array, RSRunLoopTimerRef rlt) {
    RSIndex cnt = RSArrayGetCount(array);
    if (cnt <= 0) {
        return 0;
    }
    if (256 < cnt) {
        RSRunLoopTimerRef item = (RSRunLoopTimerRef)RSArrayObjectAtIndex(array, cnt - 1);
        if (item->_fireTSR <= rlt->_fireTSR) {
            return cnt;
        }
        item = (RSRunLoopTimerRef)RSArrayObjectAtIndex(array, 0);
        if (rlt->_fireTSR < item->_fireTSR) {
            return 0;
        }
    }
    
    RSIndex add = (1 << flsl(cnt)) * 2;
    RSIndex idx = 0;
    BOOL lastTestLEQ;
    do {
        add = add / 2;
        lastTestLEQ = NO;
        RSIndex testIdx = idx + add;
        if (testIdx < cnt) {
            RSRunLoopTimerRef item = (RSRunLoopTimerRef)RSArrayObjectAtIndex(array, testIdx);
            if (item->_fireTSR <= rlt->_fireTSR) {
                idx = testIdx;
                lastTestLEQ = YES;
            }
        }
    } while (0 < add);
    
    return lastTestLEQ ? idx + 1 : idx;
}

static void __RSArmNextTimerInMode(RSRunLoopModeRef rlm) {
    RSRunLoopTimerRef nextTimer = nil;
    for (RSIndex idx = 0, cnt = RSArrayGetCount(rlm->_timers); idx < cnt; idx++) {
        RSRunLoopTimerRef t = (RSRunLoopTimerRef)RSArrayObjectAtIndex(rlm->_timers, idx);
        if (!__RSRunLoopTimerIsFiring(t)) {
            nextTimer = t;
            break;
        }
    }
    if (nextTimer) {
        int64_t fireTSR = nextTimer->_fireTSR;
        fireTSR = (fireTSR / tenus + 1) * tenus;
        mk_timer_arm(rlm->_timerPort, __RSUInt64ToAbsoluteTime(fireTSR));
    }
}

// call with rlm and its run loop locked, and the TSRLock locked; rlt not locked; returns with same state
static void __RSRepositionTimerInMode(RSRunLoopModeRef rlm, RSRunLoopTimerRef rlt, BOOL isInArray) __attribute__((noinline));
static void __RSRepositionTimerInMode(RSRunLoopModeRef rlm, RSRunLoopTimerRef rlt, BOOL isInArray) {
    if (!rlt || !rlm->_timers) return;
    BOOL found = NO;
    if (isInArray) {
        
        RSIndex idx = RSArrayIndexOfObject(rlm->_timers, rlt);
        if (RSNotFound != idx) {
            RSRetain(rlt);
            RSArrayRemoveObjectAtIndex(rlm->_timers, idx);
            found = YES;
        }
    }
    if (!found && isInArray) return;
    RSIndex newIdx = __RSRunLoopInsertionIndexInTimerArray(rlm->_timers, rlt);
    RSArrayInsertObjectAtIndex(rlm->_timers, newIdx, rlt);
    __RSArmNextTimerInMode(rlm);
    if (isInArray) RSRelease(rlt);
}

void RSRunLoopTimerInvalidate(RSRunLoopTimerRef rlt);
// mode and rl are locked on entry and exit
static BOOL __RSRunLoopDoTimer(RSRunLoopRef rl, RSRunLoopModeRef rlm, RSRunLoopTimerRef rlt) {	/* DOES CALLOUT */
    BOOL timerHandled = NO;
    int64_t oldFireTSR = 0;
    
    /* Fire a timer */
    RSRetain(rlt);
    __RSRunLoopTimerLock(rlt);
    
    if (__RSRunLoopIsValid(rlt) && rlt->_fireTSR <= (int64_t)mach_absolute_time() && !__RSRunLoopTimerIsFiring(rlt) && rlt->_runLoop == rl) {
        void *context_info = nil;
        void (*context_release)(const void *) = nil;
        if (rlt->_context.retain) {
            context_info = (void *)rlt->_context.retain(rlt->_context.info);
            context_release = rlt->_context.release;
        } else {
            context_info = rlt->_context.info;
        }
        BOOL doInvalidate = (0.0 == rlt->_interval);
        __RSRunLoopTimerSetFiring(rlt);
        __RSRunLoopTimerUnlock(rlt);
        __RSRunLoopTimerFireTSRLock();
        oldFireTSR = rlt->_fireTSR;
        __RSRunLoopTimerFireTSRUnlock();
        
        __RSArmNextTimerInMode(rlm);
        
        __RSRunLoopModeUnlock(rlm);
        __RSRunLoopUnlock(rl);
        __RSRUNLOOP_IS_CALLING_OUT_TO_A_TIMER_CALLBACK_FUNCTION__(rlt->_callout, rlt, context_info);
        CHECK_FOR_FORK();
        if (doInvalidate) {
            RSRunLoopTimerInvalidate(rlt);      /* DOES CALLOUT */
        }
        if (context_release) {
            context_release(context_info);
        }
        __RSRunLoopLock(rl);
        __RSRunLoopModeLock(rlm);
        __RSRunLoopTimerLock(rlt);
        timerHandled = YES;
        __RSRunLoopTimerUnsetFiring(rlt);
    }
    if (__RSRunLoopIsValid(rlt) && timerHandled) {
        /* This is just a little bit tricky: we want to support calling
         * RSRunLoopTimerSetNextFireDate() from within the callout and
         * honor that new time here if it is a later date, otherwise
         * it is completely ignored. */
        if (oldFireTSR < rlt->_fireTSR) {
            /* Next fire TSR was set, and set to a date after the previous
             * fire date, so we honor it. */
            __RSRunLoopTimerUnlock(rlt);
            // The timer was adjusted and repositioned, during the
            // callout, but if it was still the min timer, it was
            // skipped because it was firing.  Need to redo the
            // min timer calculation in case rlt should now be that
            // timer instead of whatever was chosen.
            __RSArmNextTimerInMode(rlm);
        } else {
            int64_t nextFireTSR = 0LL;
            int64_t intervalTSR = 0LL;
            if (rlt->_interval <= 0.0) {
            } else if (TIMER_INTERVAL_LIMIT < rlt->_interval) {
                intervalTSR = __RSTimeIntervalToTSR(TIMER_INTERVAL_LIMIT);
            } else {
                intervalTSR = __RSTimeIntervalToTSR(rlt->_interval);
            }
            if (LLONG_MAX - intervalTSR <= oldFireTSR) {
                nextFireTSR = LLONG_MAX;
            } else {
                int64_t currentTSR = (int64_t)mach_absolute_time();
                nextFireTSR = oldFireTSR;
                while (nextFireTSR <= currentTSR) {
                    nextFireTSR += intervalTSR;
                }
            }
            RSRunLoopRef rlt_rl = rlt->_runLoop;
            if (rlt_rl) {
                RSRetain(rlt_rl);
                RSIndex cnt = RSSetGetCount(rlt->_rlModes);
                STACK_BUFFER_DECL(RSTypeRef, modes, cnt);
                RSSetGetValues(rlt->_rlModes, (const void **)modes);
                // To avoid A->B, B->A lock ordering issues when coming up
                // towards the run loop from a source, the timer has to be
                // unlocked, which means we have to protect from object
                // invalidation, although that's somewhat expensive.
                for (RSIndex idx = 0; idx < cnt; idx++) {
                    RSRetain(modes[idx]);
                }
                __RSRunLoopTimerUnlock(rlt);
                for (RSIndex idx = 0; idx < cnt; idx++) {
                    RSStringRef name = (RSStringRef)modes[idx];
                    modes[idx] = (RSTypeRef)__RSRunLoopFindMode(rlt_rl, name, NO);
                    RSRelease(name);
                }
                __RSRunLoopTimerFireTSRLock();
                rlt->_fireTSR = nextFireTSR;
                rlt->_nextFireDate = RSAbsoluteTimeGetCurrent() + __RSTSRToTimeInterval(nextFireTSR - (int64_t)mach_absolute_time());
                for (RSIndex idx = 0; idx < cnt; idx++) {
                    RSRunLoopModeRef rlm = (RSRunLoopModeRef)modes[idx];
                    if (rlm) {
                        __RSRepositionTimerInMode(rlm, rlt, YES);
                    }
                }
                __RSRunLoopTimerFireTSRUnlock();
                for (RSIndex idx = 0; idx < cnt; idx++) {
                    __RSRunLoopModeUnlock((RSRunLoopModeRef)modes[idx]);
                }
                RSRelease(rlt_rl);
            } else {
                __RSRunLoopTimerUnlock(rlt);
                __RSRunLoopTimerFireTSRLock();
                rlt->_fireTSR = nextFireTSR;
                rlt->_nextFireDate = RSAbsoluteTimeGetCurrent() + __RSTSRToTimeInterval(nextFireTSR - (int64_t)mach_absolute_time());
                __RSRunLoopTimerFireTSRUnlock();
            }
        }
    } else {
        __RSRunLoopTimerUnlock(rlt);
    }
    RSRelease(rlt);
    return timerHandled;
}

// rl and rlm are locked on entry and exit
static BOOL __RSRunLoopDoTimers(RSRunLoopRef rl, RSRunLoopModeRef rlm, int64_t limitTSR) {	/* DOES CALLOUT */
    BOOL timerHandled = NO;
    RSMutableArrayRef timers = nil;
    for (RSIndex idx = 0, cnt = rlm->_timers ? RSArrayGetCount(rlm->_timers) : 0; idx < cnt; idx++) {
        RSRunLoopTimerRef rlt = (RSRunLoopTimerRef)RSArrayObjectAtIndex(rlm->_timers, idx);
        if (__RSRunLoopIsValid(rlt) && rlt->_fireTSR <= limitTSR && !__RSRunLoopTimerIsFiring(rlt)) {
            if (!timers) timers = RSArrayCreateMutable(RSAllocatorSystemDefault, 0);
            RSArrayAddObject(timers, rlt);
        }
    }
    for (RSIndex idx = 0, cnt = timers ? RSArrayGetCount(timers) : 0; idx < cnt; idx++) {
        RSRunLoopTimerRef rlt = (RSRunLoopTimerRef)RSArrayObjectAtIndex(timers, idx);
        BOOL did = __RSRunLoopDoTimer(rl, rlm, rlt);
        timerHandled = timerHandled || did;
    }
    if (timers) RSRelease(timers);
    return timerHandled;
}


RSExport BOOL _RSRunLoopFinished(RSRunLoopRef rl, RSStringRef modeName) {
    CHECK_FOR_FORK();
    RSRunLoopModeRef rlm;
    BOOL result = NO;
    __RSRunLoopLock(rl);
    rlm = __RSRunLoopFindMode(rl, modeName, NO);
    if (nil == rlm || __RSRunLoopModeIsEmpty(rl, rlm, nil)) {
        result = YES;
    }
    if (rlm) __RSRunLoopModeUnlock(rlm);
    __RSRunLoopUnlock(rl);
    return result;
}

static int32_t __RSRunLoopRun(RSRunLoopRef rl, RSRunLoopModeRef rlm, RSTimeInterval seconds, BOOL stopAfterHandle, RSRunLoopModeRef previousMode) __attribute__((noinline));

#if DEPLOYMENT_TARGET_MACOSX || DEPLOYMENT_TARGET_EMBEDDED || DEPLOYMENT_TARGET_EMBEDDED_MINI

#define TIMEOUT_INFINITY (~(mach_msg_timeout_t)0)

static BOOL __RSRunLoopServiceMachPort(mach_port_name_t port, mach_msg_header_t **buffer, size_t buffer_size, mach_msg_timeout_t timeout) {
    BOOL originalBuffer = YES;
    for (;;) {		/* In that sleep of death what nightmares may come ... */
        mach_msg_header_t *msg = (mach_msg_header_t *)*buffer;
        msg->msgh_bits = 0;
        msg->msgh_local_port = port;
        msg->msgh_remote_port = MACH_PORT_NULL;
        msg->msgh_size = (mach_msg_size_t)buffer_size;
        msg->msgh_id = 0;
        kern_return_t ret = mach_msg(msg, MACH_RCV_MSG|MACH_RCV_LARGE|((TIMEOUT_INFINITY != timeout) ? MACH_RCV_TIMEOUT : 0)|MACH_RCV_TRAILER_TYPE(MACH_MSG_TRAILER_FORMAT_0)|MACH_RCV_TRAILER_ELEMENTS(MACH_RCV_TRAILER_AV), 0, msg->msgh_size, port, timeout, MACH_PORT_NULL);
        if (MACH_MSG_SUCCESS == ret) return YES;
        if (MACH_RCV_TIMED_OUT == ret) {
            if (!originalBuffer) free(msg);
            *buffer = nil;
            return NO;
        }
        if (MACH_RCV_TOO_LARGE != ret) break;
        buffer_size = round_msg(msg->msgh_size + MAX_TRAILER_SIZE);
        if (originalBuffer) *buffer = nil;
        originalBuffer = NO;
        *buffer = realloc(*buffer, buffer_size);
    }
    HALT;
    return NO;
}

#elif DEPLOYMENT_TARGET_WINDOWS

#define TIMEOUT_INFINITY INFINITE

// pass in either a portSet or onePort
static BOOL __RSRunLoopWaitForMultipleObjects(__RSPortSet portSet, HANDLE *onePort, DWORD timeout, DWORD mask, HANDLE *livePort, BOOL *msgReceived) {
    DWORD waitResult = WAIT_TIMEOUT;
    HANDLE handleBuf[MAXIMUM_WAIT_OBJECTS];
    HANDLE *handles = nil;
    uint32_t handleCount = 0;
    BOOL freeHandles = NO;
    BOOL result = NO;
    
    if (portSet) {
        // copy out the handles to be safe from other threads at work
        handles = __RSPortSetGetPorts(portSet, handleBuf, MAXIMUM_WAIT_OBJECTS, &handleCount);
        freeHandles = (handles != handleBuf);
    } else {
        handles = onePort;
        handleCount = 1;
        freeHandles = NO;
    }
    
    // The run loop mode and loop are already in proper unlocked state from caller
    waitResult = MsgWaitForMultipleObjectsEx(__RSMin(handleCount, MAXIMUM_WAIT_OBJECTS), handles, timeout, mask, MWMO_INPUTAVAILABLE);
    
    RSAssert2(waitResult != WAIT_FAILED, __RSLogAssertion, "%s(): error %d from MsgWaitForMultipleObjects", __PRETTY_FUNCTION__, GetLastError());
    
    if (waitResult == WAIT_TIMEOUT) {
        // do nothing, just return to caller
        result = NO;
    } else if (waitResult >= WAIT_OBJECT_0 && waitResult < WAIT_OBJECT_0+handleCount) {
        // a handle was signaled
        if (livePort) *livePort = handles[waitResult-WAIT_OBJECT_0];
        result = YES;
    } else if (waitResult == WAIT_OBJECT_0+handleCount) {
        // windows message received
        if (msgReceived) *msgReceived = YES;
        result = YES;
    } else if (waitResult >= WAIT_ABANDONED_0 && waitResult < WAIT_ABANDONED_0+handleCount) {
        // an "abandoned mutex object"
        if (livePort) *livePort = handles[waitResult-WAIT_ABANDONED_0];
        result = YES;
    } else {
        RSAssert2(waitResult == WAIT_FAILED, __RSLogAssertion, "%s(): unexpected result from MsgWaitForMultipleObjects: %d", __PRETTY_FUNCTION__, waitResult);
        result = NO;
    }
    
    if (freeHandles) {
        RSAllocatorDeallocate(RSAllocatorSystemDefault, handles);
    }
    
    return result;
}

#endif

struct __timeout_context {
    dispatch_source_t ds;
    RSRunLoopRef rl;
    int64_t termTSR;
};

static void __RSRunLoopTimeoutCancel(void *arg) {
    struct __timeout_context *context = (struct __timeout_context *)arg;
    RSRelease(context->rl);
    dispatch_release(context->ds);
    free(context);
}

static void __RSRunLoopTimeout(void *arg) {
    struct __timeout_context *context = (struct __timeout_context *)arg;
    context->termTSR = 0LL;
    RSRunLoopWakeUp(context->rl);
    // The interval is DISPATCH_TIME_FOREVER, so this won't fire again
}

/* rl, rlm are locked on entrance and exit */
static int32_t __RSRunLoopRun(RSRunLoopRef rl, RSRunLoopModeRef rlm, RSTimeInterval seconds, BOOL stopAfterHandle, RSRunLoopModeRef previousMode) {
    int64_t startTSR = (int64_t)mach_absolute_time();
    
    if (__RSRunLoopIsStopped(rl)) {
        __RSRunLoopUnsetStopped(rl);
        return RSRunLoopRunStopped;
    } else if (rlm->_stopped) {
        rlm->_stopped = NO;
        return RSRunLoopRunStopped;
    }
    
    mach_port_name_t dispatchPort = MACH_PORT_NULL;
    BOOL libdispatchQSafe = pthread_main_np() && ((HANDLE_DISPATCH_ON_BASE_INVOCATION_ONLY && nil == previousMode) || (!HANDLE_DISPATCH_ON_BASE_INVOCATION_ONLY && 0 == _RSGetTSD(__RSTSDKeyIsInGCDMainQ)));
    if (libdispatchQSafe && (RSRunLoopGetMain() == rl) && RSSetContainsValue(rl->_commonModes, rlm->_name)) dispatchPort = _dispatch_get_main_queue_port_4RS();
    
    dispatch_source_t timeout_timer = nil;
    struct __timeout_context *timeout_context = (struct __timeout_context *)RSAllocatorAllocate(RSAllocatorSystemDefault, sizeof(*timeout_context));
    if (seconds <= 0.0) { // instant timeout
        seconds = 0.0;
        timeout_context->termTSR = 0LL;
    }
    else if (seconds <= TIMER_INTERVAL_LIMIT)
    {
        dispatch_queue_t queue = dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_HIGH, DISPATCH_QUEUE_OVERCOMMIT);
        timeout_timer = dispatch_source_create(DISPATCH_SOURCE_TYPE_TIMER, 0, 0, queue);
        dispatch_retain(timeout_timer);
        timeout_context->ds = timeout_timer;
        timeout_context->rl = (RSRunLoopRef)RSRetain(rl);
        timeout_context->termTSR = startTSR + __RSTimeIntervalToTSR(seconds);
        dispatch_set_context(timeout_timer, timeout_context); // source gets ownership of context
        dispatch_source_set_event_handler_f(timeout_timer, __RSRunLoopTimeout);
        dispatch_source_set_cancel_handler_f(timeout_timer, __RSRunLoopTimeoutCancel);
        uint64_t nanos = (uint64_t)(seconds * 1000 * 1000 + 1) * 1000;
        dispatch_source_set_timer(timeout_timer, dispatch_time(DISPATCH_TIME_NOW, nanos), DISPATCH_TIME_FOREVER, 0);
        dispatch_resume(timeout_timer);
    }
    else { // infinite timeout
        seconds = 9999999999.0;
        timeout_context->termTSR = INT64_MAX;
    }
    
    BOOL didDispatchPortLastTime = YES;
    int32_t retVal = 0;
    do
    {
        uint8_t msg_buffer[3 * 1024];
#if DEPLOYMENT_TARGET_MACOSX || DEPLOYMENT_TARGET_EMBEDDED || DEPLOYMENT_TARGET_EMBEDDED_MINI
        mach_msg_header_t *msg = nil;
#elif DEPLOYMENT_TARGET_WINDOWS
        HANDLE livePort = nil;
        BOOL windowsMessageReceived = NO;
#endif
        __RSPortSet waitSet = rlm->_portSet;
        
        __RSRunLoopUnsetIgnoreWakeUps(rl);
        
        if (rlm->_observerMask & RSRunLoopBeforeTimers) __RSRunLoopDoObservers(rl, rlm, RSRunLoopBeforeTimers);
        if (rlm->_observerMask & RSRunLoopBeforeSources) __RSRunLoopDoObservers(rl, rlm, RSRunLoopBeforeSources);
        
        __RSRunLoopDoBlocks(rl, rlm);
        
        BOOL sourceHandledThisLoop = __RSRunLoopDoSources0(rl, rlm, stopAfterHandle);
        if (sourceHandledThisLoop) {
            __RSRunLoopDoBlocks(rl, rlm);
        }
        
        BOOL poll = sourceHandledThisLoop || (0LL == timeout_context->termTSR);
        
        if (MACH_PORT_NULL != dispatchPort && !didDispatchPortLastTime) {
#if DEPLOYMENT_TARGET_MACOSX || DEPLOYMENT_TARGET_EMBEDDED || DEPLOYMENT_TARGET_EMBEDDED_MINI
            msg = (mach_msg_header_t *)msg_buffer;
            if (__RSRunLoopServiceMachPort(dispatchPort, &msg, sizeof(msg_buffer), 0)) {
                goto handle_msg;
            }
#elif DEPLOYMENT_TARGET_WINDOWS
            if (__RSRunLoopWaitForMultipleObjects(nil, &dispatchPort, 0, 0, &livePort, nil)) {
                goto handle_msg;
            }
#endif
        }
        
        didDispatchPortLastTime = NO;
        
        if (!poll && (rlm->_observerMask & RSRunLoopBeforeWaiting)) __RSRunLoopDoObservers(rl, rlm, RSRunLoopBeforeWaiting);
        __RSRunLoopSetSleeping(rl);
        // do not do any user callouts after this point (after notifying of sleeping)
        
        // Must push the local-to-this-activation ports in on every loop
        // iteration, as this mode could be run re-entrantly and we don't
        // want these ports to get serviced.
        
        __RSPortSetInsert(dispatchPort, waitSet);
        
        __RSRunLoopModeUnlock(rlm);
        __RSRunLoopUnlock(rl);
        
#if DEPLOYMENT_TARGET_MACOSX || DEPLOYMENT_TARGET_EMBEDDED || DEPLOYMENT_TARGET_EMBEDDED_MINI
        if (RSUseCollectableAllocator) {
//            objc_clear_stack(0);
            memset(msg_buffer, 0, sizeof(msg_buffer));
        }
        msg = (mach_msg_header_t *)msg_buffer;
        __RSRunLoopServiceMachPort(waitSet, &msg, sizeof(msg_buffer), poll ? 0 : TIMEOUT_INFINITY);
#elif DEPLOYMENT_TARGET_WINDOWS
        // Here, use the app-supplied message queue mask. They will set this if they are interested in having this run loop receive windows messages.
        __RSRunLoopWaitForMultipleObjects(waitSet, nil, poll ? 0 : TIMEOUT_INFINITY, rlm->_msgQMask, &livePort, &windowsMessageReceived);
#endif
        
        __RSRunLoopLock(rl);
        __RSRunLoopModeLock(rlm);
        
        // Must remove the local-to-this-activation ports in on every loop
        // iteration, as this mode could be run re-entrantly and we don't
        // want these ports to get serviced. Also, we don't want them left
        // in there if this function returns.
        
        __RSPortSetRemove(dispatchPort, waitSet);
        
        __RSRunLoopSetIgnoreWakeUps(rl);
        
        // user callouts now OK again
        __RSRunLoopUnsetSleeping(rl);
        if (!poll && (rlm->_observerMask & RSRunLoopAfterWaiting)) __RSRunLoopDoObservers(rl, rlm, RSRunLoopAfterWaiting);
        
    handle_msg:;
        __RSRunLoopSetIgnoreWakeUps(rl);
        
#if DEPLOYMENT_TARGET_MACOSX || DEPLOYMENT_TARGET_EMBEDDED || DEPLOYMENT_TARGET_EMBEDDED_MINI
        mach_port_t livePort = msg ? msg->msgh_local_port : MACH_PORT_NULL;
#endif
#if DEPLOYMENT_TARGET_WINDOWS
        if (windowsMessageReceived) {
            // These Win32 APIs cause a callout, so make sure we're unlocked first and relocked after
            __RSRunLoopModeUnlock(rlm);
            __RSRunLoopUnlock(rl);
            
            if (rlm->_msgPump) {
                rlm->_msgPump();
            } else {
                MSG msg;
                if (PeekMessage(&msg, nil, 0, 0, PM_REMOVE | PM_NOYIELD)) {
                    TranslateMessage(&msg);
                    DispatchMessage(&msg);
                }
            }
            
            __RSRunLoopLock(rl);
            __RSRunLoopModeLock(rlm);
            sourceHandledThisLoop = YES;
            
            // To prevent starvation of sources other than the message queue, we check again to see if any other sources need to be serviced
            // Use 0 for the mask so windows messages are ignored this time. Also use 0 for the timeout, because we're just checking to see if the things are signalled right now -- we will wait on them again later.
            // NOTE: Ignore the dispatch source (it's not in the wait set anymore) and also don't run the observers here since we are polling.
            __RSRunLoopSetSleeping(rl);
            __RSRunLoopModeUnlock(rlm);
            __RSRunLoopUnlock(rl);
            
            __RSRunLoopWaitForMultipleObjects(waitSet, nil, 0, 0, &livePort, nil);
            
            __RSRunLoopLock(rl);
            __RSRunLoopModeLock(rlm);
            __RSRunLoopUnsetSleeping(rl);
            // If we have a new live port then it will be handled below as normal
        }
        
        
#endif
        if (MACH_PORT_NULL == livePort) {
            // handle nothing
        } else if (livePort == rl->_wakeUpPort) {
            // do nothing on Mac OS
#if DEPLOYMENT_TARGET_WINDOWS
            // Always reset the wake up port, or risk spinning forever
            ResetEvent(rl->_wakeUpPort);
#endif
        } else if (livePort == rlm->_timerPort) {
#if DEPLOYMENT_TARGET_WINDOWS
            // On Windows, we have observed an issue where the timer port is set before the time which we requested it to be set. For example, we set the fire time to be TSR 167646765860, but it is actually observed firing at TSR 167646764145, which is 1715 ticks early. On the hardware where this was observed, multiplying by the TSR to seconds conversion rate shows that this was 0.000478~ seconds early, but the tenus used when setting fire dates is 0.00001 seconds. The result is that, when __RSRunLoopDoTimers checks to see if any of the run loop timers should be firing, it appears to be 'too early' for the next timer, and no timers are handled.
            // In this case, the timer port has been automatically reset (since it was returned from MsgWaitForMultipleObjectsEx), and if we do not re-arm it, then no timers will ever be serviced again unless something adjusts the timer list (e.g. adding or removing timers). The fix for the issue is to reset the timer here if RSRunLoopDoTimers did not handle a timer itself. 9308754
            if (!__RSRunLoopDoTimers(rl, rlm, mach_absolute_time())) {
                // Re-arm the next timer
                __RSArmNextTimerInMode(rlm);
            }
#else
            __RSRunLoopDoTimers(rl, rlm, mach_absolute_time());
#endif
        } else if (livePort == dispatchPort) {
	        __RSRunLoopModeUnlock(rlm);
	        __RSRunLoopUnlock(rl);
            _RSSetTSD(__RSTSDKeyIsInGCDMainQ, (void *)6, nil);
 	        _dispatch_main_queue_callback_4RS(msg);
            _RSSetTSD(__RSTSDKeyIsInGCDMainQ, (void *)0, nil);
	        __RSRunLoopLock(rl);
	        __RSRunLoopModeLock(rlm);
 	        sourceHandledThisLoop = YES;
            didDispatchPortLastTime = YES;
        } else {
            // Despite the name, this works for windows handles as well
            RSRunLoopSourceRef rls = __RSRunLoopModeFindSourceForMachPort(rl, rlm, livePort);
            if (rls) {
#if DEPLOYMENT_TARGET_MACOSX || DEPLOYMENT_TARGET_EMBEDDED || DEPLOYMENT_TARGET_EMBEDDED_MINI
                mach_msg_header_t *reply = nil;
                sourceHandledThisLoop = __RSRunLoopDoSource1(rl, rlm, rls, msg, msg->msgh_size, &reply) || sourceHandledThisLoop;
                if (nil != reply) {
                    (void)mach_msg(reply, MACH_SEND_MSG, reply->msgh_size, 0, MACH_PORT_NULL, 0, MACH_PORT_NULL);
                    RSAllocatorDeallocate(RSAllocatorSystemDefault, reply);
                }
#elif DEPLOYMENT_TARGET_WINDOWS
                sourceHandledThisLoop = __RSRunLoopDoSource1(rl, rlm, rls) || sourceHandledThisLoop;
#endif
            }
        }
#if DEPLOYMENT_TARGET_MACOSX || DEPLOYMENT_TARGET_EMBEDDED || DEPLOYMENT_TARGET_EMBEDDED_MINI
        if (msg && msg != (mach_msg_header_t *)msg_buffer) free(msg);
#endif
        
        __RSRunLoopDoBlocks(rl, rlm);
        
        if (sourceHandledThisLoop && stopAfterHandle) {
            retVal = RSRunLoopRunHandledSource;
        } else if (timeout_context->termTSR < (int64_t)mach_absolute_time()) {
            retVal = RSRunLoopRunTimedOut;
        } else if (__RSRunLoopIsStopped(rl)) {
            __RSRunLoopUnsetStopped(rl);
            retVal = RSRunLoopRunStopped;
        } else if (rlm->_stopped) {
            rlm->_stopped = NO;
            retVal = RSRunLoopRunStopped;
        } else if (__RSRunLoopModeIsEmpty(rl, rlm, previousMode)) {
            retVal = RSRunLoopRunFinished;
        }
    } while (0 == retVal);
    
    if (timeout_timer) {
        dispatch_source_cancel(timeout_timer);
        dispatch_release(timeout_timer);
    } else {
        free(timeout_context);
    }
    
    return retVal;
}

SInt32 RSRunLoopRunSpecific(RSRunLoopRef rl, RSStringRef modeName, RSTimeInterval seconds, BOOL returnAfterSourceHandled) {     /* DOES CALLOUT */
    CHECK_FOR_FORK();
    if (__RSRunLoopIsDeallocating(rl)) return RSRunLoopRunFinished;
    __RSRunLoopLock(rl);
    RSRunLoopModeRef currentMode = __RSRunLoopFindMode(rl, modeName, NO);
    if (nil == currentMode || __RSRunLoopModeIsEmpty(rl, currentMode, rl->_currentMode)) {
        BOOL did = NO;
        if (currentMode) __RSRunLoopModeUnlock(currentMode);
        __RSRunLoopUnlock(rl);
        return did ? RSRunLoopRunHandledSource : RSRunLoopRunFinished;
    }
    volatile _per_run_data *previousPerRun = __RSRunLoopPushPerRunData(rl);
    RSRunLoopModeRef previousMode = rl->_currentMode;
    rl->_currentMode = currentMode;
    int32_t result;
	if (currentMode->_observerMask & RSRunLoopEntry ) __RSRunLoopDoObservers(rl, currentMode, RSRunLoopEntry);
	result = __RSRunLoopRun(rl, currentMode, seconds, returnAfterSourceHandled, previousMode);
	if (currentMode->_observerMask & RSRunLoopExit ) __RSRunLoopDoObservers(rl, currentMode, RSRunLoopExit);
    __RSRunLoopModeUnlock(currentMode);
    __RSRunLoopPopPerRunData(rl, previousPerRun);
	rl->_currentMode = previousMode;
    __RSRunLoopUnlock(rl);
    return result;
}

void RSRunLoopRun(void) {	/* DOES CALLOUT */
    int32_t result;
    do {
        result = RSRunLoopRunSpecific(RSRunLoopGetCurrent(), RSRunLoopDefaultMode, 1.0e10, NO);
        CHECK_FOR_FORK();
    } while (RSRunLoopRunStopped != result && RSRunLoopRunFinished != result);
}

SInt32 RSRunLoopRunInMode(RSStringRef modeName, RSTimeInterval seconds, BOOL returnAfterSourceHandled) {     /* DOES CALLOUT */
    CHECK_FOR_FORK();
    return RSRunLoopRunSpecific(RSRunLoopGetCurrent(), modeName, seconds, returnAfterSourceHandled);
}

RSAbsoluteTime RSRunLoopTimerGetNextFireDate(RSRunLoopTimerRef rlt);

RSAbsoluteTime RSRunLoopGetNextTimerFireDate(RSRunLoopRef rl, RSStringRef modeName) {
    CHECK_FOR_FORK();
    __RSRunLoopLock(rl);
    RSRunLoopModeRef rlm = __RSRunLoopFindMode(rl, modeName, NO);
    RSAbsoluteTime at = 0.0;
    RSRunLoopTimerRef timer = (rlm && rlm->_timers && 0 < RSArrayGetCount(rlm->_timers)) ? (RSRunLoopTimerRef)RSArrayObjectAtIndex(rlm->_timers, 0) : nil;
    if (timer) at = RSRunLoopTimerGetNextFireDate(timer);
    if (rlm) __RSRunLoopModeUnlock(rlm);
    __RSRunLoopUnlock(rl);
    return at;
}

BOOL RSRunLoopIsWaiting(RSRunLoopRef rl) {
    CHECK_FOR_FORK();
    return __RSRunLoopIsSleeping(rl);
}

void RSRunLoopWakeUp(RSRunLoopRef rl) {
    CHECK_FOR_FORK();
    // This lock is crucial to ignorable wakeups, do not remove it.
    __RSRunLoopLock(rl);
    if (__RSRunLoopIsIgnoringWakeUps(rl)) {
        __RSRunLoopUnlock(rl);
        return;
    }
#if DEPLOYMENT_TARGET_MACOSX || DEPLOYMENT_TARGET_EMBEDDED || DEPLOYMENT_TARGET_EMBEDDED_MINI
    kern_return_t ret;
    /* We unconditionally try to send the message, since we don't want
     * to lose a wakeup, but the send may fail if there is already a
     * wakeup pending, since the queue length is 1. */
    ret = __RSSendTrivialMachMessage(rl->_wakeUpPort, 0, MACH_SEND_TIMEOUT, 0);
    if (ret != MACH_MSG_SUCCESS && ret != MACH_SEND_TIMED_OUT) {
        char msg[256];
        snprintf(msg, 256, "*** Unable to send message to wake up port. (%d) ***", ret);
        //CRSetCrashLogMessage(msg);
        HALT;
    }
#elif DEPLOYMENT_TARGET_WINDOWS
    SetEvent(rl->_wakeUpPort);
#endif
    __RSRunLoopUnlock(rl);
}

void RSRunLoopStop(RSRunLoopRef rl) {
    BOOL doWake = NO;
    CHECK_FOR_FORK();
    __RSRunLoopLock(rl);
    if (rl->_currentMode) {
        __RSRunLoopSetStopped(rl);
        doWake = YES;
    }
    __RSRunLoopUnlock(rl);
    if (doWake) {
        RSRunLoopWakeUp(rl);
    }
}

RSExport void _RSRunLoopStopMode(RSRunLoopRef rl, RSStringRef modeName) {
    CHECK_FOR_FORK();
    RSRunLoopModeRef rlm;
    __RSRunLoopLock(rl);
    rlm = __RSRunLoopFindMode(rl, modeName, YES);
    if (nil != rlm) {
        rlm->_stopped = YES;
        __RSRunLoopModeUnlock(rlm);
    }
    __RSRunLoopUnlock(rl);
    RSRunLoopWakeUp(rl);
}

RSExport BOOL _RSRunLoopModeContainsMode(RSRunLoopRef rl, RSStringRef modeName, RSStringRef candidateContainedName) {
    CHECK_FOR_FORK();
    return NO;
}

void RSRunLoopPerformBlock(RSRunLoopRef rl, RSTypeRef mode, void (^block)(void)) {
    CHECK_FOR_FORK();
    if (RSStringGetTypeID() == RSGetTypeID(mode)) {
        mode = RSStringCreateCopy(RSAllocatorSystemDefault, (RSStringRef)mode);
        __RSRunLoopLock(rl);
        // ensure mode exists
        RSRunLoopModeRef currentMode = __RSRunLoopFindMode(rl, (RSStringRef)mode, YES);
        if (currentMode) __RSRunLoopModeUnlock(currentMode);
        __RSRunLoopUnlock(rl);
    } else if (RSArrayGetTypeID() == RSGetTypeID(mode)) {
        RSIndex cnt = RSArrayGetCount((RSArrayRef)mode);
        const void **values = (const void **)RSAllocatorAllocate(RSAllocatorSystemDefault, sizeof(const void *) * cnt);
        RSArrayGetObjects((RSArrayRef)mode, RSMakeRange(0, cnt), values);
        mode = RSSetCreate(RSAllocatorSystemDefault, values, cnt, &RSTypeSetCallBacks);
        __RSRunLoopLock(rl);
        // ensure modes exist
        for (RSIndex idx = 0; idx < cnt; idx++) {
            RSRunLoopModeRef currentMode = __RSRunLoopFindMode(rl, (RSStringRef)values[idx], YES);
            if (currentMode) __RSRunLoopModeUnlock(currentMode);
        }
        __RSRunLoopUnlock(rl);
        free(values);
    } else if (RSSetGetTypeID() == RSGetTypeID(mode)) {
        RSIndex cnt = RSSetGetCount((RSSetRef)mode);
        const void **values = (const void **)RSAllocatorAllocate(RSAllocatorSystemDefault, sizeof(const void *) * cnt);
        RSSetGetValues((RSSetRef)mode, values);
        mode = RSSetCreate(RSAllocatorSystemDefault, values, cnt, &RSTypeSetCallBacks);
        __RSRunLoopLock(rl);
        // ensure modes exist
        for (RSIndex idx = 0; idx < cnt; idx++) {
            RSRunLoopModeRef currentMode = __RSRunLoopFindMode(rl, (RSStringRef)values[idx], YES);
            if (currentMode) __RSRunLoopModeUnlock(currentMode);
        }
        __RSRunLoopUnlock(rl);
        free(values);
    } else {
        mode = nil;
    }
    block = Block_copy(block);
    if (!mode || !block) {
        if (mode) RSRelease(mode);
        if (block) Block_release(block);
        return;
    }
    __RSRunLoopLock(rl);
    struct _block_item *new_item = (struct _block_item *)RSAllocatorAllocate(RSAllocatorSystemDefault, sizeof(struct _block_item));
    new_item->_next = nil;
    new_item->_mode = mode;
    new_item->_block = block;
    if (!rl->_blocks_tail) {
        rl->_blocks_head = new_item;
    } else {
        rl->_blocks_tail->_next = new_item;
    }
    rl->_blocks_tail = new_item;
    __RSRunLoopUnlock(rl);
}

BOOL RSRunLoopContainsSource(RSRunLoopRef rl, RSRunLoopSourceRef rls, RSStringRef modeName) {
    CHECK_FOR_FORK();
    RSRunLoopModeRef rlm;
    BOOL hasValue = NO;
    __RSRunLoopLock(rl);
    if (modeName == RSRunLoopCommonModes) {
        if (nil != rl->_commonModeItems) {
            hasValue = RSSetContainsValue(rl->_commonModeItems, rls);
        }
    } else {
        rlm = __RSRunLoopFindMode(rl, modeName, NO);
        if (nil != rlm) {
            hasValue = (rlm->_sources0 ? RSSetContainsValue(rlm->_sources0, rls) : NO) || (rlm->_sources1 ? RSSetContainsValue(rlm->_sources1, rls) : NO);
            __RSRunLoopModeUnlock(rlm);
        }
    }
    __RSRunLoopUnlock(rl);
    return hasValue;
}

void RSRunLoopAddSource(RSRunLoopRef rl, RSRunLoopSourceRef rls, RSStringRef modeName) {	/* DOES CALLOUT */
    CHECK_FOR_FORK();
    if (__RSRunLoopIsDeallocating(rl)) return;
    if (!__RSRunLoopIsValid(rls)) return;
    BOOL doVer0Callout = NO;
    __RSRunLoopLock(rl);
    if (modeName == RSRunLoopCommonModes)
    {
        RSSetRef set = rl->_commonModes ? RSSetCreateCopy(RSAllocatorSystemDefault, rl->_commonModes) : nil;
        if (nil == rl->_commonModeItems) {
            rl->_commonModeItems = RSSetCreateMutable(RSAllocatorSystemDefault, 0, &RSTypeSetCallBacks);
        }
        RSSetAddValue(rl->_commonModeItems, rls);
        if (nil != set) {
            RSTypeRef context[2] = {rl, rls};
            /* add new item to all common-modes */
            RSSetApplyFunction(set, (__RSRunLoopAddItemToCommonModes), (void *)context);
            RSRelease(set);
        }
    } else
    {
        RSRunLoopModeRef rlm = __RSRunLoopFindMode(rl, modeName, YES);
        if (nil != rlm && nil == rlm->_sources0) {
            rlm->_sources0 = RSSetCreateMutable(RSAllocatorSystemDefault, 0, &RSTypeSetCallBacks);
            rlm->_sources1 = RSSetCreateMutable(RSAllocatorSystemDefault, 0, &RSTypeSetCallBacks);
            rlm->_portToV1SourceMap = RSDictionaryCreateMutable(RSAllocatorSystemDefault, 0, nil);
        }
        if (nil != rlm && !RSSetContainsValue(rlm->_sources0, rls) && !RSSetContainsValue(rlm->_sources1, rls)) {
            if (0 == rls->_context.version0.version) {
                RSSetAddValue(rlm->_sources0, rls);
            } else if (1 == rls->_context.version0.version) {
                RSSetAddValue(rlm->_sources1, rls);
                __RSPort src_port = rls->_context.version1.getPort(rls->_context.version1.info);
                if (RSPORT_nil != src_port) {
                    RSDictionarySetValue(rlm->_portToV1SourceMap, (const void *)(uintptr_t)src_port, rls);
                    __RSPortSetInsert(src_port, rlm->_portSet);
                }
            }
            __RSRunLoopSourceLock(rls);
            if (nil == rls->_runLoops) {
                rls->_runLoops = RSBagCreateMutable(RSAllocatorSystemDefault, 0, &RSTypeBagCallBacks); // sources retain run loops!
            }
            RSBagAddValue(rls->_runLoops, rl);
            __RSRunLoopSourceUnlock(rls);
            if (0 == rls->_context.version0.version) {
                if (nil != rls->_context.version0.schedule) {
                    doVer0Callout = YES;
                }
            }
        }
        if (nil != rlm) {
            __RSRunLoopModeUnlock(rlm);
        }
    }
    __RSRunLoopUnlock(rl);
    if (doVer0Callout) {
        // although it looses some protection for the source, we have no choice but
        // to do this after unlocking the run loop and mode locks, to avoid deadlocks
        // where the source wants to take a lock which is already held in another
        // thread which is itself waiting for a run loop/mode lock
        rls->_context.version0.schedule(rls->_context.version0.info, rl, modeName);	/* CALLOUT */
    }
}

void RSRunLoopRemoveSource(RSRunLoopRef rl, RSRunLoopSourceRef rls, RSStringRef modeName) {	/* DOES CALLOUT */
    CHECK_FOR_FORK();
    BOOL doVer0Callout = NO, doRLSRelease = NO;
    __RSRunLoopLock(rl);
    if (modeName == RSRunLoopCommonModes) {
        if (nil != rl->_commonModeItems && RSSetContainsValue(rl->_commonModeItems, rls)) {
            RSSetRef set = rl->_commonModes ? RSSetCreateCopy(RSAllocatorSystemDefault, rl->_commonModes) : nil;
            RSSetRemoveValue(rl->_commonModeItems, rls);
            if (nil != set) {
                RSTypeRef context[2] = {rl, rls};
                /* remove new item from all common-modes */
                RSSetApplyFunction(set, (__RSRunLoopRemoveItemFromCommonModes), (void *)context);
                RSRelease(set);
            }
        } else {
        }
    } else {
        RSRunLoopModeRef rlm = __RSRunLoopFindMode(rl, modeName, NO);
        if (nil != rlm && ((nil != rlm->_sources0 && RSSetContainsValue(rlm->_sources0, rls)) || (nil != rlm->_sources1 && RSSetContainsValue(rlm->_sources1, rls)))) {
            RSRetain(rls);
            if (1 == rls->_context.version0.version) {
                __RSPort src_port = rls->_context.version1.getPort(rls->_context.version1.info);
                if (RSPORT_nil != src_port) {
                    RSDictionaryRemoveValue(rlm->_portToV1SourceMap, (const void *)(uintptr_t)src_port);
                    __RSPortSetRemove(src_port, rlm->_portSet);
                }
            }
            RSSetRemoveValue(rlm->_sources0, rls);
            RSSetRemoveValue(rlm->_sources1, rls);
            __RSRunLoopSourceLock(rls);
            if (nil != rls->_runLoops) {
                RSBagRemoveValue(rls->_runLoops, rl);
            }
            __RSRunLoopSourceUnlock(rls);
            if (0 == rls->_context.version0.version) {
                if (nil != rls->_context.version0.schedule) {
                    doVer0Callout = YES;
                }
            }
            doRLSRelease = YES;
        }
        if (nil != rlm) {
            __RSRunLoopModeUnlock(rlm);
        }
    }
    __RSRunLoopUnlock(rl);
    if (doVer0Callout) {
        // although it looses some protection for the source, we have no choice but
        // to do this after unlocking the run loop and mode locks, to avoid deadlocks
        // where the source wants to take a lock which is already held in another
        // thread which is itself waiting for a run loop/mode lock
        rls->_context.version0.cancel(rls->_context.version0.info, rl, modeName);	/* CALLOUT */
    }
    if (doRLSRelease) RSRelease(rls);
}

static void __RSRunLoopRemoveSourcesFromCommonMode(const void *value, void *ctx) {
    RSStringRef modeName = (RSStringRef)value;
    RSRunLoopRef rl = (RSRunLoopRef)ctx;
    __RSRunLoopRemoveAllSources(rl, modeName);
}

static void __RSRunLoopRemoveSourceFromMode(const void *value, void *ctx) {
    RSRunLoopSourceRef rls = (RSRunLoopSourceRef)value;
    RSRunLoopRef rl = (RSRunLoopRef)(((RSTypeRef *)ctx)[0]);
    RSStringRef modeName = (RSStringRef)(((RSTypeRef *)ctx)[1]);
    RSRunLoopRemoveSource(rl, rls, modeName);
}

static void __RSRunLoopRemoveAllSources(RSRunLoopRef rl, RSStringRef modeName) {
    CHECK_FOR_FORK();
    RSRunLoopModeRef rlm;
    __RSRunLoopLock(rl);
    if (modeName == RSRunLoopCommonModes) {
        if (nil != rl->_commonModeItems) {
            RSSetRef set = rl->_commonModes ? RSSetCreateCopy(RSAllocatorSystemDefault, rl->_commonModes) : nil;
            if (nil != set) {
                RSSetApplyFunction(set, (__RSRunLoopRemoveSourcesFromCommonMode), (void *)rl);
                RSRelease(set);
            }
        } else {
        }
    } else {
        rlm = __RSRunLoopFindMode(rl, modeName, NO);
        if (nil != rlm && nil != rlm->_sources0) {
            RSSetRef set = RSSetCreateCopy(RSAllocatorSystemDefault, rlm->_sources0);
            RSTypeRef context[2] = {rl, modeName};
            RSSetApplyFunction(set, (__RSRunLoopRemoveSourceFromMode), (void *)context);
            RSRelease(set);
        }
        if (nil != rlm && nil != rlm->_sources1) {
            RSSetRef set = RSSetCreateCopy(RSAllocatorSystemDefault, rlm->_sources1);
            RSTypeRef context[2] = {rl, modeName};
            RSSetApplyFunction(set, (__RSRunLoopRemoveSourceFromMode), (void *)context);
            RSRelease(set);
        }
        if (nil != rlm) {
            __RSRunLoopModeUnlock(rlm);
        }
    }
    __RSRunLoopUnlock(rl);
}

BOOL RSRunLoopContainsObserver(RSRunLoopRef rl, RSRunLoopObserverRef rlo, RSStringRef modeName) {
    CHECK_FOR_FORK();
    RSRunLoopModeRef rlm;
    BOOL hasValue = NO;
    __RSRunLoopLock(rl);
    if (modeName == RSRunLoopCommonModes) {
        if (nil != rl->_commonModeItems) {
            hasValue = RSSetContainsValue(rl->_commonModeItems, rlo);
        }
    } else {
        rlm = __RSRunLoopFindMode(rl, modeName, NO);
        if (nil != rlm && nil != rlm->_observers) {
            hasValue = RSArrayContainsObject(rlm->_observers, RSMakeRange(0, RSArrayGetCount(rlm->_observers)), rlo);
        }
        if (nil != rlm) {
            __RSRunLoopModeUnlock(rlm);
        }
    }
    __RSRunLoopUnlock(rl);
    return hasValue;
}

void RSRunLoopAddObserver(RSRunLoopRef rl, RSRunLoopObserverRef rlo, RSStringRef modeName) {
    CHECK_FOR_FORK();
    RSRunLoopModeRef rlm;
    if (__RSRunLoopIsDeallocating(rl)) return;
    if (!__RSRunLoopIsValid(rlo) || (nil != rlo->_runLoop && rlo->_runLoop != rl)) return;
    __RSRunLoopLock(rl);
    if (modeName == RSRunLoopCommonModes) {
        RSSetRef set = rl->_commonModes ? RSSetCreateCopy(RSAllocatorSystemDefault, rl->_commonModes) : nil;
        if (nil == rl->_commonModeItems) {
            rl->_commonModeItems = RSSetCreateMutable(RSAllocatorSystemDefault, 0, &RSTypeSetCallBacks);
        }
        RSSetAddValue(rl->_commonModeItems, rlo);
        if (nil != set) {
            RSTypeRef context[2] = {rl, rlo};
            /* add new item to all common-modes */
            RSSetApplyFunction(set, (__RSRunLoopAddItemToCommonModes), (void *)context);
            RSRelease(set);
        }
    } else {
        rlm = __RSRunLoopFindMode(rl, modeName, YES);
        if (nil != rlm && nil == rlm->_observers) {
            rlm->_observers = RSArrayCreateMutable(RSAllocatorSystemDefault, 0);
        }
        if (nil != rlm && !RSArrayContainsObject(rlm->_observers, RSMakeRange(0, RSArrayGetCount(rlm->_observers)), rlo)) {
            BOOL inserted = NO;
            for (RSIndex idx = RSArrayGetCount(rlm->_observers); idx--; ) {
                RSRunLoopObserverRef obs = (RSRunLoopObserverRef)RSArrayObjectAtIndex(rlm->_observers, idx);
                if (obs->_order <= rlo->_order) {
                    RSArrayInsertObjectAtIndex(rlm->_observers, idx + 1, rlo);
                    inserted = YES;
                    break;
                }
            }
            if (!inserted) {
                RSArrayInsertObjectAtIndex(rlm->_observers, 0, rlo);
            }
            rlm->_observerMask |= rlo->_activities;
            __RSRunLoopObserverSchedule(rlo, rl, rlm);
        }
        if (nil != rlm) {
            __RSRunLoopModeUnlock(rlm);
        }
    }
    __RSRunLoopUnlock(rl);
}

void RSRunLoopRemoveObserver(RSRunLoopRef rl, RSRunLoopObserverRef rlo, RSStringRef modeName) {
    CHECK_FOR_FORK();
    RSRunLoopModeRef rlm;
    __RSRunLoopLock(rl);
    if (modeName == RSRunLoopCommonModes) {
        if (nil != rl->_commonModeItems && RSSetContainsValue(rl->_commonModeItems, rlo)) {
            RSSetRef set = rl->_commonModes ? RSSetCreateCopy(RSAllocatorSystemDefault, rl->_commonModes) : nil;
            RSSetRemoveValue(rl->_commonModeItems, rlo);
            if (nil != set) {
                RSTypeRef context[2] = {rl, rlo};
                /* remove new item from all common-modes */
                RSSetApplyFunction(set, (__RSRunLoopRemoveItemFromCommonModes), (void *)context);
                RSRelease(set);
            }
        } else {
        }
    } else {
        rlm = __RSRunLoopFindMode(rl, modeName, NO);
        if (nil != rlm && nil != rlm->_observers) {
            RSRetain(rlo);
            RSIndex idx = RSArrayIndexOfObject(rlm->_observers, rlo);
            if (RSNotFound != idx) {
                RSArrayRemoveObjectAtIndex(rlm->_observers, idx);
                __RSRunLoopObserverCancel(rlo, rl, rlm);
            }
            RSRelease(rlo);
        }
        if (nil != rlm) {
            __RSRunLoopModeUnlock(rlm);
        }
    }
    __RSRunLoopUnlock(rl);
}

BOOL RSRunLoopContainsTimer(RSRunLoopRef rl, RSRunLoopTimerRef rlt, RSStringRef modeName) {
    CHECK_FOR_FORK();
    if (nil == rlt->_runLoop || rl != rlt->_runLoop) return NO;
    BOOL hasValue = NO;
    __RSRunLoopLock(rl);
    if (modeName == RSRunLoopCommonModes) {
        if (nil != rl->_commonModeItems) {
            hasValue = RSSetContainsValue(rl->_commonModeItems, rlt);
        }
    } else {
        RSRunLoopModeRef rlm = __RSRunLoopFindMode(rl, modeName, NO);
        if (nil != rlm && nil != rlm->_timers) {
            RSIndex idx = RSArrayIndexOfObject(rlm->_timers, rlt);
            hasValue = (RSNotFound != idx);
        }
        if (nil != rlm) {
            __RSRunLoopModeUnlock(rlm);
        }
    }
    __RSRunLoopUnlock(rl);
    return hasValue;
}

void RSRunLoopAddTimer(RSRunLoopRef rl, RSRunLoopTimerRef rlt, RSStringRef modeName) {
    CHECK_FOR_FORK();
    if (__RSRunLoopIsDeallocating(rl)) return;
    if (!__RSRunLoopIsValid(rlt) || (nil != rlt->_runLoop && rlt->_runLoop != rl)) return;
    __RSRunLoopLock(rl);
    if (modeName == RSRunLoopCommonModes) {
        RSSetRef set = rl->_commonModes ? RSSetCreateCopy(RSAllocatorSystemDefault, rl->_commonModes) : nil;
        if (nil == rl->_commonModeItems) {
            rl->_commonModeItems = RSSetCreateMutable(RSAllocatorSystemDefault, 0, &RSTypeSetCallBacks);
        }
        RSSetAddValue(rl->_commonModeItems, rlt);
        if (nil != set) {
            RSTypeRef context[2] = {rl, rlt};
            /* add new item to all common-modes */
            RSSetApplyFunction(set, (__RSRunLoopAddItemToCommonModes), (void *)context);
            RSRelease(set);
        }
    } else {
        RSRunLoopModeRef rlm = __RSRunLoopFindMode(rl, modeName, YES);
        if (nil != rlm && nil == rlm->_timers) {
            extern RSArrayCallBacks RSTypeArrayCallBacks;
            RSArrayCallBacks cb = RSTypeArrayCallBacks;
            cb.equal = nil;
            rlm->_timers = __RSArrayCreateMutable0(RSAllocatorSystemDefault, 0, &cb);
        }
        if (nil != rlm && !RSSetContainsValue(rlt->_rlModes, rlm->_name)) {
            __RSRunLoopTimerLock(rlt);
            if (nil == rlt->_runLoop) {
                rlt->_runLoop = rl;
            } else if (rl != rlt->_runLoop) {
                __RSRunLoopTimerUnlock(rlt);
                __RSRunLoopModeUnlock(rlm);
                __RSRunLoopUnlock(rl);
                return;
            }
            RSSetAddValue(rlt->_rlModes, rlm->_name);
            __RSRunLoopTimerUnlock(rlt);
            __RSRunLoopTimerFireTSRLock();
            __RSRepositionTimerInMode(rlm, rlt, NO);
            __RSRunLoopTimerFireTSRUnlock();
            if (!_RSExecutableLinkedOnOrAfter(RSSystemVersionLion)) {
                // Normally we don't do this on behalf of clients, but for
                // backwards compatibility due to the change in timer handling...
                if (rl != RSRunLoopGetCurrent()) RSRunLoopWakeUp(rl);
            }
        }
        if (nil != rlm) {
            __RSRunLoopModeUnlock(rlm);
        }
    }
    __RSRunLoopUnlock(rl);
}

void RSRunLoopRemoveTimer(RSRunLoopRef rl, RSRunLoopTimerRef rlt, RSStringRef modeName) {
    CHECK_FOR_FORK();
    __RSRunLoopLock(rl);
    if (modeName == RSRunLoopCommonModes) {
        if (nil != rl->_commonModeItems && RSSetContainsValue(rl->_commonModeItems, rlt)) {
            RSSetRef set = rl->_commonModes ? RSSetCreateCopy(RSAllocatorSystemDefault, rl->_commonModes) : nil;
            RSSetRemoveValue(rl->_commonModeItems, rlt);
            if (nil != set) {
                RSTypeRef context[2] = {rl, rlt};
                /* remove new item from all common-modes */
                RSSetApplyFunction(set, (__RSRunLoopRemoveItemFromCommonModes), (void *)context);
                RSRelease(set);
            }
        } else {
        }
    } else {
        RSRunLoopModeRef rlm = __RSRunLoopFindMode(rl, modeName, NO);
        RSIndex idx = RSNotFound;
        if (nil != rlm && nil != rlm->_timers) {
            idx = RSArrayIndexOfObject(rlm->_timers, rlt);
        }
        if (RSNotFound != idx) {
            __RSRunLoopTimerLock(rlt);
            RSSetRemoveValue(rlt->_rlModes, rlm->_name);
            if (0 == RSSetGetCount(rlt->_rlModes)) {
                rlt->_runLoop = nil;
            }
            __RSRunLoopTimerUnlock(rlt);
            RSArrayRemoveObjectAtIndex(rlm->_timers, idx);
            if (0 == RSArrayGetCount(rlm->_timers)) {
                AbsoluteTime dummy;
                mk_timer_cancel(rlm->_timerPort, &dummy);
            } else if (0 == idx) {
                __RSArmNextTimerInMode(rlm);
            }
        }
        if (nil != rlm) {
            __RSRunLoopModeUnlock(rlm);
        }
    }
    __RSRunLoopUnlock(rl);
}

/* RSRunLoopSource */

static BOOL __RSRunLoopSourceClassEqual(RSTypeRef rs1, RSTypeRef rs2) {	/* DOES CALLOUT */
    RSRunLoopSourceRef rls1 = (RSRunLoopSourceRef)rs1;
    RSRunLoopSourceRef rls2 = (RSRunLoopSourceRef)rs2;
    if (rls1 == rls2) return YES;
    if (__RSRunLoopIsValid(rls1) != __RSRunLoopIsValid(rls2)) return NO;
    if (rls1->_order != rls2->_order) return NO;
    if (rls1->_context.version0.version != rls2->_context.version0.version) return NO;
    if (rls1->_context.version0.hash != rls2->_context.version0.hash) return NO;
    if (rls1->_context.version0.equal != rls2->_context.version0.equal) return NO;
    if (0 == rls1->_context.version0.version && rls1->_context.version0.perform != rls2->_context.version0.perform) return NO;
    if (1 == rls1->_context.version0.version && rls1->_context.version1.perform != rls2->_context.version1.perform) return NO;
    if (rls1->_context.version0.equal)
        return rls1->_context.version0.equal(rls1->_context.version0.info, rls2->_context.version0.info);
    return (rls1->_context.version0.info == rls2->_context.version0.info);
}

static RSHashCode __RSRunLoopSourceClassHash(RSTypeRef rs) {	/* DOES CALLOUT */
    RSRunLoopSourceRef rls = (RSRunLoopSourceRef)rs;
    if (rls->_context.version0.hash)
        return rls->_context.version0.hash(rls->_context.version0.info);
    return (RSHashCode)rls->_context.version0.info;
}

static RSStringRef __RSRunLoopSourceClassDescription(RSTypeRef rs) {	/* DOES CALLOUT */
    RSRunLoopSourceRef rls = (RSRunLoopSourceRef)rs;
    RSStringRef result;
    RSStringRef contextDesc = nil;
    if (nil != rls->_context.version0.description) {
        contextDesc = rls->_context.version0.description(rls->_context.version0.info);
    }
    if (nil == contextDesc) {
        void *addr = rls->_context.version0.version == 0 ? (void *)rls->_context.version0.perform : (rls->_context.version0.version == 1 ? (void *)rls->_context.version1.perform : nil);
#if DEPLOYMENT_TARGET_WINDOWS
        contextDesc = RSStringCreateWithFormat(RSAllocatorSystemDefault, nil, RSSTR("<RSRunLoopSource context>{version = %ld, info = %p, callout = %p}"), rls->_context.version0.version, rls->_context.version0.info, addr);
#elif DEPLOYMENT_TARGET_MACOSX || DEPLOYMENT_TARGET_EMBEDDED || DEPLOYMENT_TARGET_EMBEDDED_MINI
        Dl_info info;
        const char *name = (dladdr(addr, &info) && info.dli_saddr == addr && info.dli_sname) ? info.dli_sname : "???";
        contextDesc = RSStringCreateWithFormat(RSAllocatorSystemDefault, nil, RSSTR("<RSRunLoopSource context>{version = %ld, info = %p, callout = %s (%p)}"), rls->_context.version0.version, rls->_context.version0.info, name, addr);
#endif
    }
#if DEPLOYMENT_TARGET_WINDOWS
    result = RSStringCreateWithFormat(RSAllocatorSystemDefault, nil, RSSTR("<RSRunLoopSource %p [%p]>{signalled = %s, valid = %s, order = %d, context = %r}"), rs, RSGetAllocator(rls), __RSRunLoopSourceIsSignaled(rls) ? "Yes" : "No", __RSRunLoopIsValid(rls) ? "Yes" : "No", rls->_order, contextDesc);
#else
    result = RSStringCreateWithFormat(RSAllocatorSystemDefault, nil, RSSTR("<RSRunLoopSource %p [%p]>{signalled = %s, valid = %s, order = %d, context = %r}"), rs, RSGetAllocator(rls), __RSRunLoopSourceIsSignaled(rls) ? "Yes" : "No", __RSRunLoopIsValid(rls) ? "Yes" : "No", rls->_order, contextDesc);
#endif
    RSRelease(contextDesc);
    return result;
}

void RSRunLoopSourceInvalidate(RSRunLoopSourceRef rls);

static void __RSRunLoopSourceClassDeallocate(RSTypeRef rs) {	/* DOES CALLOUT */
    RSRunLoopSourceRef rls = (RSRunLoopSourceRef)rs;
    RSRunLoopSourceInvalidate(rls);
    if (rls->_context.version0.release) {
        rls->_context.version0.release(rls->_context.version0.info);
    }
    pthread_mutex_destroy(&rls->_lock);
    memset((char *)rs + sizeof(RSRuntimeBase), 0, sizeof(struct __RSRunLoopSource) - sizeof(RSRuntimeBase));
}

static const RSRuntimeClass __RSRunLoopSourceClass = {
    _RSRuntimeScannedObject,
    "RSRunLoopSource",
    nil,      // init
    nil,      // copy
    __RSRunLoopSourceClassDeallocate,
    __RSRunLoopSourceClassEqual,
    __RSRunLoopSourceClassHash,
    __RSRunLoopSourceClassDescription,
    nil,
    nil
};

RSExport void __RSRunLoopSourceInitialize(void) {
    __RSRunLoopSourceTypeID = __RSRuntimeRegisterClass(&__RSRunLoopSourceClass);
    __RSRuntimeSetClassTypeID(&__RSRunLoopSourceClass, __RSRunLoopSourceTypeID);
}

RSTypeID RSRunLoopSourceGetTypeID(void) {
    return __RSRunLoopSourceTypeID;
}

RSRunLoopSourceRef RSRunLoopSourceCreate(RSAllocatorRef allocator, RSIndex order, RSRunLoopSourceContext *context) {
    CHECK_FOR_FORK();
    RSRunLoopSourceRef memory;
    uint32_t size;
    if (nil == context) {
        //CRSetCrashLogMessage("*** nil context value passed to RSRunLoopSourceCreate(). ***");
        HALT;
    }
    size = sizeof(struct __RSRunLoopSource) - sizeof(RSRuntimeBase);
    memory = (RSRunLoopSourceRef)__RSRuntimeCreateInstance(allocator, __RSRunLoopSourceTypeID, size);
    if (nil == memory) {
        return nil;
    }
    __RSRunLoopSetValid(memory);
    __RSRunLoopSourceUnsetSignaled(memory);
    __RSRunLoopLockInit(&memory->_lock);
    memory->_bits = 0;
    memory->_order = order;
    memory->_runLoops = nil;
    size = 0;
    switch (context->version) {
        case 0:
            size = sizeof(RSRunLoopSourceContext);
            break;
        case 1:
            size = sizeof(RSRunLoopSourceContext1);
            break;
    }
    memmove(&memory->_context, context, size);
    if (context->retain) {
        memory->_context.version0.info = (void *)context->retain(context->info);
    }
    return memory;
}

RSIndex RSRunLoopSourceGetOrder(RSRunLoopSourceRef rls) {
    CHECK_FOR_FORK();
    __RSGenericValidInstance(rls, __RSRunLoopSourceTypeID);
    return rls->_order;
}

static void __RSRunLoopSourceWakeUpLoop(const void *value, void *context) {
    RSRunLoopWakeUp((RSRunLoopRef)value);
}

static void __RSRunLoopSourceRemoveFromRunLoop(const void *value, void *context) {
    RSRunLoopRef rl = (RSRunLoopRef)value;
    RSTypeRef *params = (RSTypeRef *)context;
    RSRunLoopSourceRef rls = (RSRunLoopSourceRef)params[0];
    RSIndex idx;
    if (rl == params[1]) return;
    
    // RSRunLoopRemoveSource will lock the run loop while it
    // needs that, but we also lock it out here to keep
    // changes from occurring for this whole sequence.
    __RSRunLoopLock(rl);
    RSArrayRef array = RSRunLoopCopyAllModes(rl);
    for (idx = RSArrayGetCount(array); idx--;) {
        RSStringRef modeName = (RSStringRef)RSArrayObjectAtIndex(array, idx);
        RSRunLoopRemoveSource(rl, rls, modeName);
    }
    RSRunLoopRemoveSource(rl, rls, RSRunLoopCommonModes);
    __RSRunLoopUnlock(rl);
    RSRelease(array);
    params[1] = rl;
}

void RSRunLoopSourceInvalidate(RSRunLoopSourceRef rls) {
    CHECK_FOR_FORK();
    __RSGenericValidInstance(rls, __RSRunLoopSourceTypeID);
    __RSRunLoopSourceLock(rls);
    RSRetain(rls);
    if (__RSRunLoopIsValid(rls)) {
        RSBagRef rloops = rls->_runLoops;
        __RSRunLoopUnsetValid(rls);
        __RSRunLoopSourceUnsetSignaled(rls);
        if (nil != rloops) {
            // To avoid A->B, B->A lock ordering issues when coming up
            // towards the run loop from a source, the source has to be
            // unlocked, which means we have to protect from object
            // invalidation.
            rls->_runLoops = nil; // transfer ownership to local stack
            __RSRunLoopSourceUnlock(rls);
            RSTypeRef params[2] = {rls, nil};
            RSBagApplyFunction(rloops, (__RSRunLoopSourceRemoveFromRunLoop), params);
            RSRelease(rloops);
            __RSRunLoopSourceLock(rls);
        }
        /* for hashing- and equality-use purposes, can't actually release the context here */
    }
    __RSRunLoopSourceUnlock(rls);
    RSRelease(rls);
}

BOOL RSRunLoopSourceIsValid(RSRunLoopSourceRef rls) {
    CHECK_FOR_FORK();
    __RSGenericValidInstance(rls, __RSRunLoopSourceTypeID);
    return __RSRunLoopIsValid(rls);
}

void RSRunLoopSourceGetContext(RSRunLoopSourceRef rls, RSRunLoopSourceContext *context) {
    CHECK_FOR_FORK();
    __RSGenericValidInstance(rls, __RSRunLoopSourceTypeID);
    RSAssert1(0 == context->version || 1 == context->version, __RSLogAssertion, "%s(): context version not initialized to 0 or 1", __PRETTY_FUNCTION__);
    RSIndex size = 0;
    switch (context->version) {
        case 0:
            size = sizeof(RSRunLoopSourceContext);
            break;
        case 1:
            size = sizeof(RSRunLoopSourceContext1);
            break;
    }
    memmove(context, &rls->_context, size);
}

void RSRunLoopSourceSignal(RSRunLoopSourceRef rls) {
    CHECK_FOR_FORK();
    __RSRunLoopSourceLock(rls);
    if (__RSRunLoopIsValid(rls)) {
        __RSRunLoopSourceSetSignaled(rls);
    }
    __RSRunLoopSourceUnlock(rls);
}

BOOL RSRunLoopSourceIsSignalled(RSRunLoopSourceRef rls) {
    CHECK_FOR_FORK();
    __RSRunLoopSourceLock(rls);
    BOOL ret = __RSRunLoopSourceIsSignaled(rls) ? YES : NO;
    __RSRunLoopSourceUnlock(rls);
    return ret;
}

RSExport void _RSRunLoopSourceWakeUpRunLoops(RSRunLoopSourceRef rls) {
    RSBagRef loops = nil;
    __RSRunLoopSourceLock(rls);
    if (__RSRunLoopIsValid(rls) && nil != rls->_runLoops) {
        loops = RSBagCreateCopy(RSAllocatorSystemDefault, rls->_runLoops);
    }
    __RSRunLoopSourceUnlock(rls);
    if (loops) {
        RSBagApplyFunction(loops, __RSRunLoopSourceWakeUpLoop, nil);
        RSRelease(loops);
    }
}

/* RSRunLoopObserver */

static RSStringRef __RSRunLoopObserverClassDescription(RSTypeRef rs) {	/* DOES CALLOUT */
    RSRunLoopObserverRef rlo = (RSRunLoopObserverRef)rs;
    RSStringRef result;
    RSStringRef contextDesc = nil;
    if (nil != rlo->_context.copyDescription) {
        contextDesc = rlo->_context.copyDescription(rlo->_context.info);
    }
    if (!contextDesc) {
        contextDesc = RSStringCreateWithFormat(RSAllocatorSystemDefault, nil, RSSTR("<RSRunLoopObserver context %p>"), rlo->_context.info);
    }
#if DEPLOYMENT_TARGET_WINDOWS
    result = RSStringCreateWithFormat(RSAllocatorSystemDefault, nil, RSSTR("<RSRunLoopObserver %p [%p]>{valid = %s, activities = 0x%x, repeats = %s, order = %d, callout = %p, context = %r}"), rs, RSGetAllocator(rlo), __RSRunLoopIsValid(rlo) ? "Yes" : "No", rlo->_activities, __RSRunLoopObserverRepeats(rlo) ? "Yes" : "No", rlo->_order, rlo->_callout, contextDesc);
#elif DEPLOYMENT_TARGET_MACOSX || DEPLOYMENT_TARGET_EMBEDDED || DEPLOYMENT_TARGET_EMBEDDED_MINI
    void *addr = rlo->_callout;
    Dl_info info;
    const char *name = (dladdr(addr, &info) && info.dli_saddr == addr && info.dli_sname) ? info.dli_sname : "???";
    result = RSStringCreateWithFormat(RSAllocatorSystemDefault, nil, RSSTR("<RSRunLoopObserver %p [%p]>{valid = %s, activities = 0x%x, repeats = %s, order = %d, callout = %s (%p), context = %r}"), rs, RSGetAllocator(rlo), __RSRunLoopIsValid(rlo) ? "Yes" : "No", rlo->_activities, __RSRunLoopObserverRepeats(rlo) ? "Yes" : "No", rlo->_order, name, addr, contextDesc);
#endif
    RSRelease(contextDesc);
    return result;
}

static void __RSRunLoopObserverClassDeallocate(RSTypeRef rs) {	/* DOES CALLOUT */
    RSRunLoopObserverRef rlo = (RSRunLoopObserverRef)rs;
    RSRunLoopObserverInvalidate(rlo);
    pthread_mutex_destroy(&rlo->_lock);
}

static const RSRuntimeClass __RSRunLoopObserverClass = {
    _RSRuntimeScannedObject,
    "RSRunLoopObserver",
    nil,      // init
    nil,      // copy
    __RSRunLoopObserverClassDeallocate,
    nil,
    nil,
    __RSRunLoopObserverClassDescription,
    nil,
    nil
};

RSExport void __RSRunLoopObserverInitialize(void) {
    __RSRunLoopObserverTypeID = __RSRuntimeRegisterClass(&__RSRunLoopObserverClass);
    __RSRuntimeSetClassTypeID(&__RSRunLoopObserverClass, __RSRunLoopObserverTypeID);
}

RSTypeID RSRunLoopObserverGetTypeID(void) {
    return __RSRunLoopObserverTypeID;
}

RSRunLoopObserverRef RSRunLoopObserverCreate(RSAllocatorRef allocator, RSOptionFlags activities, BOOL repeats, RSIndex order, RSRunLoopObserverCallBack callout, RSRunLoopObserverContext *context) {
    CHECK_FOR_FORK();
    RSRunLoopObserverRef memory;
    UInt32 size;
    size = sizeof(struct __RSRunLoopObserver) - sizeof(RSRuntimeBase);
    memory = (RSRunLoopObserverRef)__RSRuntimeCreateInstance(allocator, __RSRunLoopObserverTypeID, size);
    if (nil == memory) {
        return nil;
    }
    __RSRunLoopSetValid(memory);
    __RSRunLoopObserverUnsetFiring(memory);
    if (repeats) {
        __RSRunLoopObserverSetRepeats(memory);
    } else {
        __RSRunLoopObserverUnsetRepeats(memory);
    }
    __RSRunLoopLockInit(&memory->_lock);
    memory->_runLoop = nil;
    memory->_rlCount = 0;
    memory->_activities = activities;
    memory->_order = order;
    memory->_callout = callout;
    if (context) {
        if (context->retain) {
            memory->_context.info = (void *)context->retain(context->info);
        } else {
            memory->_context.info = context->info;
        }
        memory->_context.retain = context->retain;
        memory->_context.release = context->release;
        memory->_context.copyDescription = context->copyDescription;
    } else {
        memory->_context.info = 0;
        memory->_context.retain = 0;
        memory->_context.release = 0;
        memory->_context.copyDescription = 0;
    }
    return memory;
}

static void _runLoopObserverWithBlockContext(RSRunLoopObserverRef observer, RSRunLoopActivity activity, void *opaqueBlock) {
    typedef void (^observer_block_t) (RSRunLoopObserverRef observer, RSRunLoopActivity activity);
    observer_block_t block = (observer_block_t)opaqueBlock;
    block(observer, activity);
}

RSRunLoopObserverRef RSRunLoopObserverCreateWithHandler(RSAllocatorRef allocator, RSOptionFlags activities, BOOL repeats, RSIndex order,
                                                        void (^block) (RSRunLoopObserverRef observer, RSRunLoopActivity activity)) {
    RSRunLoopObserverContext blockContext;
    blockContext.version = 0;
    blockContext.info = (void *)block;
    blockContext.retain = (const void *(*)(const void *info))_Block_copy;
    blockContext.release = (void (*)(const void *info))_Block_release;
    blockContext.copyDescription = nil;
    return RSRunLoopObserverCreate(allocator, activities, repeats, order, _runLoopObserverWithBlockContext, &blockContext);
}

RSOptionFlags RSRunLoopObserverGetActivities(RSRunLoopObserverRef rlo) {
    CHECK_FOR_FORK();
    __RSGenericValidInstance(rlo, __RSRunLoopObserverTypeID);
    return rlo->_activities;
}

RSIndex RSRunLoopObserverGetOrder(RSRunLoopObserverRef rlo) {
    CHECK_FOR_FORK();
    __RSGenericValidInstance(rlo, __RSRunLoopObserverTypeID);
    return rlo->_order;
}

BOOL RSRunLoopObserverDoesRepeat(RSRunLoopObserverRef rlo) {
    CHECK_FOR_FORK();
    __RSGenericValidInstance(rlo, __RSRunLoopObserverTypeID);
    return __RSRunLoopObserverRepeats(rlo);
}

void RSRunLoopObserverInvalidate(RSRunLoopObserverRef rlo) {    /* DOES CALLOUT */
    CHECK_FOR_FORK();
    __RSGenericValidInstance(rlo, __RSRunLoopObserverTypeID);
    __RSRunLoopObserverLock(rlo);
    RSRetain(rlo);
    if (__RSRunLoopIsValid(rlo)) {
        RSRunLoopRef rl = rlo->_runLoop;
        void *info = rlo->_context.info;
        rlo->_context.info = nil;
        __RSRunLoopUnsetValid(rlo);
        if (nil != rl) {
            // To avoid A->B, B->A lock ordering issues when coming up
            // towards the run loop from an observer, it has to be
            // unlocked, which means we have to protect from object
            // invalidation.
            RSRetain(rl);
            __RSRunLoopObserverUnlock(rlo);
            // RSRunLoopRemoveObserver will lock the run loop while it
            // needs that, but we also lock it out here to keep
            // changes from occurring for this whole sequence.
            __RSRunLoopLock(rl);
            RSArrayRef array = RSRunLoopCopyAllModes(rl);
            for (RSIndex idx = RSArrayGetCount(array); idx--;) {
                RSStringRef modeName = (RSStringRef)RSArrayObjectAtIndex(array, idx);
                RSRunLoopRemoveObserver(rl, rlo, modeName);
            }
            RSRunLoopRemoveObserver(rl, rlo, RSRunLoopCommonModes);
            __RSRunLoopUnlock(rl);
            RSRelease(array);
            RSRelease(rl);
            __RSRunLoopObserverLock(rlo);
        }
        if (nil != rlo->_context.release) {
            rlo->_context.release(info);        /* CALLOUT */
        }
    }
    __RSRunLoopObserverUnlock(rlo);
    RSRelease(rlo);
}

BOOL RSRunLoopObserverIsValid(RSRunLoopObserverRef rlo) {
    CHECK_FOR_FORK();
    return __RSRunLoopIsValid(rlo);
}

void RSRunLoopObserverGetContext(RSRunLoopObserverRef rlo, RSRunLoopObserverContext *context) {
    CHECK_FOR_FORK();
    __RSGenericValidInstance(rlo, __RSRunLoopObserverTypeID);
    RSAssert1(0 == context->version, __RSLogAssertion, "%s(): context version not initialized to 0", __PRETTY_FUNCTION__);
    *context = rlo->_context;
}

/* RSRunLoopTimer */

static RSStringRef __RSRunLoopTimerClassDescription(RSTypeRef rs) {	/* DOES CALLOUT */
    RSRunLoopTimerRef rlt = (RSRunLoopTimerRef)rs;
    RSStringRef contextDesc = nil;
    if (nil != rlt->_context.copyDescription) {
        contextDesc = rlt->_context.copyDescription(rlt->_context.info);
    }
    if (nil == contextDesc) {
        contextDesc = RSStringCreateWithFormat(RSAllocatorSystemDefault, nil, RSSTR("<RSRunLoopTimer context %p>"), rlt->_context.info);
    }
    void *addr = (void *)rlt->_callout;
    const char *name = "???";
#if DEPLOYMENT_TARGET_MACOSX || DEPLOYMENT_TARGET_EMBEDDED || DEPLOYMENT_TARGET_EMBEDDED_MINI
    Dl_info info;
    name = (dladdr(addr, &info) && info.dli_saddr == addr && info.dli_sname) ? info.dli_sname : "???";
#endif
    RSStringRef result = RSStringCreateWithFormat(RSAllocatorSystemDefault, nil, RSSTR("<RSRunLoopTimer %p [%p]>{valid = %s, firing = %s, interval = %0.09g, next fire date = %0.09g, callout = %s (%p), context = %r}"), rs, RSGetAllocator(rlt), __RSRunLoopIsValid(rlt) ? "Yes" : "No", __RSRunLoopTimerIsFiring(rlt) ? "Yes" : "No", rlt->_interval, rlt->_nextFireDate, name, addr, contextDesc);
    RSRelease(contextDesc);
    return result;
}

static void __RSRunLoopTimerClassDeallocate(RSTypeRef rs) {	/* DOES CALLOUT */
    //RSLog(6, RSSTR("__RSRunLoopTimerDeallocate(%p)"), rs);
    RSRunLoopTimerRef rlt = (RSRunLoopTimerRef)rs;
    __RSRunLoopTimerSetDeallocating(rlt);
    RSRunLoopTimerInvalidate(rlt);	/* DOES CALLOUT */
    RSRelease(rlt->_rlModes);
    rlt->_rlModes = nil;
    pthread_mutex_destroy(&rlt->_lock);
}

static const RSRuntimeClass __RSRunLoopTimerClass = {
    _RSRuntimeScannedObject,
    "RSRunLoopTimer",
    nil,      // init
    nil,      // copy
    __RSRunLoopTimerClassDeallocate,
    nil,	// equal
    nil,
    __RSRunLoopTimerClassDescription,
    nil,
    nil
};

RSExport void __RSRunLoopTimerInitialize(void) {
    __RSRunLoopTimerTypeID = __RSRuntimeRegisterClass(&__RSRunLoopTimerClass);
    __RSRuntimeSetClassTypeID(&__RSRunLoopTimerClass, __RSRunLoopTimerTypeID);
}

RSTypeID RSRunLoopTimerGetTypeID(void) {
    return __RSRunLoopTimerTypeID;
}

RSRunLoopTimerRef RSRunLoopTimerCreate(RSAllocatorRef allocator, RSAbsoluteTime fireDate, RSTimeInterval interval, RSOptionFlags flags, RSIndex order, RSRunLoopTimerCallBack callout, RSRunLoopTimerContext *context) {
    CHECK_FOR_FORK();
    RSRunLoopTimerRef memory;
    UInt32 size;
    size = sizeof(struct __RSRunLoopTimer) - sizeof(RSRuntimeBase);
    memory = (RSRunLoopTimerRef)__RSRuntimeCreateInstance(allocator, __RSRunLoopTimerTypeID, size);
    if (nil == memory) {
        return nil;
    }
    __RSRunLoopSetValid(memory);
    __RSRunLoopTimerUnsetFiring(memory);
    __RSRunLoopLockInit(&memory->_lock);
    memory->_runLoop = nil;
    memory->_rlModes = RSSetCreateMutable(RSAllocatorSystemDefault, 0, &RSTypeSetCallBacks);
    memory->_order = order;
    if (interval < 0.0) interval = 0.0;
    memory->_interval = interval;
    if (TIMER_DATE_LIMIT < fireDate) fireDate = TIMER_DATE_LIMIT;
    memory->_nextFireDate = fireDate;
    memory->_fireTSR = 0LL;
    int64_t now2 = (int64_t)mach_absolute_time();
    RSAbsoluteTime now1 = RSAbsoluteTimeGetCurrent();
    if (fireDate < now1) {
        memory->_fireTSR = now2;
    } else if (TIMER_INTERVAL_LIMIT < fireDate - now1) {
        memory->_fireTSR = now2 + __RSTimeIntervalToTSR(TIMER_INTERVAL_LIMIT);
    } else {
        memory->_fireTSR = now2 + __RSTimeIntervalToTSR(fireDate - now1);
    }
    memory->_callout = callout;
    if (nil != context) {
        if (context->retain) {
            memory->_context.info = (void *)context->retain(context->info);
        } else {
            memory->_context.info = context->info;
        }
        memory->_context.retain = context->retain;
        memory->_context.release = context->release;
        memory->_context.copyDescription = context->copyDescription;
    } else {
        memory->_context.info = 0;
        memory->_context.retain = 0;
        memory->_context.release = 0;
        memory->_context.copyDescription = 0;
    }
    return memory;
}

static void _runLoopTimerWithBlockContext(RSRunLoopTimerRef timer, void *opaqueBlock) {
    typedef void (^timer_block_t) (RSRunLoopTimerRef timer);
    timer_block_t block = (timer_block_t)opaqueBlock;
    block(timer);
}

RSRunLoopTimerRef RSRunLoopTimerCreateWithHandler(RSAllocatorRef allocator, RSAbsoluteTime fireDate, RSTimeInterval interval, RSOptionFlags flags, RSIndex order,
                                                  void (^block) (RSRunLoopTimerRef timer)) {
    
    RSRunLoopTimerContext blockContext;
    blockContext.version = 0;
    blockContext.info = (void *)block;
    blockContext.retain = (const void *(*)(const void *info))_Block_copy;
    blockContext.release = (void (*)(const void *info))_Block_release;
    blockContext.copyDescription = nil;
    return RSRunLoopTimerCreate(allocator, fireDate, interval, flags, order, _runLoopTimerWithBlockContext, &blockContext);
}

RSAbsoluteTime RSRunLoopTimerGetNextFireDate(RSRunLoopTimerRef rlt) {
    CHECK_FOR_FORK();
    RS_OBJC_FUNCDISPATCHV(__RSRunLoopTimerTypeID, RSAbsoluteTime, (NSTimer *)rlt, _rsfireTime);
    __RSGenericValidInstance(rlt, __RSRunLoopTimerTypeID);
    RSAbsoluteTime at = 0.0;
    __RSRunLoopTimerLock(rlt);
    __RSRunLoopTimerFireTSRLock();
    if (__RSRunLoopIsValid(rlt)) {
        at = rlt->_nextFireDate;
    }
    __RSRunLoopTimerFireTSRUnlock();
    __RSRunLoopTimerUnlock(rlt);
    return at;
}

void RSRunLoopTimerSetNextFireDate(RSRunLoopTimerRef rlt, RSAbsoluteTime fireDate) {
    CHECK_FOR_FORK();
    if (!__RSRunLoopIsValid(rlt)) return;
    if (TIMER_DATE_LIMIT < fireDate) fireDate = TIMER_DATE_LIMIT;
    int64_t nextFireTSR = 0LL;
    int64_t now2 = (int64_t)mach_absolute_time();
    RSAbsoluteTime now1 = RSAbsoluteTimeGetCurrent();
    if (fireDate < now1) {
        nextFireTSR = now2;
    } else if (TIMER_INTERVAL_LIMIT < fireDate - now1) {
        nextFireTSR = now2 + __RSTimeIntervalToTSR(TIMER_INTERVAL_LIMIT);
    } else {
        nextFireTSR = now2 + __RSTimeIntervalToTSR(fireDate - now1);
    }
    __RSRunLoopTimerLock(rlt);
    if (nil != rlt->_runLoop) {
        RSIndex cnt = RSSetGetCount(rlt->_rlModes);
        STACK_BUFFER_DECL(RSTypeRef, modes, cnt);
        RSSetGetValues(rlt->_rlModes, (const void **)modes);
        // To avoid A->B, B->A lock ordering issues when coming up
        // towards the run loop from a source, the timer has to be
        // unlocked, which means we have to protect from object
        // invalidation, although that's somewhat expensive.
        for (RSIndex idx = 0; idx < cnt; idx++) {
            RSRetain(modes[idx]);
        }
        RSRunLoopRef rl = (RSRunLoopRef)RSRetain(rlt->_runLoop);
        __RSRunLoopTimerUnlock(rlt);
        __RSRunLoopLock(rl);
        for (RSIndex idx = 0; idx < cnt; idx++) {
            RSStringRef name = (RSStringRef)modes[idx];
            modes[idx] = __RSRunLoopFindMode(rl, name, NO);
            RSRelease(name);
        }
        __RSRunLoopTimerFireTSRLock();
        rlt->_fireTSR = nextFireTSR;
        rlt->_nextFireDate = fireDate;
        for (RSIndex idx = 0; idx < cnt; idx++) {
            RSRunLoopModeRef rlm = (RSRunLoopModeRef)modes[idx];
            if (rlm) {
                __RSRepositionTimerInMode(rlm, rlt, YES);
            }
        }
        __RSRunLoopTimerFireTSRUnlock();
        for (RSIndex idx = 0; idx < cnt; idx++) {
            __RSRunLoopModeUnlock((RSRunLoopModeRef)modes[idx]);
        }
        __RSRunLoopUnlock(rl);
        // This is setting the date of a timer, not a direct
        // interaction with a run loop, so we'll do a wakeup
        // (which may be costly) for the caller, just in case.
        // (And useful for binary compatibility with older
        // code used to the older timer implementation.)
        if (rl != RSRunLoopGetCurrent()) RSRunLoopWakeUp(rl);
        RSRelease(rl);
    } else {
        __RSRunLoopTimerFireTSRLock();
        rlt->_fireTSR = nextFireTSR;
        rlt->_nextFireDate = fireDate;
        __RSRunLoopTimerFireTSRUnlock();
        __RSRunLoopTimerUnlock(rlt);
    }
}

RSTimeInterval RSRunLoopTimerGetInterval(RSRunLoopTimerRef rlt) {
    CHECK_FOR_FORK();
    RS_OBJC_FUNCDISPATCHV(__RSRunLoopTimerTypeID, RSTimeInterval, (NSTimer *)rlt, timeInterval);
    __RSGenericValidInstance(rlt, __RSRunLoopTimerTypeID);
    return rlt->_interval;
}

BOOL RSRunLoopTimerDoesRepeat(RSRunLoopTimerRef rlt) {
    CHECK_FOR_FORK();
    __RSGenericValidInstance(rlt, __RSRunLoopTimerTypeID);
    return (0.0 < rlt->_interval);
}

RSIndex RSRunLoopTimerGetOrder(RSRunLoopTimerRef rlt) {
    CHECK_FOR_FORK();
    __RSGenericValidInstance(rlt, __RSRunLoopTimerTypeID);
    return rlt->_order;
}

void RSRunLoopTimerInvalidate(RSRunLoopTimerRef rlt) {	/* DOES CALLOUT */
    CHECK_FOR_FORK();
    RS_OBJC_FUNCDISPATCHV(__RSRunLoopTimerTypeID, void, (NSTimer *)rlt, invalidate);
    __RSGenericValidInstance(rlt, __RSRunLoopTimerTypeID);
    __RSRunLoopTimerLock(rlt);
    if (!__RSRunLoopTimerIsDeallocating(rlt)) {
        RSRetain(rlt);
    }
    if (__RSRunLoopIsValid(rlt)) {
        RSRunLoopRef rl = rlt->_runLoop;
        void *info = rlt->_context.info;
        rlt->_context.info = nil;
        __RSRunLoopUnsetValid(rlt);
        if (nil != rl) {
            RSIndex cnt = RSSetGetCount(rlt->_rlModes);
            STACK_BUFFER_DECL(RSStringRef, modes, cnt);
            RSSetGetValues(rlt->_rlModes, (const void **)modes);
            // To avoid A->B, B->A lock ordering issues when coming up
            // towards the run loop from a source, the timer has to be
            // unlocked, which means we have to protect from object
            // invalidation, although that's somewhat expensive.
            for (RSIndex idx = 0; idx < cnt; idx++) {
                RSRetain(modes[idx]);
            }
            RSRetain(rl);
            __RSRunLoopTimerUnlock(rlt);
            // RSRunLoopRemoveTimer will lock the run loop while it
            // needs that, but we also lock it out here to keep
            // changes from occurring for this whole sequence.
            __RSRunLoopLock(rl);
            for (RSIndex idx = 0; idx < cnt; idx++) {
                RSRunLoopRemoveTimer(rl, rlt, modes[idx]);
            }
            RSRunLoopRemoveTimer(rl, rlt, RSRunLoopCommonModes);
            __RSRunLoopUnlock(rl);
            for (RSIndex idx = 0; idx < cnt; idx++) {
                RSRelease(modes[idx]);
            }
            RSRelease(rl);
            __RSRunLoopTimerLock(rlt);
        }
        if (nil != rlt->_context.release) {
            rlt->_context.release(info);	/* CALLOUT */
        }
    }
    __RSRunLoopTimerUnlock(rlt);
    if (!__RSRunLoopTimerIsDeallocating(rlt)) {
        RSRelease(rlt);
    }
}

BOOL RSRunLoopTimerIsValid(RSRunLoopTimerRef rlt) {
    CHECK_FOR_FORK();
    RS_OBJC_FUNCDISPATCHV(__RSRunLoopTimerTypeID, BOOL, (NSTimer *)rlt, isValid);
    __RSGenericValidInstance(rlt, __RSRunLoopTimerTypeID);
    return __RSRunLoopIsValid(rlt);
}

void RSRunLoopTimerGetContext(RSRunLoopTimerRef rlt, RSRunLoopTimerContext *context) {
    CHECK_FOR_FORK();
    __RSGenericValidInstance(rlt, __RSRunLoopTimerTypeID);
    RSAssert1(0 == context->version, __RSLogAssertion, "%s(): context version not initialized to 0", __PRETTY_FUNCTION__);
    *context = rlt->_context;
}
#elif (RSRunLoopVersion == 3)

#pragma mark -
#pragma mark Apple RunLoop Version 2013

extern RSTimeInterval __RSTimeIntervalUntilTSR(uint64_t tsr);
extern uint64_t __RSTSRToNanoseconds(uint64_t tsr);
extern dispatch_time_t __RSTSRToDispatchTime(uint64_t tsr);

/*
 * Copyright (c) 2013 Apple Inc. All rights reserved.
 *
 * @APPLE_LICENSE_HEADER_START@
 *
 * This file contains Original Code and/or Modifications of Original Code
 * as defined in and that are subject to the Apple Public Source License
 * Version 2.0 (the 'License'). You may not use this file except in
 * compliance with the License. Please obtain a copy of the License at
 * http://www.opensource.apple.com/apsl/ and read it before using this
 * file.
 *
 * The Original Code and all software distributed under the License are
 * distributed on an 'AS IS' basis, WITHOUT WARRANTY OF ANY KIND, EITHER
 * EXPRESS OR IMPLIED, AND APPLE HEREBY DISCLAIMS ALL SUCH WARRANTIES,
 * INCLUDING WITHOUT LIMITATION, ANY WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE, QUIET ENJOYMENT OR NON-INFRINGEMENT.
 * Please see the License for the specific language governing rights and
 * limitations under the License.
 *
 * @APPLE_LICENSE_HEADER_END@
 */

/*	RSRunLoop.c
 Copyright (c) 1998-2013, Apple Inc. All rights reserved.
 Responsibility: Tony Parker
 */

#include <RSCoreFoundation/RSRunLoop.h>
#include <RSCoreFoundation/RSSet.h>
#include <RSCoreFoundation/RSBag.h>
#include <RSCoreFoundation/RSNumber.h>
//#include <RSCoreFoundation/RSPreferences.h>
#include "RSInternal.h"
#include <math.h>
#include <stdio.h>
#include <limits.h>
#include <pthread.h>
#include <dispatch/dispatch.h>


#if DEPLOYMENT_TARGET_WINDOWS
#include <typeinfo.h>
#endif
#include <checkint.h>

#if DEPLOYMENT_TARGET_MACOSX || DEPLOYMENT_TARGET_EMBEDDED || DEPLOYMENT_TARGET_EMBEDDED_MINI
#include <sys/param.h>
//#include <dispatch/private.h>
//#include <RSCoreFoundation/RSUserNotification.h>
#include <mach/mach.h>
#include <mach/clock_types.h>
#include <mach/clock.h>
#include <unistd.h>
#include <dlfcn.h>
//#include <pthread/private.h>
extern dispatch_queue_t _dispatch_runloop_root_queue_create_4CF(const char *label, unsigned long flags);
extern mach_port_t _dispatch_get_main_queue_port_4CF(void);
extern mach_port_t _dispatch_runloop_root_queue_get_port_4CF(dispatch_queue_t);
extern void _dispatch_source_set_runloop_timer_4CF(dispatch_source_t source, dispatch_time_t start, uint64_t interval, uint64_t leeway);
extern void _dispatch_main_queue_callback_4CF(mach_msg_header_t *msg);
extern bool _dispatch_runloop_root_queue_perform_4CF(dispatch_queue_t queue);

#elif DEPLOYMENT_TARGET_WINDOWS
#include <process.h>
DISPATCH_EXPORT HANDLE _dispatch_get_main_queue_handle_4RS(void);
DISPATCH_EXPORT void _dispatch_main_queue_callback_4RS(void);

#define MACH_PORT_NULL 0
#define mach_port_name_t HANDLE
#define _dispatch_get_main_queue_port_4CF _dispatch_get_main_queue_handle_4CF
#define _dispatch_main_queue_callback_4CF(x) _dispatch_main_queue_callback_4CF()

#define AbsoluteTime LARGE_INTEGER

#endif
extern mach_port_t _dispatch_get_main_queue_port_4CF(void);
extern void _dispatch_main_queue_callback_4CF(mach_msg_header_t *msg);
extern pthread_t pthread_main_thread_np();
#elif DEPLOYMENT_TARGET_WINDOWS
#include <process.h>
DISPATCH_EXPORT HANDLE _dispatch_get_main_queue_handle_4CF(void);
DISPATCH_EXPORT void _dispatch_main_queue_callback_4CF(void);

#define MACH_PORT_NULL 0
#define mach_port_name_t HANDLE
#define mach_port_t HANDLE
#define _dispatch_get_main_queue_port_4CF _dispatch_get_main_queue_handle_4CF
#define _dispatch_main_queue_callback_4CF(x) _dispatch_main_queue_callback_4CF()

#define AbsoluteTime LARGE_INTEGER

#endif

#if DEPLOYMENT_TARGET_WINDOWS || DEPLOYMENT_TARGET_IPHONESIMULATOR
RS_EXPORT pthread_t _RS_pthread_main_thread_np(void);
#define pthread_main_thread_np() _RS_pthread_main_thread_np()
#endif

#include <Block.h>
//#include <Block_private.h>

#if DEPLOYMENT_TARGET_MACOSX
#define USE_DISPATCH_SOURCE_FOR_TIMERS 1
#define USE_MK_TIMER_TOO 1
#else
#define USE_DISPATCH_SOURCE_FOR_TIMERS 0
#define USE_MK_TIMER_TOO 1
#endif


static int _LogRSRunLoop = 0;
static void _runLoopTimerWithBlockContext(RSRunLoopTimerRef timer, void *opaqueBlock);

// for conservative arithmetic safety, such that (TIMER_DATE_LIMIT + TIMER_INTERVAL_LIMIT + RSAbsoluteTimeIntervalSince1970) * 10^9 < 2^63
#define TIMER_DATE_LIMIT	4039289856.0
#define TIMER_INTERVAL_LIMIT	504911232.0

#define HANDLE_DISPATCH_ON_BASE_INVOCATION_ONLY 0

#define CRASH(string, errcode) do { char msg[256]; snprintf(msg, 256, string, errcode); __RSCLog(RSLogLevelError, "%s\n", msg); __HALT(); } while (0)

#if DEPLOYMENT_TARGET_WINDOWS

static pthread_t kNilPthreadT = { nil, nil };
#define pthreadPointer(a) a.p
typedef	int kern_return_t;
#define KERN_SUCCESS 0

#else

static pthread_t NilPthreadT = (pthread_t)0;
#define pthreadPointer(a) a
#define lockCount(a) a
#endif

#pragma mark -

#define RS_RUN_LOOP_PROBES 0

#if RS_RUN_LOOP_PROBES
#include "RSRunLoopProbes.h"
#else
#define	RSRUNLOOP_NEXT_TIMER_ARMED(arg0) do { } while (0)
#define	RSRUNLOOP_NEXT_TIMER_ARMED_ENABLED() (0)
#define	RSRUNLOOP_POLL() do { } while (0)
#define	RSRUNLOOP_POLL_ENABLED() (0)
#define	RSRUNLOOP_SLEEP() do { } while (0)
#define	RSRUNLOOP_SLEEP_ENABLED() (0)
#define	RSRUNLOOP_SOURCE_FIRED(arg0, arg1, arg2) do { } while (0)
#define	RSRUNLOOP_SOURCE_FIRED_ENABLED() (0)
#define	RSRUNLOOP_TIMER_CREATED(arg0, arg1, arg2, arg3, arg4, arg5, arg6) do { } while (0)
#define	RSRUNLOOP_TIMER_CREATED_ENABLED() (0)
#define	RSRUNLOOP_TIMER_FIRED(arg0, arg1, arg2, arg3, arg4) do { } while (0)
#define	RSRUNLOOP_TIMER_FIRED_ENABLED() (0)
#define	RSRUNLOOP_TIMER_RESCHEDULED(arg0, arg1, arg2, arg3, arg4, arg5) do { } while (0)
#define	RSRUNLOOP_TIMER_RESCHEDULED_ENABLED() (0)
#define	RSRUNLOOP_WAKEUP(arg0) do { } while (0)
#define	RSRUNLOOP_WAKEUP_ENABLED() (0)
#define	RSRUNLOOP_WAKEUP_FOR_DISPATCH() do { } while (0)
#define	RSRUNLOOP_WAKEUP_FOR_DISPATCH_ENABLED() (0)
#define	RSRUNLOOP_WAKEUP_FOR_NOTHING() do { } while (0)
#define	RSRUNLOOP_WAKEUP_FOR_NOTHING_ENABLED() (0)
#define	RSRUNLOOP_WAKEUP_FOR_SOURCE() do { } while (0)
#define	RSRUNLOOP_WAKEUP_FOR_SOURCE_ENABLED() (0)
#define	RSRUNLOOP_WAKEUP_FOR_TIMEOUT() do { } while (0)
#define	RSRUNLOOP_WAKEUP_FOR_TIMEOUT_ENABLED() (0)
#define	RSRUNLOOP_WAKEUP_FOR_TIMER() do { } while (0)
#define	RSRUNLOOP_WAKEUP_FOR_TIMER_ENABLED() (0)
#define	RSRUNLOOP_WAKEUP_FOR_WAKEUP() do { } while (0)
#define	RSRUNLOOP_WAKEUP_FOR_WAKEUP_ENABLED() (0)
#endif

// In order to reuse most of the code across Mach and Windows v1 RunLoopSources, we define a
// simple abstraction layer spanning Mach ports and Windows HANDLES
#if DEPLOYMENT_TARGET_MACOSX || DEPLOYMENT_TARGET_EMBEDDED || DEPLOYMENT_TARGET_EMBEDDED_MINI

RSPrivate uint32_t __RSGetProcessPortCount(void) {
    ipc_info_space_t info;
    ipc_info_name_array_t table = 0;
    mach_msg_type_number_t tableCount = 0;
    ipc_info_tree_name_array_t tree = 0;
    mach_msg_type_number_t treeCount = 0;
    
    kern_return_t ret = mach_port_space_info(mach_task_self(), &info, &table, &tableCount, &tree, &treeCount);
    if (ret != KERN_SUCCESS) {
        return (uint32_t)0;
    }
    if (table != nil) {
        ret = vm_deallocate(mach_task_self(), (vm_address_t)table, tableCount * sizeof(*table));
    }
    if (tree != nil) {
        ret = vm_deallocate(mach_task_self(), (vm_address_t)tree, treeCount * sizeof(*tree));
    }
    return (uint32_t)tableCount;
}

RSPrivate RSArrayRef __RSStopAllThreads(void) {
    RSMutableArrayRef suspended_list = __RSArrayCreateMutable0(RSAllocatorSystemDefault, 0, nil);
    mach_port_t my_task = mach_task_self();
    mach_port_t my_thread = mach_thread_self();
    thread_act_array_t thr_list = 0;
    mach_msg_type_number_t thr_cnt = 0;
    
    // really, should loop doing the stopping until no more threads get added to the list N times in a row
    kern_return_t ret = task_threads(my_task, &thr_list, &thr_cnt);
    if (ret == KERN_SUCCESS) {
        for (RSIndex idx = 0; idx < thr_cnt; idx++) {
            thread_act_t thread = thr_list[idx];
            if (thread == my_thread) continue;
            if (RSArrayContainsObject(suspended_list, RSMakeRange(0, RSArrayGetCount(suspended_list)), (const void *)(uintptr_t)thread)) continue;
            ret = thread_suspend(thread);
            if (ret == KERN_SUCCESS) {
                RSArrayAddObject(suspended_list, (const void *)(uintptr_t)thread);
            } else {
                mach_port_deallocate(my_task, thread);
            }
        }
        vm_deallocate(my_task, (vm_address_t)thr_list, sizeof(thread_t) * thr_cnt);
    }
    mach_port_deallocate(my_task, my_thread);
    return suspended_list;
}

RSPrivate void __RSRestartAllThreads(RSArrayRef threads) {
    for (RSIndex idx = 0; idx < RSArrayGetCount(threads); idx++) {
        thread_act_t thread = (thread_act_t)(uintptr_t)RSArrayObjectAtIndex(threads, idx);
        kern_return_t ret = thread_resume(thread);
        if (ret != KERN_SUCCESS) CRASH("*** Failure from thread_resume (%d) ***", ret);
        mach_port_deallocate(mach_task_self(), thread);
    }
}

static uint32_t __RS_last_warned_port_count = 0;

static void foo() __attribute__((unused));
static void foo() {
    uint32_t pcnt = __RSGetProcessPortCount();
    if (__RS_last_warned_port_count + 1000 < pcnt) {
        RSArrayRef threads = __RSStopAllThreads();
        
        
        // do stuff here
//        RSOptionFlags responseFlags = 0;
//        SInt32 result = RSUserNotificationDisplayAlert(0.0, RSUserNotificationCautionAlertLevel, nil, nil, nil, RSSTR("High Mach Port Usage"), RSSTR("This application is using a lot of Mach ports."), RSSTR("Default"), RSSTR("Altern"), RSSTR("Other b"), &responseFlags);
//        if (0 != result) {
//            RSLog(3, RSSTR("ERROR"));
//        } else {
//            switch (responseFlags) {
//                case RSUserNotificationDefaultResponse: RSLog(3, RSSTR("DefaultR")); break;
//                case RSUserNotificationAlternateResponse: RSLog(3, RSSTR("AltR")); break;
//                case RSUserNotificationOtherResponse: RSLog(3, RSSTR("OtherR")); break;
//                case RSUserNotificationCancelResponse: RSLog(3, RSSTR("CancelR")); break;
//            }
//        }
        
        
        __RSRestartAllThreads(threads);
        RSRelease(threads);
        __RS_last_warned_port_count = pcnt;
    }
}


typedef mach_port_t __RSPort;
#define RSPORT_NULL MACH_PORT_NULL
typedef mach_port_t __RSPortSet;

static void __THE_SYSTEM_HAS_NO_PORTS_AVAILABLE__(kern_return_t ret) __attribute__((noinline));
static void __THE_SYSTEM_HAS_NO_PORTS_AVAILABLE__(kern_return_t ret) { HALT; };

static __RSPort __RSPortAllocate(void) {
    __RSPort result = RSPORT_NULL;
    kern_return_t ret = mach_port_allocate(mach_task_self(), MACH_PORT_RIGHT_RECEIVE, &result);
    if (KERN_SUCCESS != ret) {
        char msg[256];
        snprintf(msg, 256, "*** The system has no mach ports available. You may be able to diagnose which application(s) are using ports by using 'top' or Activity Monitor. (%d) ***", ret);
//        CRSetCrashLogMessage(msg);
        __RSCLog(RSLogLevelError, "%s\n", msg);
        __THE_SYSTEM_HAS_NO_PORTS_AVAILABLE__(ret);
        return RSPORT_NULL;
    }
    
    ret = mach_port_insert_right(mach_task_self(), result, result, MACH_MSG_TYPE_MAKE_SEND);
    if (KERN_SUCCESS != ret) CRASH("*** Unable to set send right on mach port. (%d) ***", ret);
    
    
    mach_port_limits_t limits;
    limits.mpl_qlimit = 1;
    ret = mach_port_set_attributes(mach_task_self(), result, MACH_PORT_LIMITS_INFO, (mach_port_info_t)&limits, MACH_PORT_LIMITS_INFO_COUNT);
    if (KERN_SUCCESS != ret) CRASH("*** Unable to set attributes on mach port. (%d) ***", ret);
    
    return result;
}

RSInline void __RSPortFree(__RSPort port) {
    mach_port_destroy(mach_task_self(), port);
}

static void __THE_SYSTEM_HAS_NO_PORT_SETS_AVAILABLE__(kern_return_t ret) __attribute__((noinline));
static void __THE_SYSTEM_HAS_NO_PORT_SETS_AVAILABLE__(kern_return_t ret) { HALT; };

RSInline __RSPortSet __RSPortSetAllocate(void) {
    __RSPortSet result;
    kern_return_t ret = mach_port_allocate(mach_task_self(), MACH_PORT_RIGHT_PORT_SET, &result);
    if (KERN_SUCCESS != ret) { __THE_SYSTEM_HAS_NO_PORT_SETS_AVAILABLE__(ret); }
    return (KERN_SUCCESS == ret) ? result : RSPORT_NULL;
}

RSInline kern_return_t __RSPortSetInsert(__RSPort port, __RSPortSet portSet) {
    if (MACH_PORT_NULL == port) {
        return -1;
    }
    return mach_port_insert_member(mach_task_self(), port, portSet);
}

RSInline kern_return_t __RSPortSetRemove(__RSPort port, __RSPortSet portSet) {
    if (MACH_PORT_NULL == port) {
        return -1;
    }
    return mach_port_extract_member(mach_task_self(), port, portSet);
}

RSInline void __RSPortSetFree(__RSPortSet portSet) {
    kern_return_t ret;
    mach_port_name_array_t array;
    mach_msg_type_number_t idx, number;
    
    ret = mach_port_get_set_status(mach_task_self(), portSet, &array, &number);
    if (KERN_SUCCESS == ret) {
        for (idx = 0; idx < number; idx++) {
            mach_port_extract_member(mach_task_self(), array[idx], portSet);
        }
        vm_deallocate(mach_task_self(), (vm_address_t)array, number * sizeof(mach_port_name_t));
    }
    mach_port_destroy(mach_task_self(), portSet);
}

#elif DEPLOYMENT_TARGET_WINDOWS

typedef HANDLE __RSPort;
#define RSPORT_nil nil

// A simple dynamic array of HANDLEs, which grows to a high-water mark
typedef struct ___RSPortSet {
    uint16_t	used;
    uint16_t	size;
    HANDLE	*handles;
    RSSpinLock_t lock;		// insert and remove must be thread safe, like the Mach calls
} *__RSPortSet;

RSInline __RSPort __RSPortAllocate(void) {
    return CreateEventA(nil, YES, NO, nil);
}

RSInline void __RSPortFree(__RSPort port) {
    CloseHandle(port);
}

static __RSPortSet __RSPortSetAllocate(void) {
    __RSPortSet result = (__RSPortSet)RSAllocatorAllocate(RSAllocatorSystemDefault, sizeof(struct ___RSPortSet), 0);
    result->used = 0;
    result->size = 4;
    result->handles = (HANDLE *)RSAllocatorAllocate(RSAllocatorSystemDefault, result->size * sizeof(HANDLE), 0);
    RS_SPINLOCK_INIT_FOR_STRUCTS(result->lock);
    return result;
}

static void __RSPortSetFree(__RSPortSet portSet) {
    RSAllocatorDeallocate(RSAllocatorSystemDefault, portSet->handles);
    RSAllocatorDeallocate(RSAllocatorSystemDefault, portSet);
}

// Returns portBuf if ports fit in that space, else returns another ptr that must be freed
static __RSPort *__RSPortSetGetPorts(__RSPortSet portSet, __RSPort *portBuf, uint32_t bufSize, uint32_t *portsUsed) {
    __RSSpinLock(&(portSet->lock));
    __RSPort *result = portBuf;
    if (bufSize < portSet->used)
        result = (__RSPort *)RSAllocatorAllocate(RSAllocatorSystemDefault, portSet->used * sizeof(HANDLE), 0);
    if (portSet->used > 1) {
        // rotate the ports to vaguely simulate round-robin behaviour
        uint16_t lastPort = portSet->used - 1;
        HANDLE swapHandle = portSet->handles[0];
        memmove(portSet->handles, &portSet->handles[1], lastPort * sizeof(HANDLE));
        portSet->handles[lastPort] = swapHandle;
    }
    memmove(result, portSet->handles, portSet->used * sizeof(HANDLE));
    *portsUsed = portSet->used;
    __RSSpinUnlock(&(portSet->lock));
    return result;
}

static kern_return_t __RSPortSetInsert(__RSPort port, __RSPortSet portSet) {
    if (nil == port) {
        return -1;
    }
    __RSSpinLock(&(portSet->lock));
    if (portSet->used >= portSet->size) {
        portSet->size += 4;
        portSet->handles = (HANDLE *)RSAllocatorReallocate(RSAllocatorSystemDefault, portSet->handles, portSet->size * sizeof(HANDLE), 0);
    }
    if (portSet->used >= MAXIMUM_WAIT_OBJECTS) {
        RSLog(RSLogLevelWarning, RSSTR("*** More than MAXIMUM_WAIT_OBJECTS (%d) ports add to a port set.  The last ones will be ignored."), MAXIMUM_WAIT_OBJECTS);
    }
    portSet->handles[portSet->used++] = port;
    __RSSpinUnlock(&(portSet->lock));
    return KERN_SUCCESS;
}

static kern_return_t __RSPortSetRemove(__RSPort port, __RSPortSet portSet) {
    int i, j;
    if (nil == port) {
        return -1;
    }
    __RSSpinLock(&(portSet->lock));
    for (i = 0; i < portSet->used; i++) {
        if (portSet->handles[i] == port) {
            for (j = i+1; j < portSet->used; j++) {
                portSet->handles[j-1] = portSet->handles[j];
            }
            portSet->used--;
            __RSSpinUnlock(&(portSet->lock));
            return YES;
        }
    }
    __RSSpinUnlock(&(portSet->lock));
    return KERN_SUCCESS;
}

#endif

#if !defined(__MACTYPES__) && !defined(_OS_OSTYPES_H)
#if defined(__BIG_ENDIAN__)
typedef	struct UnsignedWide {
    UInt32		hi;
    UInt32		lo;
} UnsignedWide;
#elif defined(__LITTLE_ENDIAN__)
typedef	struct UnsignedWide {
    UInt32		lo;
    UInt32		hi;
} UnsignedWide;
#endif
typedef UnsignedWide		AbsoluteTime;
#endif

#if DEPLOYMENT_TARGET_MACOSX || DEPLOYMENT_TARGET_EMBEDDED || DEPLOYMENT_TARGET_EMBEDDED_MINI

#if USE_DISPATCH_SOURCE_FOR_TIMERS
#endif
#if USE_MK_TIMER_TOO
extern mach_port_name_t mk_timer_create(void);
extern kern_return_t mk_timer_destroy(mach_port_name_t name);
extern kern_return_t mk_timer_arm(mach_port_name_t name, AbsoluteTime expire_time);
extern kern_return_t mk_timer_cancel(mach_port_name_t name, AbsoluteTime *result_time);

RSInline AbsoluteTime __RSUInt64ToAbsoluteTime(uint64_t x) {
    AbsoluteTime a;
    a.hi = x >> 32;
    a.lo = x & (uint64_t)0xFFFFFFFF;
    return a;
}
#endif

static uint32_t __RSSendTrivialMachMessage(mach_port_t port, uint32_t msg_id, RSOptionFlags options, uint32_t timeout) {
    kern_return_t result;
    mach_msg_header_t header;
    header.msgh_bits = MACH_MSGH_BITS(MACH_MSG_TYPE_COPY_SEND, 0);
    header.msgh_size = sizeof(mach_msg_header_t);
    header.msgh_remote_port = port;
    header.msgh_local_port = MACH_PORT_NULL;
    header.msgh_id = msg_id;
    result = mach_msg(&header, MACH_SEND_MSG|(mach_msg_option_t)options, header.msgh_size, 0, MACH_PORT_NULL, timeout, MACH_PORT_NULL);
    if (result == MACH_SEND_TIMED_OUT) mach_msg_destroy(&header);
    return result;
}
#elif DEPLOYMENT_TARGET_WINDOWS

static HANDLE mk_timer_create(void) {
    return CreateWaitableTimer(nil, NO, nil);
}

static kern_return_t mk_timer_destroy(HANDLE name) {
    BOOL res = CloseHandle(name);
    if (!res) {
        DWORD err = GetLastError();
        RSLog(RSLogLevelError, RSSTR("RSRunLoop: Unable to destroy timer: %d"), err);
    }
    return (int)res;
}

static kern_return_t mk_timer_arm(HANDLE name, LARGE_INTEGER expire_time) {
    BOOL res = SetWaitableTimer(name, &expire_time, 0, nil, nil, NO);
    if (!res) {
        DWORD err = GetLastError();
        RSLog(RSLogLevelError, RSSTR("RSRunLoop: Unable to set timer: %d"), err);
    }
    return (int)res;
}

static kern_return_t mk_timer_cancel(HANDLE name, LARGE_INTEGER *result_time) {
    BOOL res = CancelWaitableTimer(name);
    if (!res) {
        DWORD err = GetLastError();
        RSLog(RSLogLevelError, RSSTR("RSRunLoop: Unable to cancel timer: %d"), err);
    }
    return (int)res;
}

// The name of this function is a lie on Windows. The return value matches the argument of the fake mk_timer functions above. Note that the Windows timers expect to be given "system time". We have to do some calculations to get the right value, which is a FILETIME-like value.
RSInline LARGE_INTEGER __RSUInt64ToAbsoluteTime(uint64_t desiredFireTime) {
    LARGE_INTEGER result;
    // There is a race we know about here, (timer fire time calculated -> thread suspended -> timer armed == late timer fire), but we don't have a way to avoid it at this time, since the only way to specify an absolute value to the timer is to calculate the relative time first. Fixing that would probably require not using the TSR for timers on Windows.
    uint64_t now = mach_absolute_time();
    if (now > desiredFireTime) {
        result.QuadPart = 0;
    } else {
        uint64_t timeDiff = desiredFireTime - now;
        RSTimeInterval amountOfTimeToWait = __RSTSRToTimeInterval(timeDiff);
        // Result is in 100 ns (10**-7 sec) units to be consistent with a FILETIME.
        // RSTimeInterval is in seconds.
        result.QuadPart = -(amountOfTimeToWait * 10000000);
    }
    return result;
}

#endif

#pragma mark -
#pragma mark Modes

/* unlock a run loop and modes before doing callouts/sleeping */
/* never try to take the run loop lock with a mode locked */
/* be very careful of common subexpression elimination and compacting code, particular across locks and unlocks! */
/* run loop mode structures should never be deallocated, even if they become empty */

static RSTypeID __RSRunLoopModeTypeID = _RSRuntimeNotATypeID;
static RSTypeID __RSRunLoopTypeID = _RSRuntimeNotATypeID;
static RSTypeID __RSRunLoopSourceTypeID = _RSRuntimeNotATypeID;
static RSTypeID __RSRunLoopObserverTypeID = _RSRuntimeNotATypeID;
static RSTypeID __RSRunLoopTimerTypeID = _RSRuntimeNotATypeID;

typedef struct __RSRunLoopMode *RSRunLoopModeRef;

struct __RSRunLoopMode {
    RSRuntimeBase _base;
    pthread_mutex_t _lock;	/* must have the run loop locked before locking this */
    RSStringRef _name;
    BOOL _stopped;
    char _padding[3];
    RSMutableSetRef _sources0;
    RSMutableSetRef _sources1;
    RSMutableArrayRef _observers;
    RSMutableArrayRef _timers;
    RSMutableDictionaryRef _portToV1SourceMap;
    __RSPortSet _portSet;
    RSIndex _observerMask;
#if USE_DISPATCH_SOURCE_FOR_TIMERS
    dispatch_source_t _timerSource;
    dispatch_queue_t _queue;
    BOOL _timerFired; // set to YES by the source when a timer has fired
    BOOL _dispatchTimerArmed;
#endif
#if USE_MK_TIMER_TOO
    mach_port_t _timerPort;
    BOOL _mkTimerArmed;
#endif
#if DEPLOYMENT_TARGET_WINDOWS
    DWORD _msgQMask;
    void (*_msgPump)(void);
#endif
    uint64_t _timerSoftDeadline; /* TSR */
    uint64_t _timerHardDeadline; /* TSR */
};

RSInline void __RSRunLoopModeLock(RSRunLoopModeRef rlm) {
    pthread_mutex_lock(&(rlm->_lock));
    //RSLog(6, RSSTR("__RSRunLoopModeLock locked %p"), rlm);
}

RSInline void __RSRunLoopModeUnlock(RSRunLoopModeRef rlm) {
    //RSLog(6, RSSTR("__RSRunLoopModeLock unlocking %p"), rlm);
    pthread_mutex_unlock(&(rlm->_lock));
}

static BOOL __RSRunLoopModeEqual(RSTypeRef rs1, RSTypeRef rs2) {
    RSRunLoopModeRef rlm1 = (RSRunLoopModeRef)rs1;
    RSRunLoopModeRef rlm2 = (RSRunLoopModeRef)rs2;
    return RSEqual(rlm1->_name, rlm2->_name);
}

static RSHashCode __RSRunLoopModeHash(RSTypeRef rs) {
    RSRunLoopModeRef rlm = (RSRunLoopModeRef)rs;
    return RSHash(rlm->_name);
}

#include <mach/mach_time.h>
static RSStringRef __RSRunLoopModeClassDescription(RSTypeRef rs) {
    RSRunLoopModeRef rlm = (RSRunLoopModeRef)rs;
    RSMutableStringRef result;
    result = RSStringCreateMutable(RSAllocatorSystemDefault, 0);
    RSStringAppendStringWithFormat(result, RSSTR("<RSRunLoopMode %p [%p]>{name = %r, "), rlm, RSGetAllocator(rlm), rlm->_name);
    RSStringAppendStringWithFormat(result, RSSTR("port set = 0x%x, "), rlm->_portSet);
#if USE_DISPATCH_SOURCE_FOR_TIMERS
    RSStringAppendStringWithFormat(result, RSSTR("queue = %p, "), rlm->_queue);
    RSStringAppendStringWithFormat(result, RSSTR("source = %p (%s), "), rlm->_timerSource, rlm->_timerFired ? "fired" : "not fired");
#endif
#if USE_MK_TIMER_TOO
    RSStringAppendStringWithFormat(result, RSSTR("timer port = 0x%x, "), rlm->_timerPort);
#endif
#if DEPLOYMENT_TARGET_WINDOWS
    RSStringAppendFormat(result, RSSTR("MSGQ mask = %p, "), rlm->_msgQMask);
#endif
    RSStringAppendStringWithFormat(result, RSSTR("\n\tsources0 = %r,\n\tsources1 = %r,\n\tobservers = %r,\n\ttimers = %r,\n\tcurrently %0.09g (%lld) / soft deadline in: %0.09g sec (@ %lld) / hard deadline in: %0.09g sec (@ %lld)\n},\n"), rlm->_sources0, rlm->_sources1, rlm->_observers, rlm->_timers, RSAbsoluteTimeGetCurrent(), mach_absolute_time(), __RSTSRToTimeInterval(rlm->_timerSoftDeadline - mach_absolute_time()), rlm->_timerSoftDeadline, __RSTSRToTimeInterval(rlm->_timerHardDeadline - mach_absolute_time()), rlm->_timerHardDeadline);
    return result;
}

static void __RSRunLoopModeDeallocate(RSTypeRef rs) {
    RSRunLoopModeRef rlm = (RSRunLoopModeRef)rs;
    if (nil != rlm->_sources0) RSRelease(rlm->_sources0);
    if (nil != rlm->_sources1) RSRelease(rlm->_sources1);
    if (nil != rlm->_observers) RSRelease(rlm->_observers);
    if (nil != rlm->_timers) RSRelease(rlm->_timers);
    if (nil != rlm->_portToV1SourceMap) RSRelease(rlm->_portToV1SourceMap);
    RSRelease(rlm->_name);
    __RSPortSetFree(rlm->_portSet);
#if USE_DISPATCH_SOURCE_FOR_TIMERS
    if (rlm->_timerSource) {
        dispatch_source_cancel(rlm->_timerSource);
        dispatch_release(rlm->_timerSource);
    }
    if (rlm->_queue) {
        dispatch_release(rlm->_queue);
    }
#endif
#if USE_MK_TIMER_TOO
    if (MACH_PORT_NULL != rlm->_timerPort) mk_timer_destroy(rlm->_timerPort);
#endif
    pthread_mutex_destroy(&rlm->_lock);
    memset((char *)rs + sizeof(RSRuntimeBase), 0x7C, sizeof(struct __RSRunLoopMode) - sizeof(RSRuntimeBase));
}

#pragma mark -
#pragma mark Run Loops

struct _block_item {
    struct _block_item *_next;
    RSTypeRef _mode;	// RSString or RSSet
    void (^_block)(void);
};

typedef struct _per_run_data {
    uint32_t a;
    uint32_t b;
    uint32_t stopped;
    uint32_t ignoreWakeUps;
} _per_run_data;

struct __RSRunLoop {
    RSRuntimeBase _base;
    pthread_mutex_t _lock;			/* locked for accessing mode list */
    __RSPort _wakeUpPort;			// used for RSRunLoopWakeUp
    BOOL _unused;
    volatile _per_run_data *_perRunData;              // reset for runs of the run loop
    pthread_t _pthread;
    uint32_t _winthread;
    RSMutableSetRef _commonModes;
    RSMutableSetRef _commonModeItems;
    RSRunLoopModeRef _currentMode;
    RSMutableSetRef _modes;
    struct _block_item *_blocks_head;
    struct _block_item *_blocks_tail;
    RSTypeRef _counterpart;
    
    dispatch_queue_t _queue;
};

/* Bit 0 of the base reserved bits is used for stopped state */
/* Bit 1 of the base reserved bits is used for sleeping state */
/* Bit 2 of the base reserved bits is used for deallocating state */

RSInline volatile _per_run_data *__RSRunLoopPushPerRunData(RSRunLoopRef rl) {
    volatile _per_run_data *previous = rl->_perRunData;
    rl->_perRunData = (volatile _per_run_data *)RSAllocatorAllocate(RSAllocatorSystemDefault, sizeof(_per_run_data));
    rl->_perRunData->a = 0x5253524C;
    rl->_perRunData->b = 0x5253524C; // 'RSRL'
    rl->_perRunData->stopped = 0x00000000;
    rl->_perRunData->ignoreWakeUps = 0x00000000;
    return previous;
}

RSInline void __RSRunLoopPopPerRunData(RSRunLoopRef rl, volatile _per_run_data *previous) {
    if (rl->_perRunData) RSAllocatorDeallocate(RSAllocatorSystemDefault, (void *)rl->_perRunData);
    rl->_perRunData = previous;
}

RSInline BOOL __RSRunLoopIsStopped(RSRunLoopRef rl) {
    return (rl->_perRunData->stopped) ? YES : NO;
}

RSInline void __RSRunLoopSetStopped(RSRunLoopRef rl) {
    rl->_perRunData->stopped = 0x53544F50;	// 'STOP'
}

RSInline void __RSRunLoopUnsetStopped(RSRunLoopRef rl) {
    rl->_perRunData->stopped = 0x0;
}

RSInline BOOL __RSRunLoopIsIgnoringWakeUps(RSRunLoopRef rl) {
    return (rl->_perRunData->ignoreWakeUps) ? YES : NO;
}

RSInline void __RSRunLoopSetIgnoreWakeUps(RSRunLoopRef rl) {
    rl->_perRunData->ignoreWakeUps = 0x57414B45; // 'WAKE'
}

RSInline void __RSRunLoopUnsetIgnoreWakeUps(RSRunLoopRef rl) {
    rl->_perRunData->ignoreWakeUps = 0x0;
}

RSInline BOOL __RSRunLoopIsSleeping(RSRunLoopRef rl) {
    return (BOOL)__RSBitfieldGetValue(RSRuntimeClassBaseFiled(rl), 1, 1);
}

RSInline void __RSRunLoopSetSleeping(RSRunLoopRef rl) {
    __RSBitfieldSetValue(RSRuntimeClassBaseFiled(rl), 1, 1, 1);
}

RSInline void __RSRunLoopUnsetSleeping(RSRunLoopRef rl) {
    __RSBitfieldSetValue(RSRuntimeClassBaseFiled(rl), 1, 1, 0);
}

RSInline BOOL __RSRunLoopIsDeallocating(RSRunLoopRef rl) {
    return (BOOL)__RSBitfieldGetValue(RSRuntimeClassBaseFiled(rl), 2, 2);
}

RSInline void __RSRunLoopSetDeallocating(RSRunLoopRef rl) {
    __RSBitfieldSetValue(RSRuntimeClassBaseFiled(rl), 2, 2, 1);
}

RSInline void __RSRunLoopLock(RSRunLoopRef rl) {
    pthread_mutex_lock(&(((RSRunLoopRef)rl)->_lock));
    //    RSLog(6, RSSTR("__RSRunLoopLock locked %p"), rl);
}

RSInline void __RSRunLoopUnlock(RSRunLoopRef rl) {
    //    RSLog(6, RSSTR("__RSRunLoopLock unlocking %p"), rl);
    pthread_mutex_unlock(&(((RSRunLoopRef)rl)->_lock));
}

RSInline void __RSRunLoopLockInit(pthread_mutex_t *lock) {
    pthread_mutexattr_t mattr;
    pthread_mutexattr_init(&mattr);
    pthread_mutexattr_settype(&mattr, PTHREAD_MUTEX_RECURSIVE);
    int32_t mret = pthread_mutex_init(lock, &mattr);
    pthread_mutexattr_destroy(&mattr);
    if (0 != mret) {
    }
}

static RSStringRef __RSRunLoopClassDescription(RSTypeRef rs) {
    RSRunLoopRef rl = (RSRunLoopRef)rs;
    RSMutableStringRef result;
    result = RSStringCreateMutable(RSAllocatorSystemDefault, 0);
#if DEPLOYMENT_TARGET_WINDOWS
    RSStringAppendFormat(result, nil, RSSTR("<RSRunLoop %p [%p]>{wakeup port = 0x%x, stopped = %s, ignoreWakeUps = %s, \ncurrent mode = %r,\n"), rs, RSGetAllocator(rs), rl->_wakeUpPort, __RSRunLoopIsStopped(rl) ? "YES" : "NO", __RSRunLoopIsIgnoringWakeUps(rl) ? "YES" : "NO", rl->_currentMode ? rl->_currentMode->_name : RSSTR("(none)"));
#else
    RSStringAppendStringWithFormat(result, nil, RSSTR("<RSRunLoop %p [%p]>{wakeup port = 0x%x, stopped = %s, ignoreWakeUps = %s, \ncurrent mode = %r,\n"), rs, RSGetAllocator(rs), rl->_wakeUpPort, __RSRunLoopIsStopped(rl) ? "YES" : "NO", __RSRunLoopIsIgnoringWakeUps(rl) ? "YES" : "NO", rl->_currentMode ? rl->_currentMode->_name : RSSTR("(none)"));
#endif
    RSStringAppendStringWithFormat(result, nil, RSSTR("common modes = %r,\ncommon mode items = %r,\nmodes = %r}\n"), rl->_commonModes, rl->_commonModeItems, rl->_modes);
    return result;
}

RSPrivate void __RSRunLoopDump() { // __private_extern__ to keep the compiler from discarding it
    RSShow(RSAutorelease(RSDescription(RSRunLoopGetCurrent())));
}

/* call with rl locked, returns mode locked */
static RSRunLoopModeRef __RSRunLoopFindMode(RSRunLoopRef rl, RSStringRef modeName, BOOL create) {
    CHECK_FOR_FORK();
    RSRunLoopModeRef rlm;
    struct __RSRunLoopMode srlm;
    memset(&srlm, 0, sizeof(srlm));
    __RSRuntimeSetInstanceTypeID(&srlm, __RSRunLoopModeTypeID);
    __RSRuntimeSetInstanceAsStackValue(&srlm);
    srlm._name = modeName;
    rlm = (RSRunLoopModeRef)RSSetGetValue(rl->_modes, &srlm);
    if (nil != rlm) {
        __RSRunLoopModeLock(rlm);
        return rlm;
    }
    if (!create) {
        return nil;
    }
    rlm = (RSRunLoopModeRef)__RSRuntimeCreateInstance(RSAllocatorSystemDefault, __RSRunLoopModeTypeID, sizeof(struct __RSRunLoopMode) - sizeof(RSRuntimeBase));
    if (nil == rlm) {
        return nil;
    }
    __RSRunLoopLockInit(&rlm->_lock);
    rlm->_name = RSStringCreateCopy(RSAllocatorSystemDefault, modeName);
    rlm->_stopped = NO;
    rlm->_portToV1SourceMap = nil;
    rlm->_sources0 = nil;
    rlm->_sources1 = nil;
    rlm->_observers = nil;
    rlm->_timers = nil;
    rlm->_observerMask = 0;
    rlm->_portSet = __RSPortSetAllocate();
    rlm->_timerSoftDeadline = UINT64_MAX;
    rlm->_timerHardDeadline = UINT64_MAX;
    
    kern_return_t ret = KERN_SUCCESS;
#if USE_DISPATCH_SOURCE_FOR_TIMERS
    rlm->_timerFired = NO;
    rlm->_queue = _dispatch_runloop_root_queue_create_4CF("Run Loop Mode Queue", 0);
    mach_port_t queuePort = _dispatch_runloop_root_queue_get_port_4CF(rlm->_queue);
    if (queuePort == MACH_PORT_NULL) CRASH("*** Unable to create run loop mode queue port. (%d) ***", -1);
    rlm->_timerSource = dispatch_source_create(DISPATCH_SOURCE_TYPE_TIMER, 0, 0, rlm->_queue);
    
    __block BOOL *timerFiredPointer = &(rlm->_timerFired);
    dispatch_source_set_event_handler(rlm->_timerSource, ^{
        *timerFiredPointer = YES;
    });
    
    // Set timer to far out there. The unique leeway makes this timer easy to spot in debug output.
    _dispatch_source_set_runloop_timer_4CF(rlm->_timerSource, DISPATCH_TIME_FOREVER, DISPATCH_TIME_FOREVER, 321);
    dispatch_resume(rlm->_timerSource);
    
    ret = __RSPortSetInsert(queuePort, rlm->_portSet);
    if (KERN_SUCCESS != ret) CRASH("*** Unable to insert timer port into port set. (%d) ***", ret);
    
#endif
#if USE_MK_TIMER_TOO
    rlm->_timerPort = mk_timer_create();
    ret = __RSPortSetInsert(rlm->_timerPort, rlm->_portSet);
    if (KERN_SUCCESS != ret) CRASH("*** Unable to insert timer port into port set. (%d) ***", ret);
#endif
    
    ret = __RSPortSetInsert(rl->_wakeUpPort, rlm->_portSet);
    if (KERN_SUCCESS != ret) CRASH("*** Unable to insert wake up port into port set. (%d) ***", ret);
    
#if DEPLOYMENT_TARGET_WINDOWS
    rlm->_msgQMask = 0;
    rlm->_msgPump = nil;
#endif
    RSSetAddValue(rl->_modes, rlm);
    RSRelease(rlm);
    __RSRunLoopModeLock(rlm);	/* return mode locked */
    return rlm;
}


// expects rl and rlm locked
static BOOL __RSRunLoopModeIsEmpty(RSRunLoopRef rl, RSRunLoopModeRef rlm, RSRunLoopModeRef previousMode) {
    CHECK_FOR_FORK();
    if (nil == rlm) return YES;
#if DEPLOYMENT_TARGET_WINDOWS
    if (0 != rlm->_msgQMask) return NO;
#endif
    BOOL libdispatchQSafe = pthread_main_np() && ((HANDLE_DISPATCH_ON_BASE_INVOCATION_ONLY && nil == previousMode) || (!HANDLE_DISPATCH_ON_BASE_INVOCATION_ONLY && 0 == _RSGetTSD(__RSTSDKeyIsInGCDMainQ)));
    if (libdispatchQSafe && (RSRunLoopGetMain() == rl) && RSSetContainsValue(rl->_commonModes, rlm->_name)) return NO; // represents the libdispatch main queue
    if (nil != rlm->_sources0 && 0 < RSSetGetCount(rlm->_sources0)) return NO;
    if (nil != rlm->_sources1 && 0 < RSSetGetCount(rlm->_sources1)) return NO;
    if (nil != rlm->_timers && 0 < RSArrayGetCount(rlm->_timers)) return NO;
    struct _block_item *item = rl->_blocks_head;
    while (item) {
        struct _block_item *curr = item;
        item = item->_next;
        BOOL doit = NO;
        if (RSStringGetTypeID() == RSGetTypeID(curr->_mode)) {
            doit = RSEqual(curr->_mode, rlm->_name) || (RSEqual(curr->_mode, RSRunLoopCommonModes) && RSSetContainsValue(rl->_commonModes, rlm->_name));
        } else {
            doit = RSSetContainsValue((RSSetRef)curr->_mode, rlm->_name) || (RSSetContainsValue((RSSetRef)curr->_mode, RSRunLoopCommonModes) && RSSetContainsValue(rl->_commonModes, rlm->_name));
        }
        if (doit) return NO;
    }
    return YES;
}

#if DEPLOYMENT_TARGET_WINDOWS

uint32_t _RSRunLoopGetWindowsMessageQueueMask(RSRunLoopRef rl, RSStringRef modeName) {
    if (modeName == RSRunLoopCommonModes) {
        RSLog(RSLogLevelError, RSSTR("_RSRunLoopGetWindowsMessageQueueMask: RSRunLoopCommonModes unsupported"));
        HALT;
    }
    DWORD result = 0;
    __RSRunLoopLock(rl);
    RSRunLoopModeRef rlm = __RSRunLoopFindMode(rl, modeName, NO);
    if (rlm) {
        result = rlm->_msgQMask;
        __RSRunLoopModeUnlock(rlm);
    }
    __RSRunLoopUnlock(rl);
    return (uint32_t)result;
}

void _RSRunLoopSetWindowsMessageQueueMask(RSRunLoopRef rl, uint32_t mask, RSStringRef modeName) {
    if (modeName == RSRunLoopCommonModes) {
        RSLog(RSLogLevelError, RSSTR("_RSRunLoopSetWindowsMessageQueueMask: RSRunLoopCommonModes unsupported"));
        HALT;
    }
    __RSRunLoopLock(rl);
    RSRunLoopModeRef rlm = __RSRunLoopFindMode(rl, modeName, YES);
    rlm->_msgQMask = (DWORD)mask;
    __RSRunLoopModeUnlock(rlm);
    __RSRunLoopUnlock(rl);
}

uint32_t _RSRunLoopGetWindowsThreadID(RSRunLoopRef rl) {
    return rl->_winthread;
}

RSWindowsMessageQueueHandler _RSRunLoopGetWindowsMessageQueueHandler(RSRunLoopRef rl, RSStringRef modeName) {
    if (modeName == RSRunLoopCommonModes) {
        RSLog(RSLogLevelError, RSSTR("_RSRunLoopGetWindowsMessageQueueMask: RSRunLoopCommonModes unsupported"));
        HALT;
    }
    if (rl != RSRunLoopGetCurrent()) {
        RSLog(RSLogLevelError, RSSTR("_RSRunLoopGetWindowsMessageQueueHandler: run loop parameter must be the current run loop"));
        HALT;
    }
    void (*result)(void) = nil;
    __RSRunLoopLock(rl);
    RSRunLoopModeRef rlm = __RSRunLoopFindMode(rl, modeName, NO);
    if (rlm) {
        result = rlm->_msgPump;
        __RSRunLoopModeUnlock(rlm);
    }
    __RSRunLoopUnlock(rl);
    return result;
}

void _RSRunLoopSetWindowsMessageQueueHandler(RSRunLoopRef rl, RSStringRef modeName, RSWindowsMessageQueueHandler func) {
    if (modeName == RSRunLoopCommonModes) {
        RSLog(RSLogLevelError, RSSTR("_RSRunLoopGetWindowsMessageQueueMask: RSRunLoopCommonModes unsupported"));
        HALT;
    }
    if (rl != RSRunLoopGetCurrent()) {
        RSLog(RSLogLevelError, RSSTR("_RSRunLoopGetWindowsMessageQueueHandler: run loop parameter must be the current run loop"));
        HALT;
    }
    __RSRunLoopLock(rl);
    RSRunLoopModeRef rlm = __RSRunLoopFindMode(rl, modeName, YES);
    rlm->_msgPump = func;
    __RSRunLoopModeUnlock(rlm);
    __RSRunLoopUnlock(rl);
}

#endif

#pragma mark -
#pragma mark Sources

/* Bit 3 in the base reserved bits is used for invalid state in run loop objects */

RSInline BOOL __RSRunLoopIsValid(const void *rs) {
    return (BOOL)__RSBitfieldGetValue(RSRuntimeClassBaseFiled(rs), 3, 3);
}

RSInline void __RSRunLoopSetValid(void *rs) {
    __RSBitfieldSetValue(RSRuntimeClassBaseFiled(rs), 3, 3, 1);
}

RSInline void __RSRunLoopUnsetValid(void *rs) {
    __RSBitfieldSetValue(RSRuntimeClassBaseFiled(rs), 3, 3, 0);
}

struct __RSRunLoopSource {
    RSRuntimeBase _base;
    uint32_t _bits;
    pthread_mutex_t _lock;
    RSIndex _order;			/* immutable */
    RSMutableBagRef _runLoops;
    union {
        RSRunLoopSourceContext version0;	/* immutable, except invalidation */
        RSRunLoopSourceContext1 version1;	/* immutable, except invalidation */
    } _context;
};

/* Bit 1 of the base reserved bits is used for signalled state */

RSInline BOOL __RSRunLoopSourceIsSignaled(RSRunLoopSourceRef rls) {
    return (BOOL)__RSBitfieldGetValue(rls->_bits, 1, 1);
}

RSInline void __RSRunLoopSourceSetSignaled(RSRunLoopSourceRef rls) {
    __RSBitfieldSetValue(rls->_bits, 1, 1, 1);
}

RSInline void __RSRunLoopSourceUnsetSignaled(RSRunLoopSourceRef rls) {
    __RSBitfieldSetValue(rls->_bits, 1, 1, 0);
}

RSInline void __RSRunLoopSourceLock(RSRunLoopSourceRef rls) {
    pthread_mutex_lock(&(rls->_lock));
    //    RSLog(6, RSSTR("__RSRunLoopSourceLock locked %p"), rls);
}

RSInline void __RSRunLoopSourceUnlock(RSRunLoopSourceRef rls) {
    //    RSLog(6, RSSTR("__RSRunLoopSourceLock unlocking %p"), rls);
    pthread_mutex_unlock(&(rls->_lock));
}

#pragma mark Observers

struct __RSRunLoopObserver {
    RSRuntimeBase _base;
    pthread_mutex_t _lock;
    RSRunLoopRef _runLoop;
    RSIndex _rlCount;
    RSOptionFlags _activities;		/* immutable */
    RSIndex _order;			/* immutable */
    RSRunLoopObserverCallBack _callout;	/* immutable */
    RSRunLoopObserverContext _context;	/* immutable, except invalidation */
};

/* Bit 0 of the base reserved bits is used for firing state */
/* Bit 1 of the base reserved bits is used for repeats state */

RSInline BOOL __RSRunLoopObserverIsFiring(RSRunLoopObserverRef rlo) {
    return (BOOL)__RSBitfieldGetValue(RSRuntimeClassBaseFiled(rlo), 0, 0);
}

RSInline void __RSRunLoopObserverSetFiring(RSRunLoopObserverRef rlo) {
    __RSBitfieldSetValue(RSRuntimeClassBaseFiled(rlo), 0, 0, 1);
}

RSInline void __RSRunLoopObserverUnsetFiring(RSRunLoopObserverRef rlo) {
    __RSBitfieldSetValue(RSRuntimeClassBaseFiled(rlo), 0, 0, 0);
}

RSInline BOOL __RSRunLoopObserverRepeats(RSRunLoopObserverRef rlo) {
    return (BOOL)__RSBitfieldGetValue(RSRuntimeClassBaseFiled(rlo), 1, 1);
}

RSInline void __RSRunLoopObserverSetRepeats(RSRunLoopObserverRef rlo) {
    __RSBitfieldSetValue(RSRuntimeClassBaseFiled(rlo), 1, 1, 1);
}

RSInline void __RSRunLoopObserverUnsetRepeats(RSRunLoopObserverRef rlo) {
    __RSBitfieldSetValue(RSRuntimeClassBaseFiled(rlo), 1, 1, 0);
}

RSInline void __RSRunLoopObserverLock(RSRunLoopObserverRef rlo) {
    pthread_mutex_lock(&(rlo->_lock));
    //    RSLog(6, RSSTR("__RSRunLoopObserverLock locked %p"), rlo);
}

RSInline void __RSRunLoopObserverUnlock(RSRunLoopObserverRef rlo) {
    //    RSLog(6, RSSTR("__RSRunLoopObserverLock unlocking %p"), rlo);
    pthread_mutex_unlock(&(rlo->_lock));
}

static void __RSRunLoopObserverSchedule(RSRunLoopObserverRef rlo, RSRunLoopRef rl, RSRunLoopModeRef rlm) {
    __RSRunLoopObserverLock(rlo);
    if (0 == rlo->_rlCount) {
        rlo->_runLoop = rl;
    }
    rlo->_rlCount++;
    __RSRunLoopObserverUnlock(rlo);
}

static void __RSRunLoopObserverCancel(RSRunLoopObserverRef rlo, RSRunLoopRef rl, RSRunLoopModeRef rlm) {
    __RSRunLoopObserverLock(rlo);
    rlo->_rlCount--;
    if (0 == rlo->_rlCount) {
        rlo->_runLoop = nil;
    }
    __RSRunLoopObserverUnlock(rlo);
}

#pragma mark Timers

struct __RSRunLoopTimer {
    RSRuntimeBase _base;
    uint16_t _bits;
    pthread_mutex_t _lock;
    RSRunLoopRef _runLoop;
    RSMutableSetRef _rlModes;
    RSAbsoluteTime _nextFireDate;
    RSTimeInterval _interval;		/* immutable */
    RSTimeInterval _tolerance;          /* mutable */
    uint64_t _fireTSR;			/* TSR units */
    RSIndex _order;			/* immutable */
    RSRunLoopTimerCallBack _callout;	/* immutable */
    RSRunLoopTimerContext _context;	/* immutable, except invalidation */
};

/* Bit 0 of the base reserved bits is used for firing state */
/* Bit 1 of the base reserved bits is used for fired-during-callout state */
/* Bit 2 of the base reserved bits is used for waking state */

RSInline BOOL __RSRunLoopTimerIsFiring(RSRunLoopTimerRef rlt) {
    return (BOOL)__RSBitfieldGetValue(rlt->_bits, 0, 0);
}

RSInline void __RSRunLoopTimerSetFiring(RSRunLoopTimerRef rlt) {
    __RSBitfieldSetValue(rlt->_bits, 0, 0, 1);
}

RSInline void __RSRunLoopTimerUnsetFiring(RSRunLoopTimerRef rlt) {
    __RSBitfieldSetValue(rlt->_bits, 0, 0, 0);
}

RSInline BOOL __RSRunLoopTimerIsDeallocating(RSRunLoopTimerRef rlt) {
    return (BOOL)__RSBitfieldGetValue(rlt->_bits, 2, 2);
}

RSInline void __RSRunLoopTimerSetDeallocating(RSRunLoopTimerRef rlt) {
    __RSBitfieldSetValue(rlt->_bits, 2, 2, 1);
}

RSInline void __RSRunLoopTimerLock(RSRunLoopTimerRef rlt) {
    pthread_mutex_lock(&(rlt->_lock));
    //    RSLog(6, RSSTR("__RSRunLoopTimerLock locked %p"), rlt);
}

RSInline void __RSRunLoopTimerUnlock(RSRunLoopTimerRef rlt) {
    //    RSLog(6, RSSTR("__RSRunLoopTimerLock unlocking %p"), rlt);
    pthread_mutex_unlock(&(rlt->_lock));
}

static RSSpinLock __RSRLTFireTSRLock = RSSpinLockInit;

RSInline void __RSRunLoopTimerFireTSRLock(void) {
    RSSpinLockLock(&__RSRLTFireTSRLock);
}

RSInline void __RSRunLoopTimerFireTSRUnlock(void) {
    RSSpinLockUnlock(&__RSRLTFireTSRLock);
}

#pragma mark -

/* RSRunLoop */

//RS_PUBLIC_CONST_STRING_DECL(RSRunLoopDefaultMode, "RSRunLoopDefaultMode")
//RS_PUBLIC_CONST_STRING_DECL(RSRunLoopCommonModes, "RSRunLoopCommonModes")

// call with rl and rlm locked
static RSRunLoopSourceRef __RSRunLoopModeFindSourceForMachPort(RSRunLoopRef rl, RSRunLoopModeRef rlm, __RSPort port) {	/* DOES CALLOUT */
    CHECK_FOR_FORK();
    RSRunLoopSourceRef found = rlm->_portToV1SourceMap ? (RSRunLoopSourceRef)RSDictionaryGetValue(rlm->_portToV1SourceMap, (const void *)(uintptr_t)port) : nil;
    return found;
}

// Remove backreferences the mode's sources have to the rl (context);
// the primary purpose of rls->_runLoops is so that Invalidation can remove
// the source from the run loops it is in, but during deallocation of a
// run loop, we already know that the sources are going to be punted
// from it, so invalidation of sources does not need to remove from a
// deallocating run loop.
static void __RSRunLoopCleanseSources(const void *value, void *context) {
    RSRunLoopModeRef rlm = (RSRunLoopModeRef)value;
    RSRunLoopRef rl = (RSRunLoopRef)context;
    RSIndex idx, cnt;
    const void **list, *buffer[256];
    if (nil == rlm->_sources0 && nil == rlm->_sources1) return;
    
    cnt = (rlm->_sources0 ? RSSetGetCount(rlm->_sources0) : 0) + (rlm->_sources1 ? RSSetGetCount(rlm->_sources1) : 0);
    list = (const void **)((cnt <= 256) ? buffer : RSAllocatorAllocate(RSAllocatorSystemDefault, cnt * sizeof(void *)));
    if (rlm->_sources0) RSSetGetValues(rlm->_sources0, list);
    if (rlm->_sources1) RSSetGetValues(rlm->_sources1, list + (rlm->_sources0 ? RSSetGetCount(rlm->_sources0) : 0));
    for (idx = 0; idx < cnt; idx++) {
        RSRunLoopSourceRef rls = (RSRunLoopSourceRef)list[idx];
        __RSRunLoopSourceLock(rls);
        if (nil != rls->_runLoops) {
            RSBagRemoveValue(rls->_runLoops, rl);
        }
        __RSRunLoopSourceUnlock(rls);
    }
    if (list != buffer) RSAllocatorDeallocate(RSAllocatorSystemDefault, list);
}

static void __RSRunLoopDeallocateSources(const void *value, void *context) {
    RSRunLoopModeRef rlm = (RSRunLoopModeRef)value;
    RSRunLoopRef rl = (RSRunLoopRef)context;
    RSIndex idx, cnt;
    const void **list, *buffer[256];
    if (nil == rlm->_sources0 && nil == rlm->_sources1) return;
    
    cnt = (rlm->_sources0 ? RSSetGetCount(rlm->_sources0) : 0) + (rlm->_sources1 ? RSSetGetCount(rlm->_sources1) : 0);
    list = (const void **)((cnt <= 256) ? buffer : RSAllocatorAllocate(RSAllocatorSystemDefault, cnt * sizeof(void *)));
    if (rlm->_sources0) RSSetGetValues(rlm->_sources0, list);
    if (rlm->_sources1) RSSetGetValues(rlm->_sources1, list + (rlm->_sources0 ? RSSetGetCount(rlm->_sources0) : 0));
    for (idx = 0; idx < cnt; idx++) {
        RSRetain(list[idx]);
    }
    if (rlm->_sources0) RSSetRemoveAllValues(rlm->_sources0);
    if (rlm->_sources1) RSSetRemoveAllValues(rlm->_sources1);
    for (idx = 0; idx < cnt; idx++) {
        RSRunLoopSourceRef rls = (RSRunLoopSourceRef)list[idx];
        __RSRunLoopSourceLock(rls);
        if (nil != rls->_runLoops) {
            RSBagRemoveValue(rls->_runLoops, rl);
        }
        __RSRunLoopSourceUnlock(rls);
        if (0 == rls->_context.version0.version) {
            if (nil != rls->_context.version0.cancel) {
                rls->_context.version0.cancel(rls->_context.version0.info, rl, rlm->_name);	/* CALLOUT */
            }
        } else if (1 == rls->_context.version0.version) {
            __RSPort port = rls->_context.version1.getPort(rls->_context.version1.info);	/* CALLOUT */
            if (RSPORT_NULL != port) {
                __RSPortSetRemove(port, rlm->_portSet);
            }
        }
        RSRelease(rls);
    }
    if (list != buffer) RSAllocatorDeallocate(RSAllocatorSystemDefault, list);
}

static void __RSRunLoopDeallocateObservers(const void *value, void *context) {
    RSRunLoopModeRef rlm = (RSRunLoopModeRef)value;
    RSRunLoopRef rl = (RSRunLoopRef)context;
    RSIndex idx, cnt;
    const void **list, *buffer[256];
    if (nil == rlm->_observers) return;
    cnt = RSArrayGetCount(rlm->_observers);
    list = (const void **)((cnt <= 256) ? buffer : RSAllocatorAllocate(RSAllocatorSystemDefault, cnt * sizeof(void *)));
    RSArrayGetObjects(rlm->_observers, RSMakeRange(0, cnt), list);
    for (idx = 0; idx < cnt; idx++) {
        RSRetain(list[idx]);
    }
    RSArrayRemoveAllObjects(rlm->_observers);
    for (idx = 0; idx < cnt; idx++) {
        __RSRunLoopObserverCancel((RSRunLoopObserverRef)list[idx], rl, rlm);
        RSRelease(list[idx]);
    }
    if (list != buffer) RSAllocatorDeallocate(RSAllocatorSystemDefault, list);
}

static void __RSRunLoopDeallocateTimers(const void *value, void *context) {
    RSRunLoopModeRef rlm = (RSRunLoopModeRef)value;
    if (nil == rlm->_timers) return;
    void (^deallocateTimers)(RSMutableArrayRef timers) = ^(RSMutableArrayRef timers) {
        RSIndex idx, cnt;
        const void **list, *buffer[256];
        cnt = RSArrayGetCount(timers);
        list = (const void **)((cnt <= 256) ? buffer : RSAllocatorAllocate(RSAllocatorSystemDefault, cnt * sizeof(void *)));
        RSArrayGetObjects(timers, RSMakeRange(0, RSArrayGetCount(timers)), list);
        for (idx = 0; idx < cnt; idx++) {
            RSRetain(list[idx]);
        }
        RSArrayRemoveAllObjects(timers);
        for (idx = 0; idx < cnt; idx++) {
            RSRunLoopTimerRef rlt = (RSRunLoopTimerRef)list[idx];
            __RSRunLoopTimerLock(rlt);
            // if the run loop is deallocating, and since a timer can only be in one
            // run loop, we're going to be removing the timer from all modes, so be
            // a little heavy-handed and direct
            RSSetRemoveAllValues(rlt->_rlModes);
            rlt->_runLoop = nil;
            __RSRunLoopTimerUnlock(rlt);
            RSRelease(list[idx]);
        }
        if (list != buffer) RSAllocatorDeallocate(RSAllocatorSystemDefault, list);
    };
    
    if (rlm->_timers && RSArrayGetCount(rlm->_timers)) deallocateTimers(rlm->_timers);
}

RSExport RSRunLoopRef _RSRunLoopGet0b(pthread_t t);

static void __RSRunLoopClassDeallocate(RSTypeRef rs) {
    RSRunLoopRef rl = (RSRunLoopRef)rs;
    
    if (_RSRunLoopGet0b(pthread_main_thread_np()) == rs) HALT;
    
    /* We try to keep the run loop in a valid state as long as possible,
     since sources may have non-retained references to the run loop.
     Another reason is that we don't want to lock the run loop for
     callback reasons, if we can get away without that.  We start by
     eliminating the sources, since they are the most likely to call
     back into the run loop during their "cancellation". Common mode
     items will be removed from the mode indirectly by the following
     three lines. */
    __RSRunLoopSetDeallocating(rl);
    if (nil != rl->_modes) {
        RSSetApplyFunction(rl->_modes, (__RSRunLoopCleanseSources), rl); // remove references to rl
        RSSetApplyFunction(rl->_modes, (__RSRunLoopDeallocateSources), rl);
        RSSetApplyFunction(rl->_modes, (__RSRunLoopDeallocateObservers), rl);
        RSSetApplyFunction(rl->_modes, (__RSRunLoopDeallocateTimers), rl);
    }
    __RSRunLoopLock(rl);
    struct _block_item *item = rl->_blocks_head;
    while (item) {
        struct _block_item *curr = item;
        item = item->_next;
        RSRelease(curr->_mode);
        Block_release(curr->_block);
        RSAllocatorDeallocate(RSAllocatorSystemDefault, curr);
    }
    if (nil != rl->_commonModeItems) {
        RSRelease(rl->_commonModeItems);
    }
    if (nil != rl->_commonModes) {
        RSRelease(rl->_commonModes);
    }
    if (nil != rl->_modes) {
        RSRelease(rl->_modes);
    }
    __RSPortFree(rl->_wakeUpPort);
    rl->_wakeUpPort = RSPORT_NULL;
    __RSRunLoopPopPerRunData(rl, nil);
    __RSRunLoopUnlock(rl);
    pthread_mutex_destroy(&rl->_lock);
    memset((char *)rs + sizeof(RSRuntimeBase), 0x8C, sizeof(struct __RSRunLoop) - sizeof(RSRuntimeBase));
}

static const RSRuntimeClass __RSRunLoopModeClass = {
    _RSRuntimeScannedObject,
    "RSRunLoopMode",
    nil,      // init
    nil,      // copy
    __RSRunLoopModeDeallocate,
    __RSRunLoopModeEqual,
    __RSRunLoopModeHash,
    __RSRunLoopModeClassDescription,      //
    nil,
    nil
};

static const RSRuntimeClass __RSRunLoopClass = {
    _RSRuntimeScannedObject,
    "RSRunLoop",
    nil,      // init
    nil,      // copy
    __RSRunLoopClassDeallocate,
    nil,
    nil,
    __RSRunLoopClassDescription,      //
    nil,
    nil
};

RSPrivate void __RSFinalizeRunLoop(uintptr_t data);

RSPrivate void __RSRunLoopInitialize(void) {
    __RSRunLoopTypeID = __RSRuntimeRegisterClass(&__RSRunLoopClass);
    __RSRuntimeSetClassTypeID(&__RSRunLoopClass, __RSRunLoopTypeID);
    __RSRunLoopModeTypeID = __RSRuntimeRegisterClass(&__RSRunLoopModeClass);    // 42
    __RSRuntimeSetClassTypeID(&__RSRunLoopModeClass, __RSRunLoopModeTypeID);
    __runloop_malloc_vm_pressure_setup();
}

RSTypeID RSRunLoopGetTypeID(void) {
    return __RSRunLoopTypeID;
}

static RSRunLoopRef __RSRunLoopCreate(pthread_t t) {
    RSRunLoopRef loop = nil;
    RSRunLoopModeRef rlm;
    uint32_t size = sizeof(struct __RSRunLoop) - sizeof(RSRuntimeBase);
    loop = (RSRunLoopRef)__RSRuntimeCreateInstance(RSAllocatorSystemDefault, __RSRunLoopTypeID, size);
    if (nil == loop) {
        return nil;
    }
    (void)__RSRunLoopPushPerRunData(loop);
    __RSRunLoopLockInit(&loop->_lock);
    loop->_wakeUpPort = __RSPortAllocate();
    if (RSPORT_NULL == loop->_wakeUpPort) HALT;
    __RSRunLoopSetIgnoreWakeUps(loop);
    loop->_commonModes = RSSetCreateMutable(RSAllocatorSystemDefault, 0, &RSTypeSetCallBacks);
    RSSetAddValue(loop->_commonModes, RSRunLoopDefaultMode);
    loop->_commonModeItems = nil;
    loop->_currentMode = nil;
    loop->_modes = RSSetCreateMutable(RSAllocatorSystemDefault, 0, &RSTypeSetCallBacks);
    loop->_blocks_head = nil;
    loop->_blocks_tail = nil;
    loop->_counterpart = nil;
    loop->_pthread = t;
#if DEPLOYMENT_TARGET_WINDOWS
    loop->_winthread = GetCurrentThreadId();
#else
    loop->_winthread = 0;
    loop->_queue = pthread_main_thread_np() ? dispatch_get_main_queue() : dispatch_get_current_queue();
#endif
    rlm = __RSRunLoopFindMode(loop, RSRunLoopDefaultMode, YES);
    if (nil != rlm) __RSRunLoopModeUnlock(rlm);
    return loop;
}

static RSMutableDictionaryRef __RSRunLoops = nil;
static RSSpinLock loopsLock = RSSpinLockInit;

// should only be called by Foundation
// t==0 is a synonym for "main thread" that always works
RSExport RSRunLoopRef _RSRunLoopGet0(pthread_t t) {
    if (pthread_equal(t, NilPthreadT)) {
        t = pthread_main_thread_np();
    }
    RSSpinLockLock(&loopsLock);
    if (!__RSRunLoops) {
        RSSpinLockUnlock(&loopsLock);
        
        RSDictionaryContext context = {0, nil, RSDictionaryRSTypeContext->valueContext};
        RSMutableDictionaryRef dict = RSDictionaryCreateMutable(RSAllocatorSystemDefault, 0, &context);
        RSRunLoopRef mainLoop = __RSRunLoopCreate(pthread_main_thread_np());
        RSDictionarySetValue(dict, pthreadPointer(pthread_main_thread_np()), mainLoop);
        if (!OSAtomicCompareAndSwapPtrBarrier(nil, dict, (void * volatile *)&__RSRunLoops)) {
            RSRelease(dict);
        }
        RSRelease(mainLoop);
        RSSpinLockLock(&loopsLock);
    }
    RSRunLoopRef loop = (RSRunLoopRef)RSDictionaryGetValue(__RSRunLoops, pthreadPointer(t));
    RSSpinLockUnlock(&loopsLock);
    if (!loop) {
        RSRunLoopRef newLoop = __RSRunLoopCreate(t);
        RSSpinLockLock(&loopsLock);
        loop = (RSRunLoopRef)RSDictionaryGetValue(__RSRunLoops, pthreadPointer(t));
        if (!loop) {
            RSDictionarySetValue(__RSRunLoops, pthreadPointer(t), newLoop);
            loop = newLoop;
        }
        // don't release run loops inside the loopsLock, because RSRunLoopDeallocate may end up taking it
        RSSpinLockUnlock(&loopsLock);
        RSRelease(newLoop);
    }
    if (pthread_equal(t, pthread_self())) {
        _RSSetTSD(__RSTSDKeyRunLoop, (void *)loop, nil);
        if (0 == _RSGetTSD(__RSTSDKeyRunLoopCntr)) {
            _RSSetTSD(__RSTSDKeyRunLoopCntr, (void *)(PTHREAD_DESTRUCTOR_ITERATIONS-1), (void (*)(void *))__RSFinalizeRunLoop);
        }
    }
    return loop;
}

// should only be called by Foundation
RSRunLoopRef _RSRunLoopGet0b(pthread_t t) {
    if (pthread_equal(t, NilPthreadT)) {
        t = pthread_main_thread_np();
    }
    RSSpinLockLock(&loopsLock);
    RSRunLoopRef loop = nil;
    if (__RSRunLoops) {
        loop = (RSRunLoopRef)RSDictionaryGetValue(__RSRunLoops, pthreadPointer(t));
    }
    RSSpinLockUnlock(&loopsLock);
    return loop;
}

static void __RSRunLoopRemoveAllSources(RSRunLoopRef rl, RSStringRef modeName);

// Called for each thread as it exits
RSPrivate void __RSFinalizeRunLoop(uintptr_t data) {
    RSRunLoopRef rl = nil;
    if (data <= 1) {
        RSSpinLockLock(&loopsLock);
        if (__RSRunLoops) {
            rl = (RSRunLoopRef)RSDictionaryGetValue(__RSRunLoops, pthreadPointer(pthread_self()));
            if (rl) RSRetain(rl);
            RSDictionaryRemoveValue(__RSRunLoops, pthreadPointer(pthread_self()));
        }
        RSSpinLockUnlock(&loopsLock);
    } else {
        _RSSetTSD(__RSTSDKeyRunLoopCntr, (void *)(data - 1), (void (*)(void *))__RSFinalizeRunLoop);
    }
    if (rl && RSRunLoopGetMain() != rl) { // protect against cooperative threads
        if (nil != rl->_counterpart) {
            RSRelease(rl->_counterpart);
            rl->_counterpart = nil;
        }
        // purge all sources before deallocation
        RSArrayRef array = RSRunLoopCopyAllModes(rl);
        for (RSIndex idx = RSArrayGetCount(array); idx--;) {
            RSStringRef modeName = (RSStringRef)RSArrayObjectAtIndex(array, idx);
            __RSRunLoopRemoveAllSources(rl, modeName);
        }
        __RSRunLoopRemoveAllSources(rl, RSRunLoopCommonModes);
        RSRelease(array);
    }
    if (rl) RSRelease(rl);
}

pthread_t _RSRunLoopGet1(RSRunLoopRef rl) {
    return rl->_pthread;
}

// should only be called by Foundation
RSExport RSTypeRef _RSRunLoopGet2(RSRunLoopRef rl) {
    RSTypeRef ret = nil;
    RSSpinLockLock(&loopsLock);
    ret = rl->_counterpart;
    RSSpinLockUnlock(&loopsLock);
    return ret;
}

// should only be called by Foundation
RSExport RSTypeRef _RSRunLoopGet2b(RSRunLoopRef rl) {
    return rl->_counterpart;
}

#if DEPLOYMENT_TARGET_MACOSX
void _RSRunLoopSetCurrent(RSRunLoopRef rl) {
    if (pthread_main_np()) return;
    RSRunLoopRef currentLoop = RSRunLoopGetCurrent();
    if (rl != currentLoop) {
        RSRetain(currentLoop); // avoid a deallocation of the currentLoop inside the lock
        RSSpinLockLock(&loopsLock);
        if (rl) {
            RSDictionarySetValue(__RSRunLoops, pthreadPointer(pthread_self()), rl);
        } else {
            RSDictionaryRemoveValue(__RSRunLoops, pthreadPointer(pthread_self()));
        }
        RSSpinLockUnlock(&loopsLock);
        RSRelease(currentLoop);
        _RSSetTSD(__RSTSDKeyRunLoop, nil, nil);
        _RSSetTSD(__RSTSDKeyRunLoopCntr, 0, (void (*)(void *))__RSFinalizeRunLoop);
    }
}
#endif

RSRunLoopRef RSRunLoopGetMain(void) {
    CHECK_FOR_FORK();
    static RSRunLoopRef __main = nil; // no retain needed
    if (!__main) __main = _RSRunLoopGet0(pthread_main_thread_np()); // no CAS needed
    return __main;
}

RSRunLoopRef RSRunLoopGetCurrent(void) {
    CHECK_FOR_FORK();
    RSRunLoopRef rl = (RSRunLoopRef)_RSGetTSD(__RSTSDKeyRunLoop);
    if (rl) return rl;
    return _RSRunLoopGet0(pthread_self());
}

RSStringRef RSRunLoopCopyCurrentMode(RSRunLoopRef rl) {
    CHECK_FOR_FORK();
    RSStringRef result = nil;
    __RSRunLoopLock(rl);
    if (nil != rl->_currentMode) {
        result = (RSStringRef)RSRetain(rl->_currentMode->_name);
    }
    __RSRunLoopUnlock(rl);
    return result;
}

static void __RSRunLoopGetModeName(const void *value, void *context) {
    RSRunLoopModeRef rlm = (RSRunLoopModeRef)value;
    RSMutableArrayRef array = (RSMutableArrayRef)context;
    RSArrayAddObject(array, rlm->_name);
}

RSArrayRef RSRunLoopCopyAllModes(RSRunLoopRef rl) {
    CHECK_FOR_FORK();
    RSMutableArrayRef array;
    __RSRunLoopLock(rl);
    array = RSArrayCreateMutable(RSAllocatorSystemDefault, RSSetGetCount(rl->_modes));
    RSSetApplyFunction(rl->_modes, (__RSRunLoopGetModeName), array);
    __RSRunLoopUnlock(rl);
    return array;
}

static void __RSRunLoopAddItemsToCommonMode(const void *value, void *ctx) {
    RSTypeRef item = (RSTypeRef)value;
    RSRunLoopRef rl = (RSRunLoopRef)(((RSTypeRef *)ctx)[0]);
    RSStringRef modeName = (RSStringRef)(((RSTypeRef *)ctx)[1]);
    if (RSGetTypeID(item) == __RSRunLoopSourceTypeID) {
        RSRunLoopAddSource(rl, (RSRunLoopSourceRef)item, modeName);
    } else if (RSGetTypeID(item) == __RSRunLoopObserverTypeID) {
        RSRunLoopAddObserver(rl, (RSRunLoopObserverRef)item, modeName);
    } else if (RSGetTypeID(item) == __RSRunLoopTimerTypeID) {
        RSRunLoopAddTimer(rl, (RSRunLoopTimerRef)item, modeName);
    }
}

static void __RSRunLoopAddItemToCommonModes(const void *value, void *ctx) {
    RSStringRef modeName = (RSStringRef)value;
    RSRunLoopRef rl = (RSRunLoopRef)(((RSTypeRef *)ctx)[0]);
    RSTypeRef item = (RSTypeRef)(((RSTypeRef *)ctx)[1]);
    if (RSGetTypeID(item) == __RSRunLoopSourceTypeID) {
        RSRunLoopAddSource(rl, (RSRunLoopSourceRef)item, modeName);
    } else if (RSGetTypeID(item) == __RSRunLoopObserverTypeID) {
        RSRunLoopAddObserver(rl, (RSRunLoopObserverRef)item, modeName);
    } else if (RSGetTypeID(item) == __RSRunLoopTimerTypeID) {
        RSRunLoopAddTimer(rl, (RSRunLoopTimerRef)item, modeName);
    }
}

static void __RSRunLoopRemoveItemFromCommonModes(const void *value, void *ctx) {
    RSStringRef modeName = (RSStringRef)value;
    RSRunLoopRef rl = (RSRunLoopRef)(((RSTypeRef *)ctx)[0]);
    RSTypeRef item = (RSTypeRef)(((RSTypeRef *)ctx)[1]);
    if (RSGetTypeID(item) == __RSRunLoopSourceTypeID) {
        RSRunLoopRemoveSource(rl, (RSRunLoopSourceRef)item, modeName);
    } else if (RSGetTypeID(item) == __RSRunLoopObserverTypeID) {
        RSRunLoopRemoveObserver(rl, (RSRunLoopObserverRef)item, modeName);
    } else if (RSGetTypeID(item) == __RSRunLoopTimerTypeID) {
        RSRunLoopRemoveTimer(rl, (RSRunLoopTimerRef)item, modeName);
    }
}

RSExport BOOL _RSRunLoop01(RSRunLoopRef rl, RSStringRef modeName) {
    __RSRunLoopLock(rl);
    BOOL present = RSSetContainsValue(rl->_commonModes, modeName);
    __RSRunLoopUnlock(rl);
    return present;
}

void RSRunLoopAddCommonMode(RSRunLoopRef rl, RSStringRef modeName) {
    CHECK_FOR_FORK();
    if (__RSRunLoopIsDeallocating(rl)) return;
    __RSRunLoopLock(rl);
    if (!RSSetContainsValue(rl->_commonModes, modeName)) {
        RSSetRef set = rl->_commonModeItems ? RSSetCreateCopy(RSAllocatorSystemDefault, rl->_commonModeItems) : nil;
        RSSetAddValue(rl->_commonModes, modeName);
        if (nil != set) {
            RSTypeRef context[2] = {rl, modeName};
            /* add all common-modes items to new mode */
            RSSetApplyFunction(set, (__RSRunLoopAddItemsToCommonMode), (void *)context);
            RSRelease(set);
        }
    } else {
    }
    __RSRunLoopUnlock(rl);
}


static void __RSRUNLOOP_IS_SERVICING_THE_MAIN_DISPATCH_QUEUE__() __attribute__((noinline));
static void __RSRUNLOOP_IS_SERVICING_THE_MAIN_DISPATCH_QUEUE__(void *msg) {
    _dispatch_main_queue_callback_4CF(msg);
    getpid(); // thwart tail-call optimization
}

static void __RSRUNLOOP_IS_CALLING_OUT_TO_AN_OBSERVER_CALLBACK_FUNCTION__() __attribute__((noinline));
static void __RSRUNLOOP_IS_CALLING_OUT_TO_AN_OBSERVER_CALLBACK_FUNCTION__(RSRunLoopObserverCallBack func, RSRunLoopObserverRef observer, RSRunLoopActivity activity, void *info) {
    if (func) {
        func(observer, activity, info);
    }
    getpid(); // thwart tail-call optimization
}

static void __RSRUNLOOP_IS_CALLING_OUT_TO_A_TIMER_CALLBACK_FUNCTION__() __attribute__((noinline));
static void __RSRUNLOOP_IS_CALLING_OUT_TO_A_TIMER_CALLBACK_FUNCTION__(RSRunLoopTimerCallBack func, RSRunLoopTimerRef timer, void *info) {
    if (func) {
        func(timer, info);
    }
    getpid(); // thwart tail-call optimization
}

static void __RSRUNLOOP_IS_CALLING_OUT_TO_A_BLOCK__() __attribute__((noinline));
static void __RSRUNLOOP_IS_CALLING_OUT_TO_A_BLOCK__(void (^block)(void)) {
    if (block) {
        block();
    }
    getpid(); // thwart tail-call optimization
}

static BOOL __RSRunLoopDoBlocks(RSRunLoopRef rl, RSRunLoopModeRef rlm) { // Call with rl and rlm locked
    if (!rl->_blocks_head) return NO;
    if (!rlm || !rlm->_name) return NO;
    BOOL did = NO;
    struct _block_item *head = rl->_blocks_head;
    struct _block_item *tail = rl->_blocks_tail;
    rl->_blocks_head = nil;
    rl->_blocks_tail = nil;
    RSSetRef commonModes = rl->_commonModes;
    RSStringRef curMode = rlm->_name;
    __RSRunLoopModeUnlock(rlm);
    __RSRunLoopUnlock(rl);
    struct _block_item *prev = nil;
    struct _block_item *item = head;
    while (item) {
        struct _block_item *curr = item;
        item = item->_next;
        BOOL doit = NO;
        if (RSStringGetTypeID() == RSGetTypeID(curr->_mode)) {
            doit = RSEqual(curr->_mode, curMode) || (RSEqual(curr->_mode, RSRunLoopCommonModes) && RSSetContainsValue(commonModes, curMode));
        } else {
            doit = RSSetContainsValue((RSSetRef)curr->_mode, curMode) || (RSSetContainsValue((RSSetRef)curr->_mode, RSRunLoopCommonModes) && RSSetContainsValue(commonModes, curMode));
        }
        if (!doit) prev = curr;
        if (doit) {
            if (prev) prev->_next = item;
            if (curr == head) head = item;
            if (curr == tail) tail = prev;
            void (^block)(void) = curr->_block;
            RSRelease(curr->_mode);
            RSAllocatorDeallocate(RSAllocatorSystemDefault, curr);
            if (doit) {
                __RSRUNLOOP_IS_CALLING_OUT_TO_A_BLOCK__(block);
                did = YES;
            }
            Block_release(block); // do this before relocking to prevent deadlocks where some yahoo wants to run the run loop reentrantly from their dealloc
        }
    }
    __RSRunLoopLock(rl);
    __RSRunLoopModeLock(rlm);
    if (head) {
        tail->_next = rl->_blocks_head;
        rl->_blocks_head = head;
        if (!rl->_blocks_tail) rl->_blocks_tail = tail;
    }
    return did;
}

/* rl is locked, rlm is locked on entrance and exit */
static void __RSRunLoopDoObservers() __attribute__((noinline));
static void __RSRunLoopDoObservers(RSRunLoopRef rl, RSRunLoopModeRef rlm, RSRunLoopActivity activity) {	/* DOES CALLOUT */
    CHECK_FOR_FORK();
    
    RSIndex cnt = rlm->_observers ? RSArrayGetCount(rlm->_observers) : 0;
    if (cnt < 1) return;
    
    /* Fire the observers */
    STACK_BUFFER_DECL(RSRunLoopObserverRef, buffer, (cnt <= 1024) ? cnt : 1);
    RSRunLoopObserverRef *collectedObservers = (cnt <= 1024) ? buffer : (RSRunLoopObserverRef *)RSAllocatorAllocate(RSAllocatorSystemDefault, cnt * sizeof(RSRunLoopObserverRef));
    RSIndex obs_cnt = 0;
    for (RSIndex idx = 0; idx < cnt; idx++) {
        RSRunLoopObserverRef rlo = (RSRunLoopObserverRef)RSArrayObjectAtIndex(rlm->_observers, idx);
        if (0 != (rlo->_activities & activity) && __RSRunLoopIsValid(rlo) && !__RSRunLoopObserverIsFiring(rlo)) {
            collectedObservers[obs_cnt++] = (RSRunLoopObserverRef)RSRetain(rlo);
        }
    }
    __RSRunLoopModeUnlock(rlm);
    __RSRunLoopUnlock(rl);
    for (RSIndex idx = 0; idx < obs_cnt; idx++) {
        RSRunLoopObserverRef rlo = collectedObservers[idx];
        __RSRunLoopObserverLock(rlo);
        if (__RSRunLoopIsValid(rlo)) {
            BOOL doInvalidate = !__RSRunLoopObserverRepeats(rlo);
            __RSRunLoopObserverSetFiring(rlo);
            __RSRunLoopObserverUnlock(rlo);
            __RSRUNLOOP_IS_CALLING_OUT_TO_AN_OBSERVER_CALLBACK_FUNCTION__(rlo->_callout, rlo, activity, rlo->_context.info);
            if (doInvalidate) {
                RSRunLoopObserverInvalidate(rlo);
            }
            __RSRunLoopObserverUnsetFiring(rlo);
        } else {
            __RSRunLoopObserverUnlock(rlo);
        }
        RSRelease(rlo);
    }
    __RSRunLoopLock(rl);
    __RSRunLoopModeLock(rlm);
    
    if (collectedObservers != buffer) RSAllocatorDeallocate(RSAllocatorSystemDefault, collectedObservers);
}

static RSComparisonResult __RSRunLoopSourceComparator(const void *val1, const void *val2, void *context) {
    RSRunLoopSourceRef o1 = (RSRunLoopSourceRef)val1;
    RSRunLoopSourceRef o2 = (RSRunLoopSourceRef)val2;
    if (o1->_order < o2->_order) return RSCompareLessThan;
    if (o2->_order < o1->_order) return RSCompareGreaterThan;
    return RSCompareEqualTo;
}

static void __RSRunLoopCollectSources0(const void *value, void *context) {
    RSRunLoopSourceRef rls = (RSRunLoopSourceRef)value;
    RSTypeRef *sources = (RSTypeRef *)context;
    if (0 == rls->_context.version0.version && __RSRunLoopIsValid(rls) && __RSRunLoopSourceIsSignaled(rls)) {
        if (nil == *sources) {
            *sources = RSRetain(rls);
        } else if (RSGetTypeID(*sources) == __RSRunLoopSourceTypeID) {
            RSTypeRef oldrls = *sources;
            *sources = RSArrayCreateMutable(RSAllocatorSystemDefault, 0);
            RSArrayAddObject((RSMutableArrayRef)*sources, oldrls);
            RSArrayAddObject((RSMutableArrayRef)*sources, rls);
            RSRelease(oldrls);
        } else {
            RSArrayAddObject((RSMutableArrayRef)*sources, rls);
        }
    }
}

static void __RSRUNLOOP_IS_CALLING_OUT_TO_A_SOURCE0_PERFORM_FUNCTION__() __attribute__((noinline));
static void __RSRUNLOOP_IS_CALLING_OUT_TO_A_SOURCE0_PERFORM_FUNCTION__(void (*perform)(void *), void *info) {
    if (perform) {
        perform(info);
    }
    getpid(); // thwart tail-call optimization
}

static void __RSRUNLOOP_IS_CALLING_OUT_TO_A_SOURCE1_PERFORM_FUNCTION__() __attribute__((noinline));
static void __RSRUNLOOP_IS_CALLING_OUT_TO_A_SOURCE1_PERFORM_FUNCTION__(
#if DEPLOYMENT_TARGET_MACOSX || DEPLOYMENT_TARGET_EMBEDDED || DEPLOYMENT_TARGET_EMBEDDED_MINI
                                                                       void *(*perform)(void *msg, RSIndex size, RSAllocatorRef allocator, void *info),
                                                                       mach_msg_header_t *msg, RSIndex size, mach_msg_header_t **reply,
#else
                                                                       void (*perform)(void *),
#endif
                                                                       void *info) {
    if (perform) {
#if DEPLOYMENT_TARGET_MACOSX || DEPLOYMENT_TARGET_EMBEDDED || DEPLOYMENT_TARGET_EMBEDDED_MINI
        *reply = perform(msg, size, RSAllocatorSystemDefault, info);
#else
        perform(info);
#endif
    }
    getpid(); // thwart tail-call optimization
}

/* rl is locked, rlm is locked on entrance and exit */
static BOOL __RSRunLoopDoSources0(RSRunLoopRef rl, RSRunLoopModeRef rlm, BOOL stopAfterHandle) __attribute__((noinline));
static BOOL __RSRunLoopDoSources0(RSRunLoopRef rl, RSRunLoopModeRef rlm, BOOL stopAfterHandle) {	/* DOES CALLOUT */
    CHECK_FOR_FORK();
    RSTypeRef sources = nil;
    BOOL sourceHandled = NO;
    
    /* Fire the version 0 sources */
    if (nil != rlm->_sources0 && 0 < RSSetGetCount(rlm->_sources0)) {
        RSSetApplyFunction(rlm->_sources0, (__RSRunLoopCollectSources0), &sources);
    }
    if (nil != sources) {
        __RSRunLoopModeUnlock(rlm);
        __RSRunLoopUnlock(rl);
        // sources is either a single (retained) RSRunLoopSourceRef or an array of (retained) RSRunLoopSourceRef
        if (RSGetTypeID(sources) == __RSRunLoopSourceTypeID) {
            RSRunLoopSourceRef rls = (RSRunLoopSourceRef)sources;
            __RSRunLoopSourceLock(rls);
            if (__RSRunLoopSourceIsSignaled(rls)) {
                __RSRunLoopSourceUnsetSignaled(rls);
                if (__RSRunLoopIsValid(rls)) {
                    __RSRunLoopSourceUnlock(rls);
                    __RSRUNLOOP_IS_CALLING_OUT_TO_A_SOURCE0_PERFORM_FUNCTION__(rls->_context.version0.perform, rls->_context.version0.info);
                    CHECK_FOR_FORK();
                    sourceHandled = YES;
                } else {
                    __RSRunLoopSourceUnlock(rls);
                }
            } else {
                __RSRunLoopSourceUnlock(rls);
            }
        } else {
            RSIndex cnt = RSArrayGetCount((RSArrayRef)sources);
            RSArraySort(sources, RSOrderedAscending, (__RSRunLoopSourceComparator), nil);
//            RSArraySortValues((RSMutableArrayRef)sources, RSMakeRange(0, cnt), (__RSRunLoopSourceComparator), nil);
            for (RSIndex idx = 0; idx < cnt; idx++) {
                RSRunLoopSourceRef rls = (RSRunLoopSourceRef)RSArrayObjectAtIndex((RSArrayRef)sources, idx);
                __RSRunLoopSourceLock(rls);
                if (__RSRunLoopSourceIsSignaled(rls)) {
                    __RSRunLoopSourceUnsetSignaled(rls);
                    if (__RSRunLoopIsValid(rls)) {
                        __RSRunLoopSourceUnlock(rls);
                        __RSRUNLOOP_IS_CALLING_OUT_TO_A_SOURCE0_PERFORM_FUNCTION__(rls->_context.version0.perform, rls->_context.version0.info);
                        CHECK_FOR_FORK();
                        sourceHandled = YES;
                    } else {
                        __RSRunLoopSourceUnlock(rls);
                    }
                } else {
                    __RSRunLoopSourceUnlock(rls);
                }
                if (stopAfterHandle && sourceHandled) {
                    break;
                }
            }
        }
        RSRelease(sources);
        __RSRunLoopLock(rl);
        __RSRunLoopModeLock(rlm);
    }
    return sourceHandled;
}

RSInline void __RSRunLoopDebugInfoForRunLoopSource(RSRunLoopSourceRef rls) {
}

// msg, size and reply are unused on Windows
static BOOL __RSRunLoopDoSource1() __attribute__((noinline));
static BOOL __RSRunLoopDoSource1(RSRunLoopRef rl, RSRunLoopModeRef rlm, RSRunLoopSourceRef rls
#if DEPLOYMENT_TARGET_MACOSX || DEPLOYMENT_TARGET_EMBEDDED || DEPLOYMENT_TARGET_EMBEDDED_MINI
                                    , mach_msg_header_t *msg, RSIndex size, mach_msg_header_t **reply
#endif
) {	/* DOES CALLOUT */
    CHECK_FOR_FORK();
    BOOL sourceHandled = NO;
    
    /* Fire a version 1 source */
    RSRetain(rls);
    __RSRunLoopModeUnlock(rlm);
    __RSRunLoopUnlock(rl);
    __RSRunLoopSourceLock(rls);
    if (__RSRunLoopIsValid(rls)) {
        __RSRunLoopSourceUnsetSignaled(rls);
        __RSRunLoopSourceUnlock(rls);
        __RSRunLoopDebugInfoForRunLoopSource(rls);
        __RSRUNLOOP_IS_CALLING_OUT_TO_A_SOURCE1_PERFORM_FUNCTION__(rls->_context.version1.perform,
#if DEPLOYMENT_TARGET_MACOSX || DEPLOYMENT_TARGET_EMBEDDED || DEPLOYMENT_TARGET_EMBEDDED_MINI
                                                                   msg, size, reply,
#endif
                                                                   rls->_context.version1.info);
        CHECK_FOR_FORK();
        sourceHandled = YES;
    } else {
        if (_LogRSRunLoop) { __RSLog(RSLogLevelDebug, RSSTR("%p (%s) __RSRunLoopDoSource1 rls %p is invalid"), RSRunLoopGetCurrent(), *_RSGetProgname(), rls); }
        __RSRunLoopSourceUnlock(rls);
    }
    RSRelease(rls);
    __RSRunLoopLock(rl);
    __RSRunLoopModeLock(rlm);
    return sourceHandled;
}

static RSIndex __RSRunLoopInsertionIndexInTimerArray(RSArrayRef array, RSRunLoopTimerRef rlt) __attribute__((noinline));
static RSIndex __RSRunLoopInsertionIndexInTimerArray(RSArrayRef array, RSRunLoopTimerRef rlt) {
    RSIndex cnt = RSArrayGetCount(array);
    if (cnt <= 0) {
        return 0;
    }
    if (256 < cnt) {
        RSRunLoopTimerRef item = (RSRunLoopTimerRef)RSArrayObjectAtIndex(array, cnt - 1);
        if (item->_fireTSR <= rlt->_fireTSR) {
            return cnt;
        }
        item = (RSRunLoopTimerRef)RSArrayObjectAtIndex(array, 0);
        if (rlt->_fireTSR < item->_fireTSR) {
            return 0;
        }
    }
    
    RSIndex add = (1 << flsl(cnt)) * 2;
    RSIndex idx = 0;
    BOOL lastTestLEQ;
    do {
        add = add / 2;
        lastTestLEQ = NO;
        RSIndex testIdx = idx + add;
        if (testIdx < cnt) {
            RSRunLoopTimerRef item = (RSRunLoopTimerRef)RSArrayObjectAtIndex(array, testIdx);
            if (item->_fireTSR <= rlt->_fireTSR) {
                idx = testIdx;
                lastTestLEQ = YES;
            }
        }
    } while (0 < add);
    
    return lastTestLEQ ? idx + 1 : idx;
}

static void __RSArmNextTimerInMode(RSRunLoopModeRef rlm, RSRunLoopRef rl) {
    uint64_t nextHardDeadline = UINT64_MAX;
    uint64_t nextSoftDeadline = UINT64_MAX;
    
    if (rlm->_timers) {
        // Look at the list of timers. We will calculate two TSR values; the next soft and next hard deadline.
        // The next soft deadline is the first time we can fire any timer. This is the fire date of the first timer in our sorted list of timers.
        // The next hard deadline is the last time at which we can fire the timer before we've moved out of the allowable tolerance of the timers in our list.
        for (RSIndex idx = 0, cnt = RSArrayGetCount(rlm->_timers); idx < cnt; idx++) {
            RSRunLoopTimerRef t = (RSRunLoopTimerRef)RSArrayObjectAtIndex(rlm->_timers , idx);
            // discount timers currently firing
            if (__RSRunLoopTimerIsFiring(t)) continue;
            
            int32_t err = CHECKINT_NO_ERROR;
            uint64_t oneTimerSoftDeadline = t->_fireTSR;
            uint64_t oneTimerHardDeadline = check_uint64_add(t->_fireTSR, __RSTimeIntervalToTSR(t->_tolerance), &err);
            if (err != CHECKINT_NO_ERROR) oneTimerHardDeadline = UINT64_MAX;
            
            // We can stop searching if the soft deadline for this timer exceeds the current hard deadline. Otherwise, later timers with lower tolerance could still have earlier hard deadlines.
            if (oneTimerSoftDeadline > nextHardDeadline) {
                break;
            }
            
            if (oneTimerSoftDeadline < nextSoftDeadline) {
                nextSoftDeadline = oneTimerSoftDeadline;
            }
            
            if (oneTimerHardDeadline < nextHardDeadline) {
                nextHardDeadline = oneTimerHardDeadline;
            }
        }
        
        if (nextSoftDeadline < UINT64_MAX && (nextHardDeadline != rlm->_timerHardDeadline || nextSoftDeadline != rlm->_timerSoftDeadline)) {
            if (RSRUNLOOP_NEXT_TIMER_ARMED_ENABLED()) {
                RSRUNLOOP_NEXT_TIMER_ARMED((unsigned long)(nextSoftDeadline - mach_absolute_time()));
            }
#if USE_DISPATCH_SOURCE_FOR_TIMERS
            // We're going to hand off the range of allowable timer fire date to dispatch and let it fire when appropriate for the system.
            uint64_t leeway = __RSTSRToNanoseconds(nextHardDeadline - nextSoftDeadline);
            dispatch_time_t deadline = __RSTSRToDispatchTime(nextSoftDeadline);
#if USE_MK_TIMER_TOO
            if (leeway > 0) {
                // Only use the dispatch timer if we have any leeway
                // <rdar://problem/14447675>
                
                // Cancel the mk timer
                if (rlm->_mkTimerArmed && rlm->_timerPort) {
                    AbsoluteTime dummy;
                    mk_timer_cancel(rlm->_timerPort, &dummy);
                    rlm->_mkTimerArmed = NO;
                }
                
                // Arm the dispatch timer
                _dispatch_source_set_runloop_timer_4CF(rlm->_timerSource, deadline, DISPATCH_TIME_FOREVER, leeway);
                rlm->_dispatchTimerArmed = YES;
            } else {
                // Cancel the dispatch timer
                if (rlm->_dispatchTimerArmed) {
                    // Cancel the dispatch timer
                    _dispatch_source_set_runloop_timer_4CF(rlm->_timerSource, DISPATCH_TIME_FOREVER, DISPATCH_TIME_FOREVER, 888);
                    rlm->_dispatchTimerArmed = NO;
                }
                
                // Arm the mk timer
                if (rlm->_timerPort) {
                    mk_timer_arm(rlm->_timerPort, __RSUInt64ToAbsoluteTime(nextSoftDeadline));
                    rlm->_mkTimerArmed = YES;
                }
            }
#else
            _dispatch_source_set_runloop_timer_4RS(rlm->_timerSource, deadline, DISPATCH_TIME_FOREVER, leeway);
#endif
#else
            if (rlm->_timerPort) {
                mk_timer_arm(rlm->_timerPort, __RSUInt64ToAbsoluteTime(nextSoftDeadline));
            }
#endif
        } else if (nextSoftDeadline == UINT64_MAX) {
            // Disarm the timers - there is no timer scheduled
            
            if (rlm->_mkTimerArmed && rlm->_timerPort) {
                AbsoluteTime dummy;
                mk_timer_cancel(rlm->_timerPort, &dummy);
                rlm->_mkTimerArmed = NO;
            }
            
#if USE_DISPATCH_SOURCE_FOR_TIMERS
            if (rlm->_dispatchTimerArmed) {
                _dispatch_source_set_runloop_timer_4CF(rlm->_timerSource, DISPATCH_TIME_FOREVER, DISPATCH_TIME_FOREVER, 333);
                rlm->_dispatchTimerArmed = NO;
            }
#endif
        }
    }
    rlm->_timerHardDeadline = nextHardDeadline;
    rlm->_timerSoftDeadline = nextSoftDeadline;
}

// call with rlm and its run loop locked, and the TSRLock locked; rlt not locked; returns with same state
static void __RSRepositionTimerInMode(RSRunLoopModeRef rlm, RSRunLoopTimerRef rlt, BOOL isInArray) __attribute__((noinline));
static void __RSRepositionTimerInMode(RSRunLoopModeRef rlm, RSRunLoopTimerRef rlt, BOOL isInArray) {
    if (!rlt) return;
    
    RSMutableArrayRef timerArray = rlm->_timers;
    if (!timerArray) return;
    BOOL found = NO;
    
    // If we know in advance that the timer is not in the array (just being added now) then we can skip this search
    if (isInArray) {
        RSIndex idx = RSArrayIndexOfObject(timerArray, rlt);
        if (RSNotFound != idx) {
            RSRetain(rlt);
            RSArrayRemoveObjectAtIndex(timerArray, idx);
            found = YES;
        }
    }
    if (!found && isInArray) return;
    RSIndex newIdx = __RSRunLoopInsertionIndexInTimerArray(timerArray, rlt);
    RSArrayInsertObjectAtIndex(timerArray, newIdx, rlt);
    __RSArmNextTimerInMode(rlm, rlt->_runLoop);
    if (isInArray) RSRelease(rlt);
}


// mode and rl are locked on entry and exit
static BOOL __RSRunLoopDoTimer(RSRunLoopRef rl, RSRunLoopModeRef rlm, RSRunLoopTimerRef rlt) {	/* DOES CALLOUT */
    BOOL timerHandled = NO;
    uint64_t oldFireTSR = 0;
    
    /* Fire a timer */
    RSRetain(rlt);
    __RSRunLoopTimerLock(rlt);
    
    if (__RSRunLoopIsValid(rlt) && rlt->_fireTSR <= mach_absolute_time() && !__RSRunLoopTimerIsFiring(rlt) && rlt->_runLoop == rl) {
        void *context_info = nil;
        void (*context_release)(const void *) = nil;
        if (rlt->_context.retain) {
            context_info = (void *)rlt->_context.retain(rlt->_context.info);
            context_release = rlt->_context.release;
        } else {
            context_info = rlt->_context.info;
        }
        BOOL doInvalidate = (0.0 == rlt->_interval);
        __RSRunLoopTimerSetFiring(rlt);
        // Just in case the next timer has exactly the same deadlines as this one, we reset these values so that the arm next timer code can correctly find the next timer in the list and arm the underlying timer.
        rlm->_timerSoftDeadline = UINT64_MAX;
        rlm->_timerHardDeadline = UINT64_MAX;
        __RSRunLoopTimerUnlock(rlt);
        __RSRunLoopTimerFireTSRLock();
        oldFireTSR = rlt->_fireTSR;
        __RSRunLoopTimerFireTSRUnlock();
        
        __RSArmNextTimerInMode(rlm, rl);
        
        __RSRunLoopModeUnlock(rlm);
        __RSRunLoopUnlock(rl);
        __RSRUNLOOP_IS_CALLING_OUT_TO_A_TIMER_CALLBACK_FUNCTION__(rlt->_callout, rlt, context_info);
        CHECK_FOR_FORK();
        if (doInvalidate) {
            RSRunLoopTimerInvalidate(rlt);      /* DOES CALLOUT */
        }
        if (context_release) {
            context_release(context_info);
        }
        __RSRunLoopLock(rl);
        __RSRunLoopModeLock(rlm);
        __RSRunLoopTimerLock(rlt);
        timerHandled = YES;
        __RSRunLoopTimerUnsetFiring(rlt);
    }
    if (__RSRunLoopIsValid(rlt) && timerHandled) {
        /* This is just a little bit tricky: we want to support calling
         * RSRunLoopTimerSetNextFireDate() from within the callout and
         * honor that new time here if it is a later date, otherwise
         * it is completely ignored. */
        if (oldFireTSR < rlt->_fireTSR) {
            /* Next fire TSR was set, and set to a date after the previous
             * fire date, so we honor it. */
            __RSRunLoopTimerUnlock(rlt);
            // The timer was adjusted and repositioned, during the
            // callout, but if it was still the min timer, it was
            // skipped because it was firing.  Need to redo the
            // min timer calculation in case rlt should now be that
            // timer instead of whatever was chosen.
            __RSArmNextTimerInMode(rlm, rl);
        } else {
            uint64_t nextFireTSR = 0LL;
            uint64_t intervalTSR = 0LL;
            if (rlt->_interval <= 0.0) {
            } else if (TIMER_INTERVAL_LIMIT < rlt->_interval) {
                intervalTSR = __RSTimeIntervalToTSR(TIMER_INTERVAL_LIMIT);
            } else {
                intervalTSR = __RSTimeIntervalToTSR(rlt->_interval);
            }
            if (LLONG_MAX - intervalTSR <= oldFireTSR) {
                nextFireTSR = LLONG_MAX;
            } else {
                uint64_t currentTSR = mach_absolute_time();
                nextFireTSR = oldFireTSR;
                while (nextFireTSR <= currentTSR) {
                    nextFireTSR += intervalTSR;
                }
            }
            RSRunLoopRef rlt_rl = rlt->_runLoop;
            if (rlt_rl) {
                RSRetain(rlt_rl);
                RSIndex cnt = RSSetGetCount(rlt->_rlModes);
                STACK_BUFFER_DECL(RSTypeRef, modes, cnt);
                RSSetGetValues(rlt->_rlModes, (const void **)modes);
                // To avoid A->B, B->A lock ordering issues when coming up
                // towards the run loop from a source, the timer has to be
                // unlocked, which means we have to protect from object
                // invalidation, although that's somewhat expensive.
                for (RSIndex idx = 0; idx < cnt; idx++) {
                    RSRetain(modes[idx]);
                }
                __RSRunLoopTimerUnlock(rlt);
                for (RSIndex idx = 0; idx < cnt; idx++) {
                    RSStringRef name = (RSStringRef)modes[idx];
                    modes[idx] = (RSTypeRef)__RSRunLoopFindMode(rlt_rl, name, NO);
                    RSRelease(name);
                }
                __RSRunLoopTimerFireTSRLock();
                rlt->_fireTSR = nextFireTSR;
                rlt->_nextFireDate = RSAbsoluteTimeGetCurrent() + __RSTimeIntervalUntilTSR(nextFireTSR);
                for (RSIndex idx = 0; idx < cnt; idx++) {
                    RSRunLoopModeRef rlm = (RSRunLoopModeRef)modes[idx];
                    if (rlm) {
                        __RSRepositionTimerInMode(rlm, rlt, YES);
                    }
                }
                __RSRunLoopTimerFireTSRUnlock();
                for (RSIndex idx = 0; idx < cnt; idx++) {
                    __RSRunLoopModeUnlock((RSRunLoopModeRef)modes[idx]);
                }
                RSRelease(rlt_rl);
            } else {
                __RSRunLoopTimerUnlock(rlt);
                __RSRunLoopTimerFireTSRLock();
                rlt->_fireTSR = nextFireTSR;
                rlt->_nextFireDate = RSAbsoluteTimeGetCurrent() + __RSTimeIntervalUntilTSR(nextFireTSR);
                __RSRunLoopTimerFireTSRUnlock();
            }
        }
    } else {
        __RSRunLoopTimerUnlock(rlt);
    }
    RSRelease(rlt);
    return timerHandled;
}


// rl and rlm are locked on entry and exit
static BOOL __RSRunLoopDoTimers(RSRunLoopRef rl, RSRunLoopModeRef rlm, uint64_t limitTSR) {	/* DOES CALLOUT */
    BOOL timerHandled = NO;
    RSMutableArrayRef timers = nil;
    for (RSIndex idx = 0, cnt = rlm->_timers ? RSArrayGetCount(rlm->_timers) : 0; idx < cnt; idx++) {
        RSRunLoopTimerRef rlt = (RSRunLoopTimerRef)RSArrayObjectAtIndex(rlm->_timers, idx);
        
        if (__RSRunLoopIsValid(rlt) && !__RSRunLoopTimerIsFiring(rlt)) {
            if (rlt->_fireTSR <= limitTSR) {
                if (!timers) timers = RSArrayCreateMutable(RSAllocatorSystemDefault, 0);
                RSArrayAddObject(timers, rlt);
            }
        }
    }
    
    for (RSIndex idx = 0, cnt = timers ? RSArrayGetCount(timers) : 0; idx < cnt; idx++) {
        RSRunLoopTimerRef rlt = (RSRunLoopTimerRef)RSArrayObjectAtIndex(timers, idx);
        BOOL did = __RSRunLoopDoTimer(rl, rlm, rlt);
        timerHandled = timerHandled || did;
    }
    if (timers) RSRelease(timers);
    return timerHandled;
}


RSExport BOOL _RSRunLoopFinished(RSRunLoopRef rl, RSStringRef modeName) {
    CHECK_FOR_FORK();
    RSRunLoopModeRef rlm;
    BOOL result = NO;
    __RSRunLoopLock(rl);
    rlm = __RSRunLoopFindMode(rl, modeName, NO);
    if (nil == rlm || __RSRunLoopModeIsEmpty(rl, rlm, nil)) {
        result = YES;
    }
    if (rlm) __RSRunLoopModeUnlock(rlm);
    __RSRunLoopUnlock(rl);
    return result;
}

static int32_t __RSRunLoopRun(RSRunLoopRef rl, RSRunLoopModeRef rlm, RSTimeInterval seconds, BOOL stopAfterHandle, RSRunLoopModeRef previousMode) __attribute__((noinline));

#if DEPLOYMENT_TARGET_MACOSX || DEPLOYMENT_TARGET_EMBEDDED || DEPLOYMENT_TARGET_EMBEDDED_MINI

#define TIMEOUT_INFINITY (~(mach_msg_timeout_t)0)

static BOOL __RSRunLoopServiceMachPort(mach_port_name_t port, mach_msg_header_t **buffer, size_t buffer_size, mach_port_t *livePort, mach_msg_timeout_t timeout) {
    BOOL originalBuffer = YES;
    kern_return_t ret = KERN_SUCCESS;
    for (;;) {		/* In that sleep of death what nightmares may come ... */
        mach_msg_header_t *msg = (mach_msg_header_t *)*buffer;
        msg->msgh_bits = 0;
        msg->msgh_local_port = port;
        msg->msgh_remote_port = MACH_PORT_NULL;
        msg->msgh_size = (mach_msg_size_t)buffer_size;
        msg->msgh_id = 0;
        if (TIMEOUT_INFINITY == timeout) { RSRUNLOOP_SLEEP(); } else { RSRUNLOOP_POLL(); }
        ret = mach_msg(msg, MACH_RCV_MSG|MACH_RCV_LARGE|((TIMEOUT_INFINITY != timeout) ? MACH_RCV_TIMEOUT : 0)|MACH_RCV_TRAILER_TYPE(MACH_MSG_TRAILER_FORMAT_0)|MACH_RCV_TRAILER_ELEMENTS(MACH_RCV_TRAILER_AV), 0, msg->msgh_size, port, timeout, MACH_PORT_NULL);
        RSRUNLOOP_WAKEUP(ret);
        if (MACH_MSG_SUCCESS == ret) {
            *livePort = msg ? msg->msgh_local_port : MACH_PORT_NULL;
            return YES;
        }
        if (MACH_RCV_TIMED_OUT == ret) {
            if (!originalBuffer) RSAllocatorDeallocate(RSAllocatorSystemDefault, msg);
            *buffer = nil;
            *livePort = MACH_PORT_NULL;
            return NO;
        }
        if (MACH_RCV_TOO_LARGE != ret) break;
        buffer_size = round_msg(msg->msgh_size + MAX_TRAILER_SIZE);
        if (originalBuffer) *buffer = nil;
        originalBuffer = NO;
        *buffer = RSAllocatorReallocate(RSAllocatorSystemDefault, *buffer, buffer_size);
    }
    HALT;
    return NO;
}

#elif DEPLOYMENT_TARGET_WINDOWS

#define TIMEOUT_INFINITY INFINITE

// pass in either a portSet or onePort
static BOOL __RSRunLoopWaitForMultipleObjects(__RSPortSet portSet, HANDLE *onePort, DWORD timeout, DWORD mask, HANDLE *livePort, BOOL *msgReceived) {
    DWORD waitResult = WAIT_TIMEOUT;
    HANDLE handleBuf[MAXIMUM_WAIT_OBJECTS];
    HANDLE *handles = nil;
    uint32_t handleCount = 0;
    BOOL freeHandles = NO;
    BOOL result = NO;
    
    if (portSet) {
        // copy out the handles to be safe from other threads at work
        handles = __RSPortSetGetPorts(portSet, handleBuf, MAXIMUM_WAIT_OBJECTS, &handleCount);
        freeHandles = (handles != handleBuf);
    } else {
        handles = onePort;
        handleCount = 1;
        freeHandles = NO;
    }
    
    // The run loop mode and loop are already in proper unlocked state from caller
    waitResult = MsgWaitForMultipleObjectsEx(__RSMin(handleCount, MAXIMUM_WAIT_OBJECTS), handles, timeout, mask, MWMO_INPUTAVAILABLE);
    
    RSAssert2(waitResult != WAIT_FAILED, __RSLogAssertion, "%s(): error %d from MsgWaitForMultipleObjects", __PRETTY_FUNCTION__, GetLastError());
    
    if (waitResult == WAIT_TIMEOUT) {
        // do nothing, just return to caller
        result = NO;
    } else if (waitResult >= WAIT_OBJECT_0 && waitResult < WAIT_OBJECT_0+handleCount) {
        // a handle was signaled
        if (livePort) *livePort = handles[waitResult-WAIT_OBJECT_0];
        result = YES;
    } else if (waitResult == WAIT_OBJECT_0+handleCount) {
        // windows message received
        if (msgReceived) *msgReceived = YES;
        result = YES;
    } else if (waitResult >= WAIT_ABANDONED_0 && waitResult < WAIT_ABANDONED_0+handleCount) {
        // an "abandoned mutex object"
        if (livePort) *livePort = handles[waitResult-WAIT_ABANDONED_0];
        result = YES;
    } else {
        RSAssert2(waitResult == WAIT_FAILED, __RSLogAssertion, "%s(): unexpected result from MsgWaitForMultipleObjects: %d", __PRETTY_FUNCTION__, waitResult);
        result = NO;
    }
    
    if (freeHandles) {
        RSAllocatorDeallocate(RSAllocatorSystemDefault, handles);
    }
    
    return result;
}

#endif

struct __timeout_context {
    dispatch_source_t ds;
    RSRunLoopRef rl;
    uint64_t termTSR;
};

static void __RSRunLoopTimeoutCancel(void *arg) {
    struct __timeout_context *context = (struct __timeout_context *)arg;
    RSRelease(context->rl);
    dispatch_release(context->ds);
    RSAllocatorDeallocate(RSAllocatorSystemDefault, context);
}

static void __RSRunLoopTimeout(void *arg) {
    struct __timeout_context *context = (struct __timeout_context *)arg;
    context->termTSR = 0ULL;
    RSRUNLOOP_WAKEUP_FOR_TIMEOUT();
    RSRunLoopWakeUp(context->rl);
    // The interval is DISPATCH_TIME_FOREVER, so this won't fire again
}

/* rl, rlm are locked on entrance and exit */
static int32_t __RSRunLoopRun(RSRunLoopRef rl, RSRunLoopModeRef rlm, RSTimeInterval seconds, BOOL stopAfterHandle, RSRunLoopModeRef previousMode) {
    uint64_t startTSR = mach_absolute_time();
    
    if (__RSRunLoopIsStopped(rl)) {
        __RSRunLoopUnsetStopped(rl);
        return RSRunLoopRunStopped;
    } else if (rlm->_stopped) {
        rlm->_stopped = NO;
        return RSRunLoopRunStopped;
    }
    
    mach_port_name_t dispatchPort = MACH_PORT_NULL;
    BOOL libdispatchQSafe = pthread_main_np() && ((HANDLE_DISPATCH_ON_BASE_INVOCATION_ONLY && nil == previousMode) || (!HANDLE_DISPATCH_ON_BASE_INVOCATION_ONLY && 0 == _RSGetTSD(__RSTSDKeyIsInGCDMainQ)));
    if (libdispatchQSafe && (RSRunLoopGetMain() == rl) && RSSetContainsValue(rl->_commonModes, rlm->_name)) dispatchPort = _dispatch_get_main_queue_port_4CF();
    
#if USE_DISPATCH_SOURCE_FOR_TIMERS
    mach_port_name_t modeQueuePort = MACH_PORT_NULL;
    if (rlm->_queue) {
        modeQueuePort = _dispatch_runloop_root_queue_get_port_4CF(rlm->_queue);
        if (!modeQueuePort) {
            CRASH("Unable to get port for run loop mode queue (%d)", -1);
        }
    }
#endif
    
    dispatch_source_t timeout_timer = nil;
    struct __timeout_context *timeout_context = (struct __timeout_context *)RSAllocatorAllocate(RSAllocatorSystemDefault, sizeof(*timeout_context));
    if (seconds <= 0.0) { // instant timeout
        seconds = 0.0;
        timeout_context->termTSR = 0ULL;
    } else if (seconds <= TIMER_INTERVAL_LIMIT) {
        dispatch_queue_t queue = dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_HIGH, 2);
        timeout_timer = dispatch_source_create(DISPATCH_SOURCE_TYPE_TIMER, 0, 0, queue);
        dispatch_retain(timeout_timer);
        timeout_context->ds = timeout_timer;
        timeout_context->rl = (RSRunLoopRef)RSRetain(rl);
        timeout_context->termTSR = startTSR + __RSTimeIntervalToTSR(seconds);
        dispatch_set_context(timeout_timer, timeout_context); // source gets ownership of context
        dispatch_source_set_event_handler_f(timeout_timer, __RSRunLoopTimeout);
        dispatch_source_set_cancel_handler_f(timeout_timer, __RSRunLoopTimeoutCancel);
        uint64_t ns_at = (uint64_t)((__RSTSRToTimeInterval(startTSR) + seconds) * 1000000000ULL);
        dispatch_source_set_timer(timeout_timer, dispatch_time(1, ns_at), DISPATCH_TIME_FOREVER, 1000ULL);
        dispatch_resume(timeout_timer);
    } else { // infinite timeout
        seconds = 9999999999.0;
        timeout_context->termTSR = UINT64_MAX;
    }
    
    BOOL didDispatchPortLastTime = YES;
    int32_t retVal = 0;
    do {
        RSAutoreleasePoolRef pool = RSAutoreleasePoolCreate(RSAllocatorSystemDefault);
        uint8_t msg_buffer[3 * 1024];
#if DEPLOYMENT_TARGET_MACOSX || DEPLOYMENT_TARGET_EMBEDDED || DEPLOYMENT_TARGET_EMBEDDED_MINI
        mach_msg_header_t *msg = nil;
        mach_port_t livePort = MACH_PORT_NULL;
#elif DEPLOYMENT_TARGET_WINDOWS
        HANDLE livePort = nil;
        BOOL windowsMessageReceived = NO;
#endif
        __RSPortSet waitSet = rlm->_portSet;
        
        __RSRunLoopUnsetIgnoreWakeUps(rl);
        
        if (rlm->_observerMask & RSRunLoopBeforeTimers) __RSRunLoopDoObservers(rl, rlm, RSRunLoopBeforeTimers);
        if (rlm->_observerMask & RSRunLoopBeforeSources) __RSRunLoopDoObservers(rl, rlm, RSRunLoopBeforeSources);
        
        __RSRunLoopDoBlocks(rl, rlm);
        
        BOOL sourceHandledThisLoop = __RSRunLoopDoSources0(rl, rlm, stopAfterHandle);
        if (sourceHandledThisLoop) {
            __RSRunLoopDoBlocks(rl, rlm);
        }
        
        BOOL poll = sourceHandledThisLoop || (0ULL == timeout_context->termTSR);
        
        if (MACH_PORT_NULL != dispatchPort && !didDispatchPortLastTime) {
#if DEPLOYMENT_TARGET_MACOSX || DEPLOYMENT_TARGET_EMBEDDED || DEPLOYMENT_TARGET_EMBEDDED_MINI
            msg = (mach_msg_header_t *)msg_buffer;
            if (__RSRunLoopServiceMachPort(dispatchPort, &msg, sizeof(msg_buffer), &livePort, 0)) {
                goto handle_msg;
            }
#elif DEPLOYMENT_TARGET_WINDOWS
            if (__RSRunLoopWaitForMultipleObjects(nil, &dispatchPort, 0, 0, &livePort, nil)) {
                goto handle_msg;
            }
#endif
        }
        
        didDispatchPortLastTime = NO;
        
        if (!poll && (rlm->_observerMask & RSRunLoopBeforeWaiting)) __RSRunLoopDoObservers(rl, rlm, RSRunLoopBeforeWaiting);
        __RSRunLoopSetSleeping(rl);
        // do not do any user callouts after this point (after notifying of sleeping)
        
        // Must push the local-to-this-activation ports in on every loop
        // iteration, as this mode could be run re-entrantly and we don't
        // want these ports to get serviced.
        
        __RSPortSetInsert(dispatchPort, waitSet);
        
        __RSRunLoopModeUnlock(rlm);
        __RSRunLoopUnlock(rl);
        
#if DEPLOYMENT_TARGET_MACOSX || DEPLOYMENT_TARGET_EMBEDDED || DEPLOYMENT_TARGET_EMBEDDED_MINI
#if USE_DISPATCH_SOURCE_FOR_TIMERS
        do {
            if (RSUseCollectableAllocator) {
//                objc_clear_stack(0);
                memset(msg_buffer, 0, sizeof(msg_buffer));
            }
            msg = (mach_msg_header_t *)msg_buffer;
            __RSRunLoopServiceMachPort(waitSet, &msg, sizeof(msg_buffer), &livePort, poll ? 0 : TIMEOUT_INFINITY);
            
            if (modeQueuePort != MACH_PORT_NULL && livePort == modeQueuePort) {
                // Drain the internal queue. If one of the callout blocks sets the timerFired flag, break out and service the timer.
                while (_dispatch_runloop_root_queue_perform_4CF(rlm->_queue));
                if (rlm->_timerFired) {
                    // Leave livePort as the queue port, and service timers below
                    rlm->_timerFired = NO;
                    break;
                } else {
                    if (msg && msg != (mach_msg_header_t *)msg_buffer) RSAllocatorDeallocate(RSAllocatorSystemDefault, msg);
                }
            } else {
                // Go ahead and leave the inner loop.
                break;
            }
        } while (1);
#else
        if (RSUseCollectableAllocator) {
            objc_clear_stack(0);
            memset(msg_buffer, 0, sizeof(msg_buffer));
        }
        msg = (mach_msg_header_t *)msg_buffer;
        __RSRunLoopServiceMachPort(waitSet, &msg, sizeof(msg_buffer), &livePort, poll ? 0 : TIMEOUT_INFINITY);
#endif
        
        
#elif DEPLOYMENT_TARGET_WINDOWS
        // Here, use the app-supplied message queue mask. They will set this if they are interested in having this run loop receive windows messages.
        __RSRunLoopWaitForMultipleObjects(waitSet, nil, poll ? 0 : TIMEOUT_INFINITY, rlm->_msgQMask, &livePort, &windowsMessageReceived);
#endif
        
        __RSRunLoopLock(rl);
        __RSRunLoopModeLock(rlm);
        
        // Must remove the local-to-this-activation ports in on every loop
        // iteration, as this mode could be run re-entrantly and we don't
        // want these ports to get serviced. Also, we don't want them left
        // in there if this function returns.
        
        __RSPortSetRemove(dispatchPort, waitSet);
        
        __RSRunLoopSetIgnoreWakeUps(rl);
        
        // user callouts now OK again
        __RSRunLoopUnsetSleeping(rl);
        if (!poll && (rlm->_observerMask & RSRunLoopAfterWaiting)) __RSRunLoopDoObservers(rl, rlm, RSRunLoopAfterWaiting);
        
    handle_msg:;
        __RSRunLoopSetIgnoreWakeUps(rl);
        
#if DEPLOYMENT_TARGET_WINDOWS
        if (windowsMessageReceived) {
            // These Win32 APIs cause a callout, so make sure we're unlocked first and relocked after
            __RSRunLoopModeUnlock(rlm);
            __RSRunLoopUnlock(rl);
            
            if (rlm->_msgPump) {
                rlm->_msgPump();
            } else {
                MSG msg;
                if (PeekMessage(&msg, nil, 0, 0, PM_REMOVE | PM_NOYIELD)) {
                    TranslateMessage(&msg);
                    DispatchMessage(&msg);
                }
            }
            
            __RSRunLoopLock(rl);
            __RSRunLoopModeLock(rlm);
            sourceHandledThisLoop = YES;
            
            // To prevent starvation of sources other than the message queue, we check again to see if any other sources need to be serviced
            // Use 0 for the mask so windows messages are ignored this time. Also use 0 for the timeout, because we're just checking to see if the things are signalled right now -- we will wait on them again later.
            // NOTE: Ignore the dispatch source (it's not in the wait set anymore) and also don't run the observers here since we are polling.
            __RSRunLoopSetSleeping(rl);
            __RSRunLoopModeUnlock(rlm);
            __RSRunLoopUnlock(rl);
            
            __RSRunLoopWaitForMultipleObjects(waitSet, nil, 0, 0, &livePort, nil);
            
            __RSRunLoopLock(rl);
            __RSRunLoopModeLock(rlm);
            __RSRunLoopUnsetSleeping(rl);
            // If we have a new live port then it will be handled below as normal
        }
        
        
#endif
        if (MACH_PORT_NULL == livePort) {
            RSRUNLOOP_WAKEUP_FOR_NOTHING();
            // handle nothing
        } else if (livePort == rl->_wakeUpPort) {
            RSRUNLOOP_WAKEUP_FOR_WAKEUP();
            // do nothing on Mac OS
#if DEPLOYMENT_TARGET_WINDOWS
            // Always reset the wake up port, or risk spinning forever
            ResetEvent(rl->_wakeUpPort);
#endif
        }
#if USE_DISPATCH_SOURCE_FOR_TIMERS
        else if (modeQueuePort != MACH_PORT_NULL && livePort == modeQueuePort) {
            RSRUNLOOP_WAKEUP_FOR_TIMER();
            if (!__RSRunLoopDoTimers(rl, rlm, mach_absolute_time())) {
                // Re-arm the next timer, because we apparently fired early
                __RSArmNextTimerInMode(rlm, rl);
            }
        }
#endif
#if USE_MK_TIMER_TOO
        else if (rlm->_timerPort != MACH_PORT_NULL && livePort == rlm->_timerPort) {
            RSRUNLOOP_WAKEUP_FOR_TIMER();
            // On Windows, we have observed an issue where the timer port is set before the time which we requested it to be set. For example, we set the fire time to be TSR 167646765860, but it is actually observed firing at TSR 167646764145, which is 1715 ticks early. The result is that, when __RSRunLoopDoTimers checks to see if any of the run loop timers should be firing, it appears to be 'too early' for the next timer, and no timers are handled.
            // In this case, the timer port has been automatically reset (since it was returned from MsgWaitForMultipleObjectsEx), and if we do not re-arm it, then no timers will ever be serviced again unless something adjusts the timer list (e.g. adding or removing timers). The fix for the issue is to reset the timer here if RSRunLoopDoTimers did not handle a timer itself. 9308754
            if (!__RSRunLoopDoTimers(rl, rlm, mach_absolute_time())) {
                // Re-arm the next timer
                __RSArmNextTimerInMode(rlm, rl);
            }
        }
#endif
        else if (livePort == dispatchPort) {
            RSRUNLOOP_WAKEUP_FOR_DISPATCH();
            __RSRunLoopModeUnlock(rlm);
            __RSRunLoopUnlock(rl);
            _RSSetTSD(__RSTSDKeyIsInGCDMainQ, (void *)6, nil);
#if DEPLOYMENT_TARGET_WINDOWS
            void *msg = 0;
#endif
            __RSRUNLOOP_IS_SERVICING_THE_MAIN_DISPATCH_QUEUE__(msg);
            _RSSetTSD(__RSTSDKeyIsInGCDMainQ, (void *)0, nil);
	        __RSRunLoopLock(rl);
	        __RSRunLoopModeLock(rlm);
 	        sourceHandledThisLoop = YES;
            didDispatchPortLastTime = YES;
        } else {
            RSRUNLOOP_WAKEUP_FOR_SOURCE();
            // Despite the name, this works for windows handles as well
            RSRunLoopSourceRef rls = __RSRunLoopModeFindSourceForMachPort(rl, rlm, livePort);
            if (rls) {
#if DEPLOYMENT_TARGET_MACOSX || DEPLOYMENT_TARGET_EMBEDDED || DEPLOYMENT_TARGET_EMBEDDED_MINI
                mach_msg_header_t *reply = nil;
                sourceHandledThisLoop = __RSRunLoopDoSource1(rl, rlm, rls, msg, msg->msgh_size, &reply) || sourceHandledThisLoop;
                if (nil != reply) {
                    (void)mach_msg(reply, MACH_SEND_MSG, reply->msgh_size, 0, MACH_PORT_NULL, 0, MACH_PORT_NULL);
                    RSAllocatorDeallocate(RSAllocatorSystemDefault, reply);
                }
#elif DEPLOYMENT_TARGET_WINDOWS
                sourceHandledThisLoop = __RSRunLoopDoSource1(rl, rlm, rls) || sourceHandledThisLoop;
#endif
            }
        }
#if DEPLOYMENT_TARGET_MACOSX || DEPLOYMENT_TARGET_EMBEDDED || DEPLOYMENT_TARGET_EMBEDDED_MINI
        if (msg && msg != (mach_msg_header_t *)msg_buffer) RSAllocatorDeallocate(RSAllocatorSystemDefault, msg);
#endif
        
        __RSRunLoopDoBlocks(rl, rlm);
        
        
        if (sourceHandledThisLoop && stopAfterHandle) {
            retVal = RSRunLoopRunHandledSource;
        } else if (timeout_context->termTSR < mach_absolute_time()) {
            retVal = RSRunLoopRunTimedOut;
        } else if (__RSRunLoopIsStopped(rl)) {
            __RSRunLoopUnsetStopped(rl);
            retVal = RSRunLoopRunStopped;
        } else if (rlm->_stopped) {
            rlm->_stopped = NO;
            retVal = RSRunLoopRunStopped;
        } else if (__RSRunLoopModeIsEmpty(rl, rlm, previousMode)) {
            retVal = RSRunLoopRunFinished;
        }
    
        RSAutoreleasePoolDrain(pool);   // RSAutoreleasePoolDrain will active __runloop_vm_pressure_handler automatically.
    } while (0 == retVal);
    
    if (timeout_timer) {
        dispatch_source_cancel(timeout_timer);
        dispatch_release(timeout_timer);
    } else {
        RSAllocatorDeallocate(RSAllocatorSystemDefault, timeout_context);
    }
    
    return retVal;
}

RSExport RSUInteger RSRunLoopRunSpecific(RSRunLoopRef rl, RSStringRef modeName, RSTimeInterval seconds, BOOL returnAfterSourceHandled) {     /* DOES CALLOUT */
    CHECK_FOR_FORK();
    if (__RSRunLoopIsDeallocating(rl)) return RSRunLoopRunFinished;
    __RSRunLoopLock(rl);
    RSRunLoopModeRef currentMode = __RSRunLoopFindMode(rl, modeName, NO);
    if (nil == currentMode || __RSRunLoopModeIsEmpty(rl, currentMode, rl->_currentMode)) {
        BOOL did = NO;
        if (currentMode) __RSRunLoopModeUnlock(currentMode);
        __RSRunLoopUnlock(rl);
        return did ? RSRunLoopRunHandledSource : RSRunLoopRunFinished;
    }
    volatile _per_run_data *previousPerRun = __RSRunLoopPushPerRunData(rl);
    RSRunLoopModeRef previousMode = rl->_currentMode;
    rl->_currentMode = currentMode;
    RSUInteger result = RSRunLoopRunFinished;
    
	if (currentMode->_observerMask & RSRunLoopEntry ) __RSRunLoopDoObservers(rl, currentMode, RSRunLoopEntry);
	result = __RSRunLoopRun(rl, currentMode, seconds, returnAfterSourceHandled, previousMode);
	if (currentMode->_observerMask & RSRunLoopExit ) __RSRunLoopDoObservers(rl, currentMode, RSRunLoopExit);
    
    __RSRunLoopModeUnlock(currentMode);
    __RSRunLoopPopPerRunData(rl, previousPerRun);
	rl->_currentMode = previousMode;
    __RSRunLoopUnlock(rl);
    return result;
}

RSExport void RSRunLoopRun(void) {	/* DOES CALLOUT */
    RSUInteger result;
    do {
        result = RSRunLoopRunSpecific(RSRunLoopGetCurrent(), RSRunLoopDefaultMode, 1.0e10, NO);
        CHECK_FOR_FORK();
    } while (RSRunLoopRunStopped != result && RSRunLoopRunFinished != result);
}

RSExport RSUInteger RSRunLoopRunInMode(RSStringRef modeName, RSTimeInterval seconds, BOOL returnAfterSourceHandled) {     /* DOES CALLOUT */
    CHECK_FOR_FORK();
    return RSRunLoopRunSpecific(RSRunLoopGetCurrent(), modeName, seconds, returnAfterSourceHandled);
}

RSAbsoluteTime RSRunLoopGetNextTimerFireDate(RSRunLoopRef rl, RSStringRef modeName) {
    CHECK_FOR_FORK();
    __RSRunLoopLock(rl);
    RSRunLoopModeRef rlm = __RSRunLoopFindMode(rl, modeName, NO);
    RSAbsoluteTime at = 0.0;
    RSRunLoopTimerRef nextTimer = (rlm && rlm->_timers && 0 < RSArrayGetCount(rlm->_timers)) ? (RSRunLoopTimerRef)RSArrayObjectAtIndex(rlm->_timers, 0) : nil;
    if (nextTimer) {
        at = RSRunLoopTimerGetNextFireDate(nextTimer);
    }
    if (rlm) __RSRunLoopModeUnlock(rlm);
    __RSRunLoopUnlock(rl);
    return at;
}

BOOL RSRunLoopIsWaiting(RSRunLoopRef rl) {
    CHECK_FOR_FORK();
    return __RSRunLoopIsSleeping(rl);
}

void RSRunLoopWakeUp(RSRunLoopRef rl) {
    CHECK_FOR_FORK();
    // This lock is crucial to ignorable wakeups, do not remove it.
    __RSRunLoopLock(rl);
    if (__RSRunLoopIsIgnoringWakeUps(rl)) {
        __RSRunLoopUnlock(rl);
        return;
    }
#if DEPLOYMENT_TARGET_MACOSX || DEPLOYMENT_TARGET_EMBEDDED || DEPLOYMENT_TARGET_EMBEDDED_MINI
    kern_return_t ret;
    /* We unconditionally try to send the message, since we don't want
     * to lose a wakeup, but the send may fail if there is already a
     * wakeup pending, since the queue length is 1. */
    ret = __RSSendTrivialMachMessage(rl->_wakeUpPort, 0, MACH_SEND_TIMEOUT, 0);
    if (ret != MACH_MSG_SUCCESS && ret != MACH_SEND_TIMED_OUT) CRASH("*** Unable to send message to wake up port. (%d) ***", ret);
#elif DEPLOYMENT_TARGET_WINDOWS
    SetEvent(rl->_wakeUpPort);
#endif
    __RSRunLoopUnlock(rl);
}

void RSRunLoopStop(RSRunLoopRef rl) {
    BOOL doWake = NO;
    CHECK_FOR_FORK();
    __RSRunLoopLock(rl);
    if (rl->_currentMode) {
        __RSRunLoopSetStopped(rl);
        doWake = YES;
    }
    __RSRunLoopUnlock(rl);
    if (doWake) {
        RSRunLoopWakeUp(rl);
    }
}

RSExport void _RSRunLoopStopMode(RSRunLoopRef rl, RSStringRef modeName) {
    CHECK_FOR_FORK();
    RSRunLoopModeRef rlm;
    __RSRunLoopLock(rl);
    rlm = __RSRunLoopFindMode(rl, modeName, YES);
    if (nil != rlm) {
        rlm->_stopped = YES;
        __RSRunLoopModeUnlock(rlm);
    }
    __RSRunLoopUnlock(rl);
    RSRunLoopWakeUp(rl);
}

RSExport BOOL _RSRunLoopModeContainsMode(RSRunLoopRef rl, RSStringRef modeName, RSStringRef candidateContainedName) {
    CHECK_FOR_FORK();
    return NO;
}

void RSRunLoopPerformBlock(RSRunLoopRef rl, RSTypeRef mode, void (^block)(void)) {
    CHECK_FOR_FORK();
    if (RSStringGetTypeID() == RSGetTypeID(mode)) {
        mode = RSStringCreateCopy(RSAllocatorSystemDefault, (RSStringRef)mode);
        __RSRunLoopLock(rl);
        // ensure mode exists
        RSRunLoopModeRef currentMode = __RSRunLoopFindMode(rl, (RSStringRef)mode, YES);
        if (currentMode) __RSRunLoopModeUnlock(currentMode);
        __RSRunLoopUnlock(rl);
    } else if (RSArrayGetTypeID() == RSGetTypeID(mode)) {
        RSIndex cnt = RSArrayGetCount((RSArrayRef)mode);
        const void **values = (const void **)RSAllocatorAllocate(RSAllocatorSystemDefault, sizeof(const void *) * cnt);
        RSArrayGetObjects((RSArrayRef)mode, RSMakeRange(0, cnt), values);
        mode = RSSetCreate(RSAllocatorSystemDefault, values, cnt, &RSTypeSetCallBacks);
        __RSRunLoopLock(rl);
        // ensure modes exist
        for (RSIndex idx = 0; idx < cnt; idx++) {
            RSRunLoopModeRef currentMode = __RSRunLoopFindMode(rl, (RSStringRef)values[idx], YES);
            if (currentMode) __RSRunLoopModeUnlock(currentMode);
        }
        __RSRunLoopUnlock(rl);
        RSAllocatorDeallocate(RSAllocatorSystemDefault, values);
    } else if (RSSetGetTypeID() == RSGetTypeID(mode)) {
        RSIndex cnt = RSSetGetCount((RSSetRef)mode);
        const void **values = (const void **)RSAllocatorAllocate(RSAllocatorSystemDefault, sizeof(const void *) * cnt);
        RSSetGetValues((RSSetRef)mode, values);
        mode = RSSetCreate(RSAllocatorSystemDefault, values, cnt, &RSTypeSetCallBacks);
        __RSRunLoopLock(rl);
        // ensure modes exist
        for (RSIndex idx = 0; idx < cnt; idx++) {
            RSRunLoopModeRef currentMode = __RSRunLoopFindMode(rl, (RSStringRef)values[idx], YES);
            if (currentMode) __RSRunLoopModeUnlock(currentMode);
        }
        __RSRunLoopUnlock(rl);
        RSAllocatorDeallocate(RSAllocatorSystemDefault, values);
    } else {
        mode = nil;
    }
    block = Block_copy(block);
    if (!mode || !block) {
        if (mode) RSRelease(mode);
        if (block) Block_release(block);
        return;
    }
    __RSRunLoopLock(rl);
    struct _block_item *new_item = (struct _block_item *)RSAllocatorAllocate(RSAllocatorSystemDefault, sizeof(struct _block_item));
    new_item->_next = nil;
    new_item->_mode = mode;
    new_item->_block = block;
    if (!rl->_blocks_tail) {
        rl->_blocks_head = new_item;
    } else {
        rl->_blocks_tail->_next = new_item;
    }
    rl->_blocks_tail = new_item;
    __RSRunLoopUnlock(rl);
}

BOOL RSRunLoopContainsSource(RSRunLoopRef rl, RSRunLoopSourceRef rls, RSStringRef modeName) {
    CHECK_FOR_FORK();
    RSRunLoopModeRef rlm;
    BOOL hasValue = NO;
    __RSRunLoopLock(rl);
    if (modeName == RSRunLoopCommonModes) {
        if (nil != rl->_commonModeItems) {
            hasValue = RSSetContainsValue(rl->_commonModeItems, rls);
        }
    } else {
        rlm = __RSRunLoopFindMode(rl, modeName, NO);
        if (nil != rlm) {
            hasValue = (rlm->_sources0 ? RSSetContainsValue(rlm->_sources0, rls) : NO) || (rlm->_sources1 ? RSSetContainsValue(rlm->_sources1, rls) : NO);
            __RSRunLoopModeUnlock(rlm);
        }
    }
    __RSRunLoopUnlock(rl);
    return hasValue;
}

void RSRunLoopAddSource(RSRunLoopRef rl, RSRunLoopSourceRef rls, RSStringRef modeName) {	/* DOES CALLOUT */
    CHECK_FOR_FORK();
    if (__RSRunLoopIsDeallocating(rl)) return;
    if (!__RSRunLoopIsValid(rls)) return;
    BOOL doVer0Callout = NO;
    __RSRunLoopLock(rl);
    if (modeName == RSRunLoopCommonModes) {
        RSSetRef set = rl->_commonModes ? RSSetCreateCopy(RSAllocatorSystemDefault, rl->_commonModes) : nil;
        if (nil == rl->_commonModeItems) {
            rl->_commonModeItems = RSSetCreateMutable(RSAllocatorSystemDefault, 0, &RSTypeSetCallBacks);
        }
        RSSetAddValue(rl->_commonModeItems, rls);
        if (nil != set) {
            RSTypeRef context[2] = {rl, rls};
            /* add new item to all common-modes */
            RSSetApplyFunction(set, (__RSRunLoopAddItemToCommonModes), (void *)context);
            RSRelease(set);
        }
    } else {
        RSRunLoopModeRef rlm = __RSRunLoopFindMode(rl, modeName, YES);
        if (nil != rlm && nil == rlm->_sources0) {
            rlm->_sources0 = RSSetCreateMutable(RSAllocatorSystemDefault, 0, &RSTypeSetCallBacks);
            rlm->_sources1 = RSSetCreateMutable(RSAllocatorSystemDefault, 0, &RSTypeSetCallBacks);
            rlm->_portToV1SourceMap = RSDictionaryCreateMutable(RSAllocatorSystemDefault, 0, nil);
        }
        if (nil != rlm && !RSSetContainsValue(rlm->_sources0, rls) && !RSSetContainsValue(rlm->_sources1, rls)) {
            if (0 == rls->_context.version0.version) {
                RSSetAddValue(rlm->_sources0, rls);
            } else if (1 == rls->_context.version0.version) {
                RSSetAddValue(rlm->_sources1, rls);
                __RSPort src_port = rls->_context.version1.getPort(rls->_context.version1.info);
                if (RSPORT_NULL != src_port) {
                    RSDictionarySetValue(rlm->_portToV1SourceMap, (const void *)(uintptr_t)src_port, rls);
                    __RSPortSetInsert(src_port, rlm->_portSet);
                }
            }
            __RSRunLoopSourceLock(rls);
            if (nil == rls->_runLoops) {
                rls->_runLoops = RSBagCreateMutable(RSAllocatorSystemDefault, 0, &RSTypeBagCallBacks); // sources retain run loops!
            }
            RSBagAddValue(rls->_runLoops, rl);
            __RSRunLoopSourceUnlock(rls);
            if (0 == rls->_context.version0.version) {
                if (nil != rls->_context.version0.schedule) {
                    doVer0Callout = YES;
                }
            }
        }
        if (nil != rlm) {
            __RSRunLoopModeUnlock(rlm);
        }
    }
    __RSRunLoopUnlock(rl);
    if (doVer0Callout) {
        // although it looses some protection for the source, we have no choice but
        // to do this after unlocking the run loop and mode locks, to avoid deadlocks
        // where the source wants to take a lock which is already held in another
        // thread which is itself waiting for a run loop/mode lock
        rls->_context.version0.schedule(rls->_context.version0.info, rl, modeName);	/* CALLOUT */
    }
}

void RSRunLoopRemoveSource(RSRunLoopRef rl, RSRunLoopSourceRef rls, RSStringRef modeName) {	/* DOES CALLOUT */
    CHECK_FOR_FORK();
    BOOL doVer0Callout = NO, doRLSRelease = NO;
    __RSRunLoopLock(rl);
    if (modeName == RSRunLoopCommonModes) {
        if (nil != rl->_commonModeItems && RSSetContainsValue(rl->_commonModeItems, rls)) {
            RSSetRef set = rl->_commonModes ? RSSetCreateCopy(RSAllocatorSystemDefault, rl->_commonModes) : nil;
            RSSetRemoveValue(rl->_commonModeItems, rls);
            if (nil != set) {
                RSTypeRef context[2] = {rl, rls};
                /* remove new item from all common-modes */
                RSSetApplyFunction(set, (__RSRunLoopRemoveItemFromCommonModes), (void *)context);
                RSRelease(set);
            }
        } else {
        }
    } else {
        RSRunLoopModeRef rlm = __RSRunLoopFindMode(rl, modeName, NO);
        if (nil != rlm && ((nil != rlm->_sources0 && RSSetContainsValue(rlm->_sources0, rls)) || (nil != rlm->_sources1 && RSSetContainsValue(rlm->_sources1, rls)))) {
            RSRetain(rls);
            if (1 == rls->_context.version0.version) {
                __RSPort src_port = rls->_context.version1.getPort(rls->_context.version1.info);
                if (RSPORT_NULL != src_port) {
                    RSDictionaryRemoveValue(rlm->_portToV1SourceMap, (const void *)(uintptr_t)src_port);
                    __RSPortSetRemove(src_port, rlm->_portSet);
                }
            }
            RSSetRemoveValue(rlm->_sources0, rls);
            RSSetRemoveValue(rlm->_sources1, rls);
            __RSRunLoopSourceLock(rls);
            if (nil != rls->_runLoops) {
                RSBagRemoveValue(rls->_runLoops, rl);
            }
            __RSRunLoopSourceUnlock(rls);
            if (0 == rls->_context.version0.version) {
                if (nil != rls->_context.version0.cancel) {
                    doVer0Callout = YES;
                }
            }
            doRLSRelease = YES;
        }
        if (nil != rlm) {
            __RSRunLoopModeUnlock(rlm);
        }
    }
    __RSRunLoopUnlock(rl);
    if (doVer0Callout) {
        // although it looses some protection for the source, we have no choice but
        // to do this after unlocking the run loop and mode locks, to avoid deadlocks
        // where the source wants to take a lock which is already held in another
        // thread which is itself waiting for a run loop/mode lock
        rls->_context.version0.cancel(rls->_context.version0.info, rl, modeName);	/* CALLOUT */
    }
    if (doRLSRelease) RSRelease(rls);
}

static void __RSRunLoopRemoveSourcesFromCommonMode(const void *value, void *ctx) {
    RSStringRef modeName = (RSStringRef)value;
    RSRunLoopRef rl = (RSRunLoopRef)ctx;
    __RSRunLoopRemoveAllSources(rl, modeName);
}

static void __RSRunLoopRemoveSourceFromMode(const void *value, void *ctx) {
    RSRunLoopSourceRef rls = (RSRunLoopSourceRef)value;
    RSRunLoopRef rl = (RSRunLoopRef)(((RSTypeRef *)ctx)[0]);
    RSStringRef modeName = (RSStringRef)(((RSTypeRef *)ctx)[1]);
    RSRunLoopRemoveSource(rl, rls, modeName);
}

static void __RSRunLoopRemoveAllSources(RSRunLoopRef rl, RSStringRef modeName) {
    CHECK_FOR_FORK();
    RSRunLoopModeRef rlm;
    __RSRunLoopLock(rl);
    if (modeName == RSRunLoopCommonModes) {
        if (nil != rl->_commonModeItems) {
            RSSetRef set = rl->_commonModes ? RSSetCreateCopy(RSAllocatorSystemDefault, rl->_commonModes) : nil;
            if (nil != set) {
                RSSetApplyFunction(set, (__RSRunLoopRemoveSourcesFromCommonMode), (void *)rl);
                RSRelease(set);
            }
        } else {
        }
    } else {
        rlm = __RSRunLoopFindMode(rl, modeName, NO);
        if (nil != rlm && nil != rlm->_sources0) {
            RSSetRef set = RSSetCreateCopy(RSAllocatorSystemDefault, rlm->_sources0);
            RSTypeRef context[2] = {rl, modeName};
            RSSetApplyFunction(set, (__RSRunLoopRemoveSourceFromMode), (void *)context);
            RSRelease(set);
        }
        if (nil != rlm && nil != rlm->_sources1) {
            RSSetRef set = RSSetCreateCopy(RSAllocatorSystemDefault, rlm->_sources1);
            RSTypeRef context[2] = {rl, modeName};
            RSSetApplyFunction(set, (__RSRunLoopRemoveSourceFromMode), (void *)context);
            RSRelease(set);
        }
        if (nil != rlm) {
            __RSRunLoopModeUnlock(rlm);
        }
    }
    __RSRunLoopUnlock(rl);
}

BOOL RSRunLoopContainsObserver(RSRunLoopRef rl, RSRunLoopObserverRef rlo, RSStringRef modeName) {
    CHECK_FOR_FORK();
    RSRunLoopModeRef rlm;
    BOOL hasValue = NO;
    __RSRunLoopLock(rl);
    if (modeName == RSRunLoopCommonModes) {
        if (nil != rl->_commonModeItems) {
            hasValue = RSSetContainsValue(rl->_commonModeItems, rlo);
        }
    } else {
        rlm = __RSRunLoopFindMode(rl, modeName, NO);
        if (nil != rlm && nil != rlm->_observers) {
            hasValue = RSArrayContainsObject(rlm->_observers, RSMakeRange(0, RSArrayGetCount(rlm->_observers)), rlo);
        }
        if (nil != rlm) {
            __RSRunLoopModeUnlock(rlm);
        }
    }
    __RSRunLoopUnlock(rl);
    return hasValue;
}

void RSRunLoopAddObserver(RSRunLoopRef rl, RSRunLoopObserverRef rlo, RSStringRef modeName) {
    CHECK_FOR_FORK();
    RSRunLoopModeRef rlm;
    if (__RSRunLoopIsDeallocating(rl)) return;
    if (!__RSRunLoopIsValid(rlo) || (nil != rlo->_runLoop && rlo->_runLoop != rl)) return;
    __RSRunLoopLock(rl);
    if (modeName == RSRunLoopCommonModes) {
        RSSetRef set = rl->_commonModes ? RSSetCreateCopy(RSAllocatorSystemDefault, rl->_commonModes) : nil;
        if (nil == rl->_commonModeItems) {
            rl->_commonModeItems = RSSetCreateMutable(RSAllocatorSystemDefault, 0, &RSTypeSetCallBacks);
        }
        RSSetAddValue(rl->_commonModeItems, rlo);
        if (nil != set) {
            RSTypeRef context[2] = {rl, rlo};
            /* add new item to all common-modes */
            RSSetApplyFunction(set, (__RSRunLoopAddItemToCommonModes), (void *)context);
            RSRelease(set);
        }
    } else {
        rlm = __RSRunLoopFindMode(rl, modeName, YES);
        if (nil != rlm && nil == rlm->_observers) {
            rlm->_observers = RSArrayCreateMutable(RSAllocatorSystemDefault, 0);
        }
        if (nil != rlm && !RSArrayContainsObject(rlm->_observers, RSMakeRange(0, RSArrayGetCount(rlm->_observers)), rlo)) {
            BOOL inserted = NO;
            for (RSIndex idx = RSArrayGetCount(rlm->_observers); idx--; ) {
                RSRunLoopObserverRef obs = (RSRunLoopObserverRef)RSArrayObjectAtIndex(rlm->_observers, idx);
                if (obs->_order <= rlo->_order) {
                    RSArrayInsertObjectAtIndex(rlm->_observers, idx + 1, rlo);
                    inserted = YES;
                    break;
                }
            }
            if (!inserted) {
                RSArrayInsertObjectAtIndex(rlm->_observers, 0, rlo);
            }
            rlm->_observerMask |= rlo->_activities;
            __RSRunLoopObserverSchedule(rlo, rl, rlm);
        }
        if (nil != rlm) {
            __RSRunLoopModeUnlock(rlm);
        }
    }
    __RSRunLoopUnlock(rl);
}

void RSRunLoopRemoveObserver(RSRunLoopRef rl, RSRunLoopObserverRef rlo, RSStringRef modeName) {
    CHECK_FOR_FORK();
    RSRunLoopModeRef rlm;
    __RSRunLoopLock(rl);
    if (modeName == RSRunLoopCommonModes) {
        if (nil != rl->_commonModeItems && RSSetContainsValue(rl->_commonModeItems, rlo)) {
            RSSetRef set = rl->_commonModes ? RSSetCreateCopy(RSAllocatorSystemDefault, rl->_commonModes) : nil;
            RSSetRemoveValue(rl->_commonModeItems, rlo);
            if (nil != set) {
                RSTypeRef context[2] = {rl, rlo};
                /* remove new item from all common-modes */
                RSSetApplyFunction(set, (__RSRunLoopRemoveItemFromCommonModes), (void *)context);
                RSRelease(set);
            }
        } else {
        }
    } else {
        rlm = __RSRunLoopFindMode(rl, modeName, NO);
        if (nil != rlm && nil != rlm->_observers) {
            RSRetain(rlo);
            RSIndex idx = RSArrayIndexOfObject(rlm->_observers, rlo);
            if (RSNotFound != idx) {
                RSArrayRemoveObjectAtIndex(rlm->_observers, idx);
                __RSRunLoopObserverCancel(rlo, rl, rlm);
            }
            RSRelease(rlo);
        }
        if (nil != rlm) {
            __RSRunLoopModeUnlock(rlm);
        }
    }
    __RSRunLoopUnlock(rl);
}

BOOL RSRunLoopContainsTimer(RSRunLoopRef rl, RSRunLoopTimerRef rlt, RSStringRef modeName) {
    CHECK_FOR_FORK();
    if (nil == rlt->_runLoop || rl != rlt->_runLoop) return NO;
    BOOL hasValue = NO;
    __RSRunLoopLock(rl);
    if (modeName == RSRunLoopCommonModes) {
        if (nil != rl->_commonModeItems) {
            hasValue = RSSetContainsValue(rl->_commonModeItems, rlt);
        }
    } else {
        RSRunLoopModeRef rlm = __RSRunLoopFindMode(rl, modeName, NO);
        if (nil != rlm) {
            if (nil != rlm->_timers) {
                RSIndex idx = RSArrayIndexOfObject(rlm->_timers, rlt);
                hasValue = (RSNotFound != idx);
            }
            __RSRunLoopModeUnlock(rlm);
        }
    }
    __RSRunLoopUnlock(rl);
    return hasValue;
}

void RSRunLoopAddTimer(RSRunLoopRef rl, RSRunLoopTimerRef rlt, RSStringRef modeName) {
    CHECK_FOR_FORK();
    if (__RSRunLoopIsDeallocating(rl)) return;
    if (!__RSRunLoopIsValid(rlt) || (nil != rlt->_runLoop && rlt->_runLoop != rl)) return;
    __RSRunLoopLock(rl);
    if (modeName == RSRunLoopCommonModes) {
        RSSetRef set = rl->_commonModes ? RSSetCreateCopy(RSAllocatorSystemDefault, rl->_commonModes) : nil;
        if (nil == rl->_commonModeItems) {
            rl->_commonModeItems = RSSetCreateMutable(RSAllocatorSystemDefault, 0, &RSTypeSetCallBacks);
        }
        RSSetAddValue(rl->_commonModeItems, rlt);
        if (nil != set) {
            RSTypeRef context[2] = {rl, rlt};
            /* add new item to all common-modes */
            RSSetApplyFunction(set, (__RSRunLoopAddItemToCommonModes), (void *)context);
            RSRelease(set);
        }
    } else {
        RSRunLoopModeRef rlm = __RSRunLoopFindMode(rl, modeName, YES);
        if (nil != rlm) {
            if (nil == rlm->_timers) {
                extern RSArrayCallBacks RSTypeArrayCallBacks;
                RSArrayCallBacks cb = RSTypeArrayCallBacks;
                cb.equal = nil;
                rlm->_timers = __RSArrayCreateMutable0(RSAllocatorSystemDefault, 0, &cb);
            }
        }
        if (nil != rlm && !RSSetContainsValue(rlt->_rlModes, rlm->_name)) {
            __RSRunLoopTimerLock(rlt);
            if (nil == rlt->_runLoop) {
                rlt->_runLoop = rl;
            } else if (rl != rlt->_runLoop) {
                __RSRunLoopTimerUnlock(rlt);
                __RSRunLoopModeUnlock(rlm);
                __RSRunLoopUnlock(rl);
                return;
            }
            RSSetAddValue(rlt->_rlModes, rlm->_name);
            __RSRunLoopTimerUnlock(rlt);
            __RSRunLoopTimerFireTSRLock();
            __RSRepositionTimerInMode(rlm, rlt, NO);
            __RSRunLoopTimerFireTSRUnlock();
            if (!_RSExecutableLinkedOnOrAfter(RSSystemVersionLion)) {
                // Normally we don't do this on behalf of clients, but for
                // backwards compatibility due to the change in timer handling...
                if (rl != RSRunLoopGetCurrent()) RSRunLoopWakeUp(rl);
            }
        }
        if (nil != rlm) {
            __RSRunLoopModeUnlock(rlm);
        }
    }
    __RSRunLoopUnlock(rl);
}

void RSRunLoopRemoveTimer(RSRunLoopRef rl, RSRunLoopTimerRef rlt, RSStringRef modeName) {
    CHECK_FOR_FORK();
    __RSRunLoopLock(rl);
    if (modeName == RSRunLoopCommonModes) {
        if (nil != rl->_commonModeItems && RSSetContainsValue(rl->_commonModeItems, rlt)) {
            RSSetRef set = rl->_commonModes ? RSSetCreateCopy(RSAllocatorSystemDefault, rl->_commonModes) : nil;
            RSSetRemoveValue(rl->_commonModeItems, rlt);
            if (nil != set) {
                RSTypeRef context[2] = {rl, rlt};
                /* remove new item from all common-modes */
                RSSetApplyFunction(set, (__RSRunLoopRemoveItemFromCommonModes), (void *)context);
                RSRelease(set);
            }
        } else {
        }
    } else {
        RSRunLoopModeRef rlm = __RSRunLoopFindMode(rl, modeName, NO);
        RSIndex idx = RSNotFound;
        RSMutableArrayRef timerList = nil;
        if (nil != rlm) {
            timerList = rlm->_timers;
            if (nil != timerList) {
                idx = RSArrayIndexOfObject(timerList, rlt);
            }
        }
        if (RSNotFound != idx) {
            __RSRunLoopTimerLock(rlt);
            RSSetRemoveValue(rlt->_rlModes, rlm->_name);
            if (0 == RSSetGetCount(rlt->_rlModes)) {
                rlt->_runLoop = nil;
            }
            __RSRunLoopTimerUnlock(rlt);
            RSArrayRemoveObjectAtIndex(timerList, idx);
            __RSArmNextTimerInMode(rlm, rl);
        }
        if (nil != rlm) {
            __RSRunLoopModeUnlock(rlm);
        }
    }
    __RSRunLoopUnlock(rl);
}

/* RSRunLoopSource */

static BOOL __RSRunLoopSourceClassEqual(RSTypeRef rs1, RSTypeRef rs2) {	/* DOES CALLOUT */
    RSRunLoopSourceRef rls1 = (RSRunLoopSourceRef)rs1;
    RSRunLoopSourceRef rls2 = (RSRunLoopSourceRef)rs2;
    if (rls1 == rls2) return YES;
    if (__RSRunLoopIsValid(rls1) != __RSRunLoopIsValid(rls2)) return NO;
    if (rls1->_order != rls2->_order) return NO;
    if (rls1->_context.version0.version != rls2->_context.version0.version) return NO;
    if (rls1->_context.version0.hash != rls2->_context.version0.hash) return NO;
    if (rls1->_context.version0.equal != rls2->_context.version0.equal) return NO;
    if (0 == rls1->_context.version0.version && rls1->_context.version0.perform != rls2->_context.version0.perform) return NO;
    if (1 == rls1->_context.version0.version && rls1->_context.version1.perform != rls2->_context.version1.perform) return NO;
    if (rls1->_context.version0.equal)
        return rls1->_context.version0.equal(rls1->_context.version0.info, rls2->_context.version0.info);
    return (rls1->_context.version0.info == rls2->_context.version0.info);
}

static RSHashCode __RSRunLoopSourceClassHash(RSTypeRef rs) {	/* DOES CALLOUT */
    RSRunLoopSourceRef rls = (RSRunLoopSourceRef)rs;
    if (rls->_context.version0.hash)
        return rls->_context.version0.hash(rls->_context.version0.info);
    return (RSHashCode)rls->_context.version0.info;
}

static RSStringRef __RSRunLoopSourceClassDescription(RSTypeRef rs) {	/* DOES CALLOUT */
    RSRunLoopSourceRef rls = (RSRunLoopSourceRef)rs;
    RSStringRef result;
    RSStringRef contextDesc = nil;
    if (nil != rls->_context.version0.description) {
        contextDesc = rls->_context.version0.description(rls->_context.version0.info);
    }
    if (nil == contextDesc) {
        void *addr = rls->_context.version0.version == 0 ? (void *)rls->_context.version0.perform : (rls->_context.version0.version == 1 ? (void *)rls->_context.version1.perform : nil);
#if DEPLOYMENT_TARGET_WINDOWS
        contextDesc = RSStringCreateWithFormat(RSAllocatorSystemDefault, RSSTR("<RSRunLoopSource context>{version = %ld, info = %p, callout = %p}"), rls->_context.version0.version, rls->_context.version0.info, addr);
#elif DEPLOYMENT_TARGET_MACOSX || DEPLOYMENT_TARGET_EMBEDDED || DEPLOYMENT_TARGET_EMBEDDED_MINI
        Dl_info info;
        const char *name = (dladdr(addr, &info) && info.dli_saddr == addr && info.dli_sname) ? info.dli_sname : "???";
        contextDesc = RSStringCreateWithFormat(RSAllocatorSystemDefault, nil, RSSTR("<RSRunLoopSource context>{version = %ld, info = %p, callout = %s (%p)}"), rls->_context.version0.version, rls->_context.version0.info, name, addr);
#endif
    }
#if DEPLOYMENT_TARGET_WINDOWS
    result = RSStringCreateWithFormat(RSAllocatorSystemDefault, RSSTR("<RSRunLoopSource %p [%p]>{signalled = %s, valid = %s, order = %d, context = %r}"), rs, RSGetAllocator(rls), __RSRunLoopSourceIsSignaled(rls) ? "Yes" : "No", __RSRunLoopIsValid(rls) ? "Yes" : "No", rls->_order, contextDesc);
#else
    result = RSStringCreateWithFormat(RSAllocatorSystemDefault, RSSTR("<RSRunLoopSource %p [%p]>{signalled = %s, valid = %s, order = %ld, context = %r}"), rs, RSGetAllocator(rls), __RSRunLoopSourceIsSignaled(rls) ? "Yes" : "No", __RSRunLoopIsValid(rls) ? "Yes" : "No", (unsigned long)rls->_order, contextDesc);
#endif
    RSRelease(contextDesc);
    return result;
}

static void __RSRunLoopSourceClassDeallocate(RSTypeRef rs) {	/* DOES CALLOUT */
    RSRunLoopSourceRef rls = (RSRunLoopSourceRef)rs;
    RSRunLoopSourceInvalidate(rls);
    if (rls->_context.version0.release) {
        rls->_context.version0.release(rls->_context.version0.info);
    }
    pthread_mutex_destroy(&rls->_lock);
    memset((char *)rs + sizeof(RSRuntimeBase), 0, sizeof(struct __RSRunLoopSource) - sizeof(RSRuntimeBase));
}

static const RSRuntimeClass __RSRunLoopSourceClass = {
    _RSRuntimeScannedObject,
    "RSRunLoopSource",
    nil,      // init
    nil,      // copy
    __RSRunLoopSourceClassDeallocate,
    __RSRunLoopSourceClassEqual,
    __RSRunLoopSourceClassHash,
    __RSRunLoopSourceClassDescription,      //
    nil,
    nil
};

RSPrivate void __RSRunLoopSourceInitialize(void) {
    __RSRunLoopSourceTypeID = __RSRuntimeRegisterClass(&__RSRunLoopSourceClass);
    __RSRuntimeSetClassTypeID(&__RSRunLoopSourceClass, __RSRunLoopSourceTypeID);
}

RSTypeID RSRunLoopSourceGetTypeID(void) {
    return __RSRunLoopSourceTypeID;
}

RSRunLoopSourceRef RSRunLoopSourceCreate(RSAllocatorRef allocator, RSIndex order, RSRunLoopSourceContext *context) {
    CHECK_FOR_FORK();
    RSRunLoopSourceRef memory;
    uint32_t size;
    if (nil == context) CRASH("*** nil context value passed to RSRunLoopSourceCreate(). (%d) ***", -1);
    
    size = sizeof(struct __RSRunLoopSource) - sizeof(RSRuntimeBase);
    memory = (RSRunLoopSourceRef)__RSRuntimeCreateInstance(allocator, __RSRunLoopSourceTypeID, size);
    if (nil == memory) {
        return nil;
    }
    __RSRunLoopSetValid(memory);
    __RSRunLoopSourceUnsetSignaled(memory);
    __RSRunLoopLockInit(&memory->_lock);
    memory->_bits = 0;
    memory->_order = order;
    memory->_runLoops = nil;
    size = 0;
    switch (context->version) {
        case 0:
            size = sizeof(RSRunLoopSourceContext);
            break;
        case 1:
            size = sizeof(RSRunLoopSourceContext1);
            break;
    }
    __builtin_memmove(&memory->_context, context, size);
    if (context->retain) {
        memory->_context.version0.info = (void *)context->retain(context->info);
    }
    return memory;
}

RSIndex RSRunLoopSourceGetOrder(RSRunLoopSourceRef rls) {
    CHECK_FOR_FORK();
    __RSGenericValidInstance(rls, __RSRunLoopSourceTypeID);
    return rls->_order;
}

static void __RSRunLoopSourceWakeUpLoop(const void *value, void *context) {
    RSRunLoopWakeUp((RSRunLoopRef)value);
}

static void __RSRunLoopSourceRemoveFromRunLoop(const void *value, void *context) {
    RSRunLoopRef rl = (RSRunLoopRef)value;
    RSTypeRef *params = (RSTypeRef *)context;
    RSRunLoopSourceRef rls = (RSRunLoopSourceRef)params[0];
    RSIndex idx;
    if (rl == params[1]) return;
    
    // RSRunLoopRemoveSource will lock the run loop while it
    // needs that, but we also lock it out here to keep
    // changes from occurring for this whole sequence.
    __RSRunLoopLock(rl);
    RSArrayRef array = RSRunLoopCopyAllModes(rl);
    for (idx = RSArrayGetCount(array); idx--;) {
        RSStringRef modeName = (RSStringRef)RSArrayObjectAtIndex(array, idx);
        RSRunLoopRemoveSource(rl, rls, modeName);
    }
    RSRunLoopRemoveSource(rl, rls, RSRunLoopCommonModes);
    __RSRunLoopUnlock(rl);
    RSRelease(array);
    params[1] = rl;
}

void RSRunLoopSourceInvalidate(RSRunLoopSourceRef rls) {
    CHECK_FOR_FORK();
    __RSGenericValidInstance(rls, __RSRunLoopSourceTypeID);
    __RSRunLoopSourceLock(rls);
    RSRetain(rls);
    if (__RSRunLoopIsValid(rls)) {
        RSBagRef rloops = rls->_runLoops;
        __RSRunLoopUnsetValid(rls);
        __RSRunLoopSourceUnsetSignaled(rls);
        if (nil != rloops) {
            // To avoid A->B, B->A lock ordering issues when coming up
            // towards the run loop from a source, the source has to be
            // unlocked, which means we have to protect from object
            // invalidation.
            rls->_runLoops = nil; // transfer ownership to local stack
            __RSRunLoopSourceUnlock(rls);
            RSTypeRef params[2] = {rls, nil};
            RSBagApplyFunction(rloops, (__RSRunLoopSourceRemoveFromRunLoop), params);
            RSRelease(rloops);
            __RSRunLoopSourceLock(rls);
        }
        /* for hashing- and equality-use purposes, can't actually release the context here */
    }
    __RSRunLoopSourceUnlock(rls);
    RSRelease(rls);
}

BOOL RSRunLoopSourceIsValid(RSRunLoopSourceRef rls) {
    CHECK_FOR_FORK();
    __RSGenericValidInstance(rls, __RSRunLoopSourceTypeID);
    return __RSRunLoopIsValid(rls);
}

void RSRunLoopSourceGetContext(RSRunLoopSourceRef rls, RSRunLoopSourceContext *context) {
    CHECK_FOR_FORK();
    __RSGenericValidInstance(rls, __RSRunLoopSourceTypeID);
    RSAssert1(0 == context->version || 1 == context->version, __RSLogAssertion, "%s(): context version not initialized to 0 or 1", __PRETTY_FUNCTION__);
    RSIndex size = 0;
    switch (context->version) {
        case 0:
            size = sizeof(RSRunLoopSourceContext);
            break;
        case 1:
            size = sizeof(RSRunLoopSourceContext1);
            break;
    }
    memmove(context, &rls->_context, size);
}

void RSRunLoopSourceSignal(RSRunLoopSourceRef rls) {
    CHECK_FOR_FORK();
    __RSRunLoopSourceLock(rls);
    if (__RSRunLoopIsValid(rls)) {
        __RSRunLoopSourceSetSignaled(rls);
    }
    __RSRunLoopSourceUnlock(rls);
}

BOOL RSRunLoopSourceIsSignalled(RSRunLoopSourceRef rls) {
    CHECK_FOR_FORK();
    __RSRunLoopSourceLock(rls);
    BOOL ret = __RSRunLoopSourceIsSignaled(rls) ? YES : NO;
    __RSRunLoopSourceUnlock(rls);
    return ret;
}

RSPrivate void _RSRunLoopSourceWakeUpRunLoops(RSRunLoopSourceRef rls) {
    RSBagRef loops = nil;
    __RSRunLoopSourceLock(rls);
    if (__RSRunLoopIsValid(rls) && nil != rls->_runLoops) {
        loops = RSBagCreateCopy(RSAllocatorSystemDefault, rls->_runLoops);
    }
    __RSRunLoopSourceUnlock(rls);
    if (loops) {
        RSBagApplyFunction(loops, __RSRunLoopSourceWakeUpLoop, nil);
        RSRelease(loops);
    }
}

/* RSRunLoopObserver */

static RSStringRef __RSRunLoopObserverClassDescription(RSTypeRef rs) {	/* DOES CALLOUT */
    RSRunLoopObserverRef rlo = (RSRunLoopObserverRef)rs;
    RSStringRef result;
    RSStringRef contextDesc = nil;
    if (nil != rlo->_context.description) {
        contextDesc = rlo->_context.description(rlo->_context.info);
    }
    if (!contextDesc) {
        contextDesc = RSStringCreateWithFormat(RSAllocatorSystemDefault, nil, RSSTR("<RSRunLoopObserver context %p>"), rlo->_context.info);
    }
#if DEPLOYMENT_TARGET_WINDOWS
    result = RSStringCreateWithFormat(RSAllocatorSystemDefault, nil, RSSTR("<RSRunLoopObserver %p [%p]>{valid = %s, activities = 0x%x, repeats = %s, order = %d, callout = %p, context = %r}"), rs, RSGetAllocator(rlo), __RSRunLoopIsValid(rlo) ? "Yes" : "No", rlo->_activities, __RSRunLoopObserverRepeats(rlo) ? "Yes" : "No", rlo->_order, rlo->_callout, contextDesc);
#elif DEPLOYMENT_TARGET_MACOSX || DEPLOYMENT_TARGET_EMBEDDED || DEPLOYMENT_TARGET_EMBEDDED_MINI
    void *addr = rlo->_callout;
    Dl_info info;
    const char *name = (dladdr(addr, &info) && info.dli_saddr == addr && info.dli_sname) ? info.dli_sname : "???";
    result = RSStringCreateWithFormat(RSAllocatorSystemDefault, nil, RSSTR("<RSRunLoopObserver %p [%p]>{valid = %s, activities = 0x%lx, repeats = %s, order = %ld, callout = %s (%p), context = %r}"), rs, RSGetAllocator(rlo), __RSRunLoopIsValid(rlo) ? "Yes" : "No", (long)rlo->_activities, __RSRunLoopObserverRepeats(rlo) ? "Yes" : "No", (long)rlo->_order, name, addr, contextDesc);
#endif
    RSRelease(contextDesc);
    return result;
}

static void __RSRunLoopObserverClassDeallocate(RSTypeRef rs) {	/* DOES CALLOUT */
    RSRunLoopObserverRef rlo = (RSRunLoopObserverRef)rs;
    RSRunLoopObserverInvalidate(rlo);
    pthread_mutex_destroy(&rlo->_lock);
}

static const RSRuntimeClass __RSRunLoopObserverClass = {
    _RSRuntimeScannedObject,
    "RSRunLoopObserver",
    nil,      // init
    nil,      // copy
    __RSRunLoopObserverClassDeallocate,
    nil,
    nil,
    __RSRunLoopObserverClassDescription,      //
    nil,
    nil
};

RSPrivate void __RSRunLoopObserverInitialize(void) {
    __RSRunLoopObserverTypeID = __RSRuntimeRegisterClass(&__RSRunLoopObserverClass);
    __RSRuntimeSetClassTypeID(&__RSRunLoopObserverClass, __RSRunLoopObserverTypeID);
}

RSTypeID RSRunLoopObserverGetTypeID(void) {
    return __RSRunLoopObserverTypeID;
}

RSRunLoopObserverRef RSRunLoopObserverCreate(RSAllocatorRef allocator, RSOptionFlags activities, BOOL repeats, RSIndex order, RSRunLoopObserverCallBack callout, RSRunLoopObserverContext *context) {
    CHECK_FOR_FORK();
    RSRunLoopObserverRef memory;
    UInt32 size;
    size = sizeof(struct __RSRunLoopObserver) - sizeof(RSRuntimeBase);
    memory = (RSRunLoopObserverRef)__RSRuntimeCreateInstance(allocator, __RSRunLoopObserverTypeID, size);
    if (nil == memory) {
        return nil;
    }
    __RSRunLoopSetValid(memory);
    __RSRunLoopObserverUnsetFiring(memory);
    if (repeats) {
        __RSRunLoopObserverSetRepeats(memory);
    } else {
        __RSRunLoopObserverUnsetRepeats(memory);
    }
    __RSRunLoopLockInit(&memory->_lock);
    memory->_runLoop = nil;
    memory->_rlCount = 0;
    memory->_activities = activities;
    memory->_order = order;
    memory->_callout = callout;
    if (context) {
        if (context->retain) {
            memory->_context.info = (void *)context->retain(context->info);
        } else {
            memory->_context.info = context->info;
        }
        memory->_context.retain = context->retain;
        memory->_context.release = context->release;
        memory->_context.description = context->description;
    } else {
        memory->_context.info = 0;
        memory->_context.retain = 0;
        memory->_context.release = 0;
        memory->_context.description = 0;
    }
    return memory;
}

static void _runLoopObserverWithBlockContext(RSRunLoopObserverRef observer, RSRunLoopActivity activity, void *opaqueBlock) {
    typedef void (^observer_block_t) (RSRunLoopObserverRef observer, RSRunLoopActivity activity);
    observer_block_t block = (observer_block_t)opaqueBlock;
    block(observer, activity);
}

RSRunLoopObserverRef RSRunLoopObserverCreateWithHandler(RSAllocatorRef allocator, RSOptionFlags activities, BOOL repeats, RSIndex order,
                                                        void (^block) (RSRunLoopObserverRef observer, RSRunLoopActivity activity)) {
    RSRunLoopObserverContext blockContext;
    blockContext.version = 0;
    blockContext.info = (void *)block;
    blockContext.retain = (const void *(*)(const void *info))_Block_copy;
    blockContext.release = (void (*)(const void *info))_Block_release;
    blockContext.description = nil;
    return RSRunLoopObserverCreate(allocator, activities, repeats, order, _runLoopObserverWithBlockContext, &blockContext);
}

RSOptionFlags RSRunLoopObserverGetActivities(RSRunLoopObserverRef rlo) {
    CHECK_FOR_FORK();
    __RSGenericValidInstance(rlo, __RSRunLoopObserverTypeID);
    return rlo->_activities;
}

RSIndex RSRunLoopObserverGetOrder(RSRunLoopObserverRef rlo) {
    CHECK_FOR_FORK();
    __RSGenericValidInstance(rlo, __RSRunLoopObserverTypeID);
    return rlo->_order;
}

BOOL RSRunLoopObserverDoesRepeat(RSRunLoopObserverRef rlo) {
    CHECK_FOR_FORK();
    __RSGenericValidInstance(rlo, __RSRunLoopObserverTypeID);
    return __RSRunLoopObserverRepeats(rlo);
}

void RSRunLoopObserverInvalidate(RSRunLoopObserverRef rlo) {    /* DOES CALLOUT */
    CHECK_FOR_FORK();
    __RSGenericValidInstance(rlo, __RSRunLoopObserverTypeID);
    __RSRunLoopObserverLock(rlo);
    RSRetain(rlo);
    if (__RSRunLoopIsValid(rlo)) {
        RSRunLoopRef rl = rlo->_runLoop;
        void *info = rlo->_context.info;
        rlo->_context.info = nil;
        __RSRunLoopUnsetValid(rlo);
        if (nil != rl) {
            // To avoid A->B, B->A lock ordering issues when coming up
            // towards the run loop from an observer, it has to be
            // unlocked, which means we have to protect from object
            // invalidation.
            RSRetain(rl);
            __RSRunLoopObserverUnlock(rlo);
            // RSRunLoopRemoveObserver will lock the run loop while it
            // needs that, but we also lock it out here to keep
            // changes from occurring for this whole sequence.
            __RSRunLoopLock(rl);
            RSArrayRef array = RSRunLoopCopyAllModes(rl);
            for (RSIndex idx = RSArrayGetCount(array); idx--;) {
                RSStringRef modeName = (RSStringRef)RSArrayObjectAtIndex(array, idx);
                RSRunLoopRemoveObserver(rl, rlo, modeName);
            }
            RSRunLoopRemoveObserver(rl, rlo, RSRunLoopCommonModes);
            __RSRunLoopUnlock(rl);
            RSRelease(array);
            RSRelease(rl);
            __RSRunLoopObserverLock(rlo);
        }
        if (nil != rlo->_context.release) {
            rlo->_context.release(info);        /* CALLOUT */
        }
    }
    __RSRunLoopObserverUnlock(rlo);
    RSRelease(rlo);
}

BOOL RSRunLoopObserverIsValid(RSRunLoopObserverRef rlo) {
    CHECK_FOR_FORK();
    return __RSRunLoopIsValid(rlo);
}

void RSRunLoopObserverGetContext(RSRunLoopObserverRef rlo, RSRunLoopObserverContext *context) {
    CHECK_FOR_FORK();
    __RSGenericValidInstance(rlo, __RSRunLoopObserverTypeID);
    RSAssert1(0 == context->version, __RSLogAssertion, "%s(): context version not initialized to 0", __PRETTY_FUNCTION__);
    *context = rlo->_context;
}

#pragma mark -
#pragma mark RSRunLoopTimer

static RSStringRef __RSRunLoopTimerClassDescription(RSTypeRef rs) {	/* DOES CALLOUT */
    RSRunLoopTimerRef rlt = (RSRunLoopTimerRef)rs;
    RSStringRef contextDesc = nil;
    if (nil != rlt->_context.description) {
        contextDesc = rlt->_context.description(rlt->_context.info);
    }
    if (nil == contextDesc) {
        contextDesc = RSStringCreateWithFormat(RSAllocatorSystemDefault, RSSTR("<RSRunLoopTimer context %p>"), rlt->_context.info);
    }
    void *addr = (void *)rlt->_callout;
    char libraryName[2048];
    char functionName[2048];
    void *functionPtr = nil;
    libraryName[0] = '?'; libraryName[1] = '\0';
    functionName[0] = '?'; functionName[1] = '\0';
    RSStringRef result = RSStringCreateWithFormat(RSAllocatorSystemDefault,
                                                  RSSTR("<RSRunLoopTimer %p [%p]>{valid = %s, firing = %s, interval = %0.09g, tolerance = %0.09g, next fire date = %0.09g (%0.09g @ %lld), callout = %s (%p / %p) (%s), context = %r}"),
                                                  rs,
                                                  RSGetAllocator(rlt),
                                                  __RSRunLoopIsValid(rlt) ? "Yes" : "No",
                                                  __RSRunLoopTimerIsFiring(rlt) ? "Yes" : "No",
                                                  rlt->_interval,
                                                  rlt->_tolerance,
                                                  rlt->_nextFireDate,
                                                  rlt->_nextFireDate - RSAbsoluteTimeGetCurrent(),
                                                  rlt->_fireTSR,
                                                  functionName,
                                                  addr,
                                                  functionPtr,
                                                  libraryName,
                                                  contextDesc);
    RSRelease(contextDesc);
    return result;
}

static void __RSRunLoopTimerClassDeallocate(RSTypeRef rs) {	/* DOES CALLOUT */
    //RSLog(6, RSSTR("__RSRunLoopTimerDeallocate(%p)"), rs);
    RSRunLoopTimerRef rlt = (RSRunLoopTimerRef)rs;
    __RSRunLoopTimerSetDeallocating(rlt);
    RSRunLoopTimerInvalidate(rlt);	/* DOES CALLOUT */
    RSRelease(rlt->_rlModes);
    rlt->_rlModes = nil;
    pthread_mutex_destroy(&rlt->_lock);
}

static const RSRuntimeClass __RSRunLoopTimerClass = {
    _RSRuntimeScannedObject,
    "RSRunLoopTimer",
    nil,      // init
    nil,      // copy
    __RSRunLoopTimerClassDeallocate,
    nil,	// equal
    nil,
    __RSRunLoopTimerClassDescription,
    nil,
    nil
};

RSPrivate void __RSRunLoopTimerInitialize(void) {
    __RSRunLoopTimerTypeID = __RSRuntimeRegisterClass(&__RSRunLoopTimerClass);
    __RSRuntimeSetClassTypeID(&__RSRunLoopTimerClass, __RSRunLoopTimerTypeID);
}

RSTypeID RSRunLoopTimerGetTypeID(void) {
    return __RSRunLoopTimerTypeID;
}

RSRunLoopTimerRef RSRunLoopTimerCreate(RSAllocatorRef allocator, RSAbsoluteTime fireDate, RSTimeInterval interval, RSOptionFlags flags, RSIndex order, RSRunLoopTimerCallBack callout, RSRunLoopTimerContext *context) {
    CHECK_FOR_FORK();
    RSRunLoopTimerRef memory;
    UInt32 size;
    size = sizeof(struct __RSRunLoopTimer) - sizeof(RSRuntimeBase);
    memory = (RSRunLoopTimerRef)__RSRuntimeCreateInstance(allocator, __RSRunLoopTimerTypeID, size);
    if (nil == memory) {
        return nil;
    }
    __RSRunLoopSetValid(memory);
    __RSRunLoopTimerUnsetFiring(memory);
    __RSRunLoopLockInit(&memory->_lock);
    memory->_runLoop = nil;
    memory->_rlModes = RSSetCreateMutable(RSAllocatorSystemDefault, 0, &RSTypeSetCallBacks);
    memory->_order = order;
    if (interval < 0.0) interval = 0.0;
    memory->_interval = interval;
    memory->_tolerance = 0.0;
    if (TIMER_DATE_LIMIT < fireDate) fireDate = TIMER_DATE_LIMIT;
    memory->_nextFireDate = fireDate;
    memory->_fireTSR = 0ULL;
    uint64_t now2 = mach_absolute_time();
    RSAbsoluteTime now1 = RSAbsoluteTimeGetCurrent();
    if (fireDate < now1) {
        memory->_fireTSR = now2;
    } else if (TIMER_INTERVAL_LIMIT < fireDate - now1) {
        memory->_fireTSR = now2 + __RSTimeIntervalToTSR(TIMER_INTERVAL_LIMIT);
    } else {
        memory->_fireTSR = now2 + __RSTimeIntervalToTSR(fireDate - now1);
    }
    memory->_callout = callout;
    if (nil != context) {
        if (context->retain) {
            memory->_context.info = (void *)context->retain(context->info);
        } else {
            memory->_context.info = context->info;
        }
        memory->_context.retain = context->retain;
        memory->_context.release = context->release;
        memory->_context.description = context->description;
    } else {
        memory->_context.info = 0;
        memory->_context.retain = 0;
        memory->_context.release = 0;
        memory->_context.description = 0;
    }
    return memory;
}

static void _runLoopTimerWithBlockContext(RSRunLoopTimerRef timer, void *opaqueBlock) {
    typedef void (^timer_block_t) (RSRunLoopTimerRef timer);
    timer_block_t block = (timer_block_t)opaqueBlock;
    block(timer);
}

RSExport RSRunLoopTimerRef RSRunLoopTimerCreateWithHandler(RSAllocatorRef allocator, RSAbsoluteTime fireDate, RSTimeInterval interval, RSOptionFlags flags, RSIndex order,
                                                  void (^block) (RSRunLoopTimerRef timer)) {
    
    RSRunLoopTimerContext blockContext;
    blockContext.version = 0;
    blockContext.info = (void *)block;
    blockContext.retain = (const void *(*)(const void *info))_Block_copy;
    blockContext.release = (void (*)(const void *info))_Block_release;
    blockContext.description = nil;
    return RSRunLoopTimerCreate(allocator, fireDate, interval, flags, order, _runLoopTimerWithBlockContext, &blockContext);
}

RSExport RSAbsoluteTime RSRunLoopTimerGetNextFireDate(RSRunLoopTimerRef rlt) {
    CHECK_FOR_FORK();
    RS_OBJC_FUNCDISPATCHV(__RSRunLoopTimerTypeID, RSAbsoluteTime, (NSTimer *)rlt, _rsfireTime);
    __RSGenericValidInstance(rlt, __RSRunLoopTimerTypeID);
    RSAbsoluteTime at = 0.0;
    __RSRunLoopTimerLock(rlt);
    __RSRunLoopTimerFireTSRLock();
    if (__RSRunLoopIsValid(rlt)) {
        at = rlt->_nextFireDate;
    }
    __RSRunLoopTimerFireTSRUnlock();
    __RSRunLoopTimerUnlock(rlt);
    return at;
}

RSExport void RSRunLoopTimerSetNextFireDate(RSRunLoopTimerRef rlt, RSAbsoluteTime fireDate) {
    CHECK_FOR_FORK();
    if (!__RSRunLoopIsValid(rlt)) return;
    if (TIMER_DATE_LIMIT < fireDate) fireDate = TIMER_DATE_LIMIT;
    uint64_t nextFireTSR = 0ULL;
    uint64_t now2 = mach_absolute_time();
    RSAbsoluteTime now1 = RSAbsoluteTimeGetCurrent();
    if (fireDate < now1) {
        nextFireTSR = now2;
    } else if (TIMER_INTERVAL_LIMIT < fireDate - now1) {
        nextFireTSR = now2 + __RSTimeIntervalToTSR(TIMER_INTERVAL_LIMIT);
    } else {
        nextFireTSR = now2 + __RSTimeIntervalToTSR(fireDate - now1);
    }
    __RSRunLoopTimerLock(rlt);
    if (nil != rlt->_runLoop) {
        RSIndex cnt = RSSetGetCount(rlt->_rlModes);
        STACK_BUFFER_DECL(RSTypeRef, modes, cnt);
        RSSetGetValues(rlt->_rlModes, (const void **)modes);
        // To avoid A->B, B->A lock ordering issues when coming up
        // towards the run loop from a source, the timer has to be
        // unlocked, which means we have to protect from object
        // invalidation, although that's somewhat expensive.
        for (RSIndex idx = 0; idx < cnt; idx++) {
            RSRetain(modes[idx]);
        }
        RSRunLoopRef rl = (RSRunLoopRef)RSRetain(rlt->_runLoop);
        __RSRunLoopTimerUnlock(rlt);
        __RSRunLoopLock(rl);
        for (RSIndex idx = 0; idx < cnt; idx++) {
            RSStringRef name = (RSStringRef)modes[idx];
            modes[idx] = __RSRunLoopFindMode(rl, name, NO);
            RSRelease(name);
        }
        __RSRunLoopTimerFireTSRLock();
        rlt->_fireTSR = nextFireTSR;
        rlt->_nextFireDate = fireDate;
        for (RSIndex idx = 0; idx < cnt; idx++) {
            RSRunLoopModeRef rlm = (RSRunLoopModeRef)modes[idx];
            if (rlm) {
                __RSRepositionTimerInMode(rlm, rlt, YES);
            }
        }
        __RSRunLoopTimerFireTSRUnlock();
        for (RSIndex idx = 0; idx < cnt; idx++) {
            __RSRunLoopModeUnlock((RSRunLoopModeRef)modes[idx]);
        }
        __RSRunLoopUnlock(rl);
        // This is setting the date of a timer, not a direct
        // interaction with a run loop, so we'll do a wakeup
        // (which may be costly) for the caller, just in case.
        // (And useful for binary compatibility with older
        // code used to the older timer implementation.)
        if (rl != RSRunLoopGetCurrent()) RSRunLoopWakeUp(rl);
        RSRelease(rl);
    } else {
        __RSRunLoopTimerFireTSRLock();
        rlt->_fireTSR = nextFireTSR;
        rlt->_nextFireDate = fireDate;
        __RSRunLoopTimerFireTSRUnlock();
        __RSRunLoopTimerUnlock(rlt);
    }
}

RSExport RSTimeInterval RSRunLoopTimerGetInterval(RSRunLoopTimerRef rlt) {
    CHECK_FOR_FORK();
    RS_OBJC_FUNCDISPATCHV(__RSRunLoopTimerTypeID, RSTimeInterval, (NSTimer *)rlt, timeInterval);
    __RSGenericValidInstance(rlt, __RSRunLoopTimerTypeID);
    return rlt->_interval;
}

RSExport BOOL RSRunLoopTimerDoesRepeat(RSRunLoopTimerRef rlt) {
    CHECK_FOR_FORK();
    __RSGenericValidInstance(rlt, __RSRunLoopTimerTypeID);
    return (0.0 < rlt->_interval);
}

RSExport RSIndex RSRunLoopTimerGetOrder(RSRunLoopTimerRef rlt) {
    CHECK_FOR_FORK();
    __RSGenericValidInstance(rlt, __RSRunLoopTimerTypeID);
    return rlt->_order;
}

RSExport void RSRunLoopTimerInvalidate(RSRunLoopTimerRef rlt) {	/* DOES CALLOUT */
    CHECK_FOR_FORK();
    RS_OBJC_FUNCDISPATCHV(__RSRunLoopTimerTypeID, void, (NSTimer *)rlt, invalidate);
    __RSGenericValidInstance(rlt, __RSRunLoopTimerTypeID);
    __RSRunLoopTimerLock(rlt);
    if (!__RSRunLoopTimerIsDeallocating(rlt)) {
        RSRetain(rlt);
    }
    if (__RSRunLoopIsValid(rlt)) {
        RSRunLoopRef rl = rlt->_runLoop;
        void *info = rlt->_context.info;
        rlt->_context.info = nil;
        __RSRunLoopUnsetValid(rlt);
        if (nil != rl) {
            RSIndex cnt = RSSetGetCount(rlt->_rlModes);
            STACK_BUFFER_DECL(RSStringRef, modes, cnt);
            RSSetGetValues(rlt->_rlModes, (const void **)modes);
            // To avoid A->B, B->A lock ordering issues when coming up
            // towards the run loop from a source, the timer has to be
            // unlocked, which means we have to protect from object
            // invalidation, although that's somewhat expensive.
            for (RSIndex idx = 0; idx < cnt; idx++) {
                RSRetain(modes[idx]);
            }
            RSRetain(rl);
            __RSRunLoopTimerUnlock(rlt);
            // RSRunLoopRemoveTimer will lock the run loop while it
            // needs that, but we also lock it out here to keep
            // changes from occurring for this whole sequence.
            __RSRunLoopLock(rl);
            for (RSIndex idx = 0; idx < cnt; idx++) {
                RSRunLoopRemoveTimer(rl, rlt, modes[idx]);
            }
            RSRunLoopRemoveTimer(rl, rlt, RSRunLoopCommonModes);
            __RSRunLoopUnlock(rl);
            for (RSIndex idx = 0; idx < cnt; idx++) {
                RSRelease(modes[idx]);
            }
            RSRelease(rl);
            __RSRunLoopTimerLock(rlt);
        }
        if (nil != rlt->_context.release) {
            rlt->_context.release(info);	/* CALLOUT */
        }
    }
    __RSRunLoopTimerUnlock(rlt);
    if (!__RSRunLoopTimerIsDeallocating(rlt)) {
        RSRelease(rlt);
    }
}

RSExport BOOL RSRunLoopTimerIsValid(RSRunLoopTimerRef rlt) {
    CHECK_FOR_FORK();
    RS_OBJC_FUNCDISPATCHV(__RSRunLoopTimerTypeID, BOOL, (NSTimer *)rlt, isValid);
    __RSGenericValidInstance(rlt, __RSRunLoopTimerTypeID);
    return __RSRunLoopIsValid(rlt);
}

RSExport void RSRunLoopTimerGetContext(RSRunLoopTimerRef rlt, RSRunLoopTimerContext *context) {
    CHECK_FOR_FORK();
    __RSGenericValidInstance(rlt, __RSRunLoopTimerTypeID);
    RSAssert1(0 == context->version, __RSLogAssertion, "%s(): context version not initialized to 0", __PRETTY_FUNCTION__);
    *context = rlt->_context;
}

RSExport RSTimeInterval RSRunLoopTimerGetTolerance(RSRunLoopTimerRef rlt) {
#if DEPLOYMENT_TARGET_MACOSX || DEPLOYMENT_TARGET_EMBEDDED || DEPLOYMENT_TARGET_EMBEDDED_MINI
    CHECK_FOR_FORK();
    RS_OBJC_FUNCDISPATCHV(__RSRunLoopTimerTypeID, RSTimeInterval, (NSTimer *)rlt, tolerance);
    __RSGenericValidInstance(rlt, __RSRunLoopTimerTypeID);
    return rlt->_tolerance;
#else
    return 0.0;
#endif
}

RSExport void RSRunLoopTimerSetTolerance(RSRunLoopTimerRef rlt, RSTimeInterval tolerance) {
#if DEPLOYMENT_TARGET_MACOSX || DEPLOYMENT_TARGET_EMBEDDED || DEPLOYMENT_TARGET_EMBEDDED_MINI
    CHECK_FOR_FORK();
    RS_OBJC_FUNCDISPATCHV(__RSRunLoopTimerTypeID, void, (NSTimer *)rlt, setTolerance:tolerance);
    __RSGenericValidInstance(rlt, __RSRunLoopTimerTypeID);
    /*
     * dispatch rules:
     *
     * For the initial timer fire at 'start', the upper limit to the allowable
     * delay is set to 'leeway' nanoseconds. For the subsequent timer fires at
     * 'start' + N * 'interval', the upper limit is MIN('leeway','interval'/2).
     */
    if (rlt->_interval > 0) {
        rlt->_tolerance = MIN(tolerance, rlt->_interval / 2);
    } else {
        // Tolerance must be a positive value or zero
        if (tolerance < 0) tolerance = 0.0;
        rlt->_tolerance = tolerance;
    }
#endif
}

RSExport void RSPerformBlockRepeat(RSIndex performCount, void (^perform)(RSIndex idx))
{
    RSPerformBlockRepeatWithFlags(performCount, RSPerformBlockDefault, perform);
}

RSExport void RSPerformBlockRepeatWithFlags(RSIndex performCount, RSPerformBlockOptionFlags performFlags, void (^perform)(RSIndex idx))
{
    if (performCount > 0 && perform)
    {
        dispatch_apply(performCount, dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_HIGH, (unsigned long)performFlags), ^(size_t idx) {
            perform(idx);
        });
    }
}
#include <RSCoreFoundation/RSThread.h>

RSPrivate void *__RSRunLoopGetQueue(RSRunLoopRef rl)
{
    return rl->_queue;
}

RSPrivate void __RSRunLoopDeallocate()
{
//    __RSRuntimeSetClassTypeID(&__RSRunLoopModeClass, __RSRunLoopModeTypeID);
//    __runloop_malloc_vm_pressure_setup();
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
    RSRunLoopTimerRef timer = RSRunLoopTimerCreateWithHandler(RSAllocatorSystemDefault, RSAbsoluteTimeGetCurrent() + timeInterval, 0, 0, 0, ^(RSRunLoopTimerRef timer) {
        perform();
    });
    RSRunLoopAddTimer(RSRunLoopGetCurrent(), timer, RSRunLoopDefaultMode);
    RSRelease(timer);
    return;
}

RSExport void RSRunLoopPerformBlockInQueue(RSTypeRef queue, void (^perform)(void))
{
    RSRunLoopPerformBlock((RSRunLoopRef)queue, RSRunLoopDefaultMode, perform);
    return;
}

#endif
