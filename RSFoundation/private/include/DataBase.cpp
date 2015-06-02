//
//  DataBase.cpp
//  RSCoreFoundation
//
//  Created by closure on 6/2/15.
//  Copyright (c) 2015 RetVal. All rights reserved.
//

#include <RSFoundation/DataBase.h>
#include <RSFoundation/Allocator.h>
#include <RSFoundation/String.hpp>
#include <RSFoundation/Lock.h>
#include "Internal.h"
#include <map>
#include <unordered_map>
#include <stdio.h>
//#include "String::Encoding::.h"
//#include "String::Encoding::ConverterPrivate.h"
//#include "String::Encoding::Database.h"
//#include "RSICUConverters.h"

namespace RSFoundation {
    namespace Collection {
        namespace Encoding {
            
#define ISO8859CODEPAGE_BASE (28590)
            
            static const String::Encoding __KnownEncodingList[] = {
                String::Encoding::MacRoman,
                String::Encoding::MacJapanese,
                String::Encoding::MacChineseTrad,
                String::Encoding::MacKorean,
                String::Encoding::MacArabic,
                String::Encoding::MacHebrew,
                String::Encoding::MacGreek,
                String::Encoding::MacCyrillic,
                String::Encoding::MacDevanagari,
                String::Encoding::MacGurmukhi,
                String::Encoding::MacGujarati,
                String::Encoding::MacOriya,
                String::Encoding::MacBengali,
                String::Encoding::MacTamil,
                String::Encoding::MacTelugu,
                String::Encoding::MacKannada,
                String::Encoding::MacMalayalam,
                String::Encoding::MacSinhalese,
                String::Encoding::MacBurmese,
                String::Encoding::MacKhmer,
                String::Encoding::MacThai,
                String::Encoding::MacLaotian,
                String::Encoding::MacGeorgian,
                String::Encoding::MacArmenian,
                String::Encoding::MacChineseSimp,
                String::Encoding::MacTibetan,
                String::Encoding::MacMongolian,
                String::Encoding::MacEthiopic,
                String::Encoding::MacCentralEurRoman,
                String::Encoding::MacVietnamese,
                String::Encoding::MacSymbol,
                String::Encoding::MacDingbats,
                String::Encoding::MacTurkish,
                String::Encoding::MacCroatian,
                String::Encoding::MacIcelandic,
                String::Encoding::MacRomanian,
                String::Encoding::MacCeltic,
                String::Encoding::MacGaelic,
                String::Encoding::MacFarsi,
                String::Encoding::MacUkrainian,
                String::Encoding::MacInuit,
                
                String::Encoding::DOSLatinUS,
                String::Encoding::DOSGreek,
                String::Encoding::DOSBalticRim,
                String::Encoding::DOSLatin1,
                String::Encoding::DOSGreek1,
                String::Encoding::DOSLatin2,
                String::Encoding::DOSCyrillic,
                String::Encoding::DOSTurkish,
                String::Encoding::DOSPortuguese,
                String::Encoding::DOSIcelandic,
                String::Encoding::DOSHebrew,
                String::Encoding::DOSCanadianFrench,
                String::Encoding::DOSArabic,
                String::Encoding::DOSNordic,
                String::Encoding::DOSRussian,
                String::Encoding::DOSGreek2,
                String::Encoding::DOSThai,
                String::Encoding::DOSJapanese,
                String::Encoding::DOSChineseSimplif,
                String::Encoding::DOSKorean,
                String::Encoding::DOSChineseTrad,
                
                String::Encoding::WindowsLatin1,
                String::Encoding::WindowsLatin2,
                String::Encoding::WindowsCyrillic,
                String::Encoding::WindowsGreek,
                String::Encoding::WindowsLatin5,
                String::Encoding::WindowsHebrew,
                String::Encoding::WindowsArabic,
                String::Encoding::WindowsBalticRim,
                String::Encoding::WindowsVietnamese,
                String::Encoding::WindowsKoreanJohab,
                String::Encoding::ASCII,
                
                String::Encoding::ShiftJIS_X0213,
                String::Encoding::GB_18030_2000,
                
                String::Encoding::ISO_2022_JP,
                String::Encoding::ISO_2022_JP_2,
                String::Encoding::ISO_2022_JP_1,
                String::Encoding::ISO_2022_JP_3,
                String::Encoding::ISO_2022_CN,
                String::Encoding::ISO_2022_CN_EXT,
                String::Encoding::ISO_2022_KR,
                String::Encoding::EUC_JP,
                String::Encoding::EUC_CN,
                String::Encoding::EUC_TW,
                String::Encoding::EUC_KR,
                
                String::Encoding::ShiftJIS,
                
                String::Encoding::KOI8_R,
                
                String::Encoding::Big5,
                
                String::Encoding::MacRomanLatin1,
                String::Encoding::HZ_GB_2312,
                String::Encoding::Big5_HKSCS_1999,
                String::Encoding::VISCII,
                String::Encoding::KOI8_U,
                String::Encoding::Big5_E,
                String::Encoding::UTF7_IMAP,
                
                String::Encoding::NextStepLatin,
                
                String::Encoding::EBCDIC_CP037
            };
            
