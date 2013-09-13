//
//  RSDate.c
//  RSCoreFoundation
//
//  Created by RetVal on 11/12/12.
//  Copyright (c) 2012 RetVal. All rights reserved.
//

#include <stdio.h>
#include <RSCoreFoundation/RSDate.h>
#include <RSCoreFoundation/RSTimeZone.h>
#include <math.h>
#if DEPLOYMENT_TARGET_MACOSX || DEPLOYMENT_TARGET_EMBEDDED || DEPLOYMENT_TARGET_EMBEDDED_MINI || DEPLOYMENT_TARGET_LINUX
#include <sys/time.h>
#endif

#include <RSCoreFoundation/RSRuntime.h>
const RSTimeInterval RSAbsoluteTimeIntervalSince1970 = 978307200.0L;
const RSTimeInterval RSAbsoluteTimeIntervalSince1904 = 3061152000.0L;

struct	__RSTimeBase
{
	long	second		: 6	;   //2^6 = 64 , 60
	long	minutes		: 6 ;   //2^6 = 64 , 60
	long	hour		: 5 ;   //2^5 = 32 , 24
	long	day			: 5 ;   //2^5 = 32 , 31
	long	month		: 4 ;   //2^4 = 16 , 12
	long	zoneInf		: 5 ;   //2^5 = 32 , 24
	long	reserved0	: 1 ;   //
    
	long	microSecond	: 18;   //2^18 = 262144, eg: 0.
	long	year		: 10;   //2^10 = 1024 , 2012
	long	dayOfWeek	: 3 ;   //2^3 = 8 , 7
	long	RegionFormat: 1 ;   //
};

RSInline struct __RSTimeBase __RSMakeTimeBase(int year, int month, int day, int hour, int minute, int second, int microsecond)
{
    struct __RSTimeBase base;
    base.year = year;
    base.month = month;
    base.day = day;
    base.hour = hour;
    base.minutes = minute;
    base.second = second;
    base.microSecond = microsecond;
    return base;
}

// y = year, m = month, d = day
// return the day of week ( based on 0, eg: 0 is monday, 6 is sunday.)
static RSUInteger __RSDateGetDayOfWeek(RSIndex y, RSIndex m, RSIndex d)
{
	if(m == 1 || m == 2)
	{
		m += 12;
		y--;
	}
	RSUInteger iWeek = (d+2*m+3*(m+1)/5+y+y/4-y/100+y/400)%7; // do not ask me why, it just works.
	return iWeek;
}

struct __RSDate
{
    RSRuntimeBase _base;
    union
    {
        RSTimeInterval _time;
    };
};

enum {
    _RSDateMutable = 1 << 0L,  _RSDateMutableMask = _RSDateMutable,
    _RSDateTimeBase = 1 << 1L, _RSDateTimeBaseMask = _RSDateTimeBase,
};

RSInline BOOL isMutable(RSDateRef date)
{
    return (date->_base._rsinfo._rsinfo1 & _RSDateMutableMask) == _RSDateMutable;
}

RSInline void markMutable(RSDateRef date)
{
    (((RSMutableDateRef)date)->_base._rsinfo._rsinfo1 |= _RSDateMutable);
}

RSInline void unMarkMutable(RSDateRef date)
{
    (((RSMutableDateRef)date)->_base._rsinfo._rsinfo1 &= ~_RSDateMutable);
}

RSInline BOOL isTimeBase(RSDateRef date)
{
    return (date->_base._rsinfo._rsinfo1 & _RSDateTimeBaseMask) == _RSDateTimeBase;
}

RSInline BOOL isTimeInterval(RSDateRef date)
{
    return (date->_base._rsinfo._rsinfo1 & _RSDateTimeBaseMask) != _RSDateTimeBase;
}

RSInline void markTimeBase(RSDateRef date)
{
    (((RSMutableDateRef)date)->_base._rsinfo._rsinfo1) |= _RSDateTimeBase;
}

RSInline void markTimeInterval(RSDateRef date)
{
    (((RSMutableDateRef)date)->_base._rsinfo._rsinfo1) &= ~_RSDateTimeBase;
}

