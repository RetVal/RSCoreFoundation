//
//  ICUConverters.h
//  RSCoreFoundation
//
//  Created by closure on 6/3/15.
//  Copyright (c) 2015 RetVal. All rights reserved.
//

#ifndef __RSCoreFoundation__ICUConverters__
#define __RSCoreFoundation__ICUConverters__

#include <RSFoundation/String.hpp>

namespace RSFoundation {
    namespace Collection {
        namespace Encoding {
            extern const char *__StringEncodingGetICUName(String::Encoding encoding);
            
            extern String::Encoding __StringEncodingGetFromICUName(const char *icuName);
            
            
            extern RSIndex __StringEncodingICUToBytes(const char *icuName, uint32_t flags, const UniChar *characters, RSIndex numChars, RSIndex *usedCharLen, uint8_t *bytes, RSIndex maxByteLen, RSIndex *usedByteLen);
            extern RSIndex __StringEncodingICUToUnicode(const char *icuName, uint32_t flags, const uint8_t *bytes, RSIndex numBytes, RSIndex *usedByteLen, UniChar *characters, RSIndex maxCharLen, RSIndex *usedCharLen);
            extern RSIndex __StringEncodingICUCharLength(const char *icuName, uint32_t flags, const uint8_t *bytes, RSIndex numBytes);
            extern RSIndex __StringEncodingICUByteLength(const char *icuName, uint32_t flags, const UniChar *characters, RSIndex numChars);
            
            // The caller is responsible for freeing the memory (use AllocatorDeallocate)
            extern String::Encoding *__StringEncodingCreateICUEncodings(RSIndex *numberOfRSIndex);
        }
    }
}

#endif /* defined(__RSCoreFoundation__ICUConverters__) */
