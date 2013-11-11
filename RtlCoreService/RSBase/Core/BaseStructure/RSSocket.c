//
//  RSSocket.c
//  RSCoreFoundation
//
//  Created by RetVal on 11/18/12.
//  Copyright (c) 2012 RetVal. All rights reserved.
//

#include <RSCoreFoundation/RSDictionary.h>
#include <RSCoreFoundation/RSNil.h>
#include <RSCoreFoundation/RSNumber.h>
#include <RSCoreFoundation/RSQueue.h>
#include <RSCoreFoundation/RSSocket.h>
#include <RSCoreFoundation/RSRuntime.h>
#include "RSPrivate/RSPacket/RSPacket.h"

#if DEPLOYMENT_TARGET_MACOSX || DEPLOYMENT_TARGET_EMBEDDED || DEPLOYMENT_TARGET_EMBEDDED_MINI
#include <sys/sysctl.h>
#include <sys/un.h>
#include <libc.h>
#include <dlfcn.h>
#endif
#include <errno.h>
#include <stdio.h>

#define SOCKET_ERROR    (-1)
#define SOCKET_SUCCESS  (0)

#if DEPLOYMENT_TARGET_MACOSX || DEPLOYMENT_TARGET_EMBEDDED || DEPLOYMENT_TARGET_EMBEDDED_MINI
#define INVALID_SOCKET (RSSocketHandle)(-1)
#define closesocket(a) close((a))
#define ioctlsocket(a,b,c) ioctl((a),(b),(c))
#endif

#define __RSSOCKET_V_0  0
#define __RSSOCKET_V_1  1
#define LOG_RSSOCKET

RSInline int __RSSocketLastError(void) {
#if DEPLOYMENT_TARGET_WINDOWS
    return WSAGetLastError();
#else
    return errno;
#endif
}

static RSCBuffer __RSSocketGetLocalName(RSBlock name[RSBufferSize])
{
    if (name == nil) return nil;
    memset(name, 0, RSBufferSize*sizeof(RSBlock));
    gethostname(name, RSBufferSize);
    return name;
}

RSExport RSStringRef RSSocketGetLocalName()
{
    RSBlock hostName[RSBufferSize] = {0};
    return RSStringCreateWithCString(RSAllocatorSystemDefault, __RSSocketGetLocalName(hostName), RSStringEncodingUTF8);
}

static RSCBuffer __RSSocketGetLocalIp(RSBlock ip[32])
{
    if (ip == nil) return nil;
    RSBlock hostName[RSBufferSize] = {0};
	struct hostent* host = nil;
	host = (struct hostent*)gethostbyname((const char*)__RSSocketGetLocalName(hostName));
    __RSCLog(RSLogLevelNotice, "%s\n", hostName);
    if (host) {
        snprintf(ip, 32, "%d.%d.%d.%d",
                 (int)(host->h_addr_list[0][0]&0x00ff),
                 (int)(host->h_addr_list[0][1]&0x00ff),
                 (int)(host->h_addr_list[0][2]&0x00ff),
                 (int)(host->h_addr_list[0][3]&0x00ff));
		
    }
    else
    {
        if (hostName)
        {
            RSStringRef _ipStr = RSStringCreateWithCString(RSAllocatorSystemDefault, hostName, RSStringEncodingUTF8);
            RSStringRef ipAddrStr = nil;
            if (YES == RSStringHasPrefix(_ipStr, RSSTR("ip-")))
            {
                ipAddrStr = RSStringCreateSubStringWithRange(RSAllocatorSystemDefault, _ipStr, RSMakeRange(3, 2));
            }
            RSRelease(_ipStr);
            if (ipAddrStr)
            {
                
                RSRelease(ipAddrStr);
            }
        }
    }
    return	ip;
}

RSExport RSStringRef RSSocketGetLocalIp()
{
    RSBlock ip[32];
    return RSStringCreateWithCString(RSAllocatorSystemDefault, __RSSocketGetLocalIp(ip), RSStringEncodingUTF8);
}

static RSArrayRef __RSSocketGetLocalIps(RSStringRef host, RSStringRef service, RSBitU32 family, RSBitU32 protocol)
{
    if (!host || !service) return nil;
    struct addrinfo hints, *res, *ressave;
    RSZeroMemory(&hints, sizeof(struct addrinfo));
    hints.ai_family = family;
    hints.ai_flags = AI_CANONNAME;
    hints.ai_socktype = protocol;
    if (getaddrinfo(RSStringGetCStringPtr(host, RSStringEncodingUTF8), RSStringGetCStringPtr(service, RSStringEncodingUTF8), &hints, &res)) return nil;
    ressave = res;
    RSMutableArrayRef results = RSArrayCreateMutable(RSAllocatorSystemDefault, 0);
    do {
        RSStringRef addrStr = RSStringCreateWithCString(RSAllocatorSystemDefault, res->ai_canonname, RSStringEncodingUTF8);
        RSArrayAddObject(results, addrStr);
        RSRelease(addrStr);
    } while ((res = res->ai_next));
    freeaddrinfo(ressave);
    return results;
}

RSExport RSArrayRef RSSocketGetLocalIps()
{
    return __RSSocketGetLocalIps(RSSTR(""), RSSTR(""), SOCK_STREAM, IPPROTO_TCP);
}

RSExport RSDictionaryRef RSSocketGetAddressInfo(struct sockaddr* addr, socklen_t addr_len)
{
    RSBlock h[RSBufferSize] = {0}, s[RSBufferSize] = {0};
    if (!getnameinfo(addr, addr_len, h, RSBufferSize, s, RSBufferSize, 0))
        return RSAutorelease(RSDictionaryCreateWithObjectsAndOKeys(RSAllocatorSystemDefault, RSAutorelease(RSStringCreateWithCString(RSAllocatorSystemDefault, h, RSStringEncodingUTF8)), RSSTR("hostName"), RSAutorelease(RSStringCreateWithCString(RSAllocatorSystemDefault, s, RSStringEncodingUTF8)), RSSTR("serverName"), NULL));
    return nil;
    
}

#define MAX_SOCKADDR_LEN 256
#define MAX_DATA_SIZE 65535
#define MAX_CONNECTION_ORIENTED_DATA_SIZE 32768
static RSSpinLock __RSAllSocketsLock = RSSpinLockInit;
static RSMutableDictionaryRef __RSAllSockets = nil;	// <socket handle, RSSocket>

static RSSpinLock __RSActiveSocketsLock = RSSpinLockInit;	//
static RSMutableArrayRef __RSWriteSockets = nil;	// writable sockets
static RSMutableArrayRef __RSReadSockets = nil;		// readable sockets
static RSMutableDataRef __RSWriteSocketsFds = nil;	// fd_set*
static RSMutableDataRef __RSReadSocketsFds = nil;	// fd_set*

static RSSocketHandle __RSWakeupSocketPair[2] = {INVALID_SOCKET};	// for initialize the socket manager, and dispatch the task sechdule.

static BOOL __RSReadSocketsTimeoutInvalid = YES;
static void *__RSSocketManagerThread = nil;


struct __RSSocket {
    RSRuntimeBase _base;
    RSSpinLock _lock;
    RSSpinLock _rwLock;
    RSSocketHandle _socket;
    RSSocketCallBackType _callbackTypes;
    RSSocketCallBack _perform;
    RSQueueRef _dataReadQueue;
    RSQueueRef _addressQueue;
    RSQueueRef _dataWriteQueue;
    
    RSDataRef  _address;
    RSDataRef  _peerAddress;
    struct timeval _readBufferTimeout;
    RSRunLoopSourceRef _source0;
    RSSocketContext _context;
    RSBitU32 _socketType;
    RSBitU32 _errorCode;
    RSBitU32 _schedule;
    struct {
        struct {
            char _read      : 1;
            char _accept    : 1;
            char _data      : 1;
            char _connect   : 1;
            char _write     : 1;
            char _error     : 1;
            char _connected : 1;
            char _reserved  : 1;
        }_untouchable;
    }_flag;
};

RSInline void __RSSocketSetValid(RSSocketRef s)
{
    s->_base._rsinfo._rsinfo1 |= 0x1;
}

RSInline void __RSSocketUnsetValid(RSSocketRef s)
{
    s->_base._rsinfo._rsinfo1 &= ~0x1;
}

RSInline BOOL __RSSocketIsValid(RSSocketRef s)
{
    return (s->_base._rsinfo._rsinfo1 & 0x1) == 0x1;
}

RSInline void __RSSocketLock(RSSocketRef s) {
    RSSpinLockLock(&(s->_lock));
}

RSInline void __RSSocketUnlock(RSSocketRef s) {
    RSSpinLockUnlock(&(s->_lock));
}

RSInline void __RSSocketWriteLock(RSSocketRef s)
{
	RSSpinLockLock(&s->_rwLock);
}

