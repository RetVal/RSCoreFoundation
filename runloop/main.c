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


#include <string.h>
#include <stdio.h>
#include <fcntl.h>
#include <stdlib.h>
#include <sys/ioctl.h>     // for _IOW, a macro required by FSEVENTS_CLONE
#include <sys/types.h>     // for uint32_t and friends, on which fsevents.h relies
//#include <sys/_types.h>     // for uint32_t and friends, on which fsevents.h relies

#include <sys/sysctl.h> // for sysctl, KERN_PROC, etc.
#include <errno.h>

//#include <sys/fsevents.h> would have been nice, but it's no longer available, as Apple
// now wraps this with FSEventStream. So instead - rip what we need from the kernel headers..


// Actions for each event type
#define FSE_IGNORE    0
#define FSE_REPORT    1
#define FSE_ASK       2    // Not implemented yet



#define FSEVENTS_CLONE          _IOW('s', 1, fsevent_clone_args)

#define FSE_INVALID             -1
#define FSE_CREATE_FILE          0
#define FSE_DELETE               1
#define FSE_STAT_CHANGED         2
#define FSE_RENAME               3
#define FSE_CONTENT_MODIFIED     4
#define FSE_EXCHANGE             5
#define FSE_FINDER_INFO_CHANGED  6
#define FSE_CREATE_DIR           7
#define FSE_CHOWN                8
#define FSE_XATTR_MODIFIED       9
#define FSE_XATTR_REMOVED       10

#define FSE_MAX_EVENTS          11
#define FSE_ALL_EVENTS         998

#define FSE_EVENTS_DROPPED     999

// The types of each of the arguments for an event
// Each type is followed by the size and then the
// data.  FSE_ARG_VNODE is just a path string

#define FSE_ARG_VNODE    0x0001   // next arg is a vnode pointer
#define FSE_ARG_STRING   0x0002   // next arg is length followed by string ptr
#define FSE_ARG_PATH     0x0003   // next arg is a full path
#define FSE_ARG_INT32    0x0004   // next arg is a 32-bit int
#define FSE_ARG_INT64    0x0005   // next arg is a 64-bit int
#define FSE_ARG_RAW      0x0006   // next arg is a length followed by a void ptr
#define FSE_ARG_INO      0x0007   // next arg is the inode number (ino_t)
#define FSE_ARG_UID      0x0008   // next arg is the file's uid (uid_t)
#define FSE_ARG_DEV      0x0009   // next arg is the file's dev_t
#define FSE_ARG_MODE     0x000a   // next arg is the file's mode (as an int32, file type only)
#define FSE_ARG_GID      0x000b   // next arg is the file's gid (gid_t)
#define FSE_ARG_FINFO    0x000c   // next arg is a packed finfo (dev, ino, mode, uid, gid)
#define FSE_ARG_DONE     0xb33f   // no more arguments

#if __LP64__
typedef struct fsevent_clone_args {
    int8_t  *event_list;
    int32_t  num_events;
    int32_t  event_queue_depth;
    int32_t *fd;
} fsevent_clone_args;
#else
typedef struct fsevent_clone_args {
    int8_t  *event_list;
    int32_t  pad1;
    int32_t  num_events;
    int32_t  event_queue_depth;
    int32_t *fd;
    int32_t  pad2;
} fsevent_clone_args;
#endif

// copied from bsd/vfs/vfs_events.c

#pragma pack(1)  // to be on the safe side. Not really necessary.. struct fields are aligned.
typedef struct kfs_event_a {
    uint16_t type;
    uint16_t refcount;
    pid_t    pid;
} kfs_event_a;

typedef struct kfs_event_arg {
    uint16_t type;
    uint16_t pathlen;
    char data[0];
} kfs_event_arg;


#pragma pack()

#define BUFSIZE 64 *1024

