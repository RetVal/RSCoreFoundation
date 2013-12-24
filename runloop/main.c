//
//  main.c
//  runloop
//
//  Created by Closure on 11/10/13.
//  Copyright (c) 2013 RetVal. All rights reserved.
//

#include <RSCoreFoundation/RSCoreFoundation.h>
#include <CoreFoundation/CoreFoundation.h>

extern int AMDeviceConnect (void *device);
extern int AMDeviceValidatePairing (void *device);
extern int AMDeviceStartSession (void *device);
extern int AMDeviceStopSession (void *device);
extern int AMDeviceDisconnect (void *device);

extern int AMDServiceConnectionReceive(void *device, char *buf,int size, int );

extern int AMDeviceSecureStartService(void *device,
                                      CFStringRef serviceName, // One of the services in lockdown's Services.plist
                                      int  flagsButZeroIsFineAlso,
                                      void *handle);

extern int AMDeviceNotificationSubscribe(void *, int , int , int, void **);

struct AMDeviceNotificationCallbackInformation {
	void 		*deviceHandle;
	uint32_t	msgType;
} ;


#define  BUFSIZE 128 // This is used by purpleConsole. So I figure I'll use it too.

void doDeviceConnect(void *deviceHandle)
{
 	int rc = AMDeviceConnect(deviceHandle);
	
	int fd;
	char buf[BUFSIZE];
    
    
	if (rc) {
		fprintf (stderr, "AMDeviceConnect returned: %d\n", rc);
		exit(1);
	}
    
	rc = AMDeviceValidatePairing(deviceHandle);
    
	if (rc)
    {
		fprintf (stderr, "AMDeviceValidatePairing() returned: %d\n", rc);
		exit(2);
        
	}
    
	rc = AMDeviceStartSession(deviceHandle);
	if (rc)
    {
		fprintf (stderr, "AMStartSession() returned: %d\n", rc);
		exit(2);
        
	}
    
	void *f;
	rc = AMDeviceSecureStartService(deviceHandle,
                                    (CFStringRef)RSSTR("com.apple.syslog_relay"), // Any of the services in lockdown's services.plist
                                    0,
                                    &f);
    
	if (rc != 0)
	{
        
		fprintf(stderr, "Unable to start service -- Rc %d fd: %p\n", rc, f);
		exit(4);
        
	}
    
    
 	rc = AMDeviceStopSession(deviceHandle);
	if (rc != 0)
	{
		fprintf(stderr, "Unable to disconnect - rc is %d\n",rc);
		exit(4);
        
	}
    
 	rc = AMDeviceDisconnect(deviceHandle);
    
    
	if (rc != 0)
	{
		fprintf(stderr, "Unable to disconnect - rc is %d\n",rc);
		exit(5);
	}
    
    
    
	memset (buf, '\0', 128); // technically, buf[0] = '\0' is enough
    
	while ((rc = AMDServiceConnectionReceive(f,
                                             buf,
                                             BUFSIZE, 0) > 0))
        
	{
		// Messages are read to the buf (Framework uses a socket descriptor)
		// For syslog_relay, messages are just the raw ASL output (including
		// kernel.debug), which makes it easy for this sample code.
		// Other services use bplists, which would call for those nasty
		// CFDictionary and other APIs..
		
		printf("%s", buf);
		fflush(NULL);
		memset (buf, '\0', 128);
        
	}
    
    
} // end doDevice


void callback(struct AMDeviceNotificationCallbackInformation *CallbackInfo)
{
	void *deviceHandle = CallbackInfo->deviceHandle;
	fprintf (stderr,"In callback, msgType is %d\n", CallbackInfo->msgType);
    
	switch (CallbackInfo->msgType)
	{
            
		case 1:
			fprintf(stderr, "Device %p connected\n", deviceHandle);
			doDeviceConnect(deviceHandle);
			break;
		case 2:
			fprintf(stderr, "Device %p disconnected\n", deviceHandle);
			break;
		case 3:
			fprintf(stderr, "Unsubscribed\n");
			break;
            
		default:
			fprintf(stderr, "Unknown message %d\n", CallbackInfo->msgType);
	}
    
}

#include <RSFileMonitor/RSFileMonitor.h>
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
    return RSStringByReplacingOccurrencesOfString(info, RSSTR("http://www.renren.com/"), RSSTR(""));
}

