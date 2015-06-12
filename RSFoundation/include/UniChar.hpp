//
//  Unichar.h
//  RSCoreFoundation
//
//  Created by closure on 6/2/15.
//  Copyright (c) 2015 RetVal. All rights reserved.
//

#ifndef RSCoreFoundation_Unichar_h
#define RSCoreFoundation_Unichar_h

#include <RSFoundation/BasicTypeDefine.hpp>
#include <RSFoundation/Order.hpp>

namespace RSFoundation {
    namespace Collection {
        using namespace Basic;
        namespace Encoding {
#define UniCharBitShiftForByte	(3)
#define UniCharBitShiftForMask	(7)
            inline bool UniCharIsSurrogateHighCharacter(UniChar character) {
                return ((character >= 0xD800UL) && (character <= 0xDBFFUL) ? YES : NO);
            }
            
            inline bool UniCharIsSurrogateLowCharacter(UniChar character) {
                return ((character >= 0xDC00UL) && (character <= 0xDFFFUL) ? YES : NO);
            }
            
            inline UTF32Char UniCharGetLongCharacterForSurrogatePair(UniChar surrogateHigh, UniChar surrogateLow) {
                return (UTF32Char)((surrogateHigh - 0xD800UL) << 10) + (surrogateLow - 0xDC00UL) + 0x0010000UL;
            }
            
            // The following values coinside TextEncodingFormat format defines in TextCommon.h
            enum  UniCharEncodingFormat : Index {
                UTF16Format = 0,
                UTF8Format = 2,
                UTF32Format = 3
            };
            
            inline bool UniCharIsMemberOfBitmap(UTF16Char theChar, const uint8_t *bitmap) {
                return (bitmap && (bitmap[(theChar) >> UniCharBitShiftForByte] & (((uint32_t)1) << (theChar & UniCharBitShiftForMask))) ? YES : NO);
            }
            
            inline void UniCharAddCharacterToBitmap(UTF16Char theChar, uint8_t *bitmap) {
                bitmap[(theChar) >> UniCharBitShiftForByte] |= (((uint32_t)1) << (theChar & UniCharBitShiftForMask));
            }
            
            inline void UniCharRemoveCharacterFromBitmap(UTF16Char theChar, uint8_t *bitmap) {
                bitmap[(theChar) >> UniCharBitShiftForByte] &= ~(((uint32_t)1) << (theChar & UniCharBitShiftForMask));
            }
            
            enum CharacterSet : Index {
                ControlCharacterSet = 1,
                WhitespaceCharacterSet,
                WhitespaceAndNewlineCharacterSet,
                DecimalDigitCharacterSet,
                LetterCharacterSet,
                LowercaseLetterCharacterSet,
                UppercaseLetterCharacterSet,
                NonBaseCharacterSet,
                CanonicalDecomposableCharacterSet,
                DecomposableCharacterSet = CanonicalDecomposableCharacterSet,
                AlphaNumericCharacterSet,
                PunctuationCharacterSet,
                IllegalCharacterSet,
                TitlecaseLetterCharacterSet,
                SymbolAndOperatorCharacterSet,
                NewlineCharacterSet,
                
                CompatibilityDecomposableCharacterSet = 100, // internal character sets begins here
                HFSPlusDecomposableCharacterSet,
                StrongRightToLeftCharacterSet,
                HasNonSelfLowercaseCharacterSet,
                HasNonSelfUppercaseCharacterSet,
                HasNonSelfTitlecaseCharacterSet,
                HasNonSelfCaseFoldingCharacterSet,
                HasNonSelfMirrorMappingCharacterSet,
                ControlAndFormatterCharacterSet,
                CaseIgnorableCharacterSet,
                GraphemeExtendCharacterSet
            };
            
            bool UniCharIsMemberOf(UTF32Char theChar, CharacterSet charset);
            
            // This function returns nil for UniCharControlCharacterSet, UniCharWhitespaceCharacterSet, UniCharWhitespaceAndNewlineCharacterSet, & UniCharIllegalCharacterSet
            const uint8_t *UniCharGetBitmapPtrForPlane(CharacterSet charset, uint32_t plane);
            
            enum {
                UniCharBitmapFilled = (uint8_t)0,
                UniCharBitmapEmpty = (uint8_t)0xFF,
                UniCharBitmapAll = (uint8_t)1
            };
            
            uint8_t UniCharGetBitmapForPlane(CharacterSet charset, uint32_t plane, void *bitmap, bool isInverted);
            
            uint32_t UniCharGetNumberOfPlanes(CharacterSet charset);
            
            enum {
                UniCharToLowercase = 0,
                UniCharToUppercase,
                UniCharToTitlecase,
                UniCharCaseFold
            };
            
            enum {
                UniCharCaseMapFinalSigma = (1UL << 0),
                UniCharCaseMapAfter_i = (1UL << 1),
                UniCharCaseMapMoreAbove = (1UL << 2),
                UniCharCaseMapDutchDigraph = (1UL << 3),
                UniCharCaseMapGreekTonos = (1UL << 4)
            };
            