RSInline void __RSSocketWriteUnlock(RSSocketRef s)
{
	RSSpinLockUnlock(&s->_rwLock);
}

RSInline RSBitU32 __RSSocketIsScheduled(RSSocketRef s)
{
    return s->_schedule;
}

RSInline void __RSSocketScheduleStart(RSSocketRef s)
{
    ++s->_schedule;
}

RSInline void __RSSocketScheduleDone(RSSocketRef s)
{
    if (__RSSocketIsScheduled(s)) --s->_schedule;
}

RSInline BOOL __RSSocketIsConnected(RSSocketRef s)
{
    return s->_flag._untouchable._connected;
}

RSInline void __RSSocketSetConnectedWithFlag(RSSocketRef s, BOOL flag)
{
    s->_flag._untouchable._connected = flag;
}

RSInline BOOL __RSSocketIsConnectionOriented(RSSocketRef s)
{
    return (SOCK_STREAM == s->_socketType || SOCK_SEQPACKET == s->_socketType);
}

RSInline RSOptionFlags __RSSocketCallBackTypes(RSSocketRef s)
{
    return s->_callbackTypes;
}

RSInline RSOptionFlags __RSFlagRemoveX(RSOptionFlags flags, RSOptionFlags f)
{
    return flags = (flags &= ~f);
}

RSInline RSOptionFlags __RSSocketEnabledCallTypes(RSSocketRef s)
{
    RSOptionFlags flags = __RSSocketCallBackTypes(s);
    if (s->_flag._untouchable._read) flags &= ~RSSocketReadCallBack;
    if (s->_flag._untouchable._write) flags &= ~RSSocketWriteCallBack;
    if (s->_flag._untouchable._connect) flags &= ~RSSocketConnectCallBack;
    if (s->_flag._untouchable._accept) flags &= ~RSSocketAcceptCallBack;
    if (s->_flag._untouchable._data) flags &= ~RSSocketDataCallBack;
    if (s->_flag._untouchable._error) flags &= ~RSSocketErrorCallBack;
    return flags;
}

RSInline void __RSSocketEnableCallTypes(RSSocketRef s, RSOptionFlags enable)
{
    if (enable & RSSocketReadCallBack) s->_flag._untouchable._read = NO;
    if (enable & RSSocketWriteCallBack) s->_flag._untouchable._write = NO;
    if (enable & RSSocketConnectCallBack) s->_flag._untouchable._connect = NO;
    if (enable & RSSocketAcceptCallBack) s->_flag._untouchable._accept = NO;
    if (enable & RSSocketDataCallBack) s->_flag._untouchable._data = NO;
    if (enable & RSSocketErrorCallBack) s->_flag._untouchable._error = NO;
    return;
}

RSInline RSOptionFlags __RSSocketReadCallBackType(RSSocketRef s)
{
    return __RSFlagRemoveX(__RSFlagRemoveX(__RSFlagRemoveX(__RSSocketCallBackTypes(s), RSSocketWriteCallBack), RSSocketConnectCallBack), RSSocketErrorCallBack);
}

RSInline RSOptionFlags __RSSocketWriteCallBackType(RSSocketRef s)
{
    return __RSFlagRemoveX(__RSFlagRemoveX(__RSFlagRemoveX(__RSSocketCallBackTypes(s), RSSocketReadCallBack), RSSocketAcceptCallBack), RSSocketErrorCallBack);
}

void __RSSocketClassInit(RSTypeRef obj)
{
    RSSocketRef memory = (RSSocketRef)obj;
    memory->_lock = RSSpinLockInit;
    memory->_rwLock = RSSpinLockInit;
    memory->_socket = INVALID_SOCKET;
    memory->_callbackTypes = RSSocketNoCallBack;
	memory->_dataReadQueue = nil;
	memory->_dataWriteQueue = nil;
	memory->_source0 = nil;
	memory->_address = nil;
	memory->_peerAddress = nil;
	timerclear(&memory->_readBufferTimeout);
    memset(&memory->_context, 0, sizeof(RSSocketContext));
    __RSSocketSetValid(memory);
    return;
}

static RSTypeRef __RSSocketClassCopy(RSAllocatorRef allocator, RSTypeRef rs, BOOL mutableCopy)
{
    RSSocketRef copy = RSSocketCreateCopy(allocator, (RSSocketRef)rs);
    return copy;
}

static void __RSSocketClassDeallocate(RSTypeRef obj)
{
    RSSocketRef s = (RSSocketRef)obj;
	__RSSocketLock(s);
    if (s->_address) RSRelease(s->_address);
    if (s->_peerAddress) RSRelease(s->_peerAddress);
    if (s->_dataReadQueue) RSRelease(s->_dataReadQueue);
	if (s->_dataWriteQueue) RSRelease(s->_dataWriteQueue);
    if (s->_context.release && s->_context.retain && s->_context.context)
		s->_context.release(s->_context.context);
    if (s->_source0) RSRelease(s->_source0);
    if (INVALID_SOCKET != s->_socket && s->_socket > 0)
	{
        closesocket(s->_socket);
		s->_socket = INVALID_SOCKET;
    }
	__RSSocketUnlock(s);
    s->_lock = RSSpinLockInit;
    
    return;
}

static BOOL __RSSocketClassEqual(RSTypeRef obj1, RSTypeRef obj2)
{
    if (obj1 == obj2) return YES;
    RSSocketRef s1 = (RSSocketRef)obj1;
    RSSocketRef s2 = (RSSocketRef)obj2;
	if (s1->_socketType != s2->_socketType) return NO;
	if (__builtin_memcmp(&s1->_readBufferTimeout, &s2->_readBufferTimeout, sizeof(struct timeval))) return NO;
	if (s1->_perform != s2->_perform) return NO;
	if (s1->_socket != s2->_socket) return NO;
    if (!RSEqual(s1->_address, s2->_address)) return NO;
    if (!RSEqual(s1->_peerAddress, s2->_peerAddress)) return NO;
    return YES;
}

static RSHashCode __RSSocketHash(RSTypeRef obj)
{
    RSSocketRef s = (RSSocketRef)obj;
    return RSHash(s->_address)^RSHash(s->_peerAddress);
}

static const RSRuntimeClass __RSSocketClass = {
    _RSRuntimeScannedObject,
    "RSSocket",
    __RSSocketClassInit,
    NULL,
    __RSSocketClassDeallocate,
    __RSSocketClassEqual,
    __RSSocketHash,
    NULL,
    NULL,
    NULL,
};

static RSTypeID _RSSocketTypeID = _RSRuntimeNotATypeID;


RSExport RSTypeID RSSocketGetTypeID()
{
    return _RSSocketTypeID;
}

RSExport RSSocketHandle RSSocketGetHandle(RSSocketRef s)
{
	if (s == nil) return INVALID_SOCKET;
    __RSGenericValidInstance(s, _RSSocketTypeID);
    return s->_socket;
}

static BOOL __RSSocketHandleIsValid(RSSocketHandle sock)
{
#if DEPLOYMENT_TARGET_WINDOWS
    SInt32 errorCode = 0;
    int errorSize = sizeof(errorCode);
    return !(0 != getsockopt(sock, SOL_SOCKET, SO_ERROR, (char *)&errorCode, &errorSize) && __RSSocketLastError() == WSAENOTSOCK);
#else
    RSBit32 flags = fcntl(sock, F_GETFL, 0);
	int error = __RSSocketLastError();
    return !(0 > flags && EBADF == (error));
#endif
}


RSExport BOOL RSSocketIsValid(RSSocketRef s)
{
	__RSGenericValidInstance(s, _RSSocketTypeID);
    return __RSSocketIsValid(s) && __RSSocketHandleIsValid(s->_socket);
}

RSInline void __RSSocketLockAll()
{
	RSSpinLockLock(&__RSAllSocketsLock);
}

RSInline void __RSSocketUnlockAll()
{
	RSSpinLockUnlock(&__RSAllSocketsLock);
}

RSInline RSIndex __RSSocketFdGetSize(RSDataRef fdSet)
{
    return NBBY * RSDataGetLength(fdSet);
}

RSInline BOOL __RSSocketFdClr(RSSocketHandle sock, RSMutableDataRef fdSet)
{
    /* returns YES if a change occurred, NO otherwise */
    BOOL retval = NO;
    if (INVALID_SOCKET != sock && 0 <= sock)
	{
        RSIndex numFds = NBBY * RSDataGetLength(fdSet);
        fd_mask *fds_bits;
        if (sock < numFds)
		{
            fds_bits = (fd_mask *)RSDataGetBytesPtr(fdSet);
            if (FD_ISSET(sock, (fd_set *)fds_bits))
			{
                retval = YES;
                FD_CLR(sock, (fd_set *)fds_bits);
            }
        }
    }
    return retval;
}

