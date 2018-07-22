//
//  RSStringEncodingConverter.c
//  RSCoreFoundation
//
//  Created by RetVal on 7/29/13.
//  Copyright (c) 2013 RetVal. All rights reserved.
//

#include "RSStringEncodingConverter.h"
#include "RSInternal.h"
#include <RSCoreFoundation/RSArray.h>
#include <RSCoreFoundation/RSDictionary.h>
#include "RSICUConverters.h"
#include "RSUniChar.h"
#include "RSFoundationEncoding.h"
#include <RSCoreFoundation/RSNotificationCenter.h>
//#include <RSCoreFoundation/RSPriv.h>
#include "RSUnicodeDecomposition.h"
#include "RSStringEncodingConverterExt.h"
#include "RSStringEncodingConverterPrivate.h"
#include "../../../../CoreFoundation/Sort/RSSortFunctions.h"
#include <stdlib.h>

typedef RSIndex (*_RSToBytesProc)(const void *converter, uint32_t flags, const UniChar *characters, RSIndex numChars, uint8_t *bytes, RSIndex maxByteLen, RSIndex *usedByteLen);
typedef RSIndex (*_RSToUnicodeProc)(const void *converter, uint32_t flags, const uint8_t *bytes, RSIndex numBytes, UniChar *characters, RSIndex maxCharLen, RSIndex *usedCharLen);

typedef struct {
    const RSStringEncodingConverter *definition;
    _RSToBytesProc toBytes;
    _RSToUnicodeProc toUnicode;
    _RSToUnicodeProc toCanonicalUnicode;
    RSStringEncodingToBytesFallbackProc toBytesFallback;
    RSStringEncodingToUnicodeFallbackProc toUnicodeFallback;
} _RSEncodingConverter;

/* Macros
 */
#define TO_BYTE(conv,flags,chars,numChars,bytes,max,used) (conv->toBytes ? conv->toBytes(conv,flags,chars,numChars,bytes,max,used) : ((RSStringEncodingToBytesProc)conv->definition->toBytes)(flags,chars,numChars,bytes,max,used))
#define TO_UNICODE(conv,flags,bytes,numBytes,chars,max,used) (conv->toUnicode ?  (flags & (RSStringEncodingUseCanonical|RSStringEncodingUseHFSPlusCanonical) ? conv->toCanonicalUnicode(conv,flags,bytes,numBytes,chars,max,used) : conv->toUnicode(conv,flags,bytes,numBytes,chars,max,used)) : ((RSStringEncodingToUnicodeProc)conv->definition->toUnicode)(flags,bytes,numBytes,chars,max,used))

#define ASCIINewLine 0x0a
#define kSurrogateHighStart 0xD800
#define kSurrogateHighEnd 0xDBFF
#define kSurrogateLowStart 0xDC00
#define kSurrogateLowEnd 0xDFFF

static const uint8_t __RSMaximumConvertedLength = 20;

/* Mapping 128..255 to lossy ASCII
 */
static const struct {
    unsigned char chars[4];
} _toLossyASCIITable[] = {
    {{' ', 0, 0, 0}}, // NO-BREAK SPACE
    {{'!', 0, 0, 0}}, // INVERTED EXCLAMATION MARK
    {{'c', 0, 0, 0}}, // CENT SIGN
    {{'L', 0, 0, 0}}, // POUND SIGN
    {{'$', 0, 0, 0}}, // CURRENCY SIGN
    {{'Y', 0, 0, 0}}, // YEN SIGN
    {{'|', 0, 0, 0}}, // BROKEN BAR
    {{0, 0, 0, 0}}, // SECTION SIGN
    {{0, 0, 0, 0}}, // DIAERESIS
    {{'(', 'C', ')', 0}}, // COPYRIGHT SIGN
    {{'a', 0, 0, 0}}, // FEMININE ORDINAL INDICATOR
    {{'<', '<', 0, 0}}, // LEFT-POINTING DOUBLE ANGLE QUOTATION MARK
    {{0, 0, 0, 0}}, // NOT SIGN
    {{'-', 0, 0, 0}}, // SOFT HYPHEN
    {{'(', 'R', ')', 0}}, // REGISTERED SIGN
    {{0, 0, 0, 0}}, // MACRON
    {{0, 0, 0, 0}}, // DEGREE SIGN
    {{'+', '-', 0, 0}}, // PLUS-MINUS SIGN
    {{'2', 0, 0, 0}}, // SUPERSCRIPT TWO
    {{'3', 0, 0, 0}}, // SUPERSCRIPT THREE
    {{0, 0, 0, 0}}, // ACUTE ACCENT
    {{0, 0, 0, 0}}, // MICRO SIGN
    {{0, 0, 0, 0}}, // PILCROW SIGN
    {{0, 0, 0, 0}}, // MIDDLE DOT
    {{0, 0, 0, 0}}, // CEDILLA
    {{'1', 0, 0, 0}}, // SUPERSCRIPT ONE
    {{'o', 0, 0, 0}}, // MASCULINE ORDINAL INDICATOR
    {{'>', '>', 0, 0}}, // RIGHT-POINTING DOUBLE ANGLE QUOTATION MARK
    {{'1', '/', '4', 0}}, // VULGAR FRACTION ONE QUARTER
    {{'1', '/', '2', 0}}, // VULGAR FRACTION ONE HALF
    {{'3', '/', '4', 0}}, // VULGAR FRACTION THREE QUARTERS
    {{'?', 0, 0, 0}}, // INVERTED QUESTION MARK
    {{'A', 0, 0, 0}}, // LATIN CAPITAL LETTER A WITH GRAVE
    {{'A', 0, 0, 0}}, // LATIN CAPITAL LETTER A WITH ACUTE
    {{'A', 0, 0, 0}}, // LATIN CAPITAL LETTER A WITH CIRCUMFLEX
    {{'A', 0, 0, 0}}, // LATIN CAPITAL LETTER A WITH TILDE
    {{'A', 0, 0, 0}}, // LATIN CAPITAL LETTER A WITH DIAERESIS
    {{'A', 0, 0, 0}}, // LATIN CAPITAL LETTER A WITH RING ABOVE
    {{'A', 'E', 0, 0}}, // LATIN CAPITAL LETTER AE
    {{'C', 0, 0, 0}}, // LATIN CAPITAL LETTER C WITH CEDILLA
    {{'E', 0, 0, 0}}, // LATIN CAPITAL LETTER E WITH GRAVE
    {{'E', 0, 0, 0}}, // LATIN CAPITAL LETTER E WITH ACUTE
    {{'E', 0, 0, 0}}, // LATIN CAPITAL LETTER E WITH CIRCUMFLEX
    {{'E', 0, 0, 0}}, // LATIN CAPITAL LETTER E WITH DIAERESIS
    {{'I', 0, 0, 0}}, // LATIN CAPITAL LETTER I WITH GRAVE
    {{'I', 0, 0, 0}}, // LATIN CAPITAL LETTER I WITH ACUTE
    {{'I', 0, 0, 0}}, // LATIN CAPITAL LETTER I WITH CIRCUMFLEX
    {{'I', 0, 0, 0}}, // LATIN CAPITAL LETTER I WITH DIAERESIS
    {{'T', 'H', 0, 0}}, // LATIN CAPITAL LETTER ETH (Icelandic)
    {{'N', 0, 0, 0}}, // LATIN CAPITAL LETTER N WITH TILDE
    {{'O', 0, 0, 0}}, // LATIN CAPITAL LETTER O WITH GRAVE
    {{'O', 0, 0, 0}}, // LATIN CAPITAL LETTER O WITH ACUTE
    {{'O', 0, 0, 0}}, // LATIN CAPITAL LETTER O WITH CIRCUMFLEX
    {{'O', 0, 0, 0}}, // LATIN CAPITAL LETTER O WITH TILDE
    {{'O', 0, 0, 0}}, // LATIN CAPITAL LETTER O WITH DIAERESIS
    {{'X', 0, 0, 0}}, // MULTIPLICATION SIGN
    {{'O', 0, 0, 0}}, // LATIN CAPITAL LETTER O WITH STROKE
    {{'U', 0, 0, 0}}, // LATIN CAPITAL LETTER U WITH GRAVE
    {{'U', 0, 0, 0}}, // LATIN CAPITAL LETTER U WITH ACUTE
    {{'U', 0, 0, 0}}, // LATIN CAPITAL LETTER U WITH CIRCUMFLEX
    {{'U', 0, 0, 0}}, // LATIN CAPITAL LETTER U WITH DIAERESIS
    {{'Y', 0, 0, 0}}, // LATIN CAPITAL LETTER Y WITH ACUTE
    {{'t', 'h', 0, 0}}, // LATIN CAPITAL LETTER THORN (Icelandic)
    {{'s', 0, 0, 0}}, // LATIN SMALL LETTER SHARP S (German)
    {{'a', 0, 0, 0}}, // LATIN SMALL LETTER A WITH GRAVE
    {{'a', 0, 0, 0}}, // LATIN SMALL LETTER A WITH ACUTE
    {{'a', 0, 0, 0}}, // LATIN SMALL LETTER A WITH CIRCUMFLEX
    {{'a', 0, 0, 0}}, // LATIN SMALL LETTER A WITH TILDE
    {{'a', 0, 0, 0}}, // LATIN SMALL LETTER A WITH DIAERESIS
    {{'a', 0, 0, 0}}, // LATIN SMALL LETTER A WITH RING ABOVE
    {{'a', 'e', 0, 0}}, // LATIN SMALL LETTER AE
    {{'c', 0, 0, 0}}, // LATIN SMALL LETTER C WITH CEDILLA
    {{'e', 0, 0, 0}}, // LATIN SMALL LETTER E WITH GRAVE
    {{'e', 0, 0, 0}}, // LATIN SMALL LETTER E WITH ACUTE
    {{'e', 0, 0, 0}}, // LATIN SMALL LETTER E WITH CIRCUMFLEX
    {{'e', 0, 0, 0}}, // LATIN SMALL LETTER E WITH DIAERESIS
    {{'i', 0, 0, 0}}, // LATIN SMALL LETTER I WITH GRAVE
    {{'i', 0, 0, 0}}, // LATIN SMALL LETTER I WITH ACUTE
    {{'i', 0, 0, 0}}, // LATIN SMALL LETTER I WITH CIRCUMFLEX
    {{'i', 0, 0, 0}}, // LATIN SMALL LETTER I WITH DIAERESIS
    {{'T', 'H', 0, 0}}, // LATIN SMALL LETTER ETH (Icelandic)
    {{'n', 0, 0, 0}}, // LATIN SMALL LETTER N WITH TILDE
    {{'o', 0, 0, 0}}, // LATIN SMALL LETTER O WITH GRAVE
    {{'o', 0, 0, 0}}, // LATIN SMALL LETTER O WITH ACUTE
    {{'o', 0, 0, 0}}, // LATIN SMALL LETTER O WITH CIRCUMFLEX
    {{'o', 0, 0, 0}}, // LATIN SMALL LETTER O WITH TILDE
    {{'o', 0, 0, 0}}, // LATIN SMALL LETTER O WITH DIAERESIS
    {{'/', 0, 0, 0}}, // DIVISION SIGN
    {{'o', 0, 0, 0}}, // LATIN SMALL LETTER O WITH STROKE
    {{'u', 0, 0, 0}}, // LATIN SMALL LETTER U WITH GRAVE
    {{'u', 0, 0, 0}}, // LATIN SMALL LETTER U WITH ACUTE
    {{'u', 0, 0, 0}}, // LATIN SMALL LETTER U WITH CIRCUMFLEX
    {{'u', 0, 0, 0}}, // LATIN SMALL LETTER U WITH DIAERESIS
    {{'y', 0, 0, 0}}, // LATIN SMALL LETTER Y WITH ACUTE
    {{'t', 'h', 0, 0}}, // LATIN SMALL LETTER THORN (Icelandic)
    {{'y', 0, 0, 0}}, // LATIN SMALL LETTER Y WITH DIAERESIS
};

