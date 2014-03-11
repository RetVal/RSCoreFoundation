//
//  RSAutoreleasePool.c
//  RSCoreFoundation
//
//  Created by RetVal on 12/11/12.
//  Copyright (c) 2012 RetVal. All rights reserved.
//

#include <stdio.h>
#include <RSCoreFoundation/RSBase.h>
#include <RSCoreFoundation/RSAutoreleasePool.h>
#include <RSCoreFoundation/RSThread.h>
#include <RSCoreFoundation/RSNil.h>
#include <pthread.h>

#include <RSCoreFoundation/RSRuntime.h>

#include "RSPrivate/RSPrivateOperatingSystem/RSPrivateTask.h"

struct magic_t
{
    uint32_t M0;
    size_t M1_len;
    uint32_t m[4];
};

#undef M1
#define M1 "AUTORELEASE!"
static const uint32_t magic_M0 = 0xA1A1A1A1;
static const size_t magic_M1_len = 12;

static void tls_dealloc(void *p);

static void magic_t_construtor(struct magic_t* m)
{
    m->M1_len = magic_M1_len;
    m->M0 = magic_M0;
    assert(m->M1_len == strlen(M1));
    assert(m->M1_len == 3 * sizeof(m->m[1]));
    
    m->m[0] = m->M0;
    strncpy((char *)&m->m[1], M1, magic_M1_len);
}

static void magic_t_destructor(struct magic_t* m)
{
    m->m[0] = m->m[1] = m->m[2] = m->m[3] = 0;
}

static BOOL magic_t_check(struct magic_t* m)
{
    return (m->m[0] == magic_M0 && 0 == strncmp((char *)&m->m[1], M1, magic_M1_len));
}

static BOOL magic_t_fastcheck(struct magic_t* m)
{
#ifdef NDEBUG
    return (m->m[0] == magic_M0);
#else
    return magic_t_check(m);
#endif
}


#undef M1

#if __LP64__
#define __poolMask 0xFFFFFFFFFFFFF000
#else
#define __poolMask 0xFFFFF000
#endif

static uint8_t const __RSAutoreleasePoolPageScribble = 0xA3;
static size_t const __RSAutoreleasePoolPageSize = 4096; //getpagesize() = 4096;
RSExport void __RS_AUTORELEASEPOOL_RELEASE__(RSTypeRef obj) __attribute__((noinline));

struct __RSAutoreleasePool
{
    RSRuntimeBase _base;
    struct magic_t _magic;
    RSTypeRef* _next;
    pthread_t _thread;
    RSAutoreleasePoolRef _parent;
    RSAutoreleasePoolRef _child;
    uint32_t _depth;
    uint32_t _hiwat;
};

static void __poolBusted(RSAutoreleasePoolRef pool)
{
    __RSCLog(RSLogLevelDebug, 
    "autorelease pool page %p corrupted\n"
     "  magic 0x%08x 0x%08x 0x%08x 0x%08x\n  pthread %p\n",
     pool, pool->_magic.m[0], pool->_magic.m[1], pool->_magic.m[2], pool->_magic.m[3],
     pool->_thread);
}

static void __poolCheck(RSAutoreleasePoolRef pool)
{
    if (!magic_t_check(&pool->_magic) || !pthread_equal(pool->_thread, pthread_self()))
    {
        __poolBusted(pool);
    }
}

RSInline void __poolFastCheck(RSAutoreleasePoolRef pool)
{
    if (!magic_t_fastcheck(&pool->_magic)) {
        __poolBusted(pool);
    }
}

RSInline RSTypeRef * __poolBegin(RSAutoreleasePoolRef pool) {
    //return (RSTypeRef *) ((uint8_t *)pool+sizeof(*pool));
    return (RSTypeRef *) ((uint8_t *)pool+sizeof(*pool));//(((uintptr_t)pool->_next) & __poolMask);
}

RSInline RSTypeRef * __poolEnd(RSAutoreleasePoolRef pool) {
    //return (RSTypeRef *) ((uint8_t *)pool+sizeof(*pool)+__RSAutoreleasePoolPageSize - 0x10);
    return (RSTypeRef *) ((uint8_t *)pool + __RSAutoreleasePoolPageSize - 0x10);//(RSTypeRef *) pool + __RSAutoreleasePoolPageSize - 16 - sizeof(struct __RSAutoreleasePool);
}

