//
//  RSCache.h
//  RSKit
//
//  Created by RetVal on 10/7/12.
//  Copyright (c) 2012 RetVal. All rights reserved.
//

#import <Foundation/Foundation.h>
#import <RSKit/RSCacheProtocol.h>
@class RSMemoryCache, RSLocalCache, RSRemoteCache;
@protocol RSMemoryCacheDelegate;
/**
 RSCache is an object that have 3 level caches,restore data from memory, file system and network automatically for using.
 
 Conforms the RSMultiLevelCache
 */
/**
 Example:
 <pre>
 NSString* const kRtlRemoteURLKey = @"RtlRemoteURL";    //write the real URL to your info.plist file.
 NSString* const kRtlRemoteCacheURLKey = @"RtlRemoteCacheURL";
 NSString* const kRtlLocalCacheRoot = @"~/Desktop";
 NSString* const kRtlLocalCacheConfiguration = @"~/Desktop/RSCache";
 #if !RSCacheUseConfiguration
 // App first use the cache, run this block.
 NSString* urlString = [[[NSBundle bundleForClass:[self class]] infoDictionary] objectForKey:kRtlRemoteCacheURLKey];
 NSString* rootString = [kRtlLocalCacheRoot stringByStandardizingPath];
 RSCache* cache = [[RSCache alloc] init];
 [cache setCacheRootPath:rootString named:nil];
 [cache setCacheRootURL:[NSURL URLWithString:urlString]];
 [cache setCapacity:40];
 [cache setConfigurationSupport:YES];
 #else
 // cache is existing, run this block.
 RSCache* cache = [[RSCache alloc] initWithConfigurationOfFile:[kRtlLocalCacheConfiguration stringByStandardizingPath]];
 #endif
 NSData* data = [cache objectForKey:@"/yifa/0001.jpg"];
 NSImage* image = [[NSImage alloc] initWithData:data];
 [self.mainImageView setImage:image];
 
 data = [cache objectForKey:@"/aojo/0003.jpg"];
 image = [[NSImage alloc] initWithData:data];
 [self.mainImageView setImage:image];
 data = [cache objectForKey:@"/yifa/0001.jpg"];
 image = [[NSImage alloc] initWithData:data];
 [self.mainImageView setImage:image];
 
 [cache objectForKey:@"/miku/0002.jpg" WithContext:nil WithHandler:^BOOL(void *context, id data) {
    NSImage* _image = [[NSImage alloc] initWithData:data];
    [self.mainImageView setImage:_image];
    return YES;
 }];
 </pre>
 */
@interface RSCache : NSObject<RSMultiLevelCache>
{
    @private
    RSMemoryCache*  _cacheL0;
    RSLocalCache*   _cacheL1;
    RSRemoteCache*  _cacheL2;
    RSCacheType  _supportCache;
    RSCacheSetting _setting;
}
/**
 RSCache support cache type.
 */
@property (assign, nonatomic, readonly) RSCacheType supportCache;
/**
 RSCache current setting. Use RSCache (Setting) method to add or remove an new setting.
 */
@property (assign, readwrite) RSCacheSetting setting;
#pragma mark -
#pragma mark Initialize
/**
 Designated initializer
 Creates a cache object, all settings are as default.
 @return RSCache successfully created cache object
 */
- (id)init;
/**
 Designated initializer
 Creates a cache object.
 @param supportCache the cache type of the cache supported.
 @return RSCache successfully created cache object
 */
- (id)initWithType:(RSCacheType)supportCache;   //default setting as [RSCache defaultSetting]
/**
 Designated initializer
 Creates a cache object.
 @param supportCache the cache type of the cache supported.
 @param setting the setting of the cache.
 @param capacity the capacity of L0.
 @param aDelegate the delegate of the L0.
 @return RSCache successfully created cache object
 */
- (id)initWithType:(RSCacheType)supportCache WithSetting:(RSCacheSetting)setting WithCapacity:(RSUInteger)capacity WithDelegate:(id<RSMemoryCacheDelegate>)aDelegate;
/**
 Designated initializer
 Creates a cache object.
 @param supportCache the cache type of the cache supported.
 @param setting the setting of the cache.
 @param capacity the capacity of L0.
 @param aDelegate the delegate of the L0.
 @param rootPath the root path of the L1, named default as RSCache.
 @return RSCache successfully created cache object
 */
