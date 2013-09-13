//
//  RSUniChar.h
//  RSCoreFoundation
//
//  Created by RetVal on 7/28/13.
//  Copyright (c) 2013 RetVal. All rights reserved.
//

#if !defined(__RSCOREFOUNDATION_RSUNICHAR__)
#define __RSCOREFOUNDATION_RSUNICHAR__ 1


#include <RSCoreFoundation/RSByteOrder.h>
#include <RSCoreFoundation/RSBase.h>

RS_EXTERN_C_BEGIN

#define RSUniCharBitShiftForByte	(3)
#define RSUniCharBitShiftForMask	(7)

RSInline BOOL RSUniCharIsSurrogateHighCharacter(UniChar character) {
    return ((character >= 0xD800UL) && (character <= 0xDBFFUL) ? YES : NO);
}

RSInline BOOL RSUniCharIsSurrogateLowCharacter(UniChar character) {
    return ((character >= 0xDC00UL) && (character <= 0xDFFFUL) ? YES : NO);
}

RSInline UTF32Char RSUniCharGetLongCharacterForSurrogatePair(UniChar surrogateHigh, UniChar surrogateLow) {
    return (UTF32Char)((surrogateHigh - 0xD800UL) << 10) + (surrogateLow - 0xDC00UL) + 0x0010000UL;
}

// The following values coinside TextEncodingFormat format defines in TextCommon.h
enum {
    RSUniCharUTF16Format = 0,
    RSUniCharUTF8Format = 2,
    RSUniCharUTF32Format = 3
};

RSInline BOOL RSUniCharIsMemberOfBitmap(UTF16Char theChar, const uint8_t *bitmap) {
    return (bitmap && (bitmap[(theChar) >> RSUniCharBitShiftForByte] & (((uint32_t)1) << (theChar & RSUniCharBitShiftForMask))) ? YES : NO);
}

RSInline void RSUniCharAddCharacterToBitmap(UTF16Char theChar, uint8_t *bitmap) {
    bitmap[(theChar) >> RSUniCharBitShiftForByte] |= (((uint32_t)1) << (theChar & RSUniCharBitShiftForMask));
}

RSInline void RSUniCharRemoveCharacterFromBitmap(UTF16Char theChar, uint8_t *bitmap) {
    bitmap[(theChar) >> RSUniCharBitShiftForByte] &= ~(((uint32_t)1) << (theChar & RSUniCharBitShiftForMask));
}

enum {
    RSUniCharControlCharacterSet = 1,
    RSUniCharWhitespaceCharacterSet,
    RSUniCharWhitespaceAndNewlineCharacterSet,
    RSUniCharDecimalDigitCharacterSet,
    RSUniCharLetterCharacterSet,
    RSUniCharLowercaseLetterCharacterSet,
    RSUniCharUppercaseLetterCharacterSet,
    RSUniCharNonBaseCharacterSet,
    RSUniCharCanonicalDecomposableCharacterSet,
    RSUniCharDecomposableCharacterSet = RSUniCharCanonicalDecomposableCharacterSet,
    RSUniCharAlphaNumericCharacterSet,
    RSUniCharPunctuationCharacterSet,
    RSUniCharIllegalCharacterSet,
    RSUniCharTitlecaseLetterCharacterSet,
    RSUniCharSymbolAndOperatorCharacterSet,
    RSUniCharNewlineCharacterSet,
    
    RSUniCharCompatibilityDecomposableCharacterSet = 100, // internal character sets begins here
    RSUniCharHFSPlusDecomposableCharacterSet,
    RSUniCharStrongRightToLeftCharacterSet,
    RSUniCharHasNonSelfLowercaseCharacterSet,
    RSUniCharHasNonSelfUppercaseCharacterSet,
    RSUniCharHasNonSelfTitlecaseCharacterSet,
    RSUniCharHasNonSelfCaseFoldingCharacterSet,
    RSUniCharHasNonSelfMirrorMappingCharacterSet,
    RSUniCharControlAndFormatterCharacterSet,
    RSUniCharCaseIgnorableCharacterSet,
    RSUniCharGraphemeExtendCharacterSet
};

RSExport BOOL RSUniCharIsMemberOf(UTF32Char theChar, uint32_t charset);

// This function returns NULL for RSUniCharControlCharacterSet, RSUniCharWhitespaceCharacterSet, RSUniCharWhitespaceAndNewlineCharacterSet, & RSUniCharIllegalCharacterSet
RSExport const uint8_t *RSUniCharGetBitmapPtrForPlane(uint32_t charset, uint32_t plane);

enum {
    RSUniCharBitmapFilled = (uint8_t)0,
    RSUniCharBitmapEmpty = (uint8_t)0xFF,
    RSUniCharBitmapAll = (uint8_t)1
};

RSExport uint8_t RSUniCharGetBitmapForPlane(uint32_t charset, uint32_t plane, void *bitmap, BOOL isInverted);

RSExport uint32_t RSUniCharGetNumberOfPlanes(uint32_t charset);

enum {
    RSUniCharToLowercase = 0,
    RSUniCharToUppercase,
    RSUniCharToTitlecase,
    RSUniCharCaseFold
};

enum {
    RSUniCharCaseMapFinalSigma = (1UL << 0),
    RSUniCharCaseMapAfter_i = (1UL << 1),
    RSUniCharCaseMapMoreAbove = (1UL << 2),
    RSUniCharCaseMapDutchDigraph = (1UL << 3),
    RSUniCharCaseMapGreekTonos = (1UL << 4)
};

RSExport RSIndex RSUniCharMapCaseTo(UTF32Char theChar, UTF16Char *convertedChar, RSIndex maxLength, uint32_t ctype, uint32_t flags, const uint8_t *langCode);

