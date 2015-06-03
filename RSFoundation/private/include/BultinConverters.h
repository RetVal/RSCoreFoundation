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
            
            extern  String::Encoding *__StringEncodingCreateListOfAvailablePlatformConverters(Index *numberOfConverters);
            extern  const StringEncodingConverter *__StringEncodingGetExternalConverter(String::Encoding encoding);
            extern  Index __StringEncodingPlatformUnicodeToBytes(String::Encoding encoding, uint32_t flags, const UniChar *characters, Index numChars, Index *usedCharLen, uint8_t *bytes, Index maxByteLen, Index *usedByteLen);
            extern  Index __StringEncodingPlatformBytesToUnicode(String::Encoding encoding, uint32_t flags, const uint8_t *bytes, Index numBytes, Index *usedByteLen, UniChar *characters, Index maxCharLen, Index *usedCharLen);
            extern  Index __StringEncodingPlatformCharLengthForBytes(String::Encoding encoding, uint32_t flags, const uint8_t *bytes, Index numBytes);
            extern  Index __StringEncodingPlatformByteLengthForCharacters(String::Encoding encoding, uint32_t flags, const UniChar *characters, Index numChars);
        }
    }
}

#endif /* defined(__RSCoreFoundation__BultinConverters__) */
