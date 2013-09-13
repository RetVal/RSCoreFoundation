//
//  RSNotification.h
//  RSCoreFoundation
//
//  Created by RetVal on 3/11/13.
//  Copyright (c) 2013 RetVal. All rights reserved.
//

#ifndef RSCoreFoundation_RSNotification_h
#define RSCoreFoundation_RSNotification_h
#include <RSCoreFoundation/RSDictionary.h>
RS_EXTERN_C_BEGIN
typedef struct __RSNotification*    RSNotificationRef RS_AVAILABLE(0_1);
RSExport RSTypeID RSNotificationGetTypeID() RS_AVAILABLE(0_1);
RSExport RSNotificationRef RSNotificationCreate(RSAllocatorRef allocator, RSStringRef name, RSTypeRef obj, RSDictionaryRef userInfo) RS_AVAILABLE(0_1);
RSExport RSStringRef RSNotificationGetName(RSNotificationRef notification) RS_AVAILABLE(0_1);
RSExport RSStringRef RSNotificationCopyName(RSNotificationRef notification) RS_AVAILABLE(0_1);
RSExport RSStringRef RSNotificationGetObject(RSNotificationRef notification) RS_AVAILABLE(0_1);
RSExport RSTypeRef RSNotificationCopyObject(RSNotificationRef notification) RS_AVAILABLE(0_1);
RSExport RSDictionaryRef RSNotificationGetUserInfo(RSNotificationRef notification) RS_AVAILABLE(0_1);
RSExport RSDictionaryRef RSNotificationCopyUserInfo(RSNotificationRef notification) RS_AVAILABLE(0_1);

RSExport RSNotificationRef RSNotificationWithName(RSStringRef name, RSDictionaryRef userInfo, RSTypeRef obj);
RS_EXTERN_C_END 
#endif
