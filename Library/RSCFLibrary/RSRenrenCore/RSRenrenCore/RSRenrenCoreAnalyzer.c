//
//  RSRenrenCoreAnalyzer.c
//  RSRenrenCore
//
//  Created by closure on 1/3/14.
//  Copyright (c) 2014 closure. All rights reserved.
//

#include "RSRenrenCoreAnalyzer.h"
#include "RSRenrenEvent.h"
#include "RSRenrenFriend.h"
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
    
    result = RSEqual(RSRenrenCoreAnalyzer1->_uid, RSRenrenCoreAnalyzer2->_uid) &&
             RSEqual(RSRenrenCoreAnalyzer1->_token, RSRenrenCoreAnalyzer2->_token);

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

RSExport void RSRenrenCoreAnalyzerCreateEventContentsWithUserId(RSRenrenCoreAnalyzerRef analyzer, RSStringRef userId, RSUInteger count, BOOL like, void (^handler)(RSRenrenEventRef), void (^compelete)()) {
    if (!analyzer || !userId || !count || !handler) return;
    __RSGenericValidInstance(analyzer, _RSRenrenCoreAnalyzerTypeID);
    RSRetain(userId);
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
                    RSArrayAddObjects(models, RSAutorelease(RSRenrenCoreAnalyzerCreateLikeEvent(analyzer, RSStringWithData(data, RSStringEncodingUTF8), like)));
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
                RSLog(RSSTR("DoEventJob %ld"), idx);
                RSRenrenEventRef event = value;
                handler(event);
            });
            RSShow(RSSTR("end"));
        }
        RSRelease(models);
        RSRelease(userId);
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

#pragma mark -
#pragma mark Friend Analyzer 

RSInline RSStringRef baseURLStringFormatForDumpFriendList()
{
    return RSSTR("http://friend.renren.com/GetFriendList.do?curpage=%ld&id=%r");
}

RSInline RSURLRef urlForCoreDumpFriendListWithUserId(RSStringRef userId, RSUInteger pageNumber)
{
    return RSURLWithString(RSStringWithFormat(baseURLStringFormatForDumpFriendList(), pageNumber, userId));
}

RSInline RSArrayRef friendsElementsFromDocument(RSXMLDocumentRef document) {
    RSXMLElementRef root = RSXMLDocumentGetRootElement(document);
    RSXMLElementRef body = RSArrayObjectAtIndex(RSXMLElementGetElementsForName(root, RSSTR("body")), 0);
    RSXMLElementRef containerDiv = RSArrayObjectAtIndex(RSXMLElementGetElementsForName(body, RSSTR("div")), 0);
    RSXMLElementRef containerForBuddyListDiv = RSArrayObjectAtIndex(RSXMLElementGetElementsForName(containerDiv, RSSTR("div")), 3);
    RSXMLElementRef contentDiv = RSArrayObjectAtIndex(RSXMLElementGetElementsForName(RSArrayObjectAtIndex(RSXMLElementGetElementsForName(RSArrayObjectAtIndex(RSXMLElementGetElementsForName(RSArrayObjectAtIndex(RSXMLElementGetElementsForName(RSArrayObjectAtIndex(RSXMLElementGetElementsForName(containerForBuddyListDiv, RSSTR("div")), 0), RSSTR("div")), 0), RSSTR("div")), 0), RSSTR("div")), 0), RSSTR("div")), 0);
    RSXMLElementRef stdContainerDiv = RSArrayObjectAtIndex(RSXMLElementGetElementsForName(contentDiv, RSSTR("div")), 1);
    RSXMLElementRef list_resultDiv = RSArrayObjectAtIndex(RSXMLElementGetElementsForName(stdContainerDiv, RSSTR("div")), 1);
    RSXMLElementRef friendListCon_ol = RSArrayObjectAtIndex(RSXMLElementGetElementsForName(list_resultDiv, RSSTR("ol")), 1);
    return RSXMLElementGetElementsForName(friendListCon_ol, RSSTR("li"));
}

RSInline RSDictionaryRef analyzeFriendAvatar(RSXMLElementRef avatar) {
    RSMutableDictionaryRef avatarInfo = RSAutorelease(RSDictionaryCreateMutable(RSAllocatorDefault, 0, RSDictionaryRSTypeContext));
    RSXMLElementRef a = RSArrayLastObject(RSXMLElementGetElementsForName(avatar, RSSTR("a")));
    RSXMLNodeRef href = RSXMLElementGetAttributeForName(a, RSSTR("href"));
    RSDictionarySetValue(avatarInfo, RSSTR("homePageLink"), RSXMLNodeGetValue(href));
    RSXMLElementRef img = RSArrayLastObject(RSXMLElementGetElementsForName(a, RSSTR("img")));
    RSXMLNodeRef src = RSXMLElementGetAttributeForName(img, RSSTR("src"));
    RSDictionarySetValue(avatarInfo, RSSTR("headImageLink"), RSXMLNodeGetValue(src));
    return avatarInfo;
}

