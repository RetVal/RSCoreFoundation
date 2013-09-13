//
//  IOCacheUnit.m
//
//  Created by RetVal on 9/1/12.
//  Copyright (c) 2012 RetVal. All rights reserved.
//

#include "IOCacheUnit.h"
#include "IOFileUnit.h"

#pragma mark IOCacheParivateFunctions
RtlInside const RtlBuffer __IOPrivateCacheRootPathPtr = "~/Library/Caches/";

RtlInside RtlBlock  __IOPrivateCacheRootPath[BUFFER_SIZE] = {"~/Library/Caches/"};
// the return buffer should be freed by MSFree or memory leaks.
RtlInside RtlBuffer __IOCacheCreateUnit(RtlBuffer cacheRoot, RtlBuffer cacheFileName, RtlBuffer cachedFilePath)
{
    
    RtlFileHandle handle = nil;
    IOCreateDirectory(cacheRoot);
    //init the cache full path
    RtlBuffer cacheFullPath = nil;
    cacheFullPath = MSAllocateIOUnit();
    
    strcpy(cacheFullPath, cacheRoot);
    strcat(cacheFullPath, cacheFileName);
    RtlDBGLog(@"%s",cacheFullPath);
    RtlDBGLog(@"%s",cachedFilePath);
    if (cachedFilePath)
    {
        IOCopyFile(cachedFilePath, cacheFullPath);
    }
    else
    {
        IOCreateFile(cacheFullPath, IO_CREATE_ALWAYS, &handle);
        IOCloseFile(&handle);
    }
    return cacheFullPath;
}

#pragma mark IOCacheUnit

RtlExport IOERR     IOCacheSetRootPath(RtlBuffer cacheRoot)
{
    if( YES == IOCheckFileExisting(cacheRoot))
        strcpy(__IOPrivateCacheRootPath, cacheRoot);
    else
        return kErrMiss;
    if (__IOPrivateCacheRootPath[strlen(__IOPrivateCacheRootPath)] != '/')
    {
        strcat(__IOPrivateCacheRootPath, "/");
    }
    return kSuccess;
}

RtlExport void      IOCacheResetRootPath()
{
    IOCacheSetRootPath(__IOPrivateCacheRootPathPtr);
}

RtlExport RtlBuffer IOCacheGetRootPath(RtlBlock buf[BUFFER_SIZE])
{
    if (buf)
    {
        strcpy(buf, IOFTranslatePath(__IOPrivateCacheRootPath));
    }
    else
        return IOFTranslatePath(__IOPrivateCacheRootPath);
    return buf;
}

RtlExport RtlBuffer IOCacheCreateFile(RtlFileHandleRef handle)
{
    RtlBuffer cachePath = nil;
    RtlBlock name[BUFFER_SIZE] = {0};
#ifdef __APPLE__
    
    cachePath = __IOCacheCreateUnit(IOCacheGetRootPath(nil), IOCacheCreateName(name), nil);
    IOOpenFile(cachePath, handle, IO_OPEN_RWB);
#elif __WIN32
    
    RtlBlock temp[MAX_PATH] = {0};
    GetTempDirectory(MAX_PATH,temp);
    MSAlloc((RtlZone)&cachePath,MAX_PATH);
    memcpy(cachePath, temp, MAX_PATH);
    RtlBuffer tempString = tempnam(temp, "Rtl");
    strcpy(cachePath, tempString);
    IOCreateFile(cachePath, IO_CREATE_ALWAYS, handle);
#endif
    return cachePath;
}

RtlExport RtlBuffer IOCahceCreateFileWithFile(RtlFileHandleRef handle, RtlBuffer needToCache)
{
    return IOCacheCreateFileWithIdentifierAndFile(handle, nil, needToCache);
}

RtlExport RtlBuffer IOCacheCreateFileWithIdentifier(RtlFileHandleRef handle, RtlBuffer IdString)
{
    if (IdString == nil)
    {
        IdString = "RtlServices.caches.store";
    }
    return IOCacheCreateFileWithIdentifierAndFile(handle, IdString, nil);
}

RtlExport RtlBuffer IOCacheCreateFileWithIdentifierAndFile( RtlFileHandleRef handle, RtlBuffer IdString, RtlBuffer needToCache)
{
    RtlBuffer cachePath = nil;
    
    RtlUInteger iRet = kSuccess;
    RtlBlock name[BUFFER_SIZE] = {0};
    do {
        RCPP(handle, iRet);
        RtlBuffer _b1 = IOCacheGetRootPath(nil);
        
        
        IOCacheCreateName(name);
        RtlDBGLog(@"%s - %s",_b1,name);
        if (IdString != nil)
        {
            strcat(_b1, IdString);
            if (IdString[strlen(IdString)] != '/')
            {
                strcat(_b1, "/");
            }
            
            IOCreateDirectory(_b1);
            
            RtlDBGLog(@"%s",IdString);
        }
        cachePath = __IOCacheCreateUnit(_b1, name , needToCache);
        IOOpenFile(cachePath, handle, IO_OPEN_RWB);
    } while (0);
    return cachePath;
}