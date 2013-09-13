//
//  RSPrivateShell.c
//  RSCoreFoundation
//
//  Created by RetVal on 2/16/13.
//  Copyright (c) 2013 RetVal. All rights reserved.
//

int __RSPrivateRunShell(const char* cmdCore, RSStringRef format, ...)
{
    if (nil == cmdCore) return -1;
    const char* runCmd = cmdCore;
    RSStringRef runStr = nil;
    int error = -1;
    if (likely(format))
    {
        va_list vl;
        va_start(vl, format);
        RSStringRef arguments = RSStringCreateWithFormatAndArguments(RSAllocatorSystemDefault, 64, format, vl);
        va_end(vl);
        runStr = RSStringCreateWithFormat(RSAllocatorSystemDefault, RSSTR("%s %R"), runCmd, arguments);
        RSRelease(arguments);
        if (likely(runStr)) runCmd = RSStringGetCStringPtr(runStr, RSStringEncodingMacRoman);
    }
    error = system(runCmd);
    if (runStr) RSRelease(runStr);
    return error;
}