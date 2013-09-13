//
//  RSDictionary+Extension.c
//  RSCoreFoundation
//
//  Created by RetVal on 6/4/13.
//  Copyright (c) 2013 RetVal. All rights reserved.
//

#include <RSCoreFoundation/RSDictionary+Extension.h>
#include <RSCoreFoundation/RSData+Extension.h>
#include <RSCoreFoundation/RSPropertyList.h>
RSExport RSDictionaryRef RSDictionaryWithKeysAndValues(const void** keys, const void** values, const RSDictionaryContext* context, RSIndex count)
{
    return RSAutorelease(RSDictionaryCreate(RSAllocatorSystemDefault, keys, values, context, count));
}

RSExport RSDictionaryRef RSDictionaryWithArray(RSArrayRef keys, RSArrayRef values, const RSDictionaryContext* context)
{
    return RSAutorelease(RSDictionaryCreateWithArray(RSAllocatorSystemDefault, keys, values, context));
}

RSExport RSDictionaryRef RSDictionaryWithObjectForKey(RSTypeRef object, RSTypeRef key)
{
    return RSAutorelease(RSDictionaryCreateWithObjectsAndOKeys(RSAllocatorSystemDefault, object, key, NULL));
}

RSExport RSDictionaryRef RSDictionaryWithContentOfPath(RSStringRef path)
{
    return RSAutorelease(RSDictionaryCreateWithContentOfPath(RSAllocatorSystemDefault, path));
}
RSExport RSDictionaryRef RSDictionaryCreateWithContentOfPath(RSAllocatorRef allocator, RSStringRef path)
{
    RSDictionaryRef dictionary = nil;
    RSPropertyListRef plist = RSPropertyListCreateWithPath(allocator, path);
    if (plist)  RSGetTypeID(RSPropertyListGetContent(plist)) == RSDictionaryGetTypeID() ? dictionary = RSRetain(RSPropertyListGetContent(plist)) : nil;
    RSRelease(plist);
    return dictionary;
}

RSExport RSDictionaryRef RSDictionaryWithData(RSDataRef data)
{
    return RSAutorelease(RSDictionaryCreateWithData(RSAllocatorSystemDefault, data));
}

RSExport RSDictionaryRef RSDictionaryCreateWithData(RSAllocatorRef allocator, RSDataRef data)
{
    RSDictionaryRef dictionary = nil;
    RSPropertyListRef plist = RSPropertyListCreateWithXMLData(allocator, data);
    if (plist)  RSGetTypeID(RSPropertyListGetContent(plist)) == RSDictionaryGetTypeID() ? dictionary = RSRetain(RSPropertyListGetContent(plist)) : nil;
    RSRelease(plist);
    return dictionary;
}

RSExport BOOL RSDictionaryWriteToFile(RSDictionaryRef dictionary, RSStringRef path, RSWriteFileMode mode)
{
    BOOL result = NO;
    RSPropertyListRef plist = RSPropertyListCreateWithContent(RSAllocatorSystemDefault, dictionary);
    if (plist)
    {
        result = RSDataWriteToFile(RSPropertyListGetData(plist, nil), path, mode);
        RSRelease(plist);
    }
    return result;
}

RSExport BOOL RSDictionaryWriteToFileWithError(RSDictionaryRef dictionary, RSStringRef path, RSErrorRef *error)
{
    BOOL result = NO;
    RSPropertyListRef plist = RSPropertyListCreateWithContent(RSAllocatorSystemDefault, dictionary);
    if (plist)
    {
        result = RSDataWriteToFileWithError(RSPropertyListGetData(plist, error), path, error);
        RSRelease(plist);
    }
    return result;
}
