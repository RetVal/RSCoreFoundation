//
//  RSPrivateTimer.c
//  RSCoreFoundation
//
//  Created by RetVal on 5/20/13.
//  Copyright (c) 2013 RetVal. All rights reserved.
//

#include <RSCoreFoundation/RSDate.h>
#include "RSPrivateTimer.h"
#include <dispatch/dispatch.h>
#if TARGET_OS_MAC
#include <mach/mach_time.h>
#endif

#include <sys/time.h>

RSPrivate RSBitU64 __dps_absolute_time()
{
#if DEPLOYMENT_TARGET_MACOSX || DEPOLYMENT_TARGET_IPHONEOS || TARGET_OS_MAC || TARGET_OS_IPHONE
    return mach_absolute_time();
#endif
    return __dps_get_nanoseconds();
}

#define DPS_NSEC_PER_SEC 1000000000ull
#define DPS_USEC_PER_SEC 1000000ull
#define DPS_NSEC_PER_USEC 1000ull

RSPrivate RSBitU64 __dps_get_nanoseconds()
{
	struct timeval now;
	int r = gettimeofday(&now, nil);
    assert(r);
	return now.tv_sec * DPS_NSEC_PER_SEC + now.tv_usec * DPS_NSEC_PER_USEC;
}

RSPrivate RSBitU64 __dps_timeout(RSBitU64 when)
{
	RSBitU64 now;
	if (when == (~0ull)) {
		return (~0ull);
	}
	if (when == 0) {
		return 0;
	}
	if ((RSBit64)when < 0) {
		when = -(RSBit64)when;
		now = __dps_get_nanoseconds();
		return now >= when ? 0 : when - now;
	}
	now = __dps_absolute_time();
	return now >= when ? 0 : (when - now);
}

static RSBitU64 ___converter(RSAbsoluteTime timer)
{
    
    static mach_timebase_info_data_t timebase_info;
    if (timebase_info.denom == 0)
    {
        kern_return_t kr = mach_timebase_info(&timebase_info);
        if (kr != KERN_SUCCESS)
        {
            RSExceptionCreateAndRaise(RSAllocatorSystemDefault, RSGenericException, RSSTR("mach_timebase_info"), nil);
        }
    }
    
    return __dps_absolute_time()* timebase_info.numer / timebase_info.denom + timer * DPS_NSEC_PER_SEC;
}

RSPrivate RSBitU64 __dps_convert_absolute(RSAbsoluteTime absoluteTime)
{
    RSAbsoluteTime now = RSAbsoluteTimeGetCurrent();
    if (fabs(now -= absoluteTime) < 0.00001f)
    {
        return 0;
    }
    RSAbsoluteTime offset = now;
    if (offset > 0)
    {
        // now > absoluteTime
        return 0;
    }
    else
    {
        // now < absoluteTime
        now = -now;
        return ___converter(now);
    }
    return 0;
}

static void __dps_timer_cancel_default_handler_f(void *info)
{
    
}

static void (^__dps_timer_cancel_default_handler)() = ^{
    __dps_timer_cancel_default_handler_f(nil);
};

RSPrivate void __dps_timer_release(__dps_timer timer)
{
    dispatch_release(timer);
}

static dispatch_source_t ___dps_timer_create(uint64_t startTime,
                                             uint64_t repeatsTick,
                                             uint64_t leeway,
                                             dispatch_queue_t queue,
                                             dispatch_block_t block)
{
    if (repeatsTick == 0 || block == nil) return nil;
    if (queue == nil) queue = dispatch_get_current_queue();
    dispatch_source_t timer = dispatch_source_create(DISPATCH_SOURCE_TYPE_TIMER,
                                                     0, 0, queue);
    if (timer)
    {
        dispatch_source_set_timer(timer, dispatch_time(startTime, 0), repeatsTick, leeway);
        if (block)
        {
            dispatch_source_set_event_handler(timer, block);
            dispatch_source_set_cancel_handler(timer, __dps_timer_cancel_default_handler);
        }
        else
        {
            __dps_timer_release(timer);
            timer = nil;
        }
    }
    return timer;
}

RSPrivate __dps_timer __dps_timer_create(RSBitU64 startTime, RSBitU64 repeatsTick, RSBitU64 leeway, BOOL repeats, RSRunLoopRef rl, void (^timer_repeats_handler)())
{
    __dps_timer timer = ___dps_timer_create(startTime, repeatsTick, leeway, __RSRunLoopGetQueue(rl), timer_repeats_handler);
    return timer;
}
RSPrivate void __dps_timer_set_context(__dps_timer timer, void* context)
{
    dispatch_set_context(timer, context);
}

RSPrivate void*__dps_timer_get_context(__dps_timer timer)
{
    return dispatch_get_context(timer);
}

RSPrivate void __dps_timer_fire(__dps_timer timer)
{
    if (timer) dispatch_resume(timer);
}

RSPrivate void __dps_timer_stop(__dps_timer timer)
{
    if (timer) dispatch_suspend(timer);
}

RSPrivate void __dps_timer_invalid(__dps_timer timer)
{
    if (timer == nil) return;
    dispatch_source_cancel(timer);
}
