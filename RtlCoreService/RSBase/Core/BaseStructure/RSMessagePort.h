//
//  RSMessagePort.h
//  RSKit
//
//  Created by RetVal on 12/15/12.
//  Copyright (c) 2012 RetVal. All rights reserved.
//

#ifndef RSKit_RSMessagePort_h
#define RSKit_RSMessagePort_h
#include <mach/mach.h>
#include <RSKit/RSRunLoop.h>
#include <RSKit/RSData.h>
typedef  struct __RSMessagePort* RSMessagePortRef RS_AVAILABLE(0_1);
typedef struct {
    RSIndex	version;
    void *	info;
    const void *(*retain)(const void *info);
    void 	(*release)(const void *info);
    RSStringRef	(*copyDescription)(const void *info);
} RSMessagePortContext;
typedef RSDataRef (*RSMessagePortCallBack)(RSMessagePortRef local, SInt32 msgid, RSDataRef data, void *info) RS_AVAILABLE(0_1);
RSExport RSTypeID RSMessagePortTypeID() RS_AVAILABLE(0_1);

RSExport mach_port_t RSMessagePortGetMachPort(RSMessagePortRef port) RS_AVAILABLE(0_1);

RSExport BOOL RSMessagePortIsValid(RSMessagePortRef port) RS_AVAILABLE(0_1);
RSExport void RSMessagePortInvalid(RSMessagePortRef port) RS_AVAILABLE(0_1);
#endif
