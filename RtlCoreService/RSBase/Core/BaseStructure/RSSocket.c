//
//  RSSocket.c
//  RSCoreFoundation
//
//  Created by RetVal on 11/18/12.
//  Copyright (c) 2012 RetVal. All rights reserved.
//

//#include <RSCoreFoundation/RSDictionary.h>
//#include <RSCoreFoundation/RSNil.h>
//#include <RSCoreFoundation/RSNumber.h>
//#include <RSCoreFoundation/RSQueue.h>
//#include <RSCoreFoundation/RSSocket.h>
//#include <RSCoreFoundation/RSRuntime.h>
//#include "RSPrivate/RSPacket/RSPacket.h"
//
//#if DEPLOYMENT_TARGET_MACOSX || DEPLOYMENT_TARGET_EMBEDDED || DEPLOYMENT_TARGET_EMBEDDED_MINI
//#include <sys/sysctl.h>
//#include <sys/un.h>
//#include <libc.h>
//#include <dlfcn.h>
//#endif
//#include <errno.h>
//#include <stdio.h>
//
//#define SOCKET_ERROR    (-1)
//#define SOCKET_SUCCESS  (0)
//
//#if DEPLOYMENT_TARGET_MACOSX || DEPLOYMENT_TARGET_EMBEDDED || DEPLOYMENT_TARGET_EMBEDDED_MINI
//#define INVALID_SOCKET (RSSocketHandle)(-1)
//#define closesocket(a) close((a))
//#define ioctlsocket(a,b,c) ioctl((a),(b),(c))
//#endif
//
//#define __RSSOCKET_V_0  0
//#define __RSSOCKET_V_1  1
//#define __RSSOCKET_V_2  2
//#define LOG_RSSOCKET
//
//RSInline int __RSSocketLastError(void) {
//#if DEPLOYMENT_TARGET_WINDOWS
//    return WSAGetLastError();
//#else
//    return errno;
//#endif
//}
//
//static RSCBuffer __RSSocketGetLocalName(RSBlock name[RSBufferSize])
//{
//    if (name == nil) return nil;
//    memset(name, 0, RSBufferSize*sizeof(RSBlock));
//    gethostname(name, RSBufferSize);
//    return name;
//}
//
//RSExport RSStringRef RSSocketGetLocalName()
//{
//    RSBlock hostName[RSBufferSize] = {0};
//    return RSStringCreateWithCString(RSAllocatorSystemDefault, __RSSocketGetLocalName(hostName), RSStringEncodingUTF8);
//}
//
//static RSCBuffer __RSSocketGetLocalIp(RSBlock ip[32])
//{
//    if (ip == nil) return nil;
//    RSBlock hostName[RSBufferSize] = {0};
//	struct hostent* host = nil;
//	host = (struct hostent*)gethostbyname((const char*)__RSSocketGetLocalName(hostName));
//    __RSCLog(RSLogLevelNotice, "%s\n", hostName);
//    if (host) {
//        snprintf(ip, 32, "%d.%d.%d.%d",
//                 (int)(host->h_addr_list[0][0]&0x00ff),
//                 (int)(host->h_addr_list[0][1]&0x00ff),
//                 (int)(host->h_addr_list[0][2]&0x00ff),
//                 (int)(host->h_addr_list[0][3]&0x00ff));
//		
//    }
//    else
//    {
//        if (hostName)
//        {
//            RSStringRef _ipStr = RSStringCreateWithCString(RSAllocatorSystemDefault, hostName, RSStringEncodingUTF8);
//            RSStringRef ipAddrStr = nil;
//            if (YES == RSStringHasPrefix(_ipStr, RSSTR("ip-")))
//            {
//                ipAddrStr = RSStringCreateSubStringWithRange(RSAllocatorSystemDefault, _ipStr, RSMakeRange(3, 2));
//            }
//            RSRelease(_ipStr);
//            if (ipAddrStr)
//            {
//                
//                RSRelease(ipAddrStr);
//            }
//        }
//    }
//    return	ip;
//}
//
//RSExport RSStringRef RSSocketGetLocalIp()
//{
//    RSBlock ip[32];
//    return RSStringCreateWithCString(RSAllocatorSystemDefault, __RSSocketGetLocalIp(ip), RSStringEncodingUTF8);
//}
//
//static RSArrayRef __RSSocketGetLocalIps(RSStringRef host, RSStringRef service, RSBitU32 family, RSBitU32 protocol)
//{
//    if (!host || !service) return nil;
//    struct addrinfo hints, *res, *ressave;
//    RSZeroMemory(&hints, sizeof(struct addrinfo));
//    hints.ai_family = family;
//    hints.ai_flags = AI_CANONNAME;
//    hints.ai_socktype = protocol;
//    if (getaddrinfo(RSStringGetCStringPtr(host, RSStringEncodingUTF8), RSStringGetCStringPtr(service, RSStringEncodingUTF8), &hints, &res)) return nil;
//    ressave = res;
//    RSMutableArrayRef results = RSArrayCreateMutable(RSAllocatorSystemDefault, 0);
//    do {
//        RSStringRef addrStr = RSStringCreateWithCString(RSAllocatorSystemDefault, res->ai_canonname, RSStringEncodingUTF8);
//        RSArrayAddObject(results, addrStr);
//        RSRelease(addrStr);
//    } while ((res = res->ai_next));
//    freeaddrinfo(ressave);
//    return results;
//}
//
//RSExport RSArrayRef RSSocketGetLocalIps()
//{
//    return __RSSocketGetLocalIps(RSSTR(""), RSSTR(""), SOCK_STREAM, IPPROTO_TCP);
//}
//
//RSExport RSDictionaryRef RSSocketGetAddressInfo(struct sockaddr* addr, socklen_t addr_len)
//{
//    RSBlock h[RSBufferSize] = {0}, s[RSBufferSize] = {0};
//    if (!getnameinfo(addr, addr_len, h, RSBufferSize, s, RSBufferSize, 0))
//        return RSAutorelease(RSDictionaryCreateWithObjectsAndOKeys(RSAllocatorSystemDefault, RSAutorelease(RSStringCreateWithCString(RSAllocatorSystemDefault, h, RSStringEncodingUTF8)), RSSTR("hostName"), RSAutorelease(RSStringCreateWithCString(RSAllocatorSystemDefault, s, RSStringEncodingUTF8)), RSSTR("serverName"), nil));
//    return nil;
//    
//}
//
//#define MAX_SOCKADDR_LEN 256
//#define MAX_DATA_SIZE 65535
//#define MAX_CONNECTION_ORIENTED_DATA_SIZE 32768
//static RSSpinLock __RSAllSocketsLock = RSSpinLockInit;
//static RSMutableDictionaryRef __RSAllSockets = nil;	// <socket handle, RSSocket>
//
//static RSSpinLock __RSActiveSocketsLock = RSSpinLockInit;	//
//static RSMutableArrayRef __RSWriteSockets = nil;	// writable sockets
//static RSMutableArrayRef __RSReadSockets = nil;		// readable sockets
//static RSMutableDataRef __RSWriteSocketsFds = nil;	// fd_set*
//static RSMutableDataRef __RSReadSocketsFds = nil;	// fd_set*
//
//static RSSocketHandle __RSWakeupSocketPair[2] = {INVALID_SOCKET};	// for initialize the socket manager, and dispatch the task sechdule.
//
//static BOOL __RSReadSocketsTimeoutInvalid = YES;
//static void *__RSSocketManagerThread = nil;
//
//
//struct __RSSocket {
//    RSRuntimeBase _base;
//    RSSpinLock _lock;
//    RSSpinLock _rwLock;
//    RSSocketHandle _socket;
//    RSSocketCallBackType _callbackTypes;
//    RSSocketCallBack _perform;
//    RSQueueRef _dataReadQueue;
//    RSQueueRef _addressQueue;
//    RSQueueRef _dataWriteQueue;
//    
//    RSDataRef  _address;
//    RSDataRef  _peerAddress;
//    struct timeval _readBufferTimeout;
//    RSRunLoopSourceRef _source0;
//    RSSocketContext _context;
//    RSBitU32 _socketType;
//    RSBitU32 _errorCode;
//    RSBitU32 _schedule;
//    struct {
//        struct {
//            char _read      : 1;
//            char _accept    : 1;
//            char _data      : 1;
//            char _connect   : 1;
//            char _write     : 1;
//            char _error     : 1;
//            char _connected : 1;
//            char _reserved  : 1;
//        }_untouchable;
//    }_flag;
//};
//
//RSInline void __RSSocketSetValid(RSSocketRef s)
//{
//    s->_base._rsinfo._rsinfo1 |= 0x1;
//}
//
//RSInline void __RSSocketUnsetValid(RSSocketRef s)
//{
//    s->_base._rsinfo._rsinfo1 &= ~0x1;
//}
//
//RSInline BOOL __RSSocketIsValid(RSSocketRef s)
//{
//    return (s->_base._rsinfo._rsinfo1 & 0x1) == 0x1;
//}
//
//RSInline void __RSSocketLock(RSSocketRef s) {
//    RSSpinLockLock(&(s->_lock));
//}
//
//RSInline void __RSSocketUnlock(RSSocketRef s) {
//    RSSpinLockUnlock(&(s->_lock));
//}
//
//RSInline void __RSSocketWriteLock(RSSocketRef s)
//{
//	RSSpinLockLock(&s->_rwLock);
//}
//
//RSInline void __RSSocketWriteUnlock(RSSocketRef s)
//{
//	RSSpinLockUnlock(&s->_rwLock);
//}
//
//RSInline RSBitU32 __RSSocketIsScheduled(RSSocketRef s)
//{
//    return s->_schedule;
//}
//
//RSInline void __RSSocketScheduleStart(RSSocketRef s)
//{
//    ++s->_schedule;
//}
//
//RSInline void __RSSocketScheduleDone(RSSocketRef s)
//{
//    if (__RSSocketIsScheduled(s)) --s->_schedule;
//}
//
//RSInline BOOL __RSSocketIsConnected(RSSocketRef s)
//{
//    return s->_flag._untouchable._connected;
//}
//
//RSInline void __RSSocketSetConnectedWithFlag(RSSocketRef s, BOOL flag)
//{
//    s->_flag._untouchable._connected = flag;
//}
//
//RSInline BOOL __RSSocketIsConnectionOriented(RSSocketRef s)
//{
//    return (SOCK_STREAM == s->_socketType || SOCK_SEQPACKET == s->_socketType);
//}
//
//RSInline RSOptionFlags __RSSocketCallBackTypes(RSSocketRef s)
//{
//    return s->_callbackTypes;
//}
//
//RSInline RSOptionFlags __RSFlagRemoveX(RSOptionFlags flags, RSOptionFlags f)
//{
//    return flags = (flags &= ~f);
//}
//
//RSInline RSOptionFlags __RSSocketEnabledCallTypes(RSSocketRef s)
//{
//    RSOptionFlags flags = __RSSocketCallBackTypes(s);
//    if (s->_flag._untouchable._read) flags &= ~RSSocketReadCallBack;
//    if (s->_flag._untouchable._write) flags &= ~RSSocketWriteCallBack;
//    if (s->_flag._untouchable._connect) flags &= ~RSSocketConnectCallBack;
//    if (s->_flag._untouchable._accept) flags &= ~RSSocketAcceptCallBack;
//    if (s->_flag._untouchable._data) flags &= ~RSSocketDataCallBack;
//    if (s->_flag._untouchable._error) flags &= ~RSSocketUnSuccessCallBack;
//    return flags;
//}
//
//RSInline void __RSSocketEnableCallTypes(RSSocketRef s, RSOptionFlags enable)
//{
//    if (enable & RSSocketReadCallBack) s->_flag._untouchable._read = NO;
//    if (enable & RSSocketWriteCallBack) s->_flag._untouchable._write = NO;
//    if (enable & RSSocketConnectCallBack) s->_flag._untouchable._connect = NO;
//    if (enable & RSSocketAcceptCallBack) s->_flag._untouchable._accept = NO;
//    if (enable & RSSocketDataCallBack) s->_flag._untouchable._data = NO;
//    if (enable & RSSocketUnSuccessCallBack) s->_flag._untouchable._error = NO;
//    return;
//}
//
//RSInline RSOptionFlags __RSSocketReadCallBackType(RSSocketRef s)
//{
//    return __RSFlagRemoveX(__RSFlagRemoveX(__RSFlagRemoveX(__RSSocketCallBackTypes(s), RSSocketWriteCallBack), RSSocketConnectCallBack), RSSocketUnSuccessCallBack);
//}
//
//RSInline RSOptionFlags __RSSocketWriteCallBackType(RSSocketRef s)
//{
//    return __RSFlagRemoveX(__RSFlagRemoveX(__RSFlagRemoveX(__RSSocketCallBackTypes(s), RSSocketReadCallBack), RSSocketAcceptCallBack), RSSocketUnSuccessCallBack);
//}
//
//void __RSSocketClassInit(RSTypeRef obj)
//{
//    RSSocketRef memory = (RSSocketRef)obj;
//    memory->_lock = RSSpinLockInit;
//    memory->_rwLock = RSSpinLockInit;
//    memory->_socket = INVALID_SOCKET;
//    memory->_callbackTypes = RSSocketNoCallBack;
//	memory->_dataReadQueue = nil;
//	memory->_dataWriteQueue = nil;
//	memory->_source0 = nil;
//	memory->_address = nil;
//	memory->_peerAddress = nil;
//	timerclear(&memory->_readBufferTimeout);
//    memset(&memory->_context, 0, sizeof(RSSocketContext));
//    __RSSocketSetValid(memory);
//    return;
//}
//
//static RSTypeRef __RSSocketClassCopy(RSAllocatorRef allocator, RSTypeRef rs, BOOL mutableCopy)
//{
//    RSSocketRef copy = RSSocketCreateCopy(allocator, (RSSocketRef)rs);
//    return copy;
//}
//
//static void __RSSocketClassDeallocate(RSTypeRef obj)
//{
//    RSSocketRef s = (RSSocketRef)obj;
//	__RSSocketLock(s);
//    if (s->_address) RSRelease(s->_address);
//    if (s->_peerAddress) RSRelease(s->_peerAddress);
//    if (s->_dataReadQueue) RSRelease(s->_dataReadQueue);
//	if (s->_dataWriteQueue) RSRelease(s->_dataWriteQueue);
//    if (s->_context.release && s->_context.retain && s->_context.context)
//		s->_context.release(s->_context.context);
//    if (s->_source0) RSRelease(s->_source0);
//    if (INVALID_SOCKET != s->_socket && s->_socket > 0)
//	{
//        closesocket(s->_socket);
//		s->_socket = INVALID_SOCKET;
//    }
//	__RSSocketUnlock(s);
//    s->_lock = RSSpinLockInit;
//    
//    return;
//}
//
//static BOOL __RSSocketClassEqual(RSTypeRef obj1, RSTypeRef obj2)
//{
//    if (obj1 == obj2) return YES;
//    RSSocketRef s1 = (RSSocketRef)obj1;
//    RSSocketRef s2 = (RSSocketRef)obj2;
//	if (s1->_socketType != s2->_socketType) return NO;
//	if (__builtin_memcmp(&s1->_readBufferTimeout, &s2->_readBufferTimeout, sizeof(struct timeval))) return NO;
//	if (s1->_perform != s2->_perform) return NO;
//	if (s1->_socket != s2->_socket) return NO;
//    if (!RSEqual(s1->_address, s2->_address)) return NO;
//    if (!RSEqual(s1->_peerAddress, s2->_peerAddress)) return NO;
//    return YES;
//}
//
//static RSHashCode __RSSocketHash(RSTypeRef obj)
//{
//    RSSocketRef s = (RSSocketRef)obj;
//    return RSHash(s->_address)^RSHash(s->_peerAddress);
//}
//
//static const RSRuntimeClass __RSSocketClass = {
//    _RSRuntimeScannedObject,
//    "RSSocket",
//    __RSSocketClassInit,
//    nil,
//    __RSSocketClassDeallocate,
//    __RSSocketClassEqual,
//    __RSSocketHash,
//    nil,
//    nil,
//    nil,
//};
//
//static RSTypeID _RSSocketTypeID = _RSRuntimeNotATypeID;
//
//
//RSExport RSTypeID RSSocketGetTypeID()
//{
//    return _RSSocketTypeID;
//}
//
//RSExport RSSocketHandle RSSocketGetHandle(RSSocketRef s)
//{
//	if (s == nil) return INVALID_SOCKET;
//    __RSGenericValidInstance(s, _RSSocketTypeID);
//    return s->_socket;
//}
//
//static BOOL __RSSocketHandleIsValid(RSSocketHandle sock)
//{
//#if DEPLOYMENT_TARGET_WINDOWS
//    SInt32 errorCode = 0;
//    int errorSize = sizeof(errorCode);
//    return !(0 != getsockopt(sock, SOL_SOCKET, SO_ERROR, (char *)&errorCode, &errorSize) && __RSSocketLastError() == WSAENOTSOCK);
//#else
//    RSBit32 flags = fcntl(sock, F_GETFL, 0);
//	int error = __RSSocketLastError();
//    return !(0 > flags && EBADF == (error));
//#endif
//}
//
//
//RSExport BOOL RSSocketIsValid(RSSocketRef s)
//{
//	__RSGenericValidInstance(s, _RSSocketTypeID);
//    return __RSSocketIsValid(s) && __RSSocketHandleIsValid(s->_socket);
//}
//
//RSInline void __RSSocketLockAll()
//{
//	RSSpinLockLock(&__RSAllSocketsLock);
//}
//
//RSInline void __RSSocketUnlockAll()
//{
//	RSSpinLockUnlock(&__RSAllSocketsLock);
//}
//
//RSInline RSIndex __RSSocketFdGetSize(RSDataRef fdSet)
//{
//    return NBBY * RSDataGetLength(fdSet);
//}
//
//RSInline BOOL __RSSocketFdClr(RSSocketHandle sock, RSMutableDataRef fdSet)
//{
//    /* returns YES if a change occurred, NO otherwise */
//    BOOL retval = NO;
//    if (INVALID_SOCKET != sock && 0 <= sock)
//	{
//        RSIndex numFds = NBBY * RSDataGetLength(fdSet);
//        fd_mask *fds_bits;
//        if (sock < numFds)
//		{
//            fds_bits = (fd_mask *)RSDataGetBytesPtr(fdSet);
//            if (FD_ISSET(sock, (fd_set *)fds_bits))
//			{
//                retval = YES;
//                FD_CLR(sock, (fd_set *)fds_bits);
//            }
//        }
//    }
//    return retval;
//}
//
//RSInline BOOL __RSSocketFdSet(RSSocketHandle sock, RSMutableDataRef fdSet)
//{
//    /* returns YES if a change occurred, NO otherwise */
//    BOOL retval = NO;
//    if (INVALID_SOCKET != sock && 0 <= sock)
//	{
//        RSIndex numFds = NBBY * RSDataGetLength(fdSet);
//        fd_mask *fds_bits;
//        if (sock >= numFds)
//		{
//            RSIndex oldSize = numFds / NFDBITS, newSize = (sock + NFDBITS) / NFDBITS, changeInBytes = (newSize - oldSize) * sizeof(fd_mask);
//            RSDataSetLength(fdSet, changeInBytes);
//            fds_bits = (fd_mask *)RSDataGetBytesPtr(fdSet);
//            memset(fds_bits + oldSize, 0, changeInBytes);
//        }
//		else
//		{
//            fds_bits = (fd_mask *)RSDataGetBytesPtr(fdSet);
//        }
//        if (!FD_ISSET(sock, (fd_set *)fds_bits))
//		{
//            retval = YES;
//            FD_SET(sock, (fd_set *)fds_bits);
//        }
//    }
//    return retval;
//}
//
//RSInline BOOL __RSSocketSetFDForRead(RSSocketRef s)
//{
//	__RSReadSocketsTimeoutInvalid = YES;
//	BOOL b = __RSSocketFdSet(s->_socket, __RSReadSocketsFds);
//	if (b && INVALID_SOCKET != __RSWakeupSocketPair[0]) {
//#if defined (LOG_RSSOCKET)
//        __RSCLog(RSLogLevelNotice, "%s for %d\n", __func__, s->_socket);
//#endif
//        uint8_t c = 'r';
//        send(__RSWakeupSocketPair[0], (const char *)&c, sizeof(c), 0);
//    }
//	return b;
//}
//
//RSInline BOOL __RSSocketClearFDForRead(RSSocketRef s)
//{
//    __RSReadSocketsTimeoutInvalid = YES;
//    BOOL b = __RSSocketFdClr(s->_socket, __RSReadSocketsFds);
//    if (b && INVALID_SOCKET != __RSWakeupSocketPair[0]) {
//#if defined (LOG_RSSOCKET)
//        __RSCLog(RSLogLevelNotice, "%s for %d\n", __func__, s->_socket);
//#endif
//        uint8_t c = 's';
//        send(__RSWakeupSocketPair[0], (const char *)&c, sizeof(c), 0);
//    }
//    return b;
//}
//
//RSInline BOOL __RSSocketSetFDForWrite(RSSocketRef s)
//{
//    BOOL b = __RSSocketFdSet(s->_socket, __RSWriteSocketsFds);
//	if (b && INVALID_SOCKET != __RSWakeupSocketPair[0]) {
//#if defined (LOG_RSSOCKET)
//        __RSCLog(RSLogLevelNotice, "%s for %d\n", __func__, s->_socket);
//#endif
//        uint8_t c = 'w';
//        send(__RSWakeupSocketPair[0], (const char *)&c, sizeof(c), 0);
//    }
//    return b;
//}
//
//RSInline BOOL __RSSocketClearFDForWrite(RSSocketRef s)
//{
//	BOOL b = __RSSocketFdClr(s->_socket, __RSWriteSocketsFds);
//	if (b && INVALID_SOCKET != __RSWakeupSocketPair[0]) {
//#if defined (LOG_RSSOCKET)
//        __RSCLog(RSLogLevelNotice, "%s for %d\n", __func__, s->_socket);
//#endif
//        uint8_t c = 'x';
//        send(__RSWakeupSocketPair[0], (const char *)&c, sizeof(c), 0);
//    }
//    return b;
//}
//
//static RSBit32 __RSSocketInitMakePair()
//{
//	RSBit32 error;
//	
//    error = socketpair(PF_LOCAL, SOCK_DGRAM, 0, __RSWakeupSocketPair);
//    if (0 <= error) error = fcntl(__RSWakeupSocketPair[0], F_SETFD, FD_CLOEXEC);
//    if (0 <= error) error = fcntl(__RSWakeupSocketPair[1], F_SETFD, FD_CLOEXEC);
//    if (0 > error)
//	{
//        closesocket(__RSWakeupSocketPair[0]);
//        closesocket(__RSWakeupSocketPair[1]);
//        __RSWakeupSocketPair[0] = INVALID_SOCKET;
//        __RSWakeupSocketPair[1] = INVALID_SOCKET;
//    }
//	return error;
//}
//
//RSPrivate void __RSSocketInitialize()
//{
//	__RSAllSockets = RSDictionaryCreateMutable(RSAllocatorSystemDefault, 0, RSDictionaryNilKeyContext);
//	__RSReadSockets = RSArrayCreateMutable(RSAllocatorSystemDefault, 0);
//	__RSWriteSockets = RSArrayCreateMutable(RSAllocatorSystemDefault, 0);
//	__RSWriteSocketsFds = RSDataCreateMutable(RSAllocatorSystemDefault, 0);
//	__RSReadSocketsFds = RSDataCreateMutable(RSAllocatorSystemDefault, 0);
//	//if (0 > __RSSocketInitMakePair()) __HALT();
//    _RSSocketTypeID = __RSRuntimeRegisterClass(&__RSSocketClass);
//    __RSRuntimeSetClassTypeID(&__RSSocketClass, _RSSocketTypeID);
//	if (0 > __RSSocketInitMakePair())
//	{
//        __RSCLog(RSLogLevelEmergency, "socket make pair failed...");
//	}
//	else
//	{
//		BOOL yes = YES;
//		ioctlsocket(__RSWakeupSocketPair[0], FIONBIO, (u_long *)&yes);
//        ioctlsocket(__RSWakeupSocketPair[1], FIONBIO, (u_long *)&yes);
//		__RSSocketFdSet(__RSWakeupSocketPair[1], __RSReadSocketsFds);
//	}
//}
//
//RSPrivate void __RSSocketDeallocate()
//{
//    RSSpinLockLock(&__RSAllSocketsLock);
//    RSSpinLockLock(&__RSActiveSocketsLock);
//    RSRelease(__RSReadSockets);
//    RSRelease(__RSWriteSockets);
//    RSRelease(__RSReadSocketsFds);
//    RSRelease(__RSWriteSocketsFds);
//    RSRelease(__RSAllSockets);
//    RSSpinLockUnlock(&__RSActiveSocketsLock);
//    RSSpinLockUnlock(&__RSAllSocketsLock);
//}
//
//static struct timeval* intervalToTimeval(RSTimeInterval timeout, struct timeval* tv)
//{
//    if (timeout == 0.0)
//        timerclear(tv);
//    else
//	{
//        tv->tv_sec = (0 >= timeout || INT_MAX <= timeout) ? INT_MAX : (int)(float)floor(timeout);
//        tv->tv_usec = (int)((timeout - floor(timeout)) * 1.0E6);
//    }
//    return tv;
//}
//
//static void _calcMinTimeout_locked(const void* val, void* ctxt)
//{
//    RSSocketRef s = (RSSocketRef) val;
//    struct timeval** minTime = (struct timeval**) ctxt;
//    if (timerisset(&s->_readBufferTimeout) && (*minTime == nil || timercmp(&s->_readBufferTimeout, *minTime, <)))
//        *minTime = &s->_readBufferTimeout;
//    else if (RSQueueGetCount(s->_dataReadQueue))
//	{
//        /* If there's anyone with leftover bytes, they'll need to be awoken immediately */
//        static struct timeval sKickerTime = { 0, 0 };
//        *minTime = &sKickerTime;
//    }
//}
//
//static void __RSSocketHandleWrite(RSSocketRef s, BOOL callBackNow);
//static void __RSSocketHandleRead(RSSocketRef s, BOOL causedByTimeout);
//static void __RSSocketDoCallback(RSSocketRef s, RSSocketCallBackType type, RSDataRef data, RSDataRef address, RSSocketHandle sock);
//
//static volatile RSBitU64 __RSSocketManagerIteration = 0;
//static void* __RSSocketManager(void* info)
//{
//    pthread_setname_np("com.retval.RSSocket.private");
//    RSBit32 nrfds, maxnrfds, fdentries = 1;
//    RSBit32 rfds, wfds;
//    fd_set *exceptfds = nil;
//    fd_set *writefds = (fd_set *)RSAllocatorAllocate(RSAllocatorSystemDefault, fdentries * sizeof(fd_mask));
//    fd_set *readfds = (fd_set *)RSAllocatorAllocate(RSAllocatorSystemDefault, fdentries * sizeof(fd_mask));
//    fd_set *tempfds;
//    RSBit32 idx, cnt;
//    RSBitU8 buffer[256] __unused;
//    RSMutableArrayRef selectedWriteSockets = RSArrayCreateMutable(RSAllocatorSystemDefault, 0);
//    RSMutableArrayRef selectedReadSockets = RSArrayCreateMutable(RSAllocatorSystemDefault, 0);
//    RSIndex selectedWriteSocketsIndex = 0, selectedReadSocketsIndex = 0;
//    
//    struct timeval tv;
//    struct timeval* pTimeout = nil;
//    struct timeval timeBeforeSelect;
//    __RSCLog(RSLogLevelNotice, "Socket pair <%d, %d>\n", __RSWakeupSocketPair[0], __RSWakeupSocketPair[1]);
//    for (;;)
//	{
//#if defined(LOG_RSSOCKET)
//        __RSCLog(RSLogLevelNotice, "\n*************************** __RSSocketManager ***************************\n");
//#endif
//
//        
//        RSSpinLockLock(&__RSActiveSocketsLock);
//        __RSSocketManagerIteration++;
//		
//        rfds = (RSBit32)__RSSocketFdGetSize(__RSReadSocketsFds);
//        wfds = (RSBit32)__RSSocketFdGetSize(__RSWriteSocketsFds);
//        maxnrfds = (RSBit32)__RSMax(rfds, wfds);
//
//		__RSCLog(RSLogLevelNotice, "maxnrfds = %d\n", maxnrfds);
//        if (maxnrfds > fdentries * (int)NFDBITS)
//		{
//            fdentries = (maxnrfds + NFDBITS - 1) / NFDBITS;
//            writefds = (fd_set *)RSAllocatorReallocate(RSAllocatorSystemDefault, writefds, fdentries * sizeof(fd_mask));
//            readfds = (fd_set *)RSAllocatorReallocate(RSAllocatorSystemDefault, readfds, fdentries * sizeof(fd_mask));
//        }
//        memset(writefds, 0, fdentries * sizeof(fd_mask));
//        memset(readfds, 0, fdentries * sizeof(fd_mask));
//		__builtin_memcpy(writefds, RSDataGetBytesPtr(__RSWriteSocketsFds), RSDataGetLength(__RSWriteSocketsFds));
//		__builtin_memcpy(readfds, RSDataGetBytesPtr(__RSReadSocketsFds), RSDataGetLength(__RSReadSocketsFds));
//		
//        if (__RSReadSocketsTimeoutInvalid)
//		{
//            struct timeval* minTimeout = nil;
//            __RSReadSocketsTimeoutInvalid = NO;
//#if defined(LOG_RSSOCKET)
//            __RSCLog(RSLogLevelNotice, "Figuring out which sockets have timeouts...\n");
//#endif
//            RSArrayApplyFunction(__RSReadSockets, RSMakeRange(0, RSArrayGetCount(__RSReadSockets)), _calcMinTimeout_locked, (void*) &minTimeout);
//			
//            if (minTimeout == nil)
//			{
//#if defined(LOG_RSSOCKET)
//                __RSCLog(RSLogLevelNotice, "No one wants a timeout!\n");
//#endif
//                pTimeout = nil;
//            }
//			else
//			{
//#if defined(LOG_RSSOCKET)
//                __RSCLog(RSLogLevelNotice, "timeout will be %ld, %d!\n", minTimeout->tv_sec, minTimeout->tv_usec);
//#endif
//                tv = *minTimeout;
//                pTimeout = &tv;
//            }
//        }
//		
//        if (pTimeout)
//		{
//#if defined(LOG_RSSOCKET)
//            __RSCLog(RSLogLevelNotice, "select will have a %ld, %d timeout\n", pTimeout->tv_sec, pTimeout->tv_usec);
//#endif
//            gettimeofday(&timeBeforeSelect, nil);
//        }
//		
//		RSSpinLockUnlock(&__RSActiveSocketsLock);
//		
//#if DEPLOYMENT_TARGET_WINDOWS
//        // On Windows, select checks connection failed sockets via the exceptfds parameter. connection succeeded is checked via writefds. We need both.
//        exceptfds = writefds;
//#endif
//        nrfds = select(maxnrfds, readfds, writefds, exceptfds, pTimeout);
//        
//        /*
//         * __RSReadSocketsFds/__RSWriteSocketFds will send a signal for new socket add or remove
//         */
//		
//#if defined(LOG_RSSOCKET)
//        __RSCLog(RSLogLevelNotice, "socket manager woke from select, ret=%ld\n", (long)nrfds);
//#endif
//		
//		/*
//		 * select returned a timeout
//		 */
//        if (0 == nrfds)
//		{
//			struct timeval timeAfterSelect;
//			struct timeval deltaTime;
//			gettimeofday(&timeAfterSelect, nil);
//			/* timeBeforeSelect becomes the delta */
//			timersub(&timeAfterSelect, &timeBeforeSelect, &deltaTime);
//			
//#if defined(LOG_RSSOCKET)
//			__RSCLog(RSLogLevelNotice, "Socket manager received timeout - kicking off expired reads (expired delta %ld, %d)\n", deltaTime.tv_sec, deltaTime.tv_usec);
//#endif
//			
//			RSSpinLockLock(&__RSActiveSocketsLock);
//			
//			tempfds = nil;
//			cnt = (RSBit32)RSArrayGetCount(__RSReadSockets);
//			for (idx = 0; idx < cnt; idx++)
//			{
//				RSSocketRef s = (RSSocketRef)RSArrayObjectAtIndex(__RSReadSockets, idx);
//				if (timerisset(&s->_readBufferTimeout)/* || s->_leftoverBytes*/)
//				{
//					RSSocketHandle sock = s->_socket;
//					// We might have an new element in __RSReadSockets that we weren't listening to,
//					// in which case we must be sure not to test a bit in the fdset that is
//					// outside our mask size.
//					BOOL sockInBounds = (0 <= sock && sock < maxnrfds);
//					/* if this sockets timeout is less than or equal elapsed time, then signal it */
//					if (INVALID_SOCKET != sock && sockInBounds)
//					{
//#if defined(LOG_RSSOCKET)
//						__RSCLog(RSLogLevelNotice, "Expiring socket %d (delta %ld, %d)\n", sock, s->_readBufferTimeout.tv_sec, s->_readBufferTimeout.tv_usec);
//#endif
//						RSArraySetObjectAtIndex(selectedReadSockets, selectedReadSocketsIndex, s);
//						selectedReadSocketsIndex++;
//						/* socket is removed from fds here, will be restored in read handling or in perform function */
//						if (!tempfds) tempfds = (fd_set *)RSDataGetBytesPtr(__RSReadSocketsFds);
//						FD_CLR(sock, tempfds);
//					}
//				}
//			}
//			
//			RSSpinLockUnlock(&__RSActiveSocketsLock);
//			
//			/* and below, we dispatch through the normal read dispatch mechanism */
//		}
//        
//		if (0 > nrfds)
//		{
//            RSBit32 selectError = __RSSocketLastError();
//#if defined(LOG_RSSOCKET)
//            __RSCLog(RSLogLevelNotice, "socket manager received error %ld from select\n", (long)selectError);
//#endif
//            if (EBADF == selectError)
//			{
//                RSMutableArrayRef invalidSockets = RSArrayCreateMutable(RSAllocatorSystemDefault, 0);
//                RSSpinLockLock(&__RSActiveSocketsLock);
//                cnt = (RSBit32)RSArrayGetCount(__RSWriteSockets);
//                for (idx = 0; idx < cnt; idx++)
//				{
//                    RSSocketRef s = (RSSocketRef)RSArrayObjectAtIndex(__RSWriteSockets, idx);
//                    if (!RSSocketIsValid(s)) {
//#if defined(LOG_RSSOCKET)
//                        __RSCLog(RSLogLevelNotice, "socket manager found write socket %d invalid\n", s->_socket);
//#endif
//                        RSArrayAddObject(invalidSockets, s);
//                    }
//                }
//                cnt = (RSBit32)RSArrayGetCount(__RSReadSockets);
//                for (idx = 0; idx < cnt; idx++)
//				{
//                    RSSocketRef s = (RSSocketRef)RSArrayObjectAtIndex(__RSReadSockets, idx);
//                    if (!__RSSocketHandleIsValid(s->_socket))
//					{
//#if defined(LOG_RSSOCKET)
//                        __RSCLog(RSLogLevelNotice, "socket manager found read socket %d invalid\n", s->_socket);
//#endif
//                        RSArrayAddObject(invalidSockets, s);
//                    }
//                }
//                RSSpinLockUnlock(&__RSActiveSocketsLock);
//				
//                cnt = (RSBit32)RSArrayGetCount(invalidSockets);
//                for (idx = 0; idx < cnt; idx++)
//				{
//                    RSSocketInvalidate(((RSSocketRef)RSArrayObjectAtIndex(invalidSockets, idx)));
//                }
//                RSRelease(invalidSockets);
//            }
//            continue;
//        }
//        if (FD_ISSET(__RSWakeupSocketPair[1], readfds))
//		{
//            recv(__RSWakeupSocketPair[1], (char *)buffer, sizeof(buffer), 0);
//#if defined(LOG_RSSOCKET)
//            __RSCLog(RSLogLevelNotice, "socket manager received %c on wakeup socket\n", buffer[0]);
//#endif
//        }
//        RSSpinLockLock(&__RSActiveSocketsLock);
//        tempfds = nil;
//        cnt = (RSBit32)RSArrayGetCount(__RSWriteSockets);
//        for (idx = 0; idx < cnt; idx++)
//		{
//            RSSocketRef s = (RSSocketRef)RSArrayObjectAtIndex(__RSWriteSockets, idx);
//            RSSocketHandle sock = s->_socket;
//            // We might have an new element in __RSWriteSockets that we weren't listening to,
//            // in which case we must be sure not to test a bit in the fdset that is
//            // outside our mask size.
//            BOOL sockInBounds = (0 <= sock && sock < maxnrfds);
//            if (INVALID_SOCKET != sock && sockInBounds)
//			{
//                if (FD_ISSET(sock, writefds))
//				{
//                    RSArraySetObjectAtIndex(selectedWriteSockets, selectedWriteSocketsIndex, s);
//                    selectedWriteSocketsIndex++;
//                    /* socket is removed from fds here, restored by RSSocketReschedule */
//                    if (!tempfds) tempfds = (fd_set *)RSDataGetBytesPtr(__RSWriteSocketsFds);
//                    FD_CLR(sock, tempfds);
//#if defined (LOG_RSSOCKET)
//                    __RSCLog(RSLogLevelNotice, "Manager: cleared socket %d from write fds\n", s->_socket);
//#endif
//					// RSLog(RSSTR("Manager: cleared socket %p from write fds"), s);
//                }
//            }
//        }
//        tempfds = nil;
//        cnt = (RSBit32)RSArrayGetCount(__RSReadSockets);
//        for (idx = 0; idx < cnt; idx++)
//		{
//            RSSocketRef s = (RSSocketRef)RSArrayObjectAtIndex(__RSReadSockets, idx);
//            RSSocketHandle sock = s->_socket;
//            // We might have an new element in __RSReadSockets that we weren't listening to,
//            // in which case we must be sure not to test a bit in the fdset that is
//            // outside our mask size.
//            BOOL sockInBounds = (0 <= sock && sock < maxnrfds);
//            if (INVALID_SOCKET != sock && sockInBounds && FD_ISSET(sock, readfds))
//			{
//                RSArraySetObjectAtIndex(selectedReadSockets, selectedReadSocketsIndex, s);
//                selectedReadSocketsIndex++;
//                /* socket is removed from fds here, will be restored in read handling or in perform function */
//                if (!tempfds) tempfds = (fd_set *)RSDataGetBytesPtr(__RSReadSocketsFds);
//                FD_CLR(sock, tempfds);
//#if defined (LOG_RSSOCKET)
//                __RSCLog(RSLogLevelNotice, "Manager: cleared socket %d from read fds\n", s->_socket);
//#endif
//            }
//        }
//        RSSpinLockUnlock(&__RSActiveSocketsLock);
//        for (idx = 0; idx < selectedReadSocketsIndex; idx++)
//		{
//            RSSocketRef s = (RSSocketRef)RSArrayObjectAtIndex(selectedReadSockets, idx);
//            if (RSNull == (RSNilRef)s) continue;
//#if defined(LOG_RSSOCKET)
//            __RSCLog(RSLogLevelNotice, "socket manager signaling socket %d for read\n", s->_socket);
//#endif
//            __RSSocketHandleRead(s, nrfds == 0);
//            RSArraySetObjectAtIndex(selectedReadSockets, idx, RSNull);
//        }
//        selectedReadSocketsIndex = 0;
//        
//        for (idx = 0; idx < selectedWriteSocketsIndex; idx++)
//		{
//            RSSocketRef s = (RSSocketRef)RSArrayObjectAtIndex(selectedWriteSockets, idx);
//            if (RSNull == (RSNilRef)s) continue;
//#if defined(LOG_RSSOCKET)
//            __RSCLog(RSLogLevelNotice, "socket manager signaling socket %d for write\n", s->_socket);
//#endif
//            __RSSocketHandleWrite(s, NO);
//            RSArraySetObjectAtIndex(selectedWriteSockets, idx, RSNull);
//        }
//        selectedWriteSocketsIndex = 0;
//    
//#if defined(LOG_RSSOCKET)
//        __RSCLog(RSLogLevelNotice, "*************************************************************************\n\n");
//#endif
//    }
//	return nil;
//}
//
//static void __RSSocketSchedule(void* info, RSRunLoopRef rl, RSStringRef mode);
//static void __RSSocketPerformV0(void* info, RSSocketCallBackType type);
//static void __RSSocketCancel(void* info, RSRunLoopRef rl, RSStringRef mode);
//// If callBackNow, we immediately do client callbacks, else we have to signal a v0 RunLoopSource so the
//// callbacks can happen in another thread.
//
//static void __RSSocketHandleWrite(RSSocketRef s, BOOL callBackNow)
//{
//	RSBit32 errorCode = 0;
//	int errorSize = sizeof(errorCode);
//	if (!RSSocketIsValid(s)) return;
//	if (0 != getsockopt(s->_socket, SOL_SOCKET, SO_ERROR, (char *)&errorCode, (socklen_t *)&errorSize)) errorCode = 0;
//	__RSSocketLock(s);
//	
//	RSOptionFlags writeAvailable = __RSSocketWriteCallBackType(s);
//	writeAvailable &= (RSSocketWriteCallBack | RSSocketConnectCallBack);
//	if (writeAvailable == 0 || (s->_flag._untouchable._write && s->_flag._untouchable._connect))
//	{
//		__RSSocketUnlock(s);
//		return;
//	}
//	s->_errorCode = errorCode;
//    if (__RSSocketIsConnected(s)) writeAvailable &= ~RSSocketConnectCallBack;
//    else writeAvailable = RSSocketConnectCallBack;
//    if (callBackNow) __RSSocketDoCallback(s, 0, nil, nil, 0);
//    else {
//        __RSSocketUnlock(s);
//        if (!__RSSocketIsScheduled(s)) __RSSocketSchedule(s, RSRunLoopGetCurrent(), RSRunLoopDefault);
//        __RSSocketPerformV0(s, writeAvailable);
////            __RSSocketCancel(s, RSRunLoopGetCurrent(), RSRunLoopDefault);
//    }
//}
//
//static void __RSSocketHandleRead(RSSocketRef s, BOOL causedByTimeout)
//{
//	RSSocketHandle sock = INVALID_SOCKET;
//    RSDataRef data = nil, address = nil;
//    if (!RSSocketIsValid(s)) return;
//	RSOptionFlags readCallBackType = __RSSocketReadCallBackType(s);
//    if (readCallBackType & RSSocketDataCallBack ||
//		readCallBackType & RSSocketReadCallBack)
//	{
//        RSBitU8 bufferArray[MAX_CONNECTION_ORIENTED_DATA_SIZE], *buffer;
//        RSBitU8 name[MAX_SOCKADDR_LEN];
//        int namelen = sizeof(name);
//        RSBit32 recvlen = 0;
//        if (__RSSocketIsConnectionOriented(s))
//		{
//            buffer = bufferArray;
//            recvlen = (RSBit32)recvfrom(s->_socket, (char *)buffer, MAX_CONNECTION_ORIENTED_DATA_SIZE, 0, (struct sockaddr *)name, (socklen_t *)&namelen);
//        }
//		else
//		{
//            buffer = (RSBitU8 *)RSAllocatorAllocate(RSAllocatorSystemDefault, MAX_DATA_SIZE);
//            if (buffer) recvlen = (RSBit32)recvfrom(s->_socket, (char *)buffer, MAX_DATA_SIZE, 0, (struct sockaddr *)name, (socklen_t *)&namelen);
//        }
//#if defined(LOG_RSSOCKET)
//        __RSCLog(RSLogLevelNotice, "read %ld bytes on socket %d\n", (long)recvlen, s->_socket);
//#endif
//		if (recvlen > 0) data = RSDataCreate(RSGetAllocator(s), buffer, recvlen);
//		else data = (RSDataRef)RSNull;
//		if (buffer && buffer != bufferArray) RSAllocatorDeallocate(RSAllocatorSystemDefault, buffer);
//		if (namelen > 0) address = RSDataCreate(RSGetAllocator(s), name, namelen);
//		else address = (RSDataRef)RSNull;
//		
//		__RSSocketLock(s);
//		if (!__RSSocketIsValid(s))
//		{
//			__RSSocketUnlock(s);
//			RSRelease(data);
//			RSRelease(address);
//			return;
//		}
//		if (s->_dataReadQueue == nil) s->_dataReadQueue = RSQueueCreate(RSGetAllocator(s), 0, RSQueueNotAtom);
//		RSQueueEnqueue(s->_dataReadQueue, data);
//		if (s->_addressQueue == nil) s->_addressQueue = RSQueueCreate(RSGetAllocator(s), 0, RSQueueNotAtom);
//		RSQueueEnqueue(s->_addressQueue, address);
//		RSRelease(data);
//		RSRelease(address);
//		
//		if (0 < recvlen &&
//            (__RSSocketCallBackTypes(s) & RSSocketDataCallBack) != 0 &&
//			(s->_flag._untouchable._data == 0) &&
//			__RSSocketIsScheduled(s))
//		{
//            RSSpinLockLock(&__RSActiveSocketsLock);
//            /* restore socket to fds */
//            __RSSocketSetFDForRead(s);
//            RSSpinLockUnlock(&__RSActiveSocketsLock);
//        }
//		if (readCallBackType & RSSocketReadCallBack)
//			__RSSocketDoCallback(s, RSSocketReadCallBack, data, address, RSSocketGetHandle(s));
//		else
//			__RSSocketUnlock(s);
//	}
//	else if (readCallBackType & RSSocketAcceptCallBack)
//	{
//		RSBitU8 name[MAX_SOCKADDR_LEN];
//        int namelen = sizeof(name);
//        sock = accept(s->_socket, (struct sockaddr *)name, (socklen_t *)&namelen);
//        if (INVALID_SOCKET == sock) {
//            //??? should return error
//            return;
//        }
//        if (nil != name && 0 < namelen) {
//            address = RSDataCreate(RSGetAllocator(s), name, namelen);
//        } else {
//            address = (RSDataRef)RSRetain(RSNil);
//        }
//        
//		__RSSocketLock(s);
//        
//		data = RSDataCreate(RSAllocatorSystemDefault, (const RSBitU8*)&sock, sizeof(RSSocketHandle));
//		if (s->_dataReadQueue == nil) s->_dataReadQueue = RSQueueCreate(RSGetAllocator(s), 0, NO);
//		RSQueueEnqueue(s->_dataReadQueue, data);
//		if (s->_addressQueue == nil) s->_addressQueue = RSQueueCreate(RSGetAllocator(s), 0, NO);
//		RSQueueEnqueue(s->_addressQueue, address);
//		RSRelease(data);
//		RSRelease(address);
//		if ((__RSSocketCallBackTypes(s) & RSSocketAcceptCallBack) != 0 &&
//			(s->_flag._untouchable._accept == 0) &&
//			__RSSocketIsScheduled(s))
//        {
//            RSSpinLockLock(&__RSActiveSocketsLock);
//            /* restore socket to fds */
//            __RSSocketSetFDForRead(s);
//            RSSpinLockUnlock(&__RSActiveSocketsLock);
//        }
//		__RSSocketUnlock(s);
//        __RSSocketPerformV0(s, RSSocketAcceptCallBack);
//        if (!__RSSocketIsScheduled(s)) __RSSocketSchedule(s, RSRunLoopGetCurrent(), RSRunLoopDefault);
//        return;
//	}
//}
//
//static void __RSSocketDoCallback(RSSocketRef s, RSSocketCallBackType type, RSDataRef data, RSDataRef address, RSSocketHandle sock)
//{
//    __RSSocketUnlock(s);
////    typedef void (*RSSocketCallBack)(RSSocketRef s, RSSocketCallBackType type, RSDataRef address, const void *data, void *info)
//    s->_perform(s, type, address, data, s->_context.context);
//}
//
//static RSSocketHandle __RSSocketCreateHandle(const RSSocketSignature* signature)
//{
//	if (signature == nil) return INVALID_SOCKET;
//	RSSocketHandle handle = socket(signature->protocolFamily, signature->socketType, signature->protocol);
//	if (handle == INVALID_SOCKET) return handle;
//	BOOL wasBlocking = YES;
//	RSBitU32 yes = 1;
//	RSBit32 flags = fcntl(handle, F_GETFL, 0);
//	if (flags >= 0) wasBlocking = ((flags & O_NONBLOCK) == 0);
//	if (wasBlocking) ioctlsocket(handle, FIONBIO, (u_long *)&yes);
//	return handle;
//}
//
//static RSSocketRef __RSSocketCreateInstance(RSAllocatorRef allocator,
//											RSSocketHandle handle,
//											RSSocketCallBackType cbtype,
//											RSSocketCallBack callback,
//											const RSSocketContext* context,
//											BOOL useInstance)
//{
//	RSSocketRef memory = nil;
//    
//	__RSSocketLockAll();
//	if (INVALID_SOCKET != handle && (memory = (RSSocketRef)RSDictionaryGetValue(__RSAllSockets, (void *)(uintptr_t)(handle))))
//	{
//		if (useInstance)
//		{
//			__RSSocketUnlockAll();
//			return (RSSocketRef)RSRetain(memory);
//		}
//		else
//		{
//			__RSSocketUnlockAll();
//			RSSocketInvalidate(memory);
//			__RSSocketLockAll();
//		}
//	}
//	memory = (RSSocketRef)__RSRuntimeCreateInstance(allocator, _RSSocketTypeID, sizeof(struct __RSSocket) - sizeof(RSRuntimeBase));
//	if (nil == memory)
//	{
//		__RSSocketUnlockAll();
//		return nil;
//	}
//	RSBitU32 typeSize = sizeof(memory->_socketType);
//	if (INVALID_SOCKET == handle || 0 != getsockopt(handle, SOL_SOCKET, SO_TYPE, (char *)&(memory->_socketType), (socklen_t *)&typeSize))
//		memory->_socketType = 0;
//	memory->_perform = callback;
//	memory->_callbackTypes = cbtype;
//	memory->_socket = handle;
//    
//	if (INVALID_SOCKET != handle) RSDictionarySetValue(__RSAllSockets, (void *)(uintptr_t)handle, memory);
//    if (nil == __RSSocketManagerThread) __RSSocketManagerThread = __RSStartSimpleThread(__RSSocketManager, 0);
//    __RSSocketUnlockAll();
//	if (nil != context)
//	{
//        void *contextInfo = context->retain ? (void *)context->retain(context->context) : context->context;
//        __RSSocketLock(memory);
//        memory->_context.retain = context->retain;
//        memory->_context.release = context->release;
//        memory->_context.desciption = context->desciption;
//        memory->_context.context = contextInfo;
//        __RSSocketUnlock(memory);
//    }
//	__RSSocketSetValid(memory);
//	return memory;
//}
//
//RSExport RSSocketRef RSSocketCreate(RSAllocatorRef allocator, RSBit32 protocolFamily, RSBit32 socketType, RSBit32 protocol,RSSocketCallBackType type, RSSocketCallBack callback, const RSSocketContext *context)
//{
//	RSSocketSignature signature = {protocolFamily, socketType, protocol, nil};
//	return RSSocketCreateWithSocketSignature(allocator, &signature, type, callback, context);
//}
//
//RSExport RSSocketRef RSSocketCreateWithHandle(RSAllocatorRef allocator, RSSocketHandle handle, RSSocketCallBackType type, RSSocketCallBack callback, const RSSocketContext *context)
//{
//	return __RSSocketCreateInstance(allocator, handle, type, callback, context, NO);
//}
//
//RSExport RSSocketRef RSSocketCreateWithSocketSignature(RSAllocatorRef allocator, const RSSocketSignature* signature, RSSocketCallBackType type, RSSocketCallBack callback, const RSSocketContext *context)
//{
//	RSSocketHandle handle = __RSSocketCreateHandle(signature);
//	if (INVALID_SOCKET != handle)
//	{
//		return __RSSocketCreateInstance(allocator, handle, type, callback, context, NO);
//	}
//	return nil;
//}
//
//RSExport RSSocketRef RSSocketCreateCopy(RSAllocatorRef allocator, RSSocketRef s)
//{
//	return (RSSocketRef)RSRetain(s);
//}
//
//RSExport RSSocketUnSuccess RSSocketSetAddress(RSSocketRef s, RSDataRef address)
//{
//	__RSGenericValidInstance(s, _RSSocketTypeID);
//	RSSocketUnSuccess err = RSSocketSuccess;
//	if (nil == address) return RSSocketUnSuccess;
//	if (!RSSocketIsValid(s)) return RSSocketUnSuccess;
//	RSSpinLockLock(&s->_lock);
//	RSSocketCallBackType type = s->_callbackTypes;
//	if (type & RSSocketAcceptCallBack)
//	{
//		struct sockaddr *name;
//		socklen_t namelen;
//		RSSocketHandle handle = RSSocketGetHandle(s);
//		name = (struct sockaddr *)RSDataGetBytesPtr(address);
//		namelen = (socklen_t)RSDataGetLength(address);
//		int yes = 1;
//		setsockopt(handle, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes));
//		int result = bind(handle, name, namelen);
//		if (0 == result)
//		{
//			listen(handle, 256);
//		}
//	}
//	else if (type & RSSocketConnectCallBack)
//	{
//        if (s->_peerAddress) RSRelease(s->_peerAddress);
//        s->_peerAddress = RSRetain(address);
//	}
//	RSSpinLockUnlock(&s->_lock);
//	return err;
//}
//
//RSExport RSSocketUnSuccess RSSocketSendData(RSSocketRef s, RSDataRef address, RSDataRef data, RSTimeInterval timeout)
//{
//    const RSBitU8 *dataptr, *addrptr = nil;
//    RSBit32 datalen, addrlen = 0, size = 0;
//    RSSocketHandle sock = INVALID_SOCKET;
//    struct timeval tv;
//    __RSGenericValidInstance(s, RSSocketGetTypeID());
//    if (address)
//	{
//        addrptr = RSDataGetBytesPtr(address);
//        addrlen = (RSBit32)RSDataGetLength(address);
//    }
//    dataptr = RSDataGetBytesPtr(data);
//    datalen = (RSBit32)RSDataGetLength(data);
//    if (RSSocketIsValid(s)) sock = RSSocketGetHandle(s);
//    if (INVALID_SOCKET != sock)
//	{
//        RSRetain(s);
//        __RSSocketWriteLock(s);
//        tv.tv_sec = (timeout <= 0.0 || (RSTimeInterval)INT_MAX <= timeout) ? INT_MAX : (int)floor(timeout);
//        tv.tv_usec = (int)floor(1.0e+6 * (timeout - floor(timeout)));
//        setsockopt(sock, SOL_SOCKET, SO_SNDTIMEO, (char *)&tv, sizeof(tv));	// cast for WinSock bad API
//        if (nil != addrptr && 0 < addrlen)
//		{
//            size = (RSBit32)sendto(sock, (char *)dataptr, datalen, 0, (struct sockaddr *)addrptr, addrlen);
//        }
//		else
//		{
//            size = (RSBit32)send(sock, (char *)dataptr, datalen, 0);
//        }
//#if defined(LOG_RSSOCKET)
//        __RSCLog(RSLogLevelNotice, "wrote %ld bytes to socket %d\n", (long)size, sock);
//#endif
//        __RSSocketWriteUnlock(s);
//        RSRelease(s);
//        if (size <= 0 && datalen) {
//            RSSocketInvalidate(s);
//        }
//    }
//    return (size > 0) ? RSSocketSuccess : RSSocketUnSuccess;
//}
//
//RSExport RSSocketUnSuccess RSSocketRecvData(RSSocketRef s, RSMutableDataRef data)
//{
//	RSSocketHandle sock = (RSSocketIsValid(s)) ? RSSocketGetHandle(s) : INVALID_SOCKET;
//	if (INVALID_SOCKET != sock)
//	{
//		RSRetain(s);
//		__RSSocketLock(s);
//		if (s->_dataReadQueue)
//		{
//			if (RSQueueGetCount(s->_dataReadQueue))
//			{
//				RSDataRef recvData = RSQueueDequeue(s->_dataReadQueue);
//				RSDataAppend(data, recvData);
//				RSRelease(recvData);
//				__RSSocketUnlock(s);
//				RSRelease(s);
//				return RSSocketSuccess;
//			}
//		}
//		__RSSocketUnlock(s);
//		RSRelease(s);
//	}
//	return RSSocketUnSuccess;
//}
//
//RSExport RSSocketUnSuccess RSSocketConnectToAddress(RSSocketRef s, RSDataRef address, RSTimeInterval timeout)
//{
//	if (s == nil || address == nil) return RSSocketUnSuccess;
//	RSSocketHandle sock = RSSocketGetHandle(s);
//	const uint8_t *name = nil;
//	RSBit32 namelen = 0, result = -1, connect_err = 0, select_err = 0;
//	RSBitU32 yes = 1, no = 0;
//	BOOL wasBlocking = YES;
//	if (sock != INVALID_SOCKET)
//	{
//		name = RSDataGetBytesPtr(address);
//		namelen = (RSBit32)RSDataGetLength(address);
//#if DEPLOYMENT_TARGET_MACOSX || DEPLOYMENT_TARGET_EMBEDDED || DEPLOYMENT_TARGET_EMBEDDED_MINI
//        RSBit32 flags = fcntl(sock, F_GETFL, 0);
//        if (flags >= 0) wasBlocking = ((flags & O_NONBLOCK) == 0);
//        if (wasBlocking && (timeout > 0.0 || timeout < 0.0)) ioctlsocket(sock, FIONBIO, (u_long *)&yes);
//#else
//        // You can set but not get this flag in WIN32, so assume it was in non-blocking mode.
//        // The downside is that when we leave this routine we'll leave it non-blocking,
//        // whether it started that way or not.
//        SInt32 flags = 0;
//        if (timeout > 0.0 || timeout < 0.0) ioctlsocket(sock, FIONBIO, (u_long *)&yes);
//        wasBlocking = NO;
//#endif
//        result = connect(sock, (struct sockaddr *)name, namelen);
//        if (result != 0)
//        {
//            connect_err = __RSSocketLastError();
//#if DEPLOYMENT_TARGET_WINDOWS
//            if (connect_err == WSAEWOULDBLOCK) connect_err = EINPROGRESS;
//#endif
//        }
//#if defined(LOG_RSSOCKET)
//        __RSCLog(RSLogLevelNotice, "connection attempt returns %d error %d on socket %d (flags 0x%x blocking %d)\n", result, connect_err, sock, flags, wasBlocking);
//#endif
//        if (EINPROGRESS == connect_err && timeout >= 0.0)
//        {
//            /* select on socket */
//            SInt32 nrfds;
//            int error_size = sizeof(select_err);
//            struct timeval tv;
//            RSMutableDataRef fds = RSDataCreateMutable(RSAllocatorSystemDefault, 0);
//            __RSSocketFdSet(sock, fds);
//            tv.tv_sec = (timeout <= 0.0 || (RSTimeInterval)INT_MAX <= timeout) ? INT_MAX : (int)floor(timeout);
//            tv.tv_usec = (int)floor(1.0e+6 * (timeout - floor(timeout)));
//            nrfds = select((int)__RSSocketFdGetSize(fds), nil, (fd_set *)RSDataGetBytesPtr(fds), nil, &tv);
//            if (nrfds < 0)
//			{
//                select_err = __RSSocketLastError();
//                result = -1;
//            }
//			else if (nrfds == 0)
//			{
//                result = -2;
//            }
//			else
//			{
//                if (0 != getsockopt(sock, SOL_SOCKET, SO_ERROR, (char *)&select_err, (socklen_t *)&error_size))
//                    select_err = 0;
//                result = (select_err == 0) ? 0 : -1;
//            }
//            RSRelease(fds);
//#if defined(LOG_RSSOCKET)
//            __RSCLog(RSLogLevelNotice, "timed connection attempt %s on socket %d, result %d, select returns %d error %d\n", (result == 0) ? "succeeds" : "fails", sock, result, nrfds, select_err);
//#endif
//        }
//        if (wasBlocking && (timeout > 0.0 || timeout < 0.0)) ioctlsocket(sock, FIONBIO, (u_long *)&no);
//        if (EINPROGRESS == connect_err && timeout < 0.0)
//		{
//            result = 0;
//#if defined(LOG_RSSOCKET)
//            __RSCLog(RSLogLevelNotice, "connection attempt continues in background on socket %d\n", sock);
//#endif
//        }
//    }
//	return result;
//}
//
//RSExport void RSSocketInvalidate(RSSocketRef s)
//{
//    if (!s) HALTWithError(RSInvalidArgumentException, "socket is nil");
//    if (!RSSocketIsValid(s)) return;
//    RSBitU64 previousSocketManagerIteration;
//    __RSGenericValidInstance(s, RSSocketGetTypeID());
//#if defined(LOG_RSSOCKET)
//    __RSCLog(RSLogLevelNotice, "invalidating socket %d\n", s->_socket);
//#endif
//    RSRetain(s);
//    RSSpinLockLock(&__RSAllSocketsLock);
//    __RSSocketLock(s);
//    if (__RSSocketIsValid(s))
//	{
//        RSIndex idx;
//        RSRunLoopSourceRef source0;
//        void *contextInfo = nil;
//        void (*contextRelease)(const void *info) = nil;
//        __RSSocketUnsetValid(s);
//        
//        RSSpinLockLock(&__RSActiveSocketsLock);
//        idx = RSArrayIndexOfObject(__RSWriteSockets, s);
//        if (0 <= idx) {
//            RSArrayRemoveObjectAtIndex(__RSWriteSockets, idx);
//            __RSSocketClearFDForWrite(s);
//        }
//        // No need to clear FD's for V1 sources, since we'll just throw the whole event away
//        idx = RSArrayIndexOfObject(__RSReadSockets, s);
//        if (0 <= idx) {
//            RSArrayRemoveObjectAtIndex(__RSReadSockets, idx);
//            __RSSocketClearFDForRead(s);
//        }
//        previousSocketManagerIteration = __RSSocketManagerIteration;
//        RSSpinLockUnlock(&__RSActiveSocketsLock);
//        RSDictionaryRemoveValue(__RSAllSockets, (void *)(uintptr_t)(s->_socket));
//        if ((1) != 0) closesocket(s->_socket);
//        s->_socket = INVALID_SOCKET;
//        if (nil != s->_peerAddress)
//		{
//            RSRelease(s->_peerAddress);
//            s->_peerAddress = nil;
//        }
//        if (nil != s->_dataReadQueue)
//		{
//            RSRelease(s->_dataReadQueue);
//            s->_dataReadQueue = nil;
//        }
//		if (nil != s->_dataWriteQueue)
//		{
//            RSRelease(s->_dataWriteQueue);
//            s->_dataWriteQueue = nil;
//        }
//        if (nil != s->_addressQueue)
//		{
//            RSRelease(s->_addressQueue);
//            s->_addressQueue = nil;
//        }
//        s->_schedule = 0;
//        
//        // we'll need this later
//		
//        source0 = s->_source0;
//        s->_source0 = nil;
//        contextInfo = s->_context.context;
//        contextRelease = s->_context.release;
//        s->_context.context = 0;
//        s->_context.retain = 0;
//        s->_context.release = 0;
//        s->_context.desciption = 0;
//        __RSSocketUnlock(s);
//        
//        // Do this after the socket unlock to avoid deadlock (10462525)
//		
//		
//        if (nil != contextRelease)
//		{
//            contextRelease(contextInfo);
//        }
//        if (nil != source0)
//		{
//            RSRelease(source0);
//        }
//    }
//	else
//	{
//        __RSSocketUnlock(s);
//    }
//    RSSpinLockUnlock(&__RSAllSocketsLock);
//    RSRelease(s);
//}
//
//static void __RSSocketEnableCallBacks(RSSocketRef s, RSOptionFlags callBackTypes, BOOL force, uint8_t wakeupChar)
//{
//    BOOL wakeup = NO;
//    if (!callBackTypes)
//	{
//        __RSSocketUnlock(s);
//        return;
//    }
//    if (__RSSocketIsValid(s) && __RSSocketIsScheduled(s))
//	{
//        BOOL turnOnWrite = NO, turnOnConnect = NO, turnOnRead = NO;
//        uint8_t readCallBackType = __RSSocketReadCallBackType(s);
//        callBackTypes &= __RSSocketCallBackTypes(s);
//        
//#if defined(LOG_RSSOCKET)
//        __RSCLog(RSLogLevelNotice, "rescheduling socket %d for types 0x%llx\n", s->_socket,callBackTypes);
//#endif
//        /* We will wait for connection only for connection-oriented, non-rendezvous sockets that are not already connected.  Mark others as already connected. */
//        if ((readCallBackType == RSSocketAcceptCallBack) || !__RSSocketIsConnectionOriented(s)) s->_flag._untouchable._connected = YES;
//        //
//        //        // First figure out what to turn on
//        if (s->_flag._untouchable._connected || (callBackTypes & RSSocketConnectCallBack) == 0)
//		{
//            //            // if we want write callbacks and they're not disabled...
//            if ((callBackTypes & RSSocketWriteCallBack) != 0 && (s->_flag._untouchable._write == 0)) turnOnWrite = YES;
//        }
//		else
//		{
//            //            // if we want connect callbacks and they're not disabled...
//            if ((callBackTypes & RSSocketConnectCallBack) != 0 && (s->_flag._untouchable._connect == 0)) turnOnConnect = YES;
//        }
//        //        // if we want read callbacks and they're not disabled...
//        if (readCallBackType != RSSocketNoCallBack &&
//			(callBackTypes & readCallBackType) != 0 &&
//			((s->_flag._untouchable._read & RSSocketReadCallBack) == 0 ||
//			 (s->_flag._untouchable._accept & RSSocketAcceptCallBack) == 0))
//			turnOnRead = YES;
//		
//        // Now turn on the callbacks we've determined that we want on
//        if (turnOnRead || turnOnWrite || turnOnConnect)
//		{
//            RSSpinLockLock(&__RSActiveSocketsLock);
//            if (turnOnWrite || turnOnConnect)
//			{
//                if (force)
//				{
//                    RSIndex idx = RSArrayIndexOfObject(__RSWriteSockets, s);
//                    if (RSNotFound == idx) RSArrayAddObject(__RSWriteSockets, s);
//					//                     if (RSNotFound == idx) RSLog(5, RSSTR("__RSSocketEnableCallBacks: put %p in __RSWriteSockets list due to force and non-presence"), s);
//                }
//                if (__RSSocketSetFDForWrite(s)) wakeup = YES;
//            }
//            if (turnOnRead)
//			{
//                if (force)
//				{
//                    RSIndex idx = RSArrayIndexOfObject(__RSReadSockets, s);
//                    if (RSNotFound == idx) RSArrayAddObject(__RSReadSockets, s);
//                }
//                if (__RSSocketSetFDForRead(s)) wakeup = YES;
//            }
//            RSSpinLockUnlock(&__RSActiveSocketsLock);
//        }
//    }
//    __RSSocketUnlock(s);
//}
//
//RSExport void RSSocketReschedule(RSSocketRef s)
//{
//    __RSGenericValidInstance(s, _RSSocketTypeID);
//    if (!RSSocketIsValid(s)) return;
//    __RSSocketLock(s);
//    __RSSocketEnableCallBacks(s, __RSSocketCallBackTypes(s), YES, 's');
//}
//
//static void __RSSocketSchedule(void* info, RSRunLoopRef rl, RSStringRef mode)
//{
//    RSLog(RSSTR("__RSSocketSchedule called."));
//	RSSocketRef s = (RSSocketRef)info;
//    __RSSocketLock(s);
//    //??? also need to arrange delivery of all pending data
//    if (__RSSocketIsValid(s))
//	{
//        
//        __RSSocketScheduleStart(s);
//        // Since the v0 source is listened to on the SocketMgr thread, no matter how many modes it
//        // is added to we just need to enable it there once (and _socketSetCount gives us a refCount
//        // to know when we can finally disable it).
//        if (1 == __RSSocketIsScheduled(s))
//		{
//#if defined(LOG_RSSOCKET)
//            __RSCLog(RSLogLevelNotice, "scheduling socket %d\n", s->_socket);
//#endif
//			// RSLog(5, RSSTR("__RSSocketSchedule(%p, %p, %p)"), s, rl, mode);
//            //            static void __RSSocketEnableCallBacks(RSSocketRef s, RSOptionFlags callBackTypes, BOOL force, uint8_t wakeupChar)
//            __RSSocketEnableCallBacks(s, __RSSocketCallBackTypes(s), YES, 's');  // unlocks s
//        }
//		else
//            __RSSocketUnlock(s);
//    }
//	else
//        __RSSocketUnlock(s);
//}
//
//static void __RSSocketCancel(void* info, RSRunLoopRef rl, RSStringRef mode)
//{
//    RSLog(RSSTR("__RSSocketCancel called."));
//	RSSocketRef s = (RSSocketRef)info;
//    RSBit32 idx;
//    __RSSocketLock(s);
//    __RSSocketScheduleDone(s);
//    if (!__RSSocketIsScheduled(s))
//	{
//        RSSpinLockLock(&__RSActiveSocketsLock);
//        idx = (RSBit32)RSArrayIndexOfObject(__RSWriteSockets, s);
//        if (0 <= idx)
//		{
//			// RSLog(5, RSSTR("__RSSocketCancel: removing %p from __RSWriteSockets list"), s);
//            RSArrayRemoveObjectAtIndex(__RSWriteSockets, idx);
//            __RSSocketClearFDForWrite(s);
//        }
//        idx = (RSBit32)RSArrayIndexOfObject(__RSReadSockets, s);
//        if (0 <= idx)
//		{
//            RSArrayRemoveObjectAtIndex(__RSReadSockets, idx);
//            __RSSocketClearFDForRead(s);
//        }
//        RSSpinLockUnlock(&__RSActiveSocketsLock);
//    }
//    if (nil != s->_source0)
//	{
//		s->_source0 = nil;
//    }
//    __RSSocketUnlock(s);
//}
//
//static void __RSSocketPerformV0(void* info, RSSocketCallBackType type)
//{
//    RSLog(RSSTR("__RSSocketPerform called."));
//    RSSocketRef s = (RSSocketRef)info;
//    __RSGenericValidInstance(s, _RSSocketTypeID);
//	__RSSocketLock(s);
//    if (!__RSSocketIsValid(s))
//	{
//		__RSSocketUnlock(s);
//		return;
//	}
//	RSDataRef data = nil;
//	RSDataRef address = nil;
//	RSSocketHandle sock = INVALID_SOCKET;
//	if (RSSocketReadCallBack == type)
//	{
//        if (nil != s->_dataReadQueue && 0 < RSQueueGetCount(s->_dataReadQueue))
//		{
//            data = (RSDataRef)RSQueueDequeue(s->_dataReadQueue);
//            address = (RSDataRef)RSQueueDequeue(s->_addressQueue);
//        }
//    }
//	else if (RSSocketAcceptCallBack == type)
//	{
//        if (nil != s->_dataReadQueue && 0 < RSQueueGetCount(s->_dataReadQueue))
//		{
//            data = (RSDataRef)RSQueueDequeue(s->_dataReadQueue);
//			if (sizeof(RSSocketHandle) == RSDataGetLength(data))
//			{
//				__builtin_memcpy(&sock, RSDataGetBytesPtr(data), sizeof(RSSocketHandle));
//			}
//            address = (RSDataRef)RSQueueDequeue(s->_addressQueue);
//        }
//    }
////    RSPerformBlockInBackGroundWaitUntilDone(^{
//        __RSSocketDoCallback(s, type, data, address, sock);
//        if (data) RSRelease(data);
//        if (address) RSRelease(address);
//        
//        if (type & RSSocketConnectCallBack)
//        {
//            if (s->_flag._untouchable._connect) return;
//            
//        }
//        else if (type & RSSocketAcceptCallBack)
//        {
//            if (s->_flag._untouchable._accept) return;
//            
//        }
//        else if (type & RSSocketDataCallBack)
//        {
//            if (s->_flag._untouchable._data) return;
//            //        if (s->_perform)
//            //            s->_perform(s, RSSocketDataCallBack, s->_peerAddress, nil, s->_context.context);
//        }
////    });
//}
//
//
//RSExport RSRunLoopSourceRef	RSSocketCreateRunLoopSource(RSAllocatorRef allocator, RSSocketRef s, RSIndex order)
//{
//    __RSSocketLock(s);
//    __RSGenericValidInstance(s, _RSSocketTypeID);
//    RSRunLoopSourceRef source = nil;
//    if (__RSSocketIsValid(s))
//    {
//        if (s->_source0 != nil && !RSRunLoopSourceIsValid(s->_source0))
//        {
//            s->_source0 = nil;
//        }
//        if (s->_source0 == nil)
//        {
//            RSRunLoopSourceContext RSSocketSourceContext =
//            {
//                0,
//                s,              // information
//                nil,
//                nil,
//                RSDescription,
//                nil,
//                nil,
//                __RSSocketSchedule,     //void	(*schedule)(void *info, RSRunLoopRef rl, RSStringRef mode);
//                nil,       //void	(*cancel)(void *info, RSRunLoopRef rl, RSStringRef mode);
//                nil                    //void	(*perform)(void *info); so the runloop can not run this source but sechdule will be called!
//            };
//            s->_source0 = RSRunLoopSourceCreate(RSAllocatorSystemDefault, order, &RSSocketSourceContext);
//        }
//		source = (RSRunLoopSourceRef)RSRetain(s->_source0);   // This retain is for the receiver
//    }
//    __RSSocketUnlock(s);
//    return source;
//}

