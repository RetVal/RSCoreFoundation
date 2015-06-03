//
//  String.h
//  RSCoreFoundation
//
//  Created by closure on 5/27/15.
//  Copyright (c) 2015 RetVal. All rights reserved.
//

#ifndef __RSCoreFoundation__String__
#define __RSCoreFoundation__String__

#include <RSFoundation/Object.h>
#include <RSFoundation/Allocator.h>

namespace RSFoundation {
    using namespace Basic;
    namespace Collection {
        class String : public Object {
        public:
            enum Encoding : Index {
                InvalidId = 0xffffffffU,
                MacRoman = 0,
                WindowsLatin1 = 0x0500, /* ANSI codepage 1252 */
                ISOLatin1 = 0x0201, /* ISO 8859-1 */
                NextStepLatin = 0x0B01, /* NextStep encoding*/
                ASCII = 0x0600, /* 0..127 (in creating , values greater than 0x7F are treated as corresponding Unicode value) */
                Unicode = 0x0100, /* kTextEncodingUnicodeDefault  + kTextEncodingDefaultFormat (aka kUnicode16BitFormat) */
                UTF8 = 0x08000100, /* kTextEncodingUnicodeDefault + kUnicodeUTF8Format */
                NonLossyASCII = 0x0BFF, /* 7bit Unicode variants used by Cocoa & Java */
                
                UTF16 = 0x0100, /* kTextEncodingUnicodeDefault + kUnicodeUTF16Format (alias of Unicode) */
                UTF16BE = 0x10000100, /* kTextEncodingUnicodeDefault + kUnicodeUTF16BEFormat */
                UTF16LE = 0x14000100, /* kTextEncodingUnicodeDefault + kUnicodeUTF16LEFormat */
                
                UTF32 = 0x0c000100, /* kTextEncodingUnicodeDefault + kUnicodeUTF32Format */
                UTF32BE = 0x18000100, /* kTextEncodingUnicodeDefault + kUnicodeUTF32BEFormat */
                UTF32LE = 0x1c000100, /* kTextEncodingUnicodeDefault + kUnicodeUTF32LEFormat */
                /*  MacRoman = 0L, defined in CoreFoundation/.h */
                MacJapanese = 1,
                MacChineseTrad = 2,
                MacKorean = 3,
                MacArabic = 4,
                MacHebrew = 5,
                MacGreek = 6,
                MacCyrillic = 7,
                MacDevanagari = 9,
                MacGurmukhi = 10,
                MacGujarati = 11,
                MacOriya = 12,
                MacBengali = 13,
                MacTamil = 14,
                MacTelugu = 15,
                MacKannada = 16,
                MacMalayalam = 17,
                MacSinhalese = 18,
                MacBurmese = 19,
                MacKhmer = 20,
                MacThai = 21,
                MacLaotian = 22,
                MacGeorgian = 23,
                MacArmenian = 24,
                MacChineseSimp = 25,
                MacTibetan = 26,
                MacMongolian = 27,
                MacEthiopic = 28,
                MacCentralEurRoman = 29,
                MacVietnamese = 30,
                MacExtArabic = 31,
                /* The following use script code 0, smRoman */
                MacSymbol = 33,
                MacDingbats = 34,
                MacTurkish = 35,
                MacCroatian = 36,
                MacIcelandic = 37,
                MacRomanian = 38,
                MacCeltic = 39,
                MacGaelic = 40,
                /* The following use script code 4, smArabic */
                MacFarsi = 0x8C,	/* Like MacArabic but uses Farsi digits */
                /* The following use script code 7, smCyrillic */
                MacUkrainian = 0x98,
                /* The following use script code 32, smUnimplemented */
                MacInuit = 0xEC,
                MacVT100 = 0xFC,	/* VT100/102 font from Comm Toolbox: Latin-1 repertoire + box drawing etc */
                /* Special Mac OS encodings*/
                MacHFS = 0xFF,	/* Meta-value, should never appear in a table */
                
                /* Unicode & ISO UCS encodings begin at 0x100 */
                /* We don't use Unicode variations defined in TextEncoding; use the ones in .h, instead. */
                
