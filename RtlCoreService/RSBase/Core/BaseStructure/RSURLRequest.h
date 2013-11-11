//
//  RSURLRequest.h
//  RSCoreFoundation
//
//  Created by Closure on 10/29/13.
//  Copyright (c) 2013 RetVal. All rights reserved.
//

#ifndef RSCoreFoundation_RSURLRequest
#define RSCoreFoundation_RSURLRequest

#include <RSCoreFoundation/RSURL.h>

RS_EXTERN_C_BEGIN

typedef const struct __RSURLRequest *RSURLRequestRef;
typedef struct __RSURLRequest *RSMutableURLRequestRef;

RSExport RSTypeID RSURLRequestGetTypeID();

RSExport RSURLRequestRef RSURLRequestCreateWithURL(RSAllocatorRef allocator, RSURLRef url);
RSExport RSURLRequestRef RSURLRequestWithURL(RSURLRef url);

RSExport RSMutableURLRequestRef RSURLRequestCreateMutable(RSAllocatorRef allocator);

RSExport RSURLRef RSURLRequestGetURL(RSURLRequestRef urlRequest);
RSExport void RSURLRequestSetURL(RSMutableURLRequestRef urlRequest, RSURLRef url);

RSExport RSDictionaryRef RSURLRequestGetHeaderField(RSURLRequestRef urlRequest);

RSExport RSTypeRef RSURLRequestGetHeaderFieldValue(RSURLRequestRef urlRequest, RSStringRef key);
RSExport void RSURLRequestSetHeaderFieldValue(RSMutableURLRequestRef urlRequest, RSStringRef key, RSTypeRef value);

RSExport RSTimeInterval RSURLRequestGetTimeoutInterval(RSURLRequestRef urlRequest);
RSExport void RSURLRequestSetTimeoutInterval(RSMutableURLRequestRef urlRequest, RSTimeInterval timeoutInterval);

RSExport RSDataRef RSURLRequestGetHttpBody(RSURLRequestRef urlRequest);
RSExport void RSURLRequestSetHTTPBody(RSMutableURLRequestRef urlRequest, RSDataRef HTTPBody);
RS_EXTERN_C_END
#endif 