RSInline RSDictionaryRef analyzeFriendForInfo(RSXMLElementRef info) {
    RSMutableDictionaryRef infoInfo = RSAutorelease(RSDictionaryCreateMutable(RSAllocatorDefault, 0, RSDictionaryRSTypeContext));
    RSXMLElementRef dl = RSArrayLastObject(RSXMLElementGetElementsForName(info, RSSTR("dl")));
    RSXMLElementRef dd = RSArrayObjectAtIndex(RSXMLElementGetElementsForName(dl, RSSTR("dd")), 0);
    RSXMLElementRef dd_location = RSArrayLastObject(RSXMLElementGetElementsForName(dl, RSSTR("dd")));
    RSXMLElementRef a = RSArrayLastObject(RSXMLElementGetElementsForName(dd, RSSTR("a")));
    RSXMLNodeRef href __unused = RSXMLElementGetAttributeForName(a, RSSTR("href"));
    if (dd_location && NO == RSEqual(dd, dd_location)) {
        RSDictionarySetValue(infoInfo, RSSTR("name"), RSXMLNodeGetValue((RSXMLNodeRef)a));
        RSDictionarySetValue(infoInfo, RSSTR("school"), RSStringByTrimmingCharactersInSet(RSXMLNodeGetValue((RSXMLNodeRef)dd_location), RSCharacterSetGetPredefined(RSCharacterSetNewline)));
    } else {
        RSDictionarySetValue(infoInfo, RSSTR("name"), RSXMLNodeGetValue((RSXMLNodeRef)a));
    }
    return infoInfo;
}

RSInline RSStringRef analyzeFriendForAccountID(RSRenrenFriendRef friend) {
    
    RSStringRef homelink = RSURLGetString(RSAutorelease(RSURLCopyAbsoluteURL(RSRenrenFriendGetHomePageURL(friend))));
    RSStringRef userId = nil;
    if (RSStringHasPrefix(homelink, RSSTR("http://www.renren.com/profile.do?id=")))
        userId = RSStringByReplacingOccurrencesOfString(RSAutorelease(RSMutableCopy(RSAllocatorDefault, homelink)), RSSTR("http://www.renren.com/profile.do?id="), RSSTR(""));
    else
        userId = RSSTR("0");
    return userId;
}

RSInline RSTypeRef analyzeFriend(RSXMLElementRef element) {
    if (!element) return nil;
    RSXMLElementRef avatar = RSArrayLastObject(RSXMLElementGetElementsForName(element, RSSTR("p")));
    RSXMLElementRef info = RSArrayLastObject(RSXMLElementGetElementsForName(element, RSSTR("div")));
    RSXMLElementRef actions = RSArrayLastObject(RSXMLElementGetElementsForName(element, RSSTR("ul")));
    if (!avatar || !info ||!actions) return nil;
    RSDictionaryRef avatarResult = analyzeFriendAvatar(avatar);
    RSDictionaryRef infoResult = analyzeFriendForInfo(info);
    RSRenrenFriendRef friend = RSAutorelease(RSRenrenFriendCreate(RSAllocatorDefault));
    RSRenrenFriendSetHomePageURL(friend, RSURLWithString(RSDictionaryGetValue(avatarResult, RSSTR("homePageLink"))));
    RSRenrenFriendSetImageURL(friend, RSURLWithString(RSDictionaryGetValue(avatarResult, RSSTR("headImageLink"))));
    RSRenrenFriendSetName(friend, RSDictionaryGetValue(infoResult, RSSTR("name")));
    RSRenrenFriendSetSchoolName(friend, RSDictionaryGetValue(infoResult, RSSTR("schoolName")));
    RSRenrenFriendSetAccountID(friend, analyzeFriendForAccountID(friend));
    RSLog(RSSTR("%p - %r(%r)"), friend, RSRenrenFriendGetName(friend), RSRenrenFriendGetAccountID(friend));
    return friend;
}

RSInline RSArrayRef friendsFromElements(RSArrayRef elements) {
    if (!elements || RSArrayGetCount(elements) == 0) return nil;
    RSArrayRef friends = RSPerformBlockConcurrentCopyResults(RSArrayGetCount(elements), ^RSTypeRef(RSIndex idx) {
        return analyzeFriend(RSArrayObjectAtIndex(elements, idx));
    });
    return RSAutorelease(friends);
}