RSInline BOOL __poolEmpty(RSAutoreleasePoolRef pool)
{
    return pool->_next == __poolBegin(pool);
}

RSInline BOOL __poolFull(RSAutoreleasePoolRef pool)
{
    return pool->_next == __poolEnd(pool);
}

RSInline BOOL __poolLessThanHalfFull(RSAutoreleasePoolRef pool) {
    return (pool->_next - __poolBegin(pool) < (__poolEnd(pool) - __poolBegin(pool)) / 2);
}

static RSAutoreleasePoolRef __RSAutoreleasePoolCreateInstance(RSAllocatorRef allocator, RSAutoreleasePoolRef parent);

static RSTypeRef* __poolAdd(RSAutoreleasePoolRef pool, RSTypeRef obj)
{
    assert(!__poolFull(pool));
    //unprotect();
    *pool->_next++ = obj;
    //protect();
    return pool->_next-1;

}

static RSAutoreleasePoolRef poolForPointer(const void* p)
{
    uintptr_t px = (uintptr_t)p;
    RSAutoreleasePoolRef result;
    uintptr_t offset = px % __RSAutoreleasePoolPageSize;
    
    assert(offset >= sizeof(struct __RSAutoreleasePool));
    
    result = (RSAutoreleasePoolRef)(px - offset);
    __poolFastCheck(result);
    
    return result;
}

RSInline RSAutoreleasePoolRef hotPool()
{
    RSAutoreleasePoolRef result = (RSAutoreleasePoolRef)_RSGetTSD(__RSTSDKeyAutoreleaseData1);//(__RSAutoreleasePoolThreadKey);
    if (result) __poolFastCheck(result);
    return result;
}

RSInline RSAutoreleasePoolRef setHotPool(RSAutoreleasePoolRef pool)
{
    if (pool) __poolFastCheck(pool);
//    pthread_setspecific(__RSAutoreleasePoolThreadKey, (void *)pool);
    
    _RSSetTSD(__RSTSDKeyAutoreleaseData1, pool, tls_dealloc);
    return pool;
}

RSInline RSAutoreleasePoolRef coolPool()
{
    RSAutoreleasePoolRef result = hotPool();
    if (result) {
        while (result->_parent) {
            result = result->_parent;
            __poolFastCheck(result);
        }
    }
    return result;
}


static __attribute__((noinline)) RSTypeRef* autoreleaseSlow(RSTypeRef obj)
{
    RSAutoreleasePoolRef pool = hotPool();
    // The code below assumes some cases are handled by autoreleaseFast()
    assert(!pool || __poolFull(pool));
    if (pool == nil) pool = __RSAutoreleasePoolCreateInstance(RSAllocatorSystemDefault, nil);
    else
    {
        do {
            if (pool->_child) pool = pool->_child;
            else pool = __RSAutoreleasePoolCreateInstance(RSAllocatorSystemDefault, pool);
        } while (__poolFull(pool));
    }
    setHotPool(pool);
    return __poolAdd(pool, obj);
}


RSInline RSTypeRef* autoreleaseFast(RSTypeRef obj)
{
    RSAutoreleasePoolRef pool = hotPool();

    if (pool && !__poolFull(pool))
    {
        return __poolAdd(pool, obj);
    }
    return autoreleaseSlow(obj);
}
static void __RSAutoreleasePoolClassDeallocate(RSTypeRef obj);

