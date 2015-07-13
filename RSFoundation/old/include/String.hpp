//
//  String.h
//  RSCoreFoundation
//
//  Created by closure on 5/27/15.
//  Copyright (c) 2015 RetVal. All rights reserved.
//

#ifndef __RSCoreFoundation__String__
#define __RSCoreFoundation__String__

#include <RSFoundation/Object.hpp>
//#include <RSFoundation/Allocator.hpp>
#include <RSFoundation/Protocol.hpp>

namespace RSFoundation {
    using namespace Basic;
    namespace Collection {
        class StringImpl;
        
        class String : public Object, public virtual Copying, public virtual Hashable {
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
        public:
            String();
            String(const String &str);
            String(const char *utf8String);
            String(const char *cStr, Encoding encoding);
            
            ~String();
        public:
            HashCode Hash() const;
            Index GetLength() const;
        public:
            bool operator==(const String &rhs) {
                return GetLength() == rhs.GetLength();
            }
            
        public:
            static String Empty;
            static String::Encoding GetSystemEncoding();
        private:
            class VarWidthCharBuffer;
            friend class VarWidthCharBuffer;
            
            const StringImpl *implementation;
        };
    }
}

#endif /* defined(__RSCoreFoundation__String__) */
