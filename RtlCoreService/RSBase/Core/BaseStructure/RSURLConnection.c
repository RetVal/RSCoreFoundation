//
//  RSURLConnection.c
//  RSCoreFoundation
//
//  Created by Closure on 11/2/13.
//  Copyright (c) 2013 RetVal. All rights reserved.
//

#include "RSURLConnection.h"
#include <assert.h>
#include <dispatch/dispatch.h>
#include <Block.h>

#include "RSInternal.h"
#include "RSThread.h"

#include <RSCoreFoundation/RSRuntime.h>
#include <RSCoreFoundation/RSURL.h>
#include <RSCoreFoundation/RSURLRequest.h>
#include <RSCoreFoundation/RSRunLoop.h>

#include "RSPrivate/libcurl/include/curl/curl.h"

//RS_CONST_STRING_DECL(__RSURLConnection
RS_CONST_STRING_DECL(__RSHTTPMessageUserAgentHeader, "User-Agent");
RS_CONST_STRING_DECL(__RSHTTPMessageAcceptRangesHeader, "Accept-Ranges")
RS_CONST_STRING_DECL(__RSHTTPMessageCacheControlHeader, "Cache-Control")
RS_CONST_STRING_DECL(__RSHTTPMessageConnectHeader, "Connection")
RS_CONST_STRING_DECL(__RSHTTPMessageContentLanguageHeader, "Content-Language")
RS_CONST_STRING_DECL(__RSHTTPMessageContentLengthHeader, "Content-Length")
RS_CONST_STRING_DECL(__RSHTTPMessageContentLocationHeader, "Content-Location")
RS_CONST_STRING_DECL(__RSHTTPMessageContentTypeHeader, "Content-Type")
RS_CONST_STRING_DECL(__RSHTTPMessageDateHeader, "Date")
RS_CONST_STRING_DECL(__RSHTTPMessageEtagHeader, "Etag")
RS_CONST_STRING_DECL(__RSHTTPMessageExpiresHeader, "Expires")
RS_CONST_STRING_DECL(__RSHTTPMessageLastModifiedHeader, "Last-Modified")
RS_CONST_STRING_DECL(__RSHTTPMessageLocationHeader, "Location")
RS_CONST_STRING_DECL(__RSHTTPMessageProxyAuthenticateHeader, "Proxy-Authenticate")
RS_CONST_STRING_DECL(__RSHTTPMessageServerHeader, "Server")
RS_CONST_STRING_DECL(__RSHTTPMessageSetCookieHeader, "Set-Cookie")

RS_CONST_STRING_DECL(__RSURLConnectionErrorDomain, "RSURLConnectionErrorDomain");

#ifndef __RSURLConnectionInvoke
#define __RSURLConnectionInvoke(method, ...) do { if ((method) != nil) (method)(__VA_ARGS__);}while(0);

#define __RSURLConnectionInvokeStartConnection(connection, ...)   __RSURLConnectionInvoke(((connection)->_impDelegate).startConnection, connection, __VA_ARGS__)
#define __RSURLConnectionInvokeReceiveResponse(connection, ...)   __RSURLConnectionInvoke(((connection)->_impDelegate).receiveResponse, connection, __VA_ARGS__)
#define __RSURLConnectionInvokeReceiveData(connection, ...)       __RSURLConnectionInvoke(((connection)->_impDelegate).receiveData, connection, __VA_ARGS__)
#define __RSURLConnectionInvokeSendData(connection, ...)          __RSURLConnectionInvoke(((connection)->_impDelegate).sendData, connection, __VA_ARGS__)
#define __RSURLConnectionInvokeFinishLoading(connection, ...)     __RSURLConnectionInvoke(((connection)->_impDelegate).finishLoading, connection)
#define __RSURLConnectionInvokeFailWithError(connection, ...)     __RSURLConnectionInvoke(((connection)->_impDelegate).failWithError, connection, __VA_ARGS__)
#endif

#ifndef __RSReleaseSafe
#define __RSReleaseSafe(rs) do { if ((rs)) RSRelease((rs)); (rs) = nil;} while(0);
#endif

struct _core
{
    void *_core;
    void *_core_chunk;
};

