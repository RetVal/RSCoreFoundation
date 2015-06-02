//
//  UnicodeDecomposition.cpp
//  RSCoreFoundation
//
//  Created by closure on 6/2/15.
//  Copyright (c) 2015 RetVal. All rights reserved.
//

#include <RSFoundation/UnicodeDecomposition.h>
#include <RSFoundation/Lock.h>
#include <RSFoundation/Allocator.h>

using namespace RSFoundation;
using namespace RSFoundation::Basic;
using namespace RSFoundation::Collection;

namespace {
    // Canonical 
    static UTF32Char *__DecompositionTable = nullptr;
    static uint32_t __DecompositionTableLength = 0;
    static UTF32Char *__MultipleDecompositionTable = nullptr;
    
    static const uint8_t *__DecomposableBitmapForBMP = nullptr;
    static const uint8_t *__HFSPlusDecomposableBitmapForBMP = nullptr;
    
    static SpinLock __DecompositionTableLock;
    
    static const uint8_t **__CombiningPriorityTable = nullptr;
    static uint8_t __CombiningPriorityTableNumPlane = 0;
    
    static void __LoadDecompositionTable(void) {
        __DecompositionTableLock.Acquire();
        
        if (nullptr == __DecompositionTable) {
            const uint32_t *bytes = (uint32_t *)UniCharGetMappingData(UnicharMappingType::CanonicalDecompMapping);
            
            if (nullptr == bytes) {
                __DecompositionTableLock.Release();
                return;
            }
            
            __DecompositionTableLength = *(bytes++);
            __DecompositionTable = (UTF32Char *)bytes;
            __MultipleDecompositionTable = (UTF32Char *)((intptr_t)bytes + __DecompositionTableLength);
            
            __DecompositionTableLength /= (sizeof(uint32_t) * 2);
            __DecomposableBitmapForBMP = UniCharGetBitmapPtrForPlane(CanonicalDecomposableCharacterSet, 0);
            __HFSPlusDecomposableBitmapForBMP = UniCharGetBitmapPtrForPlane(HFSPlusDecomposableCharacterSet, 0);
            
            RSIndex idx;
            
            __CombiningPriorityTableNumPlane = UniCharGetNumberOfPlanesForUnicodePropertyData(UniCharCombiningProperty);
            
            __CombiningPriorityTable = (const uint8_t **)Allocator<uint8_t*>::AllocatorSystemDefault.Allocate<uint8_t*>(__CombiningPriorityTableNumPlane);
            for (idx = 0;idx < __CombiningPriorityTableNumPlane;idx++) __CombiningPriorityTable[idx] = (const uint8_t *)UniCharGetUnicodePropertyDataForPlane(UniCharCombiningProperty, (uint32_t)idx);
        }
        
        __DecompositionTableLock.Release();
    }
    
    static SpinLock __CompatibilityDecompositionTableLock;
    static UTF32Char *__CompatibilityDecompositionTable = nullptr;
    static uint32_t __CompatibilityDecompositionTableLength = 0;
    static UTF32Char *__CompatibilityMultipleDecompositionTable = nullptr;
    
    static void __LoadCompatibilityDecompositionTable(void) {
        __CompatibilityDecompositionTableLock.Acquire();
        
        if (nullptr == __CompatibilityDecompositionTable) {
            const uint32_t *bytes = (uint32_t *)UniCharGetMappingData(UnicharMappingType::CompatibilityDecompMapping);
            
            if (nullptr == bytes) {
                __CompatibilityDecompositionTableLock.Release();
                return;
            }
            
            __CompatibilityDecompositionTableLength = *(bytes++);
            __CompatibilityDecompositionTable = (UTF32Char *)bytes;
            __CompatibilityMultipleDecompositionTable = (UTF32Char *)((intptr_t)bytes + __CompatibilityDecompositionTableLength);
            
            __CompatibilityDecompositionTableLength /= (sizeof(uint32_t) * 2);
        }
        
        __CompatibilityDecompositionTableLock.Release();
    }
    
