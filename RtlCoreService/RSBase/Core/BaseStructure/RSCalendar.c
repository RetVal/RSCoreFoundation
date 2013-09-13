//
//  RSCalendar.c
//  RSCoreFoundation
//
//  Created by RetVal on 11/14/12.
//  Copyright (c) 2012 RetVal. All rights reserved.
//

#include <stdio.h>
#include <RSCoreFoundation/RSCalendar.h>
#include <RSCoreFoundation/RSDate.h>
#include <RSCoreFoundation/RSString.h>
#include <RSCoreFoundation/RSRuntime.h>
#include "ucal.h"

//RS_STRING_DECL(RSGregorianCalendar,"");

/*
 RSExport RSStringRef const RSGregorianCalendar;
 RSExport RSStringRef const RSBuddhistCalendar;
 RSExport RSStringRef const RSChineseCalendar;
 RSExport RSStringRef const RSHebrewCalendar;
 RSExport RSStringRef const RSIslamicCalendar;
 RSExport RSStringRef const RSIslamicCivilCalendar;
 RSExport RSStringRef const RSJapaneseCalendar;
 */
RS_PUBLIC_CONST_STRING_DECL(RSGregorianCalendar, "RSGregorianCalendar");
RS_PUBLIC_CONST_STRING_DECL(RSBuddhistCalendar, "RSBuddhistCalendar");
RS_PUBLIC_CONST_STRING_DECL(RSChineseCalendar, "RSChineseCalendar");
RS_PUBLIC_CONST_STRING_DECL(RSHebrewCalendar, "RSHebrewCalendar");
RS_PUBLIC_CONST_STRING_DECL(RSIslamicCalendar, "RSIslamicCalendar");
RS_PUBLIC_CONST_STRING_DECL(RSIslamicCivilCalendar, "RSIslamicCivilCalendar");
RS_PUBLIC_CONST_STRING_DECL(RSJapaneseCalendar, "RSJapaneseCalendar");

//RS_CONST_STRING_DECL(__RSCalendarMonday, "Monday");
//RS_CONST_STRING_DECL(__RSCalendarTuesday, "Tuesday");
//RS_CONST_STRING_DECL(__RSCalendarWednesday, "Wednesday");
//RS_CONST_STRING_DECL(__RSCalendarThursday, "Thursday");
//RS_CONST_STRING_DECL(__RSCalendarFriday, "Friday");
//RS_CONST_STRING_DECL(__RSCalendarSaturday, "Saturday");
//RS_CONST_STRING_DECL(__RSCalendarSunday, "Sunday");
//
//RS_CONST_STRING_DECL(__RSCalendarJanuary, "January");
//RS_CONST_STRING_DECL(__RSCalendarFebruary, "February");
//RS_CONST_STRING_DECL(__RSCalendarMarch, "March");
//RS_CONST_STRING_DECL(__RSCalendarApril, "April");
//RS_CONST_STRING_DECL(__RSCalendarMay, "May");
//RS_CONST_STRING_DECL(__RSCalendarJune, "June");
//RS_CONST_STRING_DECL(__RSCalendarJuly, "July");
//RS_CONST_STRING_DECL(__RSCalendarAugust, "August");
//RS_CONST_STRING_DECL(__RSCalendarSeptember, "September");
//RS_CONST_STRING_DECL(__RSCalendarOctober, "October");
//RS_CONST_STRING_DECL(__RSCalendarNovember, "November");
//RS_CONST_STRING_DECL(__RSCalendarDecember, "December");


struct __RSCalendar
{
    RSRuntimeBase _base;
    RSStringRef _identifier;
    RSTimeZoneRef _tz;
    UCalendar* _cal;
};

static RSTypeRef __RSCalendarClassCopy(RSAllocatorRef allocator, RSTypeRef rs, BOOL mutableCopy)
{
    RSStringRef iden = RSCalendarCopyIndentifier((RSCalendarRef)rs);
    RSCalendarRef copy = RSCalendarCreateWithIndentifier(allocator, iden);
    RSRelease(iden);
    return copy;
}

static BOOL __RSCalendarEqual(RSTypeRef obj1, RSTypeRef obj2)
{
    RSCalendarRef calendar1 = (RSCalendarRef)obj1;
    RSCalendarRef calendar2 = (RSCalendarRef)obj2;
    return RSEqual(calendar1->_identifier, calendar2->_identifier);
}

static RSHashCode __RSCalendarHash(RSTypeRef obj)
{
    RSCalendarRef calendar = (RSCalendarRef)obj;
    return RSHash(calendar->_identifier);
}

static RSStringRef __RSCalendarDescription(RSTypeRef obj)
{
    RSCalendarRef calendar = (RSCalendarRef)obj;
    RSGregorianDate gd = RSAbsoluteTimeGetGregorianDate(RSAbsoluteTimeGetCurrent(), calendar->_tz);
    return RSStringCreateWithFormat(RSAllocatorSystemDefault,
                                    RSSTR("{identifier = '%R'\nCalendar : %04d-%02d-%02d %02d:%02d:%f (%R)"),
                                    calendar->_identifier,
                                    gd.year,
                                    gd.month,
                                    gd.day,
                                    gd.hour,
                                    gd.minute,
                                    gd.second,
                                    calendar->_tz);
}

static void __RSCalendarDeallocate(RSTypeRef obj)
{
    RSCalendarRef calendar = (RSCalendarRef)obj;
    RSRelease(calendar->_identifier);
    
    RSRelease(calendar->_tz);
    if (calendar->_cal) ucal_close(calendar->_cal);
    //if (calendar->_cal) RSAllocatorDeallocate(nil, calendar->_cal);
}

static RSRuntimeClass __RSCalendarClass = {
    _RSRuntimeScannedObject,
    "RSCalendar",
    NULL,
    NULL,
    __RSCalendarDeallocate,
    __RSCalendarEqual,
    __RSCalendarHash,
    __RSCalendarDescription,
    NULL,
    NULL,
};

static RSTypeID __RSCalendarTypeID = _RSRuntimeNotATypeID;

RSPrivate void __RSCalendarInitialize()
{
    __RSCalendarTypeID = __RSRuntimeRegisterClass(&__RSCalendarClass);
    __RSRuntimeSetClassTypeID(&__RSCalendarClass, __RSCalendarTypeID);
    //RSGregorianCalendar = RSSTR("");
}
#define BUFFER_SIZE 512
RSPrivate UCalendar *__RSCalendarCreateUCalendar(RSStringRef calendarID, RSTimeZoneRef tz)
{
    const char *cstr = RSStringGetCStringPtr(calendarID, RSStringEncodingASCII);
    if (nil == cstr)
    {
        //if (RSStringGetCString(localeID, buffer, BUFFER_SIZE, RSStringEncodingASCII)) cstr = buffer;
        
    }
    if (nil == cstr)
    {
        return nil;
    }
    
    UChar ubuffer[BUFFER_SIZE];
    RSStringRef tznam = RSTimeZoneGetName(tz);
    RSIndex cnt = RSStringGetLength(tznam);
    if (BUFFER_SIZE < cnt) cnt = BUFFER_SIZE;

    RSStringGetCharacters(tznam, RSMakeRange(0, cnt), (UniChar *)ubuffer);
    
    UErrorCode status = U_ZERO_ERROR;
    UCalendar *cal = ucal_open(ubuffer, (RSBit32)cnt, cstr, UCAL_DEFAULT, &status);

    return cal;
}

