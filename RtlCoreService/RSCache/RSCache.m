//
//  RSCache.m
//  RSKit
//
//  Created by RetVal on 10/7/12.
//  Copyright (c) 2012 RetVal. All rights reserved.
//

#import "RSCache.h"
#import "RSMemoryCache.h"
#import "RSLocalCache.h"
#import "RSRemoteCache.h"
enum
{
    RSCacheAutoSaveToL0 = 1 << 0,
    RSCacheAutoSaveToL1 = 1 << 1,
    RSCacheAutoSaveToL2 = 1 << 2,
    RSCacheAutoResetL0   = 0 << 3,
    RSCacheAutoResetL1   = 1 << 4,
    RSCacheAutoResetL2   = 0 << 5,
    RSCacheConfiguration = 1 << 6
    
};
NSUInteger const RSCacheKeyNumber = 9;
NSString* const RSLocalCacheDefaultName = @"RSCache";
NSString* const RSLocalCacheDefaultConfigurationName = @"RSCache.plist";
// L0
NSString* const RSCacheL0Launchload = @"RSCacheL0Launchload";       //NSArray (Identifiers)
NSString* const RSCacheL0Capacity = @"RSCacheL0Cacpcity";           //NSNumber
// L1
NSString* const RSCacheL1Root = @"RSCacheL1Root";                   //NSString
NSString* const RSCacheL1Name = @"RSCacheL1Name";                   //NSString
NSString* const RSCacheL1StoreLimitSize = @"RSCacheStoreLimitSize"; //NSNumber
NSString* const RSCacheL1Capacity = @"RSCacheL1Capacity";           //NSNumber
// L2
NSString* const RSCacheL2Root = @"RSCacheL2Root";                   //NSString

// Setting
NSString* const RSCacheSettings = @"RSCacheSettings";               //NSNumber
NSString* const RSCacheSupport = @"RSCacheSupport";                 //NSNumber


@interface RSCache()
{
    NSMutableDictionary* _configuration;
    BOOL _supportConfiguration;
}

@end
@interface RSCache (Private)
- (BOOL)_isL0Supported;
- (BOOL)_isL1Supported;
- (BOOL)_isL2Supported;
- (BOOL)_isXSupported:(RSCacheType)type X:(RSCacheType)x;

- (BOOL)_isAutoCacheL0;
- (BOOL)_isAutoCacheL1;
- (BOOL)_isAutoCacheL2;

- (BOOL)_isAutoCacheL0L1;
- (BOOL)_isAutoCacheL0L2;
- (BOOL)_isAutoCacheL1L2;

@end

@interface RSCache (Configuration)
- (BOOL)_loadConfiguration;
- (void)_flushConfiguration;
- (BOOL)_saveConfiguration;
+ (NSMutableDictionary*)_defaultConfiguration;
@end


@implementation RSCache (Private)

- (BOOL)_isL0Supported
{
    return _supportCache & RSCacheSupportL0;
}

- (BOOL)_isL1Supported
{
    return _supportCache & RSCacheSupportL1;
}

- (BOOL)_isL2Supported
{
    return _supportCache & RSCacheSupportL2;
}

- (BOOL)_isXSupported:(RSCacheType)type X:(RSCacheType)x
{
    return x & type;
}

- (BOOL)_isAutoCacheL0
{
    return _setting & RSCacheAutoSaveToL0;
}

- (BOOL)_isAutoCacheL1
{
    return _setting & RSCacheAutoSaveToL1;
}

- (BOOL)_isAutoCacheL2
{
    return _setting & RSCacheAutoSaveToL2;
}

- (BOOL)_isAutoCacheL0L1
{
    return [self _isAutoCacheL0] & [self _isAutoCacheL1];
}

- (BOOL)_isAutoCacheL0L2
{
    return [self _isAutoCacheL0] & [self _isAutoCacheL2];
}

- (BOOL)_isAutoCacheL1L2
{
    return [self _isAutoCacheL1] & [self _isAutoCacheL2];
}
@end




@implementation RSCache
- (id)init
{
    return [self initWithType:RSCacheDefaultSupport];
}

- (id)initWithType:(RSCacheType)supportCache
{
    return [self initWithType:supportCache WithSetting:[RSCache defaultSetting] WithCapacity:RSMemoryCacheDefaultCapacity WithDelegate:nil];
}

