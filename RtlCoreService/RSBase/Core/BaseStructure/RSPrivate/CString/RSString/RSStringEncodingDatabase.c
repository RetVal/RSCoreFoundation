//
//  RSStringEncodingDatabase.c
//  RSCoreFoundation
//
//  Created by RetVal on 7/29/13.
//  Copyright (c) 2013 RetVal. All rights reserved.
//

#include "RSInternal.h"
#include "RSStringEncoding.h"
#include "RSStringEncodingConverterPrivate.h"
#include "RSStringEncodingDatabase.h"
#include "RSICUConverters.h"
#include <stdio.h>

#define ISO8859CODEPAGE_BASE (28590)

static const uint16_t __RSKnownEncodingList[] = {
    RSStringEncodingMacRoman,
    RSStringEncodingMacJapanese,
    RSStringEncodingMacChineseTrad,
    RSStringEncodingMacKorean,
    RSStringEncodingMacArabic,
    RSStringEncodingMacHebrew,
    RSStringEncodingMacGreek,
    RSStringEncodingMacCyrillic,
    RSStringEncodingMacDevanagari,
    RSStringEncodingMacGurmukhi,
    RSStringEncodingMacGujarati,
    RSStringEncodingMacOriya,
    RSStringEncodingMacBengali,
    RSStringEncodingMacTamil,
    RSStringEncodingMacTelugu,
    RSStringEncodingMacKannada,
    RSStringEncodingMacMalayalam,
    RSStringEncodingMacSinhalese,
    RSStringEncodingMacBurmese,
    RSStringEncodingMacKhmer,
    RSStringEncodingMacThai,
    RSStringEncodingMacLaotian,
    RSStringEncodingMacGeorgian,
    RSStringEncodingMacArmenian,
    RSStringEncodingMacChineseSimp,
    RSStringEncodingMacTibetan,
    RSStringEncodingMacMongolian,
    RSStringEncodingMacEthiopic,
    RSStringEncodingMacCentralEurRoman,
    RSStringEncodingMacVietnamese,
    RSStringEncodingMacSymbol,
    RSStringEncodingMacDingbats,
    RSStringEncodingMacTurkish,
    RSStringEncodingMacCroatian,
    RSStringEncodingMacIcelandic,
    RSStringEncodingMacRomanian,
    RSStringEncodingMacCeltic,
    RSStringEncodingMacGaelic,
    RSStringEncodingMacFarsi,
    RSStringEncodingMacUkrainian,
    RSStringEncodingMacInuit,
    
    RSStringEncodingDOSLatinUS,
    RSStringEncodingDOSGreek,
    RSStringEncodingDOSBalticRim,
    RSStringEncodingDOSLatin1,
    RSStringEncodingDOSGreek1,
    RSStringEncodingDOSLatin2,
    RSStringEncodingDOSCyrillic,
    RSStringEncodingDOSTurkish,
    RSStringEncodingDOSPortuguese,
    RSStringEncodingDOSIcelandic,
    RSStringEncodingDOSHebrew,
    RSStringEncodingDOSCanadianFrench,
    RSStringEncodingDOSArabic,
    RSStringEncodingDOSNordic,
    RSStringEncodingDOSRussian,
    RSStringEncodingDOSGreek2,
    RSStringEncodingDOSThai,
    RSStringEncodingDOSJapanese,
    RSStringEncodingDOSChineseSimplif,
    RSStringEncodingDOSKorean,
    RSStringEncodingDOSChineseTrad,
    
    RSStringEncodingWindowsLatin1,
    RSStringEncodingWindowsLatin2,
    RSStringEncodingWindowsCyrillic,
    RSStringEncodingWindowsGreek,
    RSStringEncodingWindowsLatin5,
    RSStringEncodingWindowsHebrew,
    RSStringEncodingWindowsArabic,
    RSStringEncodingWindowsBalticRim,
    RSStringEncodingWindowsVietnamese,
    RSStringEncodingWindowsKoreanJohab,
    RSStringEncodingASCII,
    
    RSStringEncodingShiftJIS_X0213,
    RSStringEncodingGB_18030_2000,
    
    RSStringEncodingISO_2022_JP,
    RSStringEncodingISO_2022_JP_2,
    RSStringEncodingISO_2022_JP_1,
    RSStringEncodingISO_2022_JP_3,
    RSStringEncodingISO_2022_CN,
    RSStringEncodingISO_2022_CN_EXT,
    RSStringEncodingISO_2022_KR,
    RSStringEncodingEUC_JP,
    RSStringEncodingEUC_CN,
    RSStringEncodingEUC_TW,
    RSStringEncodingEUC_KR,
    
    RSStringEncodingShiftJIS,
    
    RSStringEncodingKOI8_R,
    
    RSStringEncodingBig5,
    
    RSStringEncodingMacRomanLatin1,
    RSStringEncodingHZ_GB_2312,
    RSStringEncodingBig5_HKSCS_1999,
    RSStringEncodingVISCII,
    RSStringEncodingKOI8_U,
    RSStringEncodingBig5_E,
    RSStringEncodingUTF7_IMAP,
    
    RSStringEncodingNextStepLatin,
    
    RSStringEncodingEBCDIC_CP037
};

