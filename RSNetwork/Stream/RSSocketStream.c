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
 *  RSSocketStream.c
 *  
 *
 *  Created by Jeremy Wyld on Mon Apr 26 2004.
 *  Copyright (c) 2004 Apple Computer, Inc. All rights reserved.
 *
 */

#if 0
#pragma mark Includes
#endif

#include "RSNetworkInternal.h"
#include "RSNetworkSchedule.h"
// For _RSNetworkUserAgentString()
#include "RSHTTPInternal.h" 

#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <sys/ioctl.h>
#include <sys/fcntl.h>

#include <CoreFoundation/RSStreamPriv.h>
#include <RSNetwork/RSSocketStreamPriv.h>
#include <SystemConfiguration/SystemConfiguration.h>
#include <Security/Security.h>
#include <Security/SecureTransportPriv.h>

#if 0
#pragma mark -
#pragma mark Constants
#endif

#define kSocketEvents ((RSOptionFlags)(kRSSocketReadCallBack | kRSSocketConnectCallBack | kRSSocketWriteCallBack))
#define kReadWriteTimeoutInterval ((RSTimeInterval)75.0)
#define kRecvBufferSize		((RSIndex)(32768L));
#define kSecurityBufferSize	((RSIndex)(32768L));

#ifndef __MACH__
const int kRSStreamErrorDomainSOCKS = 5;	/* On Mach this lives in RS for historical reasons, even though it is declared in RSNetwork */
#endif


#if 0
#pragma mark *Constant Strings
#pragma mark **Stream Property Keys
#endif

/* Properties made available as API */
CONST_STRING_DECL(kRSStreamPropertySocketRemoteHost, "kRSStreamPropertySocketRemoteHost")
CONST_STRING_DECL(kRSStreamPropertySocketRemoteNetService, "kRSStreamPropertySocketRemoteNetService")
CONST_STRING_DECL(kRSStreamPropertyShouldCloseNativeSocket, "kRSStreamPropertyShouldCloseNativeSocket")

CONST_STRING_DECL(_kRSStreamPropertySocketPeerName, "_kRSStreamPropertySocketPeerName")

CONST_STRING_DECL(kRSStreamPropertySSLPeerCertificates, "kRSStreamPropertySSLPeerCertificates")
CONST_STRING_DECL(_kRSStreamPropertySSLClientCertificates, "_kRSStreamPropertySSLClientCertificates")
CONST_STRING_DECL(_kRSStreamPropertySSLClientCertificateState, "_kRSStreamPropertySSLClientCertificateState")
CONST_STRING_DECL(kRSStreamPropertySSLSettings, "kRSStreamPropertySSLSettings")
CONST_STRING_DECL(kRSStreamSSLAllowsAnyRoot, "kRSStreamSSLAllowsAnyRoot")
CONST_STRING_DECL(kRSStreamSSLAllowsExpiredCertificates, "kRSStreamSSLAllowsExpiredCertificates")
CONST_STRING_DECL(kRSStreamSSLAllowsExpiredRoots, "kRSStreamSSLAllowsExpiredRoots")
CONST_STRING_DECL(kRSStreamSSLCertificates, "kRSStreamSSLCertificates")
CONST_STRING_DECL(kRSStreamSSLIsServer, "kRSStreamSSLIsServer")
CONST_STRING_DECL(kRSStreamSSLLevel, "kRSStreamSSLLevel")
CONST_STRING_DECL(kRSStreamSSLPeerName, "kRSStreamSSLPeerName")
CONST_STRING_DECL(kRSStreamSSLValidatesCertificateChain, "kRSStreamSSLValidatesCertificateChain")
CONST_STRING_DECL(kRSStreamSocketSecurityLevelTLSv1SSLv3, "kRSStreamSocketSecurityLevelTLSv1SSLv3")

/*!
    @constant _kRSStreamPropertySSLAllowAnonymousCiphers
    @discussion Stream property key both set and copy operations. RSBOOLRef to set whether
        anonymous ciphers are allowed or not. The value is kRSBOOLFalse by default.
*/
CONST_STRING_DECL(_kRSStreamPropertySSLAllowAnonymousCiphers, "_kRSStreamPropertySSLAllowAnonymousCiphers")

/* Properties made available as SPI */
CONST_STRING_DECL(kRSStreamPropertyUseAddressCache, "kRSStreamPropertyUseAddressCache")
CONST_STRING_DECL(_kRSStreamSocketIChatWantsSubNet, "_kRSStreamSocketIChatWantsSubNet")
CONST_STRING_DECL(_kRSStreamSocketCreatedCallBack, "_kRSStreamSocketCreatedCallBack")
CONST_STRING_DECL(kRSStreamPropertyProxyExceptionsList, "ExceptionsList")
CONST_STRING_DECL(kRSStreamPropertyProxyLocalBypass, "ExcludeSimpleHostnames");

/* CONNECT tunnel properties.  Still SPI. */
CONST_STRING_DECL(kRSStreamPropertyCONNECTProxy, "kRSStreamPropertyCONNECTProxy")
CONST_STRING_DECL(kRSStreamPropertyCONNECTProxyHost, "kRSStreamPropertyCONNECTProxyHost")
CONST_STRING_DECL(kRSStreamPropertyCONNECTProxyPort, "kRSStreamPropertyCONNECTProxyPort")
CONST_STRING_DECL(kRSStreamPropertyCONNECTVersion, "kRSStreamPropertyCONNECTVersion")
CONST_STRING_DECL(kRSStreamPropertyCONNECTAdditionalHeaders, "kRSStreamPropertyCONNECTAdditionalHeaders")
CONST_STRING_DECL(kRSStreamPropertyCONNECTResponse, "kRSStreamPropertyCONNECTResponse")
CONST_STRING_DECL(kRSStreamPropertyPreviousCONNECTResponse, "kRSStreamPropertyPreviousCONNECTResponse")

/* Properties used internally to RSSocketStream */
#ifdef __CONSTANT_RSSTRINGS__
#define _kRSStreamProxySettingSOCKSEnable			RSSTR("SOCKSEnable")
#define _kRSStreamPropertySocketRemotePort			RSSTR("_kRSStreamPropertySocketRemotePort")
#define _kRSStreamPropertySocketAddressAttempt		RSSTR("_kRSStreamPropertySocketAddressAttempt")
#define _kRSStreamPropertySocketFamilyTypeProtocol	RSSTR("_kRSStreamPropertySocketFamilyTypeProtocol")
#define _kRSStreamPropertyHostForOpen				RSSTR("_kRSStreamPropertyHostForOpen")
#define _kRSStreamPropertyNetworkReachability		RSSTR("_kRSStreamPropertyNetworkReachability")
#define _kRSStreamPropertyRecvBuffer				RSSTR("_kRSStreamPropertyRecvBuffer")
#define _kRSStreamPropertyRecvBufferCount			RSSTR("_kRSStreamPropertyRecvBufferCount")
#define _kRSStreamPropertyRecvBufferSize			RSSTR("_kRSStreamPropertyRecvBufferSize")
#define _kRSStreamPropertySecurityRecvBuffer		RSSTR("_kRSStreamPropertySecurityRecvBuffer")
#define _kRSStreamPropertySecurityRecvBufferSize	RSSTR("_kRSStreamPropertySecurityRecvBufferSize")
#define _kRSStreamPropertySecurityRecvBufferCount	RSSTR("_kRSStreamPropertySecurityRecvBufferCount")
#define _kRSStreamPropertyHandshakes				RSSTR("_kRSStreamPropertyHandshakes")
#define _kRSStreamPropertyCONNECTSendBuffer			RSSTR("_kRSStreamPropertyCONNECTSendBuffer")
#define _kRSStreamPropertySOCKSSendBuffer			RSSTR("_kRSStreamPropertySOCKSSendBuffer")
#define _kRSStreamPropertySOCKSRecvBuffer			RSSTR("_kRSStreamPropertySOCKSRecvBuffer")
#define _kRSStreamPropertyReadTimeout				RSSTR("_kRSStreamPropertyReadTimeout")
#define _kRSStreamPropertyWriteTimeout				RSSTR("_kRSStreamPropertyWriteTimeout")
#define _kRSStreamPropertyReadCancel				RSSTR("_kRSStreamPropertyReadCancel")
#define _kRSStreamPropertyWriteCancel				RSSTR("_kRSStreamPropertyWriteCancel")
#else
static CONST_STRING_DECL(_kRSStreamProxySettingSOCKSEnable, "SOCKSEnable")
static CONST_STRING_DECL(_kRSStreamPropertySocketRemotePort, "_kRSStreamPropertySocketRemotePort")
static CONST_STRING_DECL(_kRSStreamPropertySocketAddressAttempt, "_kRSStreamPropertySocketAddressAttempt")
static CONST_STRING_DECL(_kRSStreamPropertySocketFamilyTypeProtocol, "_kRSStreamPropertySocketFamilyTypeProtocol")
static CONST_STRING_DECL(_kRSStreamPropertyHostForOpen, "_kRSStreamPropertyHostForOpen")
static CONST_STRING_DECL(_kRSStreamPropertyNetworkReachability, "_kRSStreamPropertyNetworkReachability")
static CONST_STRING_DECL(_kRSStreamPropertyRecvBuffer, "_kRSStreamPropertyRecvBuffer")
static CONST_STRING_DECL(_kRSStreamPropertyRecvBufferCount, "_kRSStreamPropertyRecvBufferCount")
static CONST_STRING_DECL(_kRSStreamPropertyRecvBufferSize, "_kRSStreamPropertyRecvBufferSize")
static CONST_STRING_DECL(_kRSStreamPropertySecurityRecvBuffer, "_kRSStreamPropertySecurityRecvBuffer")
static CONST_STRING_DECL(_kRSStreamPropertySecurityRecvBufferSize, "_kRSStreamPropertySecurityRecvBufferSize")
static CONST_STRING_DECL(_kRSStreamPropertySecurityRecvBufferCount, "_kRSStreamPropertySecurityRecvBufferCount")
static CONST_STRING_DECL(_kRSStreamPropertyHandshakes, "_kRSStreamPropertyHandshakes")
static CONST_STRING_DECL(_kRSStreamPropertyCONNECTSendBuffer, "_kRSStreamPropertyCONNECTSendBuffer")
static CONST_STRING_DECL(_kRSStreamPropertySOCKSSendBuffer, "_kRSStreamPropertySOCKSSendBuffer")
static CONST_STRING_DECL(_kRSStreamPropertySOCKSRecvBuffer, "_kRSStreamPropertySOCKSRecvBuffer")
static CONST_STRING_DECL(_kRSStreamPropertyReadCancel, "_kRSStreamPropertyReadCancel")
static CONST_STRING_DECL(_kRSStreamPropertyWriteCancel, "_kRSStreamPropertyWriteCancel")
#endif	/* __CONSTANT_RSSTRINGS__ */

#ifdef __MACH__
extern const RSStringRef kRSStreamPropertyAutoErrorOnSystemChange;
CONST_STRING_DECL(kRSStreamPropertySocketSSLContext, "kRSStreamPropertySocketSSLContext")
CONST_STRING_DECL(_kRSStreamPropertySocketSecurityAuthenticatesServerCertificate, "_kRSStreamPropertySocketSecurityAuthenticatesServerCertificate")
#else
/* On Mach these live in RS for historical reasons, even though they are declared in RSNetwork */
CONST_STRING_DECL(kRSStreamPropertySOCKSProxy, "kRSStreamPropertySOCKSProxy")
CONST_STRING_DECL(kRSStreamPropertySOCKSProxyHost, "SOCKSProxy")
CONST_STRING_DECL(kRSStreamPropertySOCKSProxyPort, "SOCKSPort")
CONST_STRING_DECL(kRSStreamPropertySOCKSVersion, "kRSStreamPropertySOCKSVersion")
CONST_STRING_DECL(kRSStreamSocketSOCKSVersion4, "kRSStreamSocketSOCKSVersion4")
CONST_STRING_DECL(kRSStreamSocketSOCKSVersion5, "kRSStreamSocketSOCKSVersion5")
CONST_STRING_DECL(kRSStreamPropertySOCKSUser, "kRSStreamPropertySOCKSUser")
CONST_STRING_DECL(kRSStreamPropertySOCKSPassword, "kRSStreamPropertySOCKSPassword")

CONST_STRING_DECL(kRSStreamPropertyAutoErrorOnSystemChange, "kRSStreamPropertyAutoErrorOnSystemChange")
#endif


#if 0
#pragma mark **Other Strings
#endif

/* Keys for _kRSStreamPropertySocketFamilyTypeProtocol dictionary */
#ifdef __CONSTANT_RSSTRINGS__
#define	_kRSStreamSocketFamily		RSSTR("_kRSStreamSocketFamily")
#define	_kRSStreamSocketType		RSSTR("_kRSStreamSocketType")
#define	_kRSStreamSocketProtocol	RSSTR("_kRSStreamSocketProtocol")
#else
static CONST_STRING_DECL(_kRSStreamSocketFamily, "_kRSStreamSocketFamily")
static CONST_STRING_DECL(_kRSStreamSocketType, "_kRSStreamSocketType")
static CONST_STRING_DECL(_kRSStreamSocketProtocol, "_kRSStreamSocketProtocol")
#endif	/* __CONSTANT_RSSTRINGS__ */

/*
** Private modes for run loop polling.  These need to each be unique
** because one half should not affect the others.  E.g. one half
** polling in write should not directly conflict with the other
** half scheduled for read.
*/
#ifdef __CONSTANT_RSSTRINGS__
#define _kRSStreamSocketOpenCompletedPrivateMode	RSSTR("_kRSStreamSocketOpenCompletedPrivateMode")
#define _kRSStreamSocketReadPrivateMode				RSSTR("_kRSStreamSocketReadPrivateMode")
#define _kRSStreamSocketCanReadPrivateMode			RSSTR("_kRSStreamSocketCanReadPrivateMode")
#define _kRSStreamSocketWritePrivateMode			RSSTR("_kRSStreamSocketWritePrivateMode")
#define _kRSStreamSocketCanWritePrivateMode			RSSTR("_kRSStreamSocketCanWritePrivateMode")
#define _kRSStreamSocketSecurityClosePrivateMode	RSSTR("_kRSStreamSocketSecurityClosePrivateMode")
#define _kRSStreamSocketBogusPrivateMode			RSSTR("_kRSStreamSocketBogusPrivateMode")
#define _kRSStreamPropertyBogusRunLoop				RSSTR("_kRSStreamPropertyBogusRunLoop")
#else
static CONST_STRING_DECL(_kRSStreamSocketOpenCompletedPrivateMode, "_kRSStreamSocketOpenCompletedPrivateMode")
static CONST_STRING_DECL(_kRSStreamSocketReadPrivateMode, "_kRSStreamSocketReadPrivateMode")
static CONST_STRING_DECL(_kRSStreamSocketCanReadPrivateMode, "_kRSStreamSocketCanReadPrivateMode")
static CONST_STRING_DECL(_kRSStreamSocketWritePrivateMode, "_kRSStreamSocketWritePrivateMode")
static CONST_STRING_DECL(_kRSStreamSocketCanWritePrivateMode, "_kRSStreamSocketCanWritePrivateMode")
static CONST_STRING_DECL(_kRSStreamSocketSecurityClosePrivateMode, "_kRSStreamSocketSecurityClosePrivateMode")

static CONST_STRING_DECL(_kRSStreamSocketBogusPrivateMode, "_kRSStreamSocketBogusPrivateMode")
static CONST_STRING_DECL(_kRSStreamPropertyBogusRunLoop, "_kRSStreamPropertyBogusRunLoop")
#endif	/* __CONSTANT_RSSTRINGS__ */

/* Special strings and formats for performing CONNECT. */
#ifdef __CONSTANT_RSSTRINGS__
#define _kRSStreamCONNECTURLFormat	RSSTR("%@:%d")
#define _kRSStreamCONNECTMethod		RSSTR("CONNECT")
#define _kRSStreamUserAgentHeader	RSSTR("User-Agent")
#define _kRSStreamHostHeader		RSSTR("Host")
#else
static CONST_STRING_DECL(_kRSStreamCONNECTURLFormat, "%@:%d")
static CONST_STRING_DECL(_kRSStreamCONNECTMethod, "CONNECT")
static CONST_STRING_DECL(_kRSStreamUserAgentHeader, "User-Agent")
static CONST_STRING_DECL(_kRSStreamHostHeader, "Host")
#endif	/* __CONSTANT_RSSTRINGS__ */

/* AutoVPN strings */
#ifdef __CONSTANT_RSSTRINGS__
#define _kRSStreamAutoHostName					RSSTR("OnDemandHostName")
#define _kRSStreamPropertyAutoConnectPriority	RSSTR("OnDemandPriority")
#define _kRSStreamAutoVPNPriorityDefault		RSSTR("Default")
#else
static CONST_STRING_DECL(_kRSStreamAutoHostName, "OnDemandHostName")					/* **FIXME** Remove after PPPControllerPriv.h comes back */
static CONST_STRING_DECL(_kRSStreamPropertyAutoConnectPriority, "OnDemandPriority")	/* **FIXME** Ditto. */
static CONST_STRING_DECL(_kRSStreamAutoVPNPriorityDefault, "Default")
#endif	/* __CONSTANT_RSSTRINGS__ */

/* String used for CopyDescription function */
#ifdef __CONSTANT_RSSTRINGS__
#define kRSSocketStreamDescriptionFormat	RSSTR("<SocketStream %p>{flags = 0x%08x, read = %p, write = %p, socket = %@, properties = %p }")
#else
static CONST_STRING_DECL(kRSSocketStreamDescriptionFormat, "<SocketStream %p>{flags = 0x%08x, read = %p, write = %p, socket = %@, properties = %p }");
#endif	/* __CONSTANT_RSSTRINGS__ */


#if 0
#pragma mark -
#pragma mark Enum Values
#endif

enum {
    /* _RSSocketStreamContext flags */
	kFlagBitOpenStarted = 0,
	
	/* NOTE that the following three need to be kept in order */
    kFlagBitOpenComplete,		/* RSSocket is open and connected */
	kFlagBitCanRead,
	kFlagBitCanWrite,
	
	/* NOTE that the following three need to be kept in order (and ordered relative to the previous three) */
	kFlagBitPollOpen,			/* If the stream is event based and this bit is not set, OpenCompleted returns "no" immediately. */
	kFlagBitPollRead,			/* If the stream is event based and this bit is not set, CanRead returns "no" immediately. */
	kFlagBitPollWrite,			/* If the stream is event based and this bit is not set, CanWrite returns "no" immediately. */
	
	kFlagBitShared,				/* Indicates the stream structure is shared (read and write) */
	kFlagBitCreatedNative,		/* Stream(s) were created from a native socket handle. */
	kFlagBitReadStreamOpened,   /* Client has called open on the read stream */
	kFlagBitWriteStreamOpened,  /* Client has called open on the write stream */
	kFlagBitUseSSL,				/* Used for quickly determining SSL code paths. */
	kFlagBitClosed,				/* Signals that a close has been received on the buffered stream */
	kFlagBitTriedVPN,			/* Marked if an attempt at AutoVPN has been made */
	kFlagBitHasHandshakes,		/* Performance check for handshakes. */
	kFlagBitIsBuffered,			/* Performance check for using buffered reads. */
	kFlagBitRecvdRead,			/* On buffered streams, indicates that a read event has been received but buffer was full. */
	
	kFlagBitReadHasCancel,		/* Performance check for detecting run loop source for canceling synchronous read. */
	kFlagBitWriteHasCancel,		/* Performance check for detecting run loop source for canceling synchronous write. */
	
	/*
	** These flag bits are used to count the number of runs through the run loop short circuit
	** code at the end of read and write.  RSSocketStream is willing to run kMaximumNumberLoopAttempts
	** times through the run loop without forcing RSSocket to signal.  If kMaximumNumberLoopAttempts
	** attempts have not occurred, RSSocketStream will short circuit by selecting on the socket and
	** automatically marking the stream as readable or writable.
	*/
	kFlagBitMinLoops,
	kFlagBitMaxLoops = kFlagBitMinLoops + 3,
	kMaximumNumberLoopAttempts = (1 << (kFlagBitMaxLoops - kFlagBitMinLoops)),
	
	kSelectModeRead = 1,
	kSelectModeWrite = 2,
	kSelectModeExcept = 4
};


#if 0
#pragma mark -
#pragma mark Type Declarations
#pragma mark *RSStream Context
#endif

typedef struct {
	
	RSSpinLock_t				_lock;				/* Protection for read-half versus write-half */
	
	UInt32						_flags;
	RSStreamError				_error;
	
	RSReadStreamRef				_clientReadStream;
	RSWriteStreamRef			_clientWriteStream;
	
	RSSocketRef					_socket;			/* Actual underlying RSSocket */
	
    RSMutableArrayRef			_readloops;
    RSMutableArrayRef			_writeloops;
    RSMutableArrayRef			_sharedloops;
	
	RSMutableArrayRef			_schedulables;		/* Items to be scheduled (i.e. socket, reachability, host, etc.) */
		
	RSMutableDictionaryRef		_properties;		/* Host and port and reachability should be here too. */
	
} _RSSocketStreamContext;

#if 0
#pragma mark *Other Types
#endif

typedef void (*_RSSocketStreamSocketCreatedCallBack)(RSSocketNativeHandle s, void* info);
typedef void (*_RSSocketStreamPerformHandshakeCallBack)(_RSSocketStreamContext* ctxt);


#if 0
#pragma mark -
#pragma mark Static Function Declarations
#pragma mark *Stream Callbacks
#endif

static void _SocketStreamFinalize(RSTypeRef stream, _RSSocketStreamContext* ctxt);
static RSStringRef _SocketStreamCopyDescription(RSTypeRef stream, _RSSocketStreamContext* ctxt);
static BOOL _SocketStreamOpen(RSTypeRef stream, RSStreamError* error, BOOL* openComplete, _RSSocketStreamContext* ctxt);
static BOOL _SocketStreamOpenCompleted(RSTypeRef stream, RSStreamError* error, _RSSocketStreamContext* ctxt);
static RSIndex _SocketStreamRead(RSReadStreamRef stream, UInt8* buffer, RSIndex bufferLength, RSStreamError* error, BOOL* atEOF, _RSSocketStreamContext* ctxt);
static BOOL _SocketStreamCanRead(RSReadStreamRef stream, _RSSocketStreamContext* ctxt);
static RSIndex _SocketStreamWrite(RSWriteStreamRef stream, const UInt8* buffer, RSIndex bufferLength, RSStreamError* error, _RSSocketStreamContext* ctxt);
static BOOL _SocketStreamCanWrite(RSWriteStreamRef stream, _RSSocketStreamContext* ctxt);
static void _SocketStreamClose(RSTypeRef stream, _RSSocketStreamContext* ctxt);
static RSTypeRef _SocketStreamCopyProperty(RSTypeRef stream, RSStringRef propertyName, _RSSocketStreamContext* ctxt);
static BOOL _SocketStreamSetProperty(RSTypeRef stream, RSStringRef propertyName, RSTypeRef propertyValue, _RSSocketStreamContext* ctxt);
static void _SocketStreamSchedule(RSTypeRef stream, RSRunLoopRef runLoop, RSStringRef runLoopMode, _RSSocketStreamContext* ctxt);
static void _SocketStreamUnschedule(RSTypeRef stream, RSRunLoopRef runLoop, RSStringRef runLoopMode, _RSSocketStreamContext* ctxt);

#if 0
#pragma mark *Utility Functions
#endif

static void _SocketCallBack(RSSocketRef s, RSSocketCallBackType type, RSDataRef address, const void* data, _RSSocketStreamContext*info);
static void _HostCallBack(RSHostRef theHost, RSHostInfoType typeInfo, const RSStreamError* error, _RSSocketStreamContext* info);
static void _NetServiceCallBack(RSNetServiceRef theService, RSStreamError* error, _RSSocketStreamContext* info);
static void _SocksHostCallBack(RSHostRef theHost, RSHostInfoType typeInfo, const RSStreamError* error, _RSSocketStreamContext* info);
static void _ReachabilityCallBack(SCNetworkReachabilityRef target, const SCNetworkConnectionFlags flags, _RSSocketStreamContext* ctxt);
static void _NetworkConnectionCallBack(SCNetworkConnectionRef conn, SCNetworkConnectionStatus status, _RSSocketStreamContext* ctxt);

static BOOL _SchedulablesAdd(RSMutableArrayRef schedulables, RSTypeRef addition);
static BOOL _SchedulablesRemove(RSMutableArrayRef schedulables, RSTypeRef removal);
static void _SchedulablesScheduleApplierFunction(RSTypeRef obj, RSTypeRef loopAndMode[]);
static void _SchedulablesUnscheduleApplierFunction(RSTypeRef obj, RSTypeRef loopAndMode[]);
static void _SchedulablesUnscheduleFromAllApplierFunction(RSTypeRef obj, RSArrayRef schedules);
static void _SchedulablesInvalidateApplierFunction(RSTypeRef obj, void* context);
static void _SocketStreamSchedule_NoLock(RSTypeRef stream, RSRunLoopRef runLoop, RSStringRef runLoopMode, _RSSocketStreamContext* ctxt);
static void _SocketStreamUnschedule_NoLock(RSTypeRef stream, RSRunLoopRef runLoop, RSStringRef runLoopMode, _RSSocketStreamContext* ctxt);

static RSNumberRef _RSNumberCopyPortForOpen(RSDictionaryRef properties);
static RSDataRef _RSDataCopyAddressByInjectingPort(RSDataRef address, RSNumberRef port);
static BOOL _ScheduleAndStartLookup(RSTypeRef lookup, RSArrayRef* schedules, RSStreamError* error, const void* cb, void* info);

static RSIndex _RSSocketRecv(RSSocketRef s, UInt8* buffer, RSIndex length, RSStreamError* error);
static RSIndex _RSSocketSend(RSSocketRef s, const UInt8* buffer, RSIndex length, RSStreamError* error);
static BOOL _RSSocketCan(RSSocketRef s, int mode);

static _RSSocketStreamContext* _SocketStreamCreateContext(RSAllocatorRef alloc);
static void _SocketStreamDestroyContext_NoLock(RSAllocatorRef alloc, _RSSocketStreamContext* ctxt);

static BOOL _SocketStreamStartLookupForOpen_NoLock(_RSSocketStreamContext* ctxt);
static BOOL _SocketStreamCreateSocket_NoLock(_RSSocketStreamContext* ctxt, RSDataRef address);
static BOOL _SocketStreamConnect_NoLock(_RSSocketStreamContext* ctxt, RSDataRef address);
static BOOL _SocketStreamAttemptNextConnection_NoLock(_RSSocketStreamContext* ctxt);

static BOOL _SocketStreamCan(_RSSocketStreamContext* ctxt, RSTypeRef stream, int test, RSStringRef mode, RSStreamError* error);

static void _SocketStreamAddReachability_NoLock(_RSSocketStreamContext* ctxt);
static void _SocketStreamRemoveReachability_NoLock(_RSSocketStreamContext* ctxt);

static RSComparisonResult _OrderHandshakes(_RSSocketStreamPerformHandshakeCallBack fn1, _RSSocketStreamPerformHandshakeCallBack fn2, void* context);
static BOOL _SocketStreamAddHandshake_NoLock(_RSSocketStreamContext* ctxt, _RSSocketStreamPerformHandshakeCallBack fn);
static void _SocketStreamRemoveHandshake_NoLock(_RSSocketStreamContext* ctxt, _RSSocketStreamPerformHandshakeCallBack fn);

static void _SocketStreamAttemptAutoVPN_NoLock(_RSSocketStreamContext* ctxt, RSStringRef name);

static RSIndex _SocketStreamBufferedRead_NoLock(_RSSocketStreamContext* ctxt, UInt8* buffer, RSIndex length);
static void _SocketStreamBufferedSocketRead_NoLock(_RSSocketStreamContext* ctxt);

static void _SocketStreamPerformCancel(void* info);

RS_INLINE SInt32 _LastError(RSStreamError* error) {
	
	error->domain = _kRSStreamErrorDomainNativeSockets;
	
#if defined(__WIN32__)
	error->error = WSAGetLastError();
	if (!error->error) {
		error->error = errno;
		error->domain = kRSStreamErrorDomainPOSIX;
	}
#else
	error->error = errno;
#endif	/* __WIN32__ */
	
	return error->error;
}


#if 0
#pragma mark *SOCKS Support
#endif

static void _PerformSOCKSv5Handshake_NoLock(_RSSocketStreamContext* ctxt);
static void _PerformSOCKSv5PostambleHandshake_NoLock(_RSSocketStreamContext* ctxt);
static void _PerformSOCKSv5UserPassHandshake_NoLock(_RSSocketStreamContext* ctxt);
static void _PerformSOCKSv4Handshake_NoLock(_RSSocketStreamContext* ctxt);
static BOOL _SOCKSSetInfo_NoLock(_RSSocketStreamContext* ctxt, RSDictionaryRef settings);

static void _SocketStreamSOCKSHandleLookup_NoLock(_RSSocketStreamContext* ctxt, RSHostRef lookup);

RS_INLINE RSStringRef _GetSOCKSVersion(RSDictionaryRef settings) {
	
	RSStringRef result = (RSStringRef)RSDictionaryGetValue(settings, kRSStreamPropertySOCKSVersion);
	if (!result)
		result = kRSStreamSocketSOCKSVersion5;
	
	return result;
}


#if 0
#pragma mark *CONNECT Support
#endif

static void _CreateNameAndPortForCONNECTProxy(RSDictionaryRef properties, RSStringRef* name, RSNumberRef* port, RSStreamError* error);
static void _PerformCONNECTHandshake_NoLock(_RSSocketStreamContext* ctxt);
static void _PerformCONNECTHaltHandshake_NoLock(_RSSocketStreamContext* ctxt);
static void _CONNECTHeaderApplier(RSStringRef key, RSStringRef value, RSHTTPMessageRef request);
static BOOL _CONNECTSetInfo_NoLock(_RSSocketStreamContext* ctxt, RSDictionaryRef settings);


#if 0
#pragma mark *SSL Support
#endif

static OSStatus _SecurityReadFunc_NoLock(_RSSocketStreamContext* ctxt, void* data, UInt32* dataLength);
static OSStatus _SecurityWriteFunc_NoLock(_RSSocketStreamContext* ctxt, const void* data, UInt32* dataLength);
static RSIndex _SocketStreamSecuritySend_NoLock(_RSSocketStreamContext* ctxt, const UInt8* buffer, RSIndex length);
static void _SocketStreamSecurityBufferedRead_NoLock(_RSSocketStreamContext* ctxt);
static void _PerformSecurityHandshake_NoLock(_RSSocketStreamContext* ctxt);
static void _PerformSecuritySendHandshake_NoLock(_RSSocketStreamContext* ctxt);
static void _SocketStreamSecurityClose_NoLock(_RSSocketStreamContext* ctxt);
static BOOL _SocketStreamSecuritySetContext_NoLock(_RSSocketStreamContext *ctxt, RSDataRef value);
static BOOL _SocketStreamSecuritySetInfo_NoLock(_RSSocketStreamContext* ctxt, RSDictionaryRef settings);
static BOOL _SocketStreamSecuritySetAuthenticatesServerCertificates_NoLock(_RSSocketStreamContext* ctxt, RSBOOLRef authenticates);
static RSStringRef _SecurityGetProtocol(SSLContextRef security);
static SSLSessionState _SocketStreamSecurityGetSessionState_NoLock(_RSSocketStreamContext* ctxt);


#if 0
#pragma mark -
#pragma mark Extern Function Declarations
#endif

extern void _RSStreamCreatePairWithRSSocketSignaturePieces(RSAllocatorRef alloc, SInt32 protocolFamily, SInt32 socketType,
														   SInt32 protocol, RSDataRef address, RSReadStreamRef* readStream,
														   RSWriteStreamRef* writeStream);
extern void _RSSocketStreamCreatePair(RSAllocatorRef alloc, RSStringRef host, UInt32 port, RSSocketNativeHandle s,
									  const RSSocketSignature* sig, RSReadStreamRef* readStream, RSWriteStreamRef* writeStream);
extern RSDataRef _RSHTTPMessageCopySerializedHeaders(RSHTTPMessageRef msg, BOOL forProxy);


#if 0
#pragma mark -
#pragma mark Callback Structs
#pragma mark *RSReadStreamCallBacks
#endif

static const RSReadStreamCallBacks
kSocketReadStreamCallBacks = {
    1,										/* version */
    NULL,									/* create */
    (void (*)(RSReadStreamRef, void*))_SocketStreamFinalize,
    (RSStringRef (*)(RSReadStreamRef, void*))_SocketStreamCopyDescription,
    (BOOL (*)(RSReadStreamRef, RSStreamError*, BOOL*, void*))_SocketStreamOpen,
    (BOOL (*)(RSReadStreamRef, RSStreamError*, void*))_SocketStreamOpenCompleted,
    (RSIndex (*)(RSReadStreamRef, UInt8*, RSIndex, RSStreamError*, BOOL*, void*))_SocketStreamRead,
    NULL,									/* getbuffer */
    (BOOL (*)(RSReadStreamRef, void*))_SocketStreamCanRead,
    (void (*)(RSReadStreamRef, void*))_SocketStreamClose,
    (RSTypeRef (*)(RSReadStreamRef, RSStringRef, void*))_SocketStreamCopyProperty,
    (BOOL (*)(RSReadStreamRef, RSStringRef, RSTypeRef, void*))_SocketStreamSetProperty,
    NULL,									/* requestEvents */
    (void (*)(RSReadStreamRef, RSRunLoopRef, RSStringRef, void*))_SocketStreamSchedule,
    (void (*)(RSReadStreamRef, RSRunLoopRef, RSStringRef, void*))_SocketStreamUnschedule
};

#if 0
#pragma mark *RSWriteStreamCallBacks
#endif