static RSMutableDictionaryRef token(RSStringRef uid) {
    __block RSStringRef requestToken = nil, _rtk = nil;
    RSAutoreleaseBlock(^{
        RSStringRef str = RSStringWithData(RSDataWithURL(RSURLWithString(RSStringWithFormat(RSSTR("http://www.renren.com/%r"), uid))), RSStringEncodingUTF8);

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
    return RSAutorelease(RSMutableCopy(RSAllocatorDefault, RSAutorelease(RSDictionaryCreateWithObjectsAndOKeys(RSAllocatorDefault, requestToken, RSSTR("requestToken"), _rtk, RSSTR("_rtk"), NULL))));
}

struct RSCoreAnalyzer {
    RSURLConnectionRef connection;
    RSMutableDataRef reiceveData;
    RSURLResponseRef response;  // weak
};
typedef struct RSCoreAnalyzer *RSCoreAnalyzer;

static struct RSCoreAnalyzer analyzer = {0};

static void RSURLConnectionWillStartConnection_CoreAnalyzer(RSURLConnectionRef connection, RSURLRequestRef willSendRequest, RSURLResponseRef redirectResponse) {
    
}

static void RSURLConnectionDidReceiveResponse_CoreAnalyzer(RSURLConnectionRef connection, RSURLResponseRef response) {
    ((RSCoreAnalyzer)RSURLConnectionGetContext(connection))->response = response;
}

static void RSURLConnectionDidReceiveData_CoreAnalyzer(RSURLConnectionRef connection, RSDataRef data){
    RSDataAppend((RSMutableDataRef)(((RSCoreAnalyzer)RSURLConnectionGetContext(connection))->reiceveData), data);
}

static void RSURLConnectionDidSendBodyData_CoreAnalyzer(RSURLConnectionRef connection, RSDataRef data, RSUInteger totalBytesWritten, RSUInteger totalBytesExpectedToWrite){
    
}

static void RSURLConnectionDidFinishLoading_CoreAnalyzer(RSURLConnectionRef connection){
    RSShow(RSSTR("finished loading"));
    token(uid(RSAutorelease(RSURLGetString(RSURLResponseGetURL(((RSCoreAnalyzer)RSURLConnectionGetContext(connection))->response)))));
}

static void RSURLConnectionDidFailWithError_CoreAnalyzer(RSURLConnectionRef connection, RSErrorRef error) {
    RSShow(error);
}


void RSCoreAnalyzerCreate(RSCoreAnalyzer analyzer, RSStringRef userName, RSStringRef password) {
    if (!analyzer) return;
    RSStringRef baseURLString = RSSTR("http://www.renren.com/PLogin.do");
    RSMutableURLRequestRef request = RSAutorelease(RSURLRequestCreateMutable(RSAllocatorDefault, RSURLWithString(baseURLString)));
    RSStringRef post = __buildURLString(RSSTR("http://www.renren.com/SysHome.do"), RSSTR("renren.com"), userName, password);
    RSShow(post);
    RSShow(request);
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
}


#include <curl/curl.h>
int main (int argc, char **argv)
{
    CURL *core = nil;
    const char *filename = RSStringGetUTF8String(RSFileManagerStandardizingPath(RSSTR("~/Desktop/cookie")));
    
    core = curl_easy_init();
    curl_easy_setopt(core, CURLOPT_URL, "http://www.baidu.com");
    curl_easy_setopt(core, CURLOPT_FOLLOWLOCATION, 1);
    curl_easy_setopt(core, CURLOPT_COOKIEFILE, filename);
//    curl_easy_setopt(core, CURLOPT_COOKIEJAR, filename);
    if (0 == curl_easy_perform(core))
        RSShow(RSSTR("baidu success"));
    
    
    RSHTTPCookieStorageRef storage = RSHTTPCookieStorageGetSharedStorage();
    RSArrayRef cookies = RSCookiesWithCore(core);
    RSArrayApplyBlock(cookies, RSMakeRange(0, RSArrayGetCount(cookies)), ^(const void *value, RSUInteger idx, BOOL *isStop) {
        RSHTTPCookieStorageSetCookie(storage, (RSHTTPCookieRef)value);
    });
    curl_easy_cleanup(core);
    RSPerformBlockAfterDelay(1.0, ^{
        RSRunLoopStop(RSRunLoopGetMain());
    });
    RSRunLoopRun();
    return 0;
    
//    
//    core = curl_easy_init();
//    
//    RSStringRef baseURLString = RSSTR("http://www.renren.com/PLogin.do");
//    RSStringRef post = __buildURLString(RSSTR("http://www.renren.com/SysHome.do"), RSSTR("renren.com"), RSSTR("821016215@qq.com"), RSSTR("retvalhusihua"));
//    struct curl_slist *chunk = nil;
//    chunk = curl_slist_append(chunk, "Content-Type: application/x-www-form-urlencoded");
//    curl_easy_setopt(core, CURLOPT_URL, RSStringGetUTF8String(baseURLString));
//    curl_easy_setopt(core, CURLOPT_POST, 1);
//    curl_easy_setopt(core, CURLOPT_POSTFIELDS, RSStringGetUTF8String(post));
//    curl_easy_setopt(core, CURLOPT_HTTPHEADER, chunk);
//    
//    curl_easy_setopt(core, CURLOPT_HEADER, 0);
//    curl_easy_setopt(core, CURLOPT_FOLLOWLOCATION, 1);
//    curl_easy_setopt(core, CURLOPT_COOKIEFILE, RSFileManagerFileExistsAtPath(RSFileManagerGetDefault(), RSFileManagerStandardizingPath(RSSTR("~/Desktop/cookie")), nil) ? filename : "");
//    curl_easy_setopt(core, CURLOPT_COOKIEJAR, filename);
//    if (0 == curl_easy_perform(core)) RSShow(RSSTR("success"));
//    curl_slist_free_all(chunk);
//    curl_easy_cleanup(core);
//    
//    RSShow(RSSTR("---------------------------------------------------------------------------------------------------\n"));
//    
//    core = curl_easy_init();
//    curl_easy_setopt(core, CURLOPT_URL, "http://www.renren.com");
//    curl_easy_setopt(core, CURLOPT_FOLLOWLOCATION, 1);
//    curl_easy_setopt(core, CURLOPT_COOKIEFILE, filename);
//    curl_easy_setopt(core, CURLOPT_COOKIEJAR, filename);
//    if (0 == curl_easy_perform(core)) RSShow(RSSTR("login success"));
//    curl_easy_cleanup(core);
//    return 0;
//    RSURLConnectionSendAsynchronousRequest(RSURLRequestWithURL(RSURLWithString(RSSTR("http://www.baidu.com/"))), nil, ^(RSURLResponseRef response, RSDataRef data, RSErrorRef error) {
//        RSShow(response);
//    });
//    RSRunLoopRun();
//    return 0;
//    RSCoreAnalyzerCreate(&analyzer, RSSTR("821016215@qq.com"), RSSTR("retvalhusihua"));
//    RSCoreAnalyzerStart(&analyzer);
//    RSRunLoopRun();
//    return 0;
//    RSAutoreleaseBlock(^{
//        RSShow(RSStringWithData(RSAutorelease(RSURLConnectionSendSynchronousRequest(RSURLRequestWithURL(RSURLWithString(RSSTR("http://www.baidu.com"))), nil, nil)), RSStringEncodingUTF8));
//    });
//    return 0;
//    RSStringRef str = RSSTR("2013-11-18 16:20:30,948");
//    RSShow(RSStringWithSubstring(str, RSMakeRange(11, 8)));
//    RSShow(RSAutorelease(RSDateCreateWithString(RSAllocatorDefault, RSAutorelease(RSStringCreateWithFormat(RSAllocatorDefault, RSSTR("%RT%RZ"), RSStringWithSubstring(str, RSMakeRange(0, 10)), RSStringWithSubstring(str, RSMakeRange(11, 8)))))));
//    return 0;
//    RSRunLoopAddSource(RSRunLoopGetCurrent(), RSAutorelease(RSFileMonitorCreateRunLoopSource(RSAllocatorDefault, RSAutorelease(RSFileMonitorCreate(RSAllocatorDefault, RSFileManagerStandardizingPath(RSSTR("~/Library/Preferences/com.apple.dock.plist")))), 0)), RSRunLoopDefaultMode);
//    RSRunLoopRun();
//    return 0;
//    void *subscribe;
//    
//    int rc = AMDeviceNotificationSubscribe(callback, 0,0,0, &subscribe);
//    if (rc <0) {
//		fprintf(stderr, "Unable to subscribe: AMDeviceNotificationSubscribe returned %d\n", rc);
//		exit(1);
//	}
////    CFRunLoopRun();
//    fprintf(stderr, "runloop end\n");
//    return 0;
}
