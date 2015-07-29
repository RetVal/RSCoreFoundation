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
        
        String(const RSStringEncoding encoding, const void *data);
        String(RSStringRef stringRef);
        String(RSMutableStringRef mutableStringRef);
        
    public:
        static const String &format(const char *format, ...); // format can not be nil
        static const String &format(const char *format, va_list ap);
        
    public:
        void retain() const;
        void release() const;
        
    public:
        RSIndex getLength() const;
        RSRange getRange() const;
        RSHashCode hashValue() const;
        
        const String& subString(RSRange range) const;
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
        bool isMutable() const { return false; }
        
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
    
    class StringI : public RefObject {
    public:
        StringI() {
            ptr = "";
        }
        
        template <int N>
        StringI(char const (&s)[N]) {
            ptr = s;
        }
        
    public:
        StringI(const StringI& str) {
            ptr = str.ptr;
        }
        
        StringI& operator=(const StringI& str) {
            ptr = str.ptr;
            return *this;
        }
        
    private:
        const char *ptr;
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
        StringM(const RSStringEncoding encoding, const void *data);
        StringM(RSStringRef stringRef);
        StringM(RSMutableStringRef mutableStringRef);
        StringM(const String &str, RSRange range);
        
        StringM(const StringM& str);
        StringM(StringM& str);
        
        StringM& operator=(const StringM& str);
        StringM& operator=(StringM& str);
        
        StringM& operator=(StringM&& str);
    public:
        bool isMutable() const { return true; }
    public:
        void trim(const String &trimString);
    };
}

#include <RSFoundation/DictionaryInfo.hpp>

namespace RSCF {
    // Provide DictionaryInfo for ints.
    template<> struct DictionaryInfo<String> {
        static inline String getEmptyKey() { return String(); }
        static inline String getTombstoneKey() { return String(); }
        static RSHashCode getHashValue(const String& Val) { return Val.hashValue(); }
        static bool isEqual(const String& LHS, const String& RHS) {
            return LHS == RHS;
        }
    };
}

#endif /* String_cpp */