static void __RSURLConnectionWillStartConnection(RSURLConnectionRef connection, RSURLRequestRef willSendRequest, RSURLResponseRef redirectResponse);
static void __RSURLConnectionDidReceiveResponse(RSURLConnectionRef connection, RSURLResponseRef response);
static void __RSURLConnectionDidReceiveData(RSURLConnectionRef connection, RSDataRef data);
static void __RSURLConnectionDidSendBodyData(RSURLConnectionRef connection, RSDataRef data, RSUInteger totalBytesWritten, RSUInteger totalBytesExpectedToWrite);
static void __RSURLConnectionDidFinishLoading(RSURLConnectionRef connection);
static void __RSURLConnectionDidFailWithError(RSURLConnectionRef connection, RSErrorRef error);

struct __RSURLConnection
{
    RSRuntimeBase _base;
    
    RSURLRequestRef _orignialRequest;
    RSMutableURLRequestRef _request;
    
    RSMutableDataRef _responseHeader;
    
    RSTypeRef _delegate;    // weak
    struct RSURLConnectionDelegate _impDelegate;
    struct _core _core;
    
    RSURLResponseRef _response;
//    RSMutableDataRef _data;
    RSErrorRef _error;
};

RSInline BOOL __RSURLConnectionIsAsync(RSURLConnectionRef connection)
{
    return __RSBitfieldGetValue(RSRuntimeClassBaseFiled(connection), 1, 1);
}

RSInline void __RSURLConnectionSetIsAsync(RSURLConnectionRef connection, BOOL async)
{
    __RSBitfieldSetValue(RSRuntimeClassBaseFiled(connection), 1, 1, async ? YES : NO);
}

RSInline void *__RSURLConnectionGetCore(RSURLConnectionRef connection)
{
    assert(connection);
    return (&connection->_core);
}

RSInline void *__RSURLConnectionCoreGet(RSURLConnectionRef connection)
{
    assert(connection);
    return (&connection->_core)->_core;
}

RSInline void __RSURLConnectionCoreAlloc(RSURLConnectionRef connection, BOOL async)
{
    struct _core *_core = __RSURLConnectionGetCore(connection);
//    if (!async) _core->_core = curl_easy_init();
//    else _core->_core = curl_multi_init();
    _core->_core = curl_easy_init();
}

RSInline void __RSURLConnectionCoreRelease(void *core, BOOL async)
{
    if (core)
    {
//        curl_easy_reset(core);
        curl_easy_cleanup(core);
//        if (!async)
//        {
//        	curl_easy_reset(core);
//            curl_easy_cleanup(core);
//        }
//        else
//        {
//            curl_multi_cleanup(core);
//        }
    }
}

RSInline void __RSURLConnectionCoreDeallocate(void *core, BOOL async)
{
    __RSURLConnectionCoreRelease(((struct _core *)core)->_core, async);
    if (((struct _core *)core)->_core_chunk) curl_slist_free_all(((struct _core *)core)->_core_chunk);
}

RSInline void __RSURLConnectionCoreSetupURL(void *core, const char *url)
{
    assert(core && url);
    curl_easy_setopt(core, CURLOPT_URL, url);
}

RSInline void *__RSURLConnectionCoreGetChunk(RSURLConnectionRef connection)
{
    assert(connection);
    
    return (&connection->_core)->_core_chunk;
}

RSInline void *__RSURLConnectionCoreSetupChunk(RSURLConnectionRef connection, void *chunk)
{
    void *org = __RSURLConnectionCoreGetChunk(connection);
    (&connection->_core)->_core_chunk = chunk;
    return org;
}

RSInline void __RSURLConnectionCoreSetupHTTPHeaderFieldIfNecessary(RSURLConnectionRef connection, RSDictionaryRef headerField)
{
    if (!headerField) return;
    assert(connection);
    void *core = __RSURLConnectionCoreGet(connection);
    RSUInteger cnt = RSDictionaryGetCount(headerField);
    if (0 == cnt) return;
    
    RSAutoreleaseBlock(^{
        struct curl_slist *chunk = __RSURLConnectionCoreGetChunk(connection);
        
        RSArrayRef keys = RSDictionaryCopyAllKeys(headerField);
        RSArrayRef values = RSDictionaryCopyAllValues(headerField);
        RSStringRef format = RSSTR("%r: %r");
        for (RSUInteger idx = 0; idx < cnt; idx++) {
            RSStringRef mutableString = RSStringCreateWithFormat(RSAllocatorDefault, format, RSArrayObjectAtIndex(keys, idx), RSArrayObjectAtIndex(values, idx));
            const char *item = RSStringCopyUTF8String(mutableString);
            __RSLog(RSLogLevelNotice, RSSTR("%s"), item);
            chunk = curl_slist_append(chunk, item);
            RSAllocatorDeallocate(RSAllocatorSystemDefault, item);
            RSRelease(mutableString);
        }
        RSRelease(keys);
        RSRelease(values);

        curl_easy_setopt(core, CURLOPT_HTTPHEADER, chunk);
        void *org = __RSURLConnectionCoreSetupChunk(connection, chunk);
        if (org != chunk) curl_slist_free_all(org);
    });
}

