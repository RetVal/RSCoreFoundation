//
//  RSBaseIO.c
//  RSKit
//
//  Created by RetVal on 10/13/12.
//  Copyright (c) 2012 RetVal. All rights reserved.
//

#include <stdio.h>
#include <fcntl.h>
#include <RSKit/RSErrorCode.h>
#include <RSKit/RSMemory.h>
#include <RSKit/RSBaseIO.h>
#include <RSKit/RSDBGLog.h>
#include <RSKit/RSOldString.h>
RSExport BOOL RSIOCopyFile(const RSBuffer a, const RSBuffer b)
{
    BOOL iRet = NO;
    NSError* error = nil;
    NSFileManager* fmg = [NSFileManager defaultManager];
    iRet = [fmg copyItemAtPath:[[NSString stringWithUTF8String:a] stringByStandardizingPath] toPath:[[NSString stringWithUTF8String:b] stringByStandardizingPath]error:&error];
    if (error)
    {
        RSDBGLog(@"%@",[error localizedDescription]);
    }
    return iRet;
}

RSExport BOOL RSIOMoveFile(const RSBuffer a, const RSBuffer b)
{
    BOOL iRet = NO;
    NSError* error = nil;
    NSFileManager* fmg = [NSFileManager defaultManager];
    iRet = [fmg moveItemAtURL:[NSURL URLWithString:[NSString stringWithUTF8String:a]] toURL:[NSURL URLWithString:[NSString stringWithUTF8String:b]] error:&error];
    if (error)
    {
        RSDBGLog(@"%@",[error localizedDescription]);
    }
    return iRet;
}

RSExport RSIOERR	RSIOCreateFile(RSBuffer szFilePath,RSUInteger createMode,RSFileHandleRef handle)
{
	RSIOERR	iRet = kSuccess;
	RSBuffer  szCreateModeArray[] = {"r","a+","r","w",""};
	do
	{
		RCM(handle,iRet,kSuccess,kErrNil);
		RCM(szFilePath,iRet,kSuccess,kErrNil);
		RCPP(handle,iRet);
		RCM(createMode < RSIO_CREATE_LIMIT,iRet,kSuccess,kErrVerify);
		RSBuffer  file_full_path = nil;
		RSBuffer  szMode = nil;
		BWI(iRet = RSMmAlloc((RSZone)&file_full_path,kRSBufferSize));
		strcpy(file_full_path,szFilePath);
        
        *handle = fopen(file_full_path,szMode = szCreateModeArray[createMode]);
        
		RSMmFree((RSZone)&file_full_path);
		if (*handle)
		{
			//RSIOCloseFile(*handle);
			return kSuccess;
		}
		
	} while (0);
	return	iRet;
}

