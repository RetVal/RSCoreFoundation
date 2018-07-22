//
//  RSFileManager.h
//  RSCoreFoundation
//
//  Created by RetVal on 1/29/13.
//  Copyright (c) 2013 RetVal. All rights reserved.
//

#ifndef RSCoreFoundation_RSFileManager_h
#define RSCoreFoundation_RSFileManager_h
#include <RSCoreFoundation/RSData.h>
#include <RSCoreFoundation/RSDate.h>
#include <RSCoreFoundation/RSError.h>
#include <RSCoreFoundation/RSNumber.h>
#include <RSCoreFoundation/RSRunLoop.h>
#include <RSCoreFoundation/RSFileHandle.h>
#include <RSCoreFoundation/RSFileWrapper.h>
RS_EXTERN_C_BEGIN
typedef const struct __RSFileManager* RSFileManagerRef;
RSExport RSFileManagerRef RSFileManagerGetDefault(void) RS_AVAILABLE(0_0);
RSExport __autorelease RSStringRef RSFileManagerStandardizingPath(RSStringRef path) RS_AVAILABLE(0_0);
RSExport __autorelease RSArrayRef RSFileManagerContentsOfDirectory(RSFileManagerRef fmg, RSStringRef path, __autorelease RSErrorRef* error) RS_AVAILABLE(0_0);
RSExport __autorelease RSArrayRef RSFileManagerSubpathsOfDirectory(RSFileManagerRef fmg, RSStringRef path, __autorelease RSErrorRef* error) RS_AVAILABLE(0_0);
RSExport __autorelease RSDataRef RSFileManagerContentsAtPath(RSFileManagerRef fmg, RSStringRef path) RS_AVAILABLE(0_0);
RSExport BOOL RSFileManagerFileExistsAtPath(RSFileManagerRef fmg, RSStringRef path,BOOL* isDirectory) RS_AVAILABLE(0_0);
RSExport BOOL RSFileManagerIsReadableFileAtPath(RSFileManagerRef fmg, RSStringRef path) RS_AVAILABLE(0_0);
RSExport BOOL RSFileManagerIsWritableFileAtPath(RSFileManagerRef fmg, RSStringRef path) RS_AVAILABLE(0_0);
RSExport BOOL RSFileManagerIsExecutableFileAtPath(RSFileManagerRef fmg, RSStringRef path) RS_AVAILABLE(0_0);
RSExport BOOL RSFileManagerIsDeletableFileAtPath(RSFileManagerRef fmg, RSStringRef path) RS_AVAILABLE(0_0);

RSExport __autorelease RSDictionaryRef RSFileManagerAttributesOfFileAtPath(RSFileManagerRef fmg, RSStringRef path) RS_AVAILABLE(0_0);
RSExport BOOL RSFileManagerCopyFileToPath(RSFileManagerRef fmg, RSStringRef from, RSStringRef to) RS_AVAILABLE(0_0);
RSExport BOOL RSFileManagerMoveFileToPath(RSFileManagerRef fmg, RSStringRef from, RSStringRef to) RS_AVAILABLE(0_0);
RSExport BOOL RSFileManagerRemoveFile(RSFileManagerRef fmg, RSStringRef fileToDelete) RS_AVAILABLE(0_0);
RSExport __autorelease RSDictionaryRef RSFileManagerAttributesOfDirectoryAtPath(RSFileManagerRef fmg, RSStringRef dir) RS_AVAILABLE(0_0);
RSExport BOOL RSFileManagerCopyDirctoryToPath(RSFileManagerRef fmg, RSStringRef from, RSStringRef to) RS_AVAILABLE(0_0);
RSExport BOOL RSFileManagerMoveDirctoryToPath(RSFileManagerRef fmg, RSStringRef from, RSStringRef to) RS_AVAILABLE(0_0);
RSExport BOOL RSFileManagerRemoveDirectory(RSFileManagerRef fmg, RSStringRef dirToDelete) RS_AVAILABLE(0_0);

RSExport BOOL RSFileManagerCreateDirectoryAtPath(RSFileManagerRef fmg, RSStringRef dirPath) RS_AVAILABLE(0_0);

RSExport __autorelease RSStringRef RSFileManagerFileExtension(RSFileManagerRef fmg, RSStringRef path) RS_AVAILABLE(0_0);
RSExport __autorelease RSStringRef RSFileManagerFileFullName(RSFileManagerRef fmg, RSStringRef path) RS_AVAILABLE(0_0);
RSExport __autorelease RSStringRef RSFileManagerFileName(RSFileManagerRef fmg, RSStringRef path) RS_AVAILABLE(0_0);

// if success, return parent point for Chaining call
RSExport RSMutableStringRef RSFileManagerAppendFileName(RSFileManagerRef fmg, RSMutableStringRef parent, RSStringRef name) RS_AVAILABLE(0_0);

RSExport RSStringRef const RSFileAccessDate;            //RSDateRef
RSExport RSStringRef const RSFileCreationDate;          //RSDateRef
RSExport RSStringRef const RSFileGroupOwnerAccountID;   //RSNumberRef
RSExport RSStringRef const RSFileGroupOwnerAccountName; //RSStringRef
RSExport RSStringRef const RSFileModificationDate;      //RSDateRef
RSExport RSStringRef const RSFileOwnerAccountID;        //RSNumberRef
RSExport RSStringRef const RSFileOwnerAccountName;      //RSStringRef
RSExport RSStringRef const RSFilePosixPermissions;      //RSNumberRef
RSExport RSStringRef const RSFileReferenceCount;        //RSNumberRef
RSExport RSStringRef const RSFileSize;                  //RSNumberRef
RSExport RSStringRef const RSFileSystemFileNumber;      //RSNumberRef
RSExport RSStringRef const RSFileSystemNumber;          //RSNumberRef
RSExport RSStringRef const RSFileType;                  //nil
RS_EXTERN_C_END
#endif
