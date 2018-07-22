//
//  RSSocketStream.c
//  RSCoreFoundation
//
//  Created by closure on 9/28/15.
//  Copyright © 2015 RetVal. All rights reserved.
//

#include "RSSocketStream.h"
#include <RSCoreFoundation/RSStream.h>
#include <RSCoreFoundation/RSNumber.h>
#include "RSInternal.h"
#include "RSStreamInternal.h"
#include "RSStreamPrivate.h"
#include <RSCoreFoundation/RSRuntime.h>

#if DEPLOYMENT_TARGET_MACOSX || DEPLOYMENT_TARGET_EMBEDDED
// On Mach these live in RS for historical reasons, even though they are declared in RSNetwork

const int RSStreamErrorDomainSSL = 3;
const int RSStreamErrorDomainSOCKS = 5;

RS_PUBLIC_CONST_STRING_DECL(RSStreamPropertyShouldCloseNativeSocket, "RSStreamPropertyShouldCloseNativeSocket")
RS_PUBLIC_CONST_STRING_DECL(RSStreamPropertyAutoErrorOnSystemChange, "RSStreamPropertyAutoErrorOnSystemChange");

RS_PUBLIC_CONST_STRING_DECL(RSStreamPropertySOCKSProxy, "RSStreamPropertySOCKSProxy")
RS_PUBLIC_CONST_STRING_DECL(RSStreamPropertySOCKSProxyHost, "SOCKSProxy")
RS_PUBLIC_CONST_STRING_DECL(RSStreamPropertySOCKSProxyPort, "SOCKSPort")
RS_PUBLIC_CONST_STRING_DECL(RSStreamPropertySOCKSVersion, "RSStreamPropertySOCKSVersion")
RS_PUBLIC_CONST_STRING_DECL(RSStreamSocketSOCKSVersion4, "RSStreamSocketSOCKSVersion4")
RS_PUBLIC_CONST_STRING_DECL(RSStreamSocketSOCKSVersion5, "RSStreamSocketSOCKSVersion5")
RS_PUBLIC_CONST_STRING_DECL(RSStreamPropertySOCKSUser, "RSStreamPropertySOCKSUser")
RS_PUBLIC_CONST_STRING_DECL(RSStreamPropertySOCKSPassword, "RSStreamPropertySOCKSPassword")

RS_PUBLIC_CONST_STRING_DECL(RSStreamPropertySocketSecurityLevel, "RSStreamPropertySocketSecurityLevel");
RS_PUBLIC_CONST_STRING_DECL(RSStreamSocketSecurityLevelNone, "RSStreamSocketSecurityLevelNone");
RS_PUBLIC_CONST_STRING_DECL(RSStreamSocketSecurityLevelSSLv2, "RSStreamSocketSecurityLevelSSLv2");
RS_PUBLIC_CONST_STRING_DECL(RSStreamSocketSecurityLevelSSLv3, "RSStreamSocketSecurityLevelSSLv3");
RS_PUBLIC_CONST_STRING_DECL(RSStreamSocketSecurityLevelTLSv1, "RSStreamSocketSecurityLevelTLSv1");
RS_PUBLIC_CONST_STRING_DECL(RSStreamSocketSecurityLevelNegotiatedSSL, "RSStreamSocketSecurityLevelNegotiatedSSL");

#endif


#if DEPLOYMENT_TARGET_WINDOWS
typedef void (*RS_SOCKET_STREAM_PAIR)(RSAllocatorRef, RSStringRef, UInt32, RSSocketHandle, const RSSocketSignature*, RSReadStreamRef*, RSWriteStreamRef*);
#endif

// These are duplicated in RSNetwork, who actually externs them in its headers
RS_PUBLIC_CONST_STRING_DECL(RSStreamPropertySocketSSLContext, "RSStreamPropertySocketSSLContext")
RS_PUBLIC_CONST_STRING_DECL(_RSStreamPropertySocketSecurityAuthenticatesServerCertificate, "_RSStreamPropertySocketSecurityAuthenticatesServerCertificate");


RS_EXPORT
void _RSSocketStreamSetAuthenticatesServerCertificateDefault(Boolean shouldAuthenticate) {
    RSLog(RSSTR("_RSSocketStreamSetAuthenticatesServerCertificateDefault(): This call has been deprecated.  Use SetProperty(_RSStreamPropertySocketSecurityAuthenticatesServerCertificate, RSBooleanTrue/False)\n"));
}


/* RS_EXPORT */ Boolean
_RSSocketStreamGetAuthenticatesServerCertificateDefault(void) {
    RSLog(RSSTR("_RSSocketStreamGetAuthenticatesServerCertificateDefault(): This call has been removed as a security risk.  Use security properties on individual streams instead.\n"));
    return FALSE;
}


