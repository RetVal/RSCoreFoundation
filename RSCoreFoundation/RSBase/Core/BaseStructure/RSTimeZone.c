//
//  RSTimeZone.c
//  RSCoreFoundation
//
//  Created by RetVal on 11/5/12.
//  Copyright (c) 2012 RetVal. All rights reserved.
//

#include <stdio.h>
#include <RSCoreFoundation/RSBase.h>
#include <RSCoreFoundation/RSAllocator.h>
#include <RSCoreFoundation/RSString.h>
#include <RSCoreFoundation/RSDictionary.h>
#include <RSCoreFoundation/RSTimeZone.h>
#include <RSCoreFoundation/RSData.h>
#include <RSCoreFoundation/RSDate.h>


#include "RSPrivate/RSPrivateTimeZoneNames.h"

#include <RSCoreFoundation/RSRuntime.h>
#if DEPLOYMENT_TARGET_MACOSX
    #include <tzfile.h>
    #elif DEPLOYMENT_TARGET_LINUX
        #ifndef TZDIR
        #define TZDIR	"/usr/share/zoneinfo" /* Time zone object file directory */
    #endif /* !defined TZDIR */
#endif

#ifndef TZDEFAULT
#define TZDEFAULT	"/etc/localtime"
#endif /* !defined TZDEFAULT */

#if DEPLOYMENT_TARGET_MACOSX || DEPLOYMENT_TARGET_EMBEDDED || DEPLOYMENT_TARGET_LINUX
#define TZZONELINK	TZDEFAULT
#define TZZONEINFO	TZDIR "/"
#elif DEPLOYMENT_TARGET_WINDOWS
static RSStringRef __tzZoneInfo = nil;
static char *__tzDir = nil;
static void __InitTZStrings(void);
#else
#error Unknown or unspecified DEPLOYMENT_TARGET
#endif

//static RSMutableDictionaryRef __RSTimeZoneInformation = nil;
struct __RSTimeZone
{
    RSRuntimeBase _base;
    //RSDateRef   _date;
    RSStringRef _zoneName;
};

static RSTypeID _RSTimeZoneTypeID ;

RSInline void __RSTimeZoneAvailable(RSTimeZoneRef timeZone)
{
    if (timeZone == nil || timeZone->_zoneName == nil) HALTWithError(RSInvalidArgumentException, "the time zone object is nil");
    __RSGenericValidInstance(timeZone, _RSTimeZoneTypeID);
    return;
}

RSInline RSStringRef __RSTimeZoneGetName(RSTimeZoneRef timeZone)
{
    return (likely(timeZone)) ? timeZone->_zoneName : nil;
}

RSInline void __RSTimeZoneSetName(RSTimeZoneRef timeZone, RSStringRef zoneName)
{
    if (timeZone->_zoneName != nil) RSRelease(timeZone->_zoneName);
    if (zoneName) ((struct __RSTimeZone*)timeZone)->_zoneName = RSRetain(zoneName);
    else ((struct __RSTimeZone*)timeZone)->_zoneName = nil;
}

//RSInline RSDateRef __RSTimeZoneGetDate(RSTimeZoneRef timeZone)
//{
//    return timeZone->_date;
//}
//
//RSInline void __RSTimeZoneSetDate(RSTimeZoneRef timeZone, RSDateRef date)
//{
//    if (timeZone->_date != nil) RSRelease(timeZone->_date);
//    if (date) ((struct __RSTimeZone*)timeZone)->_date = RSRetain(date);
//    else ((struct __RSTimeZone*)timeZone)->_date = nil;
//}
//
static void __RSTimeZoneClassInit(RSTypeRef obj)
{
    RSTimeZoneRef tz = (RSTimeZoneRef)obj;
    //((struct __RSTimeZone*)tz)->_date = nil;
    ((struct __RSTimeZone*)tz)->_zoneName = nil;
    return;
}

static RSTypeRef __RSTimeZoneClassCopy(RSAllocatorRef allocator, RSTypeRef rs, BOOL mutableCopy)
{
    return RSTimeZoneCreateWithName(allocator, RSTimeZoneGetName(rs));
}

static void __RSTimeZoneClassDeallocate(RSTypeRef obj)
{
    RSTimeZoneRef tz = (RSTimeZoneRef)obj;
    //if (tz->_date) RSRelease(tz->_date);
    RSRelease(tz->_zoneName);
    //RSAllocatorDeallocate(RSAllocatorSystemDefault, tz);
    return;
}

static BOOL __RSTimeZoneClassEqual(RSTypeRef obj1, RSTypeRef obj2)
{
    return NO;
}

