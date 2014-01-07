//
//  RSErrorCode.h
//  RSCoreFoundation
//
//  Created by RetVal on 12/5/12.
//  Copyright (c) 2012 RetVal. All rights reserved.
//

#ifndef RSCoreFoundation_RSError_h
#define RSCoreFoundation_RSError_h

#include <RSCoreFoundation/RSErrorCode.h>
#include <RSCoreFoundation/RSDictionary.h>

RS_EXTERN_C_BEGIN
typedef struct __RSError* RSErrorRef RS_AVAILABLE(0_0);

RSExport RSTypeID RSErrorGetTypeID() RS_AVAILABLE(0_0);

RSExport RSErrorRef RSErrorCreate(RSAllocatorRef allocator, RSStringRef domain, RSIndex code, RSDictionaryRef userInfo) RS_AVAILABLE(0_0);
RSExport RSErrorRef RSErrorCreateWithKeysAndValues(RSAllocatorRef allocator, RSStringRef domain, RSIndex code, const void** keys, const void** values, RSIndex userInfoValuesCount) RS_AVAILABLE(0_0);

RSExport RSIndex RSErrorGetCode(RSErrorRef err) RS_AVAILABLE(0_0);
RSExport RSStringRef RSErrorGetDomain(RSErrorRef err) RS_AVAILABLE(0_0);
RSExport RSDictionaryRef RSErrorCopyUserInfo(RSErrorRef err) RS_AVAILABLE(0_0);
RSExport RSStringRef RSErrorCopyDescription(RSErrorRef err) RS_AVAILABLE(0_0);

RSExport RSIndex RSErrorRSCoreFoundationDomainGetNeedTargetCount(RSErrorRef err) RS_AVAILABLE(0_0);

RSExport RSStringRef const RSErrorLocalizedDescriptionKey RS_AVAILABLE(0_0);
RSExport RSStringRef const RSErrorLocalizedFailureReasonKey RS_AVAILABLE(0_0);
RSExport RSStringRef const RSErrorLocalizedRecoverySuggestionKey RS_AVAILABLE(0_0);
RSExport RSStringRef const RSErrorDescriptionKey RS_AVAILABLE(0_0);
RSExport RSStringRef const RSErrorDebugDescriptionKey RS_AVAILABLE(0_0);
RSExport RSStringRef const RSErrorUnderlyingErrorKey RS_AVAILABLE(0_0);
RSExport RSStringRef const RSErrorURLKey RS_AVAILABLE(0_0);
RSExport RSStringRef const RSErrorFilePathKey RS_AVAILABLE(0_0);
RSExport RSStringRef const RSErrorTargetKey RS_AVAILABLE(0_0);           // - RSArray. used for RSErrorDomainRSCoreFoundation, push the number of objects return from RSErrorRSCoreFoundationDomainGetNeedTargetCount. if zero return from RSErrorRSCoreFoundationDomainGetNeedTargetCount, can be nil.

RSExport RSStringRef const RSErrorDomainPOSIX RS_AVAILABLE(0_0);
RSExport RSStringRef const RSErrorDomainOSStatus RS_AVAILABLE(0_0);
RSExport RSStringRef const RSErrorDomainMach RS_AVAILABLE(0_0);
RSExport RSStringRef const RSErrorDomainRSCoreFoundation RS_AVAILABLE(0_0);
RS_EXTERN_C_END
#endif