//RSInline struct __RSTimeBase __RSDateGetTimeBase(RSDateRef date)
//{
//    return date->_time;
//}
//
//RSInline void __RSDateSetTimeBase(RSDateRef date, struct __RSTimeBase _time)
//{
//    ((RSMutableDateRef)date)->_time = _time;
//    markTimeBase(date);
//}

RSInline RSTimeInterval __RSDateGetTimeInterval(RSDateRef date)
{
    return date->_time;
}

RSInline void __RSDateSetTimeInterval(RSDateRef date, RSTimeInterval _interval)
{
    ((RSMutableDateRef)date)->_time = _interval;
    markTimeInterval(date);
}

static const uint8_t daysInMonth[16] = {0, 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31, 0, 0, 0};
static const uint16_t daysBeforeMonth[16] = {0, 0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334, 365, 0, 0};
static const uint16_t daysAfterMonth[16] = {365, 334, 306, 275, 245, 214, 184, 153, 122, 92, 61, 31, 0, 0, 0, 0};
RSInline int32_t __RSDoubleModToInt(double d, int32_t modulus)
{
    int32_t result = (int32_t)(float)floor(d - floor(d / modulus) * modulus);
    if (result < 0) result += modulus;
    return result;
}

RSInline double __RSDoubleMod(double d, int32_t modulus)
{
    double result = d - floor(d / modulus) * modulus;
    if (result < 0.0) result += (double)modulus;
    return result;
}

RSInline bool isleap(int64_t year)
{
    int64_t y = (year + 1) % 400;	/* correct to nearest multiple-of-400 year, then find the remainder */
    if (y < 0) y = -y;
    return (0 == (y & 3) && 100 != y && 200 != y && 300 != y);
}

/* year arg is absolute year; Gregorian 2001 == year 0; 2001/1/1 = absolute date 0 */
RSInline uint8_t __RSDaysInMonth(int8_t month, int64_t year, bool leap)
{
    return daysInMonth[month] + (2 == month && leap);
}

/* year arg is absolute year; Gregorian 2001 == year 0; 2001/1/1 = absolute date 0 */
RSInline uint16_t __RSDaysBeforeMonth(int8_t month, int64_t year, bool leap)
{
    return daysBeforeMonth[month] + (2 < month && leap);
}

/* year arg is absolute year; Gregorian 2001 == year 0; 2001/1/1 = absolute date 0 */
RSInline uint16_t __RSDaysAfterMonth(int8_t month, int64_t year, bool leap)
{
    return daysAfterMonth[month] + (month < 2 && leap);
}

/* year arg is absolute year; Gregorian 2001 == year 0; 2001/1/1 = absolute date 0 */
static void __RSYMDFromAbsolute(int64_t absolute, int64_t *year, int8_t *month, int8_t *day)
{
    int64_t b = absolute / 146097; // take care of as many multiples of 400 years as possible
    int64_t y = b * 400;
    uint16_t ydays;
    absolute -= b * 146097;
    while (absolute < 0)
    {
        y -= 1;
        absolute += __RSDaysAfterMonth(0, y, isleap(y));
    }
    /* Now absolute is non-negative days to add to year */
    ydays = __RSDaysAfterMonth(0, y, isleap(y));
    while (ydays <= absolute)
    {
        y += 1;
        absolute -= ydays;
        ydays = __RSDaysAfterMonth(0, y, isleap(y));
    }
    /* Now we have year and days-into-year */
    if (year) *year = y;
    if (month || day)
    {
        int8_t m = absolute / 33 + 1; /* search from the approximation */
        bool leap = isleap(y);
        while (__RSDaysBeforeMonth(m + 1, y, leap) <= absolute) m++;
        if (month) *month = m;
        if (day) *day = absolute - __RSDaysBeforeMonth(m, y, leap) + 1;
    }
}

