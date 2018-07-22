//
//  RSStringCoder.c
//  RSCoreFoundation
//
//  Created by RetVal on 12/16/12.
//  Copyright (c) 2012 RetVal. All rights reserved.
//

#include <stdio.h>
#include "unicode/ucnv.h"
#include "unicode/uversion.h"
#include <wchar.h>
#include <RSCoreFoundation/RSStringEncoding.h>

#define RSStringSwitchEncoding(encoding, name)case (encoding):\
formatCStr = #name; break;

static RSCBuffer __RSStringTranslateEncoding(RSStringEncoding encoding)
{
    RSCBuffer formatCStr = "";
    switch (encoding)
    {
            RSStringSwitchEncoding(RSStringEncodingASCII, "US-ASCII");
            RSStringSwitchEncoding(RSStringEncodingMacRoman, "UTF-8");
            RSStringSwitchEncoding(RSStringEncodingWindowsLatin1, "ibm-850_P100-1995");
            RSStringSwitchEncoding(RSStringEncodingISOLatin1, "iso-8859_16-2001");
            RSStringSwitchEncoding(RSStringEncodingUTF8, "UTF-8");
            
            RSStringSwitchEncoding(RSStringEncodingUTF16, "UTF-16");   //RSStringEncodingUnicode
            RSStringSwitchEncoding(RSStringEncodingUTF16BE, "UTF-16BE");
            RSStringSwitchEncoding(RSStringEncodingUTF16LE, "UTF-16LE");
            
            RSStringSwitchEncoding(RSStringEncodingUTF32, "UTF-32");
            RSStringSwitchEncoding(RSStringEncodingUTF32BE, "UTF-32BE");
            RSStringSwitchEncoding(RSStringEncodingUTF32LE, "UTF-32LE");
        default:
            formatCStr = nil;
            break;
    }
    return formatCStr;
}
#undef RSStringSwitchEncoding

RSPrivate BOOL __RSStringEncodingAvailable(RSStringEncoding _encoding)
{
    return __RSStringTranslateEncoding(_encoding) ? YES : NO;
}

//RSExport RSStringRef RSStringCreateConvert(RSStringRef aString, RSStringEncoding toEncoding)
//{
//    if (aString == nil) return nil;
//    RSMutableStringRef copy = RSMutableCopy(RSGetAllocator(aString), aString);
//    RSMutableStringConvert(copy, toEncoding);
//    RSStringRef result = RSCopy(RSGetAllocator(copy), copy);
//    RSRelease(copy);
//    return result;
//}
//
//RSExport void RSMutableStringConvert(RSMutableStringRef aString, RSStringEncoding toEncoding)
//{
//    RSStringEncoding fromEncoding = RSStringGetEncoding(aString);
//    UniChar* characters = nil;
//    do
//    {
//        RSCBuffer fromEncodingName = __RSStringTranslateEncoding(fromEncoding);
//        RSCBuffer toEncodingName = __RSStringTranslateEncoding(toEncoding);
//        if (fromEncodingName == nil || toEncodingName == nil) break;
//        RSIndex length = RSStringGetLength(aString), capacity = RSAllocatorSize(RSAllocatorSystemDefault, length), endflag = 0;;
//        
//        if (capacity == length) endflag = 2;
//        
//        UErrorCode errorCode = U_ZERO_ERROR;
//        RSBit32 retCode = ucnv_convert(toEncodingName, fromEncodingName, nil, 0, (RSCBuffer)RSStringGetCStringPtr(aString), (RSBit32)length, &errorCode);
//        if (U_BUFFER_OVERFLOW_ERROR == errorCode)
//        {
//            errorCode = U_ZERO_ERROR;
//            
//            capacity = retCode; // make sure the capacity of the buffer is enough.
//            capacity += endflag;
//            characters = RSAllocatorAllocate(RSAllocatorSystemDefault, capacity);
//            
//            retCode = ucnv_convert(toEncodingName, fromEncodingName, (RSBuffer)characters, (RSBit32)capacity, (RSCBuffer)RSStringGetCStringPtr(aString), (RSBit32)length, &errorCode);
//            
//            if (U_ZERO_ERROR == errorCode)
//            {
//                RSStringDelete(aString, RSStringGetRange(aString));
//                RSStringSetEncoding(aString, toEncoding);
//                RSStringAppendCharacters(aString, characters, retCode);
//            }
//        }
//        if (characters) RSAllocatorDeallocate(RSAllocatorSystemDefault, characters);
//    } while (0);
//    return ;
//}

