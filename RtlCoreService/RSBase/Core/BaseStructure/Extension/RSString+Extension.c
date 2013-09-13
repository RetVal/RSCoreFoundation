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

RSExport RSStringRef RSStringWithUTF8String(UTF8Char* nullTerminatingString)
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
    return RSAutorelease(RSStringCreateWithContentOfPath(RSAllocatorSystemDefault, path));
}

RSExport RSStringRef RSStringCreateWithContentOfPath(RSAllocatorRef allocator, RSStringRef path)
{
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
