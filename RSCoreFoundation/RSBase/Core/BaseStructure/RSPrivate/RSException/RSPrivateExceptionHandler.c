//
//  RSPrivateExceptionHandler.c
//  RSCoreFoundation
//
//  Created by RetVal on 5/18/13.
//  Copyright (c) 2013 RetVal. All rights reserved.
//

#include "RSPrivateExceptionHandler.h"
#include <RSCoreFoundation/RSRuntime.h>
#include <pthread.h>
#include <Block.h>
static size_t const __kPageSize = 4096;
static pthread_key_t const __RSExceptionHandlerKey = RSExceptionKey;
struct ___RSExceptionHandlerPool {
    pthread_t _thread;
    struct ___RSExceptionHandlerPool *_parent;
    struct ___RSExceptionHandlerPool *_child;
    _exception_handler_block **_next;
};

static void tls_dealloc(void *p);

RSInline _exception_handler_block **__poolBegin(struct ___RSExceptionHandlerPool *pool)
{
    return (_exception_handler_block **)((uint8_t *)pool + sizeof(*pool));
}

RSInline _exception_handler_block **__poolEnd(struct ___RSExceptionHandlerPool *pool)
{
    return (_exception_handler_block **)(pool + __kPageSize);
}

RSInline BOOL __poolEmpty(struct ___RSExceptionHandlerPool *pool)
{
    return pool->_next == __poolBegin(pool);
}

RSInline BOOL __poolFull(struct ___RSExceptionHandlerPool *pool)
{
    return pool->_next == __poolEnd(pool);
}

RSInline BOOL __poolLessThanHalfFull(struct ___RSExceptionHandlerPool *pool)
{
    return (pool->_next - __poolBegin(pool) < (__poolEnd(pool) - __poolBegin(pool)) / 2);
}

RSInline _exception_handler_block **__poolAdd(struct ___RSExceptionHandlerPool *pool, _exception_handler_block* block)
{
    assert(!__poolFull(pool));
    *pool->_next++ = block;
    return pool->_next - 1;
}

static struct ___RSExceptionHandlerPool * poolForPointer(const void* p)
{
    uintptr_t px = (uintptr_t)p;
    struct ___RSExceptionHandlerPool *result;
    uintptr_t offset = px % __kPageSize;
    
    assert(offset >= sizeof(struct ___RSExceptionHandlerPool));
    
    result = (struct ___RSExceptionHandlerPool *)(px - offset);
    return result;
}

RSInline struct ___RSExceptionHandlerPool * hotPool()
{
//    struct ___RSExceptionHandlerPool *result = (struct ___RSExceptionHandlerPool *)pthread_getspecific(__RSExceptionHandlerKey);
    struct ___RSExceptionHandlerPool *result = (struct ___RSExceptionHandlerPool *)_RSGetTSD(__RSTSDKeyExceptionData);
    return result;
}

RSInline struct ___RSExceptionHandlerPool * setHotPool(struct ___RSExceptionHandlerPool *pool)
{
//    pthread_setspecific(__RSExceptionHandlerKey, (void *)pool);
    _RSSetTSD(__RSTSDKeyExceptionData, pool, tls_dealloc);
    return pool;
}

RSInline struct ___RSExceptionHandlerPool * coolPool()
{
    struct ___RSExceptionHandlerPool *result = hotPool();
    if (result) {
        while (result->_parent) {
            result = result->_parent;
        }
    }
    return result;
}

struct ___RSExceptionHandlerPool *___exception_create_pool(RSAllocatorRef allocator, struct ___RSExceptionHandlerPool*parent)
{
    
    struct ___RSExceptionHandlerPool* p = (struct ___RSExceptionHandlerPool*)RSAllocatorVallocate(allocator, __kPageSize);
    if (parent) p->_parent = parent;
    p->_thread = pthread_self();
    int32_t r __unused = (int32_t)_RSSetTSD(__RSTSDKeyExceptionData, p, tls_dealloc);
//    pthread_key_init_np(__RSExceptionHandlerKey, tls_dealloc);
    p->_next = __poolBegin(p);
    return p;
}

static __attribute__((noinline)) _exception_handler_block ** pushHandlerSlow(_exception_handler_block *handler)
{
    struct ___RSExceptionHandlerPool *pool = hotPool();
    assert(!pool || __poolFull(pool));
    if (pool == nil) pool = ___exception_create_pool(RSAllocatorSystemDefault, nil);
    else {
        do {
            if (pool->_child) pool = pool->_child;
            else pool = ___exception_create_pool(RSAllocatorSystemDefault, pool);
        } while (__poolFull(pool));
    }
    setHotPool(pool);
    
    return __poolAdd(pool, handler);
}

RSInline _exception_handler_block **pushHandlerFast(_exception_handler_block *handler)
{
    struct ___RSExceptionHandlerPool *pool = hotPool();
    if (pool && !__poolFull(pool)) return __poolAdd(pool, handler);
    return pushHandlerSlow(handler);
}

