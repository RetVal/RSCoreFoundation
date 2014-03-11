//
//  RSRuntimeVersion.c
//  RSCoreFoundation
//
//  Created by RetVal on 10/14/12.
//  Copyright (c) 2012 RetVal. All rights reserved.
//

#include <stdio.h>
#include <RSCoreFoundation/RSBase.h>
#include <RSCoreFoundation/RSRuntime.h>
static RSRuntimeClassVersion __RSRuntimeClassInavailableVersion = {0,0,0};
RSInline RSRuntimeClassVersion* __RSRuntimeVersion(const RSRuntimeClass* const cls)
{

    if (cls) {
        RSRuntimeClassVersion* version = (RSRuntimeClassVersion*)&cls->version;
        return version;
    }
    return &__RSRuntimeClassInavailableVersion;
}

#define __RSRuntimeVersionCheckInavailable(version) ((version) == (&__RSRuntimeClassInavailableVersion) ? (YES): (NO))

RSPrivate int __RSRuntimeVersionRetainConstomer(const RSRuntimeClass* const cls)
{
    RSRuntimeClassVersion* restrict version = __RSRuntimeVersion(cls);
    if (!__RSRuntimeVersionCheckInavailable(version)) {
        if ((version->reftype & _RSRuntimeCustomRefCount) && !cls->refcount) {
            return NO;
        }
        else if ((version->reftype & _RSRuntimeCustomRefCount) && cls->refcount)
        {
            return YES;
        }
        return NO;
    }
    return NO;
}

RSExport RSTypeID __RSRuntimeGetClassTypeID(const RSRuntimeClass* const cls)
{
    RSRuntimeClassVersion* version = __RSRuntimeVersion(cls);
    if (!__RSRuntimeVersionCheckInavailable(version)) {
        return version->Id;
    }
    return NO;
}

RSExport void __RSRuntimeSetClassTypeID(const RSRuntimeClass* const cls, RSTypeID id)
{
    RSRuntimeClassVersion* version = __RSRuntimeVersion(cls);
    if (!__RSRuntimeVersionCheckInavailable(version)) {
        version->Id = id;
        return;
    }
    return;
}

RSPrivate void __RSRuntimeSetInstanceTypeID(RSTypeRef obj, RSTypeID id)
{
    //__RSRuntimeVersionCheckInavailable(obj)
    if ((id != _RSRuntimeNotATypeID)) {
        ((RSRuntimeBase*)obj)->_rsinfo._objId = (RSBitU32)id;
    }
}
