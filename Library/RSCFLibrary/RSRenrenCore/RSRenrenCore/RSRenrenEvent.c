//
//  RSRenrenEvent.c
//  RSRenrenCore
//
//  Created by closure on 1/3/14.
//  Copyright (c) 2014 closure. All rights reserved.
//

#include "RSRenrenEvent.h"

#include <RSCoreFoundation/RSRuntime.h>

struct __RSRenrenEvent
{
    RSRuntimeBase _base;
    RSStringRef _mode;
    RSMutableDictionaryRef _properties;
    RSMutableArrayRef _container;
};

static RSDictionaryRef initLikeEvent(RSStringRef content, RSStringRef mode) {
    const char *ptr = RSStringGetUTF8String(content);
    if (!content || *ptr != '(' || !RSStringHasSuffix(content, RSSTR(")"))) return nil;
    
    RSMutableDictionaryRef model = RSDictionaryCreateMutable(RSAllocatorDefault, 0, RSDictionaryRSTypeContext);
    RSDictionarySetValue(model, RSSTR("mode"), mode);
    RSMutableArrayRef container = RSArrayCreateMutable(RSAllocatorDefault, 5);
    ptr++;
    unsigned int count = 0;
    while (count < 5) {
        BOOL inside = NO;
        while (*ptr != '\'')
            ptr++;
        inside = YES;
        ptr++;
        char *end = (char *)ptr;
        while (*end != '\'')
            end++;
        RSStringRef subContent = RSStringCreateWithBytes(RSAllocatorDefault, ptr, end - ptr, RSStringEncodingUTF8, 0);
        RSArraySetObjectAtIndex(container, count, subContent);
        RSRelease(subContent);
        end++;
        ptr = end;
        count++;
    }
    RSDictionarySetValue(model, RSSTR("container"), container);
    RSRelease(container);
    return RSAutorelease(model);
}

static void likeEventAction(RSRenrenEventRef event) {
    RSStringRef urlFormat = RSAutorelease(RSURLCreateStringByAddingPercentEscapes(RSAllocatorDefault,
                                                                                  RSStringWithFormat(RSSTR("http://like.renren.com/%r?gid=%r_%r&uid=%r&owner=%r&type=%r&name=%r"),
                                                                                                     event->_mode,
                                                                                                     RSArrayObjectAtIndex(event->_container, 0),
                                                                                                     RSArrayObjectAtIndex(event->_container, 1),
                                                                                                     RSArrayObjectAtIndex(event->_container, 2),
                                                                                                     RSArrayObjectAtIndex(event->_container, 3),
                                                                                                     RSNumberWithInt(3),
                                                                                                     RSArrayObjectAtIndex(event->_container, 4)),
                                                                                  nil,
                                                                                  nil,
                                                                                  RSStringEncodingUTF8));
    RSURLResponseRef response = nil;
    RSDataRef data = nil;
    RSErrorRef error = nil;
    data = RSURLConnectionSendSynchronousRequest(RSURLRequestWithURL(RSURLWithString(urlFormat)), &response, &error);
    if (error) {
        RSShow(error);
        return;
    }
    if (!data) return;
    RSStringRef str = RSStringCreateWithData(RSAllocatorDefault, data, RSStringEncodingUTF8);
    if (str) RSLog(RSSTR("json result string = %r"), str);
    RSRelease(str);
    if (RSURLResponseGetStatusCode(response) == 200)
        RSShow(RSSTR("success"));
}

static RSTypeRef __RSRenrenEventClassCopy(RSAllocatorRef allocator, RSTypeRef rs, BOOL mutableCopy)
{
    return RSRetain(rs);
}

static void __RSRenrenEventClassDeallocate(RSTypeRef rs)
{
    RSRenrenEventRef event = (RSRenrenEventRef)rs;
    RSRelease(event->_mode);
    RSRelease(event->_properties);
    RSRelease(event->_container);
}

