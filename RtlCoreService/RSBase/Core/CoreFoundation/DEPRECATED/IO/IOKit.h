//  Created by RetVal on 10/13/11.
//  Copyright (c) 2012 RetVal. All rights reserved.
//
#ifndef IOKit_core_h
#define IOKit_core_h 1

#include "MSKit.h"
#include "RtlBufferKit.h"
#include "fcntl.h"
#include "RtlServices.h"

#include "IOFileUnit.h"
#include "IOCacheUnit.h"


RtlExport BOOL  IOCreateDirectory(const RtlBuffer  dir);

RtlExport BOOL  IOIsDirectory(const RtlBuffer dir);

RtlExport RtlBuffer IOFTranslatePath(RtlBuffer path);
RtlExport RtlBuffer IOTranslatePath(const RtlBuffer path);

RtlExport RtlBuffer IOCacheCreateName(RtlBuffer buf);
//////////////////////////////////////////////////////////////////////////
/************************************************************************/
/* 1.2                                                                  */
/************************************************************************/
#ifdef __WIN32
RtlBuffer 			IOGetAppDirectory(RtlBufferRef  appDir);
RtlBuffer 			IOGetRootDirectory(RtlBufferRef  root);
RtlBuffer 			IOGetUserDirectory(RtlBufferRef  usr);
RtlBuffer 			IOGetDeveloperDirectory(RtlBufferRef  dev);
RtlBuffer 			IOGetConfigurationDirectory(RtlBufferRef  configDir);
RtlBuffer 			IOGetCurrentVolume(RtlBufferRef  volume);
RtlBuffer 			IOGetCurrentWorkDirectory(RtlBufferRef  directory);
RtlBuffer 			IOGetCacheDirectory(RtlBufferRef  cacheDir);
RtlBuffer 			IOGetCoreServiceDirectory(RtlBufferRef  serviceDir);

RtlBuffer 			IOGetAppRegisterDirectory(RtlBufferRef  appRegDir);
RtlBuffer 			IOGetUsrRegisterDirectory(RtlBufferRef  usrRegDir);
RtlBuffer 			IOGetDevRegisterDirectory(RtlBufferRef  devRegDir);
RtlBuffer 			IOGetLogDirectory(RtlBufferRef  logDir);
RtlBuffer 			IOGetPreferenceDirectory(RtlBufferRef  preferenceDir);

RtlBuffer 			IOFGetAppDirectory(RtlBlock	dir[BUFFER_SIZE]);
RtlBuffer 			IOFGetRootDirectory(RtlBlock dir[BUFFER_SIZE]);
RtlBuffer 			IOFGetUserDirectory(RtlBlock dir[BUFFER_SIZE]);
RtlBuffer 			IOFGetDeveloperDirectory(RtlBlock dir[BUFFER_SIZE]);
RtlBuffer 			IOFGetConfigurationDirectory(RtlBlock dir[BUFFER_SIZE]);
RtlBuffer 			IOFGetCurrentVolume(RtlBlock dir[BUFFER_SIZE]);
RtlBuffer 			IOFGetCurrentWorkDirectory(RtlBlock dir[BUFFER_SIZE]);
RtlBuffer 			IOFGetCacheDirectory(RtlBlock dir[BUFFER_SIZE]);
RtlBuffer 			IOFGetCoreServiceDirectory(RtlBlock dir[BUFFER_SIZE]);

RtlBuffer 			IOFGetAppRegisterDirectory(RtlBlock dir[BUFFER_SIZE]);
RtlBuffer 			IOFGetUsrRegisterDirectory(RtlBlock dir[BUFFER_SIZE]);
RtlBuffer 			IOFGetDevRegisterDirectory(RtlBlock dir[BUFFER_SIZE]);
RtlBuffer 			IOFGetLogDirectory(RtlBlock dir[BUFFER_SIZE]);
RtlBuffer 			IOFGetPreferenceDirectory(RtlBlock dir[BUFFER_SIZE]);

#endif
//////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////
//RtlUInteger	IORegisterApplication(RtlBuffer  application);
//IOERR	IOLockFile(RtlFileHandle handle);
//IOERR	IOUnlockFile(RtlFileHandle handle);
#ifdef __WIN32
#define IO_BASE_APPLICATION		0
#define IO_BASE_ROOT			1
#define IO_BASE_USER			2
#define IO_BASE_DEVELOPER		3
#define IO_BASE_CONFIGURATION	4
#define IO_BASE_CACHE			5
#define IO_BASE_CORESERVICE		6
#define IO_BASE_REG_APPLICATION	7
#define IO_BASE_REG_USER		8
#define IO_BASE_REG_DEVELOPER	9
#define IO_BASE_LOG				10
#define IO_BASE_PREFERENCE		11

#define IO_BASE_LIMIT			12
#define IO_CONFIGURATION_FILE	"Kit.release.configurations"
#define IO_DEFUALT_ROOT			"%r00t%"
//bool	initBasePathForIOKit(char IOBasePath[IO_BASE_LIMIT][BUFFER_SIZE]);
#endif

#ifdef __APPLE__
RtlExport NSArray* IOEnumObjectsAtPath(const RtlBuffer path);
RtlExport NSArray* IOEnumDirectoryAtPath(const RtlBuffer path);
RtlExport NSArray* IOEnumFileAtPath(const RtlBuffer path);

RtlExport NSArray* IOEnumSubpathAtPath(const RtlBuffer path);
RtlExport RtlBuffer IOGetCurrentBundlePath();
#elif __WIN32

#endif

#endif