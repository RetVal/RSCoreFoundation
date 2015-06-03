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
            
            
            extern Index __StringEncodingICUToBytes(const char *icuName, uint32_t flags, const UniChar *characters, Index numChars, Index *usedCharLen, uint8_t *bytes, Index maxByteLen, Index *usedByteLen);
            extern Index __StringEncodingICUToUnicode(const char *icuName, uint32_t flags, const uint8_t *bytes, Index numBytes, Index *usedByteLen, UniChar *characters, Index maxCharLen, Index *usedCharLen);
            extern Index __StringEncodingICUCharLength(const char *icuName, uint32_t flags, const uint8_t *bytes, Index numBytes);
            extern Index __StringEncodingICUByteLength(const char *icuName, uint32_t flags, const UniChar *characters, Index numChars);
            
            // The caller is responsible for freeing the memory (use AllocatorDeallocate)
            extern String::Encoding *__StringEncodingCreateICUEncodings(Index *numberOfIndex);
        }
    }
}

#endif /* defined(__RSCoreFoundation__ICUConverters__) */
