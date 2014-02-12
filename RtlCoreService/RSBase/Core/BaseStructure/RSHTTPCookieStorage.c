//
//  RSHTTPCookieStorage.c
//  RSCoreFoundation
//
//  Created by closure on 12/17/13.
//  Copyright (c) 2013 RetVal. All rights reserved.
//

#include "RSHTTPCookieStorage.h"
#include <RSCoreFoundation/RSData+Extension.h>
#include <RSCoreFoundation/RSDictionary+Extension.h>
#include <RSCoreFoundation/RSRuntime.h>
#include <RSCoreFoundation/RSArchiver.h>
#include <RSCoreFoundation/RSArchiverRoutine.h>
#include <RSCoreFoundation/RSBundle.h>
#include <RSCoreFoundation/RSFileManager.h>
#include <RSCoreFoundation/RSUUID.h>
#include <RSCoreFoundation/RSNotificationCenter.h>
#include "RSPrivate/RSPrivateOperatingSystem/RSPrivateFileSystem.h"

RS_CONST_STRING_DECL(__RSHTTPCookieStoragePreferencePathTargetFormat, "~/Library/Cookies/RSCoreFoundation/%s/RSCookie.plist");
RS_CONST_STRING_DECL(__RSHTTPCookieStoragePreferencePathTargetRSFormat, "~/Library/Cookies/RSCoreFoundation/%r/RSCookie.plist");
RS_CONST_STRING_DECL(__RSHTTPCookieStoragePreferencePathDirectoryFormat, "~/Library/Cookies/RSCoreFoundation/%s/");
RS_CONST_STRING_DECL(__RSHTTPCookieStoragePreferencePathDirectoryRSFormat, "~/Library/Cookies/RSCoreFoundation/%r/");


RS_CONST_STRING_DECL(__RSHTTPCookieStorageBasePathKey, "BasePath");             //  RSString


// cache
RS_CONST_STRING_DECL(__RSHTTPCookieStorageTransformation, "Transformation");    //  RSArray - RSDictionary - {HOST - UUID }
RS_CONST_STRING_DECL(__RSHTTPCookieStorageUUID, "UUID");                        //  RSArray - RSArray - (cookie - properties)


struct __RSHTTPCookieStorage {
    RSRuntimeBase _base;
    RSSpinLock _lock;
    RSMutableDictionaryRef _cache;
    RSStringRef _storageBasePath;
    RSDictionaryRef _preferences;
};

static RSStringRef __RSHTTPCookieStorageDefaultFilePath() {
    const char *name = nil;
    RSBundleRef bundle = RSBundleGetMainBundle();
    if (!bundle) {
        name = *_RSGetProgname();
    }
    RSStringRef path = RSStringCreateWithFormat(RSAllocatorSystemDefault, name ? __RSHTTPCookieStoragePreferencePathTargetFormat : __RSHTTPCookieStoragePreferencePathTargetRSFormat, (name ? (RSStringRef)name : RSBundleGetIdentifier(bundle)));
    RSStringRef returnPath = RSFileManagerStandardizingPath(path);
    RSRelease(path);
    return returnPath;
}

static RSStringRef __RSHTTPCookieStorageDefaultDirectoryPath() {
    const char *name = nil;
    RSBundleRef bundle = RSBundleGetMainBundle();
    if (!bundle) {
        name = *_RSGetProgname();
    }
    RSStringRef path = RSStringCreateWithFormat(RSAllocatorSystemDefault, name ? __RSHTTPCookieStoragePreferencePathDirectoryFormat : __RSHTTPCookieStoragePreferencePathDirectoryRSFormat, (name ? (RSStringRef)name : RSBundleGetIdentifier(bundle)));
    RSStringRef returnPath = RSFileManagerStandardizingPath(path);
    RSRelease(path);
    return returnPath;
}

