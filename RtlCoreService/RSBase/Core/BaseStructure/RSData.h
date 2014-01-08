//
//  RSData.h
//  RSCoreFoundation
//
//  Created by RetVal on 10/21/12.
//  Copyright (c) 2012 RetVal. All rights reserved.
//

#ifndef RSCoreFoundation_RSData_h
#define RSCoreFoundation_RSData_h
#include <RSCoreFoundation/RSBase.h>
RS_EXTERN_C_BEGIN
typedef const struct __RSData* RSDataRef RS_AVAILABLE(0_0);
typedef struct __RSData* RSMutableDataRef RS_AVAILABLE(0_0);

RSExport RSTypeID RSDataGetTypeID() RS_AVAILABLE(0_0);

RSExport RSAllocatorRef RSDataGetAllocator(RSDataRef data) RS_AVAILABLE(0_0);
RSExport const void* RSDataCopyBytes(RSDataRef data) RS_AVAILABLE(0_0);
RSExport __autorelease void* RSDataMutableBytes(RSDataRef data) RS_AVAILABLE(0_4);
RSExport void* RSDataGetMutableBytesPtr(RSDataRef data) RS_AVAILABLE(0_4);
RSExport const void* RSDataGetBytesPtr(RSDataRef data) RS_AVAILABLE(0_0);
RSExport void RSDataGetBytes(RSDataRef data, RSRange range, uint8_t *buffer) RS_AVAILABLE(0_4);
RSExport void RSDataSetLength(RSMutableDataRef data, RSIndex extraLength) RS_AVAILABLE(0_3);
RSExport void RSDataIncreaseLength(RSMutableDataRef data, RSIndex extraLength) RS_AVAILABLE(0_3);
RSExport RSIndex RSDataGetLength(RSDataRef data) RS_AVAILABLE(0_0);
RSExport RSIndex RSDataGetCapacity(RSDataRef data) RS_AVAILABLE(0_4);

RSExport RSDataRef RSDataCreate(RSAllocatorRef allocator, const void* bytes, RSIndex length) RS_AVAILABLE(0_0);
RSExport RSDataRef RSDataCreateCopy(RSAllocatorRef allocator, RSDataRef data) RS_AVAILABLE(0_0);
RSExport RSDataRef RSDataCreateWithNoCopy(RSAllocatorRef allocator, const void* bytes, RSIndex length, BOOL freeWhenDone, RSAllocatorRef bytesDeallocator) RS_AVAILABLE(0_0);

RSExport RSMutableDataRef RSDataCreateMutable(RSAllocatorRef allocator, RSIndex capacity) RS_AVAILABLE(0_0);
RSExport RSMutableDataRef RSDataCreateMutableCopy(RSAllocatorRef allocator, RSDataRef data) RS_AVAILABLE(0_0);
RSExport RSMutableDataRef RSDataCreateMutableWithNoCopy(RSAllocatorRef allocator, const void* bytes, RSIndex length, BOOL freeWhenDone) RS_AVAILABLE(0_0);

RSExport void RSDataAppendBytes(RSMutableDataRef data, const void* appendBytes, RSIndex length) RS_AVAILABLE(0_0);
RSExport void RSDataReplaceBytes(RSMutableDataRef data, RSRange rangeToReplace, const void* replaceBytes, RSIndex length) RS_AVAILABLE(0_0);
RSExport void RSDataDeleteBytes(RSMutableDataRef data, RSRange rangeToDelete, RSIndex length) RS_AVAILABLE(0_0);

RSExport void RSDataAppend(RSMutableDataRef data, RSDataRef appendData) RS_AVAILABLE(0_0);
RSExport void RSDataReplace(RSMutableDataRef data, RSRange rangeToReplace, RSDataRef replaceData) RS_AVAILABLE(0_0);
RSExport void RSDataDelete(RSMutableDataRef data, RSRange rangeToDelete) RS_AVAILABLE(0_0);

RSExport RSComparisonResult RSDataCompare(RSDataRef data1, RSDataRef data2) RS_AVAILABLE(0_0);

RSExport void RSDataLog(RSDataRef data) RS_AVAILABLE(0_0);
RS_EXTERN_C_END
#endif
