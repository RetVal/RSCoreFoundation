//
//  RSRenrenEvent.h
//  RSRenrenCore
//
//  Created by closure on 1/3/14.
//  Copyright (c) 2014 closure. All rights reserved.
//

#ifndef RSRenrenCore_RSRenrenEvent
#define RSRenrenCore_RSRenrenEvent

#include <RSCoreFoundation/RSCoreFoundation.h>

RS_EXTERN_C_BEGIN

typedef const struct __RSRenrenEvent *RSRenrenEventRef;

RSExport RSTypeID RSRenrenEventGetTypeID();
RSExport RSRenrenEventRef RSRenrenEventCreateLikeWithContent(RSAllocatorRef allocator, RSStringRef content, BOOL addlike);
RSExport void RSRenrenEventDo(RSRenrenEventRef event);
RS_EXTERN_C_END
#endif 
