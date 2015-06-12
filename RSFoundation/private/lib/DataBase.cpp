//
//  DataBase.cpp
//  RSCoreFoundation
//
//  Created by closure on 6/2/15.
//  Copyright (c) 2015 RetVal. All rights reserved.
//

#include <RSFoundation/DataBase.hpp>
#include <RSFoundation/Allocator.hpp>
#include <RSFoundation/String.hpp>
#include <RSFoundation/Lock.hpp>
#include "Internal.hpp"
#include <map>
#include <unordered_map>

namespace RSFoundation {
    namespace Collection {
        namespace Encoding {
            #define ISO8859CODEPAGE_BASE (28590)
            constexpr String::Encoding DataBase::KnownEncodingList[];
            constexpr UInt16 DataBase::WindowsCPList[]; // Windows codepage mapping
            constexpr const char *DataBase::CanonicalNameList[]; // Canonical name
            
            const DataBase& DataBase::SharedDataBase() {
                static DataBase db;
                return db;
            }
            
            DataBase::DataBase() {}
            
            DataBase::~DataBase() {}
            
            UInt16 DataBase::GetWindowsCodePage(String::Encoding enc) const {
                Index encoding = enc;
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
                    Index index = _GetEncodingIndex(String::Encoding(encoding));
                    if (NotFound != index) return DataBase::WindowsCPList[index];
                }
                
                return 0;
            }
            
            String::Encoding DataBase::GetFromWindowsCodePage(UInt16 codepage) const {
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
                    static std::map<UInt16, String::Encoding> *mappingTable = nullptr;
                    static SpinLock lock;
                    
                    lock.Acquire();
                    if (nullptr == mappingTable) {
                        Index index, count = sizeof(DataBase::KnownEncodingList) / sizeof(*DataBase::KnownEncodingList);
                        
                        mappingTable = Allocator<std::map<UInt16, String::Encoding>>::AllocatorSystemDefault.Allocate();
                        //                        Autorelease(mappingTable);
                        for (index = 0;index < count;index++) {
                            if (0 != DataBase::WindowsCPList[index]) {
                                (*mappingTable)[DataBase::WindowsCPList[index]] = DataBase::KnownEncodingList[index];
                            }
                        }
                    }
                    lock.Release();
                    
                    //        if (DictionaryGetValueIfPresent(mappingTable, (const void *)(uintptr_t)codepage, (const void **)&value))
                    //            return (String::Encoding::)value;
                    return (*mappingTable)[codepage];
                }
                return String::Encoding::InvalidId;
            }
            
            bool DataBase::GetCanonicalName(String::Encoding encoding, char *buffer, Index bufferSize) const {
                const char *format = "%s";
                const char *name = nullptr;
                uint32_t value = 0;
                Index index;
                
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
                        index = _GetEncodingIndex(encoding);
                        
                        if (NotFound != index) {
                            value = DataBase::WindowsCPList[index];
                            if (0 != value) format = ((0x0400 == (encoding & 0x0F00)) ? "cp%d" : "windows-%d");
                        }
                        break;
                        
                    default: // others
                        index = _GetEncodingIndex(encoding);
                        
                        if (NotFound != index) {
                            if (((0 == (encoding & 0x0F00)) && (String::Encoding::MacRoman != encoding)) || (String::Encoding::MacRomanLatin1 == encoding)) format = "x-mac-%s";
                            name = (const char *)DataBase::CanonicalNameList[index];
                        }
                        break;
                }
                
                if ((0 == value) && (nullptr == name)) {
                    return NO;
                } else if (0 != value) {
                    return ((snprintf(buffer, bufferSize, format, value) < bufferSize) ? YES : NO);
                } else {
                    return ((snprintf(buffer, bufferSize, format, name) < bufferSize) ? YES : NO);
                }
            }
            
#define LENGTH_LIMIT (256)
//            static bool __CanonicalNameCompare(const void *value1, const void *value2) { return 0 == strncasecmp_l((const char *)value1, (const char *)value2, LENGTH_LIMIT, nullptr) ? YES : NO; }
            