RSInline BOOL __RSSocketFdSet(RSSocketHandle sock, RSMutableDataRef fdSet)
{
    /* returns YES if a change occurred, NO otherwise */
    BOOL retval = NO;
    if (INVALID_SOCKET != sock && 0 <= sock)
	{
        RSIndex numFds = NBBY * RSDataGetLength(fdSet);
        fd_mask *fds_bits;
        if (sock >= numFds)
		{
            RSIndex oldSize = numFds / NFDBITS, newSize = (sock + NFDBITS) / NFDBITS, changeInBytes = (newSize - oldSize) * sizeof(fd_mask);
            RSDataSetLength(fdSet, changeInBytes);
            fds_bits = (fd_mask *)RSDataGetBytesPtr(fdSet);
            memset(fds_bits + oldSize, 0, changeInBytes);
        }
		else
		{
            fds_bits = (fd_mask *)RSDataGetBytesPtr(fdSet);
        }
        if (!FD_ISSET(sock, (fd_set *)fds_bits))
		{
            retval = YES;
            FD_SET(sock, (fd_set *)fds_bits);
        }
    }
    return retval;
}

RSInline BOOL __RSSocketSetFDForRead(RSSocketRef s)
{
	__RSReadSocketsTimeoutInvalid = YES;
	BOOL b = __RSSocketFdSet(s->_socket, __RSReadSocketsFds);
	if (b && INVALID_SOCKET != __RSWakeupSocketPair[0]) {
#if defined (LOG_RSSOCKET)
        __RSCLog(RSLogLevelNotice, "%s for %d\n", __func__, s->_socket);
#endif
        uint8_t c = 'r';
        send(__RSWakeupSocketPair[0], (const char *)&c, sizeof(c), 0);
    }
	return b;
}

RSInline BOOL __RSSocketClearFDForRead(RSSocketRef s)
{
    __RSReadSocketsTimeoutInvalid = YES;
    BOOL b = __RSSocketFdClr(s->_socket, __RSReadSocketsFds);
    if (b && INVALID_SOCKET != __RSWakeupSocketPair[0]) {
#if defined (LOG_RSSOCKET)
        __RSCLog(RSLogLevelNotice, "%s for %d\n", __func__, s->_socket);
#endif
        uint8_t c = 's';
        send(__RSWakeupSocketPair[0], (const char *)&c, sizeof(c), 0);
    }
    return b;
}

RSInline BOOL __RSSocketSetFDForWrite(RSSocketRef s)
{
    BOOL b = __RSSocketFdSet(s->_socket, __RSWriteSocketsFds);
	if (b && INVALID_SOCKET != __RSWakeupSocketPair[0]) {
#if defined (LOG_RSSOCKET)
        __RSCLog(RSLogLevelNotice, "%s for %d\n", __func__, s->_socket);
#endif
        uint8_t c = 'w';
        send(__RSWakeupSocketPair[0], (const char *)&c, sizeof(c), 0);
    }
    return b;
}

RSInline BOOL __RSSocketClearFDForWrite(RSSocketRef s)
{
	BOOL b = __RSSocketFdClr(s->_socket, __RSWriteSocketsFds);
	if (b && INVALID_SOCKET != __RSWakeupSocketPair[0]) {
#if defined (LOG_RSSOCKET)
        __RSCLog(RSLogLevelNotice, "%s for %d\n", __func__, s->_socket);
#endif
        uint8_t c = 'x';
        send(__RSWakeupSocketPair[0], (const char *)&c, sizeof(c), 0);
    }
    return b;
}

static RSBit32 __RSSocketInitMakePair()
{
	RSBit32 error;
	
    error = socketpair(PF_LOCAL, SOCK_DGRAM, 0, __RSWakeupSocketPair);
    if (0 <= error) error = fcntl(__RSWakeupSocketPair[0], F_SETFD, FD_CLOEXEC);
    if (0 <= error) error = fcntl(__RSWakeupSocketPair[1], F_SETFD, FD_CLOEXEC);
    if (0 > error)
	{
        closesocket(__RSWakeupSocketPair[0]);
        closesocket(__RSWakeupSocketPair[1]);
        __RSWakeupSocketPair[0] = INVALID_SOCKET;
        __RSWakeupSocketPair[1] = INVALID_SOCKET;
    }
	return error;
}

RSPrivate void __RSSocketInitialize()
{
	__RSAllSockets = RSDictionaryCreateMutable(RSAllocatorSystemDefault, 0, RSDictionaryNilKeyContext);
	__RSReadSockets = RSArrayCreateMutable(RSAllocatorSystemDefault, 0);
	__RSWriteSockets = RSArrayCreateMutable(RSAllocatorSystemDefault, 0);
	__RSWriteSocketsFds = RSDataCreateMutable(RSAllocatorSystemDefault, 0);
	__RSReadSocketsFds = RSDataCreateMutable(RSAllocatorSystemDefault, 0);
	//if (0 > __RSSocketInitMakePair()) __HALT();
    _RSSocketTypeID = __RSRuntimeRegisterClass(&__RSSocketClass);
    __RSRuntimeSetClassTypeID(&__RSSocketClass, _RSSocketTypeID);
	if (0 > __RSSocketInitMakePair())
	{
        __RSCLog(RSLogLevelEmergency, "socket make pair failed...");
	}
	else
	{
		BOOL yes = YES;
		ioctlsocket(__RSWakeupSocketPair[0], FIONBIO, (u_long *)&yes);
        ioctlsocket(__RSWakeupSocketPair[1], FIONBIO, (u_long *)&yes);
		__RSSocketFdSet(__RSWakeupSocketPair[1], __RSReadSocketsFds);
	}
}

RSPrivate void __RSSocketDeallocate()
{
    RSSpinLockLock(&__RSAllSocketsLock);
    RSSpinLockLock(&__RSActiveSocketsLock);
    RSRelease(__RSReadSockets);
    RSRelease(__RSWriteSockets);
    RSRelease(__RSReadSocketsFds);
    RSRelease(__RSWriteSocketsFds);
    RSRelease(__RSAllSockets);
    RSSpinLockUnlock(&__RSActiveSocketsLock);
    RSSpinLockUnlock(&__RSAllSocketsLock);
}

static struct timeval* intervalToTimeval(RSTimeInterval timeout, struct timeval* tv)
{
    if (timeout == 0.0)
        timerclear(tv);
    else
	{
        tv->tv_sec = (0 >= timeout || INT_MAX <= timeout) ? INT_MAX : (int)(float)floor(timeout);
        tv->tv_usec = (int)((timeout - floor(timeout)) * 1.0E6);
    }
    return tv;
}

static void _calcMinTimeout_locked(const void* val, void* ctxt)
{
    RSSocketRef s = (RSSocketRef) val;
    struct timeval** minTime = (struct timeval**) ctxt;
    if (timerisset(&s->_readBufferTimeout) && (*minTime == NULL || timercmp(&s->_readBufferTimeout, *minTime, <)))
        *minTime = &s->_readBufferTimeout;
    else if (RSQueueGetCount(s->_dataReadQueue))
	{
        /* If there's anyone with leftover bytes, they'll need to be awoken immediately */
        static struct timeval sKickerTime = { 0, 0 };
        *minTime = &sKickerTime;
    }
}

static void __RSSocketHandleWrite(RSSocketRef s, BOOL callBackNow);
static void __RSSocketHandleRead(RSSocketRef s, BOOL causedByTimeout);
static void __RSSocketDoCallback(RSSocketRef s, RSSocketCallBackType type, RSDataRef data, RSDataRef address, RSSocketHandle sock);