RSInline RSUInteger friendNumberFromDocument(RSXMLDocumentRef document) {
    return RSStringIntegerValue(RSXMLNodeGetValue(RSArrayObjectAtIndex(RSXMLElementGetElementsForName(RSArrayObjectAtIndex(RSXMLElementGetElementsForName(RSArrayObjectAtIndex(RSXMLElementGetElementsForName(RSArrayObjectAtIndex(RSXMLElementGetElementsForName(RSArrayObjectAtIndex(RSXMLElementGetElementsForName(RSArrayObjectAtIndex(RSXMLElementGetElementsForName(RSArrayObjectAtIndex(RSXMLElementGetElementsForName(RSArrayObjectAtIndex(RSXMLElementGetElementsForName(RSArrayObjectAtIndex(RSXMLElementGetElementsForName(RSArrayObjectAtIndex(RSXMLElementGetElementsForName(RSArrayObjectAtIndex(RSXMLElementGetElementsForName(RSXMLDocumentGetRootElement(document), RSSTR("body")), 0), RSSTR("div")), 0), RSSTR("div")), 3), RSSTR("div")), 0), RSSTR("div")), 0), RSSTR("div")), 0), RSSTR("div")), 0), RSSTR("div")), 0), RSSTR("div")), 0), RSSTR("p")), 0), RSSTR("span")), 0)));
}

void dump(RSRenrenCoreAnalyzerRef analyzer) {
    RSUInteger pageNumber = 0;
    RSURLRef URL = urlForCoreDumpFriendListWithUserId(RSRenrenCoreAnalyzerGetUserId(analyzer), pageNumber);
    RSShow(URL);
    RSDataRef data = RSDataWithURL(URL);
//    RSDataWriteToFile(data, RSFileManagerStandardizingPath(RSSTR("~/Desktop/renren.html")), RSWriteFileAutomatically);
//    RSXMLDocumentRef document = RSXMLDocumentCreateWithContentOfFile(RSAllocatorDefault, RSFileManagerStandardizingPath(RSSTR("~/Desktop/renren.html")), RSXMLDocumentTidyHTML);
    RSXMLDocumentRef document = RSXMLDocumentCreateWithXMLData(RSAllocatorDefault, data, RSXMLDocumentTidyHTML);
    if (document) {
        RSShow(RSSTR("start"));
        RSAutoreleaseBlock(^{
            RSShow(RSNumberWithInteger(friendNumberFromDocument(document)));
            RSShow(friendsFromElements(friendsElementsFromDocument(document)));
        });
        RSRelease(document);
    }
}

#pragma mark -
#pragma mark Upload Image

