//
//  UnicodeDecomposition.h
//  RSCoreFoundation
//
//  Created by closure on 6/2/15.
//  Copyright (c) 2015 RetVal. All rights reserved.
//

#ifndef __RSCoreFoundation__UnicodeDecomposition__
#define __RSCoreFoundation__UnicodeDecomposition__

#include <RSFoundation/BasicTypeDefine.h>
#include <RSFoundation/Unichar.h>
#include <RSFoundation/UnicharPrivate.h>

namespace RSFoundation {
    namespace Collection {
        class UnicodeDecoposition {
        public:
            inline BOOL IsDecomposableCharacter(UTF32Char character, BOOL isHFSPlusCanonical) {
                if (isHFSPlusCanonical && !isHFSPlusCanonical) return NO;	// hack to get rid of "unused" warning
                if (character < 0x80) return NO;
                return UniCharIsMemberOf(character, HFSPlusDecomposableCharacterSet);
            }
            
            static RSIndex DecomposeCharacter(UTF32Char character, UTF32Char *convertedChars, RSIndex maxBufferLength);
            static RSIndex CompatibilityDecompose(UTF32Char *convertedChars, RSIndex length, RSIndex maxBufferLength);
            
            static BOOL Decompose(const UTF16Char *src, RSIndex length, RSIndex *consumedLength, void *dst, RSIndex maxLength, RSIndex *filledLength, BOOL needToReorder, uint32_t dstFormat, BOOL isHFSPlus);
            
            static void PrioritySort(UTF32Char *characters, RSIndex length);
        };
    }
}

#endif /* defined(__RSCoreFoundation__UnicodeDecomposition__) */
