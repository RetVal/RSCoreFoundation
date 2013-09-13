//
//  RSCacheProtocol.h
//  RSKit
//
//  Created by RetVal on 10/7/12.
//  Copyright (c) 2012 RetVal. All rights reserved.
//

#import <Foundation/Foundation.h>
#import <RSKit/RSBase.h>

enum
{
    RSCacheL0Mask = 0,
    RSCacheL1Mask = 1,
    RSCacheL2Mask = 2
};
enum
{
    RSCacheSupportL0 = 1 << RSCacheL0Mask,
    RSCacheSupportL1 = 1 << RSCacheL1Mask,
    RSCacheSupportL2 = 1 << RSCacheL2Mask,
    RSCacheSupportL0L1 = RSCacheSupportL0 | RSCacheSupportL1,
    RSCacheSupportL0L2 = RSCacheSupportL0 | RSCacheSupportL2,
    RSCacheSupportL1L2 = RSCacheSupportL1 | RSCacheSupportL2,
    RSCacheSupportAll = RSCacheSupportL0 | RSCacheSupportL1 | RSCacheSupportL2,
    RSCacheDefaultSupport = RSCacheSupportAll
};
typedef RSUInteger RSCacheType;

enum
{
    RSCacheL0 = RSCacheSupportL0,
    RSCacheL1 = RSCacheSupportL1,
    RSCacheL2 = RSCacheSupportL2,
    RSCacheAllLevel = RSCacheL0 | RSCacheL1 | RSCacheL2
};
typedef RSUInteger RSCacheLevel;

enum
{
    RSCacheL0Success = RSCacheL0,
    RSCacheL1Success = RSCacheL1,
    RSCacheL2Success = RSCacheL2,
    RSCacheAllSuccess= RSCacheL0Success | RSCacheL1Success | RSCacheL2Success
};
typedef RSUInteger RSCacheResult;

//return by RSCache (Setting)
typedef RSUInteger RSCacheSetting;

/**
 RSCacheHandler is a block that use for RSCache.
 */
typedef BOOL(^RSCacheHandler)(void* context, id data, RSCacheLevel level);
/**
 RSCache is a protocol that conformed by RSMemoryCache, RSLocalCache, RSRemoteCache.
 Define the Key-Value-Coding protocol for the cache data getter and setter.
 Conforms the NSObject
 */
@protocol RSCache <NSObject>
@required
/**
 Return an object from cache with the Identifier, return nil if not existing.
 @param Identifier the Identifier of the object in the cache.
 @return id return the object for the identifier, return nil if not existing.
 */
- (id)objectForKey:(NSString *)Identifier;

/**
 Return immediately, when data finished, RSCache call this handler to do something.
 @param Identifier the Identifier of the object in the cache.
 @param context the context the handler needed.
 @Param handler the call back block will be called when the data is finished.
 */
- (void)objectForKey:(NSString *)Identifier WithContext:(void*)context WithHandler:(RSCacheHandler)handler;

/**
 Add the object to cache with its Identifier.
 @param obj the object will be stored in the cache.
 @param Identifier the Identifier of the object.
 */
- (void)setObject:(id)obj forKey:(NSString* )Identifier;

/**
 Remove an object from cache with Identifier.
 @param Identifier the Identifier of the object in the Cache.
 */
- (void)removeObjectForKey:(NSString *)Identifier;
/**
 Remove all objects in the cache.
 */
- (void)removeAllObjects;
@end


@protocol RSMultiLevelCache <NSObject>

@required
/**
 Add the object to cache with its Identifier.
 @param obj the object will be stored in the cache.
 @param Identifier the Identifier of the object.
 @param level set the object to which cache.
 @return RSCacheResult
 */
- (RSCacheResult)setObject:(id)obj forKey:(NSString*)Identifier toCacheLevel:(RSCacheLevel)level;
/**
 Return an object from cache with the Identifier, return nil if not existing.
 @param Identifier the Identifier of the object in the cache.
 @return id return the object for the identifier, return nil if not existing.
 */
- (id)objectForKey:(NSString*)Identifier;
/**
 Return immediately, when data finished, RSCache call this handler to do something.
 @param Identifier the Identifier of the object in the cache.
 @param context the context the handler needed.
 @Param handler the call back block will be called when the data is finished.
 */
- (void)objectForKey:(NSString *)Identifier WithContext:(void*)context WithHandler:(RSCacheHandler)handler;
/**
 Remove an object from cache with Identifier.
 @param Identifier the Identifier of the object in the Cache.
 @param level remove from which cache.
 */
- (void)removeObjectForKey:(NSString *)Identifier fromCache:(RSCacheLevel)level;
/**
 Remove all objects from cache.
 @param level remove from which cache.
 */
- (void)removeAllObjectsFromCache:(RSCacheLevel)level;

@end


