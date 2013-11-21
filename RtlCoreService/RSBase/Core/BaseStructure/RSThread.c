//
//  RSThread.c
//  RSCoreFoundation
//
//  Created by RetVal on 4/9/13.
//  Copyright (c) 2013 RetVal. All rights reserved.
//

#include <RSCoreFoundation/RSDictionary.h>
#include <RSCoreFoundation/RSException.h>
#include <RSCoreFoundation/RSNumber.h>
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
    OSMemoryBarrier(); // ensure arg is fully initialized and set in memory
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

