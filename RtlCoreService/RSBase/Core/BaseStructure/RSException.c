//
//  RSException.c
//  RSCoreFoundation
//
//  Created by RetVal on 5/17/13.
//  Copyright (c) 2013 RetVal. All rights reserved.
//

#include <RSCoreFoundation/RSException.h>
#include <RSCoreFoundation/RSRuntime.h>
#include <execinfo.h>
#include "RSPrivate/RSException/RSPrivateExceptionHandler.h"

RS_PUBLIC_CONST_STRING_DECL(RSGenericException, "RSGenericException")
RS_PUBLIC_CONST_STRING_DECL(RSRangeException, "RSRangeException")
RS_PUBLIC_CONST_STRING_DECL(RSInvalidArgumentException, "RSInvalidArgumentException")
RS_PUBLIC_CONST_STRING_DECL(RSInternalInconsistencyException, "RSInternalInconsistencyException")

RS_PUBLIC_CONST_STRING_DECL(RSMallocException, "RSMallocException")

RS_PUBLIC_CONST_STRING_DECL(RSObjectInaccessibleException, "RSObjectInaccessibleException")
RS_PUBLIC_CONST_STRING_DECL(RSObjectNotAvailableException, "RSObjectNotAvailableException")
RS_PUBLIC_CONST_STRING_DECL(RSDestinationInvalidException, "RSDestinationInvalidException")

RS_PUBLIC_CONST_STRING_DECL(RSPortTimeoutException, "RSPortTimeoutException")
RS_PUBLIC_CONST_STRING_DECL(RSInvalidSendPortException, "RSInvalidSendPortException")
RS_PUBLIC_CONST_STRING_DECL(RSInvalidReceivePortException, "RSInvalidReceivePortException")
RS_PUBLIC_CONST_STRING_DECL(RSPortSendException, "RSPortSendException")
RS_PUBLIC_CONST_STRING_DECL(RSPortReceiveException, "RSPortReceiveException")

RS_PUBLIC_CONST_STRING_DECL(RSOldStyleException, "RSOldStyleException")

RS_PUBLIC_CONST_STRING_DECL(RSExceptionObject, "RSExceptionObject")
RS_PUBLIC_CONST_STRING_DECL(RSExceptionThreadId, "RSExceptionThreadId")
RS_PUBLIC_CONST_STRING_DECL(RSExceptionDetailInformation, "RSExceptionDetailInformation")

static void ___RSExceptionDump(int signo)
{
    RSExceptionRef e = RSExceptionCreate(RSAllocatorSystemDefault, RSGenericException, RSSTR(""), nil);
    RSStringRef s = RSDescription(e);
    __RSLog(RSLogLevelDebug, RSSTR("%R"), s);
    RSRelease(s);
    RSRelease(e);
}

static void ___RSExceptionDefaultDumpHandler(int signo)
{
    ___RSExceptionDump(signo);
}

static void ___RSExceptionInstallHandler()
{
    signal(SIGABRT, ___RSExceptionDefaultDumpHandler);
	signal(SIGILL, ___RSExceptionDefaultDumpHandler);
	signal(SIGSEGV, ___RSExceptionDefaultDumpHandler);
	signal(SIGFPE, ___RSExceptionDefaultDumpHandler);
	signal(SIGBUS, ___RSExceptionDefaultDumpHandler);
	signal(SIGPIPE, ___RSExceptionDefaultDumpHandler);
}

struct __RSException {
    RSRuntimeBase _base;
    RSStringRef _name;
    RSStringRef _exceptionReason;
    RSDictionaryRef _userInfo;
    int _frameDepth;
    void **_stackFrames;
    __strong char **_infoFrames;
};

static void __RSExceptionClassDeallocate(RSTypeRef rs)
{
    struct __RSException* _e = (struct __RSException *)rs;
    if (_e->_infoFrames) free(_e->_infoFrames);
    _e->_infoFrames = nil;
    if (_e->_userInfo) RSRelease(_e->_userInfo);
    _e->_userInfo = nil;
    return;
}

static RSHashCode __RSExceptionClassHash(RSTypeRef rs)
{
    return RSHash(((RSExceptionRef)rs)->_exceptionReason);
}

static BOOL __RSExceptionClassEqual(RSTypeRef rs1, RSTypeRef rs2)
{
    RSExceptionRef e1 = (RSExceptionRef)rs1;
    RSExceptionRef e2 = (RSExceptionRef)rs2;
    if (e1->_frameDepth != e2->_frameDepth) return NO;
    if (NO == RSEqual(e1->_exceptionReason, e2->_exceptionReason)) return NO;
    for (RSUInteger idx = 0; idx < e1->_frameDepth; ++idx)
        if (e1->_stackFrames[idx] != e2->_stackFrames[idx] ||
            0 != strcmp(e1->_infoFrames[idx], e2->_infoFrames[idx])) return NO;
    return YES;
}

