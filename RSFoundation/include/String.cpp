//
//  String.cpp
//  RSCoreFoundation
//
//  Created by closure on 7/7/15.
//  Copyright © 2015 RetVal. All rights reserved.
//

#include "String.hpp"
#include <type_traits>

namespace RSCF {
    String::String() {
        __impl = RSRetain(RSStringGetEmptyString());
    }
    
    String::String(const String &str) {
        if (__impl) {
            RSRelease(__impl);
        }
        __impl = RSRetain(str.__impl);
    }
    
    String& String::operator=(const String &str) {
        if (this == &str) {
            return *this;
        }
        if (__impl) {
            RSRelease(__impl);
        }
        __impl = RSRetain(str.__impl);
        return *this;
    }
    
    String& String::operator=(String &&str) {
        assert(this != &str);
        if (__impl) {
            RSRelease(__impl);
        }
        __impl = RSRetain(str.__impl);
        RSRelease(str.__impl);
        str.__impl = nullptr;
        return *this;
    }
    
    String::~String() {
        if (__impl) {
            RSRelease(__impl);
            __impl = nullptr;
        }
    }
    
    String::String(const char *utf8String) {
        __impl = RSRetain(__RSStringMakeConstantString(utf8String));
//        __impl = RSStringCreateWithCString(RSAllocatorDefault, utf8String, RSStringEncodingUTF8);
    }
    
    String::String(const void *data, const RSStringEncoding encoding) {
        __impl = RSStringCreateWithCString(RSAllocatorDefault, (const char*)data, encoding);
    }
    
    String::String(RSStringRef stringRef) {
        if (stringRef == nullptr) stringRef = RSStringGetEmptyString();
        __impl = RSRetain(stringRef);
    }
    
    String::String(RSMutableStringRef mutableStringRef) {
        if (mutableStringRef == nullptr) {
            __impl = RSRetain(RSStringGetEmptyString());
        } else {
            __impl = RSCopy(RSAllocatorDefault, mutableStringRef);
        }
    }
    
    RSIndex String::getLength() const {
        if (nullptr == __impl) return 0;
        return RSStringGetLength(static_cast<RSStringRef>(__impl));
    }
    
    bool String::operator==(const String &str) const {
        return RSStringCompare(static_cast<RSStringRef>(__impl), static_cast<RSStringRef>(str.__impl), nullptr);
    }
    
    bool String::operator==(const char *utf8String) const {
        return *this == String(utf8String);
    }
    
    bool String::operator==(RSStringRef stringRef) const {
        return *this == String(stringRef);
    }
    
    bool String::operator==(RSMutableStringRef mutableStringRef) const {
        return *this == String(mutableStringRef);
    }
    
    String::operator bool() const {
        return __impl == nullptr;
    }
    
    void String::retain() const {
        if (__impl) {
            RSRetain(static_cast<RSStringRef>(__impl));
        }
        RefObject::retain();
    }
    
    void String::release() const {
        if (__impl) {
            RSRelease(static_cast<RSStringRef>(__impl));
        }
        RefObject::release();
    }
}
