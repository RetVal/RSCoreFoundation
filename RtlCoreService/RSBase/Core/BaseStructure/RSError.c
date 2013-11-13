//
//  RSError.c
//  RSCoreFoundation
//
//  Created by RetVal on 12/5/12.
//  Copyright (c) 2012 RetVal. All rights reserved.
//

#include <stdio.h>
#include <RSCoreFoundation/RSBase.h>
#include <RSCoreFoundation/RSDictionary.h>
#include <RSCoreFoundation/RSError.h>
#include <RSCoreFoundation/RSString.h>

#include <RSCoreFoundation/RSRuntime.h>

#include "RSPrivate/RSErrorPrivate/RSErrorPrivate.h"

typedef RSTypeRef (*RSErrorUserInfoKeyCallBack)(RSErrorRef err, RSStringRef key);
RSExport void RSErrorSetCallBackForDomain(RSStringRef domainName, RSErrorUserInfoKeyCallBack callBack);
RSExport RSErrorUserInfoKeyCallBack RSErrorGetCallBackForDomain(RSStringRef domainName);
struct __RSError
{
    RSRuntimeBase _base;
    RSIndex _code;
    RSStringRef _domain;
    RSDictionaryRef _userInfo;
};

static void __RSErrorClassInit(RSTypeRef obj)
{
    return;
}

static RSTypeRef __RSErrorClassCopy(RSAllocatorRef allocator, RSTypeRef rs, BOOL mutableCopy)
{
    RSErrorRef error = (RSErrorRef)rs;
    RSIndex errCode = RSErrorGetCode(error);
    RSStringRef errStr = RSErrorGetDomain(error);
    RSDictionaryRef errUserInfo = RSErrorCopyUserInfo(error);
    RSErrorRef copy = RSErrorCreate(allocator, errStr, errCode, errUserInfo);
    RSRelease(errUserInfo);
    return copy;
}
static void __RSErrorClassDeallocate(RSTypeRef obj)
{
    RSErrorRef err = (RSErrorRef)obj;
    if (err->_domain) RSRelease(err->_domain);
    if (err->_userInfo) RSRelease(err->_userInfo);
    err->_code = kSuccess;
}

static BOOL __RSErrorClassEqual(RSTypeRef obj1, RSTypeRef obj2)
{
    if (obj1 == obj2) return YES;
    RSErrorRef err1 = (RSErrorRef)obj1;
    RSErrorRef err2 = (RSErrorRef)obj2;
    if (err1->_code != err2->_code) return NO;
    if (RSEqual(err1->_domain, err2->_domain) == NO) return NO;
    if (RSEqual(err1->_userInfo, err2->_userInfo) == NO) return NO;
    return YES;
}

static RSStringRef __RSErrorClassDescription(RSTypeRef obj)
{
    RSErrorRef err = (RSErrorRef)obj;
    RSMutableStringRef description = RSStringCreateMutable(RSAllocatorSystemDefault, 128);
    if (err->_userInfo) {
        RSStringAppendStringWithFormat(description,
                                       RSSTR("RSError = {\n\tError Code = %ld\n\tDomain = %R\n\tUser Information = %R\n"),
                                       err->_code,
                                       err->_domain,
                                       err->_userInfo);
    }else {
        RSStringAppendStringWithFormat(description, RSSTR("RSError = {\n\tError Code = %ld\n\tDomain = %R\n\tUser Information = nil.\n"),
                                       err->_code,
                                       err->_domain);
    }
    
    return description;
}

static RSHashCode __RSErrorClassHash(RSTypeRef obj)
{
    RSErrorRef err = (RSErrorRef)obj;
    return (RSHashCode)RSHash(err->_domain) + err->_code;
}

static RSRuntimeClass __RSErrorClass = {
    _RSRuntimeScannedObject,
    "RSError",
    __RSErrorClassInit,
    nil,
    __RSErrorClassDeallocate,
    __RSErrorClassEqual,
    __RSErrorClassHash,
    __RSErrorClassDescription,
    nil,
    nil,
};

static RSTypeID __RSErrorTypeID = _RSRuntimeNotATypeID;

RSExport RSTypeID RSErrorGetTypeID()
{
    return __RSErrorTypeID;
}

