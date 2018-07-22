//
//  RSBundlePrivateC0Context.c
//  RSCoreFoundation
//
//  Created by RetVal on 2/20/13.
//  Copyright (c) 2013 RetVal. All rights reserved.
//

#include <stdio.h>
#include "RSBundlePrivate.h"
#include "RSBundlePrivateC0Context.h"
#include <RSCoreFoundation/RSBundle.h>
#include <RSCoreFoundation/RSFileManager.h>
#pragma mark __RSBundle Check Context (C0)
struct __RSBundleC0Context
{
    RSStringRef _fileSystemPath;    // the bundle base path in the file system (standardizing path with file manager with self retain(strong))
    RSStringRef _extension;         // app, framework, bundle, customer-bundle
    struct
    {
        RSBitU8 _avaiable       : 1;
        RSBitU8 _versions       : 1;
        RSBitU8 _contentsOrSupportFiles : 1;
        union
        {
            struct
            {
                RSBitU8 __infoPlist     : 1;
                
                RSBitU8 __PkgInfo       : 1;
                RSBitU8 __Resources     : 1;
                RSBitU8 ___Resources_JavaResource : 1;
                RSBitU8 ___Resources_infoPlist : 1;
                
                RSBitU8 __MacOS         : 1;
                RSBitU8 __Frameworks    : 1;
                RSBitU8 __PlugIns       : 1;
                RSBitU8 __Plug_ins      : 1;
                RSBitU8 __SharedFrameworks : 1;
                RSBitU8 __SharedSupport : 1;
                RSBitU8 __XPCServices   : 1;
                
                RSBitU8 __HeaderFiles   : 1;
                RSBitU8 __PrivateFiles  : 1;
            }_app;
            
            struct
            {
                RSBitU8 _HeaderLink     : 1;
                RSBitU8 _PrivateLink    : 1;
                RSBitU8 _executeLink    : 1;
                RSBitU8 _Versions       : 1;
                
                RSBitU8 __VersionsCurLnk: 1;
                RSBitU8 __versionList   : 1;
                
                RSBitU8 ___Resources    : 1;
                RSBitU8 ___Resources_JavaResource : 1;
                RSBitU8 ___Reousrces_infoPlist : 1;
                RSBitU8 ___Headers      : 1;
                RSBitU8 ___PrivateHeaders:1;
                RSBitU8 ___executeFile  : 1;
                
                RSBitU8 ___Frameworks    : 1;
                RSBitU8 ___PlugIns       : 1;
                RSBitU8 ___Plug_ins      : 1;
                RSBitU8 ___SharedFrameworks : 1;
                RSBitU8 ___SharedSupport : 1;
                RSBitU8 ___XPCServices   : 1;
            }_framework;
        }_kind;
    }_flag ;
};

RSInline BOOL ___RSBundleC0ContextCheckForBundleDirectory(RSAllocatorRef allocator, RSFileManagerRef fmg, RSStringRef bundlePath, RSStringRef appendPath, BOOL shouldExisting, BOOL shouldDirectory)
{
    RSMutableStringRef path = RSMutableCopy(allocator, bundlePath);
    BOOL result = NO, isDir = NO;
    result = RSFileManagerFileExistsAtPath(fmg, RSFileManagerAppendFileName(fmg, path, appendPath), &isDir);
    if (result == shouldExisting) result = YES;
    if (isDir == shouldDirectory) result = YES;
    RSRelease(path);
    return result;
}

