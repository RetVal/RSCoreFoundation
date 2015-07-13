//
//  Protocol.h
//  RSCoreFoundation
//
//  Created by closure on 5/27/15.
//  Copyright (c) 2015 RetVal. All rights reserved.
//

#ifndef __RSCoreFoundation__Protocol__
#define __RSCoreFoundation__Protocol__

#include <RSFoundation/BasicTypeDefine.hpp>
#include <RSFoundation/TypeTraits.hpp>

namespace RSFoundation {
    namespace Basic {
        class Object;
        
        class NotCopyable {
        private:
            NotCopyable(const NotCopyable &) {}
            NotCopyable& operator=(const NotCopyable &) { return *this; };
        public:
            NotCopyable() {};
            virtual ~NotCopyable() {};
        };
        
        class Protocol : private NotCopyable {
        public:
            virtual ~Protocol() {};
        };
        
        class Hashable : public virtual Protocol {
        public:
            virtual HashCode Hash() const { return (uintptr_t)this; };
        };
        
        class Copying : public virtual Protocol {
        public:
            virtual const Object *Copy() const { return nullptr; }
        };
    }
}

#endif /* defined(__RSCoreFoundation__Protocol__) */
