//
//  RSRenrenCoreAnalyzer.c
//  RSRenrenCore
//
//  Created by closure on 1/3/14.
//  Copyright (c) 2014 closure. All rights reserved.
//

#include "RSRenrenCoreAnalyzer.h"
#include "RSRenrenEvent.h"
#include <RSCoreFoundation/RSRuntime.h>

struct __RSRenrenCoreAnalyzer
{
    RSRuntimeBase _base;
    RSURLConnectionRef _connection;
    RSMutableDataRef _reiceveData;
    RSURLResponseRef _response;
    RSDictionaryRef _token;
    RSStringRef _uid;
    
    RSStringRef _email;
    RSStringRef _password;
    void (^_handler)(RSRenrenCoreAnalyzerRef analyzer, RSDataRef data, RSURLResponseRef response, RSErrorRef error);
};

static void __RSRenrenCoreAnalyzerClassInit(RSTypeRef rs)
{
    RSRenrenCoreAnalyzerRef analyzer = (RSRenrenCoreAnalyzerRef)rs;
    analyzer->_reiceveData = RSDataCreateMutable(RSAllocatorDefault, 0);
}

static void __RSRenrenCoreAnalyzerClassDeallocate(RSTypeRef rs)
{
    RSRenrenCoreAnalyzerRef analyzer = (RSRenrenCoreAnalyzerRef)rs;
    RSRelease(analyzer->_connection);
    RSRelease(analyzer->_reiceveData);
    RSRelease(analyzer->_response);
    RSRelease(analyzer->_token);
    RSRelease(analyzer->_uid);
    RSRelease(analyzer->_email);
    RSRelease(analyzer->_password);
}

static BOOL __RSRenrenCoreAnalyzerClassEqual(RSTypeRef rs1, RSTypeRef rs2)
{
    RSRenrenCoreAnalyzerRef RSRenrenCoreAnalyzer1 = (RSRenrenCoreAnalyzerRef)rs1;
    RSRenrenCoreAnalyzerRef RSRenrenCoreAnalyzer2 = (RSRenrenCoreAnalyzerRef)rs2;
    BOOL result = NO;
    
    result = RSEqual(RSRenrenCoreAnalyzer1->_uid, RSRenrenCoreAnalyzer2->_uid);
    
    return result;
}

static RSHashCode __RSRenrenCoreAnalyzerClassHash(RSTypeRef rs)
{
    return RSHash(((RSRenrenCoreAnalyzerRef)rs)->_uid);
}

static RSStringRef __RSRenrenCoreAnalyzerClassDescription(RSTypeRef rs)
{
    RSStringRef description = RSStringCreateWithFormat(RSAllocatorDefault, RSSTR("RSRenrenCoreAnalyzer <%p> - %r"), rs, ((RSRenrenCoreAnalyzerRef)rs)->_uid);
    return description;
}

static RSRuntimeClass __RSRenrenCoreAnalyzerClass =
{
    _RSRuntimeScannedObject,
    "RSRenrenCoreAnalyzer",
    __RSRenrenCoreAnalyzerClassInit,
    nil,
    __RSRenrenCoreAnalyzerClassDeallocate,
    __RSRenrenCoreAnalyzerClassEqual,
    __RSRenrenCoreAnalyzerClassHash,
    __RSRenrenCoreAnalyzerClassDescription,
    nil,
    nil
};

static RSSpinLock _RSRenrenCoreAnalyzerSpinLock = RSSpinLockInit;
static RSTypeID _RSRenrenCoreAnalyzerTypeID = _RSRuntimeNotATypeID;

static void __RSRenrenCoreAnalyzerInitialize()
{
    _RSRenrenCoreAnalyzerTypeID = __RSRuntimeRegisterClass(&__RSRenrenCoreAnalyzerClass);
    __RSRuntimeSetClassTypeID(&__RSRenrenCoreAnalyzerClass, _RSRenrenCoreAnalyzerTypeID);
}

RSExport RSTypeID RSRenrenCoreAnalyzerGetTypeID()
{
    RSSyncUpdateBlock(&_RSRenrenCoreAnalyzerSpinLock, ^{
        if (_RSRuntimeNotATypeID == _RSRenrenCoreAnalyzerTypeID)
            __RSRenrenCoreAnalyzerInitialize();
    });
    return _RSRenrenCoreAnalyzerTypeID;
}

static RSStringRef uid(RSStringRef info) {
    RSStringRef uid = RSStringByReplacingOccurrencesOfString(info, RSSTR("http://www.renren.com/"), RSSTR(""));
    return uid;
}