static const RSWriteStreamCallBacks
kSocketWriteStreamCallBacks = {
    1,										/* version */
    NULL,									/* create */
    (void (*)(RSWriteStreamRef, void*))_SocketStreamFinalize,
    (RSStringRef (*)(RSWriteStreamRef, void*))_SocketStreamCopyDescription,
    (BOOL (*)(RSWriteStreamRef, RSStreamError*, BOOL*, void*))_SocketStreamOpen,
    (BOOL (*)(RSWriteStreamRef, RSStreamError*, void*))_SocketStreamOpenCompleted,
    (RSIndex (*)(RSWriteStreamRef, const UInt8*, RSIndex, RSStreamError*, void*))_SocketStreamWrite,
    (BOOL (*)(RSWriteStreamRef, void*))_SocketStreamCanWrite,
    (void (*)(RSWriteStreamRef, void*))_SocketStreamClose,
    (RSTypeRef (*)(RSWriteStreamRef, RSStringRef, void*))_SocketStreamCopyProperty,
    (BOOL (*)(RSWriteStreamRef, RSStringRef, RSTypeRef, void*))_SocketStreamSetProperty,
    NULL,									/* requestEvents */
    (void (*)(RSWriteStreamRef, RSRunLoopRef, RSStringRef, void*))_SocketStreamSchedule,
    (void (*)(RSWriteStreamRef, RSRunLoopRef, RSStringRef, void*))_SocketStreamUnschedule
};


#if 0
#pragma mark -
#pragma mark RSStream Callback Functions
#endif


/* static */ void
_SocketStreamFinalize(RSTypeRef stream, _RSSocketStreamContext* ctxt) {

	/* Make sure the stream is shutdown */
	_SocketStreamClose(stream, ctxt);
	
	/* Lock down the context so it doesn't get touched again */
	RSSpinLockLock(&ctxt->_lock);
	
	/*
	** If the other half is still using the struct, simply
	** mark one half as gone.
	*/
	if (__RSBitIsSet(ctxt->_flags, kFlagBitShared)) {
		
		/* No longer shared by two halves */
		__RSBitClear(ctxt->_flags, kFlagBitShared);
		
		/* Unlock and proceed */
		RSSpinLockUnlock(&ctxt->_lock);
	}
	
	else {
		
		/* Destroy the context now */
		_SocketStreamDestroyContext_NoLock(RSGetAllocator(stream), ctxt);
	}
}


/* static */ RSStringRef
_SocketStreamCopyDescription(RSTypeRef stream, _RSSocketStreamContext* ctxt) {
	
	return RSStringCreateWithFormat(RSGetAllocator(stream),
									NULL,
									kRSSocketStreamDescriptionFormat,
									stream,
									ctxt->_flags,
									ctxt->_clientReadStream,
									ctxt->_clientWriteStream,
									ctxt->_socket,
									ctxt->_properties);
}


/* static */ BOOL
_SocketStreamOpen(RSTypeRef stream, RSStreamError* error, BOOL* openComplete,
				  _RSSocketStreamContext* ctxt)
{
	BOOL result = TRUE;
	RSRunLoopRef rl = NULL;
	
	/* Assume success and no error. */
	memset(error, 0, sizeof(error[0]));
	
	/* Assume that open is not complete; why else would it be here? */
	*openComplete = FALSE;
	
	/* Lock down the context */
	RSSpinLockLock(&ctxt->_lock);

	/* Mark the stream having been called for open. */
	__RSBitSet(ctxt->_flags, (stream == ctxt->_clientReadStream) ? kFlagBitReadStreamOpened : kFlagBitWriteStreamOpened);
	
	/* Workaround the fact that some objects don't signal events when not scheduled permanently. */
	rl = (RSRunLoopRef)RSDictionaryGetValue(ctxt->_properties, _kRSStreamPropertyBogusRunLoop);
	if (!rl) {
		rl = RSRunLoopGetCurrent();
		RSDictionaryAddValue(ctxt->_properties, _kRSStreamPropertyBogusRunLoop, rl);
	}
	
	/* If the open has finished on the other half, mark as such. */
	if (__RSBitIsSet(ctxt->_flags, kFlagBitOpenComplete)) {
		
		/* Copy any error that may have occurred. */
		memmove(error, &ctxt->_error, sizeof(error[0]));
		
		/* Open is done */
		*openComplete = TRUE;
	}
	
	else if (!__RSBitIsSet(ctxt->_flags, kFlagBitOpenStarted)) {
		
		/* Mark as started */
		__RSBitSet(ctxt->_flags, kFlagBitOpenStarted);
		
		/* If there is a carryover error, mark as complete. */
		if (ctxt->_error.error) {
			__RSBitSet(ctxt->_flags, kFlagBitOpenComplete);
			__RSBitClear(ctxt->_flags, kFlagBitPollOpen);
		}
		
		/* If a completed host has already been set, start using it. */
		else if (RSDictionaryGetValue(ctxt->_properties, _kRSStreamPropertyHostForOpen)) {
			_SocketStreamAttemptNextConnection_NoLock(ctxt);
		}

		else if (!_SocketStreamStartLookupForOpen_NoLock(ctxt)) {
			
			/*
			** If no lookup started and no error, everything must be
			** ready for a connect attempt.
			*/
			if (!ctxt->_error.error)
				_SocketStreamAttemptNextConnection_NoLock(ctxt);
		}
		
		/* Did connect actually progress all the way through? */
		if (__RSBitIsSet(ctxt->_flags, kFlagBitOpenComplete)) {
			
			/* Remove the "started" flag */
			__RSBitClear(ctxt->_flags, kFlagBitOpenStarted);
			
			/* Mark as complete */
			*openComplete = TRUE;
		}
		
		/* Copy the error if one occurred. */
		if (ctxt->_error.error)
			memmove(error, &ctxt->_error, sizeof(error[0]));
	}
	
	/* Take care of simple things if there was an error. */
	if (error->error) {
		
		/* Mark the open as being complete */
		__RSBitSet(ctxt->_flags, kFlagBitOpenComplete);

		__RSBitClear(ctxt->_flags, kFlagBitPollOpen);

		/* It's complete now that there's a failure. */
		*openComplete = TRUE;
		
		/* Open failed. */
		result = FALSE;
	}
	
	/* Workaround the fact that some objects don't signal events when not scheduled permanently. */
	if (result && rl)
		_SocketStreamSchedule_NoLock(stream, rl, _kRSStreamSocketBogusPrivateMode, ctxt);
	
	/* Unlock */
	RSSpinLockUnlock(&ctxt->_lock);
	
	return result;
}


/* static */ BOOL
_SocketStreamOpenCompleted(RSTypeRef stream, RSStreamError* error, _RSSocketStreamContext* ctxt) {

	/* Find out if open (polling if necessary). */
	return _SocketStreamCan(ctxt, stream, kFlagBitOpenComplete, _kRSStreamSocketOpenCompletedPrivateMode, error);
}


/* static */ RSIndex
_SocketStreamRead(RSReadStreamRef stream, UInt8* buffer, RSIndex bufferLength,
				  RSStreamError* error, BOOL* atEOF, _RSSocketStreamContext* ctxt)
{
	RSIndex result = 0;
	RSStreamEventType event = kRSStreamEventNone;

	/* Set as no error to start. */
	memset(error, 0, sizeof(error[0]));

	/* Not at end yet. */
	*atEOF = FALSE;
	
	/* Lock down the context */
	RSSpinLockLock(&ctxt->_lock);
	
	while (1) {
		
		/* Wasn't time to read, so run in private mode for timeout or ability to read. */
		if (!ctxt->_error.error && !__RSBitIsSet(ctxt->_flags, kFlagBitCanRead)) {
			
			RSRunLoopRef rl = RSRunLoopGetCurrent();
			RSRunLoopSourceContext src = {0, rl, NULL, NULL, NULL, NULL, NULL, NULL, NULL, _SocketStreamPerformCancel};
			RSRunLoopSourceRef cancel = RSRunLoopSourceCreate(RSGetAllocator(stream), 0, &src);
			
			if (cancel) {
				RSAbsoluteTime later;
				RSTimeInterval interval;
				RSNumberRef value = (RSNumberRef)RSDictionaryGetValue(ctxt->_properties, _kRSStreamPropertyReadTimeout);
				
				RSTypeRef loopAndMode[2] = {rl, _kRSStreamSocketReadPrivateMode};
				
				RSDictionaryAddValue(ctxt->_properties, _kRSStreamPropertyReadCancel, cancel);
				RSRelease(cancel);
				
				if (!value || !RSNumberGetValue(value, kRSNumberDoubleType, &interval))
					interval = kReadWriteTimeoutInterval;
				
				later = (interval == 0.0) ? DBL_MAX : RSAbsoluteTimeGetCurrent() + interval;
				
				/* Add the current loop and the private mode to the list */
				_SchedulesAddRunLoopAndMode(ctxt->_readloops, (RSRunLoopRef)loopAndMode[0], (RSStringRef)loopAndMode[1]);
				
				/* Make sure to schedule all the schedulables on this loop and mode. */
				RSArrayApplyFunction(ctxt->_schedulables,
									 RSRangeMake(0, RSArrayGetCount(ctxt->_schedulables)),
									 (RSArrayApplierFunction)_SchedulablesScheduleApplierFunction,
									 loopAndMode);
				
				RSRunLoopAddSource((RSRunLoopRef)loopAndMode[0], cancel, (RSStringRef)loopAndMode[1]);
				
				__RSBitSet(ctxt->_flags, kFlagBitReadHasCancel);
				
				do {
					/* Unlock the context to allow things to fire */
					RSSpinLockUnlock(&ctxt->_lock);
					
					/* Run the run loop for a poll (0.0) */
					RSRunLoopRunInMode(_kRSStreamSocketReadPrivateMode, interval, TRUE);
					
					/* Lock the context back up. */
					RSSpinLockLock(&ctxt->_lock);
					
				} while (!ctxt->_error.error &&
						 !__RSBitIsSet(ctxt->_flags, kFlagBitCanRead) &&
						 (0 < (interval = (later - RSAbsoluteTimeGetCurrent()))));
				
				__RSBitClear(ctxt->_flags, kFlagBitReadHasCancel);
				
				RSRunLoopRemoveSource((RSRunLoopRef)loopAndMode[0], cancel, (RSStringRef)loopAndMode[1]);
				
				/* Make sure to unschedule all the schedulables on this loop and mode. */
				RSArrayApplyFunction(ctxt->_schedulables,
									 RSRangeMake(0, RSArrayGetCount(ctxt->_schedulables)),
									 (RSArrayApplierFunction)_SchedulablesUnscheduleApplierFunction,
									 loopAndMode);
				
				/* Remove this loop and private mode from the list. */
				_SchedulesRemoveRunLoopAndMode(ctxt->_readloops, (RSRunLoopRef)loopAndMode[0], (RSStringRef)loopAndMode[1]);
				
				RSDictionaryRemoveValue(ctxt->_properties, _kRSStreamPropertyReadCancel);
				
				/* If fell out, still not time, and no error, set to timed out. */
				if (!__RSBitIsSet(ctxt->_flags, kFlagBitCanRead) && !ctxt->_error.error) {				
					ctxt->_error.error = ETIMEDOUT;
					ctxt->_error.domain = kRSStreamErrorDomainPOSIX;
				}
			}
			else {
				ctxt->_error.error = ENOMEM;
				ctxt->_error.domain = kRSStreamErrorDomainPOSIX;
			}
		}
		
		/* Using buffered reads? */
		if (__RSBitIsSet(ctxt->_flags, kFlagBitIsBuffered)) {
			result = _SocketStreamBufferedRead_NoLock(ctxt, buffer, bufferLength);
		}
		
		/* If there's no error, try to read now. */
		else if (!ctxt->_error.error) {
			result = _RSSocketRecv(ctxt->_socket, buffer, bufferLength, &ctxt->_error);
		}
		
		/* Did a read, so the event is no longer good. */
		__RSBitClear(ctxt->_flags, kFlagBitCanRead);
		
		/* Got a "would block" error, so clear it and wait for time to read. */
		if ((ctxt->_error.error == EAGAIN) && (ctxt->_error.domain == _kRSStreamErrorDomainNativeSockets)) {
			memset(&ctxt->_error, 0, sizeof(ctxt->_error));
			continue;
		}
					
		break;
	}
	
	/*
	** 3863115 The following processing may seem backwards, but it is intentional.
	** The idea is to allow processing of buffered bytes even if there is an error
	** sitting on the stream's context.
	*/
	
	/* Make sure to set things up correctly as a result of success. */
	if (result > 0) {

		/* If handshakes are in play, don't even attempt further checks. */
		if (!__RSBitIsSet(ctxt->_flags, kFlagBitHasHandshakes)) {

			/* Right now only SSL is using the buffered reads. */
			if (__RSBitIsSet(ctxt->_flags, kFlagBitIsBuffered)) {
				
				/* Attempt to get the count of buffered bytes. */
				RSDataRef c = (RSDataRef)RSDictionaryGetValue(ctxt->_properties, _kRSStreamPropertyRecvBufferCount);
										
				/* Are there bytes or has SSL closed the connection? */
				if (__RSBitIsSet(ctxt->_flags, kFlagBitClosed) || (c && *((RSIndex*)RSDataGetBytePtr(c)))) {
					__RSBitSet(ctxt->_flags, kFlagBitCanRead);
					__RSBitClear(ctxt->_flags, kFlagBitPollRead);
					event = kRSStreamEventHasBytesAvailable;
				}			
			}
			
			/* Can still read?  If not, re-enable. */
			else if (!_RSSocketCan(ctxt->_socket, kSelectModeRead))
				RSSocketEnableCallBacks(ctxt->_socket, kRSSocketReadCallBack);
			
			/* Still can read, so signal the "has bytes" now. */
			else {
				event = kRSStreamEventHasBytesAvailable;
				__RSBitSet(ctxt->_flags, kFlagBitCanRead);
				__RSBitClear(ctxt->_flags, kFlagBitPollRead);
			}
		}
		
		else {
			RSMutableArrayRef handshakes = (RSMutableArrayRef)RSDictionaryGetValue(ctxt->_properties, _kRSStreamPropertyHandshakes);
			
			if (handshakes && 
				RSArrayGetCount(handshakes) &&
				(_PerformCONNECTHaltHandshake_NoLock == RSArrayGetValueAtIndex(handshakes, 0)))
			{
				/* Can still read?  If not, re-enable. */
				if (!_RSSocketCan(ctxt->_socket, kSelectModeRead))
					RSSocketEnableCallBacks(ctxt->_socket, kRSSocketReadCallBack);
				
				/* Still can read, so signal the "has bytes" now. */
				else {
					event = kRSStreamEventHasBytesAvailable;
					__RSBitSet(ctxt->_flags, kFlagBitCanRead);
					__RSBitClear(ctxt->_flags, kFlagBitPollRead);
				}
			}
		}
	}
	
	/* If there was an error, make sure to propagate and signal. */
	else if (ctxt->_error.error) {
		
		/* Copy the error for return */
		memmove(error, &ctxt->_error, sizeof(error[0]));
		
		/* If there is a client stream and it's been opened, signal the error. */
		if (ctxt->_clientWriteStream && __RSBitIsSet(ctxt->_flags, kFlagBitWriteStreamOpened))
			_RSWriteStreamSignalEventDelayed(ctxt->_clientWriteStream, kRSStreamEventErrorOccurred, error);
		
		/* Mark as done and error the result. */
		*atEOF = TRUE;
		result = -1;
            
		/* Make sure the socket doesn't signal anymore. */
		RSSocketDisableCallBacks(ctxt->_socket, kRSSocketReadCallBack | kRSSocketWriteCallBack);
	}
	
	/* A read of zero is EOF. */
	else if (!result) {
	
		*atEOF = TRUE;
		
		/* Make sure the socket doesn't signal anymore. */
		RSSocketDisableCallBacks(ctxt->_socket, kRSSocketReadCallBack);
	}
	
	/* Unlock */
	RSSpinLockUnlock(&ctxt->_lock);
	
	if (event != kRSStreamEventNone)
        RSReadStreamSignalEvent(stream, event, NULL);
	
	return result;
}


/* static */ BOOL
_SocketStreamCanRead(RSReadStreamRef stream, _RSSocketStreamContext* ctxt) {
	
	RSStreamError error;
	BOOL result = FALSE;
	
	/* Lock down the context */
	RSSpinLockLock(&ctxt->_lock);
	
	/* Right now only SSL is using the buffered reads. */
	if (!__RSBitIsSet(ctxt->_flags, kFlagBitHasHandshakes) && __RSBitIsSet(ctxt->_flags, kFlagBitIsBuffered)) {
		
		/* Similar to the end of _SocketStreamRead. */
		RSDataRef c = (RSDataRef)RSDictionaryGetValue(ctxt->_properties, _kRSStreamPropertyRecvBufferCount);
		
		/* Need to check for buffered bytes or EOF. */
		if (__RSBitIsSet(ctxt->_flags, kFlagBitClosed) || (c && *((RSIndex*)RSDataGetBytePtr(c)))) {
			
			result = TRUE;
						
			/* Unlock */
			RSSpinLockUnlock(&ctxt->_lock);
		}
		
		/* If none there, check to see if there are encrypted bytes that are buffered. */
		else if (__RSBitIsSet(ctxt->_flags, kFlagBitUseSSL)) {
			
			_SocketStreamSecurityBufferedRead_NoLock(ctxt);
			result = __RSBitIsSet(ctxt->_flags, kFlagBitCanRead) || __RSBitIsSet(ctxt->_flags, kFlagBitClosed);
			
			/* Unlock */
			RSSpinLockUnlock(&ctxt->_lock);
		}
		
		else {
			
			/* Unlock */
			RSSpinLockUnlock(&ctxt->_lock);
		
			/* Find out if can read (polling if necessary). */
			result = _SocketStreamCan(ctxt, stream, kFlagBitCanRead, _kRSStreamSocketCanReadPrivateMode, &error);
		}
	}
	
	else {
	
		/* Unlock */
		RSSpinLockUnlock(&ctxt->_lock);
			
		/* Find out if can read (polling if necessary). */
		result = _SocketStreamCan(ctxt, stream, kFlagBitCanRead, _kRSStreamSocketCanReadPrivateMode, &error);
	}
	
	return result;
}


/* static */ RSIndex
_SocketStreamWrite(RSWriteStreamRef stream, const UInt8* buffer, RSIndex bufferLength,
				   RSStreamError* error, _RSSocketStreamContext* ctxt)
{
	RSIndex result = 0;
	RSStreamEventType event = kRSStreamEventNone;
	
	/* Set as no error to start. */
	memset(error, 0, sizeof(error[0]));
	
	/* Lock down the context */
	RSSpinLockLock(&ctxt->_lock);
	
	while (1) {			
		/* Wasn't time to write, so run in private mode for timeout or ability to write. */
		if (!ctxt->_error.error && !__RSBitIsSet(ctxt->_flags, kFlagBitCanWrite)) {
			
			RSRunLoopRef rl = RSRunLoopGetCurrent();
			RSRunLoopSourceContext src = {0, rl, NULL, NULL, NULL, NULL, NULL, NULL, NULL, _SocketStreamPerformCancel};
			RSRunLoopSourceRef cancel = RSRunLoopSourceCreate(RSGetAllocator(stream), 0, &src);
			
			if (cancel) {
					
				RSAbsoluteTime later;
				RSTimeInterval interval;
				RSNumberRef value = (RSNumberRef)RSDictionaryGetValue(ctxt->_properties, _kRSStreamPropertyWriteTimeout);
				
				RSTypeRef loopAndMode[2] = {rl, _kRSStreamSocketWritePrivateMode};
				
				RSDictionaryAddValue(ctxt->_properties, _kRSStreamPropertyWriteCancel, cancel);
				RSRelease(cancel);
				
				if (!value || !RSNumberGetValue(value, kRSNumberDoubleType, &interval))
					interval = kReadWriteTimeoutInterval;
				
				later = (interval == 0.0) ? DBL_MAX : RSAbsoluteTimeGetCurrent() + interval;
				
				/* Add the current loop and the private mode to the list */
				_SchedulesAddRunLoopAndMode(ctxt->_writeloops, (RSRunLoopRef)loopAndMode[0], (RSStringRef)loopAndMode[1]);
				
				__RSBitSet(ctxt->_flags, kFlagBitWriteHasCancel);
				
				/* Make sure to schedule all the schedulables on this loop and mode. */
				RSArrayApplyFunction(ctxt->_schedulables,
									 RSRangeMake(0, RSArrayGetCount(ctxt->_schedulables)),
									 (RSArrayApplierFunction)_SchedulablesScheduleApplierFunction,
									 loopAndMode);
				
				RSRunLoopAddSource((RSRunLoopRef)loopAndMode[0], cancel, (RSStringRef)loopAndMode[1]);
				
				do {
					/* Unlock the context to allow things to fire */
					RSSpinLockUnlock(&ctxt->_lock);
					
					/* Run the run loop for a poll (0.0) */
					RSRunLoopRunInMode(_kRSStreamSocketWritePrivateMode, interval, FALSE);
					
					/* Lock the context back up. */
					RSSpinLockLock(&ctxt->_lock);
					
				} while (!ctxt->_error.error &&
						 !__RSBitIsSet(ctxt->_flags, kFlagBitCanWrite) &&
						 (0 < (interval = (later - RSAbsoluteTimeGetCurrent()))));
				
				__RSBitClear(ctxt->_flags, kFlagBitWriteHasCancel);
				
				RSRunLoopRemoveSource((RSRunLoopRef)loopAndMode[0], cancel, (RSStringRef)loopAndMode[1]);
				
				/* Make sure to unschedule all the schedulables on this loop and mode. */
				RSArrayApplyFunction(ctxt->_schedulables,
									 RSRangeMake(0, RSArrayGetCount(ctxt->_schedulables)),
									 (RSArrayApplierFunction)_SchedulablesUnscheduleApplierFunction,
									 loopAndMode);
				
				/* Remove this loop and private mode from the list. */
				_SchedulesRemoveRunLoopAndMode(ctxt->_writeloops, (RSRunLoopRef)loopAndMode[0], (RSStringRef)loopAndMode[1]);
				
				RSDictionaryRemoveValue(ctxt->_properties, _kRSStreamPropertyWriteCancel);
				
				/* If fell out, still not time, and no error, set to timed out. */
				if (!__RSBitIsSet(ctxt->_flags, kFlagBitCanWrite) && !ctxt->_error.error) {				
					ctxt->_error.error = ETIMEDOUT;
					ctxt->_error.domain = kRSStreamErrorDomainPOSIX;
				}
			}
			else {
				ctxt->_error.error = ENOMEM;
				ctxt->_error.domain = kRSStreamErrorDomainPOSIX;
			}
		}
		
		/* If there's no error, try to write now. */
		if (!ctxt->_error.error) {
		
			if (__RSBitIsSet(ctxt->_flags, kFlagBitUseSSL))
				result = _SocketStreamSecuritySend_NoLock(ctxt, buffer, bufferLength);
			else
				result = _RSSocketSend(ctxt->_socket, buffer, bufferLength, &ctxt->_error);
		}
		
		/* Did a write, so the event is no longer good. */
		__RSBitClear(ctxt->_flags, kFlagBitCanWrite);
		
		/* Got a "would block" error, so clear it and wait for time to write. */
		if ((ctxt->_error.error == EAGAIN) && (ctxt->_error.domain == _kRSStreamErrorDomainNativeSockets)) {
			memset(&ctxt->_error, 0, sizeof(ctxt->_error));
			continue;
		}

		break;
	}
		
	/* If there was an error, make sure to propagate and signal. */
	if (ctxt->_error.error) {
	
		RSDataRef c = (RSDataRef)RSDictionaryGetValue(ctxt->_properties, _kRSStreamPropertyRecvBufferCount);
		
		/* 3863115 Only signal the read error if it's not buffered and no bytes waiting. */
		if (!c || !*((RSIndex*)RSDataGetBytePtr(c))) {
			
			/* Copy the error for return */
			memmove(error, &ctxt->_error, sizeof(error[0]));
			
			/* If there is a client stream for the other half and it's been opened, signal the error. */
			if (ctxt->_clientReadStream && __RSBitIsSet(ctxt->_flags, kFlagBitReadStreamOpened))
				_RSReadStreamSignalEventDelayed(ctxt->_clientReadStream, kRSStreamEventErrorOccurred, error);
		}
		
		/* Mark as done and error the result. */
		result = -1;
            
		/* Make sure the socket doesn't signal anymore. */
		RSSocketDisableCallBacks(ctxt->_socket, kRSSocketWriteCallBack | kRSSocketReadCallBack);
	}
	
	/* A write of zero is EOF. */
	else if (!result) {

		/* Make sure the socket doesn't signal anymore. */
		RSSocketDisableCallBacks(ctxt->_socket, kRSSocketWriteCallBack);
	}
		
	/* Make sure to set things up correctly as a result of success. */
	else {
		
		/* If handshakes are in progress, don't perform further checks. */
		if (!__RSBitIsSet(ctxt->_flags, kFlagBitHasHandshakes)) {
			
			/* If the end, signal EOF. */
			if (__RSBitIsSet(ctxt->_flags, kFlagBitClosed)) {
				event = kRSStreamEventEndEncountered;
				__RSBitSet(ctxt->_flags, kFlagBitCanWrite);
				__RSBitClear(ctxt->_flags, kFlagBitPollWrite);
			}
			
			/* If can't write then enable RSSocket to tell when. */
			else if (!_RSSocketCan(ctxt->_socket, kSelectModeWrite))
				RSSocketEnableCallBacks(ctxt->_socket, kRSSocketWriteCallBack);
			
			/* Can still write so signal right away. */
			else {
				event = kRSStreamEventCanAcceptBytes;
				__RSBitSet(ctxt->_flags, kFlagBitCanWrite);
				__RSBitClear(ctxt->_flags, kFlagBitPollWrite);
			}
		}
	}
	
	/* Unlock */
	RSSpinLockUnlock(&ctxt->_lock);
	
	if (event != kRSStreamEventNone)
        RSWriteStreamSignalEvent(stream, event, NULL);
	
	return result;
}


/* static */ BOOL
_SocketStreamCanWrite(RSWriteStreamRef stream, _RSSocketStreamContext* ctxt) {
	
	RSStreamError error;
	
	/* Find out if can write (polling if necessary). */
	return _SocketStreamCan(ctxt, stream, kFlagBitCanWrite, _kRSStreamSocketCanWritePrivateMode, &error);
}


/* static */ void
_SocketStreamClose(RSTypeRef stream, _RSSocketStreamContext* ctxt) {
	
	RSMutableArrayRef loops, otherloops;
	RSIndex count;
	RSRunLoopRef rl;
	
	/* Lock down the context */
	RSSpinLockLock(&ctxt->_lock);
	
	/* Workaround the fact that some objects don't signal events when not scheduled permanently. */
	rl = (RSRunLoopRef)RSDictionaryGetValue(ctxt->_properties, _kRSStreamPropertyBogusRunLoop);
	if (rl) {
		_SocketStreamUnschedule_NoLock(stream, rl, _kRSStreamSocketBogusPrivateMode, ctxt);
	}
	
	/* Figure out the proper list of schedules on which to operate. */
	if (RSGetTypeID(stream) == RSReadStreamGetTypeID()) {
		ctxt->_clientReadStream = NULL;
		loops = ctxt->_readloops;
		otherloops = ctxt->_writeloops;
	}
	else {
		ctxt->_clientWriteStream = NULL;
		loops = ctxt->_writeloops;
		otherloops = ctxt->_readloops;
	}
	
	/* Unschedule the items that are scheduled only for this half. */
	RSArrayApplyFunction(ctxt->_schedulables,
						 RSRangeMake(0, RSArrayGetCount(ctxt->_schedulables)),
						 (RSArrayApplierFunction)_SchedulablesUnscheduleFromAllApplierFunction,
						 loops);
	
	/* Remove the list of schedules from this half. */
	RSArrayRemoveAllValues(loops);

	/* Move the list of shared loops and modes over to the other half. */
	if ((count = RSArrayGetCount(ctxt->_sharedloops))) {
	
		/* Move the shared list to the other half. */
		RSArrayAppendArray(otherloops, ctxt->_sharedloops, RSRangeMake(0, count));
		
		/* Dump the shared list. */
		RSArrayRemoveAllValues(ctxt->_sharedloops);
	}
	
	if (!ctxt->_clientReadStream && !ctxt->_clientWriteStream) {
		
		RSRange r;
		
		if (RSDictionaryGetValue(ctxt->_properties, kRSStreamPropertySocketSSLContext))
			_SocketStreamSecurityClose_NoLock(ctxt);
		
		r = RSRangeMake(0, RSArrayGetCount(ctxt->_schedulables));
		
		/* Unschedule the items that are scheduled.  This shouldn't happen. */
		RSArrayApplyFunction(ctxt->_schedulables, r, (RSArrayApplierFunction)_SchedulablesUnscheduleFromAllApplierFunction, otherloops);
		
		/* Dump the final list of schedules. */
		RSArrayRemoveAllValues(otherloops);

		/* Make sure to invalidate all the schedulables. */
		RSArrayApplyFunction(ctxt->_schedulables, r, (RSArrayApplierFunction)_SchedulablesInvalidateApplierFunction, NULL);
		
		/* Unscheduled and invalidated, so let them go. */
		RSArrayRemoveAllValues(ctxt->_schedulables);
		
		/* Take care of the socket if there is one. */
		if (ctxt->_socket) {
			
			/* Make sure to invalidate the socket */
			RSSocketInvalidate(ctxt->_socket);
		
			/* Dump and forget it. */
			RSRelease(ctxt->_socket);
			ctxt->_socket = NULL;
		}
		
		/* No more schedules/unschedules, so get rid of this workaround. */
		RSDictionaryRemoveValue(ctxt->_properties, _kRSStreamPropertyBogusRunLoop);
	}
		
	/* Unlock */
	RSSpinLockUnlock(&ctxt->_lock);
}


/* static */ RSTypeRef
_SocketStreamCopyProperty(RSTypeRef stream, RSStringRef propertyName, _RSSocketStreamContext* ctxt) {
	
	RSTypeRef result = NULL;
	RSTypeRef property;
	
	/* Lock down the context */
	RSSpinLockLock(&ctxt->_lock);
	
	/* Try to just get the property from the dictionary */
	property = RSDictionaryGetValue(ctxt->_properties, propertyName);
	
	/* Must be some other type that takes a little more work to produce. */
	if (!property) {
		
		/* Client wants the far end host's name. */
		if (RSEqual(kRSStreamPropertySocketRemoteHostName, propertyName)) {
			
			/* Attempt to get the RSHostRef used for connecting. */
			RSTypeRef host = RSDictionaryGetValue(ctxt->_properties, kRSStreamPropertySocketRemoteHost);
			
			/* If got the host, need to go for the name. */
			if (host) {
				
				/* Get the list of names. */
				RSArrayRef list = RSHostGetNames((RSHostRef)host, NULL);
				
				/* If it has names, pull the first. */
				if (list && RSArrayGetCount(list))
					property = RSArrayGetValueAtIndex(list, 0);
			}
			
			/* Not a RSHostRef, so go for the RSNetService instead. */
			else {
			
				host = RSDictionaryGetValue(ctxt->_properties, kRSStreamPropertySocketRemoteNetService);
				
				/* RSNetService's have a target instead. */
				if (host)
					property = RSNetServiceGetTargetHost((RSNetServiceRef)host);
			}
		}

		/* Client wants the native socket, but make sure there is one first. */
		else if (RSEqual(kRSStreamPropertySocketNativeHandle, propertyName) && ctxt->_socket) {
			
			RSSocketNativeHandle s = RSSocketGetNative(ctxt->_socket);
			
			/* Create the return value */
			result = RSDataCreate(RSGetAllocator(stream), (const void*)(&s), sizeof(s));
		}
		
		/* Support for legacy ordering.  Response was available right away. */
		else if (RSEqual(kRSStreamPropertyCONNECTResponse, propertyName)) {

			if (RSDictionaryGetValue(ctxt->_properties, kRSStreamPropertyCONNECTProxy))
				result = RSHTTPMessageCreateEmpty(RSGetAllocator(stream), FALSE); 
		}
		
		else if (RSEqual(kRSStreamPropertySSLPeerCertificates, propertyName)) {
			
			RSDataRef wrapper = (RSDataRef)RSDictionaryGetValue(ctxt->_properties, kRSStreamPropertySocketSSLContext);
			if (wrapper) {
				if (SSLGetPeerCertificates(*((SSLContextRef*)RSDataGetBytePtr(wrapper)), (RSArrayRef*)&result) && result) {
					RSRelease(result);
					result = NULL;
				}
			}
		}

		else if (RSEqual(_kRSStreamPropertySSLClientCertificates, propertyName)) {
			
			RSDataRef wrapper = (RSDataRef)RSDictionaryGetValue(ctxt->_properties, kRSStreamPropertySocketSSLContext);
			if (wrapper) {
				if (SSLGetCertificate(*((SSLContextRef*)RSDataGetBytePtr(wrapper)), (RSArrayRef*)&result) && result) {
					// note: result of SSLGetCertificate is not retained
					result = NULL;
				} else if (result) {
					RSRetain(result);
				}
			}
		}

		else if (RSEqual(_kRSStreamPropertySSLClientCertificateState, propertyName)) {
			
			RSDataRef wrapper = (RSDataRef)RSDictionaryGetValue(ctxt->_properties, kRSStreamPropertySocketSSLContext);
			if (wrapper) {
				SSLClientCertificateState clientState = kSSLClientCertNone;
				if (SSLGetClientCertificateState(*((SSLContextRef*)RSDataGetBytePtr(wrapper)), &clientState)) {
					result = NULL;
				} else {
					result = RSNumberCreate(RSGetAllocator(ctxt->_properties), kRSNumberIntType, &clientState);
				}
			}
		}

	}
	
    if (RSEqual(propertyName, kRSStreamPropertySocketSecurityLevel)) {
		
		RSDataRef wrapper = (RSDataRef)RSDictionaryGetValue(ctxt->_properties, kRSStreamPropertySocketSSLContext);
		
		if (wrapper)
			property = _SecurityGetProtocol(*((SSLContextRef*)RSDataGetBytePtr(wrapper)));
    }
	
	/* Do whatever is needed to "copy" the type if found. */
	if (property) {
		
		RSTypeID type = RSGetTypeID(property);
		
		/* Create copies of host, services, dictionaries, arrays, and http messages. */
		if (RSHostGetTypeID() == type)
			result = RSHostCreateCopy(RSGetAllocator(stream), (RSHostRef)property);
		
		else if (RSNetServiceGetTypeID() == type)
			result = RSNetServiceCreateCopy(RSGetAllocator(stream), (RSNetServiceRef)property);
		
		else if (RSDictionaryGetTypeID() == type)
			result = RSDictionaryCreateCopy(RSGetAllocator(stream), (RSDictionaryRef)property);
		
		else if (RSArrayGetTypeID() == type)
			result = RSArrayCreateCopy(RSGetAllocator(stream), (RSArrayRef)property);
		
		else if (RSHTTPMessageGetTypeID() == type)
			result = RSHTTPMessageCreateCopy(RSGetAllocator(stream), (RSHTTPMessageRef)property);
		
		/* All other types are just retained. */
		else 
			result = RSRetain(property);
	}
	
	/* Unlock */
	RSSpinLockUnlock(&ctxt->_lock);
	
	return result;
}