RSInline RSIndex __RSToASCIILatin1Fallback(UniChar character, uint8_t *bytes, RSIndex maxByteLen) {
    const uint8_t *losChars = (const uint8_t*)_toLossyASCIITable + (character - 0xA0) * sizeof(uint8_t[4]);
    RSIndex numBytes = 0;
    RSIndex idx, max = (maxByteLen && (maxByteLen < 4) ? maxByteLen : 4);
    
    for (idx = 0;idx < max;idx++) {
        if (losChars[idx]) {
            if (maxByteLen) bytes[idx] = losChars[idx];
            ++numBytes;
        } else {
            break;
        }
    }
    
    return numBytes;
}

static RSIndex __RSDefaultToBytesFallbackProc(const UniChar *characters, RSIndex numChars, uint8_t *bytes, RSIndex maxByteLen, RSIndex *usedByteLen) {
    RSIndex processCharLen = 1, filledBytesLen = 1;
    uint8_t byte = '?';
    
    if (*characters < 0xA0) { // 0x80 to 0x9F maps to ASCII C0 range
        byte = (uint8_t)(*characters - 0x80);
    } else if (*characters < 0x100) {
        *usedByteLen = __RSToASCIILatin1Fallback(*characters, bytes, maxByteLen);
        return 1;
    } else if (*characters >= kSurrogateHighStart && *characters <= kSurrogateLowEnd) {
        processCharLen = (numChars > 1 && *characters <= kSurrogateLowStart && *(characters + 1) >= kSurrogateLowStart && *(characters + 1) <= kSurrogateLowEnd ? 2 : 1);
    } else if (RSUniCharIsMemberOf(*characters, RSUniCharWhitespaceCharacterSet)) {
        byte = ' ';
    } else if (RSUniCharIsMemberOf(*characters, RSUniCharWhitespaceAndNewlineCharacterSet)) {
        byte = ASCIINewLine;
    } else if (*characters == 0x2026) { // ellipsis
        if (0 == maxByteLen) {
            filledBytesLen = 3;
        } else if (maxByteLen > 2) {
            memset(bytes, '.', 3);
            *usedByteLen = 3;
            return processCharLen;
        }
    } else if (RSUniCharIsMemberOf(*characters, RSUniCharDecomposableCharacterSet)) {
        UTF32Char decomposed[MAX_DECOMPOSED_LENGTH];
        
        (void)RSUniCharDecomposeCharacter(*characters, decomposed, MAX_DECOMPOSED_LENGTH);
        if (*decomposed < 0x80) {
            byte = (uint8_t)(*decomposed);
        } else {
            UTF16Char theChar = *decomposed;
            
            return __RSDefaultToBytesFallbackProc(&theChar, 1, bytes, maxByteLen, usedByteLen);
        }
    }
    
    if (maxByteLen) *bytes = byte;
    *usedByteLen = filledBytesLen;
    return processCharLen;
}

static RSIndex __RSDefaultToUnicodeFallbackProc(const uint8_t *bytes, RSIndex numBytes, UniChar *characters, RSIndex maxCharLen, RSIndex *usedCharLen) {
    if (maxCharLen) *characters = (UniChar)'?';
    *usedCharLen = 1;
    return 1;
}

#define TO_BYTE_FALLBACK(conv,chars,numChars,bytes,max,used) (conv->toBytesFallback(chars,numChars,bytes,max,used))
#define TO_UNICODE_FALLBACK(conv,bytes,numBytes,chars,max,used) (conv->toUnicodeFallback(bytes,numBytes,chars,max,used))

#define EXTRA_BASE (0x0F00)

/* Wrapper funcs for non-standard converters
 */
static RSIndex __RSToBytesCheapEightBitWrapper(const void *converter, uint32_t flags, const UniChar *characters, RSIndex numChars, uint8_t *bytes, RSIndex maxByteLen, RSIndex *usedByteLen) {
    RSIndex processedCharLen = 0;
    RSIndex length = (maxByteLen && (maxByteLen < numChars) ? maxByteLen : numChars);
    uint8_t byte;
    
    while (processedCharLen < length) {
        if (!((RSStringEncodingCheapEightBitToBytesProc)((const _RSEncodingConverter*)converter)->definition->toBytes)(flags, characters[processedCharLen], &byte)) break;
        
        if (maxByteLen) bytes[processedCharLen] = byte;
        processedCharLen++;
    }
    
    *usedByteLen = processedCharLen;
    return processedCharLen;
}

static RSIndex __RSToUnicodeCheapEightBitWrapper(const void *converter, uint32_t flags, const uint8_t *bytes, RSIndex numBytes, UniChar *characters, RSIndex maxCharLen, RSIndex *usedCharLen) {
    RSIndex processedByteLen = 0;
    RSIndex length = (maxCharLen && (maxCharLen < numBytes) ? maxCharLen : numBytes);
    UniChar character;
    
    while (processedByteLen < length) {
        if (!((RSStringEncodingCheapEightBitToUnicodeProc)((const _RSEncodingConverter*)converter)->definition->toUnicode)(flags, bytes[processedByteLen], &character)) break;
        
        if (maxCharLen) characters[processedByteLen] = character;
        processedByteLen++;
    }
    
    *usedCharLen = processedByteLen;
    return processedByteLen;
}

