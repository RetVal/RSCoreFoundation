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
 *  RSHost.cpp
 *  RSNetwork
 *
 *  Created by Jeremy Wyld on Thu Nov 28 2002.
 *  Copyright (c) 2002 Apple Computer, Inc. All rights reserved.
 *
 */

#if 0
#pragma mark Description
#endif

/*
	RSHost is built as a RSRuntimeBase object.  The actual registration of the class type
	takes place when the first call for the type id is made (RSHostGetTypeID).  The object
	instantiation functions use this call for creation, therefore any of the creators will
	cause registration of the class.
 
	RSHost's underlying lookups can be any asynchronous RSType (i.e. RSMachPort, RSSocket,
	SCNetworkReachability, etc.).  The lookup should be created and scheduled on the run
	loops and modes saved in the "schedules" array.  The array is maintained in order to
	allow scheduling separate from the lookup.  With this, lookup can be created after
	schedules have been placed on the object.  The lookup can then be scheduled the same
	as the object.  The schedules array contains a list of pairs of run loops and modes
	(e.g. [<rl1>, <mode1>, <rl2>, <mode2>, ...]).  There can be zero or more items in
	the array, but the count should always be divisible by 2.

	A cancel is just another type of lookup.  A custom RSRunLoopSource is created which
	is simply signalled instantly.  This will cause synchronous lookups on other run loops
	(threads) to cancel out immediately.
 
	All resolved information is stored in a dictionary on the host object.  The key is the
	RSHostInfoType with the value being specific to the type.  Value types should be
	documented with the RSHostInfoType declarations.  In the case where a lookup produces
	no data, RSNull should be used for the value of the type.  This distinguishes the
	lookup as being performed and returning no data, which is different from not ever
	performing the lookup.
	
	Duplicate suppression is performed for hostname lookups.  The first hostname lookup
	that is performed creates a "master" lookup.  The master is just another RSHostRef
	whose lookup is started as a special info type.  This signals to it that it is the
	master and that there are clients of it.  The master is then placed in a global dictionary
	of outstanding lookups.  When a second is started, it is checked for existance in the
	global list.  If/When found, the second request is added to the list of clients.  The
	master lookup is scheduled on all loops and modes as the list of clients.  When the
	master lookup completes, all clients in the list are informed.  If all clients cancel,
	the master lookup will be canceled and removed from the master lookups list.
*/


#if 0
#pragma mark -
#pragma mark Includes
#endif
#include <RSNetwork/RSNetwork.h>
#include <RSNetwork/RSNetworkPriv.h>
#include "RSNetworkInternal.h"							/* for RSSpinLockLock and RSSpinLockUnlock */
#include "RSNetworkSchedule.h"

#include <math.h>										/* for fabs */
#include <sys/socket.h>
#include <netdb.h>
#include <SystemConfiguration/SystemConfiguration.h>	/* for SCNetworkReachability and flags */


#if 0
#pragma mark -
#pragma mark Constants
#endif

/* extern */ const SInt32 RSStreamErrorDomainNetDB = 12;
/* extern */ const SInt32 RSStreamErrorDomainSystemConfiguration = 13;

#define _RSNullHostInfoType				((RSHostInfoType)0xFFFFFFFF)

#define _RSHostIPv4Addresses				((RSHostInfoType)0x0000FFFE)
#define _RSHostIPv6Addresses				((RSHostInfoType)0x0000FFFD)
#define _RSHostMasterAddressLookup			((RSHostInfoType)0x0000FFFC)
#define _RSHostByPassMasterAddressLookup	((RSHostInfoType)0x0000FFFB)

#define _RSHostCacheMaxEntries				25
#define _RSHostCacheTimeout				((RSTimeInterval)1.0)


#if 0
#pragma mark -
#pragma mark Constant Strings
#endif

#ifdef __CONSTANT_RSSTRINGS__
#define _RSHostBlockingMode	RSSTR("_RSHostBlockingMode")
#define _RSHostDescribeFormat	RSSTR("<RSHost 0x%x>{info=%@}")
#else
static CONST_STRING_DECL(_RSHostBlockingMode, "_RSHostBlockingMode")
static CONST_STRING_DECL(_RSHostDescribeFormat, "<RSHost 0x%x>{info=%@}")
#endif	/* __CONSTANT_RSSTRINGS__ */


#if 0
#pragma mark -
#pragma mark RSHost struct
#endif

typedef struct {

	RSRuntimeBase 			_base;
	
	RSSpinLock              _lock;

	RSStreamError			_error;

	RSMutableDictionaryRef	_info;

	//RSMutableDictionaryRef  _lookups;		// key = RSHostInfoType and value = RSTypeRef
	RSTypeRef				_lookup;
	RSHostInfoType			_type;

	RSMutableArrayRef		_schedules;		// List of loops and modes
	RSHostClientCallBack	_callback;
	RSHostClientContext		_client;
} _RSHost;


#if 0
#pragma mark -
#pragma mark Static Function Declarations
#endif

static void _RSHostRegisterClass(void);
static _RSHost* _HostCreate(RSAllocatorRef allocator);

static void _HostDestroy(_RSHost* host);
static RSStringRef _HostDescribe(_RSHost* host);

static void _HostCancel(_RSHost* host);

static Boolean _HostBlockUntilComplete(_RSHost* host);

static Boolean _CreateLookup_NoLock(_RSHost* host, RSHostInfoType info, Boolean* _Radar4012176);

static RSMachPortRef _CreateMasterAddressLookup(RSStringRef name, RSHostInfoType info, RSTypeRef context, RSStreamError* error);
static RSTypeRef _CreateAddressLookup(RSStringRef name, RSHostInfoType info, void* context, RSStreamError* error);
static RSMachPortRef _CreateNameLookup(RSDataRef address, void* context, RSStreamError* error);
static SCNetworkReachabilityRef _CreateReachabilityLookup(RSTypeRef thing, void* context, RSStreamError* error);
static RSMachPortRef _CreateDNSLookup(RSTypeRef thing, RSHostInfoType type, void* context, RSStreamError* error);

static void _GetAddrInfoCallBack(int32_t status, struct addrinfo* res, void* ctxt);
static void _GetAddrInfoMachPortCallBack(RSMachPortRef port, void* msg, RSIndex size, void* info);

static void _GetNameInfoCallBack(int32_t status, char *hostname, char *serv, void* ctxt);
static void _GetNameInfoMachPortCallBack(RSMachPortRef port, void* msg, RSIndex size, void* info);

static void _NetworkReachabilityCallBack(SCNetworkReachabilityRef target, SCNetworkConnectionFlags flags, void* ctxt);
static void _NetworkReachabilityByIPCallBack(_RSHost* host);

static void _DNSCallBack(int32_t status, char *buf, uint32_t len, struct sockaddr *from, int fromlen, void *context);
static void _DNSMachPortCallBack(RSMachPortRef port, void* msg, RSIndex size, void* info);

static void _MasterCallBack(RSHostRef theHost, RSHostInfoType typeInfo, const RSStreamError *error, RSStringRef name);
static void _AddressLookupSchedule_NoLock(_RSHost* host, RSRunLoopRef rl, RSStringRef mode);
static void _AddressLookupPerform(_RSHost* host);

static void _ExpireCacheEntries(void);

static RSArrayRef _RSArrayCreateDeepCopy(RSAllocatorRef alloc, RSArrayRef array);
static Boolean _IsDottedIp(RSStringRef name);


#if 0
#pragma mark -
#pragma mark Globals
#endif

static _RSOnceLock _RSHostRegisterClassLock = _RSOnceInitializer;
static RSTypeID _RSHostTypeID = _RSRuntimeNotATypeID;

static _RSMutex* _HostLock;						/* Lock used for cache and master list */
static RSMutableDictionaryRef _HostLookups;		/* Active hostname lookups; for duplicate supression */
static RSMutableDictionaryRef _HostCache;		/* Cached hostname lookups (successes only) */


#if 0
#pragma mark -
#pragma mark Static Function Definitions
#endif

/* static */ void
_RSHostRegisterClass(void) {
	
	static const RSRuntimeClass _RSHostClass = {
		0,												// version
        0,
		"RSHost",										// class name
		NULL,      										// init
		NULL,      										// copy
		(void(*)(RSTypeRef))_HostDestroy,				// dealloc
		NULL,      										// equal
		NULL,      										// hash
		(RSStringRef(*)(RSTypeRef cf))_HostDescribe		// copyDebugDesc
	};
	

    _RSHostTypeID = __RSRuntimeRegisterClass(&_RSHostClass);

	/* Set up the "master" for simultaneous, duplicate lookups. */
	_HostLock = (_RSMutex*)RSAllocatorAllocate(RSAllocatorDefault, sizeof(_HostLock[0]));
	if (_HostLock) _RSMutexInit(_HostLock, FALSE);
	_HostLookups = RSDictionaryCreateMutable(RSAllocatorDefault,
													0,
													RSDictionaryRSTypeContext);
	
	_HostCache = RSDictionaryCreateMutable(RSAllocatorDefault,
										   0,
										   RSDictionaryRSTypeContext);
}


/* static */ _RSHost*
_HostCreate(RSAllocatorRef allocator) {
	
	_RSHost* result = (_RSHost*)__RSRuntimeCreateInstance(allocator,
														   RSHostGetTypeID(),
														   sizeof(result[0]) - sizeof(RSRuntimeBase));

	if (result) {

		// Save a copy of the base so it's easier to zero the struct
		RSRuntimeBase copy = result->_base;

		// Clear everything.
		memset(result, 0, sizeof(result[0]));

		// Put back the base
		memmove(&(result->_base), &copy, sizeof(result->_base));

		// No lookup by default
		result->_type = _RSNullHostInfoType;		

		// Create the dictionary of lookup information
		result->_info = RSDictionaryCreateMutable(allocator, 0, RSDictionaryNilKeyContext);

		// Create the list of loops and modes
		result->_schedules = RSArrayCreateMutable(allocator, 0);

		// If any failed, need to release and return null
		if (!result->_info || !result->_schedules) {
			RSRelease((RSTypeRef)result);
			result = NULL;
		}
	}

	return result;
}