static void __RSCalendarSetupCal(RSCalendarRef calendar) {
    calendar->_cal = __RSCalendarCreateUCalendar(calendar->_identifier, calendar->_tz);
}

static void __RSCalendarZapCal(RSCalendarRef calendar) {
    ucal_close(calendar->_cal);
    calendar->_cal = nil;
}

RSExport RSTypeID RSCalendarGetTypeID()
{
    return __RSCalendarTypeID;
}

static BOOL __RSCalendarAvailable(RSCalendarRef calendar)
{
    if (calendar == nil) return NO;
    if (RSGetTypeID(calendar) != RSCalendarGetTypeID()) HALTWithError(RSInvalidArgumentException, "calendar not available");
    return YES;
}

static void __RSCalendarSetTimeZone(RSCalendarRef calendar, RSTimeZoneRef tz)
{
    if (calendar->_tz) RSRelease(calendar->_tz);
    if (tz) calendar->_tz = RSRetain(tz);
    else calendar->_tz = RSTimeZoneCopySystem();
}

static void __RSCalendarSetIndentifier(RSCalendarRef calendar, RSStringRef identifier)
{
    if (calendar->_identifier) RSRelease(calendar->_identifier);
    if (identifier) calendar->_identifier = RSRetain(identifier);
    else calendar->_identifier = nil;
}

static RSCalendarRef __RSCalendarCreateInstance(RSAllocatorRef allocator, RSStringRef identifier)
{
    if (identifier != RSGregorianCalendar &&
        identifier != RSBuddhistCalendar &&
        identifier != RSJapaneseCalendar &&
        identifier != RSIslamicCalendar &&
        identifier != RSIslamicCivilCalendar &&
        identifier != RSHebrewCalendar)
    {
        //    if (identifier != RSGregorianCalendar && identifier != RSBuddhistCalendar && identifier != RSJapaneseCalendar && identifier != RSIslamicCalendar && identifier != RSIslamicCivilCalendar && identifier != RSHebrewCalendar && identifier != RSChineseCalendar) {
        if (RSEqual(RSGregorianCalendar, identifier)) identifier = RSGregorianCalendar;
        else if (RSEqual(RSBuddhistCalendar, identifier)) identifier = RSBuddhistCalendar;
        else if (RSEqual(RSJapaneseCalendar, identifier)) identifier = RSJapaneseCalendar;
        else if (RSEqual(RSIslamicCalendar, identifier)) identifier = RSIslamicCalendar;
        else if (RSEqual(RSIslamicCivilCalendar, identifier)) identifier = RSIslamicCivilCalendar;
        else if (RSEqual(RSHebrewCalendar, identifier)) identifier = RSHebrewCalendar;
        //	else if (RSEqual(RSChineseCalendar, identifier)) identifier = RSChineseCalendar;
        else return NULL;
    }
    RSCalendarRef calendar = (RSCalendarRef)__RSRuntimeCreateInstance(allocator, RSCalendarGetTypeID(), sizeof(struct __RSCalendar) - sizeof(struct __RSRuntimeBase));
    __RSCalendarSetIndentifier(calendar, identifier);
    return calendar;
}

RSExport RSCalendarRef RSCalendarCreateWithIndentifier(RSAllocatorRef allocator, RSStringRef identifier)
{
    RSCalendarRef calendar = __RSCalendarCreateInstance(allocator, identifier);
    RSCalendarSetTimeZone(calendar, RSTimeZoneCopySystem());
    return calendar;
}

RSExport RSTimeZoneRef RSCalendarGetTimeZone(RSCalendarRef calendar)
{
    if (__RSCalendarAvailable(calendar))
    {
        if(calendar->_tz)
            return (calendar->_tz);
        return nil;
    }
    return nil;
}

RSExport RSTimeZoneRef RSCalendarCopyTimeZone(RSCalendarRef calendar)
{
    if (__RSCalendarAvailable(calendar))
    {
        if(calendar->_tz)
            return RSRetain(calendar->_tz);
        return nil;
    }
    return nil;
}

RSExport void RSCalendarSetTimeZone(RSCalendarRef calendar, RSTimeZoneRef tz)
{
    if (__RSCalendarAvailable(calendar) && (RSGetTypeID(tz) == RSTimeZoneGetTypeID()))
    {
        __RSCalendarSetTimeZone(calendar, tz);
    }
}

RSExport RSStringRef RSCalendarGetIdentifier(RSCalendarRef calendar)
{
    if (__RSCalendarAvailable(calendar))
    {
        return calendar->_identifier;
    }
    return nil;
}

RSExport RSStringRef RSCalendarCopyIndentifier(RSCalendarRef calendar)
{
    if (__RSCalendarAvailable(calendar))
    {
        if (calendar->_identifier)
        {
            return RSRetain(calendar->_identifier);
        }
    }
    return nil;
}

RSExport RSIndex RSCalendarGetFirstWeekday(RSCalendarRef calendar)
{
    RS_OBJC_FUNCDISPATCHV(RSCalendarGetTypeID(), RSIndex, calendar, firstWeekday);
    __RSGenericValidInstance(calendar, __RSCalendarTypeID);
    if (!calendar->_cal) __RSCalendarSetupCal(calendar);
    if (calendar->_cal)
    {
        return ucal_getAttribute(calendar->_cal, UCAL_FIRST_DAY_OF_WEEK);
    }
    return -1;
}

RSExport void RSCalendarSetFirstWeekday(RSCalendarRef calendar, RSIndex wkdy)
{
    RS_OBJC_FUNCDISPATCHV(RSCalendarGetTypeID(), RSIndex, calendar, firstWeekday);
    __RSGenericValidInstance(calendar, __RSCalendarTypeID);
    if (!calendar->_cal) __RSCalendarSetupCal(calendar);
    if (calendar->_cal)
    {
        ucal_setAttribute(calendar->_cal, UCAL_FIRST_DAY_OF_WEEK, (RSBit32)wkdy);
    }
}

RSExport RSIndex RSCalendarGetMinimumDaysInFirstWeek(RSCalendarRef calendar)
{
    RS_OBJC_FUNCDISPATCHV(RSCalendarGetTypeID(), RSIndex, calendar, minimumDaysInFirstWeek);
    __RSGenericValidInstance(calendar, RSCalendarGetTypeID());
    if (!calendar->_cal) __RSCalendarSetupCal(calendar);
    return calendar->_cal ? ucal_getAttribute(calendar->_cal, UCAL_MINIMAL_DAYS_IN_FIRST_WEEK) : -1;
}

RSExport void RSCalendarSetMinimumDaysInFirstWeek(RSCalendarRef calendar, RSIndex mwd)
{
    RS_OBJC_FUNCDISPATCHV(RSCalendarGetTypeID(), void, calendar, setMinimumDaysInFirstWeek:mwd);
    __RSGenericValidInstance(calendar, RSCalendarGetTypeID());
    if (!calendar->_cal) __RSCalendarSetupCal(calendar);
    if (calendar->_cal) ucal_setAttribute(calendar->_cal, UCAL_MINIMAL_DAYS_IN_FIRST_WEEK, (RSBit32)mwd);
}