/* year arg is absolute year; Gregorian 2001 == year 0; 2001/1/1 = absolute date 0 */
static double __RSAbsoluteFromYMD(int64_t year, int8_t month, int8_t day)
{
    double absolute = 0.0;
    int64_t idx;
    int64_t b = year / 400; // take care of as many multiples of 400 years as possible
    absolute += b * 146097.0;
    year -= b * 400;
    if (year < 0)
    {
        for (idx = year; idx < 0; idx++)
            absolute -= __RSDaysAfterMonth(0, idx, isleap(idx));
    }
    else
    {
        for (idx = 0; idx < year; idx++)
            absolute += __RSDaysAfterMonth(0, idx, isleap(idx));
    }
    /* Now add the days into the original year */
    absolute += __RSDaysBeforeMonth(month, year, isleap(year)) + day - 1;
    return absolute;
}

static  void __RSDateClassInit(RSTypeRef obj)
{
    return;
}

static  RSTypeRef __RSDateClassCopy(RSAllocatorRef allocator, RSTypeRef rs, BOOL mutableCopy)
{
    RSAbsoluteTime time = RSDateGetAbsoluteTime(rs);
    RSDateRef copy = RSDateCreate(allocator, time);
    return copy;
}
static  BOOL __RSDateClassEqual(RSTypeRef obj1, RSTypeRef obj2)
{
    RSDateRef date1 = (RSDateRef)obj1;
    RSDateRef date2 = (RSDateRef)obj2;
    if (date1->_time == date2->_time) return YES;
    return NO;
}

static  RSHashCode __RSDateClassHash(RSTypeRef obj)
{
    RSMutableDateRef date = (RSMutableDateRef)obj;
    return (RSHashCode)(date->_time);
}

static  RSStringRef __RSDateClassDescription(RSTypeRef obj)
{
    RSDateRef date = (RSDateRef)obj;
    RSGregorianDate gd = RSAbsoluteTimeGetGregorianDate(date->_time, nil);
        // %04d-%02d-%02d %02d:%02d:%02d.%03d
    RSStringRef description = RSStringCreateWithFormat(RSAllocatorSystemDefault, RSSTR("%04d-%02d-%02d %02d:%02d:%f"),
                                                       gd.year,gd.month,gd.day,
                                                       gd.hour,gd.minute,gd.second);
    return description;
}

struct  __RSRuntimeClass __RSDateClass =
{
    _RSRuntimeScannedObject,
    "RSDate",
    __RSDateClassInit,
    __RSDateClassCopy,
    NULL,
    __RSDateClassEqual,
    __RSDateClassHash,
    __RSDateClassDescription,
    NULL,
    NULL,
};
static  void __RSDateAvailable(RSDateRef date)
{
    if (date == nil) HALTWithError(RSInvalidArgumentException, "date is nil");
    if (RSGetTypeID(date) != RSDateGetTypeID()) HALTWithError(RSInvalidArgumentException, "date is not kind of RSDate");
    return;
}
static  RSTypeID __RSDateTypeID = _RSRuntimeNotATypeID;

RSPrivate void __RSDateInitialize()
{
    __RSDateTypeID = __RSRuntimeRegisterClass(&__RSDateClass);
    __RSRuntimeSetClassTypeID(&__RSDateClass, __RSDateTypeID);
}

RSExport RSTypeID RSDateGetTypeID()
{
    return __RSDateTypeID;
}

RSExport RSAbsoluteTime RSAbsoluteTimeGetCurrent()
{
    RSAbsoluteTime ret;
    struct timeval tv;
    gettimeofday(&tv, NULL);
    ret = (RSTimeInterval)tv.tv_sec - RSAbsoluteTimeIntervalSince1970;
    ret += (1.0E-6 * (RSTimeInterval)tv.tv_usec);
    return ret;
}

