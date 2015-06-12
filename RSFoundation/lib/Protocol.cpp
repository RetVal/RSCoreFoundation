//
//  Protocol.cpp
//  RSCoreFoundation
//
//  Created by closure on 5/27/15.
//  Copyright (c) 2015 RetVal. All rights reserved.
//

#include <RSFoundation/Protocol.hpp>

namespace RSFoundation {
    namespace Basic {
        Protocol::~Protocol() {}
        
        Index Hashable::Hash() const { return (uintptr_t)this; }
        
        Object *Copying::Copy() const { return nullptr; }
    }
}
