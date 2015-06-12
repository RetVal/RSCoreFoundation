//
//  UnicodePrecomposition.cpp
//  RSCoreFoundation
//
//  Created by closure on 6/3/15.
//  Copyright (c) 2015 RetVal. All rights reserved.
//

#include <RSFoundation/UnicodePrecomposition.hpp>
#include <RSFoundation/UniChar.hpp>
#include <RSFoundation/Lock.hpp>
#include "Internal.hpp"
#include <RSFoundation/UniCharPrivate.hpp>

namespace RSFoundation {
    namespace Collection {
        namespace Encoding {
            // Canonical Precomposition
            static UTF32Char *__UniCharPrecompSourceTable = nil;
            static uint32_t __UniCharPrecompositionTableLength = 0;
            static uint16_t *__UniCharBMPPrecompDestinationTable = nil;
            static uint32_t *__UniCharNonBMPPrecompDestinationTable = nil;
            
            static const uint8_t *__UniCharNonBaseBitmapForBMP_P = nil; // Adding _P so the symbol name is different from the one in UnicodeDecomposition.c
            static const uint8_t *__UniCharCombiningClassForBMP = nil;
            
            static SpinLock __UniCharPrecompositionTableLock;
            
            static void __UniCharLoadPrecompositionTable(void) {
                __UniCharPrecompositionTableLock.Acquire();
                
                if (nil == __UniCharPrecompSourceTable) {
                    const uint32_t *bytes = (const uint32_t *)UniCharEncodingPrivate::GetMappingData(UnicharMappingType::CanonicalPrecompMapping);
                    uint32_t bmpMappingLength;
                    
                    if (nil == bytes) {
                        __UniCharPrecompositionTableLock.Release();
                        return;
                    }
                    
                    __UniCharPrecompositionTableLength = *(bytes++);
                    bmpMappingLength = *(bytes++);
                    __UniCharPrecompSourceTable = (UTF32Char *)bytes;
                    __UniCharBMPPrecompDestinationTable = (uint16_t *)((intptr_t)bytes + (__UniCharPrecompositionTableLength * sizeof(UTF32Char) * 2));
                    __UniCharNonBMPPrecompDestinationTable = (uint32_t *)(((intptr_t)__UniCharBMPPrecompDestinationTable) + bmpMappingLength);
                    
                    __UniCharNonBaseBitmapForBMP_P = UniCharGetBitmapPtrForPlane(NonBaseCharacterSet, 0);
                    __UniCharCombiningClassForBMP = (const uint8_t *)UniCharGetUnicodePropertyDataForPlane(UniCharCombiningProperty, 0);
                }
                __UniCharPrecompositionTableLock.Release();
            }
            
            // Adding _P so the symbol name is different from the one in UnicodeDecomposition.c
#define __UniCharIsNonBaseCharacter	__UniCharIsNonBaseCharacter_P
            inline bool __UniCharIsNonBaseCharacter(UTF32Char character) {
                return UniCharIsMemberOfBitmap(character, (character < 0x10000 ? __UniCharNonBaseBitmapForBMP_P : UniCharGetBitmapPtrForPlane(NonBaseCharacterSet, ((character >> 16) & 0xFF))));
            }
            
            typedef struct {
                UTF16Char _key;
                UTF16Char _value;
            } __UniCharPrecomposeBMPMappings;
            
            static UTF16Char __UniCharGetMappedBMPValue(const __UniCharPrecomposeBMPMappings *theTable, uint32_t numElem, UTF16Char character) {
                const __UniCharPrecomposeBMPMappings *p, *q, *divider;
                
                if ((character < theTable[0]._key) || (character > theTable[numElem-1]._key)) {
                    return 0;
                }
                p = theTable;
                q = p + (numElem-1);
                while (p <= q) {
                    divider = p + ((q - p) >> 1);	/* divide by 2 */
                    if (character < divider->_key) { q = divider - 1; }
                    else if (character > divider->_key) { p = divider + 1; }
                    else { return divider->_value; }
                }
                return 0;
            }
            
            typedef struct {
                UTF32Char _key;
                uint32_t _value;
            } __UniCharPrecomposeMappings;
            
            static uint32_t __UniCharGetMappedValue_P(const __UniCharPrecomposeMappings *theTable, uint32_t numElem, UTF32Char character) {
                const __UniCharPrecomposeMappings *p, *q, *divider;
                
                if ((character < theTable[0]._key) || (character > theTable[numElem-1]._key)) {
                    return 0;
                }
                p = theTable;
                q = p + (numElem-1);
                while (p <= q) {
                    divider = p + ((q - p) >> 1);	/* divide by 2 */
                    if (character < divider->_key) { q = divider - 1; }
                    else if (character > divider->_key) { p = divider + 1; }
                    else { return divider->_value; }
                }
                return 0;
            }
            
