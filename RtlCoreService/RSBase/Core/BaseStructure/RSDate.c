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

RSPrivate double __RSTSRRate = 0.0;
static double __RS1_TSRRate = 0.0;

RSPrivate int64_t __RSTimeIntervalToTSR(RSTimeInterval ti) {
    if ((ti * __RSTSRRate) > INT64_MAX / 2) return (INT64_MAX / 2);
    return (int64_t)(ti * __RSTSRRate);
}

RSPrivate RSTimeInterval __RSTSRToTimeInterval(int64_t tsr) {
    return (RSTimeInterval)((double)tsr * __RS1_TSRRate);
}

#include <dispatch/dispatch.h>
#include <mach/mach_time.h>
RSPrivate RSTimeInterval __RSTimeIntervalUntilTSR(uint64_t tsr) {
    RSDateGetTypeID();
    uint64_t now = mach_absolute_time();
    if (tsr >= now) {
        return __RSTSRToTimeInterval(tsr - now);
    } else {
        return -__RSTSRToTimeInterval(now - tsr);
    }
}
// Technically this is 'TSR units' not a strict 'TSR' absolute time
RSPrivate uint64_t __RSTSRToNanoseconds(uint64_t tsr) {
    double tsrInNanoseconds = floor(tsr * __RS1_TSRRate * NSEC_PER_SEC);
    uint64_t ns = (uint64_t)tsrInNanoseconds;
    return ns;
}

RSPrivate dispatch_time_t __RSTSRToDispatchTime(uint64_t tsr) {
    uint64_t tsrInNanoseconds = __RSTSRToNanoseconds(tsr);
    
    // It's important to clamp this value to INT64_MAX or it will become interpreted by dispatch_time as a relative value instead of absolute time
    if (tsrInNanoseconds > INT64_MAX - 1) tsrInNanoseconds = INT64_MAX - 1;
    
    // 2nd argument of dispatch_time is a value in nanoseconds, but tsr does not equal nanoseconds on all platforms.
    return dispatch_time(1, (int64_t)tsrInNanoseconds);
}


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
    int64_t y = (year + 1) % 400;
    if (y < 0) y = -y;
    return (0 == (y & 3) && 100 != y && 200 != y && 300 != y);
}

RSInline uint8_t __RSDaysInMonth(int8_t month, int64_t year, bool leap)
{
    return daysInMonth[month] + (2 == month && leap);
}

RSInline uint16_t __RSDaysBeforeMonth(int8_t month, int64_t year, bool leap)
{
    return daysBeforeMonth[month] + (2 < month && leap);
}

RSInline uint16_t __RSDaysAfterMonth(int8_t month, int64_t year, bool leap)
{
    return daysAfterMonth[month] + (month < 2 && leap);
}

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
    0,
    "RSDate",
    __RSDateClassInit,
    __RSDateClassCopy,
    nil,
    __RSDateClassEqual,
    __RSDateClassHash,
    __RSDateClassDescription,
    nil,
    nil,
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
#if DEPLOYMENT_TARGET_MACOSX || DEPLOYMENT_TARGET_EMBEDDED || DEPLOYMENT_TARGET_EMBEDDED_MINI
    struct mach_timebase_info info;
    mach_timebase_info(&info);
    __RSTSRRate = (1.0E9 / (double)info.numer) * (double)info.denom;
    __RS1_TSRRate = 1.0 / __RSTSRRate;
#elif DEPLOYMENT_TARGET_WINDOWS
    LARGE_INTEGER freq;
    if (!QueryPerformanceFrequency(&freq)) {
        HALT;
    }
    __RSTSRRate = (double)freq.QuadPart;
    __RS1_TSRRate = 1.0 / __RSTSRRate;
#elif DEPLOYMENT_TARGET_LINUX
    struct timespec res;
    if (clock_getres(CLOCK_MONOTONIC, &res) != 0) {
        HALT;
    }
    __RSTSRRate = res.tv_sec + (1000000000 * res.tv_nsec);
    __RS1_TSRRate = 1.0 / __RSTSRRate;
