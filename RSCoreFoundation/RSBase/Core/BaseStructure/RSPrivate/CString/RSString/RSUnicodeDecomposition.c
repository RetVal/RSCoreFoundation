//
//  RSUnicodeDecomposition.c
//  RSCoreFoundation
//
//  Created by RetVal on 7/28/13.
//  Copyright (c) 2013 RetVal. All rights reserved.
//
#include <string.h>
#include <RSCoreFoundation/RSBase.h>
//#include <RSCoreFoundation/RSCharacterSet.h>
//#include <RSCoreFoundation/RSUniChar.h>
#include "RSUniChar.h"
//#include <RSCoreFoundation/RSUnicodeDecomposition.h>
#include "RSUnicodeDecomposition.h"
#include "RSInternal.h"
#include "RSUniCharPrivate.h"

// Canonical 
static UTF32Char *__RSUniCharDecompositionTable = nil;
static uint32_t __RSUniCharDecompositionTableLength = 0;
static UTF32Char *__RSUniCharMultipleDecompositionTable = nil;

static const uint8_t *__RSUniCharDecomposableBitmapForBMP = nil;
static const uint8_t *__RSUniCharHFSPlusDecomposableBitmapForBMP = nil;

static RSSpinLock __RSUniCharDecompositionTableLock = RSSpinLockInit;

static const uint8_t **__RSUniCharCombiningPriorityTable = nil;
static uint8_t __RSUniCharCombiningPriorityTableNumPlane = 0;

static void __RSUniCharLoadDecompositionTable(void) {
    
    RSSpinLockLock(&__RSUniCharDecompositionTableLock);
    
    if (nil == __RSUniCharDecompositionTable) {
        const uint32_t *bytes = (uint32_t *)RSUniCharGetMappingData(RSUniCharCanonicalDecompMapping);
        
        if (nil == bytes) {
            RSSpinLockUnlock(&__RSUniCharDecompositionTableLock);
            return;
        }
        
        __RSUniCharDecompositionTableLength = *(bytes++);
        __RSUniCharDecompositionTable = (UTF32Char *)bytes;
        __RSUniCharMultipleDecompositionTable = (UTF32Char *)((intptr_t)bytes + __RSUniCharDecompositionTableLength);
        
        __RSUniCharDecompositionTableLength /= (sizeof(uint32_t) * 2);
        __RSUniCharDecomposableBitmapForBMP = RSUniCharGetBitmapPtrForPlane(RSUniCharCanonicalDecomposableCharacterSet, 0);
        __RSUniCharHFSPlusDecomposableBitmapForBMP = RSUniCharGetBitmapPtrForPlane(RSUniCharHFSPlusDecomposableCharacterSet, 0);
        
        RSIndex idx;
        
        __RSUniCharCombiningPriorityTableNumPlane = RSUniCharGetNumberOfPlanesForUnicodePropertyData(RSUniCharCombiningProperty);
        __RSUniCharCombiningPriorityTable = (const uint8_t **)RSAllocatorAllocate(RSAllocatorSystemDefault, sizeof(uint8_t *) * __RSUniCharCombiningPriorityTableNumPlane);
        for (idx = 0;idx < __RSUniCharCombiningPriorityTableNumPlane;idx++) __RSUniCharCombiningPriorityTable[idx] = (const uint8_t *)RSUniCharGetUnicodePropertyDataForPlane(RSUniCharCombiningProperty, (RSBitU32)idx);
    }
    
    RSSpinLockUnlock(&__RSUniCharDecompositionTableLock);
}

static RSSpinLock __RSUniCharCompatibilityDecompositionTableLock = RSSpinLockInit;
static UTF32Char *__RSUniCharCompatibilityDecompositionTable = nil;
static uint32_t __RSUniCharCompatibilityDecompositionTableLength = 0;
static UTF32Char *__RSUniCharCompatibilityMultipleDecompositionTable = nil;

