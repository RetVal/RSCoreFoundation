//
//  RSArray+Extension.h
//  RSCoreFoundation
//
//  Created by RetVal on 6/5/13.
//  Copyright (c) 2013 RetVal. All rights reserved.
//

#ifndef RSCoreFoundation_RSArray_Extension_h
#define RSCoreFoundation_RSArray_Extension_h

#include <RSCoreFoundation/RSError.h>

RS_EXTERN_C_BEGIN

RSExport RSArrayRef RSArrayWithObject(RSTypeRef object) RS_AVAILABLE(0_4);
RSExport RSArrayRef RSArrayWithObjects(RSTypeRef *objects, RSIndex count) RS_AVAILABLE(0_4);
RSExport RSArrayRef RSArrayWithObjectsNoCopy(RSTypeRef* objects, RSIndex count, BOOL freeWhenDone) RS_AVAILABLE(0_4);
RSExport RSArrayRef RSArrayBySeparatingStrings(RSStringRef string, RSStringRef separatorString) RS_AVAILABLE(0_4);

RSExport RSArrayRef RSArrayWithContentOfPath(RSStringRef path) RS_AVAILABLE(0_3);
RSExport RSArrayRef RSArrayCreateWithContentOfPath(RSAllocatorRef allocator, RSStringRef path) RS_AVAILABLE(0_3);
RSExport BOOL RSArrayWriteToFile(RSArrayRef array, RSStringRef path, RSWriteFileMode mode) RS_AVAILABLE(0_3);
RSExport BOOL RSArrayWriteToFileWithError(RSArrayRef array, RSStringRef path, RSErrorRef *error) RS_AVAILABLE(0_3);

RS_EXTERN_C_END
#endif
