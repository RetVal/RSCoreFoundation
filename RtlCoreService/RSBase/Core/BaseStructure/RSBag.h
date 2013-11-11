//
//  RSBag.h
//  RSCoreFoundation
//
//  Created by Closure on 11/10/13.
//  Copyright (c) 2013 RetVal. All rights reserved.
//

#ifndef RSCoreFoundation_RSBag
#define RSCoreFoundation_RSBag

#include <RSCoreFoundation/RSCoreFoundation.h>

RS_EXTERN_C_BEGIN

RSExport RSTypeID RSBagGetTypeID();
typedef const void *	(*RSBagRetainCallBack)(RSAllocatorRef allocator, const void *value);
typedef void		(*RSBagReleaseCallBack)(RSAllocatorRef allocator, const void *value);
typedef RSStringRef	(*RSBagCopyDescriptionCallBack)(const void *value);
typedef BOOL		(*RSBagEqualCallBack)(const void *value1, const void *value2);
typedef RSHashCode	(*RSBagHashCallBack)(const void *value);
typedef struct {
    RSIndex				version;
    RSBagRetainCallBack			retain;
    RSBagReleaseCallBack		release;
    RSBagCopyDescriptionCallBack	copyDescription;
    RSBagEqualCallBack			equal;
    RSBagHashCallBack			hash;
} RSBagCallBacks;

RSExport const RSBagCallBacks RSTypeBagCallBacks;
RSExport const RSBagCallBacks RSCopyStringBagCallBacks;

typedef void (*RSBagApplierFunction)(const void *value, void *context);

typedef const struct __RSBag * RSBagRef;
typedef struct __RSBag * RSMutableBagRef;

RSExport RSTypeID RSBagGetTypeID(void);

RSExport RSBagRef RSBagCreate(RSAllocatorRef allocator, const void **values, RSIndex numValues, const RSBagCallBacks *callBacks);

RSExport RSBagRef RSBagCreateCopy(RSAllocatorRef allocator, RSBagRef theBag);

RSExport RSMutableBagRef RSBagCreateMutable(RSAllocatorRef allocator, RSIndex capacity, const RSBagCallBacks *callBacks);

RSExport RSMutableBagRef RSBagCreateMutableCopy(RSAllocatorRef allocator, RSIndex capacity, RSBagRef theBag);

RSExport RSIndex RSBagGetCount(RSBagRef theBag);

RSExport RSIndex RSBagGetCountOfValue(RSBagRef theBag, const void *value);

RSExport BOOL RSBagContainsValue(RSBagRef theBag, const void *value);

RSExport const void *RSBagGetValue(RSBagRef theBag, const void *value);

RSExport BOOL RSBagGetValueIfPresent(RSBagRef theBag, const void *candidate, const void **value);

RSExport void RSBagGetValues(RSBagRef theBag, const void **values);

RSExport void RSBagApplyFunction(RSBagRef theBag, RSBagApplierFunction applier, void *context);

RSExport void RSBagAddValue(RSMutableBagRef theBag, const void *value);

RSExport void RSBagReplaceValue(RSMutableBagRef theBag, const void *value);

RSExport void RSBagSetValue(RSMutableBagRef theBag, const void *value);

RSExport void RSBagRemoveValue(RSMutableBagRef theBag, const void *value);

RSExport void RSBagRemoveAllValues(RSMutableBagRef theBag);

RS_EXTERN_C_END
#endif 
