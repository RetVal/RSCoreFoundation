//
//  RSCFPreference.c
//  RSCoreFoundation
//
//  Created by RetVal on 2/27/13.
//  Copyright (c) 2013 RetVal. All rights reserved.
//

#include "RSCFPreference.h"
#include <RSCoreFoundation/RSStringNumberSupport.h>
#include <RSCoreFoundation/RSRuntime.h>

#ifndef ___RSCoreFoundationLoadPath
#define ___RSCoreFoundationLoadPath RSSTR("~/Library/Frameworks/RSCoreFoundation.framework/Resources/Info.plist")
#endif

typedef RS_ENUM(RSIndex, ___RSCFPVersionIndex)
{
    ___RSCFPVersionSelfBuild = 0,
    ___RSCFPVersionSelf = 1,
    ___RSCFPVersionOSBuild = 2,
    ___RSCFPVersionOSProduct = 3,
    ___RSCFPVersionOSProductUserVisible = 4,
    ___RSCFPVersionMax
};

#ifndef ___RSCFPVersionCount
#define ___RSCFPVersionCount    ___RSCFPVersionMax
#endif

static RSVersion ___RSCFPVersionListCache[___RSCFPVersionCount] = {0};

static BOOL ___RSCFPreferenceGetCFVersion(RSVersion* productVersion, RSVersion* buildVersion)
{
    if (likely(___RSCFPVersionListCache[___RSCFPVersionSelfBuild])) return ___RSCFPVersionListCache[___RSCFPVersionSelfBuild];
    RSDictionaryRef _contents = RSDictionaryCreateWithContentOfPath(RSAllocatorDefault, RSFileManagerStandardizingPath(___RSCoreFoundationLoadPath));
    RSVersion _version1 = 0, _version2 = 0;
    RSStringRef version = RSDictionaryGetValue(_contents, RSSTR("RSProductVersion"));
    if (version)
    {
        _version1 = RSStringFloatValue(version);
        ___RSCFPVersionListCache[___RSCFPVersionSelfBuild] = _version1;
        if (*productVersion) *productVersion = _version1;
    }
    
    version = RSDictionaryGetValue(_contents, RSSTR("RSBundleVersion"));
    if (version)
    {
        _version2 = RSStringFloatValue(version);
        if (*buildVersion) *buildVersion = _version2;
    }
    RSRelease(_contents);
    return (_version1 || _version2) ? YES : NO;
}
#undef ___RSCoreFoundationLoadPath

#ifndef ___RSCoreOperatingSystemVersion
#define ___RSCoreOperatingSystemVersion RSSTR("/System/Library/CoreServices/SystemVersion.plist")
#endif

static RSVersion ___RSCFPVersionCacheGetVersionWithID(___RSCFPVersionIndex idx)
{
    if (idx < 0 || idx >= ___RSCFPVersionCount) return 0;
    if (likely(___RSCFPVersionListCache[idx])) return ___RSCFPVersionListCache[idx];
    RSDictionaryRef _contents = RSDictionaryCreateWithContentOfPath(RSAllocatorDefault, ___RSCoreOperatingSystemVersion);
    RSVersion productVersion = 0, buildVersion = 0;
    if (___RSCFPreferenceGetCFVersion(&productVersion, &buildVersion))
    {
        ___RSCFPVersionListCache[___RSCFPVersionSelfBuild] = buildVersion;
        ___RSCFPVersionListCache[___RSCFPVersionSelf] = productVersion;
    }
    ___RSCFPVersionListCache[___RSCFPVersionOSBuild] = RSStringUnsignedLongValue(RSDictionaryGetValue(_contents, RSSTR("ProductBuildVersion")));
    ___RSCFPVersionListCache[___RSCFPVersionOSProduct] = RSStringUnsignedLongValue(RSDictionaryGetValue(_contents, RSSTR("ProductVersion")));
    ___RSCFPVersionListCache[___RSCFPVersionOSProductUserVisible] = RSStringUnsignedLongValue(RSDictionaryGetValue(_contents, RSSTR("ProductUserVisibleVersion")));
    if (_contents) RSRelease(_contents);
    return ___RSCFPVersionListCache[idx];
}
#undef ___RSCoreOperationSystemVersion
RSExport RSVersion RSGetVersion(int key)
{
    if (key < 0 || key > ___RSCFPVersionCount)
        return 0;
    return ___RSCFPVersionCacheGetVersionWithID(key);
}