- (id)initWithType:(RSCacheType)supportCache WithSetting:(RSCacheSetting)setting WithCapacity:(NSUInteger)capacity WithDelegate:(id<RSMemoryCacheDelegate>)aDelegate
{
    return [self initWithType:supportCache WithSetting:setting WithCapacity:capacity WithDelegate:aDelegate atRootPath:nil];
}
- (id)initWithType:(RSCacheType)supportCache WithSetting:(RSCacheSetting)setting WithCapacity:(NSUInteger)capacity WithDelegate:(id<RSMemoryCacheDelegate>)aDelegate atRootPath:(NSString *)rootPath
{
    return [self initWithType:supportCache WithSetting:setting WithCapacity:capacity WithDelegate:aDelegate atRootPath:rootPath atRootURL:nil];
}

- (id)initWithType:(RSCacheType)supportCache WithSetting:(RSCacheSetting)setting WithCapacity:(NSUInteger)capacity WithDelegate:(id<RSMemoryCacheDelegate>)aDelegate atRootPath:(NSString *)rootPath atRootURL:(NSURL *)rootURL 
{
    if (self = [super init]) {
        _supportCache = supportCache;
        [self setSetting:setting];
        [self setCapacity:capacity];
        [self setDelegate:aDelegate];
        [self setCacheRootPath:rootPath named:nil];
        [self setCacheRootURL:rootURL];
        
    }
    return self;
}

#pragma mark -
#pragma mark L0
- (void)setCapacity:(NSUInteger)capacity
{
    if ([self _isL0Supported] && capacity) {
        [_cacheL0 removeAllObjects];
        _cacheL0 = nil;
        if (_cacheL0 == nil) {
            _cacheL0 = [[RSMemoryCache alloc] initWithCapacity:capacity];
        }
    }
}
- (NSUInteger)capacity
{
    return [_cacheL0 capacity];
}
- (id<RSMemoryCacheDelegate>)delegate
{
    return [_cacheL0 delegate];
}
- (void)setDelegate:(id<RSMemoryCacheDelegate>)aDelegate
{
    [_cacheL0 setDelegate:aDelegate];
}

#pragma mark -
#pragma mark L1
- (void)setCacheRootPath:(NSString *)rootPath named:(NSString *)name
{
    if ([self _isL1Supported] && rootPath) {
        if (_cacheL1 == nil) {
            _cacheL1 = [[RSLocalCache alloc] initWithContentsOfPath:rootPath named:RSLocalCacheDefaultName];
        }
    }
}
#pragma mark -
#pragma mark L2
- (void)setCacheRootURL:(NSURL *)rootURL
{
    if ([self _isL2Supported] && rootURL) {
        if (_cacheL2 == nil) {
            _cacheL2 = [[RSRemoteCache alloc] initWithContentsOfURL:rootURL];
        }
    }
}
#pragma mark -
- (void)setRootIdentifier:(NSString *)rootIdentifier WithCache:(RSCacheType)type
{
    if ([self _isXSupported:type X:RSCacheSupportL0]) {
        NSLog(@"L0 is not supported root Identifier");
    }
    if ([self _isXSupported:type X:RSCacheSupportL1]) {
        NSLog(@"L1 is not supported root Identifier");
    }
    if ([self _isXSupported:type X:RSCacheSupportL2]) {
        NSLog(@"L2 is not supported root Identifier");
    }
    
}

- (RSCacheResult)setObject:(id)obj forKey:(NSString*)Identifier toCacheLevel:(RSCacheLevel)level
{
    RSCacheResult result = 0;
    if ([self _isXSupported:level X:RSCacheL0]) {
        if (_cacheL0) {
            [_cacheL0 setObject:obj forKey:Identifier];
            result |= RSCacheL0Success;
        }
    }
    if ([self _isXSupported:level X:RSCacheL1]) {
        if (_cacheL1) {
            [_cacheL1 setObject:obj forKey:Identifier];
            result |= RSCacheL1Success;
        }
    }
    if ([self _isXSupported:level X:RSCacheL2]) {
        if (_cacheL2) {
            [_cacheL2 setObject:obj forKey:Identifier];
            result |= RSCacheL2Success;
        }
    }
    return result;
}