static RSStringRef __RSHTTPCookieStorageBasePath(RSUUIDRef uuid) {
    RSStringRef dir = __RSHTTPCookieStorageDefaultDirectoryPath();
    RSStringRef path = RSStringWithFormat(RSSTR("%r%r"), dir, RSAutorelease(RSUUIDCreateString(RSAllocatorSystemDefault, uuid)));
    return path;
}

static BOOL __RSHTTPCookieStorageCacheInit(RSStringRef path) {
    RSDictionaryRef cache = RSDictionaryCreateWithObjectsAndOKeys(RSAllocatorSystemDefault, RSAutorelease(RSDictionaryCreateMutable(RSAllocatorSystemDefault, 0, RSDictionaryRSTypeContext)), __RSHTTPCookieStorageTransformation, RSAutorelease(RSDictionaryCreateMutable(RSAllocatorSystemDefault, 0, RSDictionaryRSTypeContext)), __RSHTTPCookieStorageUUID, NULL);
//    BOOL result = RSDictionaryWriteToFile(cache, path, RSWriteFileAutomatically);
    
    RSArchiverRef archiver = RSArchiverCreate(RSAllocatorSystemDefault);
    RSArchiverEncodeObjectForKey(archiver, RSKeyedArchiveRootObjectKey, cache);
    RSRelease(cache);
    
    RSDataRef data = RSArchiverCopyData(archiver);
    RSRelease(archiver);
    
    BOOL result = RSDataWriteToFile(data, path, RSWriteFileAutomatically);
    RSRelease(data);
    return result;
}

static BOOL __RSHTTPCookieStoragePreferencesInit(RSStringRef path) {
    RSStringRef targetDirectory = __RSHTTPCookieStorageDefaultDirectoryPath();
    RSStringRef targetFile = __RSHTTPCookieStorageDefaultFilePath();
    RSFileManagerCreateDirectoryAtPath(RSFileManagerGetDefault(), targetDirectory);
    
    RSStringRef basePath = path ? : __RSHTTPCookieStorageBasePath(RSAutorelease(RSUUIDCreate(RSAllocatorSystemDefault)));
    if (__RSHTTPCookieStorageCacheInit(basePath)) {
        RSDictionaryRef preferences = RSDictionaryCreateWithObjectsAndOKeys(RSAllocatorSystemDefault, basePath, __RSHTTPCookieStorageBasePathKey, nil);
        BOOL result = RSDictionaryWriteToFile(preferences, targetFile, RSWriteFileAutomatically);
        RSRelease(preferences);
        return result;
    }
    return NO;
}

static void __RSHTTPCookieStorageClassInit(RSTypeRef rs) {
    RSHTTPCookieStorageRef storage = (RSHTTPCookieStorageRef)rs;
    BOOL isFile = NO;
    RSStringRef targetFile = __RSHTTPCookieStorageDefaultFilePath();
    if (!(RSFileManagerFileExistsAtPath(RSFileManagerGetDefault(), targetFile, &isFile) && !isFile)) {
        if (!__RSHTTPCookieStoragePreferencesInit(nil)) {
            __RSCLog(RSLogLevelWarning, "%s %s\n", __func__, "can not init default settings.");
            return;
        }
    }
    storage->_preferences = RSDictionaryCreateWithContentOfPath(RSAllocatorSystemDefault, __RSHTTPCookieStorageDefaultFilePath());
    storage->_storageBasePath = RSRetain(RSDictionaryGetValue(storage->_preferences, __RSHTTPCookieStorageBasePathKey));
    if (!RSFileManagerFileExistsAtPath(RSFileManagerGetDefault(), storage->_storageBasePath, &isFile)) {
        __RSHTTPCookieStoragePreferencesInit(storage->_storageBasePath);
    }
    RSUnarchiverRef unarchiver = RSUnarchiverCreateWithContentOfPath(RSAllocatorSystemDefault, storage->_storageBasePath);
    RSTypeRef rootObject = RSUnarchiverDecodeObjectForKey(unarchiver, RSKeyedArchiveRootObjectKey);
    RSRelease(unarchiver);
    storage->_cache = RSMutableCopy(RSAllocatorSystemDefault, rootObject);
    RSRelease(rootObject);
}