            UTF32Char UniCharPrecompose::PrecomposeCharacter(UTF32Char base, UTF32Char combining)  {
                uint32_t value;
                
                if (nil == __UniCharPrecompSourceTable) __UniCharLoadPrecompositionTable();
                
                if (!(value = __UniCharGetMappedValue_P((const __UniCharPrecomposeMappings *)__UniCharPrecompSourceTable, __UniCharPrecompositionTableLength, combining))) return 0xFFFD;
                
                // We don't have precomposition in non-BMP
                if (value & UniCharNonBmpFlag) {
                    value = __UniCharGetMappedValue_P((const __UniCharPrecomposeMappings *)((uint32_t *)__UniCharNonBMPPrecompDestinationTable + (value & 0xFFFF)), (value >> 16) & 0x7FFF, base);
                } else {
                    value = __UniCharGetMappedBMPValue((const __UniCharPrecomposeBMPMappings *)((uint32_t *)__UniCharBMPPrecompDestinationTable + (value & 0xFFFF)), (value >> 16), base);
                }
                return (value ? value : 0xFFFD);
            }
            
#define HANGUL_SBASE 0xAC00
#define HANGUL_LBASE 0x1100
#define HANGUL_VBASE 0x1161
#define HANGUL_TBASE 0x11A7
#define HANGUL_SCOUNT 11172
#define HANGUL_LCOUNT 19
#define HANGUL_VCOUNT 21
#define HANGUL_TCOUNT 28
#define HANGUL_NCOUNT (HANGUL_VCOUNT * HANGUL_TCOUNT)
            
            inline void __UniCharMoveBufferFromEnd0(UTF16Char *convertedChars, Index length, Index delta) {
                const UTF16Char *limit = convertedChars;
                UTF16Char *dstP;
                
                convertedChars += length;
                dstP = convertedChars + delta;
                
                while (convertedChars > limit) *(--dstP) = *(--convertedChars);
            }
            
