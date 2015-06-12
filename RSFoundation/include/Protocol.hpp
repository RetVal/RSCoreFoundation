//
//  Protocol.h
//  RSCoreFoundation
//
//  Created by closure on 5/27/15.
//  Copyright (c) 2015 RetVal. All rights reserved.
//

#ifndef __RSCoreFoundation__Protocol__
#define __RSCoreFoundation__Protocol__

#include <RSFoundation/Object.hpp>

namespace RSFoundation {
    namespace Basic {
        class Protocol : public NotCopyable {
        public:
            virtual ~Protocol();
        };
    }
}

#endif /* defined(__RSCoreFoundation__Protocol__) */
