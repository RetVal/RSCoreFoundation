//
//  RSCalendar.h
//  RSCoreFoundation
//
//  Created by RetVal on 11/14/12.
//  Copyright (c) 2012 RetVal. All rights reserved.
//

#ifndef RSCoreFoundation_RSCalendar_h
#define RSCoreFoundation_RSCalendar_h
#include <RSCoreFoundation/RSTimeZone.h>
#include <RSCoreFoundation/RSDate.h>
RS_EXTERN_C_BEGIN
typedef struct __RSCalendar* RSCalendarRef RS_AVAILABLE(0_0);


typedef RS_OPTIONS(RSOptionFlags, RSCalendarUnit) {
	RSCalendarUnitEra = (1UL << 1),
	RSCalendarUnitYear = (1UL << 2),
	RSCalendarUnitMonth = (1UL << 3),
	RSCalendarUnitDay = (1UL << 4),
	RSCalendarUnitHour = (1UL << 5),
	RSCalendarUnitMinute = (1UL << 6),
	RSCalendarUnitSecond = (1UL << 7),
	RSCalendarUnitWeek = (1UL << 8),
	RSCalendarUnitWeekday = (1UL << 9),
	RSCalendarUnitWeekdayOrdinal = (1UL << 10),
	RSCalendarUnitQuarter = (1UL << 11),
	RSCalendarUnitWeekOfMonth = (1UL << 12),
	RSCalendarUnitWeekOfYear = (1UL << 13),
	RSCalendarUnitYearForWeekOfYear = (1UL << 14),
} RS_AVAILABLE(0_0);

/*! @function RSCalendarGetTypeID
 @abstract Return the RSTypeID type id.
 @discussion This function return the type id of RSCalendarRef from the runtime.
 @result A RSTypeID the type id of RSCalendarRef.
 */
RSExport RSTypeID RSCalendarGetTypeID() RS_AVAILABLE(0_0);

/*! @function RSCalendarCreateWithIndentifier
 @abstract Return a RSCalendarRef object.
 @discussion This function return a new RSCalendarRef object is created with its indentifier.
 @param allocator push RSAllocatorDefault.
 @param indentifier a RSStringRef indentifier of the calendar.
 @result A RSCalendarRef a new calendar object.
 */
RSExport RSCalendarRef RSCalendarCreateWithIndentifier(RSAllocatorRef allocator, RSStringRef indentifier) RS_AVAILABLE(0_0);

/*! @function RSCalendarCopyTimeZone
 @abstract Return a RSTimeZoneRef object.
 @discussion This function return a RSTimeZoneRef from a calendar.
 @param calendar a RSCalendarRef calendar.
 @param indentifier a RSStringRef indentifier of the calendar.
 @result A RSTimeZoneRef from calendar with retain.
 */
    
RSExport RSTimeZoneRef RSCalendarGetTimeZone(RSCalendarRef calendar) RS_AVAILABLE(0_0);
RSExport RSTimeZoneRef RSCalendarCopyTimeZone(RSCalendarRef calendar) RS_AVAILABLE(0_0);

/*! @function RSCalendarSetTimeZone
 @abstract Return nothing.
 @discussion This function set the tz to the calendar.
 @param calendar a RSCalendarRef calendar.
 @param tz a RSTimeZone tz.
 @result void.
 */
RSExport void RSCalendarSetTimeZone(RSCalendarRef calendar, RSTimeZoneRef tz) RS_AVAILABLE(0_0);


RSExport RSStringRef RSCalendarGetIdentifier(RSCalendarRef calendar) RS_AVAILABLE(0_0);
RSExport RSStringRef RSCalendarCopyIndentifier(RSCalendarRef calendar) RS_AVAILABLE(0_0);

RSExport RSStringRef RSCalendarGetHumanReadableString(RSCalendarRef calendar) RS_AVAILABLE(0_0);

RSExport RSIndex RSCalendarGetFirstWeekday(RSCalendarRef calendar) RS_AVAILABLE(0_0);
RSExport void RSCalendarSetFirstWeekday(RSCalendarRef calendar, RSIndex wkdy) RS_AVAILABLE(0_0);
RSExport RSIndex RSCalendarGetMinimumDaysInFirstWeek(RSCalendarRef calendar) RS_AVAILABLE(0_0);
RSExport void RSCalendarSetMinimumDaysInFirstWeek(RSCalendarRef calendar, RSIndex mwd) RS_AVAILABLE(0_0);
RSExport RSDateRef RSCalendarCopyGregorianStartDate(RSCalendarRef calendar) RS_AVAILABLE(0_0);
RSExport void RSCalendarSetGregorianStartDate(RSCalendarRef calendar, RSDateRef date) RS_AVAILABLE(0_0);
RSExport RSRange RSCalendarGetMinimumRangeOfUnit(RSCalendarRef calendar, RSCalendarUnit unit) RS_AVAILABLE(0_0);
RSExport RSRange RSCalendarGetMaximumRangeOfUnit(RSCalendarRef calendar, RSCalendarUnit unit) RS_AVAILABLE(0_0);
RSExport RSRange RSCalendarGetRangeOfUnit(RSCalendarRef calendar, RSCalendarUnit smallerUnit, RSCalendarUnit biggerUnit, RSAbsoluteTime at) RS_AVAILABLE(0_0);
RSExport RSIndex RSCalendarGetOrdinalityOfUnit(RSCalendarRef calendar, RSCalendarUnit smallerUnit, RSCalendarUnit biggerUnit, RSAbsoluteTime at) RS_AVAILABLE(0_0);
RSExport BOOL RSCalendarComposeAbsoluteTime(RSCalendarRef calendar, /* out */ RSAbsoluteTime *atp, const char *componentDesc, ...) RS_AVAILABLE(0_0);
RSExport BOOL RSCalendarDecomposeAbsoluteTime(RSCalendarRef calendar, RSAbsoluteTime at, const char *componentDesc, ...) RS_AVAILABLE(0_0);
RSExport BOOL RSCalendarAddComponents(RSCalendarRef calendar, /* inout */ RSAbsoluteTime *atp, RSOptionFlags options, const char *componentDesc, ...) RS_AVAILABLE(0_0);
RSExport BOOL RSCalendarGetComponentDifference(RSCalendarRef calendar, RSAbsoluteTime startingAT, RSAbsoluteTime resultAT, RSOptionFlags options, const char *componentDesc, ...) RS_AVAILABLE(0_0);
RSExport BOOL RSCalendarGetTimeRangeOfUnit(RSCalendarRef calendar, RSCalendarUnit unit, RSAbsoluteTime at, RSAbsoluteTime *startp, RSTimeInterval *tip) RS_AVAILABLE(0_0);

enum {
    RSCalendarComponentsWrap = (1UL << 0)  // option for adding
} RS_AVAILABLE(0_0);

// identifier
RSExport RSStringRef const RSGregorianCalendar RS_AVAILABLE(0_0);
RSExport RSStringRef const RSBuddhistCalendar RS_AVAILABLE(0_0);
RSExport RSStringRef const RSChineseCalendar RS_AVAILABLE(0_0);
RSExport RSStringRef const RSHebrewCalendar RS_AVAILABLE(0_0);
RSExport RSStringRef const RSIslamicCalendar RS_AVAILABLE(0_0);
RSExport RSStringRef const RSIslamicCivilCalendar RS_AVAILABLE(0_0);
RSExport RSStringRef const RSJapaneseCalendar RS_AVAILABLE(0_0);

RS_EXTERN_C_END
#endif
