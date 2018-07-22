//
//  RSTimeZone.h
//  RSCoreFoundation
//
//  Created by RetVal on 11/5/12.
//  Copyright (c) 2012 RetVal. All rights reserved.
//

#ifndef RSCoreFoundation_RSTimeZone_h
#define RSCoreFoundation_RSTimeZone_h
#include <RSCoreFoundation/RSArray.h>
RS_EXTERN_C_BEGIN
typedef const struct __RSTimeZone* RSTimeZoneRef RS_AVAILABLE(0_0);
RSExport RSTypeID RSTimeZoneGetTypeID(void) RS_AVAILABLE(0_0);
RSExport RSArrayRef RSTimeZoneCopyKnownNames(void) RS_AVAILABLE(0_0);

RSExport RSTimeZoneRef RSTimeZoneCopySystem(void) RS_AVAILABLE(0_0);    // copy system timezone, autoreleased.
RSExport RSTimeZoneRef RSTimeZoneCopyDefault(void) RS_AVAILABLE(0_0);

RSExport RSTimeZoneRef RSTimeZoneCreateWithName(RSAllocatorRef allocator, RSStringRef timeZoneName) RS_AVAILABLE(0_0);
RSExport RSStringRef RSTimeZoneGetName(RSTimeZoneRef timeZone) RS_AVAILABLE(0_0);
RSExport RSTimeInterval RSTimeZoneGetSecondsFromGMT(RSTimeZoneRef zone, RSTimeInterval time) RS_AVAILABLE(0_0);
RS_EXTERN_C_END
#endif