RSPrivate void __RSAutoreleasePoolReleaseUntil(RSAutoreleasePoolRef pool, RSTypeRef *stop)
{
    // Not recursive: we don't want to blow out the stack
    // if a thread accumulates a stupendous amount of garbage
    while (pool && pool->_next != stop)
    {
        // Restart from hotPage() every time, in case -release
        // autoreleased more objects
        RSAutoreleasePoolRef hpool = hotPool();
        if (hpool == nil) HALTWithError(RSInvalidArgumentException, "autorelease pool is nil");
        // fixme I think this `while` can be `if`, but I can't prove it
        while (__poolEmpty(hpool))
        {
            hpool = hpool->_parent;
            setHotPool(hpool);
        }
        
        RSTypeRef obj = *--hpool->_next;
        memset((void*)hpool->_next, __RSAutoreleasePoolPageScribble, sizeof(*hpool->_next));
        
        if (obj != nil)
        {
#if __RSRuntimeDebugPreference
            if (___RSDebugLogPreference._RSRuntimeCheckAutoreleaseFlag) {
                if (!__RSIsAutorelease(obj))
                {
                    HALTWithError(RSInvalidArgumentException, "RSAutoreleasePool should not have nil!");
                    return;
                }
                if (RSGetRetainCount(obj) == 1)
                {
                    __RSSetUnAutorelease(obj);
                }
            }
#endif
//            __RSCLog(RSLogLevelNotice, "RSAutoreleasePool release %p [%lld][rc = %lld]\n", obj, RSGetTypeID(obj), RSGetRetainCount(obj));
            __RS_AUTORELEASEPOOL_RELEASE__(obj);
        }
        else HALTWithError(RSInvalidArgumentException, "RSAutoreleasePool should not have nil!");
    }
    
    setHotPool(pool);
    
#ifndef NDEBUG
    // we expect any children to be completely empty
    for (RSAutoreleasePoolRef sub = pool->_child; sub; sub = sub->_child)
    {
        assert(__poolEmpty(sub));
    }
#endif
}

static void releaseAll(RSAutoreleasePoolRef pool)
{
    __RSAutoreleasePoolReleaseUntil(pool, __poolBegin(pool));
}



static void __poolKill(RSAutoreleasePoolRef pool)
{
    // Not recursive: we don't want to blow out the stack
    // if a thread accumulates a stupendous amount of garbage
    while (pool->_child) pool = pool->_child;
    
    RSAutoreleasePoolRef deathptr = nil;
    do {
        deathptr = pool;
        pool = pool->_parent;
        if (pool) {
            //pool->unprotect();
            pool->_child = nil;
            //pool->protect();
        }
        if (deathptr) RSDeallocateInstance(deathptr);
    } while (pool && deathptr != pool);
}

RSInline RSTypeRef autorelease(RSTypeRef obj)
{
    assert(obj);
    //assert(!OBJC_IS_TAGGED_PTR(obj));
    RSTypeRef *dest __unused = autoreleaseFast(obj);
    
    assert(!dest  ||  *dest == obj);
    return obj;
}


RSInline void *push()
{
    if (!hotPool()) {
        setHotPool(__RSAutoreleasePoolCreateInstance(RSAllocatorSystemDefault, nil));
    }
    RSTypeRef *dest = autoreleaseFast(RSNil);
    assert(*dest == RSNil);
    return dest;
}
RSInline void pop(void *token)
{
    RSAutoreleasePoolRef pool;
    RSTypeRef *stop;
    
    if (token)
    {
        pool = poolForPointer(token);
        stop = (RSTypeRef *)token;
        assert(*stop == nil);
    }
    else
    {
        // Token 0 is top-level pool
        pool = coolPool();
        assert(pool);
        stop = __poolBegin(pool);
    }
    
    __RSAutoreleasePoolReleaseUntil(pool, stop);
    // memory: delete empty children
    // hysteresis: keep one empty child if this page is more than half full
    // special case: delete everything for pop(0)
    if (!token)
    {
        __poolKill(pool);
        setHotPool(nil);
    }
    else if (pool->_child)
    {
        if (__poolLessThanHalfFull(pool))
        {
            __poolKill(pool->_child);
        }
        else if (pool->_child->_child)
        {
            __poolKill(pool->_child->_child);
        }
    }
}


static void tls_dealloc(void *p)
{
    // reinstate TLS value while we work
    setHotPool((RSAutoreleasePoolRef)p);
    pop(0);
    setHotPool(nil);
}

static RSTypeID _RSAutoreleasePoolTypeID = _RSRuntimeNotATypeID;

static void __RSAutoreleasePoolClassInit(RSTypeRef obj)
{
    RSAutoreleasePoolRef pool = (RSAutoreleasePoolRef)obj;
    magic_t_construtor(&pool->_magic);
    pool->_thread = pthread_self();
    pool->_child = nil;
    if (___RSDebugLogPreference._RSRuntimeInstanceManageWatcher)
        __RSCLog(RSLogLevelDebug, "%s alloc - <%p>\n", "RSAutoreleasePool", obj);
}

