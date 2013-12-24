//
//  RSFileMonitor.c
//  RSCoreFoundation
//
//  Created by Closure on 11/13/13.
//  Copyright (c) 2013 RetVal. All rights reserved.
//

#include "RSFileMonitor.h"

#include <RSCoreFoundation/RSRuntime.h>

#include <RSCoreFoundation/RSArray.h>
#include <RSCoreFoundation/RSFileHandle.h>
#include <RSCoreFoundation/RSNotificationCenter.h>

//社会学家研究表明：难看和没钱的男人，并不比高帅富更加靠谱
#include <dispatch/dispatch.h>
#include <fcntl.h>

struct __RSFileMonitor
{
    RSRuntimeBase _base;
    pthread_mutex_t _lock;
    RSStringRef _filePath;
    dispatch_source_t _source;
    RSRunLoopSourceRef _runloopSource;
    
    int _fd;
};

RSInline void __RSFileMonitorLock(RSFileMonitorRef monitor) {
    pthread_mutex_lock(&(((RSFileMonitorRef)monitor)->_lock));
    //    RSLog(6, RSSTR("__RSRunLoopLock locked %p"), rl);
}

RSInline void __RSFileMonitorUnlock(RSFileMonitorRef monitor) {
    //    RSLog(6, RSSTR("__RSRunLoopLock unlocking %p"), rl);
    pthread_mutex_unlock(&(((RSFileMonitorRef)monitor)->_lock));
}

RSInline void __RSFileMonitorLockInit(pthread_mutex_t *lock) {
    pthread_mutexattr_t mattr;
    pthread_mutexattr_init(&mattr);
    pthread_mutexattr_settype(&mattr, PTHREAD_MUTEX_RECURSIVE);
    int32_t mret = pthread_mutex_init(lock, &mattr);
    pthread_mutexattr_destroy(&mattr);
    if (0 != mret) {
    }
}

static void __RSFileMonitorClassInit(RSTypeRef rs)
{
    RSFileMonitorRef monitor = (RSFileMonitorRef)rs;
    __RSFileMonitorLockInit(&monitor->_lock);
}

static RSTypeRef __RSFileMonitorClassCopy(RSAllocatorRef allocator, RSTypeRef rs, BOOL mutableCopy)
{
    return RSRetain(rs);
}

static void __RSFileMonitorClassDeallocate(RSTypeRef rs)
{
    RSFileMonitorRef monitor = (RSFileMonitorRef)rs;
    __RSFileMonitorLock(monitor);
    if (monitor->_filePath) RSRelease(monitor->_filePath);
    if (monitor->_source) {
        dispatch_source_cancel(monitor->_source);
        close(monitor->_fd);
        monitor->_fd = 0;
        dispatch_release(monitor->_source);
        monitor->_source = nil;
    }
    __RSFileMonitorUnlock(monitor);
}

static BOOL __RSFileMonitorClassEqual(RSTypeRef rs1, RSTypeRef rs2)
{
    RSFileMonitorRef RSFileMonitor1 = (RSFileMonitorRef)rs1;
    RSFileMonitorRef RSFileMonitor2 = (RSFileMonitorRef)rs2;
    BOOL result = NO;
    
    result = RSEqual(RSFileMonitor1->_filePath, RSFileMonitor2->_filePath);
    
    return result;
}

static RSHashCode __RSFileMonitorClassHash(RSTypeRef rs)
{
    return 0;
}

static RSStringRef __RSFileMonitorClassDescription(RSTypeRef rs)
{
    RSStringRef description = RSStringCreateWithFormat(RSAllocatorDefault, RSSTR("RSFileMonitor %p"), rs);
    return description;
}

static RSRuntimeClass __RSFileMonitorClass =
{
    _RSRuntimeScannedObject,
    "RSFileMonitor",
    __RSFileMonitorClassInit,
    __RSFileMonitorClassCopy,
    __RSFileMonitorClassDeallocate,
    __RSFileMonitorClassEqual,
    __RSFileMonitorClassHash,
    __RSFileMonitorClassDescription,
    nil,
    nil
};