static RSIndex __RSToCanonicalUnicodeCheapEightBitWrapper(const void *converter, uint32_t flags, const uint8_t *bytes, RSIndex numBytes, UniChar *characters, RSIndex maxCharLen, RSIndex *usedCharLen) {
    RSIndex processedByteLen = 0;
    RSIndex theUsedCharLen = 0;
    UTF32Char charBuffer[MAX_DECOMPOSED_LENGTH];
    RSIndex usedLen;
    UniChar character;
    BOOL isHFSPlus = (flags & RSStringEncodingUseHFSPlusCanonical ? YES : NO);
    
    while ((processedByteLen < numBytes) && (!maxCharLen || (theUsedCharLen < maxCharLen))) {
        if (!((RSStringEncodingCheapEightBitToUnicodeProc)((const _RSEncodingConverter*)converter)->definition->toUnicode)(flags, bytes[processedByteLen], &character)) break;
        
        if (RSUniCharIsDecomposableCharacter(character, isHFSPlus)) {
            RSIndex idx;
            
            usedLen = RSUniCharDecomposeCharacter(character, charBuffer, MAX_DECOMPOSED_LENGTH);
            *usedCharLen = theUsedCharLen;
            
            for (idx = 0;idx < usedLen;idx++) {
                if (charBuffer[idx] > 0xFFFF) { // Non-BMP
                    if (theUsedCharLen + 2 > maxCharLen)  return processedByteLen;
                    theUsedCharLen += 2;
                    if (maxCharLen) {
                        charBuffer[idx] = charBuffer[idx] - 0x10000;
                        *(characters++) = (UniChar)(charBuffer[idx] >> 10) + 0xD800UL;
                        *(characters++) = (UniChar)(charBuffer[idx] & 0x3FF) + 0xDC00UL;
                    }
                } else {
                    if (theUsedCharLen + 1 > maxCharLen)  return processedByteLen;
                    ++theUsedCharLen;
                    *(characters++) = charBuffer[idx];
                }
            }
        } else {
            if (maxCharLen) *(characters++) = character;
            ++theUsedCharLen;
        }
        processedByteLen++;
    }
    
    *usedCharLen = theUsedCharLen;
    return processedByteLen;
}

static RSIndex __RSToBytesStandardEightBitWrapper(const void *converter, uint32_t flags, const UniChar *characters, RSIndex numChars, uint8_t *bytes, RSIndex maxByteLen, RSIndex *usedByteLen) {
    RSIndex processedCharLen = 0;
    uint8_t byte;
    RSIndex usedLen;
    
    *usedByteLen = 0;
    
    while (numChars && (!maxByteLen || (*usedByteLen < maxByteLen))) {
        if (!(usedLen = ((RSStringEncodingStandardEightBitToBytesProc)((const _RSEncodingConverter*)converter)->definition->toBytes)(flags, characters, numChars, &byte))) break;
        
        if (maxByteLen) bytes[*usedByteLen] = byte;
        (*usedByteLen)++;
        characters += usedLen;
        numChars -= usedLen;
        processedCharLen += usedLen;
    }
    
    return processedCharLen;
}

static RSIndex __RSToUnicodeStandardEightBitWrapper(const void *converter, uint32_t flags, const uint8_t *bytes, RSIndex numBytes, UniChar *characters, RSIndex maxCharLen, RSIndex *usedCharLen) {
    RSIndex processedByteLen = 0;
    UniChar charBuffer[__RSMaximumConvertedLength];
    RSIndex usedLen;
    
    *usedCharLen = 0;
    
    while ((processedByteLen < numBytes) && (!maxCharLen || (*usedCharLen < maxCharLen))) {
        if (!(usedLen = ((RSStringEncodingCheapEightBitToUnicodeProc)((const _RSEncodingConverter*)converter)->definition->toUnicode)(flags, bytes[processedByteLen], charBuffer))) break;
        
        if (maxCharLen) {
            RSIndex idx;
            
            if (*usedCharLen + usedLen > maxCharLen) break;
            
            for (idx = 0;idx < usedLen;idx++) {
                characters[*usedCharLen + idx] = charBuffer[idx];
            }
        }
        *usedCharLen += usedLen;
        processedByteLen++;
    }
    
    return processedByteLen;
}

static RSIndex __RSToCanonicalUnicodeStandardEightBitWrapper(const void *converter, uint32_t flags, const uint8_t *bytes, RSIndex numBytes, UniChar *characters, RSIndex maxCharLen, RSIndex *usedCharLen) {
    RSIndex processedByteLen = 0;
    UniChar charBuffer[__RSMaximumConvertedLength];
    UTF32Char decompBuffer[MAX_DECOMPOSED_LENGTH];
    RSIndex usedLen;
    RSIndex decompedLen;
    RSIndex idx, decompIndex;
    BOOL isHFSPlus = (flags & RSStringEncodingUseHFSPlusCanonical ? YES : NO);
    RSIndex theUsedCharLen = 0;
    
    while ((processedByteLen < numBytes) && (!maxCharLen || (theUsedCharLen < maxCharLen)))
    {
        if (!(usedLen = ((RSStringEncodingCheapEightBitToUnicodeProc)((const _RSEncodingConverter*)converter)->definition->toUnicode)(flags, bytes[processedByteLen], charBuffer))) break;
        
        for (idx = 0;idx < usedLen;idx++)
        {
            if (RSUniCharIsDecomposableCharacter(charBuffer[idx], isHFSPlus))
            {
                decompedLen = RSUniCharDecomposeCharacter(charBuffer[idx], decompBuffer, MAX_DECOMPOSED_LENGTH);
                *usedCharLen = theUsedCharLen;
                
                for (decompIndex = 0;decompIndex < decompedLen;decompIndex++)
                {
                    if (decompBuffer[decompIndex] > 0xFFFF)
                    {
                        // Non-BMP
                        if (theUsedCharLen + 2 > maxCharLen)  return processedByteLen;
                        theUsedCharLen += 2;
                        if (maxCharLen)
                        {
                            charBuffer[idx] = charBuffer[idx] - 0x10000;
                            *(characters++) = (charBuffer[idx] >> 10) + 0xD800UL;
                            *(characters++) = (charBuffer[idx] & 0x3FF) + 0xDC00UL;
                        }
                    }
                    else
                    {
                        if (theUsedCharLen + 1 > maxCharLen)  return processedByteLen;
                        ++theUsedCharLen;
                        *(characters++) = charBuffer[idx];
                    }
                }
            }
            else
            {
                if (maxCharLen) *(characters++) = charBuffer[idx];
                ++theUsedCharLen;
            }
        }
        processedByteLen++;
    }
    
    *usedCharLen = theUsedCharLen;
    return processedByteLen;
}

static RSIndex __RSToBytesCheapMultiByteWrapper(const void *converter, uint32_t flags, const UniChar *characters, RSIndex numChars, uint8_t *bytes, RSIndex maxByteLen, RSIndex *usedByteLen) {
    RSIndex processedCharLen = 0;
    uint8_t byteBuffer[__RSMaximumConvertedLength];
    RSIndex usedLen;
    
    *usedByteLen = 0;
    
    while ((processedCharLen < numChars) && (!maxByteLen || (*usedByteLen < maxByteLen))) {
        if (!(usedLen = ((RSStringEncodingCheapMultiByteToBytesProc)((const _RSEncodingConverter*)converter)->definition->toBytes)(flags, characters[processedCharLen], byteBuffer))) break;
        
        if (maxByteLen) {
            RSIndex idx;
            
            if (*usedByteLen + usedLen > maxByteLen) break;
            
            for (idx = 0;idx <usedLen;idx++) {
                bytes[*usedByteLen + idx] = byteBuffer[idx];
            }
        }
        
        *usedByteLen += usedLen;
        processedCharLen++;
    }
    
    return processedCharLen;
}

static RSIndex __RSToUnicodeCheapMultiByteWrapper(const void *converter, uint32_t flags, const uint8_t *bytes, RSIndex numBytes, UniChar *characters, RSIndex maxCharLen, RSIndex *usedCharLen) {
    RSIndex processedByteLen = 0;
    UniChar character;
    RSIndex usedLen;
    
    *usedCharLen = 0;
    
    while (numBytes && (!maxCharLen || (*usedCharLen < maxCharLen))) {
        if (!(usedLen = ((RSStringEncodingCheapMultiByteToUnicodeProc)((const _RSEncodingConverter*)converter)->definition->toUnicode)(flags, bytes, numBytes, &character))) break;
        
        if (maxCharLen) *(characters++) = character;
        (*usedCharLen)++;
        processedByteLen += usedLen;
        bytes += usedLen;
        numBytes -= usedLen;
    }
    
    return processedByteLen;
}