RSExport RSDateRef RSCalendarCopyGregorianStartDate(RSCalendarRef calendar) {
    RS_OBJC_FUNCDISPATCHV(RSCalendarGetTypeID(), RSDateRef, calendar, _gregorianStartDate);
    __RSGenericValidInstance(calendar, RSCalendarGetTypeID());
    if (!calendar->_cal) __RSCalendarSetupCal(calendar);
    UErrorCode status = U_ZERO_ERROR;
    UDate udate = calendar->_cal ? ucal_getGregorianChange(calendar->_cal, &status) : 0;
    if (calendar->_cal && U_SUCCESS(status)) {
        RSAbsoluteTime at = (double)udate / 1000.0 - RSAbsoluteTimeIntervalSince1970;
        return RSDateCreate(RSGetAllocator(calendar), at);
    }
    return NULL;
}

RSExport void RSCalendarSetGregorianStartDate(RSCalendarRef calendar, RSDateRef date) {
    RS_OBJC_FUNCDISPATCHV(RSCalendarGetTypeID(), void, calendar, _setGregorianStartDate:date);
    __RSGenericValidInstance(calendar, RSCalendarGetTypeID());
    if (date) __RSGenericValidInstance(date, RSDateGetTypeID());
    if (!calendar->_cal) __RSCalendarSetupCal(calendar);
    if (!calendar->_cal) return;
    if (!date) {
        UErrorCode status = U_ZERO_ERROR;
        UCalendar *cal = __RSCalendarCreateUCalendar(calendar->_identifier, calendar->_tz);
        UDate udate = cal ? ucal_getGregorianChange(cal, &status) : 0;
        if (cal && U_SUCCESS(status)) {
            status = U_ZERO_ERROR;
            if (calendar->_cal) ucal_setGregorianChange(calendar->_cal, udate, &status);
        }
        if (cal) ucal_close(cal);
    } else {
        RSAbsoluteTime at = RSDateGetAbsoluteTime(date);
        UDate udate = (at + RSAbsoluteTimeIntervalSince1970) * 1000.0;
        UErrorCode status = U_ZERO_ERROR;
        if (calendar->_cal) ucal_setGregorianChange(calendar->_cal, udate, &status);
    }
}


static UCalendarDateFields __RSCalendarGetICUFieldCode(RSCalendarUnit unit)
{
    switch (unit)
    {
        case RSCalendarUnitEra: return UCAL_ERA;
        case RSCalendarUnitYear: return UCAL_YEAR;
        case RSCalendarUnitMonth: return UCAL_MONTH;
        case RSCalendarUnitDay: return UCAL_DAY_OF_MONTH;
        case RSCalendarUnitHour: return UCAL_HOUR_OF_DAY;
        case RSCalendarUnitMinute: return UCAL_MINUTE;
        case RSCalendarUnitSecond: return UCAL_SECOND;
        case RSCalendarUnitWeek: return UCAL_WEEK_OF_YEAR;
        case RSCalendarUnitWeekOfYear: return UCAL_WEEK_OF_YEAR;
        case RSCalendarUnitWeekOfMonth: return UCAL_WEEK_OF_MONTH;
        case RSCalendarUnitYearForWeekOfYear: return UCAL_YEAR_WOY;
        case RSCalendarUnitWeekday: return UCAL_DAY_OF_WEEK;
        case RSCalendarUnitWeekdayOrdinal: return UCAL_DAY_OF_WEEK_IN_MONTH;
    }
    return (UCalendarDateFields)-1;
}

static UCalendarDateFields __RSCalendarGetICUFieldCodeFromChar(char ch) {
    switch (ch) {
        case 'G': return UCAL_ERA;
        case 'y': return UCAL_YEAR;
        case 'M': return UCAL_MONTH;
        case 'd': return UCAL_DAY_OF_MONTH;
        case 'h': return UCAL_HOUR;
        case 'H': return UCAL_HOUR_OF_DAY;
        case 'm': return UCAL_MINUTE;
        case 's': return UCAL_SECOND;
        case 'S': return UCAL_MILLISECOND;
        case 'w': return UCAL_WEEK_OF_YEAR;
        case 'W': return UCAL_WEEK_OF_MONTH;
        case 'Y': return UCAL_YEAR_WOY;
        case 'E': return UCAL_DAY_OF_WEEK;
        case 'D': return UCAL_DAY_OF_YEAR;
        case 'F': return UCAL_DAY_OF_WEEK_IN_MONTH;
        case 'a': return UCAL_AM_PM;
        case 'g': return UCAL_JULIAN_DAY;
    }
    return (UCalendarDateFields)-1;
}

static RSCalendarUnit __RSCalendarGetCalendarUnitFromChar(char ch) {
    switch (ch) {
        case 'G': return RSCalendarUnitEra;
        case 'y': return RSCalendarUnitYear;
        case 'M': return RSCalendarUnitMonth;
        case 'd': return RSCalendarUnitDay;
        case 'H': return RSCalendarUnitHour;
        case 'm': return RSCalendarUnitMinute;
        case 's': return RSCalendarUnitSecond;
        case 'w': return RSCalendarUnitWeekOfYear;
        case 'W': return RSCalendarUnitWeekOfMonth;
        case 'Y': return RSCalendarUnitYearForWeekOfYear;
        case 'E': return RSCalendarUnitWeekday;
        case 'F': return RSCalendarUnitWeekdayOrdinal;
    }
    return (UCalendarDateFields)-1;
}

RSExport RSRange RSCalendarGetMinimumRangeOfUnit(RSCalendarRef calendar, RSCalendarUnit unit) {
    RS_OBJC_FUNCDISPATCHV(RSCalendarGetTypeID(), RSRange, calendar, _minimumRangeOfUnit:unit);
    RSRange range = {RSNotFound, RSNotFound};
    __RSGenericValidInstance(calendar, RSCalendarGetTypeID());
    if (!calendar->_cal) __RSCalendarSetupCal(calendar);
    if (calendar->_cal) {
        ucal_clear(calendar->_cal);
        UCalendarDateFields field = __RSCalendarGetICUFieldCode(unit);
        UErrorCode status = U_ZERO_ERROR;
        range.location = ucal_getLimit(calendar->_cal, field, UCAL_GREATEST_MINIMUM, &status);
        range.length = ucal_getLimit(calendar->_cal, field, UCAL_LEAST_MAXIMUM, &status) - range.location + 1;
        if (UCAL_MONTH == field) range.location++;
        if (100000 < range.length) range.length = 100000;
    }
    return range;
}

