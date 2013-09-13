//
//  RSProcessInfo.h
//  RSCoreFoundation
//
//  Created by RetVal on 8/16/13.
//  Copyright (c) 2013 RetVal. All rights reserved.
//

#ifndef RSCoreFoundation_RSProcessInfo
#define RSCoreFoundation_RSProcessInfo

#include <RSCoreFoundation/RSCoreFoundation.h>

RS_EXTERN_C_BEGIN

typedef struct __RSProcessInfo *RSProcessInfoRef;

RSExport RSTypeID RSProcessInfoGetTypeID();
RSExport RSProcessInfoRef RSProcessInfoGetDefault();
RSExport RSDictionaryRef RSProcessInfoGetEnvironment(RSProcessInfoRef processInfo);
RSExport RSStringRef RSProcessInfoGetProgramName(RSProcessInfoRef processInfo);
RSExport RSStringRef RSProcessInfoGetMachineType(RSProcessInfoRef processInfo);
RSExport RSStringRef RSProcessInfoGetMachineSerinalNumber(RSProcessInfoRef processInfo);
RSExport RSStringRef RSProcessInfoGetOperatingSystemVersion(RSProcessInfoRef processInfo);
RSExport RSStringRef RSProcessInfoGetKernelBuildVersion(RSProcessInfoRef processInfo);
RSExport RSStringRef RSProcessInfoGetKernelTypeName(RSProcessInfoRef processInfo);
RSExport RSStringRef RSProcessInfoGetKernelUUID(RSProcessInfoRef processInfo);
RSExport RSStringRef RSProcessInfoGetKernelReleaseVersion(RSProcessInfoRef processInfo);
RSExport unsigned long long RSProcessInfoGetMemorySize(RSProcessInfoRef processInfo);
RSExport unsigned long long RSProcessInfoGetCache1Size(RSProcessInfoRef processInfo);
RSExport unsigned long long RSProcessInfoGetCache2Size(RSProcessInfoRef processInfo);
RSExport unsigned long long RSProcessInfoGetCache3Size(RSProcessInfoRef processInfo);
RSExport unsigned long long RSProcessInfoGetProcessorCount(RSProcessInfoRef processInfo);
RSExport RSStringRef RSProcessInfoGetProcessorDescription(RSProcessInfoRef processInfo);

RS_EXTERN_C_END
#endif 