static RSIndex __RSToCanonicalUnicodeCheapMultiByteWrapper(const void *converter, uint32_t flags, const uint8_t *bytes, RSIndex numBytes, UniChar *characters, RSIndex maxCharLen, RSIndex *usedCharLen) {
    RSIndex processedByteLen = 0;
    UTF32Char charBuffer[MAX_DECOMPOSED_LENGTH];
    UniChar character;
    RSIndex usedLen;
    RSIndex decomposedLen;
    RSIndex theUsedCharLen = 0;
    BOOL isHFSPlus = (flags & RSStringEncodingUseHFSPlusCanonical ? YES : NO);
    
    while (numBytes && (!maxCharLen || (theUsedCharLen < maxCharLen))) {
        if (!(usedLen = ((RSStringEncodingCheapMultiByteToUnicodeProc)((const _RSEncodingConverter*)converter)->definition->toUnicode)(flags, bytes, numBytes, &character))) break;
        
        if (RSUniCharIsDecomposableCharacter(character, isHFSPlus)) {
            RSIndex idx;
            
            decomposedLen = RSUniCharDecomposeCharacter(character, charBuffer, MAX_DECOMPOSED_LENGTH);
            *usedCharLen = theUsedCharLen;
            
            for (idx = 0;idx < decomposedLen;idx++) {
                if (charBuffer[idx] > 0xFFFF) {
                    // Non-BMP
                    if (theUsedCharLen + 2 > maxCharLen)  return processedByteLen;
                    theUsedCharLen += 2;
                    if (maxCharLen) {
                        charBuffer[idx] = charBuffer[idx] - 0x10000;
                        *(characters++) = (UniChar)(charBuffer[idx] >> 10) + 0xD800UL;
                        *(characters++) = (UniChar)(charBuffer[idx] & 0x3FF) + 0xDC00UL;
                    }
                } else {
                    if (theUsedCharLen + 1 > maxCharLen)  return processedByteLen;
                    ++theUsedCharLen;
                    *(characters++) = charBuffer[idx];
                }
            }
        } else {
            if (maxCharLen) *(characters++) = character;
            ++theUsedCharLen;
        }
        
        processedByteLen += usedLen;
        bytes += usedLen;
        numBytes -= usedLen;
    }
    *usedCharLen = theUsedCharLen;
    return processedByteLen;
}

/* static functions
 */
RSInline _RSEncodingConverter *__RSEncodingConverterFromDefinition(const RSStringEncodingConverter *definition, RSStringEncoding encoding) {
#define NUM_OF_ENTRIES_CYCLE (10)
    static uint32_t _currentIndex = 0;
    static uint32_t _allocatedSize = 0;
    static _RSEncodingConverter *_allocatedEntries = nil;
    _RSEncodingConverter *converter;
    
    
    if ((_currentIndex + 1) >= _allocatedSize) {
        _currentIndex = 0;
        _allocatedSize = 0;
        _allocatedEntries = nil;
    }
    if (_allocatedEntries == nil) { // Not allocated yet
        _allocatedEntries = (_RSEncodingConverter *)__RSAutoReleaseISA(RSAllocatorSystemDefault, RSAllocatorAllocate(RSAllocatorSystemDefault, sizeof(_RSEncodingConverter) * NUM_OF_ENTRIES_CYCLE));
        
        _allocatedSize = NUM_OF_ENTRIES_CYCLE;
        converter = &(_allocatedEntries[_currentIndex]);
    } else {
        converter = &(_allocatedEntries[++_currentIndex]);
    }
    
    memset(converter, 0, sizeof(_RSEncodingConverter));
    
    converter->definition = definition;
    
    switch (definition->encodingClass) {
        case RSStringEncodingConverterStandard:
            converter->toBytes = nil;
            converter->toUnicode = nil;
            converter->toCanonicalUnicode = nil;
            break;
            
        case RSStringEncodingConverterCheapEightBit:
            converter->toBytes = __RSToBytesCheapEightBitWrapper;
            converter->toUnicode = __RSToUnicodeCheapEightBitWrapper;
            converter->toCanonicalUnicode = __RSToCanonicalUnicodeCheapEightBitWrapper;
            break;
            
        case RSStringEncodingConverterStandardEightBit:
            converter->toBytes = __RSToBytesStandardEightBitWrapper;
            converter->toUnicode = __RSToUnicodeStandardEightBitWrapper;
            converter->toCanonicalUnicode = __RSToCanonicalUnicodeStandardEightBitWrapper;
            break;
            
        case RSStringEncodingConverterCheapMultiByte:
            converter->toBytes = __RSToBytesCheapMultiByteWrapper;
            converter->toUnicode = __RSToUnicodeCheapMultiByteWrapper;
            converter->toCanonicalUnicode = __RSToCanonicalUnicodeCheapMultiByteWrapper;
            break;
            
#if DEPLOYMENT_TARGET_MACOSX || DEPLOYMENT_TARGET_EMBEDDED || DEPLOYMENT_TARGET_WINDOWS || DEPLOYMENT_TARGET_LINUX
        case RSStringEncodingConverterICU:
            converter->toBytes = (_RSToBytesProc)__RSStringEncodingGetICUName(encoding);
            break;
#endif
            
        case RSStringEncodingConverterPlatformSpecific:
            break;
            
        default: // Shouln't be here
            return nil;
    }
    
    converter->toBytesFallback = (definition->toBytesFallback ? definition->toBytesFallback : __RSDefaultToBytesFallbackProc);
    converter->toUnicodeFallback = (definition->toUnicodeFallback ? definition->toUnicodeFallback : __RSDefaultToUnicodeFallbackProc);
    
    return converter;
}

RSInline const RSStringEncodingConverter *__RSStringEncodingConverterGetDefinition(RSStringEncoding encoding) {
    switch (encoding) {
        case RSStringEncodingUTF8:
            return &__RSConverterUTF8;
            
        case RSStringEncodingMacRoman:
            return &__RSConverterMacRoman;
            
        case RSStringEncodingWindowsLatin1:
            return &__RSConverterWinLatin1;
            
        case RSStringEncodingASCII:
            return &__RSConverterASCII;
            
        case RSStringEncodingISOLatin1:
            return &__RSConverterISOLatin1;
            
            
        case RSStringEncodingNextStepLatin:
            return &__RSConverterNextStepLatin;
            
            
        default:
            return __RSStringEncodingGetExternalConverter(encoding);
    }
}

static RSMutableDictionaryRef mappingTable = nil;
static _RSEncodingConverter *mappingTableCommonConverters[3] = {nil, nil, nil}; // UTF8, MacRoman/WinLatin1, and the default encoding*
static RSSpinLock mappingTableGetConverterLock = RSSpinLockInit;

static void __RSEncodingMappingTableReleaseRoutine(RSNotificationRef notification)
{
    RSSpinLockLock(&mappingTableGetConverterLock);
    if (mappingTable)
        RSRelease(mappingTable);
    RSSpinLockUnlock(&mappingTableGetConverterLock);
}

static void __RSEncodingMappingTableRegister(RSDictionaryRef dict)
{
    RSNotificationCenterAddObserver(RSNotificationCenterGetDefault(), RSAutorelease(RSObserverCreate(RSAllocatorSystemDefault, RSCoreFoundationWillDeallocateNotification, __RSEncodingMappingTableReleaseRoutine, nil)));
}

static const _RSEncodingConverter *__RSGetConverter(uint32_t encoding)
{
    const _RSEncodingConverter *converter = nil;
    const _RSEncodingConverter **commonConverterSlot = nil;
    
    switch (encoding)
    {
        case RSStringEncodingUTF8: commonConverterSlot = (const _RSEncodingConverter **)&(mappingTableCommonConverters[0]); break;
            
            /* the swith here should avoid possible bootstrap issues in the default: case below when invoked from RSStringGetSystemEncoding() */
#if DEPLOYMENT_TARGET_MACOSX || DEPLOYMENT_TARGET_EMBEDDED || DEPLOYMENT_TARGET_EMBEDDED_MINI || DEPLOYMENT_TARGET_LINUX
        case RSStringEncodingMacRoman: commonConverterSlot = (const _RSEncodingConverter **)&(mappingTableCommonConverters[1]); break;
#elif DEPLOYMENT_TARGET_WINDOWS
        case RSStringEncodingWindowsLatin1: commonConverterSlot = (const _RSEncodingConverter **)(&(commonConverters[1])); break;
#else
#warning This case must match __defaultEncoding value defined in RSString.c
        case RSStringEncodingISOLatin1: commonConverterSlot = (const _RSEncodingConverter **)(&(commonConverters[1])); break;
#endif
            
        default: if (RSStringGetSystemEncoding() == encoding) commonConverterSlot = (const _RSEncodingConverter **)&(mappingTableCommonConverters[2]); break;
    }
    
    RSSpinLockLock(&mappingTableGetConverterLock);
    converter = ((nil == commonConverterSlot) ? ((nil == mappingTable) ? nil : (const _RSEncodingConverter *)RSDictionaryGetValue(mappingTable, (const void *)(uintptr_t)encoding)) : *commonConverterSlot);
    RSSpinLockUnlock(&mappingTableGetConverterLock);
    
    if (nil == converter)
    {
        const RSStringEncodingConverter *definition = __RSStringEncodingConverterGetDefinition(encoding);
        
        if (nil != definition)
        {
            RSSpinLockLock(&mappingTableGetConverterLock);
            converter = ((nil == commonConverterSlot) ? ((nil == mappingTable) ? nil : (const _RSEncodingConverter *)RSDictionaryGetValue(mappingTable, (const void *)(uintptr_t)encoding)) : *commonConverterSlot);
            
            if (nil == converter)
            {
                converter = __RSEncodingConverterFromDefinition(definition, encoding);
                
                if (nil == commonConverterSlot)
                {
                    if (nil == mappingTable)
                    {
                        mappingTable = RSDictionaryCreateMutable(nil, 0, nil);
//                        RSAutorelease(mappingTable);
                        __RSEncodingMappingTableRegister(mappingTable);
                    }
                    
                    RSDictionarySetValue(mappingTable, (const void *)(uintptr_t)encoding, converter);
                }
                else
                {
                    *commonConverterSlot = converter;
                }
            }
            RSSpinLockUnlock(&mappingTableGetConverterLock);
        }
    }
    
    return converter;
}