BOOL RSStringGetFileSystemRepresentation(RSStringRef string, char *buffer, RSIndex maxBufLen) {
    if (maxBufLen >= RSStringGetLength(string))
        RSStringGetCharacters(string, RSStringGetRange(string), (UniChar *)buffer);
    else
        RSStringGetCharacters(string, RSMakeRange(0, maxBufLen), (UniChar *)buffer);
    return YES;
}

BOOL _RSStringGetFileSystemRepresentation(RSStringRef string, uint8_t *buffer, RSIndex maxBufLen) {
    return RSStringGetFileSystemRepresentation(string, (char *)buffer, maxBufLen);
}

RSPrivate BOOL (*__RSCharToUniCharFunc)(UInt32 flags, uint8_t ch, UniChar *unicodeChar) = nil;
RSPrivate UniChar __RSCharToUniCharTable[256] = {
    0,   1,   2,   3,   4,   5,   6,   7,   8,   9,  10,  11,  12,  13,  14,  15,
    16,  17,  18,  19,  20,  21,  22,  23,  24,  25,  26,  27,  28,  29,  30,  31,
    32,  33,  34,  35,  36,  37,  38,  39,  40,  41,  42,  43,  44,  45,  46,  47,
    48,  49,  50,  51,  52,  53,  54,  55,  56,  57,  58,  59,  60,  61,  62,  63,
    64,  65,  66,  67,  68,  69,  70,  71,  72,  73,  74,  75,  76,  77,  78,  79,
    80,  81,  82,  83,  84,  85,  86,  87,  88,  89,  90,  91,  92,  93,  94,  95,
    96,  97,  98,  99, 100, 101, 102, 103, 104, 105, 106, 107, 108, 109, 110, 111,
    112, 113, 114, 115, 116, 117, 118, 119, 120, 121, 122, 123, 124, 125, 126, 127,
    128, 129, 130, 131, 132, 133, 134, 135, 136, 137, 138, 139, 140, 141, 142, 143,
    144, 145, 146, 147, 148, 149, 150, 151, 152, 153, 154, 155, 156, 157, 158, 159,
    160, 161, 162, 163, 164, 165, 166, 167, 168, 169, 170, 171, 172, 173, 174, 175,
    176, 177, 178, 179, 180, 181, 182, 183, 184, 185, 186, 187, 188, 189, 190, 191,
    192, 193, 194, 195, 196, 197, 198, 199, 200, 201, 202, 203, 204, 205, 206, 207,
    208, 209, 210, 211, 212, 213, 214, 215, 216, 217, 218, 219, 220, 221, 222, 223,
    224, 225, 226, 227, 228, 229, 230, 231, 232, 233, 234, 235, 236, 237, 238, 239,
    240, 241, 242, 243, 244, 245, 246, 247, 248, 249, 250, 251, 252, 253, 254, 255
};

RSPrivate void __RSSetCharToUniCharFunc(BOOL (*func)(UInt32 flags, UInt8 ch, UniChar *unicodeChar)) {
    if (__RSCharToUniCharFunc != func) {
        int ch;
        __RSCharToUniCharFunc = func;
        if (func) {
            for (ch = 128; ch < 256; ch++) {
                UniChar uch;
                __RSCharToUniCharTable[ch] = (__RSCharToUniCharFunc(0, ch, &uch) ? uch : 0xFFFD);
            }
        } else {	// If we have no __RSCharToUniCharFunc, assume 128..255 return the value as-is
            for (ch = 128; ch < 256; ch++) __RSCharToUniCharTable[ch] = ch;
        }
    }
}

RSPrivate void __RSStrConvertBytesToUnicode(const uint8_t *bytes, UniChar *buffer, RSIndex numChars) {
    RSIndex idx;
    for (idx = 0; idx < numChars; idx++) buffer[idx] = __RSCharToUniCharTable[bytes[idx]];
}

