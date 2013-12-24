//
//  RSHTTPCookie.c
//  RSCoreFoundation
//
//  Created by Closure on 12/14/13.
//  Copyright (c) 2013 RetVal. All rights reserved.
//

#include "RSHTTPCookie.h"

#include <RSCoreFoundation/RSRuntime.h>
#include <RSCoreFoundation/RSArchiver.h>

#include "RSPrivate/libcurl/lib/share.h"
#include "RSPrivate/libcurl/lib/cookie.h"

RS_PUBLIC_CONST_STRING_DECL(RSHTTPCookieName, "name");
RS_PUBLIC_CONST_STRING_DECL(RSHTTPCookieValue, "value");
RS_PUBLIC_CONST_STRING_DECL(RSHTTPCookieOriginURL, "origin-url");
RS_PUBLIC_CONST_STRING_DECL(RSHTTPCookieVersion, "version");
RS_PUBLIC_CONST_STRING_DECL(RSHTTPCookieDomain, "domain");
RS_PUBLIC_CONST_STRING_DECL(RSHTTPCookiePath, "path");
RS_PUBLIC_CONST_STRING_DECL(RSHTTPCookieSecure, "secure");
RS_PUBLIC_CONST_STRING_DECL(RSHTTPCookieExpires, "expires");
RS_PUBLIC_CONST_STRING_DECL(RSHTTPCookieComment, "comment");
RS_PUBLIC_CONST_STRING_DECL(RSHTTPCookieCommentURL, "comment-url");
RS_PUBLIC_CONST_STRING_DECL(RSHTTPCookieDiscard, "discard");
RS_PUBLIC_CONST_STRING_DECL(RSHTTPCookieMaximumAge, "maximum-age");
RS_PUBLIC_CONST_STRING_DECL(RSHTTPCookiePort, "port");

static RSArrayRef walkThroughCurlCookies(struct SessionHandle *data, RSDictionaryRef (^block)(struct Cookie *cookie)){
    RSMutableArrayRef cookies = RSArrayCreateMutable(RSAllocatorSystemDefault, 0);
    Curl_share_lock(data, CURL_LOCK_DATA_COOKIE, CURL_LOCK_ACCESS_SINGLE);
    
    if (data && data->cookies && data->cookies->numcookies) {
        struct Cookie *cookie = data->cookies->cookies;
        while (cookie) {
            RSDictionaryRef properties = block(cookie);
            if (properties) {
                RSArrayAddObject(cookies, properties);
                RSRelease(properties);
            }
            cookie = cookie->next;
        }
    }
    Curl_share_unlock(data, CURL_LOCK_DATA_COOKIE);
    return RSAutorelease(cookies);
}

static RSArrayRef cookiesFromCurl(CURL *curl) {
    return walkThroughCurlCookies(curl, ^RSDictionaryRef(struct Cookie *cookie) {
        RSMutableDictionaryRef properties = RSDictionaryCreateMutable(RSAllocatorSystemDefault, 0, RSDictionaryRSTypeContext);
        RSDictionarySetValue(properties, RSHTTPCookieName, RSStringWithUTF8String(cookie->name));
        RSDictionarySetValue(properties, RSHTTPCookieValue, RSStringWithUTF8String(cookie->value));
        RSDictionarySetValue(properties, RSHTTPCookieDomain, RSStringWithUTF8String(cookie->domain));
        RSDictionarySetValue(properties, RSHTTPCookiePath, RSStringWithUTF8String(cookie->path));
        RSDictionarySetValue(properties, RSHTTPCookieExpires, RSAutorelease(RSDateCreate(RSAllocatorSystemDefault, cookie->expires)));
        RSDictionarySetValue(properties, RSHTTPCookieVersion, cookie->version ? RSStringWithUTF8String(cookie->version) : RSSTR("0"));
        RSDictionarySetValue(properties, RSHTTPCookieMaximumAge, RSStringWithUTF8String(cookie->maxage));
        RSDictionarySetValue(properties, RSHTTPCookieSecure, cookie->secure ? RSSTR("YES") : RSSTR("NO"));
        RSDictionarySetValue(properties, RSSTR("HTTPOnly"), cookie->httponly ? RSSTR("YES") : RSSTR("NO"));
        return properties;
    });
}

