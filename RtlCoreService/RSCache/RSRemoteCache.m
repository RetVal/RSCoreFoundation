//
//  RSRemoteCache.m
//  RSKit
//
//  Created by RetVal on 10/7/12.
//  Copyright (c) 2012 RetVal. All rights reserved.
//

#import "RSRemoteCache.h"

@implementation RSRemoteCache
#pragma mark RSRemoteCache Private APIs
- (NSData*)_cacheDataFromServer:(NSString*)Identifier
{
    if ([self serverRoot] != nil)
    {
        // get the data from the server root
        NSLog(@"RSRemoteCache - objectForKey: Get %@",Identifier);
        NSData* data = [NSData dataWithContentsOfURL:[self cacheURLWithIdentifier:Identifier]];
        if (data) {
            NSLog(@"RSRemoteCache - objectForKey: return %@",Identifier);
        }else
        {
            [[NSException exceptionWithName:NSPortTimeoutException reason:@"RSRemoteCache data missed" userInfo:nil]raise];
        }
        return data;
    }
    return nil;
}

- (id)initWithCache:(RSRemoteCache *)aCache
{
    return [aCache copy];
}

- (id)initWithContentsOfURL:(NSURL *)aURL
{
    if (aURL && (self = [super init]))
    {
        _serverRoot = aURL;
        
        return self;
    }
    return nil;
}
- (void)setObject:(id)obj forKey:(NSString *)Identifier
{
    [obj writeToURL:[self cacheURLWithIdentifier:Identifier] atomically:YES];
}
- (id)objectForKey:(NSString *)Identifier
{
    NSData* data = nil;
    if (data == nil) {
        data = [self _cacheDataFromServer:Identifier];
    }
    return data;
}

- (void)objectForKey:(NSString *)Identifier WithContext:(void*)context WithHandler:(RSCacheHandler)handler
{
    dispatch_queue_t queue = dispatch_queue_create("RSRemoteCache Update Data", NULL);
    dispatch_async(queue, ^{
        NSData* data = [self objectForKey:Identifier];
        dispatch_async(dispatch_get_main_queue(), ^{
            handler(context,data, RSCacheL2);
        });
    });
    
    //dispatch_release(queue);
    
    return;
}

- (void)removeObjectForKey:(NSString *)Identifier
{
    
}

- (void)removeAllObjects
{
    
}
@end

@implementation RSRemoteCache (RemoteFS)

- (NSURL *)cacheURLWithIdentifier:(NSString *)Identifier
{
    NSMutableString* atomiRSRLString = [[[self serverRoot]absoluteString] mutableCopy];
    [atomiRSRLString appendFormat:@"%@", Identifier];
    NSLog(@"RSRemoteCache - cacheURLWithIdentifier: URL String is %@",atomiRSRLString);
    return [NSURL URLWithString:atomiRSRLString];
}

@end