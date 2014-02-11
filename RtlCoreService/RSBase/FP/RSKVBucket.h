//
//  RSKVBucket.h
//  RSCoreFoundation
//
//  Created by closure on 2/2/14.
//  Copyright (c) 2014 RetVal. All rights reserved.
//

#ifndef RSCoreFoundation_RSKVBucket
#define RSCoreFoundation_RSKVBucket

#include <RSCoreFoundation/RSCoreFoundation.h>

RS_EXTERN_C_BEGIN

typedef struct __RSKVBucket *RSKVBucketRef;

RSExport RSTypeID RSKVBucketGetTypeID();
RSExport RSKVBucketRef RSKVBucketCreate(RSAllocatorRef allocator, RSTypeRef key, RSTypeRef value);
RSExport RSTypeRef RSKVBucketGetKey(RSKVBucketRef bucket);
RSExport RSTypeRef RSKVBucketGetValue(RSKVBucketRef bucket);

RS_EXTERN_C_END
#endif 
