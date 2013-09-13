//
//  RSStack.h
//  RSKit
//
//  Created by RetVal on 12/30/12.
//  Copyright (c) 2012 RetVal. All rights reserved.
//

#ifndef RSKit_RSStack_h
#define RSKit_RSStack_h

typedef struct __RSStack* RSStackRef RS_AVAILABLE(0_1);
typedef const void *	(*RSStackRetainCallBack)(const void *value);
typedef void            (*RSStackReleaseCallBack)(const void *value);
typedef RSStringRef     (*RSStackCopyDescriptionCallBack)(const void *value);
typedef RSComparisonResult (*RSStackCompareCallBack)(const void *value1, const void *value2);
typedef RSHashCode      (*RSStackHashCallBack)(const void *value);

typedef struct __RSStackContext
{
    RSIndex version;
    RSStackRetainCallBack retain;
    RSStackReleaseCallBack release;
    RSStackCopyDescriptionCallBack description;
    RSStackCompareCallBack compare;
    RSStackHashCallBack hash;
}RSStackContext RS_AVAILABLE(0_1);
RSExport const RSStackContext* kRSStackRSTypeContext RS_AVAILABLE(0_1);

RSExport RSTypeID RSStackGetTypeID() RS_AVAILABLE(0_1);
#endif
