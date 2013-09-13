//
//  RSData+Extension.c
//  RSCoreFoundation
//
//  Created by RetVal on 5/30/13.
//  Copyright (c) 2013 RetVal. All rights reserved.
//

#include <RSCoreFoundation/RSData+Extension.h>
#include <RSCoreFoundation/RSData.h>
#include <RSCoreFoundation/RSFileHandle.h>
#include <RSCoreFoundation/RSPathUtilities.h>

RSExport RSDataRef RSDataWithBytes(const void* bytes, RSIndex length)
{
    return RSAutorelease(RSDataCreate(RSAllocatorSystemDefault, bytes, length));
}

RSExport RSDataRef RSDataWithBytesNoCopy(const void* bytes, RSIndex length, BOOL freeWhenDone, RSAllocatorRef bytesDeallocator)
{
    return RSAutorelease(RSDataCreateWithNoCopy(RSAllocatorSystemDefault, bytes, length, freeWhenDone, bytesDeallocator));
}

RSExport RSDataRef RSDataWithString(RSStringRef string, RSStringEncoding encoding)
{
    return RSAutorelease(RSDataCreateWithString(RSAllocatorSystemDefault, string, encoding));
}

RSExport RSDataRef RSDataCreateWithString(RSAllocatorRef allocator, RSStringRef string, RSStringEncoding usingEncoding)
{
    if (!string) return nil;
    RSBitU8 *ptr = (RSBitU8 *)RSStringGetCStringPtr(string, usingEncoding);
    if (ptr != nil)
        return RSDataCreate(allocator, (const RSBitU8*)ptr, RSStringGetLength(string));
    RSIndex length = RSStringGetMaximumSizeForEncoding(RSStringGetLength(string), usingEncoding) + 2;
    ptr = RSAllocatorAllocate(RSAllocatorSystemDefault, length);
    if (ptr)
    {
        if (0 <= (length = RSStringGetCString(string, (char *)ptr, length, usingEncoding)))
        {
           return RSDataCreateWithNoCopy(RSAllocatorSystemDefault, ptr, length, YES, RSAllocatorSystemDefault);
        }
        RSAllocatorDeallocate(RSAllocatorSystemDefault, ptr);
    }
    return nil;
}

RSExport RSDataRef RSDataWithContentOfPath(RSStringRef path)
{
    return RSAutorelease(RSDataCreateWithContentOfPath(RSAllocatorSystemDefault, path));
}

RSExport RSMutableDataRef RSMutableDataWithBytes(const void* bytes, RSIndex length)
{
    return RSAutorelease(RSMutableCopy(RSAllocatorSystemDefault, RSDataWithBytes(bytes, length)));
}
RSExport RSMutableDataRef RSMutableDataWithBytesNoCopy(const void* bytes, RSIndex length, BOOL freeWhenDone, RSAllocatorRef bytesDeallocator)
{
    return RSAutorelease(RSMutableCopy(RSAllocatorSystemDefault, RSDataWithBytesNoCopy(bytes, length, freeWhenDone, bytesDeallocator)));
}
RSExport RSMutableDataRef RSMutableDataWithContentOfPath(RSStringRef path)
{
    return RSAutorelease(RSMutableCopy(RSAllocatorSystemDefault, RSDataWithContentOfPath(path)));
}

RSExport RSDataRef RSDataCreateWithContentOfPath(RSAllocatorRef allocator, RSStringRef path)
{
    RSDataRef data = nil;
    RSFileHandleRef handle = RSFileHandleCreateForReadingAtPath(path);
    if (handle)
        data = RSFileHandleReadDataToEndOfFile(handle);
    RSRelease(handle);
    return data;
}

static BOOL _RSDataWriteToPath(RSDataRef data, RSStringRef path)
{
    BOOL result = NO;
    RSFileHandleRef handler = RSFileHandleCreateForWritingAtPath(path);
    if (handler) result = RSFileHandleWriteData(handler, data);
    RSRelease(handler);
    return result;
}

static BOOL _RSDataWriteAutomatically(RSDataRef data, RSStringRef path)
{
    RSStringRef tmpPath = RSTemporaryPath(RSSTR("RSCoreFoundation-cache-"), RSSTR(".atdata"));
    BOOL result = _RSDataWriteToPath(data, tmpPath);
    if (result) result = RSFileManagerMoveFileToPath(RSFileManagerGetDefault(), tmpPath, path);
    return result;
}

RSExport BOOL RSDataWriteToFile(RSDataRef data, RSStringRef path, RSWriteFileMode writeMode)
{
    if (!data) return NO;
    if (writeMode == RSWriteFileAutomatically) return _RSDataWriteAutomatically(data, path);
    return RSDataWriteToFileWithError(data, path, nil);
}

RSExport BOOL RSDataWriteToFileWithError(RSDataRef data, RSStringRef path, RSErrorRef *error)
{
    __block RSErrorCode ec = kSuccess;
    RSAutoreleaseBlock(^{
        RSFileHandleRef handle = RSFileHandleCreateForWritingAtPath(path);
        if (handle)
        {
            if (RSFileHandleWriteData(RSAutorelease(handle), data)) return ;
            else ec = kErrWrite;
        }
        else ec = kErrOpen;
    });
    if (ec && error) {
        RSDictionaryRef info = RSDictionaryCreateWithObjectsAndOKeys(RSAllocatorSystemDefault, path, RSErrorFilePathKey, NULL);
        *error = RSAutorelease(RSErrorCreate(RSAllocatorSystemDefault, RSErrorDomainRSCoreFoundation, ec, info));
        RSRelease(info);
    }
    return ec ? NO : YES;
}