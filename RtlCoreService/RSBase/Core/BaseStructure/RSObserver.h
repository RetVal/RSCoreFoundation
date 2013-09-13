//
//  RSObserver.h
//  RSCoreFoundation
//
//  Created by RetVal on 3/11/13.
//  Copyright (c) 2013 RetVal. All rights reserved.
//

#ifndef RSCoreFoundation_RSObserver_h
#define RSCoreFoundation_RSObserver_h
RS_EXTERN_C_BEGIN
typedef struct __RSObserver* RSObserverRef RS_AVAILABLE(0_1);
typedef void (*RSObserverCallback)(RSNotificationRef notification) RS_AVAILABLE(0_1);
RSExport RSTypeID RSObserverGetTypeID() RS_AVAILABLE(0_1);
RSExport RSObserverRef RSObserverCreate(RSAllocatorRef allocator, RSStringRef name, RSObserverCallback cb, RSTypeRef sender) RS_AVAILABLE(0_1);
RSExport RSStringRef RSObserverGetNotificationName(RSObserverRef observer) RS_AVAILABLE(0_1);
RSExport ISA RSObserverGetCallBack(RSObserverRef observer) RS_AVAILABLE(0_1);
RS_EXTERN_C_END
#endif