static void __RSHTTPCookieStorageClassDeallocate(RSTypeRef rs) {
    RSHTTPCookieStorageRef storage = (RSHTTPCookieStorageRef)rs;
    RSArchiverRef archiver = RSArchiverCreate(RSAllocatorSystemDefault);
    RSArchiverEncodeObjectForKey(archiver, RSKeyedArchiveRootObjectKey, storage->_cache);
    RSDataRef data = RSArchiverCopyData(archiver);
    RSRelease(archiver);
    RSDataWriteToFile(data, storage->_storageBasePath, RSWriteFileAutomatically);
    RSRelease(data);
    RSRelease(storage->_preferences);
    RSRelease(storage->_storageBasePath);
    RSRelease(storage->_cache);
}

static RSHashCode __RSHTTPCookieStorageClassHash(RSTypeRef rs) {
    return RSHash(((RSHTTPCookieStorageRef)rs)->_storageBasePath);
}

static RSStringRef __RSHTTPCookieStorageClassDescription(RSTypeRef rs) {
    RSStringRef description = RSStringCreateWithFormat(RSAllocatorDefault, RSSTR("RSHTTPCookieStorage %p"), rs);
    return description;
}

static RSRuntimeClass __RSHTTPCookieStorageClass =
{
    _RSRuntimeScannedObject,
    0,
    "RSHTTPCookieStorage",
    __RSHTTPCookieStorageClassInit,
    nil,
    __RSHTTPCookieStorageClassDeallocate,
    nil,
    __RSHTTPCookieStorageClassHash,
    __RSHTTPCookieStorageClassDescription,
    nil,
    nil
};

static RSDataRef __RSHTTPCookieStorageSerializeCallback(RSArchiverRef archiver, RSTypeRef object) {
    RSHTTPCookieStorageRef storage = (RSHTTPCookieStorageRef)object;
    RSDataRef data = RSArchiverEncodeObject(archiver, storage->_cache);
    return data;
}

static RSTypeRef __RSHTTPCookieStorageDeserializeCallback(RSUnarchiverRef unarchiver, RSDataRef data) {
    RSDictionaryRef dict = RSUnarchiverDecodeObject(unarchiver, data);
    return dict;
}

static RSSpinLock __RSHTTPCookieSharedStorageLock = RSSpinLockInit;
static RSHTTPCookieStorageRef __RSHTTPCookieSharedStorage = nil;

static RSHTTPCookieStorageRef __RSHTTPCookieStorageCreateInstance(RSAllocatorRef allocator);
static void __RSHTTPCookieStorageDeallocate(RSNotificationRef notification);

static RSTypeID _RSHTTPCookieStorageTypeID = _RSRuntimeNotATypeID;

#include <dispatch/dispatch.h>

RSExport RSTypeID RSHTTPCookieStorageGetTypeID() {
    static dispatch_once_t onceToken;
    dispatch_once(&onceToken, ^{
        RSHTTPCookieStorageGetSharedStorage();
    });
    return _RSHTTPCookieStorageTypeID;
}