RSExport void RSRenrenCoreAnalyzerUploadImage(RSRenrenCoreAnalyzerRef analyzer, RSDataRef imageData, RSStringRef description, RSDictionaryRef (^selectAlbum)(RSArrayRef albumList), void (^complete)(RSTypeRef photo, BOOL success)) {
    if (!analyzer || !imageData) return;
    __RSGenericValidInstance(analyzer, _RSRenrenCoreAnalyzerTypeID);
    RSShow(RSRenrenCoreAnalyzerGetToken(analyzer));
    RSAutoreleaseBlock(^{
        RSStringRef parent_formCallback = RSSTR("parent.formCallback");
        RSStringRef uploadFilePath = RSFileManagerStandardizingPath(RSSTR("~/Pictures/PJ/35a5ab5906177b4e79f731c7a09a976e.jpg"));
        RSMutableDictionaryRef requestProperty = RSAutorelease(RSDictionaryCreateMutable(RSAllocatorDefault, 0, RSDictionaryRSTypeContext));
        RSMutableArrayRef albumCollection = RSAutorelease(RSArrayCreateMutable(RSAllocatorDefault, 0));
        RSStringRef urlString = RSSTR("http://upload.renren.com/addphotoPlain.do");
        RSXMLDocumentRef document = RSAutorelease(RSXMLDocumentCreateWithXMLData(RSAllocatorDefault, RSDataWithURL(RSURLWithString(urlString)), RSXMLDocumentTidyHTML));
        RSXMLElementRef body = RSArrayObjectAtIndex(RSXMLElementGetElementsForName(RSXMLDocumentGetRootElement(document), RSSTR("body")), 0);
        if (!document) return;
        RSArrayRef subElements = RSXMLElementGetElementsForName(body, RSSTR("div"));
        if (!subElements) return;
        if (RSArrayGetCount(subElements) != 4) return;
        if (!RSEqual(RSXMLNodeGetValue(RSXMLElementGetAttributeForName(RSArrayObjectAtIndex(subElements, 3), RSSTR("id"))), RSSTR("container-for-buddylist"))) return;
        RSXMLElementRef div = RSArrayObjectAtIndex(subElements, 3);
        div = RSArrayObjectAtIndex(RSXMLElementGetElementsForName(RSArrayObjectAtIndex(RSXMLElementGetElementsForName(RSArrayObjectAtIndex(RSXMLElementGetElementsForName(RSArrayObjectAtIndex(RSXMLElementGetElementsForName(RSArrayObjectAtIndex(RSXMLElementGetElementsForName(RSArrayObjectAtIndex(RSXMLElementGetElementsForName(RSArrayObjectAtIndex(RSXMLElementGetElementsForName(RSArrayObjectAtIndex(RSXMLElementGetElementsForName(div, RSSTR("div")), 0), RSSTR("div")), 0), RSSTR("div")), 0), RSSTR("div")), 0), RSSTR("div")), 0), RSSTR("div")), 0), RSSTR("div")), 0), RSSTR("div")), 2);
        div = RSArrayObjectAtIndex(RSXMLElementGetElementsForName(div, RSSTR("div")), 1);
        if (!RSEqual(RSXMLNodeGetValue(RSXMLElementGetAttributeForName(div, RSSTR("id"))), RSSTR("single-column"))) return;
        RSXMLElementRef form = RSArrayObjectAtIndex(RSXMLElementGetElementsForName(div, RSSTR("form")), 0);
        if (!RSEqual(RSXMLNodeGetValue(RSXMLElementGetAttributeForName(form, RSSTR("target"))), RSSTR("uploadPlainIframe"))) return;
        RSDictionarySetValue(requestProperty, RSSTR("method"), RSXMLNodeGetValue(RSXMLElementGetAttributeForName(form, RSSTR("method"))));
        RSDictionarySetValue(requestProperty, RSSTR("action"), RSXMLNodeGetValue(RSXMLElementGetAttributeForName(form, RSSTR("action"))));
        RSDictionarySetValue(requestProperty, RSSTR("enctype"), RSXMLNodeGetValue(RSXMLElementGetAttributeForName(form, RSSTR("enctype"))));
        RSShow(requestProperty);
        RSArrayRef albumlist = RSXMLElementGetElementsForName(RSArrayObjectAtIndex(RSXMLElementGetElementsForName(RSArrayObjectAtIndex(RSXMLElementGetElementsForName(RSArrayObjectAtIndex(RSXMLElementGetElementsForName(form, RSSTR("p")), 0), RSSTR("span")), 0), RSSTR("select")), 0), RSSTR("option"));
        RSArrayApplyBlock(albumlist, RSMakeRange(0, RSArrayGetCount(albumlist)), ^(const void *value, RSUInteger idx, BOOL *isStop) {
            RSXMLElementRef album = value;
            RSMutableDictionaryRef dict = RSAutorelease(RSMutableCopy(RSAllocatorDefault, RSAutorelease(RSDictionaryCreateWithObjectsAndOKeys(RSAllocatorDefault, RSXMLNodeGetValue(RSXMLElementGetAttributeForName(album, RSSTR("value"))), RSSTR("id"), RSXMLNodeGetValue((RSXMLNodeRef)album), RSSTR("name"), NULL))));
            if (RSEqual(RSXMLNodeGetValue(RSXMLElementGetAttributeForName(album, RSSTR("disabled"))), RSSTR("disabled")))
                RSDictionarySetValue(dict, RSSTR("disabled"), RSBooleanTrue);
            RSArrayAddObject(albumCollection, dict);
        });
        RSDictionaryRef album = nil;
        if (selectAlbum) album = selectAlbum(albumCollection);
        else album = RSArrayGetCount(albumCollection) ? RSArrayObjectAtIndex(albumCollection, 0) : nil;
        if (!album) return;
        if (RSNotFound == RSArrayIndexOfObject(albumCollection, album)) return;
        RSStringRef albumId = RSDictionaryGetValue(album, RSSTR("id"));
        RSArrayRef files = RSXMLElementGetElementsForName(RSArrayObjectAtIndex(RSXMLElementGetElementsForName(RSArrayObjectAtIndex(RSXMLElementGetElementsForName(form, RSSTR("div")), 0), RSSTR("div")), 0), RSSTR("p"));
        RSMutableArrayRef filesCollection = RSAutorelease(RSArrayCreateMutable(RSAllocatorDefault, 0));
        RSArrayApplyBlock(files, RSMakeRange(0, RSArrayGetCount(files)), ^(const void *value, RSUInteger idx, BOOL *isStop) {
            RSXMLElementRef file = value;
            RSArrayRef inputs = RSXMLElementGetElementsForName(file, RSSTR("input"));
            if (RSArrayGetCount(inputs)) {
                RSXMLElementRef input = RSArrayObjectAtIndex(inputs, 0);
                if (RSEqual(RSXMLNodeGetValue(RSXMLElementGetAttributeForName(input, RSSTR("input"))), RSSTR("file"))) {
                    RSArrayAddObject(filesCollection, RSAutorelease(RSDictionaryCreateWithObjectsAndOKeys(RSAllocatorDefault, RSXMLNodeGetValue(RSXMLElementGetAttributeForName(input, RSSTR("name"))), RSSTR("name"), RSSTR(""), RSSTR("filename"), NULL)));
                }
            }
        });
        if (RSArrayGetCount(filesCollection)) {
            RSDictionarySetValue((RSMutableDictionaryRef)RSArrayObjectAtIndex(filesCollection, 0), RSSTR("filename"), uploadFilePath);
        } else {
            RSArrayAddObject(filesCollection, RSAutorelease(RSDictionaryCreateWithObjectsAndOKeys(RSAllocatorDefault, RSXMLNodeGetValue(RSXMLElementGetAttributeForName(RSArrayObjectAtIndex(RSXMLElementGetElementsForName(RSArrayObjectAtIndex(files, 0), RSSTR("input")), 0), RSSTR("name"))), RSSTR("name"), uploadFilePath, RSSTR("filename"), NULL)));
        }
        RSMutableURLRequestRef request = RSAutorelease(RSURLRequestCreateMutable(RSAllocatorDefault, RSURLWithString(RSDictionaryGetValue(requestProperty, RSSTR("action")))));
        RSStringRef boundary = RSSTR("----WebKitFormBoundaryRJUGdi7326XY1u1b");
        RSMutableDataRef postData = RSAutorelease(RSDataCreateMutable(RSAllocatorDefault, 0));
        RSDataAppend(postData, RSDataWithString(RSStringWithFormat(RSSTR("--%r\r\nContent-Disposition: form-data; name=\"%r\"\r\n\r\n%r\r\n"), boundary, RSSTR("id"), albumId), RSStringEncodingUTF8));
        RSArrayApplyBlock(filesCollection, RSMakeRange(0, RSArrayGetCount(filesCollection)), ^(const void *value, RSUInteger idx, BOOL *isStop) {
            RSDictionaryRef file = value;
            if (RSStringGetLength(RSDictionaryGetValue(file, RSSTR("filename")))) {
                RSDataAppend(postData, RSDataWithString(RSStringWithFormat(RSSTR("--%r\r\n"), boundary), RSStringEncodingUTF8));
                RSDataAppend(postData, RSDataWithString(RSStringWithFormat(RSSTR("Content-Disposition: form-data; name=\"%r\"; filename=\"%r\"\r\n"), RSDictionaryGetValue(file, RSSTR("name")), RSFileManagerFileFullName(RSFileManagerGetDefault(), uploadFilePath)), RSStringEncodingUTF8));
                RSDataAppend(postData, RSDataWithString(RSSTR("Content-Type: image/jpeg\r\n\r\n"), RSStringEncodingUTF8));
                RSDataAppend(postData, imageData);
                RSDataAppend(postData, RSDataWithString(RSSTR("\r\n"), RSStringEncodingUTF8));
            }
        });
        RSDataAppend(postData, RSDataWithString(RSStringWithFormat(RSSTR("--%r\r\nContent-Disposition: form-data; name=\"privacyParams\"\r\n\r\n%r\r\n"), boundary, RSSTR("{\"sourceControl\":99}")), RSStringEncodingUTF8));
        RSDataAppend(postData, RSDataWithString(RSStringWithFormat(RSSTR("--%r\r\nContent-Disposition: form-data; name=\"callback\"\r\n\r\n%r\r\n"), boundary, RSSTR("parent.formCallback")), RSStringEncodingUTF8));
        RSDataAppend(postData, RSDataWithString(RSStringWithFormat(RSSTR("--%r--\r\r\n"), boundary), RSStringEncodingUTF8));
        RSStringRef postLength = RSStringWithFormat(RSSTR("%ld"), RSDataGetLength(postData));
        RSURLRequestSetHeaderFieldValue(request, RSSTR("Accept"), RSSTR("text/html,application/xhtml+xml,application/xml;q=0.9,*/*;q=0.8"));
        RSURLRequestSetHeaderFieldValue(request, RSSTR("Referer"), RSSTR("http://upload.renren.com/addphotoPlain.do"));
        RSURLRequestSetHeaderFieldValue(request, RSSTR("Origin"), RSSTR("http://upload.renren.com"));
        RSURLRequestSetHeaderFieldValue(request, RSSTR("User-Agent"), RSSTR("Mozilla/5.0 (Macintosh; Intel Mac OS X 10_9) AppleWebKit/537.71 (KHTML, like Gecko) Version/7.0 Safari/537.71"));
        RSURLRequestSetHeaderFieldValue(request, RSSTR("DNT"), RSSTR("1"));
        RSURLRequestSetHeaderFieldValue(request, RSSTR("Content-Type"), RSStringWithFormat(RSSTR("multipart/form-data; boundary=%r"), boundary));
        RSURLRequestSetHeaderFieldValue(request, RSSTR("Content-Length"), postLength);
        RSURLRequestSetHTTPMethod(request, RSSTR("POST"));
        RSURLRequestSetHTTPBody(request, postData);
        RSURLResponseRef response = nil;
        RSErrorRef error = nil;
        RSShow(request);
        RSShow(RSStringWithData(RSURLRequestGetHTTPBody(request), RSStringEncodingUTF8));
        const char * ptr __unused = RSStringGetUTF8String(RSStringWithData(RSURLRequestGetHTTPBody(request), RSStringEncodingUTF8));
        RSDataRef data = RSURLConnectionSendSynchronousRequest(request, &response, &error);
        if (error) RSShow(error);
        RSLog(RSSTR("status code = %ld"), RSURLResponseGetStatusCode(response));
        RSStringRef des = data ? RSStringWithData(data, RSStringEncodingUTF8) : RSSTR("");
        RSShow(des);
        if (200 != RSURLResponseGetStatusCode(response) && 100 != RSURLResponseGetStatusCode(response)) return;
        RSXMLDocumentRef jumpScript = RSAutorelease(RSXMLDocumentCreateWithXMLData(RSAllocatorDefault, data, RSXMLDocumentTidyHTML));
        if (!jumpScript) return;
        RSXMLElementRef head = RSArrayObjectAtIndex(RSXMLElementGetElementsForName(RSXMLDocumentGetRootElement(jumpScript), RSSTR("head")), 0);
        if (!head) return;
        RSXMLElementRef script = RSArrayObjectAtIndex(RSXMLElementGetElementsForName(head, RSSTR("script")), 0);
        if (!RSEqual(RSXMLNodeGetValue(RSXMLElementGetAttributeForName(script, RSSTR("type"))), RSSTR("text/javascript"))) return;
        if (10 < RSStringRangeOfString(RSXMLNodeGetValue((RSXMLNodeRef)script), RSSTR("document.domain = \"renren.com\";")).location) return;
        RSStringRef value = RSXMLNodeGetValue((RSXMLNodeRef)script);
        RSRange range = RSStringRangeOfString(value, RSStringWithFormat(RSSTR("%r("), parent_formCallback));
        value = RSStringWithSubstring(value, RSMakeRange(range.location + range.length, RSStringGetLength(value) - range.location - range.length));
        range = RSStringRangeOfString(value, RSSTR(");"));
        value = RSStringWithSubstring(value, RSMakeRange(0, range.location));
        RSTypeRef dict = RSAutorelease(RSJSONSerializationCreateWithJSONData(RSAllocatorDefault, RSDataWithString(value, RSStringEncodingUTF8)));
        if (!dict) return;
        RSMutableDictionaryRef post = RSAutorelease(RSDictionaryCreateMutable(RSAllocatorDefault, 0, RSDictionaryRSTypeContext));
        RSURLRef saveURL = RSURLWithString(RSStringWithFormat(RSSTR("http://upload.renren.com/upload/%r/photo/save"), RSRenrenCoreAnalyzerGetUserId(analyzer)));
        RSDictionarySetValue(post, RSSTR("flag"), RSSTR("0/"));
        RSDictionarySetValue(post, RSSTR("album.id"), albumId);
        RSDictionarySetValue(post, RSSTR("album.description"), description);
        RSDictionarySetValue(post, RSSTR("privacyParams"), RSSTR("{\"sourceControl\":99}"));
        RSDictionarySetValue(post, RSSTR("photos"), RSDictionaryGetValue(dict, RSSTR("files")));
        RSMutableURLRequestRef saveRequest = RSAutorelease(RSURLRequestCreateMutable(RSAllocatorDefault, saveURL));
        RSURLRequestSetHeaderFieldValue(saveRequest, RSSTR("Accept"), RSSTR("text/html,application/xhtml+xml,application/xml;q=0.9,image/webp,*/*;q=0.8"));
        RSURLRequestSetHeaderFieldValue(saveRequest, RSSTR("Content-Type"), RSSTR("application/x-www-form-urlencoded"));
        RSURLRequestSetHTTPMethod(saveRequest, RSSTR("POST"));
        RSDataRef savePostData = RSDataWithString(RSStringURLEncode(post), RSStringEncodingUTF8);
        RSURLRequestSetHTTPBody(saveRequest, savePostData);
        RSURLRequestSetHeaderFieldValue(saveRequest, RSSTR("Content-Length"), RSStringWithFormat(RSSTR("%ld"), RSDataGetLength(savePostData)));
        RSURLRequestSetHeaderFieldValue(saveRequest, RSSTR("Origin"), RSSTR("http://upload.renren.com"));
        RSURLRequestSetHeaderFieldValue(saveRequest, RSSTR("Referer"), RSSTR("http://upload.renren.com/addphotoPlain.do"));
        RSURLRequestSetHeaderFieldValue(saveRequest, RSSTR("Connection"), RSSTR("keep-alive"));
        data = RSURLConnectionSendSynchronousRequest(saveRequest, &response, &error);
        if (RSURLResponseGetStatusCode(response) != 200 && RSURLResponseGetStatusCode(response) != 100) return;
        RSShow(response);
        RSXMLDocumentRef publishDocument = RSAutorelease(RSXMLDocumentCreateWithXMLData(RSAllocatorDefault, data, RSXMLDocumentTidyHTML));
        if (!publishDocument) return;
        body = RSArrayObjectAtIndex(RSXMLElementGetElementsForName(RSXMLDocumentGetRootElement(publishDocument), RSSTR("body")), 0);
        if (!RSEqual(RSXMLNodeGetValue(RSXMLElementGetAttributeForName(body, RSSTR("id"))), RSSTR("pageAlbum"))) return;
        div = RSArrayObjectAtIndex(RSXMLElementGetElementsForName(RSArrayObjectAtIndex(RSXMLElementGetElementsForName(RSArrayObjectAtIndex(RSXMLElementGetElementsForName(RSArrayObjectAtIndex(RSXMLElementGetElementsForName(RSArrayObjectAtIndex(RSXMLElementGetElementsForName(RSArrayObjectAtIndex(RSXMLElementGetElementsForName(RSArrayObjectAtIndex(RSXMLElementGetElementsForName(body, RSSTR("div")), 0), RSSTR("div")), 3), RSSTR("div")), 0), RSSTR("div")), 0), RSSTR("div")), 0), RSSTR("div")), 0), RSSTR("div")), 0);
        
        if (!RSEqual(RSXMLNodeGetValue(RSXMLElementGetAttributeForName(div, RSSTR("id"))), RSSTR("content"))) return;
        form = RSArrayObjectAtIndex(RSXMLElementGetElementsForName(div, RSSTR("form")), 0);
        if (!RSEqual(RSXMLNodeGetValue(RSXMLElementGetAttributeForName(form, RSSTR("id"))), RSSTR("albumEditForm"))) return;
        div = RSArrayObjectAtIndex(RSXMLElementGetElementsForName(RSArrayObjectAtIndex(RSXMLElementGetElementsForName(RSArrayObjectAtIndex(RSXMLElementGetElementsForName(RSArrayObjectAtIndex(RSXMLElementGetElementsForName(RSArrayObjectAtIndex(RSXMLElementGetElementsForName(form, RSSTR("div")), 0), RSSTR("div")), 0), RSSTR("div")), 1), RSSTR("div")), 0), RSSTR("div")), 0);
        RSArrayRef inputs = RSXMLElementGetElementsForName(div, RSSTR("input"));
        RSTypeRef aid = RSXMLNodeGetValue(RSXMLElementGetAttributeForName(RSArrayObjectAtIndex(inputs, 0), RSSTR("value")));
        if (!aid) return;
        RSRenrenCoreAnalyzerPublicPhoto(analyzer, albumId, aid, description, complete);
        // wait for public
    });
}