/* static */ void
_HostDestroy(_RSHost* host) {
	
	// Prevent anything else from taking hold
	RSSpinLockLock(&(host->_lock));
	
	// Release the user's context info if there is some and a release method
	if (host->_client.info && host->_client.release)
		host->_client.release(host->_client.info);
	
	// If there is a lookup, release it.
	if (host->_lookup) {
		
		// Remove the lookup from run loops and modes
		_RSTypeUnscheduleFromMultipleRunLoops(host->_lookup, host->_schedules);
		
		// Go ahead and invalidate the lookup
		_RSTypeInvalidate(host->_lookup);
		
		// Release the lookup now.
		RSRelease(host->_lookup);
	}
	
	// Release any gathered information
	if (host->_info)
		RSRelease(host->_info);

	// Release the list of loops and modes
	if (host->_schedules)
		RSRelease(host->_schedules);
}


/* static */ RSStringRef
_HostDescribe(_RSHost* host) {
	
	RSStringRef result;
	
	RSSpinLockLock(&host->_lock);
	
	result = RSStringCreateWithFormat(RSGetAllocator((RSHostRef)host),
									  NULL,
									  _RSHostDescribeFormat,
									  host,
									  host->_info);
	
	RSSpinLockUnlock(&host->_lock);
	
	return result;
}


/* static */ void
_HostCancel(_RSHost* host) {
	
	RSHostClientCallBack cb = NULL;
	RSStreamError error;
	void* info = NULL;
	RSHostInfoType type = _RSNullHostInfoType;
	
	// Retain here to guarantee safety really after the lookups release,
	// but definitely before the callback.
	RSRetain((RSHostRef)host);
	
	// Lock the host
	RSSpinLockLock(&host->_lock);
	
	// If the lookup canceled, don't need to do any of this.
	if (host->_lookup) {
		
		// Save the callback if there is one at this time.
		cb = host->_callback;
		
		// Save the type of lookup for the callback.
		type = host->_type;
		
		// Save the error and client information for the callback
		memmove(&error, &(host->_error), sizeof(error));
		info = host->_client.info;
		
		// Remove the lookup from run loops and modes
		_RSTypeUnscheduleFromMultipleRunLoops(host->_lookup, host->_schedules);
		
		// Invalidate the run loop source that got here
		RSRunLoopSourceInvalidate((RSRunLoopSourceRef)(host->_lookup));
		
		// Release the lookup now.
		RSRelease(host->_lookup);
		host->_lookup = NULL;
		host->_type = _RSNullHostInfoType;
	}
	
	// Unlock the host so the callback can be made safely.
	RSSpinLockUnlock(&host->_lock);
	
	// If there is a callback, inform the client of the finish.
	if (cb)
		cb((RSHostRef)host, type, &error, info);
	
	// Go ahead and release now that the callback is done.
	RSRelease((RSHostRef)host);
}


/* static */ Boolean
_HostBlockUntilComplete(_RSHost* host) {
	
	// Assume success by default
	Boolean result = TRUE;
	RSRunLoopRef rl = RSRunLoopGetCurrent();
	
	// Schedule in the blocking mode.
	RSHostScheduleWithRunLoop((RSHostRef)host, rl, _RSHostBlockingMode);
	
	// Lock in order to check for lookup
	RSSpinLockLock(&(host->_lock));
	
	// Check that lookup exists.
	while (host->_lookup) {
		
		// Unlock again so the host can continue to be processed.
		RSSpinLockUnlock(&(host->_lock));
		
		// Run the loop in a private mode with it returning whenever a source
		// has been handled.
		RSRunLoopRunInMode(_RSHostBlockingMode, DBL_MAX, TRUE);
		
		// Lock again in preparation for lookup check
		RSSpinLockLock(&(host->_lock));		
	}
	
	// Fail if there was an error.
	if (host->_error.error)
		result = FALSE;
	
	// Unlock the host again.
	RSSpinLockUnlock(&(host->_lock));
	
	// Unschedule from the blocking mode
	RSHostUnscheduleFromRunLoop((RSHostRef)host, rl, _RSHostBlockingMode);
	
	return result;
}


/* static */ Boolean
_CreateLookup_NoLock(_RSHost* host, RSHostInfoType info, Boolean* _Radar4012176) {

	Boolean result = FALSE;

	// Get the existing names and addresses
	RSArrayRef names = (RSArrayRef)RSDictionaryGetValue(host->_info, (const void*)RSHostNames);
	RSArrayRef addrs = (RSArrayRef)RSDictionaryGetValue(host->_info, (const void*)RSHostAddresses);
	
	// Grab the first of each if they exist in order to perform any of the lookups
	RSStringRef name = names && ((RSTypeRef)names != RSNull) && RSArrayGetCount(names) ? (RSStringRef)RSArrayObjectAtIndex(names, 0) : NULL;
	RSDataRef addr = addrs && ((RSTypeRef)addrs != RSNull) && RSArrayGetCount(addrs) ? (RSDataRef)RSArrayObjectAtIndex(addrs, 0) : NULL;
	
	*_Radar4012176 = FALSE;
	
	// Only allow one lookup at a time
	if (host->_lookup)
		return result;
	
	switch (info) {

		// If a address lookup and there is a name, create and start the lookup.
		case RSHostAddresses:
		
			if (name) {
				
				RSArrayRef cached = NULL;
				
				/* Expire any entries from the cache */
				_ExpireCacheEntries();
				
				/* Lock the cache */
				_RSMutexLock(_HostLock);
				
				/* Go for a cache entry. */
				if (_HostCache)
					cached = (RSArrayRef)RSDictionaryGetValue(_HostCache, name);
	
				if (cached)
					RSRetain(cached);
	
				_RSMutexUnlock(_HostLock);

				/* Create a lookup if no cache entry. */
				if (!cached)
					host->_lookup = _CreateAddressLookup(name, info, host, &(host->_error));
					
				else {
					
					RSAllocatorRef alloc = RSGetAllocator(name);
					
					/* Make a copy of the addresses in the cached entry. */
					RSArrayRef cp = _RSArrayCreateDeepCopy(alloc,
														   RSHostGetInfo((RSHostRef)RSArrayObjectAtIndex(cached, 0), _RSHostMasterAddressLookup, NULL));
					
					RSRunLoopSourceContext ctxt = {
						0,
						host,
						RSRetain,
						RSRelease,
						RSDescription,
						NULL,
						NULL,
						NULL,
						NULL,
						(void (*)(void*))_AddressLookupPerform
					};
						
					/* Create the lookup source.  This source will be signalled immediately. */
					host->_lookup = RSRunLoopSourceCreate(alloc, 0, &ctxt);
					
					/* Upon success, add the data and signal the source. */
					if (host->_lookup && cp) {

						RSDictionarySetValue(host->_info, (const void*)info, cp);

						RSRunLoopSourceSignal((RSRunLoopSourceRef)host->_lookup);
						*_Radar4012176 = TRUE;
					}
					
					else {
						
						host->_error.error = ENOMEM;
						host->_error.domain = RSStreamErrorDomainPOSIX;
					}
					
					if (cp)
						RSRelease(cp);
					else if (host->_lookup) {
						RSRelease(host->_lookup);
						host->_lookup = NULL;
					}
					
					RSRelease(cached);
				}
			}
			
			break;

		// If a name lookup and there is an address, create and start the lookup.
		case RSHostNames:
			if (addr) host->_lookup = _CreateNameLookup(addr, host, &(host->_error));
			break;

		// Create a reachability check using the address or name (prefers address).
		case RSHostReachability:
			{
				RSTypeRef use = (addr != NULL) ? (RSTypeRef)addr : (RSTypeRef)name;
				
				/* Create the reachability lookup. */
				host->_lookup = _CreateReachabilityLookup(use, host, &(host->_error));
				
				/*
				** <rdar://problem/3612320> Check reachability by IP address doesn't work?
				**
				** Reachability when created with an IP has not future trigger point in
				** order to get the flags callback.  The behavior of the reachabilty object
				** can not change, so as a workaround, RSHost does an immediate flags
				** request and then creates the RSRunLoopSourceRef for the asynchronous
				** trigger.
				*/
				if (host->_lookup && ((use == addr) || _IsDottedIp(use))) {
					
					RSRunLoopSourceContext ctxt = {
						0,														// version
						host,													// info
						NULL,													// retain
						NULL,													// release
						NULL,													// copyDescription
						NULL,													// equal
						NULL,													// hash
						NULL,													// schedule
						NULL,													// cancel
						(void(*)(void*))(&_NetworkReachabilityByIPCallBack)		// perform
					};
					
					SCNetworkConnectionFlags flags = 0;
					RSAllocatorRef alloc = RSGetAllocator(host);
					
					/* Get the flags right away for dotted IP. */
					SCNetworkReachabilityGetFlags((SCNetworkReachabilityRef)(host->_lookup), &flags);
					
					/* Remove the callback that was set already. */
					SCNetworkReachabilitySetCallback((SCNetworkReachabilityRef)(host->_lookup), NULL, NULL);
					
					/* Toss out the lookup because a new one will be set up. */
					RSRelease(host->_lookup);
					host->_lookup = NULL;
					
					/* Create the asynchronous source */
					host->_lookup = RSRunLoopSourceCreate(alloc, 0, &ctxt);

					if (!host->_lookup) {
						host->_error.error = ENOMEM;
						host->_error.domain = RSStreamErrorDomainPOSIX;
					}
					
					else {
					
						// Create the data for hanging off the host info dictionary					
						RSDataRef reachability = RSDataCreate(alloc, (const UInt8*)&flags, sizeof(flags));
						
						// Make sure to toss the cached info now.
						RSDictionaryRemoveValue(host->_info, (const void*)RSHostReachability);
						
						// If didn't create the data, fail with out of memory.
						if (!reachability) {
							
							/* Release and toss the lookup. */
							RSRelease(host->_lookup);
							host->_lookup = NULL;
							
							host->_error.error = ENOMEM;
							host->_error.domain = RSStreamErrorDomainPOSIX;
						}
						
						else {
							// Save the reachability information
							RSDictionarySetValue(host->_info, (const void*)RSHostReachability, reachability);
							RSRelease(reachability);
							
							/* Signal the reachability for immediate attention. */
							RSRunLoopSourceSignal((RSRunLoopSourceRef)(host->_lookup));
						}
					}
				}
			}
			break;
		
		case 0x0000FFFC /* _RSHostMasterAddressLookup */:
			host->_lookup = _CreateMasterAddressLookup(name, info, host, &(host->_error));
			break;
		
		// Create a general DNS check using the name or address (prefers name).
		default:

			if (name) {
				if ((info == _RSHostIPv4Addresses) || (info == _RSHostIPv6Addresses) || (info == _RSHostByPassMasterAddressLookup))
					host->_lookup = _CreateMasterAddressLookup(name, info, host, &(host->_error));
				else
					host->_lookup = _CreateDNSLookup(name, info, host, &(host->_error));
			}
			else if (addr) {
				
				name = _RSNetworRSStringCreateWithRSDataAddress(RSGetAllocator(addr), addr);
				
				if (name) {
					
					host->_lookup = _CreateDNSLookup(name, info, host, &(host->_error));
					
					RSRelease(name);
				}
				
				else {
					
					host->_error.error = ENOMEM;
					host->_error.domain = RSStreamErrorDomainPOSIX;
				}
			}
			break;
	}
	
	if (host->_lookup) {
		host->_type = info;
		result = TRUE;
	}
		
	return result;
}


