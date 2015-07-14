//
//  RSBundle.c
//  RSCoreFoundation
//
//  Created by RetVal on 2/11/13.
//  Copyright (c) 2013 RetVal. All rights reserved.
//
//  the check / load binary files' source code is inherited from Apple Inc, CoreFoundation/CFBundle*.c

#include "RSPrivate/RSBundle/RSBundlePrivate.h"
#include "RSPrivate/RSFileManager/RSFileManagerPathSupport.h"
#include "RSPrivate/RSPrivateOperatingSystem/RSPrivateFileSystem.h"
#include <RSCoreFoundation/RSArray.h>
#include <RSCoreFoundation/RSBundle.h>
#include <RSCoreFoundation/RSData.h>
#include <RSCoreFoundation/RSDictionary.h>
#include <RSCoreFoundation/RSError.h>
#include <RSCoreFoundation/RSFileManager.h>
#include <RSCoreFoundation/RSPropertyList.h>
#include <RSCoreFoundation/RSRuntime.h>
#include <RSCoreFoundation/RSString.h>
#include <RSCoreFoundation/RSSet.h>
#include <stdio.h>

#if defined(BINARY_SUPPORT_DYLD)
// Import the mach-o headers that define the macho magic numbers
#include <mach-o/loader.h>
#include <mach-o/fat.h>
#include <mach-o/arch.h>
#include <mach-o/dyld.h>
#include <mach-o/getsect.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <crt_externs.h>
#if defined(USE_DYLD_PRIV)
//#include <mach-o/dyld_priv.h>
#endif /* USE_DYLD_PRIV */
#endif /* BINARY_SUPPORT_DYLD */

#if defined(BINARY_SUPPORT_DLFCN)
#include <dlfcn.h>
#endif /* BINARY_SUPPORT_DLFCN */

#include <sys/stat.h>
#if DEPLOYMENT_TARGET_MACOSX || DEPLOYMENT_TARGET_EMBEDDED || DEPLOYMENT_TARGET_EMBEDDED_MINI
#include <fcntl.h>
#elif DEPLOYMENT_TARGET_WINDOWS
#include <fcntl.h>
#include <io.h>

#define open _RS_open
#define stat(x,y) _NS_stat(x,y)
#endif

#if DEPLOYMENT_TARGET_MACOSX || DEPLOYMENT_TARGET_EMBEDDED || DEPLOYMENT_TARGET_EMBEDDED_MINI || DEPLOYMENT_TARGET_LINUX
#include <unistd.h>
#include <sys/sysctl.h>
#include <sys/stat.h>
#include <dirent.h>
#endif

#if DEPLOYMENT_TARGET_WINDOWS
#define RS_OPENFLGS	(_O_BINARY|_O_NOINHERIT)
#else
#define RS_OPENFLGS	(0)
#endif

#if DEPLOYMENT_TARGET_WINDOWS
#define statinfo _stat

// Windows isspace implementation limits the input chars to < 256 in the ASCII range.  It will
// assert in debug builds.  This is annoying.  We merrily grok chars > 256.
static inline BOOL isspace(char c)
{
    return (c == ' ' || c == '\t' || c == '\n' || c == '\r'|| c == '\v' || c == '\f');
}

#else
#define statinfo stat
#endif

// Public RSBundle Info plist keys
RS_PUBLIC_CONST_STRING_DECL(RSBundleInfoDictionaryVersionKey, "RSBundleInfoDictionaryVersion")
RS_PUBLIC_CONST_STRING_DECL(RSBundleExecutableKey, "RSBundleExecutable")
RS_PUBLIC_CONST_STRING_DECL(RSBundleIdentifierKey, "RSBundleIdentifier")
RS_PUBLIC_CONST_STRING_DECL(RSBundleVersionKey, "RSBundleVersion")
RS_PUBLIC_CONST_STRING_DECL(RSBundleDevelopmentRegionKey, "RSBundleDevelopmentRegion")
RS_PUBLIC_CONST_STRING_DECL(RSBundleLocalizationsKey, "RSBundleLocalizations")

// Private RSBundle Info plist keys, possible candidates for public constants
RS_CONST_STRING_DECL(_RSBundleAllowMixedLocalizationsKey, "RSBundleAllowMixedLocalizations")
RS_CONST_STRING_DECL(_RSBundleSupportedPlatformsKey, "RSBundleSupportedPlatforms")
RS_CONST_STRING_DECL(_RSBundleResourceSpecificationKey, "RSBundleResourceSpecification")

// Finder stuff
RS_CONST_STRING_DECL(_RSBundlePackageTypeKey, "RSBundlePackageType")
RS_CONST_STRING_DECL(_RSBundleSignatureKey, "RSBundleSignature")
RS_CONST_STRING_DECL(_RSBundleIconFileKey, "RSBundleIconFile")
RS_CONST_STRING_DECL(_RSBundleDocumentTypesKey, "RSBundleDocumentTypes")
RS_CONST_STRING_DECL(_RSBundlePathTypesKey, "RSBundlePathTypes")

// Keys that are usually localized in InfoPlist.strings
RS_CONST_STRING_DECL(RSBundleNameKey, "RSBundleName")
RS_CONST_STRING_DECL(_RSBundleDisplayNameKey, "RSBundleDisplayName")
RS_CONST_STRING_DECL(_RSBundleShortVersionStringKey, "RSBundleShortVersionString")
RS_CONST_STRING_DECL(_RSBundleGetInfoStringKey, "RSBundleGetInfoString")
RS_CONST_STRING_DECL(_RSBundleGetInfoHTMLKey, "RSBundleGetInfoHTML")

// Sub-keys for RSBundleDocumentTypes dictionaries
RS_CONST_STRING_DECL(_RSBundleTypeNameKey, "RSBundleTypeName")
RS_CONST_STRING_DECL(_RSBundleTypeRoleKey, "RSBundleTypeRole")
RS_CONST_STRING_DECL(_RSBundleTypeIconFileKey, "RSBundleTypeIconFile")
RS_CONST_STRING_DECL(_RSBundleTypeOSTypesKey, "RSBundleTypeOSTypes")
RS_CONST_STRING_DECL(_RSBundleTypeExtensionsKey, "RSBundleTypeExtensions")
RS_CONST_STRING_DECL(_RSBundleTypeMIMETypesKey, "RSBundleTypeMIMETypes")

// Sub-keys for RSBundlePathTypes dictionaries
RS_CONST_STRING_DECL(_RSBundlePathNameKey, "RSBundlePathName")
RS_CONST_STRING_DECL(_RSBundlePathIconFileKey, "RSBundlePathIconFile")
RS_CONST_STRING_DECL(_RSBundlePathSchemesKey, "RSBundlePathSchemes")

// Compatibility key names
RS_CONST_STRING_DECL(_RSBundleOldExecutableKey, "RSExecutable")
RS_CONST_STRING_DECL(_RSBundleOldInfoDictionaryVersionKey, "RSInfoPlistVersion")
RS_CONST_STRING_DECL(_RSBundleOldNameKey, "RSHumanReadableName")
RS_CONST_STRING_DECL(_RSBundleOldIconFileKey, "RSIcon")
RS_CONST_STRING_DECL(_RSBundleOldDocumentTypesKey, "RSTypes")
RS_CONST_STRING_DECL(_RSBundleOldShortVersionStringKey, "RSAppVersion")

// Compatibility RSBundleDocumentTypes key names
RS_CONST_STRING_DECL(_RSBundleOldTypeNameKey, "RSName")
RS_CONST_STRING_DECL(_RSBundleOldTypeRoleKey, "RSRole")
RS_CONST_STRING_DECL(_RSBundleOldTypeIconFileKey, "RSIcon")
RS_CONST_STRING_DECL(_RSBundleOldTypeExtensions1Key, "RSUnixExtensions")
RS_CONST_STRING_DECL(_RSBundleOldTypeExtensions2Key, "RSDOSExtensions")
RS_CONST_STRING_DECL(_RSBundleOldTypeOSTypesKey, "RSMacOSType")

// Internally used keys for loaded Info plists.
RS_CONST_STRING_DECL(_RSBundleInfoPlistPathKey, "RSBundleInfoPlistPath")
RS_CONST_STRING_DECL(_RSBundleRawInfoPlistPathKey, "RSBundleRawInfoPlistPath")
RS_CONST_STRING_DECL(_RSBundleNumericVersionKey, "RSBundleNumericVersion")
RS_CONST_STRING_DECL(_RSBundleExecutablePathKey, "RSBundleExecutablePath")
RS_CONST_STRING_DECL(_RSBundleExecutableCFNameKey, "CFBundleExecutable")
RS_CONST_STRING_DECL(_RSBundleIdentifierCFNameKey, "CFBundleIdentifier")
RS_CONST_STRING_DECL(_RSBundleExecutableRSNameKey, "RSBundleExecutable")
RS_CONST_STRING_DECL(_RSBundleResourcesFileMappedKey, "RSResourcesFileMapped")
RS_CONST_STRING_DECL(_RSBundleRSMLoadAsBundleKey, "RSBundleRSMLoadAsBundle")

RS_CONST_STRING_DECL(_RSBundleHasSpeicificInfoKey, "RSBundleHasSpeicificInfo")

// Keys used by RSBundle for loaded Info plists.
RS_CONST_STRING_DECL(_RSBundleInitialPathKey, "RSBundleInitialPath")
RS_CONST_STRING_DECL(_RSBundleResolvedPathKey, "RSBundleResolvedPath")
RS_CONST_STRING_DECL(_RSBundlePrincipalClassKey, "RSPrincipalClass")

#define UNKNOWN_FILETYPE 0x0
#define PEF_FILETYPE 0x1000
#define PEF_MAGIC 0x4a6f7921
#define PEF_CIGAM 0x21796f4a
#define TEXT_SEGMENT "__TEXT"
#define PLIST_SECTION "__info_plist"
#define OBJC_SEGMENT "__OBJC"
#define IMAGE_INFO_SECTION "__image_info"
#define OBJC_SEGMENT_64 "__DATA"
#define IMAGE_INFO_SECTION_64 "__objc_imageinfo"
#define LIB_X11 "/usr/X11R6/lib/libX"

#define XLS_NAME "Book"
#define XLS_NAME2 "Workbook"
#define DOC_NAME "WordDocument"
#define PPT_NAME "PowerPoint Document"

#define ustrncmp(x, y, z) strncmp((char *)(x), (char *)(y), (z))
#define ustrncasecmp(x, y, z) strncasecmp_l((char *)(x), (char *)(y), (z), nil)

static const uint32_t __RSBundleMagicNumbersArray[] = {
    0xcafebabe, 0xbebafeca, 0xfeedface, 0xcefaedfe, 0xfeedfacf, 0xcffaedfe, 0x4a6f7921, 0x21796f4a,
    0x7f454c46, 0xffd8ffe0, 0x4d4d002a, 0x49492a00, 0x47494638, 0x89504e47, 0x69636e73, 0x00000100,
    0x7b5c7274, 0x25504446, 0x2e7261fd, 0x2e524d46, 0x2e736e64, 0x2e736400, 0x464f524d, 0x52494646,
    0x38425053, 0x000001b3, 0x000001ba, 0x4d546864, 0x504b0304, 0x53495421, 0x53495432, 0x53495435,
    0x53495444, 0x53747566, 0x30373037, 0x3c212d2d, 0x25215053, 0xd0cf11e0, 0x62656769, 0x3d796265,
    0x6b6f6c79, 0x3026b275, 0x0000000c, 0xfe370023, 0x09020600, 0x09040600, 0x4f676753, 0x664c6143,
    0x00010000, 0x74727565, 0x4f54544f, 0x41433130, 0xc809fe02, 0x0809fe02, 0x2356524d, 0x67696d70,
    0x3c435058, 0x28445746, 0x424f4d53, 0x49544f4c, 0x72746664, 0x63616666, 0x802a5fd7, 0x762f3101
};

// string, with groups of 5 characters being 1 element in the array
static const char * __RSBundleExtensionsArray =
"mach\0"  "mach\0"  "mach\0"  "mach\0"  "mach\0"  "mach\0"  "pef\0\0" "pef\0\0"
"elf\0\0" "jpeg\0"  "tiff\0"  "tiff\0"  "gif\0\0" "png\0\0" "icns\0"  "ico\0\0"
"rtf\0\0" "pdf\0\0" "ra\0\0\0""rm\0\0\0""au\0\0\0""au\0\0\0""iff\0\0" "riff\0"
"psd\0\0" "mpeg\0"  "mpeg\0"  "mid\0\0" "zip\0\0" "sit\0\0" "sit\0\0" "sit\0\0"
"sit\0\0" "sit\0\0" "cpio\0"  "html\0"  "ps\0\0\0""ole\0\0" "uu\0\0\0""ync\0\0"
"dmg\0\0" "wmv\0\0" "jp2\0\0" "doc\0\0" "xls\0\0" "xls\0\0" "ogg\0\0" "flac\0"
"ttf\0\0" "ttf\0\0" "otf\0\0" "dwg\0\0" "dgn\0\0" "dgn\0\0" "wrl\0\0" "xcf\0\0"
"cpx\0\0" "dwf\0\0" "bom\0\0" "lit\0\0" "rtfd\0"  "caf\0\0" "cin\0\0" "exr\0\0";

typedef enum {
    _RSBundleAllContents = 0,
    _RSBundleDirectoryContents = 1,
    _RSBundleUnknownContents = 2
} _RSBundleDirectoryContentsType;

static BOOL _RSAllocatorIsGCRefZero(RSAllocatorRef allocator)
{
    return NO;
}

static const char * __RSBundleOOExtensionsArray = "sxc\0\0" "sxd\0\0" "sxg\0\0" "sxi\0\0" "sxm\0\0" "sxw\0\0";
static const char * __RSBundleODExtensionsArray = "odc\0\0" "odf\0\0" "odg\0\0" "oth\0\0" "odi\0\0" "odm\0\0" "odp\0\0" "ods\0\0" "odt\0\0";

#define _RSBundleNumberOfPlatforms 7
static RSStringRef _RSBundleSupportedPlatforms[_RSBundleNumberOfPlatforms] = { nil, nil, nil, nil, nil, nil, nil };
static const char *_RSBundleSupportedPlatformStrings[_RSBundleNumberOfPlatforms] = { "iphoneos", "macos", "windows", "linux", "freebsd", "solaris", "hpux" };

#define _RSBundleNumberOfProducts 3
static RSStringRef _RSBundleSupportedProducts[_RSBundleNumberOfProducts] = { nil, nil, nil };
static const char *_RSBundleSupportedProductStrings[_RSBundleNumberOfProducts] = { "iphone", "ipod", "ipad" };

#define _RSBundleNumberOfiPhoneOSPlatformProducts 3
static RSStringRef _RSBundleSupportediPhoneOSPlatformProducts[_RSBundleNumberOfiPhoneOSPlatformProducts] = { nil, nil, nil };
static const char *_RSBundleSupportediPhoneOSPlatformProductStrings[_RSBundleNumberOfiPhoneOSPlatformProducts] = { "iphone", "ipod", "ipad" };

RSPrivate void _RSBundleResourcesInitialize()
{
    for (unsigned int i = 0; i < _RSBundleNumberOfPlatforms; i++) _RSBundleSupportedPlatforms[i] = RSStringCreateWithCString(RSAllocatorSystemDefault, _RSBundleSupportedPlatformStrings[i], RSStringEncodingUTF8);
    
    for (unsigned int i = 0; i < _RSBundleNumberOfProducts; i++) _RSBundleSupportedProducts[i] = RSStringCreateWithCString(RSAllocatorSystemDefault, _RSBundleSupportedProductStrings[i], RSStringEncodingUTF8);
    
    for (unsigned int i = 0; i < _RSBundleNumberOfiPhoneOSPlatformProducts; i++) _RSBundleSupportediPhoneOSPlatformProducts[i] = RSStringCreateWithCString(RSAllocatorSystemDefault, _RSBundleSupportediPhoneOSPlatformProductStrings[i], RSStringEncodingUTF8);
}

#define EXTENSION_LENGTH                5
#define NUM_EXTENSIONS                  64
#define MAGIC_BYTES_TO_READ             512
#define DMG_BYTES_TO_READ               512
#define ZIP_BYTES_TO_READ               1024
#define OLE_BYTES_TO_READ               512
#define X11_BYTES_TO_READ               4096
#define IMAGE_INFO_BYTES_TO_READ        4096

struct __RSBundle
{
    RSRuntimeBase _base;
    
    RSStringRef _path;                   // full path for file system include bundle name.
    RSDictionaryRef _infoDict;          // info.plist.
    RSDictionaryRef _localizeInfoDict;  // Resources/info.plist.
    RSArrayRef _supportLanguages;       // *.lproj.

    __RSPBinaryType _bundleType;   // the type of bundle.
    RSBitU8 _version;                   // version of bundle.
    BOOL _isLoaded;                     // loaded = YES.
    BOOL _sharing;                      // sharing = YES.
    char _padding1[1];
    
    /* RSM goop */
    RSHandle _connectionCookie;
    
    /* DYLD goop */
    RSHandle _imageCookie;
    RSHandle _moduleCookie;
    
    /* dlfcn goop */
    RSHandle _handleCookie;

    pthread_mutex_t _bundleLoadingLock; // locking itself.
    
    _RSResourceData _resourceData;
    RSSpinLock _queryTableLock;         // locking the querytable.
    RSMutableDictionaryRef _queryTable; //
    
    //RSStringRef _bundleBasePath;        // the base path of bundle, without the bundle name.
    RSArrayRef _subBundles;             // the bundle's plugins.
};

static SInt32 _RSBundleCurrentArchitecture(void)
{
    SInt32 arch = 0;
#if defined(__ppc__)
    arch = RSBundleExecutableArchitecturePPC;
#elif defined(__ppc64__)
    arch = RSBundleExecutableArchitecturePPC64;
#elif defined(__i386__)
    arch = RSBundleExecutableArchitectureI386;
#elif defined(__x86_64__)
    arch = RSBundleExecutableArchitectureX86_64;
#elif defined(BINARY_SUPPORT_DYLD)
    const NXArchInfo *archInfo = NXGetLocalArchInfo();
    if (archInfo) arch = archInfo->cputype;
#endif
    return arch;
}


RSExport RSStringRef _RSGetPlatformName(void)
{
#if DEPLOYMENT_TARGET_MACOSX
    return _RSBundleMacOSXPlatformName;
#elif DEPLOYMENT_TARGET_EMBEDDED || DEPLOYMENT_TARGET_EMBEDDED_MINI
    return _RSBundleiPhoneOSPlatformName;
#elif DEPLOYMENT_TARGET_WINDOWS
    return _RSBundleWindowsPlatformName;
#elif DEPLOYMENT_TARGET_SOLARIS
    return _RSBundleSolarisPlatformName;
#elif DEPLOYMENT_TARGET_HPUX
    return _RSBundleHPUXPlatformName;
#elif DEPLOYMENT_TARGET_LINUX
    return _RSBundleLinuxPlatformName;
#elif DEPLOYMENT_TARGET_FREEBSD
    return _RSBundleFreeBSDPlatformName;
#else
#error Unknown or unspecified DEPLOYMENT_TARGET
#endif
}

RSExport RSStringRef _RSGetAlternatePlatformName(void) {
#if DEPLOYMENT_TARGET_MACOSX
    return _RSBundleAlternateMacOSXPlatformName;
#elif DEPLOYMENT_TARGET_EMBEDDED || DEPLOYMENT_TARGET_EMBEDDED_MINI
    return _RSBundleMacOSXPlatformName;
#elif DEPLOYMENT_TARGET_WINDOWS
    return _RSBundleWindowsPlatformName;
#else
#error Unknown or unspecified DEPLOYMENT_TARGET
#endif
}

RSExport RSStringRef _RSGetProductName(void)
{
#if DEPLOYMENT_TARGET_EMBEDDED || DEPLOYMENT_TARGET_EMBEDDED_MINI
    if (!platform)
    {
        char buffer[256];
        memset(buffer, 0, sizeof(buffer));
        size_t buflen = sizeof(buffer);
        int ret = sysctlbyname("hw.machine", buffer, &buflen, nil, 0);
        if (0 == ret || (-1 == ret && ENOMEM == errno))
        {
            if (6 <= buflen && 0 == memcmp(buffer, "iPhone", 6))
            {
                platform = RSSTR("iphone");
            }
            else if (4 <= buflen && 0 == memcmp(buffer, "iPod", 4))
            {
                platform = RSSTR("ipod");
            }
            else if (4 <= buflen && 0 == memcmp(buffer, "iPad", 4))
            {
                platform = RSSTR("ipad");
            }
            else
            {
                const char *env = __RSRuntimeGetEnvironment("IPHONE_SIMULATOR_DEVICE");
                if (env)
                {
                    if (0 == strcmp(env, "iPhone"))
                    {
                        platform = RSSTR("iphone");
                    }
                    else if (0 == strcmp(env, "iPad"))
                    {
                        platform = RSSTR("ipad");
                    }
                    else
                    {
                        // fallback, unrecognized IPHONE_SIMULATOR_DEVICE
                    }
                }
                else
                {
                    // fallback, unrecognized hw.machine and no IPHONE_SIMULATOR_DEVICE
                }
            }
        }
        if (!platform) platform = RSSTR("iphone"); // fallback
    }
    return platform;
#endif
    return RSSTR("");
}

static UniChar *_AppSupportUniChars1 = nil;
static RSIndex _AppSupportLen1 = 0;
static UniChar *_AppSupportUniChars2 = nil;
static RSIndex _AppSupportLen2 = 0;
static UniChar *_ResourcesUniChars = nil;
static RSIndex _ResourcesLen = 0;
static UniChar *_PlatformUniChars = nil;
static RSIndex _PlatformLen = 0;
static UniChar *_AlternatePlatformUniChars = nil;
static RSIndex _AlternatePlatformLen = 0;
static UniChar *_LprojUniChars = nil;
static RSIndex _LprojLen = 0;
static UniChar *_BaseUniChars = nil;
static RSIndex _BaseLen;
static UniChar *_GlobalResourcesUniChars = nil;
static RSIndex _GlobalResourcesLen = 0;
static UniChar *_InfoExtensionUniChars = nil;
static RSIndex _InfoExtensionLen = 0;

#if 0
static UniChar _ResourceSuffix3[32];
static RSIndex _ResourceSuffix3Len = 0;
#endif
static UniChar _ResourceSuffix2[16];
static RSIndex _ResourceSuffix2Len = 0;
static UniChar _ResourceSuffix1[16];
static RSIndex _ResourceSuffix1Len = 0;