/* Public API
 */
uint32_t RSStringEncodingUnicodeToBytes(uint32_t encoding, uint32_t flags, const UniChar *characters, RSIndex numChars, RSIndex *usedCharLen, uint8_t *bytes, RSIndex maxByteLen, RSIndex *usedByteLen) {
    if (encoding == RSStringEncodingUTF8) {
        static RSStringEncodingToBytesProc __RSToUTF8 = nil;
        RSIndex convertedCharLen;
        RSIndex usedLen;
        
        if ((flags & RSStringEncodingUseCanonical) || (flags & RSStringEncodingUseHFSPlusCanonical)) {
            (void)RSUniCharDecompose(characters, numChars, &convertedCharLen, (void *)bytes, maxByteLen, &usedLen, YES, RSUniCharUTF8Format, (flags & RSStringEncodingUseHFSPlusCanonical ? YES : NO));
        } else {
            if (!__RSToUTF8) {
                const RSStringEncodingConverter *utf8Converter = RSStringEncodingGetConverter(RSStringEncodingUTF8);
                __RSToUTF8 = (RSStringEncodingToBytesProc)utf8Converter->toBytes;
            }
            convertedCharLen = __RSToUTF8(0, characters, numChars, bytes, maxByteLen, &usedLen);
        }
        if (usedCharLen) *usedCharLen = convertedCharLen;
        if (usedByteLen) *usedByteLen = usedLen;
        
        if (convertedCharLen == numChars) {
            return RSStringEncodingConversionSuccess;
        } else if ((maxByteLen > 0) && ((maxByteLen - usedLen) < 10)) { // could be filled outbuf
            UTF16Char character = characters[convertedCharLen];
            
            if (((character >= kSurrogateLowStart) && (character <= kSurrogateLowEnd)) || ((character >= kSurrogateHighStart) && (character <= kSurrogateHighEnd) && ((1 == (numChars - convertedCharLen)) || (characters[convertedCharLen + 1] < kSurrogateLowStart) || (characters[convertedCharLen + 1] > kSurrogateLowEnd)))) return RSStringEncodingInvalidInputStream;
            
            return RSStringEncodingInsufficientOutputBufferLength;
        } else {
            return RSStringEncodingInvalidInputStream;
        }
    } else {
        const _RSEncodingConverter *converter = __RSGetConverter(encoding);
        RSIndex usedLen = 0;
        RSIndex localUsedByteLen;
        RSIndex theUsedByteLen = 0;
        uint32_t theResult = RSStringEncodingConversionSuccess;
        RSStringEncodingToBytesPrecomposeProc toBytesPrecompose = nil;
        RSStringEncodingIsValidCombiningCharacterProc isValidCombiningChar = nil;
        
        if (!converter) return RSStringEncodingConverterUnavailable;
        
        if (flags & RSStringEncodingSubstituteCombinings) {
            if (!(flags & RSStringEncodingAllowLossyConversion)) isValidCombiningChar = converter->definition->isValidCombiningChar;
        } else {
            isValidCombiningChar = converter->definition->isValidCombiningChar;
            if (!(flags & RSStringEncodingIgnoreCombinings)) {
                toBytesPrecompose = converter->definition->toBytesPrecompose;
                flags |= RSStringEncodingComposeCombinings;
            }
        }
        
#if DEPLOYMENT_TARGET_MACOSX || DEPLOYMENT_TARGET_EMBEDDED || DEPLOYMENT_TARGET_WINDOWS || DEPLOYMENT_TARGET_LINUX
        if (RSStringEncodingConverterICU == converter->definition->encodingClass) return (uint32_t)__RSStringEncodingICUToBytes((const char *)converter->toBytes, flags, characters, numChars, usedCharLen, bytes, maxByteLen, usedByteLen);
#endif
        
        /* Platform converter */
        if (RSStringEncodingConverterPlatformSpecific == converter->definition->encodingClass) return (uint32_t)__RSStringEncodingPlatformUnicodeToBytes(encoding, flags, characters, numChars, usedCharLen, bytes, maxByteLen, usedByteLen);
        
        while ((usedLen < numChars) && (!maxByteLen || (theUsedByteLen < maxByteLen))) {
            if ((usedLen += TO_BYTE(converter, flags, characters + usedLen, numChars - usedLen, bytes + theUsedByteLen, (maxByteLen ? maxByteLen - theUsedByteLen : 0), &localUsedByteLen)) < numChars) {
                RSIndex dummy;
                
                if (isValidCombiningChar && (usedLen > 0) && isValidCombiningChar(characters[usedLen])) {
                    if (toBytesPrecompose) {
                        RSIndex localUsedLen = usedLen;
                        
                        while (isValidCombiningChar(characters[--usedLen]));
                        theUsedByteLen += localUsedByteLen;
                        if (converter->definition->maxBytesPerChar > 1) {
                            TO_BYTE(converter, flags, characters + usedLen, localUsedLen - usedLen, nil, 0, &localUsedByteLen);
                            theUsedByteLen -= localUsedByteLen;
                        } else {
                            theUsedByteLen--;
                        }
                        if ((localUsedLen = toBytesPrecompose(flags, characters + usedLen, numChars - usedLen, bytes + theUsedByteLen, (maxByteLen ? maxByteLen - theUsedByteLen : 0), &localUsedByteLen)) > 0) {
                            usedLen += localUsedLen;
                            if ((usedLen < numChars) && isValidCombiningChar(characters[usedLen])) { // There is a non-base char not combined remaining
                                theUsedByteLen += localUsedByteLen;
                                theResult = RSStringEncodingInvalidInputStream;
                                break;
                            }
                        } else if (flags & RSStringEncodingAllowLossyConversion) {
                            uint8_t lossyByte = RSStringEncodingMaskToLossyByte(flags);
                            
                            if (lossyByte) {
                                while (isValidCombiningChar(characters[++usedLen]));
                                localUsedByteLen = 1;
                                if (maxByteLen) *(bytes + theUsedByteLen) = lossyByte;
                            } else {
                                ++usedLen;
                                usedLen += TO_BYTE_FALLBACK(converter, characters + usedLen, numChars - usedLen, bytes + theUsedByteLen, (maxByteLen ? maxByteLen - theUsedByteLen : 0), &localUsedByteLen);
                            }
                        } else {
                            theResult = RSStringEncodingInvalidInputStream;
                            break;
                        }
                    } else if (maxByteLen && ((maxByteLen == theUsedByteLen + localUsedByteLen) || TO_BYTE(converter, flags, characters + usedLen, numChars - usedLen, nil, 0, &dummy))) { // buffer was filled up
                        theUsedByteLen += localUsedByteLen;
                        theResult = RSStringEncodingInsufficientOutputBufferLength;
                        break;
                    } else if (flags & RSStringEncodingIgnoreCombinings) {
                        while ((++usedLen < numChars) && isValidCombiningChar(characters[usedLen]));
                    } else {
                        uint8_t lossyByte = RSStringEncodingMaskToLossyByte(flags);
                        
                        theUsedByteLen += localUsedByteLen;
                        if (lossyByte) {
                            ++usedLen;
                            localUsedByteLen = 1;
                            if (maxByteLen) *(bytes + theUsedByteLen) = lossyByte;
                        } else {
                            usedLen += TO_BYTE_FALLBACK(converter, characters + usedLen, numChars - usedLen, bytes + theUsedByteLen, (maxByteLen ? maxByteLen - theUsedByteLen : 0), &localUsedByteLen);
                        }
                    }
                } else if (maxByteLen && ((maxByteLen == theUsedByteLen + localUsedByteLen) || TO_BYTE(converter, flags, characters + usedLen, numChars - usedLen, nil, 0, &dummy))) { // buffer was filled up
                    theUsedByteLen += localUsedByteLen;
                    
                    if (flags & RSStringEncodingAllowLossyConversion && !RSStringEncodingMaskToLossyByte(flags)) {
                        RSIndex localUsedLen;
                        
                        localUsedByteLen = 0;
                        while ((usedLen < numChars) && !localUsedByteLen && (localUsedLen = TO_BYTE_FALLBACK(converter, characters + usedLen, numChars - usedLen, nil, 0, &localUsedByteLen))) usedLen += localUsedLen;
                    }
                    if (usedLen < numChars) theResult = RSStringEncodingInsufficientOutputBufferLength;
                    break;
                } else if (flags & RSStringEncodingAllowLossyConversion) {
                    uint8_t lossyByte = RSStringEncodingMaskToLossyByte(flags);
                    
                    theUsedByteLen += localUsedByteLen;
                    if (lossyByte) {
                        ++usedLen;
                        localUsedByteLen = 1;
                        if (maxByteLen) *(bytes + theUsedByteLen) = lossyByte;
                    } else {
                        usedLen += TO_BYTE_FALLBACK(converter, characters + usedLen, numChars - usedLen, bytes + theUsedByteLen, (maxByteLen ? maxByteLen - theUsedByteLen : 0), &localUsedByteLen);
                    }
                } else {
                    theUsedByteLen += localUsedByteLen;
                    theResult = RSStringEncodingInvalidInputStream;
                    break;
                }
            }
            theUsedByteLen += localUsedByteLen;
        }
        
        if (usedLen < numChars && maxByteLen && theResult == RSStringEncodingConversionSuccess) {
            if (flags & RSStringEncodingAllowLossyConversion && !RSStringEncodingMaskToLossyByte(flags)) {
                RSIndex localUsedLen;
                
                localUsedByteLen = 0;
                while ((usedLen < numChars) && !localUsedByteLen && (localUsedLen = TO_BYTE_FALLBACK(converter, characters + usedLen, numChars - usedLen, nil, 0, &localUsedByteLen))) usedLen += localUsedLen;
            }
            if (usedLen < numChars) theResult = RSStringEncodingInsufficientOutputBufferLength;
        }
        if (usedByteLen) *usedByteLen = theUsedByteLen;
        if (usedCharLen) *usedCharLen = usedLen;
        
        return theResult;
    }
}