/* static */ RSMachPortRef
_CreateMasterAddressLookup(RSStringRef name, RSHostInfoType info, RSTypeRef context, RSStreamError* error) {
	
	UInt8* buffer;
	RSAllocatorRef allocator = RSGetAllocator(name);
	RSIndex converted, length = RSStringGetLength(name);
	RSMachPortRef result = NULL;
	
	// Get the bytes of the conversion
	buffer =  _RSStringGetOrCreateCString(allocator, name, NULL, &converted, RSStringEncodingUTF8);
	
	// If the buffer failed to create, set the error and bail.
	if (!buffer) {
	
		// Set the no memory error.
		error->error = ENOMEM;
		error->domain = RSStreamErrorDomainPOSIX;
		
		// Bail
		return result;
	}
	
	// See if all the bytes got converted.
	if (converted != length) {
		
		// If not, this amounts to a host not found error.  This is to primarily
		// deal with embedded bad characters in host names coming from URL's
		// (e.g. www.apple.com%00www.notapple.com).
		error->error = HOST_NOT_FOUND;
		error->domain = (RSStreamErrorDomain)RSStreamErrorDomainNetDB;
	}
	
	// Got a good name to send to lookup.
	else {
		
        struct addrinfo hints;
		mach_port_t prt = MACH_PORT_NULL;
		RSMachPortContext ctxt = {0, (void*)context, RSRetain, RSRelease, RSDescription};
		
		// Set up the hints for getaddrinfo
        memset(&hints, 0, sizeof(hints));
		
#ifdef AI_PARALLEL
        hints.ai_flags = AI_ADDRCONFIG | AI_PARALLEL;
#else
        hints.ai_flags = AI_ADDRCONFIG;
#endif /* AI_PARALLEL */
		
		hints.ai_socktype = SOCK_STREAM;
		
		hints.ai_family = (info == _RSHostIPv4Addresses) ? AF_INET :
			(info == _RSHostIPv6Addresses) ? AF_INET6 : AF_UNSPEC;
			
		// Start the async lookup
		error->error = getaddrinfo_async_start(&prt, (const char*)buffer, NULL, &hints, _GetAddrInfoCallBack, (void*)context);
		
		// If the callback port was created, attempt to create the RSMachPort wrapper on it.
		if (!prt ||
			!(result = RSMachPortCreateWithPort(allocator, prt, _GetAddrInfoMachPortCallBack, &ctxt, NULL)))
		{
			
			// Failure somewhere so setup error the proper way.  If error->error is
			// set already, it was a netdb error.
			if (error->error) {
				
				/* If it's a system error, get the real error otherwise it's a NetDB error. */
				if (EAI_SYSTEM != error->error)
					error->domain = (RSStreamErrorDomain)RSStreamErrorDomainNetDB;
				else {
					error->error = errno;
					error->domain = RSStreamErrorDomainPOSIX;
				}
			}
			
			// No error set, see if errno has anything.  If so, mark the error as
			// a POSIX error.
			else if (error->error == errno)
				error->domain = (RSStreamErrorDomain)RSStreamErrorDomainPOSIX;
				
			// Don't know what happened, so mark it as an internal netdb error.
			else {
				error->error = NETDB_INTERNAL;
				error->domain = (RSStreamErrorDomain)RSStreamErrorDomainNetDB;
			}
		}
	}
	
	// Release the buffer that was allocated for the name
	RSAllocatorDeallocate(allocator, buffer);
	
	return result;
}


/* static */ RSTypeRef
_CreateAddressLookup(RSStringRef name, RSHostInfoType info, void* context, RSStreamError* error) {
	
	RSTypeRef result = NULL;
	
	memset(error, 0, sizeof(error[0]));

	if (info == _RSHostMasterAddressLookup)
		result = _CreateMasterAddressLookup(name, info, context, error);
		
	else {
		RSHostRef host = NULL;
		RSMutableArrayRef list = NULL;
		
		/* Lock the master lookups list and cache */
		_RSMutexLock(_HostLock);

		/* Get the list with the host lookup and other sources for this name */
		list = (RSMutableArrayRef)RSDictionaryGetValue(_HostLookups, name);
		
		/* Get the host if there is a list.  Host is at index zero. */
		if (list)
			host = (RSHostRef)RSArrayObjectAtIndex(list, 0);
		
		/* If there is no list, this is the first; so set everything up. */
		else {
			
			/* Create the list to hold the host and sources. */
			list = RSArrayCreateMutable(RSAllocatorDefault, 0, &RSTypeArrayCallBacks);
			
			/* Set up the error in case the list wasn't created. */
			if (!list) {
				error->error = ENOMEM;
				error->domain = RSStreamErrorDomainPOSIX;
			}
			
			else {
				
				name = RSStringCreateCopy(RSAllocatorDefault, name);
				
				/* Add the list of clients for the name to the dictionary. */
				RSDictionarySetValue(_HostLookups, name, list);
				
				RSRelease(name);
				
				/* Dictionary holds it now. */
				RSRelease(list);
				
				/* Make the real lookup. */
				host = RSHostCreateWithName(RSAllocatorDefault, name);
				
				if (!host) {
					error->error = ENOMEM;
					error->domain = RSStreamErrorDomainPOSIX;
				}
				
				else {
					RSHostClientContext ctxt = {0, (void*)name, RSRetain, RSRelease, RSDescription};
					
					/* Place the RSHost at index 0. */
					RSArrayAppendValue(list, host);
					
					/* The list holds it now. */
					RSRelease(host);
					
					/* Set the client for asynchronous callback. */
					RSHostSetClient(host, (RSHostClientCallBack)_MasterCallBack, &ctxt);
					
					/* Kick off the resolution.  NULL the client if the resolution can't start. */
					if (!RSHostStartInfoResolution(host, _RSHostMasterAddressLookup, error)) {
					
						RSHostSetClient(host, NULL, NULL);
						
						/* If it failed, don't keep it in the outstanding lookups list. */
						RSDictionaryRemoveValue(_HostLookups, name);
					}
				}
			}
		}
		
		/* Everything is still good? */
		if (!error->error) {
			
			RSRunLoopSourceContext ctxt = {
				0,
				context,
				RSRetain,
				RSRelease,
				RSDescription,
				NULL,
				NULL,
				(void (*)(void*, RSRunLoopRef, RSStringRef))_AddressLookupSchedule_NoLock,
				NULL,
				(void (*)(void*))_AddressLookupPerform
			};
				
			/* Create the lookup source.  This source will be signalled once the shared lookup finishes. */
			result = RSRunLoopSourceCreate(RSGetAllocator(name), 0, &ctxt);
			
			/* If it succeed, add it to the list of other pending clients. */
			if (result) {
				RSArrayAppendValue(list, result);
			}
			
			else {
				
				error->error = ENOMEM;
				error->domain = RSStreamErrorDomainPOSIX;
				
				/* If this was going to be the only client, need to clean up. */
				if (host && RSArrayGetCount(list) == 1) {
					
					/* NULL the client for the Mmster lookup and cancel it. */
					RSHostSetClient(host, NULL, NULL);
					RSHostCancelInfoResolution(host, _RSHostMasterAddressLookup);
					
					/* Remove it from the list of pending lookups and clients. */
					RSDictionaryRemoveValue(_HostLookups, name);
				}
			}
		}
		
		_RSMutexUnlock(_HostLock);
	}

	return result;
}


