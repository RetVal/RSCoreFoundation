//
//  RSLocalCache.h
//  RSKit
//
//  Created by RetVal on 10/2/12.
//  Copyright (c) 2012 RetVal. All rights reserved.
//

#import <Foundation/Foundation.h>
#import <RSKit/RSCacheProtocol.h>
//RSLocalCacheLayer
//RSLocalCache(Root)
/*-Purpose
    The Caller get the all the data from RSLocalCache.
    RSLocalCache can store data on the local file system from server automatically,
    and load the data from local cache if the data is already existing on the local file system automatically.
    All the data has an URL and local identifier.
 */
/*
 Cache on the FS 
 ../Cache/  ---- + (root)
        
 
 */
//enum
//{
//    kRSCacheReadablyType = 1,
//    kRSCacheBinaryType = 2,
//    kRSCacheImageType = 3,
//    kRSCacheConfigurationType = 4,
//    kRSCachePrivateConfigurationType = 5
//};
//typedef NSUInteger RSLocalCacheFileType;
@interface RSLocalCache : NSObject <RSCache>
/**
 RSLocalCache current cache root.
 */
@property (retain,readonly,nonatomic) NSString* cacheRoot;
/**
 RSLocalCache current cache name. Default as "RSCache".
 */
@property (retain,nonatomic) NSString* cacheName;    
/**
 Copy a cache object.
 @param aCache copy from this cache, only copy the setting.
 @return RSLocalCache successfully created cache object
 */
- (id)initWithCache:(RSLocalCache*)aCache;
/**
 Creates a cache object. (not supported)
 @param aURL the URL of the remote cache.
 @return RSLocalCache successfully created cache object
 */
- (id)initWithContentsOfURL:(NSURL*)aURL UNAVAILABLE_ATTRIBUTE;
/**
 Creates a cache object.
 @param aPath the root path of the cache.
 @param cacheName the name of the caceh.
 @return RSLocalCache successfully created cache object
 */
- (id)initWithContentsOfPath:(NSString*)aPath named:(NSString*)cacheName;

/**
 Creates a cache object that support on local filesystem and internet. If you want to get the support of internet, use RSRemoteCache instead.
 @param aPath the root path of the cache.
 @param aServerPath an content of URL String.
 @return RSLocalCache successfully created cache object
 */
- (id)initWithContentsOfPath:(NSString*)aPath AndServerPath:(NSString*)aServerPath UNAVAILABLE_ATTRIBUTE;
@end

@interface RSLocalCache (FileSystem)
/**
 Build the path for the identifier in cache.
 @param Identifier the identifier of object.
 */
- (void)buildPathWithIdentifier:(NSString*)Identifier;
/**
 Get the real local path of the object with Identifier.
 @param Identifier the identifier of object.
 @return NSString successfully return a local path for the object.
 */
- (NSString*)cachePathWithIdentifier:(NSString*)Identifier;
@end

RSExtern NSString* const RSLocalCacheDefaultName ;