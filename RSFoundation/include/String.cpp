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
        __impI = RSRetain(RSStringGetEmptyString());
    }
    
    String::String(const String &str) {
        __impI = RSRetain(str.__getI());
    }
    
    String::String(String &str) {
        __impI = RSCopy(RSAllocatorDefault, str.__getI());
    }
    
    String& String::operator=(const String &str) {
        if (this == &str) {
            return *this;
        }
        if (__getI()) {
            RSRelease(__getI());
        }
        __impI = RSRetain(str.__getI());
        return *this;
    }
    
    String& String::operator=(String &str) {
        if (this == &str) {
            return *this;
        }
        if (__getI()) {
            RSRelease(__getI());
        }
        __impI = RSCopy(RSAllocatorDefault, str.__getI());
        return *this;
    }
    
    String& String::operator=(RSStringRef str) {
        if (__getI() == str) {
            return *this;
        }
        if (__getI()) {
            RSRelease(__getI());
        }
        if (str) {
            __impI = RSRetain(str);
        }
        return *this;
    }
    
    String& String::operator=(RSMutableStringRef str) {
        if (__getI() == str) {
            return *this;
        }
        if (__getI()) {
            RSRelease(__getI());
        }
        if (str) {
            __impI = RSCopy(RSAllocatorDefault, str);
        }
        return *this;
    }
    
    String& String::operator=(String &&str) {
        assert(this != &str);
        if (__getI()) {
            RSRelease(__getI());
        }
        __impI = RSRetain(str.__getI());
        RSRelease(str.__getI());
        str.__impI = nullptr;
        return *this;
    }
    
    String::~String() {
        if (__getI()) {
            RSRelease(__getI());
            __impI = nullptr;
        }
    }
    
    String::String(const char *utf8String) {
        __impI = RSRetain(__RSStringMakeConstantString(utf8String));
//        __impl = RSStringCreateWithCString(RSAllocatorDefault, utf8String, RSStringEncodingUTF8);
    }
    
    String::String(const void *data, const RSStringEncoding encoding) {
        __impI = RSStringCreateWithCString(RSAllocatorDefault, (const char*)data, encoding);
    }
    
    String::String(RSStringRef stringRef) {
        if (stringRef == nullptr) stringRef = RSStringGetEmptyString();
        __impI = RSRetain(stringRef);
    }
    
    String::String(RSMutableStringRef mutableStringRef) {
        if (mutableStringRef == nullptr) {
            __impI = RSRetain(RSStringGetEmptyString());
        } else {
            __impI = RSCopy(RSAllocatorDefault, mutableStringRef);
        }
    }
    
    String::String(const String &str, RSRange range) {
        __impI = RSStringCreateWithSubstring(RSAllocatorDefault, str.__getI(), range);
    }
    
    RSIndex String::getLength() const {
        if (nullptr == __getI()) return 0;
        return RSStringGetLength(__getI());
    }
    
    RSRange String::getRange() const {
        return RSStringGetRange(__getI());
    }
    
    bool String::find(const String &toFind, RSRange searchRange, RSRange &result) const {
        return find(toFind, searchRange, CaseInsensitive, result);
    }
    
    bool String::find(const String &toFind, RSRange searchRange, CompareFlags compareFlags, RSRange &result) const {
        return RSStringFindWithOptions(__getI(), toFind.__getI(), searchRange, compareFlags, &result);
    }
    
    bool String::operator==(const String &str) const {
        return RSCompareEqualTo == RSStringCompare(__getI(), str.__getI(), nullptr);
    }
    
    bool String::operator==(String &str) const {
        return RSCompareEqualTo == RSStringCompare(__getI(), str.__getI(), nullptr);
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
        return __impI != nullptr;
    }
    
    void String::retain() const {
        if (__impI) {
            RSRetain(__getI());
        }
        RefObject::retain();
    }
    
    void String::release() const {
        if (__impI) {
            RSRelease(__getI());
        }
        RefObject::release();
    }
    
    StringM::StringM() {
        __impM = RSStringCreateMutable(RSAllocatorDefault, 0);
    }
    
    StringM::StringM(const String& str) {
        __impM = (RSMutableStringRef)RSMutableCopy(RSAllocatorDefault, str.__getI());
    }
    
    StringM::StringM(String& str) {
        __impM = RSMutableCopy(RSAllocatorDefault, str.__getI());
    }
    
    StringM& StringM::operator=(const String& str) {
        if (this == &str) {
            return *this;
        }
        if (__getI()) {
            RSRelease(__getI());
        }
        __impM = RSMutableCopy(RSAllocatorDefault, str.__getI());
        return *this;
    }
    
    StringM& StringM::operator=(String& str) {
        if (this == &str) {
            return *this;
        }
        if (__getI()) {
            RSRelease(__getI());
        }
        __impM = RSMutableCopy(RSAllocatorDefault, str.__getI());
        return *this;
    }
    
    StringM& StringM::operator=(RSStringRef str) {
        if (__getI() == str) {
            return *this;
        }
        if (__getI()) {
            RSRelease(__getI());
        }
        __impM = RSMutableCopy(RSAllocatorDefault, str);
        return *this;
    }
    
    StringM& StringM::operator=(RSMutableStringRef str) {
        if (__getM() == str) {
            return *this;
        }
        if (__getM()) {
            RSRelease(__getM());
        }
        __impM = RSMutableCopy(RSAllocatorDefault, str);
        return *this;
    }
    
    StringM& StringM::operator=(String&& str) {
        assert(this != &str);
        if (__getM()) {
            RSRelease(__getM());
        }
        __impM = RSMutableCopy(RSAllocatorDefault, str.__getI());
        RSRelease(str.__getI());
        str.__impI = nullptr;
        return *this;
    }
    
    StringM::~StringM() {
        String::~String();
    }
    
    StringM::StringM(const char *utf8String) {
        __impM = RSStringCreateMutable(RSAllocatorDefault, 0);
        RSStringAppendString(__getM(), String(utf8String).__getI());
    }
    
    StringM::StringM(const void *data, const RSStringEncoding encoding) {
        __impM = RSStringCreateMutable(RSAllocatorDefault, 0);
        RSStringAppendString(__getM(), String(data, encoding).__getI());
    }
    
    StringM::StringM(RSStringRef stringRef) {
        __impM = RSMutableCopy(RSAllocatorDefault, stringRef);
    }
    
    StringM::StringM(RSMutableStringRef mutableStringRef) {
        __impM = RSMutableCopy(RSAllocatorDefault, mutableStringRef);
    }
    
    StringM::StringM(const String &str, RSRange range) {
        __impI = RSStringCreateWithSubstring(RSAllocatorDefault, str.__getI(), range);
    }
    
    StringM::StringM(const StringM& str) {
        __impM = RSMutableCopy(RSAllocatorDefault, str.__getM());
    }
    
    StringM::StringM(StringM& str) {
        __impM = RSMutableCopy(RSAllocatorDefault, str.__getM());
    }
    
    StringM& StringM::operator=(const StringM& str) {
        if (__getM()) {
            RSRelease(__getM());
            __impM = nullptr;
        }
        __impM = RSMutableCopy(RSAllocatorDefault, str.__getM());
        return *this;
    }
    
    StringM& StringM::operator=(StringM& str) {
        if (__getM()) {
            RSRelease(__getM());
            __impM = nullptr;
        }
        __impM = RSMutableCopy(RSAllocatorDefault, str.__getM());
        return *this;
    }
    
    StringM& StringM::operator=(StringM&& str) {
        assert(this != &str);
        if (__getI()) {
            RSRelease(__getI());
        }
        __impM = (RSMutableStringRef)RSRetain(str.__getM());
        RSRelease(str.__getM());
        str.__impM = nullptr;
        return *this;
    }
    
    void StringM::trim(const String &trimString) {
        assert(__impM && "string is empty");
        RSStringTrim(__getM(), trimString.__getI());
    }
}


