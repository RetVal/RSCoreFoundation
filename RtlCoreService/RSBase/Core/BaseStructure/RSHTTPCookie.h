//
//  RSHTTPCookie.h
//  RSCoreFoundation
//
//  Created by Closure on 12/14/13.
//  Copyright (c) 2013 RetVal. All rights reserved.
//

#ifndef RSCoreFoundation_RSHTTPCookie
#define RSCoreFoundation_RSHTTPCookie

#include <RSCoreFoundation/RSArray.h>
#include <RSCoreFoundation/RSDictionary.h>
#include <RSCoreFoundation/RSString.h>
#include <RSCoreFoundation/RSURL.h>

RS_EXTERN_C_BEGIN

typedef const struct __RSHTTPCookie *RSHTTPCookieRef;

RSExport RSTypeID RSHTTPCookieGetTypeID();

RSExport RSHTTPCookieRef RSHTTPCookieCreate(RSAllocatorRef allocator, RSDictionaryRef properties) RS_AVAILABLE(0_4);
RSExport RSDictionaryRef RSHTTPCookieGetProperties(RSHTTPCookieRef cookie) RS_AVAILABLE(0_4);
RSExport RSUInteger RSHTTPCookieGetVersion(RSHTTPCookieRef cookie) RS_AVAILABLE(0_4);
RSExport RSStringRef RSHTTPCookieGetName(RSHTTPCookieRef cookie) RS_AVAILABLE(0_4);
RSExport RSStringRef RSHTTPCookieGetValue(RSHTTPCookieRef cookie) RS_AVAILABLE(0_4);
RSExport RSDateRef  RSHTTPCookieGetExpiresDate(RSHTTPCookieRef cookie) RS_AVAILABLE(0_4);
RSExport RSTimeInterval RSHTTPCookieGetMaximumAge(RSHTTPCookieRef cookie) RS_AVAILABLE(0_4);
RSExport BOOL RSHTTPCookieIsSessionOnly(RSHTTPCookieRef cookie) RS_AVAILABLE(0_4);
RSExport RSStringRef RSHTTPCookieGetDomain(RSHTTPCookieRef cookie) RS_AVAILABLE(0_4);
RSExport RSStringRef RSHTTPCookieGetPath(RSHTTPCookieRef cookie) RS_AVAILABLE(0_4);
RSExport BOOL RSHTTPCookieIsSecure(RSHTTPCookieRef cookie) RS_AVAILABLE(0_4);
RSExport BOOL RSHTTPCookieIsHTTPOnly(RSHTTPCookieRef cookie) RS_AVAILABLE(0_4);
RSExport RSStringRef RSHTTPCookieGetComment(RSHTTPCookieRef cookie) RS_AVAILABLE(0_4);
RSExport RSURLRef RSHTTPCookieGetCommentURL(RSHTTPCookieRef cookie) RS_AVAILABLE(0_4);
RSExport RSArrayRef RSHTTPCookieGetPortList(RSHTTPCookieRef cookie) RS_AVAILABLE(0_4);

/*!
 @const RSHTTPCookieName
 @discussion Key for cookie name
 */
RSExport RSStringRef const RSHTTPCookieName;

/*!
 @const RSHTTPCookieValue
 @discussion Key for cookie value
 */
RSExport RSStringRef const RSHTTPCookieValue;

/*!
 @const RSHTTPCookieOriginURL
 @discussion Key for cookie origin URL
 */
RSExport RSStringRef const RSHTTPCookieOriginURL;

/*!
 @const RSHTTPCookieVersion
 @discussion Key for cookie version
 */
RSExport RSStringRef const RSHTTPCookieVersion;

/*!
 @const RSHTTPCookieDomain
 @discussion Key for cookie domain
 */
RSExport RSStringRef const RSHTTPCookieDomain;

/*!
 @const RSHTTPCookiePath
 @discussion Key for cookie path
 */
RSExport RSStringRef const RSHTTPCookiePath;

/*!
 @const RSHTTPCookieSecure
 @discussion Key for cookie secure flag
 */
RSExport RSStringRef const RSHTTPCookieSecure;

/*!
 @const RSHTTPCookieExpires
 @discussion Key for cookie expiration date
 */
RSExport RSStringRef const RSHTTPCookieExpires;

/*!
 @const RSHTTPCookieComment
 @discussion Key for cookie comment text
 */
RSExport RSStringRef const RSHTTPCookieComment;

/*!
 @const RSHTTPCookieCommentURL
 @discussion Key for cookie comment URL
 */
RSExport RSStringRef const RSHTTPCookieCommentURL;

/*!
 @const RSHTTPCookieDiscard
 @discussion Key for cookie discard (session-only) flag
 */
RSExport RSStringRef const RSHTTPCookieDiscard;

/*!
 @const RSHTTPCookieMaximumAge
 @discussion Key for cookie maximum age (an alternate way of specifying the expiration)
 */
RSExport RSStringRef const RSHTTPCookieMaximumAge;

/*!
 @const RSHTTPCookiePort
 @discussion Key for cookie ports
 */
RSExport RSStringRef const RSHTTPCookiePort;
RS_EXTERN_C_END
#endif