RSExport RSIOERR	RSIOCloseFile(RSFileHandleRef handle)
{
	if (*handle)
	{
		fclose(*handle);
		*handle = nil;
		return kSuccess;
	}
	else
	{
        
	}
	return kErrUnknown;
}
RSExport BOOL   RSIOCheckFileExisting(RSBuffer  path)
{
	if (path)
	{
        BOOL result = NO;
        RSDBGLog(@"RSIOCheckFileExisting:%s",path);
        NSString* t= nil;
        result = [[NSFileManager defaultManager]fileExistsAtPath:t = [[NSString stringWithUTF8String:path]stringByStandardizingPath]];
        RSDBGLog(@"%@",t);
        return result ;
	}
	return NO;
}
RSExport RSIOERR	RSIOOpenFile(RSBuffer  szFile,RSFileHandleRef handle,unsigned int openMode)
{
	RSIOERR	iRet = kSuccess;
	static	RSBuffer  openModeC[] = {"ab+","rb","wb","ab+",""};
	do
	{
		RCM(handle,iRet,kSuccess,kErrNil);
		RCM(!*handle,iRet,kSuccess,kErrNNil);
		RCM(szFile,iRet,kSuccess,kErrNil);
		RCM(openMode < RSIO_OPEN_LIMIT,iRet,kSuccess,kErrVerify);
		//RSBuffer  file_full_path = nil;
		
		*handle = fopen(szFile,openModeC[openMode]);
		if (*handle)
		{
			RSIOSetFileOffset(*handle,0);
			return kSuccess;
		}
		else
		{
			iRet = kErrRead;
		}
	} while (0);
	return	iRet;
}
RSExport RSIOERR	RSIOGetFileSize(RSFileHandle  handle)
{
	if (handle)
	{
		//return (RSIOERR)filelength(fileno(handle));
        RSIOERR org = ftell(handle);
        fseek(handle, 0, SEEK_END); // seek to end of file
        RSIOERR size = ftell(handle); // get current file pointer
        fseek(handle, org, SEEK_SET);
        return size;
	}
	else
	{
		return	0;
	}
}
RSExport RSIOERR RSIOGetFileNameByFullName(RSBuffer  fullName,RSBufferRef name)
{
	RSIOERR iRet = kSuccess;
	do
	{
		RCM(name,iRet,kSuccess,kErrNil);
		RCM(!*name,iRet,kSuccess,kErrNNil);
		RCM(fullName,iRet,kSuccess,kErrNil);
		RSUInteger offset = 0;
		if (__RSOldStringGetLast(fullName,".",&offset))
		{
			__RSOldStringCopyByOffset(fullName,name,0,offset-1);
		}
		else
		{
			__RSOldStringCopyByLength(fullName,name,strlen(fullName));
		}
	} while (0);
	return iRet;
}
RSExport RSIOERR RSIOGetFileNameByPath(RSBuffer  path,RSBufferRef name)
{
	RSIOERR iRet = kSuccess;
	do
	{
		RCM(name,iRet,kSuccess,kErrNil);
		RCM(!*name,iRet,kSuccess,kErrNNil);
		RCM(path,iRet,kSuccess,kErrNil);
		RSUInteger offset = 0;
		if ((iRet = __RSOldStringGetLast(path,"\\",&offset)))
		{
			__RSOldStringCopyByOffset(path,name,offset,strlen(path));
			iRet = kSuccess;
		}
        else if ((iRet = __RSOldStringGetLast(path,"/",&offset)))
        {
            __RSOldStringCopyByOffset(path,name,offset,strlen(path));
			iRet = kSuccess;
        }
	} while (0);
	return iRet;
}
RSExport RSIOERR RSIOGetFileTypeByPath(RSBuffer  path,RSBufferRef type)
{
	RSIOERR iRet = kSuccess;
	do
	{
		RCM(type,iRet,kSuccess,kErrNil);
		RCM(!*type,iRet,kSuccess,kErrNNil);
		RCM(path,iRet,kSuccess,kErrNil);
		RSUInteger offset = 0;
		if ((iRet = __RSOldStringGetLast(path,".",&offset)))
		{
			__RSOldStringCopyByOffset(path,type,offset,strlen(path));
			iRet = kSuccess;
		}
	} while (0);
	return iRet;
}
RSExport RSIOERR	RSIOGetFileFolderByPath(RSBuffer  path,RSBufferRef  folder)
{
	RSIOERR iRet = kSuccess;
	do
	{
		RCM(folder,iRet,kSuccess,kErrNil);
		RCM(!*folder,iRet,kSuccess,kErrNNil);
		RCM(path,iRet,kSuccess,kErrNil);
		RSUInteger offset = 0;
		if ((iRet = __RSOldStringGetLast(path,"\\",&offset)))
		{
			__RSOldStringCopyByOffset(path,folder,0,offset);
			iRet = kSuccess;
		}
        else if ((iRet = __RSOldStringGetLast(path,"/",&offset)))
		{
			__RSOldStringCopyByOffset(path,folder,0,offset);
			iRet = kSuccess;
		}
        
	} while (0);
	return iRet;
}
RSExport RSIOERR	RSIOGetFileParentFolderByPath(RSBuffer  path,RSBufferRef  folder)
{
	RSIOERR iRet = kSuccess;
    RSBuffer tmp = nil;
	do
	{
		RCM(folder,iRet,kSuccess,kErrNil);
		RCM(!*folder,iRet,kSuccess,kErrNNil);
		RCM(path,iRet,kSuccess,kErrNil);
		RSUInteger offset1 = 0;
        RSUInteger offset2 = 0;
		if ((iRet = __RSOldStringGetLast(path,"\\",&offset1)))
		{
			__RSOldStringCopyByOffset(path,folder,0,offset1-1);
            if(__RSOldStringGetLast(tmp, "\\", &offset2))
            {
                __RSOldStringCopyByOffset(path, folder, offset2-1, offset1-1);
                RSMmFree((RSZone)&tmp);
            }
            else
            {
                *folder = tmp;
            }
			iRet = kSuccess;
		}
        else if ((iRet = __RSOldStringGetLast(path,"/",&offset1)))
		{
			__RSOldStringCopyByOffset(path,&tmp,0,offset1-1);
            if(__RSOldStringGetLast(tmp, "/", &offset2))
            {
                __RSOldStringCopyByOffset(path, folder, offset2-1, offset1-1);
                RSMmFree((RSZone)&tmp);
            }
            else
            {
                *folder = tmp;
            }
			iRet = kSuccess;
		}
        
	} while (0);
	return iRet;
}

