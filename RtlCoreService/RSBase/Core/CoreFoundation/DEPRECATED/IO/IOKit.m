//  Created by RetVal on 10/13/11.
//  Copyright (c) 2012 RetVal. All rights reserved.
//

#include "IOKit.h"
RtlExport RtlBuffer IOCacheCreateName(RtlBuffer buf)
{
    static RtlBuffer __nameFormat = "%010ld%010ld";
    static RtlUInteger __nameMask1 = 0xFFFFABCD;
    static RtlUInteger __nameMask2 = 0xEF87FFFF;
    if (buf)
    {
        RtlZeroMemory(buf, BUFFER_SIZE);
        
        sprintf(buf, __nameFormat,random()&__nameMask1,random()&__nameMask2);
    }
    return buf;
}
RtlExport BOOL    IOCreateDirectory(RtlBuffer dir)
{
#ifdef __APPLE__
    if (dir)
    {
        NSError* err = nil;
        BOOL ret = [[NSFileManager defaultManager]createDirectoryAtPath:[NSString stringWithUTF8String:dir] withIntermediateDirectories:YES attributes:[NSDictionary dictionary] error:&err];
        if (err)
        {
            return NO;
        }
        return ret;
    }
#else //ifdef __WIN32
    if (dir)
    {
        RtlBlock cmd[BUFFER_SIZE*2] = {0};
        strcpy(cmd, "md ");
        strcat(cmd, dir);
        system(cmd);
    }
    return YES;
#endif
    return NO;
}

RtlExport BOOL IOIsDirectory(const RtlBuffer dir)
{
    BOOL type = 0;
#ifdef __APPLE__
    NSFileManager* fmg = [NSFileManager defaultManager];
    [fmg fileExistsAtPath:[NSString stringWithUTF8String:dir] isDirectory:&type];
#else
#endif
    return type;
}

RtlExport RtlBuffer IOFTranslatePath(const RtlBuffer path)
{
    if (path)
    {
#ifdef  __APPLE__
        strcpy(path, [[[NSString stringWithUTF8String:path]stringByStandardizingPath]UTF8String]);
        if (path[strlen(path)] != '/')
        {
            strcat(path, "/");
        }
#endif
        return path;
    }
    return nil;
}

RtlExport RtlBuffer IOTranslatePath(const RtlBuffer path)
{
    RtlBuffer buf = nil;
    if (path)
    {
#ifdef  __APPLE__
        strcpy(buf = MSAllocateIOUnit(), [[[NSString stringWithUTF8String:path]stringByStandardizingPath]UTF8String]);
#endif
        return buf;
    }
    return buf;
}

#ifdef __APPLE__
RtlExport NSArray* IOEnumObjectsAtPath(RtlBuffer path)
{
    NSArray* array = nil;
    NSError* err = nil;
    BOOL isExisting = IOCheckFileExisting(path);
    if (isExisting == YES)
    {
        NSFileManager* fmg = [NSFileManager defaultManager];
        array = [fmg contentsOfDirectoryAtPath:[NSString stringWithUTF8String:path] error:&err];
        if (err )
        {
            RtlDBGLog(@"%@",[err localizedDescription]);
            array = nil;
        }
    }
    return array;
}

RtlExport NSArray* IOEnumDirectoryAtPath(RtlBuffer path)
{
    NSArray* array = IOEnumObjectsAtPath(path);
    NSMutableArray* retArray = [[NSMutableArray alloc]init];
    
    for (id obj in array)
    {
        if (YES == IOIsDirectory((const RtlBuffer)[obj UTF8String]))
        {
            [retArray addObject:obj];
        }
    }
    return retArray;
}

RtlExport NSArray* IOEnumFileAtPath(RtlBuffer path)
{
    NSArray* array = IOEnumObjectsAtPath(path);
    NSMutableArray* retArray = [[NSMutableArray alloc]init];
    
    for (id obj in array)
    {
        if (NO == IOIsDirectory((const RtlBuffer)[obj UTF8String]))
        {
            [retArray addObject:obj];
        }
    }
    return retArray;
}

RtlExport NSArray* IOEnumSubpathAtPath(RtlBuffer path)
{
    NSArray* array = nil;
    BOOL isExisting = IOCheckFileExisting(path);
    if (isExisting == YES)
    {
        NSFileManager* fmg = [NSFileManager defaultManager];
        array = [fmg subpathsAtPath:[NSString stringWithUTF8String:path]];
    }
    return array;
}

RtlExport RtlBuffer IOGetCurrentBundlePath()
{
    NSBundle* bundle = [NSBundle mainBundle];
    return (RtlBuffer)[[bundle bundlePath]UTF8String];
}
#endif