static void __RSHTTPCookieStorageInitialize() {
    _RSHTTPCookieStorageTypeID = __RSRuntimeRegisterClass(&__RSHTTPCookieStorageClass);
    __RSRuntimeSetClassTypeID(&__RSHTTPCookieStorageClass, _RSHTTPCookieStorageTypeID);
    
    const struct __RSArchiverCallBacks __RSHTTPCookieStorageArchiverCallbacks = {
        0,
        _RSHTTPCookieStorageTypeID,
        __RSHTTPCookieStorageSerializeCallback,
        __RSHTTPCookieStorageDeserializeCallback
    };
    if (!RSArchiverRegisterCallbacks(&__RSHTTPCookieStorageArchiverCallbacks))
        __RSCLog(RSLogLevelWarning, "%s %s\n", __func__, "fail to register archiver routine");
    RSNotificationCenterAddObserver(RSNotificationCenterGetDefault(), RSAutorelease(RSObserverCreate(RSAllocatorSystemDefault, RSCoreFoundationWillDeallocateNotification, &__RSHTTPCookieStorageDeallocate, nil)));
}

static void __RSHTTPCookieStorageDeallocate(RSNotificationRef notification) {
    RSSyncUpdateBlock(&__RSHTTPCookieSharedStorageLock, ^{
        if (nil != __RSHTTPCookieSharedStorage) {
            __RSRuntimeSetInstanceSpecial(__RSHTTPCookieSharedStorage, NO);
            RSRelease(__RSHTTPCookieSharedStorage);
            __RSHTTPCookieSharedStorage = nil;
        }
    });
}

static RSHTTPCookieStorageRef __RSHTTPCookieStorageCreateInstance(RSAllocatorRef allocator)
{
    RSHTTPCookieStorageRef instance = (RSHTTPCookieStorageRef)__RSRuntimeCreateInstance(allocator, _RSHTTPCookieStorageTypeID, sizeof(struct __RSHTTPCookieStorage) - sizeof(RSRuntimeBase));
    
    return instance;
}

RSExport RSHTTPCookieStorageRef RSHTTPCookieStorageGetSharedStorage() {
    RSSyncUpdateBlock(&__RSHTTPCookieSharedStorageLock, ^{
        if (nil == __RSHTTPCookieSharedStorage) {
            __RSHTTPCookieStorageInitialize();
            __RSHTTPCookieSharedStorage = __RSHTTPCookieStorageCreateInstance(RSAllocatorSystemDefault);
            __RSRuntimeSetInstanceSpecial(__RSHTTPCookieSharedStorage, YES);
        }
    });
    return __RSHTTPCookieSharedStorage;
}


