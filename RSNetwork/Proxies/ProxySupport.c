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
 *  ProxySupport.c
 *  RSNetwork
 *
 *  Created by Jeremy Wyld on 11/4/04.
 *  Copyright 2004 Apple Computer, Inc. All rights reserved.
 *
 */

#include "ProxySupport.h"

#include <RSNetwork/RSNetworkPriv.h>
#include "RSNetworkInternal.h"
#include <RSCoreFoundation/RSStreamPrivate.h>

#ifndef __WIN32__
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#endif


#if defined(__MACH__)
#include <JavaScriptCore/JavaScriptCore.h>
#include <SystemConfiguration/SystemConfiguration.h>


#ifndef __CONSTANT_RSSTRINGS__
#define _kProxySupportRSNetworkBundleID		RSSTR("com.apple.RSNetwork")
#define _kProxySupportLocalhost				RSSTR("localhost")
#define _kProxySupportIPv4Loopback			RSSTR("127.0.0.1")
#define _kProxySupportIPv6Loopback			RSSTR("::1")
#define _kProxySupportDot					RSSTR(".")
#define _kProxySupportSlash					RSSTR("/")
#define _kProxySupportDotZero				RSSTR(".0")
#define _kProxySupportDotFormat				RSSTR(".%@")
#define _kProxySupportStar					RSSTR("*")
#define _kProxySupportColon					RSSTR(":")
#define _kProxySupportSemiColon				RSSTR(";")
#define _kProxySupportFILEScheme			RSSTR("file")
#define _kProxySupportHTTPScheme			RSSTR("http")
#define _kProxySupportHTTPSScheme			RSSTR("https")
#define _kProxySupportHTTPPort				RSSTR("80")
#define _kProxySupportFTPScheme				RSSTR("ftp")
#define _kProxySupportFTPSScheme			RSSTR("ftps")
#define _kProxySupportFTPPort				RSSTR("21")
#define _kProxySupportSOCKS4Scheme			RSSTR("socks4")
#define _kProxySupportSOCKS5Scheme			RSSTR("socks5")
#define _kProxySupportSOCKSPort				RSSTR("1080")
#define _kProxySupportDIRECT				RSSTR("DIRECT")
#define _kProxySupportPROXY					RSSTR("PROXY")
#define _kProxySupportSOCKS					RSSTR("SOCKS")
#define _kProxySupportGETMethod				RSSTR("GET")
#define _kProxySupportURLLongFormat			RSSTR("%@://%@:%@@%@:%d")
#define _kProxySupportURLShortFormat		RSSTR("%@://%@:%d")
#define _kProxySupportExceptionsList		RSSTR("ExceptionsList")
#define _kProxySupportLoadingPacPrivateMode	RSSTR("_kProxySupportLoadingPacPrivateMode")
#define _kProxySupportPacSupportFileName	RSSTR("PACSupport")
#define _kProxySupportJSExtension			RSSTR("js")
#define _kProxySupportExpiresHeader			RSSTR("Expires")
#define _kProxySupportNowHeader                 RSSTR("Date")
#else
RS_CONST_STRING_DECL(_kProxySupportRSNetworkBundleID, "com.apple.RSNetwork")
RS_CONST_STRING_DECL(_kProxySupportLocalhost, "localhost")
RS_CONST_STRING_DECL(_kProxySupportIPv4Loopback, "127.0.0.1")
RS_CONST_STRING_DECL(_kProxySupportIPv6Loopback, "::1")
RS_CONST_STRING_DECL(_kProxySupportDot, ".")
RS_CONST_STRING_DECL(_kProxySupportSlash, "/")
RS_CONST_STRING_DECL(_kProxySupportDotZero, ".0")
RS_CONST_STRING_DECL(_kProxySupportDotFormat, ".%@")
RS_CONST_STRING_DECL(_kProxySupportStar, "*")
RS_CONST_STRING_DECL(_kProxySupportColon, ":")
RS_CONST_STRING_DECL(_kProxySupportSemiColon, ";")
RS_CONST_STRING_DECL(_kProxySupportFILEScheme, "file")
RS_CONST_STRING_DECL(_kProxySupportHTTPScheme, "http")
RS_CONST_STRING_DECL(_kProxySupportHTTPSScheme, "https")
RS_CONST_STRING_DECL(_kProxySupportHTTPPort, "80")
RS_CONST_STRING_DECL(_kProxySupportFTPScheme, "ftp")
RS_CONST_STRING_DECL(_kProxySupportFTPSScheme, "ftps")
RS_CONST_STRING_DECL(_kProxySupportFTPPort, "21")
RS_CONST_STRING_DECL(_kProxySupportSOCKS4Scheme, "socks4")
RS_CONST_STRING_DECL(_kProxySupportSOCKS5Scheme, "socks5")
RS_CONST_STRING_DECL(_kProxySupportSOCKSPort, "1080")
RS_CONST_STRING_DECL(_kProxySupportDIRECT, "DIRECT")
RS_CONST_STRING_DECL(_kProxySupportPROXY, "PROXY")
RS_CONST_STRING_DECL(_kProxySupportSOCKS, "SOCKS")
RS_CONST_STRING_DECL(_kProxySupportGETMethod, "GET")
RS_CONST_STRING_DECL(_kProxySupportURLLongFormat, "%@://%@:%@@%@:%d")
RS_CONST_STRING_DECL(_kProxySupportURLShortFormat, "%@://%@:%d")
RS_CONST_STRING_DECL(_kProxySupportExceptionsList, "ExceptionsList")
RS_CONST_STRING_DECL(_kProxySupportLoadingPacPrivateMode, "_kProxySupportLoadingPacPrivateMode")
RS_CONST_STRING_DECL(_kProxySupportPacSupportFileName, "PACSupport")
RS_CONST_STRING_DECL(_kProxySupportJSExtension, "js")
RS_CONST_STRING_DECL(_kProxySupportExpiresHeader, "Expires")
RS_CONST_STRING_DECL(_kProxySupportNowHeader, "Date")
#endif	/* __CONSTANT_RSSTRINGS__ */


static JSObjectRef _JSDnsResolveFunction(void* context, JSObjectRef ctxt, RSArrayRef args);
static JSObjectRef _JSPrimaryIpv4AddressesFunction(void* context, JSObjectRef ctxt, RSArrayRef args);

#elif defined(__WIN32__)

#include <winsock2.h>
#include <ws2tcpip.h>	// for ipv6
#include <wininet.h>	// for InternetTimeToSystemTime
                        // WinHTTP has the same function, but it has more OS/SP constraints
#include <objbase.h>

// RunGuts are defined below, with other COM code

RS_EXPORT RSAbsoluteTime _RSAbsoluteTimeFromFileTime(FILETIME* ftime);

// Returns the path to the RS DLL, which we can then use to find resources
static const char *_RSDLLPath(void) {
    static TCHAR cachedPath[MAX_PATH+1] = "";
    
    if ('\0' == cachedPath[0]) {
#if defined(DEBUG)
        char *DLLFileName = "RSNetwork_debug";
#elif defined(PROFILE)
        char *DLLFileName = "RSNetwork_profile";
#else
        char *DLLFileName = "RSNetwork";
#endif
        HMODULE ourModule = GetModuleHandle(DLLFileName);
        assert(ourModule);      // GetModuleHandle failed to find our own DLL
        
        DWORD wResult = GetModuleFileName(ourModule, cachedPath, MAX_PATH+1);
        assert(wResult > 0);            // GetModuleFileName failure
        assert(wResult < MAX_PATH+1);   // GetModuleFileName result truncated
		
        // strip off last component, the DLL name
        RSIndex idx;
        for (idx = wResult - 1; idx; idx--) {
            if ('\\' == cachedPath[idx]) {
                cachedPath[idx] = '\0';
                break;
            }
        }
    }
    return cachedPath;
}

#endif	/* __WIN32__ */

static RSStringRef _JSFindProxyForURL(RSURLRef pac, RSURLRef url, RSStringRef host);
static RSStringRef _JSFindProxyForURLAsync(RSURLRef pac, RSURLRef url, RSStringRef host, BOOL *mustBlock);


#define PAC_STREAM_LOAD_TIMEOUT		30.0

static RSReadStreamRef BuildStreamForPACURL(RSAllocatorRef alloc, RSURLRef pacURL, RSURLRef targetURL, RSStringRef targetScheme, RSStringRef targetHost, _RSProxyStreamCallBack callback, void *clientInfo);
static RSStringRef _loadJSSupportFile(void);
static RSStringRef _loadPARSile(RSAllocatorRef alloc, RSURLRef pac, RSAbsoluteTime *expires, RSStreamError *err);
static JSContextRef _createJSRuntime(RSAllocatorRef alloc, RSStringRef js_support, RSStringRef js_pac);
static void _freeJSRuntime(JSContextRef runtime);
static RSStringRef _callPARSunction(RSAllocatorRef alloc, JSContextRef runtime, RSURLRef url, RSStringRef host);
static RSArrayRef _resolveDNSName(RSStringRef name);
static RSReadStreamRef _streamForPARSile(RSAllocatorRef alloc, RSURLRef pac, BOOL *isFile);
RSStringRef _stringFromLoadedPACStream(RSAllocatorRef alloc, RSMutableDataRef contents, RSReadStreamRef stream, RSAbsoluteTime *expires);
static void _JSSetEnvironmentForPAC(RSAllocatorRef alloc, RSURLRef url, RSAbsoluteTime expires, RSStringRef pacString);

/*
 ** Determine whether a given "enabled" entry ("HTTPEnable", "HTTPSEnable", ...) means 
 ** that the described proxy should be enabled.
 **
 ** Although this seems wrong, a NULL entry means the proxy SHOULD be enabled.  The
 ** idea is that a developer could create their own proxy dictionary which would be
 ** missing the "enable" flag.  In this case, the lack of the "enable" flag should 
 ** be interpreted as enabled if the other relevant proxy information is available 
 ** (e.g. the proxy host name).
 **
 ** Also, because SysConfig does little to no checking of its values, we must be prepared
 ** for an arbitrary RSTypeRef, just as if the value were coming out of RSPrefs.
 */
static inline BOOL _proxyEnabled(RSTypeRef enabledEntry) {
    if (!enabledEntry) return TRUE;
    if (RSGetTypeID(enabledEntry) == RSNumberGetTypeID()) {
        SInt32 val;
        RSNumberGetValue(enabledEntry, &val);
        return (val != 0);
    }
    return (enabledEntry == RSBooleanTrue);
}


// Currently not used, so hand dead-stripping for now.
#if 0
/* extern */ BOOL
_RSNetworRSHostDoesNeedProxy(RSHostRef host, RSArrayRef bypasses, RSNumberRef localBypass) {
    
    RSArrayRef names = RSHostGetNames(host, NULL);
	
	localBypass = _proxyEnabled(localBypass) ? RSBooleanTrue : RSBooleanFalse;
    
    if (names && RSArrayGetCount(names)) {
        return _RSNetworkDoesNeedProxy((RSStringRef)RSArrayObjectAtIndex(names, 0), bypasses, localBypass);
    }
    
    names = RSHostGetAddressing(host, NULL);
    if (names && RSArrayGetCount(names)) {
        
        RSDataRef saData = (RSDataRef)RSArrayObjectAtIndex(names, 0);
        RSStringRef name = stringFromAddr((const struct sockaddr*)RSDataGetBytesPtr(saData), RSDataGetLength(saData));
        if (name) {
            BOOL result = _RSNetworkDoesNeedProxy(name, bypasses, localBypass);
            RSRelease(name);
            return result;
        }
        return FALSE;
    }
    
    return TRUE;
}
#endif


