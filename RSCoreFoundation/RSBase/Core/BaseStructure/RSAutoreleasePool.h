//
//  RSAutoreleasePool.h
//  RSCoreFoundation
//
//  Created by RetVal on 12/11/12.
//  Copyright (c) 2012 RetVal. All rights reserved.
//

#ifndef RSCoreFoundation_RSAutoreleasePool_h
#define RSCoreFoundation_RSAutoreleasePool_h

#include <RSCoreFoundation/RSBase.h>
#include <RSCoreFoundation/RSString.h>
RS_EXTERN_C_BEGIN
typedef struct __RSAutoreleasePool* RSAutoreleasePoolRef;

RSExport RSTypeID RSAutoreleasePoolGetTypeID(void)  RS_AVAILABLE(0_1);
RSExport RSAutoreleasePoolRef RSAutoreleasePoolCreate(RSAllocatorRef allocator) RS_AVAILABLE(0_2);

RSExport void RSAutoreleasePoolDrain(RSAutoreleasePoolRef pool) RS_AVAILABLE(0_2);

RSExport void RSAutoreleaseBlock(void (^do_block)(void)) RS_AVAILABLE(0_3);

RSExport void RSAutoreleasePoolPush(void) RS_AVAILABLE(0_4);
RSExport void RSAutoreleasePoolPop(void) RS_AVAILABLE(0_4);
RS_EXTERN_C_END
#endif
