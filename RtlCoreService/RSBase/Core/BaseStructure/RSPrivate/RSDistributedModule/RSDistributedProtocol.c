//
//  RSDistributedProtocol.c
//  RSCoreFoundation
//
//  Created by RetVal on 3/17/13.
//  Copyright (c) 2013 RetVal. All rights reserved.
//

#include <RSCoreFoundation/RSPropertyList.h>
#include <RSCoreFoundation/RSRuntime.h>
#include "RSDistributedProtocol.h"


struct __RSDistributedModule
{
    RSRuntimeBase _base;
    RSUUIDRef   _clientID;
    RSSocketRef _client;
    RSTypeRef   _token;
    
    RSQueueRef  _requestSendQueue;
    RSQueueRef  _requestRecvQueue;
    
    RSQueueRef  _dataSendQueue;
    RSQueueRef  _dataRecvQueue;
    
    RSDistributedProtocol _protocol;
    RSDistributedNotifyCallback _callback;
};

static void __RSDistributedModuleClassInit(RSTypeRef rs)
{
    RSDistributedModuleRef dm = (RSDistributedModuleRef)rs;
    RSAllocatorRef allocator = RSGetAllocator(rs);
    dm->_clientID = RSUUIDCreate(allocator);
    dm->_requestSendQueue = RSQueueCreate(allocator, 0, RSQueueAtom);
    dm->_requestRecvQueue = RSQueueCreate(allocator, 0, RSQueueAtom);
    dm->_dataSendQueue = RSQueueCreate(allocator, 0, RSQueueAtom);
    dm->_dataRecvQueue = RSQueueCreate(allocator, 0, RSQueueAtom);
}

static RSTypeRef __RSDistributedModuleClassCopy(RSAllocatorRef allocator, RSTypeRef rs, BOOL mutalbeCopy)
{
    return nil;
}

static void __RSDistributedModuleClassDeallocate(RSTypeRef rs)
{
    RSDistributedModuleRef dm = (RSDistributedModuleRef)rs;
    dm->_callback.RSDistributedModuleWillDeallocate(dm);
    
    RSRelease(dm->_requestSendQueue);
    RSRelease(dm->_requestRecvQueue);
    RSRelease(dm->_dataSendQueue);
    RSRelease(dm->_dataRecvQueue);
    
    RSRelease(dm->_token);
    RSRelease(dm->_clientID);
    RSRelease(dm->_client);
}

static RSStringRef __RSDistributedModuleClassDescription(RSTypeRef rs)
{
    return nil;
}

static RSRuntimeClass __RSDistributedModuleClass =
{
    _RSRuntimeScannedObject,
    "__RSDistributedModule",
    0,
    __RSDistributedModuleClassInit,
    __RSDistributedModuleClassCopy,
    __RSDistributedModuleClassDeallocate,
    nil,
    nil,
    __RSDistributedModuleClassDescription,
    nil,
    nil
};

static RSTypeID __RSDistributedModuleTypeID = _RSRuntimeNotATypeID;

RSExport RSTypeID _RSDistributedModuleGetTypeID()
{
    return __RSDistributedModuleTypeID;
}

RSPrivate void __RSDistributedModuleInitialize()
{
    __RSDistributedModuleTypeID = __RSRuntimeRegisterClass(&__RSDistributedModuleClass);
    __RSRuntimeSetClassTypeID(&__RSDistributedModuleClass, __RSDistributedModuleTypeID);
}

static BOOL __RSDistributedProtocolCheckAvailable(const RSDistributedProtocol* protocol)
{
    if (protocol == nil) return NO;
    if (protocol->version != 0 ||
        protocol->signature.address == nil)
        return NO;
    if (protocol->RSDistributedProtocolBuildRequest &&
        protocol->RSDistributedProtocolInit &&
        protocol->RSDistributedProtocolRecv &&
        protocol->RSDistributedProtocolSend &&
        protocol->RSDistributedProtocolRecvRequest &&
        protocol->RSDistributedProtocolSendRequest)
        return YES;
    return NO;
}

static BOOL __RSDistributedNotifyCallbackCheckAvailable(const RSDistributedNotifyCallback* callback)
{
    if (callback == nil) return NO;
    if (callback->version != 0) return NO;
    if (callback->RSDistributedDataRecvCallback &&
        callback->RSDistributedDataSendCallback &&
        callback->RSDistributedRequestRecvCallback &&
        callback->RSDistributedRequestSendCallback &&
        callback->RSDistributedModuleWillDeallocate)
        return YES;
    return NO;
}

static void __RSDistributedModuleSocketCallBackDispatcher(RSSocketRef s, RSSocketCallBackType type, RSDataRef address, const void *data, void *info);

