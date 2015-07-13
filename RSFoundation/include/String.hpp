//
//  String.hpp
//  RSCoreFoundation
//
//  Created by closure on 7/7/15.
//  Copyright © 2015 RetVal. All rights reserved.
//

#ifndef String_cpp
#define String_cpp

#include <RSFoundation/Object.hpp>

namespace RSCF {
    class String : public RefObject {
    public:
        String();
        String(const String& str);
        
        String& operator=(const String& str);
        String& operator=(String&& str);
        ~String();
        
        String(const char *utf8String);
        String(const void *data, const RSStringEncoding encoding);
        String(RSStringRef stringRef);
        String(RSMutableStringRef mutableStringRef);
        
    public:
        void retain() const;
        void release() const;
        RSIndex getLength() const;
        
    public:
        bool operator==(const String& str) const;
        bool operator==(const char* utf8String) const;
        bool operator==(RSStringRef stringRef) const;
        bool operator==(RSMutableStringRef mutableStringRef) const;
        explicit operator bool() const;
        
    private:
        const void *__impl;
    };
}

#endif /* String_cpp */