#define NEW_SOCKET 0

#if NEW_SOCKET
/*
 
 #include <CoreFoundation/RSSocket.h>
 #include "RSInternal.h"
 #include <dispatch/dispatch.h>
 #include <dispatch/private.h>
 #include <netinet/in.h>
 #include <sys/sysctl.h>
 #include <sys/socket.h>
 #include <sys/ioctl.h>
 #include <sys/stat.h>
 #include <unistd.h>
 #include <dlfcn.h>
 #include <sys/select.h>
 
 
 extern void _RSRunLoopSourceWakeUpRunLoops(RSRunLoopSourceRef rls);
 
 #define INVALID_SOCKET (RSSocketHandle)(-1)
 #define MAX_SOCKADDR_LEN 256
 
 
 DISPATCH_HELPER_FUNCTIONS(sock, RSSocket)
 
 static BOOL sockfd_is_readable(int fd) {
 if (fd < 0 || 1048576 <= fd) HALT;
 size_t sz = ((fd + CHAR_BIT) / CHAR_BIT) + 7; // generous
 fd_set *fdset = malloc(sz);
 int ret;
 do {
 memset(fdset, 0, sz);
 FD_SET(fd, fdset);
 struct timespec ts = {0, 1000UL}; // 1 us
 ret = pselect(fd + 1, fdset, nil, nil, &ts, nil);
 } while (ret < 0 && (EINTR == errno || EAGAIN == errno));
 BOOL isSet = ((0 < ret) && FD_ISSET(fd, fdset));
 free(fdset);
 return isSet;
 }
 
 static BOOL sockfd_is_writeable(int fd) {
 if (fd < 0 || 1048576 <= fd) HALT;
 size_t sz = ((fd + CHAR_BIT) / CHAR_BIT) + 7; // generous
 fd_set *fdset = malloc(sz);
 int ret;
 do {
 memset(fdset, 0, sz);
 FD_SET(fd, fdset);
 struct timespec ts = {0, 1000UL}; // 1 us
 ret = pselect(fd + 1, nil, fdset, nil, &ts, nil);
 } while (ret < 0 && (EINTR == errno || EAGAIN == errno));
 BOOL isSet = ((0 < ret) && FD_ISSET(fd, fdset));
 free(fdset);
 return isSet;
 }
 
 
 enum {
 RSSocketStateReady = 0,
 RSSocketStateInvalidating = 1,
 RSSocketStateInvalid = 2,
 RSSocketStateDeallocating = 3
 };
 
 struct __shared_blob {
 dispatch_source_t _rdsrc;
 dispatch_source_t _wrsrc;
 RSRunLoopSourceRef _source;
 RSSocketHandle _socket;
 uint8_t _closeFD;
 uint8_t _refCnt;
 };
 
 struct __RSSocket {
 RSRuntimeBase _base;
 struct __shared_blob *_shared; // non-nil when valid, nil when invalid
 
 uint8_t _state:2;         // mutable, not written safely
 uint8_t _isSaneFD:1;      // immutable
 uint8_t _connOriented:1;  // immutable
 uint8_t _wantConnect:1;   // immutable
 uint8_t _wantWrite:1;     // immutable
 uint8_t _wantReadType:2;  // immutable
 
 uint8_t _error;
 
 uint8_t _rsuspended:1;
 uint8_t _wsuspended:1;
 uint8_t _readable:1;
 uint8_t _writeable:1;
 uint8_t _unused:4;
 
 uint8_t _reenableRead:1;
 uint8_t _readDisabled:1;
 uint8_t _reenableWrite:1;
 uint8_t _writeDisabled:1;
 uint8_t _connectDisabled:1;
 uint8_t _connected:1;
 uint8_t _leaveErrors:1;
 uint8_t _closeOnInvalidate:1;
 
 int32_t _runLoopCounter;
 
 RSDataRef _address;         // immutable, once created
 RSDataRef _peerAddress;     // immutable, once created
 RSSocketCallBack _callout;  // immutable
 RSSocketContext _context;   // immutable
 };
 
 
 RSInline BOOL __RSSocketIsValid(RSSocketRef sock) {
 return RSSocketStateReady == sock->_state;
 }
 
 static RSStringRef __RSSocketCopyDescription(RSTypeRef rs) {
 RSSocketRef sock = (RSSocketRef)rs;
 RSStringRef contextDesc = nil;
 if (nil != sock->_context.context && nil != sock->_context.description) {
 contextDesc = sock->_context.description(sock->_context.context);
 }
 if (nil == contextDesc) {
 contextDesc = RSStringCreateWithFormat(RSAllocatorSystemDefault, nil, RSSTR("<RSSocket context %p>"), sock->_context.context);
 }
 Dl_info info;
 void *addr = sock->_callout;
 const char *name = (dladdr(addr, &info) && info.dli_saddr == addr && info.dli_sname) ? info.dli_sname : "???";
 int avail = -1;
 ioctlsocket(sock->_shared ? sock->_shared->_socket : -1, FIONREAD, &avail);
 RSStringRef result = RSStringCreateWithFormat(RSAllocatorSystemDefault, nil, RSSTR(
 "<RSSocket %p [%p]>{valid = %s, socket = %d, "
 "want connect = %s, connect disabled = %s, "
 "want write = %s, reenable write = %s, write disabled = %s, "
 "want read = %s, reenable read = %s, read disabled = %s, "
 "leave errors = %s, close on invalidate = %s, connected = %s, "
 "last error code = %d, bytes available for read = %d, "
 "source = %p, callout = %s (%p), context = %r}"),
 rs, RSGetAllocator(sock), __RSSocketIsValid(sock) ? "Yes" : "No", sock->_shared ? sock->_shared->_socket : -1,
 sock->_wantConnect ? "Yes" : "No", sock->_connectDisabled ? "Yes" : "No",
 sock->_wantWrite ? "Yes" : "No", sock->_reenableWrite ? "Yes" : "No", sock->_writeDisabled ? "Yes" : "No",
 sock->_wantReadType ? "Yes" : "No", sock->_reenableRead ? "Yes" : "No", sock->_readDisabled? "Yes" : "No",
 sock->_leaveErrors ? "Yes" : "No", sock->_closeOnInvalidate ? "Yes" : "No", sock->_connected ? "Yes" : "No",
 sock->_error, avail,
 sock->_shared ? sock->_shared->_source : nil, name, addr, contextDesc);
 if (nil != contextDesc) {
 RSRelease(contextDesc);
 }
 return result;
 }
 
 static void __RSSocketDeallocate(RSTypeRef rs) {
 CHECK_FOR_FORK_RET();
 RSSocketRef sock = (RSSocketRef)rs;
 // Since RSSockets are cached, we can only get here sometime after being invalidated
 sock->_state = RSSocketStateDeallocating;
 if (sock->_peerAddress) {
 RSRelease(sock->_peerAddress);
 sock->_peerAddress = nil;
 }
 if (sock->_address) {
 RSRelease(sock->_address);
 sock->_address = nil;
 }
 }
 
 static RSTypeID __RSSocketTypeID = _RSRuntimeNotATypeID;
 
 static const RSRuntimeClass __RSSocketClass = {
 0,
 "RSSocket",
 nil,      // init
 nil,      // copy
 __RSSocketDeallocate,
 nil,      // equal
 nil,      // hash
 nil,      //
 __RSSocketCopyDescription
 };
 
 static RSMutableArrayRef __RSAllSockets = nil;
 
 RSTypeID RSSocketGetTypeID(void) {
 if (_RSRuntimeNotATypeID == __RSSocketTypeID) {
 __RSSocketTypeID = _RSRuntimeRegisterClass(&__RSSocketClass);
 __RSAllSockets = RSArrayCreateMutable(RSAllocatorSystemDefault, 0, &RSTypeArrayCallBacks);
 struct rlimit lim1;
 int ret1 = getrlimit(RLIMIT_NOFILE, &lim1);
 int mib[] = {CTL_KERN, KERN_MAXFILESPERPROC};
 int maxfd = 0;
 size_t len = sizeof(int);
 int ret0 = sysctl(mib, 2, &maxfd, &len, nil, 0);
 if (0 == ret0 && 0 == ret1 && lim1.rlim_max < maxfd) maxfd = lim1.rlim_max;
 if (0 == ret1 && lim1.rlim_cur < maxfd) {
 struct rlimit lim2 = lim1;
 lim2.rlim_cur += 2304;
 if (maxfd < lim2.rlim_cur) lim2.rlim_cur = maxfd;
 setrlimit(RLIMIT_NOFILE, &lim2);
 // we try, but do not go to extraordinary measures
 }
 }
 return __RSSocketTypeID;
 }
 
 RSSocketRef RSSocketCreateWithNative(RSAllocatorRef allocator, RSSocketHandle ufd, RSOptionFlags callBackTypes, RSSocketCallBack callout, const RSSocketContext *context) {
 CHECK_FOR_FORK_RET(nil);
 
 RSSocketGetTypeID(); // cause initialization if necessary
 
 struct stat statbuf;
 int ret = fstat(ufd, &statbuf);
 if (ret < 0) ufd = INVALID_SOCKET;
 
 BOOL sane = NO;
 if (INVALID_SOCKET != ufd) {
 uint32_t type = (statbuf.st_mode & S_IFMT);
 sane = (S_IFSOCK == type) || (S_IFIFO == type) || (S_IFCHR == type);
 if (1 && !sane) {
 RSLog(RSLogLevelWarning, RSSTR("*** RSSocketCreateWithNative(): creating RSSocket with silly fd type (%07o) -- may or may not work"), type);
 }
 }
 
 if (INVALID_SOCKET != ufd) {
 BOOL canHandle = NO;
 int tmp_kq = kqueue();
 if (0 <= tmp_kq) {
 struct kevent ev[2];
 EV_SET(&ev[0], ufd, EVFILT_READ, EV_ADD, 0, 0, 0);
 EV_SET(&ev[1], ufd, EVFILT_WRITE, EV_ADD, 0, 0, 0);
 int ret = kevent(tmp_kq, ev, 2, nil, 0, nil);
 canHandle = (0 <= ret); // if kevent(ADD) succeeds, can handle
 close(tmp_kq);
 }
 if (1 && !canHandle) {
 RSLog(RSLogLevelWarning, RSSTR("*** RSSocketCreateWithNative(): creating RSSocket with unsupported fd type -- may or may not work"));
 }
 }
 
 if (INVALID_SOCKET == ufd) {
 // Historically, bad ufd was allowed, but gave an uncached and already-invalid RSSocketRef
 SInt32 size = sizeof(struct __RSSocket) - sizeof(RSRuntimeBase);
 RSSocketRef memory = (RSSocketRef)_RSRuntimeCreateInstance(allocator, RSSocketGetTypeID(), size, nil);
 if (nil == memory) {
 return nil;
 }
 memory->_callout = callout;
 memory->_state = RSSocketStateInvalid;
 return memory;
 }
 
 __block RSSocketRef sock = nil;
 dispatch_sync(__sockQueue(), ^{
 for (RSIndex idx = 0, cnt = RSArrayGetCount(__RSAllSockets); idx < cnt; idx++) {
 RSSocketRef s = (RSSocketRef)RSArrayGetValueAtIndex(__RSAllSockets, idx);
 if (s->_shared->_socket == ufd) {
 RSRetain(s);
 sock = s;
 return;
 }
 }
 
 SInt32 size = sizeof(struct __RSSocket) - sizeof(RSRuntimeBase);
 RSSocketRef memory = (RSSocketRef)_RSRuntimeCreateInstance(allocator, RSSocketGetTypeID(), size, nil);
 if (nil == memory) {
 return;
 }
 
 int socketType = 0;
 if (INVALID_SOCKET != ufd) {
 socklen_t typeSize = sizeof(socketType);
 int ret = getsockopt(ufd, SOL_SOCKET, SO_TYPE, (void *)&socketType, (socklen_t *)&typeSize);
 if (ret < 0) socketType = 0;
 }
 
 memory->_rsuspended = YES;
 memory->_wsuspended = YES;
 memory->_readable = NO;
 memory->_writeable = NO;
 
 memory->_isSaneFD = sane ? 1 : 0;
 memory->_wantReadType = (callBackTypes & 0x3);
 memory->_reenableRead = memory->_wantReadType ? YES : NO;
 memory->_readDisabled = NO;
 memory->_wantWrite = (callBackTypes & RSSocketWriteCallBack) ? YES : NO;
 memory->_reenableWrite = NO;
 memory->_writeDisabled = NO;
 memory->_wantConnect = (callBackTypes & RSSocketConnectCallBack) ? YES : NO;
 memory->_connectDisabled = NO;
 memory->_leaveErrors = NO;
 memory->_closeOnInvalidate = YES;
 memory->_connOriented = (SOCK_STREAM == socketType || SOCK_SEQPACKET == socketType);
 memory->_connected = (memory->_wantReadType == RSSocketAcceptCallBack || !memory->_connOriented) ? YES : NO;
 
 memory->_error = 0;
 memory->_runLoopCounter = 0;
 memory->_address = nil;
 memory->_peerAddress = nil;
 memory->_context.context = nil;
 memory->_context.retain = nil;
 memory->_context.release = nil;
 memory->_context.description = nil;
 memory->_callout = callout;
 if (nil != context) {
 objc_memmove_collectable(&memory->_context, context, sizeof(RSSocketContext));
 memory->_context.context = context->retain ? (void *)context->retain(context->info) : context->info;
 }
 
 struct __shared_blob *shared = malloc(sizeof(struct __shared_blob));
 shared->_rdsrc = nil;
 shared->_wrsrc = nil;
 shared->_source = nil;
 shared->_socket = ufd;
 shared->_closeFD = YES; // copy of _closeOnInvalidate
 shared->_refCnt = 1; // one for the RSSocket
 memory->_shared = shared;
 
 if (memory->_wantReadType) {
 dispatch_source_t dsrc = nil;
 if (sane) {
 dsrc = dispatch_source_create(DISPATCH_SOURCE_TYPE_READ, ufd, 0, __sockQueue());
 } else {
 dsrc = dispatch_source_create(DISPATCH_SOURCE_TYPE_TIMER, 0, 0, __sockQueue());
 dispatch_source_set_timer(dsrc, dispatch_time(DISPATCH_TIME_NOW, 0), NSEC_PER_SEC / 2, NSEC_PER_SEC);
 }
 dispatch_block_t event_block = ^{
 memory->_readable = YES;
 if (!memory->_rsuspended) {
 dispatch_suspend(dsrc);
 // RSLog(5, RSSTR("suspend %p due to read event block"), memory);
 memory->_rsuspended = YES;
 }
 if (shared->_source) {
 RSRunLoopSourceSignal(shared->_source);
 _RSRunLoopSourceWakeUpRunLoops(shared->_source);
 }
 };
 dispatch_block_t cancel_block = ^{
 shared->_rdsrc = nil;
 shared->_refCnt--;
 if (0 == shared->_refCnt) {
 if (shared->_closeFD) {
 // thoroughly stop anything else from using the fd
 (void)shutdown(shared->_socket, SHUT_RDWR);
 int nullfd = open("/dev/null", O_RDONLY);
 dup2(nullfd, shared->_socket);
 close(nullfd);
 close(shared->_socket);
 }
 free(shared);
 }
 dispatch_release(dsrc);
 };
 dispatch_source_set_event_handler(dsrc, event_block);
 dispatch_source_set_cancel_handler(dsrc, cancel_block);
 shared->_rdsrc = dsrc;
 }
 if (memory->_wantWrite || memory->_wantConnect) {
 dispatch_source_t dsrc = nil;
 if (sane) {
 dsrc = dispatch_source_create(DISPATCH_SOURCE_TYPE_WRITE, ufd, 0, __sockQueue());
 } else {
 dsrc = dispatch_source_create(DISPATCH_SOURCE_TYPE_TIMER, 0, 0, __sockQueue());
 dispatch_source_set_timer(dsrc, dispatch_time(DISPATCH_TIME_NOW, 0), NSEC_PER_SEC / 2, NSEC_PER_SEC);
 }
 dispatch_block_t event_block = ^{
 memory->_writeable = YES;
 if (!memory->_wsuspended) {
 dispatch_suspend(dsrc);
 // RSLog(5, RSSTR("suspend %p due to write event block"), memory);
 memory->_wsuspended = YES;
 }
 if (shared->_source) {
 RSRunLoopSourceSignal(shared->_source);
 _RSRunLoopSourceWakeUpRunLoops(shared->_source);
 }
 };
 dispatch_block_t cancel_block = ^{
 shared->_wrsrc = nil;
 shared->_refCnt--;
 if (0 == shared->_refCnt) {
 if (shared->_closeFD) {
 // thoroughly stop anything else from using the fd
 (void)shutdown(shared->_socket, SHUT_RDWR);
 int nullfd = open("/dev/null", O_RDONLY);
 dup2(nullfd, shared->_socket);
 close(nullfd);
 close(shared->_socket);
 }
 free(shared);
 }
 dispatch_release(dsrc);
 };
 dispatch_source_set_event_handler(dsrc, event_block);
 dispatch_source_set_cancel_handler(dsrc, cancel_block);
 shared->_wrsrc = dsrc;
 }
 
 if (shared->_rdsrc) {
 shared->_refCnt++;
 }
 if (shared->_wrsrc) {
 shared->_refCnt++;
 }
 
 memory->_state = RSSocketStateReady;
 RSArrayRemoveObjectAtIndex(__RSAllSockets, memory);
 sock = memory;
 });
 // RSLog(5, RSSTR("RSSocketCreateWithNative(): created socket %p with callbacks 0x%x"), sock, callBackTypes);
 if (sock && !RSSocketIsValid(sock)) { // must do this outside lock to avoid deadlock
 RSRelease(sock);
 sock = nil;
 }
 return sock;
 }
 
 RSSocketHandle RSSocketGetNative(RSSocketRef sock) {
 CHECK_FOR_FORK_RET(INVALID_SOCKET);
 __RSGenericValidInstance(sock, RSSocketGetTypeID());
 return sock->_shared ? sock->_shared->_socket : INVALID_SOCKET;
 }
 
 void RSSocketGetContext(RSSocketRef sock, RSSocketContext *context) {
 __RSGenericValidInstance(sock, RSSocketGetTypeID());
 RSAssert1(0 == context->version, __RSLogAssertion, "%s(): context version not initialized to 0", __PRETTY_FUNCTION__);
 objc_memmove_collectable(context, &sock->_context, sizeof(RSSocketContext));
 }
 
 RSDataRef RSSocketCopyAddress(RSSocketRef sock) {
 CHECK_FOR_FORK_RET(nil);
 __RSGenericValidInstance(sock, RSSocketGetTypeID());
 __block RSDataRef result = nil;
 dispatch_sync(__sockQueue(), ^{
 if (!sock->_address) {
 if (!__RSSocketIsValid(sock)) return;
 uint8_t name[MAX_SOCKADDR_LEN];
 socklen_t namelen = sizeof(name);
 int ret = getsockname(sock->_shared->_socket, (struct sockaddr *)name, (socklen_t *)&namelen);
 if (0 == ret && 0 < namelen) {
 sock->_address = RSDataCreate(RSGetAllocator(sock), name, namelen);
 }
 }
 result = sock->_address ? (RSDataRef)RSRetain(sock->_address) : nil;
 });
 return result;
 }
 
 RSDataRef RSSocketCopyPeerAddress(RSSocketRef sock) {
 CHECK_FOR_FORK_RET(nil);
 __RSGenericValidInstance(sock, RSSocketGetTypeID());
 __block RSDataRef result = nil;
 dispatch_sync(__sockQueue(), ^{
 if (!sock->_peerAddress) {
 if (!__RSSocketIsValid(sock)) return;
 uint8_t name[MAX_SOCKADDR_LEN];
 socklen_t namelen = sizeof(name);
 int ret = getpeername(sock->_shared->_socket, (struct sockaddr *)name, (socklen_t *)&namelen);
 if (0 == ret && 0 < namelen) {
 sock->_peerAddress = RSDataCreate(RSGetAllocator(sock), name, namelen);
 }
 }
 result = sock->_peerAddress ? (RSDataRef)RSRetain(sock->_peerAddress) : nil;
 });
 return result;
 }
 
 RSOptionFlags RSSocketGetSocketFlags(RSSocketRef sock) {
 CHECK_FOR_FORK();
 __RSGenericValidInstance(sock, RSSocketGetTypeID());
 __block RSOptionFlags flags = 0;
 dispatch_sync(__sockQueue(), ^{
 if (sock->_reenableRead) flags |= sock->_wantReadType; // flags are same as types here
 if (sock->_reenableWrite) flags |= RSSocketAutomaticallyReenableWriteCallBack;
 if (sock->_leaveErrors) flags |= RSSocketLeaveErrors;
 if (sock->_closeOnInvalidate) flags |= RSSocketCloseOnInvalidate;
 });
 return flags;
 }
 
 void RSSocketSetSocketFlags(RSSocketRef sock, RSOptionFlags flags) {
 CHECK_FOR_FORK();
 // RSLog(5, RSSTR("RSSocketSetSocketFlags(%p, 0x%x) starting"), sock, flags);
 __RSGenericValidInstance(sock, RSSocketGetTypeID());
 dispatch_sync(__sockQueue(), ^{
 sock->_reenableRead = (sock->_wantReadType && ((flags & 0x3) == sock->_wantReadType)) ? YES : NO;
 sock->_reenableWrite = (sock->_wantWrite && (flags & RSSocketAutomaticallyReenableWriteCallBack)) ? YES : NO;
 sock->_leaveErrors = (flags & RSSocketLeaveErrors) ? YES : NO;
 sock->_closeOnInvalidate = (flags & RSSocketCloseOnInvalidate) ? YES : NO;
 if (sock->_shared) sock->_shared->_closeFD = sock->_closeOnInvalidate;
 });
 // RSLog(5, RSSTR("RSSocketSetSocketFlags(%p, 0x%x) done"), sock, flags);
 }
 
 void RSSocketEnableCallBacks(RSSocketRef sock, RSOptionFlags callBackTypes) {
 CHECK_FOR_FORK_RET();
 __RSGenericValidInstance(sock, RSSocketGetTypeID());
 // RSLog(5, RSSTR("RSSocketEnableCallBacks(%p, 0x%x) starting"), sock, callBackTypes);
 dispatch_sync(__sockQueue(), ^{
 if (!__RSSocketIsValid(sock)) return;
 if (sock->_wantReadType && (callBackTypes & 0x3) == sock->_wantReadType) {
 if (sockfd_is_readable(sock->_shared->_socket)) {
 sock->_readable = YES;
 // RSLog(5, RSSTR("RSSocketEnableCallBacks(%p, 0x%x) socket is readable"), sock, callBackTypes);
 if (!sock->_rsuspended) {
 dispatch_suspend(sock->_shared->_rdsrc);
 sock->_rsuspended = YES;
 }
 // If the source exists, but is now invalid, this next stuff is relatively harmless.
 if (sock->_shared->_source) {
 RSRunLoopSourceSignal(sock->_shared->_source);
 _RSRunLoopSourceWakeUpRunLoops(sock->_shared->_source);
 }
 } else if (sock->_rsuspended && sock->_shared->_rdsrc) {
 sock->_rsuspended = NO;
 dispatch_resume(sock->_shared->_rdsrc);
 }
 sock->_readDisabled = NO;
 }
 if (sock->_wantWrite && (callBackTypes & RSSocketWriteCallBack)) {
 if (sockfd_is_writeable(sock->_shared->_socket)) {
 sock->_writeable = YES;
 if (!sock->_wsuspended) {
 dispatch_suspend(sock->_shared->_wrsrc);
 sock->_wsuspended = YES;
 }
 // If the source exists, but is now invalid, this next stuff is relatively harmless.
 if (sock->_shared->_source) {
 RSRunLoopSourceSignal(sock->_shared->_source);
 _RSRunLoopSourceWakeUpRunLoops(sock->_shared->_source);
 }
 } else if (sock->_wsuspended && sock->_shared->_wrsrc) {
 sock->_wsuspended = NO;
 dispatch_resume(sock->_shared->_wrsrc);
 }
 sock->_writeDisabled = NO;
 }
 if (sock->_wantConnect && !sock->_connected && (callBackTypes & RSSocketConnectCallBack)) {
 if (sockfd_is_writeable(sock->_shared->_socket)) {
 sock->_writeable = YES;
 if (!sock->_wsuspended) {
 dispatch_suspend(sock->_shared->_wrsrc);
 sock->_wsuspended = YES;
 }
 // If the source exists, but is now invalid, this next stuff is relatively harmless.
 if (sock->_shared->_source) {
 RSRunLoopSourceSignal(sock->_shared->_source);
 _RSRunLoopSourceWakeUpRunLoops(sock->_shared->_source);
 }
 } else if (sock->_wsuspended && sock->_shared->_wrsrc) {
 sock->_wsuspended = NO;
 dispatch_resume(sock->_shared->_wrsrc);
 }
 sock->_connectDisabled = NO;
 }
 });
 // RSLog(5, RSSTR("RSSocketEnableCallBacks(%p, 0x%x) done"), sock, callBackTypes);
 }
 
 void RSSocketDisableCallBacks(RSSocketRef sock, RSOptionFlags callBackTypes) {
 CHECK_FOR_FORK_RET();
 __RSGenericValidInstance(sock, RSSocketGetTypeID());
 // RSLog(5, RSSTR("RSSocketDisableCallBacks(%p, 0x%x) starting"), sock, callBackTypes);
 dispatch_sync(__sockQueue(), ^{
 if (!__RSSocketIsValid(sock)) return;
 if (sock->_wantReadType && (callBackTypes & 0x3) == sock->_wantReadType) {
 if (!sock->_rsuspended && sock->_shared->_rdsrc) {
 dispatch_suspend(sock->_shared->_rdsrc);
 sock->_rsuspended = YES;
 }
 sock->_readDisabled = YES;
 }
 if (sock->_wantWrite && (callBackTypes & RSSocketWriteCallBack)) {
 if (!sock->_wsuspended && sock->_shared->_wrsrc) {
 dispatch_suspend(sock->_shared->_wrsrc);
 sock->_wsuspended = YES;
 }
 sock->_writeDisabled = YES;
 }
 if (sock->_wantConnect && !sock->_connected && (callBackTypes & RSSocketConnectCallBack)) {
 if (!sock->_wsuspended && sock->_shared->_wrsrc) {
 dispatch_suspend(sock->_shared->_wrsrc);
 sock->_wsuspended = YES;
 }
 sock->_connectDisabled = YES;
 }
 });
 // RSLog(5, RSSTR("RSSocketDisableCallBacks(%p, 0x%x) done"), sock, callBackTypes);
 }
 
 void RSSocketInvalidate(RSSocketRef sock) {
 CHECK_FOR_FORK_RET();
 __RSGenericValidInstance(sock, RSSocketGetTypeID());
 RSRetain(sock);
 // RSLog(5, RSSTR("RSSocketInvalidate(%p) starting"), sock);
 __block RSRunLoopSourceRef source = nil;
 __block BOOL wasReady = NO;
 dispatch_sync(__sockQueue(), ^{
 wasReady = (sock->_state == RSSocketStateReady);
 if (wasReady) {
 sock->_state = RSSocketStateInvalidating;
 OSMemoryBarrier();
 for (RSIndex idx = 0, cnt = RSArrayGetCount(__RSAllSockets); idx < cnt; idx++) {
 RSSocketRef s = (RSSocketRef)RSArrayGetValueAtIndex(__RSAllSockets, idx);
 if (s == sock) {
 RSArrayRemoveObjectAtIndex(__RSAllSockets, idx);
 break;
 }
 }
 if (sock->_shared->_rdsrc) {
 dispatch_source_cancel(sock->_shared->_rdsrc);
 if (sock->_rsuspended) {
 sock->_rsuspended = NO;
 dispatch_resume(sock->_shared->_rdsrc);
 }
 }
 if (sock->_shared->_wrsrc) {
 dispatch_source_cancel(sock->_shared->_wrsrc);
 if (sock->_wsuspended) {
 sock->_wsuspended = NO;
 dispatch_resume(sock->_shared->_wrsrc);
 }
 }
 source = sock->_shared->_source;
 sock->_shared->_source = nil;
 sock->_shared->_refCnt--;
 if (0 == sock->_shared->_refCnt) {
 if (sock->_shared->_closeFD) {
 // thoroughly stop anything else from using the fd
 (void)shutdown(sock->_shared->_socket, SHUT_RDWR);
 int nullfd = open("/dev/null", O_RDONLY);
 dup2(nullfd, sock->_shared->_socket);
 close(nullfd);
 close(sock->_shared->_socket);
 }
 free(sock->_shared);
 }
 sock->_shared = nil;
 }
 });
 if (wasReady) {
 if (nil != source) {
 RSRunLoopSourceInvalidate(source);
 RSRelease(source);
 }
 void *info = sock->_context.context;
 sock->_context.context = nil;
 if (sock->_context.release) {
 sock->_context.release(info);
 }
 sock->_state = RSSocketStateInvalid;
 OSMemoryBarrier();
 }
 // RSLog(5, RSSTR("RSSocketInvalidate(%p) done%s"), sock, wasReady ? " -- done on this thread" : "");
 RSRelease(sock);
 }
 
 BOOL RSSocketIsValid(RSSocketRef sock) {
 __RSGenericValidInstance(sock, RSSocketGetTypeID());
 if (!__RSSocketIsValid(sock)) return NO;
 struct stat statbuf;
 int ret = sock->_shared ? fstat(sock->_shared->_socket, &statbuf) : -1;
 if (ret < 0) {
 RSSocketInvalidate(sock);
 return NO;
 }
 return YES;
 }
 
 
 static void __RSSocketPerform(void *info) { // RSRunLoop should only call this on one thread at a time
 CHECK_FOR_FORK_RET();
 RSSocketRef sock = (RSSocketRef)info;
 
 // RSLog(5, RSSTR("__RSSocketPerform(%p) starting '%r'"), sock, sock);
 __block BOOL doRead = NO, doWrite = NO, doConnect = NO, isValid = NO;
 __block int fd = INVALID_SOCKET;
 __block SInt32 errorCode = 0;
 __block int new_fd = INVALID_SOCKET;
 __block RSDataRef address = nil;
 __block RSMutableDataRef data = nil;
 __block void *context_info = nil;
 __block void (*context_release)(const void *) = nil;
 dispatch_sync(__sockQueue(), ^{
 isValid = __RSSocketIsValid(sock);
 if (!isValid) return;
 fd = sock->_shared->_socket;
 doRead = sock->_readable && sock->_wantReadType && !sock->_readDisabled;
 if (doRead) {
 sock->_readable = NO;
 doRead = sockfd_is_readable(fd);
 // if (!doRead) RSLog(5, RSSTR("__RSSocketPerform(%p) socket is not actually readable"), sock);
 }
 doWrite = sock->_writeable && sock->_wantWrite && !sock->_writeDisabled;
 doConnect = sock->_writeable && sock->_wantConnect && !sock->_connectDisabled && !sock->_connected;
 if (doWrite || doConnect) {
 sock->_writeable = NO;
 if (doWrite) doWrite = sockfd_is_writeable(fd);
 if (doConnect) doConnect = sockfd_is_writeable(fd);
 }
 if (!sock->_leaveErrors && (doWrite || doConnect)) { // not on read, for whatever reason
 int errorSize = sizeof(errorCode);
 int ret = getsockopt(fd, SOL_SOCKET, SO_ERROR, (void *)&errorCode, (socklen_t *)&errorSize);
 if (0 != ret) errorCode = 0;
 sock->_error = errorCode;
 }
 sock->_connected = YES;
 // RSLog(5, RSSTR("__RSSocketPerform(%p) doing %d %d %d"), sock, doRead, doWrite, doConnect);
 if (doRead) {
 switch (sock->_wantReadType) {
 case RSSocketReadCallBack:
 break;
 case RSSocketAcceptCallBack: {
 uint8_t name[MAX_SOCKADDR_LEN];
 socklen_t namelen = sizeof(name);
 new_fd = accept(fd, (struct sockaddr *)name, (socklen_t *)&namelen);
 if (INVALID_SOCKET != new_fd) {
 address = RSDataCreate(RSGetAllocator(sock), name, namelen);
 }
 break;
 }
 case RSSocketDataCallBack: {
 uint8_t name[MAX_SOCKADDR_LEN];
 socklen_t namelen = sizeof(name);
 int avail = 0;
 int ret = ioctlsocket(fd, FIONREAD, &avail);
 if (ret < 0 || avail < 256) avail = 256;
 if ((1 << 20) < avail) avail = (1 << 20);
 data = RSDataCreateMutable(RSGetAllocator(sock), 0);
 RSDataSetLength(data, avail);
 ssize_t len = recvfrom(fd, RSDataGetMutableBytePtr(data), avail, 0, (struct sockaddr *)name, (socklen_t *)&namelen);
 RSIndex datalen = (len < 0) ? 0 : len;
 RSDataSetLength(data, datalen);
 if (0 < namelen) {
 address = RSDataCreate(RSGetAllocator(sock), name, namelen);
 } else if (sock->_connOriented) {
 // cannot call RSSocketCopyPeerAddress(), or deadlock
 if (!sock->_peerAddress) {
 uint8_t name[MAX_SOCKADDR_LEN];
 socklen_t namelen = sizeof(name);
 int ret = getpeername(sock->_shared->_socket, (struct sockaddr *)name, (socklen_t *)&namelen);
 if (0 == ret && 0 < namelen) {
 sock->_peerAddress = RSDataCreate(RSGetAllocator(sock), name, namelen);
 }
 }
 address = sock->_peerAddress ? (RSDataRef)RSRetain(sock->_peerAddress) : nil;
 }
 if (nil == address) {
 address = RSDataCreate(RSGetAllocator(sock), nil, 0);
 }
 break;
 }
 }
 }
 if (sock->_reenableRead) {
 // RSLog(5, RSSTR("__RSSocketPerform(%p) reenabling read %d %p"), sock, sock->_rsuspended, sock->_shared->_rdsrc);
 if (sock->_rsuspended && sock->_shared->_rdsrc) {
 sock->_rsuspended = NO;
 dispatch_resume(sock->_shared->_rdsrc);
 }
 }
 if (sock->_reenableWrite) {
 if (sock->_wsuspended && sock->_shared->_wrsrc) {
 sock->_wsuspended = NO;
 dispatch_resume(sock->_shared->_wrsrc);
 }
 }
 if (sock->_context.retain && (doConnect || doRead || doWrite)) {
 context_info = (void *)sock->_context.retain(sock->_context.context);
 context_release = sock->_context.release;
 } else {
 context_info = sock->_context.context;
 }
 });
 // RSLog(5, RSSTR("__RSSocketPerform(%p) isValid:%d, doRead:%d, doWrite:%d, doConnect:%d error:%d"), sock, isValid, doRead, doWrite, doConnect, errorCode);
 if (!isValid || !(doConnect || doRead || doWrite)) return;
 
 BOOL calledOut = NO;
 if (doConnect) {
 if (sock->_callout) sock->_callout(sock, RSSocketConnectCallBack, nil, (0 != errorCode) ? &errorCode : nil, context_info);
 calledOut = YES;
 }
 if (doRead && (!calledOut || __RSSocketIsValid(sock))) {
 switch (sock->_wantReadType) {
 case RSSocketReadCallBack:
 if (sock->_callout) sock->_callout(sock, RSSocketReadCallBack, nil, nil, context_info);
 calledOut = YES;
 break;
 case RSSocketAcceptCallBack:
 if (INVALID_SOCKET != new_fd) {
 if (sock->_callout) sock->_callout(sock, RSSocketAcceptCallBack, address, &new_fd, context_info);
 calledOut = YES;
 }
 break;
 case RSSocketDataCallBack:
 if (sock->_callout) sock->_callout(sock, RSSocketDataCallBack, address, data, context_info);
 calledOut = YES;
 break;
 }
 }
 if (doWrite && (!calledOut || __RSSocketIsValid(sock))) {
 if (0 == errorCode) {
 if (sock->_callout) sock->_callout(sock, RSSocketWriteCallBack, nil, nil, context_info);
 calledOut = YES;
 }
 }
 
 if (data && 0 == RSDataGetLength(data)) RSSocketInvalidate(sock);
 if (address) RSRelease(address);
 if (data) RSRelease(data);
 if (context_release) {
 context_release(context_info);
 }
 
 CHECK_FOR_FORK_RET();
 // RSLog(5, RSSTR("__RSSocketPerform(%p) done"), sock);
 }
 
 static void __RSSocketSchedule(void *info, RSRunLoopRef rl, RSStringRef mode) {
 RSSocketRef sock = (RSSocketRef)info;
 int32_t newVal = OSAtomicIncrement32Barrier(&sock->_runLoopCounter);
 if (1 == newVal) { // on a transition from 0->1, the old code forced all desired callbacks enabled
 RSOptionFlags types = sock->_wantReadType | (sock->_wantWrite ? RSSocketWriteCallBack : 0) | (sock->_wantConnect ? RSSocketConnectCallBack : 0);
 RSSocketEnableCallBacks(sock, types);
 }
 RSRunLoopWakeUp(rl);
 }
 
 static void __RSSocketCancel(void *info, RSRunLoopRef rl, RSStringRef mode) {
 RSSocketRef sock = (RSSocketRef)info;
 OSAtomicDecrement32Barrier(&sock->_runLoopCounter);
 RSRunLoopWakeUp(rl);
 }
 
 RSRunLoopSourceRef RSSocketCreateRunLoopSource(RSAllocatorRef allocator, RSSocketRef sock, RSIndex order) {
 CHECK_FOR_FORK_RET(nil);
 __RSGenericValidInstance(sock, RSSocketGetTypeID());
 if (!RSSocketIsValid(sock)) return nil;
 __block RSRunLoopSourceRef result = nil;
 dispatch_sync(__sockQueue(), ^{
 if (!__RSSocketIsValid(sock)) return;
 if (nil != sock->_shared->_source && !RSRunLoopSourceIsValid(sock->_shared->_source)) {
 RSRelease(sock->_shared->_source);
 sock->_shared->_source = nil;
 }
 if (nil == sock->_shared->_source) {
 RSRunLoopSourceContext context;
 context.version = 0;
 context.info = (void *)sock;
 context.retain = (const void *(*)(const void *))RSRetain;
 context.release = (void (*)(const void *))RSRelease;
 context.description = (RSStringRef (*)(const void *))__RSSocketCopyDescription;
 context.equal = nil;
 context.hash = nil;
 context.schedule = __RSSocketSchedule;
 context.cancel = __RSSocketCancel;
 context.perform = __RSSocketPerform;
 sock->_shared->_source = RSRunLoopSourceCreate(allocator, order, (RSRunLoopSourceContext *)&context);
 if (sock->_shared->_source) {
 if (sock->_wantReadType) {
 if (sockfd_is_readable(sock->_shared->_socket)) {
 sock->_readable = YES;
 if (!sock->_rsuspended) {
 dispatch_suspend(sock->_shared->_rdsrc);
 sock->_rsuspended = YES;
 }
 if (sock->_shared->_source) {
 RSRunLoopSourceSignal(sock->_shared->_source);
 _RSRunLoopSourceWakeUpRunLoops(sock->_shared->_source);
 }
 } else if (sock->_rsuspended && sock->_shared->_rdsrc) {
 sock->_rsuspended = NO;
 dispatch_resume(sock->_shared->_rdsrc);
 }
 }
 if (sock->_wantWrite || (sock->_wantConnect && !sock->_connected)) {
 if (sockfd_is_writeable(sock->_shared->_socket)) {
 sock->_writeable = YES;
 if (!sock->_wsuspended) {
 dispatch_suspend(sock->_shared->_wrsrc);
 sock->_wsuspended = YES;
 }
 if (sock->_shared->_source) {
 RSRunLoopSourceSignal(sock->_shared->_source);
 _RSRunLoopSourceWakeUpRunLoops(sock->_shared->_source);
 }
 } else if (sock->_wsuspended && sock->_shared->_wrsrc) {
 sock->_wsuspended = NO;
 dispatch_resume(sock->_shared->_wrsrc);
 }
 }
 }
 }
 result = sock->_shared->_source ? (RSRunLoopSourceRef)RSRetain(sock->_shared->_source) : nil;
 });
 // RSLog(5, RSSTR("RSSocketCreateRunLoopSource(%p) => %p"), sock, result);
 return result;
 }
 
 
 void __RSSocketSetSocketReadBufferAttrs(RSSocketRef s, RSTimeInterval timeout, RSIndex length) {
 }
 
 RSIndex __RSSocketRead(RSSocketRef s, UInt8* buffer, RSIndex length, int* error) {
 *error = 0;
 int ret = read(RSSocketGetNative(s), buffer, length);
 if (ret < 0) {
 *error = errno;
 }
 return ret;
 }
 
 BOOL __RSSocketGetBytesAvailable(RSSocketRef s, RSIndex* ctBytesAvailable) {
 int bytesAvailable;
 int ret = ioctlsocket(RSSocketGetNative(s), FIONREAD, &bytesAvailable);
 if (ret < 0) return NO;
 *ctBytesAvailable = (RSIndex)bytesAvailable;
 return YES;
 }
 
 */