static RSErrorCode __RSDistributedProtocolInit(RSDistributedModuleRef distributed, RSSocketSignature* signature)
{
    const RSSocketContext context = {0, (RSMutableTypeRef)distributed, RSRetain, RSRelease, RSDescription};
    // do not worry, this operation is safe, do not active the cycle retain bug.
    // because of socket will do nothing with context even itself will be deallocated.
    distributed->_client = RSSocketCreateWithSocketSignature(RSGetAllocator(distributed), &distributed->_protocol.signature, distributed->_protocol.type, __RSDistributedModuleSocketCallBackDispatcher, &context);
    if (distributed->_client != nil) return kSuccess;
    return kErrInit;
}

static RSErrorCode __RSDistributedProtocolSendRequest(RSDistributedModuleRef distributed, RSDistributedRequestRef request)
{
    RSErrorCode errorCode = kSuccess;
    RSPropertyListRef plist = RSPropertyListCreateWithContent(RSGetAllocator(request), request);
    RSDataRef data = RSPropertyListGetData(plist, nil);
    if (data)
        errorCode = RSSocketSendData(distributed->_client, nil, data, 0.0f);
    else
        errorCode = kErrWrite;
    return errorCode;
}

static void __RSDistributedModuleSocketCallBackDispatcher(RSSocketRef s,
                                                          RSSocketCallBackType type,
                                                          RSDataRef address,
                                                          const void *data,
                                                          void *info) RS_AVAILABLE(0_0)
{
    RSDistributedModuleRef dm = (RSDistributedModuleRef)info;
    
    if (type & RSSocketAcceptCallBack ||
        type & RSSocketConnectCallBack)
    {
        RSDistributedRequestRef request;
        RSErrorCode errorCode = kSuccess;
        dm->_protocol.RSDistributedProtocolBuildRequest(dm, &request, RSDistributedRequest, nil);
        if (request)
        {
            if (dm->_protocol.RSDistributedProtocolSendRequest)
                errorCode = dm->_protocol.RSDistributedProtocolSendRequest(dm, request);
            else
                errorCode = __RSDistributedProtocolSendRequest(dm, request);
            RSRelease(request);
        }
    }
    else if (type & RSSocketDataCallBack)
    {
        
    }
}


RSExport RSDistributedModuleRef _RSDistributedModuleCreate(RSAllocatorRef allocator, const RSDistributedProtocol* protocol, const RSDistributedNotifyCallback* callback)
{
    if (__RSDistributedProtocolCheckAvailable(protocol) == NO ||
        __RSDistributedNotifyCallbackCheckAvailable(callback) == NO)
        return nil;
    RSDistributedModuleRef dm = (RSDistributedModuleRef)__RSRuntimeCreateInstance(allocator, __RSDistributedModuleTypeID, sizeof(struct __RSDistributedModule) - sizeof(RSRuntimeBase));
    memcpy(&dm->_protocol, protocol, sizeof(RSDistributedProtocol));
    memcpy(&dm->_callback, callback, sizeof(RSDistributedNotifyCallback));
    // call the base protocol init.
    
    RSErrorCode errorCode = kSuccess;
    errorCode = __RSDistributedProtocolInit(dm, &dm->_protocol.signature);
    
    if (!errorCode)
    {
        // call the user protocol init
        errorCode = dm->_protocol.RSDistributedProtocolInit(dm, &dm->_protocol.signature);
    }
    return dm;
}

RSExport RSDataRef _RSDistributedGetDataFromRecvQueue(RSDistributedModuleRef distributed)
{
    __RSGenericValidInstance(distributed, __RSDistributedModuleTypeID);
    return RSQueueDequeue(distributed->_dataRecvQueue);
}

RSExport RSDistributedRequestRef _RSDistributedGetRequestFromRecvQueue(RSDistributedModuleRef distributed)
{
    __RSGenericValidInstance(distributed, __RSDistributedModuleTypeID);
    return RSQueueDequeue(distributed->_requestRecvQueue);
}

RS_PUBLIC_CONST_STRING_DECL(RSDistributedConnectRequest, "RSConnectRequest")
RS_PUBLIC_CONST_STRING_DECL(RSDistributedAcceptRequest, "RSAcceptRequest")



RS_PUBLIC_CONST_STRING_DECL(RSDistributedRequest, "RSRequest")
RS_PUBLIC_CONST_STRING_DECL(RSDistributedRequestReason, "RSRequestReason")
RS_PUBLIC_CONST_STRING_DECL(RSDistributedRequestResult, "RSRequestResult")
RS_PUBLIC_CONST_STRING_DECL(RSDistributedRequestDomain, "RSRequestDomain")
RS_PUBLIC_CONST_STRING_DECL(RSDistributedRequestPayload, "RSRequestPayload")

RS_PUBLIC_CONST_STRING_DECL(RSDistributedDataPacket, "RSDataPacket")

