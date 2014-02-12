//
//  RSNil.c
//  RSCoreFoundation
//
//  Created by RetVal on 10/16/12.
//  Copyright (c) 2012 RetVal. All rights reserved.
//

#include <stdio.h>
#include <RSCoreFoundation/RSBase.h>
#include <RSCoreFoundation/RSRuntime.h>
#include "RSNil.h"
struct __RSNil {
    RSRuntimeBase _base;
};

static struct __RSNil __RSNil = {
    RSRuntimeBaseDefault()
};

const RSNilRef RSNil = &__RSNil;
const RSNilRef RSNill = &__RSNil;
const RSNilRef RSNull = &__RSNil;
void __RSNilClassInit(RSTypeRef obj)
{
    RSNilRef knil = (RSNilRef)obj;
    __RSRuntimeSetInstanceSpecial(knil, YES);
    return;
}

RSTypeRef __RSNilClassCopy(RSAllocatorRef allocator, RSTypeRef obj, BOOL mutableCopy)
{
    return (RSTypeRef)RSNil;
}

void __RSNilClassDeallocate(RSTypeRef obj)
{
    return;
}

BOOL __RSNilClassEqual(RSTypeRef obj1 , RSTypeRef obj2)
{
    if (obj1 == obj2) {
        return YES;
    }
    RSTypeID ID1 = RSGetTypeID(obj1);
    RSTypeID ID2 = RSGetTypeID(obj2);
    if (ID1 == ID2) {
        if (ID1 == RSNilGetTypeID()) {
            return YES;
        }
        else return NO;
    }
    return YES;
}

RSStringRef __RSNilClassDescription(RSTypeRef obj)
{
    return RSRetain(RSSTR("RSNil"));
}
//RSHashCode __RSNilClassHash(RSTypeRef obj)
//{
//    RSTypeID ID = RSGetTypeID(obj);
//    if (ID == RSGetNilType()) {
//        return *(RSHashCode*)RSNil;
//    }
//    return 0;
//}

void __RSNilClassReclaim(RSTypeRef obj)
{
    return;
}

RSUInteger __RSNilClassRefcount(intptr_t op, RSTypeRef obj)
{
    return 1;
}


static RSRuntimeClass __RSNilClass = {
    _RSRuntimeScannedObject,
    0,
    "RSNil",
    __RSNilClassInit,
    __RSNilClassCopy,
    __RSNilClassDeallocate,
    __RSNilClassEqual,
    nil,
    __RSNilClassDescription,
    __RSNilClassReclaim,
    __RSNilClassRefcount,
};

static RSTypeID _RSNilTypeID = _RSRuntimeNotATypeID;

RSPrivate void __RSNilInitialize()
{
    _RSNilTypeID = __RSRuntimeRegisterClass(&__RSNilClass);
    __RSRuntimeSetClassTypeID(&__RSNilClass, _RSNilTypeID);
    __RSRuntimeSetInstanceTypeID(&__RSNil, _RSNilTypeID);
}

RSExport RSTypeID RSNilGetTypeID()
{
    return _RSNilTypeID;
}

RSNilRef RSNilCreate()
{
    return RSNil;
}

