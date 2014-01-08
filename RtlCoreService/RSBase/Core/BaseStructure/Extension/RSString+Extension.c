//
//  RSString+Extension.c
//  RSCoreFoundation
//
//  Created by RetVal on 6/5/13.
//  Copyright (c) 2013 RetVal. All rights reserved.
//

#include <RSCoreFoundation/RSString.h>
#include <RSCoreFoundation/RSData+Extension.h>
#include <RSCoreFoundation/RSString+Extension.h>
#include <RSCoreFoundation/RSURL.h>

RSExport RSStringRef RSStringWithUTF8String(const char* nullTerminatingString)
{
    return RSStringWithCString((RSCBuffer)nullTerminatingString, RSStringEncodingUTF8);
}

RSExport RSStringRef RSStringWithCString(RSCBuffer nullTerminatingString, RSStringEncoding encoding)
{
    return RSAutorelease(RSStringCreateWithCString(RSAllocatorSystemDefault, nullTerminatingString, encoding));
}

RSExport RSStringRef RSStringWithCStringNoCopy(RSCBuffer nullTerminatingString, RSStringEncoding encoding, RSAllocatorRef contentsDeallocator)
{
    return RSAutorelease(RSStringCreateWithCStringNoCopy(RSAllocatorSystemDefault, nullTerminatingString, encoding, contentsDeallocator));
}

RSExport RSStringRef RSStringWithFormat(RSStringRef format, ...)
{
    va_list ap ;
    va_start(ap, format);
    RSStringRef result = RSStringWithFormatAndArguments(0, format, ap);
    va_end(ap);
    return result;
}

RSExport RSStringRef RSStringWithFormatAndArguments(RSIndex capacity, RSStringRef format, va_list arguments)
{
    return RSAutorelease(RSStringCreateWithFormatAndArguments(RSAllocatorSystemDefault, capacity, format, arguments));
}

RSExport RSStringRef RSStringWithBytes(RSCUBuffer bytes, RSIndex numBytes, RSStringEncoding encoding)
{
    return RSAutorelease(RSStringCreateWithBytes(RSAllocatorSystemDefault, bytes, numBytes, encoding, NO));
}

RSExport RSStringRef RSStringWithBytesNoCopy(RSCUBuffer bytes, RSIndex numBytes, RSStringEncoding encoding, RSAllocatorRef contentsDeallocator)
{
    return RSAutorelease(RSStringCreateWithBytesNoCopy(RSAllocatorSystemDefault, bytes, numBytes, encoding, NO, contentsDeallocator));
}

RSExport RSStringRef RSStringWithCharacters(const UniChar *characters, RSIndex numChars)
{
    return RSAutorelease(RSStringCreateWithCharacters(RSAllocatorSystemDefault, characters, numChars));
}

RSExport RSStringRef RSStringWithCharactersNoCopy(const UniChar *characters, RSIndex numChars)
{
    return RSAutorelease(RSStringCreateWithCharactersNoCopy(RSAllocatorSystemDefault, characters, numChars, RSAllocatorSystemDefault));
}

RSExport RSStringRef RSStringWithSubstring(RSStringRef string, RSRange range)
{
    return RSAutorelease(RSStringCreateWithSubstring(RSAllocatorSystemDefault, string, range));
}

RSExport RSStringRef RSStringWithArrayBySeparatingString(RSStringRef string, RSStringRef seperatorString)
{
    return RSAutorelease(RSStringCreateArrayBySeparatingStrings(RSAllocatorSystemDefault, string, seperatorString));
}

RSExport RSStringRef RSStringWithCombingStrings(RSArrayRef strings, RSStringRef separatorString)
{
    return RSAutorelease(RSStringCreateByCombiningStrings(RSAllocatorSystemDefault, strings, separatorString));
}

RSExport RSStringRef RSStringWithExternalRepresentation(RSStringRef aString, RSStringEncoding encoding, RSIndex n)
{
    return RSAutorelease(RSStringCreateExternalRepresentation(RSAllocatorSystemDefault, aString, encoding, n));
}

RSExport RSStringRef RSStringConvert(RSStringRef aString, RSStringEncoding encoding)
{
    return nil;
}

RSExport RSStringRef RSStringCreateWithData(RSAllocatorRef allocator, RSDataRef data, RSStringEncoding encoding)
{
    if (!data) return nil;
    return RSStringCreateWithBytes(allocator, RSDataGetBytesPtr(data), RSDataGetLength(data), encoding, NO);
}

