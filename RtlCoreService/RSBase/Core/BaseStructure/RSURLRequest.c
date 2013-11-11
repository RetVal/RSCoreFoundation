//
//  RSURLRequest.c
//  RSCoreFoundation
//
//  Created by Closure on 10/29/13.
//  Copyright (c) 2013 RetVal. All rights reserved.
//

#include "RSURLRequest.h"

#include <RSCoreFoundation/RSRuntime.h>
#include <RSCoreFoundation/RSURL.h>

//#include "RSPrivate/libcurl/include/curl/curl.h"

struct __RSURLRequest
{
    RSRuntimeBase _base;
    RSURLRef _url;
    RSDataRef _HTTPBody;
    RSMutableDictionaryRef _headerFields;
    RSUInteger _cacheMode;
    RSTimeInterval _timeout;
};

RSInline BOOL __RSURLRequestIsMutable(RSURLRequestRef urlRequest)
{
    return __RSBitfieldGetValue(RSRuntimeClassBaseFiled(urlRequest), 1, 1);
}

RSInline void __RSURLRequestSetIsMutable(RSURLRequestRef urlRequest, BOOL mutable)
{
    __RSBitfieldSetValue(RSRuntimeClassBaseFiled(urlRequest), 1, 1, mutable ? YES : NO);
}

static void __RSURLRequestClassInit(RSTypeRef rs)
{
    
}

static RSTypeRef __RSURLRequestClassCopy(RSAllocatorRef allocator, RSTypeRef rs, BOOL mutableCopy)
{
    return RSRetain(rs);
}

static void __RSURLRequestClassDeallocate(RSTypeRef rs)
{
    RSURLRequestRef urlRequest = (RSURLRequestRef)rs;
    if (urlRequest->_url) RSRelease(urlRequest->_url);
    if (urlRequest->_headerFields) RSRelease(urlRequest->_headerFields);
    if (urlRequest->_HTTPBody) RSRelease(urlRequest->_HTTPBody);
}

static BOOL __RSURLRequestClassEqual(RSTypeRef rs1, RSTypeRef rs2)
{
    RSURLRequestRef RSURLRequest1 = (RSURLRequestRef)rs1;
    RSURLRequestRef RSURLRequest2 = (RSURLRequestRef)rs2;
    BOOL result = NO;
    
    result = RSEqual(RSURLRequest1->_url, RSURLRequest2->_url);
    
    return result;
}

static RSHashCode __RSURLRequestClassHash(RSTypeRef rs)
{
    return RSHash(((RSURLRequestRef)rs)->_url);
}

static RSStringRef __RSURLRequestClassDescription(RSTypeRef rs)
{
    RSStringRef description = RSStringCreateWithFormat(RSAllocatorDefault, RSSTR("RSURLRequest %p"), rs);
    return description;
}

static RSRuntimeClass __RSURLRequestClass =
{
    _RSRuntimeScannedObject,
    "RSURLRequest",
    __RSURLRequestClassInit,
    __RSURLRequestClassCopy,
    __RSURLRequestClassDeallocate,
    __RSURLRequestClassEqual,
    __RSURLRequestClassHash,
    __RSURLRequestClassDescription,
    nil,
    nil
};

static RSTypeID _RSURLRequestTypeID = _RSRuntimeNotATypeID;

RSExport RSTypeID RSURLRequestGetTypeID()
{
    return _RSURLRequestTypeID;
}

RSPrivate void __RSURLRequestInitialize()
{
    _RSURLRequestTypeID = __RSRuntimeRegisterClass(&__RSURLRequestClass);
    __RSRuntimeSetClassTypeID(&__RSURLRequestClass, _RSURLRequestTypeID);
}

RSPrivate void __RSURLRequestDeallocate()
{
//    <#do your finalize operation#>
}


RSInline RSURLRequestRef __RSURLRequestSetHTTPBody(RSMutableURLRequestRef urlRequest, RSDataRef data)
{
    if (urlRequest) urlRequest->_HTTPBody = RSRetain(data);
    return urlRequest;
}

RSInline RSURLRef __RSURLRequestGetURL(RSURLRequestRef urlRequest)
{
    return urlRequest->_url;
}

RSInline RSURLRequestRef __RSURLRequestSetURL(RSMutableURLRequestRef urlRequest, RSURLRef URL)
{
    if (urlRequest && URL) urlRequest->_url = RSRetain(URL);
    return urlRequest;
}

RSInline RSMutableDictionaryRef __RSURLRequestGetHeaderField(RSURLRequestRef urlRequest)
{
    if (!urlRequest) return nil;
    return urlRequest->_headerFields;
}

RSInline RSURLRequestRef __RSURLRequestSetHeaderFields(RSMutableURLRequestRef urlRequest, RSDictionaryRef dict)
{
    if (urlRequest && dict) urlRequest->_headerFields = RSMutableCopy(RSAllocatorSystemDefault, dict);
    return urlRequest;
}

RSInline void __RSURLRequestSetValue(RSURLRequestRef urlRequest, RSStringRef key, RSTypeRef value)
{
    return RSDictionarySetValue(__RSURLRequestGetHeaderField(urlRequest), key, value);
}

RSInline RSTypeRef __RSURLRequestGetValue(RSURLRequestRef urlRequest, RSStringRef key)
{
    if (__RSURLRequestGetHeaderField(urlRequest)) return nil;
    return RSDictionaryGetValue(__RSURLRequestGetHeaderField(urlRequest), key);
}

