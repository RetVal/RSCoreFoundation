//
//  RSDistributedProtocol.h
//  RSCoreFoundation
//
//  Created by RetVal on 3/17/13.
//  Copyright (c) 2013 RetVal. All rights reserved.
//

#ifndef RSCoreFoundation_RSDistributedProtocol_h
#define RSCoreFoundation_RSDistributedProtocol_h

#include <RSCoreFoundation/RSArray.h>
#include <RSCoreFoundation/RSData.h>
#include <RSCoreFoundation/RSDate.h>
#include <RSCoreFoundation/RSDictionary.h>
#include <RSCoreFoundation/RSError.h>
#include <RSCoreFoundation/RSQueue.h>
#include <RSCoreFoundation/RSString.h>
#include <RSCoreFoundation/RSUUID.h>
#include <RSCoreFoundation/RSSocket.h>

typedef struct __RSDistributedModule* RSDistributedModuleRef;
typedef RSDictionaryRef RSDistributedRequestRef;
typedef struct RSDistributedProtocol
{
    RSIndex version;
    
    RSTypeRef userInfomation;
    RSSocketCallBackType type;
    RSSocketSignature signature;
    
    RSErrorCode (*RSDistributedProtocolInit)(RSDistributedModuleRef distributed, RSSocketSignature* signature);
    RSErrorCode (*RSDistributedProtocolSendRequest)(RSDistributedModuleRef distributed, RSDistributedRequestRef request);
    RSErrorCode (*RSDistributedProtocolRecvRequest)(RSDistributedModuleRef distributed);
    RSErrorCode (*RSDistributedProtocolBuildRequest)(RSDistributedModuleRef distribtued, RSDistributedRequestRef *request, RSStringRef key, ISA context);
    RSErrorCode (*RSDistributedProtocolSend)(RSDistributedModuleRef distributed, RSStringRef key, RSTypeRef request);
    RSErrorCode (*RSDistributedProtocolRecv)(RSDistributedModuleRef distributed, RSStringRef key);
}RSDistributedProtocol;

typedef struct RSDistributedNotifyCallback
{
    RSIndex version;
    RSErrorCode (*RSDistributedDataRecvCallback)(RSDistributedModuleRef distributed);
    RSErrorCode (*RSDistributedDataSendCallback)(RSDistributedModuleRef distributed, RSDataRef alreadySendedData);
    RSErrorCode (*RSDistributedRequestRecvCallback)(RSDistributedModuleRef distributed, RSUUIDRef requestUUID);
    RSErrorCode (*RSDistributedRequestSendCallback)(RSDistributedModuleRef distributed, RSUUIDRef requestUUID);
    RSErrorCode (*RSDistributedModuleWillDeallocate)(RSDistributedModuleRef distributed);
}RSDistributedNotifyCallback;

RSExport RSDistributedModuleRef _RSDistributedModuleCreate(RSAllocatorRef allocator, const RSDistributedProtocol* protocol, const RSDistributedNotifyCallback* callback);
RSExport RSDataRef _RSDistributedGetDataFromRecvQueue(RSDistributedModuleRef distributed);
RSExport RSDistributedRequestRef _RSDistributedGetRequestFromRecvQueue(RSDistributedModuleRef distributed);

RSExport RSStringRef const RSDistributedRequest RS_AVAILABLE(0_3);
RSExport RSStringRef const RSDistributedRequestReason RS_AVAILABLE(0_3);
RSExport RSStringRef const RSDistributedRequestResult RS_AVAILABLE(0_3);
RSExport RSStringRef const RSDistributedRequestDomain RS_AVAILABLE(0_3);
RSExport RSStringRef const RSDistributedRequestPayload RS_AVAILABLE(0_3);

RSExport RSStringRef const RSDistributedDataPacket RS_AVAILABLE(0_3);
#endif