            bool UniCharPrecompose::Precompose(const UTF16Char *characters, Index length, Index *consumedLength, UTF16Char *precomposed, Index maxLength, Index *filledLength){
                UTF32Char currentChar = 0, lastChar = 0, precomposedChar = 0xFFFD;
                Index originalLength = length, usedLength = 0;
                UTF16Char *currentBase = precomposed;
                uint8_t currentClass, lastClass = 0;
                bool currentBaseIsBMP = true;
                bool isPrecomposed;
                
                if (nil == __UniCharPrecompSourceTable) __UniCharLoadPrecompositionTable();
                
                while (length > 0) {
                    currentChar = *(characters++);
                    --length;
                    
                    if (UniCharIsSurrogateHighCharacter(currentChar) && (length > 0) && UniCharIsSurrogateLowCharacter(*characters)) {
                        currentChar = UniCharGetLongCharacterForSurrogatePair(currentChar, *(characters++));
                        --length;
                    }
                    
                    if (lastChar && __UniCharIsNonBaseCharacter(currentChar)) {
                        isPrecomposed = (precomposedChar == 0xFFFD ? false : true);
                        if (isPrecomposed) lastChar = precomposedChar;
                        
                        currentClass = (currentChar > 0xFFFF ? UniCharGetUnicodeProperty(currentChar, UniCharCombiningProperty) : UniCharGetCombiningPropertyForCharacter(currentChar, __UniCharCombiningClassForBMP));
                        
                        if ((lastClass == 0) || (currentClass > lastClass)) {
                            if ((precomposedChar = UniCharPrecompose::PrecomposeCharacter(lastChar, currentChar)) == 0xFFFD) {
                                if (isPrecomposed) precomposedChar = lastChar;
                                lastClass = currentClass;
                            } else {
                                continue;
                            }
                        }
                        if (currentChar > 0xFFFF) { // Non-BMP
                            usedLength += 2;
                            if (usedLength > maxLength) break;
                            currentChar -= 0x10000;
                            *(precomposed++) = (UTF16Char)((currentChar >> 10) + 0xD800UL);
                            *(precomposed++) = (UTF16Char)((currentChar & 0x3FF) + 0xDC00UL);
                        } else {
                            ++usedLength;
                            if (usedLength > maxLength) break;
                            *(precomposed++) = (UTF16Char)currentChar;
                        }
                    } else {
                        if ((currentChar >= HANGUL_LBASE) && (currentChar < (HANGUL_LBASE + 0xFF))) { // Hangul Jamo
                            int8_t lIndex = currentChar - HANGUL_LBASE;
                            
                            if ((length > 0) && (0 <= lIndex) && (lIndex <= HANGUL_LCOUNT)) {
                                int16_t vIndex = *characters - HANGUL_VBASE;
                                
                                if ((vIndex >= 0) && (vIndex <= HANGUL_VCOUNT)) {
                                    int16_t tIndex = 0;
                                    
                                    ++characters; --length;
                                    
                                    if (length > 0) {
                                        tIndex = *characters - HANGUL_TBASE;
                                        if ((tIndex < 0) || (tIndex > HANGUL_TCOUNT)) {
                                            tIndex = 0;
                                        } else {
                                            ++characters; --length;
                                        }
                                    }
                                    currentChar = (lIndex * HANGUL_VCOUNT + vIndex) * HANGUL_TCOUNT + tIndex + HANGUL_SBASE;
                                }
                            }
                        }
                        
                        if (precomposedChar != 0xFFFD) {
                            if (currentBaseIsBMP) { // Non-BMP
                                if (lastChar > 0xFFFF) { // Last char was Non-BMP
                                    --usedLength;
                                    memmove(currentBase + 1, currentBase + 2, (precomposed - (currentBase + 2)) * sizeof(UTF16Char));
                                }
                                *(currentBase) = (UTF16Char)precomposedChar;
                            } else {
                                if (lastChar < 0x10000) { // Last char was BMP
                                    ++usedLength;
                                    if (usedLength > maxLength) break;
                                    __UniCharMoveBufferFromEnd0(currentBase + 1, precomposed - (currentBase + 1), 1);
                                }
                                precomposedChar -= 0x10000;
                                *currentBase = (UTF16Char)((precomposedChar >> 10) + 0xD800UL);
                                *(currentBase + 1) = (UTF16Char)((precomposedChar & 0x3FF) + 0xDC00UL);
                            }
                            precomposedChar = 0xFFFD;
                        }
                        currentBase = precomposed;
                        
                        lastChar = currentChar;
                        lastClass = 0;
                        
                        if (currentChar > 0xFFFF) { // Non-BMP
                            usedLength += 2;
                            if (usedLength > maxLength) break;
                            currentChar -= 0x10000;
                            *(precomposed++) = (UTF16Char)((currentChar >> 10) + 0xD800UL);
                            *(precomposed++) = (UTF16Char)((currentChar & 0x3FF) + 0xDC00UL);
                            currentBaseIsBMP = false;
                        } else {
                            ++usedLength;
                            if (usedLength > maxLength) break;
                            *(precomposed++) = (UTF16Char)currentChar;
                            currentBaseIsBMP = true;
                        }
                    }
                }
                
                if (precomposedChar != 0xFFFD) {
                    if (currentChar > 0xFFFF) { // Non-BMP
                        if (lastChar < 0x10000) { // Last char was BMP
                            ++usedLength;
                            if (usedLength > maxLength) {
                                if (consumedLength) *consumedLength = originalLength - length;
                                if (filledLength) *filledLength = usedLength;
                                return false;
                            }
                            __UniCharMoveBufferFromEnd0(currentBase + 1, precomposed - (currentBase + 1), 1);
                        }
                        precomposedChar -= 0x10000;
                        *currentBase = (UTF16Char)((precomposedChar >> 10) + 0xD800UL);
                        *(currentBase + 1) = (UTF16Char)((precomposedChar & 0x3FF) + 0xDC00UL);
                    } else {
                        if (lastChar > 0xFFFF) { // Last char was Non-BMP
                            --usedLength;
                            memmove(currentBase + 1, currentBase + 2, (precomposed - (currentBase + 2)) * sizeof(UTF16Char));
                        }
                        *(currentBase) = (UTF16Char)precomposedChar;
                    }
                }
                
                if (consumedLength) *consumedLength = originalLength - length;
                if (filledLength) *filledLength = usedLength;
                
                return true;
            }
            
#undef __UniCharIsNonBaseCharacter
#undef HANGUL_SBASE
#undef HANGUL_LBASE
#undef HANGUL_VBASE
#undef HANGUL_TBASE
#undef HANGUL_SCOUNT
#undef HANGUL_LCOUNT
#undef HANGUL_VCOUNT
#undef HANGUL_TCOUNT
#undef HANGUL_NCOUNT
        }
    }
}
