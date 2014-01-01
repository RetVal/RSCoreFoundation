//
//  RSString+Extension.h
//  RSCoreFoundation
//
//  Created by RetVal on 6/5/13.
//  Copyright (c) 2013 RetVal. All rights reserved.
//

#ifndef RSCoreFoundation_RSString_Extension_h
#define RSCoreFoundation_RSString_Extension_h

#include <RSCoreFoundation/RSError.h>

RS_EXTERN_C_BEGIN

RSExport RSStringRef RSStringWithUTF8String(const char* nullTerminatingString) RS_AVAILABLE(0_4);
RSExport RSStringRef RSStringWithCString(RSCBuffer nullTerminatingString, RSStringEncoding encoding) RS_AVAILABLE(0_4);
RSExport RSStringRef RSStringWithCStringNoCopy(RSCBuffer nullTerminatingString, RSStringEncoding encoding, RSAllocatorRef contentsDeallocator) RS_AVAILABLE(0_4);
RSExport RSStringRef RSStringWithFormat(RSStringRef format, ...) RS_AVAILABLE(0_4);
RSExport RSStringRef RSStringWithFormatAndArguments(RSIndex capacity, RSStringRef format, va_list arguments) RS_AVAILABLE(0_4);
RSExport RSStringRef RSStringWithBytes(RSCUBuffer bytes, RSIndex numBytes, RSStringEncoding encoding) RS_AVAILABLE(0_4);
RSExport RSStringRef RSStringWithBytesNoCopy(RSCUBuffer bytes, RSIndex numBytes, RSStringEncoding encoding, RSAllocatorRef contentsDeallocator) RS_AVAILABLE(0_4);
RSExport RSStringRef RSStringWithCharacters(const UniChar *characters, RSIndex numChars) RS_AVAILABLE(0_4);
RSExport RSStringRef RSStringWithCharactersNoCopy(const UniChar *characters, RSIndex numChars) RS_AVAILABLE(0_4);
RSExport RSStringRef RSStringWithSubstring(RSStringRef string, RSRange range) RS_AVAILABLE(0_4);
RSExport RSStringRef RSStringWithArrayBySeparatingString(RSStringRef string, RSStringRef seperatorString) RS_AVAILABLE(0_4);
RSExport RSStringRef RSStringWithCombingStrings(RSArrayRef strings, RSStringRef separatorString) RS_AVAILABLE(0_4);
RSExport RSStringRef RSStringWithExternalRepresentation(RSStringRef aString, RSStringEncoding encoding, RSIndex n) RS_AVAILABLE(0_4);
RSExport RSStringRef RSStringConvert(RSStringRef aString, RSStringEncoding encoding) RS_AVAILABLE(0_4);

RSExport RSStringRef RSStringCreateWithData(RSAllocatorRef allocator, RSDataRef data, RSStringEncoding encoding) RS_AVAILABLE(0_3);
RSExport RSStringRef RSStringWithData(RSDataRef data, RSStringEncoding encoding) RS_AVAILABLE(0_3);

RSExport RSStringRef RSStringCreateWithContentOfPath(RSAllocatorRef allocator, RSStringRef path, RSStringEncoding encoding, __autorelease RSErrorRef *error) RS_AVAILABLE(0_4);
RSExport RSStringRef RSStringWithContentOfPath(RSStringRef path) RS_AVAILABLE(0_3);

RSExport BOOL RSStringWriteToFile(RSStringRef string, RSStringRef path, RSWriteFileMode mode) RS_AVAILABLE(0_3);
RSExport BOOL RSStringWriteToFileWithError(RSStringRef string, RSStringRef path, RSErrorRef *error) RS_AVAILABLE(0_3);

RSExport RSRange RSStringRangeOfString(RSStringRef string, RSStringRef find) RS_AVAILABLE(0_4);

RSExport RSStringRef RSStringByReplacingOccurrencesOfString(RSStringRef string, RSStringRef target, RSStringRef replacement) RS_AVAILABLE(0_4);
RS_EXTERN_C_END
#endif