/* static */ RSMachPortRef
_CreateNameLookup(RSDataRef address, void* context, RSStreamError* error) {
	
	mach_port_t prt = MACH_PORT_NULL;
	RSMachPortRef result = NULL;

	RSMachPortContext ctxt = {0, (void*)context, RSRetain, RSRelease, RSDescription};		
	struct sockaddr* sa = (struct sockaddr*)RSDataGetBytePtr(address);
		
	// Start the async lookup
	error->error = getnameinfo_async_start(&prt, sa, sa->sa_len, 0, _GetNameInfoCallBack, (void*)context);
	
	// If the callback port was created, attempt to create the RSMachPort wrapper on it.
	if (!prt ||
		!(result = RSMachPortCreateWithPort(RSGetAllocator(address), prt, _GetNameInfoMachPortCallBack, &ctxt, NULL)))
	{
		
		// Failure somewhere so setup error the proper way.  If error->error is
		// set already, it was a netdb error.
		if (error->error) {
			
			/* If it's a system error, get the real error otherwise it's a NetDB error. */
			if (EAI_SYSTEM != error->error)
				error->domain = (RSStreamErrorDomain)RSStreamErrorDomainNetDB;
			else {
				error->error = errno;
				error->domain = RSStreamErrorDomainPOSIX;
			}
		}
		
		// No error set, see if errno has anything.  If so, mark the error as
		// a POSIX error.
		else if (error->error = errno)
			error->domain = (RSStreamErrorDomain)RSStreamErrorDomainPOSIX;
			
		// Don't know what happened, so mark it as an internal netdb error.
		else {
			error->error = NETDB_INTERNAL;
			error->domain = (RSStreamErrorDomain)RSStreamErrorDomainNetDB;
		}
	}

	// Return the RSMachPortRef
	return result;
}


/* static */ SCNetworkReachabilityRef
_CreateReachabilityLookup(RSTypeRef thing, void* context, RSStreamError* error) {
	
	SCNetworkReachabilityRef result = NULL;
	
	// If the passed in argument is a RSData, create the reachability object
	// with the address.
	if (RSGetTypeID(thing) == RSDataGetTypeID()) {
		result = SCNetworkReachabilityCreateWithAddress(RSGetAllocator(thing),
														(struct sockaddr*)RSDataGetBytePtr((RSDataRef)thing));
	}
	
	// A RSStringRef means to create a reachability object by name.
	else {
		UInt8* buffer;
		RSAllocatorRef allocator = RSGetAllocator(thing);
		RSIndex converted, length = RSStringGetLength((RSStringRef)thing);
		
		// Get the bytes of the conversion
		buffer =  _RSStringGetOrCreateCString(allocator, (RSStringRef)thing, NULL, &converted, RSStringEncodingUTF8);
		
		// If the buffer failed to create, set the error and bail.
		if (!buffer) {
			
			// Set the no memory error.
			error->error = ENOMEM;
			error->domain = RSStreamErrorDomainPOSIX;
			
			// Bail
			return result;
		}
		
		// See if all the bytes got converted.
		if (converted != length) {
			
			// If not, this amounts to a host not found error.  This is to primarily
			// deal with embedded bad characters in host names coming from URL's
			// (e.g. www.apple.com%00www.notapple.com).
			error->error = HOST_NOT_FOUND;
			error->domain = (RSStreamErrorDomain)RSStreamErrorDomainNetDB;
		}
		
		// Got a good name to send to lookup.
		else {
			
			// Create the reachability lookup
			result = SCNetworkReachabilityCreateWithName(allocator, (const char*)buffer);
		}
		
		// Release the buffer that was allocated for the name
		RSAllocatorDeallocate(allocator, buffer);
	}
	
	// If the reachability object was created, need to set the callback context.
	if (result) {
		SCNetworkReachabilityContext ctxt = {0, (void*)context, RSRetain, RSRelease, RSDescription};
		
		// Set the callback information
		SCNetworkReachabilitySetCallback(result, _NetworkReachabilityCallBack, &ctxt);
	}
	
	// If no reachability was created, make sure the error is set.
	else if (!error->error) {
		
		// Set it to errno
		error->error = errno;
		
		// If errno was set, place in the POSIX error domain.
		if (error->error)
			error->domain = (RSStreamErrorDomain)RSStreamErrorDomainPOSIX;
	}
	
	return result;
}


/* static */ RSMachPortRef
_CreateDNSLookup(RSTypeRef thing, RSHostInfoType type, void* context, RSStreamError* error) {
	
	UInt8* buffer;
	RSAllocatorRef allocator = RSGetAllocator(thing);
	RSIndex converted, length = RSStringGetLength((RSStringRef)thing);
	RSMachPortRef result = NULL;
	
	// Get the bytes of the conversion
	buffer =  _RSStringGetOrCreateCString(allocator, (RSStringRef)thing, NULL, &converted, RSStringEncodingUTF8);
	
	// If the buffer failed to create, set the error and bail.
	if (!buffer) {
		
		// Set the no memory error.
		error->error = ENOMEM;
		error->domain = RSStreamErrorDomainPOSIX;
		
		// Bail
		return result;
	}
	
	// See if all the bytes got converted.
	if (converted != length) {
		
		// If not, this amounts to a host not found error.  This is to primarily
		// deal with embedded bad characters in host names coming from URL's
		// (e.g. www.apple.com%00www.notapple.com).
		error->error = HOST_NOT_FOUND;
		error->domain = (RSStreamErrorDomain)RSStreamErrorDomainNetDB;
	}
	
	// Got a good name to send to lookup.
	else {
		
		mach_port_t prt = MACH_PORT_NULL;
		RSMachPortContext ctxt = {0, (void*)context, RSRetain, RSRelease, RSDescription};
		
		// Start the async lookup
		error->error = dns_async_start(&prt, (const char*)buffer, ((type & 0xFFFF0000) >> 16), (type & 0x0000FFFF), 1, _DNSCallBack, (void*)context);
		
		// If the callback port was created, attempt to create the RSMachPort wrapper on it.
		if (!prt ||
			!(result = RSMachPortCreateWithPort(allocator, prt, _DNSMachPortCallBack, &ctxt, NULL)))
		{
			
			// Failure somewhere so setup error the proper way.  If error->error is
			// set already, it was a netdb error.
			if (error->error) {
				
				/* If it's a system error, get the real error otherwise it's a NetDB error. */
				if (EAI_SYSTEM != error->error)
					error->domain = (RSStreamErrorDomain)RSStreamErrorDomainNetDB;
				else {
					error->error = errno;
					error->domain = RSStreamErrorDomainPOSIX;
				}
			}
			
			// No error set, see if errno has anything.  If so, mark the error as
			// a POSIX error.
			else if (error->error = errno)
				error->domain = (RSStreamErrorDomain)RSStreamErrorDomainPOSIX;
			
			// Don't know what happened, so mark it as an internal netdb error.
			else {
				error->error = NETDB_INTERNAL;
				error->domain = (RSStreamErrorDomain)RSStreamErrorDomainNetDB;
			}
		}
	}
	
	// Release the buffer that was allocated for the name
	RSAllocatorDeallocate(allocator, buffer);
	
	return result;
}


/* static */ void
_GetAddrInfoCallBack(int32_t status, struct addrinfo* res, void* ctxt) {

	_RSHost* host = (_RSHost*)ctxt;
	RSHostClientCallBack cb = NULL;
	RSStreamError error;
	void* info = NULL;
	RSHostInfoType type = _RSNullHostInfoType;
	
	// Retain here to guarantee safety really after the lookups release,
	// but definitely before the callback.
	RSRetain((RSHostRef)host);
	
	// Lock the host
	RSSpinLockLock(&host->_lock);

	// If the lookup canceled, don't need to do any of this.
	if (host->_lookup) {
		
		// Make sure to toss the cached info now.
		RSDictionaryRemoveValue(host->_info, (const void*)(host->_type));
		
		// Set the error if got one back from getaddrinfo
		if (status) {
			
			/* If it's a system error, get the real error. */
			if (EAI_SYSTEM == status) {
				host->_error.error = errno;
				host->_error.domain = RSStreamErrorDomainPOSIX;
			}
			
			else {
				host->_error.error = status;
				host->_error.domain = (RSStreamErrorDomain)RSStreamErrorDomainNetDB;
			}
			
			// Mark to indicate the resolution was performed.
			RSDictionarySetValue(host->_info, (const void*)(host->_type), RSNull);
		}
		
		else {

			RSMutableArrayRef addrs;
			RSAllocatorRef allocator = RSGetAllocator((RSHostRef)host);
			
			// This is the list of new addresses to be saved.
			addrs = RSArrayCreateMutable(allocator, 0, &RSTypeArrayCallBacks);
			
			// Save the memory error if the address cache failed to create.
			if (!addrs) {
				host->_error.error = ENOMEM;
				host->_error.domain = RSStreamErrorDomainPOSIX;
				
				// Mark to indicate the resolution was performed.
				RSDictionarySetValue(host->_info, (const void*)(host->_type), RSNull);
			}
			
			else {
				struct addrinfo* i;
				
				// Loop through all of the addresses saving them in the array.
				for (i = res; i; i = i->ai_next) {
					
					RSDataRef data;
					
					// Bypass any address families that are not understood by RSSocketStream
					if (i->ai_addr->sa_family != AF_INET && i->ai_addr->sa_family != AF_INET6)
						continue;
					
					// Wrap the address in a RSData
					data = RSDataCreate(allocator, (UInt8*)(i->ai_addr), i->ai_addr->sa_len);
					
					// Fail with a memory error if the address wouldn't wrap.
					if (!data) {
						
						host->_error.error = ENOMEM;
						host->_error.domain = RSStreamErrorDomainPOSIX;
						
						// Release the addresses and mark as NULL so as not to save later.
						RSRelease(addrs);
						addrs = NULL;
						
						// Just fail now.
						break;
					}
					
					// Add the address and continue on to the next.
					RSArrayAppendValue(addrs, data);
					RSRelease(data);
				}
				
				// If the list is still good, need to save it.
				if (addrs) {
					
					// Save the list of address on the host.
					RSDictionarySetValue(host->_info, (const void*)(host->_type), addrs);
					RSRelease(addrs);
				}
			}
		}
		
		// Save the callback if there is one at this time.
		cb = host->_callback;
		
		type = host->_type;
		
		// Save the error and client information for the callback
		memmove(&error, &(host->_error), sizeof(error));
		info = host->_client.info;
		
		// Remove the lookup from run loops and modes
		_RSTypeUnscheduleFromMultipleRunLoops(host->_lookup, host->_schedules);
		
		// Go ahead and invalidate the lookup
		RSMachPortInvalidate((RSMachPortRef)(host->_lookup));
		
		// Release the lookup now.
		RSRelease(host->_lookup);
		host->_lookup = NULL;
		host->_type = _RSNullHostInfoType;
	}
	
	// Unlock the host so the callback can be made safely.
	RSSpinLockUnlock(&host->_lock);
    
	// Release the results if some were received.
    if (res)
        freeaddrinfo(res);
	
	// If there is a callback, inform the client of the finish.
	if (cb)
		cb((RSHostRef)host, type, &error, info);
	
	// Go ahead and release now that the callback is done.
	RSRelease((RSHostRef)host);	
}


