//
//  RSBundle.h
//  RSCoreFoundation
//
//  Created by RetVal on 2/11/13.
//  Copyright (c) 2013 RetVal. All rights reserved.
//

#ifndef RSCoreFoundation_RSBundle_h
#define RSCoreFoundation_RSBundle_h

#include <RSCoreFoundation/RSBase.h>
#include <RSCoreFoundation/RSByteOrder.h>
#include <RSCoreFoundation/RSError.h>

#if DEPLOYMENT_TARGET_MACOSX
#define BINARY_SUPPORT_DYLD 1
#define BINARY_SUPPORT_DLFCN 1
#define USE_DYLD_PRIV 1
#elif DEPLOYMENT_TARGET_EMBEDDED || DEPLOYMENT_TARGET_EMBEDDED_MINI
#define BINARY_SUPPORT_DYLD 1
#define BINARY_SUPPORT_DLFCN 1
#define USE_DYLD_PRIV 1
#elif DEPLOYMENT_TARGET_WINDOWS
#define BINARY_SUPPORT_DLL 1
#else
#error Unknown or unspecified DEPLOYMENT_TARGET
#endif
RS_EXTERN_C_BEGIN


enum {
    RSBundleExecutableArchitectureI386     = 0x00000007,
    RSBundleExecutableArchitecturePPC      = 0x00000012,
    RSBundleExecutableArchitectureX86_64   = 0x01000007,
    RSBundleExecutableArchitecturePPC64    = 0x01000012
} RS_AVAILABLE(0_0);

typedef struct __RSBundle* RSBundleRef;

RSExport RSTypeID RSBundleGetTypeID() RS_AVAILABLE(0_0);
RSExport RSStringRef RSBundleGetIdentifier(RSBundleRef bundle) RS_AVAILABLE(0_0);
RSExport RSStringRef RSBundleCopyExecutablePath(RSBundleRef bundle) RS_AVAILABLE(0_0);
RSExport RSTypeRef RSBundleGetValueForInfoDictionaryKey(RSBundleRef bundle, RSStringRef key) RS_AVAILABLE(0_0);
RSExport RSStringRef RSBundleGetPath(RSBundleRef bundle) RS_AVAILABLE(0_0);

RSExport BOOL RSBundleLoadExecutableAndReturnError(RSBundleRef bundle, BOOL forceGloable, __autorelease RSErrorRef* error);
RSExport BOOL RSBundleLoadExecutable(RSBundleRef bundel, BOOL forceGloable) RS_AVAILABLE(0_0);

RSExport RSBundleRef RSBunleCreateWithPath(RSAllocatorRef allocator, RSStringRef path) RS_AVAILABLE(0_0);
RSExport RSBundleRef RSBundleCreateFromFileSystem(RSAllocatorRef allocator, RSStringRef bundlePath) RS_AVAILABLE(0_0);

RSExport RSStringRef RSBundleCreatePathForResource(RSBundleRef bundle, RSStringRef name, RSStringRef type) RS_AVAILABLE(0_0);
RSExport BOOL RSBundleIsExecutableLoaded(RSBundleRef bundle) RS_AVAILABLE(0_0);

RSExport RSDictionaryRef RSBundleGetInfoDict(RSBundleRef bundle) RS_AVAILABLE(0_0);
RSExport RSBundleRef RSBundleGetWithIdentifier(RSStringRef identifier) RS_AVAILABLE(0_0);

RSExport void *RSBundleGetFunctionPointerForName(RSBundleRef bundle, RSStringRef funcName) RS_AVAILABLE(0_4);

RSExport RSBundleRef RSBundleGetMainBundle() RS_AVAILABLE(0_3);

/* Intended for eventual public consumption */
typedef enum {
    RSBundleOtherExecutableType = 0,
    RSBundleMachOExecutableType,
    RSBundlePEFExecutableType,
    RSBundleELFExecutableType,
    RSBundleDLLExecutableType
} RSBundleExecutableType;

typedef RS_ENUM(RSErrorCode, RSBundleErrorCode)
{
    RSBundleExecutableNotFoundError             = 3583,
    RSBundleExecutableNotLoadableError          = 3584,
    RSBundleExecutableArchitectureMismatchError = 3585,
    RSBundleExecutableRuntimeMismatchError      = 3586,
    RSBundleExecutableLoadError                 = 3587,
    RSBundleExecutableLinkError                 = 3588
};
RS_EXTERN_C_END
#endif
