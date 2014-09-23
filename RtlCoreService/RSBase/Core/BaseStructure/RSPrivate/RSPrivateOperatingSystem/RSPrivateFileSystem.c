//
//  RSPrivateFileSystem.c
//  RSCoreFoundation
//
//  Created by RetVal on 5/14/13.
//  Copyright (c) 2013 RetVal. All rights reserved.
//

#include "RSPrivateFileSystem.h"
#include <RSCoreFoundation/RSFileManager.h>
#include <RSCoreFoundation/RSString+Extension.h>
#include <libproc.h>

BOOL _RSGetCurrentDirectory(char *path, int maxlen)
{
    return getcwd(path, maxlen) != nil;
}

BOOL _RSGetExecutablePath(char *path, int maxlen)
{
    if (path == nil || maxlen < 1) return NO;
#if DEPLOYMENT_TARGET_MACOSX || DEPLOYMENT_TARGET_IPHONE
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

static const char *__RSProcessPath = nil;
static const char *__RSprogname = nil;

const char **_RSGetProgname(void) {
    if (!__RSprogname)
        _RSProcessPath();		// sets up __RSprogname as a side-effect
    return &__RSprogname;
}

const char **_RSGetProcessPath(void) {
    if (!__RSProcessPath)
        _RSProcessPath();		// sets up __RSProcessPath as a side-effect
    return &__RSProcessPath;
}

#if DEPLOYMENT_TARGET_WINDOWS
const char *_RSProcessPath(void) {
    if (__RSProcessPath) return __RSProcessPath;
    wchar_t buf[RSMaxPathSize] = {0};
    DWORD rlen = GetModuleFileNameW(nil, buf, sizeof(buf) / sizeof(buf[0]));
    if (0 < rlen) {
        char asciiBuf[RSMaxPathSize] = {0};
        int res = WideCharToMultiByte(CP_UTF8, 0, buf, rlen, asciiBuf, sizeof(asciiBuf) / sizeof(asciiBuf[0]), nil, nil);
        if (0 < res) {
            __RSProcessPath = strdup(asciiBuf);
            __RSprogname = strrchr(__RSProcessPath, PATH_SEP);
            __RSprogname = (__RSprogname ? __RSprogname + 1 : __RSProcessPath);
        }
    }
    if (!__RSProcessPath) {
        __RSProcessPath = "";
        __RSprogname = __RSProcessPath;
    }
    return __RSProcessPath;
}
#endif

#if DEPLOYMENT_TARGET_MACOSX || DEPLOYMENT_TARGET_EMBEDDED || DEPLOYMENT_TARGET_EMBEDDED_MINI
#include <mach-o/dyld.h>
const char *_RSProcessPath(void) {
    if (__RSProcessPath) return __RSProcessPath;
#if DEPLOYMENT_TARGET_MACOSX
    if (!issetugid()) {
        const char *path = (char *)__RSRuntimeGetEnvironment("RSProcessPath");
        if (path) {
            __RSProcessPath = strdup(path);
            __RSprogname = strrchr(__RSProcessPath, PATH_SEP);
            __RSprogname = (__RSprogname ? __RSprogname + 1 : __RSProcessPath);
            return __RSProcessPath;
        }
    }
#endif
    uint32_t size = RSMaxPathSize;
    char buffer[size];
    if (0 == _NSGetExecutablePath(buffer, &size)) {
        __RSProcessPath = strdup(buffer);
        __RSprogname = strrchr(__RSProcessPath, PATH_SEP);
        __RSprogname = (__RSprogname ? __RSprogname + 1 : __RSProcessPath);
    }
    if (!__RSProcessPath) {
        __RSProcessPath = "";
        __RSprogname = __RSProcessPath;
    }
    return __RSProcessPath;
}
#endif

#if DEPLOYMENT_TARGET_LINUX
#include <unistd.h>

const char *_RSProcessPath(void) {
    if (__RSProcessPath) return __RSProcessPath;
    char buf[RSMaxPathSize + 1];
    
    ssize_t res = readlink("/proc/self/exe", buf, RSMaxPathSize);
    if (res > 0) {
        // null terminate, readlink does not
        buf[res] = 0;
        __RSProcessPath = strdup(buf);
        __RSprogname = strrchr(__RSProcessPath, PATH_SEP);
        __RSprogname = (__RSprogname ? __RSprogname + 1 : __RSProcessPath);
    } else {
        __RSProcessPath = "";
        __RSprogname = __RSProcessPath;
    }
    return __RSProcessPath;
}
#endif

