//
//  RSStringEncoding.h
//  RSCoreFoundation
//
//  Created by RetVal on 12/13/12.
//  Copyright (c) 2012 RetVal. All rights reserved.
//

#ifndef RSCoreFoundation_RSStringEncoding_h
#define RSCoreFoundation_RSStringEncoding_h
RS_EXTERN_C_BEGIN
#define RSStringEncodingSupport 1

#define RSStringEncodingInvalidId (0xffffffffU)
typedef RSBitU32 RSStringEncoding RS_AVAILABLE(0_0);

typedef RS_ENUM(RSStringEncoding, RSStringBuiltInEncodings) {
    RSStringEncodingMacRoman = 0,
    RSStringEncodingWindowsLatin1 = 0x0500, /* ANSI codepage 1252 */
    RSStringEncodingISOLatin1 = 0x0201, /* ISO 8859-1 */
    RSStringEncodingNextStepLatin = 0x0B01, /* NextStep encoding*/
    RSStringEncodingASCII = 0x0600, /* 0..127 (in creating RSString, values greater than 0x7F are treated as corresponding Unicode value) */
    RSStringEncodingUnicode = 0x0100, /* kTextEncodingUnicodeDefault  + kTextEncodingDefaultFormat (aka kUnicode16BitFormat) */
    RSStringEncodingUTF8 = 0x08000100, /* kTextEncodingUnicodeDefault + kUnicodeUTF8Format */
    RSStringEncodingNonLossyASCII = 0x0BFF, /* 7bit Unicode variants used by Cocoa & Java */
    
    RSStringEncodingUTF16 = 0x0100, /* kTextEncodingUnicodeDefault + kUnicodeUTF16Format (alias of RSStringEncodingUnicode) */
    RSStringEncodingUTF16BE = 0x10000100, /* kTextEncodingUnicodeDefault + kUnicodeUTF16BEFormat */
    RSStringEncodingUTF16LE = 0x14000100, /* kTextEncodingUnicodeDefault + kUnicodeUTF16LEFormat */
    
    RSStringEncodingUTF32 = 0x0c000100, /* kTextEncodingUnicodeDefault + kUnicodeUTF32Format */
    RSStringEncodingUTF32BE = 0x18000100, /* kTextEncodingUnicodeDefault + kUnicodeUTF32BEFormat */
    RSStringEncodingUTF32LE = 0x1c000100 /* kTextEncodingUnicodeDefault + kUnicodeUTF32LEFormat */
} RS_AVAILABLE(0_0);

