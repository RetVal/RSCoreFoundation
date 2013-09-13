//
//  RSPathUtilities.c
//  RSCoreFoundation
//
//  Created by RetVal on 6/5/13.
//  Copyright (c) 2013 RetVal. All rights reserved.
//

#include <RSCoreFoundation/RSPathUtilities.h>

RSExport RSStringRef RSTemporaryPath(RSStringRef prefix, RSStringRef suffix)
{
    RSStringRef tmpPath = nil;
    RSBlock tmpFileName[RSBufferSize] = "RSCoreFoundation-cache-XXXXXX";
    if (prefix && suffix) tmpPath = RSStringCreateWithFormat(RSAllocatorSystemDefault, RSSTR("%r%r%s%r"), RSTemporaryDirectory(), prefix, mktemp(tmpFileName), suffix);
    else if (prefix) tmpPath = RSStringCreateWithFormat(RSAllocatorSystemDefault, RSSTR("%r%r%s"), RSTemporaryDirectory(), prefix, mktemp(tmpFileName));
    else if (suffix) tmpPath = RSStringCreateWithFormat(RSAllocatorSystemDefault, RSSTR("%r%s%r"), RSTemporaryDirectory(), mktemp(tmpFileName), suffix);
    else tmpPath = RSStringCreateWithFormat(RSAllocatorSystemDefault, RSSTR("%r%s"), RSTemporaryDirectory(), mktemp(tmpFileName));
    return RSAutorelease(tmpPath);
}
extern RSStringRef _RSTemporaryDirectory;   // declare in <RSCoreFoundation/RSFileManager.c>
RSExport RSStringRef RSTemporaryDirectory()
{
    
    if (_RSTemporaryDirectory) return _RSTemporaryDirectory;
    RSBlock td[PATH_MAX] = {0};
    size_t rlen = confstr(_CS_DARWIN_USER_TEMP_DIR, td, PATH_MAX);
    if (rlen && rlen <= PATH_MAX) {
        _RSTemporaryDirectory = RSStringCreateWithCString(RSAllocatorSystemDefault, td, RSStringEncodingUTF8);
        __RSRuntimeSetInstanceSpecial(_RSTemporaryDirectory, YES);
    }
    return _RSTemporaryDirectory;
}