static void _RSBundleInitStaticUniCharBuffers(void) {
    RSStringRef appSupportStr1 = _RSBundleSupportFilesDirectoryName1;
    RSStringRef appSupportStr2 = _RSBundleSupportFilesDirectoryName2;
    RSStringRef resourcesStr = _RSBundleResourcesDirectoryName;
    RSStringRef platformStr = _RSGetPlatformName();
    RSStringRef alternatePlatformStr = _RSGetAlternatePlatformName();
    RSStringRef lprojStr = _RSBundleLprojExtension;
    RSStringRef globalResourcesStr = _RSBundleNonLocalizedResourcesDirectoryName;
    RSStringRef infoExtensionStr = _RSBundleInfoExtension;
    RSStringRef baseStr = _RSBundleBaseDirectory;
    
    _AppSupportLen1 = RSStringGetLength(appSupportStr1);
    _AppSupportLen2 = RSStringGetLength(appSupportStr2);
    _ResourcesLen = RSStringGetLength(resourcesStr);
    _PlatformLen = RSStringGetLength(platformStr);
    _AlternatePlatformLen = RSStringGetLength(alternatePlatformStr);
    _LprojLen = RSStringGetLength(lprojStr);
    _GlobalResourcesLen = RSStringGetLength(globalResourcesStr);
    _InfoExtensionLen = RSStringGetLength(infoExtensionStr);
    _BaseLen = RSStringGetLength(baseStr);
    
    _AppSupportUniChars1 = (UniChar *)RSAllocatorAllocate(RSAllocatorSystemDefault, sizeof(UniChar) * (_AppSupportLen1 + _AppSupportLen2 + _ResourcesLen + _PlatformLen + _AlternatePlatformLen + _LprojLen + _GlobalResourcesLen + _InfoExtensionLen + _BaseLen));
    if (_AppSupportUniChars1) __RSAutoReleaseISA(RSAllocatorSystemDefault, _AppSupportUniChars1);
    _AppSupportUniChars2 = _AppSupportUniChars1 + _AppSupportLen1;
    _ResourcesUniChars = _AppSupportUniChars2 + _AppSupportLen2;
    _PlatformUniChars = _ResourcesUniChars + _ResourcesLen;
    _AlternatePlatformUniChars = _PlatformUniChars + _PlatformLen;
    _LprojUniChars = _AlternatePlatformUniChars + _AlternatePlatformLen;
    _GlobalResourcesUniChars = _LprojUniChars + _LprojLen;
    _InfoExtensionUniChars = _GlobalResourcesUniChars + _GlobalResourcesLen;
    _BaseUniChars = _InfoExtensionUniChars + _InfoExtensionLen;
    
    if (_AppSupportLen1 > 0) RSStringGetCharacters(appSupportStr1, RSMakeRange(0, _AppSupportLen1), (UniChar *)_AppSupportUniChars1);
    if (_AppSupportLen2 > 0) RSStringGetCharacters(appSupportStr2, RSMakeRange(0, _AppSupportLen2), (UniChar *)_AppSupportUniChars2);
    if (_ResourcesLen > 0) RSStringGetCharacters(resourcesStr, RSMakeRange(0, _ResourcesLen), (UniChar *)_ResourcesUniChars);
    if (_PlatformLen > 0) RSStringGetCharacters(platformStr, RSMakeRange(0, _PlatformLen), (UniChar *)_PlatformUniChars);
    if (_AlternatePlatformLen > 0) RSStringGetCharacters(alternatePlatformStr, RSMakeRange(0, _AlternatePlatformLen), (UniChar *)_AlternatePlatformUniChars);
    if (_LprojLen > 0) RSStringGetCharacters(lprojStr, RSMakeRange(0, _LprojLen), (UniChar *)_LprojUniChars);
    if (_GlobalResourcesLen > 0) RSStringGetCharacters(globalResourcesStr, RSMakeRange(0, _GlobalResourcesLen), (UniChar *)_GlobalResourcesUniChars);
    if (_InfoExtensionLen > 0) RSStringGetCharacters(infoExtensionStr, RSMakeRange(0, _InfoExtensionLen), (UniChar *)_InfoExtensionUniChars);
    if (_BaseLen > 0) RSStringGetCharacters(baseStr, RSMakeRange(0, _BaseLen), (UniChar *)_BaseUniChars);
    
    _ResourceSuffix1Len = RSStringGetLength(platformStr);
    if (_ResourceSuffix1Len > 0) _ResourceSuffix1[0] = '-';
    if (_ResourceSuffix1Len > 0) RSStringGetCharacters(platformStr, RSMakeRange(0, _ResourceSuffix1Len), (UniChar *)_ResourceSuffix1 + 1);
    if (_ResourceSuffix1Len > 0) _ResourceSuffix1Len++;
    RSStringRef productStr = _RSGetProductName();
    if (RSEqual(productStr, RSSTR("ipod"))) { // For now, for resource lookups, hide ipod distinction and make it look for iphone resources
        productStr = RSSTR("iphone");
    }
    _ResourceSuffix2Len = RSStringGetLength(productStr);
    if (_ResourceSuffix2Len > 0) _ResourceSuffix2[0] = '~';
    if (_ResourceSuffix2Len > 0) RSStringGetCharacters(productStr, RSMakeRange(0, _ResourceSuffix2Len), (UniChar *)_ResourceSuffix2 + 1);
    if (_ResourceSuffix2Len > 0) _ResourceSuffix2Len++;
#if 0
    if (_ResourceSuffix1Len > 1 && _ResourceSuffix2Len > 1) {
        _ResourceSuffix3Len = _ResourceSuffix1Len + _ResourceSuffix2Len;
        memmove(_ResourceSuffix3, _ResourceSuffix1, sizeof(UniChar) * _ResourceSuffix1Len);
        memmove(_ResourceSuffix3 + _ResourceSuffix1Len, _ResourceSuffix2, sizeof(UniChar) * _ResourceSuffix2Len);
    }
#endif
}

RSInline BOOL __RSBundlePathIsPath(RSBundleRef rs)
{
    return __RSBitfieldGetValue(RSRuntimeClassBaseFiled(rs), 1, 1);
}

RSInline BOOL __RSBundlePathIsName(RSBundleRef rs)
{
    return __RSBundlePathIsPath(rs);
}

RSInline void __RSBundleSetPathIsPath(RSBundleRef rs)
{
    __RSBitfieldSetValue(RSRuntimeClassBaseFiled(rs), 1, 1, 1);
}

RSInline void __RSBundleSetPathIsName(RSBundleRef rs)
{
    __RSBitfieldSetValue(RSRuntimeClassBaseFiled(rs), 1, 1, 0);
}


static void __RSBundleClassInit(RSTypeRef rs)
{
    RSBundleRef bundle = (RSBundleRef)rs;
    bundle->_padding1[0] = 63;
    bundle->_isLoaded = NO;
    pthread_mutex_init(&bundle->_bundleLoadingLock, nil);
    bundle->_bundleType = __RSBundleUnknownBinary;
    
    bundle->_handleCookie = nil;
    bundle->_connectionCookie = nil;
    bundle->_imageCookie = nil;
    bundle->_moduleCookie = nil;
}

static RSTypeRef __RSBundleClassCopy(RSAllocatorRef allocator, RSTypeRef rs, BOOL mutableCopy)
{
    return nil;
}

static void __RSBundleClassDeallocate(RSTypeRef rs)
{
    RSBundleRef bundle = (RSBundleRef)rs;
    pthread_mutex_lock(&bundle->_bundleLoadingLock);
    if (bundle->_path) RSRelease(bundle->_path);
    if (bundle->_infoDict) RSRelease(bundle->_infoDict);
    if (bundle->_localizeInfoDict) RSRelease(bundle->_localizeInfoDict);
    if (bundle->_supportLanguages) RSRelease(bundle->_supportLanguages);
    RSSpinLockLock(&bundle->_queryTableLock);
    if (bundle->_queryTable) RSRelease(bundle->_queryTable);
    RSSpinLockUnlock(&bundle->_queryTableLock);
    //if (bundle->_bundleBasePath) RSRelease(bundle->_bundleBasePath);
    if (bundle->_subBundles) RSRelease(bundle->_subBundles);
    pthread_mutex_unlock(&bundle->_bundleLoadingLock);
    pthread_mutex_destroy(&bundle->_bundleLoadingLock);
//    unsigned long long tick = 0;
//    __asm__ __volatile__ ("rdtsc" : "=A"(tick));
}

static RSStringRef __RSBundleClassDescription(RSTypeRef rs)
{
    RSBundleRef bundle = (RSBundleRef)rs;
    RSStringRef description = nil, binaryType = nil;
    switch (bundle->_bundleType)
    {
        case __RSBundleDYLDExecutableBinary:
            binaryType = RSSTR("executable, ");
            break;
        case __RSBundleDYLDBundleBinary:
            binaryType = RSSTR("bundle, ");
            break;
        case __RSBundleDYLDFrameworkBinary:
            binaryType = RSSTR("framework, ");
            break;
        case __RSBundleDLLBinary:
            binaryType = RSSTR("DLL, ");
            break;
        case __RSBundleUnreadableBinary:
        case __RSBundleRSMBinary:
        default:
            binaryType = RSSTR("");
            break;
    }
    return description = RSStringCreateWithFormat(RSAllocatorSystemDefault,
                                                  RSSTR("RSBundle %p <%R> (%R%R loaded)"),
                                                  rs,
                                                  bundle->_path,
                                                  binaryType,
                                                  bundle->_isLoaded ? RSSTR("") : RSSTR("not"));
}

static RSRuntimeClass __RSBundleClass =
{
    _RSRuntimeScannedObject,
    0,
    "RSBundle",
    __RSBundleClassInit,
    __RSBundleClassCopy,
    __RSBundleClassDeallocate,
    nil,
    nil,
    __RSBundleClassDescription,
    nil,
    nil
};

static RSTypeID _RSBundleTypeID = _RSRuntimeNotATypeID;
RSPrivate void __RSBundleInitialize()
{
    _RSBundleInitStaticUniCharBuffers();
    _RSBundleTypeID = __RSRuntimeRegisterClass(&__RSBundleClass);
    __RSRuntimeSetClassTypeID(&__RSBundleClass, _RSBundleTypeID);
    _RSBundleInitMainBundle();
}

RSPrivate void __RSBundleDeallocate()
{
//    _RSBundleFinalMainBundle();
    _RSBundlePrivateCacheDeallocate(NO);
//    if (_AppSupportUniChars1) RSAllocatorDeallocate(RSAllocatorSystemDefault, _AppSupportUniChars1);
    _AppSupportUniChars1 = nil;
}

RSExport RSTypeID RSBundleGetTypeID()
{
    return _RSBundleTypeID;
}

static BOOL _isValidPlatformSuffix(RSStringRef suffix)
{
    for (RSIndex idx = 0; idx < _RSBundleNumberOfPlatforms; idx++)
    {
        if (RSEqual(suffix, _RSBundleSupportedPlatforms[idx])) return YES;
    }
    return NO;
}

static BOOL _isValidProductSuffix(RSStringRef suffix)
{
    for (RSIndex idx = 0; idx < _RSBundleNumberOfProducts; idx++)
    {
        if (RSEqual(suffix, _RSBundleSupportedProducts[idx])) return YES;
    }
    return NO;
}

static BOOL _isValidiPhoneOSPlatformProductSuffix(RSStringRef suffix)
{
    for (RSIndex idx = 0; idx < _RSBundleNumberOfiPhoneOSPlatformProducts; idx++)
    {
        if (RSEqual(suffix, _RSBundleSupportediPhoneOSPlatformProducts[idx])) return YES;
    }
    return NO;
}

static BOOL _isValidPlatformAndProductSuffixPair(RSStringRef platform, RSStringRef product)
{
    if (!platform && !product) return YES;
    if (!platform)
    {
        return _isValidProductSuffix(product);
    }
    if (!product)
    {
        return _isValidPlatformSuffix(platform);
    }
    if (RSEqual(platform, _RSBundleiPhoneOSPlatformName))
    {
        return _isValidiPhoneOSPlatformProductSuffix(product);
    }
    return NO;
}

static BOOL _isBlacklistedKey(RSStringRef keyName)
{
#define _RSBundleNumberOfBlacklistedInfoDictionaryKeys 2
    RSStringRef const _RSBundleBlacklistedInfoDictionaryKeys[_RSBundleNumberOfBlacklistedInfoDictionaryKeys] = {RSBundleExecutableKey, RSBundleIdentifierKey};
    for (RSIndex idx = 0; idx < _RSBundleNumberOfBlacklistedInfoDictionaryKeys; idx++)
    {
        if (RSEqual(keyName, _RSBundleBlacklistedInfoDictionaryKeys[idx]))
            return YES;
    }
    return NO;
}

static BOOL _isOverrideKey(RSStringRef fullKey, RSStringRef *outBaseKey, RSStringRef *outPlatformSuffix, RSStringRef *outProductSuffix)
{
    if (outBaseKey)
    {
        *outBaseKey = nil;
    }
    if (outPlatformSuffix)
    {
        *outPlatformSuffix = nil;
    }
    if (outProductSuffix)
    {
        *outProductSuffix = nil;
    }
    if (!fullKey)
        return NO;
    RSRange minusRange; //RSStringFind(fullKey, RSSTR("-"), RSStringGetRange(fullKey), &minusRange); //= RSStringFind(fullKey, RSSTR("-"), RSCompareBackwards);
    RSStringFindWithOptions(fullKey, RSSTR("-"), RSStringGetRange(fullKey), RSCompareBackwards, &minusRange);
    RSRange tildeRange ;//= RSStringFind(fullKey, RSSTR("~"), RSCompareBackwards);
    RSStringFindWithOptions(fullKey, RSSTR("~"), RSStringGetRange(fullKey), RSCompareBackwards, &tildeRange);
    //RSStringFind(fullKey, RSSTR("~"), RSStringGetRange(fullKey), &tildeRange);
    if (minusRange.location == RSNotFound && tildeRange.location == RSNotFound) return NO;
    // minus must come before tilde if both are present
    if (minusRange.location != RSNotFound && tildeRange.location != RSNotFound && tildeRange.location <= minusRange.location) return NO;
    
    RSIndex strLen = RSStringGetLength(fullKey);
    RSRange baseKeyRange = (minusRange.location != RSNotFound) ? RSMakeRange(0, minusRange.location) : RSMakeRange(0, tildeRange.location);
    RSRange platformRange = RSMakeRange(RSNotFound, 0);
    RSRange productRange = RSMakeRange(RSNotFound, 0);
    if (minusRange.location != RSNotFound)
    {
        platformRange.location = minusRange.location + minusRange.length;
        platformRange.length = ((tildeRange.location != RSNotFound) ? tildeRange.location : strLen) - platformRange.location;
    }
    if (tildeRange.location != RSNotFound)
    {
        productRange.location = tildeRange.location + tildeRange.length;
        productRange.length = strLen - productRange.location;
    }
    if (baseKeyRange.length < 1) return NO;
    if (platformRange.location != RSNotFound && platformRange.length < 1) return NO;
    if (productRange.location != RSNotFound && productRange.length < 1) return NO;
    
    RSStringRef platform = (platformRange.location != RSNotFound) ? RSStringCreateSubStringWithRange(RSAllocatorSystemDefault, fullKey, platformRange) : nil;
    RSStringRef product = (productRange.location != RSNotFound) ? RSStringCreateSubStringWithRange(RSAllocatorSystemDefault, fullKey, productRange) : nil;
    BOOL result = _isValidPlatformAndProductSuffixPair(platform, product);
    
    if (result)
    {
        if (outBaseKey)
        {
            *outBaseKey = RSStringCreateSubStringWithRange(RSAllocatorSystemDefault, fullKey, baseKeyRange);
        }
        if (outPlatformSuffix)
        {
            *outPlatformSuffix = platform;
        }
        else
        {
            if (platform && !_RSAllocatorIsGCRefZero(RSAllocatorSystemDefault)) RSRelease(platform);
        }
        if (outProductSuffix)
        {
            *outProductSuffix = product;
        }
        else
        {
            if (product && !_RSAllocatorIsGCRefZero(RSAllocatorSystemDefault)) RSRelease(product);
        }
    }
    else
    {
        if (platform && !_RSAllocatorIsGCRefZero(RSAllocatorSystemDefault)) RSRelease(platform);
        if (product && !_RSAllocatorIsGCRefZero(RSAllocatorSystemDefault)) RSRelease(product);
    }
    return result;
}

RSInline uint32_t _RSBundleSwapInt32Conditional(uint32_t arg, BOOL swap) {return swap ? RSSwapInt32(arg) : arg;}
RSInline uint64_t _RSBundleSwapInt64Conditional(uint64_t arg, BOOL swap) {return swap ? RSSwapInt64(arg) : arg;}

static BOOL _isCurrentPlatformAndProduct(RSStringRef platform, RSStringRef product)
{
    if (!platform && !product) return YES;
    if (!platform)
    {
        return RSEqual(_RSGetProductName(), product);
    }
    if (!product)
    {
        return RSEqual(_RSGetPlatformName(), platform);
    }
    
    return RSEqual(_RSGetProductName(), product) && RSEqual(_RSGetPlatformName(), platform);
}

static RSArrayRef _CopySortedOverridesForBaseKey(RSStringRef keyName, RSDictionaryRef dict)
{
    RSMutableArrayRef overrides = RSArrayCreateMutable(RSAllocatorSystemDefault, 0);
    RSStringRef keyNameWithBoth = RSStringCreateWithFormat(RSAllocatorSystemDefault, RSSTR("%R-%R~%R"), keyName, _RSGetPlatformName(), _RSGetProductName());
    RSStringRef keyNameWithProduct = RSStringCreateWithFormat(RSAllocatorSystemDefault, RSSTR("%R~%R"), keyName, _RSGetProductName());
    RSStringRef keyNameWithPlatform = RSStringCreateWithFormat(RSAllocatorSystemDefault, RSSTR("%R-%R"), keyName, _RSGetPlatformName());
    
    RSIndex count = RSDictionaryGetCount(dict);
    
    if (count > 0)
    {
        RSTypeRef *keys = (RSTypeRef *)RSAllocatorAllocate(RSAllocatorSystemDefault, 2 * count * sizeof(RSTypeRef));
        RSTypeRef *values = &(keys[count]);
        
        RSDictionaryGetKeysAndValues(dict, keys, values);
        for (RSIndex idx = 0; idx < count; idx++)
        {
			if (RSEqual(keys[idx], keyNameWithBoth))
            {
				RSArrayAddObject(overrides, keys[idx]);
				break;
			}
		}
        for (RSIndex idx = 0; idx < count; idx++)
        {
			if (RSEqual(keys[idx], keyNameWithProduct))
            {
				RSArrayAddObject(overrides, keys[idx]);
				break;
			}
		}
        for (RSIndex idx = 0; idx < count; idx++)
        {
			if (RSEqual(keys[idx], keyNameWithPlatform))
            {
				RSArrayAddObject(overrides, keys[idx]);
				break;
			}
		}
        for (RSIndex idx = 0; idx < count; idx++)
        {
			if (RSEqual(keys[idx], keyName)) {
				RSArrayAddObject(overrides, keys[idx]);
				break;
			}
		}
        
        if (!_RSAllocatorIsGCRefZero(RSAllocatorSystemDefault))
        {
            RSAllocatorDeallocate(RSAllocatorSystemDefault, keys);
		}
	}
    
	if (!_RSAllocatorIsGCRefZero(RSAllocatorSystemDefault))
    {
		RSRelease(keyNameWithProduct);
		RSRelease(keyNameWithPlatform);
		RSRelease(keyNameWithBoth);
	}
    
    return overrides;
}

RSPrivate void _processInfoDictionary(RSMutableDictionaryRef dict, RSStringRef platformSuffix, RSStringRef productSuffix)
{
    RSIndex count = RSDictionaryGetCount(dict);
    
    if (count > 0)
    {
        RSTypeRef *keys = (RSTypeRef *)RSAllocatorAllocate(RSAllocatorSystemDefault, 2 * count * sizeof(RSTypeRef));
        RSTypeRef *values = &(keys[count]);
        RSMutableArrayRef guard = RSArrayCreateMutable(RSAllocatorSystemDefault, 0);
        
        RSDictionaryGetKeysAndValues(dict, keys, values);
        for (RSIndex idx = 0; idx < count; idx++)
        {
            RSStringRef keyPlatformSuffix, keyProductSuffix, keyName;
            if (_isOverrideKey((RSStringRef)keys[idx], &keyName, &keyPlatformSuffix, &keyProductSuffix))
            {
                RSArrayRef keysForBaseKey = nil;
                if (_isCurrentPlatformAndProduct(keyPlatformSuffix, keyProductSuffix) &&
                    !_isBlacklistedKey(keyName) &&
                    RSDictionaryGetValue(dict, keys[idx]))
                {
                    keysForBaseKey = _CopySortedOverridesForBaseKey(keyName, dict);
                    RSIndex keysForBaseKeyCount = RSArrayGetCount(keysForBaseKey);
                    
                    //make sure the other keys for this base key don't get released out from under us until we're done
                    RSArrayAddObject(guard, keysForBaseKey);
                    
                    //the winner for this base key will be sorted to the front, do the override with it
                    RSTypeRef highestPriorityKey = RSArrayObjectAtIndex(keysForBaseKey, 0);
                    RSDictionarySetValue(dict, keyName, RSDictionaryGetValue(dict, highestPriorityKey));
                    
                    //remove everything except the now-overridden key; this will cause them to fail the RSDictionaryContainsKey(dict, keys[idx]) check in the enclosing if() and not be reprocessed
                    for (RSIndex presentKeysIdx = 0; presentKeysIdx < keysForBaseKeyCount; presentKeysIdx++)
                    {
                        RSStringRef currentKey = (RSStringRef)RSArrayObjectAtIndex(keysForBaseKey, presentKeysIdx);
                        if (!RSEqual(currentKey, keyName))
                            RSDictionaryRemoveValue(dict, currentKey);
                    }
                }
                else
                {
                    RSDictionaryRemoveValue(dict, keys[idx]);
                }
                
                
                if (!_RSAllocatorIsGCRefZero(RSAllocatorSystemDefault))
                {
                    if (keyPlatformSuffix) RSRelease(keyPlatformSuffix);
                    if (keyProductSuffix) RSRelease(keyProductSuffix);
                    RSRelease(keyName);
                    if (keysForBaseKey) RSRelease(keysForBaseKey);
                }
            }
        }
        
        if (!_RSAllocatorIsGCRefZero(RSAllocatorSystemDefault))
        {
            RSAllocatorDeallocate(RSAllocatorSystemDefault, keys);
            RSRelease(guard);
        }
    }
}

static RSMutableDictionaryRef _RSBundleGrokInfoDictFromData(const char *bytes, uint32_t length)
{
    RSMutableDictionaryRef result = nil;
    RSDataRef infoData = nil;
    if (bytes && 0 < length)
    {
        infoData = RSDataCreateWithNoCopy(RSAllocatorSystemDefault, (uint8_t *)bytes, length, YES, RSAllocatorSystemDefault);
        if (infoData)
        {
            //result = (RSMutableDictionaryRef)RSPropertyListCreateFromXMLData(RSAllocatorSystemDefault, infoData, RSPropertyListMutableContainers, nil);
            RSPropertyListRef plist = RSPropertyListCreateWithXMLData(RSAllocatorSystemDefault, infoData);
            result = RSMutableCopy(RSAllocatorSystemDefault, RSPropertyListGetContent(plist));
            RSRelease(plist);
            
            if (result && RSDictionaryGetTypeID() != RSGetTypeID(result))
            {
                RSRelease(result);
                result = nil;
            }
            RSRelease(infoData);
        }
        if (!result) result = RSDictionaryCreateMutable(RSAllocatorSystemDefault, 0, RSDictionaryRSTypeContext);
    }
    if (result) _processInfoDictionary((RSMutableDictionaryRef)result, _RSGetPlatformName(), _RSGetProductName());
    return result;
}

static char *_RSBundleGetSectData(const char *segname, const char *sectname, unsigned long *size)
{
    char *retval = nil;
    unsigned long localSize = 0;
    uint32_t i, numImages = _dyld_image_count();
    const void *mhp = (const void *)_NSGetMachExecuteHeader();
    
    for (i = 0; i < numImages; i++)
    {
        if (mhp == (void *)_dyld_get_image_header(i))
        {
#if __LP64__
            const struct section_64 *sp = getsectbynamefromheader_64((const struct mach_header_64 *)mhp, segname, sectname);
            if (sp)
            {
                retval = (char *)(sp->addr + _dyld_get_image_vmaddr_slide(i));
                localSize = (unsigned long)sp->size;
            }
#else /* __LP64__ */
            const struct section *sp = getsectbynamefromheader((const struct mach_header *)mhp, segname, sectname);
            if (sp)
            {
                retval = (char *)(sp->addr + _dyld_get_image_vmaddr_slide(i));
                localSize = (unsigned long)sp->size;
            }
#endif /* __LP64__ */
            break;
        }
    }
    if (size) *size = localSize;
    return retval;
}