// Windows codepage mapping
static const uint16_t __RSWindowsCPList[] = {
    10000,
    10001,
    10002,
    10003,
    10004,
    10005,
    10006,
    10007,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    10021,
    0,
    0,
    0,
    10008,
    0,
    0,
    0,
    10029,
    0,
    0,
    0,
    10081,
    10082,
    10079,
    10010,
    0,
    0,
    0,
    10017,
    0,
    
    437,
    737,
    775,
    850,
    851,
    852,
    855,
    857,
    860,
    861,
    862,
    863,
    864,
    865,
    866,
    869,
    874,
    932,
    936,
    949,
    950,
    
    1252,
    1250,
    1251,
    1253,
    1254,
    1255,
    1256,
    1257,
    1258,
    1361,
    
    20127,
    
    0,
    54936,
    
    50221, // we prefere this over 50220/50221 since that's what RS coverter generates
    0,
    0,
    0,
    50227,
    0,
    50225,
    
    51932,
    51936,
    51950,
    51949,
    
    0,
    
    20866,
    
    0,
    
    0,
    52936,
    0,
    0,
    21866,
    0,
    0,
    
    0,
    
    37
};

// Canonical name
static const char *__RSCanonicalNameList[] = {
    "macintosh",
    "japanese",
    "trad-chinese",
    "korean",
    "arabic",
    "hebrew",
    "greek",
    "cyrillic",
    "devanagari",
    "gurmukhi",
    "gujarati",
    "oriya",
    "bengali",
    "tamil",
    "telugu",
    "kannada",
    "malayalam",
    "sinhalese",
    "burmese",
    "khmer",
    "thai",
    "laotian",
    "georgian",
    "armenian",
    "simp-chinese",
    "tibetan",
    "mongolian",
    "ethiopic",
    "centraleurroman",
    "vietnamese",
    "symbol",
    "dingbats",
    "turkish",
    "croatian",
    "icelandic",
    "romanian",
    "celtic",
    "gaelic",
    "farsi",
    "ukrainian",
    "inuit",
    
    nil,
    nil,
    nil,
    nil,
    nil,
    nil,
    nil,
    nil,
    nil,
    nil,
    nil,
    nil,
    nil,
    nil,
    nil,
    nil,
    nil,
    nil,
    nil,
    nil,
    nil,
    
    nil,
    nil,
    nil,
    nil,
    nil,
    nil,
    nil,
    nil,
    nil,
    nil,
    
    "us-ascii",
    
    nil,
    "gb18030",
    
    "iso-2022-jp",
    "iso-2022-jp-2",
    "iso-2022-jp-1",
    "iso-2022-jp-3",
    "iso-2022-cn",
    "iso-2022-cn-ext",
    "iso-2022-kr",
    "euc-jp",
    "gb2312",
    "euc-tw",
    "euc-kr",
    
    "shift_jis",
    
    "koi8-r",
    
    "big5",
    
    "roman-latin1",
    "hz-gb-2312",
    "big5-hkscs",
    "viscii",
    "koi8-u",
    nil,
    "utf7-imap",
    
    "x-nextstep",
    
    "ibm037",
};

