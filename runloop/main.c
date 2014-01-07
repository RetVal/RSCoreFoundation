//
//  main.c
//  runloop
//
//  Created by Closure on 11/10/13.
//  Copyright (c) 2013 RetVal. All rights reserved.
//

#include <RSCoreFoundation/RSCoreFoundation.h>
#include "RSRenrenFriend.h"

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

//- (id)_analyzeFriendForAvatar:(NSXMLElement *)avatar
//{
//    NSMutableDictionary *avatarInfo = [[NSMutableDictionary alloc] initWithCapacity:2];
//    NSXMLElement *a = [[avatar elementsForName:@"a"] lastObject];
//    NSXMLNode *href = [a attributeForName:@"href"];
//
//    [avatarInfo setObject:[href objectValue] forKey:_kCAHomePageLinkKey];
//
//    NSXMLElement *img = [[a elementsForName:@"img"] lastObject];
//    NSXMLNode *src = [img attributeForName:@"src"];
//
//    [avatarInfo setObject:[src objectValue] forKey:_kCAHeadImageLinkKey];
//    return avatarInfo;
//}

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

//- (id)_analyzeFriendForInfo:(NSXMLElement *)info
//{
//    NSMutableDictionary *infoInfo = [[NSMutableDictionary alloc] initWithCapacity:3];
//    NSXMLElement *dl = [[info elementsForName:@"dl"] lastObject];
//    NSXMLElement *dd = [dl elementsForName:@"dd"][0];
//    NSXMLElement *dd_location = [[dl elementsForName:@"dd"] lastObject];
//
//    NSXMLElement *a = [[dd elementsForName:@"a"] lastObject];
//    NSXMLNode *href __unused = [a attributeForName:@"href"];
//
//    if (dd_location && NO == [dd isEqual:dd_location])
//    {
//        [infoInfo setObject:[a objectValue] forKey:_kCANameKey];
//        [infoInfo setObject:[[dd_location objectValue] stringByTrimmingCharactersInSet:[NSCharacterSet newlineCharacterSet]] forKey:_kCASchoolKey];
//    }
//    else
//    {
//        [infoInfo setObject:[a objectValue] forKey:_kCANameKey];
//    }
//
//    return infoInfo;
//}

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

//- (NSString *)_analyzeFriendForAccountId:(RSBaseModel *)model
//{
//    NSString *homelink = [[model homePageURL] absoluteString];
//    NSString *userId = nil;
//    if ([homelink hasPrefix:@"http://www.renren.com/profile.do?id="])
//        userId = [[homelink mutableCopy] stringByReplacingOccurrencesOfString:@"http://www.renren.com/profile.do?id=" withString:@""];
//        else
//            userId = nil;
//            return userId;
//}
RSInline RSStringRef analyzeFriendForAccountID(RSRenrenFriendRef friend) {
    
    RSStringRef homelink = RSURLGetString(RSAutorelease(RSURLCopyAbsoluteURL(RSRenrenFriendGetHomePageURL(friend))));
    RSStringRef userId = nil;
    if (RSStringHasPrefix(homelink, RSSTR("http://www.renren.com/profile.do?id=")))
        userId = RSStringByReplacingOccurrencesOfString(RSAutorelease(RSMutableCopy(RSAllocatorDefault, homelink)), RSSTR("http://www.renren.com/profile.do?id="), RSSTR(""));
    else
        userId = RSSTR("0");
    return userId;
}

//- (id)analyzeFriend:(NSXMLElement *)friendInfo
//{
//    if (!friendInfo) return nil;
//    NSXMLElement *avatar = [[friendInfo elementsForName:@"p"] lastObject];
//    NSXMLElement *info = [[friendInfo elementsForName:@"div"] lastObject];
//    NSXMLElement *actions = [[friendInfo elementsForName:@"ul"] lastObject];
//    if (!avatar || !info || !actions) return nil;
//    NSDictionary *avatarResult = [self _analyzeFriendForAvatar:avatar];
//    NSDictionary *infoResult = [self _analyzeFriendForInfo:info];
//
//    RSBaseModel *model = [[RSBaseModel alloc] init];
//    [model setHomePageURL:[NSURL URLWithString:[avatarResult objectForKey:_kCAHomePageLinkKey]]];
//    [model setImageURL:[NSURL URLWithString:[avatarResult objectForKey:_kCAHeadImageLinkKey]]];
//    [model setName:[infoResult objectForKey:_kCANameKey]];
//    [model setSchoolName:[infoResult objectForKey:_kCASchoolKey]];
//    [model setAccount:[self _analyzeFriendForAccountId:model]];
//    return model;
//}

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
//    sleep(2);
    return friend;
}

