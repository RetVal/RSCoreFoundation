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
    return RSAutorelease(RSDictionaryCreate(RSAllocatorSystemDefault, keys, values, count, context));
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

RS_CONST_STRING_DECL(__RSDictionayKeyPathSeparatingStrings, ".");
RSExport RSTypeRef RSDictionaryGetValueForKeyPath(RSDictionaryRef dictionary, RSStringRef keyPath) {
    if (!dictionary) return nil;
    RSTypeRef rst = nil;
    RSArrayRef keys = RSStringCreateComponentsSeparatedByStrings(RSAllocatorSystemDefault, keyPath, __RSDictionayKeyPathSeparatingStrings);
    rst = RSDictionaryGetValueForKeys(dictionary, keys);
    RSRelease(keys);
    return rst;
}

RSExport RSTypeRef RSDictionaryGetValueForKeys(RSDictionaryRef dictionary, RSArrayRef keys) {
    if (nil == dictionary || !keys || !RSArrayGetCount(keys)) return nil;
    RSTypeRef rst = nil;
    RSClassRef dictClass = RSClassGetWithUTF8String("RSDictionary");
    RSUInteger cnt = RSArrayGetCount(keys);
    RSDictionaryRef tmp = dictionary;
    for (RSUInteger idx = 0; idx < cnt; idx++) {
        if (RSInstanceIsMemberOfClass(tmp, dictClass)) {
            tmp = RSDictionaryGetValue(tmp, RSArrayObjectAtIndex(keys, idx));
        } else {
            return nil;
        }
    }
    return rst = tmp;
}

RSExport void RSDictionarySetValueForKeyPath(RSMutableDictionaryRef dictionary, RSStringRef keyPath, RSTypeRef value) {
    if (nil == dictionary || nil == keyPath) return;
    RSArrayRef keys = RSStringCreateComponentsSeparatedByStrings(RSAllocatorDefault, keyPath, __RSDictionayKeyPathSeparatingStrings);
    RSDictionarySetValueForKeys(dictionary, keys, value);
    RSRelease(keys);
    return;
}

RSExport void RSDictionarySetValueForKeys(RSMutableDictionaryRef dictionary, RSArrayRef keys, RSTypeRef value) {
    if (nil == dictionary || nil == keys || !RSArrayGetCount(keys)) return;
    RSClassRef dictClass = RSClassGetWithUTF8String("RSDictionary");
    RSUInteger cnt = RSArrayGetCount(keys);
    if (cnt < 2) {
        return RSDictionarySetValue(dictionary, RSArrayObjectAtIndex(keys, 0), value);
    }
    cnt --;
    RSMutableDictionaryRef tmp = dictionary, tmp1 = tmp;
    for (RSUInteger idx = 0; idx < cnt; idx++) {
        if (RSInstanceIsMemberOfClass(tmp, dictClass)) {
            tmp1 = (RSMutableDictionaryRef)RSDictionaryGetValue(tmp, RSArrayObjectAtIndex(keys, idx));
            if (!tmp1) {
                tmp1 = RSDictionaryCreateMutable(RSAllocatorSystemDefault, 0, RSDictionaryRSTypeContext);
                RSDictionarySetValue(tmp, RSArrayObjectAtIndex(keys, idx), tmp1);
                RSRelease(tmp1);
            }
            tmp = tmp1;
        } else {
            return ;
        }
    }
    return RSDictionarySetValue(tmp, RSArrayLastObject(keys), value);
}
