//
//  RSQueue.h
//  RSCoreFoundation
//
//  Created by RetVal on 3/8/13.
//  Copyright (c) 2013 RetVal. All rights reserved.
//

#ifndef RSCoreFoundation_RSQueue_h
#define RSCoreFoundation_RSQueue_h

#include <RSCoreFoundation/RSBase.h>
RS_EXTERN_C_BEGIN
typedef struct __RSQueue*  RSQueueRef;
typedef RS_OPTIONS(RSBitU8, RSQueueAtomType) {
	RSQueueNotAtom = 0,
	RSQueueAtom = 1,
};
RSExport RSTypeID RSQueueGetTypeID() RS_AVAILABLE(0_2);
RSExport RSQueueRef RSQueueCreate(RSAllocatorRef allocator, RSIndex capacity, RSQueueAtomType type) RS_AVAILABLE(0_2);
RSExport RSTypeRef RSQueueDequeue(RSQueueRef queue) RS_AVAILABLE(0_2);
RSExport void RSQueueEnqueue(RSQueueRef queue, RSTypeRef object) RS_AVAILABLE(0_2);
RSExport RSTypeRef RSQueuePeekObject(RSQueueRef queue) RS_AVAILABLE(0_2);
RSExport RSIndex RSQueueGetCapacity(RSQueueRef queue) RS_AVAILABLE(0_2);
RSExport RSIndex RSQueueGetCount(RSQueueRef queue) RS_AVAILABLE(0_2);
RS_EXTERN_C_END
#endif