static RSDictionaryRef _RSBundleGrokInfoDictFromFile(int fd, const void *bytes, RSIndex length, uint32_t offset, BOOL swapped, BOOL sixtyFour)
{
    struct statinfo statBuf;
    off_t fileLength = 0;
    char *maploc = nil;
    const char *loc;
    unsigned i, j;
    RSDictionaryRef result = nil;
    BOOL foundit = NO;
    if (fd >= 0 && fstat(fd, &statBuf) == 0 && (maploc = mmap(0, statBuf.st_size, PROT_READ, MAP_PRIVATE, fd, 0)) != (void *)-1)
    {
        loc = maploc;
        fileLength = statBuf.st_size;
    }
    else
    {
        loc = bytes;
        fileLength = length;
    }
    if (fileLength > offset + sizeof(struct mach_header_64))
    {
        if (sixtyFour)
        {
            uint32_t ncmds = _RSBundleSwapInt32Conditional(((struct mach_header_64 *)(loc + offset))->ncmds, swapped);
            uint32_t sizeofcmds = _RSBundleSwapInt32Conditional(((struct mach_header_64 *)(loc + offset))->sizeofcmds, swapped);
            const char *startofcmds = loc + offset + sizeof(struct mach_header_64);
            const char *endofcmds = startofcmds + sizeofcmds;
            struct segment_command_64 *sgp = (struct segment_command_64 *)startofcmds;
            if (endofcmds > loc + fileLength) endofcmds = loc + fileLength;
            for (i = 0; !foundit && i < ncmds && startofcmds <= (char *)sgp && (char *)sgp < endofcmds; i++)
            {
                if (LC_SEGMENT_64 == _RSBundleSwapInt32Conditional(sgp->cmd, swapped)) {
                    struct section_64 *sp = (struct section_64 *)((char *)sgp + sizeof(struct segment_command_64));
                    uint32_t nsects = _RSBundleSwapInt32Conditional(sgp->nsects, swapped);
                    for (j = 0; !foundit && j < nsects && startofcmds <= (char *)sp && (char *)sp < endofcmds; j++)
                    {
                        if (0 == strncmp(sp->sectname, PLIST_SECTION, sizeof(sp->sectname)) && 0 == strncmp(sp->segname, TEXT_SEGMENT, sizeof(sp->segname)))
                        {
                            uint64_t sectlength64 = _RSBundleSwapInt64Conditional(sp->size, swapped);
                            uint32_t sectlength = (uint32_t)(sectlength64 & 0xffffffff);
                            uint32_t sectoffset = _RSBundleSwapInt32Conditional(sp->offset, swapped);
                            const char *sectbytes = loc + offset + sectoffset;
                            // we don't support huge-sized plists
                            if (sectlength64 <= 0xffffffff && loc <= sectbytes && sectbytes + sectlength <= loc + fileLength) result = (RSDictionaryRef)_RSBundleGrokInfoDictFromData(sectbytes, sectlength);
                            foundit = YES;
                        }
                        sp = (struct section_64 *)((char *)sp + sizeof(struct section_64));
                    }
                }
                sgp = (struct segment_command_64 *)((char *)sgp + _RSBundleSwapInt32Conditional(sgp->cmdsize, swapped));
            }
        }
        else
        {
            uint32_t ncmds = _RSBundleSwapInt32Conditional(((struct mach_header *)(loc + offset))->ncmds, swapped);
            uint32_t sizeofcmds = _RSBundleSwapInt32Conditional(((struct mach_header *)(loc + offset))->sizeofcmds, swapped);
            const char *startofcmds = loc + offset + sizeof(struct mach_header);
            const char *endofcmds = startofcmds + sizeofcmds;
            struct segment_command *sgp = (struct segment_command *)startofcmds;
            if (endofcmds > loc + fileLength) endofcmds = loc + fileLength;
            for (i = 0; !foundit && i < ncmds && startofcmds <= (char *)sgp && (char *)sgp < endofcmds; i++)
            {
                if (LC_SEGMENT == _RSBundleSwapInt32Conditional(sgp->cmd, swapped))
                {
                    struct section *sp = (struct section *)((char *)sgp + sizeof(struct segment_command));
                    uint32_t nsects = _RSBundleSwapInt32Conditional(sgp->nsects, swapped);
                    for (j = 0; !foundit && j < nsects && startofcmds <= (char *)sp && (char *)sp < endofcmds; j++)
                    {
                        if (0 == strncmp(sp->sectname, PLIST_SECTION, sizeof(sp->sectname)) && 0 == strncmp(sp->segname, TEXT_SEGMENT, sizeof(sp->segname)))
                        {
                            uint32_t sectlength = _RSBundleSwapInt32Conditional(sp->size, swapped);
                            uint32_t sectoffset = _RSBundleSwapInt32Conditional(sp->offset, swapped);
                            const char *sectbytes = loc + offset + sectoffset;
                            if (loc <= sectbytes && sectbytes + sectlength <= loc + fileLength) result = (RSDictionaryRef)_RSBundleGrokInfoDictFromData(sectbytes, sectlength);
                            foundit = YES;
                        }
                        sp = (struct section *)((char *)sp + sizeof(struct section));
                    }
                }
                sgp = (struct segment_command *)((char *)sgp + _RSBundleSwapInt32Conditional(sgp->cmdsize, swapped));
            }
        }
    }
    if (maploc) munmap(maploc, statBuf.st_size);
    return result;
}

static void _RSBundleGrokObjcImageInfoFromFile(int fd, const void *bytes, RSIndex length, uint32_t offset, BOOL swapped, BOOL sixtyFour, BOOL *hasObjc, uint32_t *objcVersion, uint32_t *objcFlags)
{
    uint32_t sectlength = 0, sectoffset = 0, localVersion = 0, localFlags = 0;
    char *buffer = nil;
    char sectbuffer[8];
    const char *loc = nil;
    unsigned i, j;
    BOOL foundit = NO, localHasObjc = NO;
    
    if (fd >= 0 && lseek(fd, offset, SEEK_SET) == (off_t)offset)
    {
        buffer = malloc(IMAGE_INFO_BYTES_TO_READ);
        if (buffer && read(fd, buffer, IMAGE_INFO_BYTES_TO_READ) >= IMAGE_INFO_BYTES_TO_READ) loc = buffer;
    }
    else if (bytes && length >= offset + IMAGE_INFO_BYTES_TO_READ)
    {
        loc = bytes + offset;
    }
    if (loc)
    {
        if (sixtyFour)
        {
            uint32_t ncmds = _RSBundleSwapInt32Conditional(((struct mach_header_64 *)loc)->ncmds, swapped);
            uint32_t sizeofcmds = _RSBundleSwapInt32Conditional(((struct mach_header_64 *)loc)->sizeofcmds, swapped);
            const char *startofcmds = loc + sizeof(struct mach_header_64);
            const char *endofcmds = startofcmds + sizeofcmds;
            struct segment_command_64 *sgp = (struct segment_command_64 *)startofcmds;
            if (endofcmds > loc + IMAGE_INFO_BYTES_TO_READ) endofcmds = loc + IMAGE_INFO_BYTES_TO_READ;
            for (i = 0; !foundit && i < ncmds && startofcmds <= (char *)sgp && (char *)sgp < endofcmds; i++)
            {
                if (LC_SEGMENT_64 == _RSBundleSwapInt32Conditional(sgp->cmd, swapped))
                {
                    struct section_64 *sp = (struct section_64 *)((char *)sgp + sizeof(struct segment_command_64));
                    uint32_t nsects = _RSBundleSwapInt32Conditional(sgp->nsects, swapped);
                    for (j = 0; !foundit && j < nsects && startofcmds <= (char *)sp && (char *)sp < endofcmds; j++)
                    {
                        if (0 == strncmp(sp->segname, OBJC_SEGMENT_64, sizeof(sp->segname))) localHasObjc = YES;
                        if (0 == strncmp(sp->sectname, IMAGE_INFO_SECTION_64, sizeof(sp->sectname)) && 0 == strncmp(sp->segname, OBJC_SEGMENT_64, sizeof(sp->segname)))
                        {
                            uint64_t sectlength64 = _RSBundleSwapInt64Conditional(sp->size, swapped);
                            sectlength = (uint32_t)(sectlength64 & 0xffffffff);
                            sectoffset = _RSBundleSwapInt32Conditional(sp->offset, swapped);
                            foundit = YES;
                        }
                        sp = (struct section_64 *)((char *)sp + sizeof(struct section_64));
                    }
                }
                sgp = (struct segment_command_64 *)((char *)sgp + _RSBundleSwapInt32Conditional(sgp->cmdsize, swapped));
            }
        }
        else
        {
            uint32_t ncmds = _RSBundleSwapInt32Conditional(((struct mach_header *)loc)->ncmds, swapped);
            uint32_t sizeofcmds = _RSBundleSwapInt32Conditional(((struct mach_header *)loc)->sizeofcmds, swapped);
            const char *startofcmds = loc + sizeof(struct mach_header);
            const char *endofcmds = startofcmds + sizeofcmds;
            struct segment_command *sgp = (struct segment_command *)startofcmds;
            if (endofcmds > loc + IMAGE_INFO_BYTES_TO_READ) endofcmds = loc + IMAGE_INFO_BYTES_TO_READ;
            for (i = 0; !foundit && i < ncmds && startofcmds <= (char *)sgp && (char *)sgp < endofcmds; i++)
            {
                if (LC_SEGMENT == _RSBundleSwapInt32Conditional(sgp->cmd, swapped))
                {
                    struct section *sp = (struct section *)((char *)sgp + sizeof(struct segment_command));
                    uint32_t nsects = _RSBundleSwapInt32Conditional(sgp->nsects, swapped);
                    for (j = 0; !foundit && j < nsects && startofcmds <= (char *)sp && (char *)sp < endofcmds; j++)
                    {
                        if (0 == strncmp(sp->segname, OBJC_SEGMENT, sizeof(sp->segname))) localHasObjc = YES;
                        if (0 == strncmp(sp->sectname, IMAGE_INFO_SECTION, sizeof(sp->sectname)) && 0 == strncmp(sp->segname, OBJC_SEGMENT, sizeof(sp->segname)))
                        {
                            sectlength = _RSBundleSwapInt32Conditional(sp->size, swapped);
                            sectoffset = _RSBundleSwapInt32Conditional(sp->offset, swapped);
                            foundit = YES;
                        }
                        sp = (struct section *)((char *)sp + sizeof(struct section));
                    }
                }
                sgp = (struct segment_command *)((char *)sgp + _RSBundleSwapInt32Conditional(sgp->cmdsize, swapped));
            }
        }
        if (sectlength >= 8)
        {
            if (fd >= 0 && lseek(fd, offset + sectoffset, SEEK_SET) == (off_t)(offset + sectoffset) && read(fd, sectbuffer, 8) >= 8)
            {
                localVersion = _RSBundleSwapInt32Conditional(*(uint32_t *)sectbuffer, swapped);
                localFlags = _RSBundleSwapInt32Conditional(*(uint32_t *)(sectbuffer + 4), swapped);
            }
            else if (bytes && length >= offset + sectoffset + 8)
            {
                localVersion = _RSBundleSwapInt32Conditional(*(uint32_t *)(bytes + offset + sectoffset), swapped);
                localFlags = _RSBundleSwapInt32Conditional(*(uint32_t *)(bytes + offset + sectoffset + 4), swapped);
            }
        }
    }
    if (buffer) free(buffer);
    if (hasObjc) *hasObjc = localHasObjc;
    if (objcVersion) *objcVersion = localVersion;
    if (objcFlags) *objcFlags = localFlags;
}

static BOOL _RSBundleGrokObjCImageInfoFromMainExecutable(uint32_t *objcVersion, uint32_t *objcFlags)
{
    BOOL retval = NO;
    uint32_t localVersion = 0, localFlags = 0;
    char *bytes = nil;
    unsigned long length = 0;
#if __LP64__
    if (getsegbyname(OBJC_SEGMENT_64)) bytes = _RSBundleGetSectData(OBJC_SEGMENT_64, IMAGE_INFO_SECTION_64, &length);
#else /* __LP64__ */
    if (getsegbyname(OBJC_SEGMENT)) bytes = _RSBundleGetSectData(OBJC_SEGMENT, IMAGE_INFO_SECTION, &length);
#endif /* __LP64__ */
    if (bytes && length >= 8)
    {
        localVersion = *(uint32_t *)bytes;
        localFlags = *(uint32_t *)(bytes + 4);
        retval = YES;
    }
    if (objcVersion) *objcVersion = localVersion;
    if (objcFlags) *objcFlags = localFlags;
    return retval;
}

static BOOL _RSBundleGrokX11FromFile(int fd, const void *bytes, RSIndex length, uint32_t offset, BOOL swapped, BOOL sixtyFour)
{
    static const char libX11name[] = LIB_X11;
    char *buffer = nil;
    const char *loc = nil;
    unsigned i;
    BOOL result = NO;
    
    if (fd >= 0 && lseek(fd, offset, SEEK_SET) == (off_t)offset)
    {
        buffer = malloc(X11_BYTES_TO_READ);
        if (buffer && read(fd, buffer, X11_BYTES_TO_READ) >= X11_BYTES_TO_READ) loc = buffer;
    }
    else if (bytes && length >= offset + X11_BYTES_TO_READ)
    {
        loc = bytes + offset;
    }
    if (loc)
    {
        if (sixtyFour)
        {
            uint32_t ncmds = _RSBundleSwapInt32Conditional(((struct mach_header_64 *)loc)->ncmds, swapped);
            uint32_t sizeofcmds = _RSBundleSwapInt32Conditional(((struct mach_header_64 *)loc)->sizeofcmds, swapped);
            const char *startofcmds = loc + sizeof(struct mach_header_64);
            const char *endofcmds = startofcmds + sizeofcmds;
            struct dylib_command *dlp = (struct dylib_command *)startofcmds;
            if (endofcmds > loc + X11_BYTES_TO_READ) endofcmds = loc + X11_BYTES_TO_READ;
            for (i = 0; !result && i < ncmds && startofcmds <= (char *)dlp && (char *)dlp < endofcmds; i++)
            {
                if (LC_LOAD_DYLIB == _RSBundleSwapInt32Conditional(dlp->cmd, swapped))
                {
                    uint32_t nameoffset = _RSBundleSwapInt32Conditional(dlp->dylib.name.offset, swapped);
                    const char *name = (const char *)dlp + nameoffset;
                    if (startofcmds <= name && name + sizeof(libX11name) <= endofcmds && 0 == strncmp(name, libX11name, sizeof(libX11name) - 1)) result = YES;
                }
                dlp = (struct dylib_command *)((char *)dlp + _RSBundleSwapInt32Conditional(dlp->cmdsize, swapped));
            }
        }
        else
        {
            uint32_t ncmds = _RSBundleSwapInt32Conditional(((struct mach_header *)loc)->ncmds, swapped);
            uint32_t sizeofcmds = _RSBundleSwapInt32Conditional(((struct mach_header *)loc)->sizeofcmds, swapped);
            const char *startofcmds = loc + sizeof(struct mach_header);
            const char *endofcmds = startofcmds + sizeofcmds;
            struct dylib_command *dlp = (struct dylib_command *)startofcmds;
            if (endofcmds > loc + X11_BYTES_TO_READ) endofcmds = loc + X11_BYTES_TO_READ;
            for (i = 0; !result && i < ncmds && startofcmds <= (char *)dlp && (char *)dlp < endofcmds; i++)
            {
                if (LC_LOAD_DYLIB == _RSBundleSwapInt32Conditional(dlp->cmd, swapped))
                {
                    uint32_t nameoffset = _RSBundleSwapInt32Conditional(dlp->dylib.name.offset, swapped);
                    const char *name = (const char *)dlp + nameoffset;
                    if (startofcmds <= name && name + sizeof(libX11name) <= endofcmds && 0 == strncmp(name, libX11name, sizeof(libX11name) - 1)) result = YES;
                }
                dlp = (struct dylib_command *)((char *)dlp + _RSBundleSwapInt32Conditional(dlp->cmdsize, swapped));
            }
        }
    }
    if (buffer) free(buffer);
    return result;
}

static UInt32 _RSBundleGrokMachTypeForFatFile(int fd, const void *bytes, RSIndex length, BOOL swap, BOOL *isX11, RSArrayRef *architectures, RSDictionaryRef *infodict, BOOL *hasObjc, uint32_t *objcVersion, uint32_t *objcFlags)
{
    RSIndex headerLength = length;
    unsigned char headerBuffer[MAGIC_BYTES_TO_READ];
    UInt32 machtype = UNKNOWN_FILETYPE, magic, numFatHeaders, maxFatHeaders, i;
    unsigned char buffer[sizeof(struct mach_header_64)];
    const unsigned char *moreBytes = nil;
    const NXArchInfo *archInfo = NXGetLocalArchInfo();
    SInt32 curArch = _RSBundleCurrentArchitecture();
    
    struct fat_arch *fat = nil;
    
    if (isX11) *isX11 = NO;
    if (architectures) *architectures = nil;
    if (infodict) *infodict = nil;
    if (hasObjc) *hasObjc = NO;
    if (objcVersion) *objcVersion = 0;
    if (objcFlags) *objcFlags = 0;
    
    if (headerLength > MAGIC_BYTES_TO_READ) headerLength = MAGIC_BYTES_TO_READ;
    (void)memmove(headerBuffer, bytes, headerLength);
    if (swap)
    {
        for (i = 0; i < headerLength; i += 4) *(UInt32 *)(headerBuffer + i) = RSSwapInt32(*(UInt32 *)(headerBuffer + i));
    }
    numFatHeaders = ((struct fat_header *)headerBuffer)->nfat_arch;
    maxFatHeaders = (UInt32)(headerLength - sizeof(struct fat_header)) / sizeof(struct fat_arch);
    if (numFatHeaders > maxFatHeaders) numFatHeaders = maxFatHeaders;
    if (numFatHeaders > 0)
    {
        if (archInfo) fat = NXFindBestFatArch(archInfo->cputype, archInfo->cpusubtype, (struct fat_arch *)(headerBuffer + sizeof(struct fat_header)), numFatHeaders);
        if (!fat && curArch != 0) fat = NXFindBestFatArch((cpu_type_t)curArch, (cpu_subtype_t)0, (struct fat_arch *)(headerBuffer + sizeof(struct fat_header)), numFatHeaders);
        if (!fat) fat = (struct fat_arch *)(headerBuffer + sizeof(struct fat_header));
        if (architectures)
        {
            RSMutableArrayRef mutableArchitectures = RSArrayCreateMutable(RSAllocatorSystemDefault, 0);
            for (i = 0; i < numFatHeaders; i++)
            {
                RSNumberRef architecture = RSNumberCreateInteger(RSAllocatorSystemDefault, (int)headerBuffer + sizeof(struct fat_header) + i * sizeof(struct fat_arch));
                
                if (RSArrayIndexOfObject(mutableArchitectures, architecture) < 0) RSArrayAddObject(mutableArchitectures, architecture);
                RSRelease(architecture);
            }
            *architectures = (RSArrayRef)mutableArchitectures;
        }
    }
    if (fat)
    {
        if (fd >= 0 && lseek(fd, fat->offset, SEEK_SET) == (off_t)fat->offset && read(fd, buffer, sizeof(struct mach_header_64)) >= (int)sizeof(struct mach_header_64))
        {
            moreBytes = buffer;
        }
        else if (bytes && (uint32_t)length >= fat->offset + sizeof(struct mach_header_64))
        {
            moreBytes = bytes + fat->offset;
        }
        if (moreBytes)
        {
            magic = *((UInt32 *)moreBytes);
            if (MH_MAGIC == magic)
            {
                machtype = ((struct mach_header *)moreBytes)->filetype;
                if (isX11 && MH_EXECUTE == machtype) *isX11 = _RSBundleGrokX11FromFile(fd, bytes, length, fat->offset, NO, NO);
                if (infodict) *infodict = _RSBundleGrokInfoDictFromFile(fd, bytes, length, fat->offset, NO, NO);
                if (hasObjc || objcVersion || objcFlags) _RSBundleGrokObjcImageInfoFromFile(fd, bytes, length, fat->offset, NO, NO, hasObjc, objcVersion, objcFlags);
            }
            else if (MH_CIGAM == magic)
            {
                machtype = RSSwapInt32(((struct mach_header *)moreBytes)->filetype);
                if (isX11 && MH_EXECUTE == machtype) *isX11 = _RSBundleGrokX11FromFile(fd, bytes, length, fat->offset, YES, NO);
                if (infodict) *infodict = _RSBundleGrokInfoDictFromFile(fd, bytes, length, fat->offset, YES, NO);
                if (hasObjc || objcVersion || objcFlags) _RSBundleGrokObjcImageInfoFromFile(fd, bytes, length, fat->offset, YES, NO, hasObjc, objcVersion, objcFlags);
            }
            else if (MH_MAGIC_64 == magic)
            {
                machtype = ((struct mach_header_64 *)moreBytes)->filetype;
                if (isX11 && MH_EXECUTE == machtype) *isX11 = _RSBundleGrokX11FromFile(fd, bytes, length, fat->offset, NO, YES);
                if (infodict) *infodict = _RSBundleGrokInfoDictFromFile(fd, bytes, length, fat->offset, NO, YES);
                if (hasObjc || objcVersion || objcFlags) _RSBundleGrokObjcImageInfoFromFile(fd, bytes, length, fat->offset, NO, YES, hasObjc, objcVersion, objcFlags);
            }
            else if (MH_CIGAM_64 == magic)
            {
                machtype = RSSwapInt32(((struct mach_header_64 *)moreBytes)->filetype);
                if (isX11 && MH_EXECUTE == machtype) *isX11 = _RSBundleGrokX11FromFile(fd, bytes, length, fat->offset, YES, YES);
                if (infodict) *infodict = _RSBundleGrokInfoDictFromFile(fd, bytes, length, fat->offset, YES, YES);
                if (hasObjc || objcVersion || objcFlags) _RSBundleGrokObjcImageInfoFromFile(fd, bytes, length, fat->offset, YES, YES, hasObjc, objcVersion, objcFlags);
            }
        }
    }
    return machtype;
}

static UInt32 _RSBundleGrokMachType(int fd, const void *bytes, RSIndex length, BOOL *isX11, RSArrayRef *architectures, RSDictionaryRef *infodict, BOOL *hasObjc, uint32_t *objcVersion, uint32_t *objcFlags)
{
    unsigned int magic = *((UInt32 *)bytes), machtype = UNKNOWN_FILETYPE, cputype;
    RSNumberRef architecture = nil;
    
    if (isX11) *isX11 = NO;
    if (architectures) *architectures = nil;
    if (infodict) *infodict = nil;
    if (hasObjc) *hasObjc = NO;
    if (objcVersion) *objcVersion = 0;
    if (objcFlags) *objcFlags = 0;
    if (MH_MAGIC == magic)
    {
        machtype = ((struct mach_header *)bytes)->filetype;
        cputype = ((struct mach_header *)bytes)->cputype;
        if (architectures) architecture = RSNumberCreateInteger(RSAllocatorSystemDefault, cputype);
        if (isX11 && MH_EXECUTE == machtype) *isX11 = _RSBundleGrokX11FromFile(fd, bytes, length, 0, NO, NO);
        if (infodict) *infodict = _RSBundleGrokInfoDictFromFile(fd, bytes, length, 0, NO, NO);
        if (hasObjc || objcVersion || objcFlags) _RSBundleGrokObjcImageInfoFromFile(fd, bytes, length, 0, NO, NO, hasObjc, objcVersion, objcFlags);
    }
    else if (MH_CIGAM == magic)
    {
        machtype = RSSwapInt32(((struct mach_header *)bytes)->filetype);
        cputype = RSSwapInt32(((struct mach_header *)bytes)->cputype);
        if (architectures) architecture = RSNumberCreateInteger(RSAllocatorSystemDefault, cputype);
        if (isX11 && MH_EXECUTE == machtype) *isX11 = _RSBundleGrokX11FromFile(fd, bytes, length, 0, YES, NO);
        if (infodict) *infodict = _RSBundleGrokInfoDictFromFile(fd, bytes, length, 0, YES, NO);
        if (hasObjc || objcVersion || objcFlags) _RSBundleGrokObjcImageInfoFromFile(fd, bytes, length, 0, YES, NO, hasObjc, objcVersion, objcFlags);
    }
    else if (MH_MAGIC_64 == magic)
    {
        machtype = ((struct mach_header_64 *)bytes)->filetype;
        cputype = ((struct mach_header_64 *)bytes)->cputype;
        if (architectures) architecture = RSNumberCreateInteger(RSAllocatorSystemDefault, cputype);
        if (isX11 && MH_EXECUTE == machtype) *isX11 = _RSBundleGrokX11FromFile(fd, bytes, length, 0, NO, YES);
        if (infodict) *infodict = _RSBundleGrokInfoDictFromFile(fd, bytes, length, 0, NO, YES);
        if (hasObjc || objcVersion || objcFlags) _RSBundleGrokObjcImageInfoFromFile(fd, bytes, length, 0, NO, YES, hasObjc, objcVersion, objcFlags);
    }
    else if (MH_CIGAM_64 == magic)
    {
        machtype = RSSwapInt32(((struct mach_header_64 *)bytes)->filetype);
        cputype = RSSwapInt32(((struct mach_header_64 *)bytes)->cputype);
        if (architectures) architecture = RSNumberCreateInteger(RSAllocatorSystemDefault, cputype);
        if (isX11 && MH_EXECUTE == machtype) *isX11 = _RSBundleGrokX11FromFile(fd, bytes, length, 0, YES, YES);
        if (infodict) *infodict = _RSBundleGrokInfoDictFromFile(fd, bytes, length, 0, YES, YES);
        if (hasObjc || objcVersion || objcFlags) _RSBundleGrokObjcImageInfoFromFile(fd, bytes, length, 0, YES, YES, hasObjc, objcVersion, objcFlags);
    }
    else if (FAT_MAGIC == magic)
    {
        machtype = _RSBundleGrokMachTypeForFatFile(fd, bytes, length, NO, isX11, architectures, infodict, hasObjc, objcVersion, objcFlags);
    }
    else if (FAT_CIGAM == magic)
    {
        machtype = _RSBundleGrokMachTypeForFatFile(fd, bytes, length, YES, isX11, architectures, infodict, hasObjc, objcVersion, objcFlags);
    }
    else if (PEF_MAGIC == magic || PEF_CIGAM == magic)
    {
        machtype = PEF_FILETYPE;
    }
    if (architectures && architecture) *architectures = RSArrayCreateWithObjects(RSAllocatorSystemDefault, (const void **)&architecture, 1);
    if (architecture) RSRelease(architecture);
    return machtype;
}