static inline RSIndex __RSGetEncodingIndex(RSStringEncoding encoding) {
    const uint16_t *head = __RSKnownEncodingList;
    const uint16_t *tail = head + ((sizeof(__RSKnownEncodingList) / sizeof(*__RSKnownEncodingList)) - 1);
    const uint16_t *middle;
    
    encoding &= 0x0FFF;
    while (head <= tail) {
        middle = head + ((tail - head) >> 1);
        
        if (encoding == *middle) {
            return middle - __RSKnownEncodingList;
        } else if (encoding < *middle) {
            tail = middle - 1;
        } else {
            head = middle + 1;
        }
    }
    
    return RSNotFound;
}

RSPrivate uint16_t __RSStringEncodingGetWindowsCodePage(RSStringEncoding encoding) {
    RSStringEncoding encodingBase = encoding & 0x0F00;
    
    if (0x0100 == encodingBase) { // UTF
        switch (encoding) {
            case RSStringEncodingUTF7: return 65000;
            case RSStringEncodingUTF8: return 65001;
            case RSStringEncodingUTF16: return 1200;
            case RSStringEncodingUTF16BE: return 1201;
            case RSStringEncodingUTF32: return 65005;
            case RSStringEncodingUTF32BE: return 65006;
        }
    } else if (0x0200 == encodingBase) { // ISO 8859 range
        return ISO8859CODEPAGE_BASE + (encoding & 0xFF);
    } else { // others
        RSIndex index = __RSGetEncodingIndex(encoding);
        
        if (RSNotFound != index) return __RSWindowsCPList[index];
    }
    
    return 0;
}

RSPrivate RSStringEncoding __RSStringEncodingGetFromWindowsCodePage(uint16_t codepage) {
    switch (codepage) {
        case 65001: return RSStringEncodingUTF8;
        case 1200: return RSStringEncodingUTF16;
        case 0: return RSStringEncodingInvalidId;
        case 1201: return RSStringEncodingUTF16BE;
        case 65005: return RSStringEncodingUTF32;
        case 65006: return RSStringEncodingUTF32BE;
        case 65000: return RSStringEncodingUTF7;
    }
    
    if ((codepage > ISO8859CODEPAGE_BASE) && (codepage <= (ISO8859CODEPAGE_BASE + 16))) {
        return (codepage - ISO8859CODEPAGE_BASE) + 0x0200;
    } else {
        static RSMutableDictionaryRef mappingTable = nil;
        static RSSpinLock lock = RSSpinLockInit;
        uintptr_t value;
        
        RSSpinLockLock(&lock);
        if (nil == mappingTable) {
            RSIndex index, count = sizeof(__RSKnownEncodingList) / sizeof(*__RSKnownEncodingList);
            
            mappingTable = RSDictionaryCreateMutable(nil, 0, nil);
            RSAutorelease(mappingTable);
            for (index = 0;index < count;index++) {
                if (0 != __RSWindowsCPList[index]) RSDictionarySetValue(mappingTable, (const void *)(uintptr_t)__RSWindowsCPList[index], (const void *)(uintptr_t)__RSKnownEncodingList[index]);
            }
        }
        RSSpinLockUnlock(&lock);
        
//        if (RSDictionaryGetValueIfPresent(mappingTable, (const void *)(uintptr_t)codepage, (const void **)&value))
//            return (RSStringEncoding)value;
        value = (uint32_t)RSDictionaryGetValue(mappingTable, codepage);
        
        return (RSStringEncoding)value;
    }
    
    
    return RSStringEncodingInvalidId;
}

