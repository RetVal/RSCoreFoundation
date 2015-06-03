//
//  DataBase.h
//  RSCoreFoundation
//
//  Created by closure on 6/2/15.
//  Copyright (c) 2015 RetVal. All rights reserved.
//

#ifndef __CoreFoundation__DataBase__
#define __CoreFoundation__DataBase__

#include <RSFoundation/String.hpp>

namespace RSFoundation {
    namespace Collection {
        namespace Encoding {
            
            class DataBase : public Object, public NotCopyable {
            private:
                DataBase();
                ~DataBase();
                
            public:
                
                static const DataBase& SharedDataBase();
                
            public:
                UInt16 GetWindowsCodePage(String::Encoding encoding) const;
                String::Encoding GetFromWindowsCodePage(UInt16 codepage) const;
                bool GetCanonicalName(String::Encoding encoding, char *buf, Index bufferSize) const;
                String::Encoding GetFromCanonicalName(const char *canonicalName) const;
                String::Encoding GetMostCompatibleMacScript(String::Encoding encoding) const;
                const char* GetName(String::Encoding encoding) const; // Returns simple non-localizd name
                
            private:
                static inline Index _GetEncodingIndex(String::Encoding enc) {
                    Index encoding = enc;
                    const String::Encoding *head = KnownEncodingList;
                    const String::Encoding *tail = head + ((sizeof(KnownEncodingList) / sizeof(*KnownEncodingList)) - 1);
                    const String::Encoding *middle;
                    
                    encoding &= 0x0FFF;
                    while (head <= tail) {
                        middle = head + ((tail - head) >> 1);
                        
                        if (encoding == *middle) {
                            return middle - KnownEncodingList;
                        } else if (encoding < *middle) {
                            tail = middle - 1;
                        } else {
                            head = middle + 1;
                        }
                    }
                    return NotFound;
                }
                
            private:
                constexpr static const String::Encoding KnownEncodingList[] = {
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
                
                constexpr static const UInt16 WindowsCPList[] = {
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
                
                constexpr static const char *CanonicalNameList[] = {
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
                    
                    nullptr,
                    nullptr,
                    nullptr,
                    nullptr,
                    nullptr,
                    nullptr,
                    nullptr,
                    nullptr,
                    nullptr,
                    nullptr,
                    nullptr,
                    nullptr,
                    nullptr,
                    nullptr,
                    nullptr,
                    nullptr,
                    nullptr,
                    nullptr,
                    nullptr,
                    nullptr,
                    nullptr,
                    
                    nullptr,
                    nullptr,
                    nullptr,
                    nullptr,
                    nullptr,
                    nullptr,
                    nullptr,
                    nullptr,
                    nullptr,
                    nullptr,
                    
                    "us-ascii",
                    
                    nullptr,
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
                    nullptr,
                    "utf7-imap",
                    
                    "x-nextstep",
                    
                    "ibm037",
                };
            };
        }
    }
}

#endif /* defined(__CoreFoundation__DataBase__) */