/* static */ BOOL
_SocketStreamSetProperty(RSTypeRef stream, RSStringRef propertyName, RSTypeRef propertyValue,
						 _RSSocketStreamContext* ctxt)
{   
	BOOL result = FALSE;
	
	/* Lock down the context */
	RSSpinLockLock(&ctxt->_lock);
	
	if (RSEqual(propertyName, kRSStreamPropertyUseAddressCache) ||
		RSEqual(propertyName, _kRSStreamSocketIChatWantsSubNet))
	{
		
		if (propertyValue)
			RSDictionarySetValue(ctxt->_properties, propertyName, propertyValue);
		else
			RSDictionaryRemoveValue(ctxt->_properties, propertyName);
		
		result = TRUE;
	}
	
	else if (RSEqual(propertyName, kRSStreamPropertyAutoErrorOnSystemChange)) {
		
		if (!propertyValue) {
			
			RSDictionaryRemoveValue(ctxt->_properties, propertyName);
			
			_SocketStreamAddReachability_NoLock(ctxt);
		}
		
		else {
			RSDictionarySetValue(ctxt->_properties, propertyName, propertyValue);
			
			if (RSEqual(propertyValue, kRSBOOLFalse))
				_SocketStreamRemoveReachability_NoLock(ctxt);
			else
				_SocketStreamAddReachability_NoLock(ctxt);
		}
		
		result = TRUE;
	}
	
    else if (RSEqual(propertyName, _kRSStreamSocketCreatedCallBack)) {
		
		if (!propertyValue)
			RSDictionaryRemoveValue(ctxt->_properties, propertyName);
			
		else {
		
			RSArrayRef old = (RSArrayRef)RSDictionaryGetValue(ctxt->_properties, propertyName);
			
			if (!old || !RSEqual(old, propertyValue))
				RSDictionarySetValue(ctxt->_properties, propertyName, propertyValue);
		}
		
        result = TRUE;
    }

	else if (RSEqual(propertyName, kRSStreamPropertyShouldCloseNativeSocket)) {
		
		if (propertyValue)
			RSDictionarySetValue(ctxt->_properties, propertyName, propertyValue);
		else
			RSDictionaryRemoveValue(ctxt->_properties, propertyName);
		
		if (ctxt->_socket) {
			
			RSOptionFlags flags = RSSocketGetSocketFlags(ctxt->_socket);
			
			if (!propertyValue) {
				
				if (__RSBitIsSet(ctxt->_flags, kFlagBitCreatedNative))
					flags &= ~kRSSocketCloseOnInvalidate;
				else
					flags |= kRSSocketCloseOnInvalidate;
			}
			
			else if (propertyValue != kRSBOOLFalse)
				flags |= kRSSocketCloseOnInvalidate;
			
			else
				flags &= ~kRSSocketCloseOnInvalidate;
			
			RSSocketSetSocketFlags(ctxt->_socket, flags);			
		}
			
		result = TRUE;
	}
	
	else if (RSEqual(propertyName, kRSStreamPropertyCONNECTProxy))
		result = _CONNECTSetInfo_NoLock(ctxt, propertyValue);
	
	else if (RSEqual(propertyName, kRSStreamPropertySocketSSLContext))
		result = _SocketStreamSecuritySetContext_NoLock(ctxt, propertyValue);
	
    else if (RSEqual(propertyName, kRSStreamPropertySSLSettings)) {

        result = _SocketStreamSecuritySetInfo_NoLock(ctxt, propertyValue);
		
		if (result) {
			
			if (propertyValue)
				RSDictionarySetValue(ctxt->_properties, kRSStreamPropertySSLSettings, propertyValue);
			else
				RSDictionaryRemoveValue(ctxt->_properties, kRSStreamPropertySSLSettings);
		}
    }
	
	else if (RSEqual(propertyName, _kRSStreamPropertySocketSecurityAuthenticatesServerCertificate)) {
		
		result = TRUE;
		
		if (RSDictionaryGetValue(ctxt->_properties, kRSStreamPropertySocketSSLContext) &&
			(_SocketStreamSecurityGetSessionState_NoLock(ctxt) == kSSLIdle))
		{
			result = _SocketStreamSecuritySetAuthenticatesServerCertificates_NoLock(ctxt, propertyValue ? propertyValue : kRSBOOLTrue);
		}
        
		if (result) {
			if (propertyValue)
				RSDictionarySetValue(ctxt->_properties, _kRSStreamPropertySocketSecurityAuthenticatesServerCertificate, propertyValue);
			else
				RSDictionaryRemoveValue(ctxt->_properties, _kRSStreamPropertySocketSecurityAuthenticatesServerCertificate);
		}
    }
	
    else if (RSEqual(propertyName, kRSStreamPropertySocketSecurityLevel)) {
		
        RSMutableDictionaryRef settings = RSDictionaryCreateMutable(RSGetAllocator(ctxt->_properties),
																	0,
																	&kRSTypeDictionaryKeyCallBacks,
																	&kRSTypeDictionaryValueCallBacks);
        
		if (settings) {
            RSDictionaryAddValue(settings, kRSStreamSSLLevel, propertyValue);
            result = _SocketStreamSecuritySetInfo_NoLock(ctxt, settings);
            RSRelease(settings);
			
			if (result) {
				if (propertyValue)
					RSDictionarySetValue(ctxt->_properties, kRSStreamPropertySocketSecurityLevel, propertyValue);
				else
					RSDictionaryRemoveValue(ctxt->_properties, kRSStreamPropertySocketSecurityLevel);
			}
		}
    }
	
	else if (RSEqual(propertyName, _kRSStreamPropertySocketPeerName)) {
		
		if (propertyValue)
			RSDictionarySetValue(ctxt->_properties, _kRSStreamPropertySocketPeerName, propertyValue);
		else
			RSDictionaryRemoveValue(ctxt->_properties, kRSStreamPropertySSLSettings);
		
		result = TRUE;
	}
	
    else if (RSEqual(propertyName, kRSStreamPropertySOCKSProxy))
		result = _SOCKSSetInfo_NoLock(ctxt, (RSDictionaryRef)propertyValue);
	
	else if (RSEqual(propertyName, _kRSStreamPropertyHostForOpen) ||
			 RSEqual(propertyName, _kRSStreamPropertyReadTimeout) ||
			 RSEqual(propertyName, _kRSStreamPropertyWriteTimeout) ||
			 RSEqual(propertyName, _kRSStreamPropertyAutoConnectPriority) ||
			 RSEqual(propertyName, _kRSStreamPropertySSLAllowAnonymousCiphers))
	{
		
		if (propertyValue)
			RSDictionarySetValue(ctxt->_properties, propertyName, propertyValue);
		else
			RSDictionaryRemoveValue(ctxt->_properties, propertyName);
		
		result = TRUE;
	}
	
	else if (RSEqual(propertyName, _kRSStreamPropertyRecvBufferSize) &&
			 !__RSBitIsSet(ctxt->_flags, kFlagBitOpenStarted) &&
			 !__RSBitIsSet(ctxt->_flags, kFlagBitOpenComplete))
	{
		if (!propertyValue) {
			RSDictionaryRemoveValue(ctxt->_properties, propertyName);
			__RSBitClear(ctxt->_flags, kFlagBitIsBuffered);
		}
		else if (RSNumberGetByteSize(propertyValue) == sizeof(RSIndex)) {
			RSDictionarySetValue(ctxt->_properties, propertyName, propertyValue);
			__RSBitSet(ctxt->_flags, kFlagBitIsBuffered);
		}
			
		result = TRUE;
	}
	
	/* 3800596 Need to signal errors if setting property caused one. */
	if (ctxt->_error.error) {

		/* Attempt to get the count of buffered bytes. */
		RSDataRef c = (RSDataRef)RSDictionaryGetValue(ctxt->_properties, _kRSStreamPropertyRecvBufferCount);
		
		/*
		** 3863115 If there is a client stream and it's been opened, signal
		** the error, but only if there is no bytes sitting in the buffer.
		*/
		if ((!c || !*((RSIndex*)RSDataGetBytePtr(c))) &&
			(ctxt->_clientReadStream && __RSBitIsSet(ctxt->_flags, kFlagBitReadStreamOpened)))
		{
			_RSReadStreamSignalEventDelayed(ctxt->_clientReadStream, kRSStreamEventErrorOccurred, &ctxt->_error);
		}
		
		/* If there is a client stream and it's been opened, signal the error. */
		if (ctxt->_clientWriteStream && __RSBitIsSet(ctxt->_flags, kFlagBitWriteStreamOpened))
			_RSWriteStreamSignalEventDelayed(ctxt->_clientWriteStream, kRSStreamEventErrorOccurred, &ctxt->_error);
	}
	
	/* Unlock */
	RSSpinLockUnlock(&ctxt->_lock);
	
	return result;
}


/* static */ void
_SocketStreamSchedule(RSTypeRef stream, RSRunLoopRef runLoop, RSStringRef runLoopMode,
					  _RSSocketStreamContext* ctxt)
{
	/* Lock down the context */
	RSSpinLockLock(&ctxt->_lock);
	
	/* Now do the actual work */
	_SocketStreamSchedule_NoLock(stream, runLoop, runLoopMode, ctxt);
	
	/* Unlock */
	RSSpinLockUnlock(&ctxt->_lock);
}


/* static */ void
_SocketStreamUnschedule(RSTypeRef stream, RSRunLoopRef runLoop, RSStringRef runLoopMode,
						_RSSocketStreamContext* ctxt)
{
	/* Lock down the context */
	RSSpinLockLock(&ctxt->_lock);
	
	/* Now do the actual work */
	_SocketStreamUnschedule_NoLock(stream, runLoop, runLoopMode, ctxt);
	
	/* Unlock */
	RSSpinLockUnlock(&ctxt->_lock);
}



#if 0
#pragma mark -
#pragma mark Utility Functions
#endif

/* static */ void
_SocketCallBack(RSSocketRef s, RSSocketCallBackType type, RSDataRef address, const void* data, _RSSocketStreamContext* ctxt) {

	RSReadStreamRef rStream = NULL;
	RSWriteStreamRef wStream = NULL;
	RSStreamEventType event = kRSStreamEventNone;
	RSStreamError error = {0, 0};
	
	RSSpinLockLock(&ctxt->_lock);

	if (!ctxt->_error.error) {

		switch (type) {
			
			case kRSSocketConnectCallBack:
				
				if (!data) {
					
					/* See if the client has turned off the error detection. */
					RSBOOLRef reach = (RSBOOLRef)RSDictionaryGetValue(ctxt->_properties, kRSStreamPropertyAutoErrorOnSystemChange);
				
					/* Mark as open. */
					__RSBitClear(ctxt->_flags, kFlagBitOpenStarted);
					__RSBitSet(ctxt->_flags, kFlagBitOpenComplete);
					__RSBitClear(ctxt->_flags, kFlagBitPollOpen);
					
					/* Get the streams and event to signal. */
					event = kRSStreamEventOpenCompleted;
					
					rStream = ctxt->_clientReadStream;
					wStream = ctxt->_clientWriteStream;
					
					/* Create and schedule reachability on this socket. */
					if (!reach || (reach != kRSBOOLFalse))
						_SocketStreamAddReachability_NoLock(ctxt);
				}
				
				else {
					int i;
					RSArrayRef loops[3] = {ctxt->_readloops, ctxt->_writeloops, ctxt->_sharedloops};

					ctxt->_error.error = *((SInt32 *)data);
					ctxt->_error.domain = _kRSStreamErrorDomainNativeSockets;
					
					/* Remove the socket from the schedulables. */
					_SchedulablesRemove(ctxt->_schedulables, s);
					
					/* Unschedule the socket from all loops and modes */
					for (i = 0; i < (sizeof(loops) / sizeof(loops[0])); i++)
						_RSTypeUnscheduleFromMultipleRunLoops(s, loops[i]);
					
					/* Invalidate the socket; never to be used again. */
					_RSTypeInvalidate(s);
					
					/* Release and forget the socket */
					RSRelease(s);
					ctxt->_socket = NULL;
					
					/* Try to start the next connection. */
					if (_SocketStreamAttemptNextConnection_NoLock(ctxt)) {
					
						/* Start fresh with no error again. */
						memset(&ctxt->_error, 0, sizeof(ctxt->_error));
					}
				}
							
				break;
			
			case kRSSocketReadCallBack:
				
				/* If handshakes in place, pump those along. */
				if (__RSBitIsSet(ctxt->_flags, kFlagBitHasHandshakes)) {
					RSArrayRef handshakes = (RSArrayRef)RSDictionaryGetValue(ctxt->_properties, _kRSStreamPropertyHandshakes);
					((_RSSocketStreamPerformHandshakeCallBack)RSArrayGetValueAtIndex(handshakes, 0))(ctxt);	
				}
				
				/* Buffered reading has special code. */
				else if (__RSBitIsSet(ctxt->_flags, kFlagBitIsBuffered)) {
					
					/* Call the buffered read stuff for SSL */
					if (__RSBitIsSet(ctxt->_flags, kFlagBitUseSSL))
						_SocketStreamSecurityBufferedRead_NoLock(ctxt);
					else
						_SocketStreamBufferedSocketRead_NoLock(ctxt);
					
					/* If that set the "can read" bit, set the event. */
					if (__RSBitIsSet(ctxt->_flags, kFlagBitCanRead)) {
						 event = kRSStreamEventHasBytesAvailable;
						 rStream = ctxt->_clientReadStream;
					}
				}
				
				else {
					__RSBitSet(ctxt->_flags, kFlagBitCanRead);
					__RSBitClear(ctxt->_flags, kFlagBitPollRead);
					event = kRSStreamEventHasBytesAvailable;
					rStream = ctxt->_clientReadStream;
				}
				
				break;
				
			case kRSSocketWriteCallBack:
				
				/* If handshakes in place, pump those along. */
				if (__RSBitIsSet(ctxt->_flags, kFlagBitHasHandshakes)) {
					RSArrayRef handshakes = (RSArrayRef)RSDictionaryGetValue(ctxt->_properties, _kRSStreamPropertyHandshakes);
					((_RSSocketStreamPerformHandshakeCallBack)RSArrayGetValueAtIndex(handshakes, 0))(ctxt);	
				}
				
				else {
					__RSBitSet(ctxt->_flags, kFlagBitCanWrite);
					__RSBitClear(ctxt->_flags, kFlagBitPollWrite);
					event = kRSStreamEventCanAcceptBytes;
					wStream = ctxt->_clientWriteStream;
				}
				break;
			
			default:
				break;
		}
	}

	/* Got an error during processing? */
	if (ctxt->_error.error) {
		
		/* Copy the error for call out. */
		memmove(&error, &ctxt->_error, sizeof(error));
		
		/* Set the event and streams for notification. */
		event = kRSStreamEventErrorOccurred;
		rStream = ctxt->_clientReadStream;
		wStream = ctxt->_clientWriteStream;
	}
	
	/* Only signal the read stream if it's been opened. */
	if (rStream && __RSBitIsSet(ctxt->_flags, kFlagBitReadStreamOpened))
		RSRetain(rStream);
	else
		rStream = NULL;
	
	/* Same is true for the write stream */
	if (wStream && __RSBitIsSet(ctxt->_flags, kFlagBitWriteStreamOpened))
		RSRetain(wStream);
	else
		wStream = NULL;
	
	/* If there is an event to signal, do so. */
	if (event != kRSStreamEventNone) {
		
		RSRunLoopRef rrl = NULL, wrl = NULL;
		RSRunLoopSourceRef rsrc = __RSBitIsSet(ctxt->_flags, kFlagBitReadHasCancel) ?
			(RSRunLoopSourceRef)RSDictionaryGetValue(ctxt->_properties, _kRSStreamPropertyReadCancel) :
			NULL;
		RSRunLoopSourceRef wsrc = __RSBitIsSet(ctxt->_flags, kFlagBitWriteHasCancel) ?
			(RSRunLoopSourceRef)RSDictionaryGetValue(ctxt->_properties, _kRSStreamPropertyWriteCancel) :
			NULL;
		
		if (rsrc) {
			
			RSRunLoopSourceContext c = {0};
			
			RSRetain(rsrc);
			
			RSRunLoopSourceGetContext(rsrc, &c);
			rrl = (RSRunLoopRef)(c.info);
		}
		
		if (wsrc) {
			
			RSRunLoopSourceContext c = {0};
			
			RSRetain(wsrc);
			
			RSRunLoopSourceGetContext(wsrc, &c);
			wrl = (RSRunLoopRef)(c.info);
		}
		
		/* 3863115 Only signal the read error if it's not buffered and no bytes waiting. */
		if (rStream && (event == kRSStreamEventErrorOccurred)) {
			
			RSDataRef c = (RSDataRef)RSDictionaryGetValue(ctxt->_properties, _kRSStreamPropertyRecvBufferCount);
			
			/* If there are bytes waiting, turn the event into a read event. */
			if (c && *((RSIndex*)RSDataGetBytePtr(c))) {
				event = kRSStreamEventHasBytesAvailable;
				memset(&error, 0, sizeof(error));
			}
		}

		RSSpinLockUnlock(&ctxt->_lock);
		
		if (rStream) {
			
			if (!rsrc)
				RSReadStreamSignalEvent(rStream, event, &error);
			else {
				RSRunLoopSourceSignal(rsrc);
				RSRunLoopWakeUp(rrl);
			}
		}
		
		if (wStream) {	

			if (!wsrc)
				RSWriteStreamSignalEvent(wStream, event, &error);
			else {
				RSRunLoopSourceSignal(wsrc);
				RSRunLoopWakeUp(wrl);
			}
		}
		
		if (rsrc) RSRelease(rsrc);
		if (wsrc) RSRelease(wsrc);
	}
	else
		RSSpinLockUnlock(&ctxt->_lock);
	
	if (rStream) RSRelease(rStream);
	if (wStream) RSRelease(wStream);
}


/* static */ void
_HostCallBack(RSHostRef theHost, RSHostInfoType typeInfo, const RSStreamError* error, _RSSocketStreamContext* ctxt) {

	int i;
	RSArrayRef addresses;
	RSMutableArrayRef loops[3];
	RSStreamError err;

	/* Only set to non-NULL if there is an error. */
	RSReadStreamRef rStream = NULL;
	RSWriteStreamRef wStream = NULL;
	
	/* NOTE the early bail!  Only care about the address callback. */
	if (typeInfo != kRSHostAddresses) return;
	
	/* Lock down the context. */
	RSSpinLockLock(&ctxt->_lock);
	
	/* Handle the error */
	if (error->error)
		memmove(&(ctxt->_error), error, sizeof(error[0]));
					
	/* Remove the host from the schedulables since it's done. */
	_SchedulablesRemove(ctxt->_schedulables, theHost);
	
	/* Invalidate it so no callbacks occur. */
	_RSTypeInvalidate(theHost);
	
	/* Grab the list of run loops and modes for unscheduling. */
	loops[0] = ctxt->_readloops;
	loops[1] = ctxt->_writeloops;
	loops[2] = ctxt->_sharedloops;
	
	/* Make sure to remove the host lookup from all loops and modes. */
	for (i = 0; i < (sizeof(loops) / sizeof(loops[0])); i++)
		_RSTypeUnscheduleFromMultipleRunLoops(theHost, loops[i]);
	
	/* Cancel the resolution for good measure. */
	RSHostCancelInfoResolution(theHost, kRSHostAddresses);
	
	if (!error->error) {
		
		/* Get the list of addresses for verification. */
		addresses = RSHostGetAddressing(theHost, NULL);
		
		/* Only attempt to connect if there are addresses. */
		if (addresses && RSArrayGetCount(addresses))
			_SocketStreamAttemptNextConnection_NoLock(ctxt);
			
		/* Mark an error that states that the host has no addresses. */
		else {
			ctxt->_error.error = EAI_NODATA;
			ctxt->_error.domain = kRSStreamErrorDomainNetDB;
		}
	}
	
	if (ctxt->_error.error) {

		/* Check to see if there is another lookup beyond this one. */
		RSTypeRef extra = (RSTypeRef)RSDictionaryGetValue(ctxt->_properties, kRSStreamPropertySocketRemoteHost);
		
		/* If didn't find one or the found one is not the current, need to invalidate and such. */
		if (!extra || (extra != theHost)) {
			
			/* If didn't find a lookup, see if there is a RSNetService lookup. */
			if (!extra)
				extra = (RSTypeRef)RSDictionaryGetValue(ctxt->_properties, kRSStreamPropertySocketRemoteNetService);
				
			/* If it's removed from the list, need to unschedule, invalidate, and cancel it. */
			if (extra && _SchedulablesRemove(ctxt->_schedulables, extra)) {
				
				/* Make sure to remove the lookup from all loops and modes. */
				for (i = 0; i < (sizeof(loops) / sizeof(loops[0])); i++)
					_RSTypeUnscheduleFromMultipleRunLoops(theHost, loops[i]);
				
				/* Invalidate it so no callbacks occur. */
				_RSTypeInvalidate(theHost);
		
				/* Cancel the resolution. */
				if (RSGetTypeID(extra) == RSHostGetTypeID())
					RSHostCancelInfoResolution((RSHostRef)extra, kRSHostAddresses);
				else
					RSNetServiceCancel((RSNetServiceRef)extra);
			}
		}
		
		/* Attempt "Auto Connect" if certain netdb errors are hit. */
		if (ctxt->_error.domain == kRSStreamErrorDomainNetDB) {
			
			switch (ctxt->_error.error) {
				
				case EAI_NODATA:
//				case 0xFECEFECE:
					_SocketStreamAttemptAutoVPN_NoLock(ctxt, (RSStringRef)RSArrayGetValueAtIndex(RSHostGetNames(theHost, NULL), 0));
					break;
					
				default:
					break;
			}
		}

		/* If there was an error at some point, mark complete and prepare for failure. */
		if (ctxt->_error.error) {
			
			__RSBitSet(ctxt->_flags, kFlagBitOpenComplete);
			__RSBitClear(ctxt->_flags, kFlagBitOpenStarted);
			__RSBitClear(ctxt->_flags, kFlagBitPollOpen);

			/* Copy the error for notification. */
			memmove(&err, &ctxt->_error, sizeof(err));
			
			/* Grab the client streams for error notification. */
			if (ctxt->_clientReadStream && __RSBitIsSet(ctxt->_flags, kFlagBitReadStreamOpened))
				rStream = (RSReadStreamRef)RSRetain(ctxt->_clientReadStream);
				
			if (ctxt->_clientWriteStream && __RSBitIsSet(ctxt->_flags, kFlagBitWriteStreamOpened))
				wStream = (RSWriteStreamRef)RSRetain(ctxt->_clientWriteStream);
		}
	}
	
	/* Unlock now. */
	RSSpinLockUnlock(&ctxt->_lock);
	
	/* Signal the streams of the error event. */
	if (rStream) {
		RSReadStreamSignalEvent(rStream, kRSStreamEventErrorOccurred, &err);
		RSRelease(rStream);
	}
		
	if (wStream) {
		RSWriteStreamSignalEvent(wStream, kRSStreamEventErrorOccurred, &err);
		RSRelease(wStream);
	}
}


/* static */ void
_NetServiceCallBack(RSNetServiceRef theService, RSStreamError* error, _RSSocketStreamContext* ctxt) {

	int i;
	RSMutableArrayRef loops[3];
	RSArrayRef addresses;

	/* Only set to non-NULL if there is an error. */
	RSReadStreamRef rStream = NULL;
	RSWriteStreamRef wStream = NULL;
	
	/* Lock down the context. */
	RSSpinLockLock(&ctxt->_lock);
	
	/* Copy the error into the context. */
	if (error->error)
		memmove(&(ctxt->_error), error, sizeof(error[0]));
	
	/* Remove the host from the schedulables since it's done. */
	_SchedulablesRemove(ctxt->_schedulables, theService);
	
	/* Invalidate it so no callbacks occur. */
	_RSTypeInvalidate(theService);
	
	/* Grab the list of run loops and modes for unscheduling. */
	loops[0] = ctxt->_readloops;
	loops[1] = ctxt->_writeloops;
	loops[2] = ctxt->_sharedloops;
	
	/* Make sure to remove the host lookup from all loops and modes. */
	for (i = 0; i < (sizeof(loops) / sizeof(loops[0])); i++)
		_RSTypeUnscheduleFromMultipleRunLoops(theService, loops[i]);
	
	/* Cancel the resolution for good measure. */
	RSNetServiceCancel(theService);
		
	if (!error->error) {
	
		/* Get the list of addresses for verification. */
		addresses = RSNetServiceGetAddressing(theService);
		
		/* Only attempt to connect if there are addresses. */
		if (addresses && RSArrayGetCount(addresses))
			_SocketStreamAttemptNextConnection_NoLock(ctxt);
			
		/* Mark an error that states that the host has no addresses. */
		else {
			ctxt->_error.error = EAI_NODATA;
			ctxt->_error.domain = kRSStreamErrorDomainNetDB;
		}
	}
		
	if (ctxt->_error.error) {
		
		/* Copy the error for notification. */
		memmove(&ctxt->_error, error, sizeof(error));
		
		/* Grab the client streams for error notification. */
		if (ctxt->_clientReadStream && __RSBitIsSet(ctxt->_flags, kFlagBitReadStreamOpened))
			rStream = (RSReadStreamRef)RSRetain(ctxt->_clientReadStream);
			
		if (ctxt->_clientWriteStream && __RSBitIsSet(ctxt->_flags, kFlagBitWriteStreamOpened))
			wStream = (RSWriteStreamRef)RSRetain(ctxt->_clientWriteStream);
	}
	
	/* Unlock now. */
	RSSpinLockUnlock(&ctxt->_lock);
	
	/* Signal the streams of the error event. */
	if (rStream) {
		RSReadStreamSignalEvent(rStream, kRSStreamEventErrorOccurred, (RSStreamError*)(&error));
		RSRelease(rStream);
	}
		
	if (wStream) {
		RSWriteStreamSignalEvent(wStream, kRSStreamEventErrorOccurred, (RSStreamError*)(&error));
		RSRelease(wStream);
	}
}


/* static */ void
_SocksHostCallBack(RSHostRef theHost, RSHostInfoType typeInfo, const RSStreamError* error, _RSSocketStreamContext* ctxt) {

	RSStreamError err;
	
	/* Only set to non-NULL if there is an error. */
	RSReadStreamRef rStream = NULL;
	RSWriteStreamRef wStream = NULL;
	
	/* NOTE the early bail!  Only care about the address callback. */
	if (typeInfo != kRSHostAddresses) return;
	
	/* Lock down the context. */
	RSSpinLockLock(&ctxt->_lock);
	
	/* Tell SOCKS to handle it. */
	_SocketStreamSOCKSHandleLookup_NoLock(ctxt, theHost);
	
	/*
	** Cancel the resolution for good measure.  Object should have
	** been unscheduled and invalidated in the SOCKS call.
	*/
	RSHostCancelInfoResolution(theHost, kRSHostAddresses);

	if (ctxt->_error.error) {
		
		__RSBitSet(ctxt->_flags, kFlagBitOpenComplete);
		__RSBitClear(ctxt->_flags, kFlagBitOpenStarted);
		__RSBitClear(ctxt->_flags, kFlagBitPollOpen);

		/* Copy the error for notification. */
		memmove(&err, &ctxt->_error, sizeof(err));
		
		/* Grab the client streams for error notification. */
		if (ctxt->_clientReadStream && __RSBitIsSet(ctxt->_flags, kFlagBitReadStreamOpened))
			rStream = (RSReadStreamRef)RSRetain(ctxt->_clientReadStream);
		
		if (ctxt->_clientWriteStream && __RSBitIsSet(ctxt->_flags, kFlagBitWriteStreamOpened))
			wStream = (RSWriteStreamRef)RSRetain(ctxt->_clientWriteStream);
	}
	
	/* Unlock now. */
	RSSpinLockUnlock(&ctxt->_lock);
	
	/* Signal the streams of the error event. */
	if (rStream) {
		RSReadStreamSignalEvent(rStream, kRSStreamEventErrorOccurred, &err);
		RSRelease(rStream);
	}
	
	if (wStream) {
		RSWriteStreamSignalEvent(wStream, kRSStreamEventErrorOccurred, &err);
		RSRelease(wStream);
	}
}


/* static */ void
_ReachabilityCallBack(SCNetworkReachabilityRef target, const SCNetworkConnectionFlags flags, _RSSocketStreamContext* ctxt) {

	RSDataRef c;
	
	/*
    ** 3483384 If the reachability callback fires, there was a change in
    ** routing for this pair and it should get an error.
	*/
	
	/* Lock down the context */
	RSSpinLockLock(&ctxt->_lock);
    
    ctxt->_error.error = ENOTCONN;
    ctxt->_error.domain = _kRSStreamErrorDomainNativeSockets;

	/* Attempt to get the count of buffered bytes. */
	c = (RSDataRef)RSDictionaryGetValue(ctxt->_properties, _kRSStreamPropertyRecvBufferCount);

	/*
	** 3863115 If there is a client stream and it's been opened, signal the error, but
	** only if there are no bytes sitting in the buffer.
	*/
	if ((!c || !*((RSIndex*)RSDataGetBytePtr(c))) &&
		(ctxt->_clientReadStream && __RSBitIsSet(ctxt->_flags, kFlagBitReadStreamOpened)))
	{
		_RSReadStreamSignalEventDelayed(ctxt->_clientReadStream, kRSStreamEventErrorOccurred, &ctxt->_error);
	}
	
	/* If there is a client stream and it's been opened, signal the error. */
	if (ctxt->_clientWriteStream && __RSBitIsSet(ctxt->_flags, kFlagBitWriteStreamOpened))
		_RSWriteStreamSignalEventDelayed(ctxt->_clientWriteStream, kRSStreamEventErrorOccurred, &ctxt->_error);
	
	/* Unlock */
	RSSpinLockUnlock(&ctxt->_lock);
}


/* static */ void
_NetworkConnectionCallBack(SCNetworkConnectionRef conn, SCNetworkConnectionStatus status, _RSSocketStreamContext* ctxt) {

	RSReadStreamRef rStream = NULL;
	RSWriteStreamRef wStream = NULL;
	RSStreamEventType event = kRSStreamEventNone;
	RSStreamError error = {0, 0};
	
	/* Only perform anything for the final states. */
	switch (status) {
		
		case kSCNetworkConnectionConnected:			
		case kSCNetworkConnectionDisconnected:
		case kSCNetworkConnectionInvalid:
			
		{
			int i;
			RSArrayRef loops[3] = {ctxt->_readloops, ctxt->_writeloops, ctxt->_sharedloops};
			
			/* Lock down the context */
			RSSpinLockLock(&ctxt->_lock);

			/* Unschedule the connection from all loops and modes */
			for (i = 0; i < (sizeof(loops) / sizeof(loops[0])); i++)
				_RSTypeUnscheduleFromMultipleRunLoops(conn, loops[i]);
			
			/* Invalidate the connection; never to be used again. */
			_RSTypeInvalidate(conn);
			
			/* Remove the connection from the schedulables. */
			_SchedulablesRemove(ctxt->_schedulables, conn);
			
			if (!_SocketStreamStartLookupForOpen_NoLock(ctxt)) {
				
				/*
				 ** If no lookup started and no error, everything must be
				 ** ready for a connect attempt.
				 */
				if (!ctxt->_error.error)
					_SocketStreamAttemptNextConnection_NoLock(ctxt);
			}
			
			/* Did connect actually progress all the way through? */
			if (__RSBitIsSet(ctxt->_flags, kFlagBitOpenComplete)) {
				
				/* Remove the "started" flag */
				__RSBitClear(ctxt->_flags, kFlagBitOpenStarted);
				
				/* Get the streams and event to signal. */
				event = kRSStreamEventOpenCompleted;
				
				if (ctxt->_clientReadStream && __RSBitIsSet(ctxt->_flags, kFlagBitReadStreamOpened))
					rStream = ctxt->_clientReadStream;
				
				if (ctxt->_clientWriteStream && __RSBitIsSet(ctxt->_flags, kFlagBitWriteStreamOpened))
					wStream = ctxt->_clientWriteStream;
			}
			
			/* Copy the error if one occurred. */
			if (ctxt->_error.error) {
				memmove(&error, &ctxt->_error, sizeof(error));
				
				/* Set the event and streams for notification. */
				event = kRSStreamEventErrorOccurred;
				
				if (ctxt->_clientReadStream && __RSBitIsSet(ctxt->_flags, kFlagBitReadStreamOpened))
					rStream = ctxt->_clientReadStream;
				
				if (ctxt->_clientWriteStream && __RSBitIsSet(ctxt->_flags, kFlagBitWriteStreamOpened))
				wStream = ctxt->_clientWriteStream;
			}
			
			if (rStream) RSRetain(rStream);
			if (wStream) RSRetain(wStream);
					
			/* Unlock */
			RSSpinLockUnlock(&ctxt->_lock);
			
			break;
		}
			
		default:
			break;
	}
	
	if (event != kRSStreamEventNone) {
		
		if (rStream)
			RSReadStreamSignalEvent(rStream, event, &error);
		
		if (wStream) 			
			RSWriteStreamSignalEvent(wStream, event, &error);
	}
	
	if (rStream) RSRelease(rStream);
	if (wStream) RSRelease(wStream);
}


/* static */ _RSSocketStreamContext*
_SocketStreamCreateContext(RSAllocatorRef alloc) {
	
	/* Allocate the base structure */
	_RSSocketStreamContext* ctxt = (_RSSocketStreamContext*)RSAllocatorAllocate(alloc,
																				sizeof(ctxt[0]),
																				0);
	
	/* Continue on if successful */
	if (ctxt) {
		
		/* Zero everything to start. */
        memset(ctxt, 0, sizeof(ctxt[0]));
		
		/* Create the arrays for run loops and modes. */
		ctxt->_readloops = RSArrayCreateMutable(alloc, 0, &kRSTypeArrayCallBacks);
		ctxt->_writeloops = RSArrayCreateMutable(alloc, 0, &kRSTypeArrayCallBacks);
		ctxt->_sharedloops = RSArrayCreateMutable(alloc, 0, &kRSTypeArrayCallBacks);
		
		/*  Create the set for the list of schedulable items. */
		ctxt->_schedulables = RSArrayCreateMutable(alloc, 0, &kRSTypeArrayCallBacks);
		
		/* Create a dictionary to hold the properties. */
        ctxt->_properties = RSDictionaryCreateMutable(alloc,
                                                      0,
                                                      &kRSTypeDictionaryKeyCallBacks,
                                                      &kRSTypeDictionaryValueCallBacks);

		/* If anything failed, need to cleanup and toss result */
		if (!ctxt->_readloops || !ctxt->_writeloops || !ctxt->_sharedloops ||
			!ctxt->_schedulables || !ctxt->_properties)
		{
			ctxt = NULL;
		}
	}
	
	return ctxt;
}