static volatile RSBitU64 __RSSocketManagerIteration = 0;
static void* __RSSocketManager(void* info)
{
    pthread_setname_np("com.retval.RSSocket.private");
    RSBit32 nrfds, maxnrfds, fdentries = 1;
    RSBit32 rfds, wfds;
    fd_set *exceptfds = NULL;
    fd_set *writefds = (fd_set *)RSAllocatorAllocate(RSAllocatorSystemDefault, fdentries * sizeof(fd_mask));
    fd_set *readfds = (fd_set *)RSAllocatorAllocate(RSAllocatorSystemDefault, fdentries * sizeof(fd_mask));
    fd_set *tempfds;
    RSBit32 idx, cnt;
    RSBitU8 buffer[256] __unused;
    RSMutableArrayRef selectedWriteSockets = RSArrayCreateMutable(RSAllocatorSystemDefault, 0);
    RSMutableArrayRef selectedReadSockets = RSArrayCreateMutable(RSAllocatorSystemDefault, 0);
    RSIndex selectedWriteSocketsIndex = 0, selectedReadSocketsIndex = 0;
    
    struct timeval tv;
    struct timeval* pTimeout = NULL;
    struct timeval timeBeforeSelect;
    __RSCLog(RSLogLevelNotice, "Socket pair <%d, %d>\n", __RSWakeupSocketPair[0], __RSWakeupSocketPair[1]);
    for (;;)
	{
#if defined(LOG_RSSOCKET)
        __RSCLog(RSLogLevelNotice, "\n*************************** __RSSocketManager ***************************\n");
#endif

        
        RSSpinLockLock(&__RSActiveSocketsLock);
        __RSSocketManagerIteration++;
		
        rfds = (RSBit32)__RSSocketFdGetSize(__RSReadSocketsFds);
        wfds = (RSBit32)__RSSocketFdGetSize(__RSWriteSocketsFds);
        maxnrfds = (RSBit32)__RSMax(rfds, wfds);

		__RSCLog(RSLogLevelNotice, "maxnrfds = %d\n", maxnrfds);
        if (maxnrfds > fdentries * (int)NFDBITS)
		{
            fdentries = (maxnrfds + NFDBITS - 1) / NFDBITS;
            writefds = (fd_set *)RSAllocatorReallocate(RSAllocatorSystemDefault, writefds, fdentries * sizeof(fd_mask));
            readfds = (fd_set *)RSAllocatorReallocate(RSAllocatorSystemDefault, readfds, fdentries * sizeof(fd_mask));
        }
        memset(writefds, 0, fdentries * sizeof(fd_mask));
        memset(readfds, 0, fdentries * sizeof(fd_mask));
		__builtin_memcpy(writefds, RSDataGetBytesPtr(__RSWriteSocketsFds), RSDataGetLength(__RSWriteSocketsFds));
		__builtin_memcpy(readfds, RSDataGetBytesPtr(__RSReadSocketsFds), RSDataGetLength(__RSReadSocketsFds));
		
        if (__RSReadSocketsTimeoutInvalid)
		{
            struct timeval* minTimeout = NULL;
            __RSReadSocketsTimeoutInvalid = NO;
#if defined(LOG_RSSOCKET)
            __RSCLog(RSLogLevelNotice, "Figuring out which sockets have timeouts...\n");
#endif
            RSArrayApplyFunction(__RSReadSockets, RSMakeRange(0, RSArrayGetCount(__RSReadSockets)), _calcMinTimeout_locked, (void*) &minTimeout);
			
            if (minTimeout == NULL)
			{
#if defined(LOG_RSSOCKET)
                __RSCLog(RSLogLevelNotice, "No one wants a timeout!\n");
#endif
                pTimeout = NULL;
            }
			else
			{
#if defined(LOG_RSSOCKET)
                __RSCLog(RSLogLevelNotice, "timeout will be %ld, %d!\n", minTimeout->tv_sec, minTimeout->tv_usec);
#endif
                tv = *minTimeout;
                pTimeout = &tv;
            }
        }
		
        if (pTimeout)
		{
#if defined(LOG_RSSOCKET)
            __RSCLog(RSLogLevelNotice, "select will have a %ld, %d timeout\n", pTimeout->tv_sec, pTimeout->tv_usec);
#endif
            gettimeofday(&timeBeforeSelect, NULL);
        }
		
		RSSpinLockUnlock(&__RSActiveSocketsLock);
		
#if DEPLOYMENT_TARGET_WINDOWS
        // On Windows, select checks connection failed sockets via the exceptfds parameter. connection succeeded is checked via writefds. We need both.
        exceptfds = writefds;
#endif
        nrfds = select(maxnrfds, readfds, writefds, exceptfds, pTimeout);
        
        /*
         * __RSReadSocketsFds/__RSWriteSocketFds will send a signal for new socket add or remove
         */
		
#if defined(LOG_RSSOCKET)
        __RSCLog(RSLogLevelNotice, "socket manager woke from select, ret=%ld\n", (long)nrfds);
#endif
		
		/*
		 * select returned a timeout
		 */
        if (0 == nrfds)
		{
			struct timeval timeAfterSelect;
			struct timeval deltaTime;
			gettimeofday(&timeAfterSelect, NULL);
			/* timeBeforeSelect becomes the delta */
			timersub(&timeAfterSelect, &timeBeforeSelect, &deltaTime);
			
#if defined(LOG_RSSOCKET)
			__RSCLog(RSLogLevelNotice, "Socket manager received timeout - kicking off expired reads (expired delta %ld, %d)\n", deltaTime.tv_sec, deltaTime.tv_usec);
#endif
			
			RSSpinLockLock(&__RSActiveSocketsLock);
			
			tempfds = NULL;
			cnt = (RSBit32)RSArrayGetCount(__RSReadSockets);
			for (idx = 0; idx < cnt; idx++)
			{
				RSSocketRef s = (RSSocketRef)RSArrayObjectAtIndex(__RSReadSockets, idx);
				if (timerisset(&s->_readBufferTimeout)/* || s->_leftoverBytes*/)
				{
					RSSocketHandle sock = s->_socket;
					// We might have an new element in __RSReadSockets that we weren't listening to,
					// in which case we must be sure not to test a bit in the fdset that is
					// outside our mask size.
					BOOL sockInBounds = (0 <= sock && sock < maxnrfds);
					/* if this sockets timeout is less than or equal elapsed time, then signal it */
					if (INVALID_SOCKET != sock && sockInBounds)
					{
#if defined(LOG_RSSOCKET)
						__RSCLog(RSLogLevelNotice, "Expiring socket %d (delta %ld, %d)\n", sock, s->_readBufferTimeout.tv_sec, s->_readBufferTimeout.tv_usec);
#endif
						RSArraySetObjectAtIndex(selectedReadSockets, selectedReadSocketsIndex, s);
						selectedReadSocketsIndex++;
						/* socket is removed from fds here, will be restored in read handling or in perform function */
						if (!tempfds) tempfds = (fd_set *)RSDataGetBytesPtr(__RSReadSocketsFds);
						FD_CLR(sock, tempfds);
					}
				}
			}
			
			RSSpinLockUnlock(&__RSActiveSocketsLock);
			
			/* and below, we dispatch through the normal read dispatch mechanism */
		}
        
		if (0 > nrfds)
		{
            RSBit32 selectError = __RSSocketLastError();
#if defined(LOG_RSSOCKET)
            __RSCLog(RSLogLevelNotice, "socket manager received error %ld from select\n", (long)selectError);
#endif
            if (EBADF == selectError)
			{
                RSMutableArrayRef invalidSockets = RSArrayCreateMutable(RSAllocatorSystemDefault, 0);
                RSSpinLockLock(&__RSActiveSocketsLock);
                cnt = (RSBit32)RSArrayGetCount(__RSWriteSockets);
                for (idx = 0; idx < cnt; idx++)
				{
                    RSSocketRef s = (RSSocketRef)RSArrayObjectAtIndex(__RSWriteSockets, idx);
                    if (!RSSocketIsValid(s)) {
#if defined(LOG_RSSOCKET)
                        __RSCLog(RSLogLevelNotice, "socket manager found write socket %d invalid\n", s->_socket);
#endif
                        RSArrayAddObject(invalidSockets, s);
                    }
                }
                cnt = (RSBit32)RSArrayGetCount(__RSReadSockets);
                for (idx = 0; idx < cnt; idx++)
				{
                    RSSocketRef s = (RSSocketRef)RSArrayObjectAtIndex(__RSReadSockets, idx);
                    if (!__RSSocketHandleIsValid(s->_socket))
					{
#if defined(LOG_RSSOCKET)
                        __RSCLog(RSLogLevelNotice, "socket manager found read socket %d invalid\n", s->_socket);
#endif
                        RSArrayAddObject(invalidSockets, s);
                    }
                }
                RSSpinLockUnlock(&__RSActiveSocketsLock);
				
                cnt = (RSBit32)RSArrayGetCount(invalidSockets);
                for (idx = 0; idx < cnt; idx++)
				{
                    RSSocketInvalidate(((RSSocketRef)RSArrayObjectAtIndex(invalidSockets, idx)));
                }
                RSRelease(invalidSockets);
            }
            continue;
        }
        if (FD_ISSET(__RSWakeupSocketPair[1], readfds))
		{
            recv(__RSWakeupSocketPair[1], (char *)buffer, sizeof(buffer), 0);
#if defined(LOG_RSSOCKET)
            __RSCLog(RSLogLevelNotice, "socket manager received %c on wakeup socket\n", buffer[0]);
#endif
        }
        RSSpinLockLock(&__RSActiveSocketsLock);
        tempfds = NULL;
        cnt = (RSBit32)RSArrayGetCount(__RSWriteSockets);
        for (idx = 0; idx < cnt; idx++)
		{
            RSSocketRef s = (RSSocketRef)RSArrayObjectAtIndex(__RSWriteSockets, idx);
            RSSocketHandle sock = s->_socket;
            // We might have an new element in __RSWriteSockets that we weren't listening to,
            // in which case we must be sure not to test a bit in the fdset that is
            // outside our mask size.
            BOOL sockInBounds = (0 <= sock && sock < maxnrfds);
            if (INVALID_SOCKET != sock && sockInBounds)
			{
                if (FD_ISSET(sock, writefds))
				{
                    RSArraySetObjectAtIndex(selectedWriteSockets, selectedWriteSocketsIndex, s);
                    selectedWriteSocketsIndex++;
                    /* socket is removed from fds here, restored by RSSocketReschedule */
                    if (!tempfds) tempfds = (fd_set *)RSDataGetBytesPtr(__RSWriteSocketsFds);
                    FD_CLR(sock, tempfds);
#if defined (LOG_RSSOCKET)
                    __RSCLog(RSLogLevelNotice, "Manager: cleared socket %d from write fds\n", s->_socket);
#endif
					// RSLog(RSSTR("Manager: cleared socket %p from write fds"), s);
                }
            }
        }
        tempfds = NULL;
        cnt = (RSBit32)RSArrayGetCount(__RSReadSockets);
        for (idx = 0; idx < cnt; idx++)
		{
            RSSocketRef s = (RSSocketRef)RSArrayObjectAtIndex(__RSReadSockets, idx);
            RSSocketHandle sock = s->_socket;
            // We might have an new element in __RSReadSockets that we weren't listening to,
            // in which case we must be sure not to test a bit in the fdset that is
            // outside our mask size.
            BOOL sockInBounds = (0 <= sock && sock < maxnrfds);
            if (INVALID_SOCKET != sock && sockInBounds && FD_ISSET(sock, readfds))
			{
                RSArraySetObjectAtIndex(selectedReadSockets, selectedReadSocketsIndex, s);
                selectedReadSocketsIndex++;
                /* socket is removed from fds here, will be restored in read handling or in perform function */
                if (!tempfds) tempfds = (fd_set *)RSDataGetBytesPtr(__RSReadSocketsFds);
                FD_CLR(sock, tempfds);
#if defined (LOG_RSSOCKET)
                __RSCLog(RSLogLevelNotice, "Manager: cleared socket %d from read fds\n", s->_socket);
#endif
            }
        }
        RSSpinLockUnlock(&__RSActiveSocketsLock);
        for (idx = 0; idx < selectedReadSocketsIndex; idx++)
		{
            RSSocketRef s = (RSSocketRef)RSArrayObjectAtIndex(selectedReadSockets, idx);
            if (RSNull == (RSNilRef)s) continue;
#if defined(LOG_RSSOCKET)
            __RSCLog(RSLogLevelNotice, "socket manager signaling socket %d for read\n", s->_socket);
#endif
            __RSSocketHandleRead(s, nrfds == 0);
            RSArraySetObjectAtIndex(selectedReadSockets, idx, RSNull);
        }
        selectedReadSocketsIndex = 0;
        
        for (idx = 0; idx < selectedWriteSocketsIndex; idx++)
		{
            RSSocketRef s = (RSSocketRef)RSArrayObjectAtIndex(selectedWriteSockets, idx);
            if (RSNull == (RSNilRef)s) continue;
#if defined(LOG_RSSOCKET)
            __RSCLog(RSLogLevelNotice, "socket manager signaling socket %d for write\n", s->_socket);
#endif
            __RSSocketHandleWrite(s, NO);
            RSArraySetObjectAtIndex(selectedWriteSockets, idx, RSNull);
        }
        selectedWriteSocketsIndex = 0;
    
#if defined(LOG_RSSOCKET)
        __RSCLog(RSLogLevelNotice, "*************************************************************************\n\n");
#endif
    }
	return nil;
}

