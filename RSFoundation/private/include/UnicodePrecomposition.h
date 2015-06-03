//
//  UnicodePrecomposition.h
//  RSCoreFoundation
//
//  Created by closure on 6/3/15.
//  Copyright (c) 2015 RetVal. All rights reserved.
//

#ifndef __RSCoreFoundation__UnicodePrecomposition__
#define __RSCoreFoundation__UnicodePrecomposition__

#include <RSFoundation/UniChar.h>

namespace RSFoundation {
    namespace Collection {
        namespace Encoding {
            // As you can see, this function cannot precompose Hangul Jamo
            UTF32Char UniCharPrecomposeCharacter(UTF32Char base, UTF32Char combining);
            
            bool UniCharPrecompose(const UTF16Char *characters, RSIndex length, RSIndex *consumedLength, UTF16Char *precomposed, RSIndex maxLength, RSIndex *filledLength);
        }
    }
}

#endif /* defined(__RSCoreFoundation__UnicodePrecomposition__) */