// Utility functions
static const char *typeToString (uint32_t Type)
{
	switch (Type)
	{
		case FSE_CREATE_FILE: return ("Created ");
		case FSE_DELETE: return ("Deleted ");
		case FSE_STAT_CHANGED: return ("Stat changed ");
		case FSE_RENAME:	return ("Renamed ");
		case FSE_CONTENT_MODIFIED:	return ("Modified ");
		case FSE_CREATE_DIR:	return ("Created dir ");
		case FSE_CHOWN:	return ("Chowned ");
            
		case FSE_EXCHANGE: return ("Exchanged "); /* 5 */
		case FSE_FINDER_INFO_CHANGED: return ("Finder Info changed for "); /* 6 */
		case FSE_XATTR_MODIFIED: return ("Extended attributes changed for "); /* 9 */
	 	case FSE_XATTR_REMOVED: return ("Extended attributesremoved for "); /* 10 */
		default : return ("Not yet ");
            
	}
}

static RSStringRef nameForProcessWithPID(pid_t pidNum)
{
    RSStringRef returnString = nil;
    int mib[4], maxarg = 0, numArgs = 0;
    size_t size = 0;
    char *args = NULL, *namePtr = NULL, *stringPtr = NULL;
    
    mib[0] = CTL_KERN;
    mib[1] = KERN_ARGMAX;
    
    size = sizeof(maxarg);
    if ( sysctl(mib, 2, &maxarg, &size, NULL, 0) == -1 ) {
        return nil;
    }
    
    args = (char *)malloc( maxarg );
    if ( args == NULL ) {
        return nil;
    }
    
    mib[0] = CTL_KERN;
    mib[1] = KERN_PROCARGS2;
    mib[2] = pidNum;
    
    size = (size_t)maxarg;
    if ( sysctl(mib, 3, args, &size, NULL, 0) == -1 ) {
        free( args );
        return nil;
    }
    
    memcpy( &numArgs, args, sizeof(numArgs) );
    stringPtr = args + sizeof(numArgs);
    printf("%s\n", stringPtr);
    if ( (namePtr = strrchr(stringPtr, '/')) != NULL ) {
        namePtr++;
        returnString = RSStringWithUTF8String(namePtr);
    }
    else {
        returnString = RSStringWithUTF8String(stringPtr);
    }
    return returnString;
}

static const char *getProcName(long pid)
{
    return RSStringGetUTF8String(nameForProcessWithPID((pid_t)pid));
}

static int doArg(char *arg)
{
	// Dump an arg value
	// Quick and dirty, but does the trick..
	unsigned short *argType = (unsigned short *) arg;
	unsigned short *argLen   = (unsigned short *) (arg + 2);
	uint32_t	*argVal = (uint32_t *) (arg+4);
	uint64_t	*argVal64 = (uint64_t *) (arg+4);
	dev_t		*dev;
	char		*str;
    
    
    
	switch (*argType)
    {
            
		case FSE_ARG_INT64: // This is a timestamp field on the FSEvent
			printf ("Arg64: %lld\n", *argVal64);
			break;
		case FSE_ARG_STRING: // This is a filename, for move/rename (Type 3)
		 	str = (char *)argVal;
			printf("%s ", str);
			break;
			
		case FSE_ARG_DEV: // Device, corresponding to block device on which fs is mounted
			dev = (dev_t *) argVal;
            
			printf ("DEV: %d,%d ", major(*dev), minor(*dev)); break;
		case FSE_ARG_MODE: // mode bits, etc
			printf("MODE: %x ", *argVal); break;
            
		case FSE_ARG_PATH: // Not really used... Implement this later..
			printf ("PATH: " ); break;
		case FSE_ARG_INO: // Inode number (unique up to device)
			printf ("INODE: %d ", *argVal); break;
		case FSE_ARG_UID: // UID of operation performer
			printf ("UID: %d ", *argVal); break;
		case FSE_ARG_GID: // Ditto, GID
			printf ("GID: %d ", *argVal); break;
		case FSE_ARG_FINFO: // Not handling this yet.. Not really used, either..
			printf ("FINFO\n"); break;
		case FSE_ARG_DONE:	printf("\n");return 2;
            
		default:
			printf ("(ARG of type %hd, len %hd)\n", *argType, *argLen);
            
            
    }
    
	return (4 + *argLen);
    
}

// And.. Ze Main

