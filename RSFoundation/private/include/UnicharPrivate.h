//
//  UnicharPrivate.h
//  RSCoreFoundation
//
//  Created by closure on 6/2/15.
//  Copyright (c) 2015 RetVal. All rights reserved.
//

#ifndef RSCoreFoundation_UnicharPrivate_h
#define RSCoreFoundation_UnicharPrivate_h

#include <RSFoundation/Object.h>

namespace RSFoundation {
    namespace Collection {
#define UniCharRecursiveDecompositionFlag	(1UL << 30)
#define UniCharNonBmpFlag			(1UL << 31)
#define UniCharConvertCountToFlag(count)	((count & 0x1F) << 24)
#define UniCharConvertFlagToCount(flag)	((flag >> 24) & 0x1F)
        
        namespace Encoding {
            enum class UnicharMappingType : Index {
                CanonicalDecompMapping = (UniCharCaseFold + 1),
                CanonicalPrecompMapping,
                CompatibilityDecompMapping
            };
            
            class UniCharEncodingPrivate : public Object, public NotCopyable {
            private:
                UniCharEncodingPrivate() {}
                ~UniCharEncodingPrivate() {}
            public:
                static const void *GetMappingData(UnicharMappingType type);
            };
        }
    }
}

#endif