- (id)initWithType:(RSCacheType)supportCache WithSetting:(RSCacheSetting)setting WithCapacity:(RSUInteger)capacity WithDelegate:(id<RSMemoryCacheDelegate>)aDelegate atRootPath:(NSString *)rootPath;
/**
 Designated initializer
 Creates a cache object.
 @param supportCache the cache type of the cache supported.
 @param setting the setting of the cache.
 @param capacity the capacity of L0.
 @param aDelegate the delegate of the L0.
 @param rootPath the root path of the L1, named default as RSCache.
 @param rootURL the root URL of the L2.
 @return RSCache successfully created cache object
 */
- (id)initWithType:(RSCacheType)supportCache WithSetting:(RSCacheSetting)setting WithCapacity:(RSUInteger)capacity WithDelegate:(id<RSMemoryCacheDelegate>)aDelegate atRootPath:(NSString *)rootPath atRootURL:(NSURL *)rootURL;
#pragma mark -
#pragma mark L0 
/**
 Set the capacity of the L0, data in L0 will lose after set the capacity.
 @param capacity the cacheL0 capacity, default as RSMmeoryCacheDefaultCapacity.
 */
- (void)setCapacity:(RSUInteger)capacity;
/**
 Get the capacity of the L0, if L0 is not available, return 0.
 @return return the capacity of the L0.
 */
- (RSUInteger)capacity;
/**
 Get the delegate of the L0, if cache do not set the delegate, default as nil.
 @return return the RSMemoryCacheDelegate of the L0.
 */

- (id<RSMemoryCacheDelegate>)delegate;

/**
 Set the delegate of the L0.
 @param aDelegate the delegate for the cacheL0, if nil, the delegate of cacheL0 will be reset.
 */
- (void)setDelegate:(id<RSMemoryCacheDelegate>)aDelegate;
#pragma mark -
#pragma mark L1
/**
 Set the root of L1 before use, otherwise L1 is not available.
 @param rootPath the root path of the L1.
 @param name the name of the L1.
 */
- (void)setCacheRootPath:(NSString *)rootPath named:(NSString *)name;
#pragma mark -
#pragma mark L2 
/**
 Set the root of L2 before use, otherwise L2 is not available.
 @param rootURL the root URL of the L2.
 */
- (void)setCacheRootURL:(NSURL*)rootURL;
#pragma mark -
#pragma mark RSCache Protocol EX
/**
 Set the root identifier of cacheL0/L1/L2. Reserved, do not call it directly.
 @param rootIdentifier the root of the L0/L1/L2. If for L0, will be ignored. If for L1, is a root path. If for L2, is a root URL.
 @param type which cache should be set.
 */
- (void)setRootIdentifier:(NSString*)rootIdentifier WithCache:(RSCacheType)type;    // not supported with L0,L1,L2.
@end
#pragma mark -
#pragma mark Setting

@interface RSCache (Setting)
/**
 Get the setting value of automatically cache data to all cahche.
 @return RSCacheSetting
 */
+ (RSCacheSetting)autoCacheAll;     // available to L0,L1.
/**
 Get the setting value of automatically cache data to only L0.
 @return RSCacheSetting
 */
+ (RSCacheSetting)autoCacheOnlyL0;
/**
 Get the setting value of automatically cache data to only L1.
 @return RSCacheSetting
 */
+ (RSCacheSetting)autoCacheOnlyL1;
/**
 Get the setting value of automatically cache data to only L2.
 @return RSCacheSetting
 */
+ (RSCacheSetting)autoCacheOnlyL2;
/**
 Get the setting value of automatically cache data to all without L2.
 @return RSCacheSetting
 */
+ (RSCacheSetting)autoCacheL0AndL1; 
/**
 Get the setting value of automatically cache data to all without L1. Reserved.
 @return RSCacheSetting
 */
+ (RSCacheSetting)autoCacheL0AndL2;
/**
 Get the setting value of automatically cache data to all without L0. Reserved.
 @return RSCacheSetting
 */
+ (RSCacheSetting)autoCacheL1AndL2;
/**
 Get the setting value of resetting L0 when dealloc. Always do it.
 @return RSCacheSetting
 */
+ (RSCacheSetting)autoCleanL0;
/**
 Get the setting value of resetting L1 when dealloc. The cache files on the local file system will be cleared.
 @return RSCacheSetting
 */
+ (RSCacheSetting)autoCleanL1;
/**
 Get the setting value of resetting L1 when dealloc. Reserved.
 @return RSCacheSetting
 */
+ (RSCacheSetting)autoCleanL2;
/**
 Get the setting value of default setting.
 Include the autoCleanL0, autoCacheL0AndL1.
 @return RSCacheSetting
 */
