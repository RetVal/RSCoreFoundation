//
//  RSList.h
//  RSCoreFoundation
//
//  Created by closure on 1/30/14.
//  Copyright (c) 2014 RetVal. All rights reserved.
//

#ifndef RSCoreFoundation_RSList
#define RSCoreFoundation_RSList

#include <RSCoreFoundation/RSCoreFoundation.h>

RS_EXTERN_C_BEGIN

typedef const struct __RSList *RSListRef;

RSExport RSTypeID RSListGetTypeID(void);
RSExport RSListRef RSListCreate(RSAllocatorRef allocator, RSTypeRef a, ...);
RSExport RSListRef RSListCreateWithArray(RSAllocatorRef allocator, RSArrayRef array);
RSExport RSIndex RSListGetCount(RSListRef list);
RSExport void RSListApplyBlock(RSListRef list, RSRange range, void (^fn)(RSTypeRef value, BOOL *isStop));
RSExport RSListRef RSListCreateDrop(RSAllocatorRef allocator, RSListRef list, RSIndex n);
RSExport RSArrayRef RSListCreateArray(RSListRef list);
RS_EXTERN_C_END
#endif 