typedef RS_ENUM(RSIndex, RSStringEncodings)
{
    /*  RSStringEncodingMacRoman = 0L, defined in CoreFoundation/RSString.h */
    RSStringEncodingMacJapanese = 1,
    RSStringEncodingMacChineseTrad = 2,
    RSStringEncodingMacKorean = 3,
    RSStringEncodingMacArabic = 4,
    RSStringEncodingMacHebrew = 5,
    RSStringEncodingMacGreek = 6,
    RSStringEncodingMacCyrillic = 7,
    RSStringEncodingMacDevanagari = 9,
    RSStringEncodingMacGurmukhi = 10,
    RSStringEncodingMacGujarati = 11,
    RSStringEncodingMacOriya = 12,
    RSStringEncodingMacBengali = 13,
    RSStringEncodingMacTamil = 14,
    RSStringEncodingMacTelugu = 15,
    RSStringEncodingMacKannada = 16,
    RSStringEncodingMacMalayalam = 17,
    RSStringEncodingMacSinhalese = 18,
    RSStringEncodingMacBurmese = 19,
    RSStringEncodingMacKhmer = 20,
    RSStringEncodingMacThai = 21,
    RSStringEncodingMacLaotian = 22,
    RSStringEncodingMacGeorgian = 23,
    RSStringEncodingMacArmenian = 24,
    RSStringEncodingMacChineseSimp = 25,
    RSStringEncodingMacTibetan = 26,
    RSStringEncodingMacMongolian = 27,
    RSStringEncodingMacEthiopic = 28,
    RSStringEncodingMacCentralEurRoman = 29,
    RSStringEncodingMacVietnamese = 30,
    RSStringEncodingMacExtArabic = 31,
    /* The following use script code 0, smRoman */
    RSStringEncodingMacSymbol = 33,
    RSStringEncodingMacDingbats = 34,
    RSStringEncodingMacTurkish = 35,
    RSStringEncodingMacCroatian = 36,
    RSStringEncodingMacIcelandic = 37,
    RSStringEncodingMacRomanian = 38,
    RSStringEncodingMacCeltic = 39,
    RSStringEncodingMacGaelic = 40,
    /* The following use script code 4, smArabic */
    RSStringEncodingMacFarsi = 0x8C,	/* Like MacArabic but uses Farsi digits */
    /* The following use script code 7, smCyrillic */
    RSStringEncodingMacUkrainian = 0x98,
    /* The following use script code 32, smUnimplemented */
    RSStringEncodingMacInuit = 0xEC,
    RSStringEncodingMacVT100 = 0xFC,	/* VT100/102 font from Comm Toolbox: Latin-1 repertoire + box drawing etc */
    /* Special Mac OS encodings*/
    RSStringEncodingMacHFS = 0xFF,	/* Meta-value, should never appear in a table */
    
    /* Unicode & ISO UCS encodings begin at 0x100 */
    /* We don't use Unicode variations defined in TextEncoding; use the ones in RSString.h, instead. */
    
    /* ISO 8-bit and 7-bit encodings begin at 0x200 */
    /*  RSStringEncodingISOLatin1 = 0x0201, defined in CoreFoundation/RSString.h */
    RSStringEncodingISOLatin2 = 0x0202,	/* ISO 8859-2 */
    RSStringEncodingISOLatin3 = 0x0203,	/* ISO 8859-3 */
    RSStringEncodingISOLatin4 = 0x0204,	/* ISO 8859-4 */
    RSStringEncodingISOLatinCyrillic = 0x0205,	/* ISO 8859-5 */
    RSStringEncodingISOLatinArabic = 0x0206,	/* ISO 8859-6, =ASMO 708, =DOS CP 708 */
    RSStringEncodingISOLatinGreek = 0x0207,	/* ISO 8859-7 */
    RSStringEncodingISOLatinHebrew = 0x0208,	/* ISO 8859-8 */
    RSStringEncodingISOLatin5 = 0x0209,	/* ISO 8859-9 */
    RSStringEncodingISOLatin6 = 0x020A,	/* ISO 8859-10 */
    RSStringEncodingISOLatinThai = 0x020B,	/* ISO 8859-11 */
    RSStringEncodingISOLatin7 = 0x020D,	/* ISO 8859-13 */
    RSStringEncodingISOLatin8 = 0x020E,	/* ISO 8859-14 */
    RSStringEncodingISOLatin9 = 0x020F,	/* ISO 8859-15 */
    RSStringEncodingISOLatin10 = 0x0210,	/* ISO 8859-16 */
    
    /* MS-DOS & Windows encodings begin at 0x400 */
    RSStringEncodingDOSLatinUS = 0x0400,	/* code page 437 */
    RSStringEncodingDOSGreek = 0x0405,		/* code page 737 (formerly code page 437G) */
    RSStringEncodingDOSBalticRim = 0x0406,	/* code page 775 */
    RSStringEncodingDOSLatin1 = 0x0410,	/* code page 850, "Multilingual" */
    RSStringEncodingDOSGreek1 = 0x0411,	/* code page 851 */
    RSStringEncodingDOSLatin2 = 0x0412,	/* code page 852, Slavic */
    RSStringEncodingDOSCyrillic = 0x0413,	/* code page 855, IBM Cyrillic */
    RSStringEncodingDOSTurkish = 0x0414,	/* code page 857, IBM Turkish */
    RSStringEncodingDOSPortuguese = 0x0415,	/* code page 860 */
    RSStringEncodingDOSIcelandic = 0x0416,	/* code page 861 */
    RSStringEncodingDOSHebrew = 0x0417,	/* code page 862 */
    RSStringEncodingDOSCanadianFrench = 0x0418, /* code page 863 */
    RSStringEncodingDOSArabic = 0x0419,	/* code page 864 */
    RSStringEncodingDOSNordic = 0x041A,	/* code page 865 */
    RSStringEncodingDOSRussian = 0x041B,	/* code page 866 */
    RSStringEncodingDOSGreek2 = 0x041C,	/* code page 869, IBM Modern Greek */
    RSStringEncodingDOSThai = 0x041D,		/* code page 874, also for Windows */
    RSStringEncodingDOSJapanese = 0x0420,	/* code page 932, also for Windows */
    RSStringEncodingDOSChineseSimplif = 0x0421, /* code page 936, also for Windows */
    RSStringEncodingDOSKorean = 0x0422,	/* code page 949, also for Windows; Unified Hangul Code */
    RSStringEncodingDOSChineseTrad = 0x0423,	/* code page 950, also for Windows */
    /*  RSStringEncodingWindowsLatin1 = 0x0500, defined in CoreFoundation/RSString.h */
    RSStringEncodingWindowsLatin2 = 0x0501,	/* code page 1250, Central Europe */
    RSStringEncodingWindowsCyrillic = 0x0502,	/* code page 1251, Slavic Cyrillic */
    RSStringEncodingWindowsGreek = 0x0503,	/* code page 1253 */
    RSStringEncodingWindowsLatin5 = 0x0504,	/* code page 1254, Turkish */
    RSStringEncodingWindowsHebrew = 0x0505,	/* code page 1255 */
    RSStringEncodingWindowsArabic = 0x0506,	/* code page 1256 */
    RSStringEncodingWindowsBalticRim = 0x0507,	/* code page 1257 */
    RSStringEncodingWindowsVietnamese = 0x0508, /* code page 1258 */
    RSStringEncodingWindowsKoreanJohab = 0x0510, /* code page 1361, for Windows NT */
    
    /* Various national standards begin at 0x600 */
    /*  RSStringEncodingASCII = 0x0600, defined in CoreFoundation/RSString.h */
    RSStringEncodingANSEL = 0x0601,	/* ANSEL (ANSI Z39.47) */
    RSStringEncodingJIS_X0201_76 = 0x0620,
    RSStringEncodingJIS_X0208_83 = 0x0621,
    RSStringEncodingJIS_X0208_90 = 0x0622,
    RSStringEncodingJIS_X0212_90 = 0x0623,
    RSStringEncodingJIS_C6226_78 = 0x0624,
    RSStringEncodingShiftJIS_X0213  = 0x0628, /* Shift-JIS format encoding of JIS X0213 planes 1 and 2*/
    RSStringEncodingShiftJIS_X0213_MenKuTen = 0x0629,	/* JIS X0213 in plane-row-column notation */
    RSStringEncodingGB_2312_80 = 0x0630,
    RSStringEncodingGBK_95 = 0x0631,		/* annex to GB 13000-93; for Windows 95 */
    RSStringEncodingGB_18030_2000 = 0x0632,
    RSStringEncodingKSC_5601_87 = 0x0640,	/* same as KSC 5601-92 without Johab annex */
    RSStringEncodingKSC_5601_92_Johab = 0x0641, /* KSC 5601-92 Johab annex */
    RSStringEncodingCNS_11643_92_P1 = 0x0651,	/* CNS 11643-1992 plane 1 */
    RSStringEncodingCNS_11643_92_P2 = 0x0652,	/* CNS 11643-1992 plane 2 */
    RSStringEncodingCNS_11643_92_P3 = 0x0653,	/* CNS 11643-1992 plane 3 (was plane 14 in 1986 version) */
    
    /* ISO 2022 collections begin at 0x800 */
    RSStringEncodingISO_2022_JP = 0x0820,
    RSStringEncodingISO_2022_JP_2 = 0x0821,
    RSStringEncodingISO_2022_JP_1 = 0x0822, /* RFC 2237*/
    RSStringEncodingISO_2022_JP_3 = 0x0823, /* JIS X0213*/
    RSStringEncodingISO_2022_CN = 0x0830,
    RSStringEncodingISO_2022_CN_EXT = 0x0831,
    RSStringEncodingISO_2022_KR = 0x0840,
    
    /* EUC collections begin at 0x900 */
    RSStringEncodingEUC_JP = 0x0920,		/* ISO 646, 1-byte katakana, JIS 208, JIS 212 */
    RSStringEncodingEUC_CN = 0x0930,		/* ISO 646, GB 2312-80 */
    RSStringEncodingEUC_TW = 0x0931,		/* ISO 646, CNS 11643-1992 Planes 1-16 */
    RSStringEncodingEUC_KR = 0x0940,		/* ISO 646, KS C 5601-1987 */
    
    /* Misc standards begin at 0xA00 */
    RSStringEncodingShiftJIS = 0x0A01,		/* plain Shift-JIS */
    RSStringEncodingKOI8_R = 0x0A02,		/* Russian internet standard */
    RSStringEncodingBig5 = 0x0A03,		/* Big-5 (has variants) */
    RSStringEncodingMacRomanLatin1 = 0x0A04,	/* Mac OS Roman permuted to align with ISO Latin-1 */
    RSStringEncodingHZ_GB_2312 = 0x0A05,	/* HZ (RFC 1842, for Chinese mail & news) */
    RSStringEncodingBig5_HKSCS_1999 = 0x0A06, /* Big-5 with Hong Kong special char set supplement*/
    RSStringEncodingVISCII = 0x0A07,	/* RFC 1456, Vietnamese */
    RSStringEncodingKOI8_U = 0x0A08,	/* RFC 2319, Ukrainian */
    RSStringEncodingBig5_E = 0x0A09,	/* Taiwan Big-5E standard */
    
    /* Other platform encodings*/
    /*  RSStringEncodingNextStepLatin = 0x0B01, defined in CoreFoundation/RSString.h */
    RSStringEncodingNextStepJapanese = 0x0B02,	/* NextStep Japanese encoding */
    
    /* EBCDIC & IBM host encodings begin at 0xC00 */
    RSStringEncodingEBCDIC_US = 0x0C01,	/* basic EBCDIC-US */
    RSStringEncodingEBCDIC_CP037 = 0x0C02,	/* code page 037, extended EBCDIC (Latin-1 set) for US,Canada... */
    
    RSStringEncodingUTF7  = 0x04000100, /* kTextEncodingUnicodeDefault + kUnicodeUTF7Format RFC2152 */
    RSStringEncodingUTF7_IMAP = 0x0A10, /* UTF-7 (IMAP folder variant) RFC3501 */
    
    /* Deprecated constants */
    RSStringEncodingShiftJIS_X0213_00 = 0x0628 /* Shift-JIS format encoding of JIS X0213 planes 1 and 2 (DEPRECATED) */
} RS_AVAILABLE(0_0);

