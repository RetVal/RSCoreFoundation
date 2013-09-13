//
//  RSTimer.c
//  RSCoreFoundation
//
//  Created by RetVal on 5/21/13.
//  Copyright (c) 2013 RetVal. All rights reserved.
//

#include <RSCoreFoundation/RSTimer.h>
#include <RSCoreFoundation/RSRuntime.h>
#include "RSPrivate/RSTimer/RSPrivateTimer.h"

static const RSBitU64 __dps_timer_tick_per_second = 1000000000ull;

struct __RSTimerDescriptor {
    RSTimeInterval _timeInterval;
    RSBitU64 _absolute_time;
};

struct __RSTimer {
    RSRuntimeBase _base;
    struct __RSTimerDescriptor _descriptor;
    __dps_timer _dps_timer;
    RSDictionaryRef _userInfo;
};

enum __RSTimerHeaderFlags {
    __RSTimerDescriptor = 0x1,
    __RSTimerRepeat = 0x2,
    __RSTimerValid = 0x3,
    __RSTimerFired = 0x4,
};

RSInline BOOL __RSTimerIsDescriptor(RSTimerRef rs)
{
    return __RSBitfieldGetValue(RSRuntimeClassBaseFiled(rs), __RSTimerDescriptor, __RSTimerDescriptor);
}

RSInline BOOL __RSTimerIsTimer(RSTimerRef rs)
{
    return !__RSBitfieldGetValue(RSRuntimeClassBaseFiled(rs), __RSTimerDescriptor, __RSTimerDescriptor);
}

RSInline void __RSTimerSetDescriptor(RSTimerRef rs)
{
    __RSBitfieldSetValue(RSRuntimeClassBaseFiled(rs), __RSTimerDescriptor, __RSTimerDescriptor, 1);
}

RSInline void __RSTimerSetTimer(RSTimerRef rs)
{
    __RSBitfieldSetValue(RSRuntimeClassBaseFiled(rs), __RSTimerDescriptor, __RSTimerDescriptor, 0);
}


RSInline BOOL __RSTimerIsRepeat(RSTimerRef rs)
{
    return __RSBitfieldGetValue(RSRuntimeClassBaseFiled(rs), __RSTimerRepeat, __RSTimerRepeat);
}

RSInline void __RSTimerSetRepeat(RSTimerRef rs)
{
    __RSBitfieldSetValue(RSRuntimeClassBaseFiled(rs), __RSTimerRepeat, __RSTimerRepeat, 1);
}

RSInline void __RSTimerSetUnrepeat(RSTimerRef rs)
{
    __RSBitfieldSetValue(RSRuntimeClassBaseFiled(rs), __RSTimerRepeat, __RSTimerRepeat, 0);
}

RSInline BOOL __RSTimerIsValid(RSTimerRef rs)
{
    return __RSBitfieldGetValue(RSRuntimeClassBaseFiled(rs), __RSTimerValid, __RSTimerValid);
}

RSInline void __RSTimerSetValid(RSTimerRef rs)
{
    __RSBitfieldSetValue(RSRuntimeClassBaseFiled(rs), __RSTimerValid, __RSTimerValid, 1);
}

RSInline void __RSTimerSetInvalid(RSTimerRef rs)
{
    __RSBitfieldSetValue(RSRuntimeClassBaseFiled(rs), __RSTimerValid, __RSTimerValid, 0);
}

RSInline BOOL __RSTimerIsFired(RSTimerRef rs)
{
    return __RSBitfieldGetValue(RSRuntimeClassBaseFiled(rs), __RSTimerFired, __RSTimerFired);
}

RSInline void __RSTimerSetFired(RSTimerRef rs)
{
    __RSBitfieldSetValue(RSRuntimeClassBaseFiled(rs), __RSTimerFired, __RSTimerFired, 1);
}

RSInline BOOL __RSTimerIsSuspend(RSTimerRef rs)
{
    return !__RSBitfieldGetValue(RSRuntimeClassBaseFiled(rs), __RSTimerFired, __RSTimerFired);
}

RSInline void __RSTimerSetSuspend(RSTimerRef rs)
{
    __RSBitfieldSetValue(RSRuntimeClassBaseFiled(rs), __RSTimerFired, __RSTimerFired, 0);
}


RSInline __dps_timer __RSTimerGetDpsTimer(RSTimerRef rs)
{
    if (__RSTimerIsTimer(rs)) return (rs->_dps_timer);
    return nil;
}