#else /* not NEW_SOCKET */


#include <RSCoreFoundation/RSSocket.h>
#include <sys/types.h>
#include <math.h>
#include <limits.h>
#if DEPLOYMENT_TARGET_MACOSX || DEPLOYMENT_TARGET_EMBEDDED || DEPLOYMENT_TARGET_EMBEDDED_MINI
#include <sys/sysctl.h>
#include <sys/un.h>
#include <libc.h>
#include <dlfcn.h>
#endif
#include <RSCoreFoundation/RSArray.h>
#include <RSCoreFoundation/RSData.h>
#include <RSCoreFoundation/RSDictionary.h>
#include <RSCoreFoundation/RSRunLoop.h>
#include <RSCoreFoundation/RSString.h>
#include <RSCoreFoundation/RSPropertyList.h>
#include "RSInternal.h"

#if DEPLOYMENT_TARGET_WINDOWS

#define EINPROGRESS WSAEINPROGRESS

// redefine this to the winsock error in this file
#undef EBADF
#define EBADF WSAENOTSOCK

#define NBBY 8
#define NFDBITS	(sizeof(int32_t) * NBBY)

typedef int32_t fd_mask;
typedef int socklen_t;

#define gettimeofday _NS_gettimeofday
RSPrivate int _NS_gettimeofday(struct timeval *tv, struct timezone *tz);

