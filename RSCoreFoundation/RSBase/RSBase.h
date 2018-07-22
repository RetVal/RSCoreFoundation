//
//  RSBase.h
//  RSCoreFoundation
//
//  Created by RetVal on 10/13/12.
//  Copyright (c) 2012 RetVal. All rights reserved.
//

#ifndef __RSBASE__
#define __RSBASE__

#include <RSCoreFoundation/RSBaseType.h>
#include <RSCoreFoundation/RSAllocator.h>
#include <RSCoreFoundation/RSArray.h>
#include <RSCoreFoundation/RSString.h>

RS_EXTERN_C_BEGIN
/****************************************************************************/
RSExport RSTypeRef RSRetain(RSTypeRef obj) RS_AVAILABLE(0_0);
RSExport void RSRelease(RSTypeRef RS_RELEASES_ARGUMENT obj) RS_AVAILABLE(0_0);
RSExport RSMutableTypeRef RSAutorelease(RSTypeRef RS_RELEASES_ARGUMENT obj) RS_AVAILABLE(0_1);
RSExport RSIndex RSGetRetainCount(RSTypeRef obj) RS_AVAILABLE(0_0);
RSExport RSTypeID RSGetTypeID(RSTypeRef obj) RS_AVAILABLE(0_0);
RSExport BOOL RSEqual(RSTypeRef obj1, RSTypeRef obj2) RS_AVAILABLE(0_0);
RSExport BOOL RSNotEqual(RSTypeRef obj1, RSTypeRef obj2) RS_AVAILABLE(0_4);
RSExport RSHashCode RSHash(RSTypeRef obj) RS_AVAILABLE(0_0);
RSExport RSTypeRef RSCopy(RSAllocatorRef allocator, RSTypeRef obj) RS_AVAILABLE(0_1);
RSExport RSMutableTypeRef RSMutableCopy(RSAllocatorRef allocator, RSTypeRef obj) RS_AVAILABLE(0_1);
RSExport RSStringRef RSDescription(RSTypeRef obj) RS_AVAILABLE(0_0);

RSExport RSAllocatorRef RSGetAllocator(RSTypeRef obj) RS_AVAILABLE(0_0);
RSExport RSIndex RSGetInstanceSize(RSTypeRef obj) RS_AVAILABLE(0_0);

RSExport void RSLog(RSStringRef format,...) RS_AVAILABLE(0_0);
RSExport RSTypeRef RSShow(RSTypeRef obj) RS_AVAILABLE(0_0);

RSExport RSStringRef RSStringFromRSRange(RSRange range) RS_AVAILABLE(0_3);
RSExport RSStringRef RSClassNameFromInstance(RSTypeRef id) RS_AVAILABLE(0_3);

RSExport RSClassRef RSClassFromInstance(RSTypeRef id) RS_AVAILABLE(0_4);
RSExport RSClassRef RSClassGetWithName(RSStringRef name) RS_AVAILABLE(0_4);
RSExport RSClassRef RSClassGetWithUTF8String(const char *name) RS_AVAILABLE(0_4);

RSExport RSStringRef RSClassGetName(RSClassRef cls) RS_AVAILABLE(0_4);
RSExport RSTypeID RSClassGetTypeID(RSClassRef cls) RS_AVAILABLE(0_4);
RSExport BOOL RSInstanceIsMemberOfClass(RSTypeRef id, RSClassRef cls) RS_AVAILABLE(0_4);
RSExport BOOL RSInstanceIsMemberOfClassByUTF8String(RSTypeRef id, const char *name) RS_AVAILABLE(0_4);

RSExport RSStringRef RSClassGetNameWithInstance(RSTypeRef id) RS_AVAILABLE(0_4);
/****************************************************************************/
extern const unsigned char RSCoreFoundationVersionString[] RS_AVAILABLE(0_0);
extern const double RSCoreFoundationVersionNumber RS_AVAILABLE(0_0);

RSExport RSStringRef const RSCoreFoundationDidFinishLoadingNotification RS_AVAILABLE(0_4);
RSExport RSStringRef const RSCoreFoundationWillDeallocateNotification RS_AVAILABLE(0_4);

RS_EXTERN_C_END
#endif
