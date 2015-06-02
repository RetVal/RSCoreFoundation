//
//  DataBase.h
//  RSCoreFoundation
//
//  Created by closure on 6/2/15.
//  Copyright (c) 2015 RetVal. All rights reserved.
//

#ifndef __CoreFoundation__DataBase__
#define __CoreFoundation__DataBase__

#include <RSFoundation/String.hpp>

namespace RSFoundation {
    namespace Collection {
        namespace Encoding {
            extern uint16_t __StringEncodingGetWindowsCodePage(String::Encoding encoding);
            extern String::Encoding __StringEncodingGetFromWindowsCodePage(uint16_t codepage);
            
            extern bool __StringEncodingGetCanonicalName(String::Encoding encoding, char *buffer, RSIndex bufferSize);
            extern String::Encoding __StringEncodingGetFromCanonicalName(const char *canonicalName);
            
            extern String::Encoding __StringEncodingGetMostCompatibleMacScript(String::Encoding encoding);
            
            extern const char *__StringEncodingGetName(String::Encoding encoding); // Returns simple non-localizd name
        }
    }
}

#endif /* defined(__CoreFoundation__DataBase__) */