    inline BOOL __IsDecomposableCharacterWithFlag(UTF32Char character, BOOL isHFSPlus) {
        return UniCharIsMemberOfBitmap(character, (character < 0x10000 ? (isHFSPlus ? __HFSPlusDecomposableBitmapForBMP : __DecomposableBitmapForBMP) : UniCharGetBitmapPtrForPlane(CanonicalDecomposableCharacterSet, ((character >> 16) & 0xFF))));
    }
    
    inline uint8_t __GetCombiningPropertyForCharacter(UTF32Char character) { return UniCharGetCombiningPropertyForCharacter(character, (((character) >> 16) < __CombiningPriorityTableNumPlane ? __CombiningPriorityTable[(character) >> 16] : nil)); }
    
    inline BOOL __IsNonBaseCharacter(UTF32Char character) { return ((0 == __GetCombiningPropertyForCharacter(character)) ? NO : YES); } // the notion of non-base in normalization is characters with non-0 combining class
    
    typedef struct {
        uint32_t _key;
        uint32_t _value;
    } __DecomposeMappings;
    
    static uint32_t __GetMappedValue(const __DecomposeMappings *theTable, uint32_t numElem, UTF32Char character) {
        const __DecomposeMappings *p, *q, *divider;
        
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
    
    static void __PrioritySort(UTF32Char *characters, RSIndex length) {
        UTF32Char *end = characters + length;
        
        while ((characters < end) && (0 == __GetCombiningPropertyForCharacter(*characters))) ++characters;
        
        if ((end - characters) > 1) {
            uint32_t p1, p2;
            UTF32Char *ch1, *ch2;
            BOOL changes = YES;
            
            do {
                changes = NO;
                ch1 = characters; ch2 = characters + 1;
                p2 = __GetCombiningPropertyForCharacter(*ch1);
                while (ch2 < end) {
                    p1 = p2; p2 = __GetCombiningPropertyForCharacter(*ch2);
                    if (p1 > p2) {
                        UTF32Char tmp = *ch1; *ch1 = *ch2; *ch2 = tmp;
                        changes = YES;
                    }
                    ++ch1; ++ch2;
                }
            } while (changes);
        }
    }
    
    static RSIndex __RecursivelyDecomposeCharacter(UTF32Char character, UTF32Char *convertedChars, RSIndex maxBufferLength) {
        uint32_t value = __GetMappedValue((const __DecomposeMappings *)__DecompositionTable, __DecompositionTableLength, character);
        RSIndex length = UniCharConvertFlagToCount(value);
        UTF32Char firstChar = value & 0xFFFFFF;
        UTF32Char *mappings = (length > 1 ? __MultipleDecompositionTable + firstChar : &firstChar);
        RSIndex usedLength = 0;
        
        if (maxBufferLength < length) return 0;
        
        if (value & UniCharRecursiveDecompositionFlag) {
            usedLength = __RecursivelyDecomposeCharacter(*mappings, convertedChars, maxBufferLength - length);
            
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
    
    RSIndex DecomposeCharacter(UTF32Char character, UTF32Char *convertedChars, RSIndex maxBufferLength) {
        if (nil == __DecompositionTable) __LoadDecompositionTable();
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
            return __RecursivelyDecomposeCharacter(character, convertedChars, maxBufferLength);
        }
    }
    
    inline BOOL __RSProcessReorderBuffer(UTF32Char *buffer, RSIndex length, void **dst, RSIndex dstLength, RSIndex *filledLength, UniCharEncodingFormat dstFormat) {
        if (length > 1) __PrioritySort(buffer, length);
        return UniCharFillDestinationBuffer(buffer, length, dst, dstLength, filledLength, dstFormat);
    }
    
#define MAX_BUFFER_LENGTH (32)
    BOOL Decompose(const UTF16Char *src, RSIndex length, RSIndex *consumedLength, void *dst, RSIndex maxLength, RSIndex *filledLength, BOOL needToReorder, UniCharEncodingFormat dstFormat, BOOL isHFSPlus) {
        RSIndex usedLength = 0;
        RSIndex originalLength = length;
        UTF32Char buffer[MAX_BUFFER_LENGTH];
        UTF32Char *decompBuffer = buffer;
        RSIndex decompBufferSize = MAX_BUFFER_LENGTH;
        RSIndex decompBufferLen = 0;
        RSIndex segmentLength = 0;
        UTF32Char currentChar;
        
        if (nil == __DecompositionTable) __LoadDecompositionTable();
        
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
                        case UniCharEncodingFormat::UTF8Format: *(uint8_t *)dst = currentChar; dst = (uint8_t *)dst + sizeof(uint8_t); break;
                        case UniCharEncodingFormat::UTF16Format: *(UTF16Char *)dst = currentChar; dst = (uint8_t *)dst + sizeof(UTF16Char); break;
                        case UniCharEncodingFormat::UTF32Format: *(UTF32Char *)dst = currentChar; dst = (uint8_t *)dst + sizeof(UTF32Char); break;
                    }
                }
                
                --length;
                ++usedLength;
            } else {
                if (UniCharIsSurrogateLowCharacter(currentChar)) { // Stray surrogagte
                    if (dstFormat != UniCharEncodingFormat::UTF16Format) break;
                } else if (UniCharIsSurrogateHighCharacter(currentChar)) {
                    if (((length - segmentLength) > 1) && UniCharIsSurrogateLowCharacter(*src)) {
                        currentChar = UniCharGetLongCharacterForSurrogatePair(currentChar, *(src++));
                    } else {
                        if (dstFormat != UniCharEncodingFormat::UTF16Format) break;
                    }
                }
                
                if (needToReorder && __IsNonBaseCharacter(currentChar)) {
                    if ((decompBufferLen + 1) >= decompBufferSize) {
                        UTF32Char *newBuffer;
                        
                        decompBufferSize += MAX_BUFFER_LENGTH;
                        newBuffer = Allocator<UTF32Char>::AllocatorSystemDefault.Allocate<UTF32Char>(decompBufferSize);
//                        newBuffer = (UTF32Char *)RSAllocatorAllocate(RSAllocatorSystemDefault, sizeof(UTF32Char) * decompBufferSize);
                        memmove(newBuffer, decompBuffer, (decompBufferSize - MAX_BUFFER_LENGTH) * sizeof(UTF32Char));
                        if (decompBuffer != buffer) {
                            Allocator<UTF32Char>::AllocatorSystemDefault.Deallocate(decompBuffer);
                        }
//                        if (decompBuffer != buffer) RSAllocatorDeallocate(RSAllocatorSystemDefault, decompBuffer);
                        decompBuffer = newBuffer;
                    }
                    
                    if (__IsDecomposableCharacterWithFlag(currentChar, isHFSPlus)) { // Vietnamese accent, etc.
                        decompBufferLen += DecomposeCharacter(currentChar, decompBuffer + decompBufferLen, decompBufferSize - decompBufferLen);
                    } else {
                        decompBuffer[decompBufferLen++] = currentChar;
                    }
                } else {
                    if (decompBufferLen > 0) {
                        if (!__RSProcessReorderBuffer(decompBuffer, decompBufferLen, &dst, maxLength, &usedLength, dstFormat)) break;
                        length -= segmentLength;
                        segmentLength = 0;
                    }
                    
                    if (__IsDecomposableCharacterWithFlag(currentChar, isHFSPlus)) {
                        decompBufferLen = DecomposeCharacter(currentChar, decompBuffer, MAX_BUFFER_LENGTH);
                    } else {
                        decompBufferLen = 1;
                        *decompBuffer = currentChar;
                    }
                    
                    if (!needToReorder || (decompBufferLen == 1)) {
                        if (!UniCharFillDestinationBuffer(decompBuffer, decompBufferLen, &dst, maxLength, &usedLength, dstFormat)) break;
                        length -= ((currentChar > 0xFFFF) ? 2 : 1);
                        decompBufferLen = 0;
                        continue;
                    }
                }
                
                segmentLength += ((currentChar > 0xFFFF) ? 2 : 1);
            }
        }
        if ((decompBufferLen > 0) && __RSProcessReorderBuffer(decompBuffer, decompBufferLen, &dst, maxLength, &usedLength, dstFormat)) length -= segmentLength;
        
        if (decompBuffer != buffer) Allocator<UTF32Char>::AllocatorSystemDefault.Deallocate(decompBuffer);
        
        if (consumedLength) *consumedLength = originalLength - length;
        if (filledLength) *filledLength = usedLength;
        
        return ((length > 0) ? NO : YES);
    }
    
