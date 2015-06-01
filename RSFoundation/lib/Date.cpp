//
//  Date.cpp
//  RSCoreFoundation
//
//  Created by closure on 5/27/15.
//  Copyright (c) 2015 RetVal. All rights reserved.
//

#include <RSFoundation/Date.h>

#include <math.h>

#if DEPLOYMENT_TARGET_MACOSX || DEPLOYMENT_TARGET_EMBEDDED || DEPLOYMENT_TARGET_EMBEDDED_MINI || DEPLOYMENT_TARGET_LINUX
#include <sys/time.h>
#endif

#if DEPLOYMENT_TARGET_MACOSX 
#include <mach/mach_time.h>
#endif

namespace RSFoundation {
    namespace Basic {
        namespace {
            constexpr static const RSUInt8 daysInMonth[16] = {0, 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31, 0, 0, 0};
            constexpr static const RSUInt16 daysBeforeMonth[16] = {0, 0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334, 365, 0, 0};
            constexpr static const RSUInt16 daysAfterMonth[16] = {365, 334, 306, 275, 245, 214, 184, 153, 122, 92, 61, 31, 0, 0, 0, 0};
            
            RSFoundationInline RSInt32 __mod2Int(Date::AbsoluteTime d, RSInt32 modulus) {
                RSInt32 result = (RSInt32)(Date::AbsoluteTime)floor(d - floor(d / modulus) * modulus);
                if (result < 0) result += modulus;
                return result;
            }
            
            RSFoundationInline Date::AbsoluteTime __doubleMod(Date::AbsoluteTime d, RSInt32 modulus) {
                Date::AbsoluteTime result = d - floor(d / modulus) * modulus;
                if (result < 0.0) result += (Date::AbsoluteTime)modulus;
                return result;
            }
            
            RSFoundationInline bool isleap(RSInt64 year) {
                RSInt64 y = (year + 1) % 400;
                if (y < 0) y = -y;
                return (0 == (y & 3) && 100 != y && 200 != y && 300 != y);
            }
            
            RSFoundationInline RSUInt8 __RSDaysInMonth(RSUInt8 month, RSInt64 year, bool leap) {
                return daysInMonth[month] + (2 == month && leap);
            }
            
            RSFoundationInline RSUInt16 __RSDaysBeforeMonth(RSUInt8 month, RSInt64 year, bool leap) {
                return daysBeforeMonth[month] + (2 < month && leap);
            }
            
            RSFoundationInline RSUInt16 __RSDaysAfterMonth(RSUInt8 month, RSInt64 year, bool leap) {
                return daysAfterMonth[month] + (month < 2 && leap);
            }
            
            static Date::AbsoluteTime __RSAbsoluteFromYMD(RSInt64 year, RSInt8 month, RSInt8 day) {
                Date::AbsoluteTime absolute = 0.0;
                RSInt64 idx;
                RSInt64 b = year / 400; // take care of as many multiples of 400 years as possible
                absolute += b * 146097.0;
                year -= b * 400;
                if (year < 0) {
                    for (idx = year; idx < 0; idx++)
                        absolute -= __RSDaysAfterMonth(0, idx, isleap(idx));
                } else {
                    for (idx = 0; idx < year; idx++)
                        absolute += __RSDaysAfterMonth(0, idx, isleap(idx));
                }
                /* Now add the days into the original year */
                absolute += __RSDaysBeforeMonth(month, year, isleap(year)) + day - 1;
                return absolute;
            }
            
            static void __RSYMDFromAbsolute(RSInt64 absolute, RSInt64 *year, RSInt8 *month, RSInt8 *day) {
                RSInt64 b = absolute / 146097; // take care of as many multiples of 400 years as possible
                RSInt64 y = b * 400;
                RSUInt16 ydays;
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
                    RSInt8 m = absolute / 33 + 1; /* search from the approximation */
                    bool leap = isleap(y);
                    while (__RSDaysBeforeMonth(m + 1, y, leap) <= absolute) m++;
                    if (month) *month = m;
                    if (day) *day = absolute - __RSDaysBeforeMonth(m, y, leap) + 1;
                }
            }
        }
        
        Date::AbsoluteTime Date::GetCurrentTime() {
            AbsoluteTime ret;
            struct timeval tv;
            gettimeofday(&tv, nullptr);
            ret = tv.tv_sec - Since1970;
            ret += (1.0E-6 * (AbsoluteTime)tv.tv_usec);
            return ret;
        }
        
        Date::Date() : time(GetCurrentTime()) {
            
        }
        
        Date::Date(const Date& value) : time(value.time) {
        }
        
        Date::Date(Date&& value) : time(MoveValue(value.time)) {
        }
        
        Date& Date::operator=(const Date& value) {
            time = value.time;
            return *this;
        }
        
        Date& Date::operator=(Date&& value) {
            time = MoveValue(value.time);
            return *this;
        }
        
        bool Date::IsEqualTo(const Date& a, const Date& b) {
            return a.time == b.time;
        }
        
        ComparisonResult Date::Compare(const Date& a, const Date& b) {
            return a.time > b.time ? GreaterThan :
            (a.time < b.time ? LessThan : EqualTo);
        }
        
        bool Date::operator==(const Date& value) const {
            return IsEqualTo(*this, value);
        }
        
        bool Date::operator!=(const Date& value) const {
            return !IsEqualTo(*this, value);
        }
        
        bool Date::operator>(const Date& value) const {
            return Compare(*this, value) == GreaterThan;
        }
        
        bool Date::operator>=(const Date& value) const {
            return Compare(*this, value) != LessThan;
        }
        
        bool Date::operator<(const Date& value) const {
            return Compare(*this, value) == LessThan;
        }
        