RSExport RSRange RSCalendarGetMaximumRangeOfUnit(RSCalendarRef calendar, RSCalendarUnit unit) {
    RS_OBJC_FUNCDISPATCHV(RSCalendarGetTypeID(), RSRange, calendar, _maximumRangeOfUnit:unit);
    RSRange range = {RSNotFound, RSNotFound};
    __RSGenericValidInstance(calendar, RSCalendarGetTypeID());
    if (!calendar->_cal) __RSCalendarSetupCal(calendar);
    if (calendar->_cal) {
        ucal_clear(calendar->_cal);
        UCalendarDateFields field = __RSCalendarGetICUFieldCode(unit);
        UErrorCode status = U_ZERO_ERROR;
        range.location = ucal_getLimit(calendar->_cal, field, UCAL_MINIMUM, &status);
        range.length = ucal_getLimit(calendar->_cal, field, UCAL_MAXIMUM, &status) - range.location + 1;
        if (UCAL_MONTH == field) range.location++;
        if (100000 < range.length) range.length = 100000;
    }
    return range;
}

static void __RSCalendarSetToFirstInstant(RSCalendarRef calendar, RSCalendarUnit unit, RSAbsoluteTime at) {
    // Set UCalendar to first instant of unit prior to 'at'
    UErrorCode status = U_ZERO_ERROR;
    UDate udate = floor((at + RSAbsoluteTimeIntervalSince1970) * 1000.0);
    ucal_setMillis(calendar->_cal, udate, &status);
    int target_era = INT_MIN;
    switch (unit) { // largest to smallest, we set the fields to their minimum value
        case RSCalendarUnitYearForWeekOfYear:;
            ucal_set(calendar->_cal, UCAL_WEEK_OF_YEAR, ucal_getLimit(calendar->_cal, UCAL_WEEK_OF_YEAR, UCAL_ACTUAL_MINIMUM, &status));
        case RSCalendarUnitWeek:
        case RSCalendarUnitWeekOfMonth:;
        case RSCalendarUnitWeekOfYear:;
        {
            // reduce to first day of week, then reduce the rest of the day
            int32_t goal = ucal_getAttribute(calendar->_cal, UCAL_FIRST_DAY_OF_WEEK);
            int32_t dow = ucal_get(calendar->_cal, UCAL_DAY_OF_WEEK, &status);
            while (dow != goal) {
                ucal_add(calendar->_cal, UCAL_DAY_OF_MONTH, -1, &status);
                dow = ucal_get(calendar->_cal, UCAL_DAY_OF_WEEK, &status);
            }
            goto day;
        }
        case RSCalendarUnitEra:
        {
            target_era = ucal_get(calendar->_cal, UCAL_ERA, &status);
            ucal_set(calendar->_cal, UCAL_YEAR, ucal_getLimit(calendar->_cal, UCAL_YEAR, UCAL_ACTUAL_MINIMUM, &status));
        }
        case RSCalendarUnitYear:
            ucal_set(calendar->_cal, UCAL_MONTH, ucal_getLimit(calendar->_cal, UCAL_MONTH, UCAL_ACTUAL_MINIMUM, &status));
        case RSCalendarUnitMonth:
            ucal_set(calendar->_cal, UCAL_DAY_OF_MONTH, ucal_getLimit(calendar->_cal, UCAL_DAY_OF_MONTH, UCAL_ACTUAL_MINIMUM, &status));
        case RSCalendarUnitWeekday:
        case RSCalendarUnitDay:
        day:;
            ucal_set(calendar->_cal, UCAL_HOUR_OF_DAY, ucal_getLimit(calendar->_cal, UCAL_HOUR_OF_DAY, UCAL_ACTUAL_MINIMUM, &status));
        case RSCalendarUnitHour:
            ucal_set(calendar->_cal, UCAL_MINUTE, ucal_getLimit(calendar->_cal, UCAL_MINUTE, UCAL_ACTUAL_MINIMUM, &status));
        case RSCalendarUnitMinute:
            ucal_set(calendar->_cal, UCAL_SECOND, ucal_getLimit(calendar->_cal, UCAL_SECOND, UCAL_ACTUAL_MINIMUM, &status));
        case RSCalendarUnitSecond:
            ucal_set(calendar->_cal, UCAL_MILLISECOND, 0);
    }
    if (INT_MIN != target_era && ucal_get(calendar->_cal, UCAL_ERA, &status) < target_era) {
        // In the Japanese calendar, and possibly others, eras don't necessarily
        // start on the first day of a year, so the previous code may have backed
        // up into the previous era, and we have to correct forward.
        UDate bad_udate = ucal_getMillis(calendar->_cal, &status);
        ucal_add(calendar->_cal, UCAL_MONTH, 1, &status);
        while (ucal_get(calendar->_cal, UCAL_ERA, &status) < target_era) {
            bad_udate = ucal_getMillis(calendar->_cal, &status);
            ucal_add(calendar->_cal, UCAL_MONTH, 1, &status);
        }
        udate = ucal_getMillis(calendar->_cal, &status);
        // target date is between bad_udate and udate
        for (;;) {
            UDate test_udate = (udate + bad_udate) / 2;
            ucal_setMillis(calendar->_cal, test_udate, &status);
            if (ucal_get(calendar->_cal, UCAL_ERA, &status) < target_era) {
                bad_udate = test_udate;
            } else {
                udate = test_udate;
            }
            if (fabs(udate - bad_udate) < 1000) break;
        }
        do {
            bad_udate = floor((bad_udate + 1000) / 1000) * 1000;
            ucal_setMillis(calendar->_cal, bad_udate, &status);
        } while (ucal_get(calendar->_cal, UCAL_ERA, &status) < target_era);
    }
}

static BOOL __validUnits(RSCalendarUnit smaller, RSCalendarUnit bigger) {
    switch (bigger) {
        case RSCalendarUnitEra:
            if (RSCalendarUnitEra == smaller) return NO;
            if (RSCalendarUnitWeekday == smaller) return NO;
            if (RSCalendarUnitMinute == smaller) return NO;	// this causes RSIndex overflow in range.length
            if (RSCalendarUnitSecond == smaller) return NO;	// this causes RSIndex overflow in range.length
            return YES;
        case RSCalendarUnitYearForWeekOfYear:
        case RSCalendarUnitYear:
            if (RSCalendarUnitEra == smaller) return NO;
            if (RSCalendarUnitYear == smaller) return NO;
            if (RSCalendarUnitYearForWeekOfYear == smaller) return NO;
            if (RSCalendarUnitWeekday == smaller) return NO;
            return YES;
        case RSCalendarUnitMonth:
            if (RSCalendarUnitEra == smaller) return NO;
            if (RSCalendarUnitYear == smaller) return NO;
            if (RSCalendarUnitMonth == smaller) return NO;
            if (RSCalendarUnitWeekday == smaller) return NO;
            return YES;
        case RSCalendarUnitDay:
            if (RSCalendarUnitHour == smaller) return YES;
            if (RSCalendarUnitMinute == smaller) return YES;
            if (RSCalendarUnitSecond == smaller) return YES;
            return NO;
        case RSCalendarUnitHour:
            if (RSCalendarUnitMinute == smaller) return YES;
            if (RSCalendarUnitSecond == smaller) return YES;
            return NO;
        case RSCalendarUnitMinute:
            if (RSCalendarUnitSecond == smaller) return YES;
            return NO;
        case RSCalendarUnitWeek:
        case RSCalendarUnitWeekOfMonth:
        case RSCalendarUnitWeekOfYear:
            if (RSCalendarUnitWeekday == smaller) return YES;
            if (RSCalendarUnitDay == smaller) return YES;
            if (RSCalendarUnitHour == smaller) return YES;
            if (RSCalendarUnitMinute == smaller) return YES;
            if (RSCalendarUnitSecond == smaller) return YES;
            return NO;
        case RSCalendarUnitSecond:
        case RSCalendarUnitWeekday:
        case RSCalendarUnitWeekdayOrdinal:
            return NO;
    }
    return NO;
};