/* static */ void
_SocketStreamDestroyContext_NoLock(RSAllocatorRef alloc, _RSSocketStreamContext* ctxt) {

	int i;
	RSMutableArrayRef loops[] = {ctxt->_readloops, ctxt->_writeloops, ctxt->_sharedloops};

	/* Make sure to unschedule all the schedulables on this loop and mode. */
	if (ctxt->_schedulables) {
		
		RSRange r = RSRangeMake(0, RSArrayGetCount(ctxt->_schedulables));
		
		/* Unschedule the schedulables from all run loops and modes. */
		for (i = 0; i < (sizeof(loops) / sizeof(loops[0])); i++) {
			if (loops[i])
				RSArrayApplyFunction(ctxt->_schedulables, r, (RSArrayApplierFunction)_SchedulablesUnscheduleFromAllApplierFunction, loops[i]);
		}
		
		/* Make sure to invalidate them all. */
		RSArrayApplyFunction(ctxt->_schedulables, r, (RSArrayApplierFunction)_SchedulablesInvalidateApplierFunction, NULL);
	
		/* Release them all now. */
		RSRelease(ctxt->_schedulables);
	}
	
	/* Get rid of the socket */
	if (ctxt->_socket) {
		
		/* Make sure to invalidate the socket */
		RSSocketInvalidate(ctxt->_socket);
		
		RSRelease(ctxt->_socket);
	}

	/* Release the lists of run loops and modes */
	for (i = 0; i < (sizeof(loops) / sizeof(loops[0])); i++)
		if (loops[i]) RSRelease(loops[i]);
	
	/* Get rid of any properties */
	if (ctxt->_properties)
		RSRelease(ctxt->_properties);
	
	/* Toss the context */
	RSAllocatorDeallocate(alloc, ctxt);
}


/* static */ BOOL
_SchedulablesAdd(RSMutableArrayRef schedulables, RSTypeRef addition) {

	if (!RSArrayContainsValue(schedulables, RSRangeMake(0, RSArrayGetCount(schedulables)), addition)) {
		RSArrayAppendValue(schedulables, addition);
		return TRUE;
	}
	
	return FALSE;
}


/* static */ BOOL
_SchedulablesRemove(RSMutableArrayRef schedulables, RSTypeRef removal) {
	
	RSIndex i = RSArrayGetFirstIndexOfValue(schedulables, RSRangeMake(0, RSArrayGetCount(schedulables)), removal);
	
	if (i != kRSNotFound) {
		RSArrayRemoveValueAtIndex(schedulables, i);
		return TRUE;
	}
	
	return FALSE;
}


/* static */ void
_SchedulablesScheduleApplierFunction(RSTypeRef obj, RSTypeRef loopAndMode[]) {
	
	/* Schedule the object on the loop and mode */
	_RSTypeScheduleOnRunLoop(obj, (RSRunLoopRef)loopAndMode[0], (RSStringRef)loopAndMode[1]);
}


/* static */ void
_SchedulablesUnscheduleApplierFunction(RSTypeRef obj, RSTypeRef loopAndMode[]) {
	
	/* Remove the object from the loop and mode */
	_RSTypeUnscheduleFromRunLoop(obj, (RSRunLoopRef)loopAndMode[0], (RSStringRef)loopAndMode[1]);
}


/* static */ void
_SchedulablesUnscheduleFromAllApplierFunction(RSTypeRef obj, RSArrayRef schedules) {
	
	/* Remove the object from all the run loops and modes */
	_RSTypeUnscheduleFromMultipleRunLoops(obj, schedules);
}


/* static */ void
_SchedulablesInvalidateApplierFunction(RSTypeRef obj, void* context) {
	
	(void)context;  /* unused */
	
	RSTypeID type = RSGetTypeID(obj);
	
	/* Invalidate the process. */
	_RSTypeInvalidate(obj);
	
	/* For RSHost and RSNetService, make sure to cancel too. */
	if (RSHostGetTypeID() == type)
		RSHostCancelInfoResolution((RSHostRef)obj, kRSHostAddresses);
	else if (RSNetServiceGetTypeID() == type)
		RSNetServiceCancel((RSNetServiceRef)obj);
}


/* static */ void
_SocketStreamSchedule_NoLock(RSTypeRef stream, RSRunLoopRef runLoop, RSStringRef runLoopMode,
							 _RSSocketStreamContext* ctxt)
{
	RSMutableArrayRef loops, otherloops;
	BOOL isReadStream = (RSGetTypeID(stream) == RSReadStreamGetTypeID());
	
	/*
	 ** Figure out the proper loops and modes to use.  loops refers to
	 ** the list of schedules for the stream half which was passed into
	 ** the function.  otherloops refers to the list of schedules for
	 ** the other half.
	 */
	if (isReadStream) {
		loops = ctxt->_readloops;
		otherloops = ctxt->_writeloops;
	}
	else {
		loops = ctxt->_writeloops;
		otherloops = ctxt->_readloops;
	}
	
	/*
	 ** If the loop and mode are already in the shared list or the current
	 ** half is already scheduled on this loop and mode, don't do anything.
	 */
	if ((kRSNotFound == _SchedulesFind(ctxt->_sharedloops, runLoop, runLoopMode)) &&
		(kRSNotFound == _SchedulesFind(loops, runLoop, runLoopMode)))
	{
		/* Different behavior if the other half is scheduled on this loop and mode */
		if (kRSNotFound == _SchedulesFind(otherloops, runLoop, runLoopMode)) {
			
			RSTypeRef loopAndMode[2] = {runLoop, runLoopMode};
			
			/* Other half not scheduled, so only schedule on this half. */
			_SchedulesAddRunLoopAndMode(loops, runLoop, runLoopMode);
			
			/* Make sure to schedule all the schedulables on this loop and mode. */
			RSArrayApplyFunction(ctxt->_schedulables,
								 RSRangeMake(0, RSArrayGetCount(ctxt->_schedulables)),
								 (RSArrayApplierFunction)_SchedulablesScheduleApplierFunction,
								 loopAndMode);								 
		}
		
		else {
			
			/* Other half is scheduled already, so remove this schedule from the other half. */
			_SchedulesRemoveRunLoopAndMode(otherloops, runLoop, runLoopMode);
			
			/* Promote this schedule to being shared. */
			_SchedulesAddRunLoopAndMode(ctxt->_sharedloops, runLoop, runLoopMode);
			
			/* NOTE that the schedulables are not scheduled since they already have been. */
		}
		
		if (isReadStream) {
			if (__RSBitIsSet(ctxt->_flags, kFlagBitCanRead) &&
				(RSArrayGetCount(loops) + RSArrayGetCount(ctxt->_sharedloops) == 4))
			{
				RSReadStreamSignalEvent((RSReadStreamRef)stream, kRSStreamEventHasBytesAvailable, NULL);
			}
		}
		
		else {
			if (__RSBitIsSet(ctxt->_flags, kFlagBitCanWrite) &&
				(RSArrayGetCount(loops) + RSArrayGetCount(ctxt->_sharedloops) == 4))
			{
				RSWriteStreamSignalEvent((RSWriteStreamRef)stream, kRSStreamEventCanAcceptBytes, NULL);
			}
		}
	}
}


/* static */ void
_SocketStreamUnschedule_NoLock(RSTypeRef stream, RSRunLoopRef runLoop, RSStringRef runLoopMode,
							   _RSSocketStreamContext* ctxt)
{
	RSMutableArrayRef loops, otherloops;
	
	/*
	 ** Figure out the proper loops and modes to use.  loops refers to
	 ** the list of schedules for the stream half which was passed into
	 ** the function.  otherloops refers to the list of schedules for
	 ** the other half.
	 */
	if (RSGetTypeID(stream) == RSReadStreamGetTypeID()) {
		loops = ctxt->_readloops;
		otherloops = ctxt->_writeloops;
	}
	else {
		loops = ctxt->_writeloops;
		otherloops = ctxt->_readloops;
	}
	
	/* Remove the loop and mode from the shared list if there */
	if (_SchedulesRemoveRunLoopAndMode(ctxt->_sharedloops, runLoop, runLoopMode)) {
		
		/* Demote the schedule down to one half instead of shared. */
		_SchedulesAddRunLoopAndMode(otherloops, runLoop, runLoopMode);
		
		/*
		 ** NOTE that the schedulables are not unscheduled, since they're
		 ** still scheduled for the other half.
		 */
	}
	
	/* Wasn't in the shared list, so try removing it from the list for this half. */
	else if (_SchedulesRemoveRunLoopAndMode(loops, runLoop, runLoopMode)) {
		
		RSTypeRef loopAndMode[2] = {runLoop, runLoopMode};
		
		/* Make sure to unschedule all the schedulables on this loop and mode. */
		RSArrayApplyFunction(ctxt->_schedulables,
							 RSRangeMake(0, RSArrayGetCount(ctxt->_schedulables)),
							 (RSArrayApplierFunction)_SchedulablesUnscheduleApplierFunction,
							 loopAndMode);
	}
}


/* static */ RSNumberRef
_RSNumberCopyPortForOpen(RSDictionaryRef properties) {
	
	RSNumberRef result = NULL;
	
	/* Attempt to grab the SOCKS proxy information */
	RSDictionaryRef proxy = (RSDictionaryRef)RSDictionaryGetValue(properties, kRSStreamPropertySOCKSProxy);
	
	/* If SOCKS proxy is being used, need to go to it. */
	if (proxy) {
	
		/* Try to get the one that was passed in. */
		result = (RSNumberRef)RSDictionaryGetValue(proxy, kRSStreamPropertySOCKSProxyPort);
		
		if (result)
			RSRetain(result);
		
		/* If not one, create one as the default. */
		else {
		
			SInt32 default_port = 1080;		/* Default SOCKS port */
			
			/* Create the RSNumber from the default port */
			result = RSNumberCreate(RSGetAllocator(properties), kRSNumberSInt32Type, &default_port);
		}
	}
	
	/* If there is no SOCKS proxy, it could be a CONNECT proxy. */
	else if ((proxy = (RSDictionaryRef)RSDictionaryGetValue(properties, kRSStreamPropertyCONNECTProxy))) {
	
		/* Try to get the one that was passed in. */
		result = (RSNumberRef)RSDictionaryGetValue(proxy, kRSStreamPropertyCONNECTProxyPort);
		
		/* There is no default port for CONNECT tunneling. */
		
		if (result) RSRetain(result);
	}
	
	/* It's direct to the host */
	else {
		result = (RSNumberRef)RSDictionaryGetValue(properties, _kRSStreamPropertySocketRemotePort);
		
		if (result) RSRetain(result);
	}
	
	return result;
}


/* static */ RSDataRef
_RSDataCopyAddressByInjectingPort(RSDataRef address, RSNumberRef port) {
	
	/*
	** If there was no port given, assume the port is in the address
	** already and just give the address an extra retain.
	*/
	if (!port)
		RSRetain(address);
	
	/* There is a port to inject */
	else {
	
		SInt32 p;
		
		/* If the port can't be retrieved from the number, return no address. */
		if (!RSNumberGetValue(port, kRSNumberSInt32Type, &p))
			address = NULL;
		
		/* Need to inject the port value now. */
		else {
			
			/* Handle injection based upon address family */
			switch (((struct sockaddr*)RSDataGetBytePtr(address))->sa_family) {
				
				case AF_INET:
					/* Create a copy for injection. */
					address = RSDataCreateMutableCopy(RSGetAllocator(address),
													  0,
													  address);
					
					/* Only place it there if a copy was made. */
					if (address)
						((struct sockaddr_in*)(RSDataGetMutableBytePtr((RSMutableDataRef)address)))->sin_port = htons(0x0000FFFF & p);
					break;
					
				case AF_INET6:
					/* Create a copy for injection. */
					address = RSDataCreateMutableCopy(RSGetAllocator(address),
													  0,
													  address);
					
					/* Only place it there if a copy was made. */
					if (address)
						((struct sockaddr_in6*)(RSDataGetMutableBytePtr((RSMutableDataRef)address)))->sin6_port = htons(0x0000FFFF & p);
					break;
				
				/*
				** Fail for an address family that is not known and is supposed
				** to get an injected port value.
				*/
				default:
					address = NULL;
					break;
			}
		}
	}

	return address;
}


/* static */ BOOL
_ScheduleAndStartLookup(RSTypeRef lookup, RSArrayRef* schedules, RSStreamError* error, const void* cb, void* info) {

	do {
		int i;
		RSArrayRef addresses = NULL;
		RSTypeID lookup_type = RSGetTypeID(lookup);
		RSTypeID host_type = RSHostGetTypeID();

		/* Set to no error. */
		memset(error, 0, sizeof(error[0]));
		
		/* Get the list of addresses, if any, from the lookup. */
		if (lookup_type == host_type)
			addresses = RSHostGetAddressing((RSHostRef)lookup, NULL);
		else
			addresses = RSNetServiceGetAddressing((RSNetServiceRef)lookup);
		
		/* If there are existing addresses, try to use them. */
		if (addresses) {
			
			/* If there is at least one address, use it. */
			if (RSArrayGetCount(addresses))
				break;
			
			/* No addresses in the list */
			else {
				
				/* Mark an error that states that the host has no addresses. */
				error->error = EAI_NODATA;
				error->domain = kRSStreamErrorDomainNetDB;
				
				break;
			}
		}
		
		/* Set the stream as the client for callback. */
		if (lookup_type == host_type) {
			RSHostClientContext c = {0, info, NULL, NULL, NULL};			
			RSHostSetClient((RSHostRef)lookup, (RSHostClientCallBack)cb, &c);
		}
		else {
			RSNetServiceClientContext c = {0, info, NULL, NULL, NULL};
			RSNetServiceSetClient((RSNetServiceRef)lookup, (RSNetServiceClientCallBack)cb, &c);
		}
		
		/* Now schedule the lookup on all loops and modes */
		for (i = 0; schedules[i]; i++)
			_RSTypeScheduleOnMultipleRunLoops(lookup, schedules[i]);
		
		/* Start the lookup */
		if (lookup_type == host_type)
			RSHostStartInfoResolution((RSHostRef)lookup, kRSHostAddresses, error);
		else
			RSNetServiceResolveWithTimeout((RSNetServiceRef)lookup, 0.0, error);
		
		/* Verify that the lookup started. */
		if (error->error) {
			
			/* Remove it from the all schedules. */
			for (i = 0; schedules[i]; i++)
				_RSTypeUnscheduleFromMultipleRunLoops(lookup, schedules[i]);
			
			/* Invalidate the lookup; never to be used again. */
			_RSTypeInvalidate(lookup);
			
			break;
		}
		
		/* Did start a lookup. */
		return TRUE;
	}
	while (0);

	/* Did not start a lookup. */
	return FALSE;
}


/* static */ RSIndex
_RSSocketRecv(RSSocketRef s, UInt8* buffer, RSIndex length, RSStreamError* error) {
	
	RSIndex result = -1;
	
	/* Zero out the error (no error). */
	memset(error, 0, sizeof(error[0]));
	
	/* If the socket is invalid, return an EINVAL error. */
	if (!s || !RSSocketIsValid(s)) {
		error->error = EINVAL;
		error->domain = kRSStreamErrorDomainPOSIX;
	}
	
	else {		
		/* Try to read some bytes off the socket. */
		result = read(RSSocketGetNative(s), buffer, length);
		
		/* If recv returned an error, get the error and make sure to return -1. */
		if (result < 0) {
			_LastError(error);
			result = -1;
		}
	}
	
    return result;
}


/* static */ RSIndex
_RSSocketSend(RSSocketRef s, const UInt8* buffer, RSIndex length, RSStreamError* error) {
	
	RSIndex result = -1;
	
	/* Zero out the error (no error). */
	memset(error, 0, sizeof(error[0]));
	
	/* If the socket is invalid, return an EINVAL error. */
	if (!s || !RSSocketIsValid(s)) {
		error->error = EINVAL;
		error->domain = kRSStreamErrorDomainPOSIX;
	}
	
	else {		
		/* Try to read some bytes off the socket. */
		result = write(RSSocketGetNative(s), buffer, length);
		
		/* If recv returned an error, get the error and make sure to return -1. */
		if (result < 0) {
			_LastError(error);
			result = -1;
		}
	}
	
    return result;
}


/* static */ BOOL
_RSSocketCan(RSSocketRef s, int mode) {
    
	/*
	** Unfortunately, this function is required as a result of some odd behavior
	** code in RSSocket.  In cases where RSSocket is not scheduled but enable/
	** disable of the callbacks are called, they are not truly enabled/disabled.
	** Once the RSSocket is scheduled again, it enables all the original events
	** which is not necessarily RSSocketStream's intended state.  This code
	** is then used in order to double check the incoming event from RSSocket.
	*/ 
	
	/*
	** This code is also used as a performance win at the end of SocketStreamRead
	** and SocketStreamWrite.  It's cheaper to quickly poll the fd than it is
	** to return to the run loop wait for the event from RSSocket and then signal
	** the client that reading or writing can be performed.
	*/
	
	int val;
    fd_set	set;
    fd_set* setptr = &set;
    
    struct timeval timeout = {0, 0};
	
	int fd = RSSocketGetNative(s);
	
    FD_ZERO(setptr);
	
#if !defined __WIN32__
    /* Irrelevant on Win32, because they don't use a bitmask for select args */
    if (fd >= FD_SETSIZE) {
        
        val = howmany(fd + 1, NFDBITS) * sizeof(fd_mask);
        
        setptr = (fd_set*)malloc(val);
        bzero(setptr, val);
    }
#endif    
    
    FD_SET(fd, setptr);
	
    val = select(fd + 1,
				 (mode & kSelectModeRead ? setptr : NULL),
				 (mode & kSelectModeWrite ? setptr : NULL),
				 (mode & kSelectModeExcept ? setptr : NULL),
				 &timeout);
    
    if (setptr != &set)
        free(setptr);
	
    return (val > 0) ? TRUE : FALSE;
}


/* static */ BOOL
_SocketStreamStartLookupForOpen_NoLock(_RSSocketStreamContext* ctxt) {
	
	BOOL result = FALSE;
	
	do {
		RSTypeRef lookup = NULL;
		RSHostRef extra = NULL;			/* Used in the case of SOCKSv4 only */
		RSTypeID lookup_type, host_type = RSHostGetTypeID();
		RSArrayRef loops[4] = {ctxt->_readloops, ctxt->_writeloops, ctxt->_sharedloops, NULL};
		
		/* Attempt to grab the SOCKS proxy information */
		RSDictionaryRef proxy = (RSDictionaryRef)RSDictionaryGetValue(ctxt->_properties, kRSStreamPropertySOCKSProxy);
		
		/* If SOCKS proxy is being used, need to go to it. */
		if (proxy) {
			
			/* Create the host from the host name. */
			lookup = RSHostCreateWithName(RSGetAllocator(ctxt->_properties),
										  (RSStringRef)RSDictionaryGetValue(proxy, kRSStreamPropertySOCKSProxyHost));
			
			/*
			** If trying to do a SOCKSv4 connection, need to get the address.
			** If lookup fails, SOCKSv4a will be tried instread.
			*/
			if (lookup && RSEqual(_GetSOCKSVersion(proxy), kRSStreamSocketSOCKSVersion4)) {
				
				/* Grab the far end, intended host. */
				extra = (RSHostRef)RSDictionaryGetValue(ctxt->_properties, kRSStreamPropertySocketRemoteHost);
				
				/* If one wasn't found, give an invalid argument error.  This should never occur. */
				if (!extra) {
					ctxt->_error.error = EINVAL;
					ctxt->_error.domain = kRSStreamErrorDomainPOSIX;
					break;
				}
				
				/* Try to schedule the lookup. */
				else if (_ScheduleAndStartLookup(extra,
												 loops,
												 &ctxt->_error,
												 (const void*)_SocksHostCallBack,
												 ctxt))
				{					
					/* Add it to the list of schedulables for future scheduling calls. */
					_SchedulablesAdd(ctxt->_schedulables, extra);
				}
				
				/* If there was an error, bail.  If no error, it means there was an address already. */
				else if (ctxt->_error.error) {
					
					/* Not needed */
					RSRelease(lookup);
					break;
				}
			}
		}
		
		/* If there is no SOCKS proxy, it could be a CONNECT proxy. */
		else if ((proxy = (RSDictionaryRef)RSDictionaryGetValue(ctxt->_properties, kRSStreamPropertyCONNECTProxy))) {
			
			/* Create the host from the host name */
			lookup = RSHostCreateWithName(RSGetAllocator(ctxt->_properties),
										  (RSStringRef)RSDictionaryGetValue(proxy, kRSStreamPropertyCONNECTProxyHost));
		}
		
		/* It's direct to the host */
		else {
			
			/* No proxies so go for the remote host. */
			lookup = (RSTypeRef)RSDictionaryGetValue(ctxt->_properties, kRSStreamPropertySocketRemoteHost);
			
			/* If no host, then it's RSNetService based. */
			if (!lookup)
				lookup = (RSTypeRef)RSDictionaryGetValue(ctxt->_properties, kRSStreamPropertySocketRemoteNetService);
			
			/* There should always be a lookup of some sort, but just in case. */
			if (lookup) RSRetain(lookup);
		}
		
		/* If there is no host for lookup, specify an error. */
		if (!lookup) {
			
			/* If there is no socket already, there must be a lookup. */
			if (!ctxt->_socket && !RSDictionaryGetValue(ctxt->_properties, kRSStreamPropertySocketNativeHandle)) {
				
				/* Attempt to get the error from errno. */
				ctxt->_error.error = errno;
				ctxt->_error.domain = kRSStreamErrorDomainPOSIX;
				
				/* If errno is not filled, assume no memory. */
				if (!ctxt->_error.error)
					ctxt->_error.error = ENOMEM;
			}
			
			break;
		}
		
		/* Get the type of lookup for type specific work. */
		lookup_type = RSGetTypeID(lookup);
		
		/* Given the lookup, try to kick it off */
		result = _ScheduleAndStartLookup(lookup,
										 loops,
										 &ctxt->_error,
										 ((lookup_type == host_type) ? (const void*)_HostCallBack : (const void*)_NetServiceCallBack),
										 ctxt);
		
		/* Add it to the list of schedulables for future scheduling calls. */
		if (result)
			_SchedulablesAdd(ctxt->_schedulables, lookup);
		
		/* Scheduling failed as a result of an error. */
		else if (ctxt->_error.error) {

			/* Release the lookup */
			RSRelease(lookup);
			
			/* Need to cancel and cleanup any lookup started as a result of socksv4. */
			if (extra) {
				
				int i;
				
				/* Remove the sockv4 lookup from the list of schedulables. */
				_SchedulablesRemove(ctxt->_schedulables, extra);
				
				/* Remove it from the list of all schedules. */
				for (i = 0; loops[i]; i++)
					_RSTypeUnscheduleFromMultipleRunLoops(extra, loops[i]);
				
				/* Invalidate the socksv4 lookup; never to be used again. */
				_RSTypeInvalidate(extra);
				
				/* Cancel any lookup that it was doing. */
				RSHostCancelInfoResolution((RSHostRef)extra, kRSHostAddresses);
			}
			
			break;
		}
		
		/* Save the lookup in the properties list for iteration and socket creation later. */
		RSDictionaryAddValue(ctxt->_properties, _kRSStreamPropertyHostForOpen, lookup);
		
		/* Release the lookup now. */
		RSRelease(lookup);

	} while (0);
	
	return result;
}


/* static */ BOOL
_SocketStreamCreateSocket_NoLock(_RSSocketStreamContext* ctxt, RSDataRef address) {
	
	do {
		SInt32 protocolFamily = PF_INET;
		SInt32 socketType = SOCK_STREAM;
		SInt32 protocol = IPPROTO_TCP;
		
        int yes = 1;
		RSOptionFlags flags;
		RSSocketNativeHandle s;		
		RSSocketContext c = {0, ctxt, NULL, NULL, NULL};
		
		RSArrayRef callback;
		RSBOOLRef boolean;
		RSDictionaryRef info = (RSDictionaryRef)RSDictionaryGetValue(ctxt->_properties, _kRSStreamPropertySocketFamilyTypeProtocol);
		
		/* If there was a dictionary for the RSSocketSignature-type values, get those values. */
		if (info) {
			
			if (RSDictionaryContainsValue(info, _kRSStreamSocketFamily)) {
				const void* tmp = RSDictionaryGetValue(info, _kRSStreamSocketFamily);
				protocolFamily = (SInt32)tmp;
			}
			
			if (RSDictionaryContainsValue(info, _kRSStreamSocketType)) {
				const void* tmp = RSDictionaryGetValue(info, _kRSStreamSocketType);
				socketType = (SInt32)tmp;
			}
			
			if (RSDictionaryContainsValue(info, _kRSStreamSocketProtocol)) {
				const void* tmp = RSDictionaryGetValue(info, _kRSStreamSocketProtocol);
				protocol = (SInt32)tmp;
			}
		}
		
		/*
		 ** Set the protocol family based upon the address family.  Do it after
		 ** setting from the signature values in order to guarantee things like
		 ** fallover from IPv4 to IPv6 succeed correctly.
		 */
		if (address)
			protocolFamily = ((struct sockaddr*)RSDataGetBytePtr(address))->sa_family;
		
		/* Attempt to create the socket */
		ctxt->_socket = RSSocketCreate(RSGetAllocator(ctxt->_properties),
									   protocolFamily,
									   socketType,
									   protocol,
									   kSocketEvents,
									   (RSSocketCallBack)_SocketCallBack, 
									   &c);
		
		if (!ctxt->_socket) {
			
			/*
			** Try to pull any error they may have just occurred.  If none,
			** assume an out of memory occurred.
			*/
			if (!_LastError(&ctxt->_error)) {
				ctxt->_error.error = ENOMEM;
				ctxt->_error.domain = kRSStreamErrorDomainPOSIX;
			}
			
			break;
		}
		
		/* Get the native socket for setting options. */
		s = RSSocketGetNative(ctxt->_socket);
		
		/* See if the client wishes to be informed of the new socket.  Call back if needed. */
		callback = (RSArrayRef)RSDictionaryGetValue(ctxt->_properties, _kRSStreamSocketCreatedCallBack);
		if (callback)
			((_RSSocketStreamSocketCreatedCallBack)RSArrayGetValueAtIndex(callback, 0))(s, (void*)RSArrayGetValueAtIndex(callback, 1));
		
		/* Find out if the ttl is supposed to be set special so as to prevent traffic beyond the subnet. */
		boolean = (RSBOOLRef)RSDictionaryGetValue(ctxt->_properties, _kRSStreamSocketIChatWantsSubNet);
		if (boolean == kRSBOOLTrue) {
            int ttl = 255;
            setsockopt(s, IPPROTO_IP, IP_TTL, (void*)&ttl, sizeof(ttl));
		}
		
#if !defined(__WIN32)
        /* Turn off SIGPIPE on the socket (SIGPIPE doesn't exist on WIN32) */
        setsockopt(s, SOL_SOCKET, SO_NOSIGPIPE, (void*)&yes, sizeof(yes));
#endif
		
        /* Place the socket in nonblocking mode. */
        ioctl(s, FIONBIO, (void*)&yes);
		
		/* Get the current socket flags and turn off the auto re-enable for reads and writes. */
		flags = RSSocketGetSocketFlags(ctxt->_socket) &
			~kRSSocketAutomaticallyReenableReadCallBack &
			~kRSSocketAutomaticallyReenableWriteCallBack;
		
		/* Find out if RSSocket should close the socket on invalidation. */
		boolean = (RSBOOLRef)RSDictionaryGetValue(ctxt->_properties, kRSStreamPropertyShouldCloseNativeSocket);
		
		/* Adjust the flags on the setting.  No value is the default which means to close. */
		if (!boolean || (boolean != kRSBOOLFalse))
			flags |= kRSSocketCloseOnInvalidate;
		else
			flags &= ~kRSSocketCloseOnInvalidate;
		
		/* Set up the correct flags and enable the callbacks. */
		RSSocketSetSocketFlags(ctxt->_socket, flags);
		//RSSocketEnableCallBacks(ctxt->_socket, kRSSocketReadCallBack | kRSSocketWriteCallBack);
		
		return TRUE;
		
	} while (0);
	
	return FALSE;
}


/* static */ BOOL
_SocketStreamConnect_NoLock(_RSSocketStreamContext* ctxt, RSDataRef address) {
	
	int i;
	BOOL result = FALSE;
	RSArrayRef loops[3] = {ctxt->_readloops, ctxt->_writeloops, ctxt->_sharedloops};
	
	/* Now schedule the socket on all loops and modes */
	for (i = 0; i < (sizeof(loops) / sizeof(loops[0])); i++)
		_RSTypeScheduleOnMultipleRunLoops(ctxt->_socket, loops[i]);
		
	/* Start the connect */
	if ((result = (RSSocketConnectToAddress(ctxt->_socket, address, -1.0) == kRSSocketSuccess))) {
		
		memset(&ctxt->_error, 0, sizeof(ctxt->_error));
		
		/* Succeeded so make sure the socket is in the list of schedulables for future. */
		_SchedulablesAdd(ctxt->_schedulables, ctxt->_socket);
	}
	
	else {
		
		/* Grab the error that occurred.  If no error, make one up. */
		if (!_LastError(&ctxt->_error)) {
			ctxt->_error.error = EINVAL;
			ctxt->_error.domain = kRSStreamErrorDomainPOSIX;
		}
		
		/* Remove the socket from all the schedules. */
		for (i = 0; i < (sizeof(loops) / sizeof(loops[0])); i++)
			_RSTypeUnscheduleFromMultipleRunLoops(ctxt->_socket, loops[i]);
		
		/* Invalidate the socket; never to be used again. */
		_RSTypeInvalidate(ctxt->_socket);
		
		/* Release and forget the socket */
		RSRelease(ctxt->_socket);
		ctxt->_socket = NULL;
	}
	
	return result;
}


/* static */ BOOL
_SocketStreamAttemptNextConnection_NoLock(_RSSocketStreamContext* ctxt) {
	
	do {
		/* Attempt to get the primary host for connecting */
		RSTypeRef lookup = (RSTypeRef)RSDictionaryGetValue(ctxt->_properties, _kRSStreamPropertyHostForOpen);
		SInt32* attempt = NULL;

		/* If there was a host, there is more work to do */
		if (lookup) {

			RSIndex count;
			RSArrayRef list = NULL;
			RSDataRef address = NULL;
			RSNumberRef port = _RSNumberCopyPortForOpen(ctxt->_properties);
			RSMutableDataRef a = (RSMutableDataRef)RSDictionaryGetValue(ctxt->_properties, _kRSStreamPropertySocketAddressAttempt);
			
			/* If there is an address attempt, point to the counter. */
			if (a)
				attempt = (SInt32*)RSDataGetMutableBytePtr(a);
			
			/* This is the first attempt so create and add the counter. */
			else {
				
				SInt32 i = 0;
				
				/* Create the counter. */
				a = RSDataCreateMutable(RSGetAllocator(ctxt->_properties), sizeof(i));
				
				/* If it fails, set out of memory and bail. */
				if (!a) {
					ctxt->_error.error = ENOMEM;
					ctxt->_error.domain = kRSStreamErrorDomainPOSIX;
					
					/* Not needed anymore. */
					if (port) RSRelease(port);

					break;
				}
				
				/* Add the attempt counter to the properties for later */
				RSDictionaryAddValue(ctxt->_properties, _kRSStreamPropertySocketAddressAttempt, a);
				RSRelease(a);
				
				/* Point the attempt at the counter */
				attempt = (SInt32*)RSDataGetMutableBytePtr(a);
				
				/* Start counting at zero. */
				*attempt = 0;
			}
			
			/* Get the address list from the lookup. */
			if (RSGetTypeID(lookup) == RSHostGetTypeID())
				list = RSHostGetAddressing((RSHostRef)lookup, NULL);
			else
				list = RSNetServiceGetAddressing((RSNetServiceRef)lookup);
			
			/* If there were no addresses, return an error. */
			if (!list || (*attempt >= (count = RSArrayGetCount(list)))) {
			
				if (!ctxt->_error.error) {
					ctxt->_error.error = EAI_NODATA;
					ctxt->_error.domain = kRSStreamErrorDomainNetDB;
				}
				
				/* Not needed anymore. */
				if (port) RSRelease(port);
				
				break;
			}
			
			/* Go through the list until a usable address is found */
			do {
				/* Create the address for connecting. */
				address = _RSDataCopyAddressByInjectingPort((RSDataRef)RSArrayGetValueAtIndex(list, *attempt), port);
				
				/* The next attempt will be the next address in the list. */
				*attempt = *attempt + 1;
				
				/* Only try to connect if there is an address */
				if (address) {
					
					/* If there was a socket, only need to connect it. */
					if (ctxt->_socket) {
						
						/*
						 ** If a socket was created previously, there is only one attempt
						 ** since the required socket type and protocol aren't known.
						 */
						*attempt = count;
						
						/* Start the connection. */
						_SocketStreamConnect_NoLock(ctxt, address);			
					}
					
					/* Try to create and start connecting to the address */
					else if (_SocketStreamCreateSocket_NoLock(ctxt, address))
						_SocketStreamConnect_NoLock(ctxt, address);
					
					/* No longer need the address */
					RSRelease(address);
					
					/* If succeeded in starting connect, don't continue anymore. */
					if (!ctxt->_error.error) {
						
						/* Not needed anymore. */
						if (port) RSRelease(port);

						return TRUE;				/* NOTE the early return here. */
					}
				}
			
			/* Continue through the list until all are exhausted. */
			} while (*attempt < count);
			
			/* Not needed anymore. */
			if (port) RSRelease(port);
			
			/*
			 ** It's an error to get to this point.  It means that none
			 ** of the addresses were suitable or worked.
			 */

			if (!ctxt->_error.error) {
				ctxt->_error.error = EINVAL;
				ctxt->_error.domain = kRSStreamErrorDomainPOSIX;
			}
				
			break;
		}
		
		/* If there is no lookup and no socket, something is bad. */
		else {
			
			int i;
			int yes = 1;
			RSOptionFlags flags;
			RSBOOLRef boolean;
			RSSocketNativeHandle s;		
			RSSocketContext c = {0, ctxt, NULL, NULL, NULL};
			RSArrayRef loops[3] = {ctxt->_readloops, ctxt->_writeloops, ctxt->_sharedloops};
			
			if (!ctxt->_socket) {
				
				/* Try to get the native socket for creation. */
				RSDataRef wrapper = (RSDataRef)RSDictionaryGetValue(ctxt->_properties, kRSStreamPropertySocketNativeHandle);
				
				if (!wrapper) {
					ctxt->_error.error = EINVAL;
					ctxt->_error.domain = kRSStreamErrorDomainPOSIX;
					break;
				}
				
				/* Create the RSSocket for riding. */
				ctxt->_socket = RSSocketCreateWithNative(RSGetAllocator(ctxt->_properties),
														 *((RSSocketNativeHandle*)RSDataGetBytePtr(wrapper)),
														 kSocketEvents,
														 (RSSocketCallBack)_SocketCallBack,
														 &c);
				
				if (!ctxt->_socket) {
					
					/*
					 ** Try to pull any error that may have just occurred.  If none,
					 ** assume an out of memory occurred.
					 */
					if (!_LastError(&ctxt->_error)) {
						ctxt->_error.error = ENOMEM;
						ctxt->_error.domain = kRSStreamErrorDomainPOSIX;
					}
					
					break;
				}
				
				/* Remove the cached value so it's only created when the client asks for it. */
				RSDictionaryRemoveValue(ctxt->_properties, kRSStreamPropertySocketNativeHandle);
			}
			
			/*
			** No host lookup and a socket means that the streams were
			** created with a connected socket already.
			*/

			__RSBitSet(ctxt->_flags, kFlagBitOpenComplete);
			__RSBitClear(ctxt->_flags, kFlagBitPollOpen);

			/* Get the native socket for setting options. */
			s = RSSocketGetNative(ctxt->_socket);
			
#if !defined(__WIN32)
			/* Turn off SIGPIPE on the socket (SIGPIPE doesn't exist on WIN32) */
			setsockopt(s, SOL_SOCKET, SO_NOSIGPIPE, (void*)&yes, sizeof(yes));
#endif
			
			/* Place the socket in nonblocking mode. */
			ioctl(s, FIONBIO, (void*)&yes);
			
			/* Get the current socket flags and turn off the auto re-enable for reads and writes. */
			flags = RSSocketGetSocketFlags(ctxt->_socket) &
				~kRSSocketAutomaticallyReenableReadCallBack &
				~kRSSocketAutomaticallyReenableWriteCallBack;
			
			/* Find out if RSSocket should close the socket on invalidation. */
			boolean = (RSBOOLRef)RSDictionaryGetValue(ctxt->_properties, kRSStreamPropertyShouldCloseNativeSocket);
			
			/* Adjust the flags on the setting.  No value is the default which means to close. */
			if (!boolean || (boolean != kRSBOOLFalse))
				flags |= kRSSocketCloseOnInvalidate;
			else
				flags &= ~kRSSocketCloseOnInvalidate;
			
			/* Set up the correct flags and enable the callbacks. */
			RSSocketSetSocketFlags(ctxt->_socket, flags);
			RSSocketEnableCallBacks(ctxt->_socket, kRSSocketReadCallBack | kRSSocketWriteCallBack);
			
			/* Now schedule the socket on all loops and modes */
			for (i = 0; i < (sizeof(loops) / sizeof(loops[0])); i++)
				_RSTypeScheduleOnMultipleRunLoops(ctxt->_socket, loops[i]);
			
			/* Succeeded so make sure the socket is in the list of schedulables for future. */
			_SchedulablesAdd(ctxt->_schedulables, ctxt->_socket);
		}
		
		return TRUE;
		
	} while (0);
	
	return FALSE;
}


