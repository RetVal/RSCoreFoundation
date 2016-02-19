/*
 * Copyright (c) 2005 Apple Computer, Inc. All rights reserved.
 *
 * @APPLE_LICENSE_HEADER_START@
 * 
 * This file contains Original Code and/or Modifications of Original Code
 * as defined in and that are subject to the Apple Public Source License
 * Version 2.0 (the 'License'). You may not use this file except in
 * compliance with the License. Please obtain a copy of the License at
 * http://www.opensource.apple.com/apsl/ and read it before using this
 * file.
 * 
 * The Original Code and all software distributed under the License are
 * distributed on an 'AS IS' basis, WITHOUT WARRANTY OF ANY KIND, EITHER
 * EXPRESS OR IMPLIED, AND APPLE HEREBY DISCLAIMS ALL SUCH WARRANTIES,
 * INCLUDING WITHOUT LIMITATION, ANY WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE, QUIET ENJOYMENT OR NON-INFRINGEMENT.
 * Please see the License for the specific language governing rights and
 * limitations under the License.
 * 
 * @APPLE_LICENSE_HEADER_END@
 */
/*
    RSSocketStreamImpl.h
    Copyright 1998-2003, Apple, Inc. All rights reserved.
    Responsibility: Jeremy Wyld

    Private header only for the set of files that implement RSSocketStream.
*/

#ifndef __RSSOCKETSTREAMIMPL__
#define __RSSOCKETSTREAMIMPL__

#include <RSNetwork/RSSocketStream.h>
#include "RSNetworkInternal.h"
#include <RSNetwork/RSHTTPStream.h>

#if defined(__MACH__)
#include <Security/SecureTransport.h>
#include <SystemConfiguration/SCNetworkReachability.h>
#endif

#if defined(__WIN32__)
#include <winsock2.h>
#include <ws2tcpip.h>	// for ipv6

// an alias for Win32 routines
#define ioctl(a, b, c)		ioctlsocket(a, b, c)

// Sockets and fds are not interchangeable on Win32, and have different error codes.
// These redefines assumes that in this file we only apply this error constant to socket ops.

#undef EAGAIN
#define EAGAIN WSAEWOULDBLOCK

#undef ECONNABORTED
#define ECONNABORTED WSAECONNABORTED

#undef ENOTCONN
#define ENOTCONN WSAENOTCONN

#undef ECONNREFUSED
#define ECONNREFUSED WSAECONNREFUSED

#undef EBADF
#define EBADF WSAENOTSOCK

#undef ETIMEDOUT
#define ETIMEDOUT WSAETIMEDOUT

#endif

