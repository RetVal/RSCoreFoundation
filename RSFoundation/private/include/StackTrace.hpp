//
//  StackTrace.h
//  RSCoreFoundation
//
//  Created by closure on 6/8/15.
//  Copyright (c) 2015 RetVal. All rights reserved.
//

#ifndef __RSCoreFoundation__StackTrace__
#define __RSCoreFoundation__StackTrace__

#include <RSFoundation/BasicTypeDefine.hpp>

namespace RSFoundation {
    namespace Basic {
        Private void print_stacktrace(FILE *out = stderr, unsigned int max_frames = 63);
    }
}

#endif /* defined(__RSCoreFoundation__StackTrace__) */