static RSTypeID _RSFileMonitorTypeID = _RSRuntimeNotATypeID;

static RSSpinLock __RSFileMonitorDefaultLock = RSSpinLockInit;
static RSTypeRef __RSFileMonitorDefault = nil;

static void __RSFileMonitorCore();

static void __RSFileMonitorDeallocate()
{
    
}

static void __RSFileMonitorInitialize()
{
    RSSyncUpdateBlock(&__RSFileMonitorDefaultLock, ^{ RSPerformBlockInBackGround(^{ RSRunLoopPerformBlock(RSRunLoopGetCurrent(), RSRunLoopDefaultMode, ^{ __RSFileMonitorCore();}); RSRunLoopRun();});});
}

RSExport RSTypeID RSFileMonitorGetTypeID()
{
    if (_RSFileMonitorTypeID) return _RSFileMonitorTypeID;
//    __RSFileMonitorInitialize();
    _RSFileMonitorTypeID = __RSRuntimeRegisterClass(&__RSFileMonitorClass);
    __RSRuntimeSetClassTypeID(&__RSFileMonitorClass, _RSFileMonitorTypeID);
    __RSFileMonitorDefault = __RSRuntimeCreateInstance(RSAllocatorSystemDefault, _RSFileMonitorTypeID, sizeof(struct __RSFileMonitor) - sizeof(struct __RSRuntimeBase));
    return _RSFileMonitorTypeID;
}

static RSFileMonitorRef __RSFileMonitorCreateInstance(RSAllocatorRef allocator)
{
    RSFileMonitorRef instance = (RSFileMonitorRef)__RSRuntimeCreateInstance(allocator, _RSFileMonitorTypeID, sizeof(struct __RSFileMonitor) - sizeof(RSRuntimeBase));
    
//    <#do your other setting for the instance#>
    
    return instance;
}

static void __RSFileMonitorCore_(RSStringRef path)
{
    dispatch_queue_t queue = dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, 0);
	int fildes = open(RSStringGetUTF8String(path), O_EVTONLY);
    if (-1 == fildes) return;
	__block dispatch_source_t source = dispatch_source_create(DISPATCH_SOURCE_TYPE_VNODE,fildes,
															  DISPATCH_VNODE_DELETE | DISPATCH_VNODE_WRITE |
                                                              DISPATCH_VNODE_EXTEND | DISPATCH_VNODE_ATTRIB |
                                                              DISPATCH_VNODE_LINK | DISPATCH_VNODE_RENAME |
                                                              DISPATCH_VNODE_REVOKE,
															  queue);
	dispatch_source_set_event_handler(source, ^{
        unsigned long data = dispatch_source_get_data(source);
        if (data & DISPATCH_VNODE_DELETE) {
        	__RSCLog(RSLogLevelNotice, "DELETE\n");
        } else if (data & DISPATCH_VNODE_WRITE) {
        	__RSCLog(RSLogLevelNotice, "WRITE\n");
        } else if (data & DISPATCH_VNODE_EXTEND) {
        	__RSCLog(RSLogLevelNotice, "EXTEND\n");
        } else if (data & DISPATCH_VNODE_ATTRIB) {
        	__RSCLog(RSLogLevelNotice, "ATTRIBUTE\n");
        } else if (data & DISPATCH_VNODE_LINK) {
        	__RSCLog(RSLogLevelNotice, "LINK\n");
        } else if (data & DISPATCH_VNODE_RENAME) {
        	__RSCLog(RSLogLevelNotice, "RENAME\n");
        } else if (data & DISPATCH_VNODE_REVOKE) {
        	__RSCLog(RSLogLevelNotice, "REVOKE\n");
        }
        if(data & DISPATCH_VNODE_DELETE)
        {
        	dispatch_source_cancel(source);
            __RSFileMonitorCore_(path);
        }
    });
	dispatch_source_set_cancel_handler(source, ^(void) 
                                       {
                                           close(fildes);
                                       });
	dispatch_resume(source);
}

