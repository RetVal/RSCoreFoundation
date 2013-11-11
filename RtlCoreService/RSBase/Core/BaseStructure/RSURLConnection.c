//
//  RSURLConnection.c
//  RSCoreFoundation
//
//  Created by Closure on 11/2/13.
//  Copyright (c) 2013 RetVal. All rights reserved.
//

#include "RSURLConnection.h"
#include <assert.h>

#include <RSCoreFoundation/RSRuntime.h>
#include <RSCoreFoundation/RSURL.h>
#include <RSCoreFoundation/RSURLRequest.h>
#include <RSCoreFoundation/RSRunLoop.h>

#include "RSPrivate/libcurl/include/curl/curl.h"

//RS_CONST_STRING_DECL(__RSURLConnection
RS_CONST_STRING_DECL(__RSURLConnectionUserAgent, "User-Agent");

typedef void (*RSURLConnectionWillStartConnection)(RSURLConnectionRef connection, RSURLRequestRef willSendRequest, RSTypeRef redirectResponse);

typedef void (*RSURLConnectionDidReceiveResponse)(RSURLConnectionRef connection, RSTypeRef response);
typedef void (*RSURLConnectionDidReceiveData)(RSURLConnectionRef connection, RSDataRef data);
typedef void (*RSURLConnectionDidSendBodyData)(RSURLConnectionRef connection, RSDataRef data, RSUInteger totalBytesWritten, RSUInteger totalBytesExpectedToWrite);

typedef void (*RSURLConnectionDidFinishLoading)(RSURLConnectionRef connection);
typedef void (*RSURLConnectionDidFailWithError)(RSURLConnectionRef connection, RSErrorRef error);

struct RSURLConnectionDelegate {
    RSURLConnectionWillStartConnection startConnection;
    
    RSURLConnectionDidReceiveResponse receiveResponse;
    RSURLConnectionDidReceiveData receiveData;
    RSURLConnectionDidSendBodyData sendData;
    
    RSURLConnectionDidFinishLoading finishLoading;
    RSURLConnectionDidFailWithError failWithError;
};

#ifndef __RSURLConnectionInvoke
#define __RSURLConnectionInvoke(method, ...) do { if ((method) != nil) (method)(__VA_ARGS__);}while(0);

#define __RSURLConnectionInvokeStartConnection(connection, ...)   __RSURLConnectionInvoke(((connection)->_impDelegate).startConnection, connection, __VA_ARGS__)
#define __RSURLConnectionInvokeReceiveResponse(connection, ...)   __RSURLConnectionInvoke(((connection)->_impDelegate).receiveResponse, connection, __VA_ARGS__)
#define __RSURLConnectionInvokeReceiveData(connection, ...)       __RSURLConnectionInvoke(((connection)->_impDelegate).receiveData, connection, __VA_ARGS__)
#define __RSURLConnectionInvokeSendData(connection, ...)          __RSURLConnectionInvoke(((connection)->_impDelegate).sendData, connection, __VA_ARGS__)
#define __RSURLConnectionInvokeFinishLoading(connection, ...)     __RSURLConnectionInvoke(((connection)->_impDelegate).finishLoading, connection, __VA_ARGS__)
#define __RSURLConnectionInvokeFailWithError(connection, ...)     __RSURLConnectionInvoke(((connection)->_impDelegate).failWithError, connection, __VA_ARGS__)
#endif


struct _core
{
    void *_core;
    void *_core_chunk;
};

struct __RSURLConnection
{
    RSRuntimeBase _base;
    RSURLRequestRef _request;
    
    RSMutableDataRef _responseHeader;
    