RSInline RSArrayRef friendsFromElements(RSArrayRef elements) {
    if (!elements || RSArrayGetCount(elements) == 0) return nil;
    RSAbsoluteTime s = 0, e = 0;
    RSShow(RSSTR("1  ---------------------------------------------------"));
    s = RSAbsoluteTimeGetCurrent();
    RSArrayRef friends = RSPerformBlockConcurrentCopyResults(RSArrayGetCount(elements), ^RSTypeRef(RSIndex idx) {
        return analyzeFriend(RSArrayObjectAtIndex(elements, idx));
    });
    e = RSAbsoluteTimeGetCurrent();
    RSLog(RSSTR("RSPerformBlockConcurrent cost %f"), e - s);
    return RSAutorelease(friends);
}

RSInline RSUInteger friendNumberFromDocument(RSXMLDocumentRef document) {
    return RSStringIntegerValue(RSXMLNodeGetValue(RSArrayObjectAtIndex(RSXMLElementGetElementsForName(RSArrayObjectAtIndex(RSXMLElementGetElementsForName(RSArrayObjectAtIndex(RSXMLElementGetElementsForName(RSArrayObjectAtIndex(RSXMLElementGetElementsForName(RSArrayObjectAtIndex(RSXMLElementGetElementsForName(RSArrayObjectAtIndex(RSXMLElementGetElementsForName(RSArrayObjectAtIndex(RSXMLElementGetElementsForName(RSArrayObjectAtIndex(RSXMLElementGetElementsForName(RSArrayObjectAtIndex(RSXMLElementGetElementsForName(RSArrayObjectAtIndex(RSXMLElementGetElementsForName(RSArrayObjectAtIndex(RSXMLElementGetElementsForName(RSXMLDocumentGetRootElement(document), RSSTR("body")), 0), RSSTR("div")), 0), RSSTR("div")), 3), RSSTR("div")), 0), RSSTR("div")), 0), RSSTR("div")), 0), RSSTR("div")), 0), RSSTR("div")), 0), RSSTR("div")), 0), RSSTR("p")), 0), RSSTR("span")), 0)));
}

void dump() {
//    RSUInteger pageNumber = 0;
//    RSURLRef URL = urlForCoreDumpFriendListWithUserId(RSRenrenCoreAnalyzerGetUserId(analyzer), pageNumber);
//    RSShow(URL);
    //    RSDataRef data = RSDataWithURL(URL);
    //    RSDataWriteToFile(data, RSFileManagerStandardizingPath(RSSTR("~/Desktop/renren.html")), RSWriteFileAutomatically);
    //    RSXMLDocumentRef document = RSXMLDocumentCreateWithXMLData(RSAllocatorDefault, data, RSXMLDocumentTidyHTML);
    RSXMLDocumentRef document = RSXMLDocumentCreateWithContentOfFile(RSAllocatorDefault, RSFileManagerStandardizingPath(RSSTR("~/Desktop/renren.html")), RSXMLDocumentTidyHTML);
    if (document) {
        RSShow(RSSTR("start"));
        RSAutoreleaseBlock(^{
            RSShow(RSNumberWithInteger(friendNumberFromDocument(document)));
            RSShow(friendsFromElements(friendsElementsFromDocument(document)));
        });
        RSRelease(document);
    }
}

typedef struct RSCoreAnalyzer {
    RSURLConnectionRef connection;
    RSMutableDataRef reiceveData;
    RSURLResponseRef response;
    RSDictionaryRef token;
}*RSCoreAnalyzer;