uint32_t RSStringEncodingBytesToUnicode(uint32_t encoding, uint32_t flags, const uint8_t *bytes, RSIndex numBytes, RSIndex *usedByteLen, UniChar *characters, RSIndex maxCharLen, RSIndex *usedCharLen) {
    const _RSEncodingConverter *converter = __RSGetConverter(encoding);
    RSIndex usedLen = 0;
    RSIndex theUsedCharLen = 0;
    RSIndex localUsedCharLen;
    uint32_t theResult = RSStringEncodingConversionSuccess;
    
    if (!converter) return RSStringEncodingConverterUnavailable;
    
#if DEPLOYMENT_TARGET_MACOSX || DEPLOYMENT_TARGET_EMBEDDED || DEPLOYMENT_TARGET_WINDOWS || DEPLOYMENT_TARGET_LINUX
    if (RSStringEncodingConverterICU == converter->definition->encodingClass) return (uint32_t)__RSStringEncodingICUToUnicode((const char *)converter->toBytes, flags, bytes, numBytes, usedByteLen, characters, maxCharLen, usedCharLen);
#endif
    
    /* Platform converter */
    if (RSStringEncodingConverterPlatformSpecific == converter->definition->encodingClass) return (uint32_t)__RSStringEncodingPlatformBytesToUnicode(encoding, flags, bytes, numBytes, usedByteLen, characters, maxCharLen, usedCharLen);
    
    while ((usedLen < numBytes) && (!maxCharLen || (theUsedCharLen < maxCharLen))) {
        if ((usedLen += TO_UNICODE(converter, flags, bytes + usedLen, numBytes - usedLen, characters + theUsedCharLen, (maxCharLen ? maxCharLen - theUsedCharLen : 0), &localUsedCharLen)) < numBytes) {
            RSIndex tempUsedCharLen;
            
            if (maxCharLen && ((maxCharLen == theUsedCharLen + localUsedCharLen) || (((flags & (RSStringEncodingUseCanonical|RSStringEncodingUseHFSPlusCanonical)) || (maxCharLen == theUsedCharLen + localUsedCharLen + 1)) && TO_UNICODE(converter, flags, bytes + usedLen, numBytes - usedLen, nil, 0, &tempUsedCharLen)))) { // buffer was filled up
                theUsedCharLen += localUsedCharLen;
                theResult = RSStringEncodingInsufficientOutputBufferLength;
                break;
            } else if (flags & RSStringEncodingAllowLossyConversion) {
                theUsedCharLen += localUsedCharLen;
                usedLen += TO_UNICODE_FALLBACK(converter, bytes + usedLen, numBytes - usedLen, characters + theUsedCharLen, (maxCharLen ? maxCharLen - theUsedCharLen : 0), &localUsedCharLen);
            } else {
                theUsedCharLen += localUsedCharLen;
                theResult = RSStringEncodingInvalidInputStream;
                break;
            }
        }
        theUsedCharLen += localUsedCharLen;
    }
    
    if (usedLen < numBytes && maxCharLen && theResult == RSStringEncodingConversionSuccess) {
        theResult = RSStringEncodingInsufficientOutputBufferLength;
    }
    if (usedCharLen) *usedCharLen = theUsedCharLen;
    if (usedByteLen) *usedByteLen = usedLen;
    
    return theResult;
}

RSPrivate BOOL RSStringEncodingIsValidEncoding(uint32_t encoding) {
    return (RSStringEncodingGetConverter(encoding) ? YES : NO);
}

RSPrivate RSIndex RSStringEncodingCharLengthForBytes(uint32_t encoding, uint32_t flags, const uint8_t *bytes, RSIndex numBytes) {
    const _RSEncodingConverter *converter = __RSGetConverter(encoding);
    
    if (converter) {
#if DEPLOYMENT_TARGET_MACOSX || DEPLOYMENT_TARGET_EMBEDDED || DEPLOYMENT_TARGET_WINDOWS || DEPLOYMENT_TARGET_LINUX
        if (RSStringEncodingConverterICU == converter->definition->encodingClass) return __RSStringEncodingICUCharLength((const char *)converter->toBytes, flags, bytes, numBytes);
#endif
        
        if (RSStringEncodingConverterPlatformSpecific == converter->definition->encodingClass) return __RSStringEncodingPlatformCharLengthForBytes(encoding, flags, bytes, numBytes);
        
        if (1 == converter->definition->maxBytesPerChar) return numBytes;
        
        if (nil == converter->definition->toUnicodeLen) {
            RSIndex usedByteLen = 0;
            RSIndex totalLength = 0;
            RSIndex usedCharLen;
            
            while (numBytes > 0) {
                usedByteLen = TO_UNICODE(converter, flags, bytes, numBytes, nil, 0, &usedCharLen);
                
                bytes += usedByteLen;
                numBytes -= usedByteLen;
                totalLength += usedCharLen;
                
                if (numBytes > 0) {
                    if (0 == (flags & RSStringEncodingAllowLossyConversion)) return 0;
                    
                    usedByteLen = TO_UNICODE_FALLBACK(converter, bytes, numBytes, nil, 0, &usedCharLen);
                    
                    bytes += usedByteLen;
                    numBytes -= usedByteLen;
                    totalLength += usedCharLen;
                }
            }
            
            return totalLength;
        } else {
            return converter->definition->toUnicodeLen(flags, bytes, numBytes);
        }
    }
    
    return 0;
}

RSPrivate RSIndex RSStringEncodingByteLengthForCharacters(uint32_t encoding, uint32_t flags, const UniChar *characters, RSIndex numChars)
{
    const _RSEncodingConverter *converter = __RSGetConverter(encoding);
    
    if (converter) {
#if DEPLOYMENT_TARGET_MACOSX || DEPLOYMENT_TARGET_EMBEDDED || DEPLOYMENT_TARGET_WINDOWS || DEPLOYMENT_TARGET_LINUX
        if (RSStringEncodingConverterICU == converter->definition->encodingClass) return __RSStringEncodingICUByteLength((const char *)converter->toBytes, flags, characters, numChars);
#endif
        
        if (RSStringEncodingConverterPlatformSpecific == converter->definition->encodingClass) return __RSStringEncodingPlatformByteLengthForCharacters(encoding, flags, characters, numChars);
        
        if (1 == converter->definition->maxBytesPerChar) return numChars;
        
        if (nil == converter->definition->toBytesLen) {
            RSIndex usedByteLen;
            
            return ((RSStringEncodingConversionSuccess == RSStringEncodingUnicodeToBytes(encoding, flags, characters, numChars, nil, nil, 0, &usedByteLen)) ? usedByteLen : 0);
        } else {
            return converter->definition->toBytesLen(flags, characters, numChars);
        }
    }
    
    return 0;
}

RSPrivate void RSStringEncodingRegisterFallbackProcedures(uint32_t encoding, RSStringEncodingToBytesFallbackProc toBytes, RSStringEncodingToUnicodeFallbackProc toUnicode) {
    _RSEncodingConverter *converter = (_RSEncodingConverter *)__RSGetConverter(encoding);
    
    if (nil != converter) {
        const RSStringEncodingConverter *body = RSStringEncodingGetConverter(encoding);
        
        converter->toBytesFallback = ((nil == toBytes) ? ((nil == body) ? __RSDefaultToBytesFallbackProc : body->toBytesFallback) : toBytes);
        converter->toUnicodeFallback = ((nil == toUnicode) ? ((nil == body) ? __RSDefaultToUnicodeFallbackProc : body->toUnicodeFallback) : toUnicode);
    }
}

RSPrivate const RSStringEncodingConverter *RSStringEncodingGetConverter(uint32_t encoding) {
    const _RSEncodingConverter *converter = __RSGetConverter(encoding);
    
    return ((nil == converter) ? nil : converter->definition);
}