static void __RSUniCharLoadCompatibilityDecompositionTable(void) {
    
    RSSpinLockLock(&__RSUniCharCompatibilityDecompositionTableLock);
    
    if (nil == __RSUniCharCompatibilityDecompositionTable) {
        const uint32_t *bytes = (uint32_t *)RSUniCharGetMappingData(RSUniCharCompatibilityDecompMapping);
        
        if (nil == bytes) {
            RSSpinLockUnlock(&__RSUniCharCompatibilityDecompositionTableLock);
            return;
        }
        
        __RSUniCharCompatibilityDecompositionTableLength = *(bytes++);
        __RSUniCharCompatibilityDecompositionTable = (UTF32Char *)bytes;
        __RSUniCharCompatibilityMultipleDecompositionTable = (UTF32Char *)((intptr_t)bytes + __RSUniCharCompatibilityDecompositionTableLength);
        
        __RSUniCharCompatibilityDecompositionTableLength /= (sizeof(uint32_t) * 2);
    }
    
    RSSpinLockUnlock(&__RSUniCharCompatibilityDecompositionTableLock);
}

RSInline BOOL __RSUniCharIsDecomposableCharacterWithFlag(UTF32Char character, BOOL isHFSPlus) {
    return RSUniCharIsMemberOfBitmap(character, (character < 0x10000 ? (isHFSPlus ? __RSUniCharHFSPlusDecomposableBitmapForBMP : __RSUniCharDecomposableBitmapForBMP) : RSUniCharGetBitmapPtrForPlane(RSUniCharCanonicalDecomposableCharacterSet, ((character >> 16) & 0xFF))));
}

RSInline uint8_t __RSUniCharGetCombiningPropertyForCharacter(UTF32Char character) { return RSUniCharGetCombiningPropertyForCharacter(character, (((character) >> 16) < __RSUniCharCombiningPriorityTableNumPlane ? __RSUniCharCombiningPriorityTable[(character) >> 16] : nil)); }

RSInline BOOL __RSUniCharIsNonBaseCharacter(UTF32Char character) { return ((0 == __RSUniCharGetCombiningPropertyForCharacter(character)) ? NO : YES); } // the notion of non-base in normalization is characters with non-0 combining class

typedef struct {
    uint32_t _key;
    uint32_t _value;
} __RSUniCharDecomposeMappings;

static uint32_t __RSUniCharGetMappedValue(const __RSUniCharDecomposeMappings *theTable, uint32_t numElem, UTF32Char character) {
    const __RSUniCharDecomposeMappings *p, *q, *divider;
    
    if ((character < theTable[0]._key) || (character > theTable[numElem-1]._key)) {
        return 0;
    }
    p = theTable;
    q = p + (numElem-1);
    while (p <= q) {
        divider = p + ((q - p) >> 1);    /* divide by 2 */
        if (character < divider->_key) { q = divider - 1; }
        else if (character > divider->_key) { p = divider + 1; }
        else { return divider->_value; }
    }
    return 0;
}

static void __RSUniCharPrioritySort(UTF32Char *characters, RSIndex length) {
    UTF32Char *end = characters + length;
    
    while ((characters < end) && (0 == __RSUniCharGetCombiningPropertyForCharacter(*characters))) ++characters;
    
    if ((end - characters) > 1) {
        uint32_t p1, p2;
        UTF32Char *ch1, *ch2;
        BOOL changes = YES;
        
        do {
            changes = NO;
            ch1 = characters; ch2 = characters + 1;
            p2 = __RSUniCharGetCombiningPropertyForCharacter(*ch1);
            while (ch2 < end) {
                p1 = p2; p2 = __RSUniCharGetCombiningPropertyForCharacter(*ch2);
                if (p1 > p2) {
                    UTF32Char tmp = *ch1; *ch1 = *ch2; *ch2 = tmp;
                    changes = YES;
                }
                ++ch1; ++ch2;
            }
        } while (changes);
    }
}

