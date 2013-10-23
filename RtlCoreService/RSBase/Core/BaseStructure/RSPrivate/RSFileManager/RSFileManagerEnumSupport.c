//
//  RSFileManagerEnumSupport.c
//  RSCoreFoundation
//
//  Created by RetVal on 1/31/13.
//  Copyright (c) 2013 RetVal. All rights reserved.
//

#include <RSCoreFoundation/RSFileManager.h>

#include "RSFileManagerEnumSupport.h"

#include <dirent.h>
#include <fcntl.h>
#if __RSFileManagerContentsContextDoNotUseRSArray
static BOOL __RSFileManagerContentContextReAllocate(RSAllocatorRef allocator, __RSFileManagerContentsContext* context, RSIndex capacity)
{
    if (context == nil || capacity <= 0) return NO;
    if (allocator == nil) allocator = RSAllocatorSystemDefault;

    if (context)
    {
        if (context->spaths)
        {
            if (capacity > context->capacity)
            {
                if (context->spaths = RSAllocatorReallocate(allocator, context->spaths, capacity))
                {
                    context->capacity = RSAllocatorSize(allocator, capacity);
                    return YES;
                }
                else return NO;
            }
            return YES;
        }
        else if (context->spaths == nil)
        {
            if (context->spaths = RSAllocatorAllocate(allocator, context->spaths, capacity) )
            {
                context->capacity = RSAllocatorSize(allocator, capacity);
                return YES;
            }
        }
    }
    return NO;
}
#else
static RSStringRef __RSFileManagerContentContextAppendPrefix(__RSFileManagerContentsContext* context, RSStringRef name)
{
    if (context == nil || name == nil) return nil;
    else if (context->dirStack == nil)
    {
        return RSRetain(name);
    }
    return RSStringCreateWithFormat(RSAllocatorSystemDefault, RSSTR("%R/%R"), context->dirStack, name);
}

static void __RSFileManagerContentContextUpdatePrefix(__RSFileManagerContentsContext* context)
{
    RSMutableStringRef fullObj = nil;
    if (context->currentPrefix) RSRelease(context->currentPrefix);
    context->currentPrefix = nil;
    if (likely(context->currentPrefix == nil))
    {
        RSIndex cnt = 0;
        if (likely(context->dirStack) && (cnt = RSArrayGetCount(context->dirStack)))
        {
            BOOL prefix = NO, suffix = NO;
            RSTypeRef objInArray = RSArrayObjectAtIndex(context->dirStack, 0);
            fullObj = RSMutableCopy(RSAllocatorSystemDefault, objInArray);
            
            for (RSIndex idx = 1; idx < cnt; idx++)
            {
                __RSRuntimeMemoryBarrier();
                if (NO == RSStringHasSuffix(fullObj, RSSTR("/")))
                    suffix = YES;
                
                objInArray = RSArrayObjectAtIndex(context->dirStack, idx);
                if (NO == RSStringHasPrefix(fullObj, RSSTR("/")))
                    prefix = YES;
                
                if (suffix || prefix) RSStringAppendCString(fullObj, "/", RSStringEncodingASCII);
                
                RSStringAppendString(fullObj, objInArray);
                suffix = prefix = NO;
            }
        }
        context->currentPrefix = fullObj;
    }
    
}

static void __RSFileManagerContentContextUpdateDirStack(__RSFileManagerContentsContext* context, RSStringRef dir)
{
    if (context == nil || context->dirStack == nil) return;
    BOOL push = dir != nil;
    if (push)
    {
        RSArrayAddObject(context->dirStack, dir);
        __RSFileManagerContentContextUpdatePrefix(context);
    }
    else {
        RSArrayRemoveLastObject(context->dirStack);
        __RSFileManagerContentContextUpdatePrefix(context);
    }
}
#endif
static void __RSFileManagerContentContextAddContent(__RSFileManagerContentsContext* context, RSStringRef obj)
{
    if (context == nil) return;
#if __RSFileManagerContentsContextDoNotUseRSArray
    if (context->spaths == nil)
    {
        if (NO == __RSFileManagerContentContextReAllocate(RSAllocatorSystemDefault, context, 64)) return;
    }
    if (context->spaths)
    {
        if (context->numberOfContents >= context->capacity)
        {
            if (NO == __RSFileManagerContentContextReAllocate(RSAllocatorSystemDefault, context, context->capacity*2)) return;
        }
        __RSRuntimeMemoryBarrier();
        context->spaths[context->numberOfContents++] = (obj);
    }
#else
    if (context->contain == nil) context->contain = RSArrayCreateMutable(RSAllocatorSystemDefault, 32);
    if (context->dirStack == nil) context->dirStack = RSArrayCreateMutable(RSAllocatorSystemDefault, 32);
    RSStringRef fullObj = nil;
    if (context->currentPrefix)
    {
        fullObj = RSStringCreateWithFormat(RSAllocatorSystemDefault, RSSTR("%R/%R"), context->currentPrefix, obj);
    	RSArrayAddObject(context->contain, fullObj);
        RSRelease(fullObj);
    }
    else
    {
        RSArrayAddObject(context->contain, obj);
    }
    
#endif
}

