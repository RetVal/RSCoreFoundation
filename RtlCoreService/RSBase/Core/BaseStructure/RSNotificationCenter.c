//
//  RSNotificationCenter.c
//  RSCoreFoundation
//
//  Created by RetVal on 12/27/12.
//  Copyright (c) 2012 RetVal. All rights reserved.
//

#include <RSCoreFoundation/RSDictionary.h>
#include <RSCoreFoundation/RSNil.h>
#include <RSCoreFoundation/RSNotificationCenter.h>
#include <RSCoreFoundation/RSDistributedNotificationCenter.h>
#include <RSCoreFoundation/RSNumber.h>
#include <RSCoreFoundation/RSPropertyList.h>
#include <RSCoreFoundation/RSQueue.h>
#include <RSCoreFoundation/RSSet.h>
#include <RSCoreFoundation/RSSocket.h>
#include <RSCoreFoundation/RSRuntime.h>

//RSInside RSSpinLock __RSNotificationCenterLock = RSSpinLockInit;
#pragma mark Notification
struct __RSNotification
{
    RSRuntimeBase _base;
    RSStringRef _name;
    RSTypeRef _object;
    RSDictionaryRef _userInfo;
};

static void __RSNotificationClassInit(RSTypeRef rs)
{
    RSNotificationRef notification = (RSNotificationRef)rs;
    notification->_name = nil;
    notification->_object = nil;
    notification->_userInfo = nil;
}

static RSTypeRef __RSNotificationClassCopy(RSAllocatorRef allocator, RSTypeRef rs, BOOL mutableCopy)
{
    return rs;
}

static RSStringRef __RSNotificationClassDescription(RSTypeRef rs)
{
    RSNotificationRef notification = (RSNotificationRef)rs;
    RSMutableStringRef description = RSStringCreateMutable(RSAllocatorSystemDefault, 128);
    RSStringAppendStringWithFormat(description, RSSTR("RSNotification - named : %R\n UserInfo - %R\n"), notification->_name, notification->_userInfo);
    return description;
}

static void __RSNotificationClassDeallocate(RSTypeRef rs)
{
    RSNotificationRef notification = (RSNotificationRef)rs;
    if (notification->_name) RSRelease(notification->_name);
    if (notification->_object) RSRelease(notification->_object);
    if (notification->_userInfo) RSRelease(notification->_userInfo);
    return;
}

static BOOL __RSNotificationClassEqual(RSTypeRef rs1, RSTypeRef rs2)
{
    RSNotificationRef n1 = (RSNotificationRef)rs1;
    RSNotificationRef n2 = (RSNotificationRef)rs2;
    if ((n1->_name && n2->_name))
    {if (NO == RSEqual(n1->_name, n2->_name)) return NO;}
    else if (!(n1->_name == nil && n2->_name == nil)) return NO;
    
    if ((n1->_object && n2->_object))
    {if (NO == RSEqual(n1->_object, n2->_object)) return NO;}
    else if (!(n1->_object == nil && n2->_object == nil)) return NO;
    
    if ((n1->_userInfo && n2->_userInfo))
    {if (NO == RSEqual(n1->_userInfo, n2->_userInfo)) return NO;}
    else if (!(n1->_userInfo == nil && n2->_userInfo == nil)) return NO;
    
    return YES;
}

static RSHashCode __RSNotificationClassHash(RSTypeRef rs)
{
    RSNotificationRef notification = (RSNotificationRef)rs;
    if (notification->_name)
        return RSHash(notification->_name);
    if (notification->_object && notification->_object != rs)
        return RSHash(notification->_object);
    if (notification->_userInfo)
        return RSHash(notification->_userInfo);
    return (RSHashCode)notification;
}

static const RSRuntimeClass __RSNotificationClass =
{
    _RSRuntimeScannedObject,
    "RSNotification",
    __RSNotificationClassInit,
    __RSNotificationClassCopy,
    __RSNotificationClassDeallocate,
    __RSNotificationClassEqual,
    __RSNotificationClassHash,
    __RSNotificationClassDescription,
    nil,
    nil,
};

static RSTypeID _RSNotificationTypeID = _RSRuntimeNotATypeID;
RSPrivate void __RSNotificationInitialize()
{
    _RSNotificationTypeID = __RSRuntimeRegisterClass(&__RSNotificationClass);
    __RSRuntimeSetClassTypeID(&__RSNotificationClass, _RSNotificationTypeID);
    
    
}

RSExport RSTypeID RSNotificationGetTypeID()
{
    return _RSNotificationTypeID;
}

RSExport RSNotificationRef RSNotificationCreate(RSAllocatorRef allocator, RSStringRef name, RSTypeRef obj, RSDictionaryRef userInfo)
{
    if (name == nil) HALTWithError(RSInvalidArgumentException, "the name of the notification must be non-nil.");
    RSNotificationRef notification = (RSNotificationRef)__RSRuntimeCreateInstance(allocator, _RSNotificationTypeID, sizeof(struct __RSNotification) - sizeof(struct __RSRuntimeBase));
    notification->_name = name ? RSRetain(name) : nil;
    notification->_object = obj ? RSRetain(obj) : nil;
    notification->_userInfo = userInfo ? RSRetain(userInfo) : nil;
    return notification;
}

RSExport RSStringRef RSNotificationGetName(RSNotificationRef notification)
{
    __RSGenericValidInstance(notification, _RSNotificationTypeID);
    return notification->_name;
}

RSExport RSStringRef RSNotificationCopyName(RSNotificationRef notification)
{
    RSStringRef name = RSNotificationGetName(notification);
    return name ? RSRetain(name) : nil;
}

RSExport RSStringRef RSNotificationGetObject(RSNotificationRef notification)
{
    __RSGenericValidInstance(notification, _RSNotificationTypeID);
    return notification->_object;
}

RSExport RSTypeRef RSNotificationCopyObject(RSNotificationRef notification)
{
    RSTypeRef object = RSNotificationGetObject(notification);
    return object ? RSCopy(RSAllocatorSystemDefault, object) : nil;// if name = nil, runtime automatically set with RSNil. so it's fine.
}

RSExport RSDictionaryRef RSNotificationGetUserInfo(RSNotificationRef notification)
{
    __RSGenericValidInstance(notification, _RSNotificationTypeID);
    return notification->_userInfo;
}

RSExport RSDictionaryRef RSNotificationCopyUserInfo(RSNotificationRef notification)
{
    RSDictionaryRef info = RSNotificationGetUserInfo(notification);
    return info ? RSRetain(info) : nil;
}

#pragma mark -
#pragma mark Observer

struct __RSObserver
{
    RSRuntimeBase _base;
    pthread_t _pthread;
    
