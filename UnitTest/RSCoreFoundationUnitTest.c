//
//  RSCoreFoundationUnitTest.c
//  RSCoreFoundation
//
//  Created by RetVal on 4/15/13.
//  Copyright (c) 2013 RetVal. All rights reserved.
//

#include <RSCoreFoundationUnitTest.h>
#include <pthread.h>
typedef BOOL (*f)(RSErrorRef *error);

struct __UTFunction
{
    f _f;
};

static RSErrorRef unit_test_create_error(RSStringRef domain, RSErrorCode errorCode, RSStringRef key, RSStringRef descriptionFormat, ...);

static BOOL unit_test_runperform(RSErrorRef *error)
{
    RSPerformBlockOnMainThread(^{
        if (pthread_main_np())
            RSLog(RSSTR("PERFORM ON MAIN THREAD"));
        else
            RSLog(RSSTR("PERFORM ON BACK THREAD"));
    });
    RSPerformBlockInBackGroundWaitUntilDone(^{
        RSPerformBlockInBackGround(^{
            RSShow(RSCoreFoundationDidFinishLoadingNotification);
        });
    });
    return YES;
}

static BOOL unit_test_plist(RSErrorRef *error)
{
    __block RSPropertyListRef plist = nil;
    __block RSStringRef path = nil;
    RSPerformBlockInBackGroundWaitUntilDone(^{
        RSAutoreleaseBlock(^{
            RSShow(plist = (RSPropertyListRef)RSRetain(RSAutorelease(RSPropertyListCreateWithPath(RSAllocatorDefault, RSFileManagerStandardizingPath(path = RSSTR("~/Desktop/info.plist"))))));
        });
    });
    if (plist == nil)
    {
        RSLog(RSSTR("(%s) plist is nil.<line is %ld>"), __func__, __LINE__);
        if (error)
        {
            *error = unit_test_create_error(RSErrorDomainRSCoreFoundation,
                                            kErrExisting,
                                            RSErrorDebugDescriptionKey,
                                            RSSTR("parser %r is error."), path);
        }
        return NO;
    }
    RSShow(plist);
    RSRelease(plist);
    return YES;
}

static BOOL unit_test_string(RSErrorRef *error)
{
    RSStringRef test = RSSTR("what the hell with the testing...");
    RSMutableStringRef need_replace = RSMutableCopy(RSAllocatorDefault, test);
    if (NO == RSEqual(RSStringReplaceAll(need_replace, RSSTR("l"), RSSTR("-")), RSSTR("what the he-- with the testing...")))
    {
        if (error)
        {
            *error = unit_test_create_error(RSErrorDomainRSCoreFoundation,
                                            kErrMmUnknown,
                                            RSErrorDebugDescriptionKey,
                                            RSSTR("%s (RSReplaceAll is error)<line is %ld>"), __func__, __LINE__);
        }
        RSRelease(need_replace);
        return NO;
    }
    else
        RSRelease(need_replace);
    RSRange rangeResult = {0};
    if (NO == RSStringFind(test, RSSTR("the"), RSStringGetRange(test), &rangeResult))
    {
        if (error)
        {
            *error = unit_test_create_error(RSErrorDomainRSCoreFoundation,
                                            kErrMmUnknown,
                                            RSErrorDebugDescriptionKey,
                                            RSSTR("%s (RSStringFind is error)<line is %ld>"), __func__, __LINE__);
        }
        return NO;
    }
    RSStringRef intest = RSSTR("wHat tHe hell with the testing...");
    if (RSCompareEqualTo != RSStringCompareCaseInsensitive(intest, test))
    {
        if (error)
        {
            *error = unit_test_create_error(RSErrorDomainRSCoreFoundation,
                                            kErrMmUnknown,
                                            RSErrorDebugDescriptionKey,
                                            RSSTR("%s (RSStringCompareCaseInsensitive is error)<line is %ld>"), __func__, __LINE__);
        }
        return NO;
    }
    if (NO == RSStringHasPrefix(test, RSSTR("what")))
    {
        if (error)
        {
            *error = unit_test_create_error(RSErrorDomainRSCoreFoundation,
                                            kErrMmUnknown,
                                            RSErrorDebugDescriptionKey,
                                            RSSTR("%s (RSStringHasPrefix is error)<line is %ld>"), __func__, __LINE__);
        }
        return NO;
    }
    if (NO == RSStringHasSuffix(test, RSSTR("testing...")))
    {
        if (error)
        {
            *error = unit_test_create_error(RSErrorDomainRSCoreFoundation,
                                            kErrMmUnknown,
                                            RSErrorDebugDescriptionKey,
                                            RSSTR("%s (RSStringHasSuffix is error)<line is %ld>"), __func__, __LINE__);
        }
        return NO;
    }
    
    RSStringRef string = RSStringCreateWithCString(RSAllocatorDefault, "SourceCache/Library/RSCoreFoundation-1208/", RSStringEncodingUTF8);
    RSArrayRef array = RSStringCreateComponentsSeparatedByStrings(RSAllocatorDefault, string, RSSTR("/"));
    RSShow(array);
    
    RSStringRef x = RSStringCreateByCombiningStrings(RSAllocatorDefault, array, RSSTR("/"));
    RSShow(x);RSRelease(x);
    RSRelease(array);
    RSRelease(string);
    
    RSMutableStringRef ms = RSMutableCopy(RSAllocatorDefault, RSSTR("RSStringAppendString"));
    RSStringInsert(ms, strlen("RSStringAppend"), RSSTR("Cajsdklfjaljdlfaj"));
    RSShow(ms);
    RSRelease(ms);
    
    
    return YES;
}