static void __RSAutoreleasePoolClassDeallocate(RSTypeRef obj)
{
    RSAutoreleasePoolRef pool = (RSAutoreleasePoolRef)obj;
    __poolCheck(pool);
    assert(__poolEmpty(pool));
    assert(!pool->_child);
    RSAutoreleasePoolRef parent = pool->_parent;
    if (parent)
    {
        __poolCheck(parent);
        assert(parent);
        
        parent->_child = nil;
        pool->_parent = nil;
        setHotPool(parent);
    }
    magic_t_destructor(&pool->_magic);

    pool->_next = (RSTypeRef*)(((uintptr_t)pool->_next) & __poolMask);

    //RSAllocatorDeallocate(RSAllocatorSystemDefault, pool->_next);
}

static BOOL __RSAutoreleasePoolClassEqual(RSTypeRef obj1, RSTypeRef obj2)
{
    RSAutoreleasePoolRef p1 = (RSAutoreleasePoolRef)obj1;
    RSAutoreleasePoolRef p2 = (RSAutoreleasePoolRef)obj2;
    if (p1->_next != p2->_next) return NO;
    if (p1->_parent != p2->_parent) return NO;
    if (p1->_child != p2->_child) return NO;
    if (p1->_depth != p2->_depth) return NO;
    if (p1->_hiwat != p2->_hiwat) return NO;
    if (NO == pthread_equal(p1->_thread, p2->_thread)) return NO;
    return YES;
}

static RSHashCode __RSAutoreleasePoolClassHash(RSTypeRef obj)
{
    RSAutoreleasePoolRef pool = (RSAutoreleasePoolRef)obj;
    return pool->_depth ^ pool->_magic.M0;
}

extern RSStringRef __RSRunLoopThreadToString(pthread_t t);
static RSStringRef __RSAutoreleasePoolClassDescription(RSTypeRef obj)
{
    RSAutoreleasePoolRef pool = (RSAutoreleasePoolRef)obj;
    RSMutableStringRef description = RSStringCreateMutable(RSAllocatorSystemDefault, 128);
    RSStringRef threadname = __RSRunLoopThreadToString(pool->_thread);
    RSStringAppendStringWithFormat(description, RSSTR("RSAutoreleasePool - {\n\tPool: %p,\n\tThread : %R,\n\tstore : %p,\n\tParent - %p,\n\tChild - %p,\n\tDepth - %d\n}"), pool, threadname, pool->_next, pool->_parent, pool->_child, pool->_depth);
    RSRelease(threadname);
    return description;
}

static const RSRuntimeClass __RSAutoreleasePoolClass =
{
    _RSRuntimeScannedObject,
    0,
    "RSAutoreleasePool",
    __RSAutoreleasePoolClassInit,
    nil,
    __RSAutoreleasePoolClassDeallocate,
    __RSAutoreleasePoolClassEqual,
    __RSAutoreleasePoolClassHash,
    __RSAutoreleasePoolClassDescription,
    nil,
    nil
};

RSPrivate void __RSAutoreleasePoolInitialize()
{
    _RSAutoreleasePoolTypeID = __RSRuntimeRegisterClass(&__RSAutoreleasePoolClass);
    __RSRuntimeSetClassTypeID(&__RSAutoreleasePoolClass, _RSAutoreleasePoolTypeID);
}

RSExport RSTypeID RSAutoreleasePoolGetTypeID()
{
    return _RSAutoreleasePoolTypeID;
}

