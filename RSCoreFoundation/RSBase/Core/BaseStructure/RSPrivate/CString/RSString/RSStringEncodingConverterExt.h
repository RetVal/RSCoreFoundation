//
//  RSStringEncodingConverterExt.h
//  RSCoreFoundation
//
//  Created by RetVal on 7/28/13.
//  Copyright (c) 2013 RetVal. All rights reserved.
//

#ifndef RSCoreFoundation_RSStringEncodingConverterExt_h
#define RSCoreFoundation_RSStringEncodingConverterExt_h

#if !defined(__RSCOREFOUNDATION_RSSTRINGENCODINGCONVERETEREXT__)
#define __RSCOREFOUNDATION_RSSTRINGENCODINGCONVERETEREXT__ 1

#include "RSStringEncodingConverter.h"

RS_EXTERN_C_BEGIN

#define MAX_DECOMPOSED_LENGTH (10)

enum {
    RSStringEncodingConverterStandard = 0,
    RSStringEncodingConverterCheapEightBit = 1,
    RSStringEncodingConverterStandardEightBit = 2,
    RSStringEncodingConverterCheapMultiByte = 3,
    RSStringEncodingConverterPlatformSpecific = 4, // Other fields are ignored
    RSStringEncodingConverterICU = 5 // Other fields are ignored
};

/* RSStringEncodingConverterStandard */
typedef RSIndex (*RSStringEncodingToBytesProc)(uint32_t flags, const UniChar *characters, RSIndex numChars, uint8_t *bytes, RSIndex maxByteLen, RSIndex *usedByteLen);
typedef RSIndex (*RSStringEncodingToUnicodeProc)(uint32_t flags, const uint8_t *bytes, RSIndex numBytes, UniChar *characters, RSIndex maxCharLen, RSIndex *usedCharLen);
/* RSStringEncodingConverterCheapEightBit */
typedef BOOL (*RSStringEncodingCheapEightBitToBytesProc)(uint32_t flags, UniChar character, uint8_t *byte);
typedef BOOL (*RSStringEncodingCheapEightBitToUnicodeProc)(uint32_t flags, uint8_t byte, UniChar *character);
/* RSStringEncodingConverterStandardEightBit */
typedef uint16_t (*RSStringEncodingStandardEightBitToBytesProc)(uint32_t flags, const UniChar *characters, RSIndex numChars, uint8_t *byte);
typedef uint16_t (*RSStringEncodingStandardEightBitToUnicodeProc)(uint32_t flags, uint8_t byte, UniChar *characters);
/* RSStringEncodingConverterCheapMultiByte */
typedef uint16_t (*RSStringEncodingCheapMultiByteToBytesProc)(uint32_t flags, UniChar character, uint8_t *bytes);
typedef uint16_t (*RSStringEncodingCheapMultiByteToUnicodeProc)(uint32_t flags, const uint8_t *bytes, RSIndex numBytes, UniChar *character);

typedef RSIndex (*RSStringEncodingToBytesLenProc)(uint32_t flags, const UniChar *characters, RSIndex numChars);
typedef RSIndex (*RSStringEncodingToUnicodeLenProc)(uint32_t flags, const uint8_t *bytes, RSIndex numBytes);

typedef RSIndex (*RSStringEncodingToBytesPrecomposeProc)(uint32_t flags, const UniChar *character, RSIndex numChars, uint8_t *bytes, RSIndex maxByteLen, RSIndex *usedByteLen);
typedef BOOL (*RSStringEncodingIsValidCombiningCharacterProc)(UniChar character);

typedef struct {
    void *toBytes;
    void *toUnicode;
    uint16_t maxBytesPerChar;
    uint16_t maxDecomposedCharLen;
    uint8_t encodingClass;
    uint32_t :24;
    RSStringEncodingToBytesLenProc toBytesLen;
    RSStringEncodingToUnicodeLenProc toUnicodeLen;
    RSStringEncodingToBytesFallbackProc toBytesFallback;
    RSStringEncodingToUnicodeFallbackProc toUnicodeFallback;
    RSStringEncodingToBytesPrecomposeProc toBytesPrecompose;
    RSStringEncodingIsValidCombiningCharacterProc isValidCombiningChar;
} RSStringEncodingConverter;

extern const RSStringEncodingConverter *RSStringEncodingGetConverter(uint32_t encoding);

enum {
    RSStringEncodingGetConverterSelector = 0,
    RSStringEncodingIsDecomposableCharacterSelector = 1,
    RSStringEncodingDecomposeCharacterSelector = 2,
    RSStringEncodingIsValidLatin1CombiningCharacterSelector = 3,
    RSStringEncodingPrecomposeLatin1CharacterSelector = 4
};

extern const void *RSStringEncodingGetAddressForSelector(uint32_t selector);

#define BOOTSTRAPFUNC_NAME	RSStringEncodingBootstrap
typedef const RSStringEncodingConverter* (*RSStringEncodingBootstrapProc)(uint32_t encoding, const void *getSelector);

/* Latin precomposition */
/* This function does not precompose recursively nor to U+01E0 and U+01E1.
 */
extern BOOL RSStringEncodingIsValidCombiningCharacterForLatin1(UniChar character);
extern UniChar RSStringEncodingPrecomposeLatinCharacter(const UniChar *character, RSIndex numChars, RSIndex *usedChars);

/* Convenience functions for converter development */
typedef struct _RSStringEncodingUnicodeTo8BitCharMap {
    UniChar _u;
    uint8_t _c;
    uint8_t :8;
} RSStringEncodingUnicodeTo8BitCharMap;

/* Binary searches RSStringEncodingUnicodeTo8BitCharMap */
RSInline BOOL RSStringEncodingUnicodeTo8BitEncoding(const RSStringEncodingUnicodeTo8BitCharMap *theTable, RSIndex numElem, UniChar character, uint8_t *ch) {
    const RSStringEncodingUnicodeTo8BitCharMap *p, *q, *divider;
    
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


RS_EXTERN_C_END

#endif /* ! __RSCOREFOUNDATION_RSSTRINGENCODINGCONVERETEREXT__ */
#endif