static BOOL __RSRenrenEventClassEqual(RSTypeRef rs1, RSTypeRef rs2)
{
    RSRenrenEventRef RSRenrenEvent1 = (RSRenrenEventRef)rs1;
    RSRenrenEventRef RSRenrenEvent2 = (RSRenrenEventRef)rs2;
    BOOL result = NO;
    
    result = RSEqual(RSRenrenEvent1->_mode, RSRenrenEvent2->_mode) &&
             RSEqual(RSRenrenEvent1->_properties, RSRenrenEvent2->_properties) &&
             RSEqual(RSRenrenEvent1->_container, RSRenrenEvent2->_container);
    
    return result;
}

static RSHashCode __RSRenrenEventClassHash(RSTypeRef rs)
{
    return RSHash(((RSRenrenEventRef)rs)->_properties);
}

static RSStringRef __RSRenrenEventClassDescription(RSTypeRef rs)
{
    RSRenrenEventRef event = (RSRenrenEventRef)rs;
    RSStringRef description = RSStringCreateWithFormat(RSAllocatorDefault, RSSTR("RSRenrenEvent <%p> %r"), rs, event->_container);
    return description;
}

static RSRuntimeClass __RSRenrenEventClass =
{
    _RSRuntimeScannedObject,
    "RSRenrenEvent",
    nil,
    __RSRenrenEventClassCopy,
    __RSRenrenEventClassDeallocate,
    __RSRenrenEventClassEqual,
    __RSRenrenEventClassHash,
    __RSRenrenEventClassDescription,
    nil,
    nil
};

static RSSpinLock _RSRenrenEventSpinLock = RSSpinLockInit;
static RSTypeID _RSRenrenEventTypeID = _RSRuntimeNotATypeID;

static void __RSRenrenEventInitialize()
{
    _RSRenrenEventTypeID = __RSRuntimeRegisterClass(&__RSRenrenEventClass);
    __RSRuntimeSetClassTypeID(&__RSRenrenEventClass, _RSRenrenEventTypeID);
}

RSExport RSTypeID RSRenrenEventGetTypeID()
{
    RSSyncUpdateBlock(&_RSRenrenEventSpinLock, ^{
        if (_RSRuntimeNotATypeID == _RSRenrenEventTypeID)
            __RSRenrenEventInitialize();
    });
    return _RSRenrenEventTypeID;
}

static RSRenrenEventRef __RSRenrenEventCreateInstance(RSAllocatorRef allocator, RSStringRef mode, RSTypeRef content)
{
    struct __RSRenrenEvent *instance = (struct __RSRenrenEvent *)__RSRuntimeCreateInstance(allocator, RSRenrenEventGetTypeID(), sizeof(struct __RSRenrenEvent) - sizeof(RSRuntimeBase));
    instance->_mode = RSRetain(mode);
    if (RSGetTypeID(content) == RSStringGetTypeID() && (RSEqual(instance->_mode, RSSTR("addlike")) || RSEqual(instance->_mode, RSSTR("removelike")))) {
        RSDictionaryRef properties = initLikeEvent(content, mode);
        instance->_properties = nil;
        instance->_container = (RSMutableArrayRef)RSRetain(RSDictionaryGetValue(properties, RSSTR("container")));
    }
    return instance;
}

RSExport RSRenrenEventRef RSRenrenEventCreateLikeWithContent(RSAllocatorRef allocator, RSStringRef content, BOOL addlike) {
    if (!content) return nil;
    return __RSRenrenEventCreateInstance(allocator, addlike ? RSSTR("addlike") : RSSTR("removelike"), content);
}

RSExport void RSRenrenEventDo(RSRenrenEventRef event) {
    if (!event) return;
    __RSGenericValidInstance(event, _RSRenrenEventTypeID);
    if (RSEqual(event->_mode, RSSTR("addlike")) || RSEqual(event->_mode, RSSTR("removelike"))) {
        likeEventAction(event);
    }
}