RSExport RSAbsoluteTime RSGregorianDateGetAbsoluteTime(RSGregorianDate gdate, RSTimeZoneRef tz)
{
    RSAbsoluteTime at;
    at = 86400.0 * __RSAbsoluteFromYMD(gdate.year - 2001, gdate.month, gdate.day);
    at += 3600.0 * gdate.hour + 60.0 * gdate.minute + gdate.second;
#if DEPLOYMENT_TARGET_MACOSX || DEPLOYMENT_TARGET_EMBEDDED || DEPLOYMENT_TARGET_WINDOWS || DEPLOYMENT_TARGET_LINUX
//    if (NULL != tz) {
//        __RSGenericValidateType(tz, RSTimeZoneGetTypeID());
//    }
    RSTimeInterval offset0, offset1;
    if (NULL != tz)
    {
        offset0 = RSTimeZoneGetSecondsFromGMT(tz, at);
        offset1 = RSTimeZoneGetSecondsFromGMT(tz, at - offset0);
        at -= offset1;
    }
#endif
    return at;
}

RSExport RSGregorianDate RSAbsoluteTimeGetGregorianDate(RSAbsoluteTime at, RSTimeZoneRef tz)
{
    RSGregorianDate gdate;
    int64_t absolute, year;
    int8_t month, day;
    RSAbsoluteTime fixedat;
#if DEPLOYMENT_TARGET_MACOSX || DEPLOYMENT_TARGET_EMBEDDED || DEPLOYMENT_TARGET_WINDOWS || DEPLOYMENT_TARGET_LINUX
//    if (NULL != tz) {
//        __RSGenericValidateType(tz, RSTimeZoneGetTypeID());
//    }
    fixedat = at + (NULL != tz ? RSTimeZoneGetSecondsFromGMT(tz, 0) : 0.0);
#else
    fixedat = at;
#endif
    absolute = (int64_t)floor(fixedat / 86400.0);
    __RSYMDFromAbsolute(absolute, &year, &month, &day);
    if (INT32_MAX - 2001 < year) year = INT32_MAX - 2001;
    gdate.year = (RSBit8)year + 2001;
    gdate.month = month;
    gdate.day = day;
    gdate.hour = __RSDoubleModToInt(floor(fixedat / 3600.0), 24);
    gdate.minute = __RSDoubleModToInt(floor(fixedat / 60.0), 60);
    gdate.second = __RSDoubleMod(fixedat, 60);
    if (0.0 == gdate.second) gdate.second = 0.0;	// stomp out possible -0.0
    return gdate;
}

static   RSDateRef __RSDateCreateInstance(RSAllocatorRef allocator, RSAbsoluteTime time)
{
    RSDateRef date = (RSDateRef)__RSRuntimeCreateInstance(allocator, RSDateGetTypeID(), sizeof(struct __RSDate) - sizeof(struct __RSRuntimeBase));
    ((struct __RSDate*)date)->_time = time;
    return date;
}
RSExport RSDateRef RSDateCreate(RSAllocatorRef allocator, RSAbsoluteTime time)
{
    RSDateRef date = __RSDateCreateInstance(allocator, time);
    return date;
}

RSExport RSDateRef RSDateGetCurrent(RSAllocatorRef allocator)
{
    return RSAutorelease(RSDateCreate(allocator, RSAbsoluteTimeGetCurrent()));
}

RSExport RSAbsoluteTime RSDateGetAbsoluteTime(RSDateRef date)
{
    __RSDateAvailable(date);
    return __RSDateGetTimeInterval(date);
}

RSExport RSTimeInterval RSDateGetTimeIntervalSinceDate(RSDateRef date, RSDateRef otherDate)
{
    __RSDateAvailable(date);
    __RSDateAvailable(otherDate);
    return date->_time - otherDate->_time;
}

RSExport RSComparisonResult RSDateCompare(RSDateRef date, RSDateRef otherDate, void *context)
{
    __RSDateAvailable(date);
    __RSDateAvailable(otherDate);
    if (date->_time < otherDate->_time) return RSCompareLessThan;
    if (date->_time > otherDate->_time) return RSCompareGreaterThan;
    return RSCompareEqualTo;
}