static void __RSSocketSchedule(void* info, RSRunLoopRef rl, RSStringRef mode);
static void __RSSocketPerformV0(void* info, RSSocketCallBackType type);
static void __RSSocketCancel(void* info, RSRunLoopRef rl, RSStringRef mode);
// If callBackNow, we immediately do client callbacks, else we have to signal a v0 RunLoopSource so the
// callbacks can happen in another thread.

static void __RSSocketHandleWrite(RSSocketRef s, BOOL callBackNow)
{
	RSBit32 errorCode = 0;
	int errorSize = sizeof(errorCode);
	if (!RSSocketIsValid(s)) return;
	if (0 != getsockopt(s->_socket, SOL_SOCKET, SO_ERROR, (char *)&errorCode, (socklen_t *)&errorSize)) errorCode = 0;
	__RSSocketLock(s);
	
	RSOptionFlags writeAvailable = __RSSocketWriteCallBackType(s);
	writeAvailable &= (RSSocketWriteCallBack | RSSocketConnectCallBack);
	if (writeAvailable == 0 || (s->_flag._untouchable._write && s->_flag._untouchable._connect))
	{
		__RSSocketUnlock(s);
		return;
	}
	s->_errorCode = errorCode;
    if (__RSSocketIsConnected(s)) writeAvailable &= ~RSSocketConnectCallBack;
    else writeAvailable = RSSocketConnectCallBack;
    if (callBackNow) __RSSocketDoCallback(s, 0, nil, nil, 0);
    else {
        __RSSocketUnlock(s);
        if (!__RSSocketIsScheduled(s)) __RSSocketSchedule(s, RSRunLoopGetCurrent(), RSRunLoopDefault);
        __RSSocketPerformV0(s, writeAvailable);
//            __RSSocketCancel(s, RSRunLoopGetCurrent(), RSRunLoopDefault);
    }
}

static void __RSSocketHandleRead(RSSocketRef s, BOOL causedByTimeout)
{
	RSSocketHandle sock = INVALID_SOCKET;
    RSDataRef data = NULL, address = NULL;
    if (!RSSocketIsValid(s)) return;
	RSOptionFlags readCallBackType = __RSSocketReadCallBackType(s);
    if (readCallBackType & RSSocketDataCallBack ||
		readCallBackType & RSSocketReadCallBack)
	{
        RSBitU8 bufferArray[MAX_CONNECTION_ORIENTED_DATA_SIZE], *buffer;
        RSBitU8 name[MAX_SOCKADDR_LEN];
        int namelen = sizeof(name);
        RSBit32 recvlen = 0;
        if (__RSSocketIsConnectionOriented(s))
		{
            buffer = bufferArray;
            recvlen = (RSBit32)recvfrom(s->_socket, (char *)buffer, MAX_CONNECTION_ORIENTED_DATA_SIZE, 0, (struct sockaddr *)name, (socklen_t *)&namelen);
        }
		else
		{
            buffer = (RSBitU8 *)RSAllocatorAllocate(RSAllocatorSystemDefault, MAX_DATA_SIZE);
            if (buffer) recvlen = (RSBit32)recvfrom(s->_socket, (char *)buffer, MAX_DATA_SIZE, 0, (struct sockaddr *)name, (socklen_t *)&namelen);
        }
#if defined(LOG_RSSOCKET)
        __RSCLog(RSLogLevelNotice, "read %ld bytes on socket %d\n", (long)recvlen, s->_socket);
#endif
		if (recvlen > 0) data = RSDataCreate(RSGetAllocator(s), buffer, recvlen);
		else data = (RSDataRef)RSNull;
		if (buffer && buffer != bufferArray) RSAllocatorDeallocate(RSAllocatorSystemDefault, buffer);
		if (namelen > 0) address = RSDataCreate(RSGetAllocator(s), name, namelen);
		else address = (RSDataRef)RSNull;
		
		__RSSocketLock(s);
		if (!__RSSocketIsValid(s))
		{
			__RSSocketUnlock(s);
			RSRelease(data);
			RSRelease(address);
			return;
		}
		if (s->_dataReadQueue == nil) s->_dataReadQueue = RSQueueCreate(RSGetAllocator(s), 0, RSQueueNotAtom);
		RSQueueEnqueue(s->_dataReadQueue, data);
		if (s->_addressQueue == nil) s->_addressQueue = RSQueueCreate(RSGetAllocator(s), 0, RSQueueNotAtom);
		RSQueueEnqueue(s->_addressQueue, address);
		RSRelease(data);
		RSRelease(address);
		
		if (0 < recvlen &&
            (__RSSocketCallBackTypes(s) & RSSocketDataCallBack) != 0 &&
			(s->_flag._untouchable._data == 0) &&
			__RSSocketIsScheduled(s))
		{
            RSSpinLockLock(&__RSActiveSocketsLock);
            /* restore socket to fds */
            __RSSocketSetFDForRead(s);
            RSSpinLockUnlock(&__RSActiveSocketsLock);
        }
		if (readCallBackType & RSSocketReadCallBack)
			__RSSocketDoCallback(s, RSSocketReadCallBack, data, address, RSSocketGetHandle(s));
		else
			__RSSocketUnlock(s);
	}
	else if (readCallBackType & RSSocketAcceptCallBack)
	{
		RSBitU8 name[MAX_SOCKADDR_LEN];
        int namelen = sizeof(name);
        sock = accept(s->_socket, (struct sockaddr *)name, (socklen_t *)&namelen);
        if (INVALID_SOCKET == sock) {
            //??? should return error
            return;
        }
        if (NULL != name && 0 < namelen) {
            address = RSDataCreate(RSGetAllocator(s), name, namelen);
        } else {
            address = (RSDataRef)RSRetain(RSNil);
        }
        
		__RSSocketLock(s);
        
		data = RSDataCreate(RSAllocatorSystemDefault, (const RSBitU8*)&sock, sizeof(RSSocketHandle));
		if (s->_dataReadQueue == nil) s->_dataReadQueue = RSQueueCreate(RSGetAllocator(s), 0, NO);
		RSQueueEnqueue(s->_dataReadQueue, data);
		if (s->_addressQueue == nil) s->_addressQueue = RSQueueCreate(RSGetAllocator(s), 0, NO);
		RSQueueEnqueue(s->_addressQueue, address);
		RSRelease(data);
		RSRelease(address);
		if ((__RSSocketCallBackTypes(s) & RSSocketAcceptCallBack) != 0 &&
			(s->_flag._untouchable._accept == 0) &&
			__RSSocketIsScheduled(s))
        {
            RSSpinLockLock(&__RSActiveSocketsLock);
            /* restore socket to fds */
            __RSSocketSetFDForRead(s);
            RSSpinLockUnlock(&__RSActiveSocketsLock);
        }
		__RSSocketUnlock(s);
        __RSSocketPerformV0(s, RSSocketAcceptCallBack);
        if (!__RSSocketIsScheduled(s)) __RSSocketSchedule(s, RSRunLoopGetCurrent(), RSRunLoopDefault);
        return;
	}
}

