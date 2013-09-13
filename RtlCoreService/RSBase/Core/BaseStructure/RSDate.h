//
//  RSDate.h
//  RSCoreFoundation
//
//  Created by RetVal on 11/12/12.
//  Copyright (c) 2012 RetVal. All rights reserved.
//

#ifndef RSCoreFoundation_RSDate_h
#define RSCoreFoundation_RSDate_h
#include <RSCoreFoundation/RSTimeZone.h>
RS_EXTERN_C_BEGIN
extern const RSTimeInterval RSAbsoluteTimeIntervalSince1970 RS_AVAILABLE(0_0);
extern const RSTimeInterval RSAbsoluteTimeIntervalSince1904 RS_AVAILABLE(0_0);
typedef struct {
    RSBit32 year ;
    RSBitU8 month ;
    RSBitU8 day ;
    RSBitU8 hour ;
    RSBitU8 minute ;
    RSTimeInterval second ;
} RSGregorianDate RS_AVAILABLE(0_0);

typedef const struct __RSDate* RSDateRef RS_AVAILABLE(0_0);
typedef struct __RSDate* RSMutableDateRef RS_AVAILABLE(0_0);
RSExport RSAbsoluteTime RSAbsoluteTimeGetCurrent() RS_AVAILABLE(0_0);
RSExport RSTypeID RSDateGetTypeID() RS_AVAILABLE(0_0);
RSExport RSAbsoluteTime RSGregorianDateGetAbsoluteTime(RSGregorianDate gdate, RSTimeZoneRef tz) RS_AVAILABLE(0_0);
RSExport RSGregorianDate RSAbsoluteTimeGetGregorianDate(RSAbsoluteTime at, RSTimeZoneRef tz) RS_AVAILABLE(0_0);

RSExport RSDateRef RSDateCreate(RSAllocatorRef allocator, RSAbsoluteTime time) RS_AVAILABLE(0_0);
RSExport RSDateRef RSDateGetCurrent(RSAllocatorRef allocator) RS_AVAILABLE(0_0);
RSExport RSAbsoluteTime RSDateGetAbsoluteTime(RSDateRef date) RS_AVAILABLE(0_0);
RSExport RSTimeInterval RSDateGetTimeIntervalSinceDate(RSDateRef date, RSDateRef otherDate) RS_AVAILABLE(0_0);
RSExport RSComparisonResult RSDateCompare(RSDateRef date, RSDateRef otherDate, void *context) RS_AVAILABLE(0_0);

RSExport RSBitU32 RSAbsoluteTimeGetDayOfWeek(RSAbsoluteTime at, RSTimeZoneRef tz) RS_AVAILABLE(0_0);
RSExport RSBitU32 RSAbsoluteTimeGetDayOfYear(RSAbsoluteTime at, RSTimeZoneRef tz) RS_AVAILABLE(0_0);
RSExport RSBitU32 RSAbsoluteTimeGetWeekOfYear(RSAbsoluteTime at, RSTimeZoneRef tz) RS_AVAILABLE(0_0);
RSExport RSUInteger RSDateGetDayOfWeek(RSDateRef date) RS_AVAILABLE(0_0);
RS_EXTERN_C_END
#endif