/* RS_EXPORT */ void
_RSSocketStreamPairSetAuthenticatesServerCertificate(RSReadStreamRef rStream, RSWriteStreamRef wStream, Boolean authenticates) {
    
    RSNumberRef value = (!authenticates ? RSBooleanFalse : RSBooleanTrue);
    
    if (rStream)
        RSReadStreamSetProperty(rStream, _RSStreamPropertySocketSecurityAuthenticatesServerCertificate, value);
    else
        RSWriteStreamSetProperty(wStream, _RSStreamPropertySocketSecurityAuthenticatesServerCertificate, value);
}


// Flags for dyld loading of libraries.
enum {
    kTriedToLoad = 0,
    kInitialized
};

static struct {
    RSSpinLock lock;
    UInt32	flags;
#if DEPLOYMENT_TARGET_WINDOWS
    HMODULE	image;
#endif
    void (*_RSSocketStreamCreatePair)(RSAllocatorRef, RSStringRef, UInt32, RSSocketHandle, const RSSocketSignature*, RSReadStreamRef*, RSWriteStreamRef*);
    RSErrorRef (*_RSErrorCreateWithStreamError)(RSAllocatorRef, RSStreamError*);
    RSStreamError (*_RSStreamErrorFromRSError)(RSErrorRef);
} RSNetworkSupport = {
    RSSpinLockInit,
    0x0,
#if DEPLOYMENT_TARGET_WINDOWS
    NULL,
#endif
    NULL,
    NULL,
    NULL
};

#define RSNETWORK_CALL(sym, args)		((RSNetworkSupport.sym)args)

#if DEPLOYMENT_TARGET_MACOSX || DEPLOYMENT_TARGET_EMBEDDED
#define RSNETWORK_LOAD_SYM(sym)   __RSLookupRSNetworkFunction(#sym)
#elif DEPLOYMENT_TARGET_WINDOWS
#define RSNETWORK_LOAD_SYM(sym)   (void *)GetProcAddress(RSNetworkSupport.image, #sym)
#endif

static void initializeRSNetworkSupport(void) {
    __RSBitSet(RSNetworkSupport.flags, kTriedToLoad);
#if DEPLOYMENT_TARGET_MACOSX || DEPLOYMENT_TARGET_EMBEDDED
    RSNetworkSupport._RSSocketStreamCreatePair = RSNETWORK_LOAD_SYM(_RSSocketStreamCreatePair);
    RSNetworkSupport._RSErrorCreateWithStreamError = RSNETWORK_LOAD_SYM(_RSErrorCreateWithStreamError);
    RSNetworkSupport._RSStreamErrorFromRSError = RSNETWORK_LOAD_SYM(_RSStreamErrorFromRSError);
#elif DEPLOYMENT_TARGET_WINDOWS
    if (!RSNetworkSupport.image) {
#if _DEBUG
        RSNetworkSupport.image = GetModuleHandleW(L"RSNetwork_debug.dll");
#else
        RSNetworkSupport.image = GetModuleHandleW(L"RSNetwork.dll");
#endif
    }
    
    if (!RSNetworkSupport.image) {
        // not loaded yet, try to load from the filesystem
        char path[MAX_PATH+1];
        if (!RSNetworkSupport.image) {
            strlcpy(path, (const char *)_RSDLLPath(), sizeof(path));
#if _DEBUG
            strlcat(path, "\\RSNetwork_debug.dll", sizeof(path));
#else
            strlcat(path, "\\RSNetwork.dll", sizeof(path));
#endif
            RSNetworkSupport.image = LoadLibraryA(path);
        }
    }
    
    if (!RSNetworkSupport.image) {
        RSLog(RSSTR("CoreFoundation: failed to dynamically load RSNetwork"));
    } else  {
        RSNetworkSupport._RSSocketStreamCreatePair = (RS_SOCKET_STREAM_PAIR)RSNETWORK_LOAD_SYM(_RSSocketStreamCreatePair);
        RSNetworkSupport._RSErrorCreateWithStreamError = (RSErrorRef(*)(RSAllocatorRef, RSStreamError *))RSNETWORK_LOAD_SYM(_RSErrorCreateWithStreamError);
        RSNetworkSupport._RSStreamErrorFromRSError = (RSStreamError(*)(RSErrorRef))RSNETWORK_LOAD_SYM(_RSStreamErrorFromRSError);
    }
#endif
    
    if (!RSNetworkSupport._RSSocketStreamCreatePair) RSLog(RSSTR("CoreFoundation: failed to dynamically link symbol _RSSocketStreamCreatePair"));
    if (!RSNetworkSupport._RSErrorCreateWithStreamError) RSLog(RSSTR("CoreFoundation: failed to dynamically link symbol _RSErrorCreateWithStreamError"));
    if (!RSNetworkSupport._RSStreamErrorFromRSError) RSLog(RSSTR("CoreFoundation: failed to dynamically link symbol _RSStreamErrorFromRSError"));
    
    __RSBitSet(RSNetworkSupport.flags, kInitialized);
}

