//
//  StringConverter.cpp
//  RSCoreFoundation
//
//  Created by closure on 6/2/15.
//  Copyright (c) 2015 RetVal. All rights reserved.
//

#include "StringConverter.h"
#include <RSFoundation/ICUConverters.h>
#include <RSFoundation/UniChar.h>
#include <RSFoundation/UnicodeDecomposition.h>
#include <RSFoundation/StringConverterExt.h>
#include <RSFoundation/Lock.h>
#include <RSFoundation/BultinConverters.h>
#include <stdlib.h>
#include <map>

namespace RSFoundation {
    namespace Collection {
        namespace Encoding {
            typedef Index (*_ToBytesProc)(const void *converter, uint32_t flags, const UniChar *characters, Index numChars, uint8_t *bytes, Index maxByteLen, Index *usedByteLen);
            typedef Index (*_ToUnicodeProc)(const void *converter, uint32_t flags, const uint8_t *bytes, Index numBytes, UniChar *characters, Index maxCharLen, Index *usedCharLen);
            
            typedef struct {
                const StringEncodingConverter *definition;
                _ToBytesProc toBytes;
                _ToUnicodeProc toUnicode;
                _ToUnicodeProc toCanonicalUnicode;
                StringEncodingToBytesFallbackProc toBytesFallback;
                StringEncodingToUnicodeFallbackProc toUnicodeFallback;
            } _EncodingConverter;
            
            /* Macros
             */
#define TO_BYTE(conv,flags,chars,numChars,bytes,max,used) (conv->toBytes ? conv->toBytes(conv,flags,chars,numChars,bytes,max,used) : ((StringEncodingToBytesProc)conv->definition->toBytes)(flags,chars,numChars,bytes,max,used))
#define TO_UNICODE(conv,flags,bytes,numBytes,chars,max,used) (conv->toUnicode ?  (flags & ((uint32_t)String::EncodingConfiguration::UseCanonical|(uint32_t)String::EncodingConfiguration::UseHFSPlusCanonical) ? conv->toCanonicalUnicode(conv,flags,bytes,numBytes,chars,max,used) : conv->toUnicode(conv,flags,bytes,numBytes,chars,max,used)) : ((StringEncodingToUnicodeProc)conv->definition->toUnicode)(flags,bytes,numBytes,chars,max,used))
            
#define ASCIINewLine 0x0a
#define kSurrogateHighStart 0xD800
#define kSurrogateHighEnd 0xDBFF
#define kSurrogateLowStart 0xDC00
#define kSurrogateLowEnd 0xDFFF
            
            static const uint8_t __MaximumConvertedLength = 20;
            
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
                {{'2', 0, 0, 0}}, // SUPECRIPT TWO
                {{'3', 0, 0, 0}}, // SUPECRIPT THREE
                {{0, 0, 0, 0}}, // ACUTE ACCENT
                {{0, 0, 0, 0}}, // MICRO SIGN
                {{0, 0, 0, 0}}, // PILCROW SIGN
                {{0, 0, 0, 0}}, // MIDDLE DOT
                {{0, 0, 0, 0}}, // CEDILLA
                {{'1', 0, 0, 0}}, // SUPECRIPT ONE
                {{'o', 0, 0, 0}}, // MASCULINE ORDINAL INDICATOR
                {{'>', '>', 0, 0}}, // RIGHT-POINTING DOUBLE ANGLE QUOTATION MARK
                {{'1', '/', '4', 0}}, // VULGAR FRACTION ONE QUARTER
                {{'1', '/', '2', 0}}, // VULGAR FRACTION ONE HALF
                {{'3', '/', '4', 0}}, // VULGAR FRACTION THREE QUARTE
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
            