                /* ISO 8-bit and 7-bit encodings begin at 0x200 */
                /*  ISOLatin1 = 0x0201, defined in CoreFoundation/.h */
                ISOLatin2 = 0x0202,	/* ISO 8859-2 */
                ISOLatin3 = 0x0203,	/* ISO 8859-3 */
                ISOLatin4 = 0x0204,	/* ISO 8859-4 */
                ISOLatinCyrillic = 0x0205,	/* ISO 8859-5 */
                ISOLatinArabic = 0x0206,	/* ISO 8859-6, =ASMO 708, =DOS CP 708 */
                ISOLatinGreek = 0x0207,	/* ISO 8859-7 */
                ISOLatinHebrew = 0x0208,	/* ISO 8859-8 */
                ISOLatin5 = 0x0209,	/* ISO 8859-9 */
                ISOLatin6 = 0x020A,	/* ISO 8859-10 */
                ISOLatinThai = 0x020B,	/* ISO 8859-11 */
                ISOLatin7 = 0x020D,	/* ISO 8859-13 */
                ISOLatin8 = 0x020E,	/* ISO 8859-14 */
                ISOLatin9 = 0x020F,	/* ISO 8859-15 */
                ISOLatin10 = 0x0210,	/* ISO 8859-16 */
                
                /* MS-DOS & Windows encodings begin at 0x400 */
                DOSLatinUS = 0x0400,	/* code page 437 */
                DOSGreek = 0x0405,		/* code page 737 (formerly code page 437G) */
                DOSBalticRim = 0x0406,	/* code page 775 */
                DOSLatin1 = 0x0410,	/* code page 850, "Multilingual" */
                DOSGreek1 = 0x0411,	/* code page 851 */
                DOSLatin2 = 0x0412,	/* code page 852, Slavic */
                DOSCyrillic = 0x0413,	/* code page 855, IBM Cyrillic */
                DOSTurkish = 0x0414,	/* code page 857, IBM Turkish */
                DOSPortuguese = 0x0415,	/* code page 860 */
                DOSIcelandic = 0x0416,	/* code page 861 */
                DOSHebrew = 0x0417,	/* code page 862 */
                DOSCanadianFrench = 0x0418, /* code page 863 */
                DOSArabic = 0x0419,	/* code page 864 */
                DOSNordic = 0x041A,	/* code page 865 */
                DOSRussian = 0x041B,	/* code page 866 */
                DOSGreek2 = 0x041C,	/* code page 869, IBM Modern Greek */
                DOSThai = 0x041D,		/* code page 874, also for Windows */
                DOSJapanese = 0x0420,	/* code page 932, also for Windows */
                DOSChineseSimplif = 0x0421, /* code page 936, also for Windows */
                DOSKorean = 0x0422,	/* code page 949, also for Windows; Unified Hangul Code */
                DOSChineseTrad = 0x0423,	/* code page 950, also for Windows */
                /*  WindowsLatin1 = 0x0500, defined in CoreFoundation/.h */
                WindowsLatin2 = 0x0501,	/* code page 1250, Central Europe */
                WindowsCyrillic = 0x0502,	/* code page 1251, Slavic Cyrillic */
                WindowsGreek = 0x0503,	/* code page 1253 */
                WindowsLatin5 = 0x0504,	/* code page 1254, Turkish */
                WindowsHebrew = 0x0505,	/* code page 1255 */
                WindowsArabic = 0x0506,	/* code page 1256 */
                WindowsBalticRim = 0x0507,	/* code page 1257 */
                WindowsVietnamese = 0x0508, /* code page 1258 */
                WindowsKoreanJohab = 0x0510, /* code page 1361, for Windows NT */
                
                /* Various national standards begin at 0x600 */
                /*  ASCII = 0x0600, defined in CoreFoundation/.h */
                ANSEL = 0x0601,	/* ANSEL (ANSI Z39.47) */
                JIS_X0201_76 = 0x0620,
                JIS_X0208_83 = 0x0621,
                JIS_X0208_90 = 0x0622,
                JIS_X0212_90 = 0x0623,
                JIS_C6226_78 = 0x0624,
                ShiftJIS_X0213  = 0x0628, /* Shift-JIS format encoding of JIS X0213 planes 1 and 2*/
                ShiftJIS_X0213_MenKuTen = 0x0629,	/* JIS X0213 in plane-row-column notation */
                GB_2312_80 = 0x0630,
                GBK_95 = 0x0631,		/* annex to GB 13000-93; for Windows 95 */
                GB_18030_2000 = 0x0632,
                KSC_5601_87 = 0x0640,	/* same as KSC 5601-92 without Johab annex */
                KSC_5601_92_Johab = 0x0641, /* KSC 5601-92 Johab annex */
                CNS_11643_92_P1 = 0x0651,	/* CNS 11643-1992 plane 1 */
                CNS_11643_92_P2 = 0x0652,	/* CNS 11643-1992 plane 2 */
                CNS_11643_92_P3 = 0x0653,	/* CNS 11643-1992 plane 3 (was plane 14 in 1986 version) */
                