static void __RSURLConnectionCoreSetupPostBehaviorIfNecessary(RSURLConnectionRef connection, RSURLRequestRef request) {
    if (!connection || !request || NO == RSEqual(RSSTR("POST"), RSURLRequestGetHTTPMethod(request))) return;
    void *core = __RSURLConnectionCoreGet(connection);
    curl_easy_setopt(core, CURLOPT_POST, 1);
    curl_easy_setopt(core, CURLOPT_POSTFIELDS, RSStringGetUTF8String(RSStringWithData(RSURLRequestGetHTTPBody(request), RSStringEncodingUTF8)));
    
    RSStringRef sizeStr = RSDictionaryGetValue(RSURLRequestGetHeaderField(request), __RSHTTPMessageContentLengthHeader);
    if (sizeStr) {
        RSDictionaryRemoveValue((RSMutableDictionaryRef)RSURLRequestGetHeaderField(request), __RSHTTPMessageContentLengthHeader);
        size_t size = RSStringLongValue(sizeStr);
        curl_easy_setopt(core, CURLOPT_POSTFIELDSIZE, size);
    }
}

static void __RSURLConnectionCoreUpdateCurrentURLRequest(RSURLConnectionRef connection) {
    unsigned long statusCode = 0;
    const char *effective_url = nil;
    curl_easy_getinfo(__RSURLConnectionCoreGet(connection), CURLINFO_RESPONSE_CODE, &statusCode);
    curl_easy_getinfo(__RSURLConnectionCoreGet(connection), CURLINFO_EFFECTIVE_URL, &effective_url);
    
    RSStringRef URLString = RSStringCreateWithCString(RSAllocatorSystemDefault, effective_url, RSStringEncodingUTF8);
    RSURLRef URL = RSURLCreateWithString(RSAllocatorSystemDefault, URLString, nil);
    RSRelease(URLString);
    
    RSMutableURLRequestRef request = connection->_request;
    RSURLRequestSetURL(request, URL);
    RSRelease(URL);
}

static size_t __RSURLConnectionCoreAppendReceiveResponseHeader(void *ptr, size_t size, size_t nmemb, RSURLConnectionRef connection)
{
    if (!connection) return size * nmemb;
    size_t realSize = size * nmemb;
    
    RSDataRef data = RSDataCreate(RSAllocatorSystemDefault, ptr, realSize);
    RSDataAppend(connection->_responseHeader, data);
    if (RSDataGetLength(data) == 2) {
        if (!connection->_response) {
            if (connection->_responseHeader && RSDataGetLength(connection->_responseHeader)) {
                __RSURLConnectionCoreUpdateCurrentURLRequest(connection);
                connection->_response = RSURLResponseCreateWithData(RSAllocatorSystemDefault, RSURLRequestGetURL(RSURLConnectionGetCurrentRequest(connection)), connection->_responseHeader);
            }
        }
        __RSURLConnectionDidReceiveResponse(connection, connection->_response);
        RSUInteger errorCode = RSURLResponseGetStatusCode(connection->_response);
        if (errorCode / 100 == 3 && RSDictionaryGetValue(RSURLResponseGetAllHeaderFields(connection->_response), RSSTR("Location"))) {
            RSRelease(connection->_responseHeader);
            connection->_responseHeader = RSDataCreateMutable(RSAllocatorSystemDefault, 0);
            RSRelease(connection->_response);
            connection->_response = nil;
        }
    }
    RSRelease(data);
    return realSize;
}

