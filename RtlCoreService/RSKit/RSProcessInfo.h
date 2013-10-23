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

/*! @function RSProcessInfoGetTypeID
 @abstract Return the RSTypeID type id.
 @discussion This function return the type id of RSProcessInfo from the runtime.
 @result A RSTypeID the type id of RSProcessInfoRef.
 */
RSExport RSTypeID RSProcessInfoGetTypeID();

/*! @function RSProcessInfoGetDefault
 @abstract Return the default RSProcessInfoRef instance.
 @discussion This function return the a global default RSProcessInfoRef instance.
 @result A RSProcessInfoRef the global default RSProcessInfoRef instance.
 */
RSExport RSProcessInfoRef RSProcessInfoGetDefault();

/*! @function RSProcessInfoGetEnvironment
 @abstract Return the runtime environment in a RSDictionaryRef.
 @pragma processInfo RSProcessInfoRef return by RSProcessInfoGetDefault().
 @discussion This function use processInfo to get a RSDictionaryRef with runtime environment information inside, the return instance is not owned by caller, need not release the instance.
 @result A RSDictionaryRef the runtime environment information.
 */
RSExport RSDictionaryRef RSProcessInfoGetEnvironment(RSProcessInfoRef processInfo);

/*! @function RSProcessInfoGetProgramName
 @abstract Return the name of Program.
 @pragma processInfo RSProcessInfoRef return by RSProcessInfoGetDefault().
 @discussion This function use processInfo to get a RSStringRef with current program name.
 @result A RSStringRef the program name.
 */
RSExport RSStringRef RSProcessInfoGetProgramName(RSProcessInfoRef processInfo);

/*! @function RSProcessInfoGetMachineType
 @abstract Return the type of Machine.
 @pragma processInfo RSProcessInfoRef return by RSProcessInfoGetDefault().
 @discussion This function use processInfo to get a RSStringRef with current machine type.
 @result A RSStringRef the machine type.
 */
RSExport RSStringRef RSProcessInfoGetMachineType(RSProcessInfoRef processInfo);

/*! @function RSProcessInfoGetMachineSerinalNumber
 @abstract Return the serinal number of current machine.
 @pragma processInfo RSProcessInfoRef return by RSProcessInfoGetDefault().
 @discussion This function use processInfo to get a RSStringRef with serinal number of current machine, need RSProcessInfo.dylib in RSCoreFoundation's bundle.
 @result A RSStringRef the serinal number of current machine.
 */
RSExport RSStringRef RSProcessInfoGetMachineSerinalNumber(RSProcessInfoRef processInfo);

/*! @function RSProcessInfoGetOperatingSystemVersion
 @abstract Return the version of operating system.
 @pragma processInfo RSProcessInfoRef return by RSProcessInfoGetDefault().
 @discussion This function use processInfo to get a RSStringRef with the version of operating system.
 @result A RSStringRef the version of operating system.
 */
RSExport RSStringRef RSProcessInfoGetOperatingSystemVersion(RSProcessInfoRef processInfo);

/*! @function RSProcessInfoGetKernelBuildVersion
 @abstract Return the build version of kernel.
 @pragma processInfo RSProcessInfoRef return by RSProcessInfoGetDefault().
 @discussion This function use processInfo to get a RSStringRef with the build version of kernel.
 @result A RSStringRef the build version of kernel.
 */
RSExport RSStringRef RSProcessInfoGetKernelBuildVersion(RSProcessInfoRef processInfo);

/*! @function RSProcessInfoGetKernelTypeName
 @abstract Return the type name of kernel.
 @pragma processInfo RSProcessInfoRef return by RSProcessInfoGetDefault().
 @discussion This function use processInfo to get a RSStringRef with the type name of kernel.
 @result A RSStringRef the type name of kernel.
 */
RSExport RSStringRef RSProcessInfoGetKernelTypeName(RSProcessInfoRef processInfo);

/*! @function RSProcessInfoGetKernelUUID
 @abstract Return the type name of kernel.
 @pragma processInfo RSProcessInfoRef return by RSProcessInfoGetDefault().
 @discussion This function use processInfo to get a RSStringRef with the type name of kernel.
 @result A RSStringRef the type name of kernel.
 */
RSExport RSStringRef RSProcessInfoGetKernelUUID(RSProcessInfoRef processInfo);

/*! @function RSProcessInfoGetKernelReleaseVersion
 @abstract Return the release version of kernel.
 @pragma processInfo RSProcessInfoRef return by RSProcessInfoGetDefault().
 @discussion This function use processInfo to get a RSStringRef with the release version of kernel.
 @result A RSStringRef the release version of kernel.
 */
RSExport RSStringRef RSProcessInfoGetKernelReleaseVersion(RSProcessInfoRef processInfo);

/*! @function RSProcessInfoGetMemorySize
 @abstract Return totoal memory size(byte).
 @pragma processInfo RSProcessInfoRef return by RSProcessInfoGetDefault().
 @discussion This function use processInfo to get totoal memory size.
 @result unsigned long long the totoal memory size.
 */
RSExport unsigned long long RSProcessInfoGetMemorySize(RSProcessInfoRef processInfo);

/*! @function RSProcessInfoGetCache1Size
 @abstract Return the release version of kernel.
 @pragma processInfo RSProcessInfoRef return by RSProcessInfoGetDefault().
 @discussion This function use processInfo to get a RSStringRef with the release version of kernel.
 @result A RSStringRef the release version of kernel.
 */
RSExport unsigned long long RSProcessInfoGetCache1Size(RSProcessInfoRef processInfo);

/*! @function RSProcessInfoGetCache2Size
 @abstract Return the release version of kernel.
 @pragma processInfo RSProcessInfoRef return by RSProcessInfoGetDefault().
 @discussion This function use processInfo to get a RSStringRef with the release version of kernel.
 @result A RSStringRef the release version of kernel.
 */
RSExport unsigned long long RSProcessInfoGetCache2Size(RSProcessInfoRef processInfo);

/*! @function RSProcessInfoGetCache3Size
 @abstract Return the release version of kernel.
 @pragma processInfo RSProcessInfoRef return by RSProcessInfoGetDefault().
 @discussion This function use processInfo to get a RSStringRef with the release version of kernel.
 @result A RSStringRef the release version of kernel.
 */
RSExport unsigned long long RSProcessInfoGetCache3Size(RSProcessInfoRef processInfo);

/*! @function RSProcessInfoGetKernelReleaseVersion
 @abstract Return the release version of kernel.
 @pragma processInfo RSProcessInfoRef return by RSProcessInfoGetDefault().
 @discussion This function use processInfo to get a RSStringRef with the release version of kernel.
 @result A RSStringRef the release version of kernel.
 */
RSExport unsigned long long RSProcessInfoGetProcessorCount(RSProcessInfoRef processInfo);

/*! @function RSProcessInfoGetProcessorDescription
 @abstract Return the processor description.
 @pragma processInfo RSProcessInfoRef return by RSProcessInfoGetDefault().
 @discussion This function use processInfo to get a RSStringRef with the processor description.
 @result A RSStringRef the processor description.
 */
RSExport RSStringRef RSProcessInfoGetProcessorDescription(RSProcessInfoRef processInfo);

RS_EXTERN_C_END
#endif 