static void __main (int argc, char **argv)
{
    
	int fsed, cloned_fsed;
	int i;
	int rc;
	fsevent_clone_args  clone_args;
    unsigned short * __unused arg_type = nil;
	char buf[BUFSIZE] = {0};
    
	// Open the device
	fsed = open ("/dev/fsevents", O_RDONLY);
    
	int8_t	events[FSE_MAX_EVENTS];
    
	if (geteuid())
	{
		fprintf(stderr,"Opening /dev/fsevents requires root permissions\n");
	}
    
	if (fsed < 0)
	{
		perror ("open");
        exit(1);
	}
    
    
	// Prepare event mask list. In our simple example, we want everything
	// (i.e. all events, so we say "FSE_REPORT" all). Otherwise, we
	// would have to specifically toggle FSE_IGNORE for each:
	//
	// e.g.
	//       events[FSE_XATTR_MODIFIED] = FSE_IGNORE;
	//       events[FSE_XATTR_REMOVED]  = FSE_IGNORE;
	// etc..
    
	for (i = 0; i < FSE_MAX_EVENTS; i++)
	{
		events[i] = FSE_REPORT;
	}
    
	// Get ready to clone the descriptor:
    
	memset(&clone_args, '\0', sizeof(clone_args));
	clone_args.fd = &cloned_fsed; // This is the descriptor we get back
	clone_args.event_queue_depth = 10;
	clone_args.event_list = events;
	clone_args.num_events = FSE_MAX_EVENTS;
	
	// Do it.
    
	rc = ioctl (fsed, FSEVENTS_CLONE, &clone_args);
	
	if (rc < 0) { perror ("ioctl"); exit(2);}
	
	// We no longer need original..
    
	close (fsed);
    
	
	// And now we simply read, ad infinitum (aut nauseam)
    
	while ((rc = (int)read (cloned_fsed, buf, BUFSIZE)) > 0)
	{
		// rc returns the count of bytes for one or more events:
		int offInBuf = 0;
        struct kfs_event_a *fse = (struct kfs_event_a *)(buf + offInBuf);
        struct kfs_event_arg *fse_arg;
		while (offInBuf < rc) {
            //		if (offInBuf) { printf ("Next event: %d\n", offInBuf);};
            long pid = fse->pid;
            printf ("%s (PID:%d) %s ", getProcName(pid), fse->pid , typeToString(fse->type) );
            if (fse->pid == 0)
            {
                __builtin_memset(buf, 0, BUFSIZE);
                printf("\n");
                break;
            }
            
            offInBuf+= sizeof(struct kfs_event_a);
            fse_arg = (struct kfs_event_arg *) &buf[offInBuf];
            printf ("%s\n", fse_arg->data);
            offInBuf += sizeof(kfs_event_arg) + fse_arg->pathlen ;
            
            
            int arg_len = doArg(buf + offInBuf);
            offInBuf += arg_len;
            while (arg_len >2)
			{
		   	    arg_len = doArg(buf + offInBuf);
                offInBuf += arg_len;
			}
            
            
		}
		if (rc > offInBuf) {
            if (fse->pid) printf ("***Warning: Some events may be lost\n");
            else printf("skip kernel_task(pid:0) events\n");
        }
        __builtin_memset(buf, 0, BUFSIZE);
	}
}

int main (int argc, char **argv)
{
    RSShow(RSSTR("begin"));
    RSRunLoopTimerRef timer = RSRunLoopTimerCreateWithHandler(RSAllocatorDefault, 1.0, 3.0, 0, 0, ^(RSRunLoopTimerRef timer) {
        RSShow(RSSTR("timer invoke"));
    });
    RSRunLoopAddTimer(RSRunLoopGetCurrent(), timer, RSRunLoopDefaultMode);
    RSRelease(timer);
    dispatch_async(dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_BACKGROUND, 0), ^{
        RSRunLoopPerformBlock(RSRunLoopGetMain(), RSRunLoopDefaultMode, ^{
            RSShow(RSSTR("main thread perform block"));
        });
    });
    
    RSRunLoopRun();
    
    return 0;
    void *subscribe;
    
    int rc = AMDeviceNotificationSubscribe(callback, 0,0,0, &subscribe);
    if (rc <0) {
		fprintf(stderr, "Unable to subscribe: AMDeviceNotificationSubscribe returned %d\n", rc);
		exit(1);
	}
//    CFRunLoopRun();
    RSRunLoopRunEx(kCFRunLoopDefaultMode);
    fprintf(stderr, "runloop end\n");
    return 0;
}