static RSRange __RSCalendarGetRangeOfUnit1(RSCalendarRef calendar, RSCalendarUnit smallerUnit, RSCalendarUnit biggerUnit, RSAbsoluteTime at) {
    RSRange range = {RSNotFound, RSNotFound};
    if (!__validUnits(smallerUnit, biggerUnit)) return range;
    RS_OBJC_FUNCDISPATCHV(RSCalendarGetTypeID(), RSRange, calendar, _rangeOfUnit:smallerUnit inUnit:biggerUnit forAT:at);
    __RSGenericValidInstance(calendar, RSCalendarGetTypeID());
    if (!calendar->_cal) __RSCalendarSetupCal(calendar);
    if (calendar->_cal) {
        int32_t dow = -1;
        ucal_clear(calendar->_cal);
        UCalendarDateFields smallField = __RSCalendarGetICUFieldCode(smallerUnit);
        UCalendarDateFields bigField = __RSCalendarGetICUFieldCode(biggerUnit);
        if (RSCalendarUnitWeekdayOrdinal == smallerUnit) {
            UErrorCode status = U_ZERO_ERROR;
            UDate udate = floor((at + RSAbsoluteTimeIntervalSince1970) * 1000.0);
            ucal_setMillis(calendar->_cal, udate, &status);
            dow = ucal_get(calendar->_cal, UCAL_DAY_OF_WEEK, &status);
        }
        // Set calendar to first instant of big unit
        __RSCalendarSetToFirstInstant(calendar, biggerUnit, at);
        UErrorCode status = U_ZERO_ERROR;
        UDate start = ucal_getMillis(calendar->_cal, &status);
        if (RSCalendarUnitWeek == biggerUnit) {
            range.location = ucal_get(calendar->_cal, smallField, &status);
            if (RSCalendarUnitMonth == smallerUnit) range.location++;
        } else {
            range.location = (RSCalendarUnitHour == smallerUnit || RSCalendarUnitMinute == smallerUnit || RSCalendarUnitSecond == smallerUnit) ? 0 : 1;
        }
        // Set calendar to first instant of next value of big unit
        if (UCAL_ERA == bigField) {
            // ICU refuses to do the addition, probably because we are
            // at the limit of UCAL_ERA.  Use alternate strategy.
            RSIndex limit = ucal_getLimit(calendar->_cal, UCAL_YEAR, UCAL_MAXIMUM, &status);
            if (100000 < limit) limit = 100000;
            ucal_add(calendar->_cal, UCAL_YEAR, (RSBit32)limit, &status);
        } else {
            ucal_add(calendar->_cal, bigField, 1, &status);
        }
        if (RSCalendarUnitWeek == smallerUnit && RSCalendarUnitYear == biggerUnit) {
            ucal_add(calendar->_cal, UCAL_SECOND, -1, &status);
            range.length = ucal_get(calendar->_cal, UCAL_WEEK_OF_YEAR, &status);
            while (1 == range.length) {
                ucal_add(calendar->_cal, UCAL_DAY_OF_MONTH, -1, &status);
                range.length = ucal_get(calendar->_cal, UCAL_WEEK_OF_YEAR, &status);
            }
            range.location = 1;
            return range;
        } else if (RSCalendarUnitWeek == smallerUnit && RSCalendarUnitMonth == biggerUnit) {
            ucal_add(calendar->_cal, UCAL_SECOND, -1, &status);
            range.length = ucal_get(calendar->_cal, UCAL_WEEK_OF_YEAR, &status);
            range.location = 1;
            return range;
        }
        UDate goal = ucal_getMillis(calendar->_cal, &status);
        // Set calendar back to first instant of big unit
        ucal_setMillis(calendar->_cal, start, &status);
        if (RSCalendarUnitWeekdayOrdinal == smallerUnit) {
            // roll day forward to first 'dow'
            while (ucal_get(calendar->_cal, (RSCalendarUnitMonth == biggerUnit) ? UCAL_WEEK_OF_MONTH : UCAL_WEEK_OF_YEAR, &status) != 1) {
                ucal_add(calendar->_cal, UCAL_DAY_OF_MONTH, 1, &status);
            }
            while (ucal_get(calendar->_cal, UCAL_DAY_OF_WEEK, &status) != dow) {
                ucal_add(calendar->_cal, UCAL_DAY_OF_MONTH, 1, &status);
            }
            start = ucal_getMillis(calendar->_cal, &status);
            goal -= 1000;
            range.location = 1;  // constant here works around ICU -- see 3948293
        }
        UDate curr = start;
        range.length = 	(RSCalendarUnitWeekdayOrdinal == smallerUnit) ? 1 : 0;
        const int multiple_table[] = {0, 0, 16, 19, 24, 26, 24, 28, 14, 14, 14};
        int multiple = (1 << multiple_table[flsl(smallerUnit) - 1]);
        BOOL divide = NO, alwaysDivide = NO;
        while (curr < goal) {
            ucal_add(calendar->_cal, smallField, multiple, &status);
            UDate newcurr = ucal_getMillis(calendar->_cal, &status);
            if (curr < newcurr && newcurr <= goal) {
                range.length += multiple;
                curr = newcurr;
            } else {
                // Either newcurr is going backwards, or not making
                // progress, or has overshot the goal; reset date
                // and try smaller multiples.
                ucal_setMillis(calendar->_cal, curr, &status);
                divide = YES;
                // once we start overshooting the goal, the add at
                // smaller multiples will succeed at most once for
                // each multiple, so we reduce it every time through
                // the loop.
                if (goal < newcurr) alwaysDivide = YES;
            }
            if (divide) {
                multiple = multiple / 2;
                if (0 == multiple) break;
                divide = alwaysDivide;
            }
        }
    }
    return range;
}