static RSStringRef __RSTimeZoneClassDescription(RSTypeRef obj)
{
    RSTimeZoneRef tz = (RSTimeZoneRef)obj;
    RSStringRef description = RSStringCreateWithFormat(RSAllocatorSystemDefault, RSSTR("%R - GMT : %5.0f"), __RSTimeZoneGetName(tz), RSTimeZoneGetSecondsFromGMT(tz, 0));
    return description;
}


static struct __RSRuntimeClass __RSTimeZoneClass =
{
    _RSRuntimeScannedObject,
    0,
    "RSTimeZone",
    __RSTimeZoneClassInit,
    __RSTimeZoneClassCopy,
    __RSTimeZoneClassDeallocate,
    __RSTimeZoneClassEqual,
    nil,
    __RSTimeZoneClassDescription,
    nil,
    nil
};

static RSTypeID _RSTimeZoneTypeID = _RSRuntimeNotATypeID;

RSExport RSTypeID RSTimeZoneGetTypeID()
{
    return _RSTimeZoneTypeID;
}

static RSArrayRef __RSTimeZoneKnownNames = nil;

static RSSpinLock    __RSTimeZoneSystemLock = RSSpinLockInit;
#define RSTimeZoneLockSystem    RSSpinLockLock(&__RSTimeZoneSystemLock)
#define RSTimeZoneUnlockSystem  RSSpinLockUnlock(&__RSTimeZoneSystemLock)
static RSTimeZoneRef __RSTimeZoneSystem = nil;

//static RSSpinLock    __RSTimeZoneDefaultLock = RSSpinLockInit;
//static RSTimeZoneRef __RSTimeZoneDefault = nil;

static RSArrayRef __RSTimeZoneKnownNamesArrayCreate()
{
    RSMutableArrayRef array = RSArrayCreateMutable(RSAllocatorSystemDefault, __RSPrivateTimeZoneNameCount);
    for (RSIndex idx = 0; idx < __RSPrivateTimeZoneNameCount; idx++)
    {
        RSStringRef name = RSStringCreateWithCString(RSAllocatorSystemDefault, __RSPrivateTimeZoneInfo[idx].timeZoneName, RSStringEncodingUTF8);
        RSArrayAddObject(array, name);
        RSRelease(name);
    }
//    RSArraySort(array, RSOrderedDescending, (RSComparatorFunction)RSStringCompare, nil);
    return array;
}

RSPrivate void __RSTimeZoneInitialize()
{
    _RSTimeZoneTypeID = __RSRuntimeRegisterClass(&__RSTimeZoneClass);
    __RSRuntimeSetClassTypeID(&__RSTimeZoneClass, _RSTimeZoneTypeID);
    
    __RSTimeZoneKnownNames = __RSTimeZoneKnownNamesArrayCreate();
}

RSPrivate void __RSTimeZoneDeallocate()
{
    RSTimeZoneLockSystem;
    if (__RSTimeZoneSystem) {
        __RSRuntimeSetInstanceSpecial(__RSTimeZoneSystem, NO);
        RSRelease(__RSTimeZoneSystem);
        __RSTimeZoneSystem = nil;
    }
    RSTimeZoneUnlockSystem;
    if (__RSTimeZoneKnownNames) RSRelease(__RSTimeZoneKnownNames);
    __RSTimeZoneKnownNames = nil;
    
}

static RSTimeZoneRef __RSTimeZoneCreateInstance(RSAllocatorRef allocator, RSStringRef zoneName)
{
    RSTimeZoneRef tz = __RSRuntimeCreateInstance(RSAllocatorSystemDefault, _RSTimeZoneTypeID, sizeof(struct __RSTimeZone) - sizeof(struct __RSRuntimeBase));
    __RSTimeZoneSetName(tz, zoneName);
    return tz;
}

RSExport RSTimeZoneRef RSTimeZoneCreateWithName(RSAllocatorRef allocator, RSStringRef timeZoneName)
{
    if (__RSTimeZoneKnownNames)
    {
        RSIndex idx = 0;
        idx = RSArrayIndexOfObject(__RSTimeZoneKnownNames, timeZoneName);
        if (idx != -1)
        {
            RSTimeZoneRef tz = __RSTimeZoneCreateInstance(allocator, timeZoneName);
            
            return tz;
        }
    }
    return nil;
}

RSExport RSStringRef RSTimeZoneGetName(RSTimeZoneRef timeZone)
{
    __RSTimeZoneAvailable(timeZone);
    return (__RSTimeZoneGetName(timeZone));
}

RSExport RSArrayRef RSTimeZoneCopyKnownNames()
{
    return __RSTimeZoneKnownNamesArrayCreate();
}