/* static */ void
_GetAddrInfoMachPortCallBack(RSMachPortRef port, void* msg, RSIndex size, void* info) {
	
	getaddrinfo_async_handle_reply(msg);
}


/* static */ void
_GetNameInfoCallBack(int32_t status, char *hostname, char *serv, void* ctxt) {

	_RSHost* host = (_RSHost*)ctxt;
	RSHostClientCallBack cb = NULL;
	RSStreamError error;
	void* info = NULL;
	
	// Retain here to guarantee safety really after the lookups release,
	// but definitely before the callback.
	RSRetain((RSHostRef)host);
	
	// Lock the host
	RSSpinLockLock(&host->_lock);
	
	// If the lookup canceled, don't need to do any of this.
	if (host->_lookup) {
		
		// Make sure to toss the cached info now.
		RSDictionaryRemoveValue(host->_info, (const void*)RSHostNames);
		
		// Set the error if got one back from getnameinfo
		if (status) {
			
			/* If it's a system error, get the real error. */
			if (EAI_SYSTEM == status) {
				host->_error.error = errno;
				host->_error.error = RSStreamErrorDomainPOSIX;
			}
			
			else {
				host->_error.error = status;
				host->_error.domain = (RSStreamErrorDomain)RSStreamErrorDomainNetDB;
			}
			
			// Mark to indicate the resolution was performed.
			RSDictionarySetValue(host->_info, (const void*)RSHostNames, RSNull);
		}
		
		else {
				
			RSAllocatorRef allocator = RSGetAllocator((RSHostRef)host);
			
			// Create the name from the given response.
			RSStringRef name = RSStringCreateWithCString(allocator, hostname, RSStringEncodingUTF8);
			
			// If didn't create the name, fail with out of memory.
			if (!name) {
				host->_error.error = ENOMEM;
				host->_error.domain = RSStreamErrorDomainPOSIX;
			}
			
			else {
				// Create the list to hold the name.
				RSArrayRef names = RSArrayCreate(allocator, (const void**)(&name), 1, &RSTypeArrayCallBacks);
				
				// Don't need the retain anymore
				RSRelease(name);
				
				// Failed to create the list of names so mark out of memory.
				if (!names) {
					host->_error.error = ENOMEM;
					host->_error.domain = RSStreamErrorDomainPOSIX;
				}
				
				// Save the list of names on the host.
				else {
					RSDictionarySetValue(host->_info, (const void*)RSHostNames, names);
					RSRelease(names);					
				}
			}
		}
		
		// Save the callback if there is one at this time.
		cb = host->_callback;
		
		// Save the error and client information for the callback
		memmove(&error, &(host->_error), sizeof(error));
		info = host->_client.info;
		
		// Remove the lookup from run loops and modes
		_RSTypeUnscheduleFromMultipleRunLoops(host->_lookup, host->_schedules);
		
		// Go ahead and invalidate the lookup
		RSMachPortInvalidate((RSMachPortRef)(host->_lookup));
		
		// Release the lookup now.
		RSRelease(host->_lookup);
		host->_lookup = NULL;
		host->_type = _RSNullHostInfoType;
	}
	
	// Unlock the host so the callback can be made safely.
	RSSpinLockUnlock(&host->_lock);

	// Release the results if there were any.
	if (serv) free(serv);
	if (hostname) free(hostname);
	
	// If there is a callback, inform the client of the finish.
	if (cb)
		cb((RSHostRef)host, RSHostNames, &error, info);
	
	// Go ahead and release now that the callback is done.
	RSRelease((RSHostRef)host);
}


/* static */ void
_GetNameInfoMachPortCallBack(RSMachPortRef port, void* msg, RSIndex size, void* info) {
	
	getnameinfo_async_handle_reply(msg);
}


/* static */ void
_NetworkReachabilityCallBack(SCNetworkReachabilityRef target, SCNetworkConnectionFlags flags, void* ctxt) {
	
	_RSHost* host = (_RSHost*)ctxt;
	RSHostClientCallBack cb = NULL;
	RSStreamError error;
	void* info = NULL;
	
	// Retain here to guarantee safety really after the lookups release,
	// but definitely before the callback.
	RSRetain((RSHostRef)host);
	
	// Lock the host
	RSSpinLockLock(&host->_lock);
	
	// If the lookup canceled, don't need to do any of this.
	if (host->_lookup) {
		
		// Create the data for hanging off the host info dictionary
		RSDataRef reachability = RSDataCreate(RSGetAllocator(target), (const UInt8*)&flags, sizeof(flags));
		
		// Make sure to toss the cached info now.
		RSDictionaryRemoveValue(host->_info, (const void*)RSHostReachability);
		
		// If didn't create the data, fail with out of memory.
		if (!reachability) {
			host->_error.error = ENOMEM;
			host->_error.domain = RSStreamErrorDomainPOSIX;
		}
		
		else {
			// Save the reachability information
			RSDictionarySetValue(host->_info, (const void*)RSHostReachability, reachability);
			RSRelease(reachability);
		}
		
		// Save the callback if there is one at this time.
		cb = host->_callback;
		
		// Save the error and client information for the callback
		memmove(&error, &(host->_error), sizeof(error));
		info = host->_client.info;
		
		// Remove the lookup from run loops and modes
		_RSTypeUnscheduleFromMultipleRunLoops(host->_lookup, host->_schedules);
		
		// "Invalidate" the reachability object by removing the client
		SCNetworkReachabilitySetCallback((SCNetworkReachabilityRef)(host->_lookup), NULL, NULL);
		
		// Release the lookup now.
		RSRelease(host->_lookup);
		host->_lookup = NULL;
		host->_type = _RSNullHostInfoType;
	}
	
	// Unlock the host so the callback can be made safely.
	RSSpinLockUnlock(&host->_lock);
	
	// If there is a callback, inform the client of the finish.
	if (cb)
		cb((RSHostRef)host, RSHostReachability, &error, info);
	
	// Go ahead and release now that the callback is done.
	RSRelease((RSHostRef)host);
}


/* static */ void
_NetworkReachabilityByIPCallBack(_RSHost* host) {
	
	RSHostClientCallBack cb = NULL;
	RSStreamError error;
	void* info = NULL;
	
	// Retain here to guarantee safety really after the lookups release,
	// but definitely before the callback.
	RSRetain((RSHostRef)host);
	
	// Lock the host
	RSSpinLockLock(&host->_lock);
	
	// If the lookup canceled, don't need to do any of this.
	if (host->_lookup) {
		
		// Save the callback if there is one at this time.
		cb = host->_callback;
		
		// Save the error and client information for the callback
		memmove(&error, &(host->_error), sizeof(error));
		info = host->_client.info;
		
		// Remove the lookup from run loops and modes
		_RSTypeUnscheduleFromMultipleRunLoops(host->_lookup, host->_schedules);
		
		// Invalidate the run loop source that got here
		RSRunLoopSourceInvalidate((RSRunLoopSourceRef)(host->_lookup));
		
		// Release the lookup now.
		RSRelease(host->_lookup);
		host->_lookup = NULL;
		host->_type = _RSNullHostInfoType;
	}
	
	// Unlock the host so the callback can be made safely.
	RSSpinLockUnlock(&host->_lock);
	
	// If there is a callback, inform the client of the finish.
	if (cb)
		cb((RSHostRef)host, RSHostReachability, &error, info);
	
	// Go ahead and release now that the callback is done.
	RSRelease((RSHostRef)host);
}