/* static */ BOOL
_SocketStreamCan(_RSSocketStreamContext* ctxt, RSTypeRef stream, int test, RSStringRef mode, RSStreamError* error) {
	
	BOOL result = TRUE;
	BOOL isRead;
	
	/* No error to  start. */
	memset(error, 0, sizeof(error[0]));
	
	/* Lock down the context */
	RSSpinLockLock(&ctxt->_lock);
	
	isRead = RSReadStreamGetTypeID() == RSGetTypeID(stream);
	
	result = __RSBitIsSet(ctxt->_flags, test);
	
	/* If not already been signalled, need to find out. */
	if (!ctxt->_error.error && !result) {
		
		RSMutableArrayRef loops = isRead ? ctxt->_readloops : ctxt->_writeloops;
		
		if (!__RSBitIsSet(ctxt->_flags, test + kFlagBitPollOpen) &&
			((RSArrayGetCount(ctxt->_sharedloops) + RSArrayGetCount(loops)) > 2))
		{
			__RSBitSet(ctxt->_flags, test + kFlagBitPollOpen);
		}
		
		else {

			RSTypeRef loopAndMode[2] = {RSRunLoopGetCurrent(), mode};

			/* Add the current loop and the private mode to the list */
			_SchedulesAddRunLoopAndMode(loops, (RSRunLoopRef)loopAndMode[0], (RSStringRef)loopAndMode[1]);
			
			if (ctxt->_socket &&
				(RSArrayGetCount(ctxt->_schedulables) == 1) &&
				(ctxt->_socket == RSArrayGetValueAtIndex(ctxt->_schedulables, 0)))
			{
				RSRunLoopSourceRef src = RSSocketCreateRunLoopSource(RSGetAllocator(ctxt->_schedulables), ctxt->_socket, 0);
				if (src) {
					RSRunLoopAddSource((RSRunLoopRef)loopAndMode[0], src, (RSStringRef)loopAndMode[1]);
					RSRelease(src);
				}
			}
			
			else {
				/* Make sure to schedule all the schedulables on this loop and mode. */
				RSArrayApplyFunction(ctxt->_schedulables,
									 RSRangeMake(0, RSArrayGetCount(ctxt->_schedulables)),
									 (RSArrayApplierFunction)_SchedulablesScheduleApplierFunction,
									 loopAndMode);
			}
			
			/* Unlock the context to allow things to fire */
			RSSpinLockUnlock(&ctxt->_lock);
			
			/* Run the run loop for a poll (0.0) */
			RSRunLoopRunInMode(mode, 0.0, FALSE);
			
			/* Lock the context back up. */
			RSSpinLockLock(&ctxt->_lock);
			
			if (ctxt->_socket &&
				(RSArrayGetCount(ctxt->_schedulables) == 1) &&
				(ctxt->_socket == RSArrayGetValueAtIndex(ctxt->_schedulables, 0)))
			{
				RSRunLoopSourceRef src = RSSocketCreateRunLoopSource(RSGetAllocator(ctxt->_schedulables), ctxt->_socket, 0);
				if (src) {
					RSRunLoopRemoveSource((RSRunLoopRef)loopAndMode[0], src, (RSStringRef)loopAndMode[1]);
					RSRelease(src);
				}
			}

			else {
				/* Make sure to unschedule all the schedulables on this loop and mode. */
				RSArrayApplyFunction(ctxt->_schedulables,
									 RSRangeMake(0, RSArrayGetCount(ctxt->_schedulables)),
									 (RSArrayApplierFunction)_SchedulablesUnscheduleApplierFunction,
									 loopAndMode);
			}
			
			/* Remove this loop and private mode from the list. */
			_SchedulesRemoveRunLoopAndMode(loops, (RSRunLoopRef)loopAndMode[0], (RSStringRef)loopAndMode[1]);
			
			result = __RSBitIsSet(ctxt->_flags, test);
		}
	}
	
	/* If there was an error, make sure to signal it. */
	if (ctxt->_error.error) {
		
		/* Attempt to get the count of buffered bytes. */
		RSDataRef c = (RSDataRef)RSDictionaryGetValue(ctxt->_properties, _kRSStreamPropertyRecvBufferCount);
		
		/* Copy the error. */
		memmove(error, &ctxt->_error, sizeof(error[0]));
		
		/* It's set now. */
		__RSBitSet(ctxt->_flags, test);
		
		result = TRUE;
		
		/*
		** 2998408 Force async callback for errors so there is no worry about the
		** context going bad underneath callers of this function.
		*/
		
		/*
		** 3863115 If there is a client stream and it's been opened, signal the
		** error, but only if there are no bytes in the buffer.
		*/
		if ((!c || !*((RSIndex*)RSDataGetBytePtr(c))) &&
			(ctxt->_clientReadStream && __RSBitIsSet(ctxt->_flags, kFlagBitReadStreamOpened)))
		{
			_RSReadStreamSignalEventDelayed(ctxt->_clientReadStream, kRSStreamEventErrorOccurred, error);
		}
		
		/* If there is a client stream and it's been opened, signal the error. */
		if (ctxt->_clientWriteStream && __RSBitIsSet(ctxt->_flags, kFlagBitWriteStreamOpened))
			_RSWriteStreamSignalEventDelayed(ctxt->_clientWriteStream, kRSStreamEventErrorOccurred, error);
	}
	
	/* Unlock */
	RSSpinLockUnlock(&ctxt->_lock);
	
	return result;
}


/* static */ void
_SocketStreamAddReachability_NoLock(_RSSocketStreamContext* ctxt) {
	
	SCNetworkReachabilityRef reachability = (SCNetworkReachabilityRef)RSDictionaryGetValue(ctxt->_properties, _kRSStreamPropertyNetworkReachability);
	
	/* There has to be an open socket and no reachibility item already. */
	if (ctxt->_socket && __RSBitIsSet(ctxt->_flags, kFlagBitOpenComplete) && !reachability) {

		/* Copy the addresses for the pipe. */
		RSDataRef localAddr = RSSocketCopyAddress(ctxt->_socket);
		RSDataRef peerAddr = RSSocketCopyPeerAddress(ctxt->_socket);
		
		if (localAddr && peerAddr) {
			
			/* Create the reachability object to watch the route. */
			SCNetworkReachabilityRef reachability = SCNetworkReachabilityCreateWithAddressPair(RSGetAllocator(ctxt->_properties),
																							   (const struct sockaddr*)RSDataGetBytePtr(localAddr),
																							   (const struct sockaddr*)RSDataGetBytePtr(peerAddr));
																					
			if (reachability) {
				int i;
				SCNetworkReachabilityContext c = {0, ctxt, NULL, NULL, NULL};
				RSArrayRef loops[3] = {ctxt->_readloops, ctxt->_writeloops, ctxt->_sharedloops};
				
				/* Add it to the properties. */
				RSDictionaryAddValue(ctxt->_properties, _kRSStreamPropertyNetworkReachability, reachability);
				
				/* Set the callback */
				SCNetworkReachabilitySetCallback(reachability, (SCNetworkReachabilityCallBack)_ReachabilityCallBack, &c);
				
				/* Schedule it on all the loops and modes. */
				for (i = 0; i < (sizeof(loops) / sizeof(loops[0])); i++)
					_RSTypeScheduleOnMultipleRunLoops(reachability, loops[i]);
				
				/* Add it to the schedulables. */
				_SchedulablesAdd(ctxt->_schedulables, reachability);
				
				/* Schedulables and properties hold it now. */
				RSRelease(reachability);
			}
		}
		
		/* Don't need these anymore. */
		if (localAddr) RSRelease(localAddr);
		if (peerAddr) RSRelease(peerAddr);
	}
}


/* static */ void
_SocketStreamRemoveReachability_NoLock(_RSSocketStreamContext* ctxt) {
	
	/* Find out if there is a reachability already. */
	SCNetworkReachabilityRef reachability = (SCNetworkReachabilityRef)RSDictionaryGetValue(ctxt->_properties, _kRSStreamPropertyNetworkReachability);

	if (reachability) {
	
		int i;
		RSArrayRef loops[3] = {ctxt->_readloops, ctxt->_writeloops, ctxt->_sharedloops};
		
		/* Invalidate the reachability; never to be used again. */
		_RSTypeInvalidate(reachability);
		
		/* Unschedule it from all the loops and modes. */
		for (i = 0; i < (sizeof(loops) / sizeof(loops[0])); i++)
			_RSTypeUnscheduleFromMultipleRunLoops(reachability, loops[i]);
		
		/* Remove the socket from the schedulables. */
		_SchedulablesRemove(ctxt->_schedulables, reachability);
		
		/* Remove it from the properties */
		RSDictionaryRemoveValue(ctxt->_properties, reachability);
	}
}


/* static */ RSIndex
_SocketStreamBufferedRead_NoLock(_RSSocketStreamContext* ctxt, UInt8* buffer, RSIndex length) {
	
	RSIndex result = 0;
	
	/* Different bits required for "buffered reading." */
	RSNumberRef s = (RSNumberRef)RSDictionaryGetValue(ctxt->_properties, _kRSStreamPropertyRecvBufferSize);
	RSMutableDataRef b = (RSMutableDataRef)RSDictionaryGetValue(ctxt->_properties, _kRSStreamPropertyRecvBuffer);
	RSMutableDataRef c = (RSMutableDataRef)RSDictionaryGetValue(ctxt->_properties, _kRSStreamPropertyRecvBufferCount);
	
	/* All have to be availbe to read. */
	if (b && c && s) {
		
		RSIndex* i = (RSIndex*)RSDataGetMutableBytePtr(c);
		UInt8* ptr = (UInt8*)RSDataGetMutableBytePtr(b);
		RSIndex max;
		
		RSNumberGetValue(s, kRSNumberRSIndexType, &max);
		
		/* Either read all the bytes or just what the client asked. */
		result = (*i < length) ? *i : length;
		*i = *i - result;
		
		/* Copy the bytes into the client buffer */
		memmove(buffer, ptr, result);
		
		/* Move down the bytes in the local buffer. */
		memmove(ptr, ptr + result, *i);
		
		/* Zero the bytes at the end of the local buffer */
		memset(ptr + *i, 0, max - *i);
		
		/* If the local buffer is empty, pump SSL along. */
		if (__RSBitIsSet(ctxt->_flags, kFlagBitUseSSL) && (*i == 0)) {
			_SocketStreamSecurityBufferedRead_NoLock(ctxt);
		}
	}
	
	/* If no bytes read and the pipe isn't closed, constitute an EAGAIN */
	if (!result) {
		if (!ctxt->_error.error && !__RSBitIsSet(ctxt->_flags, kFlagBitClosed)) {
			ctxt->_error.error = EAGAIN;
			ctxt->_error.domain = kRSStreamErrorDomainPOSIX;
		}
	}
	else if (__RSBitIsSet(ctxt->_flags, kFlagBitRecvdRead)) {
		__RSBitClear(ctxt->_flags, kFlagBitRecvdRead);
		if (ctxt->_socket)
			RSSocketEnableCallBacks(ctxt->_socket, kRSSocketReadCallBack);
	}
	
	return result;
}


/* static */ void
_SocketStreamBufferedSocketRead_NoLock(_RSSocketStreamContext* ctxt) {
	
	RSIndex* i;
	RSIndex s = kRecvBufferSize;
	
	/* Get the bits required in order to work with the buffer. */
	RSNumberRef size = (RSNumberRef)RSDictionaryGetValue(ctxt->_properties, _kRSStreamPropertyRecvBufferSize);
	RSMutableDataRef buffer = (RSMutableDataRef)RSDictionaryGetValue(ctxt->_properties, _kRSStreamPropertyRecvBuffer);
	RSMutableDataRef count = (RSMutableDataRef)RSDictionaryGetValue(ctxt->_properties, _kRSStreamPropertyRecvBufferCount);
	
	/* No buffer assumes all are missing. */
	if (!buffer) {
		
		RSAllocatorRef alloc = RSGetAllocator(ctxt->_properties);
		
		/* If no size, assume a default.  Can be overridden by properties. */
		if (!size)
			size = RSNumberCreate(alloc, kRSNumberRSIndexType, &s);
		else
			RSNumberGetValue(size, kRSNumberRSIndexType, &s);

		/* Create the backing for the buffer and the counter. */
		if (size) {
			buffer = RSDataCreateMutable(alloc, s);
			count = RSDataCreateMutable(alloc, sizeof(RSIndex));
		}
		
		/* If anything failed, set out of memory and bail. */
		if (!buffer || !count || !size) {
			
			if (buffer) RSRelease(buffer);
			if (count) RSRelease(count);
			if (size) RSRelease(size);
			
			ctxt->_error.error = ENOMEM;
			ctxt->_error.domain = kRSStreamErrorDomainPOSIX;
			
			return;								/* NOTE the eary return. */
		}
		
		/* Save the buffer information. */
		RSDictionarySetValue(ctxt->_properties, _kRSStreamPropertyRecvBufferSize, size);
		RSDictionarySetValue(ctxt->_properties, _kRSStreamPropertyRecvBuffer, buffer);
		RSDictionarySetValue(ctxt->_properties, _kRSStreamPropertyRecvBufferCount, count);
		
		RSRelease(size);
		RSRelease(buffer);
		RSRelease(count);
		
		/* Start with a zero byte count. */
		*((RSIndex*)RSDataGetMutableBytePtr(count)) = 0;
	}
	
	/* Get the count and size of the buffer, respectively. */
	i = (RSIndex*)RSDataGetMutableBytePtr(count);
	RSNumberGetValue(size, kRSNumberRSIndexType, &s);
	
	/* Only read if there is room in the buffer. */
	if (*i < s) {
		
		UInt8* ptr = (UInt8*)RSDataGetMutableBytePtr(buffer);		
		RSIndex bytesRead = _RSSocketRecv(ctxt->_socket, ptr + *i, s - *i, &ctxt->_error);

		__RSBitClear(ctxt->_flags, kFlagBitRecvdRead);

		/* If did read bytes, increase the count. */
		if (bytesRead > 0) {
			*i = *i + bytesRead;
			RSSocketEnableCallBacks(ctxt->_socket, kRSSocketReadCallBack);
			__RSBitSet(ctxt->_flags, kFlagBitCanRead);
			__RSBitClear(ctxt->_flags, kFlagBitPollRead);
		}
		
		else if (bytesRead == 0) {
			__RSBitSet(ctxt->_flags, kFlagBitClosed);
			__RSBitSet(ctxt->_flags, kFlagBitCanRead);
			__RSBitClear(ctxt->_flags, kFlagBitPollRead);
		}
	}
	else
		__RSBitSet(ctxt->_flags, kFlagBitRecvdRead);
}


/* static */ RSComparisonResult
_OrderHandshakes(_RSSocketStreamPerformHandshakeCallBack fn1, _RSSocketStreamPerformHandshakeCallBack fn2, void* context) {
	
	if (!fn1) return kRSCompareGreaterThan;
	if (!fn2) return kRSCompareLessThan;
	
	/*
	** Order of handshakes in increasing priority:
	**
	** 1) SOCKSv5 \__ Conceivably should not be set at the same time.
	** 2) SOCKSv4 /
	** 3) SOCKSv5 user/pass negotiation
	** 3) SOCKSv5 postamble
	** 4) CONNECT halt <- Used to halt the stream for another CONNECT
	** 5) CONNECT
	** 6) SSL send <- could be required for #4
	** 7) SSL
	*/
	
	if (*fn1 == _PerformSOCKSv5Handshake_NoLock) return kRSCompareLessThan;
	if (*fn2 == _PerformSOCKSv5Handshake_NoLock) return kRSCompareGreaterThan;
	
	if (*fn1 == _PerformSOCKSv5UserPassHandshake_NoLock) return kRSCompareLessThan;
	if (*fn2 == _PerformSOCKSv5UserPassHandshake_NoLock) return kRSCompareGreaterThan;

	if (*fn1 == _PerformSOCKSv5PostambleHandshake_NoLock) return kRSCompareLessThan;
	if (*fn2 == _PerformSOCKSv5PostambleHandshake_NoLock) return kRSCompareGreaterThan;

	if (*fn1 == _PerformSOCKSv4Handshake_NoLock) return kRSCompareLessThan;
	if (*fn2 == _PerformSOCKSv4Handshake_NoLock) return kRSCompareGreaterThan;
	
	if (*fn1 == _PerformCONNECTHaltHandshake_NoLock) return kRSCompareLessThan;
	if (*fn2 == _PerformCONNECTHaltHandshake_NoLock) return kRSCompareGreaterThan;
	
	if (*fn1 == _PerformCONNECTHandshake_NoLock) return kRSCompareLessThan;
	if (*fn2 == _PerformCONNECTHandshake_NoLock) return kRSCompareGreaterThan;
	
	if (*fn1 == _PerformSecuritySendHandshake_NoLock) return kRSCompareLessThan;
	if (*fn2 == _PerformSecuritySendHandshake_NoLock) return kRSCompareGreaterThan;
	
	if (*fn1 == _PerformSecurityHandshake_NoLock) return kRSCompareLessThan;
	if (*fn2 == _PerformSecurityHandshake_NoLock) return kRSCompareGreaterThan;
	
	return kRSCompareEqualTo;
}


/* static */ BOOL
_SocketStreamAddHandshake_NoLock(_RSSocketStreamContext* ctxt, _RSSocketStreamPerformHandshakeCallBack fn) {
	
	RSIndex i;
	
	/* Get the existing list of handshakes. */
	RSMutableArrayRef handshakes = (RSMutableArrayRef)RSDictionaryGetValue(ctxt->_properties, _kRSStreamPropertyHandshakes);
	
	/* If there is no list, need to create one. */
	if (!handshakes) {
		
		RSArrayCallBacks cb = {0, NULL, NULL, NULL, NULL};
		
		/* Create the list of handshakes. */
		handshakes = RSArrayCreateMutable(RSGetAllocator(ctxt->_properties), 0, &cb);
		
		if (!handshakes)
			return FALSE;
		
		/* Add the list to the properties for later work. */
		RSDictionaryAddValue(ctxt->_properties, _kRSStreamPropertyHandshakes, handshakes);
		__RSBitSet(ctxt->_flags, kFlagBitHasHandshakes);
		
		RSRelease(handshakes);
	}
	
	/* Find out if the handshake is in the list already. */
	i = RSArrayGetFirstIndexOfValue(handshakes, RSRangeMake(0, RSArrayGetCount(handshakes)), fn);
	
	/* Need to add it? */
	if (i == kRSNotFound) {
		
		RSRange r;
		
		/* Add the new handshake to the list. */
		RSArrayAppendValue(handshakes, fn);
		
		r = RSRangeMake(0, RSArrayGetCount(handshakes));
		
		/* Make sure to order the list of handshakes correctly */
		RSArraySortValues(handshakes, r, (RSComparatorFunction)_OrderHandshakes, NULL);
		
		/* Update the location. */
		i = RSArrayGetFirstIndexOfValue(handshakes, r, fn);
	}
	
	if (!i) {
		
		__RSBitClear(ctxt->_flags, kFlagBitCanRead);
		__RSBitClear(ctxt->_flags, kFlagBitCanWrite);
		
		__RSBitClear(ctxt->_flags, kFlagBitRecvdRead);
		
		/* Make sure all callbacks are set up for handshaking. */
		if (ctxt->_socket && __RSBitIsSet(ctxt->_flags, kFlagBitOpenComplete))
			RSSocketEnableCallBacks(ctxt->_socket, kRSSocketReadCallBack | kRSSocketWriteCallBack);
	}
		
	return TRUE;
}


/* static */ void
_SocketStreamRemoveHandshake_NoLock(_RSSocketStreamContext* ctxt, _RSSocketStreamPerformHandshakeCallBack fn) {

	/* Get the existing list of handshakes. */
	RSMutableArrayRef handshakes = (RSMutableArrayRef)RSDictionaryGetValue(ctxt->_properties, _kRSStreamPropertyHandshakes);

	if (handshakes) {
		
		/* Find out if the handshake is in the list. */
		RSIndex i = RSArrayGetFirstIndexOfValue(handshakes, RSRangeMake(0, RSArrayGetCount(handshakes)), fn);
		
		/* If it exists, need to remove it. */
		if (i != kRSNotFound)
			RSArrayRemoveValueAtIndex(handshakes, i);
		
		if (!RSArrayGetCount(handshakes)) {
			RSDictionaryRemoveValue(ctxt->_properties, _kRSStreamPropertyHandshakes);
			__RSBitClear(ctxt->_flags, kFlagBitHasHandshakes);
		}
	}
	
	/* Need to reset the flags as a result of a handshake removal. */
	__RSBitClear(ctxt->_flags, kFlagBitCanRead);
	__RSBitClear(ctxt->_flags, kFlagBitCanWrite);
	
	__RSBitClear(ctxt->_flags, kFlagBitRecvdRead);
			
	/* Make sure all callbacks are reset if open. */
	if (ctxt->_socket && __RSBitIsSet(ctxt->_flags, kFlagBitOpenComplete)) {
		
		if (!__RSBitIsSet(ctxt->_flags, kFlagBitHasHandshakes) && __RSBitIsSet(ctxt->_flags, kFlagBitIsBuffered)) {

			RSStreamError error = ctxt->_error;
			
			/* Similar to the end of _SocketStreamRead. */
			RSDataRef c = (RSDataRef)RSDictionaryGetValue(ctxt->_properties, _kRSStreamPropertyRecvBufferCount);
			BOOL buffered = (c && *((RSIndex*)RSDataGetBytePtr(c)));
			
			if (buffered)
				memset(&error, 0, sizeof(error));
			
			/* Need to check for buffered bytes or EOF. */
			if (__RSBitIsSet(ctxt->_flags, kFlagBitClosed) || buffered) {
				
				if (ctxt->_clientReadStream && __RSBitIsSet(ctxt->_flags, kFlagBitReadStreamOpened))
					_RSReadStreamSignalEventDelayed(ctxt->_clientReadStream, kRSStreamEventHasBytesAvailable, &error);
			}
			
			/* If none there, check to see if there are encrypted bytes that are buffered. */
			else if (__RSBitIsSet(ctxt->_flags, kFlagBitUseSSL)) {
				
				_SocketStreamSecurityBufferedRead_NoLock(ctxt);
				
				if (__RSBitIsSet(ctxt->_flags, kFlagBitCanRead) || __RSBitIsSet(ctxt->_flags, kFlagBitClosed)) {
					
					if (ctxt->_clientReadStream && __RSBitIsSet(ctxt->_flags, kFlagBitReadStreamOpened)) {
						
						if (c && *((RSIndex*)RSDataGetBytePtr(c)))
							memset(&error, 0, sizeof(error));
						
						_RSReadStreamSignalEventDelayed(ctxt->_clientReadStream, kRSStreamEventHasBytesAvailable, &error);
					}
				}
			}
		}
		
		RSSocketEnableCallBacks(ctxt->_socket, kRSSocketReadCallBack | kRSSocketWriteCallBack);
	}
}
	

/* static */ void
_SocketStreamAttemptAutoVPN_NoLock(_RSSocketStreamContext* ctxt, RSStringRef name) {
	
	if (!__RSBitIsSet(ctxt->_flags, kFlagBitTriedVPN)) {
		
		RSTypeRef values[2] = {name, NULL};
		const RSStringRef keys[2] = {_kRSStreamAutoHostName, _kRSStreamPropertyAutoConnectPriority};
		RSAllocatorRef alloc = RSGetAllocator(ctxt->_properties);
		RSDictionaryRef options;
		
		/* Grab the intended value.  If none, give if "default." */
		values[1] = (RSStringRef)RSDictionaryGetValue(ctxt->_properties, keys[1]);
		if (!values[1])
			values[1] = _kRSStreamAutoVPNPriorityDefault;
		
		/* Create the dictionary of options for SC. */
		options = RSDictionaryCreate(alloc,
									 (const void **)keys,
									 (const void **)values,
									 sizeof(values) / sizeof(values[0]),
									 &kRSTypeDictionaryKeyCallBacks,
									 &kRSTypeDictionaryValueCallBacks);
		
		/* Mark the attempt, so another is not made. */
		__RSBitSet(ctxt->_flags, kFlagBitTriedVPN);
		
		/* No options = no memory. */
		if (!options) {
			ctxt->_error.error = ENOMEM;
			ctxt->_error.domain = kRSStreamErrorDomainPOSIX;
		}
		
		else {
			RSStringRef service_id = NULL;
			RSDictionaryRef user_options = NULL;

			/* Create the service id and settings. */
			if (SCNetworkConnectionCopyUserPreferences(options, &service_id, &user_options)) {
				
				SCNetworkConnectionContext c = {0, ctxt, NULL, NULL, NULL};
				
				/* Create the connection for auto connect. */
				SCNetworkConnectionRef conn = SCNetworkConnectionCreateWithServiceID(alloc,
																					 service_id,
																					 (SCNetworkConnectionCallBack)_NetworkConnectionCallBack,
																					 &c);
				
				/* Did it create? */
				if (conn) {

					int i;
					RSArrayRef loops[3] = {ctxt->_readloops, ctxt->_writeloops, ctxt->_sharedloops};
					
					/* Now schedule the connection on all loops and modes */
					for (i = 0; i < (sizeof(loops) / sizeof(loops[0])); i++)
						_RSTypeScheduleOnMultipleRunLoops(conn, loops[i]);
					
					if (SCNetworkConnectionStart(conn, user_options, TRUE)) {
						
						memset(&ctxt->_error, 0, sizeof(ctxt->_error));
						
						/* Succeeded so make sure the connection is in the list of schedulables for future. */
						_SchedulablesAdd(ctxt->_schedulables, conn);
					}
					
					else {
						
						/* Remove the connection from all the schedules. */
						for (i = 0; i < (sizeof(loops) / sizeof(loops[0])); i++)
							_RSTypeUnscheduleFromMultipleRunLoops(conn, loops[i]);
						
						/* Invalidate the connection; never to be used again. */
						_RSTypeInvalidate(conn);
					}
				}
				
				/* Clean up. */
				if (conn) RSRelease(conn);
			}
			
			/* Clean up. */
			RSRelease(options);
			if (service_id) RSRelease(service_id);
			if (user_options) RSRelease(user_options);
		}
	}
}


/* static */ void
_SocketStreamPerformCancel(void* info) {
	(void)info;  /* unused */
}


#if 0
#pragma mark *SOCKS Support
#endif

#define kSOCKSv4BufferMaximum	((RSIndex)(8L))
#define kSOCKSv5BufferMaximum	((RSIndex)(2L))

/* static */ void
_PerformSOCKSv5Handshake_NoLock(_RSSocketStreamContext* ctxt) {
	
	do {
		/* Get the buffer of stuff to send */
		RSMutableDataRef to_send = (RSMutableDataRef)RSDictionaryGetValue(ctxt->_properties,
																		  _kRSStreamPropertySOCKSSendBuffer);
		RSMutableDataRef to_recv = (RSMutableDataRef)RSDictionaryGetValue(ctxt->_properties,
																		  _kRSStreamPropertySOCKSRecvBuffer);
		
		if (!to_recv) {
			
			RSStreamError error = {0, 0};
			RSIndex length, sent;
				
			if (!to_send) {
				
				UInt8* ptr;
				
				/* Get the user/pass to determine how many methods are supported. */
				RSDictionaryRef proxy = (RSDictionaryRef)RSDictionaryGetValue(ctxt->_properties, kRSStreamPropertySOCKSProxy);
				RSStringRef user = (RSStringRef)RSDictionaryGetValue(proxy, kRSStreamPropertySOCKSUser);
				RSStringRef pass = (RSStringRef)RSDictionaryGetValue(proxy, kRSStreamPropertySOCKSPassword);
				
				/* Create the 4 byte buffer for the intial connect. */
				to_send = RSDataCreateMutable(RSGetAllocator(ctxt->_properties), 4);
				
				/* Couldn't create so error out on no memory. */
				if (!to_send) {
					ctxt->_error.error = ENOMEM;
					ctxt->_error.domain = kRSStreamErrorDomainPOSIX;
					break;
				}
				
				/* Make sure to save the buffer for later. */
				RSDictionarySetValue(ctxt->_properties, _kRSStreamPropertySOCKSSendBuffer, to_send);
				RSRelease(to_send);
								
				/* Get the local pointer to set the values. */
				ptr = RSDataGetMutableBytePtr(to_send);
				RSDataSetLength(to_send, 4);
				
				/* By default, perform only 1 method (no authentication). */
				ptr[0] = 0x05; ptr[1] = 0x01;
				ptr[2] = 0x00; ptr[3] = 0x02;
		
				/* If there is a valid user and pass, indicate willing to do two methods. */
				if (user && RSStringGetLength(user) && pass && RSStringGetLength(pass))
					ptr[1] = 0x02;
				else
					RSDataSetLength(to_send, 3);
			}
			
			/* Try sending out the bytes. */
			length = RSDataGetLength(to_send);
			sent = _RSSocketSend(ctxt->_socket, RSDataGetBytePtr(to_send), length, &error);
			
			/* If sent everything, dump the buffer. */
			if (sent == length) {
				RSDictionaryRemoveValue(ctxt->_properties, _kRSStreamPropertySOCKSSendBuffer);
				
				/* Create the buffer for receive. */
				to_recv = RSDataCreateMutable(RSGetAllocator(ctxt->_properties), 2);
				
				/* Fail so error on no memory. */
				if (!to_recv) {
					ctxt->_error.error = ENOMEM;
					ctxt->_error.domain = kRSStreamErrorDomainPOSIX;
					break;
				}
				
				/* Make sure to save the buffer for later. */
				RSDictionarySetValue(ctxt->_properties, _kRSStreamPropertySOCKSRecvBuffer, to_recv);
				RSRelease(to_recv);
			}
			
			/* If couldn't send everything, trim the buffer. */
			else if (sent > 0) {
				
				UInt8* ptr = RSDataGetMutableBytePtr(to_send);
				
				/* New length */
				length -= sent;
				
				/* Move the bytes down in the buffer. */
				memmove(ptr, ptr + sent, length);
				
				/* Trim it. */
				RSDataSetLength(to_send, length);
				
				/* Re-enable so the rest can be written later. */
				RSSocketEnableCallBacks(ctxt->_socket, kRSSocketWriteCallBack);
			}
			
			/* If got an error other than EAGAIN, set the error in the context. */
			else if ((error.error != EAGAIN) || (error.domain != kRSStreamErrorDomainPOSIX))
				memmove(&ctxt->_error, &error, sizeof(error));
		}
		
		else {
			
			UInt8* ptr = RSDataGetMutableBytePtr(to_recv);
			RSIndex length = RSDataGetLength(to_recv);
			
			if (length != kSOCKSv5BufferMaximum) {
				
				RSStreamError error = {0, 0};
				RSIndex recvd = _RSSocketRecv(ctxt->_socket, ptr + length, kSOCKSv5BufferMaximum - length, &error);
				
				/* If read 0 bytes, this is an early close from the other side. */
				if (recvd == 0) {
					
					/* Mark as not connected. */
					ctxt->_error.error = ENOTCONN;
					ctxt->_error.domain = _kRSStreamErrorDomainNativeSockets;
				}
				
				/* Successfully read? */
				else if (recvd > 0) {
					
					UInt8 tmp[kSOCKSv5BufferMaximum];
					
					/* Set the length of the buffer. */
					length += recvd;
					
					/* RS is so kind as to zero the bytes on SetLength, even though it's a fixed capacity. */
					memmove(tmp, ptr, length);
					
					/* Set the length of the buffer. */
					RSDataSetLength(to_recv, length);
					
					/* Put the bytes back. */
					memmove(ptr, tmp, length);
					
					/* Re-enable after performing a successful read. */
					RSSocketEnableCallBacks(ctxt->_socket, kRSSocketReadCallBack);
				}
				
				/* If got an error other than EAGAIN, set the error in the context. */
				else if ((error.error != EAGAIN) || (error.domain != kRSStreamErrorDomainPOSIX))
					memmove(&ctxt->_error, &error, sizeof(error));
			}
			
			/* Is there enough now? */
			if (length == kSOCKSv5BufferMaximum) {
				
				switch (ptr[1]) {
					
					case 0x00:
						/* Don't need to do anything for "No Authentication Required." */
						RSDictionaryRemoveValue(ctxt->_properties, _kRSStreamPropertySOCKSRecvBuffer);
						_SocketStreamAddHandshake_NoLock(ctxt, _PerformSOCKSv5PostambleHandshake_NoLock);
						_SocketStreamRemoveHandshake_NoLock(ctxt, _PerformSOCKSv5Handshake_NoLock);
						break;
						
						/* **FIXME** Add GSS API support (0x01) */
						
					case 0x02:
					{
						RSDictionaryRef proxy = (RSDictionaryRef)RSDictionaryGetValue(ctxt->_properties, kRSStreamPropertySOCKSProxy);
						RSStringRef user = (RSStringRef)RSDictionaryGetValue(proxy, kRSStreamPropertySOCKSUser);
						RSStringRef pass = (RSStringRef)RSDictionaryGetValue(proxy, kRSStreamPropertySOCKSPassword);
						
						RSDictionaryRemoveValue(ctxt->_properties, _kRSStreamPropertySOCKSRecvBuffer);
						
						if (user && pass) {
							_SocketStreamAddHandshake_NoLock(ctxt, _PerformSOCKSv5UserPassHandshake_NoLock);
							_SocketStreamAddHandshake_NoLock(ctxt, _PerformSOCKSv5PostambleHandshake_NoLock);
							_SocketStreamRemoveHandshake_NoLock(ctxt, _PerformSOCKSv5Handshake_NoLock);
						}
						
						else {
							ctxt->_error.domain = kRSStreamErrorDomainSOCKS;
							ctxt->_error.error = ((kRSStreamErrorSOCKS5SubDomainMethod << 16) | (ptr[1] & 0x000000FF));
						}
						
						break;
					}
						
						/* **FIXME** Add CHAP support (0x03) */
						
					default:
						ctxt->_error.domain = kRSStreamErrorDomainSOCKS;
						ctxt->_error.error = ((kRSStreamErrorSOCKS5SubDomainMethod << 16) | (ptr[1] & 0x000000FF));
						break;
				}
			}
		}
	} while (0);
	
	/* If there was an error remove the handshake. */
	if (ctxt->_error.error)
		_SocketStreamRemoveHandshake_NoLock(ctxt, _PerformSOCKSv5Handshake_NoLock);
}