/* extern */ BOOL
_RSNetworkDoesNeedProxy(RSStringRef hostname, RSArrayRef bypasses, RSNumberRef localBypass) {
	
    BOOL result = TRUE;
	
    RSIndex i, hostnc, count, length;
    RSArrayRef hostnodes;
	
	struct in_addr ip;
	BOOL is_ip = FALSE;
    
    if (!hostname) return TRUE;
	
	localBypass = _proxyEnabled(localBypass) ? RSBooleanTrue : RSBooleanFalse;
    
    if (RSStringCompare(hostname, _kProxySupportLocalhost, RSCompareCaseInsensitive) == RSCompareEqualTo)
        return FALSE;
    if (RSStringCompare(hostname, _kProxySupportIPv4Loopback, RSCompareCaseInsensitive) == RSCompareEqualTo)
        return FALSE;
    if (RSStringCompare(hostname, _kProxySupportIPv6Loopback, RSCompareCaseInsensitive) == RSCompareEqualTo)
        return FALSE;
	
	length = RSStringGetLength(hostname);

	/* Uncomment the following code to bypass .local. addresses by default */
	/*
	if ((length > 7) && RSStringCompareWithOptions(hostname, RSSTR(".local."), RSMakeRange(length - 7, 7), RSCompareCaseInsensitive) == RSCompareEqualTo)
		return FALSE;
	*/
    RSRange Result;
    if (localBypass && RSEqual(localBypass, RSBooleanTrue) && !RSStringFind(hostname, _kProxySupportDot, RSStringGetRange(hostname), &Result))
        return FALSE;
        
    count = bypasses ? RSArrayGetCount(bypasses) : 0;
    if (!count) return result;
    
    hostnodes = RSStringCreateComponentsSeparatedByStrings(NULL, hostname, _kProxySupportDot);
    hostnc = hostnodes ? RSArrayGetCount(hostnodes) : 0;
    if (!hostnc) {
        RSRelease(hostnodes);
        hostnodes = NULL;
    }
    
	if (((hostnc == 4) || ((hostnc == 5) && (RSStringGetLength((RSStringRef)RSArrayObjectAtIndex(hostnodes, 4)) == 0))) && 
		(length <= 16))
	{
		UInt8 stack_buffer[32];
		UInt8* buffer = stack_buffer;
		RSIndex bufferLength = sizeof(stack_buffer);
		RSAllocatorRef allocator = RSGetAllocator(hostname);
		
		buffer = _RSStringGetOrCreateCString(allocator, hostname, buffer, &bufferLength, RSStringEncodingASCII);
		
		if (inet_pton(AF_INET, (const char*)buffer, &ip) == 1)
			is_ip = TRUE;
			
		if (buffer != stack_buffer)
			RSAllocatorDeallocate(allocator, buffer);
	}
	
    for (i = 0; result && (i < count); i++) {
		
        RSStringRef bypass = (RSStringRef)RSArrayObjectAtIndex(bypasses, i);
        
        // Explicitely listed hosts gets bypassed
        if (RSStringCompare(hostname, bypass, RSCompareCaseInsensitive) == RSCompareEqualTo) {
            result = FALSE;
        }
        
		else if (is_ip && RSStringFindWithOptions(bypass, _kProxySupportSlash, RSMakeRange(0, RSStringGetLength(bypass)), 0, FALSE)) {
		
            RSArrayRef pieces = RSStringCreateComponentsSeparatedByStrings(NULL, bypass, _kProxySupportSlash);
			
			if (pieces && (RSArrayGetCount(pieces) == 2)) {
				RSInteger cidr = RSStringIntegerValue((RSStringRef)RSArrayObjectAtIndex(pieces, 1));
				if ((cidr > 0) && (cidr < 33)) {
				
					RSArrayRef bypassnodes = RSStringCreateComponentsSeparatedByStrings(NULL,
																					(RSStringRef)RSArrayObjectAtIndex(pieces, 0),
																					_kProxySupportDot);
																					
					if (bypassnodes) {
					
						RSIndex bypassnc = RSArrayGetCount(bypassnodes);
						if (bypassnc <= 4) {
							
							RSIndex n;
							RSMutableStringRef cp = RSStringCreateMutableCopy(RSGetAllocator(hostname),
																			  0,
																			  (RSStringRef)RSArrayObjectAtIndex(bypassnodes, 0));
																			  
							for (n = 1; n < 4; n++) {
								
								if (n >= bypassnc)
									RSStringAppendString(cp, _kProxySupportDotZero);
									
								else {
									
									RSStringRef piece = (RSStringRef)RSArrayObjectAtIndex(bypassnodes, n);
									
									if (RSStringGetLength(piece))
										RSStringAppendStringWithFormat(cp, _kProxySupportDotFormat, piece);
									else
										RSStringAppendString(cp, _kProxySupportDotZero);
								}
							}
							
							n = RSStringGetLength(cp);
							
							if (n <= 16) {
								UInt8 stack_buffer[32];
								UInt8* buffer = stack_buffer;
								RSIndex bufferLength = sizeof(stack_buffer);
								RSAllocatorRef allocator = RSGetAllocator(hostname);
								struct in_addr bypassip;
								
								buffer = _RSStringGetOrCreateCString(allocator, cp, buffer, &bufferLength, RSStringEncodingASCII);
								
								if ((inet_pton(AF_INET, (const char*)buffer, &bypassip) == 1) &&
									(((((1 << cidr) - 1) << (32 - cidr)) & ntohl(*((uint32_t*)(&ip)))) == ntohl(*((uint32_t*)(&bypassip)))))
								{
									result = FALSE;
								}
									
								if (buffer != stack_buffer)
									RSAllocatorDeallocate(allocator, buffer);
							}
							
							RSRelease(cp);
						}
					
						RSRelease(bypassnodes);
					}
				}
			}

			if (pieces)
				RSRelease(pieces);
		}
		
        else if (hostnodes) {
        
            RSIndex bypassnc;
            RSArrayRef bypassnodes = RSStringCreateComponentsSeparatedByStrings(NULL, bypass, _kProxySupportDot);
            
            if (!bypassnodes) continue;
            
            bypassnc = RSArrayGetCount(bypassnodes);
            if (bypassnc > 1) {
            
                RSIndex j = hostnc - 1;
                RSIndex k = bypassnc - 1;
                
                while ((j >= 0) && (k >= 0)) {
                
                    RSStringRef hostnode = (RSStringRef)RSArrayObjectAtIndex(hostnodes, j);
                    RSStringRef bypassnode = (RSStringRef)RSArrayObjectAtIndex(bypassnodes, k);
                    
					if ((k == 0) && (RSStringGetLength(bypassnode) == 0))
						bypassnode = _kProxySupportStar;
					
                    if (RSStringCompare(hostnode, bypassnode, RSCompareCaseInsensitive) == RSCompareEqualTo) {
                        if (!j && !k) {
                            result = FALSE;
                            break;
                        }
                        else {
                            j--, k--;
                        }
                    }
                    
                    else if (RSStringCompare(bypassnode, _kProxySupportStar, RSCompareCaseInsensitive) == RSCompareEqualTo) {
                        
                        while (k >= 0) {
                            bypassnode = (RSStringRef)RSArrayObjectAtIndex(bypassnodes, k);

							if ((k == 0) && (RSStringGetLength(bypassnode) == 0))
								bypassnode = _kProxySupportStar;

                            if (RSStringCompare(bypassnode, _kProxySupportStar, RSCompareCaseInsensitive) != RSCompareEqualTo)
                                break;
                            k--;
                        }
                        
                        if (k < 0) {
                            result = FALSE;
                            break;
                        }
                        
                        else {
                            
                            while (j >= 0) {
                                
                                hostnode = (RSStringRef)RSArrayObjectAtIndex(hostnodes, j);
                                if (RSStringCompare(bypassnode, hostnode, RSCompareCaseInsensitive) == RSCompareEqualTo)
                                    break;
                                j--;
                            }
                            
                            if (j < 0)
                                break;
                        }
                    }
                    
                    else
                        break;
                }
            }
            
            RSRelease(bypassnodes);
        }
    }	

    if (hostnodes)
    RSRelease(hostnodes);
	
    return result;
}

static RSURLRef _URLForProxyEntry(RSAllocatorRef alloc, RSStringRef entry, RSIndex startIndex, RSStringRef scheme) {

    RSCharacterSetRef whitespaceSet = RSCharacterSetGetPredefined(RSCharacterSetWhitespace);
    RSIndex len = RSStringGetLength(entry);
    RSRange colonRange;
    RSStringFind(entry, _kProxySupportColon, RSStringGetRange(entry), &colonRange);

    RSStringRef host;
    RSStringRef portString;
    BOOL hasPort = TRUE;
    
    RSMutableStringRef urlString;
    RSURLRef url;
    
    if (colonRange.location == RSNotFound) {
        colonRange.location = len;
        hasPort = FALSE;
    }
    while (startIndex < len && RSCharacterSetIsCharacterMember(whitespaceSet, RSStringGetCharacterAtIndex(entry, startIndex))) {
        startIndex ++;
    }
    if (startIndex >= colonRange.location) {
        return NULL;
    }
    host = RSStringCreateWithSubstring(RSGetAllocator(entry), entry, RSMakeRange(startIndex, colonRange.location - startIndex));

    if (hasPort) {
        portString = RSStringCreateWithSubstring(RSGetAllocator(entry), entry, RSMakeRange(colonRange.location + 1, len - colonRange.location - 1));
    } else if (RSStringCompare(scheme, _kProxySupportHTTPScheme, RSCompareCaseInsensitive) == RSCompareEqualTo || 
        RSStringCompare(scheme, _kProxySupportHTTPSScheme, RSCompareCaseInsensitive) == RSCompareEqualTo) {
        portString = _kProxySupportHTTPPort;
        RSRetain(portString);
    } else if (RSStringCompare(scheme, _kProxySupportFTPScheme, RSCompareCaseInsensitive) == RSCompareEqualTo) {
        portString = _kProxySupportFTPPort;
        RSRetain(portString);
    } else if (RSStringCompare(scheme, _kProxySupportSOCKS4Scheme, RSCompareCaseInsensitive) == RSCompareEqualTo || RSStringCompare(scheme, _kProxySupportSOCKS5Scheme, RSCompareCaseInsensitive) == RSCompareEqualTo) {
        portString = _kProxySupportSOCKSPort;
        RSRetain(portString);
    } else {
        RSRelease(host);
        return NULL;
    }
    
    urlString = RSStringCreateMutable(alloc, RSStringGetLength(scheme) + RSStringGetLength(host) + RSStringGetLength(portString) + 4);
    RSStringAppendString(urlString, scheme);
    RSStringAppendCString(urlString, "://", RSStringEncodingASCII);
    RSStringAppendString(urlString, host);
    RSRelease(host);
    RSStringAppendCString(urlString, ":", RSStringEncodingASCII);
    RSStringAppendString(urlString, portString);
    RSRelease(portString);
    url = RSURLCreateWithString(NULL, urlString, NULL);
    RSRelease(urlString);
    return url;
}


