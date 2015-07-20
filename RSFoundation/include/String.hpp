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
        String(String& str);
        
        String& operator=(const String& str);
        String& operator=(String& str);
        String& operator=(RSStringRef str);
        String& operator=(RSMutableStringRef str);
        
        String& operator=(String&& str);
        ~String();
        
        String(const char *utf8String);
        String(const void *data, const RSStringEncoding encoding);
        String(RSStringRef stringRef);
        String(RSMutableStringRef mutableStringRef);
        String(const String &str, RSRange range);
        
    public:
        void retain() const;
        void release() const;
        
    public:
        RSIndex getLength() const;
        RSRange getRange() const;
        
    public:
        enum CompareFlags : RSInteger {
            CaseInsensitive = 1,
            Backwards = 4,		/* Starting from the end of the string */
            Anchored = 8,         /* Only at the specified starting point */
            Nonliteral = 16,		/* If specified, loose equivalence is performed (o-umlaut == o, umlaut) */
            Localized = 32,		/* User's default locale is used for the comparisons */
            Numerically = 64		/* Numeric comparison is used RS_AVAILABLE(0_0); that is, Foo2.txt < Foo7.txt < Foo25.txt */
            ,
            DiacriticInsensitive = 128, /* If specified, ignores diacritics (o-umlaut == o) */
            WidthInsensitive = 256, /* If specified, ignores width differences ('a' == UFF41) */
            ForcedOrdering = 512 /* If specified, comparisons are forced to return either RSCompareLessThan or RSCompareGreaterThan if the strings are equivalent but not strictly equal, for stability when sorting (e.g. "aaa" > "AAA" with RSCompareCaseInsensitive specified) */
        };
        bool find(const String &toFind, RSRange searchRange, RSRange &result) const;
        bool find(const String &toFind, RSRange searchRange, CompareFlags compareFlags, RSRange &result) const;
        
    public:
        
    public:
        bool operator==(const String& str) const;
        bool operator==(String &str) const;
        bool operator==(const char* utf8String) const;
        bool operator==(RSStringRef stringRef) const;
        bool operator==(RSMutableStringRef mutableStringRef) const;
        explicit operator bool() const;
        
    private:
        friend class StringM;
        
        inline RSStringRef __getI() const {
            return static_cast<RSStringRef>(__impI);
        }
        
        inline RSMutableStringRef __getM() const {
            return static_cast<RSMutableStringRef>(__impM);
        }
        union {
            const void *__impI;
            void *__impM;
        };
    };
    
    class StringI : public String {
        
    };
    
    class StringM : public String {
    public:
        StringM();
        StringM(const String& str);
        StringM(String& str);
        
        StringM& operator=(const String& str);
        StringM& operator=(String& str);
        StringM& operator=(RSStringRef str);
        StringM& operator=(RSMutableStringRef str);
        
        StringM& operator=(String&& str);
        ~StringM();
        
        StringM(const char *utf8String);
        StringM(const void *data, const RSStringEncoding encoding);
        StringM(RSStringRef stringRef);
        StringM(RSMutableStringRef mutableStringRef);
        StringM(const String &str, RSRange range);
        
        StringM(const StringM& str);
        StringM(StringM& str);
        
        StringM& operator=(const StringM& str);
        StringM& operator=(StringM& str);
        
        StringM& operator=(StringM&& str);
    public:
        void trim(const String &trimString);
    };
}

#endif /* String_cpp */
