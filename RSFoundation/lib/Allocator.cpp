//
//  Allocator.cpp
//  RSCoreFoundation
//
//  Created by closure on 5/27/15.
//  Copyright (c) 2015 RetVal. All rights reserved.
//

#include "Allocator.h"
//#include "debug_new.h"  
#include "gc_cpp.h"

namespace RSFoundation {
    namespace Basic {
        class Test : public gc {
        public:
            static Test *ptr;
        };
        
        Test *Test::ptr = new Test;
    }
}