/* static */ void
_DNSCallBack(int32_t status, char *buf, uint32_t len, struct sockaddr *from, int fromlen, void *context) {
	
	_RSHost* host = (_RSHost*)context;
	RSHostClientCallBack cb = NULL;
	RSStreamError error;
	void* info = NULL;
	RSHostInfoType type = _RSNullHostInfoType;
	
	// Retain here to guarantee safety really after the lookups release,
	// but definitely before the callback.
	RSRetain((RSHostRef)context);
	
	// Lock the host
	RSSpinLockLock(&host->_lock);
	
	// If the lookup canceled, don't need to do any of this.
	if (host->_lookup) {
		
		// Make sure to toss the cached info now.
		RSDictionaryRemoveValue(host->_info, (const void*)(host->_type));
		
		// Set the error if got one back from the lookup
		if (status) {
			
			/* If it's a system error, get the real error. */
			if (EAI_SYSTEM == status) {
				host->_error.error = errno;
				host->_error.domain = RSStreamErrorDomainPOSIX;
			}
			
			else {
				host->_error.error = status;
				host->_error.domain = (RSStreamErrorDomain)RSStreamErrorDomainNetDB;
			}
			
			// Mark to indicate the resolution was performed.
			RSDictionarySetValue(host->_info, (const void*)(host->_type), RSNull);
		}
		
		else {
			RSAllocatorRef allocator = RSGetAllocator((RSHostRef)context);
			
			// Wrap the reply and the source of the reply
			RSDataRef rr = RSDataCreate(allocator, (const UInt8*)buf, len);
			RSDataRef sa = RSDataCreate(allocator, (const UInt8*)from, fromlen);
			
			// If couldn't wrap, fail with no memory error.
			if (!rr || !sa) {
				host->_error.error = ENOMEM;
				host->_error.domain = RSStreamErrorDomainPOSIX;
			}
			
			else {
				
				// Create the information to put in the info dictionary.
				RSTypeRef list[2] = {rr, sa};
				RSArrayRef array = RSArrayCreate(allocator, list, sizeof(list) / sizeof(list[0]), &RSTypeArrayCallBacks);
				
				// Make sure it was created and add it.
				if (array) {
					RSDictionarySetValue(host->_info, (const void*)(host->_type), array);
					RSRelease(array);
				}
				
				// Did make the information list so fail with out of memory
				else {
					host->_error.error = ENOMEM;
					host->_error.domain = RSStreamErrorDomainPOSIX;
				}
			}
			
			// Release the reply if it was created.
			if (rr)
				RSRelease(rr);
			
			// Release the sockaddr wrapper if it was created
			if (sa)
				RSRelease(sa);
		}
		
		// Save the callback if there is one at this time.
		cb = host->_callback;
		
		// Save the type of lookup for the callback.
		type = host->_type;
		
		// Save the error and client information for the callback
		memmove(&error, &(host->_error), sizeof(error));
		info = host->_client.info;
		
		// Remove the lookup from run loops and modes
		_RSTypeUnscheduleFromMultipleRunLoops(host->_lookup, host->_schedules);
		
		// Go ahead and invalidate the lookup
		RSMachPortInvalidate((RSMachPortRef)(host->_lookup));
		
		// Release the lookup now.
		RSRelease(host->_lookup);
		host->_lookup = NULL;
		host->_type = _RSNullHostInfoType;
	}
	
	// Unlock the host so the callback can be made safely.
	RSSpinLockUnlock(&host->_lock);
	
	// If there is a callback, inform the client of the finish.
	if (cb)
		cb((RSHostRef)context, type, &error, info);
	
	// Go ahead and release now that the callback is done.
	RSRelease((RSHostRef)context);
}


/* static */ void
_DNSMachPortCallBack(RSMachPortRef port, void* msg, RSIndex size, void* info) {
	
	dns_async_handle_reply(msg);
}


/* static */ void
_MasterCallBack(RSHostRef theHost, RSHostInfoType typeInfo, const RSStreamError *error, RSStringRef name) {
	
	RSArrayRef list;
	
	/* Shut down the host lookup. */
	RSHostSetClient(theHost, NULL, NULL);
	
	/* Lock the host master list and cache */
	_RSMutexLock(_HostLock);
	
	/* Get the list of clients. */
	list = RSDictionaryGetValue(_HostLookups, name);
	
	if (list) {

		RSRetain(list);
	
		/* Remove the entry from the list of master lookups. */
		RSDictionaryRemoveValue(_HostLookups, name);
	}
	
	_RSMutexUnlock(_HostLock);	
	
	if (list) {
		
		RSIndex i, count;
		RSArrayRef addrs = RSHostGetInfo(theHost, _RSHostMasterAddressLookup, NULL);
		
		/* If no error, add the host to the cache. */
		if (!error->error) {
				
			/* The host will be saved for each name in the list of names for the host. */
			RSArrayRef names = RSHostGetInfo(theHost, RSHostNames, NULL);
			
			if (names && ((RSTypeRef)names != RSNull)) {
					
				/* Each host cache entry is a host with its fetch time. */
				RSTypeRef orig[2] = {theHost, RSDateCreate(RSAllocatorDefault, RSAbsoluteTimeGetCurrent())};
				
				/* Only add the entries if the date was created. */
				if (orig[1]) {
					
					/* Create the RSArray to be added into the cache. */
					RSArrayRef items = RSArrayCreate(RSAllocatorDefault, orig, sizeof(orig) / sizeof(orig[0]), &RSTypeArrayCallBacks);
					
					RSRelease(orig[1]);
					
					/* Once again, only add if the list was created. */
					if (items) {
						
						/* Loop through all the names of the host. */
						count = RSArrayGetCount(names);
						
						/* Add an entry for each name. */
						for (i = 0; i < count; i++)
							RSDictionarySetValue(_HostCache, RSArrayObjectAtIndex(names, i), items);
						
						RSRelease(items);
					}
				}
			}
		}
		
		count = RSArrayGetCount(list);
		
		for (i = 1; i < count; i++) {
			
			_RSHost* client;
			RSRunLoopSourceContext ctxt = {0};
			RSRunLoopSourceRef src = (RSRunLoopSourceRef)RSArrayObjectAtIndex(list, i);
			
			RSRunLoopSourceGetContext(src, &ctxt);
			client = (_RSHost*)ctxt.info;
			
			RSSpinLockLock(&client->_lock);
			
			/* Make sure to toss the cached info now. */
			RSDictionaryRemoveValue(client->_info, (const void*)(client->_type));
			
			/* Deal with the error if there was one. */
			if (error->error) {
				
				/* Copy the error over to the client. */
				memmove(&client->_error, error, sizeof(error[0]));
				
				/* Mark to indicate the resolution was performed. */
				RSDictionarySetValue(client->_info, (const void*)(client->_type), RSNull);
			}
			
			else {
				
				/* Make a copy of the addresses with the client's allocator. */
				RSArrayRef cp = _RSArrayCreateDeepCopy(RSGetAllocator((RSHostRef)client), addrs);
				
				if (cp) {
					
					RSDictionarySetValue(client->_info, (const void*)(client->_type), addrs);
				
					RSRelease(cp);
				}
				
				else {
					
					/* Make sure to error if couldn't create the list. */
					client->_error.error = ENOMEM;
					client->_error.domain = RSStreamErrorDomainPOSIX;
					
					/* Mark to indicate the resolution was performed. */
					RSDictionarySetValue(client->_info, (const void*)(client->_type), RSNull);
				}
			}
			
			/* Signal the client for immediate attention. */
			RSRunLoopSourceSignal((RSRunLoopSourceRef)(client->_lookup));
			
			RSArrayRef schedules = client->_schedules;
			RSIndex j, c = RSArrayGetCount(schedules);
			
			/* Make sure the signal can make it through */
			for (j = 0; j < c; j += 2) {
				
				/* Grab the run loop for checking */
				RSRunLoopRef runloop = (RSRunLoopRef)RSArrayObjectAtIndex(schedules, j);

				/* If it's sleeping, need to further check it. */
				if (RSRunLoopIsWaiting(runloop)) {
					
					/* Grab the mode for further check */
					RSStringRef mode = RSRunLoopCopyCurrentMode(runloop);
					
					if (mode) {
						
						/* If the lookup is in the right mode, need to wake up the run loop. */
						if (RSRunLoopContainsSource(runloop, (RSRunLoopSourceRef)(client->_lookup), mode)) {
							RSRunLoopWakeUp(runloop);
						}
						
						/* Don't need this anymore. */
						RSRelease(mode);
					}
				}
			}
			
			RSSpinLockUnlock(&client->_lock);
		}
		
		RSRelease(list);
	}
}


/* static */ void
_AddressLookupSchedule_NoLock(_RSHost* host, RSRunLoopRef rl, RSStringRef mode) {
	
	RSArrayRef list;
	RSArrayRef names = (RSArrayRef)RSDictionaryGetValue(host->_info, (const void*)RSHostNames);
	RSStringRef name = (RSStringRef)RSArrayObjectAtIndex(names, 0);

	/* Lock the list of master lookups and cache */
	_RSMutexLock(_HostLock);
	
	list = RSDictionaryGetValue(_HostLookups, name);

	if (list)
		RSHostScheduleWithRunLoop((RSHostRef)RSArrayObjectAtIndex(list, 0), rl, mode);

	_RSMutexUnlock(_HostLock);	
}


/* static */ void
_AddressLookupPerform(_RSHost* host) {
		
	RSHostClientCallBack cb = NULL;
	RSStreamError error;
	void* info = NULL;
	
	// Retain here to guarantee safety really after the lookups release,
	// but definitely before the callback.
	RSRetain((RSHostRef)host);
	
	// Lock the host
	RSSpinLockLock(&host->_lock);
	
	// Save the callback if there is one at this time.
	cb = host->_callback;
	
	// Save the error and client information for the callback
	memmove(&error, &(host->_error), sizeof(error));
	info = host->_client.info;
	
	// Remove the lookup from run loops and modes
	_RSTypeUnscheduleFromMultipleRunLoops(host->_lookup, host->_schedules);
	
	// Go ahead and invalidate the lookup
	RSRunLoopSourceInvalidate((RSRunLoopSourceRef)(host->_lookup));
	
	// Release the lookup now.
	RSRelease(host->_lookup);
	host->_lookup = NULL;
	host->_type = _RSNullHostInfoType;	
	
	// Unlock the host so the callback can be made safely.
	RSSpinLockUnlock(&host->_lock);
	
	// If there is a callback, inform the client of the finish.
	if (cb)
		cb((RSHostRef)host, RSHostAddresses, &error, info);
	
	// Go ahead and release now that the callback is done.
	RSRelease((RSHostRef)host);	
}


