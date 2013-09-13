//
//  RSDistributedNotificationCenter.h
//  RSCoreFoundation
//
//  Created by RetVal on 3/17/13.
//  Copyright (c) 2013 RetVal. All rights reserved.
//

#ifndef RSCoreFoundation_RSDistributedNotificationCenter_h
#define RSCoreFoundation_RSDistributedNotificationCenter_h
RS_EXTERN_C_BEGIN
typedef struct __RSDistributedNotificationCenter* RSDistributedNotificationCenterRef;
RSExport RSTypeID RSDistributedNotificationCenterGetTypeID() RS_AVAILABLE(0_3);
RSExport RSDistributedNotificationCenterRef RSDistributedNotificationCenterGetDefault() RS_AVAILABLE(0_3);
RS_EXTERN_C_END
#endif
