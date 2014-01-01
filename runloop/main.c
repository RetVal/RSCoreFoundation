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
    return RSAutorelease(RSMutableCopy(RSAllocatorDefault, RSAutorelease(RSDictionaryCreateWithObjectsAndOKeys(RSAllocatorDefault, RSAutorelease(requestToken), RSSTR("requestToken"), RSAutorelease(_rtk), RSSTR("_rtk"), NULL))));
}

struct RSCoreAnalyzer {
    RSURLConnectionRef connection;
    RSMutableDataRef reiceveData;
    RSURLResponseRef response;  // weak
};
typedef struct RSCoreAnalyzer *RSCoreAnalyzer;

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

static void RSURLConnectionDidFinishLoading_CoreAnalyzer(RSURLConnectionRef connection){
    RSShow(RSSTR("finished loading"));
    RSShow(token(uid(RSURLGetString(RSURLResponseGetURL(((RSCoreAnalyzer)RSURLConnectionGetContext(connection))->response)))));
    RSRunLoopStop(RSRunLoopGetMain());
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
//    RSURLRequestSetHeaderFieldValue(request, RSSTR("Content-Length"), RSStringWithFormat(RSSTR("%ld"), RSDataGetLength(postData)));
    RSURLRequestSetHeaderFieldValue(request, RSSTR("Content-Type"), RSSTR("application/x-www-form-urlencoded"));
//    RSURLRequestSetHeaderFieldValue(request, RSSTR("debug"), RSBooleanTrue);
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

#include "curl.h"
int main (int argc, char **argv)
{
//    RSHTTPCookieStorageRemoveAllCookies(RSHTTPCookieStorageGetSharedStorage());
//    RSCoreAnalyzerCreate(&analyzer, RSSTR("821016215@qq.com"), RSSTR("retvalhusihua"));
//    RSCoreAnalyzerStart(&analyzer);
    
    RSPerformBlockAfterDelay(1.0, ^{
        RSShow(token(uid(RSURLGetString(RSURLWithString(RSSTR("http://www.renren.com/340278563"))))));
        RSRunLoopStop(RSRunLoopGetMain());
    });
    RSRunLoopRun();
    RSCoreAnalyzerRelease(&analyzer);
    sleep(1);
    sleep(1);
    return 0;
}
