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
    RSStringRef _HTTPMethod;
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
    RSMutableURLRequestRef request = (RSMutableURLRequestRef)rs;
    request->_HTTPMethod = RSSTR("GET");
}

static RSTypeRef __RSURLRequestClassCopy(RSAllocatorRef allocator, RSTypeRef rs, BOOL mutableCopy)
{
    if (NO == __RSURLRequestIsMutable(rs) && NO == mutableCopy) return RSRetain(rs);
    RSURLRequestRef request = (RSURLRequestRef)rs;
    RSMutableURLRequestRef copy = RSURLRequestCreateMutable(RSAllocatorSystemDefault, nil);
    
    RSURLRequestSetTimeoutInterval(copy, RSURLRequestGetTimeoutInterval(request));
    
    if (RSURLRequestGetHTTPBody(request)) {
        RSDataRef bodyCopy = RSCopy(RSAllocatorSystemDefault, RSURLRequestGetHTTPBody(request));
        RSURLRequestSetHTTPBody(copy, bodyCopy);
        RSRelease(bodyCopy);
    }
    
    if (RSURLRequestGetHTTPMethod(request)) {
        RSStringRef stringCopy = RSCopy(RSAllocatorSystemDefault, RSURLRequestGetHTTPMethod(request));
        RSURLRequestSetHTTPMethod(copy, stringCopy);
        RSRelease(stringCopy);
    }
    
    if (RSURLRequestGetURL(request)) {
        RSURLRef url = RSCopy(RSAllocatorSystemDefault, RSURLRequestGetURL(request));
        RSURLRequestSetURL(copy, url);
        RSRelease(url);
    }
    
    if (RSURLRequestGetHeaderField(request)) {
        RSDictionaryApplyBlock(RSURLRequestGetHeaderField(request), ^(const void *key, const void *value, BOOL *stop) {
            RSURLRequestSetHeaderFieldValue(copy, key, value);
        });
    }
    __RSURLRequestSetIsMutable(copy, mutableCopy ? YES : NO);
    return copy;
}

static void __RSURLRequestClassDeallocate(RSTypeRef rs)
{
    RSURLRequestRef urlRequest = (RSURLRequestRef)rs;
    if (urlRequest->_url) RSRelease(urlRequest->_url);
    if (urlRequest->_headerFields) RSRelease(urlRequest->_headerFields);
    if (urlRequest->_HTTPBody) RSRelease(urlRequest->_HTTPBody);
    if (urlRequest->_HTTPMethod) RSRelease(urlRequest->_HTTPMethod);
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
    RSURLRequestRef request = (RSURLRequestRef)rs;
    RSStringRef description = RSStringCreateWithFormat(RSAllocatorDefault, RSSTR("<RSURLRequest %p> {URL: %R} - Method : %r\nheader field - %r\n body - %r"), request, request->_url, request->_HTTPMethod, request->_headerFields, request->_HTTPBody);
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
    if (urlRequest && URL) {
        if (urlRequest->_url) RSRelease(urlRequest->_url);
        urlRequest->_url = RSRetain(URL);
    }
    return urlRequest;
}

RSInline RSMutableDictionaryRef __RSURLRequestGetHeaderField(RSURLRequestRef urlRequest)
{
    if (!urlRequest) return nil;
    return urlRequest->_headerFields;
}

RSInline RSURLRequestRef __RSURLRequestSetHeaderFields(RSMutableURLRequestRef urlRequest, RSDictionaryRef dict)
{
    if (urlRequest && dict) {
        if (urlRequest->_headerFields) RSRelease(urlRequest->_headerFields);
        urlRequest->_headerFields = RSMutableCopy(RSAllocatorSystemDefault, dict);
    }
    return urlRequest;
}

RSInline void __RSURLRequestSetValue(RSMutableURLRequestRef urlRequest, RSStringRef key, RSTypeRef value)
{
    if (!urlRequest->_headerFields)
        urlRequest->_headerFields = RSDictionaryCreateMutable(RSAllocatorSystemDefault, 0, RSDictionaryRSTypeContext);
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

RSExport RSMutableURLRequestRef RSURLRequestCreateMutable(RSAllocatorRef allocator, RSURLRef aURL)
{
    return __RSURLRequestCreateInstance(allocator, aURL, nil, 0.0f, YES);
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
    RSAssert1(__RSURLRequestIsMutable(urlRequest), RSLogAlert, "%r is not mutable", urlRequest);
    __RSURLRequestSetURL(urlRequest, url);
}

RSExport void RSURLRequestSetHeaderFieldValue(RSMutableURLRequestRef urlRequest, RSStringRef key, RSTypeRef value)
{
    if (!urlRequest) return;
    if (!key || !value) return;
    __RSGenericValidInstance(urlRequest, _RSURLRequestTypeID);
    RSAssert1(__RSURLRequestIsMutable(urlRequest), RSLogAlert, "%r is not mutable", urlRequest);
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
    RSAssert1(__RSURLRequestIsMutable(urlRequest), RSLogAlert, "%r is not mutable", urlRequest);
    __RSURLRequestSetTimeout(urlRequest, timeoutInterval);
}

RSInline RSDataRef __RSURLRequestGetHTTPBody(RSURLRequestRef urlRequest)
{
    if (!urlRequest) return nil;
    return urlRequest->_HTTPBody;
}

RSExport RSDataRef RSURLRequestGetHTTPBody(RSURLRequestRef urlRequest)
{
    __RSGenericValidInstance(urlRequest, _RSURLRequestTypeID);
    return __RSURLRequestGetHTTPBody(urlRequest);
}

RSExport void RSURLRequestSetHTTPBody(RSMutableURLRequestRef urlRequest, RSDataRef HTTPBody)
{
    if (!urlRequest) return;
    __RSGenericValidInstance(urlRequest, _RSURLRequestTypeID);
    RSAssert1(__RSURLRequestIsMutable(urlRequest), RSLogAlert, "%r is not mutable", urlRequest);
    __RSURLRequestSetHTTPBody(urlRequest, HTTPBody);
}

RSExport RSStringRef RSURLRequestGetHTTPMethod(RSURLRequestRef urlRequest) {
    if (!urlRequest) return nil;
    __RSGenericValidInstance(urlRequest, _RSURLRequestTypeID);
    if (!urlRequest->_HTTPMethod)
        ((RSMutableURLRequestRef)urlRequest)->_HTTPMethod = RSSTR("GET");
    return urlRequest->_HTTPMethod;
}

RSExport void RSURLRequestSetHTTPMethod(RSMutableURLRequestRef urlRequest, RSStringRef method) {
    if (!urlRequest) return;
    __RSGenericValidInstance(urlRequest, _RSURLRequestTypeID);
    RSAssert1(__RSURLRequestIsMutable(urlRequest), RSLogAlert, "%r is not mutable", urlRequest);
    if (urlRequest->_HTTPMethod) RSRelease(urlRequest->_HTTPMethod);
    urlRequest->_HTTPMethod = RSRetain(method);
}

RSExport RSURLRequestRef RSURLRequestWithURL(RSURLRef url) {
    return RSAutorelease(RSURLRequestCreateWithURL(RSAllocatorSystemDefault, url));
}
