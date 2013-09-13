//
//  RSData+CreateExtension.h
//  RSCoreFoundation
//
//  Created by RetVal on 5/30/13.
//  Copyright (c) 2013 RetVal. All rights reserved.
//

#ifndef RSCoreFoundation_RSData_CreateExtension_h
#define RSCoreFoundation_RSData_CreateExtension_h

#include <RSCoreFoundation/RSError.h>

RS_EXTERN_C_BEGIN

RSExport RSDataRef RSDataWithBytes(const void* bytes, RSIndex length) RS_AVAILABLE(0_4);
RSExport RSDataRef RSDataWithBytesNoCopy(const void* bytes, RSIndex length, BOOL freeWhenDone, RSAllocatorRef bytesDeallocator) RS_AVAILABLE(0_4);
RSExport RSDataRef RSDataWithContentOfPath(RSStringRef path) RS_AVAILABLE(0_3);
RSExport RSDataRef RSDataWithString(RSStringRef string, RSStringEncoding encoding) RS_AVAILABLE(0_3);

RSExport RSMutableDataRef RSMutableDataWithBytes(const void* bytes, RSIndex length) RS_AVAILABLE(0_4);
RSExport RSMutableDataRef RSMutableDataWithBytesNoCopy(const void* bytes, RSIndex length, BOOL freeWhenDone, RSAllocatorRef bytesDeallocator) RS_AVAILABLE(0_4);
RSExport RSMutableDataRef RSMutableDataWithContentOfPath(RSStringRef path) RS_AVAILABLE(0_3);

RSExport RSDataRef RSDataCreateWithString(RSAllocatorRef allocator, RSStringRef string, RSStringEncoding encoding) RS_AVAILABLE(0_3);
RSExport RSDataRef RSDataCreateWithContentOfPath(RSAllocatorRef allocator, RSStringRef path) RS_AVAILABLE(0_3);
RSExport BOOL RSDataWriteToFile(RSDataRef data, RSStringRef path, RSWriteFileMode writeMode) RS_AVAILABLE(0_3);
RSExport BOOL RSDataWriteToFileWithError(RSDataRef data, RSStringRef path, RSErrorRef *error) RS_AVAILABLE(0_3);

RS_EXTERN_C_END
#endif
