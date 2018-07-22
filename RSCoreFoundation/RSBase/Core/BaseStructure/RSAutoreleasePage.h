//
//  RSAutoreleasePage.h
//  RSKit
//
//  Created by RetVal on 12/10/12.
//  Copyright (c) 2012 RetVal. All rights reserved.
//

#ifndef __RSKit__RSAutoreleasePage__
#define __RSKit__RSAutoreleasePage__

#include <iostream>
#include <malloc/malloc.h>
#include <pthread.h>
#include "RSRuntime.h"
extern int       pthread_key_init_np(int, void (*)(void *));
namespace RSAutoreleaseZone{
struct magic_t
{
    static const uint32_t M0 = 0xA1A1A1A1;
#   define M1 "AUTORELEASE!"
    static const size_t M1_len = 12;
    uint32_t m[4];
    
    magic_t() {
        assert(M1_len == strlen(M1));
        assert(M1_len == 3 * sizeof(m[1]));
        
        m[0] = M0;
        strncpy((char *)&m[1], M1, M1_len);
    }
    
    ~magic_t() {
        m[0] = m[1] = m[2] = m[3] = 0;
    }
    
    BOOL magic_check() const {
        return (m[0] == M0 && 0 == strncmp((char *)&m[1], M1, M1_len));
    }
    
    BOOL fastcheck() const {
#ifdef NDEBUG
        return (m[0] == M0);
#else
        return magic_check();
#endif
    }
    
#   undef M1
};

#define RSAutoreleasePageKey    0xA1A2A3A4
class RSAutoreleasePage
{
    static pthread_key_t const key = RSAutoreleasePageKey;
    static uint8_t const SCRIBBLE = 0xA3;  // 0xA3A3A3A3 after releasing
    static size_t const SIZE =
#if PROTECT_AUTORELEASEPOOL
    4096;  // must be multiple of vm page size
#else
    4096;  // size and alignment, power of 2
#endif
    static size_t const COUNT = SIZE / sizeof(RSTypeRef);
    
    magic_t const magic;
    RSTypeRef* next;
    pthread_t const thread;
    RSAutoreleasePage* const parent;
    RSAutoreleasePage* child;
    uint32_t const depth;
    uint32_t hiwat;
    
    // SIZE-sizeof(*this) bytes of contents follow
    
    static void * operator new(size_t size) {
        return malloc_zone_memalign(malloc_default_zone(), SIZE, SIZE);
    }
    static void operator delete(void * p) {
        return free(p);
    }
    
    inline void protect() {
#if PROTECT_AUTORELEASEPOOL
        mprotect(this, SIZE, PROT_READ);
        check();
#endif
    }
    
    inline void unprotect() {
#if PROTECT_AUTORELEASEPOOL
        check();
        mprotect(this, SIZE, PROT_READ | PROT_WRITE);
#endif
    }
    static inline RSAutoreleasePage *coolPage()
    {
        RSAutoreleasePage *result = hotPage();
        if (result) {
            while (result->parent) {
                result = result->parent;
                result->fastcheck();
            }
        }
        return result;
    }
    
//private:
    void busted(BOOL die = YES)
    {
        __RSCLog(RSLogLevelDebug, 
        "autorelease pool page %p corrupted\n"
         "  magic 0x%08x 0x%08x 0x%08x 0x%08x\n  pthread %p\n",
         this, magic.m[0], magic.m[1], magic.m[2], magic.m[3],
         this->thread);
    }
    
    void selfCheck(BOOL die = YES)
    {
        if (!magic.magic_check() || !pthread_equal(thread, pthread_self())) {
            busted(die);
        }
    }
    
    void fastcheck(bool die = true)
    {
        if (! magic.fastcheck()) {
            busted(die);
        }
    }
    
    RSTypeRef * begin() {
        return (RSTypeRef *) ((uint8_t *)this+sizeof(*this));
    }
    
    RSTypeRef * end() {
        return (RSTypeRef *) ((uint8_t *)this+SIZE);
    }
    
    BOOL empty() {
        return next == begin();
    }
    
    BOOL full() {
        return next == end();
    }
    
    BOOL lessThanHalfFull() {
        return (next - begin() < (end() - begin()) / 2);
    }
    RSTypeRef* add(RSTypeRef obj);

    static RSAutoreleasePage* hotPage();
    static RSAutoreleasePage* setHotPage(RSAutoreleasePage* pool);
    static RSAutoreleasePage* pageForPointer(const void* p)
    {
        uintptr_t px = (uintptr_t)p;
        RSAutoreleasePage *result;
        uintptr_t offset = px % SIZE;
        
        assert(offset >= sizeof(RSAutoreleasePage));
        
        result = (RSAutoreleasePage *)(px - offset);
        result->fastcheck();
        
        return result;
    }
        