                /* ISO 2022 collections begin at 0x800 */
                ISO_2022_JP = 0x0820,
                ISO_2022_JP_2 = 0x0821,
                ISO_2022_JP_1 = 0x0822, /* RFC 2237*/
                ISO_2022_JP_3 = 0x0823, /* JIS X0213*/
                ISO_2022_CN = 0x0830,
                ISO_2022_CN_EXT = 0x0831,
                ISO_2022_KR = 0x0840,
                
                /* EUC collections begin at 0x900 */
                EUC_JP = 0x0920,		/* ISO 646, 1-byte katakana, JIS 208, JIS 212 */
                EUC_CN = 0x0930,		/* ISO 646, GB 2312-80 */
                EUC_TW = 0x0931,		/* ISO 646, CNS 11643-1992 Planes 1-16 */
                EUC_KR = 0x0940,		/* ISO 646, KS C 5601-1987 */
                
                /* Misc standards begin at 0xA00 */
                ShiftJIS = 0x0A01,		/* plain Shift-JIS */
                KOI8_R = 0x0A02,		/* Russian internet standard */
                Big5 = 0x0A03,		/* Big-5 (has variants) */
                MacRomanLatin1 = 0x0A04,	/* Mac OS Roman permuted to align with ISO Latin-1 */
                HZ_GB_2312 = 0x0A05,	/* HZ (RFC 1842, for Chinese mail & news) */
                Big5_HKSCS_1999 = 0x0A06, /* Big-5 with Hong Kong special char set supplement*/
                VISCII = 0x0A07,	/* RFC 1456, Vietnamese */
                KOI8_U = 0x0A08,	/* RFC 2319, Ukrainian */
                Big5_E = 0x0A09,	/* Taiwan Big-5E standard */
                
                /* Other platform encodings*/
                /*  NextStepLatin = 0x0B01, defined in CoreFoundation/.h */
                NextStepJapanese = 0x0B02,	/* NextStep Japanese encoding */
                
                /* EBCDIC & IBM host encodings begin at 0xC00 */
                EBCDIC_US = 0x0C01,	/* basic EBCDIC-US */
                EBCDIC_CP037 = 0x0C02,	/* code page 037, extended EBCDIC (Latin-1 set) for US,Canada... */
                
                UTF7  = 0x04000100, /* kTextEncodingUnicodeDefault + kUnicodeUTF7Format RFC2152 */
                UTF7_IMAP = 0x0A10, /* UTF-7 (IMAP folder variant) RFC3501 */
                
                /* Deprecated constants */
                ShiftJIS_X0213_00 = 0x0628 /* Shift-JIS format encoding of JIS X0213 planes 1 and 2 (DEPRECATED) */
            };
            
            enum  EncodingConfiguration : Index {
                AllowLossyConversion = (1UL << 0), // Uses fallback functions to substitutes non mappable chars
                BasicDirectionLeftToRight = (1UL << 1), // Converted with original direction left-to-right.
                BasicDirectionRightToLeft = (1UL << 2), // Converted with original direction right-to-left.
                SubstituteCombinings = (1UL << 3), // Uses fallback function to combining chars.
                ComposeCombinings = (1UL << 4), // Checks mappable precomposed equivalents for decomposed sequences.  This is the default behavior.
                IgnoreCombinings = (1UL << 5), // Ignores combining chars.
                UseCanonical = (1UL << 6), // Always use canonical form
                UseHFSPlusCanonical = (1UL << 7), // Always use canonical form but leaves 0x2000 ranges
                PrependBOM = (1UL << 8), // Prepend BOM sequence (i.e. ISO2022KR)
                DisableCorporateArea = (1UL << 9), // Disable the usage of 0xF8xx area for Apple proprietary chars in converting to UniChar, resulting loosely mapping.
                ASCIICompatibleConversion = (1UL << 10), // This flag forces strict ASCII compatible converion. i.e. MacJapanese 0x5C maps to Unicode 0x5C.
                LenientUTF8Conversion = (1UL << 11), // 10.1 (Puma) compatible lenient UTF-8 conversion.
                PartialInput = (1UL << 12), // input buffer is a part of stream
                PartialOutput = (1UL << 13) // output buffer streaming
            };
            