static RSAutoreleasePoolRef __RSAutoreleasePoolCreateInstance(RSAllocatorRef allocator, RSAutoreleasePoolRef parent)
{
//    RSAutoreleasePoolRef pool = (RSAutoreleasePoolRef)__RSRuntimeCreateInstance(allocator, __RSAutoreleasePoolTypeID, sizeof(struct __RSAutoreleasePool) - sizeof(struct __RSRuntimeBase));
    /*
     simulate the runtime to initialize the instance with the simple process.
     */
    allocator = allocator ? RSAllocatorSystemDefault : RSAllocatorSystemDefault;    // must be the RSAllocatorSystemDefault when create instance.
    RSAutoreleasePoolRef pool = nil;
    pool = RSAllocatorVallocate(allocator, __RSAutoreleasePoolPageSize);
    //__RSAllocatorSetHeader(RSAllocatorSystemDefault, pool, __RSAutoreleasePoolPageSize);
    __RSRuntimeSetInstanceTypeID(pool, _RSAutoreleasePoolTypeID);
    const RSRuntimeClass* cls = __RSRuntimeGetClassWithTypeID(_RSAutoreleasePoolTypeID);
    __RSSetValid(pool);
//    if (cls->refcount) {
//        ((RSRuntimeBase*)pool)->_rsinfo._customRef = 1;
//    }else
//        ((RSRuntimeBase*)pool)->_rsinfo._customRef = 0;
    RSRetain(pool);
    __RSRuntimeSetInstanceSpecial(pool, YES);
    if (cls->init) cls->init(pool);
    /*
     end simulation
     */
    
    pool->_next = __poolBegin(pool);
    pool->_parent = parent;
    
    pool->_depth = (parent) ? parent->_depth + 1 : 0;
    pool->_hiwat = (parent) ? parent->_hiwat : 0;
    
    if (pool->_parent)
    {
        __poolCheck(parent);
        assert(parent);
        parent->_child = pool;
    }
//    int r __unused = pthread_key_init_np(__RSAutoreleasePoolThreadKey,
//                                         tls_dealloc);
    int r __unused = _RSSetTSD(__RSTSDKeyAutoreleaseData1, pool, tls_dealloc);
    
//    assert(r == 0);
    void* endptr __unused = __poolEnd(pool);
    return pool;
}

RSPrivate RSAutoreleasePoolRef __RSAutoreleasePoolGetCurrent()
{
    push();
    return hotPool();
}

RSPrivate RSTypeRef __RSAutorelease(RSTypeRef obj)
{
    return autorelease(obj);
}

RSPrivate void __RSAutoreleasePoolDeallocate()
{
    RSAutoreleasePoolRef pool = coolPool();
    if (pool == nil) return;
    pop(0);
    setHotPool(nil);
}

RSExport RSAutoreleasePoolRef RSAutoreleasePoolCreate(RSAllocatorRef allocator)
{
    RSAutoreleasePoolRef pool = __RSAutoreleasePoolCreateInstance(allocator, hotPool());
    if (pool == nil) return nil;
    setHotPool(pool);
    return pool;
}

#if DEPLOYMENT_TARGET_MACOSX || DEPLOYMENT_TARGET_IPHONEOS || DEPLOYMENT_TARGET_MINI || DEPLOYMENT_TARGET_EMBEDDED_MINI
#include <malloc/malloc.h>
static void __runloop_vm_pressure_handler(void *context __unused)
{
	malloc_zone_pressure_relief(0,0);
//    __RSCLog(RSLogLevelWarning, "%s\n", "RSRuntime reduce memory alloc to handle the memory pressure...");
}
#else
#define __runloop_vm_pressure_handler(context) do {}while(0)
#endif

RSExport void __RS_AUTORELEASEPOOL_RELEASE__(RSTypeRef obj) {
    return RSRelease(obj);
}

RSExport void RSAutoreleasePoolDrain(RSAutoreleasePoolRef pool)
{
    if (pool == nil) return;
    __RSGenericValidInstance(pool, _RSAutoreleasePoolTypeID);
    __poolCheck(pool);
    RSAutoreleasePoolRef orginal = pool->_parent ? : pool;
    while (pool->_child)
        pool = pool->_child;
    if (!(pool != nil && pool != orginal))
    {
        //
        releaseAll(pool);
        __RSRuntimeSetInstanceSpecial(pool, NO);
        __RS_AUTORELEASEPOOL_RELEASE__(pool);
        setHotPool(nil);
        return;
    }
    
    while (pool != nil && pool != orginal)
    {
        releaseAll(pool);
        pool = pool->_parent;
        __RSRuntimeSetInstanceSpecial(pool->_child, NO);
        __RS_AUTORELEASEPOOL_RELEASE__(pool->_child);
        pool->_child = nil;
    }
    setHotPool(orginal);
//    __runloop_vm_pressure_handler(nil);
}

RSExport void RSAutoreleaseBlock(void (^do_block)())
{
    if (do_block)
    {
        
        RSAutoreleasePoolRef p = RSAutoreleasePoolCreate(RSAllocatorSystemDefault);
        do_block();
        RSAutoreleasePoolDrain(p);
    }
}

RSExport void RSAutoreleaseShowCurrentPool()
{
    RSShow(__RSAutoreleasePoolGetCurrent());
}

#undef __poolMask
