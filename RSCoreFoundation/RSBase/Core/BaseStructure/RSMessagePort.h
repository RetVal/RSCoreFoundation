/*
 * Copyright (c) 2013 Apple Inc. All rights reserved.
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

/*	RSMessagePort.h
	Copyright (c) 1998-2013, Apple Inc. All rights reserved.
*/

#if !defined(__RSCOREFOUNDATION_RSMESSAGEPORT__)
#define __RSCOREFOUNDATION_RSMESSAGEPORT__ 1

#include <RSCoreFoundation/RSString.h>
#include <RSCoreFoundation/RSRunLoop.h>
#include <RSCoreFoundation/RSData.h>
#include <dispatch/dispatch.h>

RS_EXTERN_C_BEGIN

typedef struct __RSMessagePort * RSMessagePortRef;

enum {
    RSMessagePortSuccess = 0,
    RSMessagePortSendTimeout = -1,
    RSMessagePortReceiveTimeout = -2,
    RSMessagePortIsInvalid = -3,
    RSMessagePortTransportError = -4,
    RSMessagePortBecameInvalidError = -5
};

typedef struct {
    RSIndex	version;
    void *	info;
    const void *(*retain)(const void *info);
    void	(*release)(const void *info);
    RSStringRef	(*copyDescription)(const void *info);
} RSMessagePortContext;

typedef RSDataRef (*RSMessagePortCallBack)(RSMessagePortRef local, SInt32 msgid, RSDataRef data, void *info);
/* If callout wants to keep a hold of the data past the return of the callout, it must COPY the data. This includes the case where the data is given to some routine which _might_ keep a hold of it; System will release returned RSData. */
typedef void (*RSMessagePortInvalidationCallBack)(RSMessagePortRef ms, void *info);

RSExport RSTypeID	RSMessagePortGetTypeID(void);

RSExport RSMessagePortRef	RSMessagePortCreateLocal(RSAllocatorRef allocator, RSStringRef name, RSMessagePortCallBack callout, RSMessagePortContext *context, BOOL *shouldFreeInfo);
RSExport RSMessagePortRef	RSMessagePortCreateRemote(RSAllocatorRef allocator, RSStringRef name);

RSExport BOOL RSMessagePortIsRemote(RSMessagePortRef ms);
RSExport RSStringRef RSMessagePortGetName(RSMessagePortRef ms);
RSExport BOOL RSMessagePortSetName(RSMessagePortRef ms, RSStringRef newName);
RSExport void RSMessagePortGetContext(RSMessagePortRef ms, RSMessagePortContext *context);
RSExport void RSMessagePortInvalidate(RSMessagePortRef ms);
RSExport BOOL RSMessagePortIsValid(RSMessagePortRef ms);
RSExport RSMessagePortInvalidationCallBack RSMessagePortGetInvalidationCallBack(RSMessagePortRef ms);
RSExport void RSMessagePortSetInvalidationCallBack(RSMessagePortRef ms, RSMessagePortInvalidationCallBack callout);

/* NULL replyMode argument means no return value expected, dont wait for it */
RSExport SInt32	RSMessagePortSendRequest(RSMessagePortRef remote, SInt32 msgid, RSDataRef data, RSTimeInterval sendTimeout, RSTimeInterval rcvTimeout, RSStringRef replyMode, RSDataRef *returnData);

RSExport RSRunLoopSourceRef	RSMessagePortCreateRunLoopSource(RSAllocatorRef allocator, RSMessagePortRef local, RSIndex order);

RSExport void RSMessagePortSetDispatchQueue(RSMessagePortRef ms, dispatch_queue_t queue) RS_AVAILABLE(0_4);

RS_EXTERN_C_END

#endif /* ! __RSCOREFOUNDATION_RSMESSAGEPORT__ */

