//
//  StringConverterExt.h
//  RSCoreFoundation
//
//  Created by closure on 6/2/15.
//  Copyright (c) 2015 RetVal. All rights reserved.
//

#ifndef __RSCoreFoundation__StringConverterExt__
#define __RSCoreFoundation__StringConverterExt__

#include <RSFoundation/Object.hpp>
#include <RSFoundation/StringConverter.hpp>

namespace RSFoundation {
    namespace Collection {
        namespace Encoding {
            #define MAX_DECOMPOSED_LENGTH (10)
            enum {
                StringEncodingConverterStandard = 0,
                StringEncodingConverterCheapEightBit = 1,
                StringEncodingConverterStandardEightBit = 2,
                StringEncodingConverterCheapMultiByte = 3,
                StringEncodingConverterPlatformSpecific = 4, // Other fields are ignored
                StringEncodingConverterICU = 5 // Other fields are ignored
            };
            
            /* StringEncodingConverterStandard */
            typedef Index (*StringEncodingToBytesProc)(uint32_t flags, const UniChar *characters, Index numChars, uint8_t *bytes, Index maxByteLen, Index *usedByteLen);
            typedef Index (*StringEncodingToUnicodeProc)(uint32_t flags, const uint8_t *bytes, Index numBytes, UniChar *characters, Index maxCharLen, Index *usedCharLen);
            /* StringEncodingConverterCheapEightBit */
            typedef bool (*StringEncodingCheapEightBitToBytesProc)(uint32_t flags, UniChar character, uint8_t *byte);
            typedef bool (*StringEncodingCheapEightBitToUnicodeProc)(uint32_t flags, uint8_t byte, UniChar *character);
            /* StringEncodingConverterStandardEightBit */
            typedef uint16_t (*StringEncodingStandardEightBitToBytesProc)(uint32_t flags, const UniChar *characters, Index numChars, uint8_t *byte);
            typedef uint16_t (*StringEncodingStandardEightBitToUnicodeProc)(uint32_t flags, uint8_t byte, UniChar *characters);
            /* StringEncodingConverterCheapMultiByte */
            typedef uint16_t (*StringEncodingCheapMultiByteToBytesProc)(uint32_t flags, UniChar character, uint8_t *bytes);
            typedef uint16_t (*StringEncodingCheapMultiByteToUnicodeProc)(uint32_t flags, const uint8_t *bytes, Index numBytes, UniChar *character);
            
            typedef Index (*StringEncodingToBytesLenProc)(uint32_t flags, const UniChar *characters, Index numChars);
            typedef Index (*StringEncodingToUnicodeLenProc)(uint32_t flags, const uint8_t *bytes, Index numBytes);
            
            typedef Index (*StringEncodingToBytesPrecomposeProc)(uint32_t flags, const UniChar *character, Index numChars, uint8_t *bytes, Index maxByteLen, Index *usedByteLen);
            typedef bool (*StringEncodingIsValidCombiningCharacterProc)(UniChar character);
            
            typedef struct {
                void *toBytes;
                void *toUnicode;
                uint16_t maxBytesPerChar;
                uint16_t maxDecomposedCharLen;
                uint8_t encodingClass;
                uint32_t :24;
                StringEncodingToBytesLenProc toBytesLen;
                StringEncodingToUnicodeLenProc toUnicodeLen;
                StringEncodingToBytesFallbackProc toBytesFallback;
                StringEncodingToUnicodeFallbackProc toUnicodeFallback;
                StringEncodingToBytesPrecomposeProc toBytesPrecompose;
                StringEncodingIsValidCombiningCharacterProc isValidCombiningChar;
            } StringEncodingConverter;
            
            extern const StringEncodingConverter *StringEncodingGetConverter(String::Encoding encoding);
            
            enum {
                StringEncodingGetConverterSelector = 0,
                StringEncodingIsDecomposableCharacterSelector = 1,
                StringEncodingDecomposeCharacterSelector = 2,
                StringEncodingIsValidLatin1CombiningCharacterSelector = 3,
                StringEncodingPrecomposeLatin1CharacterSelector = 4
            };
            
            extern const void *StringEncodingGetAddressForSelector(uint32_t selector);
            
#define BOOTSTRAPFUNC_NAME	StringEncodingBootstrap
            typedef const StringEncodingConverter* (*StringEncodingBootstrapProc)(uint32_t encoding, const void *getSelector);
            
            /* Latin precomposition */
            /* This function does not precompose recursively nor to U+01E0 and U+01E1.
             */
            extern bool StringEncodingIsValidCombiningCharacterForLatin1(UniChar character);
            extern UniChar StringEncodingPrecomposeLatinCharacter(const UniChar *character, Index numChars, Index *usedChars);
            
            /* Convenience functions for converter development */
            typedef struct _StringEncodingUnicodeTo8BitCharMap {
                UniChar _u;
                uint8_t _c;
                uint8_t :8;
            } StringEncodingUnicodeTo8BitCharMap;
            
            /* Binary searches StringEncodingUnicodeTo8BitCharMap */
            inline bool StringEncodingUnicodeTo8BitEncoding(const StringEncodingUnicodeTo8BitCharMap *theTable, Index numElem, UniChar character, uint8_t *ch) {
                const StringEncodingUnicodeTo8BitCharMap *p, *q, *divider;
                
                if ((character < theTable[0]._u) || (character > theTable[numElem-1]._u)) {
                    return 0;
                }
                p = theTable;
                q = p + (numElem-1);
                while (p <= q) {
                    divider = p + ((q - p) >> 1);	/* divide by 2 */
                    if (character < divider->_u) { q = divider - 1; }
                    else if (character > divider->_u) { p = divider + 1; }
                    else { *ch = divider->_c; return 1; }
                }
                return 0;
            }

        }
    }
}

#endif /* defined(__RSCoreFoundation__StringConverterExt__) */
