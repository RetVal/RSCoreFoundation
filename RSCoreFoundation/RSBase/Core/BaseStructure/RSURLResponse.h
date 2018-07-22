//
//  RSURLResponse.h
//  RSCoreFoundation
//
//  Created by Closure on 11/24/13.
//  Copyright (c) 2013 RetVal. All rights reserved.
//

#ifndef RSCoreFoundation_RSURLResponse
#define RSCoreFoundation_RSURLResponse

#include <RSCoreFoundation/RSBase.h>
#include <RSCoreFoundation/RSData.h>
#include <RSCoreFoundation/RSDictionary.h>
#include <RSCoreFoundation/RSURL.h>

RS_EXTERN_C_BEGIN

typedef struct __RSURLResponse *RSURLResponseRef;

RSExport RSTypeID RSURLResponseGetTypeID(void);

RSExport RSURLResponseRef RSURLResponseCreateWithData(RSAllocatorRef allocator, RSURLRef URL, RSDataRef data);
RSExport RSURLResponseRef RSURLResponseCreateWithInfo(RSAllocatorRef allocator, RSURLRef URL, RSInteger httpMarjorVersion, RSInteger httpMinorVersion, RSInteger httpStatusCode, RSDictionaryRef headerFields);

RSExport RSURLRef RSURLResponseGetURL(RSURLResponseRef response);
RSExport long long RSURLResponseGetExpectedContentLength(RSURLResponseRef response);
RSExport RSInteger RSURLResponseGetStatusCode(RSURLResponseRef response);
RSExport RSDictionaryRef RSURLResponseGetAllHeaderFields(RSURLResponseRef response);

RS_EXTERN_C_END
#endif 