static BOOL _RSBundleGrokFileTypeForZipMimeType(const unsigned char *bytes, RSIndex length, const char **ext)
{
    unsigned namelength = RSSwapInt16HostToLittle(*((UInt16 *)(bytes + 26))), extralength = RSSwapInt16HostToLittle(*((UInt16 *)(bytes + 28)));
    const unsigned char *data = bytes + 30 + namelength + extralength;
    int i = -1;
    if (bytes < data && data + 56 <= bytes + length && 0 == RSSwapInt16HostToLittle(*((UInt16 *)(bytes + 8))) && (0 == ustrncasecmp(data, "application/vnd.", 16) || 0 == ustrncasecmp(data, "application/x-vnd.", 18)))
    {
        data += ('.' == *(data + 15)) ? 16 : 18;
        if (0 == ustrncasecmp(data, "sun.xml.", 8))
        {
            data += 8;
            if (0 == ustrncasecmp(data, "calc", 4)) i = 0;
            else if (0 == ustrncasecmp(data, "draw", 4)) i = 1;
            else if (0 == ustrncasecmp(data, "writer.global", 13)) i = 2;
            else if (0 == ustrncasecmp(data, "impress", 7)) i = 3;
            else if (0 == ustrncasecmp(data, "math", 4)) i = 4;
            else if (0 == ustrncasecmp(data, "writer", 6)) i = 5;
            if (i >= 0 && ext) *ext = __RSBundleOOExtensionsArray + i * EXTENSION_LENGTH;
        }
        else if (0 == ustrncasecmp(data, "oasis.opendocument.", 19))
        {
            data += 19;
            if (0 == ustrncasecmp(data, "chart", 5)) i = 0;
            else if (0 == ustrncasecmp(data, "formula", 7)) i = 1;
            else if (0 == ustrncasecmp(data, "graphics", 8)) i = 2;
            else if (0 == ustrncasecmp(data, "text-web", 8)) i = 3;
            else if (0 == ustrncasecmp(data, "image", 5)) i = 4;
            else if (0 == ustrncasecmp(data, "text-master", 11)) i = 5;
            else if (0 == ustrncasecmp(data, "presentation", 12)) i = 6;
            else if (0 == ustrncasecmp(data, "spreadsheet", 11)) i = 7;
            else if (0 == ustrncasecmp(data, "text", 4)) i = 8;
            if (i >= 0 && ext) *ext = __RSBundleODExtensionsArray + i * EXTENSION_LENGTH;
        }
    }
    else if (bytes < data && data + 41 <= bytes + length && 8 == RSSwapInt16HostToLittle(*((UInt16 *)(bytes + 8))) && 0x4b2c28c8 == RSSwapInt32HostToBig(*((UInt32 *)data)) && 0xc94c4e2c == RSSwapInt32HostToBig(*((UInt32 *)(data + 4))))
    {
        // AbiWord compressed mimetype odt
        if (ext) *ext = "odt";
        // almost certainly this should set i to 0 but I don't want to upset the apple cart now
    }
    else if (bytes < data && data + 29 <= bytes + length && (0 == ustrncasecmp(data, "application/oebps-package+xml", 29)))
    {
        // epub, official epub 3 mime type
        if (ext) *ext = "epub";
        i = 0;
    }
    else if (bytes < data && data + 20 <= bytes + length && (0 == ustrncasecmp(data, "application/epub+zip", 20)))
    {
        // epub, unofficial epub 2 mime type
        if (ext) *ext = "epub";
        i = 0;
    }
    return (i >= 0);
}

static const char *_RSBundleGrokFileTypeForZipFile(int fd, const unsigned char *bytes, RSIndex length, off_t fileLength)
{
    const char *ext = "zip";
    const unsigned char *moreBytes = nil;
    unsigned char *buffer = nil;
    RSIndex i;
    BOOL foundMimetype = NO, hasMetaInf = NO, hasContentXML = NO, hasManifestMF = NO, hasManifestXML = NO, hasRels = NO, hasContentTypes = NO, hasWordDocument = NO, hasExcelDocument = NO, hasPowerPointDocument = NO, hasOPF = NO, hasSMIL = NO;
    
    if (bytes)
    {
        for (i = 0; !foundMimetype && i + 30 < length; i++)
        {
            if (0x50 == bytes[i] && 0x4b == bytes[i + 1])
            {
                unsigned namelength = 0, offset = 0;
                if (0x01 == bytes[i + 2] && 0x02 == bytes[i + 3])
                {
                    namelength = (unsigned)RSSwapInt16HostToLittle(*((UInt16 *)(bytes + i + 28)));
                    offset = 46;
                }
                else if (0x03 == bytes[i + 2] && 0x04 == bytes[i + 3])
                {
                    namelength = (unsigned)RSSwapInt16HostToLittle(*((UInt16 *)(bytes + i + 26)));
                    offset = 30;
                }
                if (offset > 0 && (RSIndex)(i + offset + namelength) <= length)
                {
                    //printf("%.*s\n", namelength, bytes + i + offset);
                    if (8 == namelength && 30 == offset && 0 == ustrncasecmp(bytes + i + offset, "mimetype", 8)) foundMimetype = _RSBundleGrokFileTypeForZipMimeType(bytes + i, length - i, &ext);
                    else if (9 == namelength && 0 == ustrncasecmp(bytes + i + offset, "META-INF/", 9)) hasMetaInf = YES;
                    else if (11 == namelength && 0 == ustrncasecmp(bytes + i + offset, "content.xml", 11)) hasContentXML = YES;
                    else if (11 == namelength && 0 == ustrncasecmp(bytes + i + offset, "_rels/.rels", 11)) hasRels = YES;
                    else if (19 == namelength && 0 == ustrncasecmp(bytes + i + offset, "[Content_Types].xml", 19)) hasContentTypes = YES;
                    else if (20 == namelength && 0 == ustrncasecmp(bytes + i + offset, "META-INF/MANIFEST.MF", 20)) hasManifestMF = YES;
                    else if (21 == namelength && 0 == ustrncasecmp(bytes + i + offset, "META-INF/manifest.xml", 21)) hasManifestXML = YES;
                    else if (4 < namelength && 0 == ustrncasecmp(bytes + i + offset + namelength - 4, ".opf", 4)) hasOPF = YES;
                    else if (4 < namelength && 0 == ustrncasecmp(bytes + i + offset + namelength - 4, ".sml", 4)) hasSMIL = YES;
                    else if (5 < namelength && 0 == ustrncasecmp(bytes + i + offset + namelength - 5, ".smil", 5)) hasSMIL = YES;
                    else if (7 < namelength && 0 == ustrncasecmp(bytes + i + offset, "xl/", 3) && 0 == ustrncasecmp(bytes + i + offset + namelength - 4, ".xml", 4)) hasExcelDocument = YES;
                    else if (8 < namelength && 0 == ustrncasecmp(bytes + i + offset, "ppt/", 4) && 0 == ustrncasecmp(bytes + i + offset + namelength - 4, ".xml", 4)) hasPowerPointDocument = YES;
                    else if (9 < namelength && 0 == ustrncasecmp(bytes + i + offset, "word/", 5) && 0 == ustrncasecmp(bytes + i + offset + namelength - 4, ".xml", 4)) hasWordDocument = YES;
                    else if (10 < namelength && 0 == ustrncasecmp(bytes + i + offset, "excel/", 6) && 0 == ustrncasecmp(bytes + i + offset + namelength - 4, ".xml", 4)) hasExcelDocument = YES;
                    else if (15 < namelength && 0 == ustrncasecmp(bytes + i + offset, "powerpoint/", 11) && 0 == ustrncasecmp(bytes + i + offset + namelength - 4, ".xml", 4)) hasPowerPointDocument = YES;
                    i += offset + namelength - 1;
                }
            }
        }
    }
    if (!foundMimetype)
    {
        if (fileLength >= ZIP_BYTES_TO_READ)
        {
            if (fd >= 0 && lseek(fd, fileLength - ZIP_BYTES_TO_READ, SEEK_SET) == fileLength - ZIP_BYTES_TO_READ)
            {
                buffer = (unsigned char *)malloc(ZIP_BYTES_TO_READ);
                if (buffer && read(fd, buffer, ZIP_BYTES_TO_READ) >= ZIP_BYTES_TO_READ) moreBytes = buffer;
            }
            else if (bytes && length >= ZIP_BYTES_TO_READ)
            {
                moreBytes = bytes + length - ZIP_BYTES_TO_READ;
            }
        }
        if (moreBytes)
        {
            for (i = 0; i + 30 < ZIP_BYTES_TO_READ; i++)
            {
                if (0x50 == moreBytes[i] && 0x4b == moreBytes[i + 1])
                {
                    unsigned namelength = 0, offset = 0;
                    if (0x01 == moreBytes[i + 2] && 0x02 == moreBytes[i + 3])
                    {
                        namelength = RSSwapInt16HostToLittle(*((UInt16 *)(moreBytes + i + 28)));
                        offset = 46;
                    }
                    else if (0x03 == moreBytes[i + 2] && 0x04 == moreBytes[i + 3])
                    {
                        namelength = RSSwapInt16HostToLittle(*((UInt16 *)(moreBytes + i + 26)));
                        offset = 30;
                    }
                    if (offset > 0 && i + offset + namelength <= ZIP_BYTES_TO_READ)
                    {
                        //printf("%.*s\n", namelength, moreBytes + i + offset);
                        if (9 == namelength && 0 == ustrncasecmp(moreBytes + i + offset, "META-INF/", 9)) hasMetaInf = YES;
                        else if (11 == namelength && 0 == ustrncasecmp(moreBytes + i + offset, "content.xml", 11)) hasContentXML = YES;
                        else if (11 == namelength && 0 == ustrncasecmp(moreBytes + i + offset, "_rels/.rels", 11)) hasRels = YES;
                        else if (19 == namelength && 0 == ustrncasecmp(moreBytes + i + offset, "[Content_Types].xml", 19)) hasContentTypes = YES;
                        else if (20 == namelength && 0 == ustrncasecmp(moreBytes + i + offset, "META-INF/MANIFEST.MF", 20)) hasManifestMF = YES;
                        else if (21 == namelength && 0 == ustrncasecmp(moreBytes + i + offset, "META-INF/manifest.xml", 21)) hasManifestXML = YES;
                        else if (4 < namelength && 0 == ustrncasecmp(moreBytes + i + offset + namelength - 4, ".opf", 4)) hasOPF = YES;
                        else if (4 < namelength && 0 == ustrncasecmp(moreBytes + i + offset + namelength - 4, ".sml", 4)) hasSMIL = YES;
                        else if (5 < namelength && 0 == ustrncasecmp(moreBytes + i + offset + namelength - 5, ".smil", 5)) hasSMIL = YES;
                        else if (7 < namelength && 0 == ustrncasecmp(moreBytes + i + offset, "xl/", 3) && 0 == ustrncasecmp(moreBytes + i + offset + namelength - 4, ".xml", 4)) hasExcelDocument = YES;
                        else if (8 < namelength && 0 == ustrncasecmp(moreBytes + i + offset, "ppt/", 4) && 0 == ustrncasecmp(moreBytes + i + offset + namelength - 4, ".xml", 4)) hasPowerPointDocument = YES;
                        else if (9 < namelength && 0 == ustrncasecmp(moreBytes + i + offset, "word/", 5) && 0 == ustrncasecmp(moreBytes + i + offset + namelength - 4, ".xml", 4)) hasWordDocument = YES;
                        else if (10 < namelength && 0 == ustrncasecmp(moreBytes + i + offset, "excel/", 6) && 0 == ustrncasecmp(moreBytes + i + offset + namelength - 4, ".xml", 4)) hasExcelDocument = YES;
                        else if (15 < namelength && 0 == ustrncasecmp(moreBytes + i + offset, "powerpoint/", 11) && 0 == ustrncasecmp(moreBytes + i + offset + namelength - 4, ".xml", 4)) hasPowerPointDocument = YES;
                        i += offset + namelength - 1;
                    }
                }
            }
        }
        //printf("hasManifestMF %d hasManifestXML %d hasContentXML %d hasRels %d hasContentTypes %d hasWordDocument %d hasExcelDocument %d hasPowerPointDocument %d hasMetaInf %d hasOPF %d hasSMIL %d\n", hasManifestMF, hasManifestXML, hasContentXML, hasRels, hasContentTypes, hasWordDocument, hasExcelDocument, hasPowerPointDocument, hasMetaInf, hasOPF, hasSMIL);
        if (hasManifestMF) ext = "jar";
        else if ((hasRels || hasContentTypes) && hasWordDocument) ext = "docx";
        else if ((hasRels || hasContentTypes) && hasExcelDocument) ext = "xlsx";
        else if ((hasRels || hasContentTypes) && hasPowerPointDocument) ext = "pptx";
        else if (hasManifestXML || hasContentXML) ext = "odt";
        else if (hasMetaInf) ext = "jar";
        else if (hasOPF && hasSMIL) ext = "dtb";
        else if (hasOPF) ext = "oeb";
        
        if (buffer) free(buffer);
    }
    return ext;
}

static BOOL _RSBundleCheckOLEName(const char *name, const char *bytes, unsigned length)
{
    BOOL retval = YES;
    unsigned j;
    for (j = 0; retval && j < length; j++) if (bytes[2 * j] != name[j]) retval = NO;
    return retval;
}

static const char *_RSBundleGrokFileTypeForOLEFile(int fd, const void *bytes, RSIndex length, off_t offset)
{
    const char *ext = "ole", *moreBytes = nil;
    char *buffer = nil;
    
    if (fd >= 0 && lseek(fd, offset, SEEK_SET) == (off_t)offset)
    {
        buffer = (char *)malloc(OLE_BYTES_TO_READ);
        if (buffer && read(fd, buffer, OLE_BYTES_TO_READ) >= OLE_BYTES_TO_READ) moreBytes = buffer;
    }
    else if (bytes && length >= offset + OLE_BYTES_TO_READ)
    {
        moreBytes = (char *)bytes + offset;
    }
    if (moreBytes)
    {
        BOOL foundit = NO;
        unsigned i;
        for (i = 0; !foundit && i < 4; i++)
        {
            char namelength = moreBytes[128 * i + 64] / 2;
            foundit = YES;
            if (sizeof(XLS_NAME) == namelength && _RSBundleCheckOLEName(XLS_NAME, moreBytes + 128 * i, namelength - 1)) ext = "xls";
            else if (sizeof(XLS_NAME2) == namelength && _RSBundleCheckOLEName(XLS_NAME2, moreBytes + 128 * i, namelength - 1)) ext = "xls";
            else if (sizeof(DOC_NAME) == namelength && _RSBundleCheckOLEName(DOC_NAME, moreBytes + 128 * i, namelength - 1)) ext = "doc";
            else if (sizeof(PPT_NAME) == namelength && _RSBundleCheckOLEName(PPT_NAME, moreBytes + 128 * i, namelength - 1)) ext = "ppt";
            else foundit = NO;
        }
    }
    if (buffer) free(buffer);
    return ext;
}