#define MAX_COMP_DECOMP_LEN (32)
    
    static RSIndex __RecursivelyCompatibilityDecomposeCharacter(UTF32Char character, UTF32Char *convertedChars) {
        uint32_t value = __GetMappedValue((const __DecomposeMappings *)__CompatibilityDecompositionTable, __CompatibilityDecompositionTableLength, character);
        RSIndex length = UniCharConvertFlagToCount(value);
        UTF32Char firstChar = value & 0xFFFFFF;
        const UTF32Char *mappings = (length > 1 ? __CompatibilityMultipleDecompositionTable + firstChar : &firstChar);
        RSIndex usedLength = length;
        UTF32Char currentChar;
        RSIndex currentLength;
        
        while (length-- > 0) {
            currentChar = *(mappings++);
            if (__IsDecomposableCharacterWithFlag(currentChar, NO)) {
                currentLength = __RecursivelyDecomposeCharacter(currentChar, convertedChars, MAX_COMP_DECOMP_LEN - length);
                convertedChars += currentLength;
                usedLength += (currentLength - 1);
            } else if (UniCharIsMemberOf(currentChar, CompatibilityDecomposableCharacterSet)) {
                currentLength = __RecursivelyCompatibilityDecomposeCharacter(currentChar, convertedChars);
                convertedChars += currentLength;
                usedLength += (currentLength - 1);
            } else {
                *(convertedChars++) = currentChar;
            }
        }
        
        return usedLength;
    }
    
    inline void __MoveBufferFromEnd1(UTF32Char *convertedChars, RSIndex length, RSIndex delta) {
        const UTF32Char *limit = convertedChars;
        UTF32Char *dstP;
        
        convertedChars += length;
        dstP = convertedChars + delta;
        
        while (convertedChars > limit) *(--dstP) = *(--convertedChars);
    }
    
    RSIndex CompatibilityDecompose(UTF32Char *convertedChars, RSIndex length, RSIndex maxBufferLength) {
        UTF32Char currentChar;
        UTF32Char buffer[MAX_COMP_DECOMP_LEN];
        const UTF32Char *bufferP;
        const UTF32Char *limit = convertedChars + length;
        RSIndex filledLength;
        
        if (nil == __CompatibilityDecompositionTable) __LoadCompatibilityDecompositionTable();
        
        while (convertedChars < limit) {
            currentChar = *convertedChars;
            
            if (UniCharIsMemberOf(currentChar, CompatibilityDecomposableCharacterSet)) {
                filledLength = __RecursivelyCompatibilityDecomposeCharacter(currentChar, buffer);
                
                if (filledLength + length - 1 > maxBufferLength) return 0;
                
                if (filledLength > 1) __MoveBufferFromEnd1(convertedChars + 1, limit - convertedChars - 1, filledLength - 1);
                
                bufferP = buffer;
                length += (filledLength - 1);
                while (filledLength-- > 0) *(convertedChars++) = *(bufferP++);
            } else {
                ++convertedChars;
            }
        }
        
        return length;
    }
    
    void PrioritySort(UTF32Char *characters, RSIndex length) {
        __PrioritySort(characters, length);
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

}
