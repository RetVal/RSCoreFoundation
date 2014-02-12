//
//  RSDictionary+Extension.h
//  RSCoreFoundation
//
//  Created by RetVal on 6/4/13.
//  Copyright (c) 2013 RetVal. All rights reserved.
//

#ifndef RSCoreFoundation_RSDictionary_Extension_h
#define RSCoreFoundation_RSDictionary_Extension_h

#include <RSCoreFoundation/RSError.h>

RS_EXTERN_C_BEGIN

RSExport RSDictionaryRef RSDictionaryWithKeysAndValues(const void** keys, const void** values, const RSDictionaryContext* context, RSIndex count) RS_AVAILABLE(0_4);
RSExport RSDictionaryRef RSDictionaryWithArray(RSArrayRef keys, RSArrayRef values, const RSDictionaryContext* context) RS_AVAILABLE(0_4);
RSExport RSDictionaryRef RSDictionaryWithObjectForKey(RSTypeRef object, RSTypeRef key) RS_AVAILABLE(0_4);


RSExport RSDictionaryRef RSDictionaryWithContentOfPath(RSStringRef path) RS_AVAILABLE(0_3);
RSExport RSDictionaryRef RSDictionaryCreateWithContentOfPath(RSAllocatorRef allocator, RSStringRef path) RS_AVAILABLE(0_3);

RSExport RSDictionaryRef RSDictionaryWithData(RSDataRef data) RS_AVAILABLE(0_3);
RSExport RSDictionaryRef RSDictionaryCreateWithData(RSAllocatorRef allocator, RSDataRef data) RS_AVAILABLE(0_3);

RSExport BOOL RSDictionaryWriteToFile(RSDictionaryRef dictionary, RSStringRef path, RSWriteFileMode mode) RS_AVAILABLE(0_3);
RSExport BOOL RSDictionaryWriteToFileWithError(RSDictionaryRef dictionary, RSStringRef path, RSErrorRef *error) RS_AVAILABLE(0_3);

RSExport RSTypeRef RSDictionaryGetValueForKeyPath(RSDictionaryRef dictionary, RSStringRef keyPath) RS_AVAILABLE(0_4);
RSExport RSTypeRef RSDictionaryGetValueForKeys(RSDictionaryRef dictionary, RSArrayRef keys) RS_AVAILABLE(0_4);

RSExport void RSDictionarySetValueForKeyPath(RSMutableDictionaryRef dictionary, RSStringRef keyPath, RSTypeRef value) RS_AVAILABLE(0_4);
RSExport void RSDictionarySetValueForKeys(RSMutableDictionaryRef dictionary, RSArrayRef keys, RSTypeRef value) RS_AVAILABLE(0_4);

RS_EXTERN_C_END
#endif