#if defined(__cplusplus)
extern "C" {
#endif


#if defined(__WIN32__)

typedef struct _SchannelState *SchannelState;

// Win32 doesn't have an equivalent, but we reuse this for porting ease
typedef enum {
    kSSLIdle = 1,
    kSSLHandshake,
    kSSLConnected,
    kSSLClosed,
    kSSLAborted
} SSLSessionState;

#endif

// The bits for a socket context's flags.  Note that these are the bit indices, not the bit masks themselves.
enum {
    SHARED	= 0,

    CREATED_WITH_SOCKET,

    OPEN_START,
    OPEN_COMPLETE,

    SHOULD_CLOSE,
    KILL_SOCKET_ON_CLOSE,

    SECURITY_CHECK_CERTIFICATE,

    VERIFIED_READ_EVENT,
    VERIFIED_WRITE_EVENT,

    WRITE_STREAM_OPENED,
    READ_STREAM_OPENED,

    USE_ADDR_CACHE,
    RECVD_READ,
    RECVD_WRITE,

    SELECT_READ,
    SELECT_WRITE,

    NO_REACHABILITY,

    ICHAT_SUBNET_SETTING = 31
};

typedef struct {
    RSMutableArrayRef	runloops;
    struct _RSStream	*stream;		// NOT retained; if it's retained, we introduce a retain loop!
} _RSSocketStreamRLSource;

typedef struct {
    UInt32			socks_flags;		// Flags used for performing SOCKS handshake.
    RSHostRef		host;				// SOCKS server
    UInt32			port;				// Port of the SOCKS server
    RSStringRef		user;				// user id for the SOCKS server
    RSStringRef		pass;				// password for the SOCKS server
    RSIndex			bytesInBuffer;		// Number of bytes in the buffer.
    UInt8*			buffer;				// Bytes read or waiting to be written.
} _SOCKSInfo;

typedef struct {
    RSHostRef			host;
    UInt32				port;
    RSDictionaryRef		settings;
    RSDataRef			request;
    RSIndex				left;
    RSHTTPMessageRef	response;
} _CONNECTInfo;

struct _RSSocketStreamContext;
typedef void (*handshakeFn)(struct _RSSocketStreamContext*);

typedef struct {
    RSSpinLock_t		lock;			// Used to lock access if two separate streams exist for the same socket.
    RSOptionFlags		flags;

    RSAllocatorRef		alloc;

    RSStreamError		error;			// Store the error code for the operation that failed

    RSTypeRef			lookup;			// async lookup; either a RSHost or a RSNetService
    RSIndex				attempt;		// current address index being attempted
    RSSocketRef			sock;			// underlying RSSocket

    RSArrayRef			cb;
#if defined(__MACH__)
    SCNetworkReachabilityRef reachability;
#endif
    union {
        UInt32			port;			// Port number if created with host/port
        int				sock;			// socket if created with a native socket
    } u;

    RSSocketSignature 	sig;			// Signature for a stream pair created with one

    RSMutableArrayRef	runloops;       // loop/mode pairs that are scheduled for
                                        // both read and write
    _RSSocketStreamRLSource		*readSource, *writeSource;

    handshakeFn			handshakes[4];	// Holds the functions to be performed as part of open.

    RSStringRef			peerName;		// if set, overrides peer name from the host we looked up
#if defined(__MACH__)
    SSLContextRef		security;
    UInt8*				sslBuffer;
    RSIndex				sslBufferCount;
#elif defined(__WIN32__)
    SchannelState       ssl;
#endif
    _SOCKSInfo*			socks_info;
    _CONNECTInfo*		connect_info;
} _RSSocketStreamContext;


// General routines used by the implementation files, implemented in RSSocketStream.c

extern RSIndex __fdRecv(int fd, UInt8* buffer, RSIndex bufferLength, RSStreamError* errorCode, BOOL *atEOF);
extern RSIndex __fdSend(int fd, const UInt8* buffer, RSIndex bufferLength, RSStreamError* errorCode);

extern char* __getServerName(_RSSocketStreamContext* ctxt, char *buffer, UInt32 *bufSize, RSAllocatorRef *allocator);

extern BOOL __AddHandshake_Unsafe(_RSSocketStreamContext* ctxt, handshakeFn fn);
extern void __RemoveHandshake_Unsafe(_RSSocketStreamContext* ctxt, handshakeFn fn);
extern void __WaitForHandshakeToComplete_Unsafe(_RSSocketStreamContext* ctxt);

#define __socketGetFD(ctxt)	((int)((__RSBitIsSet(ctxt->flags, CREATED_WITH_SOCKET)) ? ctxt->u.sock : (ctxt->sock ? RSSocketGetNative(ctxt->sock) : -1)))


// SSL routines used by RSSocketStream.c, implemented on both Mach and Win32

extern RSIndex sslRecv(_RSSocketStreamContext* ctxt, UInt8* buffer, RSIndex bufferLength, RSStreamError* errorCode, BOOL *atEOF);
extern RSIndex sslSend(_RSSocketStreamContext* ctxt, const UInt8* buffer, RSIndex bufferLength, RSStreamError* errorCode);
extern void sslClose(_RSSocketStreamContext* ctxt);
extern RSIndex sslBytesAvailableForRead(_RSSocketStreamContext* ctxt);

extern void performSSLHandshake(_RSSocketStreamContext* ctxt);
extern void performSSLSendHandshake(_RSSocketStreamContext* ctxt);

extern BOOL __SetSecuritySettings_Unsafe(_RSSocketStreamContext* ctxt, RSDictionaryRef settings);
extern RSStringRef __GetSSLProtocol(_RSSocketStreamContext* ctxt);
extern SSLSessionState __GetSSLSessionState(_RSSocketStreamContext* ctxt);

#if 0
#define SSL_LOG printf
#else
#define SSL_LOG while(0) printf
#endif

#if defined(__MACH__)

#define IS_SECURE(x)				((x)->security != NULL)
#define SSL_WOULD_BLOCK(error) ((error->domain == kRSStreamErrorDomainSSL) && (errSSLWouldBlock == error->error) )

#elif defined(__WIN32__)

#define IS_SECURE(ctxt)	((ctxt)->ssl != NULL)
// On Windows there is no SSL-specific way to represent this, it uses the normal EAGAIN
#define SSL_WOULD_BLOCK(error) (FALSE)

#endif  /* __WIN32__ */

#if defined(__cplusplus)
}
#endif

#endif	/* __RSSOCKETSTREAMIMPL__ */