#else
#error Unable to initialize date
#endif
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
    gettimeofday(&tv, nil);
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
//    if (nil != tz) {
//        __RSGenericValidateType(tz, RSTimeZoneGetTypeID());
//    }
    RSTimeInterval offset0, offset1;
    if (nil != tz)
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
//    if (nil != tz) {
//        __RSGenericValidateType(tz, RSTimeZoneGetTypeID());
//    }
    fixedat = at + (nil != tz ? RSTimeZoneGetSecondsFromGMT(tz, 0) : 0.0);
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
    fixedat = at + (nil != tz ? RSTimeZoneGetSecondsFromGMT(tz, at) : 0.0);
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
    fixedat = at + (nil != tz ? RSTimeZoneGetSecondsFromGMT(tz, at) : 0.0);
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
    fixedat = at + (nil != tz ? RSTimeZoneGetSecondsFromGMT(tz, at) : 0.0);
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

RSInline BOOL __dateCheckRange(int date, int min, int max) {
    if (date < min || date > max) {
        return NO;
    }
    return YES;
}

static BOOL __checkIfNumberString(const UniChar* numStr, RSUInteger length) {
    UniChar* str = (UniChar*)numStr;
    for (int i = 0; i < length; str++, i++) {
        switch (*str) {
            case '0':
            case '1':
            case '2':
            case '3':
            case '4':
            case '5':
            case '6':
            case '7':
            case '8':
            case '9':
                break;
            default:
                return NO;
        }
    }
    return YES;
}

static BOOL __unichar2i(UniChar *ptr, RSUInteger len, RSIndex *n) {
    if (!ptr || !n || 0 == len) return NO;
    BOOL sig = 1;
    *n = 0;
    RSUInteger idx = 0;
    if (ptr[0] == '-') {
        sig = -1;
        idx = 1;
    }
    if (!__checkIfNumberString(ptr + idx, len - idx)) return NO;
    for (; idx < len; idx++) {
        *n *= 10;
        *n += ptr[idx] - '0';
    }
    *n *= sig;
    return YES;
}

