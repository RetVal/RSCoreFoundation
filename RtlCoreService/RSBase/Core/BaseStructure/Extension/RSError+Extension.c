//
//  RSError+Extension.c
//  RSCoreFoundation
//
//  Created by RetVal on 6/26/13.
//  Copyright (c) 2013 RetVal. All rights reserved.
//

#include <RSCoreFoundation/RSError+Extension.h>
#include <RSCoreFoundation/RSDictionary+Extension.h>

RSExport RSErrorRef RSErrorWithDomainCodeAndUserInfo(RSStringRef domain, RSErrorCode code, RSDictionaryRef userInfo)
{
    RSErrorRef error = RSErrorCreate(RSAllocatorSystemDefault, domain, code, userInfo);
    return RSAutorelease(error);
}

RSExport RSErrorRef RSErrorWithReason(RSStringRef reason)
{
    return RSErrorWithDomainCodeAndUserInfo(RSErrorDomainRSCoreFoundation, kErrUnknown, RSDictionaryWithObjectForKey(reason, RSErrorLocalizedFailureReasonKey));
}
