//
//  RSHTTPCookieStorage.h
//  RSCoreFoundation
//
//  Created by closure on 12/17/13.
//  Copyright (c) 2013 RetVal. All rights reserved.
//

#ifndef RSCoreFoundation_RSHTTPCookieStorage
#define RSCoreFoundation_RSHTTPCookieStorage

#include <RSCoreFoundation/RSArray.h>
#include <RSCoreFoundation/RSDictionary.h>
#include <RSCoreFoundation/RSString.h>
#include <RSCoreFoundation/RSHTTPCookie.h>

RS_EXTERN_C_BEGIN

typedef struct __RSHTTPCookieStorage *RSHTTPCookieStorageRef;

RSExport RSTypeID RSHTTPCookieStorageGetTypeID();

RSExport RSHTTPCookieStorageRef RSHTTPCookieStorageGetSharedStorage();

RSExport void RSHTTPCookieStorageSetCookie(RSHTTPCookieStorageRef storage, RSHTTPCookieRef cookie);
RSExport void RSHTTPCookieStorageRemoveCookie(RSHTTPCookieStorageRef storage, RSHTTPCookieRef cookie);
RS_EXTERN_C_END
#endif 