    RSStringRef _notificationName;
    RSTypeRef _sender;
    union{
        RSMutableSetRef _callbacks;     // V0.4 RSNumber inside, is function call back address.
                                        // V0.4+ __RSObserver inside, for checking sender
        RSObserverCallback _callback;
    };
};

RSInline BOOL __RSObserverIsUserCreate(RSObserverRef rs)
{
    return __RSBitfieldGetValue(rs->_base._rsinfo._rsinfo1, 3, 3);
}

RSInline BOOL __RSObserverIsRuntimeCreate(RSObserverRef rs)
{
    return !__RSBitfieldGetValue(rs->_base._rsinfo._rsinfo1, 3, 3);
}

RSInline void __RSObserverSetUserCreate(RSObserverRef rs)
{
    __RSBitfieldSetValue(rs->_base._rsinfo._rsinfo1, 3, 3, 1);
}

RSInline void __RSObserverSetRuntimeCreate(RSObserverRef rs)
{
    __RSBitfieldSetValue(rs->_base._rsinfo._rsinfo1, 3, 3, 0);
}

static void __RSObserverClassInit(RSTypeRef rs)
{
    RSObserverRef observer = (RSObserverRef)rs;
    observer->_pthread = pthread_self();
}

static void __RSObserverClassDeallocate(RSTypeRef rs)
{
    RSObserverRef observer = (RSObserverRef)rs;
    observer->_pthread = nil;
    if (observer->_notificationName) RSRelease(observer->_notificationName);
    if (__RSObserverIsRuntimeCreate(observer))
    {
        if (observer->_callbacks)
            RSRelease(observer->_callbacks);
    }
    else observer->_callback = nil;
    return;
}

static BOOL __RSObserverClassEqual(RSTypeRef rs1, RSTypeRef rs2)
{
    RSObserverRef ob1 = (RSObserverRef)rs1;
    RSObserverRef ob2 = (RSObserverRef)rs2;
    if (ob1->_notificationName && ob2->_notificationName)
    {
        if (NO == RSEqual(ob1->_notificationName, ob2->_notificationName))
            return NO;
    }
    if ((ob1->_notificationName != nil && ob2->_notificationName == nil) ||
        (ob1->_notificationName == nil && ob2->_notificationName != nil))
        return NO;
    BOOL runtmeCreate = __RSObserverIsRuntimeCreate(ob1);
    if (runtmeCreate != __RSObserverIsRuntimeCreate(ob2)) return NO;
    else if (runtmeCreate)
    {
        if (ob1->_callbacks && ob2->_callbacks)
        {
            if (NO == RSEqual(ob1->_callbacks, ob2->_callbacks))
                return NO;
        }
        if ((ob1->_callbacks != nil && ob2->_callbacks == nil) ||
            (ob1->_callbacks == nil && ob2->_callbacks != nil))
            return NO;
        
    }
    else {
        //user create
        if (ob1->_callback != ob2->_callback) return NO;
    }
    return YES;
}

static RSStringRef __RSObserverClassDescription(RSTypeRef rs)
{
    RSObserverRef ob = (RSObserverRef)rs;
    RSMutableStringRef description = RSStringCreateMutable(RSAllocatorSystemDefault, 128);
    RSStringRef threadName = __RSRunLoopThreadToString(ob->_pthread);
#if __LP64__
    RSStringAppendStringWithFormat(description, RSSTR("RSObserver(%R) at <%R> - %lld"), ob->_notificationName, threadName, RSSetGetCount(ob->_callbacks));
#else
    RSStringAppendStringWithFormat(description, RSSTR("RSObserver(%R) at <%R> - %ld"), ob->_notificationName, threadName, RSSetGetCount(ob->_callbacks));
#endif
    RSRelease(threadName);
    return description;
}

static RSHashCode __RSObserverClassHash(RSTypeRef rs)
{
    RSObserverRef ob = (RSObserverRef)rs;
    RSStringRef thread = __RSRunLoopThreadToString(ob->_pthread);
    RSHashCode code =  RSHash(((RSObserverRef)rs)->_notificationName)^RSHash(thread);
    RSRelease(thread);
    return code;
}

static const RSRuntimeClass __RSObserverClass = {
    _RSRuntimeScannedObject,
    "RSObserver",
    __RSObserverClassInit,
    nil,
    __RSObserverClassDeallocate,
    __RSObserverClassEqual,
    __RSObserverClassHash,
    __RSObserverClassDescription,
    nil,
    nil
};

static RSTypeID _RSObserverTypeID = _RSRuntimeNotATypeID;
RSPrivate void __RSObserverInitialize()
{
    _RSObserverTypeID = __RSRuntimeRegisterClass(&__RSObserverClass);
    __RSRuntimeSetClassTypeID(&__RSObserverClass, _RSObserverTypeID);
}

RSExport RSTypeID RSObserverGetTypeID()
{
    return _RSObserverTypeID;
}

RSPrivate RSObserverRef __RSObserverCreate(RSAllocatorRef allocator, RSTypeRef sender, RSStringRef notificationName, void* callback, BOOL runtimeCall)
{
    RSObserverRef observer = (RSObserverRef)__RSRuntimeCreateInstance(allocator, _RSObserverTypeID, sizeof(struct __RSObserver) - sizeof(struct __RSRuntimeBase));
    observer->_notificationName = RSRetain(notificationName);
    observer->_callbacks = nil;
    observer->_sender = sender;
    if (runtimeCall)
    {
        __RSObserverSetRuntimeCreate(observer);
        observer->_callbacks = RSSetCreateMutable(allocator, 0, &RSTypeSetCallBacks);
    }
    else
    {
        __RSObserverSetUserCreate(observer);
        observer->_callback = callback;
    }
    
    return observer;
}

RSExport RSObserverRef RSObserverCreate(RSAllocatorRef allocator, RSStringRef name, RSObserverCallback cb, RSTypeRef sender)
{
    if (cb == nil || name == nil) return nil;
    return __RSObserverCreate(allocator, sender, name, cb, NO);
}

RSExport RSStringRef RSObserverGetNotificationName(RSObserverRef observer)
{
    __RSGenericValidInstance(observer, _RSObserverTypeID);
    return observer->_notificationName;
}

RSExport ISA RSObserverGetCallBack(RSObserverRef observer)
{
    __RSGenericValidInstance(observer, _RSObserverTypeID);
    if (__RSObserverIsRuntimeCreate(observer)) {
        return nil;
    }
    return (ISA)observer->_callback;
}