static RSMutableDictionaryRef token(RSStringRef uid) {
    __block RSStringRef requestToken = nil, _rtk = nil;
    RSAutoreleaseBlock(^{
        RSStringRef str = RSStringWithData(RSDataWithURL(RSURLWithString(RSStringWithFormat(RSSTR("http://www.renren.com/%r"), uid))), RSStringEncodingUTF8);
        RSStringWriteToFile(str, RSFileManagerStandardizingPath(RSSTR("~/Desktop/renren.html")), RSWriteFileAutomatically);
        RSRange range = RSStringRangeOfString(str, RSSTR("get_check:'"));
        str = RSStringWithSubstring(str, RSMakeRange(range.location + range.length, RSStringGetLength(str) - range.location - range.length - 1));
        range = RSStringRangeOfString(str, RSSTR("'"));
        
        requestToken = RSStringWithSubstring(str, RSMakeRange(0, range.location));
        str = RSStringWithSubstring(str, RSMakeRange(range.location + range.length, RSStringGetLength(str) - range.location - range.length - 1));
        
        range = RSStringRangeOfString(str, RSSTR("get_check_x:'"));
        str = RSStringWithSubstring(str, RSMakeRange(range.location + range.length, RSStringGetLength(str) - range.location - range.length - 1));
        
        range = RSStringRangeOfString(str, RSSTR("'"));
        _rtk = RSStringWithSubstring(str, RSMakeRange(0, range.location));
        
        RSRetain(requestToken);
        RSRetain(_rtk);
    });
    return RSAutorelease(RSMutableCopy(RSAllocatorDefault, RSAutorelease(RSDictionaryCreateWithObjectsAndOKeys(RSAllocatorDefault, RSAutorelease(requestToken), RSSTR("requestToken"), RSAutorelease(_rtk), RSSTR("_rtk"), NULL))));
}

#pragma mark -
#pragma mark RSURLConnectionDelegate

static void RSURLConnectionWillStartConnection_CoreAnalyzer(RSURLConnectionRef connection, RSURLRequestRef willSendRequest, RSURLResponseRef redirectResponse) {
    ;
}

static void RSURLConnectionDidReceiveResponse_CoreAnalyzer(RSURLConnectionRef connection, RSURLResponseRef response) {
    if (((RSRenrenCoreAnalyzerRef)RSURLConnectionGetContext(connection))->_response) {
        RSRelease(((RSRenrenCoreAnalyzerRef)RSURLConnectionGetContext(connection))->_response);
        ((RSRenrenCoreAnalyzerRef)RSURLConnectionGetContext(connection))->_response = nil;
    }
    ((RSRenrenCoreAnalyzerRef)RSURLConnectionGetContext(connection))->_response = (RSURLResponseRef)RSRetain(response);
}

static void RSURLConnectionDidReceiveData_CoreAnalyzer(RSURLConnectionRef connection, RSDataRef data){
    RSDataAppend((RSMutableDataRef)(((RSRenrenCoreAnalyzerRef)RSURLConnectionGetContext(connection))->_reiceveData), data);
}

static void RSURLConnectionDidSendBodyData_CoreAnalyzer(RSURLConnectionRef connection, RSDataRef data, RSUInteger totalBytesWritten, RSUInteger totalBytesExpectedToWrite){
    
}

static void RSURLConnectionDidFailWithError_CoreAnalyzer(RSURLConnectionRef connection, RSErrorRef error) {
    RSShow(RSSTR("login failed"));
    RSRenrenCoreAnalyzerRef analyzer = (RSRenrenCoreAnalyzerRef)RSURLConnectionGetContext(connection);
    analyzer->_handler(analyzer, analyzer->_reiceveData, analyzer->_response, error);
}

static RSURLRequestRef remoteUpdateUser(RSRenrenCoreAnalyzerRef analyzer, RSStringRef uid, RSUInteger count) {
    if (!analyzer) return nil;
    RSMutableURLRequestRef request = RSURLRequestCreateMutable(RSAllocatorDefault, RSURLWithString(RSStringWithFormat(RSSTR("http://www.renren.com/moreminifeed.do?p=%r&u=%r&requestToken=%r&_rtk=%r"), RSNumberWithUnsignedInteger(count), uid, RSDictionaryGetValue(analyzer->_token, RSSTR("requestToken")), RSDictionaryGetValue(analyzer->_token, RSSTR("_rtk")))));
    return RSAutorelease(request);
}

static void RSURLConnectionDidFinishLoading_CoreAnalyzer(RSURLConnectionRef connection){
    RSRenrenCoreAnalyzerRef analyzer = (RSRenrenCoreAnalyzerRef)RSURLConnectionGetContext(connection);
    if (RSStringRangeOfString(RSURLGetString(RSURLResponseGetURL(analyzer->_response)), RSSTR("failCode=")).length > 0) {
        RSURLConnectionDidFailWithError_CoreAnalyzer(connection, nil);
        return;
    }
    analyzer->_token = RSRetain(token(analyzer->_uid = RSRetain(uid(RSURLGetString(RSURLResponseGetURL(analyzer->_response))))));
    analyzer->_handler(analyzer, analyzer->_reiceveData, analyzer->_response, nil);
}

