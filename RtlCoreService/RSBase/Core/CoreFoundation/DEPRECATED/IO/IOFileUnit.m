//
//  IOFileUnit.m
//
//  Created by RetVal on 9/1/12.
//  Copyright (c) 2012 RetVal. All rights reserved.
//

#import "IOFileUnit.h"

RtlExport BOOL IOCopyFile(const RtlBuffer a, const RtlBuffer b)
{
    BOOL iRet = NO;
    NSError* error = nil;
    NSFileManager* fmg = [NSFileManager defaultManager];
    iRet = [fmg copyItemAtPath:[[NSString stringWithUTF8String:a] stringByStandardizingPath] toPath:[[NSString stringWithUTF8String:b] stringByStandardizingPath]error:&error];
    if (error)
    {
        RtlDBGLog(@"%@",[error localizedDescription]);
    }
    return iRet;
}

RtlExport BOOL IOMoveFile(const RtlBuffer a, const RtlBuffer b)
{
    BOOL iRet = NO;
    NSError* error = nil;
    NSFileManager* fmg = [NSFileManager defaultManager];
    iRet = [fmg copyItemAtURL:[NSURL URLWithString:[NSString stringWithUTF8String:a]] toURL:[NSURL URLWithString:[NSString stringWithUTF8String:b]] error:&error];
    if (error)
    {
        RtlDBGLog(@"%@",[error localizedDescription]);
    }
    return iRet;
}

RtlExport IOERR	IOCreateFile(RtlBuffer szFilePath,RtlUInteger createMode,RtlFileHandleRef handle)
{
	IOERR	iRet = kSuccess;
	RtlBuffer  szCreateModeArray[] = {"r","a+","r","w",""};
	do
	{
		RCM(handle,iRet,kSuccess,kErrNil);
		RCM(szFilePath,iRet,kSuccess,kErrNil);
		RCPP(handle,iRet);
		RCM(createMode < IO_CREATE_LIMIT,iRet,kSuccess,kErrVerify);
		RtlBuffer  file_full_path = nil;
		RtlBuffer  szMode = nil;
		BWI(iRet = MSAlloc((RtlZone)&file_full_path,BUFFER_SIZE));
		strcpy(file_full_path,szFilePath);
        
        *handle = fopen(file_full_path,szMode = szCreateModeArray[createMode]);
        
		MSFree((RtlZone)&file_full_path);
		if (*handle)
		{
			//IOCloseFile(*handle);
			return kSuccess;
		}
		
	} while (0);
	return	iRet;
}