RSPrivate BOOL __RSStringEncodingGetCanonicalName(RSStringEncoding encoding, char *buffer, RSIndex bufferSize) {
    const char *format = "%s";
    const char *name = nil;
    uint32_t value = 0;
    RSIndex index;
    
    switch (encoding & 0x0F00) {
        case 0x0100: // UTF range
            switch (encoding) {
                case RSStringEncodingUTF7: name = "utf-7"; break;
                case RSStringEncodingUTF8: name = "utf-8"; break;
                case RSStringEncodingUTF16: name = "utf-16"; break;
                case RSStringEncodingUTF16BE: name = "utf-16be"; break;
                case RSStringEncodingUTF16LE: name = "utf-16le"; break;
                case RSStringEncodingUTF32: name = "utf-32"; break;
                case RSStringEncodingUTF32BE: name = "utf-32be"; break;
                case RSStringEncodingUTF32LE: name = "utf-32le"; break;
            }
            break;
            
        case 0x0200: // ISO 8859 range
            format = "iso-8859-%d";
            value = (encoding & 0xFF);
            break;
            
        case 0x0400: // DOS code page range
        case 0x0500: // Windows code page range
            index = __RSGetEncodingIndex(encoding);
            
            if (RSNotFound != index) {
                value = __RSWindowsCPList[index];
                if (0 != value) format = ((0x0400 == (encoding & 0x0F00)) ? "cp%d" : "windows-%d");
            }
            break;
            
        default: // others
            index = __RSGetEncodingIndex(encoding);
            
            if (RSNotFound != index) {
                if (((0 == (encoding & 0x0F00)) && (RSStringEncodingMacRoman != encoding)) || (RSStringEncodingMacRomanLatin1 == encoding)) format = "x-mac-%s";
                name = (const char *)__RSCanonicalNameList[index];
            }
            break;
    }
    
    if ((0 == value) && (nil == name)) {
        return NO;
    } else if (0 != value) {
        return ((snprintf(buffer, bufferSize, format, value) < bufferSize) ? YES : NO);
    } else {
        return ((snprintf(buffer, bufferSize, format, name) < bufferSize) ? YES : NO);
    }
}

#define LENGTH_LIMIT (256)
static BOOL __RSCanonicalNameCompare(const void *value1, const void *value2) { return 0 == strncasecmp_l((const char *)value1, (const char *)value2, LENGTH_LIMIT, nil) ? YES : NO; }

static RSHashCode __RSCanonicalNameHash(const void *value) {
    const char *name = (const char *)value;
    RSHashCode code = 0;
    
    while ((0 != *name) && ((name - (const char *)value) < LENGTH_LIMIT)) {
        char character = *(name++);
        
        code += (character + (((character >= 'A') && (character <= 'Z')) ? 'a' - 'A' : 0));
    }
    
    return code * (name - (const char *)value);
}

RSPrivate RSStringEncoding __RSStringEncodingGetFromCanonicalName(const char *canonicalName) {
    RSStringEncoding encoding;
    RSIndex prefixLength;
    static RSMutableDictionaryRef mappingTable = nil;
    static RSSpinLock lock = RSSpinLockInit;
    
    prefixLength = strlen("iso-8859-");
    if (0 == strncasecmp_l(canonicalName, "iso-8859-", prefixLength, nil)) {// do ISO
        encoding = (RSStringEncoding)strtol(canonicalName + prefixLength, nil, 10);
        
        return (((0 == encoding) || (encoding > 16)) ? RSStringEncodingInvalidId : encoding + 0x0200);
    }
    
    prefixLength = strlen("cp");
    if (0 == strncasecmp_l(canonicalName, "cp", prefixLength, nil)) {// do DOS
        encoding = (RSStringEncoding)strtol(canonicalName + prefixLength, nil, 10);
        
        return __RSStringEncodingGetFromWindowsCodePage(encoding);
    }
    
    prefixLength = strlen("windows-");
    if (0 == strncasecmp_l(canonicalName, "windows-", prefixLength, nil)) {// do DOS
        encoding = (RSStringEncoding)strtol(canonicalName + prefixLength, nil, 10);
        
        return __RSStringEncodingGetFromWindowsCodePage(encoding);
    }
    
    RSSpinLockLock(&lock);
    if (nil == mappingTable) {
        RSIndex index, count = sizeof(__RSKnownEncodingList) / sizeof(*__RSKnownEncodingList);
        
        RSDictionaryKeyContext keys = {
            nil, nil, nil, nil, &__RSCanonicalNameHash, &__RSCanonicalNameCompare
        };
        RSDictionaryContext context = {
            0, &keys, RSDictionaryDoNilValueContext
        };
        
        mappingTable = RSDictionaryCreateMutable(RSAllocatorSystemDefault, 0, &context);
        RSAutorelease(mappingTable);
        // Add UTFs
        RSDictionarySetValue(mappingTable, "utf-7", (const void *)RSStringEncodingUTF7);
        RSDictionarySetValue(mappingTable, "utf-8", (const void *)RSStringEncodingUTF8);
        RSDictionarySetValue(mappingTable, "utf-16", (const void *)RSStringEncodingUTF16);
        RSDictionarySetValue(mappingTable, "utf-16be", (const void *)RSStringEncodingUTF16BE);
        RSDictionarySetValue(mappingTable, "utf-16le", (const void *)RSStringEncodingUTF16LE);
        RSDictionarySetValue(mappingTable, "utf-32", (const void *)RSStringEncodingUTF32);
        RSDictionarySetValue(mappingTable, "utf-32be", (const void *)RSStringEncodingUTF32BE);
        RSDictionarySetValue(mappingTable, "utf-32le", (const void *)RSStringEncodingUTF32LE);
        
        for (index = 0;index < count;index++) {
            if (nil != __RSCanonicalNameList[index]) RSDictionarySetValue(mappingTable, (const void *)(uintptr_t)__RSCanonicalNameList[index], (const void *)(uintptr_t)__RSKnownEncodingList[index]);
        }
    }
    RSSpinLockUnlock(&lock);
    
    if (0 == strncasecmp_l(canonicalName, "macintosh", sizeof("macintosh") - 1, nil)) return RSStringEncodingMacRoman;
    
    
    prefixLength = strlen("x-mac-");
    encoding = (RSStringEncoding)(RSIndex)RSDictionaryGetValue(mappingTable, canonicalName + ((0 == strncasecmp_l(canonicalName, "x-mac-", prefixLength, nil)) ? prefixLength : 0));
    
    return ((0 == encoding) ? RSStringEncodingInvalidId : encoding);
}
#undef LENGTH_LIMIT