// although this is only used for debug info, we define it for compatibility
#define	timersub(tvp, uvp, vvp) \
do { \
(vvp)->tv_sec = (tvp)->tv_sec - (uvp)->tv_sec;		\
(vvp)->tv_usec = (tvp)->tv_usec - (uvp)->tv_usec;	\
if ((vvp)->tv_usec < 0) {				\
(vvp)->tv_sec--;				\
(vvp)->tv_usec += 1000000;			\
}							\
} while (0)


#endif // DEPLOYMENT_TARGET_WINDOWS


// On Mach we use a v0 RunLoopSource to make client callbacks.  That source is signalled by a
// separate SocketManager thread who uses select() to watch the sockets' fds.

//#define LOG_RSSOCKET

#if DEPLOYMENT_TARGET_MACOSX || DEPLOYMENT_TARGET_EMBEDDED || DEPLOYMENT_TARGET_EMBEDDED_MINI
#define INVALID_SOCKET (RSSocketHandle)(-1)
#define closesocket(a) close((a))
#define ioctlsocket(a,b,c) ioctl((a),(b),(c))
#endif

RSInline int __RSSocketLastError(void) {
#if DEPLOYMENT_TARGET_WINDOWS
    return WSAGetLastError();
#else
    return errno;
#endif
}

