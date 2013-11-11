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


int main (int argc, char **argv)
{
    void *subscribe;
    
    int rc = AMDeviceNotificationSubscribe(callback, 0,0,0, &subscribe);
    if (rc <0) {
		fprintf(stderr, "Unable to subscribe: AMDeviceNotificationSubscribe returned %d\n", rc);
		exit(1);
	}
//    CFRunLoopRun();
    RSRunLoopRunEx(kCFRunLoopCommonModes);
    fprintf(stderr, "runloop end\n");
    return 0;
}
