//
//  RSMemoryCache.h
//  RSKit
//
//  Created by RetVal on 10/6/12.
//  Copyright (c) 2012 RetVal. All rights reserved.
//

#import <Foundation/Foundation.h>
#import <RSKit/RSCacheProtocol.h>
@protocol RSMemoryCacheDelegate;
@interface RSMemoryCache : NSObject <RSCache>
/**
 the root identifier of the memory cache. Reserved.
 */
@property (readonly,nonatomic) NSString* rootIdentifier;
/**
 Create a cache object with default setting.
 @return RSMemoryCache successfully created cache object
 */
- (id)init;
/**
 Create a cache object with default setting and root identifier. Reserved.
 @param rootIdentifier the rootIdentifier is not uesd, just push nil.
 @return RSMemoryCache successfully created cache object
 */
- (id)initWithRootIdentifier:(NSString *) rootIdentifier;
/**
 Create a cache object with capacity.
 @param capacity the capacity of cache. Default as RSMemoryCacheDefaultCapacity.
 @return RSMemoryCache successfully created cache object
 */
- (id)initWithCapacity:(NSUInteger)capacity;
/**
 Create a cache object with capacity. Reserved.
 @param rootIdentifier the rootIdentifier is not uesd, just push nil. 
 @param capacity the capacity of cache. Default as RSMemoryCacheDefaultCapacity.
 @return RSMemoryCache successfully created cache object
 */
- (id)initWithRootIdentifier:(NSString *) rootIdentifier WithCapacity:(NSUInteger)capacity;
/**
 Get the capacity of cache.
 @return NSUInteger return the capacity of cache, default as RSMemoryCacheDefaultCapacity, failed return 0.
 */
- (NSUInteger)capacity;

/**
 Get the delegate of cache. 
 @return id<RSMemoryCacheDelegate> successfully return the delegate or nil if the delegate is not setted.
 */
- (id<RSMemoryCacheDelegate>)delegate;                      // get a delegate.
/**
 Set the delegate of cache. The delegate will be active when the cache evict object out of cache
 @param delegate the delegate of the cache.
 */
- (void)setDelegate:(id<RSMemoryCacheDelegate>)delegate;    // set a delegate.


@end
@interface RSMemoryCache (Creation)
/**
 RSMemoryCache Creation
 Create a default memory cache and return with autorelease.
 @return RSMemoryCache successfully create cache object or nil if failed
 */
+ (RSMemoryCache *)cache;   // create an cache autorelease automatically with default setting.
/**
 RSMemoryCache Creation 
 Create a memory cache with its capacity and return with autorelease.
 @param capacity the capacity of the cache.
 @return RSMemoryCache successfully create cache object or nil if failed
 */
+ (RSMemoryCache *)cacheWithCapacity:(NSUInteger)capacity; 

@end

@protocol RSMemoryCacheDelegate <NSObject>

@optional
- (void)cache:(RSMemoryCache *)cache willEvictObject:(id)obj;

- (void)cache:(RSMemoryCache *)cache missObjectForKey:(id)key;
@end

RSExtern NSString * const RSMemoryCacheEvictObjectNotification ;  // this notification will post if the object is evicted from cache.

RSExtern NSString * const RSMemoryCacheObjectKey ;    // the notification userInfo save the object which will be removed, the key is RSMemoryCacheObjectKey

RSExtern NSUInteger const RSMemoryCacheDefaultCapacity;