        bool Date::operator<=(const Date& value) const {
            return Compare(*this, value) != GreaterThan;
        }
        
        Date::operator bool() const {
            return time != 0;
        }
        
        Date::GregorianDate Date::GetGregorianDate() const {
            GregorianDate gdate;
            RSInt64 absolute, year;
            RSInt8 month, day;
            AbsoluteTime fixedat;
            const AbsoluteTime at = time;
#if DEPLOYMENT_TARGET_MACOSX || DEPLOYMENT_TARGET_EMBEDDED || DEPLOYMENT_TARGET_WINDOWS || DEPLOYMENT_TARGET_LINUX
            //    if (nil != tz) {
            //        __RSGenericValidateType(tz, RSTimeZoneGetTypeID());
            //    }
//            fixedat = at + (nil != tz ? RSTimeZoneGetSecondsFromGMT(tz, 0) : 0.0);
            fixedat = at;
#else
            fixedat = at;
#endif
            absolute = (RSInt64)floor(fixedat / 86400.0);
            __RSYMDFromAbsolute(absolute, &year, &month, &day);
            if (INT32_MAX - 2001 < year) year = INT32_MAX - 2001;
            gdate.year = (RSInt8)year + 2001;
            gdate.month = month;
            gdate.day = day;
            gdate.hour = __mod2Int(floor(fixedat / 3600.0), 24);
            gdate.minute = __mod2Int(floor(fixedat / 60.0), 60);
            gdate.second = __doubleMod(fixedat, 60);
            if (0.0 == gdate.second) gdate.second = 0.0;	// stomp out possible -0.0
            return MoveValue(gdate);
        }
        
        Date::AbsoluteTime Date::GetTimeSince(const Date &value) const {
            return time - value.time;
        }
        
        RSUInt32 Date::GetDayOfWeek() const {
            RSInt64 absolute;
            AbsoluteTime fixedat;
            AbsoluteTime at = time;
#if DEPLOYMENT_TARGET_MACOSX || DEPLOYMENT_TARGET_EMBEDDED || DEPLOYMENT_TARGET_WINDOWS || DEPLOYMENT_TARGET_LINUX
//            if (tz) {
//                if (RSTimeZoneGetTypeID() != RSGetTypeID(tz)) HALTWithError(RSInvalidArgumentException, "timezone is not kind of RSTimeZone");
//            }
            fixedat = at ;//+ (nil != tz ? RSTimeZoneGetSecondsFromGMT(tz, at) : 0.0);
#else
            fixedat = at;
#endif
            absolute = (RSInt64)floor(fixedat / 86400.0);
            return (absolute < 0) ? ((absolute + 1) % 7 + 7) : (absolute % 7 + 1); /* Monday = 1, etc. */
        }
        
        RSUInt32 Date::GetDayOfYear() const {
            AbsoluteTime fixedat;
            RSInt64 absolute, year;
            RSInt8 month, day;
            AbsoluteTime at = time;
#if DEPLOYMENT_TARGET_MACOSX || DEPLOYMENT_TARGET_EMBEDDED || DEPLOYMENT_TARGET_WINDOWS || DEPLOYMENT_TARGET_LINUX
//            if (tz) {
//                if (RSTimeZoneGetTypeID() != RSGetTypeID(tz)) HALTWithError(RSInvalidArgumentException, "timezone is not kind of RSTimeZone");
//            }
            fixedat = at;// + (nil != tz ? RSTimeZoneGetSecondsFromGMT(tz, at) : 0.0);
#else
            fixedat = at;
#endif
            absolute = (RSInt64)floor(fixedat / 86400.0);
            __RSYMDFromAbsolute(absolute, &year, &month, &day);
            return __RSDaysBeforeMonth(month, year, isleap(year)) + day;
        }
        
        RSUInt32 Date::GetWeekOfYear() const {
            RSInt64 absolute, year;
            RSInt8 month, day;
            AbsoluteTime fixedat;
            AbsoluteTime at = time;
#if DEPLOYMENT_TARGET_MACOSX || DEPLOYMENT_TARGET_EMBEDDED || DEPLOYMENT_TARGET_WINDOWS || DEPLOYMENT_TARGET_LINUX
//            if (tz) {
//                if (RSTimeZoneGetTypeID() != RSGetTypeID(tz)) HALTWithError(RSInvalidArgumentException, "timezone is not kind of RSTimeZone");
//            }
            fixedat = at;// + (nil != tz ? RSTimeZoneGetSecondsFromGMT(tz, at) : 0.0);
#else
            fixedat = at;
#endif
            absolute = (RSInt64)floor(fixedat / 86400.0);
            __RSYMDFromAbsolute(absolute, &year, &month, &day);
            AbsoluteTime absolute0101 = __RSAbsoluteFromYMD(year, 1, 1);
            RSInt64 dow0101 = __mod2Int(absolute0101, 7) + 1;
            /* First three and last three days of a year can end up in a week of a different year */
            if (1 == month && day < 4)
            {
                if ((day < 4 && 5 == dow0101) || (day < 3 && 6 == dow0101) || (day < 2 && 7 == dow0101)) {
                    return 53;
                }
            }
            if (12 == month && 28 < day)
            {
                AbsoluteTime absolute20101 = __RSAbsoluteFromYMD(year + 1, 1, 1);
                RSInt64 dow20101 = __mod2Int(absolute20101, 7) + 1;
                if ((28 < day && 4 == dow20101) || (29 < day && 3 == dow20101) || (30 < day && 2 == dow20101))
                {
                    return 1;
                }
            }
            /* Days into year, plus a week-shifting correction, divided by 7. First week is 1. */
            return (__RSDaysBeforeMonth(month, year, isleap(year)) + day + (dow0101 - 11) % 7 + 2) / 7 + 1;
        }
    }
}