    void releaseAll()
    {
        releaseUntil(begin());
    }
    
    void releaseUntil(RSTypeRef *stop)
    {
        // Not recursive: we don't want to blow out the stack
        // if a thread accumulates a stupendous amount of garbage
        
        while (this->next != stop) {
            // Restart from hotPage() every time, in case -release
            // autoreleased more objects
            RSAutoreleasePage *pool = hotPage();
            
            // fixme I think this `while` can be `if`, but I can't prove it
            while (pool->empty()) {
                pool = pool->parent;
                setHotPage(pool);
            }
            
            pool->unprotect();
            RSTypeRef obj = *--pool->next;
            memset((void*)pool->next, SCRIBBLE, sizeof(*pool->next));
            pool->protect();
            
            if (obj != nil) {
                RSRelease(obj);
            }
        }
        
        setHotPage(this);
        
#ifndef NDEBUG
        // we expect any children to be completely empty
        for (RSAutoreleasePage *pool = child; pool; pool = pool->child) {
            assert(pool->empty());
        }
#endif
    }
    
    void kill()
    {
        // Not recursive: we don't want to blow out the stack
        // if a thread accumulates a stupendous amount of garbage
        RSAutoreleasePage *pool = this;
        while (pool->child) pool = pool->child;
        
        RSAutoreleasePage *deathptr;
        do {
            deathptr = pool;
            pool = pool->parent;
            if (pool) {
                pool->unprotect();
                pool->child = NULL;
                pool->protect();
            }
            delete deathptr;
        } while (deathptr != this);
    }
    
    static void tls_dealloc(void *p)
    {
        // reinstate TLS value while we work
        setHotPage((RSAutoreleasePage *)p);
        pop(0);
        setHotPage(NULL);
    }
    
    static inline RSTypeRef* autoreleaseFast(RSTypeRef obj)
    {
        RSAutoreleasePage* pool = hotPage();
        if (pool && !pool->full()) {
            return pool->add(obj);
        }
        return pool->autoreleaseSlow(obj);
    }
    
    static __attribute__((noinline)) RSTypeRef* autoreleaseSlow(RSTypeRef obj)
    {
        RSAutoreleasePage* pool = hotPage();
        // The code below assumes some cases are handled by autoreleaseFast()
        assert(!pool || pool->full());
        do {
            if (pool->child) pool = pool->child;
            else pool = new RSAutoreleasePage(pool);
        } while (pool->full());
        setHotPage(pool);
        return pool->add(obj);
    }

    
    static inline void *push()
    {
        if (!hotPage()) {
            setHotPage(new RSAutoreleasePage(NULL));
        }
        RSTypeRef *dest = autoreleaseFast(nil);
        assert(*dest == nil);
        return dest;
    }
    static inline void pop(void *token)
    {
        RSAutoreleasePage *pool;
        RSTypeRef *stop;
        
        if (token) {
            pool = pageForPointer(token);
            stop = (RSTypeRef *)token;
            assert(*stop == nil);
        } else {
            // Token 0 is top-level pool
            pool = coolPage();
            assert(pool);
            stop = pool->begin();
        }
        
        //if (PrintPoolHiwat) printHiwat();
        
        pool->releaseUntil(stop);
        
        // memory: delete empty children
        // hysteresis: keep one empty child if this page is more than half full
        // special case: delete everything for pop(0)
        if (!token) {
            pool->kill();
            setHotPage(NULL);
        } else if (pool->child) {
            if (pool->lessThanHalfFull()) {
                pool->child->kill();
            }
            else if (pool->child->child) {
                pool->child->child->kill();
            }
        }
    }

    static void init()
    {
        int r __unused = pthread_key_init_np(RSAutoreleasePage::key,
                                             RSAutoreleasePage::tls_dealloc);
        assert(r == 0);
    }

    public:
    RSAutoreleasePage(RSAutoreleasePage* newParent);
    ~RSAutoreleasePage();
    static RSTypeRef autorelease(RSTypeRef obj)
    {
        RSAutoreleasePage* page = hotPage();
        RSTypeRef reobj = *(page->autoreleaseFast(obj));
        return reobj ?:*(page->autoreleaseSlow(obj));
    }
};
};  // end of namespace
#endif /* defined(__RSKit__RSAutoreleasePage__) */