static size_t __RSURLConnectionCoreAppendReceiveData(void *ptr, size_t size, size_t nmemb, RSURLConnectionRef connection)
{
    if (!connection) return size * nmemb;
    size_t realSize = size * nmemb;
    
    RSDataRef data = RSDataCreate(RSAllocatorSystemDefault, ptr, realSize);
//    RSLog(RSSTR("%s : %r"), __func__, RSStringWithData(data, RSStringEncodingUTF8));
    __RSURLConnectionDidReceiveData(connection, data);
    RSRelease(data);
    
    return realSize;
}

RSInline void __RSURLConnectionCoreSetupHTTPDataReceiver(RSURLConnectionRef connection) {
    void *core = __RSURLConnectionCoreGet(connection);
    curl_easy_setopt(core, CURLOPT_WRITEDATA, connection);
    curl_easy_setopt(core, CURLOPT_WRITEFUNCTION, __RSURLConnectionCoreAppendReceiveData);
    curl_easy_setopt(core, CURLOPT_HEADERDATA, connection);
    curl_easy_setopt(core, CURLOPT_HEADERFUNCTION,__RSURLConnectionCoreAppendReceiveResponseHeader);
    curl_easy_setopt(core, CURLOPT_AUTOREFERER, 1);
    curl_easy_setopt(core, CURLOPT_FOLLOWLOCATION, 1);
    curl_easy_setopt(core, CURLOPT_COOKIEFILE, "");
}

static const char* __RSURLConnectionDefaultUserAgent = "Mozilla/5.0 (Macintosh; Intel Mac OS X 10_9) AppleWebKit/537.71 (KHTML, like Gecko) Version/7.0 Safari/537.71";
//User-Agent	Mozilla/5.0 (Macintosh; Intel Mac OS X 10_9) AppleWebKit/537.71 (KHTML, like Gecko) Version/7.0 Safari/537.71

static void __RSURLConnectionCoreSetupDefault(RSURLConnectionRef connection, const char *userAgent)
{
    void *core = __RSURLConnectionCoreGet(connection);
    assert(core);
    curl_easy_setopt(core, CURLOPT_HEADER, 0);
    curl_easy_setopt(core, CURLOPT_USERAGENT, userAgent ? : __RSURLConnectionDefaultUserAgent);
    curl_easy_setopt(core, CURLOPT_ACCEPT_ENCODING, "text/html,application/xhtml+xml,application/xml;q=0.9,*/*;q=0.8");
    
    RSMutableDictionaryRef header = RSDictionaryCreateMutable(RSAllocatorSystemDefault, 0, RSDictionaryRSTypeContext);
    RSDictionarySetValue(header, RSSTR("DNT"), RSNumberWithInt(1));
    RSDictionarySetValue(header, RSSTR("Cache-Control"), RSSTR("max-age=0"));
    
    __RSURLConnectionCoreSetupHTTPHeaderFieldIfNecessary(connection, header);
    RSRelease(header);
}

RSInline void __RSURLConnectionCostCleanup(RSURLConnectionRef connection) {
    if (connection->_responseHeader) RSDataSetLength(connection->_responseHeader, 0);
//    if (connection->_data) RSDataSetLength(connection->_data, 0);
    
    __RSReleaseSafe(connection->_response);
    __RSReleaseSafe(connection->_error);
    if (__RSURLConnectionGetCore(connection)) __RSURLConnectionCoreDeallocate(__RSURLConnectionGetCore(connection), __RSURLConnectionIsAsync(connection));
}

static RSURLConnectionRef __RSURLConnectionSetupWithURLRequest(RSURLConnectionRef connection, RSURLRequestRef request, BOOL async)
{
    __RSURLConnectionCostCleanup(connection);
    if (!__RSURLConnectionCoreGet(connection)) __RSURLConnectionCoreAlloc(connection, async);
    const char *c_url = RSStringCopyUTF8String(RSURLGetString(RSURLRequestGetURL(request)));
    if (c_url) {
        __RSURLConnectionCoreSetupURL(__RSURLConnectionCoreGet(connection), c_url);
        RSAllocatorDeallocate(RSAllocatorSystemDefault, c_url);
    }
    __RSURLConnectionCoreSetupDefault(connection, nil);
    __RSURLConnectionCoreSetupPostBehaviorIfNecessary(connection, request);
    __RSURLConnectionCoreSetupHTTPHeaderFieldIfNecessary(connection, RSURLRequestGetHeaderField(request));
    
    return connection;
}

