//
//  RSLocalCache.m
//  RSKit
//
//  Created by RetVal on 10/2/12.
//  Copyright (c) 2012 RetVal. All rights reserved.
//

#import "RSLocalCache.h"

@interface RSLocalCache()
{
    NSString* _cacheRoot;
    NSString* _cacheUseRoot;
}
@end
@implementation RSLocalCache
#pragma mark -
#pragma mark Private APIs

- (NSData*)_cacheDataFromLocal:(NSString*)Identifier
{
    NSString* atomicPath = [self cachePathWithIdentifier:Identifier];
    NSFileManager* fmg = [NSFileManager defaultManager];
    BOOL result = [fmg fileExistsAtPath:atomicPath];
    if (YES == result) {
        NSData* data = [NSData dataWithContentsOfFile:atomicPath];
        NSLog(@"RSLocalCache  - objectForKey: return (%@)",Identifier);
        
        return data;
    }
    else
    {
        NSLog(@"RSLocalCache  - objectForKey: miss Object Identifier in cache (%@)",Identifier);
    }
    return nil;
}

#pragma mark - 
#pragma mark Initialize 
- (id)init
{
    if (self = [super init]) {
        
    }
    return self;
}

- (NSString *)cacheName
{
    if (_cacheName == nil) {
        _cacheName = RSLocalCacheDefaultName;
    }
    return _cacheName;
}
- (id)initWithCache:(RSLocalCache *)aCache
{
    return [aCache copy];
}

- (id)initWithContentsOfPath:(NSString*)aPath named:(NSString*)cacheName;
{
    if ((aPath != nil) && (cacheName != nil) && (self = [self init]))
    {
        [self setCacheName:cacheName];
        NSMutableString* cacheRealPath = [[aPath stringByStandardizingPath] mutableCopy];
        [cacheRealPath appendFormat:@"/%@",[self cacheName]];
        NSFileManager* fmg = [NSFileManager defaultManager];
        BOOL isDirectory = NO;
        
        BOOL result = NO;
        [fmg createDirectoryAtPath:cacheRealPath withIntermediateDirectories:YES attributes:nil error:nil];
        result = [fmg fileExistsAtPath:cacheRealPath isDirectory:&isDirectory];
        if (result && isDirectory) {
            _cacheRoot = aPath;
            _cacheUseRoot = cacheRealPath;
            return self;
        }
    }
    return nil;
}

//- (id)initWithContentsOfPath:(NSString*)aPath AndServerPath:(NSString*)aServerPath
//{
//    if ((aPath != nil) && (self = [self init]))
//    {
//        NSMutableString* cacheRealPath = [[aPath stringByStandardizingPath] mutableCopy];
//        [cacheRealPath appendFormat:@"/%@",[self cacheName]];
//        NSFileManager* fmg = [NSFileManager defaultManager];
//        BOOL isDirectory = NO;
//        
//        BOOL result = NO;
//        [fmg createDirectoryAtPath:cacheRealPath withIntermediateDirectories:YES attributes:nil error:nil];
//        result = [fmg fileExistsAtPath:cacheRealPath isDirectory:&isDirectory];
//        if (result && isDirectory) {
//            _cacheRoot = cacheRealPath;
//            return self;
//        }
//    }
//    return nil;
//}
#pragma mark -
#pragma mark Cache Data to the FS
- (void)setObject:(id)obj forKey:(NSString *)Identifier;
{
    NSMutableString* writePath = [_cacheUseRoot mutableCopy];
    if (writePath) {
        [self buildPathWithIdentifier:Identifier];
        [writePath appendFormat:@"%@",Identifier];
        if ([obj writeToFile:writePath atomically:YES])
        {
            NSLog(@"RSLocalCache  - setObject:forKey: save (%@)",Identifier);
        }
    }
    return;
}

#pragma mark -
#pragma mark Get Data from Cache Synchronous
- (id)objectForKey:(NSString *)Identifier
{
    NSData* data = [self _cacheDataFromLocal:Identifier];
//    if (data == nil) {
//        data = [self _cacheDataFromServer:Identifier];
//        [self buildPathWithIdentifier:Identifier];
//        [data writeToFile:[self cachePathWithIdentifier:Identifier] atomically:YES];
//    }
    return data;
}

#pragma mark Get Data from Cache Asynchronous

- (void)objectForKey:(NSString *)Identifier WithContext:(void*)context WithHandler:(RSCacheHandler)handler
{
    dispatch_queue_t queue = dispatch_queue_create("RSLocalCache Update Data", NULL);
    dispatch_async(queue, ^{
        NSData* data = [self objectForKey:Identifier];
        dispatch_async(dispatch_get_main_queue(), ^{
            handler(context,data,RSCacheL1);
        });
    });
    
    //dispatch_release(queue);
    
    return;
}

#pragma mark -
#pragma mark Clean Cache
- (void)removeObjectForKey:(NSString *)Identifier
{
    NSMutableString* writePath = [_cacheUseRoot mutableCopy];
    if (writePath) {
        [self buildPathWithIdentifier:Identifier];
        [writePath appendFormat:@"%@",Identifier];
        NSError* error = nil;
        if ([[NSFileManager defaultManager] removeItemAtPath:writePath error:&error])
        {
            NSLog(@"RSLocalCache  - removeObjectForKey: remove (%@)",Identifier);
        }
        else
        {
            NSLog(@"RSLocalCache  - removeObjectForKey: error (%@)",error);
        }
    }
    return;
}

- (void)removeAllObjects
{
    if ([self cacheRoot]) {
        NSError* error = nil;
        BOOL result = [[NSFileManager defaultManager] removeItemAtPath:[self cacheRoot] error:&error];
        if (result == NO) {
            NSLog(@"RSLocalCache  - removeAllObjects error %@",[error localizedDescription]);
        }
    }
}
//#pragma mark -
//#pragma mark RSDownloaderDelegate API
//- (void)RSDownloadFinished:(NSString *)filePath
//{
//    
//}
@end

@implementation RSLocalCache (FileSystem)
- (void)buildPathWithIdentifier:(NSString *)Identifier
{
    NSMutableString* root = [_cacheUseRoot mutableCopy];
    if (root) {
        NSArray* parts = [Identifier componentsSeparatedByString:@"/"];
        NSFileManager* fmg = [NSFileManager defaultManager];
        NSError* error = nil;
        BOOL result = NO;
        for (NSString* part in parts) {
            [root appendFormat:@"/%@",part];
            result = [fmg createDirectoryAtPath:root withIntermediateDirectories:YES attributes:nil error:&error];
            if (result == NO) {
                if (error) {
                    NSLog(@"RSLocalCache  - buildPathWithIdentifier: %@",[error localizedDescription]);
                }
            }
        }
        [fmg removeItemAtPath:root error:nil];
    }
    return;
}

- (NSString*)cachePathWithIdentifier:(NSString*)Identifier
{
    NSMutableString* atomicPath = [_cacheUseRoot mutableCopy];
    [atomicPath appendFormat:@"%@",Identifier];
    return atomicPath;
}
@end