RSInline RSIndex __RSSocketFdGetSize(RSDataRef fdSet) {
    return NBBY * RSDataGetLength(fdSet);
}

RSInline BOOL __RSSocketFdSet(RSSocketHandle sock, RSMutableDataRef fdSet) {
    /* returns YES if a change occurred, NO otherwise */
    BOOL retval = NO;
    if (INVALID_SOCKET != sock && 0 <= sock) {
        RSIndex numFds = NBBY * RSDataGetLength(fdSet);
        fd_mask *fds_bits;
        if (sock >= numFds) {
            RSIndex oldSize = numFds / NFDBITS, newSize = (sock + NFDBITS) / NFDBITS, changeInBytes = (newSize - oldSize) * sizeof(fd_mask);
            RSDataIncreaseLength(fdSet, changeInBytes);
            fds_bits = (fd_mask *)RSDataGetMutableBytesPtr(fdSet);
            memset(fds_bits + oldSize, 0, changeInBytes);
        } else {
            fds_bits = (fd_mask *)RSDataGetMutableBytesPtr(fdSet);
        }
        if (!FD_ISSET(sock, (fd_set *)fds_bits)) {
            retval = YES;
            FD_SET(sock, (fd_set *)fds_bits);
        }
    }
    return retval;
}


#define MAX_SOCKADDR_LEN 256
#define MAX_DATA_SIZE 65535
#define MAX_CONNECTION_ORIENTED_DATA_SIZE 32768

/* locks are to be acquired in the following order:
 (1) __RSAllSocketsLock
 (2) an individual RSSocket's lock
 (3) __RSActiveSocketsLock
 */
static RSSpinLock __RSAllSocketsLock = RSSpinLockInit; /* controls __RSAllSockets */
static RSMutableDictionaryRef __RSAllSockets = nil;
static RSSpinLock __RSActiveSocketsLock = RSSpinLockInit; /* controls __RSRead/WriteSockets, __RSRead/WriteSocketsFds, __RSSocketManagerThread, and __RSSocketManagerIteration */
static volatile UInt32 __RSSocketManagerIteration = 0;
static RSMutableArrayRef __RSWriteSockets = nil;
static RSMutableArrayRef __RSReadSockets = nil;
static RSMutableDataRef __RSWriteSocketsFds = nil;
static RSMutableDataRef __RSReadSocketsFds = nil;
static RSDataRef zeroLengthData = nil;
static BOOL __RSReadSocketsTimeoutInvalid = YES;  /* rebuild the timeout value before calling select */

static RSSocketHandle __RSWakeupSocketPair[2] = {INVALID_SOCKET, INVALID_SOCKET};
static void *__RSSocketManagerThread = nil;

static void __RSSocketDoCallback(RSSocketRef s, RSDataRef data, RSDataRef address, RSSocketHandle sock);

struct __RSSocket {
    RSRuntimeBase _base;
    struct {
        unsigned client:8;	// flags set by client (reenable, CloseOnInvalidate)
        unsigned disabled:8;	// flags marking disabled callbacks
        unsigned connected:1;	// Are we connected yet?  (also YES for connectionless sockets)
        unsigned writableHint:1;  // Did the polling the socket show it to be writable?
        unsigned closeSignaled:1;  // Have we seen FD_CLOSE? (only used on Win32)
        unsigned unused:13;
    } _f;
    RSSpinLock _lock;
    RSSpinLock _writeLock;
    RSSocketHandle _socket;	/* immutable */
    SInt32 _socketType;
    SInt32 _errorCode;
    RSDataRef _address;
    RSDataRef _peerAddress;
    SInt32 _socketSetCount;
    RSRunLoopSourceRef _source0;	// v0 RLS, messaged from SocketMgr
    RSMutableArrayRef _runLoops;
    RSSocketCallBack _callout;		/* immutable */
    RSSocketContext _context;		/* immutable */
    RSMutableArrayRef _dataQueue;	// queues to pass data from SocketMgr thread
    RSMutableArrayRef _addressQueue;
	
	struct timeval _readBufferTimeout;
	RSMutableDataRef _readBuffer;
	RSIndex _bytesToBuffer;			/* is length of _readBuffer */
	RSIndex _bytesToBufferPos;		/* where the next _RSSocketRead starts from */
	RSIndex _bytesToBufferReadPos;	/* Where the buffer will next be read into (always after _bytesToBufferPos, but less than _bytesToBuffer) */
	BOOL _atEOF;
    int _bufferedReadError;
	
	RSMutableDataRef _leftoverBytes;
};

/* Bit 6 in the base reserved bits is used for write-signalled state (mutable) */
/* Bit 5 in the base reserved bits is used for read-signalled state (mutable) */
/* Bit 4 in the base reserved bits is used for invalid state (mutable) */
/* Bits 0-3 in the base reserved bits are used for callback types (immutable) */
/* Of this, bits 0-1 are used for the read callback type. */

RSInline BOOL __RSSocketIsWriteSignalled(RSSocketRef s) {
    return (BOOL)__RSBitfieldGetValue(RSRuntimeClassBaseFiled(s), 6, 6);
}

RSInline void __RSSocketSetWriteSignalled(RSSocketRef s) {
    __RSBitfieldSetValue(RSRuntimeClassBaseFiled(s), 6, 6, 1);
}

RSInline void __RSSocketUnsetWriteSignalled(RSSocketRef s) {
    __RSBitfieldSetValue(RSRuntimeClassBaseFiled(s), 6, 6, 0);
}

RSInline BOOL __RSSocketIsReadSignalled(RSSocketRef s) {
    return (BOOL)__RSBitfieldGetValue(RSRuntimeClassBaseFiled(s), 5, 5);
}

RSInline void __RSSocketSetReadSignalled(RSSocketRef s) {
    __RSBitfieldSetValue(RSRuntimeClassBaseFiled(s), 5, 5, 1);
}

RSInline void __RSSocketUnsetReadSignalled(RSSocketRef s) {
    __RSBitfieldSetValue(RSRuntimeClassBaseFiled(s), 5, 5, 0);
}

RSInline BOOL __RSSocketIsValid(RSSocketRef s) {
    return (BOOL)__RSBitfieldGetValue(RSRuntimeClassBaseFiled(s), 4, 4);
}

RSInline void __RSSocketSetValid(RSSocketRef s) {
    __RSBitfieldSetValue(RSRuntimeClassBaseFiled(s), 4, 4, 1);
}

RSInline void __RSSocketUnsetValid(RSSocketRef s) {
    __RSBitfieldSetValue(RSRuntimeClassBaseFiled(s), 4, 4, 0);
}

RSInline uint8_t __RSSocketCallBackTypes(RSSocketRef s) {
    return (uint8_t)__RSBitfieldGetValue(RSRuntimeClassBaseFiled(s), 3, 0);
}

RSInline uint8_t __RSSocketReadCallBackType(RSSocketRef s) {
    return (uint8_t)__RSBitfieldGetValue(RSRuntimeClassBaseFiled(s), 1, 0);
}

RSInline void __RSSocketSetCallBackTypes(RSSocketRef s, uint8_t types) {
    __RSBitfieldSetValue(RSRuntimeClassBaseFiled(s), 3, 0, types & 0xF);
}

RSInline void __RSSocketLock(RSSocketRef s) {
    RSSpinLockLock(&(s->_lock));
}

RSInline void __RSSocketUnlock(RSSocketRef s) {
    RSSpinLockUnlock(&(s->_lock));
}

RSInline BOOL __RSSocketIsConnectionOriented(RSSocketRef s) {
    return (SOCK_STREAM == s->_socketType || SOCK_SEQPACKET == s->_socketType);
}

RSInline BOOL __RSSocketIsScheduled(RSSocketRef s) {
    return (s->_socketSetCount > 0);
}

RSInline void __RSSocketEstablishAddress(RSSocketRef s) {
    /* socket should already be locked */
    uint8_t name[MAX_SOCKADDR_LEN];
    int namelen = sizeof(name);
    if (__RSSocketIsValid(s) && nil == s->_address && INVALID_SOCKET != s->_socket && 0 == getsockname(s->_socket, (struct sockaddr *)name, (socklen_t *)&namelen) && nil != name && 0 < namelen) {
        s->_address = RSDataCreate(RSGetAllocator(s), name, namelen);
    }
}

RSInline void __RSSocketEstablishPeerAddress(RSSocketRef s) {
    /* socket should already be locked */
    uint8_t name[MAX_SOCKADDR_LEN];
    int namelen = sizeof(name);
    if (__RSSocketIsValid(s) && nil == s->_peerAddress && INVALID_SOCKET != s->_socket && 0 == getpeername(s->_socket, (struct sockaddr *)name, (socklen_t *)&namelen) && nil != name && 0 < namelen) {
        s->_peerAddress = RSDataCreate(RSGetAllocator(s), name, namelen);
    }
}

static BOOL __RSNativeSocketIsValid(RSSocketHandle sock) {
#if DEPLOYMENT_TARGET_WINDOWS
    SInt32 errorCode = 0;
    int errorSize = sizeof(errorCode);
    return !(0 != getsockopt(sock, SOL_SOCKET, SO_ERROR, (char *)&errorCode, &errorSize) && __RSSocketLastError() == WSAENOTSOCK);
#else
    SInt32 flags = fcntl(sock, F_GETFL, 0);
    return !(0 > flags && EBADF == __RSSocketLastError());
#endif
}

RSInline BOOL __RSSocketFdClr(RSSocketHandle sock, RSMutableDataRef fdSet) {
    /* returns YES if a change occurred, NO otherwise */
    BOOL retval = NO;
    if (INVALID_SOCKET != sock && 0 <= sock) {
        RSIndex numFds = NBBY * RSDataGetLength(fdSet);
        fd_mask *fds_bits;
        if (sock < numFds) {
            fds_bits = (fd_mask *)RSDataGetMutableBytesPtr(fdSet);
            if (FD_ISSET(sock, (fd_set *)fds_bits)) {
                retval = YES;
                FD_CLR(sock, (fd_set *)fds_bits);
            }
        }
    }
    return retval;
}

static SInt32 __RSSocketCreateWakeupSocketPair(void) {
#if DEPLOYMENT_TARGET_MACOSX || DEPLOYMENT_TARGET_EMBEDDED || DEPLOYMENT_TARGET_EMBEDDED_MINI
    SInt32 error;
    
    error = socketpair(PF_LOCAL, SOCK_DGRAM, 0, __RSWakeupSocketPair);
    if (0 <= error) error = fcntl(__RSWakeupSocketPair[0], F_SETFD, FD_CLOEXEC);
    if (0 <= error) error = fcntl(__RSWakeupSocketPair[1], F_SETFD, FD_CLOEXEC);
    if (0 > error) {
        closesocket(__RSWakeupSocketPair[0]);
        closesocket(__RSWakeupSocketPair[1]);
        __RSWakeupSocketPair[0] = INVALID_SOCKET;
        __RSWakeupSocketPair[1] = INVALID_SOCKET;
    }
#else
    UInt32 i;
    SInt32 error = 0;
    struct sockaddr_in address[2];
    int namelen = sizeof(struct sockaddr_in);
    for (i = 0; i < 2; i++) {
        __RSWakeupSocketPair[i] = socket(PF_INET, SOCK_DGRAM, 0);
        memset(&(address[i]), 0, sizeof(struct sockaddr_in));
        address[i].sin_family = AF_INET;
        address[i].sin_addr.s_addr = htonl(INADDR_LOOPBACK);
        if (0 <= error) error = bind(__RSWakeupSocketPair[i], (struct sockaddr *)&(address[i]), sizeof(struct sockaddr_in));
        if (0 <= error) error = getsockname(__RSWakeupSocketPair[i], (struct sockaddr *)&(address[i]), &namelen);
        if (sizeof(struct sockaddr_in) != namelen) error = -1;
    }
    if (0 <= error) error = connect(__RSWakeupSocketPair[0], (struct sockaddr *)&(address[1]), sizeof(struct sockaddr_in));
    if (0 <= error) error = connect(__RSWakeupSocketPair[1], (struct sockaddr *)&(address[0]), sizeof(struct sockaddr_in));
    if (0 > error) {
        closesocket(__RSWakeupSocketPair[0]);
        closesocket(__RSWakeupSocketPair[1]);
        __RSWakeupSocketPair[0] = INVALID_SOCKET;
        __RSWakeupSocketPair[1] = INVALID_SOCKET;
    }
#endif
#if defined(LOG_RSSOCKET)
    fprintf(stdout, "wakeup socket pair is %d / %d\n", __RSWakeupSocketPair[0], __RSWakeupSocketPair[1]);
#endif
    return error;
}


// Version 0 RunLoopSources set a mask in an FD set to control what socket activity we hear about.
// Changes to the master fs_sets occur via these 4 functions.
RSInline BOOL __RSSocketSetFDForRead(RSSocketRef s) {
    __RSReadSocketsTimeoutInvalid = YES;
    BOOL b = __RSSocketFdSet(s->_socket, __RSReadSocketsFds);
    if (b && INVALID_SOCKET != __RSWakeupSocketPair[0]) {
        uint8_t c = 'r';
        send(__RSWakeupSocketPair[0], (const char *)&c, sizeof(c), 0);
    }
    return b;
}

RSInline BOOL __RSSocketClearFDForRead(RSSocketRef s) {
    __RSReadSocketsTimeoutInvalid = YES;
    BOOL b = __RSSocketFdClr(s->_socket, __RSReadSocketsFds);
    if (b && INVALID_SOCKET != __RSWakeupSocketPair[0]) {
        uint8_t c = 's';
        send(__RSWakeupSocketPair[0], (const char *)&c, sizeof(c), 0);
    }
    return b;
}

RSInline BOOL __RSSocketSetFDForWrite(RSSocketRef s) {
    // RSLog(5, RSSTR("__RSSocketSetFDForWrite(%p)"), s);
    BOOL b = __RSSocketFdSet(s->_socket, __RSWriteSocketsFds);
    if (b && INVALID_SOCKET != __RSWakeupSocketPair[0]) {
        uint8_t c = 'w';
        send(__RSWakeupSocketPair[0], (const char *)&c, sizeof(c), 0);
    }
    return b;
}

RSInline BOOL __RSSocketClearFDForWrite(RSSocketRef s) {
    // RSLog(5, RSSTR("__RSSocketClearFDForWrite(%p)"), s);
    BOOL b = __RSSocketFdClr(s->_socket, __RSWriteSocketsFds);
    if (b && INVALID_SOCKET != __RSWakeupSocketPair[0]) {
        uint8_t c = 'x';
        send(__RSWakeupSocketPair[0], (const char *)&c, sizeof(c), 0);
    }
    return b;
}

#if DEPLOYMENT_TARGET_WINDOWS
static BOOL WinSockUsed = FALSE;

static void __RSSocketInitializeWinSock_Guts(void) {
    if (!WinSockUsed) {
        WinSockUsed = TRUE;
        WORD versionRequested = MAKEWORD(2, 2);
        WSADATA wsaData;
        int errorStatus = WSAStartup(versionRequested, &wsaData);
        if (errorStatus != 0 || LOBYTE(wsaData.wVersion) != LOBYTE(versionRequested) || HIBYTE(wsaData.wVersion) != HIBYTE(versionRequested)) {
            WSACleanup();
            RSLog(RSLogLevelWarning, RSSTR("*** Could not initialize WinSock subsystem!!!"));
        }
    }
}

RS_EXPORT void __RSSocketInitializeWinSock(void) {
    RSSpinLockLock(&__RSActiveSocketsLock);
    __RSSocketInitializeWinSock_Guts();
    RSSpinLockUnlock(&__RSActiveSocketsLock);
}

RS_PRIVATE void __RSSocketCleanup(void) {
    if (INVALID_SOCKET != __RSWakeupSocketPair[0]) {
        closesocket(__RSWakeupSocketPair[0]);
        __RSWakeupSocketPair[0] = INVALID_SOCKET;
    }
    if (INVALID_SOCKET != __RSWakeupSocketPair[1]) {
        closesocket(__RSWakeupSocketPair[1]);
        __RSWakeupSocketPair[1] = INVALID_SOCKET;
    }
    if (WinSockUsed) {
        // technically this is not supposed to be called here since it will be called from dllmain, but I don't know where else to put it
        WSACleanup();
    }
}

#endif

// RSNetwork needs to call this, especially for Win32 to get WSAStartup
static void __RSSocketInitializeSockets(void) {
    __RSWriteSockets = __RSArrayCreateMutable0(RSAllocatorSystemDefault, 0, nil);
    __RSReadSockets = __RSArrayCreateMutable0(RSAllocatorSystemDefault, 0, nil);
    __RSWriteSocketsFds = RSDataCreateMutable(RSAllocatorSystemDefault, 0);
    __RSReadSocketsFds = RSDataCreateMutable(RSAllocatorSystemDefault, 0);
    zeroLengthData = RSDataCreateMutable(RSAllocatorSystemDefault, 0);
#if DEPLOYMENT_TARGET_WINDOWS
    __RSSocketInitializeWinSock_Guts();
#endif
    if (0 > __RSSocketCreateWakeupSocketPair()) {
        RSLog(RSLogLevelWarning, RSSTR("*** Could not create wakeup socket pair for RSSocket!!!"));
    } else {
        UInt32 yes = 1;
        /* wakeup sockets must be non-blocking */
        ioctlsocket(__RSWakeupSocketPair[0], FIONBIO, (u_long *)&yes);
        ioctlsocket(__RSWakeupSocketPair[1], FIONBIO, (u_long *)&yes);
        __RSSocketFdSet(__RSWakeupSocketPair[1], __RSReadSocketsFds);
    }
}

static RSRunLoopRef __RSSocketCopyRunLoopToWakeUp(RSRunLoopSourceRef src, RSMutableArrayRef runLoops) {
    if (!src) return nil;
    RSRunLoopRef rl = nil;
    RSIndex idx, cnt = RSArrayGetCount(runLoops);
    if (0 < cnt) {
        rl = (RSRunLoopRef)RSArrayObjectAtIndex(runLoops, 0);
        for (idx = 1; nil != rl && idx < cnt; idx++) {
            RSRunLoopRef value = (RSRunLoopRef)RSArrayObjectAtIndex(runLoops, idx);
            if (value != rl) rl = nil;
        }
        if (nil == rl) {	/* more than one different rl, so we must pick one */
            /* ideally, this would be a run loop which isn't also in a
             * signaled state for this or another source, but that's tricky;
             * we pick one that is running in an appropriate mode for this
             * source, and from those if possible one that is waiting; then
             * we move this run loop to the end of the list to scramble them
             * a bit, and always search from the front */
            BOOL foundIt = NO, foundBackup = NO;
            RSIndex foundIdx = 0;
            for (idx = 0; !foundIt && idx < cnt; idx++) {
                RSRunLoopRef value = (RSRunLoopRef)RSArrayObjectAtIndex(runLoops, idx);
                RSStringRef currentMode = RSRunLoopCopyCurrentMode(value);
                if (nil != currentMode) {
                    if (RSRunLoopContainsSource(value, src, currentMode)) {
                        if (RSRunLoopIsWaiting(value)) {
                            foundIdx = idx;
                            foundIt = YES;
                        } else if (!foundBackup) {
                            foundIdx = idx;
                            foundBackup = YES;
                        }
                    }
                    RSRelease(currentMode);
                }
            }
            rl = (RSRunLoopRef)RSArrayObjectAtIndex(runLoops, foundIdx);
            RSRetain(rl);
            RSArrayRemoveObjectAtIndex(runLoops, foundIdx);
            RSArrayAddObject(runLoops, rl);
        } else {
            RSRetain(rl);
        }
    }
    return rl;
}

// If callBackNow, we immediately do client callbacks, else we have to signal a v0 RunLoopSource so the
// callbacks can happen in another thread.
static void __RSSocketHandleWrite(RSSocketRef s, BOOL callBackNow) {
    SInt32 errorCode = 0;
    int errorSize = sizeof(errorCode);
    RSOptionFlags writeCallBacksAvailable;
    
    if (!RSSocketIsValid(s)) return;
    if (0 != (s->_f.client & RSSocketLeaveErrors) || 0 != getsockopt(s->_socket, SOL_SOCKET, SO_ERROR, (char *)&errorCode, (socklen_t *)&errorSize)) errorCode = 0;
    // cast for WinSock bad API
#if defined(LOG_RSSOCKET)
    if (errorCode) fprintf(stdout, "error %ld on socket %d\n", (long)errorCode, s->_socket);
#endif
    __RSSocketLock(s);
    writeCallBacksAvailable = __RSSocketCallBackTypes(s) & (RSSocketWriteCallBack | RSSocketConnectCallBack);
    if ((s->_f.client & RSSocketConnectCallBack) != 0) writeCallBacksAvailable &= ~RSSocketConnectCallBack;
    if (!__RSSocketIsValid(s) || ((s->_f.disabled & writeCallBacksAvailable) == writeCallBacksAvailable)) {
        __RSSocketUnlock(s);
        return;
    }
    s->_errorCode = errorCode;
    __RSSocketSetWriteSignalled(s);
    // RSLog(5, RSSTR("__RSSocketHandleWrite() signalling write on socket %p"), s);
#if defined(LOG_RSSOCKET)
    fprintf(stdout, "write signaling source for socket %d\n", s->_socket);
#endif
    if (callBackNow) {
        __RSSocketDoCallback(s, nil, nil, 0);
    } else {
        RSRunLoopSourceSignal(s->_source0);
        RSMutableArrayRef runLoopsOrig = (RSMutableArrayRef)RSRetain(s->_runLoops);
        RSMutableArrayRef runLoopsCopy = RSMutableCopy(RSAllocatorSystemDefault, s->_runLoops);
        RSRunLoopSourceRef source0 = s->_source0;
        if (nil != source0 && !RSRunLoopSourceIsValid(source0)) {
            source0 = nil;
        }
        if (source0) RSRetain(source0);
        __RSSocketUnlock(s);
        RSRunLoopRef rl = __RSSocketCopyRunLoopToWakeUp(source0, runLoopsCopy);
        if (source0) RSRelease(source0);
        if (nil != rl) {
            RSRunLoopWakeUp(rl);
            RSRelease(rl);
        }
        __RSSocketLock(s);
        if (runLoopsOrig == s->_runLoops) {
            s->_runLoops = runLoopsCopy;
            runLoopsCopy = nil;
            RSRelease(runLoopsOrig);
        }
        __RSSocketUnlock(s);
        RSRelease(runLoopsOrig);
        if (runLoopsCopy) RSRelease(runLoopsCopy);
    }
}