// update cache with cookie
// return cookie uuid in cache, maybe nil.
static RSStringRef __RSHTTPCookieStorageUpdateCookieInCache(RSDictionaryRef cache, RSHTTPCookieRef cookie, BOOL remove) {
    RSMutableDictionaryRef transformation = (RSMutableDictionaryRef)RSDictionaryGetValue(cache, __RSHTTPCookieStorageTransformation);
    if (!transformation) {
        RSMutableDictionaryRef dict = RSDictionaryCreateMutable(RSAllocatorSystemDefault, 0, RSDictionaryRSTypeContext);
        RSDictionarySetValue((RSMutableDictionaryRef)cache, __RSHTTPCookieStorageTransformation, dict);
        RSRelease(dict);
        transformation = dict;
    }
    RSDictionaryRef paths = RSDictionaryGetValue(transformation, RSHTTPCookieGetDomain(cookie));
    if (!paths) {
        if (remove) {
            // if remove, the paths need not be built.
            return nil;
        }
        RSMutableDictionaryRef dict = RSDictionaryCreateMutable(RSAllocatorSystemDefault, 0, RSDictionaryRSTypeContext);
        RSDictionarySetValue((RSMutableDictionaryRef)transformation, RSHTTPCookieGetDomain(cookie), dict);
        RSRelease(dict);
        paths = dict;
    }
    RSMutableArrayRef uuids = (RSMutableArrayRef)RSDictionaryGetValue(paths, RSHTTPCookieGetPath(cookie));
    if (!(uuids && RSArrayGetCount(uuids))) {
        if (remove) {
            // if remove, the collection need not be built, and empty collection should be removed!
            if (uuids && RSArrayGetCount(uuids))
                RSDictionaryRemoveValue((RSMutableDictionaryRef)paths, RSHTTPCookieGetPath(cookie));
            if (paths && RSDictionaryGetCount(paths))
                RSDictionaryRemoveValue((RSMutableDictionaryRef)transformation, RSHTTPCookieGetDomain(cookie));
            return nil;
        }
        RSMutableArrayRef array = RSArrayCreateMutable(RSAllocatorSystemDefault, 0);
        RSDictionarySetValue((RSMutableDictionaryRef)paths, RSHTTPCookieGetPath(cookie), array);
        RSRelease(array);
        uuids = array;
    }
    __block RSStringRef result = nil;
    RSArrayApplyBlock(uuids, RSMakeRange(0, RSArrayGetCount(uuids)), ^(const void *value, RSUInteger idx, BOOL *isStop) {
        *isStop = RSEqual(cookie, RSDictionaryGetValue(RSDictionaryGetValue(cache, __RSHTTPCookieStorageUUID), value));
        if (*isStop)
            result = value;
        else {
//            RSLog(RSSTR("\n%r - \n%r\n"), cookie, RSDictionaryGetValue(RSDictionaryGetValue(cache, __RSHTTPCookieStorageUUID), value));
        }
    });
    
    if (!remove) {
        if (!result) {
            result = RSUUIDCreateString(RSAllocatorSystemDefault, RSAutorelease(RSUUIDCreate(RSAllocatorSystemDefault)));
            RSArrayAddObject(uuids, result);
            RSRelease(result);
        }
        RSDictionarySetValue((RSMutableDictionaryRef)RSDictionaryGetValue(cache, __RSHTTPCookieStorageUUID), result, cookie);
    } else {
        if (result) {
            RSStringRef key = RSRetain(result);
            RSArrayRemoveObject(uuids, result);
            if (0 == RSArrayGetCount(uuids))
                RSDictionaryRemoveValue((RSMutableDictionaryRef)paths, RSHTTPCookieGetPath(cookie));
            if (0 == RSDictionaryGetCount(paths))
                RSDictionaryRemoveValue((RSMutableDictionaryRef)transformation, RSHTTPCookieGetDomain(cookie));
            RSDictionaryRemoveValue((RSMutableDictionaryRef)RSDictionaryGetValue(cache, __RSHTTPCookieStorageUUID), key);
            RSRelease(key);
        }
    }
    return result;
}

static RSArrayRef __RSHTTPCookieStorageCopyGuessDomain(RSStringRef domain) {
    if (!domain) return nil;
    RSMutableArrayRef results = RSArrayCreateMutable(RSAllocatorSystemDefault, 0);
    RS_CONST_STRING_DECL(__RSHTTPCookieStorageGuessDoaminFormat0, "%r");
    RS_CONST_STRING_DECL(__RSHTTPCookieStorageGuessDoaminFormat1, ".%r");
    RS_CONST_STRING_DECL(__RSHTTPCookieStorageGuessDoaminFormat2, "*.%r");
    
    RSStringRef __RSHTTPCookieStorageGuessDoaminFormat[] = {__RSHTTPCookieStorageGuessDoaminFormat0, __RSHTTPCookieStorageGuessDoaminFormat1, __RSHTTPCookieStorageGuessDoaminFormat2};
    
    const RSUInteger cnt = sizeof(__RSHTTPCookieStorageGuessDoaminFormat) / sizeof(RSStringRef);
    for (RSUInteger idx = 0; idx < cnt; idx++) {
        RSStringRef guess = RSStringCreateWithFormat(RSAllocatorSystemDefault, __RSHTTPCookieStorageGuessDoaminFormat[idx], domain);
        RSArrayAddObject(results, guess);
        RSRelease(guess);
    }
    
    return results;
}