            enum ConversionResult {
                Success = 0,
                InvalidInputStream = 1,
                InsufficientOutputBufferLength = 2,
                Unavailable = 3
            };
            String() {};
        private:
            
            
            String(const char*);
            
            friend String *_CreateInstanceImmutable(Allocator<String> *allocator,
                                                    const void *bytes,
                                                    Index numBytes,
                                                    String::Encoding encoding,
                                                    bool possiblyExternalFormat,
                                                    bool tryToReduceUnicode,
                                                    bool hasLengthByte,
                                                    bool hasNullByte,
                                                    bool noCopy,
                                                    Allocator<String> *contentsDeallocator,
                                                    UInt32 reservedFlags);
//            template<>
//            friend class Allocator<String>::_AllocateImpl<String, false>;
            
//            template<>
//            friend class Allocator<String>::_AllocateImpl<String, true>;
            
            
            friend String *Allocator<String>::_AllocateImpl<String, false>::_Allocate(malloc_zone_t *zone, size_t size);
//            friend RSFoundation::Collection::String* RSFoundation::Basic::Allocator<RSFoundation::Collection::String>::_AllocateImpl<RSFoundation::Collection::String, false>::_Allocate(malloc_zone_t *zone, size_t size);
        public:
            static const String *Create();
            static const String *Create(const char*cStr, String::Encoding encoding);
            ~String();
        public:
            
            static String Empty;
            static String::Encoding GetSystemEncoding();
            
        public:
            
            Index GetLength() const;
            Index GetCapacity() const;
            
        public:
            bool operator==(const String &rhs) {
                return GetLength() == rhs.GetLength();
            }
            
        private:
            enum {
                FreeContentsWhenDoneMask = 0x020,
                FreeContentsWhenDone = 0x020,
                ContentsMask = 0x060,
                HasInlineContents = 0x000,
                NotInlineContentsNoFree = 0x040,		// Don't free
                NotInlineContentsDefaultFree = 0x020,	// Use allocator's free function
                NotInlineContentsCustomFree = 0x060,		// Use a specially provided free function
                HasContentsAllocatorMask = 0x060,
                HasContentsAllocator = 0x060,		// (For mutable strings) use a specially provided allocator
                HasContentsDeallocatorMask = 0x060,
                HasContentsDeallocator = 0x060,
                IsMutableMask = 0x01,
                IsMutable = 0x01,
                IsUnicodeMask = 0x10,
                IsUnicode = 0x10,
                HasNullByteMask = 0x08,
                HasNullByte = 0x08,
                HasLengthByteMask = 0x04,
                HasLengthByte = 0x04,
            };
            
            inline void _SetInfo(UInt32 info) {
                _info1 = info;
            }
            
            inline bool _IsMutable() const {
                return (_info1 & IsMutableMask) == IsMutable;
            }
            
            inline bool _IsInline() const {
                return (_info1 & ContentsMask) == HasInlineContents;
            }
            
            inline bool _IsFreeContentsWhenDone() const {
                return (_info1 & FreeContentsWhenDoneMask) == FreeContentsWhenDone;
            }
            
            inline bool _HasContentsDeallocator() const {
                return (_info1 & HasContentsDeallocatorMask) == HasContentsDeallocator;
            }
            
            inline bool _IsUnicode() const {
                return (_info1 & IsUnicodeMask) == IsUnicode;
            }
            
            inline bool _IsEightBit() const {
                return (_info1 & IsUnicodeMask) != IsUnicode;
            }
            
            inline bool _HasNullByte() const {
                return (_info1 & HasNullByteMask) == HasNullByte;
            }
            
            inline bool _HasLengthByte() const {
                return (_info1 & HasLengthByteMask) == HasLengthByteMask;
            }
            
            inline bool _HasExplicitLength() const {
                return (_info1 & (IsMutableMask | HasLengthByteMask)) != HasLengthByte;
            }
            
            inline UInt32 _SkipAnyLengthByte() const {
                return _HasLengthByte() ? 1 : 0;
            }
            
            inline const void *_Contents() const {
                if (_IsInline()) {
                    uintptr_t vbase = (uintptr_t)&(this->_variants);
                    uintptr_t offset = (_HasExplicitLength() ? sizeof(Index) : 0);
                    uintptr_t rst = vbase + offset;
                    return (const void *)rst;
                }
                return this->_variants._notInlineImmutable1._buffer;
            }
            