#pragma mark -
#pragma mark Notification Center
/*
 Notification push in a queue, wait for sending to the destination.
 Notification receive
 Notification Center receive all link request though _centerPort, then the two remote client can communicate with each other.
 EG: task A want to communicate with task B, but A do not know where B is, and is same to B. So A send a packet to NotificationCenter to want to connect with B, B also do the something.
 NotificationCenter get the request from A, called RA, do the follow operations:
 1. get the convid from RSMessagePort A.
 2. wait for the second message with the same convid, then check if the MessagePort is same to A.
 3. if is not equal to A, it is B, go to step 4. Otherwise, is re-send message, go to step 2.
 4. NotificationCenter reset Port A, the replyport will be reset with B information, send back to A, do the same thing with Port B.
 5. set the port's connect status, remove it from the queue, check the queue is empty or not, if not, go to step 1.
 all those steps are running in the source.
 All format of data in the message channel is XML(plist).
 */
typedef struct __RSProcessInNotificationCenter   // one task one notification center. single instance.
{
    //RSMessagePortRef _centerPort;   // boot start port for this task
    
    RSSpinLock _observerLock;
    RSMutableDictionaryRef _observers;     // <notification, observer>
    
    RSQueueRef _queue;       // RSMessagePortRef
    RSRunLoopSourceRef _sendSource;
}__RSProcessInNotificationCenter;

struct __RSNotificationCenter   // one task one notification center. single instance.
{
    RSRuntimeBase _base;
    RSSpinLock _observerLock;
    RSMutableDictionaryRef _observers;     // <notification, observer>
    
    RSQueueRef _queue;       // RSMessagePortRef
    RSRunLoopSourceRef _sendSource;
};

RSInline void __RSNotificationCenterProcessInLockObservers(RSNotificationCenterRef nc) {RSSpinLockLock(&nc->_observerLock);}
RSInline void __RSNotificationCenterProcessInUnlockObservers(RSNotificationCenterRef nc) {RSSpinLockUnlock(&nc->_observerLock);}

#if 0
#define ____RSNotificationCenterProcessInLockObservers(nc) do{__RSCLog(RSLogLevelDebug, "__RSNotificationCenterProcessInLockObservers   call in %s<%p>\n", __func__, pthread_self());__RSNotificationCenterProcessInLockObservers(nc);}while(0)
#define ____RSNotificationCenterProcessInUnlockObservers(nc) do{__RSCLog(RSLogLevelDebug, "__RSNotificationCenterProcessInUnlockObservers call in %s<%p>\n", __func__, pthread_self());__RSNotificationCenterProcessInUnlockObservers(nc);}while(0)
#else
#define ____RSNotificationCenterProcessInLockObservers(nc) __RSNotificationCenterProcessInLockObservers(nc)
#define ____RSNotificationCenterProcessInUnlockObservers(nc) __RSNotificationCenterProcessInUnlockObservers(nc)
#endif

static void __RSProcessInNotificationCenterClassInit(RSNotificationCenterRef nc)
{
    nc->_observerLock = RSSpinLockInit;
    nc->_observers = RSDictionaryCreateMutable(RSAllocatorSystemDefault, 0, RSDictionaryRSTypeContext);
}

static void __RSNotificationCenterClassInit(RSTypeRef rs)
{
    RSNotificationCenterRef nc = (RSNotificationCenterRef)rs;
    //nc->_centerPort = nil;
    __RSProcessInNotificationCenterClassInit(nc);
}

static RSSpinLock __RSNotificationCenterlock = RSSpinLockInit;
static RSNotificationCenterRef __defaultNC = nil;
#ifndef __RSNCL // only call in public api
#define __RSNCL RSSpinLockLock(&__RSNotificationCenterlock)
#endif

#ifndef __RSNCUL // only call in public api
#define __RSNCUL RSSpinLockUnlock(&__RSNotificationCenterlock)
#endif

static void __RSProcessInNotificationCenterDeallocate(RSNotificationCenterRef nc)
{
    ____RSNotificationCenterProcessInLockObservers(nc);
    if (nc->_observers) RSRelease(nc->_observers);
    nc->_observers = nil;
    ____RSNotificationCenterProcessInUnlockObservers(nc);
    
    if (nc->_queue) RSRelease(nc->_queue);
    nc->_queue = nil;
    if (nc->_sendSource) RSRunLoopRemoveSource(RSRunLoopGetCurrent(), nc->_sendSource, RSRunLoopDefault);
    nc->_sendSource = nil;
}

static void __RSNotificationCenterClassDeallocate(RSTypeRef rs)
{
    RSNotificationCenterRef nc = (RSNotificationCenterRef)rs;
    __RSNCL;
    __RSProcessInNotificationCenterDeallocate(nc);
    __RSNCUL;
}

static RSStringRef __RSNotificationCenterClassDescription(RSTypeRef rs)
{
    RSNotificationCenterRef nc = (RSNotificationCenterRef)rs;
    RSMutableStringRef description = RSStringCreateMutable(RSAllocatorSystemDefault, 128);   // UTF8 encoding.
    RSIndex cnt = RSDictionaryGetCount(nc->_observers);
    //RSStringAppendStringWithFormat(description, RSSTR("RSNotificationCenter - %R\n"), nc->_centerPort);
#if __LP64__
    RSStringAppendStringWithFormat(description, RSSTR("Observers count - %lld\n"), cnt);
#else
    RSStringAppendStringWithFormat(description, RSSTR("Observers count - %ld\n"), cnt);
#endif
    return description;
}

static BOOL __RSNotificationCenterClassEqual(RSTypeRef rs1, RSTypeRef rs2)
{
    RSNotificationCenterRef nc1 = (RSNotificationCenterRef)rs1;
    RSNotificationCenterRef nc2 = (RSNotificationCenterRef)rs2;
    if (nc1 == nc2) return YES;
    else HALTWithError(RSInvalidArgumentException, "There is no way to have two RSNotificationCenter in a single task.");
    // never run here.
    BOOL result = NO;
    do {
        ____RSNotificationCenterProcessInLockObservers(nc1);
        ____RSNotificationCenterProcessInLockObservers(nc2);
        if (nc1->_observers && nc2->_observers)
            if (NO == RSEqual(nc1->_observers, nc2->_observers))
                BWI(1);
        if (nc1->_queue && nc2->_queue)
            if (NO == RSEqual(nc1->_queue, nc2->_queue))
                BWI(1);
        result = YES;
    } while (0);
    ____RSNotificationCenterProcessInUnlockObservers(nc1);
    ____RSNotificationCenterProcessInUnlockObservers(nc2);
    return result;
}

static RSHashCode __RSNotificationCenterClassHash(RSTypeRef rs)
{
    RSNotificationCenterRef nc = (RSNotificationCenterRef)rs;
    return RSDictionaryGetCount(nc->_observers) ^ RSQueueGetCount(nc->_queue) ;//RSMessagePortGetMachPort(nc->_centerPort) ^;
}