static RSStringRef __RSExceptionClassDescription(RSTypeRef rs)
{
    RSExceptionRef e = (RSExceptionRef)rs;
    RSMutableStringRef description = RSStringCreateMutable(RSAllocatorSystemDefault, 0);
    if (e->_exceptionReason) RSStringAppendStringWithFormat(description, RSSTR("<%R> - %R\n"), e->_name, e->_exceptionReason);
    if (e->_userInfo)
    {
        BOOL append = NO;
        RSTypeRef obj = RSDictionaryGetValue(e->_userInfo, RSExceptionObject);
        if (obj) RSStringAppendStringWithFormat(description, RSSTR("%R = %R\n"), RSExceptionObject, obj ? append = YES, obj : nil);
        obj = RSDictionaryGetValue(e->_userInfo, RSExceptionDetailInformation);
        if (obj) RSStringAppendStringWithFormat(description, RSSTR("%R = %R\n"), RSExceptionDetailInformation, obj ? append = YES, obj : nil);
        if (append) RSStringAppendCString(description, "\n", RSStringEncodingASCII);
    }
    if (e->_stackFrames == nil) return description;
    if (e->_infoFrames)
    {
        for (int idx = 1; idx < e->_frameDepth; ++idx)
        {
            RSStringAppendStringWithFormat(description, RSSTR("%s\n"),  e->_infoFrames[idx]);
        }
    }
    
    return description;
}

static RSRuntimeClass __RSExceptionClass = {
    _RSRuntimeScannedObject,
    0,
    "RSException",
    nil,
    nil,
    __RSExceptionClassDeallocate,
    __RSExceptionClassEqual,
    __RSExceptionClassHash,
    __RSExceptionClassDescription,
    nil,
    nil
};

static RSTypeID _RSExceptionTypeID = _RSRuntimeNotATypeID;
RSPrivate void __RSExceptionInitialize()
{
    _RSExceptionTypeID = __RSRuntimeRegisterClass(&__RSExceptionClass);
    __RSRuntimeSetClassTypeID(&__RSExceptionClass, _RSExceptionTypeID);
    ___RSExceptionInstallHandler();
}

RSExport RSTypeID  RSExceptionGetTypeID()
{
    return _RSExceptionTypeID;
}

static BOOL __RSExceptionInit(RSExceptionRef ce)
{
    struct __RSException *_e = (struct __RSException *)ce;
    if (_e->_stackFrames)
    {
        _e->_infoFrames = backtrace_symbols(_e->_stackFrames, _e->_frameDepth);
        if (_e->_infoFrames) return YES;
    }
    return NO;
}

static RSExceptionRef __RSExceptionCreateWithFrames(RSAllocatorRef allocator,RSStringRef name, RSStringRef reason, RSDictionaryRef userInfo, int numFrames, void **frames)
{
    if (frames == nil || numFrames == 0) return nil;
    struct __RSException *e = (struct __RSException *)__RSRuntimeCreateInstance(allocator, _RSExceptionTypeID, sizeof(struct __RSException) - sizeof(RSRuntimeBase));
    e->_frameDepth = numFrames;
    e->_stackFrames = frames;
    e->_exceptionReason = reason;
    e->_name = name ?: RSGenericException;
    if (userInfo) e->_userInfo = RSRetain(userInfo);
    return e;
}

RSExport RSExceptionRef RSExceptionCreate(RSAllocatorRef allocator, RSStringRef name, RSStringRef reason, RSDictionaryRef userInfo)
{
    if (reason == nil) return nil;
    void *frames[100] = {0};
    int num = backtrace(frames, 100);
    struct __RSException *e = (struct __RSException*)__RSExceptionCreateWithFrames(allocator, name, reason, userInfo, num, frames);
    __RSExceptionInit(e);
    return e;
}

static RSExceptionRef __RSExceptionCreateWithArguments(RSAllocatorRef allocator, RSStringRef name, RSDictionaryRef userInfo, RSStringRef reasonFormat, va_list ap)
{
    RSStringRef reason = RSStringCreateWithFormatAndArguments(allocator, 0, reasonFormat, ap);
    RSExceptionRef e = RSExceptionCreate(allocator, name, reason, userInfo);
    RSRelease(reason);
    return e;
}

RSExport RSExceptionRef RSExceptionCreateWithFormat(RSAllocatorRef allocator, RSStringRef name, RSDictionaryRef userInfo, RSStringRef reasonFormat, ...)
{
    if (reasonFormat == nil) return nil;
    va_list ap;
    va_start(ap, reasonFormat);
    RSExceptionRef e = __RSExceptionCreateWithArguments(allocator, name, userInfo, reasonFormat, ap);
    va_end(ap);
    return e;
}

RSExport RSDictionaryRef RSExceptionGetUserInfo(RSExceptionRef e)
{
    if (nil == e) return nil;
    __RSGenericValidInstance(e, _RSExceptionTypeID);
    return e->_userInfo;
}