static void __RSFileMonitorPraperInfo(void *info)
{
    RSFileMonitorRef monitor = (RSFileMonitorRef)info;
    RSStringRef file = monitor->_filePath;
    RSRetain(file);
    const char *buf = nil;
    monitor->_fd = open(buf = RSStringCopyUTF8String(file), O_EVTONLY);
    RSAllocatorDeallocate(RSAllocatorSystemDefault, buf);
    buf = nil;
    RSRelease(file);
}

static void __RSFileMonitorSchedule(void *info, RSRunLoopRef rl, RSStringRef mode)
{
    if (!info) return;
    __RSFileMonitorPraperInfo(info);
    RSFileMonitorRef monitor = (RSFileMonitorRef)info;
    __RSFileMonitorCore_(monitor->_filePath);
    return;
    dispatch_queue_t queue = dispatch_get_current_queue();
    dispatch_source_t source = dispatch_source_create(DISPATCH_SOURCE_TYPE_VNODE, monitor->_fd,
                                                      DISPATCH_VNODE_DELETE | DISPATCH_VNODE_WRITE |
                                                      DISPATCH_VNODE_EXTEND | DISPATCH_VNODE_ATTRIB |
                                                      DISPATCH_VNODE_LINK | DISPATCH_VNODE_RENAME |
                                                      DISPATCH_VNODE_REVOKE,
                                                      queue);
    monitor->_source = source;
    RSRunLoopSourceSignal(monitor->_runloopSource);
}

static void __RSFileMonitorPerformV0(void *info)
{
    RSFileMonitorRef monitor = (RSFileMonitorRef)info;
    dispatch_source_set_event_handler(monitor->_source, ^{
        unsigned long data = dispatch_source_get_data(monitor->_source);
        
        if (data & DISPATCH_VNODE_DELETE) {
            __RSCLog(RSLogLevelNotice, "DELETE\n");
            RSRunLoopRemoveSource(RSRunLoopGetCurrent(), monitor->_runloopSource, RSRunLoopDefaultMode);
            RSRelease(monitor->_runloopSource);
            monitor->_runloopSource = nil;
            
            RSRelease(RSFileMonitorCreateRunLoopSource(RSAllocatorSystemDefault, monitor, 0));
            RSRunLoopAddSource(RSRunLoopGetCurrent(), monitor->_runloopSource, RSRunLoopDefaultMode);
        } else if (data & DISPATCH_VNODE_WRITE) {
            __RSCLog(RSLogLevelNotice, "WRITE\n");
        } else if (data & DISPATCH_VNODE_EXTEND) {
            __RSCLog(RSLogLevelNotice, "EXTEND\n");
        } else if (data & DISPATCH_VNODE_ATTRIB) {
            __RSCLog(RSLogLevelNotice, "ATTRIBUTE\n");
        } else if (data & DISPATCH_VNODE_LINK) {
            __RSCLog(RSLogLevelNotice, "LINK\n");
        } else if (data & DISPATCH_VNODE_RENAME) {
            __RSCLog(RSLogLevelNotice, "RENAME\n");
        } else if (data & DISPATCH_VNODE_REVOKE) {
            __RSCLog(RSLogLevelNotice, "REVOKE\n");
        }
    });
    dispatch_resume(monitor->_source);
}

static void __RSFileMonitorCancel(void *info, RSRunLoopRef rl, RSStringRef mode)
{
    RSFileMonitorRef monitor = (RSFileMonitorRef)info;
//    dispatch_source_cancel(monitor->_source);
    dispatch_source_cancel(monitor->_source);
    close(monitor->_fd);
    monitor->_fd = 0;
    dispatch_release(monitor->_source);
    monitor->_source = nil;
}

RSExport RSRunLoopSourceRef RSFileMonitorCreateRunLoopSource(RSAllocatorRef allocator, RSFileMonitorRef monitor, RSIndex order)
{
    if (monitor->_runloopSource && !RSRunLoopSourceIsValid(monitor->_runloopSource))
    {
        RSRelease(monitor->_runloopSource);
        monitor->_runloopSource = nil;
    }
    if (!monitor->_runloopSource)
    {
        RSRunLoopSourceContext context = {0};
        context.version = 0;
        context.info = monitor;
        context.retain = RSRetain;
        context.release = RSRelease;
        context.hash = RSHash;
        context.equal = RSEqual;
        context.description = RSDescription;
        context.schedule = __RSFileMonitorSchedule;
        context.perform = __RSFileMonitorPerformV0;
        context.cancel = __RSFileMonitorCancel;
        RSRunLoopSourceRef source = RSRunLoopSourceCreate(allocator, order, &context);
        monitor->_runloopSource = source;
    }
    return (RSRunLoopSourceRef)RSRetain(monitor->_runloopSource);
}

