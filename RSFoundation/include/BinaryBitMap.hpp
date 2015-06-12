//
//  BinaryBitMap.h
//  RSCoreFoundation
//
//  Created by closure on 5/27/15.
//  Copyright (c) 2015 RetVal. All rights reserved.
//

#ifndef __RSCoreFoundation__BinaryBitMap__
#define __RSCoreFoundation__BinaryBitMap__

#include <RSFoundation/BasicTypeDefine.h>

namespace RSFoundation {
    namespace Basic {
        template<typename T, UInt64 minSize>
        union BinaryBitMap {
            T t;
            char bin[sizeof(T) > minSize ? sizeof(T) : minSize];
        };
    }
}

#endif /* defined(__RSCoreFoundation__BinaryBitMap__) */