static RSArrayRef __RSHTTPCookieStorageCopyCookiesForURLDomainAndPath(RSDictionaryRef cache, RSURLRef URL, RSStringRef domain, RSStringRef path) {
    RSDictionaryRef transformation = RSDictionaryGetValue(cache, __RSHTTPCookieStorageTransformation);
    if (!transformation) {
        RSMutableDictionaryRef dict = RSDictionaryCreateMutable(RSAllocatorSystemDefault, 0, RSDictionaryRSTypeContext);
        RSDictionarySetValue((RSMutableDictionaryRef)cache, __RSHTTPCookieStorageTransformation, dict);
        RSRelease(dict);
        return nil;
    }
    
    // guess the domain
    RSArrayRef domains = __RSHTTPCookieStorageCopyGuessDomain(domain);
    RSStringRef host = RSURLCopyHostName(URL);
    RSArrayAddObject((RSMutableArrayRef)domains, host);
    RSRelease(host);
    RSUInteger cnt = RSArrayGetCount(domains);
    
    RSMutableArrayRef paths = RSArrayCreateMutable(RSAllocatorSystemDefault, 0);
    RSArrayRef pathDomain = RSStringCreateArrayBySeparatingStrings(RSAllocatorSystemDefault, path, RSSTR("/"));
    RSMutableStringRef currentPath = RSStringCreateMutable(RSAllocatorSystemDefault, 0);
    RSArrayApplyBlock(pathDomain, RSMakeRange(0, RSArrayGetCount(pathDomain) - 1), ^(const void *value, RSUInteger idx, BOOL *isStop) {
        RSStringRef path = RSStringCreateWithFormat(RSAllocatorSystemDefault, RSSTR("%r%r/"), currentPath, value);
        RSStringAppendStringWithFormat(currentPath, RSSTR("%r/"), value);
        RSArrayAddObject(paths, path);
        RSRelease(path);
    });
    RSStringRef tmp = RSStringCreateWithFormat(RSAllocatorSystemDefault, RSSTR("%r%r"), currentPath, RSArrayLastObject(pathDomain));
    RSArrayAddObject(paths, tmp);
    RSRelease(tmp);
    RSStringAppendString(currentPath, RSArrayLastObject(pathDomain));
    RSRelease(pathDomain);
    tmp = RSStringCreateWithFormat(RSAllocatorSystemDefault, RSSTR("%r/"), currentPath);
    RSArrayAddObject(paths, tmp);
    RSRelease(tmp);
    RSRelease(currentPath);

    RSMutableArrayRef results = RSArrayCreateMutable(RSAllocatorSystemDefault, 0);
    for (RSUInteger idx = 0; idx < cnt; idx++)
    {
        RSStringRef domain = RSArrayObjectAtIndex(domains, idx);
        RSDictionaryRef pathsTransformation = RSDictionaryGetValue(transformation, domain);
        if (!pathsTransformation) {
            continue;
        }
        RSArrayApplyBlock(paths, RSMakeRange(0, RSArrayGetCount(paths)), ^(const void *value, RSUInteger idx, BOOL *isStop) {
            RSStringRef path = value;
            RSArrayRef uuids = RSDictionaryGetValue(pathsTransformation, path);
            if ((uuids && RSArrayGetCount(uuids))) {
                RSArrayApplyBlock(uuids, RSMakeRange(0, RSArrayGetCount(uuids)), ^(const void *value, RSUInteger idx, BOOL *isStop) {
                    RSArrayAddObject(results, RSDictionaryGetValue(RSDictionaryGetValue(cache, __RSHTTPCookieStorageUUID), value));
                });
            }
        });
    }
    RSRelease(paths);
    RSRelease(domains);
    return results;
}