RSPrivate void __RSErrorInitialize()
{
    __RSErrorTypeID = __RSRuntimeRegisterClass(&__RSErrorClass);
    __RSRuntimeSetClassTypeID(&__RSErrorClass, __RSErrorTypeID);
}

/* Pre-defined userInfo keys
 */
RS_PUBLIC_CONST_STRING_DECL(RSErrorLocalizedDescriptionKey,          "RSLocalizedDescription");
RS_PUBLIC_CONST_STRING_DECL(RSErrorLocalizedFailureReasonKey,        "RSLocalizedFailureReason");
RS_PUBLIC_CONST_STRING_DECL(RSErrorLocalizedRecoverySuggestionKey,   "RSLocalizedRecoverySuggestion");
RS_PUBLIC_CONST_STRING_DECL(RSErrorDescriptionKey,                   "RSDescription");
RS_PUBLIC_CONST_STRING_DECL(RSErrorDebugDescriptionKey,              "RSDebugDescription");
RS_PUBLIC_CONST_STRING_DECL(RSErrorUnderlyingErrorKey,               "RSUnderlyingError");
RS_PUBLIC_CONST_STRING_DECL(RSErrorURLKey,                           "RSURL");
RS_PUBLIC_CONST_STRING_DECL(RSErrorFilePathKey,                      "RSFilePath");
RS_PUBLIC_CONST_STRING_DECL(RSErrorTargetKey,                        "RSTargets");

/* Pre-defined error domains
 */
RS_PUBLIC_CONST_STRING_DECL(RSErrorDomainPOSIX, "RSPOSIXErrorDomain");
RS_PUBLIC_CONST_STRING_DECL(RSErrorDomainOSStatus, "RSOSStatusErrorDomain");
RS_PUBLIC_CONST_STRING_DECL(RSErrorDomainMach, "RSMachErrorDomain");
RS_PUBLIC_CONST_STRING_DECL(RSErrorDomainRSCoreFoundation, "RSCoreFoundationErrorDomain");

static RSSpinLock __RSErrorCallBackTableSpinlock = RSSpinLockInit;
static RSMutableDictionaryRef __RSErrorCallBackTable = nil;


#define __RSAssertIsError(rs)   __RSGenericValidInstance(rs, __RSErrorTypeID)

RSExport RSIndex RSErrorGetCode(RSErrorRef err)
{
    __RSAssertIsError(err);
    return err->_code;
}

RSExport RSStringRef RSErrorGetDomain(RSErrorRef err)
{
    __RSAssertIsError(err);
    return err->_domain;
}

static RSDictionaryRef __RSErrorGetUserInfo(RSErrorRef err)
{
    __RSAssertIsError(err);
    return err->_userInfo;
}

RSExport RSDictionaryRef RSErrorCopyUserInfo(RSErrorRef err)
{
    __RSAssertIsError(err);
    return (err->_userInfo) ? RSRetain(err->_userInfo) : RSDictionaryCreate(RSAllocatorSystemDefault, nil, nil, RSDictionaryRSTypeContext, 0);
}

RSExport RSErrorRef RSErrorCreate(RSAllocatorRef allocator, RSStringRef domain, RSIndex code, RSDictionaryRef userInfo)
{
    RSErrorRef err = (RSErrorRef)__RSRuntimeCreateInstance(allocator, RSErrorGetTypeID(), sizeof(struct __RSError) - sizeof(RSRuntimeBase));
    err->_code = code;
    if (domain) err->_domain = RSRetain(domain);
    else HALTWithError(RSInvalidArgumentException, "the domain is nil.");
    if (userInfo) err->_userInfo = RSRetain(userInfo);
    
    return err;
}

RSExport RSErrorRef RSErrorCreateWithKeysAndValues(RSAllocatorRef allocator, RSStringRef domain, RSIndex code, const void** keys, const void** values, RSIndex userInfoValuesCount)
{
    RSDictionaryRef userInfo = RSDictionaryCreate(allocator, keys, values, RSDictionaryRSTypeContext, userInfoValuesCount);
    RSErrorRef err = RSErrorCreate(allocator, domain, code, userInfo);
    RSRelease(userInfo);
    return err;
}

