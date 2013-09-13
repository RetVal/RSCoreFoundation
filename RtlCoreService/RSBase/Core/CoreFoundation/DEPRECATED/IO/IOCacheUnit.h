//
//  IOCacheUnit.h
//
//  Created by RetVal on 9/1/12.
//  Copyright (c) 2012 RetVal. All rights reserved.
//

#include "MSKit.h"
#include "RtlBufferKit.h"
#include "RtlServices.h"
#include "IOPublicDefine.h"

//cacheRoot : set the cache unit root path, all the cache files will store in this path.
//default as ~/Library/Caches/
RtlExport IOERR     IOCacheSetRootPath(RtlBuffer cacheRoot);
RtlExport void      IOCacheResetRootPath();

//buf : store the current cache path and return, can be nil, if buf is nil, the return buffer is the root path.
//return buffer need not to free.
RtlExport RtlBuffer IOCacheGetRootPath(RtlBlock buf[BUFFER_SIZE]);

//handle : the handle of the current create an empty cache file.
//return buffer is the current cache path, if nil, the handle is also nil, the operation is failed.
//if not nil, return buffer should be freed by MSFree after use or memory leaks.
RtlExport RtlBuffer IOCacheCreateFile(RtlFileHandleRef handle);

//handle : the handle of the current create the cache file.
//needToCache : choose a file which is need to be cached.
//return buffer is the current cache path, if nil, the handle is also nil, the operation is failed.
//if not nil, return buffer should be freed by MSFree after use or memory leaks.
RtlExport RtlBuffer IOCahceCreateFileWithFile(RtlFileHandleRef handle, RtlBuffer needToCache);

//handle : the handle of the current create the cache file.
//IdString : set the sub directory of the cache, if nil, default as your app's bundle id or "RtlServices.cache.store"
//return buffer is the current cache path, if nil, the handle is also nil, the operation is failed.
//if not nil, return buffer should be freed by MSFree after use or memory leaks.
RtlExport RtlBuffer IOCacheCreateFileWithIdentifier( RtlFileHandleRef handle, RtlBuffer IdString);

//handle : the handle of the current create the cache file.
//IdString : set the sub directory of the cache, if nil, default as your app's bundle id or "RtlServices.cache.store"
//return buffer is the current cache path, if nil, the handle is also nil, the operation is failed.
//if not nil, return buffer should be freed by MSFree after use or memory leaks.
RtlExport RtlBuffer IOCacheCreateFileWithIdentifierAndFile( RtlFileHandleRef handle, RtlBuffer IdString, RtlBuffer needToCache);