#if DEPLOYMENT_TARGET_MACOSX
// This list indexes from DOS range
static uint16_t __RSISO8859SimilarScriptList[] = {
    RSStringEncodingMacRoman,
    RSStringEncodingMacCentralEurRoman,
    RSStringEncodingMacRoman,
    RSStringEncodingMacCentralEurRoman,
    RSStringEncodingMacCyrillic,
    RSStringEncodingMacArabic,
    RSStringEncodingMacGreek,
    RSStringEncodingMacHebrew,
    RSStringEncodingMacTurkish,
    RSStringEncodingMacInuit,
    RSStringEncodingMacThai,
    RSStringEncodingMacRoman,
    RSStringEncodingMacCentralEurRoman,
    RSStringEncodingMacCeltic,
    RSStringEncodingMacRoman,
    RSStringEncodingMacRomanian};

static uint16_t __RSOtherSimilarScriptList[] = {
    RSStringEncodingMacRoman,
    RSStringEncodingMacGreek,
    RSStringEncodingMacCentralEurRoman,
    RSStringEncodingMacRoman,
    RSStringEncodingMacGreek,
    RSStringEncodingMacCentralEurRoman,
    RSStringEncodingMacCyrillic,
    RSStringEncodingMacTurkish,
    RSStringEncodingMacRoman,
    RSStringEncodingMacIcelandic,
    RSStringEncodingMacHebrew,
    RSStringEncodingMacRoman,
    RSStringEncodingMacArabic,
    RSStringEncodingMacInuit,
    RSStringEncodingMacCyrillic,
    RSStringEncodingMacGreek,
    RSStringEncodingMacThai,
    RSStringEncodingMacJapanese,
    RSStringEncodingMacChineseSimp,
    RSStringEncodingMacKorean,
    RSStringEncodingMacChineseTrad,
    
    RSStringEncodingMacRoman,
    RSStringEncodingMacCentralEurRoman,
    RSStringEncodingMacCyrillic,
    RSStringEncodingMacGreek,
    RSStringEncodingMacTurkish,
    RSStringEncodingMacHebrew,
    RSStringEncodingMacArabic,
    RSStringEncodingMacCentralEurRoman,
    RSStringEncodingMacVietnamese,
    RSStringEncodingMacKorean,
    
    RSStringEncodingMacRoman,
    
    RSStringEncodingMacJapanese,
    RSStringEncodingMacChineseSimp,
    
    RSStringEncodingMacJapanese,
    RSStringEncodingMacJapanese,
    RSStringEncodingMacJapanese,
    RSStringEncodingMacJapanese,
    RSStringEncodingMacChineseSimp,
    RSStringEncodingMacChineseSimp,
    RSStringEncodingMacKorean,
    RSStringEncodingMacJapanese,
    RSStringEncodingMacChineseSimp,
    RSStringEncodingMacChineseTrad,
    RSStringEncodingMacKorean,
    
    RSStringEncodingMacJapanese,
    
    RSStringEncodingMacCyrillic,
    
    RSStringEncodingMacChineseTrad,
    
    RSStringEncodingMacRoman,
    RSStringEncodingMacChineseSimp,
    RSStringEncodingMacChineseTrad,
    RSStringEncodingMacVietnamese,
    RSStringEncodingMacUkrainian,
    RSStringEncodingMacChineseTrad,
    RSStringEncodingMacRoman,
    
    RSStringEncodingMacRoman,
    
    RSStringEncodingMacRoman
};