- (id)objectForKey:(NSString *)Identifier
{
    NSData* data = nil;
    if (_cacheL0) {
        data = [_cacheL0 objectForKey:Identifier];
        if (data) {
            return data;
        }
    }
    if (_cacheL1) {
        data = [_cacheL1 objectForKey:Identifier];
        if (data) {
            if ([self _isAutoCacheL0]) {
                [_cacheL0 setObject:data forKey:Identifier];
            }
            return data;
        }
    }
    if (_cacheL2) {
        data = [_cacheL2 objectForKey:Identifier];
        if (data) {
            if ([self _isAutoCacheL0]) {
                [_cacheL0 setObject:data forKey:Identifier];
            }
            if ([self _isAutoCacheL1]) {
                [_cacheL1 setObject:data forKey:Identifier];
            }
            return data;
        }
    }
    return data;
}

- (void)objectForKey:(NSString *)Identifier WithContext:(void*)context WithHandler:(RSCacheHandler)handler
{
    NSData* data = nil;
    if (_cacheL0) {
        data = [_cacheL0 objectForKey:Identifier];
        if (data) {
            handler(context,data,RSCacheL0);
        }
        return;
    }
    if (_cacheL1) {
        data = [_cacheL1 objectForKey:Identifier];
        if (data) {
            handler(context,data,RSCacheL0);
        }
        return;
    }
    if (_cacheL2) {
        [_cacheL2 objectForKey:Identifier WithContext:context WithHandler:handler];
    }
    return ;
}
- (void)removeObjectForKey:(NSString *)Identifier fromCache:(RSCacheLevel)level
{
    if (((level & _supportCache) & RSCacheL0) && _cacheL0) {
        [_cacheL0 removeObjectForKey:Identifier];
    }
    if (((level & _supportCache) & RSCacheL1) && _cacheL1) {
        [_cacheL1 removeObjectForKey:Identifier];
    }
    if (((level & _supportCache) & RSCacheL2) && _cacheL2) {
        [_cacheL2 removeObjectForKey:Identifier];
    }
}
- (void)removeAllObjectsFromCache:(RSCacheLevel)level
{
    if (((level & _supportCache) & RSCacheL0) && _cacheL0) {
        [_cacheL0 removeAllObjects];
    }
    if (((level & _supportCache) & RSCacheL1) && _cacheL1) {
        [_cacheL1 removeAllObjects];
    }
    if (((level & _supportCache) & RSCacheL2) && _cacheL2) {
        [_cacheL2 removeAllObjects];
    }
}
#pragma mark -

- (void)dealloc
{
    if (_cacheL1 && (_setting & RSCacheAutoResetL1))
    {
        [_cacheL1 removeAllObjects];
    }
    [self _saveConfiguration];
    _cacheL0 = nil;
    _cacheL1 = nil;
    _cacheL2 = nil;
    _setting = 0;
    _supportCache = 0;
    _configuration = nil;
}
@end

@implementation RSCache (Setting)
+ (RSCacheSetting)autoCacheOnlyL0
{
    return RSCacheAutoSaveToL0;
}

+ (RSCacheSetting)autoCacheOnlyL1
{
    return RSCacheAutoSaveToL1;
}

+ (RSCacheSetting)autoCacheOnlyL2
{
    return RSCacheAutoSaveToL2;
}

+ (RSCacheSetting)autoCacheL0AndL1
{
    return [RSCache autoCacheOnlyL0] | [RSCache autoCacheOnlyL1];
}

+ (RSCacheSetting)autoCacheL0AndL2
{
    return [RSCache autoCacheOnlyL0] | [RSCache autoCacheOnlyL2];
}

+ (RSCacheSetting)autoCacheL1AndL2
{
    return [RSCache autoCacheOnlyL1] | [RSCache autoCacheOnlyL2];
}


+ (RSCacheSetting)autoCacheAll
{
    return [RSCache autoCacheL0AndL1] | [RSCache autoCacheOnlyL2];
}
+ (RSCacheSetting)autoCleanL0
{
    return RSCacheAutoResetL0;
}

+ (RSCacheSetting)autoCleanL1
{
    return RSCacheAutoResetL1;
}