static void __RSSocketHandleRead(RSSocketRef s, BOOL causedByTimeout)
{
    RSDataRef data = nil, address = nil;
    RSSocketHandle sock = INVALID_SOCKET;
    if (!RSSocketIsValid(s)) return;
    if (__RSSocketReadCallBackType(s) == RSSocketDataCallBack) {
        uint8_t bufferArray[MAX_CONNECTION_ORIENTED_DATA_SIZE], *buffer;
        uint8_t name[MAX_SOCKADDR_LEN];
        int namelen = sizeof(name);
        ssize_t recvlen = 0;
        if (__RSSocketIsConnectionOriented(s)) {
            buffer = bufferArray;
            recvlen = recvfrom(s->_socket, (char *)buffer, MAX_CONNECTION_ORIENTED_DATA_SIZE, 0, (struct sockaddr *)name, (socklen_t *)&namelen);
        } else {
            buffer = (uint8_t *)malloc(MAX_DATA_SIZE);
            if (buffer) recvlen = recvfrom(s->_socket, (char *)buffer, MAX_DATA_SIZE, 0, (struct sockaddr *)name, (socklen_t *)&namelen);
        }
#if defined(LOG_RSSOCKET)
        fprintf(stdout, "read %ld bytes on socket %d\n", (long)recvlen, s->_socket);
#endif
        if (0 >= recvlen) {
            //??? should return error if <0
            /* zero-length data is the signal for perform to invalidate */
            data = (RSDataRef)RSRetain(zeroLengthData);
        } else {
            data = RSDataCreate(RSGetAllocator(s), buffer, recvlen);
        }
        if (buffer && buffer != bufferArray) free(buffer);
        __RSSocketLock(s);
        if (!__RSSocketIsValid(s)) {
            RSRelease(data);
            __RSSocketUnlock(s);
            return;
        }
        __RSSocketSetReadSignalled(s);
        if (nil != name && 0 < namelen) {
            //??? possible optimizations:  uniquing; storing last value
            address = RSDataCreate(RSGetAllocator(s), name, namelen);
        } else if (__RSSocketIsConnectionOriented(s)) {
            if (nil == s->_peerAddress) __RSSocketEstablishPeerAddress(s);
            if (nil != s->_peerAddress) address = (RSDataRef)RSRetain(s->_peerAddress);
        }
        if (nil == address) {
            address = (RSDataRef)RSRetain(zeroLengthData);
        }
        if (nil == s->_dataQueue) {
            s->_dataQueue = RSArrayCreateMutable(RSGetAllocator(s), 0);
        }
        if (nil == s->_addressQueue) {
            s->_addressQueue = RSArrayCreateMutable(RSGetAllocator(s), 0);
        }
        RSArrayRemoveObjectAtIndex(s->_dataQueue, data);
        RSRelease(data);
        RSArrayRemoveObjectAtIndex(s->_addressQueue, address);
        RSRelease(address);
        if (0 < recvlen
            && (s->_f.client & RSSocketDataCallBack) != 0 && (s->_f.disabled & RSSocketDataCallBack) == 0
            && __RSSocketIsScheduled(s)
            ) {
            RSSpinLockLock(&__RSActiveSocketsLock);
            /* restore socket to fds */
            __RSSocketSetFDForRead(s);
            RSSpinLockUnlock(&__RSActiveSocketsLock);
        }
    } else if (__RSSocketReadCallBackType(s) == RSSocketAcceptCallBack) {
        uint8_t name[MAX_SOCKADDR_LEN];
        int namelen = sizeof(name);
        sock = accept(s->_socket, (struct sockaddr *)name, (socklen_t *)&namelen);
        if (INVALID_SOCKET == sock) {
            //??? should return error
            return;
        }
        if (nil != name && 0 < namelen) {
            address = RSDataCreate(RSGetAllocator(s), name, namelen);
        } else {
            address = (RSDataRef)RSRetain(zeroLengthData);
        }
        __RSSocketLock(s);
        if (!__RSSocketIsValid(s)) {
            closesocket(sock);
            RSRelease(address);
            __RSSocketUnlock(s);
            return;
        }
        __RSSocketSetReadSignalled(s);
        if (nil == s->_dataQueue) {
            s->_dataQueue = __RSArrayCreateMutable0(RSGetAllocator(s), 0, nil);
        }
        if (nil == s->_addressQueue) {
            s->_addressQueue = RSArrayCreateMutable(RSGetAllocator(s), 0);
        }
        RSArrayRemoveObjectAtIndex(s->_dataQueue, (void *)(uintptr_t)sock);
        RSArrayRemoveObjectAtIndex(s->_addressQueue, address);
        RSRelease(address);
        if ((s->_f.client & RSSocketAcceptCallBack) != 0 && (s->_f.disabled & RSSocketAcceptCallBack) == 0
            && __RSSocketIsScheduled(s)
            ) {
            RSSpinLockLock(&__RSActiveSocketsLock);
            /* restore socket to fds */
            __RSSocketSetFDForRead(s);
            RSSpinLockUnlock(&__RSActiveSocketsLock);
        }
    } else {
        __RSSocketLock(s);
        if (!__RSSocketIsValid(s) || (s->_f.disabled & RSSocketReadCallBack) != 0) {
            __RSSocketUnlock(s);
            return;
        }
		
		if (causedByTimeout) {
#if defined(LOG_RSSOCKET)
			fprintf(stdout, "TIMEOUT RECEIVED - WILL SIGNAL IMMEDIATELY TO FLUSH (%ld buffered)\n", s->_bytesToBufferPos);
#endif
            /* we've got a timeout, but no bytes read, and we don't have any bytes to send.  Ignore the timeout. */
            if (s->_bytesToBufferPos == 0 && s->_leftoverBytes == nil) {
#if defined(LOG_RSSOCKET)
                fprintf(stdout, "TIMEOUT - but no bytes, restoring to active set\n");
                fflush(stdout);
#endif
                
                RSSpinLockLock(&__RSActiveSocketsLock);
                /* restore socket to fds */
                __RSSocketSetFDForRead(s);
                RSSpinLockUnlock(&__RSActiveSocketsLock);
                __RSSocketUnlock(s);
                return;
            }
		} else if (s->_bytesToBuffer != 0 && ! s->_atEOF) {
			UInt8* base;
			RSIndex ctRead;
			RSIndex ctRemaining = s->_bytesToBuffer - s->_bytesToBufferPos;
            
			/* if our buffer has room, we go ahead and buffer */
			if (ctRemaining > 0) {
				base = RSDataGetMutableBytesPtr(s->_readBuffer);
                
				do {
					ctRead = read(RSSocketGetHandle(s), &base[s->_bytesToBufferPos], ctRemaining);
				} while (ctRead == -1 && errno == EAGAIN);
                
				switch (ctRead) {
                    case -1:
                        s->_bufferedReadError = errno;
                        s->_atEOF = YES;
#if defined(LOG_RSSOCKET)
                        fprintf(stderr, "BUFFERED READ GOT ERROR %d\n", errno);
#endif
                        break;
                        
                    case 0:
#if defined(LOG_RSSOCKET)
                        fprintf(stdout, "DONE READING (EOF) - GOING TO SIGNAL\n");
#endif
                        s->_atEOF = YES;
                        break;
                        
                    default:
                        s->_bytesToBufferPos += ctRead;
                        if (s->_bytesToBuffer != s->_bytesToBufferPos) {
#if defined(LOG_RSSOCKET)
                            fprintf(stdout, "READ %ld - need %ld MORE - GOING BACK FOR MORE\n", ctRead, s->_bytesToBuffer - s->_bytesToBufferPos);
#endif
                            RSSpinLockLock(&__RSActiveSocketsLock);
                            /* restore socket to fds */
                            __RSSocketSetFDForRead(s);
                            RSSpinLockUnlock(&__RSActiveSocketsLock);
                            __RSSocketUnlock(s);
                            return;
                        } else {
#if defined(LOG_RSSOCKET)
                            fprintf(stdout, "DONE READING (read %ld bytes) - GOING TO SIGNAL\n", ctRead);
#endif
                        }
				}
			}
		}
        
		__RSSocketSetReadSignalled(s);
    }
#if defined(LOG_RSSOCKET)
    fprintf(stdout, "read signaling source for socket %d\n", s->_socket);
#endif
    RSRunLoopSourceSignal(s->_source0);
    RSMutableArrayRef runLoopsOrig = (RSMutableArrayRef)RSRetain(s->_runLoops);
    RSMutableArrayRef runLoopsCopy = RSMutableCopy(RSAllocatorSystemDefault, s->_runLoops);
    RSRunLoopSourceRef source0 = s->_source0;
    if (nil != source0 && !RSRunLoopSourceIsValid(source0)) {
        source0 = nil;
    }
    if (source0) RSRetain(source0);
    __RSSocketUnlock(s);
    RSRunLoopRef rl = __RSSocketCopyRunLoopToWakeUp(source0, runLoopsCopy);
    if (source0) RSRelease(source0);
    if (nil != rl) {
        RSRunLoopWakeUp(rl);
        RSRelease(rl);
    }
    __RSSocketLock(s);
    if (runLoopsOrig == s->_runLoops) {
        s->_runLoops = runLoopsCopy;
        runLoopsCopy = nil;
        RSRelease(runLoopsOrig);
    }
    __RSSocketUnlock(s);
    RSRelease(runLoopsOrig);
    if (runLoopsCopy) RSRelease(runLoopsCopy);
}

static struct timeval* intervalToTimeval(RSTimeInterval timeout, struct timeval* tv)
{
    if (timeout == 0.0)
        timerclear(tv);
    else {
        tv->tv_sec = (0 >= timeout || INT_MAX <= timeout) ? INT_MAX : (int)(float)floor(timeout);
        tv->tv_usec = (int)((timeout - floor(timeout)) * 1.0E6);
    }
    return tv;
}

/* note that this returns a pointer to the min value, which won't have changed during
 the dictionary apply, since we've got the active sockets lock held */
static void _calcMinTimeout_locked(const void* val, void* ctxt)
{
    RSSocketRef s = (RSSocketRef) val;
    struct timeval** minTime = (struct timeval**) ctxt;
    if (timerisset(&s->_readBufferTimeout) && (*minTime == nil || timercmp(&s->_readBufferTimeout, *minTime, <)))
        *minTime = &s->_readBufferTimeout;
    else if (s->_leftoverBytes) {
        /* If there's anyone with leftover bytes, they'll need to be awoken immediately */
        static struct timeval sKickerTime = { 0, 0 };
        *minTime = &sKickerTime;
    }
}

void __RSSocketSetSocketReadBufferAttrs(RSSocketRef s, RSTimeInterval timeout, RSIndex length)
{
    struct timeval timeoutVal;
    
    intervalToTimeval(timeout, &timeoutVal);
    
    /* lock ordering is socket lock, activesocketslock */
    /* activesocketslock protects our timeout calculation */
    __RSSocketLock(s);
    RSSpinLockLock(&__RSActiveSocketsLock);
    
    if (s->_bytesToBuffer != length) {
        RSIndex ctBuffer = s->_bytesToBufferPos - s->_bytesToBufferReadPos;
        
        if (ctBuffer) {
            /* As originally envisaged, you were supposed to be sure to drain the buffer before
             * issuing another request on the socket.  In practice, there seem to be times when we want to re-use
             * the stream (or perhaps, are on our way to closing it out) and this policy doesn't work so well.
             * So, if someone changes the buffer size while we have bytes already buffered, we put them
             * aside and use them to satisfy any subsequent reads.
             */
#if defined(LOG_RSSOCKET)
            fprintf(stdout, "%s(%d): WARNING: shouldn't set read buffer length while data (%ld bytes) is still in the read buffer (leftover total %ld)", __FUNCTION__, __LINE__, ctBuffer, s->_leftoverBytes? RSDataGetLength(s->_leftoverBytes) : 0);
#endif
            
            if (s->_leftoverBytes == nil)
                s->_leftoverBytes = RSDataCreateMutable(RSGetAllocator(s), 0);
            
            /* append the current buffered bytes over.  We'll keep draining _leftoverBytes while we have them... */
            RSDataAppendBytes(s->_leftoverBytes, RSDataGetBytesPtr(s->_readBuffer) + s->_bytesToBufferReadPos, ctBuffer);
            RSRelease(s->_readBuffer);
            s->_readBuffer = nil;
            
            s->_bytesToBuffer = 0;
            s->_bytesToBufferPos = 0;
            s->_bytesToBufferReadPos = 0;
        }
        if (length == 0) {
            s->_bytesToBuffer = 0;
            s->_bytesToBufferPos = 0;
            s->_bytesToBufferReadPos = 0;
            if (s->_readBuffer) {
                RSRelease(s->_readBuffer);
                s->_readBuffer = nil;
            }
            // Zero length buffer, smash the timeout
            timeoutVal.tv_sec = 0;
            timeoutVal.tv_usec = 0;
        } else {
            /* if the buffer shrank, we can re-use the old one */
            if (length > s->_bytesToBuffer) {
                if (s->_readBuffer) {
                    RSRelease(s->_readBuffer);
                    s->_readBuffer = nil;
                }
            }
            
            s->_bytesToBuffer = length;
            s->_bytesToBufferPos = 0;
            s->_bytesToBufferReadPos = 0;
            if (s->_readBuffer == nil) {
                s->_readBuffer = RSDataCreateMutable(RSAllocatorSystemDefault, length);
                RSDataSetLength(s->_readBuffer, length);
            }
        }
    }
    
    if (timercmp(&s->_readBufferTimeout, &timeoutVal, !=)) {
        s->_readBufferTimeout = timeoutVal;
        __RSReadSocketsTimeoutInvalid = YES;
    }
    
    RSSpinLockUnlock(&__RSActiveSocketsLock);
    __RSSocketUnlock(s);
}

RSIndex __RSSocketRead(RSSocketRef s, UInt8* buffer, RSIndex length, int* error)
{
#if defined(LOG_RSSOCKET)
	fprintf(stdout, "READING BYTES FOR SOCKET %d (%ld buffered, out of %ld desired, eof = %d, err = %d)\n", s->_socket, s->_bytesToBufferPos, s->_bytesToBuffer, s->_atEOF, s->_bufferedReadError);
#endif
    
    RSIndex result = -1;
    
    __RSSocketLock(s);
    
	*error = 0;
	
	/* Any leftover buffered bytes? */
	if (s->_leftoverBytes) {
		RSIndex ctBuffer = RSDataGetLength(s->_leftoverBytes);
#if defined(DEBUG)
		fprintf(stderr, "%s(%ld): WARNING: Draining %ld leftover bytes first\n\n", __FUNCTION__, (long)__LINE__, (long)ctBuffer);
#endif
		if (ctBuffer > length)
			ctBuffer = length;
		memcpy(buffer, RSDataGetBytesPtr(s->_leftoverBytes), ctBuffer);
		if (ctBuffer < RSDataGetLength(s->_leftoverBytes))
			RSDataReplaceBytes(s->_leftoverBytes, RSMakeRange(0, ctBuffer), nil, 0);
		else {
			RSRelease(s->_leftoverBytes);
			s->_leftoverBytes = nil;
		}
		result = ctBuffer;
		goto unlock;
	}
	
	/* return whatever we've buffered */
	if (s->_bytesToBuffer != 0) {
		RSIndex ctBuffer = s->_bytesToBufferPos - s->_bytesToBufferReadPos;
		if (ctBuffer > 0) {
			/* drain our buffer first */
			if (ctBuffer > length)
				ctBuffer = length;
			memcpy(buffer, RSDataGetBytesPtr(s->_readBuffer) + s->_bytesToBufferReadPos, ctBuffer);
			s->_bytesToBufferReadPos += ctBuffer;
			if (s->_bytesToBufferReadPos == s->_bytesToBufferPos) {
#if defined(LOG_RSSOCKET)
				fprintf(stdout, "DRAINED BUFFER - SHOULD START BUFFERING AGAIN!\n");
#endif
				s->_bytesToBufferPos = 0;
				s->_bytesToBufferReadPos = 0;
			}
			
#if defined(LOG_RSSOCKET)
			fprintf(stdout, "SLURPED %ld BYTES FROM BUFFER %ld LEFT TO READ!\n", ctBuffer, length);
#endif
            
			result = ctBuffer;
            goto unlock;
		}
	}
	/* nothing buffered, or no buffer selected */
	
	/* Did we get an error on a previous read (or buffered read)? */
	if (s->_bufferedReadError != 0) {
#if defined(LOG_RSSOCKET)
		fprintf(stdout, "RETURNING ERROR %d\n", s->_bufferedReadError);
#endif
		*error = s->_bufferedReadError;
        result = -1;
        goto unlock;
	}
	
	/* nothing buffered, if we've hit eof, don't bother reading any more */
	if (s->_atEOF) {
#if defined(LOG_RSSOCKET)
		fprintf(stdout, "RETURNING EOF\n");
#endif
		result = 0;
        goto unlock;
	}
	
	/* normal read */
	result = read(RSSocketGetHandle(s), buffer, length);
#if defined(LOG_RSSOCKET)
	fprintf(stdout, "READ %ld bytes", result);
#endif
    
    if (result == 0) {
        /* note that we hit EOF */
        s->_atEOF = YES;
    } else if (result < 0) {
        *error = errno;
        
        /* if it wasn't EAGAIN, record it (although we shouldn't get called again) */
        if (*error != EAGAIN) {
            s->_bufferedReadError = *error;
        }
    }
    
unlock:
    __RSSocketUnlock(s);
    
    return result;
}

BOOL __RSSocketGetBytesAvailable(RSSocketRef s, RSIndex* ctBytesAvailable)
{
	RSIndex ctBuffer = s->_bytesToBufferPos - s->_bytesToBufferReadPos;
	if (ctBuffer != 0) {
		*ctBytesAvailable = ctBuffer;
		return YES;
	} else {
		int result;
	    unsigned long bytesAvailable;
	    result = ioctlsocket(RSSocketGetHandle(s), FIONREAD, &bytesAvailable);
		if (result < 0)
			return NO;
		*ctBytesAvailable = (RSIndex) bytesAvailable;
		return YES;
	}
}

#if defined(LOG_RSSOCKET)
static void __RSSocketWriteSocketList(RSArrayRef sockets, RSDataRef fdSet, BOOL onlyIfSet) {
    fd_set *tempfds = (fd_set *)RSDataGetBytesPtr(fdSet);
    SInt32 idx, cnt;
    for (idx = 0, cnt = RSArrayGetCount(sockets); idx < cnt; idx++) {
        RSSocketRef s = (RSSocketRef)RSArrayGetValueAtIndex(sockets, idx);
        if (FD_ISSET(s->_socket, tempfds)) {
            fprintf(stdout, "%d ", s->_socket);
        } else if (!onlyIfSet) {
            fprintf(stdout, "(%d) ", s->_socket);
        }
    }
}
#endif

static void
clearInvalidFileDescriptors(RSMutableDataRef d)
{
    if (d) {
        RSIndex count = __RSSocketFdGetSize(d);
        fd_set* s = (fd_set*) RSDataGetMutableBytesPtr(d);
        for (SInt32 idx = 0;  idx < count;  idx++) {
            if (FD_ISSET(idx, s))
                if (! __RSNativeSocketIsValid(idx)) {
                    FD_CLR(idx, s);
                }
        }
    }
}

static void
manageSelectError()
{
    SInt32 selectError = __RSSocketLastError();
#if defined(LOG_RSSOCKET)
    fprintf(stdout, "socket manager received error %ld from select\n", (long)selectError);
#endif
    if (EBADF == selectError) {
        RSMutableArrayRef invalidSockets = RSArrayCreateMutable(RSAllocatorSystemDefault, 0);
        
        RSSpinLockLock(&__RSActiveSocketsLock);
        RSIndex cnt = RSArrayGetCount(__RSWriteSockets);
        RSIndex idx;
        for (idx = 0; idx < cnt; idx++) {
            RSSocketRef s = (RSSocketRef)RSArrayObjectAtIndex(__RSWriteSockets, idx);
            if (!__RSNativeSocketIsValid(s->_socket)) {
#if defined(LOG_RSSOCKET)
                fprintf(stdout, "socket manager found write socket %d invalid\n", s->_socket);
#endif
                RSArrayRemoveObjectAtIndex(invalidSockets, s);
            }
        }
        cnt = RSArrayGetCount(__RSReadSockets);
        for (idx = 0; idx < cnt; idx++) {
            RSSocketRef s = (RSSocketRef)RSArrayObjectAtIndex(__RSReadSockets, idx);
            if (!__RSNativeSocketIsValid(s->_socket)) {
#if defined(LOG_RSSOCKET)
                fprintf(stdout, "socket manager found read socket %d invalid\n", s->_socket);
#endif
                RSArrayRemoveObjectAtIndex(invalidSockets, s);
            }
        }
        
        
        cnt = RSArrayGetCount(invalidSockets);
        
        /* Note that we're doing this only when we got EBADF but otherwise
         * don't have an explicit bad descriptor.  Note that the lock is held now.
         * Finally, note that cnt == 0 doesn't necessarily mean
         * that this loop will do anything, since fd's may have been invalidated
         * while we were in select.
         */
        if (cnt == 0) {
#if defined(LOG_RSSOCKET)
            fprintf(stdout, "socket manager received EBADF(1): No sockets were marked as invalid, cleaning out fdsets\n");
#endif
            
            clearInvalidFileDescriptors(__RSReadSocketsFds);
            clearInvalidFileDescriptors(__RSWriteSocketsFds);
        }
        
        RSSpinLockUnlock(&__RSActiveSocketsLock);
        
        for (idx = 0; idx < cnt; idx++) {
            RSSocketInvalidate(((RSSocketRef)RSArrayObjectAtIndex(invalidSockets, idx)));
        }
        RSRelease(invalidSockets);
    }
}

#ifdef __GNUC__
__attribute__ ((noreturn))	// mostly interesting for shutting up a warning
#endif /* __GNUC__ */
static void __RSSocketManager(void * arg)
{
    pthread_setname_np("com.retval.RSSocket.private");
//    if (objc_collectingEnabled()) objc_registerThreadWithCollector();
    RSBit32 nrfds, maxnrfds, fdentries = 1;
    RSBit32 rfds, wfds;
    fd_set *exceptfds = nil;
    fd_set *writefds = (fd_set *)RSAllocatorAllocate(RSAllocatorSystemDefault, fdentries * sizeof(fd_mask));
    fd_set *readfds = (fd_set *)RSAllocatorAllocate(RSAllocatorSystemDefault, fdentries * sizeof(fd_mask));
    fd_set *tempfds;
    RSIndex idx, cnt;
    uint8_t buffer[256];
    RSMutableArrayRef selectedWriteSockets = RSArrayCreateMutable(RSAllocatorSystemDefault, 0);
    RSMutableArrayRef selectedReadSockets = RSArrayCreateMutable(RSAllocatorSystemDefault, 0);
    RSIndex selectedWriteSocketsIndex = 0, selectedReadSocketsIndex = 0;
    
    struct timeval tv;
    struct timeval* pTimeout = nil;
    struct timeval timeBeforeSelect;
    
    for (;;) {
        RSSpinLockLock(&__RSActiveSocketsLock);
        __RSSocketManagerIteration++;
#if defined(LOG_RSSOCKET)
        fprintf(stdout, "socket manager iteration %lu looking at read sockets ", (unsigned long)__RSSocketManagerIteration);
        __RSSocketWriteSocketList(__RSReadSockets, __RSReadSocketsFds, FALSE);
        if (0 < RSArrayGetCount(__RSWriteSockets)) {
            fprintf(stdout, " and write sockets ");
            __RSSocketWriteSocketList(__RSWriteSockets, __RSWriteSocketsFds, FALSE);
        }
        fprintf(stdout, "\n");
#endif
        rfds = (RSBit32)__RSSocketFdGetSize(__RSReadSocketsFds);
        wfds = (RSBit32)__RSSocketFdGetSize(__RSWriteSocketsFds);
        maxnrfds = __RSMax(rfds, wfds);
        if (maxnrfds > fdentries * (int)NFDBITS) {
            fdentries = (maxnrfds + NFDBITS - 1) / NFDBITS;
            writefds = (fd_set *)RSAllocatorReallocate(RSAllocatorSystemDefault, writefds, fdentries * sizeof(fd_mask));
            readfds = (fd_set *)RSAllocatorReallocate(RSAllocatorSystemDefault, readfds, fdentries * sizeof(fd_mask));
        }
        memset(writefds, 0, fdentries * sizeof(fd_mask));
        memset(readfds, 0, fdentries * sizeof(fd_mask));
        
        RSDataGetBytes(__RSWriteSocketsFds, RSMakeRange(0, RSDataGetLength(__RSWriteSocketsFds)), (UInt8 *)writefds);
        RSDataGetBytes(__RSReadSocketsFds, RSMakeRange(0, RSDataGetLength(__RSReadSocketsFds)), (UInt8 *)readfds);
		
        if (__RSReadSocketsTimeoutInvalid) {
            struct timeval* minTimeout = nil;
            __RSReadSocketsTimeoutInvalid = NO;
#if defined(LOG_RSSOCKET)
            fprintf(stdout, "Figuring out which sockets have timeouts...\n");
#endif
            RSArrayApplyFunction(__RSReadSockets, RSMakeRange(0, RSArrayGetCount(__RSReadSockets)), _calcMinTimeout_locked, (void*) &minTimeout);
            
            if (minTimeout == nil) {
#if defined(LOG_RSSOCKET)
                fprintf(stdout, "No one wants a timeout!\n");
#endif
                pTimeout = nil;
            } else {
#if defined(LOG_RSSOCKET)
                fprintf(stdout, "timeout will be %ld, %d!\n", minTimeout->tv_sec, minTimeout->tv_usec);
#endif
                tv = *minTimeout;
                pTimeout = &tv;
            }
        }
        
        if (pTimeout) {
#if defined(LOG_RSSOCKET)
            fprintf(stdout, "select will have a %ld, %d timeout\n", pTimeout->tv_sec, pTimeout->tv_usec);
#endif
            gettimeofday(&timeBeforeSelect, nil);
        }
		
		RSSpinLockUnlock(&__RSActiveSocketsLock);
        
#if DEPLOYMENT_TARGET_WINDOWS
        // On Windows, select checks connection failed sockets via the exceptfds parameter. connection succeeded is checked via writefds. We need both.
        exceptfds = writefds;
#endif
        nrfds = select(maxnrfds, readfds, writefds, exceptfds, pTimeout);
        
#if defined(LOG_RSSOCKET)
        fprintf(stdout, "socket manager woke from select, ret=%ld\n", (long)nrfds);
#endif
        
		/*
		 * select returned a timeout
		 */
        if (0 == nrfds) {
			struct timeval timeAfterSelect;
			struct timeval deltaTime;
			gettimeofday(&timeAfterSelect, nil);
			/* timeBeforeSelect becomes the delta */
			timersub(&timeAfterSelect, &timeBeforeSelect, &deltaTime);
			
#if defined(LOG_RSSOCKET)
			fprintf(stdout, "Socket manager received timeout - kicking off expired reads (expired delta %ld, %d)\n", deltaTime.tv_sec, deltaTime.tv_usec);
#endif
			
			RSSpinLockLock(&__RSActiveSocketsLock);
			
			tempfds = nil;
			cnt = RSArrayGetCount(__RSReadSockets);
			for (idx = 0; idx < cnt; idx++) {
				RSSocketRef s = (RSSocketRef)RSArrayObjectAtIndex(__RSReadSockets, idx);
				if (timerisset(&s->_readBufferTimeout) || s->_leftoverBytes) {
					RSSocketHandle sock = s->_socket;
					// We might have an new element in __RSReadSockets that we weren't listening to,
					// in which case we must be sure not to test a bit in the fdset that is
					// outside our mask size.
					BOOL sockInBounds = (0 <= sock && sock < maxnrfds);
					/* if this sockets timeout is less than or equal elapsed time, then signal it */
					if (INVALID_SOCKET != sock && sockInBounds) {
#if defined(LOG_RSSOCKET)
						fprintf(stdout, "Expiring socket %d (delta %ld, %d)\n", sock, s->_readBufferTimeout.tv_sec, s->_readBufferTimeout.tv_usec);
#endif
						RSArraySetObjectAtIndex(selectedReadSockets, selectedReadSocketsIndex, s);
						selectedReadSocketsIndex++;
						/* socket is removed from fds here, will be restored in read handling or in perform function */
						if (!tempfds) tempfds = (fd_set *)RSDataGetMutableBytesPtr(__RSReadSocketsFds);
						FD_CLR(sock, tempfds);
					}
				}
			}
			
			RSSpinLockUnlock(&__RSActiveSocketsLock);
			
			/* and below, we dispatch through the normal read dispatch mechanism */
		}
		
        if (0 > nrfds) {
            manageSelectError();
            continue;
        }
        if (FD_ISSET(__RSWakeupSocketPair[1], readfds)) {
            recv(__RSWakeupSocketPair[1], (char *)buffer, sizeof(buffer), 0);
#if defined(LOG_RSSOCKET)
            fprintf(stdout, "socket manager received %c on wakeup socket\n", buffer[0]);
#endif
        }
        RSSpinLockLock(&__RSActiveSocketsLock);
        tempfds = nil;
        cnt = RSArrayGetCount(__RSWriteSockets);
        for (idx = 0; idx < cnt; idx++) {
            RSSocketRef s = (RSSocketRef)RSArrayObjectAtIndex(__RSWriteSockets, idx);
            RSSocketHandle sock = s->_socket;
            // We might have an new element in __RSWriteSockets that we weren't listening to,
            // in which case we must be sure not to test a bit in the fdset that is
            // outside our mask size.
            BOOL sockInBounds = (0 <= sock && sock < maxnrfds);
            if (INVALID_SOCKET != sock && sockInBounds) {
                if (FD_ISSET(sock, writefds)) {
                    RSArraySetObjectAtIndex(selectedWriteSockets, selectedWriteSocketsIndex, s);
                    selectedWriteSocketsIndex++;
                    /* socket is removed from fds here, restored by RSSocketReschedule */
                    if (!tempfds) tempfds = (fd_set *)RSDataGetMutableBytesPtr(__RSWriteSocketsFds);
                    FD_CLR(sock, tempfds);
                    // RSLog(5, RSSTR("Manager: cleared socket %p from write fds"), s);
                }
            }
        }
        tempfds = nil;
        cnt = RSArrayGetCount(__RSReadSockets);
        for (idx = 0; idx < cnt; idx++) {
            RSSocketRef s = (RSSocketRef)RSArrayObjectAtIndex(__RSReadSockets, idx);
            RSSocketHandle sock = s->_socket;
            // We might have an new element in __RSReadSockets that we weren't listening to,
            // in which case we must be sure not to test a bit in the fdset that is
            // outside our mask size.
            BOOL sockInBounds = (0 <= sock && sock < maxnrfds);
            if (INVALID_SOCKET != sock && sockInBounds && FD_ISSET(sock, readfds)) {
                RSArraySetObjectAtIndex(selectedReadSockets, selectedReadSocketsIndex, s);
                selectedReadSocketsIndex++;
                /* socket is removed from fds here, will be restored in read handling or in perform function */
                if (!tempfds) tempfds = (fd_set *)RSDataGetMutableBytesPtr(__RSReadSocketsFds);
                FD_CLR(sock, tempfds);
            }
        }
        RSSpinLockUnlock(&__RSActiveSocketsLock);
        
        for (idx = 0; idx < selectedWriteSocketsIndex; idx++) {
            RSSocketRef s = (RSSocketRef)RSArrayObjectAtIndex(selectedWriteSockets, idx);
            if (RSNull == (RSNilRef)s) continue;
#if defined(LOG_RSSOCKET)
            fprintf(stdout, "socket manager signaling socket %d for write\n", s->_socket);
#endif
            __RSSocketHandleWrite(s, FALSE);
            RSArraySetObjectAtIndex(selectedWriteSockets, idx, RSNull);
        }
        selectedWriteSocketsIndex = 0;
        
        for (idx = 0; idx < selectedReadSocketsIndex; idx++) {
            RSSocketRef s = (RSSocketRef)RSArrayObjectAtIndex(selectedReadSockets, idx);
            if (RSNull == (RSNilRef)s) continue;
#if defined(LOG_RSSOCKET)
            fprintf(stdout, "socket manager signaling socket %d for read\n", s->_socket);
#endif
            __RSSocketHandleRead(s, nrfds == 0);
            RSArraySetObjectAtIndex(selectedReadSockets, idx, RSNull);
        }
        selectedReadSocketsIndex = 0;
    }
}