static RSBundleC0ContextPtr __RSBundleC0ContextCreateForFramework(RSAllocatorRef allocator, RSStringRef path)
{
    RSBundleC0ContextPtr ptr = RSAllocatorAllocate(allocator, sizeof(struct __RSBundleC0Context));
    RSFileManagerRef fmg = RSFileManagerGetDefault();
    BOOL result = NO, isDir = NO;
    RSMutableStringRef bundlePath = RSMutableCopy(allocator, path);
    result = RSFileManagerFileExistsAtPath(fmg,
                                           RSFileManagerAppendFileName(fmg,
                                                                       bundlePath,
                                                                       RSSTR("Versions")),
                                           &isDir);
    if (result && isDir)
    {
        ptr->_flag._versions = 0;
        ptr->_flag._contentsOrSupportFiles = 1;
        
        ptr->_flag._kind._app.__infoPlist = ___RSBundleC0ContextCheckForBundleDirectory(allocator, fmg, bundlePath,
                                                                                        _RSBundleInfoPathFromBase3,
                                                                                        YES, NO);
        
        ptr->_flag._kind._app.__Resources = ___RSBundleC0ContextCheckForBundleDirectory(allocator, fmg, bundlePath,
                                                                                        _RSBundleResourcesPathFromBase2,
                                                                                        YES, YES);
        
        ptr->_flag._kind._app.__MacOS     = ___RSBundleC0ContextCheckForBundleDirectory(allocator, fmg, bundlePath,
                                                                                        RSSTR("MacOS"),
                                                                                        YES, YES);
        
        ptr->_flag._kind._app.__Frameworks= ___RSBundleC0ContextCheckForBundleDirectory(allocator, fmg, bundlePath,
                                                                                        _RSBundlePrivateFrameworksPathFromBase2,
                                                                                        YES, YES);
        
        ptr->_flag._kind._app.__PkgInfo   = ___RSBundleC0ContextCheckForBundleDirectory(allocator, fmg, bundlePath,
                                                                                        _RSBundlePkgInfoPathFromBase2,
                                                                                        YES, NO);
        if (ptr->_flag._kind._app.__PkgInfo == YES &&
            ptr->_flag._kind._app.__infoPlist == YES &&
            ptr->_flag._kind._app.__Resources == YES &&
            ptr->_flag._kind._app.__MacOS == YES &&
            ptr->_flag._kind._app.__Frameworks == YES)
        {
            ptr->_flag._avaiable = YES;
        }
    }
    
    RSRelease(path);
    return ptr;
}

