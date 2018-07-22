//
//  RSPropertList.h
//  RSCoreFoundation
//
//  Created by RetVal on 12/13/12.
//  Copyright (c) 2012 RetVal. All rights reserved.
//

#ifndef RSCoreFoundation_RSPropertList_h
#define RSCoreFoundation_RSPropertList_h
#include <RSCoreFoundation/RSArray.h>
#include <RSCoreFoundation/RSData.h>
#include <RSCoreFoundation/RSDate.h>
#include <RSCoreFoundation/RSDictionary.h>
#include <RSCoreFoundation/RSFileManager.h>
#include <RSCoreFoundation/RSNil.h>
#include <RSCoreFoundation/RSNumber.h>
#include <RSCoreFoundation/RSError.h>
RS_EXTERN_C_BEGIN
typedef RS_ENUM(RSErrorCode, RSPlistParserError)
{
    RSPropertyListReadCorruptError = 10050,
    
    RSPropertyListImmutable = 1090,
    RSPropertyListMutableContainersAndLeaves = 1100,
};

typedef RS_ENUM(RSUInteger, RSPropertyListWriteMode)
{
    RSPropertyListWriteDefault = 0,
    RSPropertyListWriteNormal = 1,
    RSPropertyListWriteBinary = 2,
};
typedef struct __RSPropertyList* RSPropertyListRef;
RSExport RSTypeID RSPropertyListGetTypeID(void) RS_AVAILABLE(0_0);

RSExport RSPropertyListRef RSPropertyListCreateWithPath(RSAllocatorRef allocator, RSStringRef filePath) RS_AVAILABLE(0_0);
RSExport RSPropertyListRef RSPropertyListCreateWithXMLData(RSAllocatorRef allocator, RSDataRef data) RS_AVAILABLE(0_0);
RSExport RSPropertyListRef RSPropertyListCreateWithHandle(RSAllocatorRef allocator, RSFileHandleRef handle) RS_AVAILABLE(0_0);
RSExport RSPropertyListRef RSPropertyListCreateWithContent(RSAllocatorRef allocator, RSTypeRef content) RS_AVAILABLE(0_0);

RSExport RSTypeRef RSPropertyListGetContent(RSPropertyListRef plist) RS_AVAILABLE(0_0);
RSExport RSTypeRef RSPropertyListCopyContent(RSAllocatorRef allocator, RSPropertyListRef plist) RS_AVAILABLE(0_0);
RSExport RSDataRef RSPropertyListGetData(RSPropertyListRef plist, __autorelease RSErrorRef* error) RS_AVAILABLE(0_3);
RSExport RSDataRef RSPropertyListCreateXMLData(RSAllocatorRef allocator, RSDictionaryRef requestDictionary) RS_AVAILABLE(0_3);
RSExport BOOL RSPropertyListWriteToFile(RSPropertyListRef plist, RSStringRef filePath, __autorelease RSErrorRef* error) RS_AVAILABLE(0_0);

RSExport BOOL RSPropertyListWriteToFileWithMode(RSPropertyListRef plist, RSStringRef filePath, RSPropertyListWriteMode mode, __autorelease RSErrorRef* error) RS_AVAILABLE(0_0);
RS_EXTERN_C_END
#endif