static const RSStringEncoding __RSBuiltinEncodings[] = {
    RSStringEncodingMacRoman,
    RSStringEncodingWindowsLatin1,
    RSStringEncodingISOLatin1,
    RSStringEncodingNextStepLatin,
    RSStringEncodingASCII,
    RSStringEncodingUTF8,
    /* These seven are available only in RSString-level */
    RSStringEncodingNonLossyASCII,
    
    RSStringEncodingUTF16,
    RSStringEncodingUTF16BE,
    RSStringEncodingUTF16LE,
    
    RSStringEncodingUTF32,
    RSStringEncodingUTF32BE,
    RSStringEncodingUTF32LE,
    
    RSStringEncodingInvalidId,
};

static RSComparisonResult __RSStringEncodingComparator(const void *v1, const void *v2, void *context) {
    RSComparisonResult val1 = (*(const RSStringEncoding *)v1) & 0xFFFF;
    RSComparisonResult val2 = (*(const RSStringEncoding *)v2) & 0xFFFF;
    
    return ((val1 == val2) ? ((RSComparisonResult)(*(const RSStringEncoding *)v1) - (RSComparisonResult)(*(const RSStringEncoding *)v2)) : val1 - val2);
}

static void __RSStringEncodingFliterDupes(RSStringEncoding *encodings, RSIndex numSlots) {
    RSStringEncoding last = RSStringEncodingInvalidId;
    const RSStringEncoding *limitEncodings = encodings + numSlots;
    
    while (encodings < limitEncodings) {
        if (last == *encodings) {
            if ((encodings + 1) < limitEncodings) memmove(encodings, encodings + 1, sizeof(RSStringEncoding) * (limitEncodings - encodings - 1));
            --limitEncodings;
        } else {
            last = *(encodings++);
        }
    }
}

RSPrivate const RSStringEncoding *RSStringEncodingListOfAvailableEncodings(void) {
    static const RSStringEncoding *encodings = nil;
    
    if (nil == encodings) {
        RSStringEncoding *list = (RSStringEncoding *)__RSBuiltinEncodings;
        RSIndex numICUConverters = 0, numPlatformConverters = 0;
#if DEPLOYMENT_TARGET_MACOSX || DEPLOYMENT_TARGET_EMBEDDED || DEPLOYMENT_TARGET_WINDOWS || DEPLOYMENT_TARGET_LINUX
        RSStringEncoding *icuConverters = __RSStringEncodingCreateICUEncodings(nil, &numICUConverters);
#else
        RSStringEncoding *icuConverters = nil;
#endif
        RSStringEncoding *platformConverters = __RSStringEncodingCreateListOfAvailablePlatformConverters(nil, &numPlatformConverters);
        
        if ((nil != icuConverters) || (nil != platformConverters)) {
            RSIndex numSlots = (sizeof(__RSBuiltinEncodings) / sizeof(*__RSBuiltinEncodings)) + numICUConverters + numPlatformConverters;
            
            list = (RSStringEncoding *)RSAllocatorAllocate(nil, sizeof(RSStringEncoding) * numSlots);
            
            memcpy(list, __RSBuiltinEncodings, sizeof(__RSBuiltinEncodings));
            
            if (nil != icuConverters) {
                memcpy(list + (sizeof(__RSBuiltinEncodings) / sizeof(*__RSBuiltinEncodings)), icuConverters, sizeof(RSStringEncoding) * numICUConverters);
                RSAllocatorDeallocate(nil, icuConverters);
            }
            
            if (nil != platformConverters) {
                memcpy(list + (sizeof(__RSBuiltinEncodings) / sizeof(*__RSBuiltinEncodings)) + numICUConverters, platformConverters, sizeof(RSStringEncoding) * numPlatformConverters);
                RSAllocatorDeallocate(nil, platformConverters);
            }
//            extern void RSQSortArray(void **list, RSIndex count, RSIndex elementSize, RSComparatorFunction comparator, BOOL ascending);
            RSSortArray((void **)&list, numSlots, sizeof(RSStringEncoding), RSOrderedDescending, (RSComparatorFunction)__RSStringEncodingComparator, nil);

            __RSStringEncodingFliterDupes(list, numSlots);
        }
//        bool __atomic_compare_exchange (type *ptr, type *expected, type *desired, bool weak, int success_memmodel, int failure_memmodel);
        
        
//        ( void *__oldValue, void *__newValue, void * volatile *__theValue )
        
        if (!OSAtomicCompareAndSwapPtrBarrier(nil, list, (void * volatile *)&encodings) && (list != __RSBuiltinEncodings)) RSAllocatorDeallocate(nil, list);
        __RSAutoReleaseISA(nil, (ISA)encodings);
    }
    
    return encodings;
}

#undef TO_BYTE
#undef TO_UNICODE
#undef ASCIINewLine
#undef kSurrogateHighStart
#undef kSurrogateHighEnd
#undef kSurrogateLowStart
#undef kSurrogateLowEnd
#undef TO_BYTE_FALLBACK
#undef TO_UNICODE_FALLBACK
#undef EXTRA_BASE
#undef NUM_OF_ENTRIES_CYCLE

/*
 * Copyright (c) 2012 Apple Inc. All rights reserved.
 *
 * @APPLE_LICENSE_HEADER_START@
 *
 * This file contains Original Code and/or Modifications of Original Code
 * as defined in and that are subject to the Apple Public Source License
 * Version 2.0 (the 'License'). You may not use this file except in
 * compliance with the License. Please obtain a copy of the License at
 * http://www.opensource.apple.com/apsl/ and read it before using this
 * file.
 *
 * The Original Code and all software distributed under the License are
 * distributed on an 'AS IS' basis, WITHOUT WARRANTY OF ANY KIND, EITHER
 * EXPRESS OR IMPLIED, AND APPLE HEREBY DISCLAIMS ALL SUCH WARRANTIES,
 * INCLUDING WITHOUT LIMITATION, ANY WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE, QUIET ENJOYMENT OR NON-INFRINGEMENT.
 * Please see the License for the specific language governing rights and
 * limitations under the License.
 *
 * @APPLE_LICENSE_HEADER_END@
 */

/*	RSPlatformConverters.c
 Copyright (c) 1998-2012, Apple Inc. All rights reserved.
 Responsibility: Aki Inoue
 */

#include "RSInternal.h"
#include <RSCoreFoundation/RSString.h>
#include "RSStringEncodingConverterExt.h"
#include "RSUniChar.h"
#include "RSUnicodeDecomposition.h"
#include "RSStringEncodingConverterPrivate.h"
#include "RSICUConverters.h"


RSInline BOOL __RSIsPlatformConverterAvailable(int encoding) {
    
#if DEPLOYMENT_TARGET_WINDOWS
    return (IsValidCodePage(RSStringConvertEncodingToWindowsCodepage(encoding)) ? YES : NO);
#else
    return NO;
#endif
}

static const RSStringEncodingConverter __RSICUBootstrap = {
    nil /* toBytes */, nil /* toUnicode */, 6 /* maxBytesPerChar */, 4 /* maxDecomposedCharLen */,
    RSStringEncodingConverterICU /* encodingClass */,
    nil /* toBytesLen */, nil /* toUnicodeLen */, nil /* toBytesFallback */,
    nil /* toUnicodeFallback */, nil /* toBytesPrecompose */, nil, /* isValidCombiningChar */
};

static const RSStringEncodingConverter __RSPlatformBootstrap = {
    nil /* toBytes */, nil /* toUnicode */, 6 /* maxBytesPerChar */, 4 /* maxDecomposedCharLen */,
    RSStringEncodingConverterPlatformSpecific /* encodingClass */,
    nil /* toBytesLen */, nil /* toUnicodeLen */, nil /* toBytesFallback */,
    nil /* toUnicodeFallback */, nil /* toBytesPrecompose */, nil, /* isValidCombiningChar */
};

RSPrivate const RSStringEncodingConverter *__RSStringEncodingGetExternalConverter(uint32_t encoding) {
    
    // we prefer Text Encoding Converter ICU since it's more reliable
    if (__RSIsPlatformConverterAvailable(encoding)) {
        return &__RSPlatformBootstrap;
    } else {
#if DEPLOYMENT_TARGET_MACOSX || DEPLOYMENT_TARGET_EMBEDDED || DEPLOYMENT_TARGET_WINDOWS || DEPLOYMENT_TARGET_LINUX
        if (__RSStringEncodingGetICUName(encoding)) {
            return &__RSICUBootstrap;
        }
#endif
        return nil;
    }
}

#if DEPLOYMENT_TARGET_MACOSX || DEPLOYMENT_TARGET_EMBEDDED
RSPrivate RSStringEncoding *__RSStringEncodingCreateListOfAvailablePlatformConverters(RSAllocatorRef allocator, RSIndex *numberOfConverters) {
    
    return nil;
}
#elif DEPLOYMENT_TARGET_WINDOWS

#include <tchar.h>

