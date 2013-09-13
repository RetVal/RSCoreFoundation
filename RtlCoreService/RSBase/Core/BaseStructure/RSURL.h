//
//  RSURL.h
//  RSCoreFoundation
//
//  Created by RetVal on 12/16/12.
//  Copyright (c) 2012 RetVal. All rights reserved.
//

#ifndef RSCoreFoundation_RSURL_h
#define RSCoreFoundation_RSURL_h
#include <RSCoreFoundation/RSArray.h>
#include <RSCoreFoundation/RSError.h>
#include <RSCoreFoundation/RSDictionary.h>
#include <RSCoreFoundation/RSString.h>
typedef RS_ENUM(RSIndex, RSURLPathStyle) {
    RSURLPOSIXPathStyle = 0,
    RSURLHFSPathStyle, /* The use of kCFURLHFSPathStyle is deprecated. The Carbon File Manager, which uses HFS style paths, is deprecated. HFS style paths are unreliable because they can arbitrarily refer to multiple volumes if those volumes have identical volume names. You should instead use kCFURLPOSIXPathStyle wherever possible. */
    RSURLWindowsPathStyle
};
typedef const struct __RSURL* RSURLRef;
RSExport RSTypeID RSURLGetTypeID() RS_AVAILABLE(0_3);
RSExport RSStringRef RSURLGetString(RSURLRef) RS_AVAILABLE(0_3);
RSExport RSURLRef RSURLCopyAbsoluteURL(RSURLRef relativeURL) RS_AVAILABLE(0_3);
RSExport RSStringRef RSURLCopyScheme(RSURLRef anURL) RS_AVAILABLE(0_3);
RSExport BOOL RSURLHasDirectoryPath(RSURLRef anURL) RS_AVAILABLE(0_3);
RSExport RSURLRef RSURLCreateWithFileSystemPathRelativeToBase(RSAllocatorRef allocator, RSStringRef filePath, RSURLPathStyle fsType, BOOL isDirectory, RSURLRef baseURL) RS_AVAILABLE(0_3);
RSExport RSURLRef RSURLCreateFromFileSystemRepresentation(RSAllocatorRef allocator, const uint8_t *buffer, RSIndex bufLen, BOOL isDirectory) RS_AVAILABLE(0_3);
RSExport RSURLRef RSURLCreateFilePathURL(RSAllocatorRef alloc, RSURLRef url, RSErrorRef *error) RS_AVAILABLE(0_3);
#endif