static RSArrayRef RSCookiesCreateWithProperties(RSArrayRef properties) {
    RSMutableArrayRef cookies = RSArrayCreateMutable(RSAllocatorSystemDefault, RSArrayGetCount(properties));
    RSArrayApplyBlock(properties, RSMakeRange(0, RSArrayGetCount(properties)), ^(const void *value, RSUInteger idx, BOOL *isStop) {
        RSHTTPCookieRef cookie = RSHTTPCookieCreate(RSAllocatorSystemDefault, value);
        RSArrayAddObject(cookies, cookie);
        RSRelease(cookie);
    });
    return cookies;
}

RSExport RSArrayRef RSCookiesWithCore(void *core) {
    return RSAutorelease(RSCookiesCreateWithProperties(cookiesFromCurl(core)));
}

struct __RSHTTPCookie
{
    RSRuntimeBase _base;
    RSDictionaryRef _properties;
    
    RSUInteger _version;
    BOOL _sessionOnly;
    BOOL _httpOnly;
};

static void __RSHTTPCookieClassInit(RSTypeRef rs)
{
    
}

static RSTypeRef __RSHTTPCookieClassCopy(RSAllocatorRef allocator, RSTypeRef rs, BOOL mutableCopy)
{
    return RSRetain(rs);
}

static void __RSHTTPCookieClassDeallocate(RSTypeRef rs)
{
    RSHTTPCookieRef cookie = (RSHTTPCookieRef)rs;
    if (cookie->_properties) RSRelease(cookie->_properties);
    return;
}

static BOOL __RSHTTPCookieClassEqual(RSTypeRef rs1, RSTypeRef rs2)
{
    RSHTTPCookieRef RSHTTPCookie1 = (RSHTTPCookieRef)rs1;
    RSHTTPCookieRef RSHTTPCookie2 = (RSHTTPCookieRef)rs2;
    BOOL result = NO;
    
    result = RSHTTPCookie1->_httpOnly == RSHTTPCookie2->_httpOnly && RSHTTPCookie1->_sessionOnly == RSHTTPCookie2->_sessionOnly;
    if (result) {
        result = RSEqual(RSHTTPCookieGetDomain(RSHTTPCookie1), RSHTTPCookieGetDomain(RSHTTPCookie2)) &&
                 RSEqual(RSHTTPCookieGetPath(RSHTTPCookie1), RSHTTPCookieGetPath(RSHTTPCookie2)) &&
                 RSEqual(RSHTTPCookieGetName(RSHTTPCookie1), RSHTTPCookieGetName(RSHTTPCookie2)) &&
//                 RSEqual(RSHTTPCookieGetValue(RSHTTPCookie1), RSHTTPCookieGetValue(RSHTTPCookie2)) &&
                 RSEqual(RSHTTPCookieGetVersion(RSHTTPCookie1), RSHTTPCookieGetVersion(RSHTTPCookie2));
    }
    return result;
}

static RSHashCode __RSHTTPCookieClassHash(RSTypeRef rs)
{
    return RSHash(((RSHTTPCookieRef)rs)->_properties);
}