RtlExport IOERR	IOCloseFile(RtlFileHandleRef handle)
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
RtlExport BOOL   IOCheckFileExisting(RtlBuffer  path)
{
	if (path)
	{
        BOOL result = NO;
        RtlDBGLog(@"IOCheckFileExisting:%s",path);
        NSString* t= nil;
        result = [[NSFileManager defaultManager]fileExistsAtPath:t = [[NSString stringWithUTF8String:path]stringByStandardizingPath]];
        RtlDBGLog(@"%@",t);
        return result ;
	}
	return NO;
}
RtlExport IOERR	IOOpenFile(RtlBuffer  szFile,RtlFileHandleRef handle,unsigned int openMode)
{
	IOERR	iRet = kSuccess;
	static	RtlBuffer  openModeC[] = {"ab+","rb","wb","ab+",""};
	do
	{
		RCM(handle,iRet,kSuccess,kErrNil);
		RCM(!*handle,iRet,kSuccess,kErrNNil);
		RCM(szFile,iRet,kSuccess,kErrNil);
		RCM(openMode < IO_OPEN_LIMIT,iRet,kSuccess,kErrVerify);
		//RtlBuffer  file_full_path = nil;
		
		*handle = fopen(szFile,openModeC[openMode]);
		if (*handle)
		{
			IOSetFileOffset(*handle,0);
			return kSuccess;
		}
		else
		{
			iRet = kErrRead;
		}
	} while (0);
	return	iRet;
}
RtlExport IOERR	IOGetFileSize(RtlFileHandle  handle)
{
	if (handle)
	{
		//return (IOERR)filelength(fileno(handle));
        IOERR org = ftell(handle);
        fseek(handle, 0, SEEK_END); // seek to end of file
        IOERR size = ftell(handle); // get current file pointer
        fseek(handle, org, SEEK_SET);
        return size;
	}
	else
	{
		return	0;
	}
}
RtlExport IOERR IOGetFileNameByFullName(RtlBuffer  fullName,RtlBufferRef name)
{
	IOERR iRet = kSuccess;
	do
	{
		RCM(name,iRet,kSuccess,kErrNil);
		RCM(!*name,iRet,kSuccess,kErrNNil);
		RCM(fullName,iRet,kSuccess,kErrNil);
		RtlUInteger offset = 0;
		if (RtlSsGetLast(fullName,".",&offset))
		{
			RtlSsCopyByOffset(fullName,name,0,offset-1);
		}
		else
		{
			RtlSsCopyByLength(fullName,name,strlen(fullName));
		}
	} while (0);
	return iRet;
}
RtlExport IOERR IOGetFileNameByPath(RtlBuffer  path,RtlBufferRef name)
{
	IOERR iRet = kSuccess;
	do
	{
		RCM(name,iRet,kSuccess,kErrNil);
		RCM(!*name,iRet,kSuccess,kErrNNil);
		RCM(path,iRet,kSuccess,kErrNil);
		RtlUInteger offset = 0;
		if ((iRet = RtlSsGetLast(path,"\\",&offset)))
		{
			RtlSsCopyByOffset(path,name,offset,strlen(path));
			iRet = kSuccess;
		}
        else if ((iRet = RtlSsGetLast(path,"/",&offset)))
        {
            RtlSsCopyByOffset(path,name,offset,strlen(path));
			iRet = kSuccess;
        }
	} while (0);
	return iRet;
}
RtlExport IOERR IOGetFileTypeByPath(RtlBuffer  path,RtlBufferRef type)
{
	IOERR iRet = kSuccess;
	do
	{
		RCM(type,iRet,kSuccess,kErrNil);
		RCM(!*type,iRet,kSuccess,kErrNNil);
		RCM(path,iRet,kSuccess,kErrNil);
		RtlUInteger offset = 0;
		if ((iRet = RtlSsGetLast(path,".",&offset)))
		{
			RtlSsCopyByOffset(path,type,offset,strlen(path));
			iRet = kSuccess;
		}
	} while (0);
	return iRet;
}
RtlExport IOERR	IOGetFileFolderByPath(RtlBuffer  path,RtlBufferRef  folder)
{
	IOERR iRet = kSuccess;
	do
	{
		RCM(folder,iRet,kSuccess,kErrNil);
		RCM(!*folder,iRet,kSuccess,kErrNNil);
		RCM(path,iRet,kSuccess,kErrNil);
		RtlUInteger offset = 0;
		if ((iRet = RtlSsGetLast(path,"\\",&offset)))
		{
			RtlSsCopyByOffset(path,folder,0,offset);
			iRet = kSuccess;
		}
        else if ((iRet = RtlSsGetLast(path,"/",&offset)))
		{
			RtlSsCopyByOffset(path,folder,0,offset);
			iRet = kSuccess;
		}
        
	} while (0);
	return iRet;
}
RtlExport IOERR	IOGetFileParentFolderByPath(RtlBuffer  path,RtlBufferRef  folder)
{
	IOERR iRet = kSuccess;
    RtlBuffer tmp = nil;
	do
	{
		RCM(folder,iRet,kSuccess,kErrNil);
		RCM(!*folder,iRet,kSuccess,kErrNNil);
		RCM(path,iRet,kSuccess,kErrNil);
		RtlUInteger offset1 = 0;
        RtlUInteger offset2 = 0;
		if ((iRet = RtlSsGetLast(path,"\\",&offset1)))
		{
			RtlSsCopyByOffset(path,folder,0,offset1-1);
            if(RtlSsGetLast(tmp, "\\", &offset2))
            {
                RtlSsCopyByOffset(path, folder, offset2-1, offset1-1);
                MSFree((RtlZone)&tmp);
            }
            else
            {
                *folder = tmp;
            }
			iRet = kSuccess;
		}
        else if ((iRet = RtlSsGetLast(path,"/",&offset1)))
		{
			RtlSsCopyByOffset(path,&tmp,0,offset1-1);
            if(RtlSsGetLast(tmp, "/", &offset2))
            {
                RtlSsCopyByOffset(path, folder, offset2-1, offset1-1);
                MSFree((RtlZone)&tmp);
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

RtlExport IOERR	IOGetFileBuffer(RtlFileHandle  handle,RtlBufferRef fileBuffer)
{
	IOERR	iRet = kSuccess;
	IOERR	realSize = 0;
	do
	{
		RCM(handle,iRet,kSuccess,kErrNil);
		RCM(fileBuffer,iRet,kSuccess,kErrNil);
		RCM(!*fileBuffer,iRet,kSuccess,kErrNNil);
		iRet = IOReadFile(handle,fileBuffer,IOGetFileSize(handle),&realSize);
	} while (0);
	return	iRet;
}
RtlExport IOERR	IOSetFileOffset(RtlFileHandle  handle,RtlUInteger offset)
{
	IOERR	iRet = kSuccess;
	do
	{
		RtlUInteger size = IOGetFileSize(handle);
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
RtlExport RtlUInteger	IOGetFileOffset(RtlFileHandle  handle)
{
	if (handle)
	{
		return ftell(handle);
	}
    return 0;
}
RtlExport IOERR	IOReadFile(RtlFileHandle  handle,RtlBufferRef  fileBuffer,RtlUInteger readSize,RtlUIntegerRef realRead)
{
	IOERR	iRet = kSuccess;
	do
	{
		RCM(handle,iRet,kSuccess,kErrNil);
		RCM(fileBuffer,iRet,kSuccess,kErrNil);
		RCM(!*fileBuffer,iRet,kSuccess,kErrNNil);
		//RCM(realRead,iRet,kSuccess,kErrNil);
		RCM(readSize,iRet,kSuccess,kErrVerify);
		RtlUInteger size = IOGetFileSize(handle);
		if (size)
		{
			size = min(size,readSize);
			BWI(iRet = MSAlloc((RtlZone  )fileBuffer,size + 2));
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
RtlExport IOERR	IOFReadFile(RtlFileHandle  handle,RtlBuffer  fileBuffer,RtlUInteger	readSize,RtlUIntegerRef realRead)
{
	IOERR	err = 0;
	do
	{
		RCM(handle,err,kSuccess,kErrNil);
		RCM(fileBuffer,err,kSuccess,kErrNil);
		//RCM(realRead,iRet,kSuccess,kErrNil);
		RCM(readSize,err,kSuccess,kErrVerify);
		RtlUInteger size = IOGetFileSize(handle);
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
RtlExport IOERR	IOReadFileLine(RtlFileHandle  handle,RtlBufferRef  fileBuffer,RtlUIntegerRef realRead)
{
	IOERR	iRet = kSuccess;
	do
	{
		RCM(handle,iRet,kSuccess,kErrNil);
		RCM(fileBuffer,iRet,kSuccess,kErrNil);
		RCM(!*fileBuffer,iRet,kSuccess,kErrNNil);
		BWI(iRet = MSAlloc((RtlZone  )fileBuffer,4096));
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
RtlExport IOERR	IOReadCacheLine(RtlBuffer  cache,RtlBufferRef  outBuf,RtlUIntegerRef realRead)
{
	IOERR	iRet = kSuccess;
	do
	{
		RCM(cache,iRet,kSuccess,kErrNil);
		RCM(outBuf,iRet,kSuccess,kErrNil);
		RCM(!*outBuf,iRet,kSuccess,kErrNNil);
		const short int line_flag = 0x0a0d;
		RtlBuffer  baseLine = cache;
		RtlUInteger offset = 0;
		while(cache[offset] && (short)(*(RtlUIntegerRef)(baseLine + offset)) != line_flag)
		{
			++offset;
		}
		BWI(iRet = RtlSsCopyByOffset(cache,outBuf,0,offset));
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
RtlExport IOERR IOWriteFile(RtlFileHandle  handle,RtlBuffer  fileBuffer,RtlUInteger writeSize,RtlUIntegerRef realWrite)
{
	IOERR	iRet = kSuccess;
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
RtlExport IOERR	IODeleteFile(RtlBuffer  file)
{
	IOERR	iRet = kSuccess;
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

RtlExport RtlBuffer IOFGetFileNameByPath(RtlBuffer path)
{
    RtlBuffer name = nil;
    static RtlBlock nameRet[BUFFER_SIZE] = {0};
    RtlUInteger iRet = kSuccess;
    do
    {
        RCMP(path,iRet);
        BWI(iRet = IOGetFileNameByPath(path,&name));
        if (name == nil)
        {
            BWI(1);
        }
        strcpy(nameRet,name);
    } while (0);
    MSFree((RtlZone)&name);
    return nameRet;
}

RtlExport RtlBuffer	IOFGetFileParentFolderByPath(RtlBuffer  path)
{
    RtlBuffer name = nil;
    static RtlBlock nameRet[BUFFER_SIZE] = {0};
    RtlUInteger iRet = kSuccess;
    do
    {
        RCMP(path,iRet);
        BWI(iRet = IOGetFileParentFolderByPath(path,&name));
        if (name == nil)
        {
            BWI(1);
        }
        strcpy(nameRet,name);
    } while (0);
    MSFree((RtlZone)&name);
    return nameRet;
}