static BOOL _RSBundleGrokFileType(RSStringRef bundlePath,
                                  RSDataRef data,
                                  RSStringRef *extension,
                                  UInt32 *machtype,
                                  RSArrayRef *architectures,
                                  RSDictionaryRef *infodict,
                                  BOOL *hasObjc,
                                  uint32_t *objcVersion,
                                  uint32_t *objcFlags)
{
    int fd = -1;
    const unsigned char *bytes = nil;
    unsigned char buffer[MAGIC_BYTES_TO_READ];
    RSIndex i, length = 0;
    off_t fileLength = 0;
    const char *ext = nil;
    UInt32 mt = UNKNOWN_FILETYPE;
#if defined(BINARY_SUPPORT_DYLD)
    BOOL isX11 = NO;
#endif /* BINARY_SUPPORT_DYLD */
    BOOL isFile = NO, isPlain = YES, isZero = YES, isSpace = YES, hasBOM = NO;
    // extensions returned:  o, tool, x11app, pef, core, dylib, bundle, elf, jpeg, jp2, tiff, gif, png, pict, icns, ico, rtf, rtfd, pdf, ra, rm, au, aiff, aifc, caf, wav, avi, wmv, ogg, flac, psd, mpeg, mid, zip, jar, sit, cpio, html, ps, mov, qtif, ttf, otf, sfont, bmp, hqx, bin, class, tar, txt, gz, Z, uu, ync, bz, bz2, sh, pl, py, rb, dvi, sgi, tga, mp3, xml, plist, xls, doc, ppt, mp4, m4a, m4b, m4p, m4v, 3gp, 3g2, dmg, cwk, webarchive, dwg, dgn, pfa, pfb, afm, tfm, xcf, cpx, dwf, swf, swc, abw, bom, lit, svg, rdf, x3d, oeb, dtb, docx, xlsx, pptx, sxc, sxd, sxg, sxi, sxm, sxw, odc, odf, odg, oth, odi, odm, odp, ods, cin, exr
    // ??? we do not distinguish between different wm types, returning wmv for any of wmv, wma, or asf
    // ??? we do not distinguish between ordinary documents and template versions (often there is no difference in file contents)
    // ??? the distinctions between docx, xlsx, and pptx may not be entirely reliable
    if (architectures) *architectures = nil;
    if (infodict) *infodict = nil;
    if (hasObjc) *hasObjc = NO;
    if (objcVersion) *objcVersion = 0;
    if (objcFlags) *objcFlags = 0;
    if (bundlePath)
    {
        BOOL gotPath = NO;
        char path[RSMaxPathSize];
        gotPath = YES;
        RSStringGetCharacters(bundlePath, RSMakeRange(0, min(RSStringGetLength(bundlePath), RSMaxPathLength)), (UniChar *)path);
        RSStringGetCString(bundlePath, path, RSMaxPathSize, __RSDefaultEightBitStringEncoding);
        struct statinfo statBuf;
        if (gotPath && stat(path, &statBuf) == 0 &&
            (statBuf.st_mode & S_IFMT) == S_IFREG && (fd = open(path, O_RDONLY | RS_OPENFLGS, 0777)) >= 0)
        {
            length = read(fd, buffer, MAGIC_BYTES_TO_READ);
            fileLength = statBuf.st_size;
            bytes = buffer;
            isFile = YES;
        }
    }
    if (!isFile && data)
    {
        length = RSDataGetLength(data);
        fileLength = (off_t)length;
        bytes = RSDataGetBytesPtr(data);
        if (length == 0) ext = "txt";
    }
    if (bytes)
    {
        if (length >= 4)
        {
            UInt32 magic = RSSwapInt32HostToBig(*((UInt32 *)bytes));
            for (i = 0; !ext && i < NUM_EXTENSIONS; i++)
            {
                if (__RSBundleMagicNumbersArray[i] == magic) ext = __RSBundleExtensionsArray + i * EXTENSION_LENGTH;
            }
            if (ext)
            {
                if (0xcafebabe == magic && 8 <= length && 0 != *((UInt16 *)(bytes + 4))) ext = "class";
#if defined(BINARY_SUPPORT_DYLD)
                else if ((int)sizeof(struct mach_header_64) <= length) mt = _RSBundleGrokMachType(fd, bytes, length, extension ? &isX11 : nil, architectures, infodict, hasObjc, objcVersion, objcFlags);
                
                if (MH_OBJECT == mt) ext = "o";
                else if (MH_EXECUTE == mt) ext = isX11 ? "x11app" : "tool";
                else if (PEF_FILETYPE == mt) ext = "pef";
                else if (MH_CORE == mt) ext = "core";
                else if (MH_DYLIB == mt) ext = "dylib";
                else if (MH_BUNDLE == mt) ext = "bundle";
#endif /* BINARY_SUPPORT_DYLD */
                else if (0x7b5c7274 == magic && (6 > length || 'f' != bytes[4])) ext = nil;
                else if (0x25504446 == magic && (6 > length || '-' != bytes[4])) ext = nil;
                else if (0x00010000 == magic && (6 > length || 0 != bytes[4])) ext = nil;
                else if (0x47494638 == magic && (6 > length || (0x3761 != RSSwapInt16HostToBig(*((UInt16 *)(bytes + 4))) && 0x3961 != RSSwapInt16HostToBig(*((UInt16 *)(bytes + 4))))))  ext = nil;
                else if (0x0000000c == magic && (6 > length || 0x6a50 != RSSwapInt16HostToBig(*((UInt16 *)(bytes + 4))))) ext = nil;
                else if (0x2356524d == magic && (6 > length || 0x4c20 != RSSwapInt16HostToBig(*((UInt16 *)(bytes + 4))))) ext = nil;
                else if (0x28445746 == magic && (6 > length || 0x2056 != RSSwapInt16HostToBig(*((UInt16 *)(bytes + 4))))) ext = nil;
                else if (0x30373037 == magic && (6 > length || 0x30 != bytes[4] || !isdigit(bytes[5]))) ext = nil;
                else if (0x41433130 == magic && (6 > length || 0x31 != bytes[4] || !isdigit(bytes[5]))) ext = nil;
                else if (0x89504e47 == magic && (8 > length || 0x0d0a1a0a != RSSwapInt32HostToBig(*((UInt32 *)(bytes + 4))))) ext = nil;
                else if (0x53747566 == magic && (8 > length || 0x66497420 != RSSwapInt32HostToBig(*((UInt32 *)(bytes + 4))))) ext = nil;
                else if (0x3026b275 == magic && (8 > length || 0x8e66cf11 != RSSwapInt32HostToBig(*((UInt32 *)(bytes + 4))))) ext = nil;
                else if (0x67696d70 == magic && (8 > length || 0x20786366 != RSSwapInt32HostToBig(*((UInt32 *)(bytes + 4))))) ext = nil;
                else if (0x424f4d53 == magic && (8 > length || 0x746f7265 != RSSwapInt32HostToBig(*((UInt32 *)(bytes + 4))))) ext = nil;
                else if (0x49544f4c == magic && (8 > length || 0x49544c53 != RSSwapInt32HostToBig(*((UInt32 *)(bytes + 4))))) ext = nil;
                else if (0x72746664 == magic && (8 > length || 0x00000000 != RSSwapInt32HostToBig(*((UInt32 *)(bytes + 4))))) ext = nil;
                else if (0x3d796265 == magic && (12 > length || 0x67696e20 != RSSwapInt32HostToBig(*((UInt32 *)(bytes + 4))) || (0x6c696e65 != RSSwapInt32HostToBig(*((UInt32 *)(bytes + 8))) && 0x70617274 != RSSwapInt32HostToBig(*((UInt32 *)(bytes + 8)))))) ext = nil;
                else if (0x63616666 == magic && (12 > length || 0 != bytes[4] || 0x64657363 != RSSwapInt32HostToBig(*((UInt32 *)(bytes + 8))))) ext = nil;
                else if (0x504b0304 == magic) ext = _RSBundleGrokFileTypeForZipFile(fd, bytes, length, fileLength);
                else if (0x25215053 == magic)
                {
                    if (11 <= length && 0 == ustrncmp(bytes + 4, "-Adobe-", 7)) ext = "ps";
                    else if (14 <= length && 0 == ustrncmp(bytes + 4, "-AdobeFont", 10)) ext = "pfa";
                    else ext = nil;
                }
                else if (0x464f524d == magic)
                {
                    // IFF
                    ext = nil;
                    if (12 <= length)
                    {
                        UInt32 iffMagic = RSSwapInt32HostToBig(*((UInt32 *)(bytes + 8)));
                        if (0x41494646 == iffMagic) ext = "aiff";
                        else if (0x414946 == iffMagic) ext = "aifc";
                    }
                }
                else if (0x52494646 == magic)
                {
                    // RIFF
                    ext = nil;
                    if (12 <= length)
                    {
                        UInt32 riffMagic = RSSwapInt32HostToBig(*((UInt32 *)(bytes + 8)));
                        if (0x57415645 == riffMagic) ext = "wav";
                        else if (0x41564920 == riffMagic) ext = "avi";
                    }
                }
                else if (0xd0cf11e0 == magic)
                {
                    // OLE
                    if (52 <= length) ext = _RSBundleGrokFileTypeForOLEFile(fd, bytes, length, 512 * (1 + RSSwapInt32HostToLittle(*((UInt32 *)(bytes + 48)))));
                }
                else if (0x62656769 == magic)
                {
                    // uu
                    ext = nil;
                    if (76 <= length && 'n' == bytes[4] && ' ' == bytes[5] && isdigit(bytes[6]) && isdigit(bytes[7]) && isdigit(bytes[8]) && ' ' == bytes[9])
                    {
                        RSIndex endOfLine = 0;
                        for (i = 10; 0 == endOfLine && i < length; i++) if ('\n' == bytes[i]) endOfLine = i;
                        if (10 <= endOfLine && endOfLine + 62 < length && 'M' == bytes[endOfLine + 1] && '\n' == bytes[endOfLine + 62])
                        {
                            ext = "uu";
                            for (i = endOfLine + 1; ext && i < endOfLine + 62; i++) if (!isprint(bytes[i])) ext = nil;
                        }
                    }
                }
            }
            if (extension && !ext)
            {
                UInt16 shortMagic = RSSwapInt16HostToBig(*((UInt16 *)bytes));
                if (5 <= length && 0 == bytes[3] && 0 == bytes[4] && ((1 == bytes[1] && 1 == (0xf7 & bytes[2])) || (0 == bytes[1] && (2 == (0xf7 & bytes[2]) || (3 == (0xf7 & bytes[2])))))) ext = "tga";
                else if (8 <= length && (0x6d6f6f76 == RSSwapInt32HostToBig(*((UInt32 *)(bytes + 4))) || 0x6d646174 == RSSwapInt32HostToBig(*((UInt32 *)(bytes + 4))) || 0x77696465 == RSSwapInt32HostToBig(*((UInt32 *)(bytes + 4))))) ext = "mov";
                else if (8 <= length && (0x69647363 == RSSwapInt32HostToBig(*((UInt32 *)(bytes + 4))) || 0x69646174 == RSSwapInt32HostToBig(*((UInt32 *)(bytes + 4))))) ext = "qtif";
                else if (8 <= length && 0x424f424f == RSSwapInt32HostToBig(*((UInt32 *)(bytes + 4)))) ext = "cwk";
                else if (8 <= length && 0x62706c69 == magic && 0x7374 == RSSwapInt16HostToBig(*((UInt16 *)(bytes + 4))) && isdigit(bytes[6]) && isdigit(bytes[7]))
                {
                    for (i = 8; !ext && i < 128 && i + 16 <= length; i++)
                    {
                        if (0 == ustrncmp(bytes + i, "WebMainResource", 15)) ext = "webarchive";
                    }
                    if (!ext) ext = "plist";
                }
                else if (0 == shortMagic && 12 <= length && 0x66747970 == RSSwapInt32HostToBig(*((UInt32 *)(bytes + 4))))
                {
                    // ??? may want more ftyp values
                    UInt32 ftyp = RSSwapInt32HostToBig(*((UInt32 *)(bytes + 8)));
                    if (0x6d703431 == ftyp || 0x6d703432 == ftyp || 0x69736f6d == ftyp || 0x69736f32 == ftyp) ext = "mp4";
                    else if (0x4d344120 == ftyp) ext = "m4a";
                    else if (0x4d344220 == ftyp) ext = "m4b";
                    else if (0x4d345020 == ftyp) ext = "m4p";
                    else if (0x4d345620 == ftyp || 0x4d345648 == ftyp || 0x4d345650 == ftyp) ext = "m4v";
                    else if (0x3367 == (ftyp >> 16))
                    {
                        UInt16 remainder = (ftyp & 0xffff);
                        if (0x6536 == remainder || 0x6537 == remainder || 0x6736 == remainder || 0x7034 == remainder || 0x7035 == remainder || 0x7036 == remainder || 0x7236 == remainder || 0x7336 == remainder || 0x7337 == remainder) ext = "3gp";
                        else if (0x3261 == remainder) ext = "3g2";
                    }
                }
                else if (0x424d == shortMagic && 18 <= length)
                {
                    UInt32 btyp = RSSwapInt32HostToLittle(*((UInt32 *)(bytes + 14)));
                    if (40 == btyp || btyp == 12 || btyp == 64 || btyp == 108 || btyp == 124) ext = "bmp";
                }
                else if (20 <= length && 0 == ustrncmp(bytes + 6, "%!PS-AdobeFont", 14)) ext = "pfb";
                else if (40 <= length && 0x42696e48 == RSSwapInt32HostToBig(*((UInt32 *)(bytes + 34))) && 0x6578 == RSSwapInt16HostToBig(*((UInt16 *)(bytes + 38)))) ext = "hqx";
                else if (128 <= length && 0x6d42494e == RSSwapInt32HostToBig(*((UInt32 *)(bytes + 102)))) ext = "bin";
                else if (128 <= length && 0 == bytes[0] && 0 < bytes[1] && bytes[1] < 64 && 0 == bytes[74] && 0 == bytes[82] && 0 == (fileLength % 128))
                {
                    UInt32 df = RSSwapInt32HostToBig(*((UInt32 *)(bytes + 83))), rf = RSSwapInt32HostToBig(*((UInt32 *)(bytes + 87))), blocks = 1 + (df + 127) / 128 + (rf + 127) / 128;
                    if (df < 0x00800000 && rf < 0x00800000 && 1 < blocks && (off_t)(128 * blocks) == fileLength) ext = "bin";
                }
                else if (265 <= length && 0x75737461 == RSSwapInt32HostToBig(*((UInt32 *)(bytes + 257))) && (0x72202000 == RSSwapInt32HostToBig(*((UInt32 *)(bytes + 261))) || 0x7200 == RSSwapInt16HostToBig(*((UInt16 *)(bytes + 261))))) ext = "tar";
                else if (0xfeff == shortMagic || 0xfffe == shortMagic)
                {
                    ext = "txt";
                    if (12 <= length && ((0x3cfeff == *((UInt32 *)bytes) && 0x740068 == *((UInt32 *)(bytes + 4)) && 0x6c006d == *((UInt32 *)(bytes + 8))) || (0xfffe3c00 == *((UInt32 *)bytes) && 0x68007400 == *((UInt32 *)(bytes + 4)) && 0x6d006c00 == *((UInt32 *)(bytes + 8))))) ext = "html";
                }
                else if (0x1f9d == shortMagic) ext = "Z";
                else if (0x1f8b == shortMagic) ext = "gz";
                else if (0x71c7 == shortMagic || 0xc771 == shortMagic) ext = "cpio";
                else if (0xf702 == shortMagic) ext = "dvi";
                else if (0x01da == shortMagic && (0 == bytes[2] || 1 == bytes[2]) && (0 < bytes[3] && 16 > bytes[3])) ext = "sgi";
                else if (0x2321 == shortMagic)
                {
                    RSIndex endOfLine = 0, lastSlash = 0;
                    for (i = 2; 0 == endOfLine && i < length; i++) if ('\n' == bytes[i]) endOfLine = i;
                    if (endOfLine > 3)
                    {
                        for (i = endOfLine - 1; 0 == lastSlash && i > 1; i--) if ('/' == bytes[i]) lastSlash = i;
                        if (lastSlash > 0)
                        {
                            if (0 == ustrncmp(bytes + lastSlash + 1, "perl", 4)) ext = "pl";
                            else if (0 == ustrncmp(bytes + lastSlash + 1, "python", 6)) ext = "py";
                            else if (0 == ustrncmp(bytes + lastSlash + 1, "ruby", 4)) ext = "rb";
                            else ext = "sh";
                        }
                    }
                }
                else if (0xffd8 == shortMagic && 0xff == bytes[2]) ext = "jpeg";
                else if (0x4657 == shortMagic && 0x53 == bytes[2]) ext = "swf";
                else if (0x4357 == shortMagic && 0x53 == bytes[2]) ext = "swc";
                else if (0x4944 == shortMagic && '3' == bytes[2] && 0x20 > bytes[3]) ext = "mp3";
                else if (0x425a == shortMagic && isdigit(bytes[2]) && isdigit(bytes[3])) ext = "bz";
                else if (0x425a == shortMagic && 'h' == bytes[2] && isdigit(bytes[3]) && 8 <= length && (0x31415926 == RSSwapInt32HostToBig(*((UInt32 *)(bytes + 4))) || 0x17724538 == RSSwapInt32HostToBig(*((UInt32 *)(bytes + 4))))) ext = "bz2";
                else if (0x0011 == RSSwapInt16HostToBig(*((UInt16 *)(bytes + 2))) || 0x0012 == RSSwapInt16HostToBig(*((UInt16 *)(bytes + 2)))) ext = "tfm";
            }
        }
        if (extension && !ext)
        {
            //??? what about MacOSRoman?
            if (0xef == bytes[0] && 0xbb == bytes[1] && 0xbf == bytes[2])
            {
                // UTF-8 BOM
                hasBOM = YES;
                isZero = NO;
            }
            for (i = (hasBOM ? 3 : 0); (isPlain || isZero) && !ext && i < length && i < 512; i++)
            {
                char c = bytes[i];
                if (isPlain && '<' == c && i + 14 <= length && 0 == ustrncasecmp(bytes + i + 1, "!doctype html", 13)) ext = "html";
                if (isSpace && '<' == c && i + 14 <= length)
                {
                    if (0 == ustrncasecmp(bytes + i + 1, "!doctype html", 13) || 0 == ustrncasecmp(bytes + i + 1, "head", 4) || 0 == ustrncasecmp(bytes + i + 1, "title", 5) || 0 == ustrncasecmp(bytes + i + 1, "script", 6) || 0 == ustrncasecmp(bytes + i + 1, "html", 4))
                    {
                        ext = "html";
                    }
                    else if (0 == ustrncasecmp(bytes + i + 1, "?xml", 4))
                    {
                        for (i += 4; !ext && i < 128 && i + 20 <= length; i++)
                        {
                            if ('<' == bytes[i])
                            {
                                if (0 == ustrncasecmp(bytes + i + 1, "abiword", 7)) ext = "abw";
                                else if (0 == ustrncasecmp(bytes + i + 1, "!doctype svg", 12)) ext = "svg";
                                else if (0 == ustrncasecmp(bytes + i + 1, "!doctype rdf", 12)) ext = "rdf";
                                else if (0 == ustrncasecmp(bytes + i + 1, "!doctype x3d", 12)) ext = "x3d";
                                else if (0 == ustrncasecmp(bytes + i + 1, "!doctype html", 13)) ext = "html";
                                else if (0 == ustrncasecmp(bytes + i + 1, "!doctype posingfont", 19)) ext = "sfont";
                                else if (0 == ustrncasecmp(bytes + i + 1, "!doctype plist", 14))
                                {
                                    for (i += 14; !ext && i < 256 && i + 16 <= length; i++)
                                    {
                                        if (0 == ustrncmp(bytes + i, "WebMainResource", 15)) ext = "webarchive";
                                    }
                                    if (!ext) ext = "plist";
                                }
                            }
                        }
                        if (!ext) ext = "xml";
                    }
                }
                if (0 != c) isZero = NO;
                if (isZero || 0x7f <= c || (0x20 > c && !isspace(c))) isPlain = NO;
                if (isZero || !isspace(c)) isSpace = NO;
            }
            if (!ext)
            {
                if (isPlain)
                {
                    if (16 <= length && 0 == ustrncmp(bytes, "StartFontMetrics", 16)) ext = "afm";
                    else ext = "txt";
                }
                else if (isZero && length >= MAGIC_BYTES_TO_READ && fileLength >= 526)
                {
                    if (isFile)
                    {
                        if (lseek(fd, 512, SEEK_SET) == 512 && read(fd, buffer, MAGIC_BYTES_TO_READ) >= 14)
                        {
                            if (0x001102ff == RSSwapInt32HostToBig(*((UInt32 *)(buffer + 10)))) ext = "pict";
                        }
                    }
                    else
                    {
                        if (526 <= length && 0x001102ff == RSSwapInt32HostToBig(*((UInt32 *)(bytes + 522)))) ext = "pict";
                    }
                }
            }
        }
        if (extension && (!ext || 0 == strcmp(ext, "bz2")) && length >= MAGIC_BYTES_TO_READ && fileLength >= DMG_BYTES_TO_READ)
        {
            if (isFile)
            {
                if (lseek(fd, fileLength - DMG_BYTES_TO_READ, SEEK_SET) == fileLength - DMG_BYTES_TO_READ && read(fd, buffer, DMG_BYTES_TO_READ) >= DMG_BYTES_TO_READ)
                {
                    if (0x6b6f6c79 == RSSwapInt32HostToBig(*((UInt32 *)buffer)) || (0x63647361 == RSSwapInt32HostToBig(*((UInt32 *)(buffer + DMG_BYTES_TO_READ - 8))) && 0x656e6372 == RSSwapInt32HostToBig(*((UInt32 *)(buffer + DMG_BYTES_TO_READ - 4))))) ext = "dmg";
                }
            }
            else
            {
                if (DMG_BYTES_TO_READ <= length && (0x6b6f6c79 == RSSwapInt32HostToBig(*((UInt32 *)(bytes + length - DMG_BYTES_TO_READ))) || (0x63647361 == RSSwapInt32HostToBig(*((UInt32 *)(bytes + length - 8))) && 0x656e6372 == RSSwapInt32HostToBig(*((UInt32 *)(bytes + length - 4)))))) ext = "dmg";
            }
        }
    }
    if (extension) *extension = ext ? RSStringCreateWithCStringNoCopy(RSAllocatorSystemDefault, ext, RSStringEncodingUTF8, RSAllocatorSystemDefault) : nil;
    if (machtype) *machtype = mt;
    if (fd >= 0) close(fd);
    return (ext ? YES : NO);
}

RSStringRef _RSBundleCopyFileTypeForFilePath(RSStringRef bundlePath)
{
    RSStringRef extension = nil;
    (void)_RSBundleGrokFileType(bundlePath, nil, &extension, nil, nil, nil, nil, nil, nil);
    return extension;
}

RSStringRef _RSBundleCopyFileTypeForFileData(RSDataRef data)
{
    RSStringRef extension = nil;
    (void)_RSBundleGrokFileType(nil, data, &extension, nil, nil, nil, nil, nil, nil);
    return extension;
}

// returns zero-ref dictionary under GC
RSPrivate RSDictionaryRef _RSBundleCopyInfoDictionaryInExecutable(RSStringRef bundlePath)
{
    RSDictionaryRef result = nil;
    (void)_RSBundleGrokFileType(bundlePath, nil, nil, nil, nil, &result, nil, nil, nil);
    return result;
}

RSPrivate RSArrayRef _RSBundleCopyArchitecturesForExecutable(RSStringRef bundlePath)
{
    RSArrayRef result = nil;
    (void)_RSBundleGrokFileType(bundlePath, nil, nil, nil, &result, nil, nil, nil, nil);
    return result;
}

#if DEPLOYMENT_TARGET_MACOSX || DEPLOYMENT_TARGET_EMBEDDED
static BOOL _RSBundleGetObjCImageInfoForExecutable(RSStringRef bundlePath, uint32_t *objcVersion, uint32_t *objcFlags)
{
    BOOL retval = NO;
    (void)_RSBundleGrokFileType(bundlePath, nil, nil, nil, nil, nil, &retval, objcVersion, objcFlags);
    return retval;
}
#endif

#if defined(BINARY_SUPPORT_DYLD)
RSPrivate __RSPBinaryType _RSBundleGrokBinaryType(RSStringRef executablePath)
{
    // Attempt to grok the type of the binary by looking for DYLD magic numbers.  If one of the DYLD magic numbers is found, find out what type of Mach-o file it is.  Otherwise, look for the PEF magic numbers to see if it is RSM.
    __RSPBinaryType result = executablePath ? __RSBundleUnreadableBinary : __RSBundleNoBinary;
    UInt32 machtype = UNKNOWN_FILETYPE;
    if (_RSBundleGrokFileType(executablePath, nil, nil, &machtype, nil, nil, nil, nil, nil))
    {
        switch (machtype)
        {
            case MH_EXECUTE:
                result = __RSBundleDYLDExecutableBinary;
                break;
            case MH_BUNDLE:
                result = __RSBundleDYLDBundleBinary;
                break;
            case MH_DYLIB:
                result = __RSBundleDYLDFrameworkBinary;
                break;
            case PEF_FILETYPE:
                result = __RSBundleRSMBinary;
                break;
        }
    }
    return result;
}
#endif /* BINARY_SUPPORT_DYLD */

static RSStringRef __RSBundleGetFSPath(RSBundleRef bundle)
{
    return bundle->_path;
}

static void __RSBundleGetPathsWithVersion(RSBitU8 version, RSStringRef* res, RSStringRef* plugins, RSStringRef* sharedSupport, RSStringRef* frameworks, RSStringRef* sharedFrameworks)
{
    RSStringRef _resources = nil, _plugins = nil, _sharedSupport = nil, _frameworks = nil, _sharedFrameworks = nil;
    switch (version)
    {
        case 0:
            _resources = _RSBundleResourcesPathFromBase0;
            _plugins = _RSBundleAlternateBuiltInPlugInsPathFromBase0;
            _sharedSupport = _RSBundleSharedSupportPathFromBase0;
            _frameworks = _RSBundlePrivateFrameworksPathFromBase0;
            _sharedFrameworks = _RSBundleSharedFrameworksPathFromBase0;
            break;
        case 1:
            _resources = _RSBundleResourcesPathFromBase1;
            _plugins = _RSBundleAlternateBuiltInPlugInsPathFromBase1;
            _sharedSupport = _RSBundleSharedSupportPathFromBase1;
            _frameworks = _RSBundlePrivateFrameworksPathFromBase1;
            _sharedFrameworks = _RSBundleSharedFrameworksPathFromBase1;
            break;
        case 2:
            _resources = _RSBundleResourcesPathFromBase2;
            _plugins = _RSBundleAlternateBuiltInPlugInsPathFromBase2;
            _sharedSupport = _RSBundleSharedSupportPathFromBase2;
            _frameworks = _RSBundlePrivateFrameworksPathFromBase2;
            _sharedFrameworks = _RSBundleSharedFrameworksPathFromBase2;
            break;
        case 3:
        case 4:
        default:
            _resources = nil;
            _plugins = nil;
            _sharedSupport = nil;
            _frameworks = nil;
            _sharedFrameworks = nil;
            break;
    }
    if (res) *res = _resources;
    if (plugins) *plugins = _plugins;
    if (sharedSupport) *sharedSupport = _sharedSupport;
    if (frameworks) *frameworks = _frameworks;
    if (sharedFrameworks) *sharedFrameworks = _sharedFrameworks;
}

static RSMutableDictionaryRef __RSBundleCreateQueryTableAtBundlePathWithVersion(RSAllocatorRef allocator, RSStringRef bundlePath, RSBitU8 version)
{
    // build the files for /Resource
    RSFileManagerRef fmg = RSFileManagerGetDefault();
    RSStringRef path = nil;
    RSMutableDictionaryRef queryTable = RSDictionaryCreateMutable(allocator, 0, RSDictionaryRSTypeContext);
    path = RSMutableCopy(RSAllocatorSystemDefault, bundlePath);
    
    RSStringRef _resources = nil, _plugins = nil, _sharedSupport = nil, _frameworks = nil, _sharedFrameworks = nil;
    // check for existence of "Resources" or "Contents" or "Support Files"
    // but check for the most likely one first
    // version 0:  old-style "Resources" bundles
    // version 1:  obsolete "Support Files" bundles
    // version 2:  modern "Contents" bundles
    // version 3:  none of the above (see below)
    // version 4:  not a bundle (for main bundle only)
    __RSBundleGetPathsWithVersion(version, &_resources, &_plugins, &_sharedSupport, &_frameworks, &_sharedFrameworks);

    RSFileManagerAppendFileName(fmg, (RSMutableStringRef)path, _resources);
    RSArrayRef files = RSFileManagerContentsOfDirectory(fmg, path, nil);
    RSRelease(path);
    
    if (files)
    {
        RSDictionarySetValue(queryTable, _RSBundleResourcesPathFromBase0, files);
//        RSRelease(files);
    }
    
    // build the files for /PlugIns
    path = RSMutableCopy(RSAllocatorSystemDefault, bundlePath);
    RSFileManagerAppendFileName(fmg, (RSMutableStringRef)path, _plugins);
    files = RSFileManagerContentsOfDirectory(RSFileManagerGetDefault(), path, nil);
    RSRelease(path);
    
    if (files)
    {
        RSDictionarySetValue(queryTable, _RSBundleAlternateBuiltInPlugInsPathFromBase0, files);
//        RSRelease(files);
    }
    
    // build the files for /SharedSupport
    path = RSMutableCopy(RSAllocatorSystemDefault, bundlePath);
    RSFileManagerAppendFileName(fmg, (RSMutableStringRef)path, _sharedSupport);
    files = RSFileManagerContentsOfDirectory(RSFileManagerGetDefault(), path, nil);
    RSRelease(path);
    
    if (files)
    {
        RSDictionarySetValue(queryTable, _RSBundleSharedSupportPathFromBase0, files);
//        RSRelease(files);
    }
    
    // build the files for /Frameworks
    path = RSMutableCopy(RSAllocatorSystemDefault, bundlePath);
    RSFileManagerAppendFileName(fmg, (RSMutableStringRef)path, _frameworks);
    files = RSFileManagerContentsOfDirectory(RSFileManagerGetDefault(), path, nil);
    RSRelease(path);
    
    if (files)
    {
        RSDictionarySetValue(queryTable, _RSBundlePrivateFrameworksPathFromBase0, files);
//        RSRelease(files);
    }
    
    // build the files for /SharedFrameworks
    path = RSMutableCopy(RSAllocatorSystemDefault, bundlePath);
    RSFileManagerAppendFileName(fmg, (RSMutableStringRef)path, _sharedFrameworks);
    files = RSFileManagerContentsOfDirectory(RSFileManagerGetDefault(), path, nil);
    RSRelease(path);
    
    if (files)
    {
        RSDictionarySetValue(queryTable, _RSBundleSharedFrameworksPathFromBase0, files);
        RSRelease(files);
    }
    
    return queryTable;
}

static RSMutableDictionaryRef __RSBundleCreateQueryTableWithBundle(RSAllocatorRef allocator, RSBundleRef bundle)
{
    if (bundle == nil) return nil;
    RSSpinLockLock(&bundle->_queryTableLock);
    RSMutableDictionaryRef result = __RSBundleCreateQueryTableAtBundlePathWithVersion(allocator, __RSBundleGetFSPath(bundle), bundle->_version);
    RSSpinLockUnlock(&bundle->_queryTableLock);
    return result;
}

