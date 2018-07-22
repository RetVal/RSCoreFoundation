//
//  RSAutoreleasePage.cpp
//  RSKit
//
//  Created by RetVal on 12/10/12.
//  Copyright (c) 2012 RetVal. All rights reserved.
//

#include "RSAutoreleasePage.h"
#include "RSRuntime.h"
RSAutoreleaseZone::RSAutoreleasePage::RSAutoreleasePage(RSAutoreleasePage* newParent)
:magic(), next(begin()), thread(pthread_self()), parent(newParent), child(nil),
depth((newParent) ? (newParent->depth + 1) : 0),
hiwat((newParent) ? (newParent->hiwat) : 0)
{
    if (parent)
    {
        parent->selfCheck();
        assert(!parent->child);
        parent->unprotect();
        parent->child = this;
        parent->protect();
    }
    protect();
}

RSAutoreleaseZone::RSAutoreleasePage::~RSAutoreleasePage()
{
    selfCheck();
    unprotect();
    assert(empty());
    
    // Not recursive: we don't want to blow out the stack
    // if a thread accumulates a stupendous amount of garbage
    assert(!child);
}

RSTypeRef * RSAutoreleaseZone::RSAutoreleasePage::add(RSTypeRef obj)
{
    assert(!full());
    unprotect();
    *next++ = obj;
    protect();
    return next-1;
}

RSAutoreleaseZone::RSAutoreleasePage* RSAutoreleaseZone::RSAutoreleasePage::hotPage()
{
    RSAutoreleasePage *result = (RSAutoreleasePage *)
    pthread_getspecific(key);
    if (result) result->fastcheck();
    return result;
}

RSAutoreleaseZone::RSAutoreleasePage* RSAutoreleaseZone::RSAutoreleasePage::setHotPage(RSAutoreleasePage* pool)
{
    if (pool) pool->fastcheck();
    pthread_setspecific(key, (void *)pool);
    return pool;
}

RSPrivate void __RSAutoreleasePageInitialize()
{
    if (!pthread_main_np()) exit(0xA1);
}

RSPrivate void __RSAutoreleasePageDeallocate()
{
    
}