RSExport void RSRenrenCoreAnalyzerPublicPhoto(RSRenrenCoreAnalyzerRef analyzer, RSStringRef albumId, RSStringRef photoId, RSStringRef description, void (^complete)(RSTypeRef photoId, BOOL success)) {
    if (!analyzer || !albumId || !photoId) return;
    __RSGenericValidInstance(analyzer, _RSRenrenCoreAnalyzerTypeID);
    RSAutoreleaseBlock(^{
        RSStringRef publishString = RSStringWithFormat(RSSTR("http://upload.renren.com/upload/%r/album-%r/editPhotoList"), RSRenrenCoreAnalyzerGetUserId(analyzer), albumId);
        RSShow(publishString);
        RSURLRef publishURL = RSURLWithString(publishString);
        RSMutableURLRequestRef publishRequest = RSAutorelease(RSURLRequestCreateMutable(RSAllocatorDefault, publishURL));
        RSStringRef boundary = RSSTR("----WebKitFormBoundaryKueLOdoAgrkoekBu");
        RSMutableDataRef postData = RSAutorelease(RSDataCreateMutable(RSAllocatorDefault, 0));
        RSStringRef standardFormat = RSSTR("--%r\r\nContent-Disposition: form-data; name=\"%r\"\r\n\r\n%r\r\n");
        void (^appendData)(RSStringRef key, RSStringRef value) = ^(RSStringRef key, RSStringRef value) {
            RSDataAppend(postData, RSDataWithString(RSStringWithFormat(standardFormat, boundary, key, value), RSStringEncodingUTF8));
        };
        appendData(RSSTR("id"), photoId);
        appendData(RSSTR("publishFeed"), RSSTR("true"));
        appendData(RSSTR("title"), description);
        appendData(RSSTR("requestToken"), RSDictionaryGetValue(RSRenrenCoreAnalyzerGetToken(analyzer), RSSTR("requestToken")));
        appendData(RSSTR("_rtk"), RSDictionaryGetValue(RSRenrenCoreAnalyzerGetToken(analyzer), RSSTR("_rtk")));
        RSDataAppend(postData, RSDataWithString(RSStringWithFormat(RSSTR("--%r--\r\n"), boundary), RSStringEncodingUTF8));
        RSDataRef data = nil;
        RSURLRequestSetHTTPBody(publishRequest, postData);
        RSURLRequestSetHeaderFieldValue(publishRequest, RSSTR("Connection"), RSSTR("keep-alive"));
        RSURLRequestSetHeaderFieldValue(publishRequest, RSSTR("Origin"), RSSTR("http://upload.renren.com"));
        RSURLRequestSetHeaderFieldValue(publishRequest, RSSTR("User-Agent"), RSSTR("Mozilla/5.0 (Macintosh; Intel Mac OS X 10_9_0) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/30.0.1599.69 Safari/537.36"));
        RSURLRequestSetHeaderFieldValue(publishRequest, RSSTR("Content-Type"), RSStringWithFormat(RSSTR("multipart/form-data; boundary=%r"), boundary));
        RSURLRequestSetHeaderFieldValue(publishRequest, RSSTR("Content-Length"), RSStringWithFormat(RSSTR("%ld"), RSDataGetLength(postData)));
        RSURLRequestSetHTTPMethod(publishRequest, RSSTR("POST"));
        RSURLResponseRef response = nil;
        RSErrorRef error = nil;
        data = RSURLConnectionSendSynchronousRequest(publishRequest, &response, &error);
        complete(photoId, error == nil && RSURLResponseGetStatusCode(response) == 200);
    });
}