static RSStringRef __RSHTTPCookieClassDescription(RSTypeRef rs)
{
    RSHTTPCookieRef cookie = (RSHTTPCookieRef)rs;
    RSStringRef domain = RSDictionaryGetValue(cookie->_properties, RSHTTPCookieDomain);
    const char *domainPtr = "";
    if (domain) {
        if (NO == RSStringHasPrefix(domain, RSSTR("."))) {
            domainPtr = ".";
        } else {
            domainPtr = "";
        }
    }
//    H_PS_PSSID=4382_1436_4414_4263; path=/; domain=.baidu.com
    RSStringRef format0 = RSSTR("%r=%r; "), format1 = RSSTR("%r=%r");
    RSStringRef format = format0;
    RSMutableStringRef buffer = nil;
    RSArrayRef keys = RSDictionaryCopyAllKeys(cookie->_properties);
    RSArrayRef values = RSDictionaryCopyAllValues(cookie->_properties);
    
    RSUInteger cnt = RSDictionaryGetCount(cookie->_properties);
    if (cnt) {
        buffer = RSStringCreateMutable(RSAllocatorSystemDefault, 0);
        for (RSUInteger idx = 0; idx < cnt - 1; idx++) {
            RSStringAppendStringWithFormat(buffer, format, RSArrayObjectAtIndex(keys, idx), RSArrayObjectAtIndex(values, idx));
        }
        format = format1;
        RSStringAppendStringWithFormat(buffer, format, RSArrayObjectAtIndex(keys, cnt - 1), RSArrayObjectAtIndex(values, cnt - 1));
    } else {
        buffer = (RSMutableStringRef)RSStringCreateWithFormat(RSAllocatorSystemDefault, format1, RSArrayObjectAtIndex(keys, 0), RSArrayObjectAtIndex(values, 0));
    }
    RSRelease(keys);
    RSRelease(values);
    RSStringRef description = RSCopy(RSAllocatorSystemDefault, buffer);
    RSRelease(buffer);
    return description;
}

static RSRuntimeClass __RSHTTPCookieClass =
{
    _RSRuntimeScannedObject,
    "RSHTTPCookie",
    __RSHTTPCookieClassInit,
    __RSHTTPCookieClassCopy,
    __RSHTTPCookieClassDeallocate,
    __RSHTTPCookieClassEqual,
    __RSHTTPCookieClassHash,
    __RSHTTPCookieClassDescription,
    nil,
    nil
};

static RSTypeID _RSHTTPCookieTypeID = _RSRuntimeNotATypeID;

RSExport RSTypeID RSHTTPCookieGetTypeID()
{
    return _RSHTTPCookieTypeID;
}

static RSDataRef __RSHTTPCookieSerializeCallback(RSArchiverRef archiver, RSTypeRef object) {
    RSHTTPCookieRef cookie = (RSHTTPCookieRef)object;
    return RSArchiverEncodeObject(archiver, cookie->_properties);
}

static RSTypeRef __RSHTTPCookieDeserializeCallback(RSUnarchiverRef unarchiver, RSDataRef data) {
    RSDictionaryRef properties = RSUnarchiverDecodeObject(unarchiver, data);
    RSHTTPCookieRef cookie = RSHTTPCookieCreate(RSAllocatorSystemDefault, properties);
    RSRelease(properties);
    return cookie;
}

RSPrivate void __RSHTTPCookieInitialize()
{
    _RSHTTPCookieTypeID = __RSRuntimeRegisterClass(&__RSHTTPCookieClass);
    __RSRuntimeSetClassTypeID(&__RSHTTPCookieClass, _RSHTTPCookieTypeID);
    
    const struct __RSArchiverCallBacks __RSHTTPCookieArchiverCallbacks = {
        0,
        _RSHTTPCookieTypeID,
        __RSHTTPCookieSerializeCallback,
        __RSHTTPCookieDeserializeCallback
    };
    if (!RSArchiverRegisterCallbacks(&__RSHTTPCookieArchiverCallbacks))
        __RSCLog(RSLogLevelWarning, "%s %s\n", __func__, "fail to register archiver routine");
}

RSPrivate void __RSHTTPCookieDeallocate()
{
//    <#do your finalize operation#>
}

static RSHTTPCookieRef __RSHTTPCookieCreateInstance(RSAllocatorRef allocator, RSDictionaryRef _properties) {
    RSHTTPCookieRef instance = (RSHTTPCookieRef)__RSRuntimeCreateInstance(allocator, _RSHTTPCookieTypeID, sizeof(struct __RSHTTPCookie) - sizeof(RSRuntimeBase));
    if (_properties) {
        instance->_properties = RSRetain(_properties);
        RSTypeRef value = RSDictionaryGetValue(_properties, RSSTR("HTTPOnly"));
        instance->_httpOnly = value ? RSStringBooleanValue(value) : NO;
        value = RSDictionaryGetValue(_properties, RSHTTPCookieSecure);
        instance->_sessionOnly = value ? RSStringBooleanValue(value) : NO;
        value = RSDictionaryGetValue(_properties, RSHTTPCookieVersion);
        instance->_version = value ? RSStringUnsignedIntegerValue(value) : 1;
    }
    return instance;
}

