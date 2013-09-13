//
//  RSBundlePrivate.h
//  RSCoreFoundation
//
//  Created by RetVal on 2/16/13.
//  Copyright (c) 2013 RetVal. All rights reserved.
//

#ifndef RSCoreFoundation_RSBundlePrivate_h
#define RSCoreFoundation_RSBundlePrivate_h
#include "RSBundle.h"
#include "RSBundlePrivateC0Context.h"

typedef struct _RSResourceData
{
    RSMutableDictionaryRef _stringTableCache;
    BOOL _executableLacksResourceFork;
    BOOL _infoDictionaryFromResourceFork;
    char _padding[2];
} _RSResourceData;

typedef struct __RSPlugInData
{
    BOOL _isPlugIn;
    BOOL _loadOnDemand;
    BOOL _isDoingDynamicRegistration;
    BOOL _unused1;
    RSBitU32 _instanceCount;
    RSMutableArrayRef _factories;
} _RSPlugInData;

#define _RSBundleSupportFilesDirectoryName1 RSSTR("Support Files")
#define _RSBundleSupportFilesDirectoryName2 RSSTR("Contents")
#define _RSBundleResourcesDirectoryName RSSTR("Resources")
#define _RSBundleExecutablesDirectoryName RSSTR("Executables")
#define _RSBundleNonLocalizedResourcesDirectoryName RSSTR("Non-localized Resources")

#define _RSBundleSupportFilesPathFromBase1 RSSTR("Support Files/")
#define _RSBundleSupportFilesPathFromBase2 RSSTR("Contents/")
#define _RSBundleResourcesPathFromBase0 RSSTR("Resources/")
#define _RSBundleResourcesPathFromBase1 RSSTR("Support Files/Resources/")
#define _RSBundleResourcesPathFromBase2 RSSTR("Contents/Resources/")
#define _RSBundleAppStoreReceiptPathFromBase0 RSSTR("_MASReceipt/receipt")
#define _RSBundleAppStoreReceiptPathFromBase1 RSSTR("Support Files/_MASReceipt/receipt")
#define _RSBundleAppStoreReceiptPathFromBase2 RSSTR("Contents/_MASReceipt/receipt")
#define _RSBundleExecutablesPathFromBase1 RSSTR("Support Files/Executables/")
#define _RSBundleExecutablesPathFromBase2 RSSTR("Contents/")
#define _RSBundleInfoPathFromBase0 RSSTR("Resources/Info.plist")
#define _RSBundleInfoPathFromBase1 RSSTR("Support Files/Info.plist")
#define _RSBundleInfoPathFromBase2 RSSTR("Contents/Info.plist")
#define _RSBundleInfoPathFromBase3 RSSTR("Info.plist")
#define _RSBundleInfoFileName RSSTR("Info.plist")

#define _RSBundleInfoPathFromRSBase0 RSSTR("Resources/Info.RS.plist")
#define _RSBundleInfoPathFromRSBase1 RSSTR("Support Files/Info.RS.plist")
#define _RSBundleInfoPathFromRSBase2 RSSTR("Contents/Info.RS.plist")
#define _RSBundleInfoPathFromRSBase3 RSSTR("Info.RS.plist")
#define _RSBundleInfoFileRSName RSSTR("Info.RS.plist")

#define _RSBundleInfoPathFromBaseNoExtension0 RSSTR("Resources/Info")
#define _RSBundleInfoPathFromBaseNoExtension1 RSSTR("Support Files/Info")
#define _RSBundleInfoPathFromBaseNoExtension2 RSSTR("Contents/Info")
#define _RSBundleInfoPathFromBaseNoExtension3 RSSTR("Info")
#define _RSBundleInfoExtension RSSTR("plist")
#define _RSBundleLocalInfoName RSSTR("InfoPlist")
#define _RSBundlePkgInfoPathFromBase1 RSSTR("Support Files/PkgInfo")
#define _RSBundlePkgInfoPathFromBase2 RSSTR("Contents/PkgInfo")
#define _RSBundlePseudoPkgInfoPathFromBase RSSTR("PkgInfo")
#define _RSBundlePrivateFrameworksPathFromBase0 RSSTR("Frameworks/")
#define _RSBundlePrivateFrameworksPathFromBase1 RSSTR("Support Files/Frameworks/")
#define _RSBundlePrivateFrameworksPathFromBase2 RSSTR("Contents/Frameworks/")
#define _RSBundleSharedFrameworksPathFromBase0 RSSTR("SharedFrameworks/")
#define _RSBundleSharedFrameworksPathFromBase1 RSSTR("Support Files/SharedFrameworks/")
#define _RSBundleSharedFrameworksPathFromBase2 RSSTR("Contents/SharedFrameworks/")
#define _RSBundleSharedSupportPathFromBase0 RSSTR("SharedSupport/")
#define _RSBundleSharedSupportPathFromBase1 RSSTR("Support Files/SharedSupport/")
#define _RSBundleSharedSupportPathFromBase2 RSSTR("Contents/SharedSupport/")
#define _RSBundleBuiltInPlugInsPathFromBase0 RSSTR("PlugIns/")
#define _RSBundleBuiltInPlugInsPathFromBase1 RSSTR("Support Files/PlugIns/")
#define _RSBundleBuiltInPlugInsPathFromBase2 RSSTR("Contents/PlugIns/")
#define _RSBundleAlternateBuiltInPlugInsPathFromBase0 RSSTR("Plug-ins/")
#define _RSBundleAlternateBuiltInPlugInsPathFromBase1 RSSTR("Support Files/Plug-ins/")
#define _RSBundleAlternateBuiltInPlugInsPathFromBase2 RSSTR("Contents/Plug-ins/")

