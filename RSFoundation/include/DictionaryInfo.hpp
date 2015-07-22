//
//  DictionaryInfo.hpp
//  RSCoreFoundation
//
//  Created by closure on 7/22/15.
//  Copyright (c) 2015 RetVal. All rights reserved.
//

#ifndef RSCoreFoundation_DictionaryInfo_hpp
#define RSCoreFoundation_DictionaryInfo_hpp

#include <RSFoundation/Object.hpp>

namespace RSCF {
    template<typename T>
    struct DictionaryInfo {
        static inline T getEmptyKey();
        static inline T getTombstoneKey();
        static RSHashCode getHashValue(const T &Val);
        static bool isEqual(const T &LHS, const T &RHS);
    };
}
#endif
