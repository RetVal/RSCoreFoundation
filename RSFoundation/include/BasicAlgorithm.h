//
//  BasicAlgorithm.h
//  RSCoreFoundation
//
//  Created by closure on 6/1/15.
//  Copyright (c) 2015 RetVal. All rights reserved.
//

#ifndef RSCoreFoundation_BasicAlgorithm_h
#define RSCoreFoundation_BasicAlgorithm_h

namespace RSFoundation {
    namespace Basic {
        template<typename T>
        const T& max(const T& a, const T& b) {
            return (a < b) ? b : a;
        }
        
        template<class T, class Compare>
        const T& max(const T& a, const T& b, Compare comp) {
            return (comp(a, b)) ? b : a;
        }
    }
}

#endif