/* static */ void
_PerformSOCKSv5PostambleHandshake_NoLock(_RSSocketStreamContext* ctxt) {
	
	do {
		/* Get the buffer of stuff to send */
		RSMutableDataRef to_send = (RSMutableDataRef)RSDictionaryGetValue(ctxt->_properties,
																		  _kRSStreamPropertySOCKSSendBuffer);
		RSMutableDataRef to_recv = (RSMutableDataRef)RSDictionaryGetValue(ctxt->_properties,
																		  _kRSStreamPropertySOCKSRecvBuffer);
		
		if (!to_recv) {
			
			RSStreamError error = {0, 0};
			RSIndex length, sent;
			
			if (!to_send) {
				
				UInt8* ptr;
				SInt32 value = 0;
				unsigned short prt;

				/* Try to get the name for sending in the CONNECT request. */
				RSHostRef host = (RSHostRef)RSDictionaryGetValue(ctxt->_properties, kRSStreamPropertySocketRemoteHost);
				RSNumberRef port = (RSNumberRef)RSDictionaryGetValue(ctxt->_properties, _kRSStreamPropertySocketRemotePort);
				RSArrayRef list = RSHostGetNames(host, NULL);
				RSStringRef name = (list && RSArrayGetCount(list)) ? (RSStringRef)RSArrayGetValueAtIndex(list, 0) : NULL;
				
				if (name)
					RSRetain(name);
				else {
					
					/* Is there one good address to create a name? */
					list = RSHostGetAddressing(host, NULL);
					if (list && RSArrayGetCount(list)) {
						name = _RSNetworkRSStringCreateWithRSDataAddress(RSGetAllocator(list),
																		 (RSDataRef)RSArrayGetValueAtIndex(list, 0));
					}
					
					/* Couldn't create so error out on no memory. */
					if (!name) {
						ctxt->_error.error = ENOMEM;
						ctxt->_error.domain = kRSStreamErrorDomainPOSIX;
						break;
					}
				}
				
				/* Create the buffer for the largest possible CONNECT request. */
				to_send = RSDataCreateMutable(RSGetAllocator(ctxt->_properties), 262);

				/* Couldn't create so error out on no memory. */
				if (!to_send) {
				  ctxt->_error.error = ENOMEM;
				  ctxt->_error.domain = kRSStreamErrorDomainPOSIX;
				  break;
				}
				
				/* Extend it all the way out for now; shorten later. */
				RSDataSetLength(to_send, 262);
				
				/* Get the local pointer to set the values. */
				ptr = RSDataGetMutableBytePtr(to_send);
				
				/* Make sure to save the buffer for later. */
				RSDictionarySetValue(ctxt->_properties, _kRSStreamPropertySOCKSSendBuffer, to_send);
				RSRelease(to_send);
				
				/* Place the name into the buffer. */
				RSStringGetPascalString(name, &(ptr[4]), 256, kRSStringEncodingUTF8);
				RSRelease(name);
				
				/* Place the header bytes. */
				ptr[0] = 0x05; ptr[1] = 0x01;
				ptr[2] = 0x00; ptr[3] = 0x03;
				
				/* Get the port value to lay down. */
				RSNumberGetValue(port, kRSNumberSInt32Type, &value);
				
				/* Lay down the port. */
				prt = htons(value & 0x0000FFFF);
				*((unsigned short*)(&ptr[ptr[4] + 5])) = prt;
				
				/* Trim down the buffer to the correct size. */
				RSDataSetLength(to_send, 7 + ptr[4]);
			}
			
			/* Try sending out the bytes. */
			length = RSDataGetLength(to_send);
			sent = _RSSocketSend(ctxt->_socket, RSDataGetBytePtr(to_send), length, &error);
			
			/* If sent everything, dump the buffer. */
			if (sent == length) {
				RSDictionaryRemoveValue(ctxt->_properties, _kRSStreamPropertySOCKSSendBuffer);
				
				/* Create the buffer for receive. */
				to_recv = RSDataCreateMutable(RSGetAllocator(ctxt->_properties), 262);
				
				/* Fail so error on no memory. */
				if (!to_recv) {
					ctxt->_error.error = ENOMEM;
					ctxt->_error.domain = kRSStreamErrorDomainPOSIX;
					break;
				}
				
				/* Make sure to save the buffer for later. */
				RSDictionarySetValue(ctxt->_properties, _kRSStreamPropertySOCKSRecvBuffer, to_recv);
				RSRelease(to_recv);
			}
			
			/* If couldn't send everything, trim the buffer. */
			else if (sent > 0) {
				
				UInt8* ptr = RSDataGetMutableBytePtr(to_send);
				
				/* New length */
				length -= sent;
				
				/* Move the bytes down in the buffer. */
				memmove(ptr, ptr + sent, length);
				
				/* Trim it. */
				RSDataSetLength(to_send, length);
				
				/* Re-enable so the rest can be written later. */
				RSSocketEnableCallBacks(ctxt->_socket, kRSSocketWriteCallBack);
			}
			
			/* If got an error other than EAGAIN, set the error in the context. */
			else if ((error.error != EAGAIN) || (error.domain != kRSStreamErrorDomainPOSIX))
				memmove(&ctxt->_error, &error, sizeof(error));
		}
		
		else {
						
			RSStreamError error = {0, 0};
			UInt8* ptr = RSDataGetMutableBytePtr(to_recv);
			RSIndex length = RSDataGetLength(to_recv);
			RSIndex recvd = 0;
			
			/* Go for the initial return code. */
			if (length < 2)
				recvd = _RSSocketRecv(ctxt->_socket, ptr + length, 2 - length, &error);
					
			/* Add the read bytes if successful */
			length += (recvd > 0) ? recvd : 0;
			
			/* Continue on if things are good. */
			if (!error.error && (length >= 2)) {
				
				/* Make sure the header starts correctly.  Fail if not. */
				if ((ptr[0] != 5) || ptr[1]) {
					ctxt->_error.domain = kRSStreamErrorDomainSOCKS;
					ctxt->_error.error = ((kRSStreamErrorSOCKS5SubDomainResponse << 16) | (((ptr[0] != 5) ? -1 : ptr[1]) & 0x000000FF));
					break;
				}
				
				else {
					
					/* Go for as many bytes as the smallest result packet. */
					if (length < 8)
						recvd = _RSSocketRecv(ctxt->_socket, ptr + length, 8 - length, &error);
					
					/* Add the read bytes if successful */
					length += (recvd > 0) ? recvd : 0;
					
					/* Can continue so long as result type and length byte are there. */
					if (!error.error && (length >= 5)) {
						
						RSIndex intended = 0;
						
						/* Check the address type. */
						switch (ptr[3]) {
							
							/* IPv4 type */
							case 0x01:
								/* Bail if 10 bytes have been read. */
								intended = 10;
								break;
								
								/* Domain name type */
							case 0x03:
								/* Bail if the 7 bytes plus domain name length have been read. */
								intended = 7 + ptr[4];
								break;
								
								/* IPv6 type */
							case 0x04:
								/* Bail if 22 bytes have been read. */
								intended = 22;
								break;
								
								/* Got crap data so bail.*/
							default:
								ctxt->_error.domain = kRSStreamErrorDomainSOCKS;
								ctxt->_error.error = ((kRSStreamErrorSOCKS5SubDomainResponse << 16) | kRSStreamErrorSOCKS5BadResponseAddr);
								break;
						}
						
						/* Not an understood resut, so bail. */
						if (ctxt->_error.error)
							break;
						
						/* If haven't read all the result, continue reading more. */
						if (length < intended)
							recvd = _RSSocketRecv(ctxt->_socket, ptr + length, intended - length, &error);
						
						/* No error on the final read? */
						if (!error.error) {
							
							/* Add the read bytes. */
							length += recvd;
							
							/* If got them all, need to remove the handshake and let things fly. */
							if (length == intended) {

								/* There is no reason to have this. */
								RSDictionaryRemoveValue(ctxt->_properties, _kRSStreamPropertySOCKSRecvBuffer);

								/* Remove the handshake and get out. */
								_SocketStreamRemoveHandshake_NoLock(ctxt, _PerformSOCKSv5PostambleHandshake_NoLock);
								
								return;							/* NOTE the early return! */
							}
						}
					}
				}
			}
						
			/* If read 0 bytes, this is an early close from the other side. */
			if (recvd == 0) {
				
				/* Mark as not connected. */
				ctxt->_error.error = ENOTCONN;
				ctxt->_error.domain = _kRSStreamErrorDomainNativeSockets;
			}
			
			/* Successfully read? */
			else if (!error.error)
				break;
			
			/* Got a blocking error, so need to copy over all the bytes and buffer. */
			else if ((error.error == EAGAIN) || (error.domain == kRSStreamErrorDomainPOSIX)) {
				
				UInt8 tmp[262];
				
				/* Set the length of the buffer. */
				length += recvd;
				
				/* RS is so kind as to zero the bytes on SetLength, even though it's a fixed capacity. */
				memmove(tmp, ptr, length);
				
				/* Set the length of the buffer. */
				RSDataSetLength(to_recv, length);
				
				/* Put the bytes back. */
				memmove(ptr, tmp, length);
				
				/* Re-enable after performing a successful read. */
				RSSocketEnableCallBacks(ctxt->_socket, kRSSocketReadCallBack);
			}
			
			/* If got an error other than EAGAIN, set the error in the context. */
			else
				memmove(&ctxt->_error, &error, sizeof(error));
		}
	} while (0);
	
	/* If there was an error remove the handshake. */
	if (ctxt->_error.error)
		_SocketStreamRemoveHandshake_NoLock(ctxt, _PerformSOCKSv5Handshake_NoLock);
}


/* static */ void
_PerformSOCKSv5UserPassHandshake_NoLock(_RSSocketStreamContext* ctxt) {
	
	do {
		/* Get the buffer of stuff to send */
		RSMutableDataRef to_send = (RSMutableDataRef)RSDictionaryGetValue(ctxt->_properties,
																		  _kRSStreamPropertySOCKSSendBuffer);
		RSMutableDataRef to_recv = (RSMutableDataRef)RSDictionaryGetValue(ctxt->_properties,
																		  _kRSStreamPropertySOCKSRecvBuffer);
		
		if (!to_recv) {
			
			RSStreamError error = {0, 0};
			RSIndex length, sent;
			
			if (!to_send) {
				
				UInt8* ptr;
				
				/* Get the user/pass. */
				RSDictionaryRef proxy = (RSDictionaryRef)RSDictionaryGetValue(ctxt->_properties, kRSStreamPropertySOCKSProxy);
				RSStringRef user = (RSStringRef)RSDictionaryGetValue(proxy, kRSStreamPropertySOCKSUser);
				RSStringRef pass = (RSStringRef)RSDictionaryGetValue(proxy, kRSStreamPropertySOCKSPassword);
				
				/* Create the buffer for the maximum user/pass packet. */
				to_send = RSDataCreateMutable(RSGetAllocator(ctxt->_properties), 513);
				
				/* Couldn't create so error out on no memory. */
				if (!to_send) {
					ctxt->_error.error = ENOMEM;
					ctxt->_error.domain = kRSStreamErrorDomainPOSIX;
					break;
				}
				
				/* Make sure to save the buffer for later. */
				RSDictionarySetValue(ctxt->_properties, _kRSStreamPropertySOCKSSendBuffer, to_send);
				RSRelease(to_send);
				
				/* Get the local pointer to set the values. */
				ptr = RSDataGetMutableBytePtr(to_send);
				RSDataSetLength(to_send, 513);

				/* Set version 1. */
				ptr[0] = 0x01;
				
				/* Place the user and pass into the buffer. */
				RSStringGetPascalString(user, &(ptr[1]), 256, kRSStringEncodingUTF8);
				RSStringGetPascalString(pass, &(ptr[2 + ptr[1]]), 256, kRSStringEncodingUTF8);
				
				/* Set the length. */
				RSDataSetLength(to_send, 3 + ptr[1] + ptr[2 + ptr[1]]);
			}
			
			/* Try sending out the bytes. */
			length = RSDataGetLength(to_send);
			sent = _RSSocketSend(ctxt->_socket, RSDataGetBytePtr(to_send), length, &error);
			
			/* If sent everything, dump the buffer. */
			if (sent == length) {
				RSDictionaryRemoveValue(ctxt->_properties, _kRSStreamPropertySOCKSSendBuffer);
				
				/* Create the buffer for receive. */
				to_recv = RSDataCreateMutable(RSGetAllocator(ctxt->_properties), 2);
				
				/* Fail so error on no memory. */
				if (!to_recv) {
					ctxt->_error.error = ENOMEM;
					ctxt->_error.domain = kRSStreamErrorDomainPOSIX;
					break;
				}
				
				/* Make sure to save the buffer for later. */
				RSDictionarySetValue(ctxt->_properties, _kRSStreamPropertySOCKSRecvBuffer, to_recv);
				RSRelease(to_recv);
			}
			
			/* If couldn't send everything, trim the buffer. */
			else if (sent > 0) {
				
				UInt8* ptr = RSDataGetMutableBytePtr(to_send);
				
				/* New length */
				length -= sent;
				
				/* Move the bytes down in the buffer. */
				memmove(ptr, ptr + sent, length);
				
				/* Trim it. */
				RSDataSetLength(to_send, length);
				
				/* Re-enable so the rest can be written later. */
				RSSocketEnableCallBacks(ctxt->_socket, kRSSocketWriteCallBack);
			}
			
			/* If got an error other than EAGAIN, set the error in the context. */
			else if ((error.error != EAGAIN) || (error.domain != kRSStreamErrorDomainPOSIX))
				memmove(&ctxt->_error, &error, sizeof(error));
		}
		
		else {
			
			UInt8* ptr = RSDataGetMutableBytePtr(to_recv);
			RSIndex length = RSDataGetLength(to_recv);
			
			if (length != kSOCKSv5BufferMaximum) {
				
				RSStreamError error = {0, 0};
				RSIndex recvd = _RSSocketRecv(ctxt->_socket, ptr + length, kSOCKSv5BufferMaximum - length, &error);
				
				/* If read 0 bytes, this is an early close from the other side. */
				if (recvd == 0) {
					
					/* Mark as not connected. */
					ctxt->_error.error = ENOTCONN;
					ctxt->_error.domain = _kRSStreamErrorDomainNativeSockets;
				}
				
				/* Successfully read? */
				else if (recvd > 0) {
					
					UInt8 tmp[kSOCKSv5BufferMaximum];
					
					/* Set the length of the buffer. */
					length += recvd;
					
					/* RS is so kind as to zero the bytes on SetLength, even though it's a fixed capacity. */
					memmove(tmp, ptr, length);
					
					/* Set the length of the buffer. */
					RSDataSetLength(to_recv, length);
					
					/* Put the bytes back. */
					memmove(ptr, tmp, length);
					
					/* Re-enable after performing a successful read. */
					RSSocketEnableCallBacks(ctxt->_socket, kRSSocketReadCallBack);
				}
				
				/* If got an error other than EAGAIN, set the error in the context. */
				else if ((error.error != EAGAIN) || (error.domain != kRSStreamErrorDomainPOSIX))
					memmove(&ctxt->_error, &error, sizeof(error));
			}
			
			/* Is there enough now? */
			if (length == kSOCKSv5BufferMaximum) {
				
				/* Status must be 0x00 for success. */
				if (ptr[1]) {
					ctxt->_error.domain = kRSStreamErrorDomainSOCKS;
					ctxt->_error.error = ((kRSStreamErrorSOCKS5SubDomainUserPass << 16) | (ptr[1] & 0x000000FF));
					break;
				}

				RSDictionaryRemoveValue(ctxt->_properties, _kRSStreamPropertySOCKSRecvBuffer);
				_SocketStreamRemoveHandshake_NoLock(ctxt, _PerformSOCKSv5UserPassHandshake_NoLock);
			}
		}
	} while (0);
	
	/* If there was an error remove the handshake. */
	if (ctxt->_error.error)
		_SocketStreamRemoveHandshake_NoLock(ctxt, _PerformSOCKSv5Handshake_NoLock);
}


/* static */ void
_PerformSOCKSv4Handshake_NoLock(_RSSocketStreamContext* ctxt) {
	
	do {
		/* Get the buffer of stuff to send */
		RSMutableDataRef to_send = (RSMutableDataRef)RSDictionaryGetValue(ctxt->_properties,
																		  _kRSStreamPropertySOCKSSendBuffer);
		RSMutableDataRef to_recv = (RSMutableDataRef)RSDictionaryGetValue(ctxt->_properties,
																		  _kRSStreamPropertySOCKSRecvBuffer);
		
		/* Send anything that is waiting. */
		if (to_send) {
			
			RSStreamError error = {0, 0};
			RSIndex length = RSDataGetLength(to_send);
			RSIndex sent = _RSSocketSend(ctxt->_socket, RSDataGetBytePtr(to_send), length, &error);
			
			/* If sent everything, dump the buffer. */
			if (sent == length)
				RSDictionaryRemoveValue(ctxt->_properties, _kRSStreamPropertySOCKSSendBuffer);
				
			/* If couldn't send everything, trim the buffer. */
			else if (sent > 0) {
				
				UInt8* ptr = RSDataGetMutableBytePtr(to_send);
				
				/* New length */
				length -= sent;
				
				/* Move the bytes down in the buffer. */
				memmove(ptr, ptr + sent, length);
				
				/* Trim it. */
				RSDataSetLength(to_send, length);
				
				/* Re-enable so the rest can be written later. */
				RSSocketEnableCallBacks(ctxt->_socket, kRSSocketWriteCallBack);
			}
			
			/* If got an error other than EAGAIN, set the error in the context. */
			else if ((error.error != EAGAIN) || (error.domain != kRSStreamErrorDomainPOSIX))
				memmove(&ctxt->_error, &error, sizeof(error));
		}
		
		else {
			
			UInt8* ptr;
			RSIndex length;
			
			/* If there is no receive buffer, need to create it. */
			if (!to_recv) {
				
				/* SOCKSv4 uses only an 8 byte buffer. */
				to_recv = RSDataCreateMutable(RSGetAllocator(ctxt->_properties), kSOCKSv4BufferMaximum);
				
				/* Did it create? */
				if (!to_recv) {
					
					/* Set no memory error and bail. */
					ctxt->_error.error = ENOMEM;
					ctxt->_error.domain = kRSStreamErrorDomainPOSIX;
					
					/* Fail now. */
					break;
				}
				
				/* Add it to the properties. */
				RSDictionaryAddValue(ctxt->_properties, _kRSStreamPropertySOCKSRecvBuffer, to_recv);
				
				RSRelease(to_recv);
			}
			
			ptr = RSDataGetMutableBytePtr(to_recv);
			length = RSDataGetLength(to_recv);
			
			if (length != kSOCKSv4BufferMaximum) {
				
				RSStreamError error = {0, 0};
				RSIndex recvd = _RSSocketRecv(ctxt->_socket, ptr + length, kSOCKSv4BufferMaximum - length, &error);
				
				/* If read 0 bytes, this is an early close from the other side. */
				if (recvd == 0) {
					
					/* Mark as not connected. */
					ctxt->_error.error = ENOTCONN;
					ctxt->_error.domain = _kRSStreamErrorDomainNativeSockets;
				}
				
				/* Successfully read? */
				else if (recvd > 0) {
					
					UInt8 tmp[kSOCKSv4BufferMaximum];
					
					/* Set the length of the buffer. */
					length += recvd;
						
					/* RS is so kind as to zero the bytes on SetLength, even though it's a fixed capacity. */
					memmove(tmp, ptr, length);
					
					/* Set the length of the buffer. */
					RSDataSetLength(to_recv, length);
					
					/* Put the bytes back. */
					memmove(ptr, tmp, length);
					
					/* Re-enable after performing a successful read. */
					RSSocketEnableCallBacks(ctxt->_socket, kRSSocketReadCallBack);
				}
				
				/* If got an error other than EAGAIN, set the error in the context. */
				else if ((error.error != EAGAIN) || (error.domain != kRSStreamErrorDomainPOSIX))
					memmove(&ctxt->_error, &error, sizeof(error));
			}
			
			/* Is there enough now? */
			if (length == kSOCKSv4BufferMaximum) {
				
				/* If successful, remove the handshake. */
				if ((ptr[0] == 0) && (ptr[1] == 90))
					_SocketStreamRemoveHandshake_NoLock(ctxt, _PerformSOCKSv4Handshake_NoLock);
				
				/* Set the error based upon the SOCKS result. */
				else {
					ctxt->_error.error = (kRSStreamErrorSOCKS4SubDomainResponse << 16) | (((ptr[0] == 0) ? ptr[1] : -1) & 0x0000FFFF);
					ctxt->_error.domain = kRSStreamErrorDomainSOCKS;
				}
				
				/* Toss this as it's not needed anymore. */
				RSDictionaryRemoveValue(ctxt->_properties, _kRSStreamPropertySOCKSRecvBuffer);
			}
		}
	} while (0);
	
	/* If there was an error remove the handshake. */
	if (ctxt->_error.error)
		_SocketStreamRemoveHandshake_NoLock(ctxt, _PerformSOCKSv4Handshake_NoLock);
}


/* static */ BOOL
_SOCKSSetInfo_NoLock(_RSSocketStreamContext* ctxt, RSDictionaryRef settings) {
	
	RSDictionaryRef old = RSDictionaryGetValue(ctxt->_properties, kRSStreamPropertySOCKSProxy);
	
	/*
	** Up front check for correct settings type before tossing
	** out the existing SOCKS info.  Can only set SOCKS proxy
	** if not opened or opening.  Can't use SOCKS if created
	** with a connected socket.
	*/	
    if ((settings && (RSDictionaryGetTypeID() != RSGetTypeID(settings))) || 
        __RSBitIsSet(ctxt->_flags, kFlagBitOpenComplete) ||
        __RSBitIsSet(ctxt->_flags, kFlagBitOpenStarted) ||
        __RSBitIsSet(ctxt->_flags, kFlagBitCreatedNative))
    {
        return FALSE;
    }
	
	if (!old || !RSEqual(old, settings)) {
		
		/* Removing the SOCKS proxy? */
		if (!settings) {
			
			/* Remove the settings. */
			RSDictionaryRemoveValue(ctxt->_properties, kRSStreamPropertySOCKSProxy);
			
			/* Remove the handshake (removes both to make sure neither socks is set). */
			_SocketStreamRemoveHandshake_NoLock(ctxt, _PerformSOCKSv4Handshake_NoLock);
			_SocketStreamRemoveHandshake_NoLock(ctxt, _PerformSOCKSv5Handshake_NoLock);
		}
		
		/* Client is setting the proxy. */
		else {
			
			RSStringRef name = NULL;
			RSStringRef user = (RSStringRef)RSDictionaryGetValue(settings, kRSStreamPropertySOCKSUser);
			RSStringRef pass = (RSStringRef)RSDictionaryGetValue(settings, kRSStreamPropertySOCKSPassword);
			RSStringRef version = _GetSOCKSVersion(settings);
			RSTypeRef lookup = RSDictionaryGetValue(ctxt->_properties, kRSStreamPropertySocketRemoteHost);
			RSNumberRef enabled = RSDictionaryGetValue(settings,  _kRSStreamProxySettingSOCKSEnable);
			SInt32 enabled_value = 0;
			
			/* Verify that a valid version has been set.  No setting is SOCKSv5. */
			if (!RSEqual(version, kRSStreamSocketSOCKSVersion4) && !RSEqual(version, kRSStreamSocketSOCKSVersion5))
				return FALSE;
            
			/* See if this is being pulled directly out of SC and try to do the right thing. */
			if (enabled && RSNumberGetValue(enabled, kRSNumberSInt32Type, &enabled_value) && !enabled_value) {
				
				/* If it's not enabled, this means "don't set it." */
				RSDictionaryRemoveValue(ctxt->_properties, kRSStreamPropertySOCKSProxy);
                
				return TRUE;
			}
			
			/* Is there far end information for setting up the tunnel? */
			if (lookup) {
				
				/* Get the list of names for setting up the tunnel */
				RSArrayRef list = RSHostGetNames((RSHostRef)lookup, NULL);
				
				/* Good with at least one name? */
				if (list && RSArrayGetCount(list))
					name = (RSStringRef)RSRetain((RSTypeRef)RSArrayGetValueAtIndex(list, 0));
				
				/* No name, but can create one with an IP. */
				else {
					
					/* Is there one good address to create a name? */
					list = RSHostGetAddressing((RSHostRef)lookup, NULL);
					if (list && RSArrayGetCount(list)) {
						name = _RSNetworkRSStringCreateWithRSDataAddress(RSGetAllocator(list),
																		 (RSDataRef)RSArrayGetValueAtIndex(list, 0));
					}
				}
			}
			
			/* No host, but is there a possible RSNetService? */
			else if ((lookup = RSDictionaryGetValue(ctxt->_properties, kRSStreamPropertySocketRemoteNetService))) {
				
				/* Can't perform SOCKS to a RSNetService. */
				return FALSE;
			}
			
			/* Fail if there is no way to put together a SOCKS request. */
			if (!name) {
				
				/* SOCKSv4 can attempt the resolve to get the name. */
				if (!RSEqual(version, kRSStreamSocketSOCKSVersion4))
					return FALSE;
			}
			
			else {
				/* The maximum hostname length to be packed is 255 in SOCKSv5 */
				RSIndex length = RSStringGetLength(name);
				
				/* Check to see if need this proxy for the intended host. */
				if (!_RSNetworkDoesNeedProxy(name,
											 RSDictionaryGetValue(settings, kRSStreamPropertyProxyExceptionsList),
											 RSDictionaryGetValue(settings, kRSStreamPropertyProxyLocalBypass)))
				{
					RSRelease(name);
					return FALSE;
				}
				
				RSRelease(name);
				
				/* SOCKSv5 requires that the host name be a maximum of 255. */
				if (RSEqual(version, kRSStreamSocketSOCKSVersion5) && ((length <= 0) || (length > 255)))
					return FALSE;
			}
			
			/* SOCKSv5 maximum password length if given is 255. */
			if (RSEqual(version, kRSStreamSocketSOCKSVersion5) && pass && (RSStringGetLength(pass) > 255))
				return FALSE;
			
			if (user) {
				
				/* SOCKSv4 maximum user name length is 512. */
				if (RSEqual(version, kRSStreamSocketSOCKSVersion4) && (RSStringGetLength(user) > 512))
					return FALSE;
				
				/* SOCKSv5 maximum user name length is 255. */
				else if (RSEqual(version, kRSStreamSocketSOCKSVersion5) && (RSStringGetLength(user) > 255))
					return FALSE;
			}
			
			/* Add the handshake to the list to perform. */
			if (!_SocketStreamAddHandshake_NoLock(ctxt, RSEqual(version, kRSStreamSocketSOCKSVersion4) ? _PerformSOCKSv4Handshake_NoLock : _PerformSOCKSv5Handshake_NoLock))
				return FALSE;
			
			/* Put the new setting in place, removing the old if previously set. */
			RSDictionarySetValue(ctxt->_properties, kRSStreamPropertySOCKSProxy, settings);
		}
	}
	
	return TRUE;
}


/* static */ void
_SocketStreamSOCKSHandleLookup_NoLock(_RSSocketStreamContext* ctxt, RSHostRef lookup) {
	
	int i;
	RSArrayRef addresses;
	RSStringRef name = NULL;
	RSMutableArrayRef loops[3];
	RSIndex user_len = 0;
	RSIndex extra = 0;
	RSMutableDataRef buffer;
	RSDictionaryRef settings = (RSDictionaryRef)RSDictionaryGetValue(ctxt->_properties, kRSStreamPropertySOCKSProxy);
	RSStringRef user = (RSStringRef)RSDictionaryGetValue(settings, kRSStreamPropertySOCKSUser);
	UInt8* ptr = NULL;
	
	RSNumberRef port = RSDictionaryGetValue(ctxt->_properties, _kRSStreamPropertySocketRemotePort);
	
	/* Remove the lookup from the schedulables since it's done. */
	_SchedulablesRemove(ctxt->_schedulables, lookup);
	
	/* Invalidate it so no callbacks occur. */
	_RSTypeInvalidate(lookup);
	
	/* Grab the list of run loops and modes for unscheduling. */
	loops[0] = ctxt->_readloops;
	loops[1] = ctxt->_writeloops;
	loops[2] = ctxt->_sharedloops;
	
	/* Make sure to remove the lookup from all loops and modes. */
	for (i = 0; i < (sizeof(loops) / sizeof(loops[0])); i++)
		_RSTypeUnscheduleFromMultipleRunLoops(lookup, loops[i]);
	
	/* Get the list of addresses. */
	addresses = RSHostGetAddressing((RSHostRef)lookup, NULL);
	
	/* If no addresses, go for the name. */
	if (!addresses || !RSArrayGetCount(addresses))
		name = (RSStringRef)RSArrayGetValueAtIndex(RSHostGetNames((RSHostRef)lookup, NULL), 0);
	
	/* Find the overhead for the user name if one. */
	if (user) {
		user_len = RSStringGetBytes(user, RSRangeMake(0, RSStringGetLength(user)),
									kRSStringEncodingUTF8, 0, FALSE, NULL, 0, NULL);
	}
	
	/* Add for null termination. */
	user_len += 1;
	
	if (name) {
		
		/* What's the cost in bytes of the host name? */
		extra = RSStringGetBytes(name, RSRangeMake(0, RSStringGetLength(name)), kRSStringEncodingUTF8, 0, FALSE, NULL, 0, NULL);
		extra += 1;
	}
	
	/* Create the send buffer. */
	buffer = RSDataCreateMutable(RSGetAllocator(ctxt->_properties), 8 + extra + user_len);
	
	/* If failed, set the no memory error and return. */
	if (!buffer) {
		ctxt->_error.error = ENOMEM;
		ctxt->_error.domain = kRSStreamErrorDomainPOSIX;
		
		return;						/* NOTE the early return. */
	}
	
	/* Extend out the length to the full capacity. */
	RSDataSetLength(buffer, 8 + extra + user_len);
	
	/* Add it to the properties for future. */
	RSDictionaryAddValue(ctxt->_properties, _kRSStreamPropertySOCKSSendBuffer, buffer);
	RSRelease(buffer);
	
	/* Get the pointer for easier manipulation. */
	ptr = RSDataGetMutableBytePtr(buffer);
	
	/* Zero out even though SetLength probably did. */
	memset(ptr, 0, RSDataGetLength(buffer));
	
	/* If a name was set, there was no address. */
	if (name) {
		
		/* Copy the name into the buffer.  */
		RSStringGetBytes(name, RSRangeMake(0, RSStringGetLength(name)),
						 kRSStringEncodingUTF8, 0, FALSE, ptr + 8 + user_len - 1, extra, NULL);
		
		/* Cap with a null. */
		ptr[8 + user_len + extra - 1] = '\0';
	}
	
	/* Use an address instead of a name. */
	else {
		
		RSIndex i, count = RSArrayGetCount(addresses);
		
		/* Loop through looking for a valid IPv4 address.  SOCKSv4 doesn't do IPv6. */
		for (i = 0; i < count; i++) {
			
			struct sockaddr_in* sin = (struct sockaddr_in*)RSDataGetBytePtr(RSArrayGetValueAtIndex(addresses, i));
			
			if (sin->sin_family == AF_INET) {
				
				/* Set the IP in the buffer. */
				memmove(&ptr[4], &sin->sin_addr, sizeof(sin->sin_addr));
				
				/* If there was no port, it must be in the address. */
				if (!port)
					memmove(&ptr[2], &sin->sin_port, sizeof(sin->sin_port));
				
				break;
			}
		}
		
		/* Went through all the addresses and found none suitable. */
		if (i == count) {
			
			/* Mark as an invalid parameter */
			ctxt->_error.error = EINVAL;
			ctxt->_error.domain = kRSStreamErrorDomainPOSIX;
			
			/* Remove the buffer, 'cause this isn't going anywhere. */
			RSDictionaryRemoveValue(ctxt->_properties, _kRSStreamPropertySOCKSSendBuffer);
			
			return;					/* NOTE the early return. */
		}
	}
	
	/* Set the protocol version and "CONNECT" command. */
	ptr[0] = 0x04;
	ptr[1] = 0x01;
	
	/* If there was a port set, need to copy that. */
	if (port) {
		
		SInt32 value;
		
		/* Grab the real value. */
		RSNumberGetValue(port, kRSNumberSInt32Type, &value);
		
		/* Place the port into the buffer. */
		*((UInt16*)(&ptr[2])) = htons(value & 0x0000FFFF);
	}
	
	/* If there was a user name, need to grab its bytes into place. */
	if (user) {
		RSStringGetBytes(user, RSRangeMake(0, RSStringGetLength(user)),
						 kRSStringEncodingUTF8, 0, FALSE, ptr + 8, user_len - 1, NULL);
	}
	
	/* If open has already finished, need to pump this thing along. */
	if (__RSBitIsSet(ctxt->_flags, kFlagBitOpenComplete)) {
		
		RSArrayRef handshakes = (RSArrayRef)RSDictionaryGetValue(ctxt->_properties, _kRSStreamPropertyHandshakes);
		
		/* Only force the "pump" if SOCKS is the head. */
		if (handshakes && (RSArrayGetValueAtIndex(handshakes, 0) == _PerformSOCKSv4Handshake_NoLock))
			_PerformSOCKSv4Handshake_NoLock(ctxt);
	}
}