#define _RSBundleLprojExtension RSSTR("lproj")
#define _RSBundleLprojExtensionWithDot RSSTR(".lproj")
#define _RSBundleDot RSSTR(".")
#define _RSBundleAllFiles RSSTR("_RSBAF_")
#define _RSBundleTypeIndicator RSSTR("_RSBT_")
// This directory contains resources (especially nibs) that may look up localized resources or may fall back to the development language resources
#define _RSBundleBaseDirectory RSSTR("Base")

#define _RSBundleMacOSXPlatformName RSSTR("macos")
#define _RSBundleAlternateMacOSXPlatformName RSSTR("macosx")
#define _RSBundleiPhoneOSPlatformName RSSTR("iphoneos")
#define _RSBundleMacOS8PlatformName RSSTR("macosclassic")
#define _RSBundleAlternateMacOS8PlatformName RSSTR("macos8")
#define _RSBundleWindowsPlatformName RSSTR("windows")
#define _RSBundleHPUXPlatformName RSSTR("hpux")
#define _RSBundleSolarisPlatformName RSSTR("solaris")
#define _RSBundleLinuxPlatformName RSSTR("linux")
#define _RSBundleFreeBSDPlatformName RSSTR("freebsd")

#define _RSBundleDefaultStringTableName RSSTR("Localizable")
#define _RSBundleStringTableType RSSTR("strings")
#define _RSBundleStringDictTableType RSSTR("stringsdict")

#define _RSBundleUserLanguagesPreferenceName RSSTR("AppleLanguages")
#define _RSBundleOldUserLanguagesPreferenceName RSSTR("NSLanguages")

#define _RSBundleLocalizedResourceForkFileName RSSTR("Localized")

#define _RSBundleWindowsResourceDirectoryExtension RSSTR("resources")

/* Old platform names (no longer used) */
#define _RSBundleMacOSXPlatformName_OLD RSSTR("macintosh")
#define _RSBundleAlternateMacOSXPlatformName_OLD RSSTR("nextstep")
#define _RSBundleWindowsPlatformName_OLD RSSTR("windows")
#define _RSBundleAlternateWindowsPlatformName_OLD RSSTR("winnt")

#define _RSBundleMacOSXInfoPlistPlatformName_OLD RSSTR("macos")
#define _RSBundleWindowsInfoPlistPlatformName_OLD RSSTR("win32")
typedef enum
{
    __RSBundleUnknownBinary,
    __RSBundleRSMBinary,
    __RSBundleDYLDExecutableBinary,
    __RSBundleDYLDBundleBinary,
    __RSBundleDYLDFrameworkBinary,
    __RSBundleDLLBinary,
    __RSBundleUnreadableBinary,
    __RSBundleNoBinary,
    __RSBundleELFBinary
}__RSPBinaryType;

typedef enum
{
    __RSBundleLoad = 1,
    
}__RSBundleCreateFlag;
RSPrivate void _RSBundleInitMainBundle();
RSPrivate void _RSBundleFinalMainBundle();
RSPrivate void __RSBundleGLock();
RSPrivate void __RSBundleGUnlock();
RSPrivate __RSPBinaryType _RSBundleGrokBinaryType(RSStringRef executablePath);
RSPrivate void _RSBundleAddToTables(RSBundleRef bundle, BOOL alreadyLocked);
RSPrivate void _RSBundleScheduleNeedUnload(BOOL alreadyLocked);
RSPrivate void _RSBundleDlfcnUnload(RSBundleRef bundle);
RSPrivate void _RSBundlePrivateCacheDeallocate(BOOL alreadyLocked);
RSPrivate RSBundleRef _RSMainBundle();
RSPrivate RSBundleRef _RSBundlePrimitiveGetBundleWithIdentifierAlreadyLocked(RSStringRef bundleID);
RSPrivate RSStringRef _RSBundleDYLDCopyLoadedImagePathForPointer(void *p);
#endif