static void _appendProxiesFromPACResponse(RSAllocatorRef alloc, RSMutableArrayRef proxyList, RSStringRef pacResponse, RSStringRef scheme) {
    RSArrayRef entries;
    RSIndex i, c;

    if (!pacResponse) return;

    entries = RSStringCreateComponentsSeparatedByStrings(alloc, pacResponse, _kProxySupportSemiColon);
    c = RSArrayGetCount(entries);
    BOOL isFTP = (RSStringCompare(scheme, _kProxySupportFTPScheme, RSCompareCaseInsensitive) == RSCompareEqualTo);
        
    for (i = 0; i < c; i++) {
            
        RSURLRef to_add = NULL;
        RSStringRef untrimmedEntry = (RSStringRef)RSArrayObjectAtIndex(entries, i);
        RSMutableStringRef entry = untrimmedEntry ? RSStringCreateMutableCopy(alloc, RSStringGetLength(untrimmedEntry), untrimmedEntry) : NULL;
        RSIndex entryLength;
        RSRange range;
        
        if (!entry)  continue;
        RSStringTrimWhitespace(entry);
        entryLength = RSStringGetLength(entry);
        if (entryLength >= 6 && RSStringFindWithOptions(entry, _kProxySupportDIRECT, RSMakeRange(0, 6), RSCompareCaseInsensitive, NULL)) {
            RSArrayAddObject(proxyList, RSNull);
            // NOTE that "to_add" is not changed and the array is altered directly.
        }

        else if (entryLength >= 5 && RSStringFindWithOptions(entry, _kProxySupportPROXY, RSMakeRange(0, 5), RSCompareCaseInsensitive, &range)) {
            RSIndex urlStart = 5;
            to_add = _URLForProxyEntry(RSGetAllocator(proxyList), entry, urlStart, scheme);
            
            // In the case of FTP, dump an extra entry to try FTP over HTTP.
            if (to_add && isFTP) {
                RSURLRef extra = _URLForProxyEntry(RSGetAllocator(proxyList), entry, urlStart, _kProxySupportHTTPScheme);
                if (extra) {
                    RSArrayAddObject(proxyList, extra);
                    RSRelease(extra);
                }
            }
        }
        
        else if (entryLength >= 5 && RSStringFindWithOptions(entry, _kProxySupportSOCKS, RSMakeRange(0, 5), RSCompareCaseInsensitive, &range)) {
            to_add = _URLForProxyEntry(RSGetAllocator(proxyList), entry, 5, _kProxySupportSOCKS5Scheme);
        }
        
        if (to_add) {
            RSArrayAddObject(proxyList, to_add);
            RSRelease(to_add);
        }
        RSRelease(entry);
    }
    RSRelease(entries);
}

static RSURLRef proxyURLForComponents(RSAllocatorRef alloc, RSStringRef scheme, RSStringRef host, SInt32 port, RSStringRef user, RSStringRef password) {
    RSStringRef urlString;
    RSURLRef url;
    
    if (user) {
        urlString = RSStringCreateWithFormat(alloc, _kProxySupportURLLongFormat, scheme, user, password, host, port);
    } else {
        urlString = RSStringCreateWithFormat(alloc, _kProxySupportURLShortFormat, scheme, host, port);
    }
    url = RSURLCreateWithString(alloc, urlString, NULL);
    RSRelease(urlString);
    return url;
}


/* Synchronous if callback == NULL. */
/* extern */ RSMutableArrayRef
_RSNetworkFindProxyForURLAsync(RSStringRef scheme, RSURLRef url, RSStringRef host, RSDictionaryRef proxies, _RSProxyStreamCallBack callback, void *clientInfo, RSReadStreamRef *proxyStream) {

    // Priority of proxies in typical usage scenarios
    //		1.  pac file
    //		2.  protocol specific proxy
    //		3.  SOCKS
    //		4.  direct

    RSAllocatorRef alloc = url ? RSGetAllocator(url) : host ? RSGetAllocator(host) : proxies ? RSGetAllocator(proxies) : RSAllocatorDefault;
    RSMutableArrayRef result = RSArrayCreateMutable(alloc, 0);
	
    if (!proxies) {
        RSArrayAddObject(result, RSNull);
        return result;
    }
    
    if (host) {
        RSRetain(host);
    } else {
        if (!url) {
            RSArrayAddObject(result, RSNull);
            return result;
        }
        host = RSURLCopyHostName(url);
		if (!host) {
			RSArrayAddObject(result, RSNull);
			return result;
		}
    }

    if (!scheme)
        scheme = url ? RSURLCopyScheme(url) : NULL;
    else
        RSRetain(scheme);

    do {
        /*
        ** Note that once a proxy or list of proxies is deteremined for the given
        ** url/host, the code will dump out of the do...while loop.  This allows fall
        ** through in proxy setup attempts according to the previously mentioned priorities.
        */
        RSStringRef proxy;
        RSNumberRef port;
        // On Windows we don't look for *Enabled* keys, so this just stays NULL
        RSTypeRef enabled = NULL;
        RSArrayRef bypass = (RSArrayRef)RSDictionaryGetValue(proxies, _kProxySupportExceptionsList);
        RSNumberRef localBypass = _proxyEnabled(RSDictionaryGetValue(proxies, RSStreamPropertyProxyLocalBypass)) ? RSBooleanTrue : RSBooleanFalse;
        
        if (bypass && RSGetTypeID(bypass) != RSArrayGetTypeID()) {
            bypass = NULL;
        }

        if (scheme) {

            SInt32 default_port = 0;
            RSStringRef proxy_key = NULL, port_key = NULL;
#if defined(__MACH__)
            RSStringRef enable_key = NULL;
#endif
            
#if defined(__MACH__)
            enabled = RSDictionaryGetValue(proxies, _RSStreamPropertyHTTPProxyProxyAutoConfigEnable);
#endif
            if (_proxyEnabled(enabled)) {

                proxy = (RSStringRef)RSDictionaryGetValue(proxies, _RSStreamPropertyHTTPProxyProxyAutoConfigURLString);

                // If PAC file exists, attempt it.
                if (proxy && RSGetTypeID(proxy) == RSStringGetTypeID()) {

                    RSStringRef url_str = RSURLCreateStringByAddingPercentEscapes(alloc, proxy, NULL, NULL, RSStringEncodingUTF8);
                    RSURLRef pac = RSURLCreateWithString(alloc, url_str, NULL);
                    RSRelease(url_str);

                    if (pac) {
                        BOOL mustLoad = FALSE;
                        RSStringRef list;
                        if (callback) {
                            // Asynchronous
                            list = _JSFindProxyForURLAsync(pac, url, host, &mustLoad);
                        } else {
                             list = _JSFindProxyForURL(pac, url, host);
                             mustLoad = FALSE;
                        }

                        if (mustLoad) {
                            *proxyStream = BuildStreamForPACURL(alloc, pac, url, scheme, host, callback, clientInfo);
                            RSRelease(result);
                            result = NULL; // NULL means async load in progress
                        } else {
                            _appendProxiesFromPACResponse(alloc, result, list, scheme);
                            if (list) RSRelease(list);
                        }
                        RSRelease(pac);
                    }

                    break;	// Proxy list should be set.  If there were failures, don't continue to
                }		// progress because the PAC file was to be consulted and used.
            }

            // No PAC file so attempt the scheme specific proxy.
            if (RSStringCompare(scheme, _kProxySupportHTTPScheme, RSCompareCaseInsensitive) == RSCompareEqualTo) {
                proxy_key = RSStreamPropertyHTTPProxyHost;
                port_key = RSStreamPropertyHTTPProxyPort;
#if defined(__MACH__)
                enable_key = kSCPropNetProxiesHTTPEnable;
#endif
                default_port = 80;
            }
            else if (RSStringCompare(scheme, _kProxySupportHTTPSScheme, RSCompareCaseInsensitive) == RSCompareEqualTo) {
                proxy_key = RSStreamPropertyHTTPSProxyHost;
                port_key = RSStreamPropertyHTTPSProxyPort;
#if defined(__MACH__)
                enable_key = kSCPropNetProxiesHTTPSEnable;
#endif
                default_port = 80;
            }
            else if (RSStringCompare(scheme, _kProxySupportFTPScheme, RSCompareCaseInsensitive) == RSCompareEqualTo) {
                proxy_key = RSStreamPropertyFTPProxyHost;
                port_key = RSStreamPropertyFTPProxyPort;
#if defined(__MACH__)
                enable_key = kSCPropNetProxiesFTPEnable;
#endif
                default_port = 21;
            }
            else if (RSStringCompare(scheme, _kProxySupportFTPSScheme, RSCompareCaseInsensitive) == RSCompareEqualTo) {
                proxy_key = RSStreamPropertyFTPProxyHost;
                port_key = RSStreamPropertyFTPProxyPort;
#if defined(__MACH__)
                enable_key = kSCPropNetProxiesFTPEnable;
#endif
                default_port = 990;
            }
            
            if (proxy_key) {
#if defined(__MACH__)
                enabled = RSDictionaryGetValue(proxies, enable_key);
#endif
                if (_proxyEnabled(enabled)) {

                    proxy = (RSStringRef)RSDictionaryGetValue(proxies, proxy_key);
                    if (proxy && RSGetTypeID(proxy) == RSStringGetTypeID() && _RSNetworkDoesNeedProxy(host, bypass, localBypass)) {
                        RSURLRef to_add;
                        SInt32 portNum;

                        port = (RSNumberRef)RSDictionaryGetValue(proxies, port_key);
                        if (!port || RSGetTypeID(port) != RSNumberGetTypeID() || !RSNumberGetValue(port, &portNum)) {
                            portNum = default_port;
                        }

                        to_add = proxyURLForComponents(alloc, scheme, proxy, portNum, NULL, NULL);
                        if (proxy_key == RSStreamPropertyFTPProxyHost) {
                            RSURLRef extra = proxyURLForComponents(alloc, _kProxySupportHTTPScheme, proxy, portNum == 21 ? 80 : portNum, NULL, NULL);
                            if (extra) {
                                RSArrayAddObject(result, extra);
                                RSRelease(extra);
                            }
                        }
                        if (to_add) {
                            RSArrayAddObject(result, to_add);
                            RSRelease(to_add);
                        }

                        break;	// Proxy list is set.  If the URL creation failed, don't continue on because
                    }		// nothing else should not be attempted.  It's better to return an empty list.
                }
            }
        }

#if defined(__MACH__)
        enabled = RSDictionaryGetValue(proxies, kSCPropNetProxiesSOCKSEnable);
#endif
        if (_proxyEnabled(enabled)) {

            RSStringRef user = (RSStringRef)RSDictionaryGetValue(proxies, RSStreamPropertySOCKSUser);
            RSStringRef pass = (RSStringRef)RSDictionaryGetValue(proxies, RSStreamPropertySOCKSPassword);
            RSStringRef version = (RSStringRef)RSDictionaryGetValue(proxies, RSStreamPropertySOCKSVersion);
            
            proxy = (RSStringRef)RSDictionaryGetValue(proxies, RSStreamPropertySOCKSProxyHost);

            if (proxy && RSGetTypeID(proxy) == RSStringGetTypeID() && (!bypass || !host || _RSNetworkDoesNeedProxy(host, bypass, localBypass))) {
                RSStringRef socksScheme;
                SInt32 portNum;
                RSURLRef to_add;
				
                port = (RSNumberRef)RSDictionaryGetValue(proxies, RSStreamPropertySOCKSProxyPort);
                if (!port || RSGetTypeID(port) != RSNumberGetTypeID() || !RSNumberGetValue(port, &portNum)) {
                    portNum = 1080;
                }
                if (!user || !pass || RSGetTypeID(user) != RSStringGetTypeID() || RSGetTypeID(pass) != RSStringGetTypeID()) {
                    user = NULL;
                    pass = NULL;
                }
                socksScheme = (version && RSEqual(version, RSStreamSocketSOCKSVersion4)) ? _kProxySupportSOCKS4Scheme : _kProxySupportSOCKS5Scheme;
                to_add = proxyURLForComponents(alloc, socksScheme, proxy, portNum, user, pass);
                if (to_add) {
                    RSArrayAddObject(result, to_add);
                    RSRelease(to_add);
                }

                break;	// Proxy list is set.  If the URL creation failed, don't put DIRECT in the list
            }		// because direct should not be attempted.  It's better to return an empty list.
        }

        // Gotten this far, so only direct is left.
        RSArrayAddObject(result, RSNull);

    } while (0);

    if (scheme) RSRelease(scheme);
    if (host) RSRelease(host);
    return result;
}