#if 0
#pragma mark *CONNECT Support
#endif

/* static */ void
_CreateNameAndPortForCONNECTProxy(RSDictionaryRef properties, RSStringRef* name, RSNumberRef* port, RSStreamError* error) {
		
	RSTypeRef lookup;
	RSDataRef addr = NULL;
	RSAllocatorRef alloc = RSGetAllocator(properties);
	
	/* NOTE that this function is used for setting the SSL peer ID for connections other than CONNECT tunnelling. */
	
	*name = NULL;
	*port = NULL;
	
	/* No error to start */
	memset(error, 0, sizeof(error[0]));
	
	/* Try to simply get the port from the properties. */
	*port = (RSNumberRef)RSDictionaryGetValue(properties, _kRSStreamPropertySocketRemotePort);
	
	/* Try to get the service for which the streams were created. */
	if ((lookup = (RSTypeRef)RSDictionaryGetValue(properties, kRSStreamPropertySocketRemoteNetService))) {
		
		/* Does it have a name?  These are checked at set, but just in case they go away. */
		*name = RSNetServiceGetTargetHost((RSNetServiceRef)lookup);
		
		/* If didn't get a port, need to go to the address to get it. */
		if (!*port) {
			
			/* Get the list of addresses from the service. */
			RSArrayRef list = RSNetServiceGetAddressing((RSNetServiceRef)lookup);
			
			/* Can only pull one out if it's been resolved. */
			if (list && RSArrayGetCount(list))
				addr = RSArrayGetValueAtIndex(list, 0);
		}
	}
	
	/* No service, so go for the host. */
	else if ((lookup = (RSTypeRef)RSDictionaryGetValue(properties, kRSStreamPropertySocketRemoteHost))) {
		
		/* Get the list of names in order to get one. */
		RSArrayRef list = RSHostGetNames((RSHostRef)lookup, NULL);
		
		/* Pull out the name */
		if (list && RSArrayGetCount(list))
			*name = (RSStringRef)RSArrayGetValueAtIndex(list, 0);
		
		else {
			
			/* No name, so get the address as a name instead. */
			list = RSHostGetAddressing((RSHostRef)lookup, NULL);
			
			/* Get the first address from the list. */
			if (list && RSArrayGetCount(list))
				addr = RSArrayGetValueAtIndex(list, 0);
		}
	}
	
	/* If there is no information at all, error and bail. */
	if (!*port && !*name && !addr) {
		error->error = EINVAL;
		error->domain = kRSStreamErrorDomainPOSIX;
		return;												/* NOTE the early return */
	}
	
	/* Got a port?  If so, just retain it. */
	if (*port)
		RSRetain(*port);
	
	else {
		
		SInt32 p;
		struct sockaddr* sa = (struct sockaddr*)RSDataGetBytePtr(addr);
		
		/* Need to go to the socket address in order to get the port value */
		switch (sa->sa_family) {
			
			case AF_INET:
				p = ntohs(((struct sockaddr_in*)sa)->sin_port);
				*port = RSNumberCreate(alloc, kRSNumberSInt32Type, &p);
				break;
				
			case AF_INET6:
				p = ntohs(((struct sockaddr_in6*)sa)->sin6_port);
				*port = RSNumberCreate(alloc, kRSNumberSInt32Type, &p);
				break;
				
			default:
				/* Not a known type.  Return an error. */
				error->error = EINVAL;
				error->domain = kRSStreamErrorDomainPOSIX;
				return;										/* NOTE the early return */
		}
	}
	
	/* Either retain the current name or create one from the address. */
	if (*name)
		RSRetain(*name);
	else
		*name = _RSNetworkRSStringCreateWithRSDataAddress(alloc, addr);
	
	/* If either failed, need to error out. */
	if (!*name || !*port) {
		
		/* Release anything that was created/retained. */
		if (*name) RSRelease(*name);
		if (*port) RSRelease(*port);
		
		/* Set the out of memory error. */
		error->error = ENOMEM;
		error->domain = kRSStreamErrorDomainPOSIX;
	}
}		


/* static */ void
_PerformCONNECTHandshake_NoLock(_RSSocketStreamContext* ctxt) {
    
    UInt8 buffer[2048];
	RSIndex count;
	RSStreamError error = {0, 0};
	RSAllocatorRef alloc = RSGetAllocator(ctxt->_properties);
	RSHTTPMessageRef response = (RSHTTPMessageRef)RSDictionaryGetValue(ctxt->_properties, kRSStreamPropertyCONNECTResponse);
	
	/* NOTE, use the lack of response to mean that need to send first. */
	if (!response) {
		
		RSIndex length;
		RSDataRef left = (RSDataRef)RSDictionaryGetValue(ctxt->_properties, _kRSStreamPropertyCONNECTSendBuffer);
		
		/* If there is nothing waiting, haven't put anything together. */
		if (!left) {
			
			RSDictionaryRef headers;
			RSHTTPMessageRef request = NULL;
			RSStringRef version, name = NULL;
			RSDictionaryRef proxy = (RSDictionaryRef)RSDictionaryGetValue(ctxt->_properties, kRSStreamPropertyCONNECTProxy);
			
			/* Get the intended version of HTTP. */
			version = (RSStringRef)RSDictionaryGetValue(proxy, kRSStreamPropertyCONNECTVersion);
			
			do {
				SInt32 p;
				RSURLRef url;
				RSNumberRef port;
				RSStringRef urlstr;

				/* Figure out what the far end name and port are. */
				_CreateNameAndPortForCONNECTProxy(ctxt->_properties, &name, &port, &ctxt->_error);
			
				/* Got an error, so bail. */
				if (ctxt->_error.error) break;
				
				/* Get the real port value. */
				RSNumberGetValue(port, kRSNumberSInt32Type, &p);
			
				/* Produce the "CONNECT" url which is <host>:<port>. */
				urlstr = RSStringCreateWithFormat(alloc, NULL, _kRSStreamCONNECTURLFormat, name, p & 0x0000FFFF);
			
				/* Don't need it. */
				RSRelease(port);
				
				/* Make sure there's an url before continuing on. */
				if (!urlstr) break;
				
				/* Create the url based upon the string. */
				url = RSURLCreateWithString(alloc, urlstr, NULL);
				
				/* Don't need it. */
				RSRelease(urlstr);
				
				/* Must have an url to continue. */
				if (!url) break;
			
				/*
				** Create the "CONNECT" request.  Default HTTP version is 1.0 if none specified.
				** NOTE there are actually some servers which will fail if 1.1 is used.
				*/
				request = RSHTTPMessageCreateRequest(alloc, _kRSStreamCONNECTMethod, url, version ? version : kRSHTTPVersion1_0);
			
				RSRelease(url);
			
			} while (0);
			
			/* Fail if the request wasn't made. */
			if (!request) {
				
				if (name) RSRelease(name);
				
				/* If an error wasn't set, assume an out of memory error. */
				if (!ctxt->_error.error) {
					ctxt->_error.error = ENOMEM;
					ctxt->_error.domain = kRSStreamErrorDomainPOSIX;
				}
				
				return;												/* NOTE the early return */
			}

			/* Add the other headers */
			headers = RSDictionaryGetValue(proxy, kRSStreamPropertyCONNECTAdditionalHeaders);
			if (headers) {
				
				/* Check to see if "Host:" needs to be added and do so. */
				RSStringRef value = (RSStringRef)RSDictionaryGetValue(headers, _kRSStreamHostHeader);
				if (!value)
					RSHTTPMessageSetHeaderFieldValue(request, _kRSStreamHostHeader, name);
				
				/* Check to see if "User-Agent:" need to be added and do so. */
				value = (RSStringRef)RSDictionaryGetValue(headers, _kRSStreamUserAgentHeader);
				if (!value)
					RSHTTPMessageSetHeaderFieldValue(request, _kRSStreamUserAgentHeader, _RSNetworkUserAgentString());
				
				/* Add all the other headers. */
				RSDictionaryApplyFunction(headers, (RSDictionaryApplierFunction)_CONNECTHeaderApplier, request);
			}
			else {
				/* CONNECT must have "Host:" and "User-Agent:" headers. */
				RSHTTPMessageSetHeaderFieldValue(request, _kRSStreamHostHeader, name);
				RSHTTPMessageSetHeaderFieldValue(request, _kRSStreamUserAgentHeader, _RSNetworkUserAgentString());
			}
			
			RSRelease(name);
			
			/*
			** Flatten the request using this special version.  This will
			** flatten it for proxy usage (it will keep the full url).
			*/
			left = _RSHTTPMessageCopySerializedHeaders(request, TRUE);
			RSRelease(request);
			
			/* If failed to flatten, specify out of memory. */
			if (!left) {
				ctxt->_error.error = ENOMEM;
				ctxt->_error.domain = kRSStreamErrorDomainPOSIX;
				return;												/* NOTE the early return */
			}
			
			/* Add the buffer to the properties for future. */
			RSDictionaryAddValue(ctxt->_properties, _kRSStreamPropertyCONNECTSendBuffer, left);
			RSRelease(left);
		}
		
		/* Find out how much is left and try sending. */
		length = RSDataGetLength(left);
		count = _RSSocketSend(ctxt->_socket, RSDataGetBytePtr(left), length, &error);
		
		if (error.error) {
		
			/* EAGAIN is not really an error. */
			if ((error.error == EAGAIN) && (error.domain == _kRSStreamErrorDomainNativeSockets))
				count = 0;
			
			/* Other errors, copy into place and bail. */
			else {
				memmove(&ctxt->_error, &error, sizeof(error));
				return;												/* NOTE the early return */
			}
		}
		
		/* Shrink by the sent amount. */
		length -= count;
		
		/* Re-enable so can continue. */
		RSSocketEnableCallBacks(ctxt->_socket, kRSSocketWriteCallBack);
		
		/* Did everything get written? */
		if (!length) {
			
			/* Toss the buffer, since it is done. */
			RSDictionaryRemoveValue(ctxt->_properties, _kRSStreamPropertyCONNECTSendBuffer);
			
			/* Make sure there is a response for reading. */
			response = RSHTTPMessageCreateEmpty(alloc, FALSE);
			
			/* Bail if failed to create. */
			if (!response) {
				ctxt->_error.error = ENOMEM;
				ctxt->_error.domain = kRSStreamErrorDomainPOSIX;
				return;												/* NOTE the early return */
			}
			
			/* Save it as a property, since that's what it is. */
			RSDictionaryAddValue(ctxt->_properties, kRSStreamPropertyCONNECTResponse, response);
			RSRelease(response);
		}
		
		/* If did read but not everything, need to trim the buffer. */
		else if (count) {
			
			/* Just create a copy of what's left. */
			left = RSDataCreate(alloc, ((UInt8*)RSDataGetBytePtr(left)) + count, length);
			
			/* Failed with an out of memory? */
			if (!left) {
				ctxt->_error.error = ENOMEM;
				ctxt->_error.domain = kRSStreamErrorDomainPOSIX;
				return;												/* NOTE the early return */
			}
			
			/* Put it over top of the other. */
			RSDictionarySetValue(ctxt->_properties, _kRSStreamPropertyCONNECTSendBuffer, left);
			RSRelease(left);
		}
		
		/* Would block. */
		else
			return;												/* NOTE the early return */
	}
	
	/*
	** Peek at the socket buffer to get the bytes.  Since there is no capability
	** of pushing bytes back, need to peek to pull out the bytes.  Don't  want to
	** read bytes beyond the response bytes.
	*/
	if (-1 == (count = recv(RSSocketGetNative(ctxt->_socket), buffer, sizeof(buffer), MSG_PEEK))) {
		
		/* If it's an EAGAIN, re-enable the callback to come back later. */
		if ((_LastError(&error) == EAGAIN) && (error.domain == _kRSStreamErrorDomainNativeSockets)) {
			RSSocketEnableCallBacks(ctxt->_socket, kRSSocketReadCallBack);
			return;												/* NOTE the early return */
		}
		
		/* Some other error, so bail. */
		else if (error.error) {
			memmove(&ctxt->_error, &error, sizeof(error));
			return;												/* NOTE the early return */
		}
	}
	
	/* Add the bytes to the response.  Let it determine the end. */
	if (RSHTTPMessageAppendBytes(response, buffer, count)) {
		
		/*
		** Detect done if the head is complete.   For CONNECT,
		** all bytes after the headers belong to the client.
		*/
		if (RSHTTPMessageIsHeaderComplete(response)) {

			/* Get the result code for check later. */
			UInt32 code = RSHTTPMessageGetResponseStatusCode(response);

			/* If there were extra bytes read, need to remove those. */
			RSDataRef body = RSHTTPMessageCopyBody(response);
			if (body) {
				
				/* Reduce the count, so the correct byte count can get sucked out of the kernel. */
				count -= RSDataGetLength(body);
				RSRelease(body);
				
				/* Get rid of the body.  CONNECT responses have no body. */
				RSHTTPMessageSetBody(response, NULL);
			}
			
			/* If it wasn't a 200 series result, stall the stream for another CONNECT. */
			if ((code < 200) || (code > 299))
				_SocketStreamAddHandshake_NoLock(ctxt, _PerformCONNECTHaltHandshake_NoLock);
			
			/* Remove the handshake now that it is complete. */
			_SocketStreamRemoveHandshake_NoLock(ctxt, _PerformCONNECTHandshake_NoLock);
			
			RSDictionaryRemoveValue(ctxt->_properties, kRSStreamPropertyPreviousCONNECTResponse);
		}
		
		/* Suck the bytes out of the kernel. */
		if (-1 == recv(RSSocketGetNative(ctxt->_socket), buffer, count, 0)) {
			
			/* Copy the last error. */
			_LastError(&ctxt->_error);
			
			return;												/* NOTE the early return */
		}
				
		/* Re-enable for future callbacks. */
		RSSocketEnableCallBacks(ctxt->_socket, kRSSocketReadCallBack);
	}
	
	/* Failed to append the bytes, so error as a HTTP parsing failure. */
	else {
		ctxt->_error.error = kRSStreamErrorHTTPParseFailure;
		ctxt->_error.domain = kRSStreamErrorDomainHTTP;
	}
}


/* static */ void
_PerformCONNECTHaltHandshake_NoLock(_RSSocketStreamContext* ctxt) {
	
	(void)ctxt;		/* unused */
	
	/* Do nothing.  This will cause a stall until it's time to go again. */
	
	if (ctxt->_clientWriteStream && __RSBitIsSet(ctxt->_flags, kFlagBitWriteStreamOpened)) {
		__RSBitSet(ctxt->_flags, kFlagBitCanWrite);
		_RSWriteStreamSignalEventDelayed(ctxt->_clientWriteStream, kRSStreamEventCanAcceptBytes, NULL);
	}
	
	if (ctxt->_clientReadStream && __RSBitIsSet(ctxt->_flags, kFlagBitReadStreamOpened)) {
		__RSBitSet(ctxt->_flags, kFlagBitCanRead);
		_RSReadStreamSignalEventDelayed(ctxt->_clientReadStream, kRSStreamEventHasBytesAvailable, NULL);
	}
}


/* static */ void
_CONNECTHeaderApplier(RSStringRef key, RSStringRef value, RSHTTPMessageRef request) {
	
    RSHTTPMessageSetHeaderFieldValue(request, key, value);
}


/* static */ BOOL
_CONNECTSetInfo_NoLock(_RSSocketStreamContext* ctxt, RSDictionaryRef settings) {
	
	BOOL resume = FALSE;
    RSStringRef server = settings ? RSDictionaryGetValue(settings, kRSStreamPropertyCONNECTProxyHost) : NULL;
	RSNumberRef port = settings ? RSDictionaryGetValue(settings, kRSStreamPropertyCONNECTProxyPort) : NULL;
	RSDictionaryRef old = RSDictionaryGetValue(ctxt->_properties, kRSStreamPropertyCONNECTProxy);
	
	/*
    ** Up front check for server information before tossing out the
    ** existing CONNECT info.  Can't use CONNECT if created with a
	** connected socket.
	*/
    if ((settings && (!server || !port)) || 
        __RSBitIsSet(ctxt->_flags, kFlagBitCreatedNative))
    {
        return FALSE;
    }

	if (__RSBitIsSet(ctxt->_flags, kFlagBitOpenComplete) || __RSBitIsSet(ctxt->_flags, kFlagBitOpenStarted)) {
	
		RSMutableArrayRef handshakes = (RSMutableArrayRef)RSDictionaryGetValue(ctxt->_properties, _kRSStreamPropertyHandshakes);
		
		if (handshakes && 
			RSArrayGetCount(handshakes) &&
			(_PerformCONNECTHaltHandshake_NoLock == RSArrayGetValueAtIndex(handshakes, 0)))
		{
			resume = TRUE;
		}
		
		else
			return FALSE;
	}
		
	/* If setting the same setting, just return TRUE. */
	if (!old || !RSEqual(old, settings)) {
		
		/* Removing the CONNECT proxy? */
		if (!settings) {
			
			/* Remove the settings. */
			RSDictionaryRemoveValue(ctxt->_properties, kRSStreamPropertyCONNECTProxy);
			
			/* Remove the handshake. */
			_SocketStreamRemoveHandshake_NoLock(ctxt, _PerformCONNECTHandshake_NoLock);
		}
		
		/* Client is setting the proxy. */
		else {
			
			BOOL hasName = FALSE;
			RSTypeRef lookup = RSDictionaryGetValue(ctxt->_properties, kRSStreamPropertySocketRemoteHost);
			
			/* Is there far end information for setting up the tunnel? */
			if (lookup) {
				
				/* Get the list of names for setting up the tunnel */
				RSArrayRef list = RSHostGetNames((RSHostRef)lookup, NULL);
				
				/* Good with at least one name? */
				if (list && RSArrayGetCount(list))
					hasName = TRUE;
				
				/* No name, but can create one with an IP. */
				else {
					
					/* Is there one good address to create a name? */
					list = RSHostGetAddressing((RSHostRef)lookup, NULL);
					if (list && RSArrayGetCount(list))
						hasName = TRUE;
				}
			}
			
			/* No host, but is there a possible RSNetService with enough information? */
			else if ((lookup = RSDictionaryGetValue(ctxt->_properties, kRSStreamPropertySocketRemoteNetService))) {
				
				/* Has the far end server been resolved already? */
				if (RSNetServiceGetTargetHost((RSNetServiceRef)lookup))
					hasName = TRUE;
				
				/*
				** NOTE that the target host is resolved.  If that doesn't exist,
				** there is no reason to check addresses since those are resolved
				** too.  There is no way to create a RSNetServiceRef from an
				** address only.
				*/
			}
			
			/* Fail if there is no way to put together a CONNECT request. */
			if (!hasName)
				return FALSE;
			
			/* Add the handshake to the list to perform. */
			if (!_SocketStreamAddHandshake_NoLock(ctxt, _PerformCONNECTHandshake_NoLock)) {

				if (resume) {
					RSDictionaryRemoveValue(ctxt->_properties, kRSStreamPropertyCONNECTResponse);
					_SocketStreamRemoveHandshake_NoLock(ctxt, _PerformCONNECTHaltHandshake_NoLock);
				}

				return FALSE;
			}
			
			if (resume) {
				RSTypeRef last = RSDictionaryGetValue(ctxt->_properties, kRSStreamPropertyCONNECTResponse);
				if (last)
					RSDictionarySetValue(ctxt->_properties, kRSStreamPropertyPreviousCONNECTResponse, last);
				RSDictionaryRemoveValue(ctxt->_properties, kRSStreamPropertyCONNECTResponse);
				_SocketStreamRemoveHandshake_NoLock(ctxt, _PerformCONNECTHaltHandshake_NoLock);
			}
			
			/* Put the new setting in place, removing the old if previously set. */
			RSDictionarySetValue(ctxt->_properties, kRSStreamPropertyCONNECTProxy, settings);
		}
	}
	
	return TRUE;
}	


#if 0
#pragma mark *SSL Support
#endif

/* static */ OSStatus
_SecurityReadFunc_NoLock(_RSSocketStreamContext* ctxt, void* data, UInt32* dataLength) {
	
	/* This is the read function used by SecureTransport in order to get bytes off the wire. */
	
	/* NOTE that SSL reads bytes off the wire and into a buffer of encrypted bytes. */
	
	RSIndex i, s;
    UInt32 try = *dataLength;
	RSStreamError error = {0, 0};
	
	/* Bits required for buffered reading. */
	RSDataRef size = (RSDataRef)RSDictionaryGetValue(ctxt->_properties, _kRSStreamPropertySecurityRecvBufferSize);
	RSMutableDataRef buffer = (RSMutableDataRef)RSDictionaryGetValue(ctxt->_properties, _kRSStreamPropertySecurityRecvBuffer);
	RSMutableDataRef count = (RSMutableDataRef)RSDictionaryGetValue(ctxt->_properties, _kRSStreamPropertySecurityRecvBufferCount);
	
	/* If missing the buffer, assume nothing has been created. */
	if (!buffer) {
		
		RSAllocatorRef alloc = RSGetAllocator(ctxt->_properties);
		
		/* Start with a size.  This could be overridden from properties. */
		if (!size) {
			s = kSecurityBufferSize;
			size = RSDataCreate(alloc, (const UInt8*)&s, sizeof(s));
		}
		
		/* If have a size, create buffers for the count and the buffer itself. */
		if (size) {
			buffer = RSDataCreateMutable(alloc, *((RSIndex*)RSDataGetBytePtr(size)));
			count = RSDataCreateMutable(alloc, sizeof(RSIndex));
		}
		
		/* If anything failed, set out of memory and return error. */
		if (!buffer || !count || !size) {
			
			if (buffer) RSRelease(buffer);
			if (count) RSRelease(count);
			if (size) RSRelease(size);
			
			ctxt->_error.error = ENOMEM;
			ctxt->_error.domain = kRSStreamErrorDomainPOSIX;
			
			return errSSLInternal;								/* NOTE the eary return. */
		}
		
		/* Place everything in the properties bucket for later. */
		RSDictionaryAddValue(ctxt->_properties, _kRSStreamPropertySecurityRecvBufferSize, size);
		RSDictionarySetValue(ctxt->_properties, _kRSStreamPropertySecurityRecvBuffer, buffer);
		RSDictionarySetValue(ctxt->_properties, _kRSStreamPropertySecurityRecvBufferCount, count);
		
		RSRelease(size);
		RSRelease(buffer);
		RSRelease(count);
		
		/* Set the intial count to zero. */
		*((RSIndex*)RSDataGetMutableBytePtr(count)) = 0;
	}
	
	/* Get the count and size, respectively. */
	i = *((RSIndex*)RSDataGetMutableBytePtr(count));
	s = *((RSIndex*)RSDataGetBytePtr(size));
	
	/* If the count is less than the request, go to the wire. */
	if (i < *dataLength) {
		
		/* Read the bytes off the wire into what's left of the buffer. */
		RSIndex r = _RSSocketRecv(ctxt->_socket, ((UInt8*)RSDataGetMutableBytePtr(buffer)) + i, s - i, &error);
		
		__RSBitClear(ctxt->_flags, kFlagBitRecvdRead);
		
		/* If no error occurred, add the read bytes to the count. */
		if (!error.error)
			i += r;
	}
	else
		__RSBitSet(ctxt->_flags, kFlagBitRecvdRead);
	
	/* If still no errr, continue on. */
	if (!error.error) {
		
		UInt8* ptr = (UInt8*)RSDataGetMutableBytePtr(buffer);
		
		/* Either read what the client asked or what is in the buffer. */
		*dataLength = (*dataLength <= i) ? *dataLength : i;
		
		/* Decrease the buffer count by the return value. */
		i -= *dataLength;
		
		/* Copy the bytes into the client's buffer. */
		memmove(data, ptr, *dataLength);
		
		/* Move the bytes in the buffer down by the read count. */
		memmove(ptr, ptr + *dataLength, i);
		
		/* Zero the bytes that are no longer part of the buffer count. */
		memset(ptr + i, 0, s - i);
		
		/* Make sure to set count again. */
		*((RSIndex*)RSDataGetMutableBytePtr(count)) = i;
		
		/* If something was read, re-enable the read callback. */
		if (*dataLength)
			RSSocketEnableCallBacks(ctxt->_socket, kRSSocketReadCallBack);
		
		/* If no bytes read, return closed.  If couldn't read all, return "would block." */
        return (!*dataLength ? errSSLClosedAbort : (try == *dataLength) ? 0 : errSSLWouldBlock);
	}
	
	/* Error condition means no bytes read. */
	*dataLength = 0;
	
	/* If it's a "would block," return such. */
	if ((error.domain == _kRSStreamErrorDomainNativeSockets) && (EAGAIN == error.error))
        return errSSLWouldBlock;
	
	/* It's a real error, so copy it into the context */
	memmove(&ctxt->_error, &error, sizeof(error));
	
	/* A bad error occurred. */
	return errSSLInternal;
}	


/* static */ OSStatus
_SecurityWriteFunc_NoLock(_RSSocketStreamContext* ctxt, const void* data, UInt32* dataLength) {
	
	/* This is the function used by SecureTransport to write bytes to the wire. */
	
	RSStreamError error = {0, 0};
    UInt32 try = *dataLength;
	
	/* Write what the ST tells. */
	*dataLength = _RSSocketSend(ctxt->_socket, data, *dataLength, &error);
	
	/* If no error, return noErr, or if couldn't write everything, return the "would block." */
	if (!error.error)
		return ((try == *dataLength) ? 0 : errSSLWouldBlock);
	
	/* Error condition means no bytes written. */
	*dataLength = 0;
	
	/* If it was a "would block," return such. */
	if ((error.domain == _kRSStreamErrorDomainNativeSockets) && (EAGAIN == error.error))
        return errSSLWouldBlock;
	
	/* Copy the real error into place. */
	memmove(&ctxt->_error, &error, sizeof(error));

	/* Something bad happened. */
	return errSSLInternal;
}


/* static */ RSIndex
_SocketStreamSecuritySend_NoLock(_RSSocketStreamContext* ctxt, const UInt8* buffer, RSIndex length) {
	
    RSIndex bytesWritten = 0;
	
	SSLContextRef ssl = *((SSLContextRef*)RSDataGetBytePtr((RSDataRef)RSDictionaryGetValue(ctxt->_properties,
																						   kRSStreamPropertySocketSSLContext)));
	
    /* Try to write bytes on the socket. */
	OSStatus result = SSLWrite(ssl, buffer, length, (size_t*)(&bytesWritten));
    
	/* Check to see if error was set during the write. */
    if (ctxt->_error.error)
        return -1;
    
	/* If the stream wrote bytes but then got a blocking error, pass it as success. */
    if ((result == errSSLWouldBlock) && bytesWritten) {
        _SocketStreamAddHandshake_NoLock(ctxt, _PerformSecuritySendHandshake_NoLock);
        result = noErr;
    }
    
    /* Deal with result. */
    switch (result) {
        case errSSLClosedGraceful:			/* Non-fatal error */
        case errSSLClosedAbort:				/* Assumed non-fatal error (but may not be) **FIXME** ?? */
			
			/* Mark SSL as closed.  There could still be bytes in the buffers. */
			__RSBitSet(ctxt->_flags, kFlagBitClosed);
			
			/* NOTE the fall through. */
			
        case noErr:
            break;
            
        default:
			ctxt->_error.error = result;
			ctxt->_error.domain = kRSStreamErrorDomainSSL;
            return -1;
    }
	
    return bytesWritten;
}


/* static */ void
_SocketStreamSecurityBufferedRead_NoLock(_RSSocketStreamContext* ctxt) {

	/*
	** This function is used to read bytes out of the encrypted buffer and
	** into the unencrypted buffer.
	*/
	
	RSIndex* i;
	RSIndex s = kRecvBufferSize;
	OSStatus status = noErr;
	
	SSLContextRef ssl = *((SSLContextRef*)RSDataGetBytePtr((RSDataRef)RSDictionaryGetValue(ctxt->_properties,
																						   kRSStreamPropertySocketSSLContext)));
	
	/* Get the bits required in order to work with the buffer. */
	RSNumberRef size = (RSNumberRef)RSDictionaryGetValue(ctxt->_properties, _kRSStreamPropertyRecvBufferSize);
	RSMutableDataRef buffer = (RSMutableDataRef)RSDictionaryGetValue(ctxt->_properties, _kRSStreamPropertyRecvBuffer);
	RSMutableDataRef count = (RSMutableDataRef)RSDictionaryGetValue(ctxt->_properties, _kRSStreamPropertyRecvBufferCount);
	
	/* No buffer assumes all are missing. */
	if (!buffer) {
		
		RSAllocatorRef alloc = RSGetAllocator(ctxt->_properties);
		
		/* If no size, assume a default.  Can be overridden by properties. */
		if (!size)
			size = RSNumberCreate(alloc, kRSNumberRSIndexType, &s);
		else
			RSNumberGetValue(size, kRSNumberRSIndexType, &s);

		/* Create the backing for the buffer and the counter. */
		if (size) {
			buffer = RSDataCreateMutable(alloc, s);
			count = RSDataCreateMutable(alloc, sizeof(RSIndex));
		}
		
		/* If anything failed, set out of memory and bail. */
		if (!buffer || !count || !size) {
			
			if (buffer) RSRelease(buffer);
			if (count) RSRelease(count);
			if (size) RSRelease(size);
			
			ctxt->_error.error = ENOMEM;
			ctxt->_error.domain = kRSStreamErrorDomainPOSIX;
			
			return;								/* NOTE the eary return. */
		}
		
		/* Save the buffer information. */
		RSDictionarySetValue(ctxt->_properties, _kRSStreamPropertyRecvBufferSize, size);
		RSDictionarySetValue(ctxt->_properties, _kRSStreamPropertyRecvBuffer, buffer);
		RSDictionarySetValue(ctxt->_properties, _kRSStreamPropertyRecvBufferCount, count);
		
		RSRelease(size);
		RSRelease(buffer);
		RSRelease(count);
		
		/* Start with a zero byte count. */
		*((RSIndex*)RSDataGetMutableBytePtr(count)) = 0;
	}
	
	/* Get the count and size of the buffer, respectively. */
	i = (RSIndex*)RSDataGetMutableBytePtr(count);
	RSNumberGetValue(size, kRSNumberRSIndexType, &s);
	
	/* Only read if there is room in the buffer. */
	if (*i < s) {
		
		RSIndex start = *i;
		UInt8* ptr = (UInt8*)RSDataGetMutableBytePtr(buffer);
		
		/* Keep reading out of the encrypted buffer until an error or full. */
		while (!status && (*i < s)) {
			
			RSIndex bytesRead = 0;
			
			/* Read out of the encrypted and into the unencrypted. */
			status = SSLRead(ssl, ptr + *i, s - *i, (size_t*)(&bytesRead));
		
			/* If did read bytes, increase the count. */
			if (bytesRead > 0)
				*i = *i + bytesRead;
		}
		
		/* If didn't read bytes and the buffer is empty but SSL hasn't closed, need read events again. */
		if ((*i == start) && (*i == 0) && !__RSBitIsSet(ctxt->_flags, kFlagBitClosed))
			RSSocketEnableCallBacks(ctxt->_socket, kRSSocketReadCallBack);
	}
	
	switch (status) {
		case errSSLClosedGraceful:			/* Non-fatal error */
		case errSSLClosedAbort:				/* Assumed non-fatal error (but may not be) **FIXME** ?? */
			
			/* SSL has closed, so mark as such. */
			__RSBitSet(ctxt->_flags, kFlagBitClosed);
			__RSBitSet(ctxt->_flags, kFlagBitCanRead);
			__RSBitClear(ctxt->_flags, kFlagBitPollRead);
			
			/* NOTE the fall through. */

		case noErr:
		case errSSLWouldBlock:
			
			/* If there are bytes in the buffer to be read, set the bit. */
			if (*i) {
				__RSBitSet(ctxt->_flags, kFlagBitCanRead);
				__RSBitClear(ctxt->_flags, kFlagBitPollRead);
			}
			break;
			
		default:
			if (!ctxt->_error.error) {
				ctxt->_error.error = status;
				ctxt->_error.domain = kRSStreamErrorDomainSSL;
			}
			break;
	}
}


