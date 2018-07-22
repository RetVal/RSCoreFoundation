//
//  RSURLConnection.h
//  RSCoreFoundation
//
//  Created by Closure on 11/2/13.
//  Copyright (c) 2013 RetVal. All rights reserved.
//

#ifndef RSCoreFoundation_RSURLConnection
#define RSCoreFoundation_RSURLConnection

#include <RSCoreFoundation/RSBase.h>
#include <RSCoreFoundation/RSData.h>
#include <RSCoreFoundation/RSDictionary.h>
#include <RSCoreFoundation/RSURL.h>
#include <RSCoreFoundation/RSURLRequest.h>
#include <RSCoreFoundation/RSURLResponse.h>
#include <RSCoreFoundation/RSRunLoop.h>
#include <RSCoreFoundation/RSError.h>
#include <RSCoreFoundation/RSError+Extension.h>
RS_EXTERN_C_BEGIN

typedef struct __RSURLConnection *RSURLConnectionRef;
typedef void (*RSURLConnectionWillStartConnection)(RSURLConnectionRef connection, RSURLRequestRef willSendRequest, RSURLResponseRef redirectResponse);

typedef void (*RSURLConnectionDidReceiveResponse)(RSURLConnectionRef connection, RSURLResponseRef response);
typedef void (*RSURLConnectionDidReceiveData)(RSURLConnectionRef connection, RSDataRef data);
typedef void (*RSURLConnectionDidSendBodyData)(RSURLConnectionRef connection, RSDataRef data, RSUInteger totalBytesWritten, RSUInteger totalBytesExpectedToWrite);

typedef void (*RSURLConnectionDidFinishLoading)(RSURLConnectionRef connection);
typedef void (*RSURLConnectionDidFailWithError)(RSURLConnectionRef connection, RSErrorRef error);

struct RSURLConnectionDelegate {
    RSIndex version;
    RSURLConnectionWillStartConnection startConnection;
    
    RSURLConnectionDidReceiveResponse receiveResponse;
    RSURLConnectionDidReceiveData receiveData;
    RSURLConnectionDidSendBodyData sendData;
    
    RSURLConnectionDidFinishLoading finishLoading;
    RSURLConnectionDidFailWithError failWithError;
};

RSExport RSTypeID RSURLConnectionGetTypeID(void);

RSExport RSURLConnectionRef RSURLConnectionCreate(RSAllocatorRef allocator, RSURLRequestRef request, RSTypeRef context, const struct RSURLConnectionDelegate *delegate);

RSExport RSURLRequestRef RSURLConnectionGetOrignialRequest(RSURLConnectionRef connection);
RSExport RSURLRequestRef RSURLConnectionGetCurrentRequest(RSURLConnectionRef connection);

RSExport RSTypeRef RSURLConnectionGetContext(RSURLConnectionRef connection);
RSExport RSURLResponseRef RSURLConnectionGetResponse(RSURLConnectionRef connection);

RSExport void RSURLConnectionSetRequest(RSURLConnectionRef connection, RSURLRequestRef request);
RSExport void RSURLConnectionStartOperation(RSURLConnectionRef connection);

RSExport RSDataRef RSURLConnectionSendSynchronousRequest(RSURLRequestRef request, __autorelease RSURLResponseRef *response, __autorelease RSErrorRef *error);
RSExport void RSURLConnectionSendAsynchronousRequest(RSURLRequestRef request, RSRunLoopRef rl, void (^completeHandler)(__autorelease RSURLResponseRef response, __autorelease RSDataRef data, __autorelease RSErrorRef error));
RS_EXTERN_C_END
#endif 