static RSIndex __RSUniCharRecursivelyDecomposeCharacter(UTF32Char character, UTF32Char *convertedChars, RSIndex maxBufferLength) {
    uint32_t value = __RSUniCharGetMappedValue((const __RSUniCharDecomposeMappings *)__RSUniCharDecompositionTable, __RSUniCharDecompositionTableLength, character);
    RSIndex length = RSUniCharConvertFlagToCount(value);
    UTF32Char firstChar = value & 0xFFFFFF;
    UTF32Char *mappings = (length > 1 ? __RSUniCharMultipleDecompositionTable + firstChar : &firstChar);
    RSIndex usedLength = 0;
    
    if (maxBufferLength < length) return 0;
    
    if (value & RSUniCharRecursiveDecompositionFlag) {
        usedLength = __RSUniCharRecursivelyDecomposeCharacter(*mappings, convertedChars, maxBufferLength - length);
        
        --length; // Decrement for the first char
        if (!usedLength || usedLength + length > maxBufferLength) return 0;
        ++mappings;
        convertedChars += usedLength;
    }
    
    usedLength += length;
    
    while (length--) *(convertedChars++) = *(mappings++);
    
    return usedLength;
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

RSIndex RSUniCharDecomposeCharacter(UTF32Char character, UTF32Char *convertedChars, RSIndex maxBufferLength) {
    if (nil == __RSUniCharDecompositionTable) __RSUniCharLoadDecompositionTable();
    if (character >= HANGUL_SBASE && character <= (HANGUL_SBASE + HANGUL_SCOUNT)) {
        RSIndex length;
        
        character -= HANGUL_SBASE;
        
        length = (character % HANGUL_TCOUNT ? 3 : 2);
        
        if (maxBufferLength < length) return 0;
        
        *(convertedChars++) = character / HANGUL_NCOUNT + HANGUL_LBASE;
        *(convertedChars++) = (character % HANGUL_NCOUNT) / HANGUL_TCOUNT + HANGUL_VBASE;
        if (length > 2) *convertedChars = (character % HANGUL_TCOUNT) + HANGUL_TBASE;
        return length;
    } else {
        return __RSUniCharRecursivelyDecomposeCharacter(character, convertedChars, maxBufferLength);
    }
}

RSInline BOOL __RSProcessReorderBuffer(UTF32Char *buffer, RSIndex length, void **dst, RSIndex dstLength, RSIndex *filledLength, uint32_t dstFormat) {
    if (length > 1) __RSUniCharPrioritySort(buffer, length);
    return RSUniCharFillDestinationBuffer(buffer, length, dst, dstLength, filledLength, dstFormat);
}

#define MAX_BUFFER_LENGTH (32)
BOOL RSUniCharDecompose(const UTF16Char *src, RSIndex length, RSIndex *consumedLength, void *dst, RSIndex maxLength, RSIndex *filledLength, BOOL needToReorder, uint32_t dstFormat, BOOL isHFSPlus) {
    RSIndex usedLength = 0;
    RSIndex originalLength = length;
    UTF32Char buffer[MAX_BUFFER_LENGTH];
    UTF32Char *decompBuffer = buffer;
    RSIndex decompBufferSize = MAX_BUFFER_LENGTH;
    RSIndex decompBufferLen = 0;
    RSIndex segmentLength = 0;
    UTF32Char currentChar;
    
    if (nil == __RSUniCharDecompositionTable) __RSUniCharLoadDecompositionTable();
    
    while ((length - segmentLength) > 0) {
        currentChar = *(src++);
        
        if (currentChar < 0x80) {
            if (decompBufferLen > 0) {
                if (!__RSProcessReorderBuffer(decompBuffer, decompBufferLen, &dst, maxLength, &usedLength, dstFormat)) break;
                length -= segmentLength;
                segmentLength = 0;
                decompBufferLen = 0;
            }
            
            if (maxLength > 0) {
                if (usedLength >= maxLength) break;
                switch (dstFormat) {
                    case RSUniCharUTF8Format: *(uint8_t *)dst = currentChar; dst = (uint8_t *)dst + sizeof(uint8_t); break;
                    case RSUniCharUTF16Format: *(UTF16Char *)dst = currentChar; dst = (uint8_t *)dst + sizeof(UTF16Char); break;
                    case RSUniCharUTF32Format: *(UTF32Char *)dst = currentChar; dst = (uint8_t *)dst + sizeof(UTF32Char); break;
                }
            }
            
            --length;
            ++usedLength;
        } else {
            if (RSUniCharIsSurrogateLowCharacter(currentChar)) { // Stray surrogagte
                if (dstFormat != RSUniCharUTF16Format) break;
            } else if (RSUniCharIsSurrogateHighCharacter(currentChar)) {
                if (((length - segmentLength) > 1) && RSUniCharIsSurrogateLowCharacter(*src)) {
                    currentChar = RSUniCharGetLongCharacterForSurrogatePair(currentChar, *(src++));
                } else {
                    if (dstFormat != RSUniCharUTF16Format) break;
                }
            }
            
            if (needToReorder && __RSUniCharIsNonBaseCharacter(currentChar)) {
                if ((decompBufferLen + 1) >= decompBufferSize) {
                    UTF32Char *newBuffer;
                    
                    decompBufferSize += MAX_BUFFER_LENGTH;
                    newBuffer = (UTF32Char *)RSAllocatorAllocate(RSAllocatorSystemDefault, sizeof(UTF32Char) * decompBufferSize);
                    memmove(newBuffer, decompBuffer, (decompBufferSize - MAX_BUFFER_LENGTH) * sizeof(UTF32Char));
                    if (decompBuffer != buffer) RSAllocatorDeallocate(RSAllocatorSystemDefault, decompBuffer);
                    decompBuffer = newBuffer;
                }
                
                if (__RSUniCharIsDecomposableCharacterWithFlag(currentChar, isHFSPlus)) { // Vietnamese accent, etc.
                    decompBufferLen += RSUniCharDecomposeCharacter(currentChar, decompBuffer + decompBufferLen, decompBufferSize - decompBufferLen);
                } else {
                    decompBuffer[decompBufferLen++] = currentChar;
                }
            } else {
                if (decompBufferLen > 0) {
                    if (!__RSProcessReorderBuffer(decompBuffer, decompBufferLen, &dst, maxLength, &usedLength, dstFormat)) break;
                    length -= segmentLength;
                    segmentLength = 0;
                }
                
                if (__RSUniCharIsDecomposableCharacterWithFlag(currentChar, isHFSPlus)) {
                    decompBufferLen = RSUniCharDecomposeCharacter(currentChar, decompBuffer, MAX_BUFFER_LENGTH);
                } else {
                    decompBufferLen = 1;
                    *decompBuffer = currentChar;
                }
                
                if (!needToReorder || (decompBufferLen == 1)) {
                    if (!RSUniCharFillDestinationBuffer(decompBuffer, decompBufferLen, &dst, maxLength, &usedLength, dstFormat)) break;
                    length -= ((currentChar > 0xFFFF) ? 2 : 1);
                    decompBufferLen = 0;
                    continue;
                }
            }
            
            segmentLength += ((currentChar > 0xFFFF) ? 2 : 1);
        }
    }
    if ((decompBufferLen > 0) && __RSProcessReorderBuffer(decompBuffer, decompBufferLen, &dst, maxLength, &usedLength, dstFormat)) length -= segmentLength;
    
    if (decompBuffer != buffer) RSAllocatorDeallocate(RSAllocatorSystemDefault, decompBuffer);
    
    if (consumedLength) *consumedLength = originalLength - length;
    if (filledLength) *filledLength = usedLength;
    
    return ((length > 0) ? NO : YES);
}

#define MAX_COMP_DECOMP_LEN (32)

static RSIndex __RSUniCharRecursivelyCompatibilityDecomposeCharacter(UTF32Char character, UTF32Char *convertedChars) {
    uint32_t value = __RSUniCharGetMappedValue((const __RSUniCharDecomposeMappings *)__RSUniCharCompatibilityDecompositionTable, __RSUniCharCompatibilityDecompositionTableLength, character);
    RSIndex length = RSUniCharConvertFlagToCount(value);
    UTF32Char firstChar = value & 0xFFFFFF;
    const UTF32Char *mappings = (length > 1 ? __RSUniCharCompatibilityMultipleDecompositionTable + firstChar : &firstChar);
    RSIndex usedLength = length;
    UTF32Char currentChar;
    RSIndex currentLength;
    
    while (length-- > 0) {
        currentChar = *(mappings++);
        if (__RSUniCharIsDecomposableCharacterWithFlag(currentChar, NO)) {
            currentLength = __RSUniCharRecursivelyDecomposeCharacter(currentChar, convertedChars, MAX_COMP_DECOMP_LEN - length);
            convertedChars += currentLength;
            usedLength += (currentLength - 1);
        } else if (RSUniCharIsMemberOf(currentChar, RSUniCharCompatibilityDecomposableCharacterSet)) {
            currentLength = __RSUniCharRecursivelyCompatibilityDecomposeCharacter(currentChar, convertedChars);
            convertedChars += currentLength;
            usedLength += (currentLength - 1);
        } else {
            *(convertedChars++) = currentChar;
        }
    }
    
    return usedLength;
}

RSInline void __RSUniCharMoveBufferFromEnd1(UTF32Char *convertedChars, RSIndex length, RSIndex delta) {
    const UTF32Char *limit = convertedChars;
    UTF32Char *dstP;
    
    convertedChars += length;
    dstP = convertedChars + delta;
    
    while (convertedChars > limit) *(--dstP) = *(--convertedChars);
}

RSPrivate RSIndex RSUniCharCompatibilityDecompose(UTF32Char *convertedChars, RSIndex length, RSIndex maxBufferLength) {
    UTF32Char currentChar;
    UTF32Char buffer[MAX_COMP_DECOMP_LEN];
    const UTF32Char *bufferP;
    const UTF32Char *limit = convertedChars + length;
    RSIndex filledLength;
    
    if (nil == __RSUniCharCompatibilityDecompositionTable) __RSUniCharLoadCompatibilityDecompositionTable();
    
    while (convertedChars < limit) {
        currentChar = *convertedChars;
        
        if (RSUniCharIsMemberOf(currentChar, RSUniCharCompatibilityDecomposableCharacterSet)) {
            filledLength = __RSUniCharRecursivelyCompatibilityDecomposeCharacter(currentChar, buffer);
            
            if (filledLength + length - 1 > maxBufferLength) return 0;
            
            if (filledLength > 1) __RSUniCharMoveBufferFromEnd1(convertedChars + 1, limit - convertedChars - 1, filledLength - 1);
            
            bufferP = buffer;
            length += (filledLength - 1);
            while (filledLength-- > 0) *(convertedChars++) = *(bufferP++);
        } else {
            ++convertedChars;
        }
    }
    
    return length;
}

RS_EXPORT void RSUniCharPrioritySort(UTF32Char *characters, RSIndex length) {
    __RSUniCharPrioritySort(characters, length);
}

#undef MAX_BUFFER_LENGTH
#undef MAX_COMP_DECOMP_LEN
#undef HANGUL_SBASE
#undef HANGUL_LBASE
#undef HANGUL_VBASE
#undef HANGUL_TBASE
#undef HANGUL_SCOUNT
#undef HANGUL_LCOUNT
#undef HANGUL_VCOUNT
#undef HANGUL_TCOUNT
#undef HANGUL_NCOUNT