static const RSRuntimeClass __RSNotificationCenterClass = {
    _RSRuntimeScannedObject,
    "RSNotificationCenter",
    __RSNotificationCenterClassInit,
    nil,
    __RSNotificationCenterClassDeallocate,
    __RSNotificationCenterClassEqual,
    __RSNotificationCenterClassHash,
    __RSNotificationCenterClassDescription,
    nil,
    nil
};

static RSTypeID _RSNotificationCenterTypeID = _RSRuntimeNotATypeID;

RSPrivate void __RSNotificationCenterInitialize()
{
    _RSNotificationCenterTypeID = __RSRuntimeRegisterClass(&__RSNotificationCenterClass);
    __RSRuntimeSetClassTypeID(&__RSNotificationCenterClass, _RSNotificationCenterTypeID);
    RSNotificationCenterGetDefault();
}

RSExport RSTypeID RSNotificationCenterGetTypeID()
{
    return _RSNotificationCenterTypeID;
}

static RSNotificationCenterRef __RSNotificationCenterGenericCreate(RSAllocatorRef allocator, BOOL special)
{
    RSNotificationCenterRef nc = (RSNotificationCenterRef)__RSRuntimeCreateInstance(RSAllocatorSystemDefault, _RSNotificationCenterTypeID, sizeof(struct __RSNotificationCenter) - sizeof(struct __RSRuntimeBase));
    //nc->_centerPort = nil;
    __RSRuntimeSetInstanceSpecial(nc, special);
    return nc;
}

RSExport RSNotificationCenterRef RSNotificationCenterGetDefault()
{
    __RSNCL;
    if (nil == __defaultNC)
    {
        __defaultNC = __RSNotificationCenterGenericCreate(RSAllocatorSystemDefault, YES);
    }
    __RSNCUL;
    return __defaultNC;
}

/*
 add the callback address in __userObserver to the __runtimeObserver which is included in NotificationCenter
 */
static void __RSNotificationCenterObserversAddObserver(RSObserverRef __runtimeObserver, RSObserverRef __userObserver)
{
//    uintptr_t address = (uintptr_t)RSObserverGetCallBack(__userObserver);
//    RSNumberRef cbdata = RSNumberCreate(RSGetAllocator(__runtimeObserver), RSNumberUnsignedLong, &address);
//    if (NO == RSSetContainsValue(__runtimeObserver->_callbacks, cbdata)) {
//        RSSetAddValue(__runtimeObserver->_callbacks, cbdata);
//    }
//    RSRelease(cbdata);
    
    if (NO == RSSetContainsValue(__runtimeObserver->_callbacks, __userObserver)) {
        RSSetAddValue(__runtimeObserver->_callbacks, __userObserver);
    }
}

static void __RSNotificationCenterAddObserver(RSNotificationCenterRef notificationCenter, RSObserverRef observer)
{
    RSObserverRef __runtimeObserver = __RSObserverCreate(RSAllocatorSystemDefault, nil, observer->_notificationName, nil, YES);
    RSDictionarySetValue(notificationCenter->_observers, RSObserverGetNotificationName(observer), __runtimeObserver);//retain
    __RSNotificationCenterObserversAddObserver(__runtimeObserver, observer);
    RSRelease(__runtimeObserver);
}

static void __RSNotificationCenterObserversRemoveObserver(RSObserverRef __runtimeObserver, RSObserverRef __userObserver)
{
//    uintptr_t address = (uintptr_t)RSObserverGetCallBack(__userObserver);
//    RSNumberRef cbdata = RSNumberCreate(RSGetAllocator(__runtimeObserver), RSNumberUnsignedLong, &address);
//    RSSetRemoveValue(__runtimeObserver->_callbacks, cbdata);
//    RSRelease(cbdata);
    RSSetRemoveValue(__runtimeObserver->_callbacks, __userObserver);
}

static void __RSNotificationCenterRemoveObserver(RSNotificationCenterRef notificationCenter, RSObserverRef observer)
{
    RSObserverRef __runtimeObserver = (RSObserverRef)RSDictionaryGetValue(__defaultNC->_observers, RSObserverGetNotificationName(observer));//not retain
    __RSNotificationCenterObserversRemoveObserver(__runtimeObserver, observer);
    if (RSSetGetCount(__runtimeObserver->_callbacks) == 0)
    {
        RSDictionaryRemoveValue(notificationCenter->_observers, RSObserverGetNotificationName(observer));
    }
    //RSRelease(__runtimeObserver);
}

RSExport void RSNotificationCenterAddObserver(RSNotificationCenterRef notificationCenter, RSObserverRef observer)
{
    if (notificationCenter != __defaultNC || __defaultNC == nil) HALTWithError(RSInvalidArgumentException, "The notification is not available.");
    __RSGenericValidInstance(observer, _RSObserverTypeID);
    if (__RSObserverIsRuntimeCreate(observer)) HALTWithError(RSInvalidArgumentException, "User can not use the runtime instance.");
    RSNotificationCenterRef nc = __defaultNC;
    ____RSNotificationCenterProcessInLockObservers(nc);
    //    if (unlikely(nc->_observers == nil))  // allocate observers when init notification center
    //        nc->_observers = RSDictionaryCreateMutable(RSAllocatorSystemDefault, 0, RSDictionaryRSTypeContext);
    RSStringRef key = RSObserverGetNotificationName(observer);
    RSObserverRef __runtimeObserver = (RSObserverRef)RSDictionaryGetValue(nc->_observers, key);
    if (likely(__runtimeObserver != nil))
    {
        __RSNotificationCenterObserversAddObserver(__runtimeObserver, observer);
        //RSRelease(__runtimeObserver);
        //__runtimeObserver->_callbacks
    }
    else
    {
        __RSNotificationCenterAddObserver(notificationCenter, observer);
    }
    ____RSNotificationCenterProcessInUnlockObservers(nc);
}

static RSObserverRef __RSNotificationCenterFindObserver(RSNotificationCenterRef notificationCenter, RSNotificationRef notification)
{
    ____RSNotificationCenterProcessInLockObservers(notificationCenter);
    RSObserverRef observer = (RSObserverRef)RSDictionaryGetValue(notificationCenter->_observers, RSNotificationGetName(notification));
    ____RSNotificationCenterProcessInUnlockObservers(notificationCenter);
    return (RSObserverRef)RSRetain(observer);
}

