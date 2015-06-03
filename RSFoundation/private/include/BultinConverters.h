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
            
            class BuiltinConverters : public Object, public NotCopyable {
            public:
                static String::Encoding *CreateListOfAvailablePlatformConverters(Index *numberOfConverters);
                static const StringEncodingConverter *GetExternalConverter(String::Encoding encoding);
                static Index PlatformUnicodeToBytes(String::Encoding encoding, uint32_t flags, const UniChar *characters, Index numChars, Index *usedCharLen, uint8_t *bytes, Index maxByteLen, Index *usedByteLen);
                static Index PlatformBytesToUnicode(String::Encoding encoding, uint32_t flags, const uint8_t *bytes, Index numBytes, Index *usedByteLen, UniChar *characters, Index maxCharLen, Index *usedCharLen);;
                static Index PlatformCharLengthForBytes(String::Encoding encoding, uint32_t flags, const uint8_t *bytes, Index numBytes);
                static Index PlatformByteLengthForCharacters(String::Encoding encoding, uint32_t flags, const UniChar *characters, Index numChars);
            };
        }
    }
}

#endif /* defined(__RSCoreFoundation__BultinConverters__) */
