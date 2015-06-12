//
//  Dictionary.hpp
//  RSCoreFoundation
//
//  Created by closure on 6/12/15.
//  Copyright © 2015 RetVal. All rights reserved.
//

#ifndef Dictionary_cpp
#define Dictionary_cpp

#include <RSFoundation/Object.hpp>
#include <RSFoundation/Allocator.hpp>

namespace RSFoundation {
    using namespace Basic;
    namespace Collection {
        template <typename KT, typename VT>
        class Dictionary : public Object {
        public:
            typedef KT key_type;
            typedef VT value_type;
            
        private:
            Allocator<KT> key_allocator = Allocator<KT>::SystemDefault;
            Allocator<VT> value_allocator =  Allocator<VT>::SystemDefault;
            
        private:
            void _spec() __unused {
                Object *k1 __unused = key_allocator.Allocate();
                Object *v1 __unused = value_allocator.Allocate();
            }
        };
    }
}

#endif /* Dictionary_cpp */