+ (RSCacheSetting)autoCleanL2
{
    return RSCacheAutoResetL2;
}

+ (RSCacheSetting)defaultSetting
{
    return [RSCache autoCacheL0AndL1];
}

- (void)addCacheSetting:(RSCacheSetting)setting
{
    _setting |= setting;
}

- (void)removeCacheSetting:(RSCacheSetting)setting
{
    _setting &= ~setting;
}

+ (RSCacheSetting)configurationSupport
{
    return RSCacheConfiguration;
}
@end

@implementation RSCache (Creation)

+ (RSCache *)cacheAllSupportWithSetting:(RSCacheSetting)setting WithCapacity:(NSUInteger)capacity WithDelegate:(id<RSMemoryCacheDelegate>)aDelegate atRootPath:(NSString *)rootPath atRootURL:(NSURL *)rootURL
{
    return [[RSCache alloc] initWithType:RSCacheSupportAll WithSetting:setting WithCapacity:capacity WithDelegate:aDelegate atRootPath:rootPath atRootURL:rootURL];
}

+ (RSCache *)cacheWithConfiguration:(NSDictionary *)configuration
{
    return [[RSCache alloc] initWithConfiguration:configuration];
}

+ (RSCache *)cacheWithConfigurationOfFile:(NSString *)path
{
    return [[RSCache alloc] initWithConfigurationOfFile:path];
}
@end

@implementation RSCache (Configuration)

- (BOOL)_loadConfiguration
{
    BOOL result = YES;
    if (_cacheL1) {
        NSMutableString* path = [_cacheL1.cacheRoot mutableCopy];
        [path appendFormat:@"/%@",RSLocalCacheDefaultConfigurationName];
        _configuration = [[NSFileManager defaultManager] fileExistsAtPath:path] ? ([[NSMutableDictionary alloc] initWithContentsOfFile:path]) : ([RSCache _defaultConfiguration]);
        [self _flushConfiguration];
    }
    return result;
}

- (void)_flushConfiguration
{
    if (_configuration == nil) {
        [self _loadConfiguration];
    }
    if (_configuration) {
        if (_cacheL0) {
            [_configuration setObject:[NSNumber numberWithUnsignedInteger:[_cacheL0 capacity]] forKey:RSCacheL0Capacity];
            [_configuration setObject:[NSArray arrayWithArray:nil] forKey:RSCacheL0Launchload];
        }
        if (_cacheL1) {
            [_configuration setObject:_cacheL1.cacheName forKey:RSCacheL1Name];
            [_configuration setObject:_cacheL1.cacheRoot forKey:RSCacheL1Root];
        }
        if (_cacheL2) {
            [_configuration setObject:[_cacheL2.serverRoot absoluteString] forKey:RSCacheL2Root];
        }
        [_configuration setObject:[NSNumber numberWithUnsignedInteger:_setting] forKey:RSCacheSettings];
        [_configuration setObject:[NSNumber numberWithUnsignedInteger:_supportCache] forKey:RSCacheSupport];
    }
}

+ (NSMutableDictionary *)_defaultConfiguration
{
    static NSMutableDictionary* configuration = nil;
    if (configuration == nil) {
        configuration = [[NSMutableDictionary alloc] initWithCapacity:RSCacheKeyNumber];
        [configuration setObject:[NSNumber numberWithUnsignedInteger:[RSCache defaultSetting]|[RSCache configurationSupport]] forKey:RSCacheSettings];
        [configuration setObject:[NSNumber numberWithUnsignedInteger:RSCacheDefaultSupport] forKey:RSCacheSupport];
        [configuration setObject:[NSArray arrayWithObject:@""] forKey:RSCacheL0Launchload];
        [configuration setObject:[NSNumber numberWithInteger:RSMemoryCacheDefaultCapacity] forKey:RSCacheL0Capacity];
        [configuration setObject:@"" forKey:RSCacheL1Root];
        [configuration setObject:RSLocalCacheDefaultName forKey:RSCacheL1Name];
        [configuration setObject:[NSNumber numberWithInteger:-1] forKey:RSCacheL1StoreLimitSize];
        [configuration setObject:[NSNumber numberWithInteger:-1] forKey:RSCacheL1Capacity];
        [configuration setObject:@"" forKey:RSCacheL2Root];
    }
    return configuration;
}

