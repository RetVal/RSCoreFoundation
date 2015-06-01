//
//  RSBasicObject.cpp
//  RSCoreFoundation
//
//  Created by closure on 5/26/15.
//  Copyright (c) 2015 RetVal. All rights reserved.
//

#include "Object.h"

namespace RSFoundation {
    namespace Basic {
        NotCopyable::NotCopyable() {
        }
        
        NotCopyable::NotCopyable(const NotCopyable &) {
        }
        
        NotCopyable& NotCopyable::operator=(const NotCopyable &) {
            return *this;
        }
        
        Object::~Object() {
        }
    }
}