static RSStringRef __RSHTTPCookieStorageFindCookieInCache(RSDictionaryRef cache, RSHTTPCookieRef cookie) {
    /*
     ----cache(dict)
     |
     |----Transformation(array)
     |            |
     |            |----Host Names, Paths (UUIDs)
     |
     |
     |----UUID----Cookie
     */
    RSDictionaryRef transformation = RSDictionaryGetValue(cache, __RSHTTPCookieStorageTransformation);
    if (!transformation) {
        RSMutableDictionaryRef dict = RSDictionaryCreateMutable(RSAllocatorSystemDefault, 0, RSDictionaryRSTypeContext);
        RSDictionarySetValue((RSMutableDictionaryRef)cache, __RSHTTPCookieStorageTransformation, dict);
        RSRelease(dict);
        return nil;
    }
    RSDictionaryRef paths = RSDictionaryGetValue(transformation, RSHTTPCookieGetDomain(cookie));
    if (!paths) {
        RSMutableDictionaryRef dict = RSDictionaryCreateMutable(RSAllocatorSystemDefault, 0, RSDictionaryRSTypeContext);
        RSDictionarySetValue((RSMutableDictionaryRef)cache, RSHTTPCookieGetDomain(cookie), dict);
        RSRelease(dict);
        return nil;
    }
    RSArrayRef uuids = RSDictionaryGetValue(paths, RSHTTPCookieGetPath(cookie));
    if (!(uuids && RSArrayGetCount(uuids))) {
        RSMutableArrayRef array = RSArrayCreateMutable(RSAllocatorSystemDefault, 0);
        RSDictionarySetValue((RSMutableDictionaryRef)cache, RSHTTPCookieGetPath(cookie), array);
        RSRelease(array);
        return nil;
    }
    
    __block RSStringRef result = nil;
    RSArrayApplyBlock(uuids, RSMakeRange(0, RSArrayGetCount(uuids)), ^(const void *value, RSUInteger idx, BOOL *isStop) {
        *isStop = RSEqual(cookie, RSDictionaryGetValue(RSDictionaryGetValue(cache, __RSHTTPCookieStorageUUID), value));
        if (*isStop) result = value;
    });
    return result;
}

static RSArrayRef __RSHTTPCookieStorageCopyCookies(RSHTTPCookieStorageRef storage) {
    RSMutableArrayRef cookies = nil;
    RSDictionaryRef uuidCookies = RSDictionaryGetValue(storage->_cache, __RSHTTPCookieStorageUUID);
    if (uuidCookies && RSDictionaryGetCount(uuidCookies)) {
        cookies = RSArrayCreateMutable(RSAllocatorSystemDefault, RSDictionaryGetCount(uuidCookies));
        RSDictionaryApplyBlock(uuidCookies, ^(const void *key, const void *value, BOOL *stop) {
            RSArrayAddObject(cookies, value);   // cookie is readonly, need not use copy method.
        });
    }
    return cookies;
}

static RSStringRef __RSHTTPCookieStorageUpdateCookieTransformationAndUUID(RSHTTPCookieStorageRef storage, RSHTTPCookieRef cookie) {
    RSStringRef uuidString = RSRetain(__RSHTTPCookieStorageFindCookieInCache(storage->_cache, cookie));
    if (!uuidString) {
        RSUUIDRef uuid = RSUUIDCreate(RSAllocatorSystemDefault);
        uuidString = RSUUIDCreateString(RSAllocatorSystemDefault, uuid);
    }
    RSRelease(uuidString);
    return uuidString;
}

static RSStringRef __RSHTTPCookieStorageCreateDomainWithHostName(RSStringRef hostName) {
    if (!hostName) return nil;
    RS_CONST_STRING_DECL(__RSHTTPCookieDomainSegmentation, ".")
    RSStringRef domain = nil;
    RSIndex numberOfResult = 0;
    RSRange *results = RSStringFindAll(hostName, __RSHTTPCookieDomainSegmentation, &numberOfResult);
    if (results && numberOfResult > 1) {
        domain = RSStringCreateWithSubstring(RSAllocatorSystemDefault, hostName, RSMakeRange(results[0].location + results[0].length, RSStringGetLength(hostName) - results[0].location - results[0].length));
        RSAllocatorDeallocate(RSAllocatorSystemDefault, results);
    }
    return domain;
}