            inline Allocator<String>** _ContentsDeallocatorPtr() const {
                return _HasExplicitLength() ? (Allocator<String> **)(&(this->_variants._notInlineImmutable1._contentsDeallocator)) : (Allocator<String> **)(&this->_variants._notInlineImmutable2._contentsDeallocator);
            }
            
            inline Allocator<String> *_ContentsDeallocator() const {
                return *_ContentsDeallocatorPtr();
            }
            
            void _SetContentsDeallocator(Allocator<String> *allocator) {
                *_ContentsDeallocatorPtr() = allocator;
            }
            
            inline Allocator<String> **_ContentsAllocatorPtr() const {
                return (Allocator<String> **)&this->_variants._notInlineMutable1._contentsAllocator;
            }
            
            inline Allocator<String> *_ContentsAllocator() const {
                return *_ContentsAllocatorPtr();
            }
            
            inline void _SetContentsAllocator(Allocator<String> *allocator) {
                *_ContentsAllocatorPtr() = allocator;
            }
            
            inline Index _Length() const {
                if (_HasExplicitLength()) {
                    if (_IsInline()) {
                        return this->_variants._inline1._length;
                    } else {
                        return this->_variants._notInlineImmutable1._length;
                    }
                } else {
                    return (Index)(*((UInt8 *)_Contents()));
                }
            }
            
            inline Index _Length2(const void *buffer) const {
                if (_HasExplicitLength()) {
                    if (_IsInline()) {
                        return this->_variants._inline1._length;
                    } else {
                        return this->_variants._notInlineImmutable1._length;
                    }
                } else {
                    return (Index)(*((UInt8 *)buffer));
                }
            }
            
            inline void _SetContentPtr(const void *buffer) {
                this->_variants._notInlineImmutable1._buffer = (void *)buffer;
            }
            
            inline void _SetExplicitLength(Index length) {
                if (_IsInline()) {
                    this->_variants._inline1._length = length;
                } else {
                    this->_variants._notInlineImmutable1._length = length;
                }
            }
            
            inline void _SetUnicode() {
                _info1 |= IsUnicode;
            }
            
            inline void _ClearUnicode() {
                _info1 &= ~IsUnicode;
            }
            
            inline void _SetHasLengthAndNullBytes() {
                _info1 |= (HasLengthByte | HasNullByte);
            }
            
            inline void _ClearHasLengthAndNullBytes() {
                _info1 &= ~(HasLengthByte | HasNullByte);
            }
            
            inline bool _IsFixed() const {
                return this->_variants._notInlineMutable1._isFixedCapacity;
            }
            
            inline bool _IsExternalMutable() const {
                return this->_variants._notInlineMutable1._isExternalMutable;
            }
            
            inline bool _HasContentsAllocator() {
                return (_info1 & HasContentsAllocatorMask) == HasContentsAllocator;
            }
            
            inline void _SetIsFixed(bool v = true) {
                this->_variants._notInlineMutable1._isFixedCapacity = v;
            }
            
            inline void _SetIsExternalMutable(bool v = true) {
                this->_variants._notInlineMutable1._isExternalMutable = v;
            }
            
            inline void _SetHasGap(bool v = true) {
                this->_variants._notInlineMutable1._hasGap = v;
            }
            
            inline bool _CapacityProvidedExternally() const {
                return this->_variants._notInlineMutable1._capacityProvidedExternally;
            }
            
            inline void _SetCapacityProvidedExternally(bool v = true) {
                this->_variants._notInlineMutable1._capacityProvidedExternally = v;
            }
            
            inline void _ClearCapacityProvidedExternally() {
                return _SetCapacityProvidedExternally(false);
            }
            
            inline Index _Capacity() const {
                return this->_variants._notInlineMutable1._capacity;
            }
            
            inline void _SetCapacity(Index capacity) {
                this->_variants._notInlineMutable1._capacity = capacity;
            }
            
            inline Index _DesiredCapacity() const {
                return this->_variants._notInlineMutable1._desiredCapacity;
            }
            
            inline void _SetDesiredCapacity(Index desiredCapacity) {
                this->_variants._notInlineMutable1._desiredCapacity = desiredCapacity;
            }
            
            static void *_AllocateMutableContents(const String &str, size_t size) {
                return new char(size);
            }
            
