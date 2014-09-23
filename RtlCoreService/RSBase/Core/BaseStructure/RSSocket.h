//
//  RSSocket.h
//  RSCoreFoundation
//
//  Created by RetVal on 11/18/12.
//  Copyright (c) 2012 RetVal. All rights reserved.
//

#ifndef RSCoreFoundation_RSSocket_h
#define RSCoreFoundation_RSSocket_h
#include <RSCoreFoundation/RSData.h>
#include <RSCoreFoundation/RSString.h>
#include <RSCoreFoundation/RSRunLoop.h>

RS_EXTERN_C_BEGIN
typedef struct __RSSocket* RSSocketRef RS_AVAILABLE(0_0);

typedef RS_ENUM(RSIndex, RSSocketError) {
    RSSocketSuccess = 0,
    RSSocketUnSuccess = -1L,
    RSSocketTimeout = -2L,
} RS_AVAILABLE(0_0);

typedef struct {
    RSBit32	protocolFamily;
    RSBit32	socketType;
    RSBit32	protocol;
    RSDataRef	address;
} RSSocketSignature RS_AVAILABLE(0_0);

typedef struct __RSSocketContext {
    RSIndex version;
    void* context;
    const void* (*retain)(const void*);
    void (*release)(const void*);
    RSStringRef (*description)(const void*);
}RSSocketContext RS_AVAILABLE(0_0);


typedef RS_OPTIONS(RSOptionFlags, RSSocketCallBackType)
{
    RSSocketNoCallBack = 0,
    RSSocketReadCallBack = 1,
    RSSocketAcceptCallBack = 2,
    RSSocketDataCallBack = 3,
    RSSocketConnectCallBack = 4,
    RSSocketWriteCallBack = 8
} RS_AVAILABLE(0_0);
    
enum {
    RSSocketAutomaticallyReenableReadCallBack = 1,
    RSSocketAutomaticallyReenableAcceptCallBack = 2,
    RSSocketAutomaticallyReenableDataCallBack = 3,
    RSSocketAutomaticallyReenableWriteCallBack = 8,
    RSSocketLeaveErrors = 64,
    RSSocketCloseOnInvalidate = 128
};

typedef int RSSocketHandle RS_AVAILABLE(0_0);


typedef void (*RSSocketCallBack)(RSSocketRef s, RSSocketCallBackType type, RSDataRef address, const void *data, void *info) RS_AVAILABLE(0_0);

RSExport RSStringRef RSSocketGetLocalIp() RS_AVAILABLE_BUT_DEPRECATED(0_0, 0_4);
RSExport RSStringRef RSSocketGetLocalName() RS_AVAILABLE_BUT_DEPRECATED(0_0, 0_4);

RSExport RSTypeID RSSocketGetTypeID() RS_AVAILABLE(0_0);

RSExport RSSocketRef RSSocketCreate(RSAllocatorRef allocator, RSBit32 protocolFamily, RSBit32 socketType, RSBit32 protocol,RSSocketCallBackType type, RSSocketCallBack callback, const RSSocketContext *context) RS_AVAILABLE(0_0);

RSExport RSSocketRef RSSocketCreateWithHandle(RSAllocatorRef allocator, RSSocketHandle handle, RSSocketCallBackType type, RSSocketCallBack callback, const RSSocketContext *context) RS_AVAILABLE(0_0);

RSExport RSSocketRef RSSocketCreateWithSocketSignature(RSAllocatorRef allocator, const RSSocketSignature* signature, RSSocketCallBackType type, RSSocketCallBack callback, const RSSocketContext *context) RS_AVAILABLE(0_0);

RSExport RSSocketRef RSSocketCreateCopy(RSAllocatorRef allocator, RSSocketRef s) RS_AVAILABLE(0_0);

RSExport RSRunLoopSourceRef	RSSocketCreateRunLoopSource(RSAllocatorRef allocator, RSSocketRef s, RSIndex order) RS_AVAILABLE(0_0);

RSExport RSSocketError RSSocketSetAddress(RSSocketRef s, RSDataRef address) RS_AVAILABLE(0_0);
RSExport RSDataRef RSSocketCopyAddress(RSSocketRef s) RS_AVAILABLE(0_0);
RSExport RSDataRef RSSocketCopyPeerAddress(RSSocketRef s) RS_AVAILABLE(0_0);

RSExport RSSocketError RSSocketConnectToAddress(RSSocketRef s, RSDataRef address, RSTimeInterval timeout) RS_AVAILABLE(0_0);
RSExport RSSocketRef RSSocketWaitForConnect(RSSocketRef s) RS_AVAILABLE(0_0); // return a new RSSocketRef for client if success, s should be setted by RSSocketSetAddress.
RSExport void RSSocketReschedule(RSSocketRef s) RS_AVAILABLE(0_4);
RSExport void RSSocketInvalidate(RSSocketRef s) RS_AVAILABLE(0_0);

RSExport RSSocketHandle RSSocketGetHandle(RSSocketRef s) RS_AVAILABLE(0_0);
RSExport void RSSocketGetContext(RSSocketRef s, RSSocketContext* context) RS_AVAILABLE(0_0);

RSExport BOOL RSSocketIsValid(RSSocketRef s) RS_AVAILABLE(0_0);

RSExport RSSocketError RSSocketSendData(RSSocketRef s, RSDataRef address, RSDataRef data, RSTimeInterval timeout) RS_AVAILABLE(0_2);
RSExport RSSocketError RSSocketRecvData(RSSocketRef s, RSMutableDataRef data) RS_AVAILABLE(0_2);
    
RSExport RSSocketError RSSocketUnregister(const RSSocketSignature *nameServerSignature, RSTimeInterval timeout, RSStringRef name);
RS_EXTERN_C_END
#endif
