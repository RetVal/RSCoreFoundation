//
//  RSPrivateFileSystem.c
//  RSCoreFoundation
//
//  Created by RetVal on 5/14/13.
//  Copyright (c) 2013 RetVal. All rights reserved.
//

#include "RSPrivateFileSystem.h"

BOOL _RSGetCurrentDirectory(char *path, int maxlen)
{
    return getcwd(path, maxlen) != NULL;
}

BOOL _RSGetExecutablePath(char *path, int maxlen)
{
    if (path == nil || maxlen < 1) return NO;
#if DEPLOYMENT_TARGET_MACOSX || DEPLOYMENT_TARGET_IPHONE
    
#include <libproc.h>
    char *selfname[PROC_PIDPATHINFO_MAXSIZE] = {0};
    if (proc_pidpath(getpid(), selfname, PROC_PIDPATHINFO_MAXSIZE) > 0)
    {
        memcpy(path, selfname, min(maxlen, PROC_PIDPATHINFO_MAXSIZE));
        return YES;
    }
#elif DEPLOYMENT_TARGET_LINUX

    const char sysfile[15] = "/proc/self/exe";
    const int  namelen = 256;
    char selfname[256] = {0};
    
    memset(selfname, 0, 256);
    
    if ( -1 != readlink( sysfile,
                        selfname,
                        namelen) )
    {
        memcpy(path, selfname, min(maxlen, namelen));
        return YES;
    }
#elif DEPLOYMENT_TARGET_WINDOWS
    if (GetModuleFileName(nil, path, maxlen) > 0)
        return YES;
#endif
    return NO;
}

#if DEPLOYMENT_TARGET_MACOSX || DEPLOYMENT_TARGET_IPHONEOS
#include <dlfcn.h>
#include <RSCoreFoundation/RSBundle.h>

RSHandle __RSLoadLibrary(RSStringRef dir, RSStringRef lib)
{
    RSHandle handle = nil;
    RSCBuffer path = nil;
    RSStringRef fullPath = nil;
    if (dir) fullPath = RSRetain(RSFileManagerStandardizingPath(RSStringWithFormat(RSSTR("%r/%r"), dir, lib)));
    else fullPath = RSRetain(lib);
    
    if (YES == RSFileManagerFileExistsAtPath(RSFileManagerGetDefault(), fullPath, nil))
    {
        path = RSStringGetCStringPtr(fullPath, RSStringEncodingMacRoman);
        handle = path ? dlopen(path, RTLD_LAZY) : nil;
    }
    
    RSRelease(fullPath);
    return handle;
}
#endif

RSHandle _RSLoadLibrary(RSStringRef library)
{
    if (!library) return nil;
    void *handle = nil;
#if DEPLOYMENT_TARGET_MACOSX
    if ((handle = __RSLoadLibrary(nil, library))) return handle;
    if ((handle = __RSLoadLibrary(RSSTR("/System/Library/Frameworks"), library))) return handle;
    if ((handle = __RSLoadLibrary(RSSTR("/Library/Frameworks"), library))) return handle;
    if ((handle = __RSLoadLibrary(RSSTR("~/Library/Frameworks"), library))) return handle;
#elif DEPLOYMENT_TARGET_IPHONEOS
    if ((handle = __RSLoadLibrary(nil, library))) return handle;
#endif
    return handle;
}

void *_RSGetImplementAddress(RSHandle handle, const char* key)
{
    if (!handle || !key) return nil;
#if DEPLOYMENT_TARGET_MACOSX || DEPLOYMENT_TARGET_IPHONEOS
    return dlsym(handle, key);
#endif
    return nil;
}

void _RSCloseHandle(RSHandle handle)
{
    if (!handle) return;
#if DEPLOYMENT_TARGET_MACOSX || DEPLOYMENT_TARGET_IPHONEOS
    dlclose(handle);
    return;
#elif DEPLOYMENT_TARGET_WINDOWS
    closeHandle(handle);
#endif
    return;
}