static BOOL unit_test_timer(RSErrorRef *error)
{
    RSLog(RSSTR("---0"));
    RSPerformBlockAfterDelay(2.0f, ^{
        RSLog(RSSTR("---TIMER"));
    });
    RSLog(RSSTR("---1"));
    
    RSTimerRef timer = RSTimerCreateSchedule(RSAllocatorDefault, 0, 1.0f, YES, nil, ^(RSTimerRef timer) {
        RSLog(RSSTR("%R is fired"), timer);
    });
    RSTimerFire(RSAutorelease(timer));
    return YES;
}

static BOOL unit_test_json(RSErrorRef *error)
{
    RSStringRef __autorelease pd = RSFileManagerStandardizingPath(RSSTR("~/Desktop/tests"));
    RSArrayRef paths = RSFileManagerSubpathsOfDirectory(RSFileManagerGetDefault(), pd, nil);
    RSShow(paths);
    
    RSStringRef full = nil;
    RSIndex cnt = RSArrayGetCount(paths);
    for (RSIndex idx = 0; idx < cnt; idx++)
    {
        full = RSStringCreateWithFormat(RSAllocatorDefault, RSSTR("%R/%R"), pd, RSArrayObjectAtIndex(paths, idx));
        RSDataRef json = RSDataCreateWithContentOfPath(RSAllocatorDefault, full);
        RSTypeRef d = RSJSONSerializationCreateWithJSONData(RSAllocatorDefault, json);;
        RSRelease(json);
        if (d)
            RSShow(d);
        RSRelease(d);
        RSRelease(full);
        
        if (!d)
        {
            return NO;
        }
    }
    
    RSRelease(paths);
    return YES;
}

struct __UTFunction _func_table[] = {unit_test_plist, unit_test_string, unit_test_runperform, unit_test_timer};

static unsigned int _kSizeOfF = sizeof(_func_table)/ sizeof(struct __UTFunction);
 
int rs_ut_main()
{
    BOOL result = YES;
    for (unsigned int idx = 0; idx < _kSizeOfF; ++idx) {
        struct __UTFunction utf = _func_table[idx];
        RSErrorRef error = nil;
        result = utf._f(&error);
        if (NO == result)
        {
            if (error)
            {
                RSShow(error);
                RSRelease(error);
            }
        }
    }
    return 0;
}

static RSErrorRef unit_test_create_error(RSStringRef domain, RSErrorCode errorCode, RSStringRef key, RSStringRef descriptionFormat, ...)
{
    if (domain)
    {
        va_list ap;
        va_start(ap, descriptionFormat);
        RSStringRef description = RSStringCreateWithFormatAndArguments(RSAllocatorDefault, 0, descriptionFormat, ap);
        va_end(ap);
        RSDictionaryRef userInfo = (key && description) ? RSDictionaryCreateWithObjectsAndOKeys(RSAllocatorDefault, description, key, NULL) : nil;
        RSRelease(description);
        RSErrorRef error = RSErrorCreate(RSAllocatorDefault, domain, errorCode, userInfo);
        return error;
    }
    return nil;
}