static void __RSSocketDoCallback(RSSocketRef s, RSSocketCallBackType type, RSDataRef data, RSDataRef address, RSSocketHandle sock)
{
    __RSSocketUnlock(s);
//    typedef void (*RSSocketCallBack)(RSSocketRef s, RSSocketCallBackType type, RSDataRef address, const void *data, void *info)
    s->_perform(s, type, address, data, s->_context.context);
}

static RSSocketHandle __RSSocketCreateHandle(const RSSocketSignature* signature)
{
	if (signature == nil) return INVALID_SOCKET;
	RSSocketHandle handle = socket(signature->protocolFamily, signature->socketType, signature->protocol);
	if (handle == INVALID_SOCKET) return handle;
	BOOL wasBlocking = YES;
	RSBitU32 yes = 1;
	RSBit32 flags = fcntl(handle, F_GETFL, 0);
	if (flags >= 0) wasBlocking = ((flags & O_NONBLOCK) == 0);
	if (wasBlocking) ioctlsocket(handle, FIONBIO, (u_long *)&yes);
	return handle;
}

static RSSocketRef __RSSocketCreateInstance(RSAllocatorRef allocator,
											RSSocketHandle handle,
											RSSocketCallBackType cbtype,
											RSSocketCallBack callback,
											const RSSocketContext* context,
											BOOL useInstance)
{
	RSSocketRef memory = nil;
    
	__RSSocketLockAll();
	if (INVALID_SOCKET != handle && (memory = (RSSocketRef)RSDictionaryGetValue(__RSAllSockets, (void *)(uintptr_t)(handle))))
	{
		if (useInstance)
		{
			__RSSocketUnlockAll();
			return (RSSocketRef)RSRetain(memory);
		}
		else
		{
			__RSSocketUnlockAll();
			RSSocketInvalidate(memory);
			__RSSocketLockAll();
		}
	}
	memory = (RSSocketRef)__RSRuntimeCreateInstance(allocator, _RSSocketTypeID, sizeof(struct __RSSocket) - sizeof(RSRuntimeBase));
	if (nil == memory)
	{
		__RSSocketUnlockAll();
		return nil;
	}
	RSBitU32 typeSize = sizeof(memory->_socketType);
	if (INVALID_SOCKET == handle || 0 != getsockopt(handle, SOL_SOCKET, SO_TYPE, (char *)&(memory->_socketType), (socklen_t *)&typeSize))
		memory->_socketType = 0;
	memory->_perform = callback;
	memory->_callbackTypes = cbtype;
	memory->_socket = handle;
    
	if (INVALID_SOCKET != handle) RSDictionarySetValue(__RSAllSockets, (void *)(uintptr_t)handle, memory);
    if (nil == __RSSocketManagerThread) __RSSocketManagerThread = __RSStartSimpleThread(__RSSocketManager, 0);
    __RSSocketUnlockAll();
	if (nil != context)
	{
        void *contextInfo = context->retain ? (void *)context->retain(context->context) : context->context;
        __RSSocketLock(memory);
        memory->_context.retain = context->retain;
        memory->_context.release = context->release;
        memory->_context.desciption = context->desciption;
        memory->_context.context = contextInfo;
        __RSSocketUnlock(memory);
    }
	__RSSocketSetValid(memory);
	return memory;
}

RSExport RSSocketRef RSSocketCreate(RSAllocatorRef allocator, RSBit32 protocolFamily, RSBit32 socketType, RSBit32 protocol,RSSocketCallBackType type, RSSocketCallBack callback, const RSSocketContext *context)
{
	RSSocketSignature signature = {protocolFamily, socketType, protocol, nil};
	return RSSocketCreateWithSocketSignature(allocator, &signature, type, callback, context);
}

RSExport RSSocketRef RSSocketCreateWithHandle(RSAllocatorRef allocator, RSSocketHandle handle, RSSocketCallBackType type, RSSocketCallBack callback, const RSSocketContext *context)
{
	return __RSSocketCreateInstance(allocator, handle, type, callback, context, NO);
}

RSExport RSSocketRef RSSocketCreateWithSocketSignature(RSAllocatorRef allocator, const RSSocketSignature* signature, RSSocketCallBackType type, RSSocketCallBack callback, const RSSocketContext *context)
{
	RSSocketHandle handle = __RSSocketCreateHandle(signature);
	if (INVALID_SOCKET != handle)
	{
		return __RSSocketCreateInstance(allocator, handle, type, callback, context, NO);
	}
	return nil;
}

RSExport RSSocketRef RSSocketCreateCopy(RSAllocatorRef allocator, RSSocketRef s)
{
	return (RSSocketRef)RSRetain(s);
}

RSExport RSSocketError RSSocketSetAddress(RSSocketRef s, RSDataRef address)
{
	__RSGenericValidInstance(s, _RSSocketTypeID);
	RSSocketError err = RSSocketSuccess;
	if (nil == address) return RSSocketUnSuccess;
	if (!RSSocketIsValid(s)) return RSSocketUnSuccess;
	RSSpinLockLock(&s->_lock);
	RSSocketCallBackType type = s->_callbackTypes;
	if (type & RSSocketAcceptCallBack)
	{
		struct sockaddr *name;
		socklen_t namelen;
		RSSocketHandle handle = RSSocketGetHandle(s);
		name = (struct sockaddr *)RSDataGetBytesPtr(address);
		namelen = (socklen_t)RSDataGetLength(address);
		int yes = 1;
		setsockopt(handle, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes));
		int result = bind(handle, name, namelen);
		if (0 == result)
		{
			listen(handle, 256);
		}
	}
	else if (type & RSSocketConnectCallBack)
	{
        if (s->_peerAddress) RSRelease(s->_peerAddress);
        s->_peerAddress = RSRetain(address);
	}
	RSSpinLockUnlock(&s->_lock);
	return err;
}

RSExport RSSocketError RSSocketSendData(RSSocketRef s, RSDataRef address, RSDataRef data, RSTimeInterval timeout)
{
    const RSBitU8 *dataptr, *addrptr = NULL;
    RSBit32 datalen, addrlen = 0, size = 0;
    RSSocketHandle sock = INVALID_SOCKET;
    struct timeval tv;
    __RSGenericValidInstance(s, RSSocketGetTypeID());
    if (address)
	{
        addrptr = RSDataGetBytesPtr(address);
        addrlen = (RSBit32)RSDataGetLength(address);
    }
    dataptr = RSDataGetBytesPtr(data);
    datalen = (RSBit32)RSDataGetLength(data);
    if (RSSocketIsValid(s)) sock = RSSocketGetHandle(s);
    if (INVALID_SOCKET != sock)
	{
        RSRetain(s);
        __RSSocketWriteLock(s);
        tv.tv_sec = (timeout <= 0.0 || (RSTimeInterval)INT_MAX <= timeout) ? INT_MAX : (int)floor(timeout);
        tv.tv_usec = (int)floor(1.0e+6 * (timeout - floor(timeout)));
        setsockopt(sock, SOL_SOCKET, SO_SNDTIMEO, (char *)&tv, sizeof(tv));	// cast for WinSock bad API
        if (NULL != addrptr && 0 < addrlen)
		{
            size = (RSBit32)sendto(sock, (char *)dataptr, datalen, 0, (struct sockaddr *)addrptr, addrlen);
        }
		else
		{
            size = (RSBit32)send(sock, (char *)dataptr, datalen, 0);
        }
#if defined(LOG_RSSOCKET)
        __RSCLog(RSLogLevelNotice, "wrote %ld bytes to socket %d\n", (long)size, sock);
#endif
        __RSSocketWriteUnlock(s);
        RSRelease(s);
        if (size <= 0 && datalen) {
            RSSocketInvalidate(s);
        }
    }
    return (size > 0) ? RSSocketSuccess : RSSocketUnSuccess;
}