static RSStringRef __buildURLString(RSStringRef baseString, RSStringRef domain, RSStringRef account, RSStringRef password) {
    RSStringRef const _append = RSSTR("&");
    RSStringRef const _origURL = RSSTR("origURL=");
    RSStringRef const _domain = RSSTR("domain=");
    RSStringRef const _password = RSSTR("password=");
    RSStringRef const _account = RSSTR("email=");
    RSStringRef urlString = RSStringWithFormat(RSSTR("%r%r%r%r%r%r%r%r%r%r%r"), _origURL, baseString, _append, _domain, domain, _append, _password, password, _append, _account, account);
    return urlString;
}

static RSRenrenCoreAnalyzerRef __RSRenrenCoreAnalyzerCreateInstance(RSAllocatorRef allocator, RSStringRef email, RSStringRef password, void (^handler)(RSRenrenCoreAnalyzerRef analyzer, RSDataRef data, RSURLResponseRef response, RSErrorRef error))
{
    RSRenrenCoreAnalyzerRef instance = (RSRenrenCoreAnalyzerRef)__RSRuntimeCreateInstance(allocator, RSRenrenCoreAnalyzerGetTypeID(), sizeof(struct __RSRenrenCoreAnalyzer) - sizeof(RSRuntimeBase));
    RSStringRef baseURLString = RSSTR("http://www.renren.com/PLogin.do");
    RSMutableURLRequestRef request = RSAutorelease(RSURLRequestCreateMutable(RSAllocatorDefault, RSURLWithString(baseURLString)));
    RSStringRef post = __buildURLString(RSSTR("http://www.renren.com/SysHome.do"), RSSTR("renren.com"), email, password);
    RSDataRef postData = RSAutorelease(RSStringCreateExternalRepresentation(RSAllocatorDefault, post, RSStringEncodingASCII, YES));
    RSURLRequestSetHTTPMethod(request, RSSTR("POST"));
    RSURLRequestSetHeaderFieldValue(request, RSSTR("Content-Length"), RSStringWithFormat(RSSTR("%ld"), RSDataGetLength(postData)));
    RSURLRequestSetHeaderFieldValue(request, RSSTR("Content-Type"), RSSTR("application/x-www-form-urlencoded"));
    RSURLRequestSetHTTPBody(request, postData);
    
    struct RSURLConnectionDelegate delegate = {
        0,
        RSURLConnectionWillStartConnection_CoreAnalyzer,
        RSURLConnectionDidReceiveResponse_CoreAnalyzer,
        RSURLConnectionDidReceiveData_CoreAnalyzer,
        RSURLConnectionDidSendBodyData_CoreAnalyzer,
        RSURLConnectionDidFinishLoading_CoreAnalyzer,
        RSURLConnectionDidFailWithError_CoreAnalyzer
    };
    instance->_connection = RSURLConnectionCreate(RSAllocatorDefault, request, instance, &delegate);
    instance->_handler = handler;
    return instance;
}

RSExport RSRenrenCoreAnalyzerRef RSRenrenCoreAnalyzerCreate(RSAllocatorRef allocator, RSStringRef email, RSStringRef password, void (^handler)(RSRenrenCoreAnalyzerRef analyzer, RSDataRef data, RSURLResponseRef response, RSErrorRef error)) {
    if (!email || !password) return nil;
    return __RSRenrenCoreAnalyzerCreateInstance(allocator, email, password, handler);
}

RSExport void RSRenrenCoreAnalyzerStartLogin(RSRenrenCoreAnalyzerRef analyzer) {
    if (!analyzer || !analyzer->_connection) return;
    RSURLConnectionStartOperation(analyzer->_connection);
}

static RSMutableArrayRef substringWithRegular(RSStringRef self, RSStringRef regular) {
    RSStringRef reg=regular;
    RSRegularExpressionRef expression = RSRegularExpressionCreate(RSAllocatorDefault, reg);
    if (!expression) return nil;
    RSArrayRef results = RSRegularExpressionApplyOnString(expression, self, nil);
    RSRelease(expression);
    if (!results) return nil;
    RSMutableArrayRef arr = RSArrayCreateMutable(RSAllocatorDefault, 0);
    RSArrayApplyBlock(results, RSMakeRange(0, RSArrayGetCount(results)), ^(const void *value, RSUInteger idx, BOOL *isStop) {
        RSArrayAddObject(arr, RSStringWithSubstring(self, RSNumberRangeValue(value)));
    });
    return RSAutorelease(arr);
}