            /* Basic algorithm is to shrink memory when capacity is SHRINKFACTOR times the required capacity or to allocate memory when the capacity is less than GROWFACTOR times the required capacity.  This function will return -1 if the new capacity is just too big (> LONG_MAX).
             Additional complications are applied in the following order:
             - desiredCapacity, which is the minimum (except initially things can be at zero)
             - rounding up to factor of 8
             - compressing (to fit the number if 16 bits), which effectively rounds up to factor of 256
             - we need to make sure GROWFACTOR computation doesn't suffer from overflow issues on 32-bit, hence the casting to unsigned. Normally for required capacity of C bytes, the allocated space is (3C+1)/2. If C > ULONG_MAX/3, we instead simply return LONG_MAX
             */
#define SHRINKFACTOR(c) (c / 2)
            
#if __LP64__
#define GROWFACTOR(c) ((c * 3 + 1) / 2)
#else
#define GROWFACTOR(c) (((c) >= (ULONG_MAX / 3UL)) ? __Max(LONG_MAX - 4095, (c)) : (((unsigned long)c * 3 + 1) / 2))
#endif
            
            inline Index _NewCapacity(unsigned long reqCapacity, Index capacity, BOOL leaveExtraRoom, Index charSize)
            {
                if (capacity != 0 || reqCapacity != 0)
                {
                    /* If initially zero, and space not needed, leave it at that... */
                    if ((capacity < reqCapacity) ||		/* We definitely need the room... */
                        (!_CapacityProvidedExternally() && 	/* Assuming we control the capacity... */
                         ((reqCapacity < SHRINKFACTOR(capacity)) ||		/* ...we have too much room! */
                          (!leaveExtraRoom && (reqCapacity < capacity)))))
                    {
                        /* ...we need to eliminate the extra space... */
                        if (reqCapacity > LONG_MAX) return -1;  /* Too big any way you cut it */
                        unsigned long newCapacity = leaveExtraRoom ? GROWFACTOR(reqCapacity) : reqCapacity;	/* Grow by 3/2 if extra room is desired */
                        Index desiredCapacity = _DesiredCapacity() * charSize;
                        if (newCapacity < desiredCapacity) {
                            /* If less than desired, bump up to desired */
                            newCapacity = desiredCapacity;
                        } else if (_IsFixed()) {
                            /* Otherwise, if fixed, no need to go above the desired (fixed) capacity */
                            newCapacity = max((unsigned long)desiredCapacity, reqCapacity);	/* !!! So, fixed is not really fixed, but "tight" */
                        }
                        if (_HasContentsAllocator()) {
                            /* Also apply any preferred size from the allocator  */
                            newCapacity = newCapacity;
//                            newCapacity = AllocatorSize(__StrContentsAllocator(str), newCapacity);
#if DEPLOYMENT_TARGET_MACOSX || DEPLOYMENT_TARGET_EMBEDDED || DEPLOYMENT_TARGET_EMBEDDED_MINI
                        } else {
                            newCapacity = malloc_good_size(newCapacity);
#endif
                        }
                        return (newCapacity > LONG_MAX) ? -1 : (Index)newCapacity; // If packing: __StrUnpackNumber(__StrPackNumber(newCapacity));
                    }
                }
                return capacity;
            }
            
            void _DeallocateMutableContents(void *buffer);

        private:
            UInt32 _info1;
#ifdef __LP64__
            UInt32 _reserved;
#endif
            struct __notInlineMutable {
                void *_buffer;
                Index _length;
                Index _capacity;                           // Capacity in bytes
                unsigned long _hasGap:1;                      // Currently unused
                unsigned long _isFixedCapacity:1;
                unsigned long _isExternalMutable:1;
                unsigned long _capacityProvidedExternally:1;
#if __LP64__
                unsigned long _desiredCapacity:60;
#else
                unsigned long _desiredCapacity:28;
#endif
                Allocator<String>* _contentsAllocator;           // Optional
            };
            
            union __variants {
                struct __inline1 {
                    Index _length;
                } _inline1;
                
                struct __inline2 {
                    Index _length;
                    void *_buffer;
                } _inline2;
                
                struct __notInlineImmutable1 {
                    void *_buffer;
                    Index _length;
                    Allocator<String> *_contentsDeallocator;
                } _notInlineImmutable1;
                
                struct __notInlineImmutable2 {
                    void *_buffer;
                    Allocator<String> *_contentsDeallocator;
                } _notInlineImmutable2;
                struct __notInlineMutable _notInlineMutable1;
                
                __variants() {
                    
                }
                
                ~__variants() {
                    
                }
            } _variants;
        };
        
    }
}

#endif /* defined(__RSCoreFoundation__String__) */
