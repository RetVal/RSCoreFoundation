//
//  UnicharPrivate.h
//  RSCoreFoundation
//
//  Created by closure on 6/2/15.
//  Copyright (c) 2015 RetVal. All rights reserved.
//

#ifndef RSCoreFoundation_UnicharPrivate_h
#define RSCoreFoundation_UnicharPrivate_h

namespace RSFoundation {
    namespace Collection {
#define UniCharRecursiveDecompositionFlag	(1UL << 30)
#define UniCharNonBmpFlag			(1UL << 31)
#define UniCharConvertCountToFlag(count)	((count & 0x1F) << 24)
#define UniCharConvertFlagToCount(flag)	((flag >> 24) & 0x1F)
        
        enum class UnicharMappingType : RSIndex {
            CanonicalDecompMapping = (UniCharCaseFold + 1),
            CanonicalPrecompMapping,
            CompatibilityDecompMapping
        };
        
        const void *UniCharGetMappingData(UnicharMappingType type);
    }
}

#endif