static uint32_t __RSWin32EncodingIndex = 0;
static RSStringEncoding *__RSWin32EncodingList = nil;

static char CALLBACK __RSWin32EnumCodePageProc(LPTSTR string) {
    uint32_t encoding = RSStringConvertWindowsCodepageToEncoding(_tcstoul(string, nil, 10));
    RSIndex idx;
    
    if (encoding != RSStringEncodingInvalidId) { // We list only encodings we know
        if (__RSWin32EncodingList) {
            for (idx = 0;idx < (RSIndex)__RSWin32EncodingIndex;idx++) if (__RSWin32EncodingList[idx] == encoding) break;
            if (idx != __RSWin32EncodingIndex) return YES;
            __RSWin32EncodingList[__RSWin32EncodingIndex] = encoding;
        }
        ++__RSWin32EncodingIndex;
    }
    return YES;
}

RSPrivate RSStringEncoding *__RSStringEncodingCreateListOfAvailablePlatformConverters(RSAllocatorRef allocator, RSIndex *numberOfConverters) {
    RSStringEncoding *encodings;
    
    EnumSystemCodePages((CODEPAGE_ENUMPROC)&__RSWin32EnumCodePageProc, CP_INSTALLED);
    __RSWin32EncodingList = (uint32_t *)RSAllocatorAllocate(allocator, sizeof(uint32_t) * __RSWin32EncodingIndex, 0);
    EnumSystemCodePages((CODEPAGE_ENUMPROC)&__RSWin32EnumCodePageProc, CP_INSTALLED);
    
    *numberOfConverters = __RSWin32EncodingIndex;
    encodings = __RSWin32EncodingList;
    
    __RSWin32EncodingIndex = 0;
    __RSWin32EncodingList = nil;
    
    return encodings;
}
#else
RSPrivate RSStringEncoding *__RSStringEncodingCreateListOfAvailablePlatformConverters(RSAllocatorRef allocator, RSIndex *numberOfConverters) { return nil; }
#endif

RSPrivate RSIndex __RSStringEncodingPlatformUnicodeToBytes(uint32_t encoding, uint32_t flags, const UniChar *characters, RSIndex numChars, RSIndex *usedCharLen, uint8_t *bytes, RSIndex maxByteLen, RSIndex *usedByteLen) {
    
#if DEPLOYMENT_TARGET_WINDOWS
    WORD dwFlags = 0;
    RSIndex usedLen;
    
    if ((RSStringEncodingUTF7 != encoding) && (RSStringEncodingGB_18030_2000 != encoding) && (0x0800 != (encoding & 0x0F00))) { // not UTF-7/GB18030/ISO-2022-*
        dwFlags |= (flags & (RSStringEncodingAllowLossyConversion|RSStringEncodingSubstituteCombinings) ? WC_DEFAULTCHAR : 0);
        dwFlags |= (flags & RSStringEncodingComposeCombinings ? WC_COMPOSITECHECK : 0);
        dwFlags |= (flags & RSStringEncodingIgnoreCombinings ? WC_DISCARDNS : 0);
    }
    
    if ((usedLen = WideCharToMultiByte(RSStringConvertEncodingToWindowsCodepage(encoding), dwFlags, (LPCWSTR)characters, numChars, (LPSTR)bytes, maxByteLen, nil, nil)) == 0) {
        if (GetLastError() == ERROR_INSUFFICIENT_BUFFER) {
            CPINFO cpInfo;
            
            if (!GetCPInfo(RSStringConvertEncodingToWindowsCodepage(encoding), &cpInfo)) {
                cpInfo.MaxCharSize = 1; // Is this right ???
            }
            if (cpInfo.MaxCharSize == 1) {
                numChars = maxByteLen;
            } else {
                usedLen = WideCharToMultiByte(RSStringConvertEncodingToWindowsCodepage(encoding), dwFlags, (LPCWSTR)characters, numChars, nil, 0, nil, nil);
                usedLen -= maxByteLen;
                numChars = (numChars > usedLen ? numChars - usedLen : 1);
            }
            if (WideCharToMultiByte(RSStringConvertEncodingToWindowsCodepage(encoding), dwFlags, (LPCWSTR)characters, numChars, (LPSTR)bytes, maxByteLen, nil, nil) == 0) {
                if (usedCharLen) *usedCharLen = 0;
                if (usedByteLen) *usedByteLen = 0;
            } else {
                RSIndex lastUsedLen = 0;
                
                while ((usedLen = WideCharToMultiByte(RSStringConvertEncodingToWindowsCodepage(encoding), dwFlags, (LPCWSTR)characters, ++numChars, (LPSTR)bytes, maxByteLen, nil, nil))) lastUsedLen = usedLen;
                if (usedCharLen) *usedCharLen = (numChars - 1);
                if (usedByteLen) *usedByteLen = lastUsedLen;
            }
            
            return RSStringEncodingInsufficientOutputBufferLength;
        } else {
            return RSStringEncodingInvalidInputStream;
        }
    } else {
        if (usedCharLen) *usedCharLen = numChars;
        if (usedByteLen) *usedByteLen = usedLen;
        return RSStringEncodingConversionSuccess;
    }
#endif /* DEPLOYMENT_TARGET_WINDOWS */
    
    return RSStringEncodingConverterUnavailable;
}

RSPrivate RSIndex __RSStringEncodingPlatformBytesToUnicode(uint32_t encoding, uint32_t flags, const uint8_t *bytes, RSIndex numBytes, RSIndex *usedByteLen, UniChar *characters, RSIndex maxCharLen, RSIndex *usedCharLen) {
    
#if DEPLOYMENT_TARGET_WINDOWS
    WORD dwFlags = 0;
    RSIndex usedLen;
    
    if ((RSStringEncodingUTF7 != encoding) && (RSStringEncodingGB_18030_2000 != encoding) && (0x0800 != (encoding & 0x0F00))) { // not UTF-7/GB18030/ISO-2022-*
        dwFlags |= (flags & (RSStringEncodingAllowLossyConversion|RSStringEncodingSubstituteCombinings) ? 0 : MB_ERR_INVALID_CHARS);
        dwFlags |= (flags & (RSStringEncodingUseCanonical|RSStringEncodingUseHFSPlusCanonical) ? MB_COMPOSITE : MB_PRECOMPOSED);
    }
    
    if ((usedLen = MultiByteToWideChar(RSStringConvertEncodingToWindowsCodepage(encoding), dwFlags, (LPCSTR)bytes, numBytes, (LPWSTR)characters, maxCharLen)) == 0) {
        if (GetLastError() == ERROR_INSUFFICIENT_BUFFER) {
            CPINFO cpInfo;
            
            if (!GetCPInfo(RSStringConvertEncodingToWindowsCodepage(encoding), &cpInfo)) {
                cpInfo.MaxCharSize = 1; // Is this right ???
            }
            if (cpInfo.MaxCharSize == 1) {
                numBytes = maxCharLen;
            } else {
                usedLen = MultiByteToWideChar(RSStringConvertEncodingToWindowsCodepage(encoding), dwFlags, (LPCSTR)bytes, numBytes, (LPWSTR)characters, maxCharLen);
                usedLen -= maxCharLen;
                numBytes = (numBytes > usedLen ? numBytes - usedLen : 1);
            }
            while ((usedLen = MultiByteToWideChar(RSStringConvertEncodingToWindowsCodepage(encoding), dwFlags, (LPCSTR)bytes, numBytes, (LPWSTR)characters, maxCharLen)) == 0) {
                if ((--numBytes) == 0) break;
            }
            if (usedCharLen) *usedCharLen = usedLen;
            if (usedByteLen) *usedByteLen = numBytes;
            
            return RSStringEncodingInsufficientOutputBufferLength;
        } else {
            return RSStringEncodingInvalidInputStream;
        }
    } else {
        if (usedCharLen) *usedCharLen = usedLen;
        if (usedByteLen) *usedByteLen = numBytes;
        return RSStringEncodingConversionSuccess;
    }
#endif /* DEPLOYMENT_TARGET_WINDOWS */
    
    return RSStringEncodingConverterUnavailable;
}

RSPrivate RSIndex __RSStringEncodingPlatformCharLengthForBytes(uint32_t encoding, uint32_t flags, const uint8_t *bytes, RSIndex numBytes) {
    RSIndex usedCharLen;
    return (__RSStringEncodingPlatformBytesToUnicode(encoding, flags, bytes, numBytes, nil, nil, 0, &usedCharLen) == RSStringEncodingConversionSuccess ? usedCharLen : 0);
}

RSPrivate RSIndex __RSStringEncodingPlatformByteLengthForCharacters(uint32_t encoding, uint32_t flags, const UniChar *characters, RSIndex numChars) {
    RSIndex usedByteLen;
    return (__RSStringEncodingPlatformUnicodeToBytes(encoding, flags, characters, numChars, nil, nil, 0, &usedByteLen) == RSStringEncodingConversionSuccess ? usedByteLen : 0);
}

#undef __RSCarbonCore_GetTextEncodingBase0
