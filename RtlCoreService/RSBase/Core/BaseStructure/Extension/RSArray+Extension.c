//
//  RSArray+Extension.c
//  RSCoreFoundation
//
//  Created by RetVal on 6/5/13.
//  Copyright (c) 2013 RetVal. All rights reserved.
//

#include <RSCoreFoundation/RSArray.h>
#include <RSCoreFoundation/RSArray+Extension.h>
#include <RSCoreFoundation/RSData+Extension.h>
#include <RSCoreFoundation/RSPropertyList.h>
RSExport RSArrayRef RSArrayWithObject(RSTypeRef object)
{
    return RSAutorelease(RSArrayCreateWithObject(RSAllocatorSystemDefault, object));
}

RSExport RSArrayRef RSArrayWithObjects(RSTypeRef *objects, RSIndex count)
{
    return RSAutorelease(RSArrayCreateWithObjects(RSAllocatorSystemDefault, objects, count));
}

RSExport RSArrayRef RSArrayWithObjectsNoCopy(RSTypeRef* objects, RSIndex count, BOOL freeWhenDone)
{
    return RSAutorelease(RSArrayCreateWithObjectsNoCopy(RSAllocatorSystemDefault, objects, count, freeWhenDone));
}

RSExport RSArrayRef RSArrayBySeparatingStrings(RSStringRef string, RSStringRef separatorString)
{
    return RSAutorelease(RSStringCreateArrayBySeparatingStrings(RSAllocatorSystemDefault, string, separatorString));
}

RSExport RSArrayRef RSArrayWithContentOfPath(RSStringRef path) RS_AVAILABLE(0_3)
{
    return RSAutorelease(RSArrayCreateWithContentOfPath(RSAllocatorSystemDefault, path));
}

RSExport RSArrayRef RSArrayCreateWithContentOfPath(RSAllocatorRef allocator, RSStringRef path) RS_AVAILABLE(0_3)
{
    RSArrayRef array = nil;
    RSPropertyListRef plist = RSPropertyListCreateWithPath(allocator, path);
    if (plist) RSGetTypeID(RSPropertyListGetContent(plist)) == RSArrayGetTypeID() ? array = RSRetain(RSPropertyListGetContent(plist)) : nil;
    RSRelease(plist);
    return array;
}

RSExport BOOL RSArrayWriteToFile(RSArrayRef array, RSStringRef path, RSWriteFileMode mode) RS_AVAILABLE(0_3)
{
    BOOL result = NO;
    RSPropertyListRef plist = RSPropertyListCreateWithContent(RSAllocatorSystemDefault, array);
    if (plist) {
        result = RSDataWriteToFile(RSPropertyListGetData(plist, nil), path, mode);
        RSRelease(plist);
    }
    return result;
}

RSExport BOOL RSArrayWriteToFileWithError(RSArrayRef array, RSStringRef path, RSErrorRef *error) RS_AVAILABLE(0_3)
{
    BOOL result = NO;
    RSPropertyListRef plist = RSPropertyListCreateWithContent(RSAllocatorSystemDefault, array);
    if (plist) {
        result = RSDataWriteToFileWithError(RSPropertyListGetData(plist, error), path, error);
        RSRelease(plist);
    }
    return result;

}