RSExport RSBitU32 RSAbsoluteTimeGetDayOfWeek(RSAbsoluteTime at, RSTimeZoneRef tz)
{
    int64_t absolute;
    RSAbsoluteTime fixedat;
#if DEPLOYMENT_TARGET_MACOSX || DEPLOYMENT_TARGET_EMBEDDED || DEPLOYMENT_TARGET_WINDOWS || DEPLOYMENT_TARGET_LINUX
    if (tz) {
        if (RSTimeZoneGetTypeID() != RSGetTypeID(tz)) HALTWithError(RSInvalidArgumentException, "timezone is not kind of RSTimeZone");
    }
    fixedat = at + (NULL != tz ? RSTimeZoneGetSecondsFromGMT(tz, at) : 0.0);
#else
    fixedat = at;
#endif
    absolute = (int64_t)floor(fixedat / 86400.0);
    return (absolute < 0) ? ((absolute + 1) % 7 + 7) : (absolute % 7 + 1); /* Monday = 1, etc. */
}

RSExport RSBitU32 RSAbsoluteTimeGetDayOfYear(RSAbsoluteTime at, RSTimeZoneRef tz)
{
    RSAbsoluteTime fixedat;
    int64_t absolute, year;
    int8_t month, day;
#if DEPLOYMENT_TARGET_MACOSX || DEPLOYMENT_TARGET_EMBEDDED || DEPLOYMENT_TARGET_WINDOWS || DEPLOYMENT_TARGET_LINUX
    if (tz) {
        if (RSTimeZoneGetTypeID() != RSGetTypeID(tz)) HALTWithError(RSInvalidArgumentException, "timezone is not kind of RSTimeZone");
    }
    fixedat = at + (NULL != tz ? RSTimeZoneGetSecondsFromGMT(tz, at) : 0.0);
#else
    fixedat = at;
#endif
    absolute = (int64_t)floor(fixedat / 86400.0);
    __RSYMDFromAbsolute(absolute, &year, &month, &day);
    return __RSDaysBeforeMonth(month, year, isleap(year)) + day;
}

RSExport RSBitU32 RSAbsoluteTimeGetWeekOfYear(RSAbsoluteTime at, RSTimeZoneRef tz)
{
    int64_t absolute, year;
    int8_t month, day;
    RSAbsoluteTime fixedat;
#if DEPLOYMENT_TARGET_MACOSX || DEPLOYMENT_TARGET_EMBEDDED || DEPLOYMENT_TARGET_WINDOWS || DEPLOYMENT_TARGET_LINUX
    if (tz) {
        if (RSTimeZoneGetTypeID() != RSGetTypeID(tz)) HALTWithError(RSInvalidArgumentException, "timezone is not kind of RSTimeZone");
    }
    fixedat = at + (NULL != tz ? RSTimeZoneGetSecondsFromGMT(tz, at) : 0.0);
#else
    fixedat = at;
#endif
    absolute = (int64_t)floor(fixedat / 86400.0);
    __RSYMDFromAbsolute(absolute, &year, &month, &day);
    double absolute0101 = __RSAbsoluteFromYMD(year, 1, 1);
    int64_t dow0101 = __RSDoubleModToInt(absolute0101, 7) + 1;
    /* First three and last three days of a year can end up in a week of a different year */
    if (1 == month && day < 4)
    {
        if ((day < 4 && 5 == dow0101) || (day < 3 && 6 == dow0101) || (day < 2 && 7 == dow0101)) {
            return 53;
        }
    }
    if (12 == month && 28 < day)
    {
        double absolute20101 = __RSAbsoluteFromYMD(year + 1, 1, 1);
        int64_t dow20101 = __RSDoubleModToInt(absolute20101, 7) + 1;
        if ((28 < day && 4 == dow20101) || (29 < day && 3 == dow20101) || (30 < day && 2 == dow20101))
        {
            return 1;
        }
    }
    /* Days into year, plus a week-shifting correction, divided by 7. First week is 1. */
    return (__RSDaysBeforeMonth(month, year, isleap(year)) + day + (dow0101 - 11) % 7 + 2) / 7 + 1;
}

RSExport RSUInteger RSDateGetDayOfWeek(RSDateRef date)
{
    __RSDateAvailable(date);
    RSGregorianDate gd = RSAbsoluteTimeGetGregorianDate(RSDateGetAbsoluteTime(date), nil);
	return __RSDateGetDayOfWeek(gd.year, gd.month, gd.day);
}