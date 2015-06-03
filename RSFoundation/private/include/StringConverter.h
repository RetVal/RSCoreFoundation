//
//  StringConverter.h
//  RSCoreFoundation
//
//  Created by closure on 6/2/15.
//  Copyright (c) 2015 RetVal. All rights reserved.
//

#ifndef __RSCoreFoundation__StringConverter__
#define __RSCoreFoundation__StringConverter__

#include <RSFoundation/String.hpp>

namespace RSFoundation {
    namespace Collection {
        namespace Encoding {
            /* Macro to shift lossByte argument.
             */
#define StringEncodingLossyByteToMask(lossByte)	((uint32_t)(lossByte << 24)|StringEncodingAllowLossyConversion)
#define StringEncodingMaskToLossyByte(flags)		((uint8_t)(flags >> 24))
            
            /* Macros for streaming support
             */
#define StringEncodingStreamIDMask                    (0x00FF0000)
#define StringEncodingStreamIDFromMask(mask)    ((mask >> 16) & 0xFF)
#define StringEncodingStreamIDToMask(identifier)            ((uint32_t)((identifier & 0xFF) << 16))
            
            /* Converts characters into the specified encoding.  Returns the constants defined above.
             If maxByteLen is 0, bytes is ignored. You can pass lossyByte by passing the value in flags argument.
             i.e. StringEncodingUnicodeToBytes(encoding, StringEncodingLossyByteToMask(lossByte), ....)
             */
            uint32_t StringEncodingUnicodeToBytes(String::Encoding encoding, uint32_t flags, const UniChar *characters, Index numChars, Index *usedCharLen, uint8_t *bytes, Index maxByteLen, Index *usedByteLen);
            
            /* Converts bytes in the specified encoding into unicode.  Returns the constants defined above.
             maxCharLen & usdCharLen are in UniChar length, not byte length.
             If maxCharLen is 0, characters is ignored.
             */
            uint32_t StringEncodingBytesToUnicode(String::Encoding encoding, uint32_t flags, const uint8_t *bytes, Index numBytes, Index *usedByteLen, UniChar *characters, Index maxCharLen, Index *usedCharLen);
            
            /* Fallback functions used when allowLossy
             */
            typedef Index (*StringEncodingToBytesFallbackProc)(const UniChar *characters, Index numChars, uint8_t *bytes, Index maxByteLen, Index *usedByteLen);
            typedef Index (*StringEncodingToUnicodeFallbackProc)(const uint8_t *bytes, Index numBytes, UniChar *characters, Index maxCharLen, Index *usedCharLen);
            
            bool StringEncodingIsValidEncoding(String::Encoding encoding);
            
            /* Returns StringEncodingInvalidId terminated encoding list
             */
            const String::Encoding *StringEncodingListOfAvailableEncodings(void);
            
            /* Returns required length of destination buffer for conversion.  These functions are faster than specifying 0 to maxByteLen (maxCharLen), but unnecessarily optimal length
             */
            Index StringEncodingCharLengthForBytes(String::Encoding encoding, uint32_t flags, const uint8_t *bytes, Index numBytes);
            Index StringEncodingByteLengthForCharacters(String::Encoding encoding, uint32_t flags, const UniChar *characters, Index numChars);
            
            /* Can register functions used for lossy conversion.  Reregisters default procs if nil
             */
            void StringEncodingRegisterFallbackProcedures(String::Encoding encoding, StringEncodingToBytesFallbackProc toBytes, StringEncodingToUnicodeFallbackProc toUnicode);
        }
    }
}

#endif /* defined(__RSCoreFoundation__StringConverter__) */