/* static */ void
_ExpireCacheEntries(void) {
	
	RSIndex count;
	
	RSStringRef keys_buffer[_RSHostCacheMaxEntries];
	RSArrayRef values_buffer[_RSHostCacheMaxEntries];
	
	RSStringRef* keys = &keys_buffer[0];
	RSArrayRef* values = &values_buffer[0];
	
	/* Lock the cache */
	_RSMutexLock(_HostLock);
	
	if (_HostCache) {
		
		/* Get the count for proper allocation if needed and for iteration. */
		count = RSDictionaryGetCount(_HostCache);
		
		/* Allocate buffers for keys and values if don't have large enough static buffers. */
		if (count > _RSHostCacheMaxEntries) {
			
			keys = (RSStringRef*)RSAllocatorAllocate(RSAllocatorDefault, sizeof(keys[0]) * count, 0);
			values = (RSArrayRef*)RSAllocatorAllocate(RSAllocatorDefault, sizeof(values[0]) * count, 0);
		}
		
		/* Only iterate if buffers were allocated. */
		if (keys && values) {
		
			RSIndex i, j = 0;
			RSTimeInterval oldest = 0.0;
			
			/* Get "now" for comparison for freshness. */
			RSDateRef now = RSDateCreate(RSAllocatorDefault, RSAbsoluteTimeGetCurrent());
			
			/* Get all the hosts in the cache */
			RSDictionaryGetKeysAndValues(_HostCache, (const void **)keys, (const void **)values);
			
			/* Iterate through and get rid of expired ones. */
			for (i = 0; i < count; i++) {
				
				/* How long since now?  Use abs in order to handle clock changes. */
				RSTimeInterval since = fabs(RSDateGetTimeIntervalSinceDate(now, (RSDateRef)RSArrayObjectAtIndex(values[i], 1)));
				
				/* If timeout, remove the entry. */
				if (since >= _RSHostCacheTimeout)
					RSDictionaryRemoveValue(_HostCache, keys[i]);
					
				/* If this one is older than the oldest, save it's index. */
				else if (since > oldest) {
					j = i;
					oldest = since;
				}
			}
			
			RSRelease(now);
			
			/* If the count still isn't in the bounds of maximum number of entries, remove the oldest. */
			if (RSDictionaryGetCount(_HostCache) >= _RSHostCacheMaxEntries)
				RSDictionaryRemoveValue(_HostCache, keys[j]);
		}
		
		/* If space for keys was made, deallocate it. */
		if (keys && (keys != &keys_buffer[0]))
			RSAllocatorDeallocate(RSAllocatorDefault, keys);
		
		/* If space for values was made, deallocate it. */
		if (values && (values != &values_buffer[0]))
			RSAllocatorDeallocate(RSAllocatorDefault, values);
	}
	
	_RSMutexUnlock(_HostLock);
}


/* static */ RSArrayRef
_RSArrayCreateDeepCopy(RSAllocatorRef alloc, RSArrayRef array) {
	
    RSArrayRef result = NULL;
    RSIndex i, c = RSArrayGetCount(array);
    RSTypeRef *values;
    if (c == 0) {
        result = RSArrayCreate(alloc, NULL, 0, &RSTypeArrayCallBacks);
    } else if ((values = (RSTypeRef*)RSAllocatorAllocate(alloc, c*sizeof(RSTypeRef), 0)) != NULL) {
        RSArrayGetValues(array, RSRangeMake(0, c), values);
        if (RSGetTypeID(values[0]) == RSStringGetTypeID()) {
            for (i = 0; i < c; i ++) {
                values[i] = RSStringCreateCopy(alloc, (RSStringRef)values[i]);
                if (values[i] == NULL) {
                    break;
                }
            }
        }
        else if (RSGetTypeID(values[0]) == RSDataGetTypeID()) {
            for (i = 0; i < c; i ++) {
                values[i] = RSDataCreateCopy(alloc, (RSDataRef)values[i]);
                if (values[i] == NULL) {
                    break;
                }
            }
        }
        else {
            for (i = 0; i < c; i ++) {
                values[i] = RSPropertyListCreateDeepCopy(alloc, values[i], RSPropertyListImmutable);
                if (values[i] == NULL) {
                    break;
                }
            }
        }
        
        result = (i == c) ? RSArrayCreate(alloc, values, c, &RSTypeArrayCallBacks) : NULL;
        c = i;
        for (i = 0; i < c; i ++) {
            RSRelease(values[i]);
        }
        RSAllocatorDeallocate(alloc, values);
    }
    return result;
}


/* static */ Boolean
_IsDottedIp(RSStringRef name) {
	
	Boolean result = FALSE;
	UInt8 stack_buffer[1024];
	UInt8* buffer = stack_buffer;
	RSIndex length = sizeof(stack_buffer);
	RSAllocatorRef alloc = RSGetAllocator(name);

	buffer = _RSStringGetOrCreateCString(alloc, name, buffer, &length, RSStringEncodingASCII);

	if (buffer) {
	
		struct addrinfo hints;
		struct addrinfo* results = NULL;
		
		memset(&hints, 0, sizeof(hints));
		hints.ai_flags = AI_NUMERICHOST;
		
		if (!getaddrinfo((const char*)buffer, NULL, &hints, &results)) {
			
			if (results) {
			
				if (results->ai_addr)
					result = TRUE;

				freeaddrinfo(results);
			}
		}
	}
	
	if (buffer != stack_buffer)
		RSAllocatorDeallocate(alloc, buffer);
	
	return result;
}


#if 0
#pragma mark -
#pragma mark Extern Function Definitions (API)
#endif

/* extern */ RSTypeID
RSHostGetTypeID(void) {

    _RSDoOnce(&_RSHostRegisterClass, _RSHostRegisterClass);

    return _RSHostTypeID;
}


/* extern */ RSHostRef
RSHostCreateWithName(RSAllocatorRef allocator, RSStringRef hostname) {

	// Create the base object
	_RSHost* result = _HostCreate(allocator);

	// Set the names only if succeeded
	if (result) {
		
		// Create the list of names
		RSArrayRef names = RSArrayCreate(allocator, (const void**)(&hostname), 1, &RSTypeArrayCallBacks);

		// Add the list to the info if it succeeded
		if (names) {
			RSDictionarySetValue(result->_info, (const void*)RSHostNames, names);
			RSRelease(names);
		}

		// Failed so release the new host and return null
		else {
			RSRelease((RSTypeRef)result);
			result = NULL;
		}
	}

	return (RSHostRef)result;
}


/* extern */ RSHostRef
RSHostCreateWithAddress(RSAllocatorRef allocator, RSDataRef addr) {

	// Create the base object
	_RSHost* result = _HostCreate(allocator);

	// Set the names only if succeeded
	if (result) {

		// Create the list of addresses
		RSArrayRef addrs = RSArrayCreate(allocator, (const void**)(&addr), 1, &RSTypeArrayCallBacks);

		// Add the list to the info if it succeeded
		if (addrs) {
			RSDictionarySetValue(result->_info, (const void*)RSHostAddresses, addrs);
			RSRelease(addrs);
		}

		// Failed so release the new host and return null
		else {
			RSRelease((RSTypeRef)result);
			result = NULL;
		}
	}

	return (RSHostRef)result;
}


/* extern */ RSHostRef
RSHostCreateCopy(RSAllocatorRef allocator, RSHostRef h) {

	_RSHost* host = (_RSHost*)h;

	// Create the base object
	_RSHost* result = _HostCreate(allocator);

	// Set the names only if succeeded
	if (result) {

		// Release the current, because a new one will be laid down
		RSRelease(result->_info);

		// Lock original before going to town on it
		RSSpinLockLock(&(host->_lock));

		// Just make a copy of all the information
		result->_info = RSDictionaryCreateMutableCopy(allocator, 0, host->_info);

		// Let the original go
		RSSpinLockUnlock(&(host->_lock));

		// If it failed, release the new host and return null
		if (!result->_info) {
			RSRelease((RSTypeRef)result);
			result = NULL;
		}
	}

	return (RSHostRef)result;
}


/* extern */ Boolean
RSHostStartInfoResolution(RSHostRef theHost, RSHostInfoType info, RSStreamError* error) {

	_RSHost* host = (_RSHost*)theHost;
	RSStreamError extra;
	Boolean result = FALSE;

	if (!error)
		error = &extra;

	memset(error, 0, sizeof(error[0]));

	// Retain so it doesn't go away underneath in the case of a callout.  This is really
	// no worry for async, but makes the memmove for the error more difficult to place
	// for synchronous without it being here.
	RSRetain(theHost);
	
	// Lock down the host to grab the info
	RSSpinLockLock(&(host->_lock));

	do {
		
		Boolean wakeup = FALSE;
		
		// Create lookup.  Bail if it fails.
		if (!_CreateLookup_NoLock(host, info, &wakeup))
			break;

		// Async mode is complete at this point
		if (host->_callback) {
			
			// Schedule the lookup on the run loops and modes.
			_RSTypeScheduleOnMultipleRunLoops(host->_lookup, host->_schedules);
			
			// 4012176 If the source was signaled, wake up the run loop.
			if (wakeup) {
				
				RSArrayRef schedules = host->_schedules;
				RSIndex i, count = RSArrayGetCount(schedules);

				// Make sure the signal can make it through
				for (i = 0; i < count; i += 2) {
					
					// Wake up run loop
					RSRunLoopWakeUp((RSRunLoopRef)RSArrayObjectAtIndex(schedules, i));
				}
			}
			
			// It's now succeeded.
			result = TRUE;
		}

		// If there is no callback, go into synchronous mode.
		else {
			
			// Unlock the host
			RSSpinLockUnlock(&(host->_lock));

			// Wait for synchronous return
			result = _HostBlockUntilComplete(host);
			
			// Lock down the host to grab the info
			RSSpinLockLock(&(host->_lock));
		}
		
	} while (0);

	// Copy the error.
	memmove(error, &host->_error, sizeof(error[0]));

	// Unlock the host
	RSSpinLockUnlock(&(host->_lock));

	// Release the earlier retain.
	RSRelease(theHost);
	
	return result;
}


/* extern */ RSTypeRef
RSHostGetInfo(RSHostRef theHost, RSHostInfoType info, Boolean* hasBeenResolved) {

	_RSHost* host = (_RSHost*)theHost;
	Boolean extra;
	RSTypeRef result = NULL;

	// Just make sure there is something to dereference.
	if (!hasBeenResolved)
		hasBeenResolved = &extra;

	// By default, it hasn't been resolved.
	*hasBeenResolved = FALSE;

	// Lock down the host to grab the info
	RSSpinLockLock(&(host->_lock));

	// Grab the requested information
	result = (RSTypeRef)RSDictionaryGetValue(host->_info, (const void*)info);

	// If there was a result, mark it as being resolved.
	if (result) {

		// If it was NULL, that means resolution actually returned nothing.
		if (RSEqual(result, RSNull))
			result = NULL;

		// It's been resolved.
		*hasBeenResolved = TRUE;
	}

	// Unlock the host
	RSSpinLockUnlock(&(host->_lock));

	return result;
}


