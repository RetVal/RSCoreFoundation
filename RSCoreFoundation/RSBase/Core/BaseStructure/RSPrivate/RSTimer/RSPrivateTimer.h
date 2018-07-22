//
//  RSPrivateTimer.h
//  RSCoreFoundation
//
//  Created by RetVal on 5/20/13.
//  Copyright (c) 2013 RetVal. All rights reserved.
//

#ifndef RSCoreFoundation_RSPrivateTimer_h
#define RSCoreFoundation_RSPrivateTimer_h

#include <RSCoreFoundation/RSRunLoop.h>
typedef void* __dps_timer;
RSPrivate RSBitU64 __dps_absolute_time(void);
RSPrivate RSBitU64 __dps_get_nanoseconds(void);
RSPrivate RSBitU64 __dps_timeout(RSBitU64 when);
RSPrivate RSBitU64 __dps_convert_absolute(RSAbsoluteTime absoluteTime);
RSPrivate __dps_timer __dps_timer_create(RSBitU64 startTime, RSBitU64 repeatsTick, RSBitU64 leeway, BOOL repeats, RSRunLoopRef rl, void (^timer_repeats_handler)(void));
RSPrivate void __dps_timer_set_context(__dps_timer timer, void* context);
RSPrivate void*__dps_timer_get_context(__dps_timer timer);
RSPrivate void __dps_timer_fire(__dps_timer timer);
RSPrivate void __dps_timer_stop(__dps_timer timer);
RSPrivate void __dps_timer_invalid(__dps_timer timer);
RSPrivate void __dps_timer_release(__dps_timer timer);
#endif
