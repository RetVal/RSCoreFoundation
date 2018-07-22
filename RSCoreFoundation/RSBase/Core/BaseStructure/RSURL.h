//
//  RSURL.h
//  RSCoreFoundation
//
//  Created by RetVal on 12/16/12.
//  Copyright (c) 2012 RetVal. All rights reserved.
//

#ifndef __RSURL__
#define __RSURL__
#include <RSCoreFoundation/RSArray.h>
#include <RSCoreFoundation/RSError.h>
#include <RSCoreFoundation/RSDictionary.h>
#include <RSCoreFoundation/RSString.h>
enum {
    RSURLPOSIXPathStyle = 0,
    RSURLHFSPathStyle, /* The use of RSURLHFSPathStyle is deprecated. The Carbon File Manager, which uses HFS style paths, is deprecated. HFS style paths are unreliable because they can arbitrarily refer to multiple volumes if those volumes have identical volume names. You should instead use RSURLPOSIXPathStyle wherever possible. */
    RSURLWindowsPathStyle
};
typedef RSIndex RSURLPathStyle;

// PUBLIC
enum RSURLComponentType {
	RSURLComponentScheme = 1,
	RSURLComponentNetLocation = 2,
	RSURLComponentPath = 3,
	RSURLComponentResourceSpecifier = 4,
    
	RSURLComponentUser = 5,
	RSURLComponentPassword = 6,
	RSURLComponentUserInfo = 7,
	RSURLComponentHost = 8,
	RSURLComponentPort = 9,
	RSURLComponentParameterString = 10,
	RSURLComponentQuery = 11,
	RSURLComponentFragment = 12
};
typedef enum RSURLComponentType RSURLComponentType;

typedef const struct __RSURL* RSURLRef;
RSExport RSTypeID RSURLGetTypeID(void) RS_AVAILABLE(0_3);
RSExport RSStringRef RSURLGetString(RSURLRef anURL) RS_AVAILABLE(0_3);
RSExport RSURLRef RSURLCopyAbsoluteURL(RSURLRef relativeURL) RS_AVAILABLE(0_3);
RSExport RSStringRef RSURLCopyScheme(RSURLRef anURL) RS_AVAILABLE(0_3);
RSExport BOOL RSURLHasDirectoryPath(RSURLRef anURL) RS_AVAILABLE(0_3);
RSExport RSURLRef RSURLCreateWithFileSystemPathRelativeToBase(RSAllocatorRef allocator, RSStringRef filePath, RSURLPathStyle fsType, BOOL isDirectory, RSURLRef baseURL) RS_AVAILABLE(0_3);
RSExport RSURLRef RSURLCreateFromFileSystemRepresentation(RSAllocatorRef allocator, const uint8_t *buffer, RSIndex bufLen, BOOL isDirectory) RS_AVAILABLE(0_3);
RSExport RSURLRef RSURLCreateFilePathURL(RSAllocatorRef alloc, RSURLRef url, RSErrorRef *error) RS_AVAILABLE(0_3);
    
RSExport RSStringRef  RSURLCreateStringByReplacingPercentEscapes(RSAllocatorRef alloc, RSStringRef  originalString, RSStringRef  charactersToLeaveEscaped) RS_AVAILABLE(0_3);

RSExport RSStringRef RSURLCreateStringByReplacingPercentEscapesUsingEncoding(RSAllocatorRef alloc, RSStringRef  originalString, RSStringRef  charactersToLeaveEscaped, RSStringEncoding enc) RS_AVAILABLE(0_3);

RSExport RSStringRef RSURLCreateStringByAddingPercentEscapes(RSAllocatorRef allocator, RSStringRef originalString, RSStringRef charactersToLeaveUnescaped, RSStringRef legalURLCharactersToBeEscaped, RSStringEncoding encoding) RS_AVAILABLE(0_3);

RSExport RSURLRef RSURLCreateWithBytes(RSAllocatorRef allocator, const uint8_t *URLBytes, RSIndex length, RSStringEncoding encoding, RSURLRef baseURL) RS_AVAILABLE(0_3);

RSExport RSDataRef RSURLCreateData(RSAllocatorRef allocator, RSURLRef  url, RSStringEncoding encoding, BOOL escapeWhitespace) RS_AVAILABLE(0_3);

RSExport RSURLRef RSURLCreateWithString(RSAllocatorRef allocator, RSStringRef  URLString, RSURLRef  baseURL) RS_AVAILABLE(0_3);

RSExport RSURLRef RSURLCreateAbsoluteURLWithBytes(RSAllocatorRef alloc, const RSBitU8 *relativeURLBytes, RSIndex length, RSStringEncoding encoding, RSURLRef baseURL, BOOL useCompatibilityMode) RS_AVAILABLE(0_3);

RSExport BOOL RSURLCanBeDecomposed(RSURLRef  anURL) RS_AVAILABLE(0_3);

