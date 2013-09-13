//
//  RSBinaryPropertyList.h
//  RSCoreFoundation
//
//  Created by RetVal on 2/21/13.
//  Copyright (c) 2013 RetVal. All rights reserved.
//

#ifndef RSCoreFoundation_RSBinaryPropertyList_h
#define RSCoreFoundation_RSBinaryPropertyList_h
#include <RSCoreFoundation/RSPropertyList.h>
#include <RSCoreFoundation/RSSet.h>
#include <RSCoreFoundation/RSUUID.h>
RS_EXTERN_C_BEGIN
RSExport BOOL RSBinaryPropertyListWriteToFile(RSPropertyListRef propertyList, RSStringRef filePath, __autorelease RSErrorRef* error);

RSExport RSDataRef RSBinaryPropertyListGetData(RSPropertyListRef plist, __autorelease RSErrorRef* error) RS_AVAILABLE(0_3);
RSExport RSDataRef RSBinaryPropertyListCreateXMLData(RSAllocatorRef allocator, RSDictionaryRef requestDictionary) RS_AVAILABLE(0_3);
RS_EXTERN_C_END 
#endif
