//
//  RSUnicodePrecomposition.c
//  RSCoreFoundation
//
//  Created by RetVal on 7/29/13.
//  Copyright (c) 2013 RetVal. All rights reserved.
//

#include <string.h>
#include <RSCoreFoundation/RSBase.h>
//#include <RSCoreFoundation/RSCharacterSet.h>
#include "RSUniChar.h"
#include "RSUnicodePrecomposition.h"
#include "RSInternal.h"
#include "RSUniCharPrivate.h"

// Canonical Precomposition
static UTF32Char *__RSUniCharPrecompSourceTable = NULL;
static uint32_t __RSUniCharPrecompositionTableLength = 0;
static uint16_t *__RSUniCharBMPPrecompDestinationTable = NULL;
static uint32_t *__RSUniCharNonBMPPrecompDestinationTable = NULL;

static const uint8_t *__RSUniCharNonBaseBitmapForBMP_P = NULL; // Adding _P so the symbol name is different from the one in RSUnicodeDecomposition.c
static const uint8_t *__RSUniCharCombiningClassForBMP = NULL;

static RSSpinLock __RSUniCharPrecompositionTableLock = RSSpinLockInit;

static void __RSUniCharLoadPrecompositionTable(void) {
    
    RSSpinLockLock(&__RSUniCharPrecompositionTableLock);
    
    if (NULL == __RSUniCharPrecompSourceTable) {
        const uint32_t *bytes = (const uint32_t *)RSUniCharGetMappingData(RSUniCharCanonicalPrecompMapping);
        uint32_t bmpMappingLength;
        
        if (NULL == bytes) {
            RSSpinLockUnlock(&__RSUniCharPrecompositionTableLock);
            return;
        }
        
        __RSUniCharPrecompositionTableLength = *(bytes++);
        bmpMappingLength = *(bytes++);
        __RSUniCharPrecompSourceTable = (UTF32Char *)bytes;
        __RSUniCharBMPPrecompDestinationTable = (uint16_t *)((intptr_t)bytes + (__RSUniCharPrecompositionTableLength * sizeof(UTF32Char) * 2));
        __RSUniCharNonBMPPrecompDestinationTable = (uint32_t *)(((intptr_t)__RSUniCharBMPPrecompDestinationTable) + bmpMappingLength);
        
        __RSUniCharNonBaseBitmapForBMP_P = RSUniCharGetBitmapPtrForPlane(RSUniCharNonBaseCharacterSet, 0);
        __RSUniCharCombiningClassForBMP = (const uint8_t *)RSUniCharGetUnicodePropertyDataForPlane(RSUniCharCombiningProperty, 0);
    }
    
    RSSpinLockUnlock(&__RSUniCharPrecompositionTableLock);
}

// Adding _P so the symbol name is different from the one in RSUnicodeDecomposition.c
#define __RSUniCharIsNonBaseCharacter	__RSUniCharIsNonBaseCharacter_P
RSInline BOOL __RSUniCharIsNonBaseCharacter(UTF32Char character) {
    return RSUniCharIsMemberOfBitmap(character, (character < 0x10000 ? __RSUniCharNonBaseBitmapForBMP_P : RSUniCharGetBitmapPtrForPlane(RSUniCharNonBaseCharacterSet, ((character >> 16) & 0xFF))));
}

typedef struct {
    UTF16Char _key;
    UTF16Char _value;
} __RSUniCharPrecomposeBMPMappings;

static UTF16Char __RSUniCharGetMappedBMPValue(const __RSUniCharPrecomposeBMPMappings *theTable, uint32_t numElem, UTF16Char character) {
    const __RSUniCharPrecomposeBMPMappings *p, *q, *divider;
    
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
} __RSUniCharPrecomposeMappings;

