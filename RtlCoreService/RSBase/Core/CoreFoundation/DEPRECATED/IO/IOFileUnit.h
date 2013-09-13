//
//  IOFileUnit.h
//
//  Created by RetVal on 9/1/12.
//  Copyright (c) 2012 RetVal. All rights reserved.
//
#include "IOPublicDefine.h"
// copy a to b
RtlExport BOOL IOCopyFile(const RtlBuffer a, const RtlBuffer b);

// move a to b
RtlExport BOOL IOMoveFile(const RtlBuffer a, const RtlBuffer b);

// szFilePath : create path
// createMode : IO_CREATE_xxxx defined in IOPublicDefine.h
// handle : return handle that own the new create file. should be closed by IOCloseFile or resource leaks.
// return code see RtlError
RtlExport IOERR IOCreateFile(const RtlBuffer  szFilePath,RtlUInteger createMode,RtlFileHandleRef handle);

// handle : the file need to be closed.
RtlExport IOERR	IOCloseFile(RtlFileHandleRef handle);

// check if the file is existing on the file system.
// return code is YES if existing.
RtlExport BOOL	IOCheckFileExisting(const RtlBuffer  path);

// szFile : the file need to open.
// handle : the return file handle, should be closed by IOCloseFile after use or resource leaks.
RtlExport IOERR	IOOpenFile(const RtlBuffer  szFile,RtlFileHandleRef handle,unsigned int openMode);

// get the file name from full name (support by RtlKit). the return name should be freed by MSFree after use or memory leaks.
RtlExport IOERR IOGetFileNameByFullName(const RtlBuffer  fullName,RtlBufferRef name);

// get the file name from file path (support by RtlKit).the return name should be freed by MSFree after use or memory leaks.
RtlExport IOERR IOGetFileNameByPath(const RtlBuffer  path,RtlBufferRef name);

// get the file parent directory full path from file path, the return folder should be freed by MSFree after use or memory leaks.
RtlExport IOERR	IOGetFileFolderByPath(const RtlBuffer  path,RtlBufferRef  folder);

// get the file parent directory(only the parent path name) from file path, the return folder should be freed by MSFree after use or memory leaks. 
RtlExport IOERR	IOGetFileParentFolderByPath(const RtlBuffer  path,RtlBufferRef  folder);

// get file type(extension) from file path. the return type should be freed by MSFree after use or memory leaks. 
RtlExport IOERR IOGetFileTypeByPath(const RtlBuffer  path,RtlBufferRef type);

// get file content and store to fileBuffer, the return buffer should be freed by MSFree after use or memory leaks. 
RtlExport IOERR	IOGetFileBuffer(RtlFileHandle handle,RtlBufferRef fileBuffer);

// get file name from handle (not support) now. reserved. do not call it anyway.
RtlExport IOERR	IOGetFileName(RtlFileHandle handle,RtlBuffer  fileName);

// get the volume from the full path, the return buffer should be freed by MSFree after use or memory leaks.
// on unix operation system, the volumn always be "/".
RtlExport IOERR	IOGetVolumeByFilePath(const RtlBuffer  path,RtlBufferRef  volume);

// get file size. the return code is file size , not a error code.
RtlExport IOERR	IOGetFileSize(RtlFileHandle handle);

// set the file current file pointer's offset.
RtlExport IOERR	IOSetFileOffset(RtlFileHandle handle,RtlUInteger offset);

// get the file current file pointer's offset, the return is the offset value not an error code.
RtlExport IOERR	IOGetFileOffset(RtlFileHandle handle);

// read the file content to the fileBuffer, readSize is what size you want to read, and the realRead is return what size of the function acturally read.
// the realRead can be nil.
// the handle and fileBuffer should be freed by IOCloseFile and MSFree after use, or resource and memory leak.
RtlExport IOERR	IOReadFile(RtlFileHandle handle,RtlBufferRef  fileBuffer,RtlUInteger readSize,RtlUIntegerRef realRead);

// read the file content to the fileBuffer which is caller give to this function, other is same as IOReadFile.
RtlExport IOERR	IOFReadFile(RtlFileHandle handle,RtlBuffer  fileBuffer,RtlUInteger	readSize,RtlUIntegerRef realRead);

// read a line content of the file, other is same as IOReadFile.
RtlExport IOERR	IOReadFileLine(RtlFileHandle handle,RtlBufferRef  fileBuffer,RtlUIntegerRef realRead);

// read a line content from cache to outBuf, other is same as IOReadFileLine.
RtlExport IOERR	IOReadCacheLine(RtlBuffer  cache,RtlBufferRef  outBuf,RtlUIntegerRef realRead);

// write the fileBuffer content in the file, the writeSize is what length you want to write, the realWrite is what size of this function acturally write.
// the realWrite can be nil.
RtlExport IOERR IOWriteFile(RtlFileHandle handle,RtlBuffer  fileBuffer,RtlUInteger writeSize,RtlUIntegerRef realWrite);

// try to delete a file in this file system.
RtlExport IOERR	IODeleteFile(const RtlBuffer  file);

// get file name from path, and this return buffer is not need to freed by caller.
RtlExport RtlBuffer IOFGetFileNameByPath(RtlBuffer path);

// same as IOFGetFileNameByPath, this function is get file parent folder and return.
RtlExport RtlBuffer	IOFGetFileParentFolderByPath(RtlBuffer  path);
