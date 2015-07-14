//
//  RSPriv.h
//  RSCoreFoundation
//
//  Created by closure on 7/14/15.
//  Copyright © 2015 RetVal. All rights reserved.
//

#ifndef RSPriv_h
#define RSPriv_h

#include <RSCoreFoundation/RSBase.h>
#include <RSCoreFoundation/RSMessagePort.h>

RSExtern void _RSMachPortInstallNotifyPort(RSRunLoopRef rl, RSStringRef mode);
RSExtern int64_t __RSTimeIntervalToTSR(RSTimeInterval ti);
RSExtern RSTimeInterval __RSTimeIntervalUntilTSR(uint64_t tsr);

typedef RSDataRef (*RSMessagePortCallBackEx)(RSMessagePortRef local, SInt32 msgid, RSDataRef data, void *info, void *trailer, uintptr_t);

#endif /* RSPriv_h */