RSExport void RSRenrenCoreAnalyzerPublicStatus(RSRenrenCoreAnalyzerRef analyzer, RSStringRef content) {
    if (!analyzer || !content) return;
    __RSGenericValidInstance(analyzer, _RSRenrenCoreAnalyzerTypeID);
    RSMutableDictionaryRef token = RSAutorelease(RSMutableCopy(RSAllocatorSystemDefault, RSRenrenCoreAnalyzerGetToken(analyzer)));
    RSDictionarySetValue(token, RSSTR("content"), content);
    RSDictionarySetValue(token, RSSTR("channel"), RSSTR("renren"));
    RSDictionarySetValue(token, RSSTR("hostid"), RSRenrenCoreAnalyzerGetUserId(analyzer));
    RSMutableURLRequestRef request = RSAutorelease(RSURLRequestCreateMutable(RSAllocatorSystemDefault, RSURLWithString(RSStringWithFormat(RSSTR("http://shell.renren.com/%r/status"), RSRenrenCoreAnalyzerGetUserId(analyzer)))));
    RSDataRef data = RSDataWithString(RSStringURLEncode(token), RSStringEncodingUTF8);
    RSURLRequestSetHTTPBody(request, data);
    RSURLRequestSetHTTPMethod(request, RSSTR("POST"));
    RSURLRequestSetHeaderFieldValue(request, RSSTR("Content-Length"), RSStringWithFormat(RSSTR("%r"), RSNumberWithInteger(RSDataGetLength(data))));
    RSURLRequestSetHeaderFieldValue(request, RSSTR("Content-Type"), RSSTR("application/x-www-form-urlencoded charset=utf-8"));
    RSURLResponseRef response = nil;
    RSErrorRef error = nil;
    data = RSURLConnectionSendSynchronousRequest(request, &response, &error);
    if (error) RSShow(error);
}