static RSRange __RSCalendarGetRangeOfUnit2(RSCalendarRef calendar, RSCalendarUnit smallerUnit, RSCalendarUnit biggerUnit, RSAbsoluteTime at) __attribute__((noinline));
static RSRange __RSCalendarGetRangeOfUnit2(RSCalendarRef calendar, RSCalendarUnit smallerUnit, RSCalendarUnit biggerUnit, RSAbsoluteTime at) {
    RS_OBJC_FUNCDISPATCHV(RSCalendarGetTypeID(), RSRange, calendar, _rangeOfUnit:smallerUnit inUnit:biggerUnit forAT:at);
    __RSGenericValidInstance(calendar, RSCalendarGetTypeID());
    RSRange range = {RSNotFound, RSNotFound};
    if (!calendar->_cal) __RSCalendarSetupCal(calendar);
    if (calendar->_cal) {
        switch (smallerUnit) {
            case RSCalendarUnitSecond:
                switch (biggerUnit) {
                    case RSCalendarUnitMinute:
                    case RSCalendarUnitHour:
                    case RSCalendarUnitDay:
                    case RSCalendarUnitWeekday:
                    case RSCalendarUnitWeek:
                    case RSCalendarUnitMonth:
                    case RSCalendarUnitYear:
                    case RSCalendarUnitEra:
                        // goto calculate;
                        range.location = 0;
                        range.length = 60;
                        break;
                }
                break;
            case RSCalendarUnitMinute:
                switch (biggerUnit) {
                    case RSCalendarUnitHour:
                    case RSCalendarUnitDay:
                    case RSCalendarUnitWeekday:
                    case RSCalendarUnitWeek:
                    case RSCalendarUnitMonth:
                    case RSCalendarUnitYear:
                    case RSCalendarUnitEra:
                        // goto calculate;
                        range.location = 0;
                        range.length = 60;
                        break;
                }
                break;
            case RSCalendarUnitHour:
                switch (biggerUnit) {
                    case RSCalendarUnitDay:
                    case RSCalendarUnitWeekday:
                    case RSCalendarUnitWeek:
                    case RSCalendarUnitMonth:
                    case RSCalendarUnitYear:
                    case RSCalendarUnitEra:
                        // goto calculate;
                        range.location = 0;
                        range.length = 24;
                        break;
                }
                break;
            case RSCalendarUnitDay:
                switch (biggerUnit) {
                    case RSCalendarUnitWeek:
                    case RSCalendarUnitMonth:
                    case RSCalendarUnitYear:
                    case RSCalendarUnitEra:
                        goto calculate;
                        break;
                }
                break;
            case RSCalendarUnitWeekday:
                switch (biggerUnit) {
                    case RSCalendarUnitWeek:
                    case RSCalendarUnitMonth:
                    case RSCalendarUnitYear:
                    case RSCalendarUnitEra:
                        goto calculate;
                        break;
                }
                break;
            case RSCalendarUnitWeekdayOrdinal:
                switch (biggerUnit) {
                    case RSCalendarUnitMonth:
                    case RSCalendarUnitYear:
                    case RSCalendarUnitEra:
                        goto calculate;
                        break;
                }
                break;
            case RSCalendarUnitWeek:
                switch (biggerUnit) {
                    case RSCalendarUnitMonth:
                    case RSCalendarUnitYear:
                    case RSCalendarUnitEra:
                        goto calculate;
                        break;
                }
                break;
            case RSCalendarUnitMonth:
                switch (biggerUnit) {
                    case RSCalendarUnitYear:
                    case RSCalendarUnitEra:
                        goto calculate;
                        break;
                }
                break;
            case RSCalendarUnitYear:
                switch (biggerUnit) {
                    case RSCalendarUnitEra:
                        goto calculate;
                        break;
                }
                break;
            case RSCalendarUnitEra:
                break;
        }
    }
    return range;
    
calculate:;
    ucal_clear(calendar->_cal);
    UCalendarDateFields smallField = __RSCalendarGetICUFieldCode(smallerUnit);
    UCalendarDateFields bigField = __RSCalendarGetICUFieldCode(biggerUnit);
    UCalendarDateFields yearField = __RSCalendarGetICUFieldCode(RSCalendarUnitYear);
    UCalendarDateFields fieldToAdd = smallField;
    if (RSCalendarUnitWeekday == smallerUnit) {
        fieldToAdd = __RSCalendarGetICUFieldCode(RSCalendarUnitDay);
    }
    int32_t dow = -1;
    if (RSCalendarUnitWeekdayOrdinal == smallerUnit) {
        UErrorCode status = U_ZERO_ERROR;
        UDate udate = floor((at + RSAbsoluteTimeIntervalSince1970) * 1000.0);
        ucal_setMillis(calendar->_cal, udate, &status);
        dow = ucal_get(calendar->_cal, UCAL_DAY_OF_WEEK, &status);
        fieldToAdd = __RSCalendarGetICUFieldCode(RSCalendarUnitWeek);
    }
    // Set calendar to first instant of big unit
    __RSCalendarSetToFirstInstant(calendar, biggerUnit, at);
    if (RSCalendarUnitWeekdayOrdinal == smallerUnit) {
        UErrorCode status = U_ZERO_ERROR;
        // roll day forward to first 'dow'
        while (ucal_get(calendar->_cal, (RSCalendarUnitMonth == biggerUnit) ? UCAL_WEEK_OF_MONTH : UCAL_WEEK_OF_YEAR, &status) != 1) {
            ucal_add(calendar->_cal, UCAL_DAY_OF_MONTH, 1, &status);
        }
        while (ucal_get(calendar->_cal, UCAL_DAY_OF_WEEK, &status) != dow) {
            ucal_add(calendar->_cal, UCAL_DAY_OF_MONTH, 1, &status);
        }
    }
    int32_t minSmallValue = INT32_MAX;
    int32_t maxSmallValue = INT32_MIN;
    UErrorCode status = U_ZERO_ERROR;
    int32_t bigValue = ucal_get(calendar->_cal, bigField, &status);
    for (;;) {
        int32_t smallValue = ucal_get(calendar->_cal, smallField, &status);
        if (smallValue < minSmallValue) minSmallValue = smallValue;
        if (smallValue > maxSmallValue) maxSmallValue = smallValue;
        ucal_add(calendar->_cal, fieldToAdd, 1, &status);
        if (bigValue != ucal_get(calendar->_cal, bigField, &status)) break;
        if (biggerUnit == RSCalendarUnitEra && ucal_get(calendar->_cal, yearField, &status) > 10000) break;
        // we assume an answer for 10000 years can be extrapolated to 100000 years, to save time
    }
    status = U_ZERO_ERROR;
    range.location = minSmallValue;
    if (smallerUnit == RSCalendarUnitMonth) range.location = 1;
    range.length = maxSmallValue - minSmallValue + 1;
    if (biggerUnit == RSCalendarUnitEra && ucal_get(calendar->_cal, yearField, &status) > 10000) range.length = 100000;
    
    return range;
}

RSExport RSRange RSCalendarGetRangeOfUnit(RSCalendarRef calendar, RSCalendarUnit smallerUnit, RSCalendarUnit biggerUnit, RSAbsoluteTime at) {
	return __RSCalendarGetRangeOfUnit2(calendar, smallerUnit, biggerUnit, at);
}

