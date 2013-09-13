//
//  RSTimer.h
//  RSCoreFoundation
//
//  Created by RetVal on 5/21/13.
//  Copyright (c) 2013 RetVal. All rights reserved.
//

#ifndef RSCoreFoundation_RSTimer_h
#define RSCoreFoundation_RSTimer_h

#include <RSCoreFoundation/RSDate.h>
#include <RSCoreFoundation/RSRunLoop.h>
RS_EXTERN_C_BEGIN
typedef const struct __RSTimer* RSTimerRef;

RSExport RSTimerRef RSTimerCreateSchedule(RSAllocatorRef allocator, RSAbsoluteTime start, RSTimeInterval timeInterval, BOOL repeats, RSDictionaryRef userInfo, void (^handler)(RSTimerRef timer)) RS_AVAILABLE(0_3);

RSExport void RSTimerFire(RSTimerRef timer) RS_AVAILABLE(0_3);
RSExport void RSTimerInvalidate(RSTimerRef timer) RS_AVAILABLE(0_3);
RSExport void RSTimerSuspend(RSTimerRef timer) RS_AVAILABLE(0_3);

RSExport RSDictionaryRef RSTimerGetUserInfo(RSTimerRef timer) RS_AVAILABLE(0_3);
RSExport RSTimeInterval RSTimerGetTimeInterval(RSTimerRef timer) RS_AVAILABLE(0_3);

RS_EXTERN_C_END
#endif