enum {
    RSStringEncodingAllowLossyConversion = (1UL << 0), // Uses fallback functions to substitutes non mappable chars
    RSStringEncodingBasicDirectionLeftToRight = (1UL << 1), // Converted with original direction left-to-right.
    RSStringEncodingBasicDirectionRightToLeft = (1UL << 2), // Converted with original direction right-to-left.
    RSStringEncodingSubstituteCombinings = (1UL << 3), // Uses fallback function to combining chars.
    RSStringEncodingComposeCombinings = (1UL << 4), // Checks mappable precomposed equivalents for decomposed sequences.  This is the default behavior.
    RSStringEncodingIgnoreCombinings = (1UL << 5), // Ignores combining chars.
    RSStringEncodingUseCanonical = (1UL << 6), // Always use canonical form
    RSStringEncodingUseHFSPlusCanonical = (1UL << 7), // Always use canonical form but leaves 0x2000 ranges
    RSStringEncodingPrependBOM = (1UL << 8), // Prepend BOM sequence (i.e. ISO2022KR)
    RSStringEncodingDisableCorporateArea = (1UL << 9), // Disable the usage of 0xF8xx area for Apple proprietary chars in converting to UniChar, resulting loosely mapping.
    RSStringEncodingASCIICompatibleConversion = (1UL << 10), // This flag forces strict ASCII compatible converion. i.e. MacJapanese 0x5C maps to Unicode 0x5C.
    RSStringEncodingLenientUTF8Conversion = (1UL << 11), // 10.1 (Puma) compatible lenient UTF-8 conversion.
    RSStringEncodingPartialInput = (1UL << 12), // input buffer is a part of stream
    RSStringEncodingPartialOutput = (1UL << 13) // output buffer streaming
};

/* Return values for RSStringEncodingUnicodeToBytes & RSStringEncodingBytesToUnicode functions
 */
enum {
    RSStringEncodingConversionSuccess = 0,
    RSStringEncodingInvalidInputStream = 1,
    RSStringEncodingInsufficientOutputBufferLength = 2,
    RSStringEncodingConverterUnavailable = 3
};

#define RSStringEncodingLossyByteToMask(lossByte)	((uint32_t)(lossByte << 24)|RSStringEncodingAllowLossyConversion)
#define RSStringEncodingMaskToLossyByte(flags)		((uint8_t)(flags >> 24))

/* Macros for streaming support
 */
#define RSStringEncodingStreamIDMask                    (0x00FF0000)
#define RSStringEncodingStreamIDFromMask(mask)    ((mask >> 16) & 0xFF)
#define RSStringEncodingStreamIDToMask(identifier)            ((uint32_t)((identifier & 0xFF) << 16))
RS_EXTERN_C_END
#endif