RSExport RSSocketError RSSocketRecvData(RSSocketRef s, RSMutableDataRef data)
{
	RSSocketHandle sock = (RSSocketIsValid(s)) ? RSSocketGetHandle(s) : INVALID_SOCKET;
	if (INVALID_SOCKET != sock)
	{
		RSRetain(s);
		__RSSocketLock(s);
		if (s->_dataReadQueue)
		{
			if (RSQueueGetCount(s->_dataReadQueue))
			{
				RSDataRef recvData = RSQueueDequeue(s->_dataReadQueue);
				RSDataAppend(data, recvData);
				RSRelease(recvData);
				__RSSocketUnlock(s);
				RSRelease(s);
				return RSSocketSuccess;
			}
		}
		__RSSocketUnlock(s);
		RSRelease(s);
	}
	return RSSocketUnSuccess;
}

RSExport RSSocketError RSSocketConnectToAddress(RSSocketRef s, RSDataRef address, RSTimeInterval timeout)
{
	if (s == nil || address == nil) return RSSocketUnSuccess;
	RSSocketHandle sock = RSSocketGetHandle(s);
	const uint8_t *name = NULL;
	RSBit32 namelen = 0, result = -1, connect_err = 0, select_err = 0;
	RSBitU32 yes = 1, no = 0;
	BOOL wasBlocking = YES;
	if (sock != INVALID_SOCKET)
	{
		name = RSDataGetBytesPtr(address);
		namelen = (RSBit32)RSDataGetLength(address);
#if DEPLOYMENT_TARGET_MACOSX || DEPLOYMENT_TARGET_EMBEDDED || DEPLOYMENT_TARGET_EMBEDDED_MINI
        RSBit32 flags = fcntl(sock, F_GETFL, 0);
        if (flags >= 0) wasBlocking = ((flags & O_NONBLOCK) == 0);
        if (wasBlocking && (timeout > 0.0 || timeout < 0.0)) ioctlsocket(sock, FIONBIO, (u_long *)&yes);
#else
        // You can set but not get this flag in WIN32, so assume it was in non-blocking mode.
        // The downside is that when we leave this routine we'll leave it non-blocking,
        // whether it started that way or not.
        SInt32 flags = 0;
        if (timeout > 0.0 || timeout < 0.0) ioctlsocket(sock, FIONBIO, (u_long *)&yes);
        wasBlocking = NO;
#endif
        result = connect(sock, (struct sockaddr *)name, namelen);
        if (result != 0)
        {
            connect_err = __RSSocketLastError();
#if DEPLOYMENT_TARGET_WINDOWS
            if (connect_err == WSAEWOULDBLOCK) connect_err = EINPROGRESS;
#endif
        }
#if defined(LOG_RSSOCKET)
        __RSCLog(RSLogLevelNotice, "connection attempt returns %d error %d on socket %d (flags 0x%x blocking %d)\n", result, connect_err, sock, flags, wasBlocking);
#endif
        if (EINPROGRESS == connect_err && timeout >= 0.0)
        {
            /* select on socket */
            SInt32 nrfds;
            int error_size = sizeof(select_err);
            struct timeval tv;
            RSMutableDataRef fds = RSDataCreateMutable(RSAllocatorSystemDefault, 0);
            __RSSocketFdSet(sock, fds);
            tv.tv_sec = (timeout <= 0.0 || (RSTimeInterval)INT_MAX <= timeout) ? INT_MAX : (int)floor(timeout);
            tv.tv_usec = (int)floor(1.0e+6 * (timeout - floor(timeout)));
            nrfds = select((int)__RSSocketFdGetSize(fds), NULL, (fd_set *)RSDataGetBytesPtr(fds), NULL, &tv);
            if (nrfds < 0)
			{
                select_err = __RSSocketLastError();
                result = -1;
            }
			else if (nrfds == 0)
			{
                result = -2;
            }
			else
			{
                if (0 != getsockopt(sock, SOL_SOCKET, SO_ERROR, (char *)&select_err, (socklen_t *)&error_size))
                    select_err = 0;
                result = (select_err == 0) ? 0 : -1;
            }
            RSRelease(fds);
#if defined(LOG_RSSOCKET)
            __RSCLog(RSLogLevelNotice, "timed connection attempt %s on socket %d, result %d, select returns %d error %d\n", (result == 0) ? "succeeds" : "fails", sock, result, nrfds, select_err);
#endif
        }
        if (wasBlocking && (timeout > 0.0 || timeout < 0.0)) ioctlsocket(sock, FIONBIO, (u_long *)&no);
        if (EINPROGRESS == connect_err && timeout < 0.0)
		{
            result = 0;
#if defined(LOG_RSSOCKET)
            __RSCLog(RSLogLevelNotice, "connection attempt continues in background on socket %d\n", sock);
#endif
        }
    }
	return result;
}

RSExport void RSSocketInvalidate(RSSocketRef s)
{
    if (!s) HALTWithError(RSInvalidArgumentException, "socket is nil");
    if (!RSSocketIsValid(s)) return;
    RSBitU64 previousSocketManagerIteration;
    __RSGenericValidInstance(s, RSSocketGetTypeID());
#if defined(LOG_RSSOCKET)
    __RSCLog(RSLogLevelNotice, "invalidating socket %d\n", s->_socket);
#endif
    RSRetain(s);
    RSSpinLockLock(&__RSAllSocketsLock);
    __RSSocketLock(s);
    if (__RSSocketIsValid(s))
	{
        RSIndex idx;
        RSRunLoopSourceRef source0;
        void *contextInfo = NULL;
        void (*contextRelease)(const void *info) = NULL;
        __RSSocketUnsetValid(s);
        
        RSSpinLockLock(&__RSActiveSocketsLock);
        idx = RSArrayIndexOfObject(__RSWriteSockets, s);
        if (0 <= idx) {
            RSArrayRemoveObjectAtIndex(__RSWriteSockets, idx);
            __RSSocketClearFDForWrite(s);
        }
        // No need to clear FD's for V1 sources, since we'll just throw the whole event away
        idx = RSArrayIndexOfObject(__RSReadSockets, s);
        if (0 <= idx) {
            RSArrayRemoveObjectAtIndex(__RSReadSockets, idx);
            __RSSocketClearFDForRead(s);
        }
        previousSocketManagerIteration = __RSSocketManagerIteration;
        RSSpinLockUnlock(&__RSActiveSocketsLock);
        RSDictionaryRemoveValue(__RSAllSockets, (void *)(uintptr_t)(s->_socket));
        if ((1) != 0) closesocket(s->_socket);
        s->_socket = INVALID_SOCKET;
        if (nil != s->_peerAddress)
		{
            RSRelease(s->_peerAddress);
            s->_peerAddress = nil;
        }
        if (nil != s->_dataReadQueue)
		{
            RSRelease(s->_dataReadQueue);
            s->_dataReadQueue = nil;
        }
		if (nil != s->_dataWriteQueue)
		{
            RSRelease(s->_dataWriteQueue);
            s->_dataWriteQueue = nil;
        }
        if (nil != s->_addressQueue)
		{
            RSRelease(s->_addressQueue);
            s->_addressQueue = nil;
        }
        s->_schedule = 0;
        
        // we'll need this later
		
        source0 = s->_source0;
        s->_source0 = nil;
        contextInfo = s->_context.context;
        contextRelease = s->_context.release;
        s->_context.context = 0;
        s->_context.retain = 0;
        s->_context.release = 0;
        s->_context.desciption = 0;
        __RSSocketUnlock(s);
        
        // Do this after the socket unlock to avoid deadlock (10462525)
		
		
        if (nil != contextRelease)
		{
            contextRelease(contextInfo);
        }
        if (nil != source0)
		{
            RSRelease(source0);
        }
    }
	else
	{
        __RSSocketUnlock(s);
    }
    RSSpinLockUnlock(&__RSAllSocketsLock);
    RSRelease(s);
}