RSExport uint32_t RSUniCharGetConditionalCaseMappingFlags(UTF32Char theChar, UTF16Char *buffer, RSIndex currentIndex, RSIndex length, uint32_t type, const uint8_t *langCode, uint32_t lastFlags);

enum {
    RSUniCharBiDiPropertyON = 0,
    RSUniCharBiDiPropertyL,
    RSUniCharBiDiPropertyR,
    RSUniCharBiDiPropertyAN,
    RSUniCharBiDiPropertyEN,
    RSUniCharBiDiPropertyAL,
    RSUniCharBiDiPropertyNSM,
    RSUniCharBiDiPropertyCS,
    RSUniCharBiDiPropertyES,
    RSUniCharBiDiPropertyET,
    RSUniCharBiDiPropertyBN,
    RSUniCharBiDiPropertyS,
    RSUniCharBiDiPropertyWS,
    RSUniCharBiDiPropertyB,
    RSUniCharBiDiPropertyRLO,
    RSUniCharBiDiPropertyRLE,
    RSUniCharBiDiPropertyLRO,
    RSUniCharBiDiPropertyLRE,
    RSUniCharBiDiPropertyPDF
};

enum {
    RSUniCharCombiningProperty = 0,
    RSUniCharBidiProperty
};

// The second arg 'bitmap' has to be the pointer to a specific plane
RSInline uint8_t RSUniCharGetBidiPropertyForCharacter(UTF16Char character, const uint8_t *bitmap) {
    if (bitmap) {
        uint8_t value = bitmap[(character >> 8)];
        
        if (value > RSUniCharBiDiPropertyPDF) {
            bitmap = bitmap + 256 + ((value - RSUniCharBiDiPropertyPDF - 1) * 256);
            return bitmap[character % 256];
        } else {
            return value;
        }
    }
    return RSUniCharBiDiPropertyL;
}

RSInline uint8_t RSUniCharGetCombiningPropertyForCharacter(UTF16Char character, const uint8_t *bitmap) {
    if (bitmap) {
        uint8_t value = bitmap[(character >> 8)];
        
        if (value) {
            bitmap = bitmap + 256 + ((value - 1) * 256);
            return bitmap[character % 256];
        }
    }
    return 0;
}

RSExport const void *RSUniCharGetUnicodePropertyDataForPlane(uint32_t propertyType, uint32_t plane);
RSExport uint32_t RSUniCharGetNumberOfPlanesForUnicodePropertyData(uint32_t propertyType);
RSExport uint32_t RSUniCharGetUnicodeProperty(UTF32Char character, uint32_t propertyType);

RSExport BOOL RSUniCharFillDestinationBuffer(const UTF32Char *src, RSIndex srcLength, void **dst, RSIndex dstLength, RSIndex *filledLength, uint32_t dstFormat);

// UTF32 support

RSInline BOOL RSUniCharToUTF32(const UTF16Char *src, RSIndex length, UTF32Char *dst, BOOL allowLossy, BOOL isBigEndien) {
    const UTF16Char *limit = src + length;
    UTF32Char character;
    
    while (src < limit) {
        character = *(src++);
        
        if (RSUniCharIsSurrogateHighCharacter(character)) {
            if ((src < limit) && RSUniCharIsSurrogateLowCharacter(*src)) {
                character = RSUniCharGetLongCharacterForSurrogatePair(character, *(src++));
            } else {
                if (!allowLossy) return NO;
                character = 0xFFFD; // replacement character
            }
        } else if (RSUniCharIsSurrogateLowCharacter(character)) {
            if (!allowLossy) return NO;
            character = 0xFFFD; // replacement character
        }
        
        *(dst++) = (isBigEndien ? RSSwapInt32HostToBig(character) : RSSwapInt32HostToLittle(character));
    }
    
    return YES;
}

RSInline BOOL RSUniCharFromUTF32(const UTF32Char *src, RSIndex length, UTF16Char *dst, BOOL allowLossy, BOOL isBigEndien) {
    const UTF32Char *limit = src + length;
    UTF32Char character;
    
    while (src < limit) {
        character = (isBigEndien ? RSSwapInt32BigToHost(*(src++)) : RSSwapInt32LittleToHost(*(src++)));
        
        if (character < 0xFFFF) { // BMP
            if (allowLossy) {
                if (RSUniCharIsSurrogateHighCharacter(character)) {
                    UTF32Char otherCharacter = 0xFFFD; // replacement character
                    
                    if (src < limit) {
                        otherCharacter = (isBigEndien ? RSSwapInt32BigToHost(*src) : RSSwapInt32LittleToHost(*src));
                        
                        
                        if ((otherCharacter < 0x10000) && RSUniCharIsSurrogateLowCharacter(otherCharacter)) {
                            *(dst++) = character; ++src;
                        } else {
                            otherCharacter = 0xFFFD; // replacement character
                        }
                    }
                    
                    character = otherCharacter;
                } else if (RSUniCharIsSurrogateLowCharacter(character)) {
                    character = 0xFFFD; // replacement character
                }
            } else {
                if (RSUniCharIsSurrogateHighCharacter(character) || RSUniCharIsSurrogateLowCharacter(character)) return NO;
            }
        } else if (character < 0x110000) { // non-BMP
            character -= 0x10000;
            *(dst++) = (UTF16Char)((character >> 10) + 0xD800UL);
            character = (UTF16Char)((character & 0x3FF) + 0xDC00UL);
        } else {
            if (!allowLossy) return NO;
            character = 0xFFFD; // replacement character
        }
        
        *(dst++) = character;
    }
    return YES;
}

RS_EXTERN_C_END

#endif /* ! __COREFOUNDATION_RSUNICHAR__ */