            Index UniCharMapCaseTo(UTF32Char theChar, UTF16Char *convertedChar, Index maxLength, uint32_t ctype, uint32_t flags, const uint8_t *langCode);
            
            uint32_t UniCharGetConditionalCaseMappingFlags(UTF32Char theChar, UTF16Char *buffer, Index currentIndex, Index length, uint32_t type, const uint8_t *langCode, uint32_t lastFlags);
            
            enum {
                UniCharBiDiPropertyON = 0,
                UniCharBiDiPropertyL,
                UniCharBiDiPropertyR,
                UniCharBiDiPropertyAN,
                UniCharBiDiPropertyEN,
                UniCharBiDiPropertyAL,
                UniCharBiDiPropertyNSM,
                UniCharBiDiPropertyCS,
                UniCharBiDiPropertyES,
                UniCharBiDiPropertyET,
                UniCharBiDiPropertyBN,
                UniCharBiDiPropertyS,
                UniCharBiDiPropertyWS,
                UniCharBiDiPropertyB,
                UniCharBiDiPropertyRLO,
                UniCharBiDiPropertyRLE,
                UniCharBiDiPropertyLRO,
                UniCharBiDiPropertyLRE,
                UniCharBiDiPropertyPDF
            };
            
            enum {
                UniCharCombiningProperty = 0,
                UniCharBidiProperty
            };
            
            // The second arg 'bitmap' has to be the pointer to a specific plane
            inline uint8_t UniCharGetBidiPropertyForCharacter(UTF16Char character, const uint8_t *bitmap) {
                if (bitmap) {
                    uint8_t value = bitmap[(character >> 8)];
                    
                    if (value > UniCharBiDiPropertyPDF) {
                        bitmap = bitmap + 256 + ((value - UniCharBiDiPropertyPDF - 1) * 256);
                        return bitmap[character % 256];
                    } else {
                        return value;
                    }
                }
                return UniCharBiDiPropertyL;
            }
            
            inline uint8_t UniCharGetCombiningPropertyForCharacter(UTF16Char character, const uint8_t *bitmap) {
                if (bitmap) {
                    uint8_t value = bitmap[(character >> 8)];
                    
                    if (value) {
                        bitmap = bitmap + 256 + ((value - 1) * 256);
                        return bitmap[character % 256];
                    }
                }
                return 0;
            }
            
            const void *UniCharGetUnicodePropertyDataForPlane(uint32_t propertyType, uint32_t plane);
            uint32_t UniCharGetNumberOfPlanesForUnicodePropertyData(uint32_t propertyType);
            uint32_t UniCharGetUnicodeProperty(UTF32Char character, uint32_t propertyType);
            
            bool UniCharFillDestinationBuffer(const UTF32Char *src, Index srcLength, void **dst, Index dstLength, Index *filledLength, UniCharEncodingFormat dstFormat);
            
            // UTF32 support
            
            inline bool UniCharToUTF32(const UTF16Char *src, Index length, UTF32Char *dst, bool allowLossy, bool isBigEndien) {
                const UTF16Char *limit = src + length;
                UTF32Char character;
                
                while (src < limit) {
                    character = *(src++);
                    
                    if (UniCharIsSurrogateHighCharacter(character)) {
                        if ((src < limit) && UniCharIsSurrogateLowCharacter(*src)) {
                            character = UniCharGetLongCharacterForSurrogatePair(character, *(src++));
                        } else {
                            if (!allowLossy) return NO;
                            character = 0xFFFD; // replacement character
                        }
                    } else if (UniCharIsSurrogateLowCharacter(character)) {
                        if (!allowLossy) return NO;
                        character = 0xFFFD; // replacement character
                    }
                    
                    *(dst++) = (isBigEndien ? SwapInt32HostToBig(character) : SwapInt32HostToLittle(character));
                }
                
                return YES;
            }
            
            inline bool UniCharFromUTF32(const UTF32Char *src, Index length, UTF16Char *dst, bool allowLossy, bool isBigEndien) {
                const UTF32Char *limit = src + length;
                UTF32Char character;
                
                while (src < limit) {
                    character = (isBigEndien ? SwapInt32BigToHost(*(src++)) : SwapInt32LittleToHost(*(src++)));
                    
                    if (character < 0xFFFF) { // BMP
                        if (allowLossy) {
                            if (UniCharIsSurrogateHighCharacter(character)) {
                                UTF32Char otherCharacter = 0xFFFD; // replacement character
                                
                                if (src < limit) {
                                    otherCharacter = (isBigEndien ? SwapInt32BigToHost(*src) : SwapInt32LittleToHost(*src));
                                    
                                    
                                    if ((otherCharacter < 0x10000) && UniCharIsSurrogateLowCharacter(otherCharacter)) {
                                        *(dst++) = character; ++src;
                                    } else {
                                        otherCharacter = 0xFFFD; // replacement character
                                    }
                                }
                                
                                character = otherCharacter;
                            } else if (UniCharIsSurrogateLowCharacter(character)) {
                                character = 0xFFFD; // replacement character
                            }
                        } else {
                            if (UniCharIsSurrogateHighCharacter(character) || UniCharIsSurrogateLowCharacter(character)) return NO;
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
        }
    }
}

#endif