static void
createPair(RSAllocatorRef alloc, RSStringRef host, UInt32 port, RSSocketHandle sock, const RSSocketSignature* sig, RSReadStreamRef *readStream, RSWriteStreamRef *writeStream)
{
    if (readStream)
        *readStream = NULL;
    
    if (writeStream)
        *writeStream = NULL;
    
    RSSpinLockLock(&(RSNetworkSupport.lock));
    if (!__RSBitIsSet(RSNetworkSupport.flags, kTriedToLoad)) initializeRSNetworkSupport();
    RSSpinLockUnlock(&(RSNetworkSupport.lock));
    
    RSNETWORK_CALL(_RSSocketStreamCreatePair, (alloc, host, port, sock, sig, readStream, writeStream));
}


RS_EXPORT void RSStreamCreatePairWithSocket(RSAllocatorRef alloc, RSSocketHandle sock, RSReadStreamRef *readStream, RSWriteStreamRef *writeStream) {
    createPair(alloc, NULL, 0, sock, NULL, readStream, writeStream);
}

RS_EXPORT void RSStreamCreatePairWithSocketToHost(RSAllocatorRef alloc, RSStringRef host, UInt32 port, RSReadStreamRef *readStream, RSWriteStreamRef *writeStream) {
    createPair(alloc, host, port, 0, NULL, readStream, writeStream);
}

RS_EXPORT void RSStreamCreatePairWithPeerSocketSignature(RSAllocatorRef alloc, const RSSocketSignature* sig, RSReadStreamRef *readStream, RSWriteStreamRef *writeStream) {
    createPair(alloc, NULL, 0, 0, sig, readStream, writeStream);
}

RSPrivate RSStreamError _RSStreamErrorFromError(RSErrorRef error) {
    RSStreamError result;
    Boolean canUpCall;
    
    RSSpinLockLock(&(RSNetworkSupport.lock));
    if (!__RSBitIsSet(RSNetworkSupport.flags, kTriedToLoad)) initializeRSNetworkSupport();
    canUpCall = (RSNetworkSupport._RSStreamErrorFromRSError != NULL);
    RSSpinLockUnlock(&(RSNetworkSupport.lock));
    
    if (canUpCall) {
        result = RSNETWORK_CALL(_RSStreamErrorFromRSError, (error));
    } else {
        RSStringRef domain = RSErrorGetDomain(error);
        if (RSEqual(domain, RSErrorDomainPOSIX)) {
            result.domain = RSStreamErrorDomainPOSIX;
        } else if (RSEqual(domain, RSErrorDomainOSStatus)) {
            result.domain = RSStreamErrorDomainMacOSStatus;
        } else if (RSEqual(domain, RSErrorDomainMach)) {
            result.domain = 11; // RSStreamErrorDomainMach, but that symbol is in RSNetwork
        } else {
            result.domain = RSStreamErrorDomainCustom;
        }
        result.error = RSErrorGetCode(error);
    }
    return result;
}

RSPrivate RSErrorRef _RSErrorFromStreamError(RSAllocatorRef alloc, RSStreamError *streamError) {
    RSErrorRef result;
    BOOL canUpCall;
    
    RSSpinLockLock(&(RSNetworkSupport.lock));
    if (!__RSBitIsSet(RSNetworkSupport.flags, kTriedToLoad)) initializeRSNetworkSupport();
    canUpCall = (RSNetworkSupport._RSErrorCreateWithStreamError != NULL);
    RSSpinLockUnlock(&(RSNetworkSupport.lock));
    
    if (canUpCall) {
        result = RSNETWORK_CALL(_RSErrorCreateWithStreamError, (alloc, streamError));
    } else {
        if (streamError->domain == RSStreamErrorDomainPOSIX) {
            return RSErrorCreate(alloc, RSErrorDomainPOSIX, streamError->error, NULL);
        } else if (streamError->domain == RSStreamErrorDomainMacOSStatus) {
            return RSErrorCreate(alloc, RSErrorDomainOSStatus, streamError->error, NULL);
        } else {
            RSStringRef key = RSSTR("RSStreamErrorDomainKey");
            RSNumberRef value = RSNumberCreate(alloc, RSNumberRSIndex, &streamError->domain);
            RSDictionaryRef dict = RSDictionaryCreate(alloc, (const void **)(&key), (const void **)(&value), 1, RSDictionaryRSTypeContext);
            result = RSErrorCreate(alloc, RSSTR("BogusRSStreamErrorCompatibilityDomain"), streamError->error, dict);
            RSRelease(value);
            RSRelease(dict);
        }
    }
    return result;
}
