//
//  BultinConverters.h
//  RSCoreFoundation
//
//  Created by closure on 6/3/15.
//  Copyright (c) 2015 RetVal. All rights reserved.
//

#ifndef __RSCoreFoundation__BultinConverters__
#define __RSCoreFoundation__BultinConverters__

#include <RSFoundation/String.hpp>
#include <RSFoundation/StringConverterExt.h>

namespace RSFoundation {
    namespace Collection {
        namespace Encoding {
            extern  const StringEncodingConverter __ConverterASCII;
            extern  const StringEncodingConverter __ConverterISOLatin1;
            extern  const StringEncodingConverter __ConverterMacRoman;
            extern  const StringEncodingConverter __ConverterWinLatin1;
            extern  const StringEncodingConverter __ConverterNextStepLatin;
            extern  const StringEncodingConverter __ConverterUTF8;
            
            extern  String::Encoding *__StringEncodingCreateListOfAvailablePlatformConverters(RSIndex *numberOfConverters);
            extern  const StringEncodingConverter *__StringEncodingGetExternalConverter(String::Encoding encoding);
            extern  RSIndex __StringEncodingPlatformUnicodeToBytes(String::Encoding encoding, uint32_t flags, const UniChar *characters, RSIndex numChars, RSIndex *usedCharLen, uint8_t *bytes, RSIndex maxByteLen, RSIndex *usedByteLen);
            extern  RSIndex __StringEncodingPlatformBytesToUnicode(String::Encoding encoding, uint32_t flags, const uint8_t *bytes, RSIndex numBytes, RSIndex *usedByteLen, UniChar *characters, RSIndex maxCharLen, RSIndex *usedCharLen);
            extern  RSIndex __StringEncodingPlatformCharLengthForBytes(String::Encoding encoding, uint32_t flags, const uint8_t *bytes, RSIndex numBytes);
            extern  RSIndex __StringEncodingPlatformByteLengthForCharacters(String::Encoding encoding, uint32_t flags, const UniChar *characters, RSIndex numChars);
        }
    }
}

#endif /* defined(__RSCoreFoundation__BultinConverters__) */