RSExport RSStringRef  RSURLGetString(RSURLRef  url) RS_AVAILABLE(0_3);

RSExport RSIndex RSURLGetBytes(RSURLRef url, RSBitU8 *buffer, RSIndex bufferLength) RS_AVAILABLE(0_3);

RSExport RSURLRef  RSURLGetBaseURL(RSURLRef  anURL) RS_AVAILABLE(0_3);

RSExport RSStringRef RSURLCopyNetLocation(RSURLRef  anURL) RS_AVAILABLE(0_3);

RSExport RSStringRef  RSURLCopyPath(RSURLRef  anURL) RS_AVAILABLE(0_3);

RSExport RSStringRef RSURLCopyStrictPath(RSURLRef anURL, BOOL *isAbsolute) RS_AVAILABLE(0_3);

RSExport BOOL RSURLHasDirectoryPath(RSURLRef  anURL) RS_AVAILABLE(0_3);

RSExport RSStringRef  RSURLCopyResourceSpecifier(RSURLRef  anURL) RS_AVAILABLE(0_3);

RSExport RSStringRef  RSURLCopyHostName(RSURLRef  anURL) RS_AVAILABLE(0_3);

RSExport SInt32 RSURLGetPortNumber(RSURLRef  anURL) RS_AVAILABLE(0_3);

RSExport RSStringRef  RSURLCopyUserName(RSURLRef  anURL) RS_AVAILABLE(0_3);

RSExport RSStringRef  RSURLCopyPassword(RSURLRef  anURL) RS_AVAILABLE(0_3);

RSExport RSStringRef  RSURLCopyParameterString(RSURLRef  anURL, RSStringRef charactersToLeaveEscaped) RS_AVAILABLE(0_3);

RSExport RSStringRef  RSURLCopyQueryString(RSURLRef  anURL, RSStringRef  charactersToLeaveEscaped) RS_AVAILABLE(0_3);

RSExport RSStringRef  RSURLCopyFragment(RSURLRef  anURL, RSStringRef  charactersToLeaveEscaped) RS_AVAILABLE(0_3);

RSExport RSRange RSURLGetByteRangeForComponent(RSURLRef url, RSURLComponentType component, RSRange *rangeIncludingSeparators) RS_AVAILABLE(0_3);

RSExport RSURLRef RSURLCreateWithFileSystemPath(RSAllocatorRef allocator, RSStringRef filePath, RSURLPathStyle fsType, BOOL isDirectory) RS_AVAILABLE(0_3);

RSExport RSStringRef RSURLCopyFileSystemPath(RSURLRef anURL, RSURLPathStyle pathStyle) RS_RETURNS_RETAINED RS_AVAILABLE(0_3);

RSExport RSStringRef RSURLCreateStringWithFileSystemPath(RSAllocatorRef allocator, RSURLRef anURL, RSURLPathStyle fsType, BOOL resolveAgainstBase) RS_AVAILABLE(0_3);

RSExport BOOL RSURLGetFileSystemRepresentation(RSURLRef url, BOOL resolveAgainstBase, uint8_t *buffer, RSIndex bufLen) RS_AVAILABLE(0_3);

RSExport RSURLRef RSURLCreateFromFileSystemRepresentation(RSAllocatorRef allocator, const uint8_t *buffer, RSIndex bufLen, BOOL isDirectory) RS_AVAILABLE(0_3);

RSExport RSURLRef RSURLCreateFromFileSystemRepresentationRelativeToBase(RSAllocatorRef allocator, const uint8_t *buffer, RSIndex bufLen, BOOL isDirectory, RSURLRef baseURL) RS_AVAILABLE(0_3);

RSExport RSStringRef RSURLCopyLastPathComponent(RSURLRef url) RS_AVAILABLE(0_3);

RSExport RSStringRef RSURLCopyPathExtension(RSURLRef url) RS_AVAILABLE(0_3);

RSExport RSURLRef RSURLCreateCopyAppendingPathComponent(RSAllocatorRef allocator, RSURLRef url, RSStringRef pathComponent, BOOL isDirectory) RS_AVAILABLE(0_3);

RSExport RSURLRef RSURLCreateCopyDeletingLastPathComponent(RSAllocatorRef allocator, RSURLRef url) RS_AVAILABLE(0_3);

RSExport RSURLRef RSURLCreateCopyAppendingPathExtension(RSAllocatorRef allocator, RSURLRef url, RSStringRef extension) RS_AVAILABLE(0_3);

RSExport RSURLRef RSURLCreateCopyDeletingPathExtension(RSAllocatorRef allocator, RSURLRef url) RS_AVAILABLE(0_3);

RSExport RSURLRef RSURLWithString(RSStringRef URLString);
RSExport RSURLRef RSURLWithFilePath(RSStringRef filePath, BOOL isDirectory);
#endif