static RSStringRef __RSErrorCopyUserInfoKey(RSErrorRef err, RSStringRef key)
{
    RSStringRef result = nil;
    RSDictionaryRef userInfo = __RSErrorGetUserInfo(err);
    if (userInfo) result = RSDictionaryGetValue(userInfo, key); // result is not retained by RSDictionaryGetValue.
    if (nil == result)
    {
        RSErrorUserInfoKeyCallBack cb = RSErrorGetCallBackForDomain(RSErrorGetDomain(err));
        if (cb) result = (RSStringRef)cb(err, key);
        return result;
    }
    return RSRetain(result);
}
/*
 typedef RSTypeRef (*RSErrorUserInfoKeyCallBack)(RSErrorRef err, RSStringRef key)
 */
static RSTypeRef __RSErrorPOSIXCallBack(RSErrorRef err, RSStringRef key)
{
    if (!RSEqual(key, RSErrorDescriptionKey) && !RSEqual(key, RSErrorLocalizedFailureReasonKey)) return nil;
    
    const char *errStr = strerror((int)RSErrorGetCode(err));
    if (errStr && strlen(errStr)) return RSStringCreateWithCString(RSAllocatorSystemDefault, errStr, RSStringEncodingUTF8);
    #if DEPLOYMENT_TARGET_MACOSX || DEPLOYMENT_TARGET_EMBEDDED
    /*RSBundle*/
    #endif
    
    return nil;
}

/*
 RSErrorDomainRSCoreFoundation
 */
//static RSSpinLock __RSErrorDomainRSCoreFoundationUserInfoLock = RSSpinLockInit;
//static RSMutableDictionaryRef __RSErrorDomainRSCoreFoundationUserInfo = nil;

static RSTypeRef __RSErrorRSCoreFoundationCallBack(RSErrorRef err, RSStringRef key)
{
    if (!RSEqual(key, RSErrorDescriptionKey) && !RSEqual(key, RSErrorLocalizedFailureReasonKey)) return nil;
    RSStringRef errStr = nil;
    struct __RSErrorPrivateFormatTable cFormat = __RSErrorDomainRSCoreFoundationGetCStringWithCode(RSErrorGetCode(err));
    if (cFormat.argsCnt)
    {
        RSStringRef format = RSStringCreateWithCString(RSAllocatorSystemDefault, cFormat.format, RSStringEncodingUTF8);
        RSArrayRef objects = RSDictionaryGetValue(__RSErrorGetUserInfo(err), RSErrorTargetKey);
        RSTypeRef object[3] = {nil};
        RSIndex minCnt = 0;
        if (objects)
        {
            minCnt = min(cFormat.argsCnt, RSArrayGetCount(objects));
            switch (minCnt)
            {
                case 1:
                    object[0] = RSArrayObjectAtIndex(objects, 0);
                    errStr = RSStringCreateWithFormat(RSAllocatorSystemDefault, format, object[0]);
                    break;
                case 2:
                    object[0] = RSArrayObjectAtIndex(objects, 0);
                    object[1] = RSArrayObjectAtIndex(objects, 1);
                    errStr = RSStringCreateWithFormat(RSAllocatorSystemDefault, format, object[0], object[1]);
                    break;
                case 3:
                    object[0] = RSArrayObjectAtIndex(objects, 0);
                    object[1] = RSArrayObjectAtIndex(objects, 1);
                    object[2] = RSArrayObjectAtIndex(objects, 2);
                    errStr = RSStringCreateWithFormat(RSAllocatorSystemDefault, format, object[0], object[1], object[2]);
                    break;
                default:
                    HALTWithError(RSInvalidArgumentException, "the formate is too long to support");
                    break;
            }
        }
        for (RSIndex idx = 0; idx < minCnt; idx++)
        {
            RSRelease(object[idx]);
        }
        RSRelease(format);
    }
    else
    {
        errStr = RSStringCreateWithCString(RSAllocatorSystemDefault, cFormat.format, RSStringEncodingUTF8);
    }

    return errStr;
}
#if DEPLOYMENT_TARGET_MACOSX || DEPLOYMENT_TARGET_EMBEDDED
/* Built-in callback for Mach domain.
 */
#include <mach/mach_error.h>
static RSTypeRef __RSErrorMachCallBack(RSErrorRef err, RSStringRef key)
{
    if (RSEqual(key, RSErrorDescriptionKey))
    {
        const char *errStr = mach_error_string((mach_error_t)RSErrorGetCode(err));
        if (errStr && strlen(errStr)) return RSStringCreateWithCString(RSAllocatorSystemDefault, errStr, RSStringEncodingUTF8);
    }
    return nil;
}
#endif


