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
            class ICUConverters : public Object, public NotCopyable {
            private:
                ICUConverters() {}
                ~ICUConverters() {}
            public:
                static const char* GetICUName(String::Encoding encoding);
                static String::Encoding GetFromICUName(const char *icuName);
                static Index ICUToBytes(const char *icuName, uint32_t flags, const UniChar *characters, Index numChars, Index *usedCharLen, uint8_t *bytes, Index maxByteLen, Index *usedByteLen);
                static Index ICUToUnicode(const char *icuName, uint32_t flags, const uint8_t *bytes, Index numBytes, Index *usedByteLen, UniChar *characters, Index maxCharLen, Index *usedCharLen);
                static Index ICUCharLength(const char *icuName, uint32_t flags, const uint8_t *bytes, Index numBytes);
                static Index ICUByteLength(const char *icuName, uint32_t flags, const UniChar *characters, Index numChars);
                static String::Encoding* CreateICUEncodings(Index *numberOfIndex);
            };
        }
    }
}

#endif /* defined(__RSCoreFoundation__ICUConverters__) */