RSExport RSIndex RSCalendarGetOrdinalityOfUnit(RSCalendarRef calendar, RSCalendarUnit smallerUnit, RSCalendarUnit biggerUnit, RSAbsoluteTime at) {
    RSIndex result = RSNotFound;
    if (!__validUnits(smallerUnit, biggerUnit)) return result;
    RS_OBJC_FUNCDISPATCHV(RSCalendarGetTypeID(), RSIndex, calendar, _ordinalityOfUnit:smallerUnit inUnit:biggerUnit forAT:at);
    __RSGenericValidInstance(calendar, RSCalendarGetTypeID());
    if (!calendar->_cal) __RSCalendarSetupCal(calendar);
    if (calendar->_cal) {
        UErrorCode status = U_ZERO_ERROR;
        ucal_clear(calendar->_cal);
        if (RSCalendarUnitWeek == smallerUnit && RSCalendarUnitYear == biggerUnit) {
            UDate udate = floor((at + RSAbsoluteTimeIntervalSince1970) * 1000.0);
            ucal_setMillis(calendar->_cal, udate, &status);
            int32_t val = ucal_get(calendar->_cal, UCAL_WEEK_OF_YEAR, &status);
            return val;
        } else if (RSCalendarUnitWeek == smallerUnit && RSCalendarUnitMonth == biggerUnit) {
            UDate udate = floor((at + RSAbsoluteTimeIntervalSince1970) * 1000.0);
            ucal_setMillis(calendar->_cal, udate, &status);
            int32_t val = ucal_get(calendar->_cal, UCAL_WEEK_OF_MONTH, &status);
            return val;
        }
        UCalendarDateFields smallField = __RSCalendarGetICUFieldCode(smallerUnit);
        // Set calendar to first instant of big unit
        __RSCalendarSetToFirstInstant(calendar, biggerUnit, at);
        UDate curr = ucal_getMillis(calendar->_cal, &status);
        UDate goal = floor((at + RSAbsoluteTimeIntervalSince1970) * 1000.0);
        result = 1;
        const int multiple_table[] = {0, 0, 16, 19, 24, 26, 24, 28, 14, 14, 14};
        int multiple = (1 << multiple_table[flsl(smallerUnit) - 1]);
        BOOL divide = NO, alwaysDivide = NO;
        while (curr < goal) {
            ucal_add(calendar->_cal, smallField, multiple, &status);
            UDate newcurr = ucal_getMillis(calendar->_cal, &status);
            if (curr < newcurr && newcurr <= goal) {
                result += multiple;
                curr = newcurr;
            } else {
                // Either newcurr is going backwards, or not making
                // progress, or has overshot the goal; reset date
                // and try smaller multiples.
                ucal_setMillis(calendar->_cal, curr, &status);
                divide = YES;
                // once we start overshooting the goal, the add at
                // smaller multiples will succeed at most once for
                // each multiple, so we reduce it every time through
                // the loop.
                if (goal < newcurr) alwaysDivide = YES;
            }
            if (divide) {
                multiple = multiple / 2;
                if (0 == multiple) break;
                divide = alwaysDivide;
            }
        }
    }
    return result;
}

RSExport BOOL _RSCalendarComposeAbsoluteTimeV(RSCalendarRef calendar, /* out */ RSAbsoluteTime *atp, const char *componentDesc, int *vector, int count) {
    if (!calendar->_cal) __RSCalendarSetupCal(calendar);
    if (calendar->_cal) {
        UErrorCode status = U_ZERO_ERROR;
        ucal_clear(calendar->_cal);
        ucal_set(calendar->_cal, UCAL_YEAR, 1);
        ucal_set(calendar->_cal, UCAL_MONTH, 0);
        ucal_set(calendar->_cal, UCAL_DAY_OF_MONTH, 1);
        ucal_set(calendar->_cal, UCAL_HOUR_OF_DAY, 0);
        ucal_set(calendar->_cal, UCAL_MINUTE, 0);
        ucal_set(calendar->_cal, UCAL_SECOND, 0);
        const char *desc = componentDesc;
        BOOL doWOY = NO;
        char ch = *desc;
        while (ch) {
            UCalendarDateFields field = __RSCalendarGetICUFieldCodeFromChar(ch);
            if (UCAL_WEEK_OF_YEAR == field) {
                doWOY = YES;
            }
            desc++;
            ch = *desc;
        }
        desc = componentDesc;
        ch = *desc;
        int value = 0;
        while (ch) {
            UCalendarDateFields field = __RSCalendarGetICUFieldCodeFromChar(ch);
            value = *vector;
            if (UCAL_YEAR == field && doWOY) field = UCAL_YEAR_WOY;
            if (UCAL_MONTH == field) value--;
            ucal_set(calendar->_cal, field, value);
            vector++;
            desc++;
            ch = *desc;
        }
        UDate udate = ucal_getMillis(calendar->_cal, &status);
        RSAbsoluteTime at = (udate / 1000.0) - RSAbsoluteTimeIntervalSince1970;
        if (atp) *atp = at;
        return U_SUCCESS(status) ? YES : NO;
    }
    return NO;
}

static BOOL _RSCalendarDecomposeAbsoluteTimeV(RSCalendarRef calendar, RSAbsoluteTime at, const char *componentDesc, int **vector, int count)
{
    if (!calendar->_cal) __RSCalendarSetupCal(calendar);
    if (calendar->_cal) {
        UErrorCode status = U_ZERO_ERROR;
        ucal_clear(calendar->_cal);
        UDate udate = floor((at + RSAbsoluteTimeIntervalSince1970) * 1000.0);
        ucal_setMillis(calendar->_cal, udate, &status);
        char ch = *componentDesc;
        while (ch) {
            UCalendarDateFields field = __RSCalendarGetICUFieldCodeFromChar(ch);
            int value = ucal_get(calendar->_cal, field, &status);
            if (UCAL_MONTH == field) value++;
            *(*vector) = value;
            vector++;
            componentDesc++;
            ch = *componentDesc;
        }
        return U_SUCCESS(status) ? YES : NO;
    }
    return NO;
}

RSExport BOOL _RSCalendarAddComponentsV(RSCalendarRef calendar, /* inout */ RSAbsoluteTime *atp, RSOptionFlags options, const char *componentDesc, int *vector, int count) {
    if (!calendar->_cal) __RSCalendarSetupCal(calendar);
    if (calendar->_cal) {
        UErrorCode status = U_ZERO_ERROR;
        ucal_clear(calendar->_cal);
        UDate udate = floor((*atp + RSAbsoluteTimeIntervalSince1970) * 1000.0);
        ucal_setMillis(calendar->_cal, udate, &status);
        char ch = *componentDesc;
        while (ch) {
            UCalendarDateFields field = __RSCalendarGetICUFieldCodeFromChar(ch);
            int amount = *vector;
            if (options & RSCalendarComponentsWrap) {
                ucal_roll(calendar->_cal, field, amount, &status);
            } else {
                ucal_add(calendar->_cal, field, amount, &status);
            }
            vector++;
            componentDesc++;
            ch = *componentDesc;
        }
        udate = ucal_getMillis(calendar->_cal, &status);
        *atp = (udate / 1000.0) - RSAbsoluteTimeIntervalSince1970;
        return U_SUCCESS(status) ? YES : NO;
    }
    return NO;
}