static RSStringRef __RSSocketClassDescription(RSTypeRef rs) {
    RSSocketRef s = (RSSocketRef)rs;
    RSMutableStringRef result;
    RSStringRef contextDesc = nil;
    void *contextInfo = nil;
    RSStringRef (*contextCopyDescription)(const void *info) = nil;
    result = RSStringCreateMutable(RSGetAllocator(s), 0);
    __RSSocketLock(s);
    void *addr = s->_callout;
#if DEPLOYMENT_TARGET_MACOSX || DEPLOYMENT_TARGET_EMBEDDED || DEPLOYMENT_TARGET_EMBEDDED_MINI
    Dl_info info;
    const char *name = (dladdr(addr, &info) && info.dli_saddr == addr && info.dli_sname) ? info.dli_sname : "???";
#else
    // don't bother trying to figure out callout names
    const char *name = "<unknown>";
#endif
    RSStringAppendStringWithFormat(result, RSSTR("<RSSocket %p [%p]>{valid = %s, type = %d, socket = %d, socket set count = %ld,\n    callback types = 0x%x, callout = %s (%p), source = %p,\n    run loops = %r,\n    context = "), rs, RSGetAllocator(s), (__RSSocketIsValid(s) ? "Yes" : "No"), (int)(s->_socketType), s->_socket, (long)s->_socketSetCount, __RSSocketCallBackTypes(s), name, addr, s->_source0, s->_runLoops);
    contextInfo = s->_context.context;
    contextCopyDescription = s->_context.description;
    __RSSocketUnlock(s);
    if (nil != contextInfo && nil != contextCopyDescription) {
        contextDesc = (RSStringRef)contextCopyDescription(contextInfo);
    }
    if (nil == contextDesc) {
        contextDesc = RSStringCreateWithFormat(RSGetAllocator(s), nil, RSSTR("<RSSocket context %p>"), contextInfo);
    }
    RSStringAppendString(result, contextDesc);
    RSStringAppendString(result, RSSTR("}"));
    RSRelease(contextDesc);
    return result;
}

static void __RSSocketClassDeallocate(RSTypeRef rs) {
    /* Since RSSockets are cached, we can only get here sometime after being invalidated */
    RSSocketRef s = (RSSocketRef)rs;
    if (nil != s->_address) {
        RSRelease(s->_address);
        s->_address = nil;
    }
    if (nil != s->_readBuffer) {
        RSRelease(s->_readBuffer);
        s->_readBuffer = nil;
    }
	if (nil != s->_leftoverBytes) {
		RSRelease(s->_leftoverBytes);
		s->_leftoverBytes = nil;
	}
    timerclear(&s->_readBufferTimeout);
    s->_bytesToBuffer = 0;
    s->_bytesToBufferPos = 0;
    s->_bytesToBufferReadPos = 0;
    s->_atEOF = YES;
	s->_bufferedReadError = 0;
}

static RSTypeID _RSSocketTypeID = _RSRuntimeNotATypeID;

static const RSRuntimeClass __RSSocketClass = {
    0,
    "RSSocket",
    nil,      // init
    nil,      // copy
    __RSSocketClassDeallocate,
    nil,      // equal
    nil,      // hash
    __RSSocketClassDescription,      //
    nil,
    nil
};

RSTypeID RSSocketGetTypeID(void) {
    if (_RSRuntimeNotATypeID == _RSSocketTypeID) {
        _RSSocketTypeID = __RSRuntimeRegisterClass(&__RSSocketClass);
        __RSRuntimeSetClassTypeID(&__RSSocketClass, _RSSocketTypeID);
#if DEPLOYMENT_TARGET_MACOSX || DEPLOYMENT_TARGET_EMBEDDED || DEPLOYMENT_TARGET_EMBEDDED_MINI
        struct rlimit lim1;
        int ret1 = getrlimit(RLIMIT_NOFILE, &lim1);
        int mib[] = {CTL_KERN, KERN_MAXFILESPERPROC};
        rlim_t maxfd = 0;
        size_t len = sizeof(int);
        int ret0 = sysctl(mib, 2, &maxfd, &len, nil, 0);
        if (0 == ret0 && 0 == ret1 && lim1.rlim_max < maxfd) maxfd = lim1.rlim_max;
        if (0 == ret1 && lim1.rlim_cur < maxfd) {
            struct rlimit lim2 = lim1;
            lim2.rlim_cur += 2304;
            if (maxfd < lim2.rlim_cur) lim2.rlim_cur = maxfd;
            setrlimit(RLIMIT_NOFILE, &lim2);
            // we try, but do not go to extraordinary measures
        }
#endif
    }
    return _RSSocketTypeID;
}

static RSSocketRef _RSSocketCreateWithNative(RSAllocatorRef allocator, RSSocketHandle sock, RSOptionFlags callBackTypes, RSSocketCallBack callout, const RSSocketContext *context, BOOL useExistingInstance) {
    CHECK_FOR_FORK();
    RSSocketRef memory;
    int typeSize = sizeof(memory->_socketType);
    RSSpinLockLock(&__RSActiveSocketsLock);
    if (nil == __RSReadSockets) __RSSocketInitializeSockets();
    RSSpinLockUnlock(&__RSActiveSocketsLock);
    RSSpinLockLock(&__RSAllSocketsLock);
    if (nil == __RSAllSockets) {
        __RSAllSockets = RSDictionaryCreateMutable(RSAllocatorSystemDefault, 0, RSDictionaryNilKeyContext);
    }
    if (INVALID_SOCKET != sock && (memory = (RSSocketRef)RSDictionaryGetValue(__RSAllSockets, (void *)(uintptr_t)sock))) {
        if (useExistingInstance) {
			RSSpinLockUnlock(&__RSAllSocketsLock);
			RSRetain(memory);
			return memory;
		} else {
#if defined(LOG_RSSOCKET)
			fprintf(stdout, "useExistingInstance is FALSE, removing existing instance %p from __RSAllSockets\n", memory);
#endif
			RSSpinLockUnlock(&__RSAllSocketsLock);
			RSSocketInvalidate(memory);
			RSSpinLockLock(&__RSAllSocketsLock);
		}
    }
    memory = (RSSocketRef)__RSRuntimeCreateInstance(allocator, RSSocketGetTypeID(), sizeof(struct __RSSocket) - sizeof(RSRuntimeBase));
    if (nil == memory) {
        RSSpinLockUnlock(&__RSAllSocketsLock);
        return nil;
    }
    __RSSocketSetCallBackTypes(memory, callBackTypes);
    if (INVALID_SOCKET != sock) __RSSocketSetValid(memory);
    __RSSocketUnsetWriteSignalled(memory);
    __RSSocketUnsetReadSignalled(memory);
    memory->_f.client = (unsigned)((callBackTypes & (~RSSocketConnectCallBack)) & (~RSSocketWriteCallBack)) | RSSocketCloseOnInvalidate;
    memory->_f.disabled = 0;
    memory->_f.connected = FALSE;
    memory->_f.writableHint = FALSE;
    memory->_f.closeSignaled = FALSE;
    memory->_lock = RSSpinLockInit;
    memory->_writeLock = RSSpinLockInit;
    memory->_socket = sock;
    if (INVALID_SOCKET == sock || 0 != getsockopt(sock, SOL_SOCKET, SO_TYPE, (char *)&(memory->_socketType), (socklen_t *)&typeSize)) memory->_socketType = 0;		// cast for WinSock bad API
    memory->_errorCode = 0;
    memory->_address = nil;
    memory->_peerAddress = nil;
    memory->_socketSetCount = 0;
    memory->_source0 = nil;
    if (INVALID_SOCKET != sock) {
        memory->_runLoops = __RSArrayCreateMutable0(RSAllocatorSystemDefault, 0, nil);
    } else {
        memory->_runLoops = nil;
    }
    memory->_callout = callout;
    memory->_dataQueue = nil;
    memory->_addressQueue = nil;
    memory->_context.context = 0;
    memory->_context.retain = 0;
    memory->_context.release = 0;
    memory->_context.description = 0;
    timerclear(&memory->_readBufferTimeout);
    memory->_readBuffer = nil;
    memory->_bytesToBuffer = 0;
    memory->_bytesToBufferPos = 0;
    memory->_bytesToBufferReadPos = 0;
    memory->_atEOF = NO;
    memory->_bufferedReadError = 0;
    memory->_leftoverBytes = nil;
    
    if (INVALID_SOCKET != sock) RSDictionarySetValue(__RSAllSockets, (void *)(uintptr_t)sock, memory);
    if (nil == __RSSocketManagerThread) __RSSocketManagerThread = __RSStartSimpleThread(__RSSocketManager, 0);
    RSSpinLockUnlock(&__RSAllSocketsLock);
    if (nil != context) {
        void *contextInfo = context->retain ? (void *)context->retain(context->context) : context->context;
        __RSSocketLock(memory);
        memory->_context.retain = context->retain;
        memory->_context.release = context->release;
        memory->_context.description = context->description;
        memory->_context.context = contextInfo;
        __RSSocketUnlock(memory);
    }
#if defined(LOG_RSSOCKET)
    RSLog(5, RSSTR("RSSocketCreateWithNative(): created socket %p (%d) with callbacks 0x%x"), memory, memory->_socket, callBackTypes);
#endif
    return memory;
}

RSSocketRef RSSocketCreateWithNative(RSAllocatorRef allocator, RSSocketHandle sock, RSOptionFlags callBackTypes, RSSocketCallBack callout, const RSSocketContext *context) {
	return _RSSocketCreateWithNative(allocator, sock, callBackTypes, callout, context, TRUE);
}

void RSSocketInvalidate(RSSocketRef s) {
    // RSLog(5, RSSTR("RSSocketInvalidate(%p) starting"), s);
    CHECK_FOR_FORK();
    UInt32 previousSocketManagerIteration;
    __RSGenericValidInstance(s, RSSocketGetTypeID());
#if defined(LOG_RSSOCKET)
    fprintf(stdout, "invalidating socket %d with flags 0x%x disabled 0x%x connected 0x%x\n", s->_socket, s->_f.client, s->_f.disabled, s->_f.connected);
#endif
    RSRetain(s);
    RSSpinLockLock(&__RSAllSocketsLock);
    __RSSocketLock(s);
    if (__RSSocketIsValid(s)) {
        RSIndex idx;
        RSRunLoopSourceRef source0;
        void *contextInfo = nil;
        void (*contextRelease)(const void *info) = nil;
        __RSSocketUnsetValid(s);
        __RSSocketUnsetWriteSignalled(s);
        __RSSocketUnsetReadSignalled(s);
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
        if ((s->_f.client & RSSocketCloseOnInvalidate) != 0) closesocket(s->_socket);
        s->_socket = INVALID_SOCKET;
        if (nil != s->_peerAddress) {
            RSRelease(s->_peerAddress);
            s->_peerAddress = nil;
        }
        if (nil != s->_dataQueue) {
            RSRelease(s->_dataQueue);
            s->_dataQueue = nil;
        }
        if (nil != s->_addressQueue) {
            RSRelease(s->_addressQueue);
            s->_addressQueue = nil;
        }
        s->_socketSetCount = 0;
        
        // we'll need this later
        RSArrayRef runLoops = (RSArrayRef)RSRetain(s->_runLoops);
        RSRelease(s->_runLoops);
        
        s->_runLoops = nil;
        source0 = s->_source0;
        s->_source0 = nil;
        contextInfo = s->_context.context;
        contextRelease = s->_context.release;
        s->_context.context = 0;
        s->_context.retain = 0;
        s->_context.release = 0;
        s->_context.description = 0;
        __RSSocketUnlock(s);
        
        // Do this after the socket unlock to avoid deadlock (10462525)
        for (idx = RSArrayGetCount(runLoops); idx--;) {
            RSRunLoopWakeUp((RSRunLoopRef)RSArrayObjectAtIndex(runLoops, idx));
        }
        RSRelease(runLoops);
        
        if (nil != contextRelease) {
            contextRelease(contextInfo);
        }
        if (nil != source0) {
            RSRunLoopSourceInvalidate(source0);
            RSRelease(source0);
        }
    } else {
        __RSSocketUnlock(s);
    }
    RSSpinLockUnlock(&__RSAllSocketsLock);
    RSRelease(s);
#if defined(LOG_RSSOCKET)
    RSLog(5, RSSTR("RSSocketInvalidate(%p) done"), s);
#endif
}

BOOL RSSocketIsValid(RSSocketRef s) {
    CHECK_FOR_FORK();
    __RSGenericValidInstance(s, RSSocketGetTypeID());
    return __RSSocketIsValid(s);
}

RSSocketHandle RSSocketGetHandle(RSSocketRef s) {
    CHECK_FOR_FORK();
    __RSGenericValidInstance(s, RSSocketGetTypeID());
    return s->_socket;
}

RSDataRef RSSocketCopyAddress(RSSocketRef s) {
    CHECK_FOR_FORK();
    RSDataRef result = nil;
    __RSGenericValidInstance(s, RSSocketGetTypeID());
    __RSSocketLock(s);
    __RSSocketEstablishAddress(s);
    if (nil != s->_address) {
        result = (RSDataRef)RSRetain(s->_address);
    }
    __RSSocketUnlock(s);
#if defined(LOG_RSSOCKET)
    RSLog(5, RSSTR("RSSocketCopyAddress(): created socket %p address %r"), s, result);
#endif
    return result;
}

RSDataRef RSSocketCopyPeerAddress(RSSocketRef s) {
    CHECK_FOR_FORK();
    RSDataRef result = nil;
    __RSGenericValidInstance(s, RSSocketGetTypeID());
    __RSSocketLock(s);
    __RSSocketEstablishPeerAddress(s);
    if (nil != s->_peerAddress) {
        result = (RSDataRef)RSRetain(s->_peerAddress);
    }
    __RSSocketUnlock(s);
#if defined(LOG_RSSOCKET)
    RSLog(5, RSSTR("RSSocketCopyAddress(): created socket %p peer address %r"), s, result);
#endif
    return result;
}

void RSSocketGetContext(RSSocketRef s, RSSocketContext *context) {
    CHECK_FOR_FORK();
    __RSGenericValidInstance(s, RSSocketGetTypeID());
    RSAssert1(0 == context->version, __RSLogAssertion, "%s(): context version not initialized to 0", __PRETTY_FUNCTION__);
    *context = s->_context;
}

RSOptionFlags RSSocketGetSocketFlags(RSSocketRef s) {
    CHECK_FOR_FORK();
    __RSGenericValidInstance(s, RSSocketGetTypeID());
    return s->_f.client;
}

void RSSocketSetSocketFlags(RSSocketRef s, RSOptionFlags flags) {
    CHECK_FOR_FORK();
    __RSGenericValidInstance(s, RSSocketGetTypeID());
    __RSSocketLock(s);
#if defined(LOG_RSSOCKET)
    fprintf(stdout, "setting flags for socket %d, from 0x%x to 0x%lx\n", s->_socket, s->_f.client, flags);
#endif
    s->_f.client = (unsigned)flags;
    __RSSocketUnlock(s);
    // RSLog(5, RSSTR("RSSocketSetSocketFlags(%p, 0x%x)"), s, flags);
}

void RSSocketDisableCallBacks(RSSocketRef s, RSOptionFlags callBackTypes) {
    CHECK_FOR_FORK();
    BOOL wakeup = NO;
    uint8_t readCallBackType;
    __RSGenericValidInstance(s, RSSocketGetTypeID());
    __RSSocketLock(s);
    if (__RSSocketIsValid(s) && __RSSocketIsScheduled(s)) {
        callBackTypes &= __RSSocketCallBackTypes(s);
        readCallBackType = __RSSocketReadCallBackType(s);
        s->_f.disabled |= callBackTypes;
#if defined(LOG_RSSOCKET)
        fprintf(stdout, "unscheduling socket %d with flags 0x%x disabled 0x%x connected 0x%x for types 0x%lx\n", s->_socket, s->_f.client, s->_f.disabled, s->_f.connected, callBackTypes);
#endif
        RSSpinLockLock(&__RSActiveSocketsLock);
        if ((readCallBackType == RSSocketAcceptCallBack) || !__RSSocketIsConnectionOriented(s)) s->_f.connected = TRUE;
        if (((callBackTypes & RSSocketWriteCallBack) != 0) || (((callBackTypes & RSSocketConnectCallBack) != 0) && !s->_f.connected)) {
            if (__RSSocketClearFDForWrite(s)) {
                // do not wake up the socket manager thread if all relevant write callbacks are disabled
                RSOptionFlags writeCallBacksAvailable = __RSSocketCallBackTypes(s) & (RSSocketWriteCallBack | RSSocketConnectCallBack);
                if (s->_f.connected) writeCallBacksAvailable &= ~RSSocketConnectCallBack;
                if ((s->_f.disabled & writeCallBacksAvailable) != writeCallBacksAvailable) wakeup = YES;
            }
        }
        if (readCallBackType != RSSocketNoCallBack && (callBackTypes & readCallBackType) != 0) {
            if (__RSSocketClearFDForRead(s)) {
                // do not wake up the socket manager thread if callback type is read
                if (readCallBackType != RSSocketReadCallBack) wakeup = YES;
            }
        }
        RSSpinLockUnlock(&__RSActiveSocketsLock);
    }
    __RSSocketUnlock(s);
}

// "force" means to clear the disabled bits set by DisableCallBacks and always reenable.
// if (!force) we respect those bits, meaning they may stop us from enabling.
// In addition, if !force we assume that the sockets have already been added to the
// __RSReadSockets and __RSWriteSockets arrays.  This is YES because the callbacks start
// enabled when the RSSocket is created (at which time we enable with force).
// Called with SocketLock held, returns with it released!
void __RSSocketEnableCallBacks(RSSocketRef s, RSOptionFlags callBackTypes, BOOL force, uint8_t wakeupChar) {
    CHECK_FOR_FORK();
    BOOL wakeup = FALSE;
    if (!callBackTypes) {
        __RSSocketUnlock(s);
        return;
    }
    if (__RSSocketIsValid(s) && __RSSocketIsScheduled(s)) {
        BOOL turnOnWrite = FALSE, turnOnConnect = FALSE, turnOnRead = FALSE;
        uint8_t readCallBackType = __RSSocketReadCallBackType(s);
        callBackTypes &= __RSSocketCallBackTypes(s);
        if (force) s->_f.disabled &= ~callBackTypes;
#if defined(LOG_RSSOCKET)
        fprintf(stdout, "rescheduling socket %d with flags 0x%x disabled 0x%x connected 0x%x for types 0x%lx\n", s->_socket, s->_f.client, s->_f.disabled, s->_f.connected, callBackTypes);
#endif
        /* We will wait for connection only for connection-oriented, non-rendezvous sockets that are not already connected.  Mark others as already connected. */
        if ((readCallBackType == RSSocketAcceptCallBack) || !__RSSocketIsConnectionOriented(s)) s->_f.connected = TRUE;
        
        // First figure out what to turn on
        if (s->_f.connected || (callBackTypes & RSSocketConnectCallBack) == 0) {
            // if we want write callbacks and they're not disabled...
            if ((callBackTypes & RSSocketWriteCallBack) != 0 && (s->_f.disabled & RSSocketWriteCallBack) == 0) turnOnWrite = TRUE;
        } else {
            // if we want connect callbacks and they're not disabled...
            if ((callBackTypes & RSSocketConnectCallBack) != 0 && (s->_f.disabled & RSSocketConnectCallBack) == 0) turnOnConnect = TRUE;
        }
        // if we want read callbacks and they're not disabled...
        if (readCallBackType != RSSocketNoCallBack && (callBackTypes & readCallBackType) != 0 && (s->_f.disabled & RSSocketReadCallBack) == 0) turnOnRead = TRUE;
        
        // Now turn on the callbacks we've determined that we want on
        if (turnOnRead || turnOnWrite || turnOnConnect) {
            RSSpinLockLock(&__RSActiveSocketsLock);
            if (turnOnWrite || turnOnConnect) {
                if (force) {
                    RSIndex idx = RSArrayIndexOfObject(__RSWriteSockets, s);
                    if (RSNotFound == idx) RSArrayRemoveObjectAtIndex(__RSWriteSockets, s);
                    //                     if (RSNotFound == idx) RSLog(5, RSSTR("__RSSocketEnableCallBacks: put %p in __RSWriteSockets list due to force and non-presence"), s);
                }
                if (__RSSocketSetFDForWrite(s)) wakeup = YES;
            }
            if (turnOnRead) {
                if (force) {
                    RSIndex idx = RSArrayIndexOfObject(__RSReadSockets, s);
                    if (RSNotFound == idx) RSArrayRemoveObjectAtIndex(__RSReadSockets, s);
                }
                if (__RSSocketSetFDForRead(s)) wakeup = YES;
            }
            RSSpinLockUnlock(&__RSActiveSocketsLock);
        }
    }
    __RSSocketUnlock(s);
}

void RSSocketEnableCallBacks(RSSocketRef s, RSOptionFlags callBackTypes) {
    CHECK_FOR_FORK();
    __RSGenericValidInstance(s, RSSocketGetTypeID());
    __RSSocketLock(s);
    __RSSocketEnableCallBacks(s, callBackTypes, TRUE, 'r');
    // RSLog(5, RSSTR("RSSocketEnableCallBacks(%p, 0x%x) done"), s, callBackTypes);
}

static void __RSSocketSchedule(void *info, RSRunLoopRef rl, RSStringRef mode) {
    RSSocketRef s = (RSSocketRef)info;
    __RSSocketLock(s);
    //??? also need to arrange delivery of all pending data
    if (__RSSocketIsValid(s)) {
        RSMutableArrayRef runLoopsOrig = s->_runLoops;
        RSMutableArrayRef runLoopsCopy = RSMutableCopy(RSAllocatorSystemDefault, s->_runLoops);
        RSArrayRemoveObjectAtIndex(runLoopsCopy, rl);
        s->_runLoops = runLoopsCopy;
        RSRelease(runLoopsOrig);
        s->_socketSetCount++;
        // Since the v0 source is listened to on the SocketMgr thread, no matter how many modes it
        // is added to we just need to enable it there once (and _socketSetCount gives us a refCount
        // to know when we can finally disable it).
        if (1 == s->_socketSetCount) {
#if defined(LOG_RSSOCKET)
            fprintf(stdout, "scheduling socket %d\n", s->_socket);
#endif
            // RSLog(5, RSSTR("__RSSocketSchedule(%p, %p, %p)"), s, rl, mode);
            __RSSocketEnableCallBacks(s, __RSSocketCallBackTypes(s), TRUE, 's');  // unlocks s
        } else
            __RSSocketUnlock(s);
    } else
        __RSSocketUnlock(s);
}

static void __RSSocketCancel(void *info, RSRunLoopRef rl, RSStringRef mode) {
    RSSocketRef s = (RSSocketRef)info;
    RSIndex idx;
    __RSSocketLock(s);
    s->_socketSetCount--;
    if (0 == s->_socketSetCount) {
        RSSpinLockLock(&__RSActiveSocketsLock);
        idx = RSArrayIndexOfObject(__RSWriteSockets, s);
        if (0 <= idx) {
            // RSLog(5, RSSTR("__RSSocketCancel: removing %p from __RSWriteSockets list"), s);
            RSArrayRemoveObjectAtIndex(__RSWriteSockets, idx);
            __RSSocketClearFDForWrite(s);
        }
        idx = RSArrayIndexOfObject(__RSReadSockets, s);
        if (0 <= idx) {
            RSArrayRemoveObjectAtIndex(__RSReadSockets, idx);
            __RSSocketClearFDForRead(s);
        }
        RSSpinLockUnlock(&__RSActiveSocketsLock);
    }
    if (nil != s->_runLoops) {
        RSMutableArrayRef runLoopsOrig = s->_runLoops;
        RSMutableArrayRef runLoopsCopy = RSMutableCopy(RSAllocatorSystemDefault, s->_runLoops);
        idx = RSArrayIndexOfObject(runLoopsCopy, rl);
        if (0 <= idx) RSArrayRemoveObjectAtIndex(runLoopsCopy, idx);
        s->_runLoops = runLoopsCopy;
        RSRelease(runLoopsOrig);
    }
    __RSSocketUnlock(s);
}

// Note:  must be called with socket lock held, then returns with it released
// Used by both the v0 and v1 RunLoopSource perform routines
static void __RSSocketDoCallback(RSSocketRef s, RSDataRef data, RSDataRef address, RSSocketHandle sock) {
    RSSocketCallBack callout = nil;
    void *contextInfo = nil;
    SInt32 errorCode = 0;
    BOOL readSignalled = NO, writeSignalled = NO, connectSignalled = NO, calledOut = NO;
    uint8_t readCallBackType, callBackTypes;
    
    callBackTypes = __RSSocketCallBackTypes(s);
    readCallBackType = __RSSocketReadCallBackType(s);
    readSignalled = __RSSocketIsReadSignalled(s);
    writeSignalled = __RSSocketIsWriteSignalled(s);
    connectSignalled = writeSignalled && !s->_f.connected;
    __RSSocketUnsetReadSignalled(s);
    __RSSocketUnsetWriteSignalled(s);
    callout = s->_callout;
    contextInfo = s->_context.context;
#if defined(LOG_RSSOCKET)
    fprintf(stdout, "entering perform for socket %d with read signalled %d write signalled %d connect signalled %d callback types %d\n", s->_socket, readSignalled, writeSignalled, connectSignalled, callBackTypes);
#endif
    if (writeSignalled) {
        errorCode = s->_errorCode;
        s->_f.connected = TRUE;
    }
    __RSSocketUnlock(s);
    if ((callBackTypes & RSSocketConnectCallBack) != 0) {
        if (connectSignalled && (!calledOut || RSSocketIsValid(s))) {
            // RSLog(5, RSSTR("__RSSocketPerformV0(%p) doing connect callback, error: %d"), s, errorCode);
            if (errorCode) {
#if defined(LOG_RSSOCKET)
                fprintf(stdout, "perform calling out error %ld to socket %d\n", (long)errorCode, s->_socket);
#endif
                if (callout) callout(s, RSSocketConnectCallBack, nil, &errorCode, contextInfo);
                calledOut = YES;
            } else {
#if defined(LOG_RSSOCKET)
                fprintf(stdout, "perform calling out connect to socket %d\n", s->_socket);
#endif
                if (callout) callout(s, RSSocketConnectCallBack, nil, nil, contextInfo);
                calledOut = YES;
            }
        }
    }
    if (RSSocketDataCallBack == readCallBackType) {
        if (nil != data && (!calledOut || RSSocketIsValid(s))) {
            RSIndex datalen = RSDataGetLength(data);
#if defined(LOG_RSSOCKET)
            fprintf(stdout, "perform calling out data of length %ld to socket %d\n", (long)datalen, s->_socket);
#endif
            if (callout) callout(s, RSSocketDataCallBack, address, data, contextInfo);
            calledOut = YES;
            if (0 == datalen) RSSocketInvalidate(s);
        }
    } else if (RSSocketAcceptCallBack == readCallBackType) {
        if (INVALID_SOCKET != sock && (!calledOut || RSSocketIsValid(s))) {
#if defined(LOG_RSSOCKET)
            fprintf(stdout, "perform calling out accept of socket %d to socket %d\n", sock, s->_socket);
#endif
            if (callout) callout(s, RSSocketAcceptCallBack, address, &sock, contextInfo);
            calledOut = YES;
        }
    } else if (RSSocketReadCallBack == readCallBackType) {
        if (readSignalled && (!calledOut || RSSocketIsValid(s))) {
#if defined(LOG_RSSOCKET)
            fprintf(stdout, "perform calling out read to socket %d\n", s->_socket);
#endif
            // RSLog(5, RSSTR("__RSSocketPerformV0(%p) doing read callback"), s);
            if (callout) callout(s, RSSocketReadCallBack, nil, nil, contextInfo);
            calledOut = YES;
        }
    }
    if ((callBackTypes & RSSocketWriteCallBack) != 0) {
        if (writeSignalled && !errorCode && (!calledOut || RSSocketIsValid(s))) {
#if defined(LOG_RSSOCKET)
            fprintf(stdout, "perform calling out write to socket %d\n", s->_socket);
#endif
            // RSLog(5, RSSTR("__RSSocketPerformV0(%p) doing write callback"), s);
            if (callout) callout(s, RSSocketWriteCallBack, nil, nil, contextInfo);
            calledOut = YES;
        }
    }
}

