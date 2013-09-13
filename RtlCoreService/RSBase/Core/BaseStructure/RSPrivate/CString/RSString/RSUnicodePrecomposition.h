//
//  RSUnicodePrecomposition.h
//  RSCoreFoundation
//
//  Created by RetVal on 7/29/13.
//  Copyright (c) 2013 RetVal. All rights reserved.
//

#ifndef RSCoreFoundation_RSUnicode_Precomposition_h
#define RSCoreFoundation_RSUnicode_Precomposition_h

#if !defined(__RSCOREFOUNDATION_RSUNICODEPRECOMPOSITION__)
#define __RSCOREFOUNDATION_RSUNICODEPRECOMPOSITION__ 1

#include "RSUniChar.h"

RS_EXTERN_C_BEGIN

// As you can see, this function cannot precompose Hangul Jamo
RSExport UTF32Char RSUniCharPrecomposeCharacter(UTF32Char base, UTF32Char combining);

RSExport BOOL RSUniCharPrecompose(const UTF16Char *characters, RSIndex length, RSIndex *consumedLength, UTF16Char *precomposed, RSIndex maxLength, RSIndex *filledLength);

RS_EXTERN_C_END

#endif /* ! __COREFOUNDATION_RSUNICODEPRECOMPOSITION__ */



#endif