RSStringRef RSStringURLEncode(RSDictionaryRef dict) {
    RSMutableArrayRef array = RSArrayCreateMutable(RSAllocatorDefault, RSDictionaryGetCount(dict));
    RSDictionaryApplyBlock(dict, ^(const void *key, const void *value, BOOL *stop) {
        RSArrayAddObject(array, RSStringWithFormat(RSSTR("%r=%r"), RSAutorelease(RSURLCreateStringByAddingPercentEscapes(RSAllocatorDefault, key, nil, RSSTR("!*'();:@&=+$,/?%#[]"), RSStringEncodingUTF8)), RSAutorelease(RSURLCreateStringByAddingPercentEscapes(RSAllocatorDefault, value, nil, RSSTR("!*'();:@&=+$,/?%#[]"), RSStringEncodingUTF8))));
    });
    RSStringRef str = RSStringCreateByCombiningStrings(RSAllocatorDefault, array, RSSTR("&"));
    RSRelease(array);
    return RSAutorelease(str);
}

static RSURLRequestRef remoteUpdateUser(RSStringRef uid, RSUInteger count, RSCoreAnalyzer analyzer) {
    if (!analyzer) return nil;
    RSTypeRef token = analyzer->token;
    RSMutableURLRequestRef request = RSURLRequestCreateMutable(RSAllocatorDefault, RSURLWithString(RSStringWithFormat(RSSTR("http://www.renren.com/moreminifeed.do?p=%r&u=%r&requestToken=%r&_rtk=%r"), RSNumberWithUnsignedInteger(count), uid, RSDictionaryGetValue(token, RSSTR("requestToken")), RSDictionaryGetValue(token, RSSTR("_rtk")))));
    return RSAutorelease(request);
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

static RSDictionaryRef initModel(RSStringRef content, RSStringRef mode) {
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

static void modelAction(RSDictionaryRef model) {
    RSStringRef urlFormat = RSAutorelease(RSURLCreateStringByAddingPercentEscapes(RSAllocatorDefault,
                                                                                  RSStringWithFormat(RSSTR("http://like.renren.com/%r?gid=%r_%r&uid=%r&owner=%r&type=%r&name=%r"),
                                                                                                     RSDictionaryGetValue(model, RSSTR("mode")),
                                                                                                     RSArrayObjectAtIndex(RSDictionaryGetValue(model, RSSTR("container")), 0),
                                                                                                     RSArrayObjectAtIndex(RSDictionaryGetValue(model, RSSTR("container")), 1),
                                                                                                     RSArrayObjectAtIndex(RSDictionaryGetValue(model, RSSTR("container")), 2),
                                                                                                     RSArrayObjectAtIndex(RSDictionaryGetValue(model, RSSTR("container")), 3),
                                                                                                     RSNumberWithInt(3),
                                                                                                     RSArrayObjectAtIndex(RSDictionaryGetValue(model, RSSTR("container")), 4)),
                                                                                  nil,
                                                                                  nil,
                                                                                  RSStringEncodingUTF8));
    RSURLResponseRef response = nil;
    RSDataRef data = nil;
    RSErrorRef error = nil;
    RSShow(urlFormat);
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

static RSArrayRef model(RSStringRef content, RSStringRef mode, RSCoreAnalyzer analyzer) {
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
        RSArrayAddObject(models, initModel(value, mode));
    });
    RSRelease(results);
    return RSAutorelease(models);
}

RSDataRef RSDataWithURL(RSURLRef URL) {
    return RSURLConnectionSendSynchronousRequest(RSURLRequestWithURL(URL), nil, nil);
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

static RSStringRef uid(RSStringRef info) {
    RSStringRef uid = RSStringByReplacingOccurrencesOfString(info, RSSTR("http://www.renren.com/"), RSSTR(""));
    RSShow(uid);
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

static struct RSCoreAnalyzer analyzer = {0};

static void RSURLConnectionWillStartConnection_CoreAnalyzer(RSURLConnectionRef connection, RSURLRequestRef willSendRequest, RSURLResponseRef redirectResponse) {
    ;
}

static void RSURLConnectionDidReceiveResponse_CoreAnalyzer(RSURLConnectionRef connection, RSURLResponseRef response) {
    ((RSCoreAnalyzer)RSURLConnectionGetContext(connection))->response = response;
}

static void RSURLConnectionDidReceiveData_CoreAnalyzer(RSURLConnectionRef connection, RSDataRef data){
    RSDataAppend((RSMutableDataRef)(((RSCoreAnalyzer)RSURLConnectionGetContext(connection))->reiceveData), data);
}

static void RSURLConnectionDidSendBodyData_CoreAnalyzer(RSURLConnectionRef connection, RSDataRef data, RSUInteger totalBytesWritten, RSUInteger totalBytesExpectedToWrite){
    
}

static void RSURLConnectionDidFailWithError_CoreAnalyzer(RSURLConnectionRef connection, RSErrorRef error) {
    RSShow(RSSTR("login failed"));
    RSShow(analyzer.response);
    RSRunLoopStop(RSRunLoopGetMain());
}

static void RSURLConnectionDidFinishLoading_CoreAnalyzer(RSURLConnectionRef connection){
    RSShow(RSSTR("finished loading"));
    if (RSStringRangeOfString(RSURLGetString(RSURLResponseGetURL(((RSCoreAnalyzer)RSURLConnectionGetContext(connection))->response)), RSSTR("failCode=")).length > 0) {
        RSURLConnectionDidFailWithError_CoreAnalyzer(connection, nil);
        return;
    }
    RSShow(analyzer.token = RSRetain(token(uid(RSURLGetString(RSURLResponseGetURL(((RSCoreAnalyzer)RSURLConnectionGetContext(connection))->response))))));
    RSPerformBlockOnMainThread(^{
        __block RSUInteger begin = 0;
        __block RSUInteger limit = 0;
        __block RSUInteger count = 5;
        __block BOOL shouldContinue = YES;
        RSMutableArrayRef models = RSArrayCreateMutable(RSAllocatorDefault, 0);
        RSStringRef userId = RSSTR("359364053");
        while (shouldContinue) {
            if (count <= begin) {
                shouldContinue = NO;
                break;
            }
            RSAutoreleaseBlock(^{
                RSURLResponseRef response = nil;
                RSDataRef data = nil;
                RSErrorRef error = nil;
                data = RSURLConnectionSendSynchronousRequest(remoteUpdateUser(userId, limit, &analyzer), &response, &error);
                if (RSURLResponseGetStatusCode(response) == 200)
                    RSArrayAddObjects(models, model(RSStringWithData(data, RSStringEncodingUTF8), RSSTR("addlike"), &analyzer));
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
                modelAction(value);
                sleep(2);
            });
            RSShow(RSSTR("end"));
        }
        RSRelease(models);
        RSRunLoopStop(RSRunLoopGetMain());
    });
}

void RSCoreAnalyzerCreate(RSCoreAnalyzer analyzer, RSStringRef userName, RSStringRef password) {
    if (!analyzer) return;
    RSStringRef baseURLString = RSSTR("http://www.renren.com/PLogin.do");
    RSMutableURLRequestRef request = RSAutorelease(RSURLRequestCreateMutable(RSAllocatorDefault, RSURLWithString(baseURLString)));
    RSStringRef post = __buildURLString(RSSTR("http://www.renren.com/SysHome.do"), RSSTR("renren.com"), userName, password);
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
    analyzer->reiceveData = RSDataCreateMutable(RSAllocatorDefault, 0);
    analyzer->connection = RSURLConnectionCreate(RSAllocatorDefault, request, analyzer, &delegate);
    
    return;
}

void RSCoreAnalyzerStart(RSCoreAnalyzer analyzer) {
    RSURLConnectionStartOperation(analyzer->connection);
}

void RSCoreAnalyzerRelease(RSCoreAnalyzer analyzer) {
    RSRelease(analyzer->reiceveData);
    RSRelease(analyzer->connection);
    RSRelease(analyzer->token);
}

int main (int argc, char **argv)
{
    dump();
    return 0;
    RSXMLDocumentRef document = RSXMLDocumentCreateWithContentOfFile(RSAllocatorDefault, RSFileManagerStandardizingPath(RSSTR("~/Desktop/renren.html")), RSXMLDocumentTidyHTML);
    
    RSRelease(document);
    return 0;
    if (argc != 3) return 0;
    RSCoreAnalyzerCreate(&analyzer, RSStringWithUTF8String(argv[1]) /*email*/, RSStringWithUTF8String(argv[2])/*password*/);
    RSCoreAnalyzerStart(&analyzer);
    RSRunLoopRun();
    RSCoreAnalyzerRelease(&analyzer);
    sleep(1);
    return 0;
}
