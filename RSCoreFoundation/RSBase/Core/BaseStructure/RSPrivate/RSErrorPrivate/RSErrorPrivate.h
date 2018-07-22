//
//  RSErrorPrivate.h
//  RSCoreFoundation
//
//  Created by RetVal on 12/6/12.
//  Copyright (c) 2012 RetVal. All rights reserved.
//

#ifndef RSCoreFoundation_RSErrorPrivate_h
#define RSCoreFoundation_RSErrorPrivate_h

#include <RSCoreFoundation/RSBase.h>
#include <RSCoreFoundation/RSErrorCode.h>

struct __RSErrorPrivateFormatTable {
    RSCBuffer format;
    RSIndex argsCnt;
};

struct __RSErrorPrivateFormatTable __RSErrorDomainRSCoreFoundationGetCStringWithCode(RSIndex code);
#endif
