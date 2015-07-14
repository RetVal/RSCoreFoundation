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

/*	RSMachPort.h
	Copyright (c) 1998-2013, Apple Inc. All rights reserved.
*/

#if !defined(__COREFOUNDATION_RSMACHPORT__)
#define __COREFOUNDATION_RSMACHPORT__ 1

#include <RSCoreFoundation/RSRunLoop.h>
#include <mach/port.h>

RS_EXTERN_C_BEGIN

typedef struct __RSMachPort * RSMachPortRef;

typedef struct {
    RSIndex	version;
    void *	info;
    const void *(*retain)(const void *info);
    void	(*release)(const void *info);
    RSStringRef	(*copyDescription)(const void *info);
} RSMachPortContext;

typedef void (*RSMachPortCallBack)(RSMachPortRef port, void *msg, RSIndex size, void *info);
typedef void (*RSMachPortInvalidationCallBack)(RSMachPortRef port, void *info);

RSExport RSTypeID	RSMachPortGetTypeID(void);

RSExport RSMachPortRef	RSMachPortCreate(RSAllocatorRef allocator, RSMachPortCallBack callout, RSMachPortContext *context, BOOL *shouldFreeInfo);
RSExport RSMachPortRef	RSMachPortCreateWithPort(RSAllocatorRef allocator, mach_port_t portNum, RSMachPortCallBack callout, RSMachPortContext *context, BOOL *shouldFreeInfo);

RSExport mach_port_t	RSMachPortGetPort(RSMachPortRef port);
RSExport void		RSMachPortGetContext(RSMachPortRef port, RSMachPortContext *context);
RSExport void		RSMachPortInvalidate(RSMachPortRef port);
RSExport BOOL	RSMachPortIsValid(RSMachPortRef port);
RSExport RSMachPortInvalidationCallBack RSMachPortGetInvalidationCallBack(RSMachPortRef port);
RSExport void		RSMachPortSetInvalidationCallBack(RSMachPortRef port, RSMachPortInvalidationCallBack callout);

RSExport RSRunLoopSourceRef	RSMachPortCreateRunLoopSource(RSAllocatorRef allocator, RSMachPortRef port, RSIndex order);

RS_EXTERN_C_END

#endif /* ! __COREFOUNDATION_RSMACHPORT__ */