RSExport BOOL _RSCalendarGetComponentDifferenceV(RSCalendarRef calendar, RSAbsoluteTime startingAT, RSAbsoluteTime resultAT, RSOptionFlags options, const char *componentDesc, int **vector, int count)
{
    if (!calendar->_cal) __RSCalendarSetupCal(calendar);
    if (calendar->_cal)
    {
        UErrorCode status = U_ZERO_ERROR;
        ucal_clear(calendar->_cal);
        UDate curr = floor((startingAT + RSAbsoluteTimeIntervalSince1970) * 1000.0);
        UDate goal = floor((resultAT + RSAbsoluteTimeIntervalSince1970) * 1000.0);
        ucal_setMillis(calendar->_cal, curr, &status);
        int direction = (startingAT <= resultAT) ? 1 : -1;
        char ch = *componentDesc;
        while (ch) {
            UCalendarDateFields field = __RSCalendarGetICUFieldCodeFromChar(ch);
            const int multiple_table[] = {0, 0, 16, 19, 24, 26, 24, 28, 14, 14, 14};
            int multiple = direction * (1 << multiple_table[flsl(__RSCalendarGetCalendarUnitFromChar(ch)) - 1]);
            BOOL divide = NO, alwaysDivide = NO;
            int result = 0;
            while ((direction > 0 && curr < goal) || (direction < 0 && goal < curr)) {
                ucal_add(calendar->_cal, field, multiple, &status);
                UDate newcurr = ucal_getMillis(calendar->_cal, &status);
                if ((direction > 0 && curr < newcurr && newcurr <= goal) || (direction < 0 && newcurr < curr && goal <= newcurr)) {
                    result += multiple;
                    curr = newcurr;
                } else {
                    // Either newcurr is going backwards, or not making
                    // progress, or has overshot the goal; reset date
                    // and try smaller multiples.
                    ucal_setMillis(calendar->_cal, curr, &status);
                    divide = YES;
                    // once we start overshooting the goal, the add at
                    // smaller multiples will succeed at most once for
                    // each multiple, so we reduce it every time through
                    // the loop.
                    if ((direction > 0 && goal < newcurr) || (direction < 0 && newcurr < goal)) alwaysDivide = YES;
                }
                if (divide) {
                    multiple = multiple / 2;
                    if (0 == multiple) break;
                    divide = alwaysDivide;
                }
            }
            *(*vector) = result;
            vector++;
            componentDesc++;
            ch = *componentDesc;
        }
        return U_SUCCESS(status) ? YES : NO;
    }
    return NO;
}

RSExport BOOL RSCalendarComposeAbsoluteTime(RSCalendarRef calendar, /* out */ RSAbsoluteTime *atp, const char *componentDesc, ...) {
    va_list args;
    va_start(args, componentDesc);
    RS_OBJC_FUNCDISPATCHV(RSCalendarGetTypeID(), BOOL, calendar, _composeAbsoluteTime:atp :componentDesc :args);
    __RSGenericValidInstance(calendar, RSCalendarGetTypeID());
    int idx, cnt = (int)strlen((char *)componentDesc);
    STACK_BUFFER_DECL(int, vector, cnt);
    for (idx = 0; idx < cnt; idx++) {
        int arg = va_arg(args, int);
        vector[idx] = arg;
    }
    va_end(args);
    return _RSCalendarComposeAbsoluteTimeV(calendar, atp, componentDesc, vector, cnt);
}

RSExport BOOL RSCalendarDecomposeAbsoluteTime(RSCalendarRef calendar, RSAbsoluteTime at, const char *componentDesc, ...) {
    va_list args;
    va_start(args, componentDesc);
    RS_OBJC_FUNCDISPATCHV(RSCalendarGetTypeID(), BOOL, calendar, _decomposeAbsoluteTime:at :componentDesc :args);
    __RSGenericValidInstance(calendar, RSCalendarGetTypeID());
    int idx, cnt = (int)strlen((char *)componentDesc);
    STACK_BUFFER_DECL(int *, vector, cnt);
    for (idx = 0; idx < cnt; idx++) {
        int *arg = va_arg(args, int *);
        vector[idx] = arg;
    }
    va_end(args);
    return _RSCalendarDecomposeAbsoluteTimeV(calendar, at, componentDesc, vector, cnt);
}

RSExport BOOL RSCalendarAddComponents(RSCalendarRef calendar, /* inout */ RSAbsoluteTime *atp, RSOptionFlags options, const char *componentDesc, ...) {
    va_list args;
    va_start(args, componentDesc);
    RS_OBJC_FUNCDISPATCHV(RSCalendarGetTypeID(), BOOL, calendar, _addComponents:atp :options :componentDesc :args);
    __RSGenericValidInstance(calendar, RSCalendarGetTypeID());
    int idx, cnt = (int)strlen((char *)componentDesc);
    STACK_BUFFER_DECL(int, vector, cnt);
    for (idx = 0; idx < cnt; idx++) {
        int arg = va_arg(args, int);
        vector[idx] = arg;
    }
    va_end(args);
    return _RSCalendarAddComponentsV(calendar, atp, options, componentDesc, vector, cnt);    
}

RSExport BOOL RSCalendarGetComponentDifference(RSCalendarRef calendar, RSAbsoluteTime startingAT, RSAbsoluteTime resultAT, RSOptionFlags options, const char *componentDesc, ...) {
    va_list args;
    va_start(args, componentDesc);
    RS_OBJC_FUNCDISPATCHV(RSCalendarGetTypeID(), BOOL, calendar, _diffComponents:startingAT :resultAT :options :componentDesc :args);
    __RSGenericValidInstance(calendar, RSCalendarGetTypeID());
    int idx, cnt = (int)strlen((char *)componentDesc);
    STACK_BUFFER_DECL(int *, vector, cnt);
    for (idx = 0; idx < cnt; idx++) {
        int *arg = va_arg(args, int *);
        vector[idx] = arg;
    }
    va_end(args);
    BOOL ret = _RSCalendarGetComponentDifferenceV(calendar, startingAT, resultAT, options, componentDesc, vector, (int)cnt);
    return ret;
}

RSExport BOOL RSCalendarGetTimeRangeOfUnit(RSCalendarRef calendar, RSCalendarUnit unit, RSAbsoluteTime at, RSAbsoluteTime *startp, RSTimeInterval *tip) {
    RS_OBJC_FUNCDISPATCHV(RSCalendarGetTypeID(), BOOL, calendar, _rangeOfUnit:unit startTime:startp interval:tip forAT:at);
    __RSGenericValidInstance(calendar, RSCalendarGetTypeID());
    if (RSCalendarUnitWeekdayOrdinal == unit) return NO;
    if (RSCalendarUnitWeekday == unit) unit = RSCalendarUnitDay;
    if (!calendar->_cal) __RSCalendarSetupCal(calendar);
    if (calendar->_cal) {
        ucal_clear(calendar->_cal);
        __RSCalendarSetToFirstInstant(calendar, unit, at);
        UErrorCode status = U_ZERO_ERROR;
        UDate start = ucal_getMillis(calendar->_cal, &status);
        UCalendarDateFields field = __RSCalendarGetICUFieldCode(unit);
        ucal_add(calendar->_cal, field, 1, &status);
        UDate end = ucal_getMillis(calendar->_cal, &status);
        if (end == start && RSCalendarUnitEra == unit) {
            // ICU refuses to do the addition, probably because we are
            // at the limit of UCAL_ERA.  Use alternate strategy.
            RSIndex limit = ucal_getLimit(calendar->_cal, UCAL_YEAR, UCAL_MAXIMUM, &status);
            if (100000 < limit) limit = 100000;
            ucal_add(calendar->_cal, UCAL_YEAR, (RSBit32)limit, &status);
            end = ucal_getMillis(calendar->_cal, &status);
        }
        if (U_SUCCESS(status)) {
            if (startp) *startp = (double)start / 1000.0 - RSAbsoluteTimeIntervalSince1970;
            if (tip) *tip = (double)(end - start) / 1000.0;
            return YES;
        }
    }
    
    return NO;
}
#undef BUFFER_SIZE