RSExport void RSNotificationCenterRemoveObserver(RSNotificationCenterRef notificationCenter, RSObserverRef observer)
{
    if (notificationCenter != __defaultNC || __defaultNC == nil) HALTWithError(RSInvalidArgumentException, "The notification is not available.");
    __RSGenericValidInstance(observer, _RSObserverTypeID);
    if (__RSObserverIsRuntimeCreate(observer)) HALTWithError(RSInvalidArgumentException, "User can not use the runtime instance.");
    ____RSNotificationCenterProcessInLockObservers(__defaultNC);
    RSStringRef key = RSObserverGetNotificationName(observer);
    RSObserverRef __runtimeObserver = (RSObserverRef)RSDictionaryGetValue(__defaultNC->_observers, key);
    if (likely(__runtimeObserver != nil))
    {
        __RSNotificationCenterObserversRemoveObserver(__runtimeObserver, observer);
        //RSRelease(__runtimeObserver);
        //__runtimeObserver->_callbacks
    }
    else
    {
        __RSNotificationCenterRemoveObserver(notificationCenter, observer);
    }
    ____RSNotificationCenterProcessInUnlockObservers(__defaultNC);
}

static void __RSNotificationCenterPostSchedule(void* info, RSRunLoopRef runloop, RSStringRef mode)
{
    if (NO == pthread_equal(pthread_self(), _RSMainPThread))
        pthread_setname_np("com.retval.RSNotificationCenter.private");
    
}

static void __RSNotificationCenterPostCancel(void* info, RSRunLoopRef runloop, RSStringRef mode)
{
    
}

static void __RSNotificationCenterActvieObserver(const void* value, void* context)
{
    // value is RSObserver (user)
    if (value && context)
    {
        if (((RSObserverRef)value)->_sender && ((RSNotificationRef)context)->_object != ((RSObserverRef)value)->_sender)
            return;
        RSObserverCallback cb = RSObserverGetCallBack((RSObserverRef)value);
        if (likely(cb)) cb((RSNotificationRef)context);
//        RSObserverCallback cb = RSObserverGetCallBack(observer);
//        RSNumberRef uaddress = (RSNumberRef)value;
//        uintptr_t address = 0;
//        RSNumberGetValue(uaddress, &address);
//        cb = (RSObserverCallback)address;
//        if (likely(cb)) cb((RSNotificationRef)context);
    }
}

static void __RSNotificationCenterPostPerform(void* info)
{
    RSNotificationRef notification = (RSNotificationRef)info;
    RSObserverRef __runtimeObserver = __RSNotificationCenterFindObserver(RSNotificationCenterGetDefault(), notification);
    if (nil == __runtimeObserver) HALTWithError(RSInvalidArgumentException, "There is no way to run here.");
    ____RSNotificationCenterProcessInLockObservers(__defaultNC);
    RSIndex cnt = RSSetGetCount(__runtimeObserver->_callbacks);
    if (cnt) {
        RSSetApplyFunction(__runtimeObserver->_callbacks, __RSNotificationCenterActvieObserver, notification);
    }
    ____RSNotificationCenterProcessInUnlockObservers(__defaultNC);
    RSRelease(__runtimeObserver);
}

static void __RSNotificationCenterPostImmediately(RSNotificationCenterRef notificationCenter, RSNotificationRef notification, BOOL immediately)
{
    if (notificationCenter != __defaultNC || __defaultNC == nil) HALTWithError(RSInvalidArgumentException, "The notification is not available.");
    __RSGenericValidInstance(notification, _RSNotificationTypeID);
    RSStringRef ntName = RSNotificationGetName(notification);
    ____RSNotificationCenterProcessInLockObservers(notificationCenter);
    RSObserverRef __runtimeObserver = (RSObserverRef)RSDictionaryGetValue(notificationCenter->_observers, ntName);
    if (__runtimeObserver)
    {
        
        // the notification has a watcher
        if (YES == immediately)
        {
            ____RSNotificationCenterProcessInUnlockObservers(notificationCenter);
            __RSNotificationCenterPostSchedule(nil, nil, nil);
            __RSNotificationCenterPostPerform(notification);    // here will try to get the lock for the observers
            __RSNotificationCenterPostCancel(nil, nil, nil);
            //RSRelease(__runtimeObserver);
            return;
        }
        else
        {
            RSStringRef mode = RSRunLoopQueue;
            RSRunLoopSourceContext context = {
                0,
                notification,
                RSRetain,
                RSRelease,
                RSDescription,
                RSEqual,
                RSHash,
                __RSNotificationCenterPostSchedule,
                __RSNotificationCenterPostCancel,
                __RSNotificationCenterPostPerform,
            };
            RSRunLoopSourceRef task = RSRunLoopSourceCreate(RSAllocatorSystemDefault, 0, &context);
			//            RSDictionaryRef reserved = nil;
			//            if ((reserved = (RSDictionaryRef)RSNotificationGetObject(notification)))
			//            {
			//                RSStringRef runMode = RSDictionaryGetValue(reserved, RSNotificationMode);
			//                if (runMode) {
			//                    mode = runMode;
			//                }
			//            }
            RSRunLoopAddSource(RSRunLoopGetCurrent(), task, mode);    // current loop.
            ____RSNotificationCenterProcessInUnlockObservers(notificationCenter);
            RSRunLoopRun();
            //RSRelease(__runtimeObserver);
            RSRelease(mode);
            RSRelease(task);
        }
        
    }
    else
    {
        ____RSNotificationCenterProcessInUnlockObservers(notificationCenter);
    }
}

RSExport void RSNotificationCenterPostNotification(RSNotificationCenterRef notificationCenter, RSNotificationRef notification)
{
    return __RSNotificationCenterPostImmediately(notificationCenter, notification, NO);
}

RSExport void RSNotificationCenterPostNotificationImmediately(RSNotificationCenterRef notificationCenter, RSNotificationRef notification)
{
    return __RSNotificationCenterPostImmediately(notificationCenter, notification, YES);
}

RSExport void RSNotificationCenterPost(RSNotificationCenterRef notificationCenter, RSStringRef name, RSTypeRef obj, RSDictionaryRef userInfo)
{
    RSNotificationRef nt = RSNotificationCreate(RSAllocatorSystemDefault, name, obj, userInfo);
    if (nt)
    {
        RSNotificationCenterPostNotification(notificationCenter, nt);
        RSRelease(nt);
    }
}

RSExport void RSNotificationCenterPostImmediately(RSNotificationCenterRef notificationCenter, RSStringRef name, RSTypeRef obj, RSDictionaryRef userInfo)
{
    RSNotificationRef nt = RSNotificationCreate(RSAllocatorSystemDefault, name, obj, userInfo);
    if (nt)
    {
        RSNotificationCenterPostNotificationImmediately(notificationCenter, nt);
        RSRelease(nt);
    }
}