static void __RSSocketEnableCallBacks(RSSocketRef s, RSOptionFlags callBackTypes, BOOL force, uint8_t wakeupChar)
{
    BOOL wakeup = NO;
    if (!callBackTypes)
	{
        __RSSocketUnlock(s);
        return;
    }
    if (__RSSocketIsValid(s) && __RSSocketIsScheduled(s))
	{
        BOOL turnOnWrite = NO, turnOnConnect = NO, turnOnRead = NO;
        uint8_t readCallBackType = __RSSocketReadCallBackType(s);
        callBackTypes &= __RSSocketCallBackTypes(s);
        
#if defined(LOG_RSSOCKET)
        __RSCLog(RSLogLevelNotice, "rescheduling socket %d for types 0x%llx\n", s->_socket,callBackTypes);
#endif
        /* We will wait for connection only for connection-oriented, non-rendezvous sockets that are not already connected.  Mark others as already connected. */
        if ((readCallBackType == RSSocketAcceptCallBack) || !__RSSocketIsConnectionOriented(s)) s->_flag._untouchable._connected = YES;
        //
        //        // First figure out what to turn on
        if (s->_flag._untouchable._connected || (callBackTypes & RSSocketConnectCallBack) == 0)
		{
            //            // if we want write callbacks and they're not disabled...
            if ((callBackTypes & RSSocketWriteCallBack) != 0 && (s->_flag._untouchable._write == 0)) turnOnWrite = YES;
        }
		else
		{
            //            // if we want connect callbacks and they're not disabled...
            if ((callBackTypes & RSSocketConnectCallBack) != 0 && (s->_flag._untouchable._connect == 0)) turnOnConnect = YES;
        }
        //        // if we want read callbacks and they're not disabled...
        if (readCallBackType != RSSocketNoCallBack &&
			(callBackTypes & readCallBackType) != 0 &&
			((s->_flag._untouchable._read & RSSocketReadCallBack) == 0 ||
			 (s->_flag._untouchable._accept & RSSocketAcceptCallBack) == 0))
			turnOnRead = YES;
		
        // Now turn on the callbacks we've determined that we want on
        if (turnOnRead || turnOnWrite || turnOnConnect)
		{
            RSSpinLockLock(&__RSActiveSocketsLock);
            if (turnOnWrite || turnOnConnect)
			{
                if (force)
				{
                    RSIndex idx = RSArrayIndexOfObject(__RSWriteSockets, s);
                    if (RSNotFound == idx) RSArrayAddObject(__RSWriteSockets, s);
					//                     if (RSNotFound == idx) RSLog(5, RSSTR("__RSSocketEnableCallBacks: put %p in __RSWriteSockets list due to force and non-presence"), s);
                }
                if (__RSSocketSetFDForWrite(s)) wakeup = YES;
            }
            if (turnOnRead)
			{
                if (force)
				{
                    RSIndex idx = RSArrayIndexOfObject(__RSReadSockets, s);
                    if (RSNotFound == idx) RSArrayAddObject(__RSReadSockets, s);
                }
                if (__RSSocketSetFDForRead(s)) wakeup = YES;
            }
            RSSpinLockUnlock(&__RSActiveSocketsLock);
        }
    }
    __RSSocketUnlock(s);
}

RSExport void RSSocketReschedule(RSSocketRef s)
{
    __RSGenericValidInstance(s, _RSSocketTypeID);
    if (!RSSocketIsValid(s)) return;
    __RSSocketLock(s);
    __RSSocketEnableCallBacks(s, __RSSocketCallBackTypes(s), YES, 's');
}

static void __RSSocketSchedule(void* info, RSRunLoopRef rl, RSStringRef mode)
{
    RSLog(RSSTR("__RSSocketSchedule called."));
	RSSocketRef s = (RSSocketRef)info;
    __RSSocketLock(s);
    //??? also need to arrange delivery of all pending data
    if (__RSSocketIsValid(s))
	{
        
        __RSSocketScheduleStart(s);
        // Since the v0 source is listened to on the SocketMgr thread, no matter how many modes it
        // is added to we just need to enable it there once (and _socketSetCount gives us a refCount
        // to know when we can finally disable it).
        if (1 == __RSSocketIsScheduled(s))
		{
#if defined(LOG_RSSOCKET)
            __RSCLog(RSLogLevelNotice, "scheduling socket %d\n", s->_socket);
#endif
			// RSLog(5, RSSTR("__RSSocketSchedule(%p, %p, %p)"), s, rl, mode);
            //            static void __RSSocketEnableCallBacks(RSSocketRef s, RSOptionFlags callBackTypes, BOOL force, uint8_t wakeupChar)
            __RSSocketEnableCallBacks(s, __RSSocketCallBackTypes(s), YES, 's');  // unlocks s
        }
		else
            __RSSocketUnlock(s);
    }
	else
        __RSSocketUnlock(s);
}

static void __RSSocketCancel(void* info, RSRunLoopRef rl, RSStringRef mode)
{
    RSLog(RSSTR("__RSSocketCancel called."));
	RSSocketRef s = (RSSocketRef)info;
    RSBit32 idx;
    __RSSocketLock(s);
    __RSSocketScheduleDone(s);
    if (!__RSSocketIsScheduled(s))
	{
        RSSpinLockLock(&__RSActiveSocketsLock);
        idx = (RSBit32)RSArrayIndexOfObject(__RSWriteSockets, s);
        if (0 <= idx)
		{
			// RSLog(5, RSSTR("__RSSocketCancel: removing %p from __RSWriteSockets list"), s);
            RSArrayRemoveObjectAtIndex(__RSWriteSockets, idx);
            __RSSocketClearFDForWrite(s);
        }
        idx = (RSBit32)RSArrayIndexOfObject(__RSReadSockets, s);
        if (0 <= idx)
		{
            RSArrayRemoveObjectAtIndex(__RSReadSockets, idx);
            __RSSocketClearFDForRead(s);
        }
        RSSpinLockUnlock(&__RSActiveSocketsLock);
    }
    if (NULL != s->_source0)
	{
		s->_source0 = nil;
    }
    __RSSocketUnlock(s);
}

static void __RSSocketPerformV0(void* info, RSSocketCallBackType type)
{
    RSLog(RSSTR("__RSSocketPerform called."));
    RSSocketRef s = (RSSocketRef)info;
    __RSGenericValidInstance(s, _RSSocketTypeID);
	__RSSocketLock(s);
    if (!__RSSocketIsValid(s))
	{
		__RSSocketUnlock(s);
		return;
	}
	RSDataRef data = nil;
	RSDataRef address = nil;
	RSSocketHandle sock = INVALID_SOCKET;
	if (RSSocketReadCallBack == type)
	{
        if (NULL != s->_dataReadQueue && 0 < RSQueueGetCount(s->_dataReadQueue))
		{
            data = (RSDataRef)RSQueueDequeue(s->_dataReadQueue);
            address = (RSDataRef)RSQueueDequeue(s->_addressQueue);
        }
    }
	else if (RSSocketAcceptCallBack == type)
	{
        if (NULL != s->_dataReadQueue && 0 < RSQueueGetCount(s->_dataReadQueue))
		{
            data = (RSDataRef)RSQueueDequeue(s->_dataReadQueue);
			if (sizeof(RSSocketHandle) == RSDataGetLength(data))
			{
				__builtin_memcpy(&sock, RSDataGetBytesPtr(data), sizeof(RSSocketHandle));
			}
            address = (RSDataRef)RSQueueDequeue(s->_addressQueue);
        }
    }
//    RSPerformBlockInBackGroundWaitUntilDone(^{
        __RSSocketDoCallback(s, type, data, address, sock);
        if (data) RSRelease(data);
        if (address) RSRelease(address);
        
        if (type & RSSocketConnectCallBack)
        {
            if (s->_flag._untouchable._connect) return;
            
        }
        else if (type & RSSocketAcceptCallBack)
        {
            if (s->_flag._untouchable._accept) return;
            
        }
        else if (type & RSSocketDataCallBack)
        {
            if (s->_flag._untouchable._data) return;
            //        if (s->_perform)
            //            s->_perform(s, RSSocketDataCallBack, s->_peerAddress, nil, s->_context.context);
        }
//    });
}


RSExport RSRunLoopSourceRef	RSSocketCreateRunLoopSource(RSAllocatorRef allocator, RSSocketRef s, RSIndex order)
{
    __RSSocketLock(s);
    __RSGenericValidInstance(s, _RSSocketTypeID);
    RSRunLoopSourceRef source = nil;
    if (__RSSocketIsValid(s))
    {
        if (s->_source0 != nil && !RSRunLoopSourceIsValid(s->_source0))
        {
            s->_source0 = nil;
        }
        if (s->_source0 == nil)
        {
            RSRunLoopSourceContext RSSocketSourceContext =
            {
                0,
                s,              // information
                nil,
                nil,
                RSDescription,
                nil,
                nil,
                __RSSocketSchedule,     //void	(*schedule)(void *info, RSRunLoopRef rl, RSStringRef mode);
                nil,       //void	(*cancel)(void *info, RSRunLoopRef rl, RSStringRef mode);
                nil                    //void	(*perform)(void *info); so the runloop can not run this source but sechdule will be called!
            };
            s->_source0 = RSRunLoopSourceCreate(RSAllocatorSystemDefault, order, &RSSocketSourceContext);
        }
		source = (RSRunLoopSourceRef)RSRetain(s->_source0);   // This retain is for the receiver
    }
    __RSSocketUnlock(s);
    return source;
}