static void
_ReadStreamClientCallBack(RSReadStreamRef stream, RSStreamEventType type, RSMutableDataRef contents) {

    if (type == RSStreamEventHasBytesAvailable) {
        UInt8 buffer[8192];
        RSIndex bytesRead = RSReadStreamRead(stream, buffer, sizeof(buffer));
        if (bytesRead > 0)
            RSDataAppendBytes(contents, buffer, bytesRead);
    }
}


static void
_RunLoopTimerCallBack(RSRunLoopTimerRef timer, BOOL* timedout) {

    *timedout = TRUE;
}


static RSStreamError
_LoadStreamIntoData(RSReadStreamRef stream, RSMutableDataRef contents, RSTimeInterval timeout, BOOL isFile) {

    RSStreamError result = {0, 0};
    RSRunLoopRef rl = RSRunLoopGetCurrent();
    RSStreamClientContext ctxt = {0, contents, NULL, NULL, NULL};

    RSReadStreamSetClient(stream,
                          RSStreamEventHasBytesAvailable | RSStreamEventErrorOccurred,
                          (RSReadStreamClientCallBack)_ReadStreamClientCallBack,
                          &ctxt);
    RSReadStreamScheduleWithRunLoop(stream, rl, _kProxySupportLoadingPacPrivateMode);

    if (!RSReadStreamOpen(stream))
        result = RSReadStreamGetError(stream);
    else {
        BOOL timedout = FALSE;
        RSRunLoopTimerContext timerCtxt = {0, &timedout, NULL, NULL, NULL};
        RSRunLoopTimerRef t = timeout ? RSRunLoopTimerCreate(RSGetAllocator(stream),
                                                             RSAbsoluteTimeGetCurrent() + timeout,
                                                             0,
                                                             0,
                                                             0,
                                                             (RSRunLoopTimerCallBack)_RunLoopTimerCallBack,
                                                             &timerCtxt) : NULL;

        if (t)
            RSRunLoopAddTimer(rl, t, _kProxySupportLoadingPacPrivateMode);

        do {
            RSStreamStatus status = RSReadStreamGetStatus(stream);
            if (status == RSStreamStatusAtEnd || status == RSStreamStatusError)
                break;
            
            RSRunLoopRunInMode(_kProxySupportLoadingPacPrivateMode, 1e+20, TRUE);
            
        } while (!timedout);
        
        if (t) {
            RSRunLoopRemoveTimer(rl, t, _kProxySupportLoadingPacPrivateMode);
            RSRelease(t);
        }

        result = RSReadStreamGetError(stream);
        if (result.error || timedout) {
            RSIndex len = RSDataGetLength(contents);
            if (len)
                RSDataDeleteBytes(contents, RSMakeRange(0, len));
        }

        if (timedout) {
            result.domain = RSStreamErrorDomainCustom;
            result.error = -1;
        }
        
        RSReadStreamClose(stream);
    }

    RSReadStreamUnscheduleFromRunLoop(stream, rl, _kProxySupportLoadingPacPrivateMode);
    RSReadStreamSetClient(stream, 0, NULL, NULL);

    return result;
}


/* static */ RSStringRef
_loadJSSupportFile(void) {
    
    static RSURLRef _JSRuntimeFunctionsLocation = NULL;
    static RSStringRef _JSRuntimeFunctions = NULL;


    if (_JSRuntimeFunctionsLocation == NULL) {
#if !defined(__WIN32__)
        RSBundleRef cfNetworkBundle = RSBundleGetWithIdentifier(_kProxySupportRSNetworkBundleID);
        _JSRuntimeFunctionsLocation = RSURLWithFilePath(RSAutorelease(RSBundleCreatePathForResource(cfNetworkBundle, _kProxySupportPacSupportFileName, _kProxySupportJSExtension)), false);
#else
        static UInt8 path[MAX_PATH+1];
        const char *resourcePath = _RSDLLPath();
        strcpy(path, resourcePath);
        strcat(path, "\\PACSupport.js");
        _JSRuntimeFunctionsLocation = RSURLCreateFromFileSystemRepresentation(NULL, path, strlen(path), FALSE);
#endif

        RSReadStreamRef stream = RSReadStreamCreateWithFile(NULL, _JSRuntimeFunctionsLocation);
        if (stream) {
        
            RSMutableDataRef contents = RSDataCreateMutable(NULL, 0);
        
            // NOTE that the result value is not taken here since this is the load
            // of the local support file.  If that fails, DIRECT should not be
            // the fallback as will happen with the actual PAC file.
            _LoadStreamIntoData(stream, contents, PAC_STREAM_LOAD_TIMEOUT, TRUE);
            RSRelease(stream);
            
            RSIndex bytesRead = RSDataGetLength(contents);
            // Check to see if read until the end of the file.
            if (bytesRead) {
                _JSRuntimeFunctions = RSStringCreateWithBytes(NULL, RSDataGetBytesPtr(contents), bytesRead, RSStringEncodingUTF8, TRUE);
            }
            RSRelease(contents);
        }
    }
    return _JSRuntimeFunctions;
}


static RSReadStreamRef _streamForPARSile(RSAllocatorRef alloc, RSURLRef pac, BOOL *isFile) {
    RSStringRef scheme = RSURLCopyScheme(pac);
    RSReadStreamRef stream = NULL;
    *isFile = FALSE;

    if (!scheme || (RSStringCompare(scheme, _kProxySupportFILEScheme, RSCompareCaseInsensitive) == RSCompareEqualTo)) {
        stream = RSReadStreamCreateWithFile(alloc, pac);
        *isFile = TRUE;
    }
    
    else if ((RSStringCompare(scheme, _kProxySupportHTTPScheme, RSCompareCaseInsensitive) == RSCompareEqualTo) ||
             (RSStringCompare(scheme, _kProxySupportHTTPSScheme, RSCompareCaseInsensitive) == RSCompareEqualTo))
    {
        RSHTTPMessageRef msg = RSHTTPMessageCreateRequest(alloc, _kProxySupportGETMethod, pac, RSHTTPVersion1_0);
        if (msg) {
            stream = RSReadStreamCreateForHTTPRequest(alloc, msg);
			if (stream)
				RSReadStreamSetProperty(stream, RSStreamPropertyHTTPShouldAutoredirect, RSBooleanTrue);
            RSRelease(msg);
        }
    }
    
    else if ((RSStringCompare(scheme, _kProxySupportFTPScheme, RSCompareCaseInsensitive) == RSCompareEqualTo) ||
             (RSStringCompare(scheme, _kProxySupportFTPSScheme, RSCompareCaseInsensitive) == RSCompareEqualTo))
    {
        stream = RSReadStreamCreateWithFTPURL(alloc, pac);
    }
    return stream;
}

RSStringRef _stringFromLoadedPACStream(RSAllocatorRef alloc, RSMutableDataRef contents, RSReadStreamRef stream, RSAbsoluteTime *expires) {
    RSHTTPMessageRef msg = (RSHTTPMessageRef)RSReadStreamCopyProperty(stream, RSStreamPropertyHTTPResponseHeader);
    RSStringRef result = NULL;
    RSAbsoluteTime now = RSAbsoluteTimeGetCurrent();
    *expires = now > 1 ? now - 1 : now; // Just some number that's less than now.

    if (msg) {
		
        UInt32 code = RSHTTPMessageGetResponseStatusCode(msg);
        
        if (code > 299) {
            RSDataSetLength(contents, 0);
        }
        
        else {
            
            RSStringRef expiryString = RSHTTPMessageCopyHeaderFieldValue(msg, _kProxySupportExpiresHeader);
            
            if (expiryString) {
                RSGregorianDate expiryDate;
                RSTimeZoneRef expiryTZ = NULL;
                
                if (_RSGregorianDateCreateWithString(alloc, expiryString, &expiryDate, &expiryTZ)) {
                    RSStringRef nowString = RSHTTPMessageCopyHeaderFieldValue(msg, _kProxySupportNowHeader);
		    if (nowString) {
                        RSTimeZoneRef nowTZ;
                        RSGregorianDate nowDate;
                        if (_RSGregorianDateCreateWithString(alloc, nowString, &nowDate, &nowTZ)) {
                            *expires = now + (RSGregorianDateGetAbsoluteTime(expiryDate, expiryTZ) - RSGregorianDateGetAbsoluteTime(nowDate, nowTZ));
                        } else {
                            *expires = RSGregorianDateGetAbsoluteTime(expiryDate, expiryTZ);
                        }
                        RSRelease(nowString);
                    } else {
                        *expires = RSGregorianDateGetAbsoluteTime(expiryDate, expiryTZ);
                    }
                }

                RSRelease(expiryString);
            }
        }
        
        RSRelease(msg);
    }
    
    if (*expires < now) {
        *expires = now + (24 * 60 * 60);
    }

    RSIndex bytesRead = RSDataGetLength(contents);
    if (bytesRead) {
       // try using UTF8 encoding
        result = RSStringCreateWithBytes(alloc, RSDataGetBytesPtr(contents), bytesRead, RSStringEncodingUTF8, TRUE);
       if ( result == NULL ) {
           // fall back and try ISOLatin1 encoding
           result = RSStringCreateWithBytes(alloc, RSDataGetBytesPtr(contents), bytesRead, RSStringEncodingISOLatin1, TRUE);
           if ( result == NULL ) {
               // fall back and try raw bytes
               result = RSStringCreateWithBytes(alloc, RSDataGetBytesPtr(contents), bytesRead, RSStringEncodingMacRoman, TRUE);
               if ( result == NULL ) {
                   // Should never get here
                   RSLog(0, RSSTR("PAC stream bytes could not be converted to RSString\n"));
               }
           }
       }
    }
    return result;
}

/* static */ RSStringRef
_loadPARSile(RSAllocatorRef alloc, RSURLRef pac, RSAbsoluteTime *expires, RSStreamError *err) {
    BOOL isFile;
    RSReadStreamRef stream = _streamForPARSile(alloc, pac, &isFile);
    RSStringRef result = NULL;

    // Load pac file at url location.
    if (stream) {
        RSMutableDataRef contents = RSDataCreateMutable(alloc, 0);
        *err = _LoadStreamIntoData(stream, contents, PAC_STREAM_LOAD_TIMEOUT, isFile);
        
        if (err->domain == 0) {
            result = _stringFromLoadedPACStream(alloc, contents, stream, expires);
        }
        RSRelease(contents);
        RSRelease(stream);
    }
    return result;
}


#if !defined(__WIN32__)

/* static */ JSContextRef
_createJSRuntime(RSAllocatorRef alloc, RSStringRef js_support, RSStringRef js_pac) {

    RSStringRef allCode = RSStringCreateWithFormat(alloc, RSSTR("%@\n%@\n"), js_support, js_pac);
    JSContextRef runtime = NULL;
    if (allCode) {
        runtime = JSContextGroupCreate();
    }
    
    if (!JSRunCheckSyntax(runtime)) {
        JSRelease(runtime);
        runtime = NULL;
    }
    
    if (runtime) {
        
        JSObjectRef g = JSRunCopyGlobalObject(runtime);
        JSObjectCallBacks c = {NULL, NULL, NULL, NULL,  NULL, NULL, NULL};
        
        if (g) {
            
            JSObjectRef func;
            
            c.callFunction = (JSObjectCallFunctionProcPtr)_JSDnsResolveFunction;
            JSLockInterpreter();
            func = JSObjectCreate(NULL, &c);
            
            if (func) {
                JSObjectSetProperty(g, RSSTR("__dnsResolve"), func);
                JSRelease(func);
            }
            
            c.callFunction = (JSObjectCallFunctionProcPtr)_JSPrimaryIpv4AddressesFunction;
            func = JSObjectCreate(NULL, &c);
            
            if (func) {
                JSObjectSetProperty(g, RSSTR("__primaryIPv4Addresses"), func);
                JSRelease(func);
            }
            
            JSObjectRef strObj = JSObjectCreateWithRSType(RSSTR("MACH"));
            JSObjectSetProperty(g, RSSTR("__platformName"), strObj);
            JSRelease(strObj);
            
            JSUnlockInterpreter();
            
            JSRelease(g);
        }
        
        g = JSRunEvaluate(runtime);
        if (g) JSRelease(g);
    }
    return runtime;
}