static void __RSSocketPerformV0(void *info) {
    RSSocketRef s = (RSSocketRef)info;
    RSDataRef data = nil;
    RSDataRef address = nil;
    RSSocketHandle sock = INVALID_SOCKET;
    uint8_t readCallBackType, callBackTypes;
    RSRunLoopRef rl = nil;
    // RSLog(5, RSSTR("__RSSocketPerformV0(%p) starting"), s);
    
    __RSSocketLock(s);
    if (!__RSSocketIsValid(s)) {
        __RSSocketUnlock(s);
        return;
    }
    callBackTypes = __RSSocketCallBackTypes(s);
    readCallBackType = __RSSocketReadCallBackType(s);
    RSOptionFlags callBacksSignalled = 0;
    if (__RSSocketIsReadSignalled(s)) callBacksSignalled |= readCallBackType;
    if (__RSSocketIsWriteSignalled(s)) callBacksSignalled |= RSSocketWriteCallBack;
    
    if (RSSocketDataCallBack == readCallBackType) {
        if (nil != s->_dataQueue && 0 < RSArrayGetCount(s->_dataQueue)) {
            data = (RSDataRef)RSArrayObjectAtIndex(s->_dataQueue, 0);
            RSRetain(data);
            RSArrayRemoveObjectAtIndex(s->_dataQueue, 0);
            address = (RSDataRef)RSArrayObjectAtIndex(s->_addressQueue, 0);
            RSRetain(address);
            RSArrayRemoveObjectAtIndex(s->_addressQueue, 0);
        }
    } else if (RSSocketAcceptCallBack == readCallBackType) {
        if (nil != s->_dataQueue && 0 < RSArrayGetCount(s->_dataQueue)) {
            sock = (RSSocketHandle)(uintptr_t)RSArrayObjectAtIndex(s->_dataQueue, 0);
            RSArrayRemoveObjectAtIndex(s->_dataQueue, 0);
            address = (RSDataRef)RSArrayObjectAtIndex(s->_addressQueue, 0);
            RSRetain(address);
            RSArrayRemoveObjectAtIndex(s->_addressQueue, 0);
        }
    }
    
    __RSSocketDoCallback(s, data, address, sock);	// does __RSSocketUnlock(s)
    if (nil != data) RSRelease(data);
    if (nil != address) RSRelease(address);
    
    __RSSocketLock(s);
    if (__RSSocketIsValid(s) && RSSocketNoCallBack != readCallBackType) {
        // if there's still more data, we want to wake back up right away
        if ((RSSocketDataCallBack == readCallBackType || RSSocketAcceptCallBack == readCallBackType) && nil != s->_dataQueue && 0 < RSArrayGetCount(s->_dataQueue)) {
#if defined(LOG_RSSOCKET)
            fprintf(stdout, "perform short-circuit signaling source for socket %d with flags 0x%x disabled 0x%x connected 0x%x\n", s->_socket, s->_f.client, s->_f.disabled, s->_f.connected);
#endif
            RSRunLoopSourceSignal(s->_source0);
            RSMutableArrayRef runLoopsOrig = (RSMutableArrayRef)RSRetain(s->_runLoops);
            RSMutableArrayRef runLoopsCopy = RSMutableCopy(RSAllocatorSystemDefault, s->_runLoops);
            RSRunLoopSourceRef source0 = s->_source0;
            if (nil != source0 && !RSRunLoopSourceIsValid(source0)) {
                source0 = nil;
            }
            if (source0) RSRetain(source0);
            __RSSocketUnlock(s);
            rl = __RSSocketCopyRunLoopToWakeUp(source0, runLoopsCopy);
            if (source0) RSRelease(source0);
            __RSSocketLock(s);
            if (runLoopsOrig == s->_runLoops) {
                s->_runLoops = runLoopsCopy;
                runLoopsCopy = nil;
                RSRelease(runLoopsOrig);
            }
            RSRelease(runLoopsOrig);
            if (runLoopsCopy) RSRelease(runLoopsCopy);
        }
    }
    // Only reenable callbacks that are auto-reenabled
    __RSSocketEnableCallBacks(s, callBacksSignalled & s->_f.client, FALSE, 'p');  // unlocks s
    
    if (nil != rl) {
        RSRunLoopWakeUp(rl);
        RSRelease(rl);
    }
    // RSLog(5, RSSTR("__RSSocketPerformV0(%p) done"), s);
}

RSRunLoopSourceRef RSSocketCreateRunLoopSource(RSAllocatorRef allocator, RSSocketRef s, RSIndex order) {
    CHECK_FOR_FORK();
    RSRunLoopSourceRef result = nil;
    __RSGenericValidInstance(s, RSSocketGetTypeID());
    __RSSocketLock(s);
    if (__RSSocketIsValid(s)) {
        if (nil != s->_source0 && !RSRunLoopSourceIsValid(s->_source0)) {
            RSRelease(s->_source0);
            s->_source0 = nil;
        }
        if (nil == s->_source0) {
            RSRunLoopSourceContext context;
            context.version = 0;
            context.info = s;
            context.retain = RSRetain;
            context.release = RSRelease;
            context.description = RSDescription;
            context.equal = RSEqual;
            context.hash = RSHash;
            context.schedule = __RSSocketSchedule;
            context.cancel = __RSSocketCancel;
            context.perform = __RSSocketPerformV0;
            s->_source0 = RSRunLoopSourceCreate(allocator, order, &context);
        }
        RSRetain(s->_source0);        /* This retain is for the receiver */
        result = s->_source0;
    }
    __RSSocketUnlock(s);
    return result;
}

#endif /* NEW_SOCKET */



static uint16_t __RSSocketDefaultNameRegistryPortNumber = 2454;
RS_CONST_STRING_DECL(RSSocketCommandKey, "Command")
RS_CONST_STRING_DECL(RSSocketNameKey, "Name")
RS_CONST_STRING_DECL(RSSocketValueKey, "Value")
RS_CONST_STRING_DECL(RSSocketResultKey, "Result")
RS_CONST_STRING_DECL(RSSocketUnSuccessKey, "Error")
RS_CONST_STRING_DECL(RSSocketRegisterCommand, "Register")
RS_CONST_STRING_DECL(RSSocketRetrieveCommand, "Retrieve")
RS_CONST_STRING_DECL(__RSSocketRegistryRequestRunLoopMode, "RSSocketRegistryRequest")

static RSSpinLock __RSSocketWriteLock_ = RSSpinLockInit;
//#warning can only send on one socket at a time now

RSInline void __RSSocketWriteLock(RSSocketRef s) {
    RSSpinLockLock(& __RSSocketWriteLock_);
}

RSInline void __RSSocketWriteUnlock(RSSocketRef s) {
    RSSpinLockUnlock(& __RSSocketWriteLock_);
}

#if NEW_SOCKET

RSInline RSIndex __RSSocketFdGetSize(RSDataRef fdSet) {
    return NBBY * RSDataGetLength(fdSet);
}

RSInline BOOL __RSSocketFdSet(RSSocketHandle sock, RSMutableDataRef fdSet) {
    /* returns YES if a change occurred, NO otherwise */
    BOOL retval = NO;
    if (INVALID_SOCKET != sock && 0 <= sock) {
        RSIndex numFds = NBBY * RSDataGetLength(fdSet);
        fd_mask *fds_bits;
        if (sock >= numFds) {
            RSIndex oldSize = numFds / NFDBITS, newSize = (sock + NFDBITS) / NFDBITS, changeInBytes = (newSize - oldSize) * sizeof(fd_mask);
            RSDataIncreaseLength(fdSet, changeInBytes);
            fds_bits = (fd_mask *)RSDataGetMutableBytePtr(fdSet);
            memset(fds_bits + oldSize, 0, changeInBytes);
        } else {
            fds_bits = (fd_mask *)RSDataGetMutableBytePtr(fdSet);
        }
        if (!FD_ISSET(sock, (fd_set *)fds_bits)) {
            retval = YES;
            FD_SET(sock, (fd_set *)fds_bits);
        }
    }
    return retval;
}

#endif

//??? need timeout, error handling, retries
RSSocketError RSSocketSendData(RSSocketRef s, RSDataRef address, RSDataRef data, RSTimeInterval timeout) {
    CHECK_FOR_FORK();
    const uint8_t *dataptr, *addrptr = nil;
    RSIndex datalen, addrlen = 0, size = 0;
    RSSocketHandle sock = INVALID_SOCKET;
    struct timeval tv;
    __RSGenericValidInstance(s, RSSocketGetTypeID());
    if (address) {
        addrptr = RSDataGetBytesPtr(address);
        addrlen = RSDataGetLength(address);
    }
    dataptr = RSDataGetBytesPtr(data);
    datalen = RSDataGetLength(data);
    if (RSSocketIsValid(s)) sock = RSSocketGetHandle(s);
    if (INVALID_SOCKET != sock) {
        RSRetain(s);
        __RSSocketWriteLock(s);
        tv.tv_sec = (timeout <= 0.0 || (RSTimeInterval)INT_MAX <= timeout) ? INT_MAX : (int)floor(timeout);
        tv.tv_usec = (int)floor(1.0e+6 * (timeout - floor(timeout)));
        setsockopt(sock, SOL_SOCKET, SO_SNDTIMEO, (char *)&tv, sizeof(tv));	// cast for WinSock bad API
        if (nil != addrptr && 0 < addrlen) {
            size = sendto(sock, (char *)dataptr, datalen, 0, (struct sockaddr *)addrptr, (socklen_t)addrlen);
        } else {
            size = send(sock, (char *)dataptr, datalen, 0);
        }
#if defined(LOG_RSSOCKET)
        fprintf(stdout, "wrote %ld bytes to socket %d\n", (long)size, sock);
#endif
        __RSSocketWriteUnlock(s);
        RSRelease(s);
    }
    return (size > 0) ? RSSocketSuccess : RSSocketUnSuccess;
}

RSSocketError RSSocketSetAddress(RSSocketRef s, RSDataRef address) {
    CHECK_FOR_FORK();
    struct sockaddr *name;
    socklen_t namelen;
    __RSGenericValidInstance(s, RSSocketGetTypeID());
    if (nil == address) return RSSocketUnSuccess;
    if (!RSSocketIsValid(s)) return RSSocketUnSuccess;
    
    name = (struct sockaddr *)RSDataGetBytesPtr(address);
    namelen = (socklen_t)RSDataGetLength(address);
    if (!name || namelen <= 0) return RSSocketUnSuccess;
    
    RSSocketHandle sock = RSSocketGetHandle(s);
#if DEPLOYMENT_TARGET_MACOSX || DEPLOYMENT_TARGET_EMBEDDED || DEPLOYMENT_TARGET_EMBEDDED_MINI
    // Verify that the namelen is correct. If not, we have to fix it up. Developers will often incorrectly use 0 or strlen(path). See 9217961 and the second half of 9098274.
    // Max size is a size byte, plus family byte, plus path of 255, plus a null byte.
    char newName[255];
    if (namelen > 2 && name->sa_family == AF_UNIX) {
        // Don't use the SUN_LEN macro, because strnlen is safer and we know the max length of the string (from RSData, minus 2 bytes for len and addr)
        socklen_t realLength = (socklen_t)(sizeof(*((struct sockaddr_un *)name)) - sizeof(((struct sockaddr_un *)name)->sun_path) + strnlen(((struct sockaddr_un *)name)->sun_path, namelen - 2));
        if (realLength > 255) return RSSocketUnSuccess;
        
        // For a UNIX domain socket, we must pass the value of name.sun_len to bind in order for getsockname() to return a result that makes sense.
        namelen = (socklen_t)(((struct sockaddr_un *)name)->sun_len);
        
        if (realLength != namelen) {
            // We got a different answer for length than was supplied by the caller. Fix it up so we don't end up truncating the path.
            RSLog(RSLogLevelWarning, RSSTR("WARNING: The sun_len field of a sockaddr_un structure passed to RSSocketSetAddress was not set correctly using the SUN_LEN macro."));
            memcpy(newName, name, realLength);
            namelen = realLength;
            ((struct sockaddr_un *)newName)->sun_len = realLength;
            name = (struct sockaddr *)newName;
        }
    }
#endif
    int result = bind(sock, name, namelen);
    if (0 == result) {
        listen(sock, 256);
    }
    //??? should return errno
    return (RSIndex)result;
}

RSSocketError RSSocketConnectToAddress(RSSocketRef s, RSDataRef address, RSTimeInterval timeout) {
    CHECK_FOR_FORK();
    //??? need error handling, retries
    const uint8_t *name;
    RSIndex namelen, result = -1, connect_err = 0, select_err = 0;
    UInt32 yes = 1, no = 0;
    BOOL wasBlocking = YES;
    
    __RSGenericValidInstance(s, RSSocketGetTypeID());
    if (!RSSocketIsValid(s)) return RSSocketUnSuccess;
    name = RSDataGetBytesPtr(address);
    namelen = RSDataGetLength(address);
    if (!name || namelen <= 0) return RSSocketUnSuccess;
    RSSocketHandle sock = RSSocketGetHandle(s);
    {
#if DEPLOYMENT_TARGET_MACOSX || DEPLOYMENT_TARGET_EMBEDDED || DEPLOYMENT_TARGET_EMBEDDED_MINI
        SInt32 flags = fcntl(sock, F_GETFL, 0);
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
        result = connect(sock, (struct sockaddr *)name, (socklen_t)namelen);
        if (result != 0) {
            connect_err = __RSSocketLastError();
#if DEPLOYMENT_TARGET_WINDOWS
            if (connect_err == WSAEWOULDBLOCK) connect_err = EINPROGRESS;
#endif
        }
#if defined(LOG_RSSOCKET)
        fprintf(stdout, "connection attempt returns %d error %d on socket %d (flags 0x%x blocking %d)\n", (int) result, (int) connect_err, sock, (int) flags, wasBlocking);
#endif
        if (EINPROGRESS == connect_err && timeout >= 0.0) {
            /* select on socket */
            SInt32 nrfds;
            int error_size = sizeof(select_err);
            struct timeval tv;
            RSMutableDataRef fds = RSDataCreateMutable(RSAllocatorSystemDefault, 0);
            __RSSocketFdSet(sock, fds);
            tv.tv_sec = (timeout <= 0.0 || (RSTimeInterval)INT_MAX <= timeout) ? INT_MAX : (int)floor(timeout);
            tv.tv_usec = (int)floor(1.0e+6 * (timeout - floor(timeout)));
            nrfds = select((int)__RSSocketFdGetSize(fds), nil, (fd_set *)RSDataGetMutableBytesPtr(fds), nil, &tv);
            if (nrfds < 0) {
                select_err = __RSSocketLastError();
                result = -1;
            } else if (nrfds == 0) {
                result = -2;
            } else {
                if (0 != getsockopt(sock, SOL_SOCKET, SO_ERROR, (char *)&select_err, (socklen_t *)&error_size)) select_err = 0;
                result = (select_err == 0) ? 0 : -1;
            }
            RSRelease(fds);
#if defined(LOG_RSSOCKET)
            fprintf(stdout, "timed connection attempt %s on socket %d, result %d, select returns %d error %d\n", (result == 0) ? "succeeds" : "fails", sock, (int) result, (int) nrfds, (int) select_err);
#endif
        }
        if (wasBlocking && (timeout > 0.0 || timeout < 0.0)) ioctlsocket(sock, FIONBIO, (u_long *)&no);
        if (EINPROGRESS == connect_err && timeout < 0.0) {
            result = 0;
#if defined(LOG_RSSOCKET)
            fprintf(stdout, "connection attempt continues in background on socket %d\n", sock);
#endif
        }
    }
    //??? should return errno
    return result;
}

RSSocketRef RSSocketCreate(RSAllocatorRef allocator, SInt32 protocolFamily, SInt32 socketType, SInt32 protocol, RSOptionFlags callBackTypes, RSSocketCallBack callout, const RSSocketContext *context) {
    CHECK_FOR_FORK();
    RSSocketHandle sock = INVALID_SOCKET;
    RSSocketRef s = nil;
    if (0 >= protocolFamily) protocolFamily = PF_INET;
    if (PF_INET == protocolFamily) {
        if (0 >= socketType) socketType = SOCK_STREAM;
        if (0 >= protocol && SOCK_STREAM == socketType) protocol = IPPROTO_TCP;
        if (0 >= protocol && SOCK_DGRAM == socketType) protocol = IPPROTO_UDP;
    }
#if DEPLOYMENT_TARGET_MACOSX || DEPLOYMENT_TARGET_EMBEDDED || DEPLOYMENT_TARGET_EMBEDDED_MINI
    if (PF_LOCAL == protocolFamily && 0 >= socketType) socketType = SOCK_STREAM;
#endif
#if DEPLOYMENT_TARGET_WINDOWS
    // make sure we've called proper Win32 startup facilities before socket()
    __RSSocketInitializeWinSock();
#endif
    sock = socket(protocolFamily, socketType, protocol);
    if (INVALID_SOCKET != sock) {
        s = RSSocketCreateWithNative(allocator, sock, callBackTypes, callout, context);
    }
    return s;
}

RSSocketRef RSSocketCreateWithSocketSignature(RSAllocatorRef allocator, const RSSocketSignature *signature, RSOptionFlags callBackTypes, RSSocketCallBack callout, const RSSocketContext *context) {
    CHECK_FOR_FORK();
    RSSocketRef s = RSSocketCreate(allocator, signature->protocolFamily, signature->socketType, signature->protocol, callBackTypes, callout, context);
    if (nil != s && (!RSSocketIsValid(s) || RSSocketSuccess != RSSocketSetAddress(s, signature->address))) {
        RSSocketInvalidate(s);
        RSRelease(s);
        s = nil;
    }
    return s;
}

RSSocketRef RSSocketCreateConnectedToSocketSignature(RSAllocatorRef allocator, const RSSocketSignature *signature, RSOptionFlags callBackTypes, RSSocketCallBack callout, const RSSocketContext *context, RSTimeInterval timeout) {
    CHECK_FOR_FORK();
    RSSocketRef s = RSSocketCreate(allocator, signature->protocolFamily, signature->socketType, signature->protocol, callBackTypes, callout, context);
    if (nil != s && (!RSSocketIsValid(s) || RSSocketSuccess != RSSocketConnectToAddress(s, signature->address, timeout))) {
        RSSocketInvalidate(s);
        RSRelease(s);
        s = nil;
    }
    return s;
}

typedef struct {
    RSSocketError *error;
    RSPropertyListRef *value;
    RSDataRef *address;
} __RSSocketNameRegistryResponse;
#include <RSCoreFoundation/RSDictionary+Extension.h>
static void __RSSocketHandleNameRegistryReply(RSSocketRef s, RSSocketCallBackType type, RSDataRef address, const void *data, void *info) {
    RSDataRef replyData = (RSDataRef)data;
    __RSSocketNameRegistryResponse *response = (__RSSocketNameRegistryResponse *)info;
    RSDictionaryRef replyDictionary = nil;
    RSPropertyListRef value;
    
    replyDictionary = (RSDictionaryRef)RSDictionaryCreateWithData(RSAllocatorSystemDefault, replyData);
    if (nil != response->error) *(response->error) = RSSocketUnSuccess;
    if (nil != replyDictionary) {
        if (RSGetTypeID((RSTypeRef)replyDictionary) == RSDictionaryGetTypeID() && nil != (value = (RSPropertyListRef)RSDictionaryGetValue(replyDictionary, RSSocketResultKey))) {
            if (nil != response->error) *(response->error) = RSSocketSuccess;
            if (nil != response->value) *(response->value) = (RSPropertyListRef)RSRetain(value);
            if (nil != response->address) *(response->address) = address ? RSDataCreateCopy(RSAllocatorSystemDefault, address) : nil;
        }
        RSRelease(replyDictionary);
    }
    RSSocketInvalidate(s);
}

static void __RSSocketSendNameRegistryRequest(RSSocketSignature *signature, RSDictionaryRef requestDictionary, __RSSocketNameRegistryResponse *response, RSTimeInterval timeout) {
    RSDataRef requestData = nil;
    RSSocketContext context = {0, response, nil, nil, nil};
    RSSocketRef s = nil;
    RSRunLoopSourceRef source = nil;
    if (nil != response->error) *(response->error) = RSSocketUnSuccess;
    requestData = RSPropertyListCreateXMLData(RSAllocatorSystemDefault, requestDictionary);
    if (nil != requestData) {
        if (nil != response->error) *(response->error) = RSSocketTimeout;
        s = RSSocketCreateConnectedToSocketSignature(RSAllocatorSystemDefault, signature, RSSocketDataCallBack, __RSSocketHandleNameRegistryReply, &context, timeout);
        if (nil != s) {
            if (RSSocketSuccess == RSSocketSendData(s, nil, requestData, timeout)) {
                source = RSSocketCreateRunLoopSource(RSAllocatorSystemDefault, s, 0);
                RSRunLoopAddSource(RSRunLoopGetCurrent(), source, __RSSocketRegistryRequestRunLoopMode);
                RSRunLoopRunInMode(__RSSocketRegistryRequestRunLoopMode, timeout, NO);
                RSRelease(source);
            }
            RSSocketInvalidate(s);
            RSRelease(s);
        }
        RSRelease(requestData);
    }
}

static void __RSSocketValidateSignature(const RSSocketSignature *providedSignature, RSSocketSignature *signature, uint16_t defaultPortNumber) {
    struct sockaddr_in sain, *sainp;
    memset(&sain, 0, sizeof(sain));
#if DEPLOYMENT_TARGET_MACOSX || DEPLOYMENT_TARGET_EMBEDDED || DEPLOYMENT_TARGET_EMBEDDED_MINI
    sain.sin_len = sizeof(sain);
#endif
    sain.sin_family = AF_INET;
    sain.sin_port = htons(__RSSocketDefaultNameRegistryPortNumber);
    sain.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    if (nil == providedSignature) {
        signature->protocolFamily = PF_INET;
        signature->socketType = SOCK_STREAM;
        signature->protocol = IPPROTO_TCP;
        signature->address = RSDataCreate(RSAllocatorSystemDefault, (uint8_t *)&sain, sizeof(sain));
    } else {
        signature->protocolFamily = providedSignature->protocolFamily;
        signature->socketType = providedSignature->socketType;
        signature->protocol = providedSignature->protocol;
        if (0 >= signature->protocolFamily) signature->protocolFamily = PF_INET;
        if (PF_INET == signature->protocolFamily) {
            if (0 >= signature->socketType) signature->socketType = SOCK_STREAM;
            if (0 >= signature->protocol && SOCK_STREAM == signature->socketType) signature->protocol = IPPROTO_TCP;
            if (0 >= signature->protocol && SOCK_DGRAM == signature->socketType) signature->protocol = IPPROTO_UDP;
        }
        if (nil == providedSignature->address) {
            signature->address = RSDataCreate(RSAllocatorSystemDefault, (uint8_t *)&sain, sizeof(sain));
        } else {
            sainp = (struct sockaddr_in *)RSDataGetBytesPtr(providedSignature->address);
            if ((int)sizeof(struct sockaddr_in) <= RSDataGetLength(providedSignature->address) && (AF_INET == sainp->sin_family || 0 == sainp->sin_family)) {
#if DEPLOYMENT_TARGET_MACOSX || DEPLOYMENT_TARGET_EMBEDDED || DEPLOYMENT_TARGET_EMBEDDED_MINI
                sain.sin_len = sizeof(sain);
#endif
                sain.sin_family = AF_INET;
                sain.sin_port = sainp->sin_port;
                if (0 == sain.sin_port) sain.sin_port = htons(defaultPortNumber);
                sain.sin_addr.s_addr = sainp->sin_addr.s_addr;
                if (htonl(INADDR_ANY) == sain.sin_addr.s_addr) sain.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
                signature->address = RSDataCreate(RSAllocatorSystemDefault, (uint8_t *)&sain, sizeof(sain));
            } else {
                signature->address = (RSDataRef)RSRetain(providedSignature->address);
            }
        }
    }
}

RSSocketError RSSocketRegisterValue(const RSSocketSignature *nameServerSignature, RSTimeInterval timeout, RSStringRef name, RSTypeRef value) {
    RSSocketSignature signature;
    RSMutableDictionaryRef dictionary = RSDictionaryCreateMutable(RSAllocatorSystemDefault, 3, RSDictionaryRSTypeContext);
    RSSocketError retval = RSSocketUnSuccess;
    __RSSocketNameRegistryResponse response = {&retval, nil, nil};
    RSDictionarySetValue(dictionary, RSSocketCommandKey, RSSocketRegisterCommand);
    RSDictionarySetValue(dictionary, RSSocketNameKey, name);
    if (nil != value) RSDictionarySetValue(dictionary, RSSocketValueKey, value);
    __RSSocketValidateSignature(nameServerSignature, &signature, __RSSocketDefaultNameRegistryPortNumber);
    __RSSocketSendNameRegistryRequest(&signature, dictionary, &response, timeout);
    RSRelease(dictionary);
    RSRelease(signature.address);
    return retval;
}

RSSocketError RSSocketCopyRegisteredValue(const RSSocketSignature *nameServerSignature, RSTimeInterval timeout, RSStringRef name, RSPropertyListRef *value, RSDataRef *serverAddress) {
    RSSocketSignature signature;
    RSMutableDictionaryRef dictionary = RSDictionaryCreateMutable(RSAllocatorSystemDefault, 2, RSDictionaryRSTypeContext);
    RSSocketError retval = RSSocketUnSuccess;
    __RSSocketNameRegistryResponse response = {&retval, value, serverAddress};
    RSDictionarySetValue(dictionary, RSSocketCommandKey, RSSocketRetrieveCommand);
    RSDictionarySetValue(dictionary, RSSocketNameKey, name);
    __RSSocketValidateSignature(nameServerSignature, &signature, __RSSocketDefaultNameRegistryPortNumber);
    __RSSocketSendNameRegistryRequest(&signature, dictionary, &response, timeout);
    RSRelease(dictionary);
    RSRelease(signature.address);
    return retval;
}

RSSocketError RSSocketRegisterSocketSignature(const RSSocketSignature *nameServerSignature, RSTimeInterval timeout, RSStringRef name, const RSSocketSignature *signature) {
    RSSocketSignature validatedSignature;
    RSMutableDataRef data = nil;
    RSSocketError retval;
    RSIndex length;
    uint8_t bytes[4];
    if (nil == signature) {
        retval = RSSocketUnregister(nameServerSignature, timeout, name);
    } else {
        __RSSocketValidateSignature(signature, &validatedSignature, 0);
        if (nil == validatedSignature.address || 0 > validatedSignature.protocolFamily || 255 < validatedSignature.protocolFamily || 0 > validatedSignature.socketType || 255 < validatedSignature.socketType || 0 > validatedSignature.protocol || 255 < validatedSignature.protocol || 0 >= (length = RSDataGetLength(validatedSignature.address)) || 255 < length) {
            retval = RSSocketUnSuccess;
        } else {
            data = RSDataCreateMutable(RSAllocatorSystemDefault, sizeof(bytes) + length);
            bytes[0] = validatedSignature.protocolFamily;
            bytes[1] = validatedSignature.socketType;
            bytes[2] = validatedSignature.protocol;
            bytes[3] = length;
            RSDataAppendBytes(data, bytes, sizeof(bytes));
            RSDataAppendBytes(data, RSDataGetBytesPtr(validatedSignature.address), length);
            retval = RSSocketRegisterValue(nameServerSignature, timeout, name, data);
            RSRelease(data);
        }
        RSRelease(validatedSignature.address);
    }
    return retval;
}

RSSocketError RSSocketCopyRegisteredSocketSignature(const RSSocketSignature *nameServerSignature, RSTimeInterval timeout, RSStringRef name, RSSocketSignature *signature, RSDataRef *nameServerAddress) {
    RSDataRef data = nil;
    RSSocketSignature returnedSignature;
    const uint8_t *ptr = nil, *aptr = nil;
    uint8_t *mptr;
    RSIndex length = 0;
    RSDataRef serverAddress = nil;
    RSSocketError retval = RSSocketCopyRegisteredValue(nameServerSignature, timeout, name, (RSPropertyListRef *)&data, &serverAddress);
    if (nil == data || RSGetTypeID(data) != RSDataGetTypeID() || nil == (ptr = RSDataGetBytesPtr(data)) || (length = RSDataGetLength(data)) < 4) retval = RSSocketUnSuccess;
    if (RSSocketSuccess == retval && nil != signature) {
        returnedSignature.protocolFamily = (SInt32)*ptr++;
        returnedSignature.socketType = (SInt32)*ptr++;
        returnedSignature.protocol = (SInt32)*ptr++;
        ptr++;
        returnedSignature.address = RSDataCreate(RSAllocatorSystemDefault, ptr, length - 4);
        __RSSocketValidateSignature(&returnedSignature, signature, 0);
        RSRelease(returnedSignature.address);
        ptr = RSDataGetBytesPtr(signature->address);
        if (RSDataGetLength(signature->address) >= (int)sizeof(struct sockaddr_in) && AF_INET == ((struct sockaddr *)ptr)->sa_family && nil != serverAddress && RSDataGetLength(serverAddress) >= (int)sizeof(struct sockaddr_in) && nil != (aptr = RSDataGetBytesPtr(serverAddress)) && AF_INET == ((struct sockaddr *)aptr)->sa_family) {
            RSMutableDataRef address = RSMutableCopy(RSAllocatorSystemDefault, signature->address);
            mptr = RSDataGetMutableBytesPtr(address);
            ((struct sockaddr_in *)mptr)->sin_addr = ((struct sockaddr_in *)aptr)->sin_addr;
            RSRelease(signature->address);
            signature->address = address;
        }
        if (nil != nameServerAddress) *nameServerAddress = serverAddress ? (RSDataRef)RSRetain(serverAddress) : nil;
    }
    if (nil != data) RSRelease(data);
    if (nil != serverAddress) RSRelease(serverAddress);
    return retval;
}

RSSocketError RSSocketUnregister(const RSSocketSignature *nameServerSignature, RSTimeInterval timeout, RSStringRef name) {
    return RSSocketRegisterValue(nameServerSignature, timeout, name, nil);
}

RS_EXPORT void RSSocketSetDefaultNameRegistryPortNumber(uint16_t port) {
    __RSSocketDefaultNameRegistryPortNumber = port;
}

RS_EXPORT uint16_t RSSocketGetDefaultNameRegistryPortNumber(void) {
    return __RSSocketDefaultNameRegistryPortNumber;
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