RSExport RSIOERR	RSIOGetFileBuffer(RSFileHandle  handle,RSBufferRef fileBuffer)
{
	RSIOERR	iRet = kSuccess;
	RSUInteger	realSize = 0;
	do
	{
		RCM(handle,iRet,kSuccess,kErrNil);
		RCM(fileBuffer,iRet,kSuccess,kErrNil);
		RCM(!*fileBuffer,iRet,kSuccess,kErrNNil);
		iRet = RSIOReadFile(handle,fileBuffer,RSIOGetFileSize(handle),&realSize);
	} while (0);
	return	iRet;
}
RSExport RSIOERR	RSIOSetFileOffset(RSFileHandle  handle,RSUInteger offset)
{
	RSIOERR	iRet = kSuccess;
	do
	{
		RSUInteger size = RSIOGetFileSize(handle);
		if (size)
		{
			if (offset <= size)
			{
				fseek(handle,offset,SEEK_SET);
			}
			else
			{
				rewind(handle);
			}
		}
	} while (0);
	return	iRet;
}
RSExport RSUInteger	RSIOGetFileOffset(RSFileHandle  handle)
{
	if (handle)
	{
		return ftell(handle);
	}
    return 0;
}
RSExport RSIOERR	RSIOReadFile(RSFileHandle  handle,RSBufferRef  fileBuffer,RSUInteger readSize,RSUIntegerRef realRead)
{
	RSIOERR	iRet = kSuccess;
	do
	{
		RCM(handle,iRet,kSuccess,kErrNil);
		RCM(fileBuffer,iRet,kSuccess,kErrNil);
		RCM(!*fileBuffer,iRet,kSuccess,kErrNNil);
		//RCM(realRead,iRet,kSuccess,kErrNil);
		RCM(readSize,iRet,kSuccess,kErrVerify);
		RSUInteger size = RSIOGetFileSize(handle);
		if (size)
		{
			size = min(size,readSize);
			BWI(iRet = RSMmAlloc((RSZone  )fileBuffer,size + 2));
			if (realRead)
			{
				if((*realRead = fread(*fileBuffer,1,readSize,handle)))
				{
					return iRet;
				}
				
			}
			else
			{
				if(fread(*fileBuffer,1,readSize,handle))
				{
                    
				}
				else
				{
					iRet = kErrRead;
				}
			}
		}
		else
		{
			iRet = kErrRead;
		}
	} while (0);
	return	iRet;
}
RSExport RSIOERR	RSIOFReadFile(RSFileHandle  handle,RSBuffer  fileBuffer,RSUInteger	readSize,RSUIntegerRef realRead)
{
	RSIOERR	err = 0;
	do
	{
		RCM(handle,err,kSuccess,kErrNil);
		RCM(fileBuffer,err,kSuccess,kErrNil);
		//RCM(realRead,iRet,kSuccess,kErrNil);
		RCM(readSize,err,kSuccess,kErrVerify);
		RSUInteger size = RSIOGetFileSize(handle);
		readSize = min(readSize,size);
		if (realRead)
		{
			if((*realRead = fread(fileBuffer,1,readSize,handle)))
			{
				return err;
			}
		}
		else
		{
			if (fread(fileBuffer,1,readSize,handle))
			{
				return err;
			}
		}
	} while (0);
	return	err;
}
RSExport RSIOERR	RSIOReadFileLine(RSFileHandle  handle,RSBufferRef  fileBuffer,RSUIntegerRef realRead)
{
	RSIOERR	iRet = kSuccess;
	do
	{
		RCM(handle,iRet,kSuccess,kErrNil);
		RCM(fileBuffer,iRet,kSuccess,kErrNil);
		RCM(!*fileBuffer,iRet,kSuccess,kErrNNil);
		BWI(iRet = RSMmAlloc((RSZone  )fileBuffer,4096));
		if (feof(handle))
		{
			return kErrFileEnd;
		}
		fgets(*fileBuffer,4096,handle);
		if (realRead)
		{
			*realRead = 4096;
		}//fscanf
	}while(0);
	return iRet;
}
RSExport RSIOERR	RSIOReadCacheLine(RSBuffer  cache,RSBufferRef  outBuf,RSUIntegerRef realRead)
{
	RSIOERR	iRet = kSuccess;
	do
	{
		RCM(cache,iRet,kSuccess,kErrNil);
		RCM(outBuf,iRet,kSuccess,kErrNil);
		RCM(!*outBuf,iRet,kSuccess,kErrNNil);
		const short int line_flag = 0x0a0d;
		RSBuffer  baseLine = cache;
		RSUInteger offset = 0;
		while(cache[offset] && (short)(*(RSUIntegerRef)(baseLine + offset)) != line_flag)
		{
			++offset;
		}
		BWI(iRet = __RSOldStringCopyByOffset(cache,outBuf,0,offset));
		if (realRead)
		{
			if (cache[offset])
			{
				*realRead = offset + 2;
			}
			else
			{
				*realRead = offset;
			}
			
		}
	} while (0);
	return iRet;
}
RSExport RSIOERR RSIOWriteFile(RSFileHandle  handle,RSBuffer  fileBuffer,RSUInteger writeSize,RSUIntegerRef realWrite)
{
	RSIOERR	iRet = kSuccess;
	do
	{
		RCM(handle,iRet,kSuccess,kErrNil);
		RCM(fileBuffer,iRet,kSuccess,kErrNil);
		//RCM(realWrite,iRet,kSuccess,kErrNil);
		RCM(writeSize,iRet,kSuccess,kErrVerify);
		
		if (realWrite)
		{
			*realWrite = 0;
			if((*realWrite = fwrite(fileBuffer,1,writeSize,handle)))
			{
				if (*realWrite == writeSize)
				{
					return iRet;
				}
				else
				{
					return	kErrWrite;
				}
			}
		}
		else
		{
			if(fwrite(fileBuffer,1,writeSize,handle))
			{
                
			}
			else
			{
				iRet = kErrRead;
			}
		}
	} while (0);
	return	iRet;
}
RSExport RSIOERR	RSIODeleteFile(RSBuffer  file)
{
	RSIOERR	iRet = kSuccess;
	do
	{
		if (remove(file))
		{
			break;
		}
		else
		{
			iRet = kErrDelete;
		}
	} while (0);
	return	iRet;
}

RSExport RSBuffer RSIOFGetFileNameByPath(RSBuffer path)
{
    RSBuffer name = nil;
    static RSBlock nameRet[kRSBufferSize] = {0};
    RSUInteger iRet = kSuccess;
    do
    {
        RCMP(path,iRet);
        BWI(iRet = RSIOGetFileNameByPath(path,&name));
        if (name == nil)
        {
            BWI(1);
        }
        strcpy(nameRet,name);
    } while (0);
    RSMmFree((RSZone)&name);
    return nameRet;
}

RSExport RSBuffer	RSIOFGetFileParentFolderByPath(RSBuffer  path)
{
    RSBuffer name = nil;
    static RSBlock nameRet[kRSBufferSize] = {0};
    RSUInteger iRet = kSuccess;
    do
    {
        RCMP(path,iRet);
        BWI(iRet = RSIOGetFileParentFolderByPath(path,&name));
        if (name == nil)
        {
            BWI(1);
        }
        strcpy(nameRet,name);
    } while (0);
    RSMmFree((RSZone)&name);
    return nameRet;
}