    RSTypeRef _delegate;    // weak
    struct RSURLConnectionDelegate _impDelegate;
    struct _core _core;
    
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
        curl_easy_reset(core);
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

RSInline void __RSURLConnectionCoreSetupHTTPHeaderField(RSURLConnectionRef connection, RSDictionaryRef headerField)
{
    assert(connection || headerField);
    void *core = __RSURLConnectionCoreGet(connection);
    RSUInteger cnt = RSDictionaryGetCount(headerField);
    if (0 == cnt) return;
    struct curl_slist *chunk = nil;
    RSMutableStringRef mutableString = RSStringCreateMutable(RSAllocatorSystemDefault, 0);
    RSArrayRef keys = RSDictionaryAllKeys(headerField);
    RSArrayRef values = RSDictionaryAllValues(headerField);
    RSStringRef format = RSSTR("%r: %r");
    for (RSUInteger idx = 0; idx < cnt; idx++)
    {
        RSStringAppendStringWithFormat(mutableString, format, RSArrayObjectAtIndex(keys, idx), RSArrayObjectAtIndex(values, idx));
        __RSLog(RSLogLevelNotice, RSSTR("%s"), RSStringGetCStringPtr(mutableString, RSStringEncodingUTF8));
        chunk = curl_slist_append(chunk, RSStringGetCStringPtr(mutableString, RSStringEncodingUTF8));
    }
    RSRelease(keys);
    RSRelease(values);
    RSRelease(mutableString);
    curl_easy_setopt(core, CURLOPT_HTTPHEADER, chunk);
    void *org = __RSURLConnectionCoreSetupChunk(connection, chunk);
    if (org) curl_slist_free_all(org);
}

static size_t __RSURLConnectionCoreAppendReceiveResponseHeader(void *ptr, size_t size, size_t nmemb, RSURLConnectionRef connection)
{
    if (!connection) return size * nmemb;
    size_t realSize = size * nmemb;
    
    RSDataAppendBytes(connection->_responseHeader, ptr, realSize);
    return realSize;
}

static size_t __RSURLConnectionCoreAppendReceiveData(void *ptr, size_t size, size_t nmemb, RSURLConnectionRef connection)
{
    if (!connection) return size * nmemb;
    size_t realSize = size * nmemb;
    
    RSDataRef data = RSDataCreate(RSAllocatorSystemDefault, ptr, realSize);
    __RSURLConnectionInvokeReceiveResponse(connection, data);
    RSRelease(data);
    
    return realSize;
}

RSInline void __RSURLConnectionCoreSetupHTTPDataReceiver(RSURLConnectionRef connection)
{
//    typedef CURLcode (*CoreSetOptFunction)(CURL *curl, CURLoption tag, ...);
//    CoreSetOptFunction core_setopt = __RSURLConnectionIsAsync(connection) ? curl_multi_setopt : curl_easy_setopt;
    curl_easy_setopt(__RSURLConnectionCoreGet(connection), CURLOPT_WRITEDATA, connection);
    curl_easy_setopt(__RSURLConnectionCoreGet(connection), CURLOPT_WRITEFUNCTION, __RSURLConnectionCoreAppendReceiveData);
    curl_easy_setopt(__RSURLConnectionCoreGet(connection), CURLOPT_HEADERDATA, connection);
    curl_easy_setopt(__RSURLConnectionCoreGet(connection), CURLOPT_HEADERFUNCTION,__RSURLConnectionCoreAppendReceiveResponseHeader);
}

static void __RSURLConnectionCoreSetupUserAgent(void *core, const char *userAgent)
{
    assert(core);
    curl_easy_setopt(core, CURLOPT_USERAGENT, userAgent ? : "RSCoreNetworking-agent/1.0");
}

static RSURLConnectionRef __RSURLConnectionSetupWithURLRequest(RSURLConnectionRef connection, RSURLRequestRef request, BOOL async)
{
    __RSURLConnectionCoreSetupURL(__RSURLConnectionCoreGet(connection), RSStringGetCStringPtr(RSURLGetString(RSURLRequestGetURL(request)), RSStringEncodingUTF8));
    RSDictionaryRef headerField = RSURLRequestGetHeaderField(request);
    if (headerField)
    {
        __RSURLConnectionCoreSetupHTTPHeaderField(connection, headerField);
    }
    else
    {
        __RSURLConnectionCoreSetupUserAgent(__RSURLConnectionCoreGet(connection), nil);
    }
    return connection;
}

static RSURLConnectionRef __RSURLConnectionCorePerform(RSURLConnectionRef connection)
{
    void *core = __RSURLConnectionCoreGet(connection);
    CURLcode retCode = curl_easy_perform(core);
    if (retCode) __RSCLog(RSLogLevelNotice, "RSURLConnection Notice ");
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
    if (connection->_request) RSRelease(connection->_request);
    connection->_request = nil;
}

static BOOL __RSURLConnectionClassEqual(RSTypeRef rs1, RSTypeRef rs2)
{
    RSURLConnectionRef RSURLConnection1 = (RSURLConnectionRef)rs1;
    RSURLConnectionRef RSURLConnection2 = (RSURLConnectionRef)rs2;
    BOOL result = NO;
    
    result = RSEqual(RSURLConnection1->_request, RSURLConnection2->_request);
    
    return result;
}

static RSHashCode __RSURLConnectionClassHash(RSTypeRef rs)
{
    return RSHash(((RSURLConnectionRef)rs)->_request);
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

RSPrivate void __RSURLConnectionInitialize()
{
    _RSURLConnectionTypeID = __RSRuntimeRegisterClass(&__RSURLConnectionClass);
    __RSRuntimeSetClassTypeID(&__RSURLConnectionClass, _RSURLConnectionTypeID);
}

RSPrivate void __RSURLConnectionDeallocate()
{
//    <#do your finalize operation#>
}

static RSURLConnectionRef __RSURLConnectionCreateInstance(RSAllocatorRef allocator, RSURLRequestRef request, BOOL async, __weak RSTypeRef delegate, struct RSURLConnectionDelegate *callbacks)
{
    RSURLConnectionRef instance = (RSURLConnectionRef)__RSRuntimeCreateInstance(allocator, _RSURLConnectionTypeID, sizeof(struct __RSURLConnection) - sizeof(RSRuntimeBase));
    
    __RSURLConnectionSetIsAsync(instance, async);

    instance->_delegate = delegate;
    if (callbacks) __builtin_memcpy(&instance->_impDelegate, callbacks, sizeof(struct RSURLConnectionDelegate));
    
    if (!request) return instance;
    __RSGenericValidInstance(request, RSURLRequestGetTypeID());
    instance->_request = RSRetain(request);
    return __RSURLConnectionSetupWithURLRequest(instance, request, async);;
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

RSExport RSURLRequestRef RSURLConnectionGetRequest(RSURLConnectionRef connection)
{
    if (!connection) return nil;
    __RSGenericValidInstance(connection, _RSURLConnectionTypeID);
    return connection->_request;
}

RSExport void RSURLConnectionSetRequest(RSURLConnectionRef connection, RSURLRequestRef request)
{
    if (!connection) return;
    __RSGenericValidInstance(connection, _RSURLConnectionTypeID);
    if (connection->_request) RSRelease(connection->_request);
    if (request) connection->_request = RSRetain(request);
    return;
}

RSExport RSURLConnectionRef RSURLConnectionCreate(RSAllocatorRef allocator, RSURLRequestRef request, RSTypeRef context, struct RSURLConnectionDelegate *delegate)
{
    RSURLConnectionRef connection = __RSURLConnectionCreateInstance(allocator, request, YES, context, delegate);
    return connection;
}

static RSRunLoopRef __RSURLConnectionRunLoop = nil;
static RSSpinLock __RSURLConnectionRunLoopSpinLock = RSSpinLockInit;

RSExport void RSURLConnectionStartOperationInQueue(RSURLConnectionRef connection)
{
//    RSSyncUpdateBlock(__RSURLConnectionRunLoopSpinLock, ^{
//        if (!__RSURLConnectionRunLoop)
//            __RSURLConnectionRunLoop = RSRunLoopGetCurrent();
//    });
//    RSSyncUpdateBlock(__RSURLConnectionRunLoopSpinLock, ^{
//        RSRunLoopPerformBlockInRunLoop(__RSURLConnectionRunLoop, ^{
//            
//        });
//    });
    return;
}

RSExport void RSURLConnectionStartOperation(RSURLConnectionRef connection)
{
    if (!connection) return;
    __RSGenericValidInstance(connection, _RSURLConnectionTypeID);
    if (!connection->_delegate) return;
    __RSURLConnectionInvokeStartConnection(connection, connection->_request, nil);
    if (__RSURLConnectionIsAsync(connection))
    {
//        RSPerformBlockInBackGround(^{
//            __RSURLConnectionCorePerform(connection);
//        });
        return;
    }
    __RSURLConnectionCorePerform(connection);
    return;
}


static void __RSURLConnectionDebug()
{
    
//    RSPerformBlockInBackGround(^{
//        RSURLConnectionStartOperation(RSAutorelease(RSURLConnectionCreate(RSAllocatorSystemDefault, RSURLRequestWithURL(RSURLWithString(RSSTR("http://www.baidu.com"))), nil, nil)));
//    });
}
