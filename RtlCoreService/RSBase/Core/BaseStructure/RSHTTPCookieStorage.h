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

RSExport RSTypeID RSHTTPCookieStorageGetTypeID() RS_AVAILABLE(0_4);

RSExport RSHTTPCookieStorageRef RSHTTPCookieStorageGetSharedStorage() RS_AVAILABLE(0_4);

RSExport RSArrayRef RSHTTPCookieStorageCopyCookiesForURL(RSHTTPCookieStorageRef storage, RSURLRef URL) RS_AVAILABLE(0_4);
RSExport void RSHTTPCookieStorageSetCookie(RSHTTPCookieStorageRef storage, RSHTTPCookieRef cookie) RS_AVAILABLE(0_4);
RSExport void RSHTTPCookieStorageRemoveCookie(RSHTTPCookieStorageRef storage, RSHTTPCookieRef cookie) RS_AVAILABLE(0_4);

RSExport void RSHTTPCookieStorageRemoveCookiesFromArray(RSHTTPCookieStorageRef stroage, RSArrayRef cookies) RS_AVAILABLE(0_4);
RSExport void RSHTTPCookieStorageRemoveAllCookies(RSHTTPCookieStorageRef stroage) RS_AVAILABLE(0_4);

RSExport RSArrayRef RSHTTPCookieStorageCopyCookies(RSHTTPCookieStorageRef storage) RS_AVAILABLE(0_4);
RS_EXTERN_C_END
#endif 
