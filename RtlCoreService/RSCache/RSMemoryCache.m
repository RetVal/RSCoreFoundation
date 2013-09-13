//
//  RSMemoryCache.m
//  RSKit
//
//  Created by RetVal on 10/6/12.
//  Copyright (c) 2012 RetVal. All rights reserved.
//

#import "RSMemoryCache.h"

NSString * const RSMemoryCacheEvictObjectNotification = @"RSMemoryCacheEvictObjectNotification";
NSString * const RSMemoryCacheObjectKey = @"RSMemoryCacheObjectKey";
#if TARGET_OS_IPHONE
NSUInteger const RSMemoryCacheDefaultCapacity = 10;
#elif TARGET_OS_MAC
NSUInteger const RSMemoryCacheDefaultCapacity = 50;
#endif
@interface RSMemoryCache()<NSCacheDelegate>
{
    @private
    NSCache* _cache;
    id<RSMemoryCacheDelegate> _delegate;
}
@end

@implementation RSMemoryCache
- (id)init
{
    return [self initWithCapacity:RSMemoryCacheDefaultCapacity];
}

- (id)initWithCapacity:(NSUInteger)capacity
{
    return [self initWithRootIdentifier:nil WithCapacity:capacity];
}
- (id)initWithRootIdentifier:(NSString *)rootIdentifier
{
    return [self initWithRootIdentifier:rootIdentifier WithCapacity:RSMemoryCacheDefaultCapacity];
}

- (id)initWithRootIdentifier:(NSString *)rootIdentifier WithCapacity:(NSUInteger)capacity
{
    if (self = [super init]) {
        _cache = [[NSCache alloc] init];
        [_cache setDelegate:self];
        [_cache setCountLimit:capacity];
        _rootIdentifier = rootIdentifier;
    }
    return self;
}

- (NSUInteger)capacity
{
    return [_cache countLimit];
}

- (id<RSMemoryCacheDelegate>)delegate
{
    return _delegate;
}

- (void)setDelegate:(id<RSMemoryCacheDelegate>)delegate
{
    _delegate = delegate;
}

- (id)objectForKey:(NSString *)Identifier
{
    id obj = [_cache objectForKey:Identifier];
    if (nil == obj)
    {
        NSLog(@"RSMemoryCache - objectForKey: miss Object Identifier in cache (%@)",Identifier);
        if (_delegate && ([_delegate respondsToSelector:@selector(cache:missObjectForKey:)] == YES)) {
            [_delegate cache:self missObjectForKey:Identifier];
        }
    }
    else{
        NSLog(@"RSMemoryCache - objectForKey: return (%@)",Identifier);
    }
    return obj;
}

- (void)objectForKey:(NSString *)Identifier WithContext:(void *)context WithHandler:(RSCacheHandler)handler
{
    id obj = [self objectForKey:Identifier];
    if (obj && handler) {
        handler(context,obj,RSCacheL0);
    }
}

- (void)setObject:(id)obj forKey:(id)key
{
    NSLog(@"RSMemoryCache - setObject:forKey: save (%@)",key);
    return [_cache setObject:obj forKey:key];
}

- (void)removeObjectForKey:(id)key
{
    return [_cache removeObjectForKey:key];
}

- (void)removeAllObjects
{
    return [_cache removeAllObjects];
}

- (void)dealloc
{
    _cache = nil;
    _delegate = nil;
}

- (void)cache:(NSCache *)cache willEvictObject:(id)obj
{
    if ([_delegate respondsToSelector:@selector(cache:willEvictObject:)]) {
        [_delegate cache:self willEvictObject:obj];
    }
    NSMutableDictionary* info = [[NSMutableDictionary alloc] initWithCapacity:1];
    [info setObject:obj forKey:RSMemoryCacheObjectKey];
    [[NSNotificationCenter defaultCenter] postNotificationName:RSMemoryCacheEvictObjectNotification object:self userInfo:info];
}
@end

@implementation RSMemoryCache (Creation)
+ (RSMemoryCache *)cache
{
    return [[RSMemoryCache alloc] init];
}

+ (RSMemoryCache *)cacheWithCapacity:(NSUInteger)capacity
{
    return [[RSMemoryCache alloc] initWithCapacity:capacity];
}
@end