RSExport RSTimeZoneRef RSTimeZoneCopySystem()
{
    RSTimeZoneLockSystem;
    if (likely(__RSTimeZoneSystem))
    {
        RSTimeZoneUnlockSystem;
        return __RSTimeZoneSystem;
    }
    RSTimeZoneUnlockSystem;
    
    RSTimeZoneRef result = nil;
    RSCBuffer tzenv = __RSRuntimeGetEnvironment("TZFILE");
    if (unlikely(tzenv))
    {
        RSStringRef name = RSStringCreateWithCString(RSAllocatorSystemDefault, tzenv, RSStringEncodingASCII);
        result = RSTimeZoneCreateWithName(RSAllocatorSystemDefault, name);
        RSRelease(name);
        RSTimeZoneLockSystem;
        if (likely(__RSTimeZoneSystem == nil))
        {
            __RSTimeZoneSystem = result;
            __RSRuntimeSetInstanceSpecial(__RSTimeZoneSystem, YES);
        }
        RSTimeZoneUnlockSystem;
        return result;
    }
    tzenv = __RSRuntimeGetEnvironment("TZ");
    if (unlikely(tzenv))
    {
        RSStringRef name = RSStringCreateWithCString(RSAllocatorSystemDefault, tzenv, RSStringEncodingASCII);
        result = RSTimeZoneCreateWithName(RSAllocatorSystemDefault, name);
        RSRelease(name);
        RSTimeZoneLockSystem;
        if (likely(__RSTimeZoneSystem == nil))
        {
            __RSTimeZoneSystem = result;
            __RSRuntimeSetInstanceSpecial(__RSTimeZoneSystem, YES);
        }
        RSTimeZoneUnlockSystem;

        return result;
    }
    char linkbuf[RSMaxPathSize];
    size_t ret = readlink(TZZONELINK, linkbuf, sizeof(linkbuf));
    if (likely(0 < ret))
    {
        RSStringRef name = nil;
        linkbuf[ret] = '\0';
        if (strncmp(linkbuf, TZZONEINFO, sizeof(TZZONEINFO) - 1) == 0)
        {
            //linkbuf[strlen(linkbuf) - sizeof(TZZONEINFO) + 1] = 0;
            
            name = RSStringCreateWithCString(RSAllocatorSystemDefault, linkbuf + sizeof(TZZONEINFO) - 1, RSStringEncodingUTF8);
        }
        else
        {
            name = RSStringCreateWithCString(RSAllocatorSystemDefault, linkbuf, RSStringEncodingUTF8);//RSStringCreateWithBytes(RSAllocatorSystemDefault, (uint8_t *)linkbuf, strlen(linkbuf), RSStringEncodingUTF8, NO);
        }
        result = RSTimeZoneCreateWithName(RSAllocatorSystemDefault, name);
        RSRelease(name);
        if (result)
        {
            RSTimeZoneLockSystem;
            if (likely(__RSTimeZoneSystem == nil))
            {
                __RSTimeZoneSystem = result;
                __RSRuntimeSetInstanceSpecial(__RSTimeZoneSystem, YES);
            }
            RSTimeZoneUnlockSystem;
        }
        return result;
    }
    return result;
}

RSExport RSTimeZoneRef RSTimeZoneCopyDefault()
{
    return RSTimeZoneCopySystem();
}

RSExport RSTimeInterval RSTimeZoneGetSecondsFromGMT(RSTimeZoneRef zone, RSTimeInterval time)
{
    if (zone == nil) return 0 + time;
    __RSTimeZoneAvailable(zone);
    if (__RSTimeZoneKnownNames)
    {
        RSIndex idx = 0;
        idx = RSArrayIndexOfObject(__RSTimeZoneKnownNames, __RSTimeZoneGetName(zone));
        if (idx != RSNotFound && idx < __RSPrivateTimeZoneNameCount)
        {
            RSTimeInterval ss = __RSPrivateTimeZoneInfo[idx].secondsFromGMT;
            return ss + time;
        }
    }
    RSStringRef name = RSTimeZoneGetName(zone);
    RSTimeInterval result = 0.0f;
    for (RSIndex idx = 0; idx < __RSPrivateTimeZoneNameCount; idx++)
    {
        if (0 == strcmp(RSStringGetCStringPtr(name, RSStringEncodingASCII), __RSPrivateTimeZoneInfo[idx].timeZoneName))
        {
            result = __RSPrivateTimeZoneInfo[idx].secondsFromGMT;
            break;
        }
    }
    RSRelease(name);
    return result;
}