//            static Index __CanonicalNameHash(const void *value) {
//                const char *name = (const char *)value;
//                Index code = 0;
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
                    Index code = 0;
                    
                    while ((0 != *name) && ((name - (const char *)value) < LENGTH_LIMIT)) {
                        char character = *(name++);
                        code += (character + (((character >= 'A') && (character <= 'Z')) ? 'a' - 'A' : 0));
                    }
                    
                    return code * (name - (const char *)value);
                }
            };
            
            String::Encoding DataBase::GetFromCanonicalName(const char *canonicalName) const {
                String::Encoding encoding;
                Index prefixLength;
                static std::unordered_map<const char *, String::Encoding, CanonicalNameHash, CanonicalNameCompare> *mappingTable = nullptr;
                static SpinLock lock;
                
                prefixLength = strlen("iso-8859-");
                if (0 == strncasecmp_l(canonicalName, "iso-8859-", prefixLength, nullptr)) {// do ISO
                    encoding = (String::Encoding)strtol(canonicalName + prefixLength, nullptr, 10);
                    
                    return (((0 == encoding) || (encoding > 16)) ? String::Encoding::InvalidId : String::Encoding(encoding + 0x0200));
                }
                
                prefixLength = strlen("cp");
                if (0 == strncasecmp_l(canonicalName, "cp", prefixLength, nullptr)) {// do DOS
                    encoding = (String::Encoding)strtol(canonicalName + prefixLength, nullptr, 10);
                    
                    return GetFromWindowsCodePage(encoding);
                }
                
                prefixLength = strlen("windows-");
                if (0 == strncasecmp_l(canonicalName, "windows-", prefixLength, nullptr)) {// do DOS
                    encoding = (String::Encoding)strtol(canonicalName + prefixLength, nullptr, 10);
                    
                    return GetFromWindowsCodePage(encoding);
                }
                
                lock.Acquire();
                if (nullptr == mappingTable) {
                    Index index, count = sizeof(DataBase::KnownEncodingList) / sizeof(*DataBase::KnownEncodingList);
                    
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
                        if (nullptr != DataBase::CanonicalNameList[index]) {
                            (*mappingTable)[DataBase::CanonicalNameList[index]] = DataBase::KnownEncodingList[index];
                        }
                    }
                    
                    //                    for (index = 0;index < count;index++) {
                    //                        if (nullptr != __CanonicalNameList[index]) DictionarySetValue(mappingTable, (const void *)(uintptr_t)__CanonicalNameList[index], (const void *)(uintptr_t)__KnownEncodingList[index]);
                    //                    }
                }
                lock.Release();
                //                SpinLockUnlock(&lock);
                
                if (0 == strncasecmp_l(canonicalName, "macintosh", sizeof("macintosh") - 1, nullptr)) return String::Encoding::MacRoman;
                
                
                prefixLength = strlen("x-mac-");
                encoding = (String::Encoding)(Index)(*mappingTable)[canonicalName + ((0 == strncasecmp_l(canonicalName, "x-mac-", prefixLength, nullptr)) ? prefixLength : 0)];
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
                nullptr,
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
                nullptr,
                "Ukrainian (KOI8-U)",
                "Traditional Chinese (Big 5-E)",
                nullptr,
                "Western (NextStep)",
                "Western (EBCDIC Latin 1)",
            };
#endif /* DEPLOYMENT_TARGET_MACOSX */
            
            String::Encoding DataBase::GetMostCompatibleMacScript(String::Encoding enc) const {
#if DEPLOYMENT_TARGET_MACOSX
                Index encoding = (Index)enc;
                switch (encoding & 0x0F00) {
                    case 0: return String::Encoding(encoding & 0xFF); break; // Mac scripts
                        
                    case 0x0100: return String::Encoding::Unicode; break; // Unicode
                        
                    case 0x200: // ISO 8859
                        return String::Encoding(((encoding & 0xFF) <= (sizeof(__ISO8859SimilarScriptList) / sizeof(*__ISO8859SimilarScriptList))) ? __ISO8859SimilarScriptList[(encoding & 0xFF) - 1] : String::Encoding::InvalidId);
                        break;
                        
                    default: {
                        Index index = _GetEncodingIndex(String::Encoding(encoding));
                        
                        if (NotFound != index) {
                            index -= _GetEncodingIndex(String::Encoding::DOSLatinUS);
                            return String::Encoding(__OtherSimilarScriptList[index]);
                        }
                    }
                }
#endif /* DEPLOYMENT_TARGET_MACOSX */
                
                return String::Encoding::InvalidId;
            }
            
            const char* DataBase::GetName(String::Encoding enc) const {
                Index encoding = enc;
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
                    Index index = _GetEncodingIndex(String::Encoding(encoding));
                    
                    if (NotFound != index) return __OtherNameList[index];
                }
#endif /* DEPLOYMENT_TARGET_MACOSX */
                
                return nullptr;
            }
        }
    }
}