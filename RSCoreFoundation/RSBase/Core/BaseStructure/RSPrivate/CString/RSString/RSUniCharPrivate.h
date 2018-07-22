//
//  RSUniCharPrivate.h
//  RSCoreFoundation
//
//  Created by RetVal on 7/28/13.
//  Copyright (c) 2013 RetVal. All rights reserved.
//

#ifndef RSCoreFoundation_RSUniCharPrivate_h
#define RSCoreFoundation_RSUniCharPrivate_h

#if !defined(__RSCOREFOUNDATION_RSUNICHARPRIV__)
#define __RSCOREFOUNDATION_RSUNICHARPRIV__ 1

#include <RSCoreFoundation/RSBase.h>
#include <RSCoreFoundation/RSUniChar.h>

#define RSUniCharRecursiveDecompositionFlag	(1UL << 30)
#define RSUniCharNonBmpFlag			(1UL << 31)
#define RSUniCharConvertCountToFlag(count)	((count & 0x1F) << 24)
#define RSUniCharConvertFlagToCount(flag)	((flag >> 24) & 0x1F)

enum {
    RSUniCharCanonicalDecompMapping = (RSUniCharCaseFold + 1),
    RSUniCharCanonicalPrecompMapping,
    RSUniCharCompatibilityDecompMapping
};

RSExport const void *RSUniCharGetMappingData(uint32_t type);

#endif /* ! __RSCOREFOUNDATION_RSUNICHARPRIV__ */

#endif