/* static */ void
_PerformSecurityHandshake_NoLock(_RSSocketStreamContext* ctxt) {
	
	OSStatus result;
	const void* peerid = NULL;
	size_t peeridlen;

	SSLContextRef ssl = *((SSLContextRef*)RSDataGetBytePtr((RSDataRef)RSDictionaryGetValue(ctxt->_properties,
																						   kRSStreamPropertySocketSSLContext)));
	
	/* Make sure the peer id has been set for ST performance. */
	result = SSLGetPeerID(ssl, &peerid, &peeridlen);
	if (!result && !peerid) {
		
		BOOL set = FALSE;
		
		/* Check to see if going through a proxy.  A different ID is used in that case. */
		RSTypeRef value = (RSTypeRef)RSDictionaryGetValue(ctxt->_properties, kRSStreamPropertyCONNECTProxy);
		if (!value)
			value = (RSTypeRef)RSDictionaryGetValue(ctxt->_properties, kRSStreamPropertySOCKSProxy);
		
		/* Use the name and port of the far end host. */
		if (value) {
			RSStringRef host = NULL;
			RSNumberRef port = NULL;
			RSStreamError error;
			
			/* Re-use some code to get the host and port of the far end. */
			_CreateNameAndPortForCONNECTProxy(ctxt->_properties, &host, &port, &error);
			
			/* Only do it if got the host and port, otherwise it'll fall through to the address. */
			if (host && port) {
				
				SInt32 p;
				RSStringRef peer;
				RSAllocatorRef alloc = RSGetAllocator(ctxt->_properties);
				
				/* Get the real port value. */
				RSNumberGetValue(port, kRSNumberSInt32Type, &p);
				
				/* Produce the ID as <host>:<port>. */
				peer = RSStringCreateWithFormat(alloc, NULL, _kRSStreamCONNECTURLFormat, host, p & 0x0000FFFF);
				
				if (peer) {
					
					UInt8 static_buffer[1024];
					UInt8* buffer = &static_buffer[0];
					RSIndex buffer_size = sizeof(static_buffer);
					
					/* Get the raw bytes to use as the ID. */
					buffer = _RSStringGetOrCreateCString(alloc, peer, static_buffer, &buffer_size, kRSStringEncodingUTF8);
					
					RSRelease(peer);
					
					/* Set the peer ID. */
					SSLSetPeerID(ssl, buffer, buffer_size);
					
					/* Did it. */
					set = TRUE;
					
					/* Clean up the allocation if made. */
					if (buffer != &static_buffer[0])
						RSAllocatorDeallocate(alloc, buffer);
				}
			}
			
			if (host) RSRelease(host);
			if (port) RSRelease(port);
		}
		
		if (!set) {
			UInt8 static_buffer[SOCK_MAXADDRLEN];
			struct sockaddr* sa = (struct sockaddr*)&static_buffer[0];
			socklen_t addrlen = sizeof(static_buffer);
			
			if (!getpeername(RSSocketGetNative(ctxt->_socket), sa, &addrlen)) {
				if (sa->sa_family == AF_INET) {
					in_port_t port = ((struct sockaddr_in*)sa)->sin_port;
					memmove(static_buffer, &(((struct sockaddr_in*)sa)->sin_addr), sizeof(((struct sockaddr_in*)sa)->sin_addr));
					memmove(static_buffer + sizeof(((struct sockaddr_in*)sa)->sin_addr),  &port, sizeof(port));
					SSLSetPeerID(ssl, static_buffer, sizeof(((struct sockaddr_in*)sa)->sin_addr) + sizeof(port));
				}
				else if (sa->sa_family == AF_INET6) {
					in_port_t port = ((struct sockaddr_in6*)sa)->sin6_port;
					memmove(static_buffer, &(((struct sockaddr_in6*)sa)->sin6_addr), sizeof(((struct sockaddr_in6*)sa)->sin6_addr));
					memmove(static_buffer + sizeof(((struct sockaddr_in6*)sa)->sin6_addr),  &port, sizeof(port));
					SSLSetPeerID(ssl, static_buffer, sizeof(((struct sockaddr_in6*)sa)->sin6_addr) + sizeof(port));
				}
			}
		}
	}
	
	/* Perform the SSL handshake. */
	result = SSLHandshake(ssl);
	
	/* If not blocking, can do something. */
	if (result != errSSLWouldBlock) {
		
		/* Was it noErr? */
		if (result) {
			ctxt->_error.error = result;
			ctxt->_error.domain = kRSStreamErrorDomainSSL;
		}
		else {
			RSBOOLRef check;
			
			check = (RSBOOLRef)RSDictionaryGetValue(ctxt->_properties, _kRSStreamPropertySSLAllowAnonymousCiphers);
			if ( !check || (RSBOOLGetValue(check) == FALSE) ) {
				SSLCipherSuite cipherSuite;
				
				if ( SSLGetNegotiatedCipher(ssl, &cipherSuite) == noErr ) {
					if ( cipherSuite == SSL_DH_anon_EXPORT_WITH_RC4_40_MD5 ||
						 cipherSuite == SSL_DH_anon_WITH_RC4_128_MD5 ||
						 cipherSuite == SSL_DH_anon_EXPORT_WITH_DES40_CBC_SHA ||
						 cipherSuite == SSL_DH_anon_WITH_DES_CBC_SHA ||
						 cipherSuite == SSL_DH_anon_WITH_3DES_EDE_CBC_SHA ||
						 cipherSuite == SSL_RSA_WITH_NULL_MD5 ||
						 cipherSuite == TLS_DH_anon_WITH_AES_128_CBC_SHA ||
						 cipherSuite == TLS_DH_anon_WITH_AES_256_CBC_SHA) {
						/* close the connnection and return errSSLBadCipherSuite */
						(void) SSLClose(ssl);
						ctxt->_error.error = errSSLBadCipherSuite;
						ctxt->_error.domain = kRSStreamErrorDomainSSL;
					}
				}
			}
		}
		
		/* Either way, it's done.  Mark the SSL bit for performance checks. */
		__RSBitSet(ctxt->_flags, kFlagBitUseSSL);
		__RSBitSet(ctxt->_flags, kFlagBitIsBuffered);
		_SocketStreamRemoveHandshake_NoLock(ctxt, _PerformSecurityHandshake_NoLock);
	}
}


/* static */ void
_PerformSecuritySendHandshake_NoLock(_RSSocketStreamContext* ctxt) {
	
	OSStatus result;
	RSIndex bytesWritten;
	
	SSLContextRef ssl = *((SSLContextRef*)RSDataGetBytePtr((RSDataRef)RSDictionaryGetValue(ctxt->_properties,
																						   kRSStreamPropertySocketSSLContext)));
	
	/* Attempt to write. */
	result = SSLWrite(ssl, NULL, 0, (size_t*)(&bytesWritten));
	
	/* If didn't get a block, can do something. */
	if (result == errSSLWouldBlock)
		RSSocketEnableCallBacks(ctxt->_socket, kRSSocketWriteCallBack);
	else {
		
		/* Remove the "send" handshake. */
		_SocketStreamRemoveHandshake_NoLock(ctxt, _PerformSecuritySendHandshake_NoLock);
			
		/* Need to save real errors. */
		if (result) {
			ctxt->_error.error = result;
			ctxt->_error.domain = kRSStreamErrorDomainSSL;
		}
	}
}


/* static */ void
_SocketStreamSecurityClose_NoLock(_RSSocketStreamContext* ctxt) {
	
	/*
	** This function is required during close in order to fully flush ST
	** and dump the SSLContextRef.
	*/
	
	SSLContextRef ssl = *((SSLContextRef*)RSDataGetBytePtr((RSDataRef)RSDictionaryGetValue(ctxt->_properties,
																						   kRSStreamPropertySocketSSLContext)));
	
	/* Attempt to close and flush out any bytes. */
	while (!ctxt->_error.error && (errSSLWouldBlock == SSLClose(ssl))) {
		  
		RSTypeRef loopAndMode[2] = {RSRunLoopGetCurrent(), _kRSStreamSocketSecurityClosePrivateMode};

		/* Add the current loop and the private mode to the list */
		_SchedulesAddRunLoopAndMode(ctxt->_sharedloops, (RSRunLoopRef)loopAndMode[0], (RSStringRef)loopAndMode[1]);

		/* Make sure to schedule all the schedulables on this loop and mode. */
		RSArrayApplyFunction(ctxt->_schedulables,
						   RSRangeMake(0, RSArrayGetCount(ctxt->_schedulables)),
						   (RSArrayApplierFunction)_SchedulablesScheduleApplierFunction,
						   loopAndMode);

		/* Unlock the context to allow things to fire */
		RSSpinLockUnlock(&ctxt->_lock);

		/* Run the run loop just waiting for the end. */
		RSRunLoopRunInMode(_kRSStreamSocketSecurityClosePrivateMode, 1e+20, TRUE);

		/* Lock the context back up. */
		RSSpinLockLock(&ctxt->_lock);

		/* Make sure to unschedule all the schedulables on this loop and mode. */
		RSArrayApplyFunction(ctxt->_schedulables,
						   RSRangeMake(0, RSArrayGetCount(ctxt->_schedulables)),
						   (RSArrayApplierFunction)_SchedulablesUnscheduleApplierFunction,
						   loopAndMode);

		/* Remove this loop and private mode from the list. */
		_SchedulesRemoveRunLoopAndMode(ctxt->_sharedloops, (RSRunLoopRef)loopAndMode[0], (RSStringRef)loopAndMode[1]);
	}

	/* Destroy the SSLContext. */
	SSLDisposeContext(ssl);

	/* Remove it from the properties for no touch. */
	RSDictionaryRemoveValue(ctxt->_properties, kRSStreamPropertySocketSSLContext);
}


/* static */ BOOL
_SocketStreamSecuritySetContext_NoLock(_RSSocketStreamContext *ctxt, RSDataRef value) {

	RSDataRef wrapper = (RSDataRef)RSDictionaryGetValue(ctxt->_properties, kRSStreamPropertySocketSSLContext);
	SSLContextRef old = wrapper ? *((SSLContextRef*)RSDataGetBytePtr(wrapper)) : NULL;
	SSLContextRef security = value ? *((SSLContextRef*)RSDataGetBytePtr(value)) : NULL;
	
	if (old) {
		
		/* Can only do something if idle (not opened). */
		if (_SocketStreamSecurityGetSessionState_NoLock(ctxt) != kSSLIdle)
			return FALSE;
        
		/* If not setting the same, destroy the old. */
        if (security != old)
            SSLDisposeContext(old);
            
		/* Remove the one that was there. */
		RSDictionaryRemoveValue(ctxt->_properties, kRSStreamPropertySocketSSLContext);
	}
	
	/* If not setting a new one, remove what's there. */
	if (!security) {
		
		/* Remove the one that was there. */
		RSDictionaryRemoveValue(ctxt->_properties, kRSStreamPropertySocketSSLContext);
		
		/* Get rid of the SSL handshake. */
		_SocketStreamRemoveHandshake_NoLock(ctxt, _PerformSecurityHandshake_NoLock);
		
		return TRUE;
	}
	
	/* Set the read/write functions on the context and set the reference. */
	if ((!(ctxt->_error.error = SSLSetIOFuncs(security, (SSLReadFunc)_SecurityReadFunc_NoLock, (SSLWriteFunc)_SecurityWriteFunc_NoLock))) &&
		(!(ctxt->_error.error = SSLSetConnection(security, (SSLConnectionRef)ctxt))))
	{
		/* Add the handshake to the list of things to perform. */
		BOOL result = _SocketStreamAddHandshake_NoLock(ctxt, _PerformSecurityHandshake_NoLock);
		
		/* If added, save the context. */
		if (result)
			RSDictionarySetValue(ctxt->_properties, kRSStreamPropertySocketSSLContext, value);
		
        return result;
	}
	
	/* Error was set, so set the domain. */
	ctxt->_error.domain = kRSStreamErrorDomainSSL;

	return FALSE;
}


/* static */ BOOL
_SocketStreamSecuritySetInfo_NoLock(_RSSocketStreamContext* ctxt, RSDictionaryRef settings) {

    SSLContextRef security = NULL;
	RSDataRef wrapper = NULL;
	
	/* Try to clear the existing, if any. */
    if (!_SocketStreamSecuritySetContext_NoLock(ctxt, NULL))
        return FALSE;

	/* If no new settings, done. */
    if (!settings)
		return TRUE;

    do {
        RSTypeRef value = RSDictionaryGetValue(settings, kRSStreamSSLIsServer);
		RSBOOLRef check = (RSBOOLRef)RSDictionaryGetValue(ctxt->_properties, _kRSStreamPropertySocketSecurityAuthenticatesServerCertificate);
		
		/* Create a new SSLContext. */
        if (SSLNewContext((value && RSEqual(value, kRSBOOLTrue)), &security))
            break;
		
		/* Figure out the correct security level to set and set it. */
        value = RSDictionaryGetValue(settings, kRSStreamSSLLevel);
        if ((!value || RSEqual(value, kRSStreamSocketSecurityLevelNegotiatedSSL)) && SSLSetProtocolVersion(security, kTLSProtocol1))
            break;

        else if (value) {
                
            if (RSEqual(value, kRSStreamSocketSecurityLevelNone)) {
                SSLDisposeContext(security);
                return TRUE;
            }
            
            else if (RSEqual(value, kRSStreamSocketSecurityLevelSSLv2) && SSLSetProtocolVersion(security, kSSLProtocol2))
                break;
    
            else if (RSEqual(value, kRSStreamSocketSecurityLevelSSLv3) && SSLSetProtocolVersion(security, kSSLProtocol3Only))
                break;
    
            else if (RSEqual(value, kRSStreamSocketSecurityLevelTLSv1) && SSLSetProtocolVersion(security, kTLSProtocol1Only))
                break;
    
            else if (RSEqual(value, kRSStreamSocketSecurityLevelTLSv1SSLv3) &&
                    (SSLSetProtocolVersion(security, kTLSProtocol1) || SSLSetProtocolVersionEnabled(security, kSSLProtocol2, FALSE)))
            {
                    break;
            }
        }
		
		/* If old property for cert auth was used, set lax now.  New settings override. */
        if (check && (check == kRSBOOLFalse)) {

            if (SSLSetAllowsExpiredRoots(security, TRUE))
                break;
            
            if (SSLSetAllowsAnyRoot(security, TRUE))
                break;
        }
		
		/* Set all the different properties based upon dictionary settings. */
        value = RSDictionaryGetValue(settings, kRSStreamSSLAllowsExpiredCertificates);
        if (value && RSEqual(value, kRSBOOLTrue) && SSLSetAllowsExpiredCerts(security, TRUE))
            break;

        value = RSDictionaryGetValue(settings, kRSStreamSSLAllowsExpiredRoots);
        if (value && RSEqual(value, kRSBOOLTrue) && SSLSetAllowsExpiredRoots(security, TRUE))
            break;

        value = RSDictionaryGetValue(settings, kRSStreamSSLAllowsAnyRoot);
        if (value && RSEqual(value, kRSBOOLTrue) && SSLSetAllowsAnyRoot(security, TRUE))
            break;

        value = RSDictionaryGetValue(settings, kRSStreamSSLValidatesCertificateChain);
        if (value && RSEqual(value, kRSBOOLFalse) && SSLSetEnableCertVerify(security, FALSE))
            break;

        value = RSDictionaryGetValue(settings, kRSStreamSSLCertificates);
        if (value && SSLSetCertificate(security, value))
			break;
		
		/* Get the peer name or figure out the peer name to use. */
        value = RSDictionaryGetValue(settings, kRSStreamSSLPeerName);
        if (!value) {
            
			RSStringRef name = (RSStringRef)RSDictionaryGetValue(ctxt->_properties, _kRSStreamPropertySocketPeerName);
			RSTypeRef lookup = (RSTypeRef)RSDictionaryGetValue(ctxt->_properties, kRSStreamPropertySocketRemoteHost);
			
			/* Old property override. */
            if (check && (check == kRSBOOLFalse))
                value = kRSNull;
			
			/*
            ** **FIXME** Once the new improved CONNECT stuff is in, peer name override
            ** should go away.
			*/
            else if (name)
                value = name;
            
			/* Try to get the name of the host from the RSHost or RSNetService. */
            else if (lookup) {
                
                RSArrayRef names = RSHostGetNames((RSHostRef)lookup, NULL);
                if (names)
                    value = RSArrayGetValueAtIndex(names, 0);
            }
			
			else if ((lookup = (RSTypeRef)RSDictionaryGetValue(ctxt->_properties, kRSStreamPropertySocketRemoteNetService)))
				value = RSNetServiceGetTargetHost((RSNetServiceRef)lookup);
        }
        
		/* If got a name, try to set it. */
        if (value) {
			
			/* kRSNull value means don't check the peer name. */
            if (RSEqual(value, kRSNull)) {
				if (SSLSetPeerDomainName(security, NULL, 0))
					break;
			}
            
			/* Pull out the bytes and set 'cause ST doesn't do RSString's for this. */
            else {
                
                OSStatus err;
                UInt8 static_buffer[1024];
                UInt8* buffer = &static_buffer[0];
				RSIndex buffer_size = sizeof(static_buffer);
				RSAllocatorRef alloc = RSGetAllocator(ctxt->_properties);
				
				buffer = _RSStringGetOrCreateCString(alloc, value, static_buffer, &buffer_size, kRSStringEncodingUTF8);
				
				/* After pulling out the bytes, set the peer name. */
                err = SSLSetPeerDomainName(security, (const char*)buffer, *((size_t*)(&buffer_size)));
    
				/* Clean up the allocation if made. */
                if (buffer != &static_buffer[0])
                    RSAllocatorDeallocate(alloc, buffer);
                
                if (err)
                    break;
            }
        }
        
		/* Wrap the SSLContextRef as a RSData. */
		wrapper = RSDataCreate(RSGetAllocator(ctxt->_properties), (void*)&security, sizeof(security));
		
		/* Set the property. */
        if (!wrapper || !_SocketStreamSecuritySetContext_NoLock(ctxt, wrapper))
            break;
        
		RSRelease(wrapper);
		
        return TRUE;
        
    } while(1);

	/* Clean up the SSLContextRef on failure. */
    if (security)
        SSLDisposeContext(security);
	
	if (wrapper)
		RSRelease(wrapper);
    
    return FALSE;
}


/* static */ BOOL
_SocketStreamSecuritySetAuthenticatesServerCertificates_NoLock(_RSSocketStreamContext* ctxt, RSBOOLRef authenticates) {
	
	SSLContextRef ssl = *((SSLContextRef*)RSDataGetBytePtr((RSDataRef)RSDictionaryGetValue(ctxt->_properties,
																						   kRSStreamPropertySocketSSLContext)));
	
	do {
		RSTypeRef value = NULL;
		RSStringRef name = (RSStringRef)RSDictionaryGetValue(ctxt->_properties, _kRSStreamPropertySocketPeerName);
		RSTypeRef lookup = (RSTypeRef)RSDictionaryGetValue(ctxt->_properties, kRSStreamPropertySocketRemoteHost);
		
		/* Set lax if turning off */
		if (SSLSetAllowsExpiredRoots(ssl, authenticates ? FALSE : TRUE))
			break;
		
		/* Set lax if turning off */
		if (SSLSetAllowsAnyRoot(ssl, authenticates ? FALSE : TRUE))
			break;
		
		/* Set peer name as kRSNull if turning off. */
		if (authenticates == kRSBOOLFalse)
			value = kRSNull;
		
		/*
		** **FIXME** Once the new improved CONNECT stuff is in, peer name override
		** should go away.
		*/
		else if (name)
			value = name;
		
		/* Figure out the proper peer name for RSHost or RSNetService. */
		else if (lookup) {
			
			RSArrayRef names = RSHostGetNames((RSHostRef)lookup, NULL);
			if (names)
				value = RSArrayGetValueAtIndex(names, 0);
		}
		
		else if ((lookup = (RSTypeRef)RSDictionaryGetValue(ctxt->_properties, kRSStreamPropertySocketRemoteNetService)))
			value = RSNetServiceGetTargetHost((RSNetServiceRef)lookup);
		
		/* No value is a failure. */
		if (!value) break;
		
		/* kRSNull means no peer name check */
		if (RSEqual(value, kRSNull)) {
			if (SSLSetPeerDomainName(ssl, NULL, 0))
				break;
		}
		
		/* Pull out the bytes and set 'cause ST doesn't do RSString's for this. */
		else {
			
			OSStatus err;
			UInt8 static_buffer[1024];
			UInt8* buffer = &static_buffer[0];
			RSIndex buffer_size = sizeof(static_buffer);
			RSAllocatorRef alloc = RSGetAllocator(ctxt->_properties);
				
			buffer = _RSStringGetOrCreateCString(alloc, value, static_buffer, &buffer_size, kRSStringEncodingUTF8);
			
			err = SSLSetPeerDomainName(ssl, (const char*)buffer, *((size_t*)(&buffer_size)));
			
			if (err)
				break;
		}
		
		return TRUE;
		
	} while (1);
	
	return FALSE;
}


/* static */ RSStringRef
_SecurityGetProtocol(SSLContextRef security) {

	SSLProtocol version = kSSLProtocolUnknown;
	
	/* Attempt to get the negotiated protocol */
	SSLGetNegotiatedProtocolVersion(security, &version);
	if (kSSLProtocolUnknown == version)
		SSLGetProtocolVersion(security, &version);
	
	/* Map the protocol from ST to RSSocketStream property values. */
	switch (version) {
	
		case kSSLProtocol2:
			return kRSStreamSocketSecurityLevelSSLv2;
			
		case kSSLProtocol3Only:
			return kRSStreamSocketSecurityLevelSSLv3;
			
		case kTLSProtocol1Only:
			return kRSStreamSocketSecurityLevelTLSv1;
			
		case kSSLProtocol3:
		case kTLSProtocol1:
		default:
        {
            BOOL enabled;
            
            if (!SSLGetProtocolVersionEnabled(security, kSSLProtocol2, &enabled) && !enabled)
                return kRSStreamSocketSecurityLevelTLSv1SSLv3;
            
            return kRSStreamSocketSecurityLevelNegotiatedSSL;
        }
	}
}


/* static */ SSLSessionState
_SocketStreamSecurityGetSessionState_NoLock(_RSSocketStreamContext* ctxt) {

	SSLContextRef ssl = *((SSLContextRef*)RSDataGetBytePtr((RSDataRef)RSDictionaryGetValue(ctxt->_properties,
																						   kRSStreamPropertySocketSSLContext)));
	
	/*
	** Slight fib to return Aborted if the SSL call fails, but if it does fail things are
	** quite hosed, so it's not such a lie.
	*/
	SSLSessionState state;
	return !SSLGetSessionState(ssl, &state) ? state : kSSLAborted;
}


#if 0
#pragma mark -
#pragma mark Extern Function Definitions (API)
#endif

/* extern */ void 
RSStreamCreatePairWithSocketToRSHost(RSAllocatorRef alloc, RSHostRef host, UInt32 port,
									 RSReadStreamRef* readStream, RSWriteStreamRef* writeStream)
{
	_RSSocketStreamContext* ctxt;
	
	/* NULL off the read stream if given */
	if (readStream) *readStream = NULL;
	
	/* Do the same to the write stream */
	if (writeStream) *writeStream = NULL;
	
	/* Create the context for the new socket stream. */
	ctxt = _SocketStreamCreateContext(alloc);
	
	/* Set up the rest if successful. */
	if (ctxt) {
		
		RSNumberRef num;
		
		port = port & 0x0000FFFF;
		num = RSNumberCreate(alloc, kRSNumberSInt32Type, &port);
		
		/* If the port wasn't created, just kill everything. */
		if (!num)				
			_SocketStreamDestroyContext_NoLock(alloc, ctxt);
		
		else {
			
			/* Add the peer host and port for connecting later. */
			RSDictionaryAddValue(ctxt->_properties, kRSStreamPropertySocketRemoteHost, host);
			RSDictionaryAddValue(ctxt->_properties, _kRSStreamPropertySocketRemotePort, num);
			
			/* Create the read stream if the client asked for it. */
			if (readStream) {
				*readStream = RSReadStreamCreate(alloc, &kSocketReadStreamCallBacks, ctxt);
				ctxt->_clientReadStream = *readStream;
			}
			
			/* Create the write stream if the client asked for it. */
			if (writeStream) {
				*writeStream = RSWriteStreamCreate(alloc, &kSocketWriteStreamCallBacks, ctxt);
				ctxt->_clientWriteStream = *writeStream;
			}
			
			if (readStream && *readStream && writeStream && *writeStream)
				__RSBitSet(ctxt->_flags, kFlagBitShared);
		}
		
		/* Release the port if it was created. */
		if (num) RSRelease(num);
	}
}


/* extern */ void 
RSStreamCreatePairWithSocketToNetService(RSAllocatorRef alloc, RSNetServiceRef service,
										 RSReadStreamRef* readStream, RSWriteStreamRef* writeStream)
{
	_RSSocketStreamContext* ctxt;
	
	/* NULL off the read stream if given */
	if (readStream) *readStream = NULL;
	
	/* Do the same to the write stream */
	if (writeStream) *writeStream = NULL;
	
	/* Create the context for the new socket stream. */
	ctxt = _SocketStreamCreateContext(alloc);
	
	/* Set up the rest if successful. */
	if (ctxt) {
		
		/* Add the peer service for connecting later. */
		RSDictionaryAddValue(ctxt->_properties, kRSStreamPropertySocketRemoteNetService, service);
		
		/* Create the read stream if the client asked for it. */
		if (readStream) {
			*readStream = RSReadStreamCreate(alloc, &kSocketReadStreamCallBacks, ctxt);
			ctxt->_clientReadStream = *readStream;
		}
		
		/* Create the write stream if the client asked for it. */
		if (writeStream) {
			*writeStream = RSWriteStreamCreate(alloc, &kSocketWriteStreamCallBacks, ctxt);
			ctxt->_clientWriteStream = *writeStream;
		}
		
		if (readStream && *readStream && writeStream && *writeStream)
			__RSBitSet(ctxt->_flags, kFlagBitShared);
	}
}


/* extern */ BOOL 
RSSocketStreamPairSetSecurityProtocol(RSReadStreamRef socketReadStream, RSWriteStreamRef socketWriteStream,
									  RSStreamSocketSecurityProtocol securityProtocol)
{
	BOOL result = FALSE;
	RSStringRef value = NULL;
	
	/* Map the old security levels to the new property values */
	switch (securityProtocol) {
	
		case kRSStreamSocketSecurityNone:
			value = kRSStreamSocketSecurityLevelNone;
			break;
			
		case kRSStreamSocketSecuritySSLv2:
			value = kRSStreamSocketSecurityLevelSSLv2;
			break;
			
		case kRSStreamSocketSecuritySSLv3:
			value = kRSStreamSocketSecurityLevelSSLv3;
			break;
			
		case kRSStreamSocketSecuritySSLv23:
			value = kRSStreamSocketSecurityLevelNegotiatedSSL;
			break;
			
		case kRSStreamSocketSecurityTLSv1:
			value = kRSStreamSocketSecurityLevelTLSv1;
			break;
			
		default:
			return result;  /* Early bail because of bad value */
	}
	
	/* Try setting on the read stream first */
	if (socketReadStream) {
	
		result = RSReadStreamSetProperty(socketReadStream,
										 kRSStreamPropertySocketSecurityLevel,
										 value);
	}
	
	/* If there was no read stream, try the write stream */
	else if (socketWriteStream) {
	
		result = RSWriteStreamSetProperty(socketWriteStream,
										  kRSStreamPropertySocketSecurityLevel,
										  value);
	}
	
	return result;
}


#if 0
#pragma mark -
#pragma mark Extern Function Definitions (SPI)
#endif


extern void
_RSStreamCreatePairWithRSSocketSignaturePieces(RSAllocatorRef alloc, SInt32 protocolFamily, SInt32 socketType,
											   SInt32 protocol, RSDataRef address, RSReadStreamRef* readStream,
											   RSWriteStreamRef* writeStream)
{
	_RSSocketStreamContext* ctxt;

	/* NULL off the read stream if given */
	if (readStream) *readStream = NULL;
	
	/* Do the same to the write stream */
	if (writeStream) *writeStream = NULL;
	
	/* Create the context for the new socket stream. */
	ctxt = _SocketStreamCreateContext(alloc);
	
	/* Set up the rest if successful. */
	if (ctxt) {
		
		RSDictionaryValueCallBacks cb = {0, NULL, NULL, NULL, NULL};
		
		/* Create a host from the address in order to connect later */
		RSHostRef h = RSHostCreateWithAddress(alloc, address);
		
		RSStringRef keys[] = {
			_kRSStreamSocketFamily,
			_kRSStreamSocketType,
			_kRSStreamSocketProtocol
		};
		SInt32 values[] = {
			protocolFamily,
			socketType,
			protocol
		};
		RSDictionaryRef props = RSDictionaryCreate(alloc,
												   (const void**)keys,
												   (const void**)values,
												   (sizeof(values) / sizeof(values[0])),
												   &kRSTypeDictionaryKeyCallBacks,
												   &cb);
		
		/* If the socket properties or host wasn't created, just kill everything. */
		if (!props || !h)
			_SocketStreamDestroyContext_NoLock(alloc, ctxt);
		
		else {

			RSDictionaryAddValue(ctxt->_properties, _kRSStreamPropertySocketFamilyTypeProtocol, props);
			
			/* Add the host as the far end for connecting later. */
			RSDictionaryAddValue(ctxt->_properties, kRSStreamPropertySocketRemoteHost, h);
			
			/* Create the read stream if the client asked for it. */
			if (readStream) {
				*readStream = RSReadStreamCreate(alloc, &kSocketReadStreamCallBacks, ctxt);
				ctxt->_clientReadStream = *readStream;
			}
			
			/* Create the write stream if the client asked for it. */
			if (writeStream) {
				*writeStream = RSWriteStreamCreate(alloc, &kSocketWriteStreamCallBacks, ctxt);
				ctxt->_clientWriteStream = *writeStream;
			}
			
			if (readStream && *readStream && writeStream && *writeStream)
				__RSBitSet(ctxt->_flags, kFlagBitShared);
		}
		
		/* Release the host if it was created. */
		if (h) RSRelease(h);

		/* Release the socket properties if it was created. */
		if (props) RSRelease(props);
	}
}


/* extern */ void 
RSStreamCreatePairWithNetServicePieces(RSAllocatorRef alloc, RSStringRef domain, RSStringRef serviceType,
									   RSStringRef name, RSReadStreamRef* readStream, RSWriteStreamRef* writeStream)
{

	/* Create a service to call directly over to the real API. */
	RSNetServiceRef service = RSNetServiceCreate(alloc, domain, serviceType, name, 0);
	
	/* NULL off the read stream if given */
	if (readStream) *readStream = NULL;
		
	/* Do the same to the write stream */
	if (writeStream) *writeStream = NULL;
	
	if (service) {
		
		/* Call the real API if there is a service */
		RSStreamCreatePairWithSocketToNetService(alloc, service, readStream, writeStream);
		
		/* No longer needed */
		RSRelease(service);
	}
}


/* extern */ void
_RSSocketStreamCreatePair(RSAllocatorRef alloc, RSStringRef host, UInt32 port, RSSocketNativeHandle s,
						  const RSSocketSignature* sig, RSReadStreamRef* readStream, RSWriteStreamRef* writeStream)
{

	/* Protect against a bad entry. */
    if (!readStream && !writeStream) return;
	
	/* NULL off the read stream if given */
	if (readStream) *readStream = NULL;
	
	/* Do the same to the write stream */
	if (writeStream) *writeStream = NULL;
	
	/* Being created with a host? */
	if (host) {
		
		/* Create a RSHost wrapper for stream creation */
		RSHostRef h = RSHostCreateWithName(alloc, host);
		
		/* Only make the streams if created host */
		if (h) {
			
			/* Create the streams */
			RSStreamCreatePairWithSocketToRSHost(alloc, h, port, readStream, writeStream);
			
			/* Don't need the host anymore. */
			RSRelease(h);
		}
	}
	
	/* Being created with a RSSocketSignature? */
	else if (sig) {
	
		/* Create the streams from the pieces of the RSSocketSignature. */
		_RSStreamCreatePairWithRSSocketSignaturePieces(alloc,
													   sig->protocolFamily,
													   sig->socketType,
													   sig->protocol,
													   sig->address,
													   readStream,
													   writeStream);
	}
	
	else {
		
		/* Create the context for the new socket stream. */
		_RSSocketStreamContext* ctxt = _SocketStreamCreateContext(alloc);
		
		/* Set up the rest if successful. */
		if (ctxt) {
				
			/* Create the wrapper for the socket.  Can't create the RSSocket until open (3784821). */
			RSDataRef wrapper = RSDataCreate(alloc, (const void*)(&s), sizeof(s));
			
			/* Mark as coming from a native socket handle */
			__RSBitSet(ctxt->_flags, kFlagBitCreatedNative);
			
			/* If the socket wasn't created, just kill everything. */
			if (!wrapper)				
				_SocketStreamDestroyContext_NoLock(alloc, ctxt);
			
			else {
				
				/* Save the native socket handle until open. */
				RSDictionaryAddValue(ctxt->_properties, kRSStreamPropertySocketNativeHandle, wrapper);
				
				/* 3938584 Make sure to release the wrapper now that it's been retained by the properties. */
				RSRelease(wrapper);
				
				/* Streams created with a native socket don't automatically close the underlying native socket. */
				RSDictionaryAddValue(ctxt->_properties, kRSStreamPropertyShouldCloseNativeSocket, kRSBOOLFalse);
				
				/* Create the read stream if the client asked for it. */
				if (readStream) {
					*readStream = RSReadStreamCreate(alloc, &kSocketReadStreamCallBacks, ctxt);
					ctxt->_clientReadStream = *readStream;
				}
				
				/* Create the write stream if the client asked for it. */
				if (writeStream) {
					*writeStream = RSWriteStreamCreate(alloc, &kSocketWriteStreamCallBacks, ctxt);
					ctxt->_clientWriteStream = *writeStream;
				}
				
				if (readStream && *readStream && writeStream && *writeStream)
					__RSBitSet(ctxt->_flags, kFlagBitShared);
			}			
		}
	}
}