static inline BOOL _RSBundleSortedArrayContains(RSArrayRef arr, RSStringRef target)
{
    RSRange arrRange = RSMakeRange(0, RSArrayGetCount(arr));
    RSIndex itemIdx = RSArrayIndexOfObject(arr, target);
    return itemIdx < arrRange.length && RSEqual(RSArrayObjectAtIndex(arr, itemIdx), target);
}

static BOOL _RSBundlePathHasSubDir(RSStringRef bundlePath, RSStringRef subName)
{
    RSFileManagerRef fmg = RSFileManagerGetDefault();
    RSMutableStringRef fullPath = RSMutableCopy(RSAllocatorSystemDefault, bundlePath);
    RSFileManagerAppendFileName(fmg, fullPath, subName);
    BOOL isDir = NO;
    isDir = (YES == RSFileManagerFileExistsAtPath(fmg, fullPath, &isDir) && YES == isDir);
    RSRelease(fullPath);
    return isDir;
}

static RSArrayRef _RSBundleCopySortedDirectoryContentsAtPath(RSStringRef path, _RSBundleDirectoryContentsType type)
{
    RSFileManagerRef fmg = RSFileManagerGetDefault();
    return RSFileManagerContentsOfDirectory(fmg, path, nil);
}

RSPrivate BOOL _RSBundlePathLooksLikeBundleVersion(RSStringRef bundlePath, uint8_t *version)
{
    // check for existence of "Resources" or "Contents" or "Support Files"
    // but check for the most likely one first
    // version 0:  old-style "Resources" bundles
    // version 1:  obsolete "Support Files" bundles
    // version 2:  modern "Contents" bundles
    // version 3:  none of the above (see below)
    // version 4:  not a bundle (for main bundle only)
    uint8_t localVersion = 3;
    RSStringRef absolutePath = RSCopy(RSAllocatorSystemDefault, bundlePath);
    RSStringRef directoryPath = RSRetain(absolutePath);
    RSArrayRef contents = _RSBundleCopySortedDirectoryContentsAtPath(directoryPath, _RSBundleAllContents);
    BOOL hasFrameworkSuffix = RSStringHasSuffix((bundlePath), RSSTR(".framework/"));
#if DEPLOYMENT_TARGET_WINDOWS
    hasFrameworkSuffix = hasFrameworkSuffix || RSStringHasSuffix(RSPathGetString(bundlePath), RSSTR(".framework\\"));
#endif
    
    if (hasFrameworkSuffix)
    {
        if (_RSBundleSortedArrayContains(contents, _RSBundleResourcesDirectoryName)) localVersion = 0;
        else if (_RSBundleSortedArrayContains(contents, _RSBundleSupportFilesDirectoryName2)) localVersion = 2;
        else if (_RSBundleSortedArrayContains(contents, _RSBundleSupportFilesDirectoryName1)) localVersion = 1;
    }
    else
    {
        if (_RSBundleSortedArrayContains(contents, _RSBundleSupportFilesDirectoryName2)) localVersion = 2;
        else if (_RSBundleSortedArrayContains(contents, _RSBundleResourcesDirectoryName)) localVersion = 0;
        else if (_RSBundleSortedArrayContains(contents, _RSBundleSupportFilesDirectoryName1)) localVersion = 1;
    }
//    RSRelease(contents);  // contents is autorelease
    RSRelease(directoryPath);
    RSRelease(absolutePath);
#if DEPLOYMENT_TARGET_MACOSX || DEPLOYMENT_TARGET_WINDOWS
    if (localVersion == 3)
    {
        if (hasFrameworkSuffix)
        {
            if (_RSBundlePathHasSubDir(bundlePath, _RSBundleResourcesPathFromBase0)) localVersion = 0;
            else if (_RSBundlePathHasSubDir(bundlePath, _RSBundleSupportFilesPathFromBase2)) localVersion = 2;
            else if (_RSBundlePathHasSubDir(bundlePath, _RSBundleSupportFilesPathFromBase1)) localVersion = 1;
        }
        else
        {
            if (_RSBundlePathHasSubDir(bundlePath, _RSBundleSupportFilesPathFromBase2)) localVersion = 2;
            else if (_RSBundlePathHasSubDir(bundlePath, _RSBundleResourcesPathFromBase0)) localVersion = 0;
            else if (_RSBundlePathHasSubDir(bundlePath, _RSBundleSupportFilesPathFromBase1)) localVersion = 1;
        }
    }
#endif
    if (version) *version = localVersion;
    return (localVersion != 3);
}

static BOOL RSBundleCreateDataAndPropertiesFromResource(RSAllocatorRef allocator, RSStringRef path, RSDataRef* data, RSDictionaryRef* infoPlist, RSArrayRef desiredProperties, RSErrorCode* errorCode)
{
    BOOL result = NO;
    if (infoPlist)
    {
        RSPropertyListRef plist = RSPropertyListCreateWithPath(allocator, path);
        if (plist)
        {
            if (RSGetTypeID(RSPropertyListGetContent(plist)) == RSDictionaryGetTypeID())
            {
                *infoPlist = RSRetain(RSPropertyListGetContent(plist));
                result = YES;
            }
        }
        else
        {
            if (errorCode) *errorCode = kErrFormat;
        }
        RSRelease(plist);
    }
    else if (data)
    {
        RSFileHandleRef handle = RSFileHandleCreateForReadingAtPath(path);
        RSDataRef fileData = RSFileHandleReadDataToEndOfFile(handle);
        if (fileData)
        {
            *data = RSRetain(fileData);
            RSRelease(fileData);
            result = YES;
        }
        else
        {
            if (errorCode) *errorCode = kErrExisting;
        }
        RSRelease(handle);
    }
    if (errorCode)
    {
        if (result == YES)
        {
            *errorCode = kSuccess;
        }
    }
    return result;
}

RSPrivate RSDictionaryRef _RSBundleCopyInfoDictionaryInDirectoryWithVersion(RSAllocatorRef alloc, RSStringRef bundlePath, uint8_t version)
{
    RSDictionaryRef result = nil;
    if (bundlePath)
    {
        RSStringRef infoPath = nil, rawInfoPath = nil;
        RSDataRef infoData = nil;
        UniChar buff[RSMaxPathSize];
        RSIndex len;
        RSMutableStringRef cheapStr;
        RSStringRef infoPathFromBaseNoExtension = _RSBundleInfoPathFromBaseNoExtension0, infoPathFromBase = _RSBundleInfoPathFromBase0, infoPathFromRSBase = _RSBundleInfoPathFromRSBase0;
        BOOL tryPlatformSpecific = YES, tryGlobal = YES, tryRSSpecific = YES;
        RSStringRef directoryPath = nil;
        RSArrayRef contents = nil;
        RSRange contentsRange = RSMakeRange(0, 0);
        RSFileManagerRef fmg = RSFileManagerGetDefault();
        if (0 == version)
        {
            directoryPath = RSMutableCopy(RSAllocatorSystemDefault, bundlePath);
            RSFileManagerAppendFileName(fmg, (RSMutableStringRef)directoryPath, _RSBundleResourcesPathFromBase0);
            infoPathFromBaseNoExtension = _RSBundleInfoPathFromBaseNoExtension0;
            infoPathFromBase = _RSBundleInfoPathFromBase0;
            infoPathFromRSBase = _RSBundleInfoPathFromRSBase0;
        }
        else if (1 == version)
        {
            directoryPath = RSMutableCopy(RSAllocatorSystemDefault, bundlePath);
            RSFileManagerAppendFileName(fmg, (RSMutableStringRef)directoryPath, _RSBundleSupportFilesPathFromBase1);
            infoPathFromBaseNoExtension = _RSBundleInfoPathFromBaseNoExtension1;
            infoPathFromBase = _RSBundleInfoPathFromBase1;
            infoPathFromRSBase = _RSBundleInfoPathFromRSBase1;
        }
        else if (2 == version)
        {
            directoryPath = RSMutableCopy(RSAllocatorSystemDefault, bundlePath);
            RSFileManagerAppendFileName(fmg, (RSMutableStringRef)directoryPath,  _RSBundleSupportFilesPathFromBase2);
            infoPathFromBaseNoExtension = _RSBundleInfoPathFromBaseNoExtension2;
            infoPathFromBase = _RSBundleInfoPathFromBase2;
            infoPathFromRSBase = _RSBundleInfoPathFromRSBase2;
        }
        else if (3 == version)
        {
            RSStringRef path = RSRetain(bundlePath);
            //RSPathCopyFileSystemPath(bundlePath, RSPathPOSIXPathStyle);
            // this test is necessary to exclude the case where a bundle is spuriously created from the innards of another bundle
            if (path)
            {
                if (!(RSStringHasSuffix(path, _RSBundleSupportFilesDirectoryName1) ||
                      RSStringHasSuffix(path, _RSBundleSupportFilesDirectoryName2) ||
                      RSStringHasSuffix(path, _RSBundleResourcesDirectoryName)))
                {
                    directoryPath = (RSStringRef)RSRetain(bundlePath);
                    infoPathFromBaseNoExtension = _RSBundleInfoPathFromBaseNoExtension3;
                    infoPathFromBase = _RSBundleInfoPathFromBase3;
                    infoPathFromRSBase = _RSBundleInfoPathFromRSBase3;
                }
                RSRelease(path);
            }
        }
        if (directoryPath)
        {
            contents = _RSBundleCopySortedDirectoryContentsAtPath(directoryPath, _RSBundleAllContents);
            contentsRange = RSMakeRange(0, RSArrayGetCount(contents));
            RSRelease(directoryPath);
        }
        
        len = RSStringGetLength(infoPathFromBaseNoExtension);
        RSStringGetCharacters(infoPathFromBaseNoExtension, RSMakeRange(0, len), (UniChar *)buff);
        buff[len++] = (UniChar)'-';
        memmove(buff + len, _PlatformUniChars, _PlatformLen * sizeof(UniChar));
        len += _PlatformLen;
        _RSAppendPathExtension(buff, &len, RSMaxPathSize, _InfoExtensionUniChars, _InfoExtensionLen);
        cheapStr = RSStringCreateMutable(RSAllocatorSystemDefault, 0);
        RSStringAppendCharacters(cheapStr, (UniChar *)buff, len);
        
        infoPath = RSMutableCopy(RSAllocatorSystemDefault, bundlePath);
        RSFileManagerAppendFileName(fmg, (RSMutableStringRef)infoPath, cheapStr);
        if (contents)
        {
            RSIndex resourcesLen, idx;
            for (resourcesLen = len; resourcesLen > 0; resourcesLen--) if (buff[resourcesLen - 1] == PATH_SEP) break;
            RSStringDelete(cheapStr, RSMakeRange(0, RSStringGetLength(cheapStr)));
            RSStringReplace(cheapStr, RSMakeRange(0, RSStringGetLength(cheapStr)), nil);
            RSStringAppendCharacters(cheapStr, (UniChar *)buff + resourcesLen, len - resourcesLen);
            for (tryPlatformSpecific = NO, idx = 0; !tryPlatformSpecific && idx < contentsRange.length; idx++)
            {
                // Need to do this case-insensitive to accommodate Palm
                if (RSCompareEqualTo == RSStringCompareCaseInsensitive(cheapStr, (RSStringRef)RSArrayObjectAtIndex(contents, idx)))
                    tryPlatformSpecific = YES;
            }
        }
        if (tryPlatformSpecific)
            RSBundleCreateDataAndPropertiesFromResource(RSAllocatorSystemDefault, infoPath, &infoData, nil, nil, nil);
        //fprintf(stderr, "looking for ");RSShow(infoPath);fprintf(stderr, infoData ? "found it\n" : (tryPlatformSpecific ? "missed it\n" : "skipped it\n"));
        RSRelease(cheapStr);
        
        if (!infoData)
        {
            RSRelease(infoPath);
            infoPath = RSMutableCopy(RSAllocatorSystemDefault, bundlePath);
            RSFileManagerAppendFileName(fmg, (RSMutableStringRef)infoPath, infoPathFromRSBase);
            if (contents)
            {
                RSIndex idx ;
                for (tryRSSpecific = NO, idx = 0; !tryRSSpecific && idx < contentsRange.length; idx++)
                    if (RSCompareEqualTo == RSStringCompareCaseInsensitive(_RSBundleInfoFileRSName, (RSStringRef)RSArrayObjectAtIndex(contents, idx)))
                        tryRSSpecific = YES;
            }
        }
        if (tryRSSpecific)
            RSBundleCreateDataAndPropertiesFromResource(RSAllocatorSystemDefault, infoPath, &infoData, nil, nil, nil);
        if (!infoData)
        {
            // Check for global Info.plist
            RSRelease(infoPath);
            infoPath = RSMutableCopy(RSAllocatorSystemDefault, bundlePath);
            RSFileManagerAppendFileName(fmg, (RSMutableStringRef)infoPath, infoPathFromBase);
            if (contents)
            {
                RSIndex idx;
                for (tryGlobal = NO, idx = 0; !tryGlobal && idx < contentsRange.length; idx++)
                {
                    // Need to do this case-insensitive to accommodate Palm
                    if (RSCompareEqualTo == RSStringCompareCaseInsensitive(_RSBundleInfoFileName, (RSStringRef)RSArrayObjectAtIndex(contents, idx)))
                        tryGlobal = YES;
                }
            }
            if (tryGlobal)
                RSBundleCreateDataAndPropertiesFromResource(RSAllocatorSystemDefault, infoPath, &infoData, nil, nil, nil);
            //fprintf(stderr, "looking for ");RSShow(infoPath);fprintf(stderr, infoData ? "found it\n" : (tryGlobal ? "missed it\n" : "skipped it\n"));
            // infoPath = /Library/Frameworks/RSCoreFoundation.framework/Versions/A/Resources/RSRSPreference.bundle/Contents/Info.plist
        }
        
        if (infoData)
        {
            RSPropertyListRef plist = nil;
            plist = RSPropertyListCreateWithXMLData(alloc, infoData);
            result = RSRetain(RSPropertyListGetContent(plist));
            RSRelease(plist);
            if (result)
            {
                if (RSDictionaryGetTypeID() == RSGetTypeID(result))
                {
                    RSDictionarySetValue((RSMutableDictionaryRef)result, _RSBundleInfoPlistPathKey, infoPath);
                }
                else
                {
                    if (!_RSAllocatorIsGCRefZero(alloc)) RSRelease(result);
                    result = nil;
                }
            }
            if (!result) rawInfoPath = infoPath;
            RSRelease(infoData);
        }
        if (!result)
        {
            result = RSDictionaryCreateMutable(alloc, 0, RSDictionaryRSTypeContext);
            if (rawInfoPath) RSDictionarySetValue((RSMutableDictionaryRef)result, _RSBundleRawInfoPlistPathKey, rawInfoPath);
        }
        
        RSRelease(infoPath);
//        if (contents) RSRelease(contents);
    }
    _processInfoDictionary((RSMutableDictionaryRef)result, _RSGetPlatformName(), _RSGetProductName());
    return result;
}

static BOOL _RSBundleCouldBeBundle(RSStringRef bundlePath)
{
    BOOL isDirectory = NO;
    if (RSFileManagerFileExistsAtPath(RSFileManagerGetDefault(), bundlePath, &isDirectory) == YES && isDirectory == YES)
    {
        __block BOOL result = NO;
        RSAutoreleaseBlock(^{
            RSDictionaryRef property = RSFileManagerAttributesOfDirectoryAtPath(RSFileManagerGetDefault(), bundlePath);
            if (property)
            {
                RSNumberRef posixMode = RSDictionaryGetValue(property, RSFilePosixPermissions);
                unsigned int mode = 0;
                if (RSNumberGetValue(posixMode, &mode))
                {
                    if (mode & 0x444) result = YES;
                }
            }
        });
        return result;
    }
    return NO;
}

#pragma mark __RSBundleGTable API

extern const char* dyld_image_path_containing_address(const void* addr);
static pthread_mutex_t RSBundleGlobalDataLock = PTHREAD_MUTEX_INITIALIZER;
static RSMutableDictionaryRef __bundleCacheByIdentifier = nil;
static RSMutableDictionaryRef __bundleCacheByPath = nil;
static RSMutableArrayRef __allBundles = nil;
static RSMutableSetRef __bundlesToUnload = nil;

static BOOL _scheduledBundlesAreUnloading = 0;

static RSBundleRef _mainBundle = nil;

RSPrivate void __RSBundleGLock()
{
    pthread_mutex_lock(&RSBundleGlobalDataLock);
}

RSPrivate void __RSBundleGUnlock()
{
    pthread_mutex_unlock(&RSBundleGlobalDataLock);
}


RSPrivate RSBundleRef _RSBundleCopyBundleForPath(RSStringRef path, BOOL alreadyLocked)
{
    RSBundleRef result = nil;
    if (!alreadyLocked) __RSBundleGLock();
    if (__bundleCacheByPath) result = (RSBundleRef)RSDictionaryGetValue(__bundleCacheByPath, path);
    if (result && !result->_path)
    {
        result = nil;
        RSDictionaryRemoveValue(__bundleCacheByPath, path);
    }
    if (result) RSRetain(result);
    if (!alreadyLocked) __RSBundleGUnlock();
    return result;
}

#include <RSCoreFoundation/RSString+Extension.h>
static RSStringRef __RSBundleCopyFrameworkPathForExecutablePath(RSStringRef executablePath, BOOL permissive);
RSPrivate void _RSBundleInitMainBundle()
{
    if (!_mainBundle)
    {
        RSBlock path[RSBufferSize] = {0};
        _RSGetExecutablePath(path, RSBufferSize);
        
        //        _RSGetCurrentDirectory(path, RSBufferSize);
        RSStringRef mainBundleExecutablePath = RSStringCreateWithCString(RSAllocatorSystemDefault, path, __RSDefaultEightBitStringEncoding);
        RSStringRef bundlePath = __RSBundleCopyFrameworkPathForExecutablePath(mainBundleExecutablePath, YES);
        if (bundlePath) _mainBundle = RSBundleCreateFromFileSystem(RSAllocatorSystemDefault, bundlePath);
        if (_mainBundle) __RSRuntimeSetInstanceSpecial(_mainBundle, YES);
        RSRelease(mainBundleExecutablePath);
        RSRelease(bundlePath);
    }
}

RSPrivate void _RSBundleFinalMainBundle()
{
    if (_mainBundle)
    {
        __RSRuntimeSetInstanceSpecial(_mainBundle, NO);
        RSRelease(_mainBundle);
        _mainBundle = nil;
    }
}

RSPrivate void _RSBundleAddToTables(RSBundleRef bundle, BOOL alreadyLocked)
{
    if (alreadyLocked == NO) __RSBundleGLock();
    if (unlikely(__bundleCacheByIdentifier == nil))
        __bundleCacheByIdentifier = RSDictionaryCreateMutable(RSAllocatorSystemDefault, 0, RSDictionaryRSTypeContext);
    if (unlikely(__bundleCacheByPath == nil))
        __bundleCacheByPath = RSDictionaryCreateMutable(RSAllocatorSystemDefault, 0, RSDictionaryRSTypeContext);
    if (unlikely(__allBundles == nil))
        __allBundles = RSArrayCreateMutable(RSAllocatorSystemDefault, 32);
    if (unlikely(__bundlesToUnload == nil))
        __bundlesToUnload = RSSetCreateMutable(RSAllocatorSystemDefault, 0, &RSTypeSetCallBacks);
    RSStringRef bid = RSBundleGetIdentifier(bundle);
    if (bid)
    {
        RSMutableArrayRef bundlesWithThisId = (RSMutableArrayRef)RSDictionaryGetValue(__bundleCacheByIdentifier, bid);
        if (bundlesWithThisId == nil) bundlesWithThisId = RSArrayCreateMutable(RSAllocatorSystemDefault, 0);
        if (RSArrayGetCount(bundlesWithThisId))
        {
            RSArrayInsertObjectAtIndex(bundlesWithThisId, 0, bundle);
        }
        else
        {
            RSArrayAddObject(bundlesWithThisId, bundle);
            RSDictionarySetValue(__bundleCacheByIdentifier, bid, bundlesWithThisId);
            RSRelease(bundlesWithThisId);
        }
    }
    bid = RSBundleGetPath(bundle);
    RSBundleRef bld = (RSBundleRef)RSDictionaryGetValue(__bundleCacheByPath, bid);
    if (bld == nil) RSDictionarySetValue(__bundleCacheByPath, bid, bundle);
    if (bid && bld)
    {
        if (NO == RSEqual(bld, bundle))
        {
            __RSCLog(RSLogLevelNotice, "__RSBundle Bundle has a conflict!\n");
            RSDictionarySetValue(__bundleCacheByPath, bid, bundle);
        }
    }
    if (RSNotFound == RSArrayIndexOfObject(__allBundles, bundle)) RSArrayAddObject(__allBundles, bundle);
    
    __RSBundleGUnlock();
}

RSPrivate void _RSBundleRemoveFromTables(RSBundleRef bundle, BOOL alreadyLocked)
{
    if (alreadyLocked == NO) __RSBundleGLock();
    if (unlikely(__bundleCacheByIdentifier == nil)) __bundleCacheByIdentifier = RSDictionaryCreateMutable(RSAllocatorSystemDefault, 0, RSDictionaryRSTypeContext);
    if (unlikely(__bundleCacheByPath == nil)) __bundleCacheByPath = RSDictionaryCreateMutable(RSAllocatorSystemDefault, 0, RSDictionaryRSTypeContext);
    if (unlikely(__allBundles == nil)) __allBundles = RSArrayCreateMutable(RSAllocatorSystemDefault, 32);
    if (unlikely(__bundlesToUnload == nil)) __bundlesToUnload = RSSetCreateMutable(RSAllocatorSystemDefault, 0, &RSTypeSetCallBacks);
    RSBundleRef bld1 = (RSBundleRef)RSDictionaryGetValue(__bundleCacheByPath, RSBundleGetPath(bundle));
    RSMutableArrayRef bundles = (RSMutableArrayRef)RSDictionaryGetValue(__bundleCacheByIdentifier, RSBundleGetIdentifier(bundle));
    if (bld1 && bundles)
    {
        RSDictionaryRemoveValue(__bundleCacheByPath, RSBundleGetPath(bundle));
        if (bundles)
        {
            RSArrayRemoveObject(bundles, bundle);
        }
        RSArrayRemoveObject(__allBundles, bundle);
        RSSetAddValue(__bundlesToUnload, bundle);
    }
    if (alreadyLocked == NO) __RSBundleGUnlock();
}

RSPrivate void _RSBundleScheduleNeedUnload(BOOL alreadyLocked)
{
    if (!alreadyLocked) __RSBundleGLock();
    _scheduledBundlesAreUnloading = YES;
    RSTypeRef _bundles[256] = {0};
    RSTypeRef* _bundlesPtr = &_bundles[0];
    if (!__bundlesToUnload) goto END;
    RSIndex cnt = RSSetGetCount(__bundlesToUnload);
    if (cnt > 0)
    {
        _bundlesPtr = (cnt <= 256) ? _bundles : RSAllocatorAllocate(RSAllocatorSystemDefault, sizeof(RSTypeRef) * cnt);
        RSSetGetValues(__bundlesToUnload, _bundlesPtr);
        for (RSIndex idx = 0; idx < cnt; idx++)
        {
            RSBundleRef bundle = (RSBundleRef)_bundlesPtr[idx];
            _RSBundleDlfcnUnload(bundle);
        }
        if (_bundlesPtr != &_bundles[0]) RSAllocatorDeallocate(RSAllocatorSystemDefault, _bundlesPtr);
        _bundlesPtr = nil;
    }
    
    _scheduledBundlesAreUnloading = NO;
END:
    if (!alreadyLocked) __RSBundleGUnlock();
}

