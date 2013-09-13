//
//  RSNotificationCenter.h
//  RSCoreFoundation
//
//  Created by RetVal on 12/27/12.
//  Copyright (c) 2012 RetVal. All rights reserved.
//

#ifndef RSCoreFoundation_RSNotificationCenter_h
#define RSCoreFoundation_RSNotificationCenter_h
#include <RSCoreFoundation/RSRunLoop.h>
#include <RSCoreFoundation/RSNotification.h>
#include <RSCoreFoundation/RSObserver.h>
RS_EXTERN_C_BEGIN
typedef struct __RSNotificationCenter* RSNotificationCenterRef RS_AVAILABLE(0_1);
RSExport RSTypeID RSNotificationCenterGetTypeID() RS_AVAILABLE(0_1);
RSExport RSNotificationCenterRef RSNotificationCenterGetDefault() RS_AVAILABLE(0_1);
RSExport void RSNotificationCenterAddObserver(RSNotificationCenterRef notificationCenter, RSObserverRef observer) RS_AVAILABLE(0_1);
RSExport void RSNotificationCenterRemoveObserver(RSNotificationCenterRef notificationCenter, RSObserverRef observer) RS_AVAILABLE(0_1);
RSExport void RSNotificationCenterPostNotification(RSNotificationCenterRef notificationCenter, RSNotificationRef notification) RS_AVAILABLE(0_1);
RSExport void RSNotificationCenterPost(RSNotificationCenterRef notificationCenter, RSStringRef name, RSTypeRef obj, RSDictionaryRef userInfo) RS_AVAILABLE(0_1);
RSExport void RSNotificationCenterPostNotificationImmediately(RSNotificationCenterRef notificationCenter, RSNotificationRef notification) RS_AVAILABLE(0_1);
RSExport void RSNotificationCenterPostImmediately(RSNotificationCenterRef notificationCenter, RSStringRef name, RSTypeRef obj, RSDictionaryRef userInfo) RS_AVAILABLE(0_1);

RSExport RSStringRef const RSNotificationMode RS_AVAILABLE(0_1);
RS_EXTERN_C_END
#endif
