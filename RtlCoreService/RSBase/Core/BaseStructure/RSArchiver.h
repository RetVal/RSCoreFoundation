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
typedef struct __RSUnarchiver *RSUnarchiverRef;

RSExport RSStringRef const RSKeyedArchiveRootObjectKey;

RSExport RSTypeID RSArchiverGetTypeID() RS_AVAILABLE(0_0);

typedef RSDataRef (*RSArchiverSerializeCallBack)(RSArchiverRef archiver, RSTypeRef object);
typedef RSTypeRef (*RSArchiverDeserializeCallBack)(RSUnarchiverRef archiver, RSDataRef data);
typedef struct __RSArchiverCallBacks {
    RSIndex version;
    RSTypeID classID;
    RSArchiverSerializeCallBack serialize;
    RSArchiverDeserializeCallBack deserialize;
}RSArchiverCallBacks RS_AVAILABLE(0_0);

RSExport RSArchiverRef RSArchiverCreate(RSAllocatorRef allocator);

RSExport RSDataRef RSArchiverContextMakeDataPacket(RSArchiverRef archiver, RSIndex count, RSDataRef data1, ...);
RSExport RSArrayRef RSArchiverContextUnarchivePacket(const RSUnarchiverRef unarchiver, RSDataRef data, RSTypeID ID);
RSExport RSDataRef RSArchiverEncodeObject(RSArchiverRef archiver, RSTypeRef object);
RSExport RSTypeRef RSUnarchiverDecodeObject(RSUnarchiverRef unarchiver, RSDataRef data);

RSExport BOOL RSArchiverEncodeObjectForKey(RSArchiverRef archiver, RSStringRef key, RSTypeRef object);
RSExport RSTypeRef RSUnarchiverDecodeObjectForKey(RSUnarchiverRef unarchiver, RSStringRef key);

// get current archived data from archiver's context.
RSExport RSDataRef RSArchiverCopyData(RSArchiverRef archiver);

// create an unarchiver with data, and restore the context from archiver copy data
RSExport RSUnarchiverRef RSUnarchiverCreate(RSAllocatorRef allocator, RSDataRef data);
RSExport RSUnarchiverRef RSUnarchiverCreateWithContext(RSAllocatorRef allocator, RSArchiverContextRef context);
RSExport RSUnarchiverRef RSUnarchiverCreateWithContentOfPath(RSAllocatorRef allocator, RSStringRef path);

RS_EXTERN_C_END
#endif 
