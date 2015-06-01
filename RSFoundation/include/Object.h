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

namespace RSFoundation {
    namespace Basic {
        class NotCopyable {
        private:
            NotCopyable(const NotCopyable &);
            NotCopyable& operator=(const NotCopyable &);
        public:
            NotCopyable();
        };
        
        class Object {
        public:
            virtual ~Object();
        };
        
        
    }
}

#endif /* defined(__RSCoreFoundation__RSBasicObject__) */
