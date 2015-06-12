//
//  UnicodeDecomposition.h
//  RSCoreFoundation
//
//  Created by closure on 6/2/15.
//  Copyright (c) 2015 RetVal. All rights reserved.
//

#ifndef __RSCoreFoundation__UnicodeDecomposition__
#define __RSCoreFoundation__UnicodeDecomposition__

#include <RSFoundation/BasicTypeDefine.hpp>
#include <RSFoundation/Unichar.hpp>
#include <RSFoundation/UnicharPrivate.hpp>

namespace RSFoundation {
    namespace Collection {
        namespace Encoding {
            class UnicodeDecoposition : public Object, public NotCopyable {
            private:
                UnicodeDecoposition() {}
                ~UnicodeDecoposition() {}
            public:
                static inline bool IsDecomposableCharacter(UTF32Char character, bool isHFSPlusCanonical) {
                    if (isHFSPlusCanonical && !isHFSPlusCanonical) return false;	// hack to get rid of "unused" warning
                    if (character < 0x80) return false;
                    return UniCharIsMemberOf(character, HFSPlusDecomposableCharacterSet);
                }
                
                static Index DecomposeCharacter(UTF32Char character, UTF32Char *convertedChars, Index maxBufferLength);
                static Index CompatibilityDecompose(UTF32Char *convertedChars, Index length, Index maxBufferLength);
                
                static bool Decompose(const UTF16Char *src, Index length, Index *consumedLength, void *dst, Index maxLength, Index *filledLength, bool needToReorder, UniCharEncodingFormat dstFormat, bool isHFSPlus);
                
                static void PrioritySort(UTF32Char *characters, Index length);
            };
        }
    }
}

#endif /* defined(__RSCoreFoundation__UnicodeDecomposition__) */