void __RSFileManagerContentContextFree(__RSFileManagerContentsContext* context)
{
    if (context == nil) return;
#if __RSFileManagerContentsContextDoNotUseRSArray
    if (context->spaths == nil) return;
    for (RSIndex idx = 0; idx < context->numberOfContents; idx++)
    {
        RSRelease(context->spaths[idx]);
    }
    RSAllocatorDeallocate(RSAllocatorSystemDefault, context->spaths);
    context->spaths = nil;
#else
    if (context->contain == nil) return;
    RSRelease(context->contain);
    context->contain = nil;
    
    if (context->dirStack == nil) return;
    RSRelease(context->dirStack);
    context->dirStack = nil;
#endif
}



BOOL __RSFileManagerContentsDirectory(RSFileManagerRef fmg, RSStringRef path, __autorelease RSErrorRef* error, __RSFileManagerContentsContext* context, BOOL shouldRecursion)
{
    DIR *db;
    char filename[2*RSMaxPathSize] = {0};
    struct dirent *p = nil;
    memcpy(filename, RSStringGetCStringPtr(path, RSStringEncodingMacRoman), RSStringGetLength(path));
    db = opendir(filename);
    if (db == nil)
    {
        if (error)
        {
            RSDictionaryRef userInfo = RSDictionaryCreate(RSAllocatorSystemDefault, (const void**)RSErrorTargetKey, (const void**)&path, RSDictionaryRSTypeContext, 1);
            *error = RSErrorCreate(RSAllocatorSystemDefault, RSErrorDomainRSCoreFoundation, kErrExisting, userInfo);
            RSRelease(userInfo);
            RSAutorelease(*error);
        }
        return NO;
    }
    memset(filename, 0, 2*RSMaxPathSize);
    RSStringRef _fileNameStr = nil;
    RSStringRef _dirPath = nil;
    BOOL success = NO;
    while ((p = readdir(db)))
    {
        if((strcmp(p->d_name, ".") == 0) ||
           (strcmp(p->d_name, "..") == 0))
            continue;
        if (RSStringGetLength(path) > 0)
        {
            if (RSStringGetCStringPtr(path, RSStringEncodingMacRoman)[RSStringGetLength(path) - 1] == '/')
                sprintf(filename,"%s%s", RSStringGetCStringPtr(path, RSStringEncodingMacRoman), p->d_name);
            else
                sprintf(filename,"%s/%s", RSStringGetCStringPtr(path, RSStringEncodingMacRoman), p->d_name);
        }
        else
            sprintf(filename,"%s/%s", RSStringGetCStringPtr(path, RSStringEncodingUTF8), p->d_name);
        _fileNameStr = RSStringCreateWithCString(RSAllocatorSystemDefault, p->d_name, RSStringEncodingMacRoman);
        __RSFileManagerContentContextAddContent(context, _fileNameStr);
        if (p->d_type & DT_DIR && shouldRecursion)
        {
            _dirPath = RSStringCreateWithCString(RSAllocatorSystemDefault, filename, RSStringEncodingMacRoman);
#if !__RSFileManagerContentsContextDoNotUseRSArray
            __RSFileManagerContentContextUpdateDirStack(context, _fileNameStr);
#endif
            success = __RSFileManagerContentsDirectory(fmg, _dirPath, error, context, shouldRecursion);
#if !__RSFileManagerContentsContextDoNotUseRSArray
            __RSFileManagerContentContextUpdateDirStack(context, nil);
#endif
            RSRelease(_dirPath);
            _dirPath = nil;
            if (success == NO) return success;
        }
        RSRelease(_fileNameStr);
        _fileNameStr = nil;
    }
    closedir(db);
    return YES;
}