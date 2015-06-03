//
//  UnicodePrecomposition.h
//  RSCoreFoundation
//
//  Created by closure on 6/3/15.
//  Copyright (c) 2015 RetVal. All rights reserved.
//

#ifndef __RSCoreFoundation__UnicodePrecomposition__
#define __RSCoreFoundation__UnicodePrecomposition__

#include <RSFoundation/Object.h>
#include <RSFoundation/UniChar.h>

namespace RSFoundation {
    namespace Collection {
        namespace Encoding {
            
            class UniCharPrecompose : public Object, public NotCopyable {
            public:
                static UTF32Char PrecomposeCharacter(UTF32Char base, UTF32Char combining);
                static bool Precompose(const UTF16Char *characters, Index length, Index *consumedLength, UTF16Char *precomposed, Index maxLength, Index *filledLength);
            };
        }
    }
}

#endif /* defined(__RSCoreFoundation__UnicodePrecomposition__) */