RSExport RSStringRef RSStringWithData(RSDataRef data, RSStringEncoding encoding)
{
    return RSAutorelease(RSStringCreateWithData(RSAllocatorSystemDefault, data, encoding));
}

RSExport RSStringRef RSStringWithContentOfPath(RSStringRef path)
{
    return RSAutorelease(RSStringCreateWithContentOfPath(RSAllocatorSystemDefault, path, RSStringEncodingUTF8, nil));
}

RSExport RSStringRef RSStringCreateWithContentOfPath(RSAllocatorRef allocator, RSStringRef path, RSStringEncoding encoding, __autorelease RSErrorRef *error) {
    RSDataRef data = RSDataCreateWithContentOfPath(allocator, path);
    if (data) {
        RSStringRef string = RSStringCreateWithData(allocator, data, RSStringEncodingUTF8);
        RSRelease(data);
        return string;
    }
    return nil;
}

RSExport BOOL RSStringWriteToFile(RSStringRef string, RSStringRef path, RSWriteFileMode mode)
{
    RSDataRef data = RSDataCreateWithString(RSGetAllocator(string), string, RSStringEncodingUTF8);
    if (data) { 
        BOOL result = RSDataWriteToFile(data, path, mode);
        RSRelease(data);
        return result;
    }
    return NO;
}

RSExport BOOL RSStringWriteToFileWithError(RSStringRef string, RSStringRef path, RSErrorRef *error)
{
    RSDataRef data = RSDataCreateWithString(RSGetAllocator(string), string, RSStringEncodingUTF8);
    if (data) {
        BOOL result = RSDataWriteToFileWithError(data, path, error);
        RSRelease(data);
        return result;
    }
    return NO;
}

RSExport RSRange RSStringRangeOfString(RSStringRef string, RSStringRef find) {
    RSRange range;
    RSStringFind(string, find, RSStringGetRange(string), &range);
    return range;
}

RSExport RSStringRef RSStringByReplacingOccurrencesOfString(RSStringRef string, RSStringRef target, RSStringRef replacement) {
    RSStringRef newString = nil;
    RSRange range = RSStringRangeOfString(string, target);
    RSMutableStringRef copy = RSMutableCopy(RSAllocatorSystemDefault, string);
    newString = RSCopy(RSAllocatorSystemDefault, RSStringReplace(copy, range, replacement));
    RSRelease(copy);
    return RSAutorelease(newString);
}

RSExport RSStringRef RSStringByTrimmingCharactersInSet(RSStringRef string, RSCharacterSetRef characterSet) {
    if (!string || !characterSet) return nil;
    RSMutableStringRef copy = RSMutableCopy(RSAllocatorSystemDefault, string);
    RSStringTrimInCharacterSet(copy, characterSet);
    RSStringRef result = RSAutorelease(RSStringCreateWithFormat(RSAllocatorSystemDefault, RSSTR("%r"), copy));
    RSRelease(copy);
    return result;
}

/*
 NSMutableArray *parts = [[NSMutableArray alloc] init];
 for (NSString *key in dictionary) {
 id encodedValue = dictionary[key];//([dictionary[key] isKindOfClass:[NSString class]]) ? [dictionary[key] stringByAddingPercentEscapesUsingEncoding:NSUTF8StringEncoding] : ;
 if ([encodedValue isKindOfClass:[NSDictionary class]])
 {
 encodedValue = [NSJSONSerialization dataWithJSONObject:encodedValue options:NSJSONWritingPrettyPrinted error:nil];
 encodedValue = [[[NSString alloc] initWithData:encodedValue encoding:NSUTF8StringEncoding] stringByAddingPercentEscapesUsingEncoding:NSUTF8StringEncoding] ? : encodedValue;
 }
 else if ([encodedValue isKindOfClass:[NSString class]]) encodedValue = [dictionary[key] stringByAddingPercentEscapesUsingEncoding:NSUTF8StringEncoding];
 else if ([encodedValue isKindOfClass:[NSNumber class]]) encodedValue = encodedValue;
 else if ([encodedValue isKindOfClass:[NSData class]]) encodedValue = [[NSString alloc] initWithData:encodedValue encoding:NSUTF8StringEncoding] ?: encodedValue;
 else if ([encodedValue isKindOfClass:[NSArray class]]) {
 encodedValue = [NSJSONSerialization dataWithJSONObject:encodedValue options:NSJSONWritingPrettyPrinted error:nil];
 encodedValue = [[[NSString alloc] initWithData:encodedValue encoding:NSUTF8StringEncoding] stringByAddingPercentEscapesUsingEncoding:NSUTF8StringEncoding] ? : encodedValue;;
 }
 NSString *encodedKey = [key stringByAddingPercentEscapesUsingEncoding:NSUTF8StringEncoding];
 NSString *part = [NSString stringWithFormat: @"%@=%@", encodedKey, encodedValue];
 [parts addObject:part];
 }
 NSString *encodedDictionary = [parts componentsJoinedByString:@"&"];
 return encodedDictionary;
 */