RSExport RSHTTPCookieRef RSHTTPCookieCreate(RSAllocatorRef allocator, RSDictionaryRef properties) {
    RSHTTPCookieRef cookie = __RSHTTPCookieCreateInstance(allocator, properties);
    return cookie;
}

RSExport RSDictionaryRef RSHTTPCookieGetProperties(RSHTTPCookieRef cookie) {
    if (!cookie) return nil;
    __RSGenericValidInstance(cookie, _RSHTTPCookieTypeID);
    return cookie->_properties;
}

RSExport RSUInteger RSHTTPCookieGetVersion(RSHTTPCookieRef cookie) {
    return 1;
}

RSExport RSStringRef RSHTTPCookieGetName(RSHTTPCookieRef cookie) {
    RSDictionaryRef properties = RSHTTPCookieGetProperties(cookie);
    if (!properties) return nil;
    return RSDictionaryGetValue(properties, RSHTTPCookieName);
}

RSExport RSStringRef RSHTTPCookieGetValue(RSHTTPCookieRef cookie) {
    RSDictionaryRef properties = RSHTTPCookieGetProperties(cookie);
    if (!properties) return nil;
    return RSDictionaryGetValue(properties, RSHTTPCookieValue);
}

RSExport RSDateRef  RSHTTPCookieGetExpiresDate(RSHTTPCookieRef cookie) {
    RSDictionaryRef properties = RSHTTPCookieGetProperties(cookie);
    if (!properties) return nil;
    return RSDictionaryGetValue(properties, RSHTTPCookieExpires);
}

RSExport BOOL RSHTTPCookieIsSessionOnly(RSHTTPCookieRef cookie) {
    if (!cookie) return NO;
    __RSGenericValidInstance(cookie, _RSHTTPCookieTypeID);
    return cookie->_sessionOnly;
}

RSExport RSStringRef RSHTTPCookieGetDomain(RSHTTPCookieRef cookie) {
    RSDictionaryRef properties = RSHTTPCookieGetProperties(cookie);
    if (!properties) return nil;
    return RSDictionaryGetValue(properties, RSHTTPCookieDomain);
}

RSExport RSStringRef RSHTTPCookieGetPath(RSHTTPCookieRef cookie) {
    RSDictionaryRef properties = RSHTTPCookieGetProperties(cookie);
    if (!properties) return nil;
    return RSDictionaryGetValue(properties, RSHTTPCookiePath);
}

RSExport BOOL RSHTTPCookieIsSecure(RSHTTPCookieRef cookie) {
    RSDictionaryRef properties = RSHTTPCookieGetProperties(cookie);
    if (!properties) return nil;
    return RSDictionaryGetValue(properties, RSHTTPCookieSecure);
}

RSExport BOOL RSHTTPCookieIsHTTPOnly(RSHTTPCookieRef cookie) {
    if (!cookie) return NO;
    __RSGenericValidInstance(cookie, _RSHTTPCookieTypeID);
    return cookie->_httpOnly;
}

RSExport RSStringRef RSHTTPCookieGetComment(RSHTTPCookieRef cookie) {
    RSDictionaryRef properties = RSHTTPCookieGetProperties(cookie);
    if (!properties) return nil;
    return RSDictionaryGetValue(properties, RSHTTPCookieComment);
}
RSExport RSURLRef RSHTTPCookieGetCommentURL(RSHTTPCookieRef cookie) {
    RSDictionaryRef properties = RSHTTPCookieGetProperties(cookie);
    if (!properties) return nil;
    return RSDictionaryGetValue(properties, RSHTTPCookieCommentURL);
}

RSExport RSArrayRef RSHTTPCookieGetPortList(RSHTTPCookieRef cookie) {
    RSDictionaryRef properties = RSHTTPCookieGetProperties(cookie);
    if (!properties) return nil;
    return RSDictionaryGetValue(properties, RSHTTPCookiePort);
}