+ (RSCacheSetting)defaultSetting;
/**
 Get the setting value of supportting the configuration with cache.
 Use setConfigurationSupport: directly.
 @return RSCacheSetting
 */

+ (RSCacheSetting)configurationSupport;
/**
 Add the new setting to the cache.
 @param setting the setting value should be add to the cache.
 */
- (void)addCacheSetting:(RSCacheSetting)setting;
/**
 Remove the setting from the cache.
 @param setting the setting value will be removed from the cache.
 */
- (void)removeCacheSetting:(RSCacheSetting)setting;
@end

@interface RSCache (Creation)
/**
 Designated Creation
 Create a cache object.
 @param setting the setting of the cache.
 @param capacity the capacity of L0.
 @param aDelegate the delegate of L0.
 @param rootPath the root path of L1.
 @param rootURL the root URL of L2.
 @return RSCache successfully created cache object with autorelease.
 */
+ (RSCache *)cacheAllSupportWithSetting:(RSCacheSetting)setting WithCapacity:(RSUInteger)capacity WithDelegate:(id<RSMemoryCacheDelegate>)aDelegate atRootPath:(NSString *)rootPath atRootURL:(NSURL *)rootURL ;
/**
 Designated Creation
 Create a cache object.
 @param configuration the configuration of the cache, the configuration can be returned by [RSCache configurationWithL0Capacity:atRoot:named:atRootURL:].
 @return RSCache successfully created cache object with autorelease.
 */
+ (RSCache *)cacheWithConfiguration:(NSDictionary *)configuration;
/**
 Designated Creation
 @param path the path of configuration file, default as a plist file.
 @return RSCache successfully created cache object with autorelease.
 */
+ (RSCache *)cacheWithConfigurationOfFile:(NSString *)path;
@end

@interface RSCache (ConfigurationSupport)
/**
 Designated initializer
 Creates a cache object, all settings are retored from configuration.
 @param configuration the configuration of the cache, the configuration can be returned by [RSCache configurationWithL0Capacity:atRoot:named:atRootURL:].
 @return RSCache successfully created cache object.
 */
- (id)initWithConfiguration:(NSDictionary *)configuration;
/**
 Designated Creation
 @param path the path of configuration file, default as a plist file.
 @return RSCache successfully created cache object autorelease.
 */
- (id)initWithConfigurationOfFile:(NSString *)path;
/**
 Set if the cache is support the configuration or not. If support the configuration feature, you can reuse the cache by the configuration, and need not to set anything.
 @param support the path of configuration file, default as a plist file.
 @return RSCache successfully created cache object autorelease.
 */
- (void)setConfigurationSupport:(BOOL)support;
/**
 If the cache support configuration now.
 @return BOOL support configuration return YES, otherwise return NO.
 */
- (BOOL)supportConfiguration;    // default as NO
/**
 Save current configuration to file system. If support configuration, will be called automatically when cache dealloc.
 */
- (void)saveConfiguration;
/**
 Designated Creation
 @param capacityL0 the capacity of cacheL0.
 @param rootL1 the root path of cacheL1.
 @param nameL1 the name of cacheL1.
 @param rootL2 the root URL of cacheL2.
 @return NSDictionary successfully return the configuration or nil if failed.
 */
+ (NSDictionary*)configurationWithL0Capacity:(RSUInteger)capacityL0
                                      atRoot:(NSString *)rootL1
                                       named:(NSString *)nameL1
                                   atRootURL:(NSURL *)rootL2;
@end
RSExtern RSUInteger const RSCacheKeyNumber ;
RSExtern NSString* const RSLocalCacheDefaultName ;
RSExtern NSString* const RSLocalCacheDefaultConfigurationName ;
//--------------------------------------------------------------------------------
// L0
RSExtern NSString* const RSCacheL0Launchload ;        //NSArray (Identifiers)
RSExtern NSString* const RSCacheL0Capacity ;          //NSNumber
// L1
RSExtern NSString* const RSCacheL1Root ;              //NSString
RSExtern NSString* const RSCacheL1Name ;              //NSString
RSExtern NSString* const RSCacheL1StoreLimitSize ;    //NSNumber
RSExtern NSString* const RSCacheL1Capacity ;          //NSNumber
// L2
RSExtern NSString* const RSCacheL2Root ;

// Setting
RSExtern NSString* const RSCacheSettings ;            //NSNumber
RSExtern NSString* const RSCacheSupport ;             //NSNumber
//--------------------------------------------------------------------------------