#pragma mark -
#pragma mark RSDistributedNotificationCenter

#include "RSPrivate/RSDistributedModule/RSDistributedProtocol.h"
struct __RSDistributedNotificationCenter
{
    RSRuntimeBase _base;
    RSSpinLock _observerLock;
    RSSocketRef _client;
    RSMutableDictionaryRef _observers;  // <notification, observer>
    
    RSQueueRef _queueForNotification;
    RSRunLoopSourceRef _sendSource;
    
    RSStringRef _name;
};

//typedef struct __RSDistributedNotificationCenter __RSRemoteNotificationCenter;

RSInline void __RSDistributedNotificationCenterLockObservers(RSDistributedNotificationCenterRef nc) {RSSpinLockLock(&nc->_observerLock);}
RSInline void __RSDistributedNotificationCenterUnlockObservers(RSDistributedNotificationCenterRef nc) {RSSpinLockUnlock(&nc->_observerLock);}

static BOOL __RSDistributedNotificationCenterSendRequest(RSDistributedNotificationCenterRef distributedNotificationCenter, RSDistributedRequestRef request)
{
    if (distributedNotificationCenter == nil || request == nil) return NO;
    RSPropertyListRef plist = RSPropertyListCreateWithContent(RSGetAllocator(request), request);
    
    if (plist)
    {
        RSSocketError error = RSSocketSuccess;
        RSDataRef data = RSPropertyListGetData(plist, nil);
        if (data)  error = RSSocketSendData(distributedNotificationCenter->_client, nil, data, 0);
        RSRelease(plist);
        return error == RSSocketSuccess;
    }
    return NO;
}

static BOOL __RSDistributedNotificationCenterRecvRequest(RSDistributedNotificationCenterRef distributedNotificationCenter, RSDistributedRequestRef* request)
{
    if (distributedNotificationCenter == nil || request == nil) return NO;
    RSMutableDataRef data = nil;
    RSSocketError error = RSSocketRecvData(distributedNotificationCenter->_client, data);
    if (error != RSSocketSuccess)
    {
        RSRelease(data);
        return NO;
    }
    
    RSPropertyListRef plist = RSPropertyListCreateWithXMLData(RSGetAllocator(distributedNotificationCenter), data);
    RSRelease(data);
    *request = RSRetain(RSPropertyListGetContent(plist));
    RSRelease(plist);
    return *request == nil;
}

#include <sys/un.h>

#define __RSHLNCHostID      "/private/tmp/com.retval.RSCoreFoundation.DistributedNotificationCenter"

static RSDictionaryRef __RSNotificationToDictionary(RSNotificationRef notification)
{
    RSDictionaryRef dict = RSDictionaryCreateWithObjectsAndOKeys(RSGetAllocator(notification),
                                                                 RSNotificationGetName(notification), RSSTR("Name"),
                                                                 RSNotificationGetUserInfo(notification), RSSTR("userInfo"), nil);
    return dict;
}

static void __RSDistributedNotificationCenterCallBack(RSSocketRef s, RSSocketCallBackType type, RSDataRef address, const void *data, void *info)
{
    if (type & RSSocketConnectCallBack)
    {
        RSLog(RSSTR("__RSNotificationCenterClassLocalNCInit new client!"));
    }
    
    if (type & RSSocketDataCallBack)
    {
        RSLog(RSSTR("__RSNotificationCenterClassLocalNCInit connect!"));
        /*
        RSFileHandleRef handle = RSFileHandleCreateForWritingAtPath(RSFileManagerStandardizingPath(RSSTR("~/Desktop/RSDistributedNotification.connect.result.txt")));
        RSNotificationRef notification = RSNotificationCreate(RSAllocatorSystemDefault, RSSTR("RSRemoteNotification"), nil, nil);
        RSDictionaryRef dictForNotification = __RSNotificationToDictionary(notification);
        RSRelease(notification);
        
        RSDictionaryRef request = RSDictionaryCreateWithObjectsAndOKeys(RSAllocatorSystemDefault,
                                                                        RSSTR("RSDistributedAddClient"), RSDistributedRequest,
                                                                        RSSTR("__RSNotificationConnectSuccess all success."), RSDistributedRequestReason,
                                                                        RSSTR("__RSNotificationLocalNC"), RSDistributedRequestDomain,
                                                                        dictForNotification, RSDistributedRequestPayload,
                                                                        nil);
        RSRelease(dictForNotification);
        
        RSPropertyListRef plist = RSPropertyListCreateWithContent(RSAllocatorSystemDefault, request);
        RSRelease(request);
        RSDataRef propertyData =  RSPropertyListGetData(plist, nil);
        RSFileHandleWriteData(handle, propertyData);
        RSRelease(handle);
        RSSocketSendData(s, nil, propertyData, 0);
        RSRelease(plist);
        */
        RSNotificationRef notification = RSNotificationCreate(RSAllocatorSystemDefault, RSSTR("RSRemoteNotification"), nil, nil);
        RSDictionaryRef dictForNotification = __RSNotificationToDictionary(notification);
        RSRelease(notification);
        
        RSDictionaryRef request = RSDictionaryCreateWithObjectsAndOKeys(RSAllocatorSystemDefault,
                                                                        RSSTR("RSDistributedAddClient"), RSDistributedRequest,
                                                                        RSSTR("__RSNotificationConnectSuccess all success."), RSDistributedRequestReason,
                                                                        RSSTR("__RSNotificationLocalNC"), RSDistributedRequestDomain,
                                                                        dictForNotification, RSDistributedRequestPayload, nil);
        RSRelease(dictForNotification);
        RSDataRef data = RSPropertyListCreateXMLData(RSAllocatorSystemDefault, request);
        RSRelease(request);
        RSDataWriteToFile(data, RSFileManagerStandardizingPath(RSSTR("~/Desktop/RSDistributedNotification.connect.result.txt")), RSWriteFileAutomatically);
        RSSocketSendData(s, nil, data, 0);
        RSRelease(data);
    }
}

