//
//  RSRemoteCache.h
//  RSKit
//
//  Created by RetVal on 10/7/12.
//  Copyright (c) 2012 RetVal. All rights reserved.
//

#import <Foundation/Foundation.h>
#import <RSKit/RSCacheProtocol.h>
@interface RSRemoteCache : NSObject <RSCache>
/**
 RSRemoteCache current server root URL.
 */
@property (retain,readonly,nonatomic) NSURL* serverRoot;
/**
 RSRemoteCache current server cache name. Reserved.
 */
@property (retain,nonatomic) NSString* cacheName;
/**
 Copy a cache object.
 @param aCache copy from this cache, only copy the setting.
 @return RSRemoteCache successfully created cache object or nil if failed
 */
- (id)initWithCache:(RSRemoteCache*)aCache;

/**
 Create a cache object.
 @param aURL the server root URL.
 @return RSRemoteCache successfully created cache object or nil if failed
 */
- (id)initWithContentsOfURL:(NSURL*)aURL;

@end

@interface RSRemoteCache (RemoteFS)
/**
 Get the URL information with object's identifier.
 @param Identifier the identifier of object in cache.
 @return NSURL successfully return a NSURL object or nil if failed
 */
- (NSURL *)cacheURLWithIdentifier:(NSString *)Identifier;
@end