            inline Index __ToASCIILatin1Fallback(UniChar character, uint8_t *bytes, Index maxByteLen) {
                const uint8_t *losChars = (const uint8_t*)_toLossyASCIITable + (character - 0xA0) * sizeof(uint8_t[4]);
                Index numBytes = 0;
                Index idx, max = (maxByteLen && (maxByteLen < 4) ? maxByteLen : 4);
                
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
            
            static Index __DefaultToBytesFallbackProc(const UniChar *characters, Index numChars, uint8_t *bytes, Index maxByteLen, Index *usedByteLen) {
                Index processCharLen = 1, filledBytesLen = 1;
                uint8_t byte = '?';
                
                if (*characters < 0xA0) { // 0x80 to 0x9F maps to ASCII C0 range
                    byte = (uint8_t)(*characters - 0x80);
                } else if (*characters < 0x100) {
                    *usedByteLen = __ToASCIILatin1Fallback(*characters, bytes, maxByteLen);
                    return 1;
                } else if (*characters >= kSurrogateHighStart && *characters <= kSurrogateLowEnd) {
                    processCharLen = (numChars > 1 && *characters <= kSurrogateLowStart && *(characters + 1) >= kSurrogateLowStart && *(characters + 1) <= kSurrogateLowEnd ? 2 : 1);
                } else if (UniCharIsMemberOf(*characters, WhitespaceCharacterSet)) {
                    byte = ' ';
                } else if (UniCharIsMemberOf(*characters, WhitespaceAndNewlineCharacterSet)) {
                    byte = ASCIINewLine;
                } else if (*characters == 0x2026) { // ellipsis
                    if (0 == maxByteLen) {
                        filledBytesLen = 3;
                    } else if (maxByteLen > 2) {
                        memset(bytes, '.', 3);
                        *usedByteLen = 3;
                        return processCharLen;
                    }
                } else if (UniCharIsMemberOf(*characters, DecomposableCharacterSet)) {
                    UTF32Char decomposed[MAX_DECOMPOSED_LENGTH];
                    
                    (void)UnicodeDecoposition::DecomposeCharacter(*characters, decomposed, MAX_DECOMPOSED_LENGTH);
                    if (*decomposed < 0x80) {
                        byte = (uint8_t)(*decomposed);
                    } else {
                        UTF16Char theChar = *decomposed;
                        
                        return __DefaultToBytesFallbackProc(&theChar, 1, bytes, maxByteLen, usedByteLen);
                    }
                }
                
                if (maxByteLen) *bytes = byte;
                *usedByteLen = filledBytesLen;
                return processCharLen;
            }
            
            static Index __DefaultToUnicodeFallbackProc(const uint8_t *bytes, Index numBytes, UniChar *characters, Index maxCharLen, Index *usedCharLen) {
                if (maxCharLen) *characters = (UniChar)'?';
                *usedCharLen = 1;
                return 1;
            }
            
#define TO_BYTE_FALLBACK(conv,chars,numChars,bytes,max,used) (conv->toBytesFallback(chars,numChars,bytes,max,used))
#define TO_UNICODE_FALLBACK(conv,bytes,numBytes,chars,max,used) (conv->toUnicodeFallback(bytes,numBytes,chars,max,used))
            
#define EXTRA_BASE (0x0F00)
            
            /* Wrapper funcs for non-standard converters
             */
            static Index __ToBytesCheapEightBitWrapper(const void *converter, uint32_t flags, const UniChar *characters, Index numChars, uint8_t *bytes, Index maxByteLen, Index *usedByteLen) {
                Index processedCharLen = 0;
                Index length = (maxByteLen && (maxByteLen < numChars) ? maxByteLen : numChars);
                uint8_t byte;
                
                while (processedCharLen < length) {
                    if (!((StringEncodingCheapEightBitToBytesProc)((const _EncodingConverter*)converter)->definition->toBytes)(flags, characters[processedCharLen], &byte)) break;
                    
                    if (maxByteLen) bytes[processedCharLen] = byte;
                    processedCharLen++;
                }
                
                *usedByteLen = processedCharLen;
                return processedCharLen;
            }
            
            static Index __ToUnicodeCheapEightBitWrapper(const void *converter, uint32_t flags, const uint8_t *bytes, Index numBytes, UniChar *characters, Index maxCharLen, Index *usedCharLen) {
                Index processedByteLen = 0;
                Index length = (maxCharLen && (maxCharLen < numBytes) ? maxCharLen : numBytes);
                UniChar character;
                
                while (processedByteLen < length) {
                    if (!((StringEncodingCheapEightBitToUnicodeProc)((const _EncodingConverter*)converter)->definition->toUnicode)(flags, bytes[processedByteLen], &character)) break;
                    
                    if (maxCharLen) characters[processedByteLen] = character;
                    processedByteLen++;
                }
                
                *usedCharLen = processedByteLen;
                return processedByteLen;
            }
            
            static Index __ToCanonicalUnicodeCheapEightBitWrapper(const void *converter, uint32_t flags, const uint8_t *bytes, Index numBytes, UniChar *characters, Index maxCharLen, Index *usedCharLen) {
                Index processedByteLen = 0;
                Index theUsedCharLen = 0;
                UTF32Char charBuffer[MAX_DECOMPOSED_LENGTH];
                Index usedLen;
                UniChar character;
                bool isHFSPlus = (flags & uint32_t(String::EncodingConfiguration::UseHFSPlusCanonical) ? true : false);
                
                while ((processedByteLen < numBytes) && (!maxCharLen || (theUsedCharLen < maxCharLen))) {
                    if (!((StringEncodingCheapEightBitToUnicodeProc)((const _EncodingConverter*)converter)->definition->toUnicode)(flags, bytes[processedByteLen], &character)) break;
                    
                    if (UnicodeDecoposition::IsDecomposableCharacter(character, isHFSPlus)) {
                        Index idx;
                        
                        usedLen = UnicodeDecoposition::DecomposeCharacter(character, charBuffer, MAX_DECOMPOSED_LENGTH);
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
            
            static Index __ToBytesStandardEightBitWrapper(const void *converter, uint32_t flags, const UniChar *characters, Index numChars, uint8_t *bytes, Index maxByteLen, Index *usedByteLen) {
                Index processedCharLen = 0;
                uint8_t byte;
                Index usedLen;
                
                *usedByteLen = 0;
                
                while (numChars && (!maxByteLen || (*usedByteLen < maxByteLen))) {
                    if (!(usedLen = ((StringEncodingStandardEightBitToBytesProc)((const _EncodingConverter*)converter)->definition->toBytes)(flags, characters, numChars, &byte))) break;
                    
                    if (maxByteLen) bytes[*usedByteLen] = byte;
                    (*usedByteLen)++;
                    characters += usedLen;
                    numChars -= usedLen;
                    processedCharLen += usedLen;
                }
                
                return processedCharLen;
            }
            
            static Index __ToUnicodeStandardEightBitWrapper(const void *converter, uint32_t flags, const uint8_t *bytes, Index numBytes, UniChar *characters, Index maxCharLen, Index *usedCharLen) {
                Index processedByteLen = 0;
                UniChar charBuffer[__MaximumConvertedLength];
                Index usedLen;
                
                *usedCharLen = 0;
                
                while ((processedByteLen < numBytes) && (!maxCharLen || (*usedCharLen < maxCharLen))) {
                    if (!(usedLen = ((StringEncodingCheapEightBitToUnicodeProc)((const _EncodingConverter*)converter)->definition->toUnicode)(flags, bytes[processedByteLen], charBuffer))) break;
                    
                    if (maxCharLen) {
                        Index idx;
                        
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
            
            static Index __ToCanonicalUnicodeStandardEightBitWrapper(const void *converter, uint32_t flags, const uint8_t *bytes, Index numBytes, UniChar *characters, Index maxCharLen, Index *usedCharLen) {
                Index processedByteLen = 0;
                UniChar charBuffer[__MaximumConvertedLength];
                UTF32Char decompBuffer[MAX_DECOMPOSED_LENGTH];
                Index usedLen;
                Index decompedLen;
                Index idx, decompIndex;
                bool isHFSPlus = (flags & uint32_t(String::EncodingConfiguration::UseHFSPlusCanonical) ? YES : NO);
                Index theUsedCharLen = 0;
                
                while ((processedByteLen < numBytes) && (!maxCharLen || (theUsedCharLen < maxCharLen)))
                {
                    if (!(usedLen = ((StringEncodingCheapEightBitToUnicodeProc)((const _EncodingConverter*)converter)->definition->toUnicode)(flags, bytes[processedByteLen], charBuffer))) break;
                    
                    for (idx = 0;idx < usedLen;idx++)
                    {
                        if (UnicodeDecoposition::IsDecomposableCharacter(charBuffer[idx], isHFSPlus))
                        {
                            decompedLen = UnicodeDecoposition::DecomposeCharacter(charBuffer[idx], decompBuffer, MAX_DECOMPOSED_LENGTH);
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
            
            static Index __ToBytesCheapMultiByteWrapper(const void *converter, uint32_t flags, const UniChar *characters, Index numChars, uint8_t *bytes, Index maxByteLen, Index *usedByteLen) {
                Index processedCharLen = 0;
                uint8_t byteBuffer[__MaximumConvertedLength];
                Index usedLen;
                
                *usedByteLen = 0;
                
                while ((processedCharLen < numChars) && (!maxByteLen || (*usedByteLen < maxByteLen))) {
                    if (!(usedLen = ((StringEncodingCheapMultiByteToBytesProc)((const _EncodingConverter*)converter)->definition->toBytes)(flags, characters[processedCharLen], byteBuffer))) break;
                    
                    if (maxByteLen) {
                        Index idx;
                        
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
            
            static Index __ToUnicodeCheapMultiByteWrapper(const void *converter, uint32_t flags, const uint8_t *bytes, Index numBytes, UniChar *characters, Index maxCharLen, Index *usedCharLen) {
                Index processedByteLen = 0;
                UniChar character;
                Index usedLen;
                
                *usedCharLen = 0;
                
                while (numBytes && (!maxCharLen || (*usedCharLen < maxCharLen))) {
                    if (!(usedLen = ((StringEncodingCheapMultiByteToUnicodeProc)((const _EncodingConverter*)converter)->definition->toUnicode)(flags, bytes, numBytes, &character))) break;
                    
                    if (maxCharLen) *(characters++) = character;
                    (*usedCharLen)++;
                    processedByteLen += usedLen;
                    bytes += usedLen;
                    numBytes -= usedLen;
                }
                
                return processedByteLen;
            }
            
            static Index __ToCanonicalUnicodeCheapMultiByteWrapper(const void *converter, uint32_t flags, const uint8_t *bytes, Index numBytes, UniChar *characters, Index maxCharLen, Index *usedCharLen) {
                Index processedByteLen = 0;
                UTF32Char charBuffer[MAX_DECOMPOSED_LENGTH];
                UniChar character;
                Index usedLen;
                Index decomposedLen;
                Index theUsedCharLen = 0;
                bool isHFSPlus = (flags & uint32_t(String::EncodingConfiguration::UseHFSPlusCanonical) ? YES : NO);
                
                while (numBytes && (!maxCharLen || (theUsedCharLen < maxCharLen))) {
                    if (!(usedLen = ((StringEncodingCheapMultiByteToUnicodeProc)((const _EncodingConverter*)converter)->definition->toUnicode)(flags, bytes, numBytes, &character))) break;
                    
                    if (UnicodeDecoposition::IsDecomposableCharacter(character, isHFSPlus)) {
                        Index idx;
                        
                        decomposedLen = UnicodeDecoposition::DecomposeCharacter(character, charBuffer, MAX_DECOMPOSED_LENGTH);
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
            inline _EncodingConverter *__EncodingConverterFromDefinition(const StringEncodingConverter *definition, String::Encoding encoding) {
#define NUM_OF_ENTRIES_CYCLE (10)
                static uint32_t _currentIndex = 0;
                static uint32_t _allocatedSize = 0;
                static _EncodingConverter *_allocatedEntries = nil;
                _EncodingConverter *converter;
                
                
                if ((_currentIndex + 1) >= _allocatedSize) {
                    _currentIndex = 0;
                    _allocatedSize = 0;
                    _allocatedEntries = nil;
                }
                if (_allocatedEntries == nil) { // Not allocated yet
                    _allocatedEntries = Allocator<_EncodingConverter>::AllocatorSystemDefault.Allocate<_EncodingConverter>(NUM_OF_ENTRIES_CYCLE);
//                    _allocatedEntries = (_EncodingConverter *)__AutoReleaseISA(AllocatorSystemDefault, AllocatorAllocate(AllocatorSystemDefault, sizeof(_EncodingConverter) * NUM_OF_ENTRIES_CYCLE));
                    
                    _allocatedSize = NUM_OF_ENTRIES_CYCLE;
                    converter = &(_allocatedEntries[_currentIndex]);
                } else {
                    converter = &(_allocatedEntries[++_currentIndex]);
                }
                
                memset(converter, 0, sizeof(_EncodingConverter));
                
                converter->definition = definition;
                
                switch (definition->encodingClass) {
                    case StringEncodingConverterStandard:
                        converter->toBytes = nil;
                        converter->toUnicode = nil;
                        converter->toCanonicalUnicode = nil;
                        break;
                        
                    case StringEncodingConverterCheapEightBit:
                        converter->toBytes = __ToBytesCheapEightBitWrapper;
                        converter->toUnicode = __ToUnicodeCheapEightBitWrapper;
                        converter->toCanonicalUnicode = __ToCanonicalUnicodeCheapEightBitWrapper;
                        break;
                        
                    case StringEncodingConverterStandardEightBit:
                        converter->toBytes = __ToBytesStandardEightBitWrapper;
                        converter->toUnicode = __ToUnicodeStandardEightBitWrapper;
                        converter->toCanonicalUnicode = __ToCanonicalUnicodeStandardEightBitWrapper;
                        break;
                        
                    case StringEncodingConverterCheapMultiByte:
                        converter->toBytes = __ToBytesCheapMultiByteWrapper;
                        converter->toUnicode = __ToUnicodeCheapMultiByteWrapper;
                        converter->toCanonicalUnicode = __ToCanonicalUnicodeCheapMultiByteWrapper;
                        break;
                        
#if DEPLOYMENT_TARGET_MACOSX || DEPLOYMENT_TARGET_EMBEDDED || DEPLOYMENT_TARGET_WINDOWS || DEPLOYMENT_TARGET_LINUX
                    case StringEncodingConverterICU:
                        converter->toBytes = (_ToBytesProc)ICUConverters::GetICUName(encoding);
                        break;
#endif
                        
                    case StringEncodingConverterPlatformSpecific:
                        break;
                        
                    default: // Shouln't be here
                        return nil;
                }
                
                converter->toBytesFallback = (definition->toBytesFallback ? definition->toBytesFallback : __DefaultToBytesFallbackProc);
                converter->toUnicodeFallback = (definition->toUnicodeFallback ? definition->toUnicodeFallback : __DefaultToUnicodeFallbackProc);
                
                return converter;
            }
            
            inline const StringEncodingConverter *__StringEncodingConverterGetDefinition(String::Encoding encoding) {
                switch (encoding) {
                    case String::Encoding::UTF8:
                        return &__ConverterUTF8;
                        
                    case String::Encoding::MacRoman:
                        return &__ConverterMacRoman;
                        
                    case String::Encoding::WindowsLatin1:
                        return &__ConverterWinLatin1;
                        
                    case String::Encoding::ASCII:
                        return &__ConverterASCII;
                        
                    case String::Encoding::ISOLatin1:
                        return &__ConverterISOLatin1;
                        
                        
                    case String::Encoding::NextStepLatin:
                        return &__ConverterNextStepLatin;
                        
                        
                    default:
                        return BuiltinConverters::GetExternalConverter(encoding);
                }
            }
            
            static std::map<String::Encoding, const _EncodingConverter*> *mappingTable = nullptr;
            static _EncodingConverter *mappingTableCommonConverters[3] = {nil, nil, nil}; // UTF8, MacRoman/WinLatin1, and the default encoding*
            static SpinLock mappingTableGetConverterLock;
            
//            static void __EncodingMappingTableReleaseRoutine(NotificationRef notification) {
//                SpinLockLock(&mappingTableGetConverterLock);
//                if (mappingTable)
//                    Release(mappingTable);
//                SpinLockUnlock(&mappingTableGetConverterLock);
//            }
            
//            static void __EncodingMappingTableRegister(DictionaryRef dict) {
//                NotificationCenterAddObserver(NotificationCenterGetDefault(), Autorelease(ObserverCreate(AllocatorSystemDefault, CoreFoundationWillDeallocateNotification, __EncodingMappingTableReleaseRoutine, nil)));
//            }
            
            static const _EncodingConverter *__GetConverter(String::Encoding encoding)
            {
                const _EncodingConverter *converter = nil;
                const _EncodingConverter **commonConverterSlot = nil;
                
                switch (encoding) {
                    case String::Encoding::UTF8: commonConverterSlot = (const _EncodingConverter **)&(mappingTableCommonConverters[0]); break;
                        
                        /* the swith here should avoid possible bootstrap issues in the default: case below when invoked from StringGetSystemEncoding() */
#if DEPLOYMENT_TARGET_MACOSX || DEPLOYMENT_TARGET_EMBEDDED || DEPLOYMENT_TARGET_EMBEDDED_MINI || DEPLOYMENT_TARGET_LINUX
                    case String::Encoding::MacRoman: commonConverterSlot = (const _EncodingConverter **)&(mappingTableCommonConverters[1]); break;
#elif DEPLOYMENT_TARGET_WINDOWS
                    case String::Encoding::WindowsLatin1: commonConverterSlot = (const _EncodingConverter **)(&(commonConverters[1])); break;
#else
#warning This case must match __defaultEncoding value defined in String.c
                    case String::Encoding::ISOLatin1: commonConverterSlot = (const _EncodingConverter **)(&(commonConverters[1])); break;
#endif
                        
                    default: if (String::GetSystemEncoding() == encoding) commonConverterSlot = (const _EncodingConverter **)&(mappingTableCommonConverters[2]); break;
                }
                
                mappingTableGetConverterLock.Acquire();
                converter = ((nil == commonConverterSlot) ? ((nil == mappingTable) ? nil : (*mappingTable)[encoding]) : *commonConverterSlot);
                mappingTableGetConverterLock.Release();
                
                if (nil == converter) {
                    const StringEncodingConverter *definition = __StringEncodingConverterGetDefinition(encoding);
                    
                    if (nil != definition) {
                        mappingTableGetConverterLock.Acquire();
                        converter = ((nil == commonConverterSlot) ? ((nil == mappingTable) ? nil : (*mappingTable)[encoding]) : *commonConverterSlot);
                        
                        if (nil == converter) {
                            converter = __EncodingConverterFromDefinition(definition, encoding);
                            
                            if (nil == commonConverterSlot) {
                                if (nil == mappingTable) {
                                    mappingTable = Allocator<std::map<String::Encoding, const _EncodingConverter*>>::AllocatorSystemDefault.Allocate();
//                                    mappingTable = DictionaryCreateMutable(nil, 0, nil);
                                    //                        Autorelease(mappingTable);
//                                    __EncodingMappingTableRegister(mappingTable);
                                }
                                (*mappingTable)[encoding] = converter;
//                                DictionarySetValue(mappingTable, (const void *)(uintptr_t)encoding, converter);
                            }
                            else
                            {
                                *commonConverterSlot = converter;
                            }
                        }
                        mappingTableGetConverterLock.Release();
                    }
                }
                
                return converter;
            }
            
            /* Public API
             */
            uint32_t StringEncodingUnicodeToBytes(String::Encoding encoding, uint32_t flags, const UniChar *characters, Index numChars, Index *usedCharLen, uint8_t *bytes, Index maxByteLen, Index *usedByteLen) {
                if (encoding == String::Encoding::UTF8) {
                    static StringEncodingToBytesProc __ToUTF8 = nil;
                    Index convertedCharLen;
                    Index usedLen;
                    
                    
                    if ((flags & (uint32_t)String::EncodingConfiguration::UseCanonical) || (flags & (uint32_t)String::EncodingConfiguration::UseHFSPlusCanonical)) {
                        (void)UnicodeDecoposition::Decompose(characters, numChars, &convertedCharLen, (void *)bytes, maxByteLen, &usedLen, YES, UniCharEncodingFormat::UTF8Format, (flags & (uint32_t)String::EncodingConfiguration::UseHFSPlusCanonical ? YES : NO));
                    } else {
                        if (!__ToUTF8) {
                            const StringEncodingConverter *utf8Converter = StringEncodingGetConverter(String::Encoding::UTF8);
                            __ToUTF8 = (StringEncodingToBytesProc)utf8Converter->toBytes;
                        }
                        convertedCharLen = __ToUTF8(0, characters, numChars, bytes, maxByteLen, &usedLen);
                    }
                    if (usedCharLen) *usedCharLen = convertedCharLen;
                    if (usedByteLen) *usedByteLen = usedLen;
                    
                    if (convertedCharLen == numChars) {
                        return String::ConversionResult::Success;
                    } else if ((maxByteLen > 0) && ((maxByteLen - usedLen) < 10)) { // could be filled outbuf
                        UTF16Char character = characters[convertedCharLen];
                        
                        if (((character >= kSurrogateLowStart) && (character <= kSurrogateLowEnd)) || ((character >= kSurrogateHighStart) && (character <= kSurrogateHighEnd) && ((1 == (numChars - convertedCharLen)) || (characters[convertedCharLen + 1] < kSurrogateLowStart) || (characters[convertedCharLen + 1] > kSurrogateLowEnd)))) return String::ConversionResult::InvalidInputStream;
                        
                        return String::ConversionResult::InsufficientOutputBufferLength;
                    } else {
                        return String::ConversionResult::InvalidInputStream;
                    }
                } else {
                    const _EncodingConverter *converter = __GetConverter(encoding);
                    Index usedLen = 0;
                    Index localUsedByteLen;
                    Index theUsedByteLen = 0;
                    uint32_t theResult = String::ConversionResult::Success;
                    StringEncodingToBytesPrecomposeProc toBytesPrecompose = nil;
                    StringEncodingIsValidCombiningCharacterProc isValidCombiningChar = nil;
                    
                    if (!converter) return String::ConversionResult::Unavailable;
                    
                    if (flags & (uint32_t)String::EncodingConfiguration::SubstituteCombinings) {
                        if (!(flags & (uint32_t)String::EncodingConfiguration::AllowLossyConversion)) isValidCombiningChar = converter->definition->isValidCombiningChar;
                    } else {
                        isValidCombiningChar = converter->definition->isValidCombiningChar;
                        if (!(flags & (uint32_t)String::EncodingConfiguration::IgnoreCombinings)) {
                            toBytesPrecompose = converter->definition->toBytesPrecompose;
                            flags |= (uint32_t)String::EncodingConfiguration::ComposeCombinings;
                        }
                    }
                    
#if DEPLOYMENT_TARGET_MACOSX || DEPLOYMENT_TARGET_EMBEDDED || DEPLOYMENT_TARGET_WINDOWS || DEPLOYMENT_TARGET_LINUX
                    if (StringEncodingConverterICU == converter->definition->encodingClass) return (uint32_t)ICUConverters::ICUToBytes((const char *)converter->toBytes, flags, characters, numChars, usedCharLen, bytes, maxByteLen, usedByteLen);
#endif
                    
                    /* Platform converter */
                    if (StringEncodingConverterPlatformSpecific == converter->definition->encodingClass) return (uint32_t)BuiltinConverters::PlatformUnicodeToBytes(encoding, flags, characters, numChars, usedCharLen, bytes, maxByteLen, usedByteLen);
                    
                    while ((usedLen < numChars) && (!maxByteLen || (theUsedByteLen < maxByteLen))) {
                        if ((usedLen += TO_BYTE(converter, flags, characters + usedLen, numChars - usedLen, bytes + theUsedByteLen, (maxByteLen ? maxByteLen - theUsedByteLen : 0), &localUsedByteLen)) < numChars) {
                            Index dummy;
                            
                            if (isValidCombiningChar && (usedLen > 0) && isValidCombiningChar(characters[usedLen])) {
                                if (toBytesPrecompose) {
                                    Index localUsedLen = usedLen;
                                    
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
                                            theResult = String::ConversionResult::InvalidInputStream;
                                            break;
                                        }
                                    } else if (flags & (uint32_t)String::EncodingConfiguration::AllowLossyConversion) {
                                        uint8_t lossyByte = StringEncodingMaskToLossyByte(flags);
                                        
                                        if (lossyByte) {
                                            while (isValidCombiningChar(characters[++usedLen]));
                                            localUsedByteLen = 1;
                                            if (maxByteLen) *(bytes + theUsedByteLen) = lossyByte;
                                        } else {
                                            ++usedLen;
                                            usedLen += TO_BYTE_FALLBACK(converter, characters + usedLen, numChars - usedLen, bytes + theUsedByteLen, (maxByteLen ? maxByteLen - theUsedByteLen : 0), &localUsedByteLen);
                                        }
                                    } else {
                                        theResult = String::ConversionResult::InvalidInputStream;
                                        break;
                                    }
                                } else if (maxByteLen && ((maxByteLen == theUsedByteLen + localUsedByteLen) || TO_BYTE(converter, flags, characters + usedLen, numChars - usedLen, nil, 0, &dummy))) { // buffer was filled up
                                    theUsedByteLen += localUsedByteLen;
                                    theResult = String::ConversionResult::InsufficientOutputBufferLength;
                                    break;
                                } else if (flags & (uint32_t)String::EncodingConfiguration::IgnoreCombinings) {
                                    while ((++usedLen < numChars) && isValidCombiningChar(characters[usedLen]));
                                } else {
                                    uint8_t lossyByte = StringEncodingMaskToLossyByte(flags);
                                    
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
                                
                                if (flags & (uint32_t)String::EncodingConfiguration::AllowLossyConversion && !StringEncodingMaskToLossyByte(flags)) {
                                    Index localUsedLen;
                                    
                                    localUsedByteLen = 0;
                                    while ((usedLen < numChars) && !localUsedByteLen && (localUsedLen = TO_BYTE_FALLBACK(converter, characters + usedLen, numChars - usedLen, nil, 0, &localUsedByteLen))) usedLen += localUsedLen;
                                }
                                if (usedLen < numChars) theResult = String::ConversionResult::InsufficientOutputBufferLength;
                                break;
                            } else if (flags & (uint32_t)String::EncodingConfiguration::AllowLossyConversion) {
                                uint8_t lossyByte = StringEncodingMaskToLossyByte(flags);
                                
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
                                theResult = String::ConversionResult::InvalidInputStream;
                                break;
                            }
                        }
                        theUsedByteLen += localUsedByteLen;
                    }
                    
                    if (usedLen < numChars && maxByteLen && theResult == String::ConversionResult::Success) {
                        if (flags & (uint32_t)String::EncodingConfiguration::AllowLossyConversion && !StringEncodingMaskToLossyByte(flags)) {
                            Index localUsedLen;
                            
                            localUsedByteLen = 0;
                            while ((usedLen < numChars) && !localUsedByteLen && (localUsedLen = TO_BYTE_FALLBACK(converter, characters + usedLen, numChars - usedLen, nil, 0, &localUsedByteLen))) usedLen += localUsedLen;
                        }
                        if (usedLen < numChars) theResult = String::ConversionResult::InsufficientOutputBufferLength;
                    }
                    if (usedByteLen) *usedByteLen = theUsedByteLen;
                    if (usedCharLen) *usedCharLen = usedLen;
                    
                    return theResult;
                }
            }
            
            uint32_t StringEncodingBytesToUnicode(String::Encoding encoding, uint32_t flags, const uint8_t *bytes, Index numBytes, Index *usedByteLen, UniChar *characters, Index maxCharLen, Index *usedCharLen) {
                const _EncodingConverter *converter = __GetConverter(encoding);
                Index usedLen = 0;
                Index theUsedCharLen = 0;
                Index localUsedCharLen;
                uint32_t theResult = String::ConversionResult::Success;
                
                if (!converter) return String::ConversionResult::Unavailable;
                
#if DEPLOYMENT_TARGET_MACOSX || DEPLOYMENT_TARGET_EMBEDDED || DEPLOYMENT_TARGET_WINDOWS || DEPLOYMENT_TARGET_LINUX
                if (StringEncodingConverterICU == converter->definition->encodingClass) return (uint32_t)ICUConverters::ICUToUnicode((const char *)converter->toBytes, flags, bytes, numBytes, usedByteLen, characters, maxCharLen, usedCharLen);
#endif
                
                /* Platform converter */
                if (StringEncodingConverterPlatformSpecific == converter->definition->encodingClass) return (uint32_t)BuiltinConverters::PlatformBytesToUnicode(encoding, flags, bytes, numBytes, usedByteLen, characters, maxCharLen, usedCharLen);
                
                while ((usedLen < numBytes) && (!maxCharLen || (theUsedCharLen < maxCharLen))) {
                    if ((usedLen += TO_UNICODE(converter, flags, bytes + usedLen, numBytes - usedLen, characters + theUsedCharLen, (maxCharLen ? maxCharLen - theUsedCharLen : 0), &localUsedCharLen)) < numBytes) {
                        Index tempUsedCharLen;
                        
                        if (maxCharLen && ((maxCharLen == theUsedCharLen + localUsedCharLen) || (((flags & ((uint32_t)String::EncodingConfiguration::UseCanonical|(uint32_t)String::EncodingConfiguration::UseHFSPlusCanonical)) || (maxCharLen == theUsedCharLen + localUsedCharLen + 1)) && TO_UNICODE(converter, flags, bytes + usedLen, numBytes - usedLen, nil, 0, &tempUsedCharLen)))) { // buffer was filled up
                            theUsedCharLen += localUsedCharLen;
                            theResult = String::ConversionResult::InsufficientOutputBufferLength;
                            break;
                        } else if (flags & (uint32_t)String::EncodingConfiguration::AllowLossyConversion) {
                            theUsedCharLen += localUsedCharLen;
                            usedLen += TO_UNICODE_FALLBACK(converter, bytes + usedLen, numBytes - usedLen, characters + theUsedCharLen, (maxCharLen ? maxCharLen - theUsedCharLen : 0), &localUsedCharLen);
                        } else {
                            theUsedCharLen += localUsedCharLen;
                            theResult = String::ConversionResult::InvalidInputStream;
                            break;
                        }
                    }
                    theUsedCharLen += localUsedCharLen;
                }
                
                if (usedLen < numBytes && maxCharLen && theResult == String::ConversionResult::Success) {
                    theResult = String::ConversionResult::InsufficientOutputBufferLength;
                }
                if (usedCharLen) *usedCharLen = theUsedCharLen;
                if (usedByteLen) *usedByteLen = usedLen;
                
                return theResult;
            }
            
            Private bool StringEncodingIsValidEncoding(String::Encoding encoding) {
                return (StringEncodingGetConverter(encoding) ? YES : NO);
            }
            
            Private Index StringEncodingCharLengthForBytes(String::Encoding encoding, uint32_t flags, const uint8_t *bytes, Index numBytes) {
                const _EncodingConverter *converter = __GetConverter(encoding);
                
                if (converter) {
#if DEPLOYMENT_TARGET_MACOSX || DEPLOYMENT_TARGET_EMBEDDED || DEPLOYMENT_TARGET_WINDOWS || DEPLOYMENT_TARGET_LINUX
                    if (StringEncodingConverterICU == converter->definition->encodingClass) return ICUConverters::ICUCharLength((const char *)converter->toBytes, flags, bytes, numBytes);
#endif
                    
                    if (StringEncodingConverterPlatformSpecific == converter->definition->encodingClass) return BuiltinConverters::PlatformCharLengthForBytes(encoding, flags, bytes, numBytes);
                    
                    if (1 == converter->definition->maxBytesPerChar) return numBytes;
                    
                    if (nil == converter->definition->toUnicodeLen) {
                        Index usedByteLen = 0;
                        Index totalLength = 0;
                        Index usedCharLen;
                        
                        while (numBytes > 0) {
                            usedByteLen = TO_UNICODE(converter, flags, bytes, numBytes, nil, 0, &usedCharLen);
                            
                            bytes += usedByteLen;
                            numBytes -= usedByteLen;
                            totalLength += usedCharLen;
                            
                            if (numBytes > 0) {
                                if (0 == (flags & (uint32_t)String::EncodingConfiguration::AllowLossyConversion)) return 0;
                                
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
            
            Private Index StringEncodingByteLengthForCharacters(String::Encoding encoding, uint32_t flags, const UniChar *characters, Index numChars)
            {
                const _EncodingConverter *converter = __GetConverter(encoding);
                
                if (converter) {
#if DEPLOYMENT_TARGET_MACOSX || DEPLOYMENT_TARGET_EMBEDDED || DEPLOYMENT_TARGET_WINDOWS || DEPLOYMENT_TARGET_LINUX
                    if (StringEncodingConverterICU == converter->definition->encodingClass) return ICUConverters::ICUByteLength((const char *)converter->toBytes, flags, characters, numChars);
#endif
                    
                    if (StringEncodingConverterPlatformSpecific == converter->definition->encodingClass) return BuiltinConverters::PlatformByteLengthForCharacters(encoding, flags, characters, numChars);
                    
                    if (1 == converter->definition->maxBytesPerChar) return numChars;
                    
                    if (nil == converter->definition->toBytesLen) {
                        Index usedByteLen;
                        
                        return ((String::ConversionResult::Success == StringEncodingUnicodeToBytes(encoding, flags, characters, numChars, nil, nil, 0, &usedByteLen)) ? usedByteLen : 0);
                    } else {
                        return converter->definition->toBytesLen(flags, characters, numChars);
                    }
                }
                
                return 0;
            }
            
            Private void StringEncodingRegisterFallbackProcedures(String::Encoding encoding, StringEncodingToBytesFallbackProc toBytes, StringEncodingToUnicodeFallbackProc toUnicode) {
                _EncodingConverter *converter = (_EncodingConverter *)__GetConverter(encoding);
                
                if (nil != converter) {
                    const StringEncodingConverter *body = StringEncodingGetConverter(encoding);
                    
                    converter->toBytesFallback = ((nil == toBytes) ? ((nil == body) ? __DefaultToBytesFallbackProc : body->toBytesFallback) : toBytes);
                    converter->toUnicodeFallback = ((nil == toUnicode) ? ((nil == body) ? __DefaultToUnicodeFallbackProc : body->toUnicodeFallback) : toUnicode);
                }
            }
            
            Private const StringEncodingConverter *StringEncodingGetConverter(String::Encoding encoding) {
                const _EncodingConverter *converter = __GetConverter(encoding);
                
                return ((nil == converter) ? nil : converter->definition);
            }
            
            static const String::Encoding __BuiltinEncodings[] = {
                String::Encoding::MacRoman,
                String::Encoding::WindowsLatin1,
                String::Encoding::ISOLatin1,
                String::Encoding::NextStepLatin,
                String::Encoding::ASCII,
                String::Encoding::UTF8,
                /* These seven are available only in String-level */
                String::Encoding::NonLossyASCII,
                
                String::Encoding::UTF16,
                String::Encoding::UTF16BE,
                String::Encoding::UTF16LE,
                
                String::Encoding::UTF32,
                String::Encoding::UTF32BE,
                String::Encoding::UTF32LE,
                
                String::Encoding::InvalidId,
            };
            
            static ComparisonResult __StringEncodingComparator(const void *v1, const void *v2, void *context) {
                ComparisonResult val1 = ComparisonResult((*(const Index *)v1) & 0xFFFF);
                ComparisonResult val2 = ComparisonResult((*(const Index *)v2) & 0xFFFF);
                
                return ((val1 == val2) ? ComparisonResult((Index)(*(const Index *)v1) - (Index)(*(const Index *)v2)) : ComparisonResult(val1 - val2));
            }
            
            static void __StringEncodingFliterDupes(String::Encoding *encodings, Index numSlots) {
                String::Encoding last = String::Encoding::InvalidId;
                const String::Encoding *limitEncodings = encodings + numSlots;
                
                while (encodings < limitEncodings) {
                    if (last == *encodings) {
                        if ((encodings + 1) < limitEncodings) memmove(encodings, encodings + 1, sizeof(String::Encoding) * (limitEncodings - encodings - 1));
                        --limitEncodings;
                    } else {
                        last = *(encodings++);
                    }
                }
            }
            
            Private const String::Encoding *StringEncodingListOfAvailableEncodings(void) {
                static const String::Encoding *encodings = nil;
                
                if (nil == encodings) {
                    String::Encoding *list = (String::Encoding *)__BuiltinEncodings;
                    Index numICUConverters = 0, numPlatformConverters = 0;
#if DEPLOYMENT_TARGET_MACOSX || DEPLOYMENT_TARGET_EMBEDDED || DEPLOYMENT_TARGET_WINDOWS || DEPLOYMENT_TARGET_LINUX
                    String::Encoding *icuConverters = ICUConverters::CreateICUEncodings(&numICUConverters);
#else
                    String::Encoding *icuConverters = nil;
#endif
                    String::Encoding *platformConverters = BuiltinConverters::CreateListOfAvailablePlatformConverters(&numPlatformConverters);
                    auto allocator = &Allocator<String::Encoding>::AllocatorSystemDefault;
                    
                    if ((nil != icuConverters) || (nil != platformConverters)) {
                        Index numSlots = (sizeof(__BuiltinEncodings) / sizeof(*__BuiltinEncodings)) + numICUConverters + numPlatformConverters;
                        
                        list = allocator->Allocate<String::Encoding>(numSlots);
//                        list = (StringEncoding *)AllocatorAllocate(nil, sizeof(StringEncoding) * numSlots);
                        
                        memcpy(list, __BuiltinEncodings, sizeof(__BuiltinEncodings));
                        
                        if (nil != icuConverters) {
                            memcpy(list + (sizeof(__BuiltinEncodings) / sizeof(*__BuiltinEncodings)), icuConverters, sizeof(String::Encoding) * numICUConverters);
                            allocator->Deallocate(icuConverters);
                        }
                        
                        if (nil != platformConverters) {
                            memcpy(list + (sizeof(__BuiltinEncodings) / sizeof(*__BuiltinEncodings)) + numICUConverters, platformConverters, sizeof(String::Encoding) * numPlatformConverters);
                            allocator->Deallocate(platformConverters);
                        }
                        //            extern void QSortArray(void **list, Index count, Index elementSize, ComparatorFunction comparator, bool ascending);
//                        SortArray((void **)&list, numSlots, sizeof(StringEncoding), OrderedDescending, (ComparatorFunction)__StringEncodingComparator, nil);
                        
                        __StringEncodingFliterDupes(list, numSlots);
                    }
                    if (!OSAtomicCompareAndSwapPtrBarrier(nil, list, (void * volatile *)&encodings) && (list != __BuiltinEncodings))
                        allocator->Deallocate(list);
//                    __AutoReleaseISA(nil, (ISA)encodings);
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

#include "ICUConverters.h"
            
            
            inline bool __IsPlatformConverterAvailable(String::Encoding encoding) {
                
#if DEPLOYMENT_TARGET_WINDOWS
                return (IsValidCodePage(StringConvertEncodingToWindowsCodepage(encoding)) ? YES : NO);
#else
                return false;
#endif
            }
            
            static const StringEncodingConverter __ICUBootstrap = {
                nil /* toBytes */, nil /* toUnicode */, 6 /* maxBytesPerChar */, 4 /* maxDecomposedCharLen */,
                StringEncodingConverterICU /* encodingClass */,
                nil /* toBytesLen */, nil /* toUnicodeLen */, nil /* toBytesFallback */,
                nil /* toUnicodeFallback */, nil /* toBytesPrecompose */, nil, /* isValidCombiningChar */
            };
            
            static const StringEncodingConverter __PlatformBootstrap = {
                nil /* toBytes */, nil /* toUnicode */, 6 /* maxBytesPerChar */, 4 /* maxDecomposedCharLen */,
                StringEncodingConverterPlatformSpecific /* encodingClass */,
                nil /* toBytesLen */, nil /* toUnicodeLen */, nil /* toBytesFallback */,
                nil /* toUnicodeFallback */, nil /* toBytesPrecompose */, nil, /* isValidCombiningChar */
            };
            
            const StringEncodingConverter *BuiltinConverters::GetExternalConverter(String::Encoding encoding) {
                
                // we prefer Text Encoding Converter ICU since it's more reliable
                if (__IsPlatformConverterAvailable(encoding)) {
                    return &__PlatformBootstrap;
                } else {
#if DEPLOYMENT_TARGET_MACOSX || DEPLOYMENT_TARGET_EMBEDDED || DEPLOYMENT_TARGET_WINDOWS || DEPLOYMENT_TARGET_LINUX
                    if (ICUConverters::GetICUName(encoding)) {
                        return &__ICUBootstrap;
                    }
#endif
                    return nil;
                }
                
            }
            
            Index BuiltinConverters::PlatformUnicodeToBytes(String::Encoding encoding, uint32_t flags, const UniChar *characters, Index numChars, Index *usedCharLen, uint8_t *bytes, Index maxByteLen, Index *usedByteLen) {
                
#if DEPLOYMENT_TARGET_WINDOWS
                WORD dwFlags = 0;
                Index usedLen;
                
                if ((StringEncodingUTF7 != encoding) && (StringEncodingGB_18030_2000 != encoding) && (0x0800 != (encoding & 0x0F00))) { // not UTF-7/GB18030/ISO-2022-*
                    dwFlags |= (flags & (StringEncodingAllowLossyConversion|StringEncodingSubstituteCombinings) ? WC_DEFAULTCHAR : 0);
                    dwFlags |= (flags & (uint32_t)String::EncodingConfiguration::ComposeCombinings ? WC_COMPOSITECHECK : 0);
                    dwFlags |= (flags & (uint32_t)String::EncodingConfiguration::IgnoreCombinings ? WC_DISCARDNS : 0);
                }
                
                if ((usedLen = WideCharToMultiByte(StringConvertEncodingToWindowsCodepage(encoding), dwFlags, (LPCWSTR)characters, numChars, (LPSTR)bytes, maxByteLen, nil, nil)) == 0) {
                    if (GetLastError() == ERROR_INSUFFICIENT_BUFFER) {
                        CPINFO cpInfo;
                        
                        if (!GetCPInfo(StringConvertEncodingToWindowsCodepage(encoding), &cpInfo)) {
                            cpInfo.MaxCharSize = 1; // Is this right ???
                        }
                        if (cpInfo.MaxCharSize == 1) {
                            numChars = maxByteLen;
                        } else {
                            usedLen = WideCharToMultiByte(StringConvertEncodingToWindowsCodepage(encoding), dwFlags, (LPCWSTR)characters, numChars, nil, 0, nil, nil);
                            usedLen -= maxByteLen;
                            numChars = (numChars > usedLen ? numChars - usedLen : 1);
                        }
                        if (WideCharToMultiByte(StringConvertEncodingToWindowsCodepage(encoding), dwFlags, (LPCWSTR)characters, numChars, (LPSTR)bytes, maxByteLen, nil, nil) == 0) {
                            if (usedCharLen) *usedCharLen = 0;
                            if (usedByteLen) *usedByteLen = 0;
                        } else {
                            Index lastUsedLen = 0;
                            
                            while ((usedLen = WideCharToMultiByte(StringConvertEncodingToWindowsCodepage(encoding), dwFlags, (LPCWSTR)characters, ++numChars, (LPSTR)bytes, maxByteLen, nil, nil))) lastUsedLen = usedLen;
                            if (usedCharLen) *usedCharLen = (numChars - 1);
                            if (usedByteLen) *usedByteLen = lastUsedLen;
                        }
                        
                        return StringEncodingInsufficientOutputBufferLength;
                    } else {
                        return StringEncodingInvalidInputStream;
                    }
                } else {
                    if (usedCharLen) *usedCharLen = numChars;
                    if (usedByteLen) *usedByteLen = usedLen;
                    return StringEncodingConversionSuccess;
                }
#endif /* DEPLOYMENT_TARGET_WINDOWS */
                
                return String::ConversionResult::Unavailable;
            }
            
            Index BuiltinConverters::PlatformBytesToUnicode(String::Encoding encoding, uint32_t flags, const uint8_t *bytes, Index numBytes, Index *usedByteLen, UniChar *characters, Index maxCharLen, Index *usedCharLen) {
                
#if DEPLOYMENT_TARGET_WINDOWS
                WORD dwFlags = 0;
                Index usedLen;
                
                if ((StringEncodingUTF7 != encoding) && (StringEncodingGB_18030_2000 != encoding) && (0x0800 != (encoding & 0x0F00))) { // not UTF-7/GB18030/ISO-2022-*
                    dwFlags |= (flags & (StringEncodingAllowLossyConversion|StringEncodingSubstituteCombinings) ? 0 : MB_ERR_INVALID_CHA);
                    dwFlags |= (flags & (StringEncodingUseCanonical|StringEncodingUseHFSPlusCanonical) ? MB_COMPOSITE : MB_PRECOMPOSED);
                }
                
                if ((usedLen = MultiByteToWideChar(StringConvertEncodingToWindowsCodepage(encoding), dwFlags, (LPCSTR)bytes, numBytes, (LPWSTR)characters, maxCharLen)) == 0) {
                    if (GetLastError() == ERROR_INSUFFICIENT_BUFFER) {
                        CPINFO cpInfo;
                        
                        if (!GetCPInfo(StringConvertEncodingToWindowsCodepage(encoding), &cpInfo)) {
                            cpInfo.MaxCharSize = 1; // Is this right ???
                        }
                        if (cpInfo.MaxCharSize == 1) {
                            numBytes = maxCharLen;
                        } else {
                            usedLen = MultiByteToWideChar(StringConvertEncodingToWindowsCodepage(encoding), dwFlags, (LPCSTR)bytes, numBytes, (LPWSTR)characters, maxCharLen);
                            usedLen -= maxCharLen;
                            numBytes = (numBytes > usedLen ? numBytes - usedLen : 1);
                        }
                        while ((usedLen = MultiByteToWideChar(StringConvertEncodingToWindowsCodepage(encoding), dwFlags, (LPCSTR)bytes, numBytes, (LPWSTR)characters, maxCharLen)) == 0) {
                            if ((--numBytes) == 0) break;
                        }
                        if (usedCharLen) *usedCharLen = usedLen;
                        if (usedByteLen) *usedByteLen = numBytes;
                        
                        return StringEncodingInsufficientOutputBufferLength;
                    } else {
                        return StringEncodingInvalidInputStream;
                    }
                } else {
                    if (usedCharLen) *usedCharLen = usedLen;
                    if (usedByteLen) *usedByteLen = numBytes;
                    return StringEncodingConversionSuccess;
                }
#endif /* DEPLOYMENT_TARGET_WINDOWS */
                
                return String::ConversionResult::Unavailable;
            }
            
            Index BuiltinConverters::PlatformCharLengthForBytes(String::Encoding encoding, uint32_t flags, const uint8_t *bytes, Index numBytes) {
                Index usedCharLen;
                return (PlatformBytesToUnicode(encoding, flags, bytes, numBytes, nil, nil, 0, &usedCharLen) == String::ConversionResult::Success ? usedCharLen : 0);
            }
            
            Index BuiltinConverters::PlatformByteLengthForCharacters(String::Encoding encoding, uint32_t flags, const UniChar *characters, Index numChars) {
                Index usedByteLen;
                return (PlatformUnicodeToBytes(encoding, flags, characters, numChars, nil, nil, 0, &usedByteLen) == String::ConversionResult::Success ? usedByteLen : 0);
            }
            
            
#undef __CarbonCore_GetTextEncodingBase0

        }
    }
}