static void __RSDistributedNotificationCenterClassInit(RSTypeRef rs)
{
    RSDistributedNotificationCenterRef nc = (RSDistributedNotificationCenterRef)rs;
    nc->_observerLock = RSSpinLockInit;
    // init to connect to the remote local server.
    RSSocketContext context = {0, (RSMutableTypeRef)RSNil, RSRetain, RSRelease, RSDescription};
    nc->_client = RSSocketCreate(RSAllocatorSystemDefault, AF_UNIX, SOCK_STREAM, IPPROTO_IP, RSSocketConnectCallBack | RSSocketDataCallBack, __RSDistributedNotificationCenterCallBack, &context);
    
    RSSocketHandle fd = RSSocketGetHandle(nc->_client);
    if (fd < 0)
    {
        return;
    }
    struct sockaddr_un un = {0};
    un.sun_family = AF_UNIX;
    RSBlock buf[256] = {0};
    sprintf(buf, "/tmp/com.retval.RSCoreFoundation.local.client%05d",getpid());
    unlink(buf);
    nc->_name = RSStringCreateWithCString(RSAllocatorSystemDefault, buf, RSStringEncodingASCII);
    strcpy(un.sun_path, buf);
    socklen_t len = (socklen_t)(offsetof(struct sockaddr_un,sun_path) + strlen(un.sun_path));
    if (bind(fd, (struct sockaddr*)&un, len) < 0)
    {
		RSSocketInvalidate(nc->_client);
        RSRelease(nc->_client);
        nc->_client = nil;
        return;
    }
    memset(&un, 0, sizeof(un));
    un.sun_family = AF_UNIX;
    sprintf(un.sun_path, __RSHLNCHostID);
    len = offsetof(struct sockaddr_un, sun_path);
    len += strlen(un.sun_path);
    RSDataRef addressData = RSDataCreate(RSAllocatorSystemDefault, (const RSBitU8*)&un, len);
    RSSocketError socketError = RSSocketConnectToAddress(nc->_client, addressData, -1);
    RSRelease(addressData);
    
    if (socketError != RSSocketSuccess)
    {
        unlink(buf);
		RSSocketInvalidate(nc->_client);
        RSRelease(nc->_client);
        nc->_client = nil;
        return;
    }
	__RSDistributedNotificationCenterCallBack(nc->_client, RSSocketDataCallBack, nil, nil, nil);
}


static void __RSDistributedNotificationCenterClassDeallocate(RSTypeRef rs)
{
    RSDistributedNotificationCenterRef nc = (RSDistributedNotificationCenterRef)rs;
    if (nc->_client)
    {
        RSSocketInvalidate(nc->_client);
        RSRelease(nc->_client);
        nc->_client = nil;
    }
    
    if (nc->_name)
    {
        unlink(RSStringGetCStringPtr(nc->_name, RSStringEncodingUTF8));
        RSRelease(nc->_name);
        nc->_name = nil;
    }
    
    __RSDistributedNotificationCenterLockObservers(nc);
    if (nc->_observers) RSRelease(nc->_observers);
    nc->_observers = nil;
    __RSDistributedNotificationCenterUnlockObservers(nc);
    
    if (nc->_queueForNotification) RSRelease(nc->_queueForNotification);
    nc->_queueForNotification = nil;
    
    if (nc->_sendSource) RSRunLoopRemoveSource(RSRunLoopGetCurrent(), nc->_sendSource, RSRunLoopDefault);
    nc->_sendSource = nil;
}

static RSTypeRef __RSDistributedNotificationCenterClassCopy(RSAllocatorRef allocator, RSTypeRef rs, BOOL mutableCopy)
{
    return nil;
}

static BOOL __RSDistributedNotificationCenterClassEqual(RSTypeRef rs1, RSTypeRef rs2)
{
    return YES;
}

static RSHashCode __RSDistributedNotificationCenterClassHash(RSTypeRef rs)
{
    return RSHash(((RSDistributedNotificationCenterRef)rs)->_client);
}

static RSStringRef __RSDistributedNotificationCenterClassDescription(RSTypeRef rs)
{
    RSDistributedNotificationCenterRef dnc = (RSDistributedNotificationCenterRef)rs;
    RSMutableStringRef description = RSStringCreateMutable(RSAllocatorSystemDefault, 128);   // UTF8 encoding.
    RSIndex cnt = RSDictionaryGetCount(dnc->_observers);
    //RSStringAppendStringWithFormat(description, RSSTR("RSNotificationCenter - %R\n"), nc->_centerPort);
#if __LP64__
    RSStringAppendStringWithFormat(description, RSSTR("Observers count - %lld\n"), cnt);
#else
    RSStringAppendStringWithFormat(description, RSSTR("Observers count - %ld\n"), cnt);
#endif
    return description;
}

static RSRuntimeClass __RSDistributedNotificationCenterClass =
{
    _RSRuntimeScannedObject,
    "RSDistributedNotificationCenter",
    __RSDistributedNotificationCenterClassInit,
    __RSDistributedNotificationCenterClassCopy,
    __RSDistributedNotificationCenterClassDeallocate,
    __RSDistributedNotificationCenterClassEqual,
    __RSDistributedNotificationCenterClassHash,
    __RSDistributedNotificationCenterClassDescription,
    nil,
    nil
};

static RSTypeID _RSDistributedNotificationCenterTypeID = _RSRuntimeNotATypeID;
static RSSpinLock __RSDistributedNotificationCenterLock = RSSpinLockInit;
static RSDistributedNotificationCenterRef __defaultDNC = nil;

#ifndef __RSDNCL
#define __RSDNCL    RSSpinLockLock(&__RSDistributedNotificationCenterLock)
#endif

#ifndef __RSDNCUL
#define __RSDNCUL   RSSpinLockUnlock(&__RSDistributedNotificationCenterLock)
#endif

static RSDistributedNotificationCenterRef __RSDistributedNotificationCenterCreateInstance(RSAllocatorRef allocator)
{
    RSDistributedNotificationCenterRef dnc = (RSDistributedNotificationCenterRef)__RSRuntimeCreateInstance(allocator, RSDistributedNotificationCenterGetTypeID(), sizeof(struct __RSDistributedNotificationCenter) - sizeof(RSRuntimeBase));
    __RSRuntimeSetInstanceSpecial(dnc, YES);
    return dnc;
}

RSExport RSDistributedNotificationCenterRef RSDistributedNotificationCenterGetDefault()
{
    RSDistributedNotificationCenterRef dnc = nil;
    __RSDNCL;
    if (likely(__defaultDNC != nil))
    {
        dnc = __defaultDNC;
        __RSDNCUL;
    	return dnc;
    }
    __defaultDNC = __RSDistributedNotificationCenterCreateInstance(RSAllocatorSystemDefault);
    dnc = __defaultDNC;
    __RSDNCUL;
    return dnc;
}

RSPrivate void __RSDistributedNotificationCenterInitialize()
{
    _RSDistributedNotificationCenterTypeID = __RSRuntimeRegisterClass(&__RSDistributedNotificationCenterClass);
    __RSRuntimeSetClassTypeID(&__RSDistributedNotificationCenterClass, _RSDistributedNotificationCenterTypeID);
}

RSExport RSTypeID RSDistributedNotificationCenterGetTypeID()
{
    if (_RSDistributedNotificationCenterTypeID == _RSRuntimeNotATypeID)
        __RSDistributedNotificationCenterInitialize();
    return _RSDistributedNotificationCenterTypeID;
}

