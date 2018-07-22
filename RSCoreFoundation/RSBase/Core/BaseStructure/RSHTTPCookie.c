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
#include <RSCoreFoundation/RSArchiverRoutine.h>
#include <RSCoreFoundation/RSString+Extension.h>
#include <RSCoreFoundation/RSStringNumberSupport.h>
#include <RSCoreFoundation/RSNumber+Extension.h>

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
                 RSHTTPCookieGetVersion(RSHTTPCookie1) == RSHTTPCookieGetVersion(RSHTTPCookie2);
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
    
    RS_CONST_STRING_DECL(format0, "%r=%r; ");
    RS_CONST_STRING_DECL(format1, "%r=%r");
    RSStringRef format = format0;
    RSMutableStringRef buffer = nil;
    RSMutableArrayRef keys = (RSMutableArrayRef)RSDictionaryCopyAllKeys(cookie->_properties);
    RSMutableArrayRef values = (RSMutableArrayRef)RSDictionaryCopyAllValues(cookie->_properties);
    
    RS_CONST_STRING_DECL(HTTPOnly, "HTTPOnly");
    RSIndex indexOfHTTPOnly = RSArrayIndexOfObject(keys, HTTPOnly);
    if (indexOfHTTPOnly != RSNotFound) {
        RSArrayRemoveObjectAtIndex(keys, indexOfHTTPOnly);
        RSArrayRemoveObjectAtIndex(values, indexOfHTTPOnly);
    }
    RSIndex indexOfSecure = RSArrayIndexOfObject(keys, RSHTTPCookieSecure);
    if (indexOfSecure != RSNotFound) {
        RSArrayRemoveObjectAtIndex(keys, indexOfSecure);
        RSArrayRemoveObjectAtIndex(values, indexOfSecure);
    }
    RSIndex indexOfName = RSArrayIndexOfObject(keys, RSHTTPCookieName);
    RSIndex indexOfValue = RSArrayIndexOfObject(keys, RSHTTPCookieValue);
    if (indexOfName != RSNotFound && indexOfValue != RSNotFound) {
        RSStringRef name = RSRetain(RSArrayObjectAtIndex(values, indexOfName));
        RSStringRef value = RSRetain(RSArrayObjectAtIndex(values, indexOfValue));
        RSArrayRemoveObjectAtIndex(keys, indexOfName);
        RSArrayRemoveObjectAtIndex(values, indexOfName);
        RSArrayRemoveObjectAtIndex(keys, indexOfValue);
        RSArrayRemoveObjectAtIndex(values, indexOfValue);
        RSArrayAddObject(keys, name);
        RSArrayAddObject(values, value);
    }
    
    RSIndex indexOfExpireDate = RSArrayIndexOfObject(keys, RSHTTPCookieExpires);
    if (indexOfExpireDate != RSNotFound) {
        RSDateRef expireDate = RSArrayObjectAtIndex(values, indexOfExpireDate);
        RSStringRef (^process)(RSDateRef date) = ^RSStringRef(RSDateRef date) {
            static const char * const c_wkday[] __unused =
            {"Mon", "Tue", "Wed", "Thu", "Fri", "Sat", "Sun"};
            static const char * const c_weekday[] __unused =
            { "Monday", "Tuesday", "Wednesday", "Thursday",
                "Friday", "Saturday", "Sunday" };
            static const char * const c_month[] __unused =
            { "Jan", "Feb", "Mar", "Apr", "May", "Jun",
                "Jul", "Aug", "Sep", "Oct", "Nov", "Dec" };
            
            RSTimeZoneRef timeZone = RSAutorelease(RSTimeZoneCreateWithName(RSAllocatorDefault, RSSTR("GMT")));
            RSGregorianDate gregorianDate = RSAbsoluteTimeGetGregorianDate(RSDateGetAbsoluteTime(date), timeZone);
            RSStringRef desc = RSStringWithFormat(RSSTR("%s, %02d-%s-%04d %2d:%2d:%02d %r"), c_wkday[RSDateGetDayOfWeek(date)], gregorianDate.day, c_month[gregorianDate.month - 1], gregorianDate.year, gregorianDate.hour, gregorianDate.minute, (RSUInteger)(gregorianDate.second), RSTimeZoneGetName(timeZone));
            return desc;
        };
        RSArraySetObjectAtIndex((RSMutableArrayRef)values, indexOfExpireDate, process(expireDate));
    }
    
    RSUInteger cnt = RSArrayGetCount(keys);
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
    0,
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
    struct __RSHTTPCookie *instance = (struct __RSHTTPCookie *)__RSRuntimeCreateInstance(allocator, _RSHTTPCookieTypeID, sizeof(struct __RSHTTPCookie) - sizeof(RSRuntimeBase));
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

RSExport RSTimeInterval RSHTTPCookieGetMaximumAge(RSHTTPCookieRef cookie) {
    RSDictionaryRef properties = RSHTTPCookieGetProperties(cookie);
    if (!properties) return 0;
    return RSStringFloatValue(RSDictionaryGetValue(properties, RSHTTPCookieMaximumAge));
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
    if (!properties) return NO;
    return RSDictionaryGetValue(properties, RSHTTPCookieSecure) != 0;
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