RSPrivate RSBundleRef _RSBundlePrimitiveGetBundleWithIdentifierAlreadyLocked(RSStringRef bundleID)
{
    RSBundleRef result = nil, bundle;
    if (__bundleCacheByIdentifier && bundleID)
    {
        // Note that this array is maintained in descending order by version number
        RSArrayRef bundlesWithThisID = (RSArrayRef)RSDictionaryGetValue(__bundleCacheByIdentifier, bundleID);
        if (bundlesWithThisID)
        {
            RSIndex i, count = RSArrayGetCount(bundlesWithThisID);
            if (count > 0)
            {
                // First check for loaded bundles so we will always prefer a loaded to an unloaded bundle
                for (i = 0; !result && i < count; i++)
                {
                    bundle = (RSBundleRef)RSArrayObjectAtIndex(bundlesWithThisID, i);
                    if (RSBundleIsExecutableLoaded(bundle)) result = bundle;
                }
                // If no loaded bundle, simply take the first item in the array, i.e. the one with the latest version number
                if (!result) result = (RSBundleRef)RSArrayObjectAtIndex(bundlesWithThisID, 0);
            }
        }
    }
    return result;
}

RSPrivate void _RSBundlePrivateCacheDeallocate(BOOL alreadyLocked)
{
    if (!alreadyLocked) __RSBundleGLock();
    _RSBundleFinalMainBundle();
    _RSBundleScheduleNeedUnload(YES);
    if (__bundleCacheByPath) RSRelease(__bundleCacheByPath);
    if (__bundleCacheByIdentifier) RSRelease(__bundleCacheByIdentifier);
    if (__bundlesToUnload) RSRelease(__bundlesToUnload);
    if (__allBundles) RSRelease(__allBundles);
    __allBundles = nil;
    __bundleCacheByPath = nil;
    __bundleCacheByIdentifier = nil;
    __bundlesToUnload = nil;
    if (!alreadyLocked) __RSBundleGUnlock();
}

RSPrivate RSBundleRef _RSMainBundle()
{
    return _mainBundle;
}

#if defined(BINARY_SUPPORT_DLFCN)

RSPrivate BOOL _RSBundleDlfcnCheckLoaded(RSBundleRef bundle)
{
    if (!bundle->_isLoaded)
    {
        RSStringRef executablePath = RSBundleCopyExecutablePath(bundle);
        
        if (executablePath)
        {
            RSHandle handle = _RSLoadLibrary(executablePath);
            if (handle)
            {
                if (!bundle->_handleCookie)
                {
                    bundle->_handleCookie = handle;
#if LOG_BUNDLE_LOAD
                    printf("dlfcn check load bundle %p, dlopen of %s mode 0x%x getting handle %p\n", bundle, buff, mode, bundle->_handleCookie);
#endif /* LOG_BUNDLE_LOAD */
                }
                bundle->_isLoaded = YES;
            }
            else
            {
#if LOG_BUNDLE_LOAD
                printf("dlfcn check load bundle %p, dlopen of %s mode 0x%x no handle\n", bundle, buff, mode);
#endif /* LOG_BUNDLE_LOAD */
            }
        }
        if (executablePath) RSRelease(executablePath);
    }
    return bundle->_isLoaded;
}

RSArrayRef RSBundleCopyExecutableArchitectures(RSBundleRef bundle) {
    RSArrayRef result = nil;
    RSStringRef executablePath = RSBundleCopyExecutablePath(bundle);
    if (executablePath) {
        result = _RSBundleCopyArchitecturesForExecutable(executablePath);
        RSRelease(executablePath);
    }
    return result;
}

static RSErrorRef _RSBundleCreateErrorDebug(RSAllocatorRef allocator, RSBundleRef bundle, RSIndex code, RSStringRef debugString)
{
    RSErrorRef error = nil;
    RSMutableDictionaryRef userInfo = RSDictionaryCreateMutable(allocator, 0, RSDictionaryRSTypeContext);
    RSMutableStringRef description = RSStringCreateMutable(allocator, 0);
    switch (code)
    {
            /*
             RSBundleExecutableNotFoundError             = 4,
             RSBundleExecutableNotLoadableError          = 3584,
             RSBundleExecutableArchitectureMismatchError = 3585,
             RSBundleExecutableRuntimeMismatchError      = 3586,
             RSBundleExecutableLoadError                 = 3587,
             RSBundleExecutableLinkError                 = 3588
             */
        case RSBundleExecutableNotFoundError:
            RSStringAppendStringWithFormat(description, RSSTR("The bundle %R couldn't be loaded because its executable couldn't be located."), bundle);
            break;
        case RSBundleExecutableNotLoadableError:
            RSStringAppendStringWithFormat(description, RSSTR("The bundle %R couldn't be loaded because its executable isn't loadable."), bundle);
            break;
        case RSBundleExecutableArchitectureMismatchError:
            RSStringAppendStringWithFormat(description, RSSTR("The bundle %R couldn't be loaded because it doesn't contain a version for the current architecture."), bundle);
            break;
        case RSBundleExecutableRuntimeMismatchError:
            RSStringAppendStringWithFormat(description, RSSTR("The bundle %R couldn't be loaded because it isn't compatible with the current application."), bundle);
            break;
        case RSBundleExecutableLoadError:
            RSStringAppendStringWithFormat(description, RSSTR("The bundle %R couldn't be loaded because it is damaged or missing necessary resources."), bundle);
            break;
        case RSBundleExecutableLinkError:
            RSStringAppendStringWithFormat(description, RSSTR("The bundle %R couldn't be loaded."), bundle);
            break;
        default:
            break;
    }
    if (RSStringGetLength(description))
        RSDictionarySetValue(userInfo, RSErrorDescriptionKey, description);
    if (RSDictionaryGetCount(userInfo))
        error = RSErrorCreate(allocator, RSErrorDomainRSCoreFoundation, code, userInfo);
    RSRelease(description);
    RSRelease(userInfo);
    return error;
}

#if DEPLOYMENT_TARGET_MACOSX || DEPLOYMENT_TARGET_EMBEDDED
static BOOL _RSBundleGetObjCImageInfo(RSBundleRef bundle, uint32_t *objcVersion, uint32_t *objcFlags)
{
    BOOL retval = NO;
    uint32_t localVersion = 0, localFlags = 0;
    RSStringRef executablePath = RSBundleCopyExecutablePath(bundle);
    if (executablePath)
    {
        retval = _RSBundleGetObjCImageInfoForExecutable(executablePath, &localVersion, &localFlags);
        RSRelease(executablePath);
    }
    if (objcVersion) *objcVersion = localVersion;
    if (objcFlags) *objcFlags = localFlags;
    return retval;
}
#endif

RSExport BOOL _RSBundleDlfcnPreflight(RSBundleRef bundle, __autorelease RSErrorRef *error)
{
    BOOL retval = YES;
    RSErrorRef localError = nil;
    if (!bundle->_isLoaded)
    {
        RSStringRef executablePath = RSBundleCopyExecutablePath(bundle);
        RSBlock buff[RSMaxPathSize];
        
        retval = NO;
        if (executablePath)
        {
            RSStringGetCString(executablePath, buff, RSMaxPathSize, __RSDefaultEightBitStringEncoding);
            retval = dlopen_preflight((RSCBuffer)buff);
            if (!retval && error)
            {
                RSArrayRef archs = RSBundleCopyExecutableArchitectures(bundle);
                RSStringRef debugString = nil;
                const char *errorString = dlerror();
                if (errorString && strlen(errorString) > 0) debugString = RSCopy(RSAllocatorSystemDefault, errorString);
                if (archs)
                {
                    BOOL hasSuitableArch = NO, hasRuntimeMismatch = NO;
                    RSIndex i, count = RSArrayGetCount(archs);
                    SInt32 arch, curArch = _RSBundleCurrentArchitecture();
                    for (i = 0; !hasSuitableArch && i < count; i++)
                    {
                        if (RSNumberGetValue((RSNumberRef)RSArrayObjectAtIndex(archs, i), (void *)&arch) && arch == curArch) hasSuitableArch = YES;
                    }
#if defined(BINARY_SUPPORT_DYLD)
                    if (hasSuitableArch)
                    {
                        uint32_t mainFlags = 0;
                        if (_RSBundleGrokObjCImageInfoFromMainExecutable(nil, &mainFlags) && (mainFlags & 0x2) != 0)
                        {
#if DEPLOYMENT_TARGET_MACOSX || DEPLOYMENT_TARGET_EMBEDDED
                            uint32_t bundleFlags = 0;
                            if (_RSBundleGetObjCImageInfo(bundle, nil, &bundleFlags) && (bundleFlags & 0x2) == 0) hasRuntimeMismatch = YES;
#endif
                        }
                    }
#endif /* BINARY_SUPPORT_DYLD */
                    if (hasRuntimeMismatch) {
                        //localError = _RSBundleCreateErrorDebug(RSGetAllocator(bundle), bundle, RSBundleExecutableRuntimeMismatchError, debugString);
                    } else if (!hasSuitableArch) {
                        //localError = _RSBundleCreateErrorDebug(RSGetAllocator(bundle), bundle, RSBundleExecutableArchitectureMismatchError, debugString);
                    } else {
                        //localError = _RSBundleCreateErrorDebug(RSGetAllocator(bundle), bundle, RSBundleExecutableLoadError, debugString);
                    }
                    RSRelease(archs);
                } else {
                    //localError = _RSBundleCreateErrorDebug(RSGetAllocator(bundle), bundle, RSBundleExecutableLoadError, debugString);
                }
                if (debugString) RSRelease(debugString);
            }
        } else {
            //if (error) localError = _RSBundleCreateError(RSGetAllocator(bundle), bundle, RSBundleExecutableNotFoundError);
        }
        if (executablePath) RSRelease(executablePath);
    }
    if (!retval && error) *error = localError;
    return retval;
}

RSPrivate BOOL _RSBundleDlfcnLoadBundle(RSBundleRef bundle, BOOL forceGlobal, RSErrorRef *error)
{
    RSErrorRef localError = nil, *subError = (error ? &localError : nil);
    if (!bundle->_isLoaded)
    {
        RSStringRef executablePath = RSBundleCopyExecutablePath(bundle);
        RSBlock buff[RSMaxPathSize];
        if (executablePath)
        {
            RSStringGetCString(executablePath, buff, RSMaxPathSize, __RSDefaultEightBitStringEncoding);
            int mode = forceGlobal ? (RTLD_LAZY | RTLD_GLOBAL | RTLD_FIRST) : (RTLD_NOW | RTLD_LOCAL | RTLD_FIRST);
            void *cookie = dlopen((RSCBuffer)buff, mode);
#if LOG_BUNDLE_LOAD
            printf("dlfcn load bundle %p, dlopen of %s mode 0x%x returns handle %p\n", bundle, buff, mode, cookie);
#endif /* LOG_BUNDLE_LOAD */
            if (cookie && cookie == bundle->_handleCookie)
            {
                // during the call to dlopen, arbitrary init routines may have run and caused bundle->_handleCookie to be set, in which case proper reference counting requires that reference to be released with dlclose
#if LOG_BUNDLE_LOAD
                printf("dlfcn load bundle %p closing existing reference to handle %p\n", bundle, cookie);
#endif /* LOG_BUNDLE_LOAD */
                dlclose(bundle->_handleCookie);
            }
            bundle->_handleCookie = cookie;
            if (bundle->_handleCookie)
            {
                bundle->_isLoaded = YES;
            }
            else
            {
                const char *errorString = dlerror();
                if (error)
                {
                    _RSBundleDlfcnPreflight(bundle, subError);
                    if (!localError)
                    {
                        //                        RSStringRef debugString = errorString ? RSStringCreateWithFileSystemRepresentation(RSAllocatorSystemDefault, errorString) : nil;
                        //                        localError = _RSBundleCreateErrorDebug(RSGetAllocator(bundle), bundle, RSBundleExecutableLinkError, debugString);
                        //                        if (debugString) RSRelease(debugString);
                    }
                }
                else
                {
                    RSStringRef executableString = RSStringCreateWithCString(RSAllocatorSystemDefault, (RSCBuffer)buff, RSStringEncodingUTF8);
                    if (errorString)
                    {
                        RSStringRef debugString = RSStringCreateWithCString(RSAllocatorSystemDefault, errorString, RSStringEncodingUTF8);
                        RSLog(RSSTR("Error loading %R:  %R"), executableString, debugString);
                        if (debugString) RSRelease(debugString);
                    }
                    else
                    {
                        RSLog(RSSTR("Error loading %R"), executableString);
                    }
                    if (executableString) RSRelease(executableString);
                }
            }
        }
        else
        {
            if (error)
            {
                //localError = _RSBundleCreateError(RSGetAllocator(bundle), bundle, RSBundleExecutableNotFoundError);
            }
            else
            {
                RSLog(RSSTR("Cannot find executable for bundle %R"), bundle);
            }
        }
        if (executablePath) RSRelease(executablePath);
    }
    if (!bundle->_isLoaded && error) *error = localError;
    return bundle->_isLoaded;
}

RSPrivate BOOL _RSBundleDlfcnLoadFramework(RSBundleRef bundle, RSErrorRef *error)
{
    RSErrorRef localError = nil, *subError = (error ? &localError : nil);
    if (!bundle->_isLoaded)
    {
        RSStringRef executablePath = RSBundleCopyExecutablePath(bundle);
        RSBlock buff[RSMaxPathSize];
        if (executablePath)
        {
            RSStringGetCString(executablePath, buff, RSMaxPathSize, __RSDefaultEightBitStringEncoding);
            int mode = RTLD_LAZY | RTLD_GLOBAL | RTLD_FIRST;
            void *cookie = dlopen((RSCBuffer)buff, mode);
#if LOG_BUNDLE_LOAD
            printf("dlfcn load framework %p, dlopen of %s mode 0x%x returns handle %p\n", bundle, buff, mode, cookie);
#endif /* LOG_BUNDLE_LOAD */
            if (cookie && cookie == bundle->_handleCookie)
            {
                // during the call to dlopen, arbitrary init routines may have run and caused bundle->_handleCookie to be set, in which case proper reference counting requires that reference to be released with dlclose
#if LOG_BUNDLE_LOAD
                printf("dlfcn load framework %p closing existing reference to handle %p\n", bundle, cookie);
#endif /* LOG_BUNDLE_LOAD */
                dlclose(bundle->_handleCookie);
            }
            bundle->_handleCookie = cookie;
            if (bundle->_handleCookie)
            {
                bundle->_isLoaded = YES;
            }
            else
            {
                const char *errorString = dlerror();
                if (error)
                {
                    _RSBundleDlfcnPreflight(bundle, subError);
                    if (!localError)
                    {
                        //                        RSStringRef debugString = errorString ? RSStringCreateWithFileSystemRepresentation(RSAllocatorSystemDefault, errorString) : nil;
                        //                        localError = _RSBundleCreateErrorDebug(RSGetAllocator(bundle), bundle, RSBundleExecutableLinkError, debugString);
                        //                        if (debugString) RSRelease(debugString);
                    }
                }
                else
                {
                    RSStringRef executableString = RSStringCreateWithCString(RSAllocatorSystemDefault, (RSCBuffer)buff, RSStringEncodingUTF8);
                    if (errorString)
                    {
                        RSStringRef debugString = RSStringCreateWithCString(RSAllocatorSystemDefault, errorString, RSStringEncodingUTF8);
                        RSLog(RSSTR("Error loading %R:  %R"), executableString, debugString);
                        if (debugString) RSRelease(debugString);
                    }
                    else
                    {
                        RSLog(RSSTR("Error loading %R"), executableString);
                    }
                    if (executableString) RSRelease(executableString);
                }
            }
        }
        else
        {
            if (error)
            {
                //localError = _RSBundleCreateError(RSGetAllocator(bundle), bundle, RSBundleExecutableNotFoundError);
            }
            else
            {
                RSLog(RSSTR("Cannot find executable for bundle %R"), bundle);
            }
        }
        if (executablePath) RSRelease(executablePath);
    }
    if (!bundle->_isLoaded && error) *error = localError;
    return bundle->_isLoaded;
}

#endif

#include <mach-o/dyld.h>
RSPrivate RSStringRef _RSBundleDYLDCopyLoadedImagePathForPointer(void *p)
{
    RSStringRef result = nil;
    const char *name = dyld_image_path_containing_address(p);
    if (name) result = RSStringCreateWithCString(RSAllocatorSystemDefault, name, __RSDefaultEightBitStringEncoding);
    if (!result)
    {
        uint32_t i, j, n = _dyld_image_count();
        BOOL foundit = NO;
        const char *name;
#if __LP64__
#define MACH_HEADER_TYPE struct mach_header_64
#define MACH_SEGMENT_CMD_TYPE struct segment_command_64
#define MACH_SEGMENT_FLAVOR LC_SEGMENT_64
#else
#define MACH_HEADER_TYPE struct mach_header
#define MACH_SEGMENT_CMD_TYPE struct segment_command
#define MACH_SEGMENT_FLAVOR LC_SEGMENT
#endif
        for (i = 0; !foundit && i < n; i++)
        {
            const MACH_HEADER_TYPE *mh = (const MACH_HEADER_TYPE *)_dyld_get_image_header(i);
            uintptr_t addr = (uintptr_t)p - _dyld_get_image_vmaddr_slide(i);
            
            if (mh)
            {
                struct load_command *lc = (struct load_command *)((char *)mh + sizeof(MACH_HEADER_TYPE));
                for (j = 0; !foundit && j < mh->ncmds; j++, lc = (struct load_command *)((char *)lc + lc->cmdsize))
                {
                    if (MACH_SEGMENT_FLAVOR == lc->cmd && ((MACH_SEGMENT_CMD_TYPE *)lc)->vmaddr <= addr && addr < ((MACH_SEGMENT_CMD_TYPE *)lc)->vmaddr + ((MACH_SEGMENT_CMD_TYPE *)lc)->vmsize)
                    {
                        foundit = YES;
                        name = _dyld_get_image_name(i);
                        if (name) result = RSStringCreateWithCString(RSAllocatorSystemDefault, name, __RSDefaultEightBitStringEncoding);
                    }
                }
            }
        }
#undef MACH_HEADER_TYPE
#undef MACH_SEGMENT_CMD_TYPE
#undef MACH_SEGMENT_FLAVOR
    }
    return result;
}

#define __RSBundleSupportCurrentVersion RSSTR("1.0")

static RSBundleRef __RSBundleCreateInstance(RSAllocatorRef allocator,
                                            RSStringRef bundlePath,
                                            RSDictionaryRef plistAboutInfo,
                                            RSDictionaryRef plistAboutVersion,
                                            RSBitU8 version,
                                            BOOL alreadyLocked)
{
    assert(bundlePath);
    assert(plistAboutInfo);
    BOOL RSSpeicific = NO;
    RSStringRef executableName = RSDictionaryGetValue(plistAboutInfo, _RSBundleExecutableRSNameKey);
    if (executableName == nil) executableName = RSDictionaryGetValue(plistAboutInfo, _RSBundleExecutableCFNameKey);
    else RSSpeicific = YES;
    RSNumberRef speicific = RSNumberCreateBoolean(RSAllocatorSystemDefault, RSSpeicific);
    RSDictionarySetValue((RSMutableDictionaryRef)plistAboutInfo, _RSBundleHasSpeicificInfoKey, speicific);
    RSRelease(speicific);
    
    RSBundleRef bundle = (RSBundleRef)__RSRuntimeCreateInstance(allocator, _RSBundleTypeID, sizeof(struct __RSBundle) - sizeof(RSRuntimeBase));
    
    bundle->_version = version;
    bundle->_infoDict = RSRetain(plistAboutInfo);
    bundle->_path = RSRetain(bundlePath);
    bundle->_queryTableLock = RSSpinLockInit;
    bundle->_queryTable = __RSBundleCreateQueryTableWithBundle(allocator, bundle);
    RSMutableStringRef path = (executableName) ? RSMutableCopy(allocator, bundlePath) : nil;
    RSFileManagerRef fmg = RSFileManagerGetDefault();
    if (path)
    {
        BOOL appendPlatform = YES;
        switch (bundle->_version)
        {
            case 0:
                appendPlatform = NO;
                break;
            case 1:
                RSFileManagerAppendFileName(fmg, path, _RSBundleExecutablesPathFromBase1);
                break;
            case 2:
                RSFileManagerAppendFileName(fmg, path, _RSBundleExecutablesPathFromBase2);
                break;
            default:
                break;
        }
        if (appendPlatform == YES)
        {
            RSFileManagerAppendFileName(fmg, path, _RSGetPlatformName());
        }
        RSFileManagerAppendFileName(RSFileManagerGetDefault(), path, executableName);
        bundle->_bundleType = _RSBundleGrokBinaryType(path);
        if (!(bundle->_bundleType == __RSBundleUnreadableBinary ||
            bundle->_bundleType == __RSBundleNoBinary))
        {
            RSDictionarySetValue((RSMutableDictionaryRef)bundle->_infoDict, _RSBundleExecutablePathKey, path);
        }
        RSRelease(path);
    }
    if (bundle->_infoDict)
    {
        RSStringRef bid = RSDictionaryGetValue(bundle->_infoDict, RSBundleIdentifierKey);
        if (!bid) bid = RSDictionaryGetValue(bundle->_infoDict, _RSBundleIdentifierCFNameKey);
        if (bid)
            RSDictionarySetValue((RSMutableDictionaryRef)bundle->_infoDict, RSBundleIdentifierKey, bid);
    }
    _RSBundleAddToTables(bundle, alreadyLocked);
    _RSBundleScheduleNeedUnload(alreadyLocked);
    return bundle;
}

static RSBundleRef __RSBundleCreateInstanceFromFileSystemL(RSAllocatorRef allocator, RSStringRef bundlePath, BOOL alreadyLocked)
{
    BOOL existing = NO, isDir = NO;
    RSBundleRef bundle = nil;
    existing = RSFileManagerFileExistsAtPath(RSFileManagerGetDefault(), bundlePath, &isDir);
    if (existing == YES && isDir == YES)
    {
        RSBitU8 version = 3;
        BOOL result = NO;
        result = _RSBundlePathLooksLikeBundleVersion(bundlePath, &version);
        if (!result) version = 3;
        RSDictionaryRef dict = _RSBundleCopyInfoDictionaryInDirectoryWithVersion(RSAllocatorSystemDefault, bundlePath, version);
        if (dict)
        {
            //            RSShow(dict);
            bundle = __RSBundleCreateInstance(allocator, bundlePath, dict, nil, version, alreadyLocked);
        }
        RSRelease(dict);
    }
    return bundle;
}

static RSBundleRef __RSBundleCreateInstanceFromFileSystem(RSAllocatorRef allocator, RSStringRef bundlePath)
{
    return __RSBundleCreateInstanceFromFileSystemL(allocator, bundlePath, NO);
}

RSExport RSStringRef RSBundleGetIdentifier(RSBundleRef bundle)
{
    if (bundle == nil) return nil;
    __RSGenericValidInstance(bundle, _RSBundleTypeID);
    RSStringRef identifier = RSDictionaryGetValue(bundle->_infoDict, RSBundleIdentifierKey);
    return identifier;
}

RSExport RSStringRef RSBundleGetPath(RSBundleRef bundle)
{
    if (bundle == nil) return nil;
    __RSGenericValidInstance(bundle, _RSBundleTypeID);
    return __RSBundleGetFSPath(bundle);
}

RSExport RSStringRef RSBundleGetExecutableName(RSBundleRef bundle)
{
    if (bundle == nil) return nil;
    __RSGenericValidInstance(bundle, _RSBundleTypeID);
    RSStringRef en = RSBundleGetValueForInfoDictionaryKey(bundle, _RSBundleExecutableCFNameKey);
    if (en) return en;
    return RSBundleGetValueForInfoDictionaryKey(bundle, _RSBundleExecutableRSNameKey);
}

RSExport RSTypeRef RSBundleGetValueForInfoDictionaryKey(RSBundleRef bundle, RSStringRef key) RS_AVAILABLE(0_0)
{
    if (bundle == nil || key == nil) return nil;
    if (bundle->_infoDict)
    {
        return RSDictionaryGetValue(bundle->_infoDict, key);
    }
    return nil;
}

RSExport RSStringRef RSBundleCopyExecutablePath(RSBundleRef bundle)
{
    if (bundle == nil) return nil;
    __RSGenericValidInstance(bundle, _RSBundleTypeID);
    RSStringRef executablePath = RSDictionaryGetValue(bundle->_infoDict, _RSBundleExecutablePathKey);;
    return executablePath ? RSRetain(executablePath) : executablePath;
}