/*
 add the callback address in __userObserver to the __runtimeObserver which is included in NotificationCenter
 */
static void __RSDistributedNotificationCenterObserversAddObserver(RSObserverRef __runtimeObserver, RSObserverRef __userObserver)
{
    uintptr_t address = (uintptr_t)RSObserverGetCallBack(__userObserver);
    RSNumberRef cbdata = RSNumberCreate(RSGetAllocator(__runtimeObserver), RSNumberUnsignedLong, &address);
    if (NO == RSSetContainsValue(__runtimeObserver->_callbacks, cbdata)) {
        RSSetAddValue(__runtimeObserver->_callbacks, cbdata);
    }
    RSRelease(cbdata);
}

static void __RSDistributedNotificationCenterAddObserver(RSDistributedNotificationCenterRef notificationCenter, RSObserverRef observer)
{
    RSObserverRef __runtimeObserver = __RSObserverCreate(RSAllocatorSystemDefault, observer->_sender, observer->_notificationName, nil, YES);
    RSDictionarySetValue(notificationCenter->_observers, RSObserverGetNotificationName(observer), __runtimeObserver);//retain
    __RSDistributedNotificationCenterObserversAddObserver(__runtimeObserver, observer);
    RSRelease(__runtimeObserver);
}

static void __RSDistributedNotificationCenterObserversRemoveObserver(RSObserverRef __runtimeObserver, RSObserverRef __userObserver)
{
    uintptr_t address = (uintptr_t)RSObserverGetCallBack(__userObserver);
    RSNumberRef cbdata = RSNumberCreate(RSGetAllocator(__runtimeObserver), RSNumberUnsignedLong, &address);
    RSSetRemoveValue(__runtimeObserver->_callbacks, cbdata);
    RSRelease(cbdata);
}

static void __RSDistributedNotificationCenterRemoveObserver(RSDistributedNotificationCenterRef notificationCenter, RSObserverRef observer)
{
    RSObserverRef __runtimeObserver = (RSObserverRef)RSDictionaryGetValue(notificationCenter->_observers, RSObserverGetNotificationName(observer));//not retain
    __RSDistributedNotificationCenterObserversRemoveObserver(__runtimeObserver, observer);
    if (RSSetGetCount(__runtimeObserver->_callbacks) == 0)
    {
        RSDictionaryRemoveValue(notificationCenter->_observers, RSObserverGetNotificationName(observer));
    }
    //RSRelease(__runtimeObserver);
}

RSExport void RSDistributedNotificationCenterAddObserver(RSDistributedNotificationCenterRef notificationCenter, RSObserverRef observer)
{
    if (nil == notificationCenter) notificationCenter = __defaultDNC;
    else __RSGenericValidInstance(notificationCenter, _RSDistributedNotificationCenterTypeID);
    __RSGenericValidInstance(observer, _RSObserverTypeID);
    if (__RSObserverIsRuntimeCreate(observer)) HALTWithError(RSInvalidArgumentException, "User can not use the runtime instance.");
    RSDistributedNotificationCenterRef nc = notificationCenter;
    __RSDistributedNotificationCenterLockObservers(nc);
    //    if (unlikely(nc->_observers == nil))  // allocate observers when init notification center
    //        nc->_observers = RSDictionaryCreateMutable(RSAllocatorSystemDefault, 0, RSDictionaryRSTypeContext);
    RSStringRef key = RSObserverGetNotificationName(observer);
    RSObserverRef __runtimeObserver = (RSObserverRef)RSDictionaryGetValue(nc->_observers, key);
    if (likely(__runtimeObserver != nil))
    {
        __RSDistributedNotificationCenterObserversAddObserver(__runtimeObserver, observer);
        //RSRelease(__runtimeObserver);
        //__runtimeObserver->_callbacks
    }
    else
    {
        __RSDistributedNotificationCenterAddObserver(notificationCenter, observer);
    }
    __RSDistributedNotificationCenterUnlockObservers(nc);
}

static RSObserverRef __RSDistributedNotificationCenterFindObserver(RSDistributedNotificationCenterRef notificationCenter, RSNotificationRef notification)
{
    __RSDistributedNotificationCenterLockObservers(notificationCenter);
    RSObserverRef observer = (RSObserverRef)RSDictionaryGetValue(notificationCenter->_observers, RSNotificationGetName(notification));
    __RSDistributedNotificationCenterUnlockObservers(notificationCenter);
    return (RSObserverRef)RSRetain(observer);
}

RSExport void RSDistributedNotificationCenterRemoveObserver(RSDistributedNotificationCenterRef notificationCenter, RSObserverRef observer)
{
    if (nil == notificationCenter) notificationCenter = __defaultDNC;
    else __RSGenericValidInstance(notificationCenter, _RSDistributedNotificationCenterTypeID);
    __RSGenericValidInstance(observer, _RSObserverTypeID);
    if (__RSObserverIsRuntimeCreate(observer)) HALTWithError(RSInvalidArgumentException, "User can not use the runtime instance.");
    __RSDistributedNotificationCenterLockObservers(notificationCenter);
    RSStringRef key = RSObserverGetNotificationName(observer);
    RSObserverRef __runtimeObserver = (RSObserverRef)RSDictionaryGetValue(notificationCenter->_observers, key);
    if (likely(__runtimeObserver != nil))
    {
        __RSDistributedNotificationCenterObserversRemoveObserver(__runtimeObserver, observer);
        //RSRelease(__runtimeObserver);
        //__runtimeObserver->_callbacks
    }
    else
    {
        __RSDistributedNotificationCenterRemoveObserver(notificationCenter, observer);
    }
    __RSDistributedNotificationCenterUnlockObservers(notificationCenter);
}


RSPrivate void __RSNotificationCenterDeallocate()
{
    __RSNCL;
    if (__defaultNC == nil)
    {
        __RSNCUL;
        return;
    }
    __RSRuntimeSetInstanceSpecial(__defaultNC, NO);
    __RSNCUL;
    RSRelease(__defaultNC);
    __defaultNC = nil;
    
    __RSDNCL;
    if (__defaultDNC == nil)
    {
        __RSDNCUL;
        return;
    }
    __RSRuntimeSetInstanceSpecial(__defaultDNC, NO);
    RSRelease(__defaultDNC);
    __defaultDNC = nil;
    __RSDNCUL;
}

RS_PUBLIC_CONST_STRING_DECL(RSNotificationMode, "RSNotificationMode");

#undef __RSNCL
#undef __RSNCUL


#undef __RSDNCL
#undef __RSDNCUL