RSExport RSFileMonitorRef RSFileMonitorCreate(RSAllocatorRef allocator, RSStringRef filePath)
{
    RSFileMonitorRef monitor = (RSFileMonitorRef)__RSRuntimeCreateInstance(allocator, RSFileMonitorGetTypeID(), sizeof(struct __RSFileMonitor) - sizeof(struct __RSRuntimeBase));
    monitor->_filePath = RSRetain(filePath);
    return monitor;
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

// Utility functions
static RSStringRef typeToString (uint32_t Type)
{
	switch (Type)
	{
		case FSE_CREATE_FILE: return RSSTR("Created ");
		case FSE_DELETE: return RSSTR("Deleted ");
		case FSE_STAT_CHANGED: return RSSTR("Stat changed ");
		case FSE_RENAME:	return RSSTR("Renamed ");
		case FSE_CONTENT_MODIFIED:	return RSSTR("Modified ");
		case FSE_CREATE_DIR:	return RSSTR("Created dir ");
		case FSE_CHOWN:	return RSSTR("Chowned ");
            
		case FSE_EXCHANGE: return RSSTR("Exchanged "); /* 5 */
		case FSE_FINDER_INFO_CHANGED: return RSSTR("Finder Info changed for "); /* 6 */
		case FSE_XATTR_MODIFIED: return RSSTR("Extended attributes changed for "); /* 9 */
	 	case FSE_XATTR_REMOVED: return RSSTR("Extended attributesremoved for "); /* 10 */
		default : return RSSTR("Not yet ");
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
//    printf("%s\n", stringPtr);
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
//			printf ("Arg64: %lld\n", *argVal64);
			break;
		case FSE_ARG_STRING: // This is a filename, for move/rename (Type 3)
		 	str = (char *)argVal;
//			printf("%s ", str);
			break;
			
		case FSE_ARG_DEV: // Device, corresponding to block device on which fs is mounted
			dev = (dev_t *) argVal;
            
//			printf ("DEV: %d,%d ", major(*dev), minor(*dev)); break;
		case FSE_ARG_MODE: // mode bits, etc
//			printf("MODE: %x ", *argVal); break;
            
		case FSE_ARG_PATH: // Not really used... Implement this later..
//			printf ("PATH: " ); break;
		case FSE_ARG_INO: // Inode number (unique up to device)
//			printf ("INODE: %d ", *argVal); break;
		case FSE_ARG_UID: // UID of operation performer
//			printf ("UID: %d ", *argVal); break;
		case FSE_ARG_GID: // Ditto, GID
//			printf ("GID: %d ", *argVal); break;
		case FSE_ARG_FINFO: // Not handling this yet.. Not really used, either..
//			printf ("FINFO\n"); break;
		case FSE_ARG_DONE:
//            printf("\n");return 2;
            
		default:
//			printf ("(ARG of type %hd, len %hd)\n", *argType, *argLen);
            ;
            
    }
    
	return (4 + *argLen);
    
}

static void __memory_pressure_test()
{
    for (RSUInteger idx = 0; idx < 5000; idx++) {
        RSStringWithFormat(RSSTR("RSStringWithFormat(RSSTR(\"%d\"), idx);"), idx);
    }
    return;
}
// And.. Ze Main

static void __RSFileMonitorCore()
{
    
	int fsed, cloned_fsed;
	int i;
	int rc;
	fsevent_clone_args  clone_args;
    unsigned short * __unused arg_type = nil;
    #define BUFSIZE 64*1024
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
    
	RSRunLoopPerformBlock(RSRunLoopGetMain(), RSRunLoopDefaultMode, ^{
        RSLog(RSSTR("perform block Main Loop"));
    });
    
    dispatch_async(dispatch_get_main_queue(), ^{
       RSLog(RSSTR("dispatch perform block main queue"));
    });
	// And now we simply read, ad infinitum (aut nauseam)
    
	while ((rc = (int)read (cloned_fsed, buf, BUFSIZE)) > 0)
	{
        RSAutoreleasePoolRef pool = RSAutoreleasePoolCreate(RSAllocatorSystemDefault);
		// rc returns the count of bytes for one or more events:
		int offInBuf = 0;
        struct kfs_event_a *fse = (struct kfs_event_a *)(buf + offInBuf);
        struct kfs_event_arg *fse_arg;
		while (offInBuf < rc)
        {
            //		if (offInBuf) { printf ("Next event: %d\n", offInBuf);};
//            __RSLog(RSLogLevelNotice, RSSTR("%r (PID:%d) %r "), nameForProcessWithPID((pid_t)fse->pid), fse->pid, typeToString(fse->type));
            nameForProcessWithPID((pid_t)fse->pid);
            typeToString(fse->type);
            if (fse->pid == 0)
            {
                __builtin_memset(buf, 0, BUFSIZE);
                break;
            }
            
            offInBuf+= sizeof(struct kfs_event_a);
            fse_arg = (struct kfs_event_arg *) &buf[offInBuf];
//            __RSLog(RSLogLevelNotice, RSSTR("%s\n"), fse_arg->data);
            offInBuf += sizeof(kfs_event_arg) + fse_arg->pathlen ;
            
            
            int arg_len = doArg(buf + offInBuf);
            offInBuf += arg_len;
            while (arg_len > 2 && (offInBuf + 1024 < BUFSIZE))
			{
		   	    arg_len = doArg(buf + offInBuf);
                offInBuf += arg_len;
			}
		}
		if (rc > offInBuf) {
//            if (fse->pid) printf ("***Warning: Some events may be lost\n");
//            else printf("skip kernel_task(pid:0) events\n");
        }
        __builtin_memset(buf, 0, BUFSIZE);
        __memory_pressure_test();
        RSAutoreleasePoolDrain(pool);
	}
    __RSCLog(RSLogLevelNotice, "out of the core");
}

#include <vproc.h>
#include <CoreServices/CoreServices.h>

static void _watch_path(int kq, const char *path, char *buf)
{
    int fd = open(path, O_EVTONLY);
    fcntl(fd, F_SETFD, 1);
    const int nevents = 1;
    struct kevent changelist[nevents];
    struct kevent eventlist[nevents];
//#define EVFILT_READ		(-1)
//#define EVFILT_WRITE		(-2)
//#define EVFILT_AIO		(-3)	/* attached to aio requests */
//#define EVFILT_VNODE		(-4)	/* attached to vnodes */
//#define EVFILT_PROC		(-5)	/* attached to struct proc */
//#define EVFILT_SIGNAL		(-6)	/* attached to struct proc */
//#define EVFILT_TIMER		(-7)	/* timers */
//#define EVFILT_MACHPORT         (-8)	/* Mach portsets */
//#define EVFILT_FS		(-9)	/* Filesystem events */
//#define EVFILT_USER             (-10)   /* User events */
//    /* (-11) unused */
//#define EVFILT_VM		(-12)	/* Virtual memory events */
//    
//    
//#define EVFILT_SYSCOUNT		14
//#define EVFILT_THREADMARKER	EVFILT_SYSCOUNT /* Internal use only */
    
    /* actions */
//#define EV_ADD		0x0001		/* add event to kq (implies enable) */
//#define EV_DELETE	0x0002		/* delete event from kq */
//#define EV_ENABLE	0x0004		/* enable event */
//#define EV_DISABLE	0x0008		/* disable event (not reported) */
//#define EV_RECEIPT	0x0040		/* force EV_ERROR on success, data == 0 */
//    
//    /* flags */
//#define EV_ONESHOT	0x0010		/* only report one occurrence */
//#define EV_CLEAR	0x0020		/* clear event state after reporting */
//#define EV_DISPATCH     0x0080          /* disable event after reporting */
//    
//#define EV_SYSFLAGS	0xF000		/* reserved by system */
//#define EV_FLAG0	0x1000		/* filter-specific flag */
//#define EV_FLAG1	0x2000		/* filter-specific flag */
//    
//    /* returned values */
//#define EV_EOF		0x8000		/* EOF detected */
//#define EV_ERROR	0x4000		/* error, data contains errno */
    
//#define	NOTE_DELETE	0x00000001		/* vnode was removed */
//#define	NOTE_WRITE	0x00000002		/* data contents changed */
//#define	NOTE_EXTEND	0x00000004		/* size increased */
//#define	NOTE_ATTRIB	0x00000008		/* attributes changed */
//#define	NOTE_LINK	0x00000010		/* link count changed */
//#define	NOTE_RENAME	0x00000020		/* vnode was renamed */
//#define	NOTE_REVOKE	0x00000040		/* vnode access was revoked */
//#define NOTE_NONE	0x00000080		/* No specific vnode event: to test for EVFILT_READ activation*/

    changelist[0].ident = fd;
    changelist[0].filter = 0xfffc;  // EVFILT_VNODE
    changelist[0].flags = 0x21;     // EV_CLEAR | EV_ADD
    changelist[0].fflags = 0x27;    // NOTE_RENAME | NOTE_EXTEND | NOTE_WRITE | NOTE_DELETE
    changelist[0].data = 0;
    changelist[0].udata = buf;
    /*
     movsxd     rax, ebx
     mov        rsi, qword [ss:rbp-0xd90+var_40]
     mov        qword [ds:rsi], rax
     mov        word [ds:rsi+0x8], 0xfffc
     mov        word [ds:rsi+0xa], 0x21
     mov        dword [ds:rsi+0xc], 0x27
     mov        qword [ds:rsi+0x10], 0x0
     mov        rax, qword [ss:rbp-0xd90+var_48]
     mov        qword [ds:rsi+0x18], rax
     mov        edi, dword [ss:rbp-0xd90+var_36]
     mov        edx, 0x1
     xor        ecx, ecx
     xor        r8d, r8d
     xor        r9d, r9d
     call       imp___stubs__kevent
     test       eax, eax
     jns        0x206f7
     */
    int rc = kevent(kq, changelist, nevents, 0, 0, 0);
    
}

//f (a, b, c, d, e, f);
//a->%rdi, b->%rsi, c->%rdx, d->%rcx, e->%r8, f->%r9
static void __useCoreServicesFSEventStream()
{
//    FSEventStreamRef stream = FSEventStreamCreate(<#CFAllocatorRef allocator#>, <#FSEventStreamCallback callback#>, <#FSEventStreamContext *context#>, <#CFArrayRef pathsToWatch#>, <#FSEventStreamEventId sinceWhen#>, <#CFTimeInterval latency#>, <#FSEventStreamCreateFlags flags#>)
    
    FSEventStreamContext *context = nil;
//    struct FSEventStreamContext {
//        CFIndex             version;
//        void *              info;
//        CFAllocatorRetainCallBack  retain;
//        CFAllocatorReleaseCallBack  release;
//        CFAllocatorCopyDescriptionCallBack  copyDescription;
//    };
    typedef struct FSEventStreamContext     FSEventStreamContext;
    void *buf = CFAllocatorAllocate(kCFAllocatorDefault, 0x1a8, 0); // structure
    bzero(buf, 0x1a8);
    CFRetain(kCFAllocatorDefault);
    struct __unknown_structure1 {
        CFAllocatorRef deallocator;
        
    };
    // rdx + 0x18 = version = r12
    // rdx + 0x20 = info = r12 + 0x8
    // rdx + 0x28 = retain = r12 + 0x10
    // rdx + 0x30 = release = r12 + 0x18
    // rdx + 0x38 = description = r12 + 0x20
    // rdx + 0x50 = CFAllocatorAllocate(...)
    // kqueue
    // CFAllocatorAllocate(...)
    // CFAllocatorAllocate(...)
    // CALL _watch_path
    // CALL _watch_all_parents
    //
/*
 check null of params
 
 */
}