static void __RSURLConnectionFinishPerform(RSURLConnectionRef connection) {
    if (!connection->_response) {
        if (connection->_responseHeader && RSDataGetLength(connection->_responseHeader)) {
            connection->_response = RSURLResponseCreateWithData(RSAllocatorSystemDefault, RSURLRequestGetURL(RSURLConnectionGetCurrentRequest(connection)), connection->_responseHeader);
        }
    }
    
}

static RSURLConnectionRef __RSURLConnectionCorePerform(RSURLConnectionRef connection)
{
    void *core = __RSURLConnectionCoreGet(connection);
    CURLcode retCode = curl_easy_perform(core);
    __RSURLConnectionFinishPerform(connection);
//    curl_easy_cleanup(core);
//    struct _core *_core = __RSURLConnectionGetCore(connection);
//    _core->_core = nil;
    
    if (retCode) {
        __RSCLog(RSLogLevelNotice, "RSURLConnection Notice ");
        __RSURLConnectionDidFailWithError(connection, RSErrorWithDomainCodeAndUserInfo(__RSURLConnectionErrorDomain, retCode, nil));
    }
    else {
        __RSURLConnectionDidFinishLoading(connection);
    }
    return connection;
}

static void __RSURLConnectionClassInit(RSTypeRef rs)
{
    RSURLConnectionRef connection = (RSURLConnectionRef)rs;
    connection->_responseHeader = RSDataCreateMutable(RSAllocatorSystemDefault, 0);
}

static RSTypeRef __RSURLConnectionClassCopy(RSAllocatorRef allocator, RSTypeRef rs, BOOL mutableCopy)
{
    return RSRetain(rs);
}

static void __RSURLConnectionClassDeallocate(RSTypeRef rs)
{
    RSURLConnectionRef connection = (RSURLConnectionRef)rs;
    if (connection->_orignialRequest) RSRelease(connection->_orignialRequest);
    connection->_orignialRequest = nil;
    if (connection->_request) RSRelease(connection->_request);
    connection->_request = nil;
//    if (connection->_data) RSRelease(connection->_data);
//    connection->_data = nil;
    if (connection->_responseHeader) RSRelease(connection->_responseHeader);
    connection->_responseHeader = nil;
    __RSURLConnectionCostCleanup(connection);
}

static BOOL __RSURLConnectionClassEqual(RSTypeRef rs1, RSTypeRef rs2)
{
    RSURLConnectionRef RSURLConnection1 = (RSURLConnectionRef)rs1;
    RSURLConnectionRef RSURLConnection2 = (RSURLConnectionRef)rs2;
    BOOL result = NO;
    
    result = RSEqual(RSURLConnection1->_orignialRequest, RSURLConnection2->_orignialRequest);
    
    return result;
}

static RSHashCode __RSURLConnectionClassHash(RSTypeRef rs)
{
    return RSHash(((RSURLConnectionRef)rs)->_orignialRequest);
}

static RSStringRef __RSURLConnectionClassDescription(RSTypeRef rs)
{
    RSStringRef description = RSStringCreateWithFormat(RSAllocatorDefault, RSSTR("RSURLConnection %p"), rs);
    return description;
}

static RSRuntimeClass __RSURLConnectionClass =
{
    _RSRuntimeScannedObject,
    "RSURLConnection",
    __RSURLConnectionClassInit,
    __RSURLConnectionClassCopy,
    __RSURLConnectionClassDeallocate,
    __RSURLConnectionClassEqual,
    __RSURLConnectionClassHash,
    __RSURLConnectionClassDescription,
    nil,
    nil
};

static RSTypeID _RSURLConnectionTypeID = _RSRuntimeNotATypeID;

RSExport RSTypeID RSURLConnectionGetTypeID()
{
    return _RSURLConnectionTypeID;
}

static RSRunLoopRef __RSURLConnectionRunLoop = nil;
static RSSpinLock __RSURLConnectionRunLoopSpinLock = RSSpinLockInit;
//static dispatch_queue_t __RSURLConnectionDispatchQueue = nil;

static void _emptyPerform(void *info) {
    ;
}