RSExport RSStringRef RSExceptionGetName(RSExceptionRef e)
{
    if (nil == e) return nil;
    __RSGenericValidInstance(e, _RSExceptionTypeID);
    return e->_name;
}

RSExport RSStringRef RSExceptionGetReason(RSExceptionRef e)
{
    if (nil == e) return nil;
    __RSGenericValidInstance(e, _RSExceptionTypeID);
    return e->_exceptionReason;
}

RSExport void **RSExceptionGetFrames(RSExceptionRef e)
{
    if (nil == e) return nil;
    __RSGenericValidInstance(e, _RSExceptionTypeID);
    return e->_stackFrames;
}

static RSExceptionHandler __RSExceptionDefaultHandler = ^(RSExceptionRef e) {
    RSShow(e);
    RSStringRef s = RSDescription(e);
    __RSLog(RSLogLevelDebug, RSSTR("%R"), s);
    RSRelease(s);
    __HALT();
};

RSExport void RSExceptionRaise(RSExceptionRef e)
{
    if (e)
    {
        __RSGenericValidInstance(e, _RSExceptionTypeID);
        if (NO == __tls_do_exception_with_handler(e))
            __RSExceptionDefaultHandler(e);
    }
    return;
}

RSExport void RSExceptionCreateAndRaise(RSAllocatorRef allocator, RSStringRef name, RSStringRef reason, RSDictionaryRef userInfo)
{
    RSExceptionRef e = RSExceptionCreate(allocator, name, reason, userInfo);
    RSExceptionRaise(e);
    RSRelease(e);
}

RSExport void RSExceptionCreateWithFormatAndRaise(RSAllocatorRef allocator, RSStringRef name, RSDictionaryRef userInfo, RSStringRef reasonFormat, ...)
{
    if (reasonFormat == nil) return;
    va_list ap;
    va_start(ap, reasonFormat);
    RSExceptionRef e = __RSExceptionCreateWithArguments(allocator, name, userInfo, reasonFormat, ap);
    va_end(ap);
    RSExceptionRaise(e);
    RSRelease(e);
}

RSExport void RSExceptionBlock(RSExecutableBlock tryBlock, RSExceptionHandler finalHandler)
{
    if (tryBlock)
    {
        if (finalHandler) __tls_add_exception_handler(&finalHandler);
        else __tls_add_exception_handler(&__RSExceptionDefaultHandler);
        tryBlock();
        if (finalHandler) __tls_cls_exception_handler(&finalHandler);
        else __tls_cls_exception_handler(&__RSExceptionDefaultHandler);
    }
}

static void __RSGetBacktrace(void** stack, int* size)
{
    *size = backtrace(stack, *size);
}

__attribute__((__format__(printf, 1, 0)))
static void vprintf_stderr_common(const char* format, va_list args)
{
#if !DEPLOYMENT_TARGET_WINDOWS
    if (strstr(format, "%@")) {
        RSStringRef rsformat = RSStringCreateWithCString(RSAllocatorSystemDefault, format, RSStringEncodingUTF8);
        
#if defined(__clang__)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wformat-nonliteral"
#endif
        RSStringRef str = RSStringCreateWithFormatAndArguments(RSAllocatorSystemDefault, 0, rsformat, args);
#if defined(__clang__)
#pragma clang diagnostic pop
#endif
        const char* buffer = RSStringCopyUTF8String(str);
    
        fputs(buffer, stderr);
        
        free((void *)buffer);
        RSRelease(str);
        RSRelease(rsformat);
        return;
    }
#endif
}

__attribute__((__format__(printf, 1, 2)))
static void printf_stderr_common(const char* format, ...)
{
    va_list args;
    va_start(args, format);
    vprintf_stderr_common(format, args);
    va_end(args);
}

static void __RSPrintBacktrace(void** stack, int size)
{
    for (int i = 0; i < size; ++i) {
        const char* mangledName = 0;
        char* cxaDemangled = 0;
        const int frameNumber = i + 1;
        if (mangledName || cxaDemangled)
            printf_stderr_common("%-3d %p %s\n", frameNumber, stack[i], cxaDemangled ? cxaDemangled : mangledName);
        else
            printf_stderr_common("%-3d %p\n", frameNumber, stack[i]);
        free(cxaDemangled);
    }
}

static void __RSReportBacktrace()
{
    static const int framesToShow = 31;
    static const int framesToSkip = 2;
    void* samples[framesToShow + framesToSkip];
    int frames = framesToShow + framesToSkip;
    
    __RSGetBacktrace(samples, &frames);
    __RSPrintBacktrace(samples + framesToSkip, frames - framesToSkip);
}

RSExport void RSCoreFoundationCrash(const char *crash)
{
    if (crash)
        __RSCLog(RSLogLevelWarning, "%s\n", crash);
    __RSReportBacktrace();
    *(int *)(uintptr_t)0xbbadbeef = 0;
    // More reliable, but doesn't say BBADBEEF.
#if defined(__clang__)
    __builtin_trap();
#else
    ((void(*)())0)();
#endif
}