#include <RSCoreFoundation/RSJSONSerialization.h>
RSExport RSStringRef RSStringByAddingPercentEscapesUsingEncoding(RSStringRef string, RSStringEncoding encoding) {
    return RSAutorelease(RSURLCreateStringByAddingPercentEscapes(RSAllocatorDefault, string, nil, RSSTR("!*'();:@&=+$,/?%#[]"), encoding));
}

RSExport RSStringRef RSStringURLEncode(RSDictionaryRef dictionary) {
    RSMutableArrayRef parts = RSArrayCreateMutable(RSAllocatorSystemDefault, RSDictionaryGetCount(dictionary));
    RSDictionaryApplyBlock(dictionary, ^(const void *key, const void *value, BOOL *stop) {
        RSTypeRef encodedValue = value;
        if (RSGetTypeID(encodedValue) == RSDictionaryGetTypeID()) {
            RSTypeRef tmp = RSJSONSerializationCreateData(RSAllocatorSystemDefault, encodedValue);
            encodedValue = RSStringByAddingPercentEscapesUsingEncoding(RSStringWithData(tmp, RSStringEncodingUTF8), RSStringEncodingUTF8) ? : encodedValue;
            RSRelease(tmp);
        } else if (RSGetTypeID(encodedValue) == RSStringGetTypeID()){
            encodedValue = RSStringByAddingPercentEscapesUsingEncoding(encodedValue, RSStringEncodingUTF8);
        } else if (RSGetTypeID(encodedValue) == RSNumberGetTypeID()) {
            encodedValue = encodedValue;
        } else if (RSGetTypeID(encodedValue) == RSDataGetTypeID()) {
            encodedValue = RSStringWithData(encodedValue, RSStringEncodingUTF8) ? : encodedValue;
        } else if (RSGetTypeID(encodedValue) == RSArrayGetTypeID()) {
            RSTypeRef tmp = RSJSONSerializationCreateData(RSAllocatorSystemDefault, encodedValue);
            encodedValue = RSStringByAddingPercentEscapesUsingEncoding(RSStringWithData(tmp, RSStringEncodingUTF8), RSStringEncodingUTF8) ? : encodedValue;
            RSRelease(tmp);
        }
        RSStringRef encodedKey = RSStringByAddingPercentEscapesUsingEncoding(key, RSStringEncodingUTF8);
        RSStringRef part = RSStringCreateWithFormat(RSAllocatorSystemDefault, RSSTR("%r=%r"), encodedKey, encodedValue);
        RSArrayAddObject(parts, part);
        RSRelease(part);
    });
    RSStringRef encodedDictionary = RSStringCreateByCombiningStrings(RSAllocatorSystemDefault, parts, RSSTR("&"));
    RSRelease(parts);
    return RSAutorelease(encodedDictionary);
//    RSMutableArrayRef array = RSArrayCreateMutable(RSAllocatorDefault, RSDictionaryGetCount(dict));
//    RSDictionaryApplyBlock(dict, ^(const void *key, const void *value, BOOL *stop) {
//        RSArrayAddObject(array, RSStringWithFormat(RSSTR("%r=%r"), RSAutorelease(RSURLCreateStringByAddingPercentEscapes(RSAllocatorDefault, key, nil, RSSTR("!*'();:@&=+$,/?%#[]"), RSStringEncodingUTF8)), RSAutorelease(RSURLCreateStringByAddingPercentEscapes(RSAllocatorDefault, value, nil, RSSTR("!*'();:@&=+$,/?%#[]"), RSStringEncodingUTF8))));
//    });
//    RSStringRef str = RSStringCreateByCombiningStrings(RSAllocatorDefault, array, RSSTR("&"));
//    RSRelease(array);
//    return RSAutorelease(str);
}