RSExport BOOL __RSBundleWriteToFileSystem(RSBundleRef bundle, RSStringRef pathToWrite, __autorelease RSErrorRef* error)
{
    return NO;
}

RSExport RSBundleRef RSBunleCreateWithPath(RSAllocatorRef allocator, RSStringRef bundlePath)
{
    if (bundlePath == nil) return nil;
    RSBundleRef bundle = __RSBundleCreateInstanceFromFileSystem(allocator, bundlePath);
    return bundle;
}

static RSArrayRef _RSBundleDYLDCopyLoadedImagePathsForHint(RSStringRef hint)
{
    uint32_t i, numImages = _dyld_image_count();
    RSMutableArrayRef result = RSArrayCreateMutable(RSAllocatorSystemDefault, 0);
    RSRange range = RSMakeRange(0, RSStringGetLength(hint)), altRange = RSMakeRange(0, 0), testRange = RSMakeRange(0, 0);
    RSBlock path[RSBufferSize] = {0};
    const char *processPath = nil;
    _RSGetExecutablePath(path, RSBufferSize);
    processPath = path;
    const void *mhp = (const void *)_NSGetMachExecuteHeader();
    
    if (range.length > 14)
    {
        // handle some common variations on framework bundle identifiers
        if (RSStringFindWithOptions(hint,
                                    RSSTR(".framework"),
                                    range,
                                    RSCompareAnchored| RSCompareBackwards| RSCompareCaseInsensitive,
                                    &testRange) &&
            testRange.location > 0 &&
            testRange.length > 0)
        {
            // identifier has .framework appended
            altRange.length = testRange.location;
        }
        else if (RSStringFindWithOptions(hint,
                                         RSSTR("framework"),
                                         range,
                                         RSCompareAnchored| RSCompareBackwards| RSCompareCaseInsensitive,
                                         &testRange) &&
                 testRange.location > 0 &&
                 testRange.length > 0)
        {
            // identifier has Framework appended
            altRange.length = testRange.location;
        }
        else if (RSStringFindWithOptions(hint,
                                         RSSTR("fw"),
                                         range,
                                         RSCompareAnchored| RSCompareBackwards| RSCompareCaseInsensitive,
                                         &testRange) &&
                 testRange.location > 0 &&
                 testRange.length > 0)
        {
            // identifier has FW appended
            altRange.length = testRange.location;
        }
    }
    for (i = 0; i < numImages; i++)
    {
        const char *curName = _dyld_get_image_name(i), *lastComponent = nil;
        if (curName && (!processPath || 0 != strcmp(curName, processPath)) && mhp != (void *)_dyld_get_image_header(i))
            lastComponent = strrchr(curName, '/');
        if (lastComponent)
        {
            RSStringRef str = RSStringCreateWithCString(RSAllocatorSystemDefault, lastComponent + 1, RSStringEncodingUTF8);
            if (str)
            {
                if (RSStringFindWithOptions(hint,
                                            str,
                                            range,
                                            RSCompareAnchored| RSCompareBackwards| RSCompareCaseInsensitive,
                                            nil) ||
                    (altRange.length > 0 &&
                     RSStringFindWithOptions(hint,
                                             str,
                                             altRange,
                                             RSCompareAnchored| RSCompareBackwards| RSCompareCaseInsensitive,
                                             nil)))
                {
                    RSStringRef curStr = RSStringCreateWithCString(RSAllocatorSystemDefault, curName, RSStringEncodingUTF8);
                    if (curStr)
                    {
                        RSArrayAddObject(result, curStr);
                        RSRelease(curStr);
                    }
                }
                RSRelease(str);
            }
        }
    }
    return result;
}

static RSStringRef __RSBundleCopyFrameworkPathForExecutablePath(RSStringRef executablePath, BOOL permissive)
{
    // MF:!!! Implement me.  We need to be able to find the bundle from the exe, dealing with old vs. new as well as the Executables dir business on Windows.
#if DEPLOYMENT_TARGET_WINDOWS
    UniChar executablesToFrameworksPathBuff[] = {'.', '.', '\\', 'F', 'r', 'a', 'm', 'e', 'w', 'o', 'r', 'k', 's'};
    UniChar executablesToPrivateFrameworksPathBuff[] = {'.', '.', '\\', 'P', 'r', 'i', 'v', 'a', 't', 'e', 'F', 'r', 'a', 'm', 'e', 'w', 'o', 'r', 'k', 's'};
    UniChar frameworksExtension[] = {'f', 'r', 'a', 'm', 'e', 'w', 'o', 'r', 'k'};
#endif
    UniChar pathBuff[RSMaxPathSize] = {0};
    UniChar nameBuff[RSMaxPathSize] = {0};
    RSIndex length, nameStart, nameLength, savedLength;
    RSMutableStringRef cheapStr = RSStringCreateMutableWithExternalCharactersNoCopy(RSAllocatorSystemDefault, nil, 0, 0, nil);
    RSStringRef bundleURL = nil;
    
    length = RSStringGetLength(executablePath);
    if (length > RSMaxPathSize) length = RSMaxPathSize;
    RSStringGetCharacters(executablePath, RSMakeRange(0, length), pathBuff);
    
    // Save the name in nameBuff
    length = _RSLengthAfterDeletingPathExtension(pathBuff, length);
    nameStart = _RSStartOfLastPathComponent(pathBuff, length);
    nameLength = length - nameStart;
    memmove(nameBuff, &(pathBuff[nameStart]), nameLength * sizeof(UniChar));
    
    // Strip the name from pathBuff
    length = _RSLengthAfterDeletingLastPathComponent(pathBuff, length);
    savedLength = length;
    
#if DEPLOYMENT_TARGET_WINDOWS
    // * (Windows-only) First check the "Executables" directory parallel to the "Frameworks" directory case.
    if (_RSAppendPathComponent(pathBuff, &length, RSMaxPathSize, executablesToFrameworksPathBuff, LENGTH_OF(executablesToFrameworksPathBuff)) && _RSAppendPathComponent(pathBuff, &length, RSMaxPathSize, nameBuff, nameLength) && _RSAppendPathExtension(pathBuff, &length, RSMaxPathSize, frameworksExtension, LENGTH_OF(frameworksExtension)))
    {
        RSStringSetExternalCharactersNoCopy(cheapStr, pathBuff, length, RSMaxPathSize);
        bundleURL = RSURLCreateWithFileSystemPath(RSAllocatorSystemDefault, cheapStr, PLATFORM_PATH_STYLE, YES);
        if (!_RSBundleCouldBeBundle(bundleURL)) {
            RSRelease(bundleURL);
            bundleURL = nil;
        }
    }
    // * (Windows-only) Next check the "Executables" directory parallel to the "PrivateFrameworks" directory case.
    if (!bundleURL)
    {
        length = savedLength;
        if (_RSAppendPathComponent(pathBuff, &length, RSMaxPathSize, executablesToPrivateFrameworksPathBuff, LENGTH_OF(executablesToPrivateFrameworksPathBuff)) && _RSAppendPathComponent(pathBuff, &length, RSMaxPathSize, nameBuff, nameLength) && _RSAppendPathExtension(pathBuff, &length, RSMaxPathSize, frameworksExtension, LENGTH_OF(frameworksExtension)))
        {
            RSStringSetExternalCharactersNoCopy(cheapStr, pathBuff, length, RSMaxPathSize);
            bundleURL = RSURLCreateWithFileSystemPath(RSAllocatorSystemDefault, cheapStr, PLATFORM_PATH_STYLE, YES);
            if (!_RSBundleCouldBeBundle(bundleURL)) {
                RSRelease(bundleURL);
                bundleURL = nil;
            }
        }
    }
#endif
    // * Finally check the executable inside the framework case.
    if (!bundleURL)
    {
        length = savedLength;
        // To catch all the cases, we just peel off level looking for one ending in .framework or one called "Supporting Files".
        
        RSStringRef name = permissive ? RSSTR("") : RSStringCreateWithCString(RSAllocatorSystemDefault, (const char *)nameBuff, __RSDefaultEightBitStringEncoding);
        
        while (length > 0)
        {
            RSIndex curStart = _RSStartOfLastPathComponent(pathBuff, length);
            if (curStart >= length) break;
            RSStringSetExternalCharactersNoCopy(cheapStr, &(pathBuff[curStart]), length - curStart, RSMaxPathSize - curStart);
            if (!permissive && RSEqual(cheapStr, _RSBundleResourcesDirectoryName)) break;
            if (RSEqual(cheapStr, _RSBundleSupportFilesDirectoryName1) || RSEqual(cheapStr, _RSBundleSupportFilesDirectoryName2))
            {
                if (!permissive)
                {
                    RSIndex fmwkStart = _RSStartOfLastPathComponent(pathBuff, length);
                    RSStringSetExternalCharactersNoCopy(cheapStr, &(pathBuff[fmwkStart]), length - fmwkStart, RSMaxPathSize - fmwkStart);
                }
                if (permissive || RSStringHasPrefix(cheapStr, name))
                {
                    length = _RSLengthAfterDeletingLastPathComponent(pathBuff, length);
                    RSStringSetExternalCharactersNoCopy(cheapStr, pathBuff, length, RSMaxPathSize);
                    
//                    bundleURL = RSURLCreateWithFileSystemPath(RSAllocatorSystemDefault, cheapStr, PLATFORM_PATH_STYLE, YES);
//                    bundleURL = RSStringCreateWithCString(RSAllocatorSystemDefault, cheapStr, RSStringEncodingMacRoman);
                    bundleURL = RSCopy(RSAllocatorSystemDefault, cheapStr);
                    if (!_RSBundleCouldBeBundle(bundleURL))
                    {
                        RSRelease(bundleURL);
                        bundleURL = nil;
                    }
                    break;
                }
            }
            else if (RSStringHasSuffix(cheapStr, RSSTR(".framework")) && (permissive || RSStringHasPrefix(cheapStr, name)))
            {
                RSStringSetExternalCharactersNoCopy(cheapStr, pathBuff, length, RSMaxPathSize);
//                bundleURL = RSURLCreateWithFileSystemPath(RSAllocatorSystemDefault, cheapStr, PLATFORM_PATH_STYLE, YES);
                bundleURL = RSCopy(RSAllocatorSystemDefault, cheapStr);
                if (!_RSBundleCouldBeBundle(bundleURL))
                {
                    RSRelease(bundleURL);
                    bundleURL = nil;
                }
                break;
            }
            length = _RSLengthAfterDeletingLastPathComponent(pathBuff, length);
        }
        if (!permissive) RSRelease(name);
    }
    RSStringSetExternalCharactersNoCopy(cheapStr, nil, 0, 0);
    RSRelease(cheapStr);
    
    return bundleURL;
}

static void _RSBundleEnsureBundlesExistForImagePath(RSStringRef imagePath)
{
    // This finds the bundle for the given path.
    // If an image path corresponds to a bundle, we see if there is already a bundle instance.  If there is and it is NOT in the _dynamicBundles array, it is added to the staticBundles.  Do not add the main bundle to the list here.
    RSBundleRef bundle;
    RSStringRef curURL = __RSBundleCopyFrameworkPathForExecutablePath(imagePath, YES);
    BOOL createdBundle = NO;

    if (curURL)
    {
        bundle = _RSBundleCopyBundleForPath(curURL, YES);
        if (!bundle)
        {
            bundle = __RSBundleCreateInstanceFromFileSystemL(RSAllocatorSystemDefault, curURL, YES);
            createdBundle = YES;
        }
        if (bundle)
        {
            __RSBundleGLock();
            if (!bundle->_isLoaded)
            {
#if defined(BINARY_SUPPORT_DLFCN)
                if (!bundle->_isLoaded) _RSBundleDlfcnCheckLoaded(bundle);
#elif defined(BINARY_SUPPORT_DYLD)
                if (!bundle->_isLoaded) _RSBundleDYLDCheckLoaded(bundle);
#endif
#if defined(BINARY_SUPPORT_DYLD)
                if (bundle->_bundleType == __RSBundleUnknownBinary)
                    bundle->_bundleType = __RSBundleDYLDFrameworkBinary;
                if (bundle->_bundleType != __RSBundleRSMBinary && bundle->_bundleType != __RSBundleUnreadableBinary)
                    bundle->_resourceData._executableLacksResourceFork = YES;
#endif
                bundle->_isLoaded = YES;
            }
            __RSBundleGUnlock();
            if (createdBundle)
            {
                // init plugins
            }
            else
            {
                
            }
            RSRelease(bundle);
        }
        RSRelease(curURL);
    }
//    if (curURL)
//    {
//        bundle = _RSBundleCopyBundleForURL(curURL, YES);
//        if (!bundle)
//        {
//            // Ensure bundle exists by creating it if necessary
//            // NB doFinalProcessing must be NO here, see below
//            bundle = _RSBundleCreate(RSAllocatorSystemDefault, curURL, YES, NO);
//            createdBundle = YES;
//        }
//        if (bundle) {
//            
//            pthread_mutex_lock(&(bundle->_bundleLoadingLock));
//            if (!bundle->_isLoaded) {
//                // make sure that these bundles listed as loaded, and mark them frameworks (we probably can't see anything else here, and we cannot unload them)
//#if defined(BINARY_SUPPORT_DLFCN)
//                if (!bundle->_isLoaded) _RSBundleDlfcnCheckLoaded(bundle);
//#elif defined(BINARY_SUPPORT_DYLD)
//                if (!bundle->_isLoaded) _RSBundleDYLDCheckLoaded(bundle);
//#endif /* BINARY_SUPPORT_DLFCN */
//#if defined(BINARY_SUPPORT_DYLD)
//                if (bundle->_binaryType == __RSBundleUnknownBinary) bundle->_binaryType = __RSBundleDYLDFrameworkBinary;
//                if (bundle->_binaryType != __RSBundleRSMBinary && bundle->_binaryType != __RSBundleUnreadableBinary) bundle->_resourceData._executableLacksResourceFork = YES;
//#endif /* BINARY_SUPPORT_DYLD */
//#if LOG_BUNDLE_LOAD
//                if (!bundle->_isLoaded) printf("ensure bundle %p set loaded fallback, handle %p image %p conn %p\n", bundle, bundle->_handleCookie, bundle->_imageCookie, bundle->_connectionCookie);
//#endif /* LOG_BUNDLE_LOAD */
//                bundle->_isLoaded = YES;
//            }
//            pthread_mutex_unlock(&(bundle->_bundleLoadingLock));
//            if (createdBundle) {
//                // Perform delayed final processing steps.
//                // This must be done after _isLoaded has been set, for security reasons (3624341).
//                if (_RSBundleNeedsInitPlugIn(bundle)) {
//                    pthread_mutex_unlock(&RSBundleGlobalDataLock);
//                    _RSBundleInitPlugIn(bundle);
//                    pthread_mutex_lock(&RSBundleGlobalDataLock);
//                }
//            } else {
//                // Release the bundle if we did not create it here
//                RSRelease(bundle);
//            }
//        }
//        RSRelease(curURL);
//    }
}


static void _RSBundleEnsureBundlesExistForImagePaths(RSArrayRef imagePaths)
{
    RSUInteger cnt = RSArrayGetCount(imagePaths);
    for (RSUInteger idx = 0; idx < cnt; idx++)
    {
        RSStringRef imagePath = RSArrayObjectAtIndex(imagePaths, idx);
        _RSBundleEnsureBundlesExistForImagePath(imagePath);
    }
}

static void _RSBundleEnsureBundlesUpToDateWithHintAlreadyLocked(RSStringRef hint) {
    RSArrayRef imagePaths = nil;
    // Tickle the main bundle into existence
#if defined(BINARY_SUPPORT_DYLD)
    imagePaths = _RSBundleDYLDCopyLoadedImagePathsForHint(hint);
    RSShow(imagePaths);
#endif /* BINARY_SUPPORT_DYLD */
    if (imagePaths)
    {
        _RSBundleEnsureBundlesExistForImagePaths(imagePaths);
        RSRelease(imagePaths);
    }
}

RSExport RSDictionaryRef RSBundleGetInfoDict(RSBundleRef bundle)
{
    __RSGenericValidInstance(bundle, _RSBundleTypeID);
    return bundle->_infoDict;
}

RSExport RSBundleRef RSBundleGetWithIdentifier(RSStringRef identifier)
{
    if (identifier == nil) return RSBundleGetMainBundle();
    __RSBundleGLock();
    RSBundleRef bundle = _RSBundlePrimitiveGetBundleWithIdentifierAlreadyLocked(identifier);
    if (!bundle)
    {
        void *address = __builtin_return_address(0);
        RSStringRef imagePath = nil;
        if (address) imagePath = _RSBundleDYLDCopyLoadedImagePathForPointer(address);
        if (imagePath)
        {
            bundle = __RSBundleCreateInstanceFromFileSystemL(RSAllocatorSystemDefault, imagePath, YES);
            RSRelease(imagePath);
        }
        if (!bundle)
        {
            _RSBundleEnsureBundlesUpToDateWithHintAlreadyLocked(identifier);
            bundle = _RSBundlePrimitiveGetBundleWithIdentifierAlreadyLocked(identifier);
        }
    }
    __RSBundleGUnlock();
    return bundle;
}

RSPrivate void _RSBundleDlfcnUnload(RSBundleRef bundle)
{
    if (bundle->_isLoaded)
    {
#if LOG_BUNDLE_LOAD
        printf("dlfcn unload bundle %p, handle %p module %p image %p\n", bundle, bundle->_handleCookie, bundle->_moduleCookie, bundle->_imageCookie);
#endif /* LOG_BUNDLE_LOAD */
        if (0 != dlclose(bundle->_handleCookie))
        {
            RSLog(RSSTR("Internal error unloading bundle %R"), bundle);
        }
        else
        {
            bundle->_connectionCookie = bundle->_handleCookie = nil;
            bundle->_imageCookie = bundle->_moduleCookie = nil;
            bundle->_isLoaded = NO;
        }
    }
}

static void *_RSBundleDlfcnGetSymbolByNameWithSearch(RSBundleRef bundle, RSStringRef symbolName, BOOL globalSearch) {
    void *result = NULL;
    char buff[1026];
    
    if (RSStringGetCString(symbolName, buff, 1024, RSStringEncodingUTF8)) {
        result = dlsym(bundle->_handleCookie, buff);
        if (!result && globalSearch) result = dlsym(RTLD_DEFAULT, buff);
#if defined(DEBUG)
        if (!result) __RSLog(RSLogLevelNotice, RSSTR("dlsym cannot find symbol %@ in %@"), symbolName, bundle);
#endif /* DEBUG */
#if LOG_BUNDLE_LOAD
        printf("bundle %p handle %p module %p image %p dlsym returns symbol %p for %s\n", bundle, bundle->_handleCookie, bundle->_moduleCookie, bundle->_imageCookie, result, buff);
#endif /* LOG_BUNDLE_LOAD */
    }
    return result;
}

RSPrivate void *_RSBundleDlfcnGetSymbolByName(RSBundleRef bundle, RSStringRef symbolName) {
    return _RSBundleDlfcnGetSymbolByNameWithSearch(bundle, symbolName, false);
}

RSExport void *RSBundleGetFunctionPointerForName(RSBundleRef bundle, RSStringRef funcName) {
    void *tvp = NULL;
    // Load if necessary
    if (!bundle->_isLoaded) {
        if (!RSBundleLoadExecutable(bundle, YES)) return NULL;
    }
    
    switch (bundle->_bundleType) {
#if defined(BINARY_SUPPORT_DYLD)
        case __RSBundleDYLDBundleBinary:
        case __RSBundleDYLDFrameworkBinary:
        case __RSBundleDYLDExecutableBinary:
#if defined(BINARY_SUPPORT_DLFCN)
            if (bundle->_handleCookie) return _RSBundleDlfcnGetSymbolByName(bundle, funcName);
#else /* BINARY_SUPPORT_DLFCN */
            return _RSBundleDYLDGetSymbolByName(bundle, funcName);
#endif /* BINARY_SUPPORT_DLFCN */
            break;
#endif /* BINARY_SUPPORT_DYLD */
#if defined(BINARY_SUPPORT_DLL)
        case __RSBundleDLLBinary:
            tvp = _RSBundleDLLGetSymbolByName(bundle, funcName);
            break;
#endif /* BINARY_SUPPORT_DLL */
        default:
#if defined(BINARY_SUPPORT_DLFCN)
            if (bundle->_handleCookie) return _RSBundleDlfcnGetSymbolByName(bundle, funcName);
#endif /* BINARY_SUPPORT_DLFCN */
            break;
    }
    return tvp;
}

static BOOL __RSBundleLoadExecutableAndReturnError(RSBundleRef bundle, BOOL forceGloable, __autorelease RSErrorRef* error)
{
    BOOL result = NO;
    if (bundle->_bundleType == __RSBundleUnknownBinary ||
        bundle->_bundleType == __RSBundleUnreadableBinary ||
        bundle->_bundleType == __RSBundleNoBinary) return result;
    switch (bundle->_bundleType)
    {
        case __RSBundleUnreadableBinary:
        case __RSBundleDYLDBundleBinary:
        case __RSBundleELFBinary:
            result = _RSBundleDlfcnLoadBundle(bundle, forceGloable, error);
            break;
        case __RSBundleDYLDFrameworkBinary:
            result = _RSBundleDlfcnLoadFramework(bundle, error);
            break;
        case __RSBundleNoBinary:
        case __RSBundleUnknownBinary:
        case __RSBundleDYLDExecutableBinary:
            if (error) *error = _RSBundleCreateErrorDebug(RSAllocatorSystemDefault, bundle, RSBundleExecutableNotFoundError, nil);
            break;
        default:
            break;
    }
    return result;
}

RSExport BOOL RSBundleLoadExecutableAndReturnError(RSBundleRef bundle, BOOL forceGloable, __autorelease RSErrorRef* error)
{
    return __RSBundleLoadExecutableAndReturnError(bundle, forceGloable, error);
}

RSExport BOOL RSBundleLoadExecutable(RSBundleRef bundel, BOOL forceGloable)
{
    return RSBundleLoadExecutableAndReturnError(bundel, forceGloable, nil);
}

RSExport BOOL RSBundleIsExecutableLoaded(RSBundleRef bundle)
{
    __RSGenericValidInstance(bundle, _RSBundleTypeID);
    return bundle->_isLoaded;
}

RSExport RSBundleRef RSBundleCreateFromFileSystem(RSAllocatorRef allocator, RSStringRef bundlePath)
{
    if (bundlePath == nil) return nil;
    return __RSBundleCreateInstanceFromFileSystem(allocator, bundlePath);
}

static RSStringRef __RSBundleCreateResourcePath(RSBundleRef bundle, RSStringRef name, RSStringRef type, RSStringRef dir)
{
    RSStringRef path = nil;
    RSStringRef resourcesPath = nil;
    
    if (bundle->_version == 0)
        resourcesPath = _RSBundleResourcesPathFromBase0;
    else if (bundle->_version == 1)
        resourcesPath = _RSBundleResourcesPathFromBase1;
    else if (bundle->_version == 2)
        resourcesPath = _RSBundleResourcesPathFromBase2;
    
    if (bundle && name)
    {
        if (dir)
            if (type)
                path = RSStringCreateWithFormat(RSGetAllocator(bundle), RSSTR("%r/%r%r/%r.%r"), RSBundleGetPath(bundle), resourcesPath, dir, name, type);
            else
                path = RSStringCreateWithFormat(RSGetAllocator(bundle), RSSTR("%r/%r%r/%r"), RSBundleGetPath(bundle), resourcesPath, dir, name);
        else
            if (type)
                path = RSStringCreateWithFormat(RSGetAllocator(bundle), RSSTR("%r/%r%r.%r"), RSBundleGetPath(bundle), resourcesPath, name, type);
            else
                path = RSStringCreateWithFormat(RSGetAllocator(bundle), RSSTR("%r/%r%r"), RSBundleGetPath(bundle), resourcesPath, name);
    }
    return path;
}

RSExport RSStringRef RSBundleCreatePathForResource(RSBundleRef bundle, RSStringRef name, RSStringRef type)
{
    return __RSBundleCreateResourcePath(bundle, name, type, nil);
}

RSExport RSBundleRef RSBundleGetMainBundle()
{
    return _RSMainBundle();
}