static const char *__RSISONameList[] = {
    "Western (ISO Latin 1)",
    "Central European (ISO Latin 2)",
    "Western (ISO Latin 3)",
    "Central European (ISO Latin 4)",
    "Cyrillic (ISO 8859-5)",
    "Arabic (ISO 8859-6)",
    "Greek (ISO 8859-7)",
    "Hebrew (ISO 8859-8)",
    "Turkish (ISO Latin 5)",
    "Nordic (ISO Latin 6)",
    "Thai (ISO 8859-11)",
    nil,
    "Baltic (ISO Latin 7)",
    "Celtic (ISO Latin 8)",
    "Western (ISO Latin 9)",
    "Romanian (ISO Latin 10)",
};

static const char *__RSOtherNameList[] = {
    "Western (Mac OS Roman)",
    "Japanese (Mac OS)",
    "Traditional Chinese (Mac OS)",
    "Korean (Mac OS)",
    "Arabic (Mac OS)",
    "Hebrew (Mac OS)",
    "Greek (Mac OS)",
    "Cyrillic (Mac OS)",
    "Devanagari (Mac OS)",
    "Gurmukhi (Mac OS)",
    "Gujarati (Mac OS)",
    "Oriya (Mac OS)",
    "Bengali (Mac OS)",
    "Tamil (Mac OS)",
    "Telugu (Mac OS)",
    "Kannada (Mac OS)",
    "Malayalam (Mac OS)",
    "Sinhalese (Mac OS)",
    "Burmese (Mac OS)",
    "Khmer (Mac OS)",
    "Thai (Mac OS)",
    "Laotian (Mac OS)",
    "Georgian (Mac OS)",
    "Armenian (Mac OS)",
    "Simplified Chinese (Mac OS)",
    "Tibetan (Mac OS)",
    "Mongolian (Mac OS)",
    "Ethiopic (Mac OS)",
    "Central European (Mac OS)",
    "Vietnamese (Mac OS)",
    "Symbol (Mac OS)",
    "Dingbats (Mac OS)",
    "Turkish (Mac OS)",
    "Croatian (Mac OS)",
    "Icelandic (Mac OS)",
    "Romanian (Mac OS)",
    "Celtic (Mac OS)",
    "Gaelic (Mac OS)",
    "Farsi (Mac OS)",
    "Cyrillic (Mac OS Ukrainian)",
    "Inuit (Mac OS)",
    "Latin-US (DOS)",
    "Greek (DOS)",
    "Baltic (DOS)",
    "Western (DOS Latin 1)",
    "Greek (DOS Greek 1)",
    "Central European (DOS Latin 2)",
    "Cyrillic (DOS)",
    "Turkish (DOS)",
    "Portuguese (DOS)",
    "Icelandic (DOS)",
    "Hebrew (DOS)",
    "Canadian French (DOS)",
    "Arabic (DOS)",
    "Nordic (DOS)",
    "Russian (DOS)",
    "Greek (DOS Greek 2)",
    "Thai (Windows, DOS)",
    "Japanese (Windows, DOS)",
    "Simplified Chinese (Windows, DOS)",
    "Korean (Windows, DOS)",
    "Traditional Chinese (Windows, DOS)",
    "Western (Windows Latin 1)",
    "Central European (Windows Latin 2)",
    "Cyrillic (Windows)",
    "Greek (Windows)",
    "Turkish (Windows Latin 5)",
    "Hebrew (Windows)",
    "Arabic (Windows)",
    "Baltic (Windows)",
    "Vietnamese (Windows)",
    "Korean (Windows Johab)",
    "Western (ASCII)",
    "Japanese (Shift JIS X0213)",
    "Chinese (GB 18030)",
    "Japanese (ISO 2022-JP)",
    "Japanese (ISO 2022-JP-2)",
    "Japanese (ISO 2022-JP-1)",
    "Japanese (ISO 2022-JP-3)",
    "Chinese (ISO 2022-CN)",
    "Chinese (ISO 2022-CN-EXT)",
    "Korean (ISO 2022-KR)",
    "Japanese (EUC)",
    "Simplified Chinese (GB 2312)",
    "Traditional Chinese (EUC)",
    "Korean (EUC)",
    "Japanese (Shift JIS)",
    "Cyrillic (KOI8-R)",
    "Traditional Chinese (Big 5)",
    "Western (Mac Mail)",
    "Simplified Chinese (HZ GB 2312)",
    "Traditional Chinese (Big 5 HKSCS)",
    nil,
    "Ukrainian (KOI8-U)",
    "Traditional Chinese (Big 5-E)",
    nil,
    "Western (NextStep)",
    "Western (EBCDIC Latin 1)",
};
#endif /* DEPLOYMENT_TARGET_MACOSX */