static void __RSURLConnectionLoaderMain(void *context) {
    pthread_setname_np("com.retval.RSURLConnectionLoaderMain");
    __RSURLConnectionRunLoop = RSRunLoopGetCurrent();
    RSRunLoopPerformBlock(RSRunLoopGetCurrent(), RSRunLoopDefaultMode, ^{
        // do nothing;
    });
    RSRunLoopSourceContext ctx;
    ctx.version = 0;
    ctx.info = nil;
    ctx.retain = nil;
    ctx.release = nil;
    ctx.description = nil;
    ctx.equal = nil;
    ctx.schedule = nil;
    ctx.cancel = nil;
    ctx.perform = _emptyPerform;
    RSRunLoopSourceRef source = RSRunLoopSourceCreate(RSAllocatorSystemDefault, 0, &ctx);
    RSRunLoopAddSource(__RSURLConnectionRunLoop, source, RSRunLoopDefaultMode);
    RSRelease(source);
    RSUInteger result __unused = RSRunLoopRunInMode(RSRunLoopDefaultMode, 1.0e10, NO);
    RSShow(RSSTR("__RSURLConnectionRunLoop return"));
}
static void __RSURLConnectionDeallocate(RSNotificationRef notification);

RSPrivate void __RSURLConnectionInitialize()
{
    _RSURLConnectionTypeID = __RSRuntimeRegisterClass(&__RSURLConnectionClass);
    __RSRuntimeSetClassTypeID(&__RSURLConnectionClass, _RSURLConnectionTypeID);
    return;
    __RSStartSimpleThread(__RSURLConnectionLoaderMain, nil);
    RSNotificationCenterAddObserver(RSNotificationCenterGetDefault(), RSAutorelease(RSObserverCreate(RSAllocatorSystemDefault, RSCoreFoundationWillDeallocateNotification, &__RSURLConnectionDeallocate, nil)));
}

static void __RSURLConnectionDeallocate(RSNotificationRef notification)
{
    RSSyncUpdateBlock(__RSURLConnectionRunLoopSpinLock, ^{
        if (__RSURLConnectionRunLoop)
            RSRunLoopStop(__RSURLConnectionRunLoop);
    });
}

static RSURLConnectionRef __RSURLConnectionCreateInstance(RSAllocatorRef allocator, RSURLRequestRef request, BOOL async, __weak RSTypeRef delegate, const struct RSURLConnectionDelegate *callbacks)
{
    RSURLConnectionRef instance = (RSURLConnectionRef)__RSRuntimeCreateInstance(allocator, _RSURLConnectionTypeID, sizeof(struct __RSURLConnection) - sizeof(RSRuntimeBase));
    
    __RSURLConnectionSetIsAsync(instance, async);

    instance->_delegate = delegate;
    if (callbacks) __builtin_memcpy(&instance->_impDelegate, callbacks, sizeof(struct RSURLConnectionDelegate));
    
    if (!request) return instance;
    __RSGenericValidInstance(request, RSURLRequestGetTypeID());
    instance->_orignialRequest = RSRetain(request);
    instance->_request = RSMutableCopy(RSAllocatorSystemDefault, request);
    return instance;
}

RSExport RSTypeRef RSURLConnectionGetDelegate(RSURLConnectionRef connection)
{
    if (!connection) return nil;
    __RSGenericValidInstance(connection, _RSURLConnectionTypeID);
    return connection->_delegate;
}

RSExport void RSURLConnectionSetDelegate(RSURLConnectionRef connection, RSTypeRef delegate)
{
    if (!connection) return;
    __RSGenericValidInstance(connection, _RSURLConnectionTypeID);
    connection->_delegate = delegate;
    return;
}

RSExport RSURLRequestRef RSURLConnectionGetOrignialRequest(RSURLConnectionRef connection)
{
    if (!connection) return nil;
    __RSGenericValidInstance(connection, _RSURLConnectionTypeID);
    return connection->_orignialRequest;
}

RSExport RSURLRequestRef RSURLConnectionGetCurrentRequest(RSURLConnectionRef connection) {
    if (!connection) return nil;
    __RSGenericValidInstance(connection, _RSURLConnectionTypeID);
    return connection->_request;
}