RSExport RSDateRef RSDateCreateWithString(RSAllocatorRef allocator, RSStringRef value) {
    const int dateLength = 20;
    
    RSDateRef _v = nil;
    //%04d-%02d-%02dT%02d:%02d:%02dZ
    //4     +1  +2      +1  +2      +1  +2      +1  +2      +1  +2      +1
    //%04d  -   %02d    -   %02d    T   %02d    :   %02d    :   %02d    Z
    //5,3,3,3,3,3 = 20
    if (dateLength == RSStringGetLength(value))
    {
        UniChar ptr[6] = {0};
        ptr[0] = RSStringGetCharacterAtIndex(value, 5 - 1); //-
        ptr[1] = RSStringGetCharacterAtIndex(value, 8 - 1); //-
        ptr[2] = RSStringGetCharacterAtIndex(value, 11 - 1);//T
        ptr[3] = RSStringGetCharacterAtIndex(value, 14 - 1);//:
        ptr[4] = RSStringGetCharacterAtIndex(value, 17 - 1);//:
        ptr[5] = RSStringGetCharacterAtIndex(value, 20 - 1);//Z
        if ((ptr[0] == ptr[1]) &&
            (ptr[1] == '-') &&
            (ptr[2] == 'T') &&
            (ptr[3] == ptr[4]) &&
            (ptr[4] == ':') &&
            (ptr[5] == 'Z'))
        {
#if 0
            UTF8Char year[5] = {0};
            UTF8Char month[3] = {0};
            UTF8Char day[3] = {0};
            UTF8Char hour[3] = {0};
            UTF8Char minute[3] = {0};
            UTF8Char second[3] = {0};
            RSStringGetUTF8Characters(value, RSMakeRange(0, 4), year);
            RSStringGetUTF8Characters(value, RSMakeRange(5, 2), month);
            RSStringGetUTF8Characters(value, RSMakeRange(8, 2), day);
            RSStringGetUTF8Characters(value, RSMakeRange(11, 2), hour);
            RSStringGetUTF8Characters(value, RSMakeRange(14, 2), minute);
            RSStringGetUTF8Characters(value, RSMakeRange(17, 2), second);
            
            RSGregorianDate gd = {0};
            gd.year = atoi((const char*)year);
            gd.month= atoi((const char*)month);
            gd.day  = atoi((const char*)day);
            gd.hour = atoi((const char*)hour);
            gd.minute = atoi((const char*)minute);
            gd.second = atoi((const char*)second);
#else
            RSGregorianDate gd = {0};
            UniChar payload[4] = {0};
            RSIndex tmp = 0;
            
            RSStringGetCharacters(value, RSMakeRange(0, 4), payload);
            if (!__unichar2i(payload, 4, &tmp)) return nil;
            gd.year = (RSBitU32)tmp;
            
            memset(payload, 0, sizeof(payload));
            
            RSStringGetCharacters(value, RSMakeRange(5, 2), payload);
            if (!__unichar2i(payload, 2, &tmp)) return nil;
            gd.month = (RSBitU8)tmp;
            if (__dateCheckRange(gd.month, 1, 12) == NO) return nil;
            
            RSStringGetCharacters(value, RSMakeRange(8, 2), payload);
            if (!__unichar2i(payload, 2, &tmp)) return nil;
            gd.day = (RSBitU8)tmp;
            
            int maxm = 0;
            switch (gd.month)
            {
                case 1:
                case 3:
                case 5:
                case 7:
                case 8:
                case 10:
                case 12:
                    maxm = 31;
                    break;
                case 2:
                    maxm = 28;
                    break;
                default:
                    maxm = 30;
                    break;
            }
            if ((gd.year % 400 == 0) || ((gd.year % 100 != 0) && (gd.year % 4 == 0)))
            {
                maxm++;
            }
            if (maxm > 32) HALTWithError(RSInvalidArgumentException, "maxm > 32");
            if (__dateCheckRange(gd.day, 1, maxm) == NO) return nil;
            
            RSStringGetCharacters(value, RSMakeRange(11, 2), payload);
            if (!__unichar2i(payload, 2, &tmp)) return nil;
            gd.hour = (RSBitU8)tmp;
            if (__dateCheckRange(gd.day, 0, 59) == NO) return nil;
            
            RSStringGetCharacters(value, RSMakeRange(14, 2), payload);
            if (!__unichar2i(payload, 2, &tmp)) return nil;
            gd.minute = (RSBitU8)tmp;
            if (__dateCheckRange(gd.day, 0, 59) == NO) return nil;
            
            RSStringGetCharacters(value, RSMakeRange(17, 2), payload);
            if (!__unichar2i(payload, 2, &tmp)) return nil;
            gd.second = (RSBitU8)tmp;
            if (__dateCheckRange(gd.day, 0, 59) == NO) return nil;
#endif
            _v = RSDateCreate(allocator, RSGregorianDateGetAbsoluteTime(gd, RSTimeZoneCopySystem()));
        }
    }
    return _v;
}

BOOL RSGregorianDateIsValid(RSGregorianDate gdate, RSOptionFlags unitFlags) {
    if ((unitFlags & RSGregorianUnitsYears) && (gdate.year <= 0)) return false;
    if ((unitFlags & RSGregorianUnitsMonths) && (gdate.month < 1 || 12 < gdate.month)) return false;
    if ((unitFlags & RSGregorianUnitsDays) && (gdate.day < 1 || 31 < gdate.day)) return false;
    if ((unitFlags & RSGregorianUnitsHours) && (gdate.hour < 0 || 23 < gdate.hour)) return false;
    if ((unitFlags & RSGregorianUnitsMinutes) && (gdate.minute < 0 || 59 < gdate.minute)) return false;
    if ((unitFlags & RSGregorianUnitsSeconds) && (gdate.second < 0.0 || 60.0 <= gdate.second)) return false;
    if ((unitFlags & RSGregorianUnitsDays) && (unitFlags & RSGregorianUnitsMonths) && (unitFlags & RSGregorianUnitsYears) && (__RSDaysInMonth(gdate.month, gdate.year - 2001, isleap(gdate.year - 2001)) < gdate.day)) return false;
    return true;
}