/* extern */ RSArrayRef
RSHostGetAddressing(RSHostRef theHost, Boolean* hasBeenResolved) {

	return (RSArrayRef)RSHostGetInfo(theHost, RSHostAddresses, hasBeenResolved);
}


/* extern */ RSArrayRef
RSHostGetNames(RSHostRef theHost, Boolean* hasBeenResolved) {

	return (RSArrayRef)RSHostGetInfo(theHost, RSHostNames, hasBeenResolved);
}


#if defined(__MACH__)
/* extern */ RSDataRef
RSHostGetReachability(RSHostRef theHost, Boolean* hasBeenResolved) {

	return (RSDataRef)RSHostGetInfo(theHost, RSHostReachability, hasBeenResolved);
}
#endif


/* extern */ void
RSHostCancelInfoResolution(RSHostRef theHost, RSHostInfoType info) {
	
	_RSHost* host = (_RSHost*)theHost;
	
	// Lock down the host
	RSSpinLockLock(&(host->_lock));
	
	// Make sure there is something to cancel.
	if (host->_lookup) {
		
		RSRunLoopSourceContext ctxt = {
			0,								// version
			NULL,							// info
			NULL,							// retain
			NULL,							// release
			NULL,							// copyDescription
			NULL,							// equal
			NULL,							// hash
			NULL,							// schedule
			NULL,							// cancel
			(void(*)(void*))(&_HostCancel)  // perform
		};

		// Remove the lookup from run loops and modes
		_RSTypeUnscheduleFromMultipleRunLoops(host->_lookup, host->_schedules);
		
		// Go ahead and invalidate the lookup
		_RSTypeInvalidate(host->_lookup);
		
		// Pull the lookup out of the list in the master list.
		if (host->_type == RSHostAddresses) {
			
			RSMutableArrayRef list;
			RSArrayRef names = (RSArrayRef)RSDictionaryGetValue(host->_info, (const void*)RSHostNames);
			RSStringRef name = (RSStringRef)RSArrayObjectAtIndex(names, 0);
			
			/* Lock the master lookup list and cache */
			_RSMutexLock(_HostLock);
			
			/* Get the list of pending clients */
			list = (RSMutableArrayRef)RSDictionaryGetValue(_HostLookups, name);
			
			if (list) {
				
				/* Try to find this lookup in the list of clients. */
				RSIndex count = RSArrayGetCount(list);
				RSIndex idx = RSArrayGetFirstIndexOfValue(list, RSRangeMake(0, count), host->_lookup);
				
				if (idx != RSNotFound) {
					
					/* Remove this lookup. */
					RSArrayRemoveValueAtIndex(list, idx);
					
					/* If this was the last client, kill the lookup. */
					if (count == 2) {
						
						RSHostRef lookup = (RSHostRef)RSArrayObjectAtIndex(list, 0);
						
						/* NULL the client for the master lookup and cancel it. */
						RSHostSetClient(lookup, NULL, NULL);
						RSHostCancelInfoResolution(lookup, _RSHostMasterAddressLookup);
						
						/* Remove it from the list of pending lookups and clients. */
						RSDictionaryRemoveValue(_HostLookups, name);
					}
				}
			}
			
			_RSMutexUnlock(_HostLock);	
		}
		
		// Release the lookup now.
		RSRelease(host->_lookup);
		
		// Create the cancel source
		host->_lookup = RSRunLoopSourceCreate(RSGetAllocator(theHost), 0, &ctxt);
		
		// If the cancel was created, need to schedule and signal it.
		if (host->_lookup) {
			
			RSArrayRef schedules = host->_schedules;
			RSIndex i, count = RSArrayGetCount(schedules);

			// Schedule the new lookup
			_RSTypeScheduleOnMultipleRunLoops(host->_lookup, schedules);
			
			// Signal the cancel for immediate attention.
			RSRunLoopSourceSignal((RSRunLoopSourceRef)(host->_lookup));
			
			// Make sure the signal can make it through
			for (i = 0; i < count; i += 2) {
				
				// Grab the run loop for checking
				RSRunLoopRef runloop = (RSRunLoopRef)RSArrayObjectAtIndex(schedules, i);
				
				// If it's sleeping, need to further check it.
				if (RSRunLoopIsWaiting(runloop)) {
					
					// Grab the mode for further check
					RSStringRef mode = RSRunLoopCopyCurrentMode(runloop);
					
					if (mode) {
						
						// If the lookup is in the right mode, need to wake up the run loop.
						if (RSRunLoopContainsSource(runloop, (RSRunLoopSourceRef)(host->_lookup), mode)) {
							RSRunLoopWakeUp(runloop);
						}
						
						// Don't need this anymore.
						RSRelease(mode);
					}
				}
			}
		}
	}
	
	// Unlock the host
	RSSpinLockUnlock(&(host->_lock));
}


/* extern */ Boolean
RSHostSetClient(RSHostRef theHost, RSHostClientCallBack clientCB, RSHostClientContext* clientContext) {

	_RSHost* host = (_RSHost*)theHost;

	// Lock down the host
	RSSpinLockLock(&(host->_lock));

	// Release the user's context info if there is some and a release method
	if (host->_client.info && host->_client.release)
		host->_client.release(host->_client.info);
	
	// NULL callback or context signals to remove the client
	if (!clientCB || !clientContext) {
		
		// Cancel the outstanding lookup
		if (host->_lookup) {
			
			// Remove the lookup from run loops and modes
			_RSTypeUnscheduleFromMultipleRunLoops(host->_lookup, host->_schedules);
			
			// Go ahead and invalidate the lookup
			_RSTypeInvalidate(host->_lookup);
			
			// Pull the lookup out of the master lookups.
			if (host->_type == RSHostAddresses) {
				
				RSMutableArrayRef list;
				RSArrayRef names = (RSArrayRef)RSDictionaryGetValue(host->_info, (const void*)RSHostNames);
				RSStringRef name = (RSStringRef)RSArrayObjectAtIndex(names, 0);
				
				/* Lock the masters list and cache */
				_RSMutexLock(_HostLock);
				
				/* Get the list of pending clients */
				list = (RSMutableArrayRef)RSDictionaryGetValue(_HostLookups, name);
				
				if (list) {
					
					/* Try to find this lookup in the list of clients. */
					RSIndex count = RSArrayGetCount(list);
					RSIndex idx = RSArrayGetFirstIndexOfValue(list, RSRangeMake(0, count), host->_lookup);
					
					if (idx != RSNotFound) {
						
						/* Remove this lookup. */
						RSArrayRemoveValueAtIndex(list, idx);
						
						/* If this was the last client, kill the lookup. */
						if (count == 2) {
							
							RSHostRef lookup = (RSHostRef)RSArrayObjectAtIndex(list, 0);
							
							/* NULL the client for the master lookup and cancel it. */
							RSHostSetClient(lookup, NULL, NULL);
							RSHostCancelInfoResolution(lookup, _RSHostMasterAddressLookup);
							
							/* Remove it from the list of pending lookups and clients. */
							RSDictionaryRemoveValue(_HostLookups, name);
						}
					}
				}
				
				_RSMutexUnlock(_HostLock);	
			}
			
			// Release the lookup now.
			RSRelease(host->_lookup);
			host->_lookup = NULL;
			host->_type = _RSNullHostInfoType;
		}

		// Zero out the callback and client context.
		host->_callback = NULL;
		memset(&(host->_client), 0, sizeof(host->_client));
	}

	//
	else {
		
		// Schedule any lookup on the run loops and modes if it hasn't been scheduled
		// already.  If there had previously been a callback, the lookup will have
		// already been scheduled.
		if (!host->_callback && host->_lookup)
			_RSTypeScheduleOnMultipleRunLoops(host->_lookup, host->_schedules);
		
		// Save the client's new callback
		host->_callback = clientCB;

		// Copy the client's context
		memmove(&(host->_client), clientContext, sizeof(host->_client));

		// If there is user data and a retain method, call it.
		if (host->_client.info && host->_client.retain)
			host->_client.info = (void*)(host->_client.retain(host->_client.info));
	}
	
	// Unlock the host
	RSSpinLockUnlock(&(host->_lock));

	return TRUE;
}


/* extern */ void
RSHostScheduleWithRunLoop(RSHostRef theHost, RSRunLoopRef runLoop, RSStringRef runLoopMode) {

	_RSHost* host = (_RSHost*)theHost;
	
	/* Lock down the host before work */
	RSSpinLockLock(&(host->_lock));

	/* Try adding the schedule to the list.  If it's added, need to do more work. */
	if (_SchedulesAddRunLoopAndMode(host->_schedules, runLoop, runLoopMode)) {

		/* If there is a current lookup, need to schedule it. */
		if (host->_lookup) {
			_RSTypeScheduleOnRunLoop(host->_lookup, runLoop, runLoopMode);
		}
	}
	
	/* Unlock the host */
	RSSpinLockUnlock(&(host->_lock));
}


/* extern */ void
RSHostUnscheduleFromRunLoop(RSHostRef theHost, RSRunLoopRef runLoop, RSStringRef runLoopMode) {

	_RSHost* host = (_RSHost*)theHost;

	/* Lock down the host before work */
	RSSpinLockLock(&(host->_lock));

	/* Try to remove the schedule from the list.  If it is removed, need to do more. */
	if (_SchedulesRemoveRunLoopAndMode(host->_schedules, runLoop, runLoopMode)) {

		/* If there is a current lookup, need to unschedule it. */
		if (host->_lookup) {
			_RSTypeUnscheduleFromRunLoop(host->_lookup, runLoop, runLoopMode);			
		}
	}

	/* Unlock the host */
	RSSpinLockUnlock(&(host->_lock));
}