RSExport RSTypeRef RSURLConnectionGetContext(RSURLConnectionRef connection) {
    if (!connection) return nil;
    __RSGenericValidInstance(connection, _RSURLConnectionTypeID);
    return connection->_delegate;
}

RSExport void RSURLConnectionSetRequest(RSURLConnectionRef connection, RSURLRequestRef request)
{
    if (!connection) return;
    __RSGenericValidInstance(connection, _RSURLConnectionTypeID);
    if (connection->_orignialRequest) RSRelease(connection->_orignialRequest);
    if (request) connection->_orignialRequest = RSRetain(request);
    return;
}

RSExport RSURLConnectionRef RSURLConnectionCreate(RSAllocatorRef allocator, RSURLRequestRef request, RSTypeRef context, const struct RSURLConnectionDelegate *delegate)
{
    RSURLConnectionRef connection = __RSURLConnectionCreateInstance(allocator, request, YES, context, delegate);
    return connection;
}

RSExport void RSURLConnectionStartOperationInQueue(RSURLConnectionRef connection)
{
    RSSyncUpdateBlock(__RSURLConnectionRunLoopSpinLock, ^{
        if (!__RSURLConnectionRunLoop)
            __RSURLConnectionRunLoop = RSRunLoopGetCurrent();
    });
    return;
}

static void __RSURLConnectionWillStartConnectionRoutine(RSURLConnectionRef connection, RSURLRequestRef willSendRequest) {
    __RSURLConnectionSetupWithURLRequest(connection, willSendRequest, __RSURLConnectionIsAsync(connection));
//    if (!connection->_data) connection->_data = RSDataCreateMutable(RSAllocatorDefault, 0);
//    else RSDataSetLength(connection->_data, 0);
}

static void __RSURLConnectionWillStartConnection(RSURLConnectionRef connection, RSURLRequestRef willSendRequest, RSURLResponseRef redirectResponse) {
    __RSURLConnectionWillStartConnectionRoutine(connection, willSendRequest);
    __RSURLConnectionCoreSetupHTTPDataReceiver(connection);
    __RSURLConnectionInvokeStartConnection(connection, RSURLConnectionGetCurrentRequest(connection), nil);
}

static void __RSURLConnectionDidReceiveResponse(RSURLConnectionRef connection, RSURLResponseRef response) {
    __RSURLConnectionInvokeReceiveResponse(connection, connection->_response);
    RSUInteger errorCode = RSURLResponseGetStatusCode(response);
    RSShow(response);
    if (errorCode == 200) return;
}

static void __RSURLConnectionDidReceiveData(RSURLConnectionRef connection, RSDataRef data) {
//    RSDataAppend((RSMutableDataRef)connection->_data, data);
    __RSURLConnectionInvokeReceiveData(connection, data);
}

static void __RSURLConnectionDidSendBodyData(RSURLConnectionRef connection, RSDataRef data, RSUInteger totalBytesWritten, RSUInteger totalBytesExpectedToWrite) {
    RSShow(RSSTR("send body data"));
    __RSURLConnectionInvokeSendData(connection, data, totalBytesWritten, totalBytesExpectedToWrite);
}

static void __RSURLConnectionDidFinishLoading(RSURLConnectionRef connection) {
    RSShow(RSAutorelease(RSURLCopyHostName(RSURLRequestGetURL(RSURLConnectionGetCurrentRequest(connection)))));
    __RSURLConnectionInvokeFinishLoading(connection);
}

static void __RSURLConnectionDidFailWithError(RSURLConnectionRef connection, RSErrorRef error) {
//    RSRelease(connection->_data);
//    connection->_data = nil;
    __RSURLConnectionInvokeFailWithError(connection, error);
}

static void __RSURLConnectionDidReceiveData_PrivateDelegate(RSURLConnectionRef connection, RSDataRef data) {
    RSDataAppend((RSMutableDataRef)RSURLConnectionGetContext(connection), data);
}

static const struct RSURLConnectionDelegate __RSURLConnectionPrivateDelegate = {
    0,
    nil,
    nil,
    __RSURLConnectionDidReceiveData_PrivateDelegate,
    nil,
    nil,
    nil
};