- (BOOL)_saveConfiguration
{
    NSMutableString* path = [_cacheL1.cacheRoot mutableCopy];
    [path appendFormat:@"/%@/%@",_cacheL1.cacheName,RSLocalCacheDefaultConfigurationName];
    [self _flushConfiguration];
    BOOL result = [_configuration writeToFile:path atomically:YES];
    return result;
}

- (void)_settingWithConfiguration:(NSDictionary* )configuration
{
    if (configuration) {
        _setting = [[configuration objectForKey:RSCacheSettings] unsignedIntegerValue];
        _supportConfiguration = _setting & [RSCache configurationSupport];
        _supportCache = [[configuration objectForKey:RSCacheSupport] unsignedIntegerValue];
        if ([self _isL0Supported]) {
            NSUInteger capacity = [[configuration objectForKey:RSCacheL0Capacity] unsignedIntegerValue];
            _cacheL0 = [[RSMemoryCache alloc] initWithCapacity:capacity];
        }
        if ([self _isL1Supported]) {
            NSString* rootL1 = [configuration objectForKey:RSCacheL1Root];
            NSString* nameL1 = [configuration objectForKey:RSCacheL1Name];
            if (rootL1 && nameL1) {
                _cacheL1 = [[RSLocalCache alloc] initWithContentsOfPath:rootL1 named:nameL1];
            }
        }
        if ([self _isL2Supported]) {
            NSString* rootL2 = [configuration objectForKey:RSCacheL2Root];
            if (rootL2) {
                _cacheL2 = [[RSRemoteCache alloc] initWithContentsOfURL:[NSURL URLWithString:rootL2]];
            }
        }
    }
}
@end
@implementation RSCache (ConfigurationSupport)
+ (NSDictionary *)configurationWithL0Capacity:(NSUInteger)capacityL0
                                       atRoot:(NSString *)rootL1
                                        named:(NSString *)nameL1
                                    atRootURL:(NSURL *)rootL2
{
    RSCacheType type = 0;
    if (capacityL0) {
        type |= RSCacheSupportL0;
    }
    if (rootL1 && nameL1) {
        type |= RSCacheSupportL1;
    }
    if (rootL2) {
        type |= RSCacheSupportL2;
    }
    return @{   RSCacheL0Capacity:[NSNumber numberWithUnsignedInteger:capacityL0],RSCacheL0Launchload:[NSArray arrayWithObject:@""],
                RSCacheL1Root:rootL1,RSCacheL1Name:nameL1,
                RSCacheL1StoreLimitSize:[NSNumber numberWithInteger:-1],RSCacheL1Capacity:[NSNumber numberWithInteger:-1],
                RSCacheL2Root:[rootL2 absoluteString],
                RSCacheSettings:[NSNumber numberWithUnsignedInteger:[RSCache defaultSetting]|[RSCache configurationSupport]],
                RSCacheSupport:[NSNumber numberWithUnsignedInteger:type]};
}
- (id)initWithConfigurationOfFile:(NSString *)path
{
    NSDictionary* info = nil;
    NSMutableString* cacheConfigurationPath = [[path stringByStandardizingPath] mutableCopy];
    [cacheConfigurationPath appendFormat:@"/%@",RSLocalCacheDefaultConfigurationName];
    if ((info = [[NSDictionary alloc] initWithContentsOfFile:cacheConfigurationPath])) {
        return [self initWithConfiguration:info];
    }
    return nil;
}
- (id)initWithConfiguration:(NSDictionary *)configuration
{
    if ((configuration) && (self = [super init])) {
        _configuration = [configuration mutableCopy];
        [self _settingWithConfiguration:configuration];
    }
    return self;
}

- (void)setConfigurationSupport:(BOOL)support
{
    _supportConfiguration = support;
    if (_supportConfiguration) {
        [self addCacheSetting:[RSCache configurationSupport]];
        [self _loadConfiguration];
    }
    else
    {
        [self removeCacheSetting:[RSCache configurationSupport]];
        [self _saveConfiguration];
        _configuration = nil;
    }
}

- (BOOL)supportConfiguration
{
    return _supportConfiguration;
}

- (void)saveConfiguration
{
    [self _saveConfiguration];
}
@end