            // Windows codepage mapping
            static const uint16_t __WindowsCPList[] = {
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
                
                50221, // we prefere this over 50220/50221 since that's what  coverter generates
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
            static const char *__CanonicalNameList[] = {
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
            
            static inline RSIndex __GetEncodingRSIndex(String::Encoding enc) {
                RSIndex encoding = enc;
                const String::Encoding *head = __KnownEncodingList;
                const String::Encoding *tail = head + ((sizeof(__KnownEncodingList) / sizeof(*__KnownEncodingList)) - 1);
                const String::Encoding *middle;
                
                encoding &= 0x0FFF;
                while (head <= tail) {
                    middle = head + ((tail - head) >> 1);
                    
                    if (encoding == *middle) {
                        return middle - __KnownEncodingList;
                    } else if (encoding < *middle) {
                        tail = middle - 1;
                    } else {
                        head = middle + 1;
                    }
                }
                
                return NotFound;
            }
            
            Private uint16_t __StringEncodingGetWindowsCodePage(String::Encoding enc) {
                RSIndex encoding = enc;
                String::Encoding encodingBase = String::Encoding(encoding & 0x0F00);
                
                if (0x0100 == encodingBase) { // UTF
                    switch (encoding) {
                        case String::Encoding::UTF7: return 65000;
                        case String::Encoding::UTF8: return 65001;
                        case String::Encoding::UTF16: return 1200;
                        case String::Encoding::UTF16BE: return 1201;
                        case String::Encoding::UTF32: return 65005;
                        case String::Encoding::UTF32BE: return 65006;
                    }
                } else if (0x0200 == encodingBase) { // ISO 8859 range
                    return ISO8859CODEPAGE_BASE + (encoding & 0xFF);
                } else { // others
                    RSIndex index = __GetEncodingRSIndex(String::Encoding(encoding));
                    
                    if (NotFound != index) return __WindowsCPList[index];
                }
                
                return 0;
            }
            
            Private String::Encoding __StringEncodingGetFromWindowsCodePage(uint16_t codepage) {
                switch (codepage) {
                    case 65001: return String::Encoding::UTF8;
                    case 1200: return String::Encoding::UTF16;
                    case 0: return String::Encoding::InvalidId;
                    case 1201: return String::Encoding::UTF16BE;
                    case 65005: return String::Encoding::UTF32;
                    case 65006: return String::Encoding::UTF32BE;
                    case 65000: return String::Encoding::UTF7;
                }
                
                if ((codepage > ISO8859CODEPAGE_BASE) && (codepage <= (ISO8859CODEPAGE_BASE + 16))) {
                    return String::Encoding((codepage - ISO8859CODEPAGE_BASE) + 0x0200);
                } else {
                    static std::map<uint16_t, String::Encoding> *mappingTable = nullptr;
                    static SpinLock lock;
                    uintptr_t value;
                    
                    lock.Acquire();
                    if (nullptr == mappingTable) {
                        RSIndex index, count = sizeof(__KnownEncodingList) / sizeof(*__KnownEncodingList);
                        
                        mappingTable = Allocator<std::map<uint16_t, String::Encoding>>::AllocatorSystemDefault.Allocate();
//                        Autorelease(mappingTable);
                        for (index = 0;index < count;index++) {
                            if (0 != __WindowsCPList[index]) {
                                (*mappingTable)[__WindowsCPList[index]] = __KnownEncodingList[index];
                            }
                        }
                    }
                    lock.Release();
                    
                    //        if (DictionaryGetValueIfPresent(mappingTable, (const void *)(uintptr_t)codepage, (const void **)&value))
                    //            return (String::Encoding::)value;
                    value = (uint32_t)(*mappingTable)[codepage];
                    return (String::Encoding)value;
                }
                
                
                return String::Encoding::InvalidId;
            }
            
            Private bool __StringEncodingGetCanonicalName(String::Encoding encoding, char *buffer, RSIndex bufferSize) {
                const char *format = "%s";
                const char *name = nil;
                uint32_t value = 0;
                RSIndex index;
                
                switch (encoding & 0x0F00) {
                    case 0x0100: // UTF range
                        switch (encoding) {
                            case String::Encoding::UTF7: name = "utf-7"; break;
                            case String::Encoding::UTF8: name = "utf-8"; break;
                            case String::Encoding::UTF16: name = "utf-16"; break;
                            case String::Encoding::UTF16BE: name = "utf-16be"; break;
                            case String::Encoding::UTF16LE: name = "utf-16le"; break;
                            case String::Encoding::UTF32: name = "utf-32"; break;
                            case String::Encoding::UTF32BE: name = "utf-32be"; break;
                            case String::Encoding::UTF32LE: name = "utf-32le"; break;
                            default: break;
                        }
                        break;
                        
                    case 0x0200: // ISO 8859 range
                        format = "iso-8859-%d";
                        value = (encoding & 0xFF);
                        break;
                        
                    case 0x0400: // DOS code page range
                    case 0x0500: // Windows code page range
                        index = __GetEncodingRSIndex(encoding);
                        
                        if (NotFound != index) {
                            value = __WindowsCPList[index];
                            if (0 != value) format = ((0x0400 == (encoding & 0x0F00)) ? "cp%d" : "windows-%d");
                        }
                        break;
                        
                    default: // others
                        index = __GetEncodingRSIndex(encoding);
                        
                        if (NotFound != index) {
                            if (((0 == (encoding & 0x0F00)) && (String::Encoding::MacRoman != encoding)) || (String::Encoding::MacRomanLatin1 == encoding)) format = "x-mac-%s";
                            name = (const char *)__CanonicalNameList[index];
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
//            static bool __CanonicalNameCompare(const void *value1, const void *value2) { return 0 == strncasecmp_l((const char *)value1, (const char *)value2, LENGTH_LIMIT, nil) ? YES : NO; }
            
//            static RSIndex __CanonicalNameHash(const void *value) {
//                const char *name = (const char *)value;
//                RSIndex code = 0;
//                
//                while ((0 != *name) && ((name - (const char *)value) < LENGTH_LIMIT)) {
//                    char character = *(name++);
//                    
//                    code += (character + (((character >= 'A') && (character <= 'Z')) ? 'a' - 'A' : 0));
//                }
//                
//                return code * (name - (const char *)value);
//            }
            
            struct CanonicalNameCompare {
                bool operator()(const char *value1, const char *value2) const {
                    return 0 == strncasecmp_l((const char *)value1, (const char *)value2, LENGTH_LIMIT, nullptr) ? YES : NO;
                }
            };
            
            struct CanonicalNameHash {
                std::size_t operator()(const char* value) const {
                    const char *name = (const char *)value;
                    RSIndex code = 0;
                    
                    while ((0 != *name) && ((name - (const char *)value) < LENGTH_LIMIT)) {
                        char character = *(name++);
                        code += (character + (((character >= 'A') && (character <= 'Z')) ? 'a' - 'A' : 0));
                    }
                    
                    return code * (name - (const char *)value);
                }
            };

            
            Private String::Encoding __StringEncodingGetFromCanonicalName(const char *canonicalName) {
                String::Encoding encoding;
                RSIndex prefixLength;
                static std::unordered_map<const char *, String::Encoding, CanonicalNameHash, CanonicalNameCompare> *mappingTable = nullptr;
                static SpinLock lock;
                
                prefixLength = strlen("iso-8859-");
                if (0 == strncasecmp_l(canonicalName, "iso-8859-", prefixLength, nil)) {// do ISO
                    encoding = (String::Encoding)strtol(canonicalName + prefixLength, nil, 10);
                    
                    return (((0 == encoding) || (encoding > 16)) ? String::Encoding::InvalidId : String::Encoding(encoding + 0x0200));
                }
                
                prefixLength = strlen("cp");
                if (0 == strncasecmp_l(canonicalName, "cp", prefixLength, nil)) {// do DOS
                    encoding = (String::Encoding)strtol(canonicalName + prefixLength, nil, 10);
                    
                    return __StringEncodingGetFromWindowsCodePage(encoding);
                }
                
                prefixLength = strlen("windows-");
                if (0 == strncasecmp_l(canonicalName, "windows-", prefixLength, nil)) {// do DOS
                    encoding = (String::Encoding)strtol(canonicalName + prefixLength, nil, 10);
                    
                    return __StringEncodingGetFromWindowsCodePage(encoding);
                }
                
                lock.Acquire();
                if (nil == mappingTable) {
                    RSIndex index, count = sizeof(__KnownEncodingList) / sizeof(*__KnownEncodingList);
                    
                    mappingTable = Allocator<std::unordered_map<const char*, String::Encoding, CanonicalNameHash, CanonicalNameCompare>>::AllocatorSystemDefault.Allocate();
//                    Autorelease(mappingTable);
                    // Add UTFs
                    (*mappingTable)["utf-7"] = String::Encoding::UTF7;
                    (*mappingTable)["utf-8"] = String::Encoding::UTF8;
                    (*mappingTable)["utf-16"] = String::Encoding::UTF16;
                    (*mappingTable)["utf-16be"] = String::Encoding::UTF16BE;
                    (*mappingTable)["utf-16le"] = String::Encoding::UTF16LE;
                    (*mappingTable)["utf-32"] = String::Encoding::UTF32;
                    (*mappingTable)["utf-32be"] = String::Encoding::UTF32BE;
                    (*mappingTable)["utf-32le"] = String::Encoding::UTF32LE;
                    
                    for (index = 0; index < count; ++index) {
                        if (nullptr != __CanonicalNameList[index]) {
                            (*mappingTable)[__CanonicalNameList[index]] = __KnownEncodingList[index];
                        }
                    }
                    
//                    for (index = 0;index < count;index++) {
//                        if (nil != __CanonicalNameList[index]) DictionarySetValue(mappingTable, (const void *)(uintptr_t)__CanonicalNameList[index], (const void *)(uintptr_t)__KnownEncodingList[index]);
//                    }
                }
                lock.Release();
//                SpinLockUnlock(&lock);
                
                if (0 == strncasecmp_l(canonicalName, "macintosh", sizeof("macintosh") - 1, nil)) return String::Encoding::MacRoman;
                
                
                prefixLength = strlen("x-mac-");
                encoding = (String::Encoding)(RSIndex)(*mappingTable)[canonicalName + ((0 == strncasecmp_l(canonicalName, "x-mac-", prefixLength, nil)) ? prefixLength : 0)];
                return ((0 == encoding) ? String::Encoding::InvalidId : encoding);
            }
#undef LENGTH_LIMIT
            
#if DEPLOYMENT_TARGET_MACOSX
            // This list indexes from DOS range
            static String::Encoding __ISO8859SimilarScriptList[] = {
                String::Encoding::MacRoman,
                String::Encoding::MacCentralEurRoman,
                String::Encoding::MacRoman,
                String::Encoding::MacCentralEurRoman,
                String::Encoding::MacCyrillic,
                String::Encoding::MacArabic,
                String::Encoding::MacGreek,
                String::Encoding::MacHebrew,
                String::Encoding::MacTurkish,
                String::Encoding::MacInuit,
                String::Encoding::MacThai,
                String::Encoding::MacRoman,
                String::Encoding::MacCentralEurRoman,
                String::Encoding::MacCeltic,
                String::Encoding::MacRoman,
                String::Encoding::MacRomanian};
            
            static String::Encoding __OtherSimilarScriptList[] = {
                String::Encoding::MacRoman,
                String::Encoding::MacGreek,
                String::Encoding::MacCentralEurRoman,
                String::Encoding::MacRoman,
                String::Encoding::MacGreek,
                String::Encoding::MacCentralEurRoman,
                String::Encoding::MacCyrillic,
                String::Encoding::MacTurkish,
                String::Encoding::MacRoman,
                String::Encoding::MacIcelandic,
                String::Encoding::MacHebrew,
                String::Encoding::MacRoman,
                String::Encoding::MacArabic,
                String::Encoding::MacInuit,
                String::Encoding::MacCyrillic,
                String::Encoding::MacGreek,
                String::Encoding::MacThai,
                String::Encoding::MacJapanese,
                String::Encoding::MacChineseSimp,
                String::Encoding::MacKorean,
                String::Encoding::MacChineseTrad,
                
                String::Encoding::MacRoman,
                String::Encoding::MacCentralEurRoman,
                String::Encoding::MacCyrillic,
                String::Encoding::MacGreek,
                String::Encoding::MacTurkish,
                String::Encoding::MacHebrew,
                String::Encoding::MacArabic,
                String::Encoding::MacCentralEurRoman,
                String::Encoding::MacVietnamese,
                String::Encoding::MacKorean,
                
                String::Encoding::MacRoman,
                
                String::Encoding::MacJapanese,
                String::Encoding::MacChineseSimp,
                
                String::Encoding::MacJapanese,
                String::Encoding::MacJapanese,
                String::Encoding::MacJapanese,
                String::Encoding::MacJapanese,
                String::Encoding::MacChineseSimp,
                String::Encoding::MacChineseSimp,
                String::Encoding::MacKorean,
                String::Encoding::MacJapanese,
                String::Encoding::MacChineseSimp,
                String::Encoding::MacChineseTrad,
                String::Encoding::MacKorean,
                
                String::Encoding::MacJapanese,
                
                String::Encoding::MacCyrillic,
                
                String::Encoding::MacChineseTrad,
                
                String::Encoding::MacRoman,
                String::Encoding::MacChineseSimp,
                String::Encoding::MacChineseTrad,
                String::Encoding::MacVietnamese,
                String::Encoding::MacUkrainian,
                String::Encoding::MacChineseTrad,
                String::Encoding::MacRoman,
                
                String::Encoding::MacRoman,
                
                String::Encoding::MacRoman
            };
            
            static const char *__ISONameList[] = {
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
            
            static const char *__OtherNameList[] = {
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
            
            Private String::Encoding __StringEncodingGetMostCompatibleMacScript(String::Encoding enc) {
#if DEPLOYMENT_TARGET_MACOSX
                RSIndex encoding = (RSIndex)enc;
                switch (encoding & 0x0F00) {
                    case 0: return String::Encoding(encoding & 0xFF); break; // Mac scripts
                        
                    case 0x0100: return String::Encoding::Unicode; break; // Unicode
                        
                    case 0x200: // ISO 8859
                        return String::Encoding(((encoding & 0xFF) <= (sizeof(__ISO8859SimilarScriptList) / sizeof(*__ISO8859SimilarScriptList))) ? __ISO8859SimilarScriptList[(encoding & 0xFF) - 1] : String::Encoding::InvalidId);
                        break;
                        
                    default: {
                        RSIndex index = __GetEncodingRSIndex(String::Encoding(encoding));
                        
                        if (NotFound != index) {
                            index -= __GetEncodingRSIndex(String::Encoding::DOSLatinUS);
                            return String::Encoding(__OtherSimilarScriptList[index]);
                        }
                    }
                }
#endif /* DEPLOYMENT_TARGET_MACOSX */
                
                return String::Encoding::InvalidId;
            }
            
            Private const char *__StringEncodingGetName(String::Encoding enc) {
                RSIndex encoding = enc;
                switch (encoding) {
                    case String::Encoding::UTF8: return "Unicode (UTF-8)"; break;
                    case String::Encoding::UTF16: return "Unicode (UTF-16)"; break;
                    case String::Encoding::UTF16BE: return "Unicode (UTF-16BE)"; break;
                    case String::Encoding::UTF16LE: return "Unicode (UTF-16LE)"; break;
                    case String::Encoding::UTF32: return "Unicode (UTF-32)"; break;
                    case String::Encoding::UTF32BE: return "Unicode (UTF-32BE)"; break;
                    case String::Encoding::UTF32LE: return "Unicode (UTF-32LE)"; break;
                    case String::Encoding::NonLossyASCII: return "Non-lossy ASCII"; break;
                    case String::Encoding::UTF7: return "Unicode (UTF-7)"; break;
                    default: break;
                }
                
#if DEPLOYMENT_TARGET_MACOSX
                if (0x0200 == (encoding & 0x0F00)) {
                    encoding &= 0x00FF;
                    
                    if (encoding <= (sizeof(__ISONameList) / sizeof(*__ISONameList))) return __ISONameList[encoding - 1];
                } else {
                    RSIndex index = __GetEncodingRSIndex(String::Encoding(encoding));
                    
                    if (NotFound != index) return __OtherNameList[index];
                }
#endif /* DEPLOYMENT_TARGET_MACOSX */
                
                return nil;
            }
            

        }
    }
}