static const struct RSURLConnectionDelegate __RSURLConnectionImpDelegate = {
    0,
    __RSURLConnectionWillStartConnection,
    __RSURLConnectionDidReceiveResponse,
    __RSURLConnectionDidReceiveData,
    __RSURLConnectionDidSendBodyData,
    __RSURLConnectionDidFinishLoading,
    __RSURLConnectionDidFailWithError
};

#include "RSHTTPCookie.h"

RSExport RSDataRef RSURLConnectionSendSynchronousRequest(RSURLRequestRef request, __autorelease RSURLResponseRef *response, __autorelease RSErrorRef *error) {
    RSMutableDataRef data = RSDataCreateMutable(RSAllocatorSystemDefault, 0);
    RSURLConnectionRef connection = RSURLConnectionCreate(RSAllocatorDefault, request, data, &__RSURLConnectionPrivateDelegate);
    __RSURLConnectionWillStartConnection(connection, RSURLConnectionGetCurrentRequest(connection), nil);
    __RSURLConnectionCorePerform(connection);
    RSArrayRef cookies = RSCookiesWithCore(__RSURLConnectionCoreGet(connection));
    RSShow(cookies);
    
//    RSDataRef data = RSAutorelease(RSRetain(connection->_data));
    if (response && connection->_response) *response = RSAutorelease(RSRetain(connection->_response));
    if (error && connection->_error) *error = RSAutorelease(RSRetain(connection->_error));
    RSRelease(connection);
    return RSAutorelease(data);
}

struct __RSURLConnectionQueueCalloutContext {
    RSURLResponseRef response;
    RSDataRef data;
    RSErrorRef error;
    void (^completeHandler)(__autorelease RSURLResponseRef response, __autorelease RSDataRef data, __autorelease RSErrorRef error);
};

static void __RSURLConnectionQueueCallout(void *info) {
    struct __RSURLConnectionQueueCalloutContext *ctx = (struct __RSURLConnectionQueueCalloutContext *)info;
    ctx->completeHandler(ctx->response, ctx->data, ctx->error);
    if (ctx->response) RSRelease(ctx->response);
    if (ctx->data) RSRelease(ctx->data);
    if (ctx->error) RSRelease(ctx->error);
    Block_release(ctx->completeHandler);
    RSAllocatorDeallocate(RSAllocatorSystemDefault, ctx);
}

RSExport void RSURLConnectionStartOperation(RSURLConnectionRef connection)
{
    if (!connection) return;
    __RSGenericValidInstance(connection, _RSURLConnectionTypeID);
    if (!connection->_delegate) return;
    RSRunLoopPerformBlock(__RSURLConnectionRunLoop, RSRunLoopDefault, ^{
        __RSURLConnectionWillStartConnection(connection, RSURLConnectionGetCurrentRequest(connection), nil);
        __RSURLConnectionCorePerform(connection);
    });
    if (RSRunLoopIsWaiting(__RSURLConnectionRunLoop)) RSRunLoopWakeUp(__RSURLConnectionRunLoop);
    return;
}

RSExport void RSURLConnectionSendAsynchronousRequest(RSURLRequestRef request, RSRunLoopRef rl, void (^completeHandler)(__autorelease RSURLResponseRef response, __autorelease RSDataRef data, __autorelease RSErrorRef error)) {
    if (!request || !completeHandler) return;
    RSRetain(request);
    if (rl == nil) rl = RSRunLoopGetMain();
    RSRunLoopPerformBlock(__RSURLConnectionRunLoop, RSRunLoopDefault, ^{
        RSURLResponseRef response = nil;
        RSErrorRef error = nil;
        RSDataRef data = RSURLConnectionSendSynchronousRequest(request, &response, &error);
        RSRelease(request);
        if (data) RSRetain(data);
        if (response) RSRetain(response);
        if (error) RSRetain(error);
        struct __RSURLConnectionQueueCalloutContext *ctx = RSAllocatorAllocate(RSAllocatorSystemDefault, sizeof(struct __RSURLConnectionQueueCalloutContext));
        ctx->response = response;
        ctx->data = data;
        ctx->error = error;
        ctx->completeHandler = Block_copy(completeHandler);
        dispatch_async_f(__RSRunLoopGetQueue(rl), ctx, __RSURLConnectionQueueCallout);
    });
    if (RSRunLoopIsWaiting(__RSURLConnectionRunLoop)) RSRunLoopWakeUp(__RSURLConnectionRunLoop);
}