RSExport RSStringRef RSErrorCopyDescription(RSErrorRef err)
{
    __RSAssertIsError(err);
    RSStringRef result = __RSErrorCopyUserInfoKey(err, RSErrorLocalizedDescriptionKey), reason = nil;
    if (result == nil)
    {
        reason = __RSErrorCopyUserInfoKey(err, RSErrorLocalizedFailureReasonKey);
        if (reason)
            result = RSStringCreateWithFormat(RSAllocatorSystemDefault, RSSTR("The operation couldn't be completed. %R"), reason);
        else if ((reason = __RSErrorCopyUserInfoKey(err, RSErrorDescriptionKey)))
            result = RSStringCreateWithFormat(RSAllocatorSystemDefault, RSSTR("The operation couldn't be completed.(%R error %ld - %R)"), RSErrorGetDomain(err), RSErrorGetCode(err), reason);
        else
            result = RSStringCreateWithFormat(RSAllocatorSystemDefault, RSSTR("The operation couldn't be completed.(%R error %ld)"), RSErrorGetDomain(err), RSErrorGetCode(err));
    }
    if (reason) RSRelease(reason);
    return result;
}

static void __RSErrorInitUserInfoKeyCallBackTable()
{
    RSMutableDictionaryRef table = RSDictionaryCreateMutable(RSAllocatorSystemDefault, 0, RSDictionaryNilValueContext);
    RSSpinLockLock(&__RSErrorCallBackTableSpinlock);
    if (!__RSErrorCallBackTable) {
        __RSErrorCallBackTable = table;
    }
    else
    {
        RSSpinLockUnlock(&__RSErrorCallBackTableSpinlock);
        return;
    }
    RSSpinLockUnlock(&__RSErrorCallBackTableSpinlock);
    RSErrorSetCallBackForDomain(RSErrorDomainPOSIX, __RSErrorPOSIXCallBack);
#if DEPLOYMENT_TARGET_MACOSX || DEPLOYMENT_TARGET_EMBEDDED
    RSErrorSetCallBackForDomain(RSErrorDomainMach, __RSErrorMachCallBack);
#endif
    RSErrorSetCallBackForDomain(RSErrorDomainRSCoreFoundation, __RSErrorRSCoreFoundationCallBack);
}

RSExport void RSErrorSetCallBackForDomain(RSStringRef domainName, RSErrorUserInfoKeyCallBack callBack)
{
    if (!__RSErrorCallBackTable) __RSErrorInitUserInfoKeyCallBackTable();
    RSSpinLockLock(&__RSErrorCallBackTableSpinlock);
    if (callBack) {
        RSDictionarySetValue(__RSErrorCallBackTable, domainName, (void *)callBack);
    } else {
        RSDictionaryRemoveValue(__RSErrorCallBackTable, domainName);
    }
    RSSpinLockUnlock(&__RSErrorCallBackTableSpinlock);
}

RSExport RSErrorUserInfoKeyCallBack RSErrorGetCallBackForDomain(RSStringRef domainName)
{
    if (!__RSErrorCallBackTable) __RSErrorInitUserInfoKeyCallBackTable();
    RSSpinLockLock(&__RSErrorCallBackTableSpinlock);
    RSErrorUserInfoKeyCallBack cb = RSDictionaryGetValue(__RSErrorCallBackTable, domainName);
    RSSpinLockUnlock(&__RSErrorCallBackTableSpinlock);
    return cb;
}

RSExport RSIndex RSErrorRSCoreFoundationDomainGetNeedTargetCount(RSErrorRef err)
{
    __RSAssertIsError(err);
    if (!RSEqual(err, RSErrorDomainRSCoreFoundation)) return 0;
    return __RSErrorDomainRSCoreFoundationGetCStringWithCode(RSErrorGetCode(err)).argsCnt;
}

RSPrivate void __RSErrorDeallocate()
{
    if (!__RSErrorCallBackTable) return;
    RSSpinLockLock(&__RSErrorCallBackTableSpinlock);
    RSRelease(__RSErrorCallBackTable);
    __RSErrorCallBackTable = nil;
    RSSpinLockUnlock(&__RSErrorCallBackTableSpinlock);
}