RSPrivate RSStringEncoding __RSStringEncodingGetMostCompatibleMacScript(RSStringEncoding encoding) {
#if DEPLOYMENT_TARGET_MACOSX
    switch (encoding & 0x0F00) {
        case 0: return encoding & 0xFF; break; // Mac scripts
            
        case 0x0100: return RSStringEncodingUnicode; break; // Unicode
            
        case 0x200: // ISO 8859
            return (((encoding & 0xFF) <= (sizeof(__RSISO8859SimilarScriptList) / sizeof(*__RSISO8859SimilarScriptList))) ? __RSISO8859SimilarScriptList[(encoding & 0xFF) - 1] : RSStringEncodingInvalidId);
            break;
            
        default: {
            RSIndex index = __RSGetEncodingIndex(encoding);
            
            if (RSNotFound != index) {
                index -= __RSGetEncodingIndex(RSStringEncodingDOSLatinUS);
                return __RSOtherSimilarScriptList[index];
            }
        }
    }
#endif /* DEPLOYMENT_TARGET_MACOSX */
    
    return RSStringEncodingInvalidId;
}

RSPrivate const char *__RSStringEncodingGetName(RSStringEncoding encoding) {
    switch (encoding) {
        case RSStringEncodingUTF8: return "Unicode (UTF-8)"; break;
        case RSStringEncodingUTF16: return "Unicode (UTF-16)"; break;
        case RSStringEncodingUTF16BE: return "Unicode (UTF-16BE)"; break;
        case RSStringEncodingUTF16LE: return "Unicode (UTF-16LE)"; break;
        case RSStringEncodingUTF32: return "Unicode (UTF-32)"; break;
        case RSStringEncodingUTF32BE: return "Unicode (UTF-32BE)"; break;
        case RSStringEncodingUTF32LE: return "Unicode (UTF-32LE)"; break;
        case RSStringEncodingNonLossyASCII: return "Non-lossy ASCII"; break;
        case RSStringEncodingUTF7: return "Unicode (UTF-7)"; break;
    }
    
#if DEPLOYMENT_TARGET_MACOSX
    if (0x0200 == (encoding & 0x0F00)) {
        encoding &= 0x00FF;
        
        if (encoding <= (sizeof(__RSISONameList) / sizeof(*__RSISONameList))) return __RSISONameList[encoding - 1];
    } else {
        RSIndex index = __RSGetEncodingIndex(encoding);
        
        if (RSNotFound != index) return __RSOtherNameList[index];
    }
#endif /* DEPLOYMENT_TARGET_MACOSX */
    
    return nil;
}

