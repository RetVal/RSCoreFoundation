//
//  RSUnicodeDecomposition.h
//  RSCoreFoundation
//
//  Created by RetVal on 7/28/13.
//  Copyright (c) 2013 RetVal. All rights reserved.
//

#ifndef RSCoreFoundation_RSUnicodeDecomposition_h
#define RSCoreFoundation_RSUnicodeDecomposition_h

#if !defined(__COREFOUNDATION_RSUNICODEDECOMPOSITION__)
#define __COREFOUNDATION_RSUNICODEDECOMPOSITION__ 1

#include "RSUniChar.h"

RS_EXTERN_C_BEGIN

RSInline BOOL RSUniCharIsDecomposableCharacter(UTF32Char character, BOOL isHFSPlusCanonical) {
    if (isHFSPlusCanonical && !isHFSPlusCanonical) return NO;	// hack to get rid of "unused" warning
    if (character < 0x80) return NO;
    return RSUniCharIsMemberOf(character, RSUniCharHFSPlusDecomposableCharacterSet);
}

RSExport RSIndex RSUniCharDecomposeCharacter(UTF32Char character, UTF32Char *convertedChars, RSIndex maxBufferLength);
RSExport RSIndex RSUniCharCompatibilityDecompose(UTF32Char *convertedChars, RSIndex length, RSIndex maxBufferLength);

RSExport BOOL RSUniCharDecompose(const UTF16Char *src, RSIndex length, RSIndex *consumedLength, void *dst, RSIndex maxLength, RSIndex *filledLength, BOOL needToReorder, uint32_t dstFormat, BOOL isHFSPlus);

RSExport void RSUniCharPrioritySort(UTF32Char *characters, RSIndex length);

RS_EXTERN_C_END

#endif /* ! __COREFOUNDATION_RSUNICODEDECOMPOSITION__ */



#endif
