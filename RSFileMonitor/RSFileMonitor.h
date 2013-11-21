//
//  RSFileMonitor.h
//  RSFileMonitor
//
//  Created by Closure on 11/13/13.
//  Copyright (c) 2013 RetVal. All rights reserved.
//

#include <RSCoreFoundation/RSCoreFoundation.h>

typedef struct __RSFileMonitor *RSFileMonitorRef;

RSExport RSTypeID RSFileMonitorGetTypeID() RS_AVAILABLE(0_4);

RSExport RSFileMonitorRef RSFileMonitorCreate(RSAllocatorRef allocator, RSStringRef filePath);

RSExport RSRunLoopSourceRef RSFileMonitorCreateRunLoopSource(RSAllocatorRef allocator, RSFileMonitorRef monitor, RSIndex order);