static RSBundleC0ContextPtr __RSBundleC0ContextCreateForApp(RSAllocatorRef allocator, RSStringRef path)
{
    RSBundleC0ContextPtr ptr = RSAllocatorAllocate(allocator, sizeof(struct __RSBundleC0Context));
    RSMutableStringRef bundlePath = RSMutableCopy(allocator, path);
    BOOL result = NO, isDir = NO;
    RSFileManagerRef fmg = RSFileManagerGetDefault();
    result = RSFileManagerFileExistsAtPath(fmg, RSFileManagerAppendFileName(fmg, bundlePath, _RSBundleSupportFilesDirectoryName2), &isDir);
    if (result && isDir)
    {
        ptr->_flag._versions = 0;
        ptr->_flag._contentsOrSupportFiles = 1;
        
        ptr->_flag._kind._app.__infoPlist = ___RSBundleC0ContextCheckForBundleDirectory(allocator, fmg, bundlePath,
                                                                                        _RSBundleInfoPathFromBase3,
                                                                                        YES, NO);
        
        ptr->_flag._kind._app.__Resources = ___RSBundleC0ContextCheckForBundleDirectory(allocator, fmg, bundlePath,
                                                                                        _RSBundleResourcesPathFromBase2,
                                                                                        YES, YES);

        ptr->_flag._kind._app.__MacOS     = ___RSBundleC0ContextCheckForBundleDirectory(allocator, fmg, bundlePath,
                                                                                        RSSTR("MacOS"),
                                                                                        YES, YES);
        
        ptr->_flag._kind._app.__Frameworks= ___RSBundleC0ContextCheckForBundleDirectory(allocator, fmg, bundlePath,
                                                                                         _RSBundlePrivateFrameworksPathFromBase2,
                                                                                         YES, YES);
        
        ptr->_flag._kind._app.__PkgInfo   = ___RSBundleC0ContextCheckForBundleDirectory(allocator, fmg, bundlePath,
                                                                                        _RSBundlePkgInfoPathFromBase2,
                                                                                        YES, NO);
        if (ptr->_flag._kind._app.__PkgInfo == YES &&
            ptr->_flag._kind._app.__infoPlist == YES &&
            ptr->_flag._kind._app.__Resources == YES &&
            ptr->_flag._kind._app.__MacOS == YES &&
            ptr->_flag._kind._app.__Frameworks == YES)
        {
            ptr->_flag._avaiable = YES;
        }
        
    }
    else
    {
        RSRelease(bundlePath);
        bundlePath = RSMutableCopy(allocator, path);
        result = RSFileManagerFileExistsAtPath(fmg, RSFileManagerAppendFileName(fmg, bundlePath, RSSTR("Support Files/")), &isDir);
        if (result && isDir)
        {
            ptr->_flag._versions = 0;
            ptr->_flag._contentsOrSupportFiles = 0;
            
            ptr->_flag._kind._app.__infoPlist = ___RSBundleC0ContextCheckForBundleDirectory(allocator, fmg, bundlePath,
                                                                                            RSSTR("Support Files/Info.plist"),
                                                                                            YES, NO);
            
            ptr->_flag._kind._app.__Resources = ___RSBundleC0ContextCheckForBundleDirectory(allocator, fmg, bundlePath,
                                                                                            RSSTR("Support Files/Resources/"),
                                                                                            YES, YES);
            
            ptr->_flag._kind._app.__MacOS     = ___RSBundleC0ContextCheckForBundleDirectory(allocator, fmg, bundlePath,
                                                                                            RSSTR("MacOS"),
                                                                                            YES, YES);
            
            ptr->_flag._kind._app.__Frameworks= ___RSBundleC0ContextCheckForBundleDirectory(allocator, fmg, bundlePath,
                                                                                            RSSTR("Support Files/Frameworks/"),
                                                                                            YES, YES);
            
            ptr->_flag._kind._app.__PkgInfo   = ___RSBundleC0ContextCheckForBundleDirectory(allocator, fmg, bundlePath,
                                                                                            RSSTR("Support Files/PkgInfo"),
                                                                                            YES, NO);
            if (ptr->_flag._kind._app.__PkgInfo == YES &&
                ptr->_flag._kind._app.__infoPlist == YES &&
                ptr->_flag._kind._app.__Resources == YES &&
                ptr->_flag._kind._app.__MacOS == YES &&
                ptr->_flag._kind._app.__Frameworks == YES)
            {
                ptr->_flag._avaiable = YES;
            }
        }
    }
    RSRelease(bundlePath);
    
    ptr->_fileSystemPath = RSRetain(path);
    ptr->_extension = RSSTR("app");
    return ptr;
}

RSBundleC0ContextPtr __RSBundleC0ContextCreate(RSAllocatorRef allocator, RSStringRef path)
{
    RSBundleC0ContextPtr ptr = nil;
    RSFileManagerRef fmg = RSFileManagerGetDefault();
    BOOL isDir = NO, result = NO;
    if (YES == (result = RSFileManagerFileExistsAtPath(fmg, path, &isDir)))
    {
        do
        {
            RSMutableStringRef bundlePath = RSMutableCopy(allocator, path);
            result = RSFileManagerFileExistsAtPath(fmg,
                                                   RSFileManagerAppendFileName(fmg,
                                                                               bundlePath,
                                                                               _RSBundleSupportFilesDirectoryName2),
                                                   &isDir);
            RSRelease(bundlePath);
            if (result == isDir && isDir == YES)
            {
                ptr = __RSBundleC0ContextCreateForApp(allocator, path);
                break;
            }
            
            bundlePath = RSMutableCopy(allocator, path);
            result = RSFileManagerFileExistsAtPath(fmg,
                                                   RSFileManagerAppendFileName(fmg,
                                                                               bundlePath,
                                                                               RSSTR("Versions")),
                                                   &isDir);
            RSRelease(bundlePath);
            if (result == isDir && isDir == YES)
            {
                ptr = __RSBundleC0ContextCreateForFramework(allocator, path);
                break;
            }
        } while (0);
    }
    return ptr;
}
