//
//  RSBaseIO.h
//  RSKit
//
//  Created by RetVal on 10/13/12.
//  Copyright (c) 2012 RetVal. All rights reserved.
//

#ifndef RSKit_RSBaseIO_h
#define RSKit_RSBaseIO_h
#include <RSKit/RSBase.h>
typedef RSErrorCode RSIOERR;
#define RSIO_CREATE_DEFAULT		0		//IO_CREATE_DEFAULT IS IO_CREATE_ALWAYS
#define RSIO_CREATE_ALWAYS		1		//
#define RSIO_CREATE_OPEN_EXISTING	2		//if is not existing, error
#define RSIO_CREATE_CREATE_NEW	3		//if is exiting, error
#define RSIO_CREATE_LIMIT			4       //the count, reserved.


#define RSIO_OPEN_RB				1       //read
#define	RSIO_OPEN_WB				2       //write
#define RSIO_OPEN_RWB				3       //read & write
#define RSIO_OPEN_LIMIT			4       //the count, reserved.
//
//  RSIOFileUnit.h
//
//  Created by RetVal on 9/1/12.
//  Copyright (c) 2012 RetVal. All rights reserved.
//
// copy a to b
RSExport BOOL RSIOCopyFile(const RSBuffer a, const RSBuffer b);

// move a to b
RSExport BOOL RSIOMoveFile(const RSBuffer a, const RSBuffer b);

// szFilePath : create path
// createMode : RSIO_CREATE_xxxx defined in RSIOPublicDefine.h
// handle : return handle that own the new create file. should be closed by RSIOCloseFile or resource leaks.
// return code see RSError
RSExport RSIOERR RSIOCreateFile(const RSBuffer  szFilePath,RSUInteger createMode,RSFileHandleRef handle);

// handle : the file need to be closed.
RSExport RSIOERR	RSIOCloseFile(RSFileHandleRef handle);

// check if the file is existing on the file system.
// return code is YES if existing.
RSExport BOOL	RSIOCheckFileExisting(const RSBuffer  path);

// szFile : the file need to open.
// handle : the return file handle, should be closed by RSIOCloseFile after use or resource leaks.
RSExport RSIOERR	RSIOOpenFile(const RSBuffer  szFile,RSFileHandleRef handle,unsigned int openMode);

// get the file name from full name (support by RSKit). the return name should be freed by MSFree after use or memory leaks.
RSExport RSIOERR RSIOGetFileNameByFullName(const RSBuffer  fullName,RSBufferRef name);

// get the file name from file path (support by RSKit).the return name should be freed by MSFree after use or memory leaks.
RSExport RSIOERR RSIOGetFileNameByPath(const RSBuffer  path,RSBufferRef name);

// get the file parent directory full path from file path, the return folder should be freed by MSFree after use or memory leaks.
RSExport RSIOERR	RSIOGetFileFolderByPath(const RSBuffer  path,RSBufferRef  folder);

// get the file parent directory(only the parent path name) from file path, the return folder should be freed by MSFree after use or memory leaks.
RSExport RSIOERR	RSIOGetFileParentFolderByPath(const RSBuffer  path,RSBufferRef  folder);

// get file type(extension) from file path. the return type should be freed by MSFree after use or memory leaks.
RSExport RSIOERR RSIOGetFileTypeByPath(const RSBuffer  path,RSBufferRef type);

// get file content and store to fileBuffer, the return buffer should be freed by MSFree after use or memory leaks.
RSExport RSIOERR	RSIOGetFileBuffer(RSFileHandle handle,RSBufferRef fileBuffer);

// get file name from handle (not support) now. reserved. do not call it anyway.
RSExport RSIOERR	RSIOGetFileName(RSFileHandle handle,RSBuffer  fileName);

// get the volume from the full path, the return buffer should be freed by MSFree after use or memory leaks.
// on unix operation system, the volumn always be "/".
RSExport RSIOERR	RSIOGetVolumeByFilePath(const RSBuffer  path,RSBufferRef  volume);

// get file size. the return code is file size , not a error code.
RSExport RSIOERR	RSIOGetFileSize(RSFileHandle handle);

// set the file current file pointer's offset.
RSExport RSIOERR	RSIOSetFileOffset(RSFileHandle handle,RSUInteger offset);

// get the file current file pointer's offset, the return is the offset value not an error code.
RSExport RSUInteger	RSIOGetFileOffset(RSFileHandle handle);

// read the file content to the fileBuffer, readSize is what size you want to read, and the realRead is return what size of the function acturally read.
// the realRead can be nil.
// the handle and fileBuffer should be freed by RSIOCloseFile and MSFree after use, or resource and memory leak.
RSExport RSIOERR	RSIOReadFile(RSFileHandle handle,RSBufferRef  fileBuffer,RSUInteger readSize,RSUIntegerRef realRead);

// read the file content to the fileBuffer which is caller give to this function, other is same as RSIOReadFile.
RSExport RSIOERR	RSIOFReadFile(RSFileHandle handle,RSBuffer  fileBuffer,RSUInteger	readSize,RSUIntegerRef realRead);

// read a line content of the file, other is same as RSIOReadFile.
RSExport RSIOERR	RSIOReadFileLine(RSFileHandle handle,RSBufferRef  fileBuffer,RSUIntegerRef realRead);

// read a line content from cache to outBuf, other is same as RSIOReadFileLine.
RSExport RSIOERR	RSIOReadCacheLine(RSBuffer  cache,RSBufferRef  outBuf,RSUIntegerRef realRead);

// write the fileBuffer content in the file, the writeSize is what length you want to write, the realWrite is what size of this function acturally write.
// the realWrite can be nil.
RSExport RSIOERR RSIOWriteFile(RSFileHandle handle,RSBuffer  fileBuffer,RSUInteger writeSize,RSUIntegerRef realWrite);

// try to delete a file in this file system.
RSExport RSIOERR	RSIODeleteFile(const RSBuffer  file);

// get file name from path, and this return buffer is not need to freed by caller.
RSExport RSBuffer RSIOFGetFileNameByPath(RSBuffer path);

// same as RSIOFGetFileNameByPath, this function is get file parent folder and return.
RSExport RSBuffer	RSIOFGetFileParentFolderByPath(RSBuffer  path);


#endif
