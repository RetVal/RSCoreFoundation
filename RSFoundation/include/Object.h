//
//  RSBasicObject.h
//  RSCoreFoundation
//
//  Created by closure on 5/26/15.
//  Copyright (c) 2015 RetVal. All rights reserved.
//

#ifndef __RSCoreFoundation__RSBasicObject__
#define __RSCoreFoundation__RSBasicObject__

#include <RSFoundation/BasicTypeDefine.h>
#include <RSFoundation/TypeTraits.h>
#include <array>

namespace RSFoundation {
    namespace Basic {
        class NotCopyable {
        private:
            NotCopyable(const NotCopyable &);
            NotCopyable& operator=(const NotCopyable &);
        public:
            NotCopyable();
        };
        
        template<typename T, size_t _Size = 1>
        class Counter {
            static_assert(POD<T>::Result, "T must be POD");
        private:
            T __elems_[_Size > 0 ? _Size : 1];
            
        public:
            Counter() = default;
            
            void Inc(size_t x = 0) { ++__elems_[x]; }
            void Dec(size_t x = 0) { --__elems_[x]; }
            
            const T& Val(size_t x = 0) const { return __elems_[x]; }
            const T& Val(size_t x = 0) { return __elems_[x]; }
        };
        
        class Object {
        public:
            virtual ~Object();
        };
        
        
    }
}

#endif /* defined(__RSCoreFoundation__RSBasicObject__) */