static void __RSTimerDescriptorInit(RSTimerRef timer, RSBitU64 when, RSTimeInterval timeInterval)
{
    if (__RSTimerIsDescriptor(timer))
    {
        struct __RSTimer *_timer = (struct __RSTimer *)timer;
        _timer->_descriptor._absolute_time = when;
        _timer->_descriptor._timeInterval = timeInterval;
    }
}

static void __RSTimerClassInit(RSTypeRef rs)
{
    RSTimerRef timer = (RSTimerRef)rs;
//    timer->_dps_timer = __dps_timer_create(RSRunLoopCurrentLoop());
    __RSTimerSetDescriptor(timer);
    __RSTimerSetInvalid(timer);
}

static RSTypeRef __RSTimerClassCopy(RSAllocatorRef allocator, RSTypeRef rs, BOOL mutableCopy)
{
    return RSRetain(rs);
}

static void __RSTimerClassDeallocate(RSTypeRef rs)
{
    RSTimerRef timer = (RSTimerRef)rs;
    struct __RSTimer *_timer = (struct __RSTimer *)timer;
    if (__RSTimerIsTimer(timer))
    {
        if (timer->_dps_timer)
        {
//            __dps_timer_invalid(timer->_dps_timer);
            if (__RSTimerIsSuspend(timer))
            {
                RSExceptionCreateAndRaise(RSAllocatorSystemDefault, RSGenericException, RSSTR("BUG: RSTimer want to invalidate a suspend timer"), RSAutorelease(RSDictionaryCreateWithObjectsAndOKeys(RSAllocatorSystemDefault, timer, RSExceptionObject, NULL)));
                return;
            }
            __dps_timer_release(timer->_dps_timer);
        }
        _timer->_dps_timer = nil;
    }
    memset(&_timer->_descriptor, 0, sizeof(struct __RSTimerDescriptor));
    if (timer->_userInfo) RSRelease(timer->_userInfo);
}

static BOOL __RSTimerClassEqual(RSTypeRef rs1, RSTypeRef rs2)
{
    return (((RSTimerRef)rs1)->_dps_timer) == (((RSTimerRef)rs2)->_dps_timer);
}

static RSHashCode __RSTimerClassHash(RSTypeRef rs)
{
    RSTimerRef timer = (RSTimerRef)rs;
    return ((RSHashCode)timer) ^ ((RSHashCode)timer->_dps_timer);
}

static RSStringRef __RSTimerClassDescription(RSTypeRef rs)
{
    RSTimerRef timer = (RSTimerRef)rs;
    return RSStringCreateWithFormat(RSAllocatorSystemDefault, RSSTR("%s <%p>"), __RSRuntimeGetClassNameWithInstance(timer), timer);
}

static RSRuntimeClass __RSTimerClass = {
    _RSRuntimeScannedObject,
    "RSTimer",
    __RSTimerClassInit,
    __RSTimerClassCopy,
    __RSTimerClassDeallocate,
    __RSTimerClassEqual,
    __RSTimerClassHash,
    __RSTimerClassDescription,
    nil,
    nil
};

static RSTypeID _RSTimerTypeID = _RSRuntimeNotATypeID;

static void __RSTimerInitialize()
{
    _RSTimerTypeID = __RSRuntimeRegisterClass(&__RSTimerClass);
    __RSRuntimeSetClassTypeID(&__RSTimerClass, _RSTimerTypeID);
}

RSExport RSTypeID RSTimerGetTypeID()
{
    if (_RSTimerTypeID == _RSRuntimeNotATypeID) __RSTimerInitialize();
    return _RSTimerTypeID;
}

static RSTimerRef __RSTimerCreateInstance(RSAllocatorRef allocator, RSAbsoluteTime start, RSTimeInterval timeInterval, BOOL repeats, RSDictionaryRef userInfo)
{
    RSTimerRef timer = (RSTimerRef)__RSRuntimeCreateInstance(allocator, RSTimerGetTypeID(), sizeof(struct __RSTimer) - sizeof(struct __RSRuntimeBase));
    struct __RSTimer *_timer = (struct __RSTimer *)timer;
    if (userInfo) _timer->_userInfo = RSRetain(userInfo);
    RSBitU64 _start = __dps_convert_absolute(start);
    __RSTimerDescriptorInit(timer, _start, timeInterval);
    if (repeats) __RSTimerSetRepeat(timer);
    return timer;
}