/* static */ void
_freeJSRuntime(JSContextRef runtime) {
    JSRelease(runtime);
}


/* static */ RSStringRef
_callPARSunction(RSAllocatorRef alloc, JSContextRef runtime, RSURLRef url, RSStringRef host) {

    JSObjectRef jsResult = NULL;
    RSStringRef result = NULL;
    JSObjectRef g = runtime ? JSRunCopyGlobalObject(runtime) : NULL;
    JSObjectRef func = g ? JSObjectCopyProperty(g, RSSTR("__Apple_FindProxyForURL")) : NULL;
    
    if (func) {
        RSArrayRef cfArgs = NULL;
        RSMutableArrayRef jsArgs;
        RSTypeRef args[2];
        RSURLRef absURL = RSURLCopyAbsoluteURL(url);
        
        args[0] = RSURLGetString(absURL);
        args[1] = host;
        
        cfArgs = RSArrayCreateWithObjects(alloc, args, 2);
        RSRelease(absURL);
        
        JSLockInterpreter();
        jsArgs = JSCreateJSArrayFromRSArray(cfArgs);
        RSRelease(cfArgs);
        
        jsResult = JSObjectCallFunction(func, g, jsArgs);
        JSUnlockInterpreter();
        JSRelease(func);
        RSRelease(jsArgs);
        
        if (jsResult) {
            result = (RSStringRef)JSObjectCopyRSValue(jsResult);
            JSRelease(jsResult);
            if (result && RSGetTypeID(result) != RSStringGetTypeID()) {
				RSRelease(result);
				result = NULL;
            }
        }
    }
    
    if (g) JSRelease(g);

    //RSLog(0, RSSTR("FindProxyForURL returned: %@"), result);
    return result;
}


#else

/*
 Doc on scripting interfaces:
     http://msdn.microsoft.com/library/en-us/script56/html/scripting.asp
 Doc on IDispatch interface:
     http://msdn.microsoft.com/library/en-us/automat/htm/chap5_78v9.asp
 Doc on passing arrays though IDispatch:
     http://msdn.microsoft.com/library/en-us/automat/htm/chap7_5dyr.asp
 
 COM code is cobbled together from the following samples:
     http://www.microsoft.com/msj/1099/visualprog/visualprog1099.aspx
     http://www.codeproject.com/com/mfcscripthost.asp
     http://www.microsoft.com/mind/0297/activescripting.asp
     MSDN KB 183698: http://support.microsoft.com:80/support/kb/articles/q183/6/98.asp
 
 Info on doing COM in C vs C++
     http://msdn.microsoft.com/library/en-us/dncomg/html/msdn_com_co.asp
 
 COM memory management rules, for items pass by reference and interfaces
     http://msdn.microsoft.com/library/en-us/com/htm/com_3rub.asp
     http://msdn.microsoft.com/library/en-us/com/htm/com_1vxv.asp
 
 Big scripting FAQ, including how to reuse a script engine with cloning - we don't do this yet
 but maybe it would be an interesting optimization.
     http://www.mindspring.com/~mark_baker/
 
 I found mixed info as to how compatible GCC's vtables are with COM.  I didn't try a C++ implementation
 yet, but there is some chance that it work work with GCC.
*/

/*
 Originally on Windows we tried doing PAC using the WinHTTP library, but that has problems:
 - Does not support ftp target URLs
 - PAC files returning "SOCKS foo" generate a result of DIRECT
 - No way to intersperse a DIRECT result with explicit server results, so no control over failover.

 The impl code is in CVS in revision 1.13.4.3

 WinHTTP PAC main link:
 http://msdn.microsoft.com/library/en-us/winhttp/http/winhttp_autoproxy_support.asp

 WinInet Pac main link:
 http://msdn.microsoft.com/library/en-us/wininet/wininet/autoproxy_support_in_wininet.asp

 Note the WinInet stuff says their support may go away, and recommends using WinHTTP.  WinHTTP only
 has PAC in version 5.1, shipped with Win2K+SP3 and WinXP+SP1.  It is NOT redistributable for running
 on the earlier SP's.
 */

// WinHttpGetIEProxyConfigForCurrentUser could also be used to get the user settings from IE.
// WinHttpGetDefaultProxyConfiguration could be used to get settings from the registry. (non-IE)


#define COBJMACROS
// Note: this one isn't in the Cygwin set, had to copy this file from MSVC.
#include <ACTIVSCP.H>

//#define DO_COM_LOGGING
#ifdef DO_COM_LOGGING
#define COM_LOG printf
#else
#define COM_LOG while(0) printf
#endif

// All the state for a particular instance of the JavaScript engine
struct _JSRunGuts {
    // interfaces in the engine we call
    IActiveScript* pActiveScript;
    IActiveScriptParse* pActiveScriptParse;
};

static HRESULT _parseCodeChunk(RSStringRef code, JSContextRef runtime);
static void _prepareStringParam(RSStringRef str, VARIANTARG *variant);
static void _prepareStringArrayReturnValue(RSArrayRef cfArray, VARIANT *pVarResult);
static RSArrayRef _JSPrimaryIpv4AddressesFunction(void);

#define CHECK_HRESULT(hr, func) if (!SUCCEEDED(hr)) { \
    RSLog(0, RSSTR("%s failed: 0x%X"), func, hResult); \
        break; \
} else COM_LOG("==== Call to %s succeeded ====\n", func);

// Name we use to add methods to the global namespace.  Basically a cookie used passed to
// IActiveScript::AddNamedItem and later received by IActiveScriptSite::GetItemInfo.
#define ITEM_NAME_ADDED_TO_JS OLESTR("_RSNetwork_Global_Routines_")


//
// Implemetation of IDispatch interface, vTable and other COM glue.
// The purpose of this class is to allow a couple C routines to be called from JS.
//

// Instance vars for our dispatcher
typedef struct {
    IDispatch iDispatch;    // mostly just the vtable
    UInt32 refCount;
} Dispatcher;

// Convert an IDispatch interface to one of our instance pointers
static inline Dispatcher *
thisFromIDispatch(IDispatch *disp) {
    return (Dispatcher*)((char *)disp - offsetof(Dispatcher, iDispatch));
}

// numerical DISPID's, which are cookies that map to our functions that we can dispatch
enum {
    DNSResolveDISPID = 22,              // the DNSResolve function
    PrimaryIPv4AddressesDISPID = 33,    // the PrimaryIPv4Addresses function
    PlatformNameDISPID = 44             // a global string property holding the platform name
};

static HRESULT STDMETHODCALLTYPE DispatchQueryInterface(IDispatch *disp, REFIID riid, void **ppv);
static ULONG STDMETHODCALLTYPE DispatchAddRef(IDispatch *disp);
static ULONG STDMETHODCALLTYPE DispatchRelease(IDispatch *disp);
static HRESULT STDMETHODCALLTYPE DispatchGetTypeInfoCount(IDispatch *disp, unsigned int *pctinfo);
static HRESULT STDMETHODCALLTYPE DispatchGetTypeInfo(IDispatch *disp, unsigned int iTInfo, LCID lcid, ITypeInfo **ppTInfo);
static HRESULT STDMETHODCALLTYPE DispatchGetIDsOfNames(IDispatch *disp, REFIID riid, OLECHAR ** rgszNames, unsigned intcNames, LCID lcid, DISPID *rgDispId);
static HRESULT STDMETHODCALLTYPE DispatchInvoke(IDispatch *disp, DISPID dispIdMember, REFIID riid, LCID lcid, WORD wFlags, DISPPARAMS *pDispParams, VARIANT *pVarResult, EXCEPINFO *pExcepInfo, unsigned int *puArgErr);

static IDispatchVtbl DispatchVTable = {
    DispatchQueryInterface,
    DispatchAddRef,
    DispatchRelease,
    DispatchGetTypeInfoCount,
    DispatchGetTypeInfo,
    DispatchGetIDsOfNames,
    DispatchInvoke
};

static HRESULT STDMETHODCALLTYPE
DispatchQueryInterface(IDispatch *disp, REFIID riid, void **ppv) {

#ifdef DO_COM_LOGGING
    LPOLESTR interfaceString;
    StringFromIID(riid, &interfaceString);
    COM_LOG("DispatchQueryInterface: %ls\n", interfaceString);
    CoTaskMemFree(interfaceString);
#endif

    if (IsEqualIID(riid, &IID_IUnknown) || IsEqualIID (riid, &IID_IDispatch)) {
        *ppv = disp;
        DispatchAddRef(disp);
        return NOERROR;
    }
    else {
        *ppv = 0;
        return E_NOINTERFACE;
    }
}

static ULONG STDMETHODCALLTYPE
DispatchAddRef(IDispatch *disp) {

    COM_LOG("DispatchAddRef\n");
    Dispatcher *this = thisFromIDispatch(disp);
    return ++this->refCount;
}

static ULONG STDMETHODCALLTYPE
DispatchRelease(IDispatch *disp) {

    COM_LOG("DispatchRelease\n");
    Dispatcher *this = thisFromIDispatch(disp);
    if (--this->refCount == 0) {
        RSAllocatorDeallocate(NULL, this);
        return 0;
    }
    return this->refCount;
}

static HRESULT STDMETHODCALLTYPE
DispatchGetTypeInfoCount(IDispatch *disp, unsigned int *pctinfo) {

    COM_LOG("DispatchGetTypeInfoCount\n");
    if (pctinfo == NULL) {
        return E_INVALIDARG;
    }
    else {
        *pctinfo = 0;
        return NOERROR;
    }
}