RSInline RSTimeInterval __RSURLRequestGetTimeout(RSURLRequestRef urlRequest)
{
    if (urlRequest) return urlRequest->_timeout;
    return 0.0f;
}

RSInline RSURLRequestRef __RSURLRequestSetTimeout(RSMutableURLRequestRef urlRequest, RSTimeInterval timeout)
{
    if (urlRequest && timeout >= -0.000001f) urlRequest->_timeout = timeout;
    return urlRequest;
}

static RSMutableURLRequestRef __RSURLRequestCreateInstance(RSAllocatorRef allocator, RSURLRef url, RSDictionaryRef headerFields, RSTimeInterval timeout, BOOL mutable)
{
    RSMutableURLRequestRef instance = (RSMutableURLRequestRef)__RSRuntimeCreateInstance(allocator, _RSURLRequestTypeID, sizeof(struct __RSURLRequest) - sizeof(RSRuntimeBase));
    __RSURLRequestSetURL(instance, url);
    __RSURLRequestSetHeaderFields(instance, headerFields);
    __RSURLRequestSetTimeout(instance, timeout);
    __RSURLRequestSetIsMutable(instance, mutable);
    return instance;
}

RSExport RSURLRequestRef RSURLRequestCreateWithURL(RSAllocatorRef allocator, RSURLRef url)
{
    if (!url) return nil;
    __RSGenericValidInstance(url, RSURLGetTypeID());
    RSURLRequestRef instance = __RSURLRequestCreateInstance(allocator, url, nil, 0.0f, NO);
    return instance;
}

RSExport RSMutableURLRequestRef RSURLRequestCreateMutable(RSAllocatorRef allocator)
{
    return __RSURLRequestCreateInstance(allocator, nil, nil, 0.0f, YES);
}

RSExport RSDictionaryRef RSURLRequestGetAllHeaderFields(RSURLRequestRef urlRequest)
{
    __RSGenericValidInstance(urlRequest, _RSURLRequestTypeID);
    return __RSURLRequestGetHeaderField(urlRequest);
}

RSExport RSURLRef RSURLRequestGetURL(RSURLRequestRef urlRequest)
{
    __RSGenericValidInstance(urlRequest, _RSURLRequestTypeID);
    return __RSURLRequestGetURL(urlRequest);
}

RSExport void RSURLRequestSetURL(RSMutableURLRequestRef urlRequest, RSURLRef url)
{
    if (!urlRequest || !url) return;
    __RSGenericValidInstance(urlRequest, _RSURLRequestTypeID);
    if (!__RSURLRequestIsMutable(urlRequest)) return;
    __RSURLRequestSetURL(urlRequest, url);
}

RSExport void RSURLRequestSetHeaderFieldValue(RSMutableURLRequestRef urlRequest, RSStringRef key, RSTypeRef value)
{
    if (!urlRequest) return;
    if (!key || !value) return;
    __RSGenericValidInstance(urlRequest, _RSURLRequestTypeID);
    if (!__RSURLRequestIsMutable(urlRequest)) return;
    __RSURLRequestSetValue(urlRequest, key, value);
}

RSExport RSTypeRef RSURLRequestGetHeaderFieldValue(RSURLRequestRef urlRequest, RSStringRef key)
{
    if (!urlRequest || !key) return nil;
    __RSGenericValidInstance(urlRequest, _RSURLRequestTypeID);
    return __RSURLRequestGetValue(urlRequest, key);
}

RSExport RSDictionaryRef RSURLRequestGetHeaderField(RSURLRequestRef urlRequest)
{
    __RSGenericValidInstance(urlRequest, _RSURLRequestTypeID);
    return __RSURLRequestGetHeaderField(urlRequest);
}

RSExport RSTimeInterval RSURLRequestGetTimeoutInterval(RSURLRequestRef urlRequest)
{
    __RSGenericValidInstance(urlRequest, _RSURLRequestTypeID);
    return __RSURLRequestGetTimeout(urlRequest);
}

RSExport void RSURLRequestSetTimeoutInterval(RSMutableURLRequestRef urlRequest, RSTimeInterval timeoutInterval)
{
    __RSGenericValidInstance(urlRequest, _RSURLRequestTypeID);
    if (__RSURLRequestIsMutable(urlRequest)) return;
    __RSURLRequestSetTimeout(urlRequest, timeoutInterval);
}

RSInline RSDataRef __RSURLRequestGetHttpBody(RSURLRequestRef urlRequest)
{
    if (!urlRequest) return nil;
    return urlRequest->_HTTPBody;
}

RSExport RSDataRef RSURLRequestGetHttpBody(RSURLRequestRef urlRequest)
{
    __RSGenericValidInstance(urlRequest, _RSURLRequestTypeID);
    return __RSURLRequestGetHttpBody(urlRequest);
}

RSExport void RSURLRequestSetHTTPBody(RSMutableURLRequestRef urlRequest, RSDataRef HTTPBody)
{
    if (!urlRequest) return;
    __RSGenericValidInstance(urlRequest, _RSURLRequestTypeID);
    __RSURLRequestSetHTTPBody(urlRequest, HTTPBody);
}

RSExport RSURLRequestRef RSURLRequestWithURL(RSURLRef url)
{
    return RSAutorelease(RSURLRequestCreateWithURL(RSAllocatorSystemDefault, url));
}