static RSTimerRef __RSTimerCreateWithTimerDescritorInside(RSTimerRef timer, void (^handler)())
{
    if (timer == nil) return nil;
    if (!__RSTimerIsDescriptor(timer)) return nil;
    struct __RSTimer *_timer = (struct __RSTimer *)timer;
    _timer->_dps_timer = __dps_timer_create(timer->_descriptor._absolute_time, timer->_descriptor._timeInterval * __dps_timer_tick_per_second, 0, __RSTimerIsRepeat(timer), RSRunLoopCurrentLoop(), handler);
    if (timer->_dps_timer == nil) return nil;
    __dps_timer_set_context(timer->_dps_timer, _timer);
    __RSTimerSetTimer(timer);
    __RSTimerSetValid(timer);
    return timer;
}

RSExport RSTimerRef RSTimerCreateSchedule(RSAllocatorRef allocator, RSAbsoluteTime start, RSTimeInterval timeInterval, BOOL repeats, RSDictionaryRef userInfo, void (^handler)(RSTimerRef timer))
{
    if (handler == nil) return nil;
    if (timeInterval <= 0) timeInterval = 1.0f;
    if (start == 0) start = RSAbsoluteTimeGetCurrent();
    RSTimerRef timer = __RSTimerCreateInstance(allocator, start, timeInterval, repeats, userInfo);
    if (timer)
    {
        if (repeats)
            return __RSTimerCreateWithTimerDescritorInside(timer, ^{
                handler(__dps_timer_get_context(__RSTimerGetDpsTimer(timer)));
            });
        else
            return __RSTimerCreateWithTimerDescritorInside(timer, ^{
                RSRetain(timer);
                handler(__dps_timer_get_context(__RSTimerGetDpsTimer(timer)));
                if (__RSTimerIsValid(timer) == YES)
                    RSTimerInvalidate(timer);
                RSRelease(timer);
            });
    }
    return timer;
}

RSExport void RSTimerFire(RSTimerRef timer)
{
    if (timer == nil) return;
    __RSGenericValidInstance(timer, _RSTimerTypeID);
    if (__RSTimerIsValid(timer) && NO == __RSTimerIsFired(timer))
    {
        __dps_timer_fire(__RSTimerGetDpsTimer(timer));
        __RSTimerSetFired(timer);
    }
    else
    {
        RSExceptionCreateAndRaise(RSAllocatorSystemDefault, RSGenericException, RSSTR("BUG: RSTimer over fire"), RSAutorelease(RSDictionaryCreateWithObjectsAndOKeys(RSAllocatorSystemDefault, timer, RSExceptionObject, NULL)));
    }
}

RSExport void RSTimerInvalidate(RSTimerRef timer)
{
    if (timer == nil) return ;
    __RSGenericValidInstance(timer, _RSTimerTypeID);
    if (__RSTimerIsValid(timer) && NO == __RSTimerIsSuspend(timer))
    {
        __dps_timer_invalid(__RSTimerGetDpsTimer(timer));
        __RSTimerSetInvalid(timer);
        RSRelease(timer);
    }
    else
    {
        RSExceptionCreateAndRaise(RSAllocatorSystemDefault, RSGenericException, RSSTR("BUG: RSTimer over invalidate or invalidate a suspend timer"), RSAutorelease(RSDictionaryCreateWithObjectsAndOKeys(RSAllocatorSystemDefault, timer, RSExceptionObject, NULL)));
    }
}

RSExport void RSTimerSuspend(RSTimerRef timer)
{
    if (timer == nil) return;
    __RSGenericValidInstance(timer, _RSTimerTypeID);
    if (__RSTimerIsValid(timer) && __RSTimerIsFired(timer))
    {
        __dps_timer_stop(__RSTimerGetDpsTimer(timer));
        __RSTimerSetSuspend(timer);
    }
    else
    {
        RSExceptionCreateAndRaise(RSAllocatorSystemDefault, RSGenericException, RSSTR("BUG: RSTimer over invalidate or over suspend"), RSAutorelease(RSDictionaryCreateWithObjectsAndOKeys(RSAllocatorSystemDefault, timer, RSExceptionObject, NULL)));
    }
}

RSExport RSDictionaryRef RSTimerGetUserInfo(RSTimerRef timer)
{
    if (timer == nil) return nil;
    __RSGenericValidInstance(timer, _RSTimerTypeID);
    return timer->_userInfo;
}

RSExport RSTimeInterval RSTimerGetTimeInterval(RSTimerRef timer)
{
    if (timer == nil) return 0.f;
    __RSGenericValidInstance(timer, _RSTimerTypeID);
    return timer->_descriptor._timeInterval;
}

RSExport BOOL RSTimerIsValid(RSTimerRef timer)
{
    if (timer == nil) return NO;
    __RSGenericValidInstance(timer, _RSTimerTypeID);
    return __RSTimerIsValid(timer);
}