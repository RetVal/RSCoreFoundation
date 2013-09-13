//
//  RSError+Extension.h
//  RSCoreFoundation
//
//  Created by RetVal on 6/26/13.
//  Copyright (c) 2013 RetVal. All rights reserved.
//

#ifndef RSCoreFoundation_RSError_Extension_h
#define RSCoreFoundation_RSError_Extension_h

#include <RSCoreFoundation/RSError.h>

RS_EXTERN_C_BEGIN

RSExport RSErrorRef RSErrorWithDomainCodeAndUserInfo(RSStringRef domain, RSErrorCode code, RSDictionaryRef userInfo) RS_AVAILABLE(0_4);
RSExport RSErrorRef RSErrorWithReason(RSStringRef reason) RS_AVAILABLE(0_4);


RS_EXTERN_C_END

#endif
