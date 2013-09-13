//
//  RSArchiver.h
//  RSCoreFoundation
//
//  Created by RetVal on 9/2/13.
//  Copyright (c) 2013 RetVal. All rights reserved.
//

#ifndef RSCoreFoundation_RSArchiver
#define RSCoreFoundation_RSArchiver

#include <RSCoreFoundation/RSBase.h>
#include <RSCoreFoundation/RSData.h>
#include <RSCoreFoundation/RSString.h>

RS_EXTERN_C_BEGIN

typedef struct __RSArchiverContext *RSArchiverContextRef;
typedef struct __RSArchiver *RSArchiverRef;
typedef struct __RSArchiver *RSUnarchiverRef;

RSExport RSTypeID RSArchiverGetTypeID() RS_AVAILABLE(0_0);

typedef RSDataRef (*RSArchiverSerializeCallBack)(RSArchiverRef archiver, RSTypeRef object);
typedef RSTypeRef (*RSArchiverDeserializeCallBack)(RSArchiverRef archiver, RSDataRef);
typedef struct __RSArchiverCallBacks
{
    RSIndex version;
    RSTypeID classID;
    RSArchiverSerializeCallBack serialize;
    RSArchiverDeserializeCallBack deserialize;
}RSArchiverCallBacks RS_AVAILABLE(0_0);

RSExport RSDataRef RSArchiveObject(RSTypeRef object);
RSExport RSArchiverRef RSArchiverCreate(RSAllocatorRef allocator);

RSExport BOOL RSArchiverEncodeObjectForKey(RSArchiverRef archiver, RSStringRef key, RSTypeRef object);

RSExport RSDataRef RSArchiverCopyData(RSArchiverRef archiver);

RSExport RSUnarchiverRef RSUnarchiverCreate(RSAllocatorRef allocator, RSDataRef data);
RSExport RSUnarchiverRef RSUnarchiverCreateWithContext(RSAllocatorRef allocator, RSArchiverContextRef context);
RSExport RSUnarchiverRef RSUnarchiverCreateWithFile(RSAllocatorRef allocator, RSStringRef path);

RSExport RSTypeRef RSUnarchiverObjectWithFilePath(RSStringRef path);
RSExport RSTypeRef RSUnarchiverCreateObjectWithFilePath(RSStringRef path);

RSExport RSTypeRef RSUnarchiverObjectWithData(RSDataRef data);
RSExport RSTypeRef RSUnarchiverCreateObjectWithData(RSDataRef data);

RSExport RSTypeRef RSUnarchiverDecodeObjectForKey(RSUnarchiverRef unarchiver, RSStringRef key);

RS_EXTERN_C_END
#endif 