static HRESULT STDMETHODCALLTYPE
DispatchGetTypeInfo(IDispatch *disp, unsigned int iTInfo, LCID lcid, ITypeInfo **ppTInfo) {

    COM_LOG("DispatchGetTypeInfo\n");
    assert(FALSE);      // should never be called, since we return 0 from GetTypeInfoCount
    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE
DispatchGetIDsOfNames(IDispatch *disp, REFIID riid, OLECHAR **rgszNames, unsigned intcNames, LCID lcid, DISPID *rgDispId) {

    HRESULT retVal = S_OK;
    int i;
    for (i = 0; i < intcNames; i++) {
        if (wcscmp(OLESTR("__dnsResolve"), rgszNames[i]) == 0) {
            COM_LOG("DispatchGetIDsOfNames - resolved DNSResolveDISPID\n");
            rgDispId[i] = DNSResolveDISPID;
        }
        else if (wcscmp(OLESTR("__primaryIPv4Addresses"), rgszNames[i]) == 0) {
            COM_LOG("DispatchGetIDsOfNames - resolved PrimaryIPv4AddressesDISPID\n");
            rgDispId[i] = PrimaryIPv4AddressesDISPID;
        }
        else if (wcscmp(OLESTR("__platformName"), rgszNames[i]) == 0) {
            COM_LOG("DispatchGetIDsOfNames - resolved PlatformNameDISPID\n");
            rgDispId[i] = PlatformNameDISPID;
        }
        else {
            COM_LOG("DispatchGetIDsOfNames - unknown member %ls\n", rgszNames[i]);
            rgDispId[i] = DISPID_UNKNOWN;
            retVal = DISP_E_UNKNOWNNAME;
        }
    }
    return retVal;
}

static HRESULT STDMETHODCALLTYPE
DispatchInvoke(IDispatch *disp, DISPID dispIdMember, REFIID riid, LCID lcid, WORD wFlags, DISPPARAMS *pDispParams, VARIANT *pVarResult, EXCEPINFO *pExcepInfo, unsigned int *puArgErr)
{

    if (dispIdMember == DNSResolveDISPID) {
        COM_LOG("DispatchInvoke - DNSResolveDISPID, wFlags=%d\n", wFlags);
        assert(wFlags & DISPATCH_METHOD);
        if (pDispParams->cArgs != 1)
            return DISP_E_BADPARAMCOUNT;
        if (pDispParams->cNamedArgs != 0)
            return DISP_E_NONAMEDARGS;
        if (pDispParams->rgvarg[0].vt != VT_BSTR) {
            puArgErr = 0;
            return DISP_E_TYPEMISMATCH;
        }
        
        // Convert COM arg from BSTR to RSString, call our C code
        BSTR bstrNameParam = pDispParams->rgvarg[0].bstrVal;
        COM_LOG("DispatchInvoke - DNSResolveDISPID - input=%ls\n", pDispParams->rgvarg[0].bstrVal);
        RSStringRef cfNameParam = RSStringCreateWithCharactersNoCopy(NULL, bstrNameParam, SysStringLen(bstrNameParam), RSAllocatorNull);
        RSArrayRef cfAddrList = _resolveDNSName(cfNameParam);
        RSRelease(cfNameParam);
        _prepareStringArrayReturnValue(cfAddrList, pVarResult);
        if (cfAddrList) RSRelease(cfAddrList);
        return S_OK;
    }

    else if (dispIdMember == PrimaryIPv4AddressesDISPID) {
        COM_LOG("DispatchInvoke - PrimaryIPv4AddressesDISPID, wFlags=%d\n", wFlags);
        assert(wFlags & DISPATCH_METHOD);
        if (pDispParams->cArgs != 0)
            return DISP_E_BADPARAMCOUNT;
        if (pDispParams->cNamedArgs != 0)
            return DISP_E_NONAMEDARGS;

        RSArrayRef cfAddrList = _JSPrimaryIpv4AddressesFunction();
        _prepareStringArrayReturnValue(cfAddrList, pVarResult);
        if (cfAddrList) RSRelease(cfAddrList);
        return S_OK;
    }
    
    else if (dispIdMember == PlatformNameDISPID) {
        COM_LOG("DispatchInvoke - PlatformNameDISPID, wFlags=%d\n", wFlags);
        assert(wFlags == DISPATCH_PROPERTYGET);
        if (pDispParams->cArgs != 0)
            return DISP_E_BADPARAMCOUNT;
        if (pDispParams->cNamedArgs != 0)
            return DISP_E_NONAMEDARGS;
        
        VariantInit(pVarResult);
        pVarResult->vt = VT_BSTR;
        pVarResult->bstrVal = SysAllocString(OLESTR("Win32"));
        return S_OK;
    }
    
    else {
        COM_LOG("DispatchInvoke - UNKNOWN MEMBER REQUESTED\n");
        return DISP_E_MEMBERNOTFOUND;
    }
}


//
// Implemetation of IActiveScriptSite interface, vTable and other COM glue
//

// Instance vars for our script site
typedef struct {
    IActiveScriptSite iSite;    // mostly just the vtable
    UInt32 refCount;
    RSAllocatorRef alloc;
} ScriptSite;

// Convert an ActiveScriptSite interface to one of our instance pointers
static inline ScriptSite *
thisFromISite(IActiveScriptSite *site) {
    return (ScriptSite*)((char *)site - offsetof(ScriptSite, iSite));
}

static HRESULT STDMETHODCALLTYPE SiteQueryInterface(IActiveScriptSite *site, REFIID riid, void **ppv);
static ULONG STDMETHODCALLTYPE SiteAddRef(IActiveScriptSite *site);
static ULONG STDMETHODCALLTYPE SiteRelease(IActiveScriptSite *site);
static HRESULT STDMETHODCALLTYPE SiteGetLCID(IActiveScriptSite *site, LCID *plcid);
static HRESULT STDMETHODCALLTYPE SiteGetItemInfo(IActiveScriptSite *site, LPCOLESTR pstrName, DWORD dwReturnMask, IUnknown **ppunkItem, ITypeInfo **ppTypeInfo);
static HRESULT STDMETHODCALLTYPE SiteGetDocVersionString(IActiveScriptSite *site, BSTR *pbstrVersionString);
static HRESULT STDMETHODCALLTYPE SiteOnScriptTerminate(IActiveScriptSite *site, const VARIANT *pvarResult, const EXCEPINFO *pexcepinfo);
static HRESULT STDMETHODCALLTYPE SiteOnStateChange(IActiveScriptSite *site, SCRIPTSTATE ssScriptState);
static HRESULT STDMETHODCALLTYPE SiteOnScriptError(IActiveScriptSite *site, IActiveScriptError *pase);
static HRESULT STDMETHODCALLTYPE SiteOnEnterScript(IActiveScriptSite *site);
static HRESULT STDMETHODCALLTYPE SiteOnLeaveScript(IActiveScriptSite *site);

static const IActiveScriptSiteVtbl ScriptSiteVTable = {
    SiteQueryInterface,
    SiteAddRef,
    SiteRelease,
    SiteGetLCID,
    SiteGetItemInfo,
    SiteGetDocVersionString,
    SiteOnScriptTerminate,
    SiteOnStateChange,
    SiteOnScriptError,
    SiteOnEnterScript,
    SiteOnLeaveScript
};

static HRESULT STDMETHODCALLTYPE
SiteQueryInterface(IActiveScriptSite *site, REFIID riid, void **ppv) {

#ifdef DO_COM_LOGGING
    LPOLESTR interfaceString;
    StringFromIID(riid, &interfaceString);
    COM_LOG("SiteQueryInterface: %ls\n", interfaceString);
    CoTaskMemFree(interfaceString);
#endif

    if (IsEqualIID(riid, &IID_IUnknown) || IsEqualIID (riid, &IID_IActiveScriptSite)) {
        *ppv = site;
        SiteAddRef(site);
        return NOERROR;
    }
    else {
        *ppv = 0;
        return E_NOINTERFACE;
    }
}

static ULONG STDMETHODCALLTYPE
SiteAddRef(IActiveScriptSite *site) {

    COM_LOG("SiteAddRef\n");
    ScriptSite *this = thisFromISite(site);
    return ++this->refCount;
}

static ULONG STDMETHODCALLTYPE
SiteRelease(IActiveScriptSite *site) {

    COM_LOG("SiteRelease\n");
    ScriptSite *this = thisFromISite(site);
    if (--this->refCount == 0) {
        RSAllocatorDeallocate(NULL, this);
        return 0;
    }
    return this->refCount;
}

static HRESULT STDMETHODCALLTYPE
SiteGetLCID(IActiveScriptSite *site, LCID *plcid) {

    COM_LOG("SiteGetLCID\n");
    return(plcid == NULL) ? E_POINTER : E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE
SiteGetItemInfo(IActiveScriptSite *site, 
            LPCOLESTR pstrName,             // address of item name
            DWORD dwReturnMask,             // bit mask for information retrieval
            IUnknown **ppunkItem,           // address of pointer to item's IUnknown
            ITypeInfo **ppTypeInfo)         // address of pointer to item's ITypeInfo
{
    COM_LOG("SiteGetItemInfo: %ls %lu\n", pstrName, dwReturnMask);

    if (dwReturnMask & SCRIPTINFO_IUNKNOWN) {
        if (!ppunkItem)
            return E_INVALIDARG;

        if (wcscmp(ITEM_NAME_ADDED_TO_JS, pstrName) == 0) {
            COM_LOG("SiteGetItemInfo: returning C gateway object\n");
            // create our dispatcher object, which we hand back to the engine
            ScriptSite *this = thisFromISite(site);
            Dispatcher *newDispatcher = RSAllocatorAllocate(this->alloc, sizeof(Dispatcher));
            newDispatcher->iDispatch.lpVtbl = &DispatchVTable;
            newDispatcher->refCount = 0;
            DispatchAddRef(&(newDispatcher->iDispatch));
            *ppunkItem = (IUnknown *)&(newDispatcher->iDispatch);
            return S_OK;
        }
        else
            *ppunkItem = NULL;
    }
    if (dwReturnMask & SCRIPTINFO_ITYPEINFO) {
        if (!ppTypeInfo)
            return E_INVALIDARG;
        *ppTypeInfo = NULL;
    }
    return TYPE_E_ELEMENTNOTFOUND;
}

static HRESULT STDMETHODCALLTYPE
SiteGetDocVersionString(IActiveScriptSite *site, BSTR *pbstrVersionString) {

    COM_LOG("SiteGetDocVersionString\n");
    return (pbstrVersionString == NULL) ? E_POINTER : E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE
SiteOnScriptTerminate(IActiveScriptSite *site, 
                      const VARIANT *pvarResult,      // address of script results
                      const EXCEPINFO *pexcepinfo)    // address of structure with exception information
{
    COM_LOG("SiteOnScriptTerminate\n");
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE
SiteOnStateChange(IActiveScriptSite *site, SCRIPTSTATE ssScriptState) {
    
    COM_LOG("SiteOnStateChange: %d\n", ssScriptState);
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE
SiteOnScriptError(IActiveScriptSite *site, IActiveScriptError *pase) {

    COM_LOG("SiteOnScriptError\n");
//#ifdef DO_COM_LOGGING
    do {
        char buffer[1024];
        EXCEPINFO exception;
        HRESULT hResult = IActiveScriptError_GetExceptionInfo(pase, &exception);
        CHECK_HRESULT(hResult, "IActiveScriptError_GetExceptionInfo");
        wcstombs(buffer, exception.bstrDescription, 1024);
        RSLog(0, RSSTR("JavaScript error when processing PAC file: %s"), buffer);
        BSTR sourceLine;
        hResult = IActiveScriptError_GetSourceLineText(pase, &sourceLine);
        if (hResult == S_OK) {
            wcstombs(buffer, sourceLine, 1024);
            RSLog(0, RSSTR("    offending code: %s"), buffer);
        }
    } while (0);
//#endif
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE
SiteOnEnterScript(IActiveScriptSite *site) {

    COM_LOG("SiteOnEnterScript\n");
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE
SiteOnLeaveScript(IActiveScriptSite *site) {

    COM_LOG("SiteOnLeaveScript\n");
    return S_OK;
}

//
//  End of COM interface implementations
//


// Feed a piece of code to the script engine
/* static */ HRESULT
_parseCodeChunk(RSStringRef code, JSContextRef runtime) {
    
    // convert from RSString to unicode buffer
    RSIndex len = RSStringGetLength(code);
    UniChar stackBuffer[12*1024];
    UniChar *uniBuf;
    if (len >= 12*1024) {
        uniBuf = malloc((len+1) * sizeof(UniChar));
    } else {
        uniBuf = stackBuffer;
    }
    RSStringGetCharacters(code, RSMakeRange(0, len), uniBuf);
    uniBuf[len] = 0;

    // parse the code
    EXCEPINFO exception = { 0 };
    HRESULT hResult = IActiveScriptParse_ParseScriptText(runtime->pActiveScriptParse, uniBuf, NULL, NULL, NULL, 0, 0, SCRIPTTEXT_ISVISIBLE, NULL, &exception);
    if (uniBuf != stackBuffer)
        free(uniBuf);
    return hResult;
}

/* static */ JSContextRef
_createJSRuntime(RSAllocatorRef alloc, RSStringRef js_support, RSStringRef js_pac) {

    JSContextRef runtime = (JSContextRef)RSAllocatorAllocate(alloc, sizeof(JSRun));
    runtime->pActiveScript = NULL;
    runtime->pActiveScriptParse = NULL;

    // create our site object, which we hand to the script object
    ScriptSite *newSite = RSAllocatorAllocate(alloc, sizeof(ScriptSite));
    newSite->iSite.lpVtbl = &ScriptSiteVTable;
    newSite->refCount = 0;
    newSite->alloc = alloc;
    // take a reference until site gets hooked up to other objects
    SiteAddRef(&(newSite->iSite));

    //??? How do we choose between COINIT_APARTMENTTHREADED and COINIT_MULTITHREADED
    //??? We need to arrange to make this call once per thread somehow
    //J: i would init/uninit for each usage and then deal with poor performance if it's a problem
    CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);

    HRESULT hResult = S_OK;
    do {
        CLSID clsid;
        hResult = CLSIDFromProgID( L"JavaScript", &clsid);
        CHECK_HRESULT(hResult, "CLSIDFromProgID");

        hResult = CoCreateInstance(&clsid, NULL, CLSCTX_INPROC_SERVER, &IID_IActiveScript, (void**)&(runtime->pActiveScript));
        CHECK_HRESULT(hResult, "CoCreateInstance");

        hResult = IActiveScript_QueryInterface(runtime->pActiveScript, &IID_IActiveScriptParse, (void **)&(runtime->pActiveScriptParse));
        CHECK_HRESULT(hResult, "QueryInterface:IID_IActiveScriptParse");
        
        hResult = IActiveScript_SetScriptSite(runtime->pActiveScript, &(newSite->iSite));
        CHECK_HRESULT(hResult, "IActiveScript_SetScriptSite");
        
        hResult = IActiveScriptParse_InitNew(runtime->pActiveScriptParse);
        CHECK_HRESULT(hResult, "IActiveScriptParse_InitNew");
        
        hResult = IActiveScript_AddNamedItem(runtime->pActiveScript, ITEM_NAME_ADDED_TO_JS, SCRIPTITEM_GLOBALMEMBERS|SCRIPTITEM_ISVISIBLE);
        CHECK_HRESULT(hResult, "IActiveScript_AddNamedItem");
        
        hResult = IActiveScript_SetScriptState(runtime->pActiveScript, SCRIPTSTATE_STARTED);
        CHECK_HRESULT(hResult, "IActiveScript_SetScriptState");
        
        hResult = _parseCodeChunk(js_support, runtime);
        CHECK_HRESULT(hResult, "IActiveScriptParse_ParseScriptText - js_support");
        
        hResult = _parseCodeChunk(js_pac, runtime);
        CHECK_HRESULT(hResult, "IActiveScriptParse_ParseScriptText - js_pac");
        
    } while (0);

    SiteRelease(&(newSite->iSite));
    if (hResult == S_OK) {
        return runtime;
    }
    else {
        _freeJSRuntime(runtime);
        return NULL;
    }
}


/* static */ void
_freeJSRuntime(JSContextRef runtime) {
    
    do {
        if (runtime->pActiveScript) {
            HRESULT hResult = IActiveScript_Close(runtime->pActiveScript);
            CHECK_HRESULT(hResult, "IActiveScript_Close");
        }

        long int refs;
#ifdef DO_COM_LOGGING
        // Print out current refCounts.  Both should be 2 at this point.  From experimenting,
        // it appears that when you Release either pActiveScript or pActiveScriptParse both end
        // losing one ref, so by releasing each one once, they both will go away.
        
        // In addition, another good test is to grep the log output for "DispatchAddRef" and
        // "DispatchRelease".  After this routine runs there should be an equal number (23 as of
        // this writing).
        if (runtime->pActiveScriptParse) {
            IActiveScriptParse_AddRef(runtime->pActiveScriptParse);
            refs = IActiveScriptParse_Release(runtime->pActiveScriptParse);
            COM_LOG("IActiveScriptParse started with %ld refs in _freeJSRuntime, should be 2\n", refs);
        }

        if (runtime->pActiveScript) {
            IActiveScript_AddRef(runtime->pActiveScript);
            refs = IActiveScript_Release(runtime->pActiveScript);
            COM_LOG("IActiveScript started with %ld refs in _freeJSRuntime, should be 2\n", refs);
        }
#endif

        if (runtime->pActiveScriptParse) {
            refs = IActiveScriptParse_Release(runtime->pActiveScriptParse);
            COM_LOG("IActiveScriptParse has %ld refs in _freeJSRuntime, should be 1\n", refs);
        }
        if (runtime->pActiveScriptParse) {
            refs = IActiveScript_Release(runtime->pActiveScript);
            COM_LOG("IActiveScript has %ld refs in _freeJSRuntime, should be 0\n", refs);
        }
    } while (0);

    RSAllocatorDeallocate(NULL, runtime);
}


// Prepare a RSString for passing as a param via IDispatch to a COM method
/* static */ void
_prepareStringParam(RSStringRef str, VARIANTARG *variant) {
    
    VariantInit(variant);
    variant->vt = VT_BSTR;
    RSIndex len = RSStringGetLength(str);
    UniChar stackBuffer[1024];
    UniChar *uniBuf;
    if (len > 1024) {
        uniBuf = malloc(len * sizeof(UniChar));
    } else {
        uniBuf = stackBuffer;
    }
    RSStringGetCharacters(str, RSMakeRange(0, len), uniBuf);
    variant->bstrVal = SysAllocStringLen(uniBuf, len);
    if (uniBuf != stackBuffer)
        free(uniBuf);
}


// Prepare a RSArray of RSString to be returned via IDispatch to a COM method
/* static */ void
_prepareStringArrayReturnValue(RSArrayRef cfArray, VARIANT *pVarResult) {

    RSIndex count = cfArray ? RSArrayGetCount(cfArray) : 0;
    VariantInit(pVarResult);
    pVarResult->vt = VT_ARRAY | VT_VARIANT;
    pVarResult->parray = SafeArrayCreateVector(VT_VARIANT, 0, count);
    long i;
    for (i = 0; i < count; i++) {
        RSStringRef cfValue = RSArrayObjectAtIndex(cfArray, i);
        UniChar stackBuffer[1024];
        UniChar *uniBuf;
        RSIndex len = RSStringGetLength(cfValue);
        if (len > 1024)
            uniBuf = malloc(len * sizeof(UniChar));
        else
            uniBuf = stackBuffer;
        RSStringGetCharacters(cfValue, RSMakeRange(0, len), uniBuf);
        BSTR bstrValue = SysAllocStringLen(uniBuf, len);
        if (uniBuf != stackBuffer)
            free(uniBuf);

        COM_LOG("DispatchInvoke - converting result string=%ls\n", bstrValue);
        VARIANT variant;
        VariantInit(&variant);
        variant.vt = VT_BSTR;
        variant.bstrVal = bstrValue;
        HRESULT hResult = SafeArrayPutElement(pVarResult->parray, &i, &variant);
        CHECK_HRESULT(hResult, "SafeArrayPutElement");
        SysFreeString(bstrValue);
    }    
}


/* static */ RSStringRef
_callPARSunction(RSAllocatorRef alloc, JSContextRef runtime, RSURLRef url, RSStringRef host) {

    RSStringRef result = NULL;
    HRESULT hResult = S_OK;
    IDispatch *pDisp = NULL;
    VARIANTARG paramValues[2];
    memset(paramValues, 0, sizeof(paramValues));
    do {
        // get the iDispatch interface we can use to call JS
        hResult = IActiveScript_GetScriptDispatch(runtime->pActiveScript, NULL, &pDisp);
        CHECK_HRESULT(hResult, "IActiveScript_GetScriptDispatch");
        
        // find the dispid (a cookie) for the function we want to call
        LPOLESTR funcName = OLESTR("__Apple_FindProxyForURL");
        DISPID dispid;
        hResult = (pDisp->lpVtbl->GetIDsOfNames)(pDisp, &IID_NULL, &funcName, 1, LOCALE_NEUTRAL, &dispid);
        CHECK_HRESULT(hResult, "IDispatch_GetIDsOfNames");

        // set up the two args we will pass to the JS routine.  args are in last-to-first order
        _prepareStringParam(host, &paramValues[0]);
        RSURLRef absURL = RSURLCopyAbsoluteURL(url);
        _prepareStringParam(RSURLGetString(absURL), &paramValues[1]);
        RSRelease(absURL);
        DISPPARAMS params = { paramValues, NULL, 2, 0 };

        // call it, afterwhich we can release the dispatch interface
        VARIANT pvarRetVal;
        hResult = (pDisp->lpVtbl->Invoke)(pDisp, dispid, &IID_NULL, LOCALE_NEUTRAL, DISPATCH_METHOD, &params, &pvarRetVal, NULL, NULL);
        CHECK_HRESULT(hResult, "IDispatch_Invoke");

        // convert result to RSString
        if (pvarRetVal.vt == VT_BSTR) {
            result = RSStringCreateWithCharacters(alloc, pvarRetVal.bstrVal, SysStringLen(pvarRetVal.bstrVal));
            COM_LOG("FindProxyForURL returned %ls\n", pvarRetVal.bstrVal);
        }
        else {
            COM_LOG("FindProxyForURL returned unexpected type %d\n", pvarRetVal.vt);
        }
        SysFreeString(pvarRetVal.bstrVal);
    } while (0);

    if (pDisp)
        (pDisp->lpVtbl->Release)(pDisp);
    if (paramValues[0].bstrVal)
        SysFreeString(paramValues[0].bstrVal);
    if (paramValues[1].bstrVal)
        SysFreeString(paramValues[1].bstrVal);

    return result;
}

/* static */ RSArrayRef
_JSPrimaryIpv4AddressesFunction(void) {

    RSMutableArrayRef list = RSArrayCreateMutable(RSAllocatorDefault, 0, &RSTypeArrayCallBacks);
    int sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock != INVALID_SOCKET) {

        // Note from MS KB 181520: this struct is a different size on NT 4.0, if we ever care about that OS
        INTERFACE_INFO interfaces[20];     // we only consider a maximum of 20, should be plenty
        DWORD len = sizeof(interfaces);
        DWORD resultLen;
        if (WSAIoctl(sock, SIO_GET_INTERFACE_LIST, NULL, 0, (LPVOID)interfaces, len, &resultLen, NULL, NULL) != SOCKET_ERROR) {
            int i;
            for (i = 0; i < (resultLen/sizeof(INTERFACE_INFO)); i++) {
                if ((interfaces[i].iiFlags & IFF_UP)
                    && !(interfaces[i].iiFlags & IFF_LOOPBACK)
                    && interfaces[i].iiAddress.Address.sa_family == AF_INET)
                {
                    RSStringRef str = stringFromAddr(&(interfaces[i].iiAddress.Address), sizeof(struct sockaddr_in));
                    if (str) {
                        RSArrayAddObject(list, str);
                        RSRelease(str);
                    }
                }
            }
        }
        closesocket(sock);
    }
    //RSLog(0, RSSTR("_JSPrimaryIpv4AddressesFunction results: %@"), list);
    return list;
}

#endif  /* __WIN32__ */

static RSURLRef _JSPacFileLocation = NULL;
static RSAbsoluteTime _JSPacFileExpiration = 0;
static JSContextRef _JSRuntime = NULL;

/* Must be called while holding the _JSLock.  It is the caller's responsibility to verify that expires is a valid 
   value; bad values can cause problems.  In general, the caller should make sure to successfully go through
   _stringFromLoadedPACStream to produce the expiry value. */ 
static void _JSSetEnvironmentForPAC(RSAllocatorRef alloc, RSURLRef url, RSAbsoluteTime expires, RSStringRef pacString) {

    if (_JSRuntime) {
        _freeJSRuntime(_JSRuntime);
        _JSRuntime = NULL;
    }

    _JSPacFileExpiration = 0;

    if (_JSPacFileLocation) {
        RSRelease(_JSPacFileLocation);
        _JSPacFileLocation = NULL;
    }
    
    RSStringRef js_support = _loadJSSupportFile();
    if (js_support) {
        _JSRuntime = _createJSRuntime(alloc, js_support, pacString);
        if (_JSRuntime) {
            _JSPacFileExpiration = expires;
            _JSPacFileLocation = RSRetain(url);
        }
    }
}

static RSSpinLock _JSLock = 0;

/* static */ RSStringRef
_JSFindProxyForURL(RSURLRef pac, RSURLRef url, RSStringRef host) {
    RSAllocatorRef alloc = RSGetAllocator(pac);
    RSStringRef result = NULL;
    RSStreamError err = {0, 0};

    if (!host) {
        return RSRetain(_kProxySupportDIRECT);
    }
    
    __RSSpinLock(&_JSLock);

    if (!_JSRuntime ||
        !_JSPacFileExpiration || (RSAbsoluteTimeGetCurrent() > _JSPacFileExpiration) ||
        !_JSPacFileLocation || !RSEqual(pac, _JSPacFileLocation)) {

        RSAbsoluteTime expires;
        RSStringRef js_pac = _loadPARSile(alloc, pac, &expires, &err);
        if (js_pac) {
            _JSSetEnvironmentForPAC(alloc, pac, expires, js_pac);
            RSRelease(js_pac);
        }
    }

    if (_JSRuntime) {
        result = _callPARSunction(alloc, _JSRuntime, url, host);
#if 0
        // debug code to enable leak checking - see more info in _freeJSRuntime
        _freeJSRuntime(_JSRuntime);
        _JSRuntime = NULL;
#endif
    }
    else if ((err.domain == RSStreamErrorDomainNetDB) ||					// Host name lookup failure
             (err.domain == RSStreamErrorDomainSystemConfiguration) ||		// Connection lost or not reachable
             (err.domain == RSStreamErrorDomainSSL) ||						// SSL errors (bad cert)
             (err.domain == _RSStreamErrorDomainNativeSockets) ||				// Socket errors
             (err.domain == RSStreamErrorDomainCustom))					// Timedout error for loader
    {
        result = RSRetain(_kProxySupportDIRECT);
    }

    __RSSpinUnlock(&_JSLock);
    return result;
}

static RSStringRef
_JSFindProxyForURLAsync(RSURLRef pac, RSURLRef url, RSStringRef host, BOOL *mustBlock) {
    RSStringRef result = NULL;
    RSAllocatorRef alloc = RSGetAllocator(pac);
    
    if (!host) {
        return RSRetain(_kProxySupportDIRECT);
    }
    
     __RSSpinLock(&_JSLock);

    if (!_JSRuntime || !_JSPacFileExpiration || 
        (RSAbsoluteTimeGetCurrent() > _JSPacFileExpiration) ||
        !_JSPacFileLocation || !RSEqual(pac, _JSPacFileLocation)) {
        *mustBlock = TRUE;
    } else {
        result = _callPARSunction(alloc, _JSRuntime, url, host);
        *mustBlock = FALSE;
    }
    __RSSpinUnlock(&_JSLock);

    return result;
}

// Platform independent piece of the DnsResolve callbacks
static RSArrayRef
_resolveDNSName(RSStringRef name) {
    
    RSStreamError error;
    RSMutableArrayRef list = NULL;
    RSHostRef lookup = RSHostCreateWithName(RSAllocatorDefault, name);
    
    if (lookup && RSHostStartInfoResolution(lookup, RSHostAddresses, &error) && !error.error) {
        
        RSArrayRef addrs = RSHostGetAddressing(lookup, NULL);
        RSIndex count = addrs ? RSArrayGetCount(addrs) : 0;
        
        if (count) {
            
            list = RSArrayCreateMutable(RSAllocatorDefault, count, &RSTypeArrayCallBacks);
            if (list) {
                RSIndex i;
                for (i = 0; i < count; i++) {
                    
                    RSDataRef saData = (RSDataRef)RSArrayObjectAtIndex(addrs, i);
					RSStringRef str = _RSNetworRSStringCreateWithRSDataAddress(RSAllocatorDefault, saData);
                    if (str) {
                        RSArrayAddObject(list, str);
                        RSRelease(str);
                    }
                }
                
                if (!RSArrayGetCount(list)) {
                    RSRelease(list);
                    list = NULL;
                }
            }
            
        }
        
    }
    
    if (lookup) RSRelease(lookup);
    
    return list;
}

#if defined(__MACH__)
static JSObjectRef
_JSDnsResolveFunction(void* context, JSObjectRef ctxt, RSArrayRef args) {

    JSObjectRef result = NULL;
    
    if (args && RSArrayGetCount(args) == 1) {
        
        RSTypeRef name = JSObjectCopyRSValue((JSObjectRef)RSArrayObjectAtIndex(args, 0));
        if (name && RSGetTypeID(name) == RSStringGetTypeID()) {
            
            RSArrayRef list = _resolveDNSName((RSStringRef)name);
            if (list) {
                result = JSObjectCreateWithRSType(list);
                RSRelease(list);
            }
        }
        
        if (name) RSRelease(name);
    }
    
    return result;
}


/* static */ JSObjectRef
_JSPrimaryIpv4AddressesFunction(void* context, JSObjectRef ctxt, RSArrayRef args) {

    JSObjectRef result = NULL;
    
    do {
        RSStringRef key = NULL;
        RSDictionaryRef value = NULL;
        RSStringRef interface = NULL;
        SCDynamicStoreRef store = SCDynamicStoreCreate(RSAllocatorDefault, RSSTR("JSEvaluator"), NULL, NULL);
        
        if (!store)
            break;
        
        key = SCDynamicStoreKeyCreateNetworkGlobalEntity(RSAllocatorDefault, kSCDynamicStoreDomainState, kSCEntNetIPv4);
        if (!key) {
            RSRelease(store);
            break;
        }
        
        value = SCDynamicStoreCopyValue(store, key);
        RSRelease(key);
        
        if (!value) {
            RSRelease(store);
            break;
        }
        
        interface = RSDictionaryGetValue(value, kSCDynamicStorePropNetPrimaryInterface);
        if (!interface) {
            RSRelease(value);
            RSRelease(store);
            break;
        }
        
        key = SCDynamicStoreKeyCreateNetworkInterfaceEntity(RSAllocatorDefault, kSCDynamicStoreDomainState, interface, kSCEntNetIPv4);
        RSRelease(value);

        if (!key) {
            RSRelease(store);
            break;
        }
        
        value = SCDynamicStoreCopyValue(store, key);
        
        RSRelease(store);

        if (!value) {
            break;
        }

        result = JSObjectCreateWithRSType(value);
        RSRelease(value);
        RSRelease(key);
        
    } while (0);
    
    return result;
}
#endif


#define BUF_SIZE 4096
static void releasePACStreamContext(void *info);

typedef struct {
    RSURLRef pacURL;
    RSURLRef targetURL;
    RSStringRef targetScheme;
    RSStringRef targetHost;
    
    RSMutableDataRef data;
    void *clientInfo;
    _RSProxyStreamCallBack cb;
} _PACStreamContext;

static void releasePACStreamContext(void *info) {
    _PACStreamContext *ctxt = (_PACStreamContext *)info;
    RSAllocatorRef alloc = RSGetAllocator(ctxt->data);
    RSRelease(ctxt->pacURL);
    RSRelease(ctxt->targetURL);
    RSRelease(ctxt->targetScheme);
    RSRelease(ctxt->targetHost);
    RSRelease(ctxt->data);
    RSAllocatorDeallocate(alloc, ctxt);
}

static void readBytesFromProxyStream(RSReadStreamRef proxyStream, _PACStreamContext *ctxt) {
    UInt8 buf[BUF_SIZE];
    RSIndex bytesRead = RSReadStreamRead(proxyStream, buf, BUF_SIZE);
    if (bytesRead > 0) {
        RSDataAppendBytes(ctxt->data, buf, bytesRead);
    }
}

static void proxyStreamCallback(RSReadStreamRef proxyStream, RSStreamEventType type, void *clientCallBackInfo) {
    _PACStreamContext *ctxt = (_PACStreamContext *)clientCallBackInfo;
    switch (type) {
    case RSStreamEventHasBytesAvailable: 
        readBytesFromProxyStream(proxyStream, ctxt);
        break;
    case RSStreamEventEndEncountered:
    case RSStreamEventErrorOccurred:
        ctxt->cb(proxyStream, ctxt->clientInfo);
        break;
    default:
        ;
    }
}


static RSReadStreamRef BuildStreamForPACURL(RSAllocatorRef alloc, RSURLRef pacURL, RSURLRef targetURL, RSStringRef targetScheme, RSStringRef targetHost, _RSProxyStreamCallBack callback, void *clientInfo) {
    BOOL isFile;
    RSReadStreamRef stream = _streamForPARSile(alloc, pacURL, &isFile);
    if (stream) {
        _PACStreamContext *pacContext = RSAllocatorAllocate(alloc, sizeof(_PACStreamContext));
        RSRetain(pacURL);
        pacContext->pacURL = pacURL;
        RSRetain(targetURL);
        pacContext->targetURL = targetURL;
        RSRetain(targetScheme);
        pacContext->targetScheme = targetScheme;
        RSRetain(targetHost);
        pacContext->targetHost = targetHost;
        
        pacContext->data = RSDataCreateMutable(alloc, 0);
        pacContext->clientInfo = clientInfo;
        pacContext->cb = callback;

        RSStreamClientContext streamContext;
        streamContext.version = 0;
        streamContext.info = pacContext;
        streamContext.retain = NULL;
        streamContext.release = releasePACStreamContext;
        streamContext.copyDescription = NULL;
        
        RSReadStreamSetClient(stream, RSStreamEventHasBytesAvailable | RSStreamEventErrorOccurred | RSStreamEventEndEncountered, proxyStreamCallback, &streamContext);
        RSReadStreamOpen(stream);
    }
    return stream;
}

RSMutableArrayRef _RSNetworkCopyProxyFromProxyStream(RSReadStreamRef proxyStream, BOOL *isComplete) {
    RSStreamStatus status = RSReadStreamGetStatus(proxyStream);
    if (status == RSStreamStatusOpen && RSReadStreamHasBytesAvailable(proxyStream)) {
        _PACStreamContext *pacContext = (_PACStreamContext *)_RSReadStreamGetClient(proxyStream);
        readBytesFromProxyStream(proxyStream, pacContext);
        status = RSReadStreamGetStatus(proxyStream);
    }
    if (status == RSStreamStatusAtEnd) {
        _PACStreamContext *pacContext = (_PACStreamContext *)_RSReadStreamGetClient(proxyStream);
        RSAllocatorRef alloc = RSGetAllocator(pacContext->data);
        RSStringRef pacString;
        RSStringRef pacResult;
        RSAbsoluteTime expiry;
        *isComplete = TRUE;
	 __RSSpinLock(&_JSLock);
        pacString = _stringFromLoadedPACStream(alloc, pacContext->data, proxyStream, &expiry);
        _JSSetEnvironmentForPAC(alloc, pacContext->pacURL, expiry, pacString);
        if (pacString)
			RSRelease(pacString);
        pacResult = _callPARSunction(alloc, _JSRuntime, pacContext->targetURL, pacContext->targetHost);
	__RSSpinUnlock(&_JSLock);

        RSMutableArrayRef result = RSArrayCreateMutable(alloc, 0, &RSTypeArrayCallBacks);
        _appendProxiesFromPACResponse(alloc, result, pacResult, pacContext->targetScheme);
        if(pacResult) RSRelease(pacResult);
        RSReadStreamClose(proxyStream);
        return result;
    } else if (status == RSStreamStatusError) {
        RSMutableArrayRef result = RSArrayCreateMutable(RSGetAllocator(proxyStream), 0, &RSTypeArrayCallBacks);
        RSArrayAddObject(result, RSNull);
        *isComplete = TRUE;
        return result;
    } else {
        *isComplete = FALSE;
        return NULL;
    }
}