RSExport RSArrayRef RSRenrenCoreAnalyzerCreateLikeEvent(RSRenrenCoreAnalyzerRef analyzer, RSStringRef content, BOOL addlike) {
    RSStringRef __kMatchExpress1 = RSSTR("\\(?onclick=\\\"ILike_toggleUserLike\\(.*?\\)\\\"");
    RSStringRef __kMatchExpress2 = RSSTR("\\(.*\\)");
    if (!analyzer) return nil;
    RSStringRef htmlContent = content;
    if (!htmlContent) return nil;
    RSArrayRef regexResults = substringWithRegular(htmlContent, __kMatchExpress1);
    RSMutableArrayRef results = RSArrayCreateMutable(RSAllocatorDefault, RSArrayGetCount(regexResults));
    RSArrayApplyBlock(regexResults, RSMakeRange(0, RSArrayGetCount(regexResults)), ^(const void *value, RSUInteger idx, BOOL *isStop) {
        RSArrayAddObject(results, RSArrayObjectAtIndex(substringWithRegular(value, __kMatchExpress2), 0));
    });
    regexResults = nil;
    RSMutableArrayRef models = RSArrayCreateMutable(RSAllocatorDefault, 0);
    RSArrayApplyBlock(results, RSMakeRange(0, RSArrayGetCount(results)), ^(const void *value, RSUInteger idx, BOOL *isStop) {
        RSArrayAddObject(models, RSAutorelease(RSRenrenEventCreateLikeWithContent(RSAllocatorDefault, value, addlike)));
    });
    RSRelease(results);
    return (models);
}

RSExport void RSRenrenCoreAnalyzerCreateEventContentsWithUserId(RSRenrenCoreAnalyzerRef analyzer, RSStringRef userId, RSUInteger count, void (^handler)(RSRenrenEventRef), void (^compelete)()) {
    if (!analyzer || !userId || !count || !handler) return;
    __RSGenericValidInstance(analyzer, _RSRenrenCoreAnalyzerTypeID);
    RSPerformBlockInBackGround(^{
        __block RSUInteger begin = 0;
        __block RSUInteger limit = 0;
        __block BOOL shouldContinue = YES;
        RSMutableArrayRef models = RSArrayCreateMutable(RSAllocatorDefault, 0);
        while (shouldContinue) {
            if (count <= begin) {
                shouldContinue = NO;
                break;
            }
            RSAutoreleaseBlock(^{
                RSURLResponseRef response = nil;
                RSDataRef data = nil;
                RSErrorRef error = nil;
                data = RSURLConnectionSendSynchronousRequest(remoteUpdateUser(analyzer, userId, limit), &response, &error);
                if (RSURLResponseGetStatusCode(response) == 200)
                    RSArrayAddObjects(models, RSAutorelease(RSRenrenCoreAnalyzerCreateLikeEvent(analyzer, RSStringWithData(data, RSStringEncodingUTF8), YES)));
                else
                    shouldContinue = NO;
                begin = RSArrayGetCount(models);
                limit ++;
                RSLog(RSSTR("Collecting items for user(%r)... Loop = %ld, count = %ld"), userId, limit, begin);
                sleep(1.25);
            });
        }
        if (RSArrayGetCount(models)) {
            RSArrayApplyBlock(models, RSMakeRange(0, RSArrayGetCount(models)), ^(const void *value, RSUInteger idx, BOOL *isStop) {
                if (idx > count) {
                    *isStop = YES;
                    return ;
                }
                RSLog(RSSTR("Zan (%ld) for user (%r)"), idx, userId);
                RSRenrenEventRef event = value;
                handler(event);
            });
            RSShow(RSSTR("end"));
        }
        RSRelease(models);
        if (compelete) compelete();
    });
}

RSExport RSStringRef RSRenrenCoreAnalyzerGetUserId(RSRenrenCoreAnalyzerRef analyzer) {
    if (!analyzer) return nil;
    __RSGenericValidInstance(analyzer, _RSRenrenCoreAnalyzerTypeID);
    return analyzer->_uid;
}

RSExport RSStringRef RSRenrenCoreAnalyzerGetEmail(RSRenrenCoreAnalyzerRef analyzer) {
    if (!analyzer) return nil;
    __RSGenericValidInstance(analyzer, _RSRenrenCoreAnalyzerTypeID);
    return analyzer->_email;
}

RSExport RSTypeRef RSRenrenCoreAnalyzerGetToken(RSRenrenCoreAnalyzerRef analyzer) {
    if (!analyzer) return nil;
    __RSGenericValidInstance(analyzer, _RSRenrenCoreAnalyzerTypeID);
    return analyzer->_token;
}

RSExport RSURLResponseRef RSRenrenCoreAnalyzerGetResponse(RSRenrenCoreAnalyzerRef analyzer) {
    if (!analyzer) return nil;
    __RSGenericValidInstance(analyzer, _RSRenrenCoreAnalyzerTypeID);
    return analyzer->_response;
}