RSPrivate void __exceptionPoolPopUnitl(struct ___RSExceptionHandlerPool *pool, _exception_handler_block **stop)
{
    while (pool && pool->_next != stop) {
        struct ___RSExceptionHandlerPool *hpool = hotPool();
        if (hpool == nil) __HALT();
        while (__poolEmpty(hpool)) {
            hpool = hpool->_parent;
            setHotPool(hpool);
        }
        
        _exception_handler_block *handler = *--hpool->_next;
        if (handler != nil) Block_release(handler);
        else __HALT();
    }
    setHotPool(pool);
}

static void __exceptionPoolPopAll(struct ___RSExceptionHandlerPool *pool)
{
    __exceptionPoolPopUnitl(pool, __poolBegin(pool));
}

static void __exceptionPoolKill(struct ___RSExceptionHandlerPool *pool)
{
    while (pool->_child) pool = pool->_child;
    
    struct ___RSExceptionHandlerPool *deathptr = nil;
    do {
        deathptr = pool;
        pool = pool->_parent;
        if (pool) pool->_child = nil;
        if (deathptr) {
            // deallocate pool
            __exceptionPoolPopAll(deathptr);
            RSAllocatorDeallocate(RSAllocatorSystemDefault, deathptr);
        }
    } while (pool && deathptr != pool);
}

RSInline _exception_handler_block *__exceptionPushHandler(_exception_handler_block *handler)
{
    assert(handler);
    _exception_handler_block **dest __unused = pushHandlerFast(handler);
    assert(!dest || *dest == handler);
    return handler;
}

static _exception_handler_block *__poolDelete(struct ___RSExceptionHandlerPool *pool)
{
    assert(!__poolEmpty(pool));
    _exception_handler_block *block = *(pool->_next - 1);
    *(pool->_next- 1) = nil;
    pool->_next--;
    if (__poolEmpty(pool))
    {
        struct ___RSExceptionHandlerPool *deathptr = pool;
        pool = pool->_parent;
        if (pool) pool->_child = nil;
        setHotPool(pool);
        if (deathptr) RSAllocatorDeallocate(RSAllocatorSystemDefault, deathptr);
    }
    return block;
}

static _exception_handler_block *popHandlerSlow()
{
    struct ___RSExceptionHandlerPool *pool = hotPool();
    assert(!pool || __poolEmpty(pool));
    if (pool == nil) pool = ___exception_create_pool(RSAllocatorSystemDefault, nil);
    else {
        do {
            if (pool->_parent) pool = pool->_parent;
            else {
                // parent is nil, and the pool is empty! can not pop any handler!
                __RSLogShowWarning(RSSTR("Exception Stack is empty!"));
                return nil;
            }
        } while (!__poolEmpty(pool));
    }
    setHotPool(pool);
    return __poolDelete(pool);
}

RSInline _exception_handler_block *__exceptionPopHandler()
{
    struct ___RSExceptionHandlerPool *pool = hotPool();
    if (pool && !__poolEmpty(pool)) return __poolDelete(pool);
    return popHandlerSlow();
}

RSInline _exception_handler_block *__exceptionStackTop()
{
    struct ___RSExceptionHandlerPool *pool = hotPool();
    _exception_handler_block *block = nil;
    if (pool && !__poolEmpty(pool))
        block = *(pool->_next - 1);
    return block;
}

RSInline _exception_handler_block **push()
{
    if (!hotPool()) setHotPool(___exception_create_pool(RSAllocatorSystemDefault, nil));
    _exception_handler_block **h = pushHandlerFast(nil);
    assert(*h == nil);
    return h;
}

RSInline void pop(_exception_handler_block **h)
{
    struct ___RSExceptionHandlerPool *pool = nil;
    _exception_handler_block **block = nil;
    if (h)
    {
        pool = poolForPointer(h);
        block = (_exception_handler_block **)h;
        assert(*h == nil);
    }
    else
    {
        pool = coolPool();
        assert(pool);
        block = __poolBegin(pool);
    }
    
    __exceptionPoolPopUnitl(pool, block);
    
    if (!h)
    {
        __exceptionPoolKill(pool);
        setHotPool(nil);
    }
    else if (pool->_child)
    {
        if (__poolLessThanHalfFull(pool))
        {
            __exceptionPoolKill(pool->_child);
        }
        else if (pool->_child->_child)
        {
            __exceptionPoolKill(pool->_child->_child);
        }
    }
}

static void tls_dealloc(void *p)
{
    setHotPool(p);
    pop(0);
    setHotPool(nil);
}

RSPrivate void __tls_add_exception_handler(_exception_handler_block *handler)
{
    __exceptionPushHandler(handler);
}

RSPrivate void __tls_cls_exception_handler(_exception_handler_block *handler)
{
    _exception_handler_block *b = __exceptionPopHandler();
    Block_release(b);
}

RSPrivate BOOL __tls_do_exception_with_handler(RSExceptionRef e)
{
    if (e == nil) return NO;
    _exception_handler_block *b = __exceptionStackTop();
    if (b) (*b)(e);
    else return NO;
    return YES;
}