RSExport RSArrayRef RSHTTPCookieStorageCopyCookiesForURL(RSHTTPCookieStorageRef storage, RSURLRef URL) {
    if (!storage || !URL) return nil;
    __RSGenericValidInstance(storage, _RSHTTPCookieStorageTypeID);
    __block RSArrayRef cookies = nil;
    RSSyncUpdateBlock(&storage->_lock, ^{
        RS_CONST_STRING_DECL(__RSHTTPCookieRootPath, "/");
        RSStringRef host = RSURLCopyHostName(URL);
        RSStringRef path = RSURLCopyPath(URL);
        if (RSEqual(path, RSStringGetEmptyString())) {
            RSRelease(path);
            path = __RSHTTPCookieRootPath;
        }
        RSStringRef domain = __RSHTTPCookieStorageCreateDomainWithHostName(host);
        RSRelease(host);
        cookies = __RSHTTPCookieStorageCopyCookiesForURLDomainAndPath(storage->_cache, URL, domain, path);
        RSRelease(domain);
        RSRelease(path);
    });
    return cookies;
}

RSExport void RSHTTPCookieStorageSetCookie(RSHTTPCookieStorageRef storage, RSHTTPCookieRef cookie) {
    if (!storage || !cookie) return;
    __RSGenericValidInstance(storage, _RSHTTPCookieStorageTypeID);
//    if (RSAbsoluteTimeGetCurrent() > RSDateGetAbsoluteTime(RSHTTPCookieGetExpiresDate(cookie)) + RSHTTPCookieGetMaximumAge(cookie))
//        return;
    RSSyncUpdateBlock(&storage->_lock, ^{
        __RSHTTPCookieStorageUpdateCookieInCache(storage->_cache, cookie, NO);
    });
}

RSExport void RSHTTPCookieStorageRemoveCookie(RSHTTPCookieStorageRef storage, RSHTTPCookieRef cookie) {
    if (!storage || !cookie) return;
    __RSGenericValidInstance(storage, _RSHTTPCookieStorageTypeID);
    RSSyncUpdateBlock(&storage->_lock, ^{
        __RSHTTPCookieStorageUpdateCookieInCache(storage->_cache, cookie, YES);
    });
}

RSExport void RSHTTPCookieStorageRemoveCookiesFromArray(RSHTTPCookieStorageRef storage, RSArrayRef cookies) {
    if (!storage || !cookies) return;
    __RSGenericValidInstance(storage, _RSHTTPCookieStorageTypeID);
    RSArrayApplyBlock(cookies, RSMakeRange(0, RSArrayGetCount(cookies)), ^(const void *value, RSUInteger idx, BOOL *isStop) {
        if (RSGetTypeID(value) == RSHTTPCookieGetTypeID())
            RSHTTPCookieStorageRemoveCookie(storage, value);
    });
}
RSExport void RSHTTPCookieStorageRemoveAllCookies(RSHTTPCookieStorageRef storage) {
    if (!storage) return;
    __RSGenericValidInstance(storage, _RSHTTPCookieStorageTypeID);
    RSArrayRef cookies = RSHTTPCookieStorageCopyCookies(storage);
    RSHTTPCookieStorageRemoveCookiesFromArray(storage, cookies);
    RSRelease(cookies);
}

RSExport RSArrayRef RSHTTPCookieStorageCopyCookies(RSHTTPCookieStorageRef storage) {
    if (!storage) return nil;
    __RSGenericValidInstance(storage, _RSHTTPCookieStorageTypeID);
    __block RSArrayRef cookies = nil;
    RSSyncUpdateBlock(&storage->_lock, ^{
        cookies = __RSHTTPCookieStorageCopyCookies(storage);
    });
    return (cookies);
}