static uint32_t __RSUniCharGetMappedValue_P(const __RSUniCharPrecomposeMappings *theTable, uint32_t numElem, UTF32Char character) {
    const __RSUniCharPrecomposeMappings *p, *q, *divider;
    
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

__private_extern__
UTF32Char RSUniCharPrecomposeCharacter(UTF32Char base, UTF32Char combining) {
    uint32_t value;
    
    if (NULL == __RSUniCharPrecompSourceTable) __RSUniCharLoadPrecompositionTable();
    
    if (!(value = __RSUniCharGetMappedValue_P((const __RSUniCharPrecomposeMappings *)__RSUniCharPrecompSourceTable, __RSUniCharPrecompositionTableLength, combining))) return 0xFFFD;
    
    // We don't have precomposition in non-BMP
    if (value & RSUniCharNonBmpFlag) {
        value = __RSUniCharGetMappedValue_P((const __RSUniCharPrecomposeMappings *)((uint32_t *)__RSUniCharNonBMPPrecompDestinationTable + (value & 0xFFFF)), (value >> 16) & 0x7FFF, base);
    } else {
        value = __RSUniCharGetMappedBMPValue((const __RSUniCharPrecomposeBMPMappings *)((uint32_t *)__RSUniCharBMPPrecompDestinationTable + (value & 0xFFFF)), (value >> 16), base);
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

RSInline void __RSUniCharMoveBufferFromEnd0(UTF16Char *convertedChars, RSIndex length, RSIndex delta) {
    const UTF16Char *limit = convertedChars;
    UTF16Char *dstP;
    
    convertedChars += length;
    dstP = convertedChars + delta;
    
    while (convertedChars > limit) *(--dstP) = *(--convertedChars);
}

BOOL RSUniCharPrecompose(const UTF16Char *characters, RSIndex length, RSIndex *consumedLength, UTF16Char *precomposed, RSIndex maxLength, RSIndex *filledLength) {
    UTF32Char currentChar = 0, lastChar = 0, precomposedChar = 0xFFFD;
    RSIndex originalLength = length, usedLength = 0;
    UTF16Char *currentBase = precomposed;
    uint8_t currentClass, lastClass = 0;
    BOOL currentBaseIsBMP = YES;
    BOOL isPrecomposed;
    
    if (NULL == __RSUniCharPrecompSourceTable) __RSUniCharLoadPrecompositionTable();
    
    while (length > 0) {
        currentChar = *(characters++);
        --length;
        
        if (RSUniCharIsSurrogateHighCharacter(currentChar) && (length > 0) && RSUniCharIsSurrogateLowCharacter(*characters)) {
            currentChar = RSUniCharGetLongCharacterForSurrogatePair(currentChar, *(characters++));
            --length;
        }
        
        if (lastChar && __RSUniCharIsNonBaseCharacter(currentChar)) {
            isPrecomposed = (precomposedChar == 0xFFFD ? NO : YES);
            if (isPrecomposed) lastChar = precomposedChar;
            
            currentClass = (currentChar > 0xFFFF ? RSUniCharGetUnicodeProperty(currentChar, RSUniCharCombiningProperty) : RSUniCharGetCombiningPropertyForCharacter(currentChar, __RSUniCharCombiningClassForBMP));
            
            if ((lastClass == 0) || (currentClass > lastClass)) {
                if ((precomposedChar = RSUniCharPrecomposeCharacter(lastChar, currentChar)) == 0xFFFD) {
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
                        __RSUniCharMoveBufferFromEnd0(currentBase + 1, precomposed - (currentBase + 1), 1);
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
                currentBaseIsBMP = NO;
            } else {
                ++usedLength;
                if (usedLength > maxLength) break;
                *(precomposed++) = (UTF16Char)currentChar;
                currentBaseIsBMP = YES;
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
                    return NO;
                }
                __RSUniCharMoveBufferFromEnd0(currentBase + 1, precomposed - (currentBase + 1), 1);
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
    
    return YES;
}

#undef __RSUniCharIsNonBaseCharacter
#undef HANGUL_SBASE
#undef HANGUL_LBASE
#undef HANGUL_VBASE
#undef HANGUL_TBASE
#undef HANGUL_SCOUNT
#undef HANGUL_LCOUNT
#undef HANGUL_VCOUNT
#undef HANGUL_TCOUNT
#undef HANGUL_NCOUNT


