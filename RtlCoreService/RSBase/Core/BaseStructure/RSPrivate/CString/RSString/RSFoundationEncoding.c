//
//  RSFoundationEncoding.c
//  RSCoreFoundation
//
//  Created by RetVal on 7/28/13.
//  Copyright (c) 2013 RetVal. All rights reserved.
//

#include "RSFoundationEncoding.h"
#include "RSUniChar.h"
#include "RSStringEncodingConverter.h"
#include "RSStringEncodingConverterExt.h"

#include <RSCoreFoundation/RSByteOrder.h>
/* ??? Is returning length when no other answer is available the right thing?
 !!! All of the (length > (LONG_MAX / N)) type checks are to avoid wrap-around and eventual malloc overflow in the client
 */
RSExport RSIndex RSStringGetMaximumSizeForEncoding(RSIndex length, RSStringEncoding encoding)
{
    if (encoding == RSStringEncodingUTF8) {
        return (length > (LONG_MAX / 3)) ? RSNotFound : (length * 3);
    } else if ((encoding == RSStringEncodingUTF32) || (encoding == RSStringEncodingUTF32BE) || (encoding == RSStringEncodingUTF32LE)) { // UTF-32
        return (length > (LONG_MAX / sizeof(UTF32Char))) ? RSNotFound : (length * sizeof(UTF32Char));
    } else {
        encoding &= 0xFFF; // Mask off non-base part
    }
    switch (encoding) {
        case RSStringEncodingUnicode:
            return (length > (LONG_MAX / sizeof(UniChar))) ? RSNotFound : (length * sizeof(UniChar));
            
        case RSStringEncodingNonLossyASCII:
            return (length > (LONG_MAX / 6)) ? RSNotFound : (length * 6);      // 1 Unichar can expand to 6 bytes
            
        case RSStringEncodingMacRoman:
        case RSStringEncodingWindowsLatin1:
        case RSStringEncodingISOLatin1:
        case RSStringEncodingNextStepLatin:
        case RSStringEncodingASCII:
            return length / sizeof(uint8_t);
            
        default:
            return length / sizeof(uint8_t);
    }
}


/* Returns whether the indicated encoding can be stored in 8-bit chars
 */
RSInline BOOL __RSStrEncodingCanBeStoredInEightBit(RSStringEncoding encoding) {
    switch (encoding & 0xFFF) { // just use encoding base
        case RSStringEncodingInvalidId:
        case RSStringEncodingUnicode:
        case RSStringEncodingNonLossyASCII:
            return NO;
            
        case RSStringEncodingMacRoman:
        case RSStringEncodingWindowsLatin1:
        case RSStringEncodingISOLatin1:
        case RSStringEncodingNextStepLatin:
        case RSStringEncodingASCII:
            return YES;
            
        default: return NO;
    }
}

/* System Encoding.
 */
static RSStringEncoding __RSDefaultSystemEncoding = RSStringEncodingInvalidId;
static RSStringEncoding __RSDefaultFileSystemEncoding = RSStringEncodingInvalidId;
RSStringEncoding __RSDefaultEightBitStringEncoding = RSStringEncodingInvalidId;


#if DEPLOYMENT_TARGET_MACOSX || DEPLOYMENT_TARGET_EMBEDDED || DEPLOYMENT_TARGET_EMBEDDED_MINI || DEPLOYMENT_TARGET_LINUX
#define __defaultEncoding RSStringEncodingMacRoman
#elif DEPLOYMENT_TARGET_WINDOWS
#define __defaultEncoding RSStringEncodingWindowsLatin1
#else
#warning This value must match __RSGetConverter condition in RSStringEncodingConverter.c
#define __defaultEncoding RSStringEncodingISOLatin1
#endif

typedef BOOL (*UNI_CHAR_FUNC)(UInt32 flags, UInt8 ch, UniChar *unicodeChar);

RSExport RSStringEncoding RSStringGetSystemEncoding(void) {
    if (__RSDefaultSystemEncoding == RSStringEncodingInvalidId) {
        __RSDefaultSystemEncoding = __defaultEncoding;
        const RSStringEncodingConverter *converter = RSStringEncodingGetConverter(__RSDefaultSystemEncoding);
        __RSSetCharToUniCharFunc(converter->encodingClass == RSStringEncodingConverterCheapEightBit ? (UNI_CHAR_FUNC)converter->toUnicode : NULL);
    }
    return __RSDefaultSystemEncoding;
}

RSExport RSStringEncoding RSStringFileSystemEncoding() {
    if (__RSDefaultFileSystemEncoding == RSStringEncodingInvalidId) {
#if DEPLOYMENT_TARGET_MACOSX || DEPLOYMENT_TARGET_EMBEDDED || DEPLOYMENT_TARGET_EMBEDDED_MINI || DEPLOYMENT_TARGET_WINDOWS
        __RSDefaultFileSystemEncoding = RSStringEncodingUTF8;
#else
        __RSDefaultFileSystemEncoding = RSStringGetSystemEncoding();
#endif
    }
    
    return __RSDefaultFileSystemEncoding;
}

/* Returns the encoding used in eight bit RSStrings (can't be any encoding which isn't 1-to-1 with Unicode)
 ??? Perhaps only ASCII fits the bill due to Unicode decomposition.
 */
RSStringEncoding __RSStringComputeEightBitStringEncoding(void) {
    if (__RSDefaultEightBitStringEncoding == RSStringEncodingInvalidId) {
        RSStringEncoding systemEncoding = RSStringGetSystemEncoding();
        if (systemEncoding == RSStringEncodingInvalidId) { // We're right in the middle of querying system encoding from default database. Delaying to set until system encoding is determined.
            return RSStringEncodingASCII;
        } else if (__RSStrEncodingCanBeStoredInEightBit(systemEncoding)) {
            __RSDefaultEightBitStringEncoding = systemEncoding;
        } else {
            __RSDefaultEightBitStringEncoding = RSStringEncodingASCII;
        }
    }
    
    return __RSDefaultEightBitStringEncoding;
}

/* The minimum length the output buffers should be in the above functions
 */
#define RSCharConversionBufferLength 512


#define MAX_LOCAL_CHARS		(sizeof(buffer->localBuffer) / sizeof(uint8_t))
#define MAX_LOCAL_UNICHARS	(sizeof(buffer->localBuffer) / sizeof(UniChar))

enum {
    __NSNonLossyErrorMode = -1,
    __NSNonLossyASCIIMode = 0,
    __NSNonLossyBackslashMode = 1,
    __NSNonLossyHexInitialMode = __NSNonLossyBackslashMode + 1,
    __NSNonLossyHexFinalMode = __NSNonLossyHexInitialMode + 4,
    __NSNonLossyOctalInitialMode = __NSNonLossyHexFinalMode + 1,
    __NSNonLossyOctalFinalMode = __NSNonLossyHexFinalMode + 3
};

static bool __RSWantsToUseASCIICompatibleConversion = NO;
RSInline UInt32 __RSGetASCIICompatibleFlag(void) { return __RSWantsToUseASCIICompatibleConversion; }

void _RSStringEncodingSetForceASCIICompatibility(BOOL flag)
{
    __RSWantsToUseASCIICompatibleConversion = (flag ? (UInt32)YES : (UInt32)NO);
}

BOOL __RSStringDecodeByteStream3(const uint8_t *bytes, RSIndex len, RSStringEncoding encoding, BOOL alwaysUnicode, RSVarWidthCharBuffer *buffer, BOOL *useClientsMemoryPtr, UInt32 converterFlags) {
    RSIndex idx;
    const uint8_t *chars = (const uint8_t *)bytes;
    const uint8_t *end = chars + len;
    BOOL result = YES;
    
    if (useClientsMemoryPtr) *useClientsMemoryPtr = NO;
    
    buffer->isASCII = !alwaysUnicode;
    buffer->shouldFreeChars = NO;
    buffer->numChars = 0;
    
    if (0 == len) return YES;
    
    buffer->allocator = (buffer->allocator ? buffer->allocator : RSAllocatorGetDefault());
    
    if ((encoding == RSStringEncodingUTF16) || (encoding == RSStringEncodingUTF16BE) || (encoding == RSStringEncodingUTF16LE))
    {
        // UTF-16
        const UTF16Char *src = (const UTF16Char *)bytes;
        const UTF16Char *limit = src + (len / sizeof(UTF16Char)); // <rdar://problem/7854378> avoiding odd len issue
        bool swap = NO;
        
        if (RSStringEncodingUTF16 == encoding)
        {
            UTF16Char bom = ((*src == 0xFFFE) || (*src == 0xFEFF) ? *(src++) : 0);
            
#if __RS_BIG_ENDIAN__
            if (bom == 0xFFFE) swap = YES;
#else
            if (bom != 0xFEFF) swap = YES;
#endif
            if (bom) useClientsMemoryPtr = NULL;
        } else {
#if __RS_BIG_ENDIAN__
            if (RSStringEncodingUTF16LE == encoding) swap = YES;
#else
            if (RSStringEncodingUTF16BE == encoding) swap = YES;
#endif
        }
        
        buffer->numChars = limit - src;
        
        if (useClientsMemoryPtr && !swap)
        {
            // If the caller is ready to deal with no-copy situation, and the situation is possible, indicate it...
            *useClientsMemoryPtr = YES;
            buffer->chars.unicode = (UniChar *)src;
            buffer->isASCII = NO;
        }
        else
        {
            if (buffer->isASCII)
            {
                // Let's see if we can reduce the Unicode down to ASCII...
                const UTF16Char *characters = src;
                UTF16Char mask = (swap ? 0x80FF : 0xFF80);
                
                while (characters < limit)
                {
                    if (*(characters++) & mask)
                    {
                        buffer->isASCII = NO;
                        break;
                    }
                }
            }
            
            if (buffer->isASCII)
            {
                uint8_t *dst;
                if (NULL == buffer->chars.ascii)
                {
                    // we never reallocate when buffer is supplied
                    if (buffer->numChars > MAX_LOCAL_CHARS)
                    {
                        buffer->chars.ascii = (UInt8 *)RSAllocatorAllocate(buffer->allocator, (buffer->numChars * sizeof(uint8_t)));
                        if (!buffer->chars.ascii) goto memoryErrorExit;
                        buffer->shouldFreeChars = YES;
                    }
                    else
                    {
                        buffer->chars.ascii = (uint8_t *)buffer->localBuffer;
                    }
                }
                dst = buffer->chars.ascii;
                
                if (swap) {
                    while (src < limit) *(dst++) = (*(src++) >> 8);
                } else {
                    while (src < limit) *(dst++) = (uint8_t)*(src++);
                }
            }
            else
            {
                UTF16Char *dst;
                
                if (NULL == buffer->chars.unicode)
                {
                    // we never reallocate when buffer is supplied
                    if (buffer->numChars > MAX_LOCAL_UNICHARS)
                    {
                        buffer->chars.unicode = (UniChar *)RSAllocatorAllocate(buffer->allocator, (buffer->numChars * sizeof(UTF16Char)));
                        if (!buffer->chars.unicode) goto memoryErrorExit;
                        buffer->shouldFreeChars = YES;
                    }
                    else
                    {
                        buffer->chars.unicode = (UTF16Char *)buffer->localBuffer;
                    }
                }
                dst = buffer->chars.unicode;
                
                if (swap)
                {
                    while (src < limit) *(dst++) = RSSwapInt16(*(src++));
                }
                else
                {
                    memmove(dst, src, buffer->numChars * sizeof(UTF16Char));
                }
            }
        }
    } else if ((encoding == RSStringEncodingUTF32) || (encoding == RSStringEncodingUTF32BE) || (encoding == RSStringEncodingUTF32LE)) {
        const UTF32Char *src = (const UTF32Char *)bytes;
        const UTF32Char *limit =  src + (len / sizeof(UTF32Char)); // <rdar://problem/7854378> avoiding odd len issue
        bool swap = NO;
        static bool strictUTF32 = (bool)-1;
        
        if ((bool)-1 == strictUTF32) strictUTF32 = (1 != 0);
        
        if (RSStringEncodingUTF32 == encoding) {
            UTF32Char bom = ((*src == 0xFFFE0000) || (*src == 0x0000FEFF) ? *(src++) : 0);
            
#if __RS_BIG_ENDIAN__
            if (bom == 0xFFFE0000) swap = YES;
#else
            if (bom != 0x0000FEFF) swap = YES;
#endif
        } else {
#if __RS_BIG_ENDIAN__
            if (RSStringEncodingUTF32LE == encoding) swap = YES;
#else
            if (RSStringEncodingUTF32BE == encoding) swap = YES;
#endif
        }
        
        buffer->numChars = limit - src;
        
        {
            // Let's see if we have non-ASCII or non-BMP
            const UTF32Char *characters = src;
            UTF32Char asciiMask = (swap ? 0x80FFFFFF : 0xFFFFFF80);
            UTF32Char bmpMask = (swap ? 0x0000FFFF : 0xFFFF0000);
            
            while (characters < limit) {
                if (*characters & asciiMask) {
                    buffer->isASCII = NO;
                    if (*characters & bmpMask) {
                        if (strictUTF32 && ((swap ? (UTF32Char)RSSwapInt32(*characters) : *characters) > 0x10FFFF)) return NO; // outside of Unicode Scaler Value. Haven't allocated buffer, yet.
                        ++(buffer->numChars);
                    }
                }
                ++characters;
            }
        }
        
        if (buffer->isASCII)
        {
            uint8_t *dst;
            if (NULL == buffer->chars.ascii)
            {
                // we never reallocate when buffer is supplied
                if (buffer->numChars > MAX_LOCAL_CHARS)
                {
                    buffer->chars.ascii = (UInt8 *)RSAllocatorAllocate(buffer->allocator, (buffer->numChars * sizeof(uint8_t)));
                    if (!buffer->chars.ascii) goto memoryErrorExit;
                    buffer->shouldFreeChars = YES;
                }
                else
                {
                    buffer->chars.ascii = (uint8_t *)buffer->localBuffer;
                }
            }
            dst = buffer->chars.ascii;
            
            if (swap)
            {
                while (src < limit) *(dst++) = (*(src++) >> 24);
            }
            else
            {
                while (src < limit) *(dst++) = *(src++);
            }
        }
        else
        {
            if (NULL == buffer->chars.unicode)
            {
                // we never reallocate when buffer is supplied
                if (buffer->numChars > MAX_LOCAL_UNICHARS)
                {
                    buffer->chars.unicode = (UniChar *)RSAllocatorAllocate(buffer->allocator, (buffer->numChars * sizeof(UTF16Char)));
                    if (!buffer->chars.unicode) goto memoryErrorExit;
                    buffer->shouldFreeChars = YES;
                }
                else
                {
                    buffer->chars.unicode = (UTF16Char *)buffer->localBuffer;
                }
            }
            result = (RSUniCharFromUTF32(src, limit - src, buffer->chars.unicode, (strictUTF32 ? NO : YES), __RS_BIG_ENDIAN__ ? !swap : swap) ? YES : NO);
        }
    }
    else if (RSStringEncodingUTF8 == encoding)
    {
        if ((len >= 3) && (chars[0] == 0xef) && (chars[1] == 0xbb) && (chars[2] == 0xbf)) {	// If UTF8 BOM, skip
            chars += 3;
            len -= 3;
            if (0 == len) return YES;
        }
        if (buffer->isASCII) {
            for (idx = 0; idx < len; idx++) {
                if (128 <= chars[idx]) {
                    buffer->isASCII = NO;
                    break;
                }
            }
        }
        if (buffer->isASCII)
        {
            buffer->numChars = len;
            buffer->shouldFreeChars = !buffer->chars.ascii && (len <= MAX_LOCAL_CHARS) ? NO : YES;
            buffer->chars.ascii = (buffer->chars.ascii ? buffer->chars.ascii : (len <= MAX_LOCAL_CHARS) ? (uint8_t *)buffer->localBuffer : (UInt8 *)RSAllocatorAllocate(buffer->allocator, len * sizeof(uint8_t)));
            if (!buffer->chars.ascii) goto memoryErrorExit;
            memmove(buffer->chars.ascii, chars, len * sizeof(uint8_t));
        }
        else
        {
            RSIndex numDone;
            static RSStringEncodingToUnicodeProc __RSFromUTF8 = NULL;
            
            if (!__RSFromUTF8)
            {
                const RSStringEncodingConverter *converter = RSStringEncodingGetConverter(RSStringEncodingUTF8);
                __RSFromUTF8 = (RSStringEncodingToUnicodeProc)converter->toUnicode;
            }
            
            buffer->shouldFreeChars = !buffer->chars.unicode && (len <= MAX_LOCAL_UNICHARS) ? NO : YES;
            buffer->chars.unicode = (buffer->chars.unicode ? buffer->chars.unicode : (len <= MAX_LOCAL_UNICHARS) ? (UniChar *)buffer->localBuffer : (UniChar *)RSAllocatorAllocate(buffer->allocator, len * sizeof(UniChar)));
            if (!buffer->chars.unicode) goto memoryErrorExit;
            buffer->numChars = 0;
            while (chars < end)
            {
                numDone = 0;
                chars += __RSFromUTF8(converterFlags, chars, end - chars, &(buffer->chars.unicode[buffer->numChars]), len - buffer->numChars, &numDone);
                
                if (0 == numDone)
                {
                    result = NO;
                    break;
                }
                buffer->numChars += numDone;
            }
        }
    }
    else if (RSStringEncodingNonLossyASCII == encoding)
    {
        UTF16Char currentValue = 0;
        uint8_t character;
        int8_t mode = __NSNonLossyASCIIMode;
        
        buffer->isASCII = NO;
        buffer->shouldFreeChars = !buffer->chars.unicode && (len <= MAX_LOCAL_UNICHARS) ? NO : YES;
        buffer->chars.unicode = (buffer->chars.unicode ? buffer->chars.unicode : (len <= MAX_LOCAL_UNICHARS) ? (UniChar *)buffer->localBuffer : (UniChar *)RSAllocatorAllocate(buffer->allocator, len * sizeof(UniChar)));
        if (!buffer->chars.unicode) goto memoryErrorExit;
        buffer->numChars = 0;
        
        while (chars < end)
        {
            character = (*chars++);
            
            switch (mode)
            {
                case __NSNonLossyASCIIMode:
                    if (character == '\\')
                    {
                        mode = __NSNonLossyBackslashMode;
                    }
                    else if (character < 0x80)
                    {
                        currentValue = character;
                    }
                    else
                    {
                        mode = __NSNonLossyErrorMode;
                    }
                    break;
                    
                case __NSNonLossyBackslashMode:
                    if ((character == 'U') || (character == 'u'))
                    {
                        mode = __NSNonLossyHexInitialMode;
                        currentValue = 0;
                    }
                    else if ((character >= '0') && (character <= '9'))
                    {
                        mode = __NSNonLossyOctalInitialMode;
                        currentValue = character - '0';
                    }
                    else if (character == '\\')
                    {
                        mode = __NSNonLossyASCIIMode;
                        currentValue = character;
                    }
                    else
                    {
                        mode = __NSNonLossyErrorMode;
                    }
                    break;
                    
                default:
                    if (mode < __NSNonLossyHexFinalMode)
                    {
                        if ((character >= '0') && (character <= '9'))
                        {
                            currentValue = (currentValue << 4) | (character - '0');
                            if (++mode == __NSNonLossyHexFinalMode) mode = __NSNonLossyASCIIMode;
                        }
                        else
                        {
                            if (character >= 'a') character -= ('a' - 'A');
                            if ((character >= 'A') && (character <= 'F'))
                            {
                                currentValue = (currentValue << 4) | ((character - 'A') + 10);
                                if (++mode == __NSNonLossyHexFinalMode) mode = __NSNonLossyASCIIMode;
                            }
                            else
                            {
                                mode = __NSNonLossyErrorMode;
                            }
                        }
                    }
                    else
                    {
                        if ((character >= '0') && (character <= '9'))
                        {
                            currentValue = (currentValue << 3) | (character - '0');
                            if (++mode == __NSNonLossyOctalFinalMode) mode = __NSNonLossyASCIIMode;
                        }
                        else
                        {
                            mode = __NSNonLossyErrorMode;
                        }
                    }
                    break;
            }
            
            if (mode == __NSNonLossyASCIIMode)
            {
                buffer->chars.unicode[buffer->numChars++] = currentValue;
            }
            else if (mode == __NSNonLossyErrorMode)
            {
                break;
            }
        }
        result = ((mode == __NSNonLossyASCIIMode) ? YES : NO);
    }
    else
    {
        const RSStringEncodingConverter *converter = RSStringEncodingGetConverter(encoding);
        
        if (!converter) return NO;
        
        BOOL isASCIISuperset = __RSStringEncodingIsSupersetOfASCII(encoding);
        
        if (!isASCIISuperset) buffer->isASCII = NO;
        
        if (buffer->isASCII)
        {
            for (idx = 0; idx < len; idx++)
            {
                if (128 <= chars[idx])
                {
                    buffer->isASCII = NO;
                    break;
                }
            }
        }
        
        if (converter->encodingClass == RSStringEncodingConverterCheapEightBit)
        {
            if (buffer->isASCII) {
                buffer->numChars = len;
                buffer->shouldFreeChars = !buffer->chars.ascii && (len <= MAX_LOCAL_CHARS) ? NO : YES;
                buffer->chars.ascii = (buffer->chars.ascii ? buffer->chars.ascii : (len <= MAX_LOCAL_CHARS) ? (uint8_t *)buffer->localBuffer : (UInt8 *)RSAllocatorAllocate(buffer->allocator, len * sizeof(uint8_t)));
                if (!buffer->chars.ascii) goto memoryErrorExit;
                memmove(buffer->chars.ascii, chars, len * sizeof(uint8_t));
            }
            else
            {
                buffer->shouldFreeChars = !buffer->chars.unicode && (len <= MAX_LOCAL_UNICHARS) ? NO : YES;
                buffer->chars.unicode = (buffer->chars.unicode ? buffer->chars.unicode : (len <= MAX_LOCAL_UNICHARS) ? (UniChar *)buffer->localBuffer : (UniChar *)RSAllocatorAllocate(buffer->allocator, len * sizeof(UniChar)));
                if (!buffer->chars.unicode) goto memoryErrorExit;
                buffer->numChars = len;
                if (RSStringEncodingASCII == encoding || RSStringEncodingISOLatin1 == encoding)
                {
                    for (idx = 0; idx < len; idx++) buffer->chars.unicode[idx] = (UniChar)chars[idx];
                }
                else
                {
                    for (idx = 0; idx < len; idx++)
                    {
                        if (chars[idx] < 0x80 && isASCIISuperset)
                        {
                            buffer->chars.unicode[idx] = (UniChar)chars[idx];
                        }
                        else if (!((RSStringEncodingCheapEightBitToUnicodeProc)converter->toUnicode)(0, chars[idx], buffer->chars.unicode + idx))
                        {
                            result = NO;
                            break;
                        }
                    }
                }
            }
        }
        else
        {
            if (buffer->isASCII)
            {
                buffer->numChars = len;
                buffer->shouldFreeChars = !buffer->chars.ascii && (len <= MAX_LOCAL_CHARS) ? NO : YES;
                buffer->chars.ascii = (buffer->chars.ascii ? buffer->chars.ascii : (len <= MAX_LOCAL_CHARS) ? (uint8_t *)buffer->localBuffer : (UInt8 *)RSAllocatorAllocate(buffer->allocator, len * sizeof(uint8_t)));
                if (!buffer->chars.ascii) goto memoryErrorExit;
                memmove(buffer->chars.ascii, chars, len * sizeof(uint8_t));
            }
            else
            {
                RSIndex guessedLength = RSStringEncodingCharLengthForBytes(encoding, 0, bytes, len);
                static UInt32 lossyFlag = (UInt32)-1;
                
                buffer->shouldFreeChars = !buffer->chars.unicode && (guessedLength <= MAX_LOCAL_UNICHARS) ? NO : YES;
                buffer->chars.unicode = (buffer->chars.unicode ? buffer->chars.unicode : (guessedLength <= MAX_LOCAL_UNICHARS) ? (UniChar *)buffer->localBuffer : (UniChar *)RSAllocatorAllocate(buffer->allocator, guessedLength * sizeof(UniChar)));
                if (!buffer->chars.unicode) goto memoryErrorExit;
                
                if (lossyFlag == (UInt32)-1) lossyFlag = 0;
                
                if (RSStringEncodingBytesToUnicode(encoding, lossyFlag|__RSGetASCIICompatibleFlag(), bytes, len, NULL, buffer->chars.unicode, (guessedLength > MAX_LOCAL_UNICHARS ? guessedLength : MAX_LOCAL_UNICHARS), &(buffer->numChars))) result = NO;
            }
        }
    }
    
    if (NO == result)
    {
    memoryErrorExit:	// Added for <rdar://problem/6581621>, but it's not clear whether an exception would be a better option
        result = NO;	// In case we come here from a goto
        if (buffer->shouldFreeChars && buffer->chars.unicode) RSAllocatorDeallocate(buffer->allocator, buffer->chars.unicode);
        buffer->isASCII = !alwaysUnicode;
        buffer->shouldFreeChars = NO;
        buffer->chars.ascii = NULL;
        buffer->numChars = 0;
    }
    return result;
}

RSExport RSIndex __RSStringEncodeByteStream(RSStringRef string, RSIndex rangeLoc, RSIndex rangeLen, BOOL generatingExternalFile, RSStringEncoding encoding, char lossByte, uint8_t *buffer, RSIndex max, RSIndex *usedBufLen)
{
    RSIndex totalBytesWritten = 0;	/* Number of written bytes */
    RSIndex numCharsProcessed = 0;	/* Number of processed chars */
    const UniChar *unichars;
    
    if (encoding == RSStringEncodingUTF8 && (unichars = RSStringGetCharactersPtr(string)))
    {
        static RSStringEncodingToBytesProc __RSToUTF8 = NULL;
        
        if (!__RSToUTF8)
        {
            const RSStringEncodingConverter *utf8Converter = RSStringEncodingGetConverter(RSStringEncodingUTF8);
            __RSToUTF8 = (RSStringEncodingToBytesProc)utf8Converter->toBytes;
        }
        numCharsProcessed = __RSToUTF8((generatingExternalFile ? RSStringEncodingPrependBOM : 0), unichars + rangeLoc, rangeLen, buffer, (buffer ? max : 0), &totalBytesWritten);
        
    }
    else if (encoding == RSStringEncodingNonLossyASCII)
    {
        const char *hex = "0123456789abcdef";
        UniChar ch;
        RSStringInlineBuffer buf;
        RSStringInitInlineBuffer(string, &buf, RSMakeRange(rangeLoc, rangeLen));
        while (numCharsProcessed < rangeLen)
        {
            RSIndex reqLength; /* Required number of chars to encode this UniChar */
            RSIndex cnt;
            char tmp[6];
            ch = RSStringGetCharacterFromInlineBuffer(&buf, numCharsProcessed);
            if ((ch >= ' ' && ch <= '~' && ch != '\\') || (ch == '\n' || ch == '\r' || ch == '\t')) {
                reqLength = 1;
                tmp[0] = (char)ch;
            }
            else
            {
                if (ch == '\\')
                {
                    tmp[1] = '\\';
                    reqLength = 2;
                }
                else if (ch < 256)
                {
                    /* \nnn; note that this is not NEXTSTEP encoding but a (small) UniChar */
                    tmp[1] = '0' + (ch >> 6);
                    tmp[2] = '0' + ((ch >> 3) & 7);
                    tmp[3] = '0' + (ch & 7);
                    reqLength = 4;
                } else
                {
                    /* \Unnnn */
                    tmp[1] = 'u'; // Changed to small+u in order to be aligned with Java
                    tmp[2] = hex[(ch >> 12) & 0x0f];
                    tmp[3] = hex[(ch >> 8) & 0x0f];
                    tmp[4] = hex[(ch >> 4) & 0x0f];
                    tmp[5] = hex[ch & 0x0f];
                    reqLength = 6;
                }
                tmp[0] = '\\';
            }
            if (buffer)
            {
                if (totalBytesWritten + reqLength > max) break; /* Doesn't fit..
                                                                 .*/
                for (cnt = 0; cnt < reqLength; cnt++)
                {
                    buffer[totalBytesWritten + cnt] = tmp[cnt];
                }
            }
            totalBytesWritten += reqLength;
            numCharsProcessed++;
        }
    }
    else if ((encoding == RSStringEncodingUTF16) || (encoding == RSStringEncodingUTF16BE) || (encoding == RSStringEncodingUTF16LE))
    {
        RSIndex extraForBOM = (generatingExternalFile && (encoding == RSStringEncodingUTF16) ? sizeof(UniChar) : 0);
        numCharsProcessed = rangeLen;
        if (buffer && (numCharsProcessed * (RSIndex)sizeof(UniChar) + extraForBOM > max))
        {
            numCharsProcessed = (max > extraForBOM) ? ((max - extraForBOM) / sizeof(UniChar)) : 0;
        }
        totalBytesWritten = (numCharsProcessed * sizeof(UniChar)) + extraForBOM;
        if (buffer) {
            if (extraForBOM) {	/* Generate BOM */
#if __RS_BIG_ENDIAN__
                *buffer++ = 0xfe; *buffer++ = 0xff;
#else
                *buffer++ = 0xff; *buffer++ = 0xfe;
#endif
            }
            RSStringGetCharacters(string, RSMakeRange(rangeLoc, numCharsProcessed), (UniChar *)buffer);
            if ((__RS_BIG_ENDIAN__ ?  RSStringEncodingUTF16LE : RSStringEncodingUTF16BE) == encoding)
            {
                // Need to swap
                UTF16Char *characters = (UTF16Char *)buffer;
                const UTF16Char *limit = characters + numCharsProcessed;
                
                while (characters < limit)
                {
                    *characters = RSSwapInt16(*characters);
                    ++characters;
                }
            }
        }
    }
    else if ((encoding == RSStringEncodingUTF32) || (encoding == RSStringEncodingUTF32BE) || (encoding == RSStringEncodingUTF32LE))
    {
        UTF32Char character;
        RSStringInlineBuffer buf;
        UTF32Char *characters = (UTF32Char *)buffer;
        
        bool swap = (encoding == (__RS_BIG_ENDIAN__ ? RSStringEncodingUTF32LE : RSStringEncodingUTF32BE) ? YES : NO);
        if (generatingExternalFile && (encoding == RSStringEncodingUTF32))
        {
            totalBytesWritten += sizeof(UTF32Char);
            if (characters)
            {
                if (totalBytesWritten > max)
                {
                    // insufficient buffer
                    totalBytesWritten = 0;
                }
                else
                {
                    *(characters++) = 0x0000FEFF;
                }
            }
        }
        
        RSStringInitInlineBuffer(string, &buf, RSMakeRange(rangeLoc, rangeLen));
        while (numCharsProcessed < rangeLen)
        {
            character = RSStringGetCharacterFromInlineBuffer(&buf, numCharsProcessed);
            
            if (RSUniCharIsSurrogateHighCharacter(character))
            {
                UTF16Char otherCharacter;
                
                if (((numCharsProcessed + 1) < rangeLen) && RSUniCharIsSurrogateLowCharacter((otherCharacter = RSStringGetCharacterFromInlineBuffer(&buf, numCharsProcessed + 1))))
                {
                    character = RSUniCharGetLongCharacterForSurrogatePair(character, otherCharacter);
                }
                else if (lossByte)
                {
                    character = lossByte;
                }
                else
                {
                    break;
                }
            }
            else if (RSUniCharIsSurrogateLowCharacter(character))
            {
                if (lossByte)
                {
                    character = lossByte;
                }
                else
                {
                    break;
                }
            }
            
            totalBytesWritten += sizeof(UTF32Char);
            
            if (characters)
            {
                if (totalBytesWritten > max)
                {
                    totalBytesWritten -= sizeof(UTF32Char);
                    break;
                }
                *(characters++) = (swap ? RSSwapInt32(character) : character);
            }
            
            numCharsProcessed += (character > 0xFFFF ? 2 : 1);
        }
    }
    else
    {
        RSIndex numChars;
        UInt32 flags;
        const unsigned char *cString = NULL;
        BOOL isASCIISuperset = __RSStringEncodingIsSupersetOfASCII(encoding);
        
        if (!RSStringEncodingIsValidEncoding(encoding)) return 0;
        
        if (!RS_IS_OBJC(RSStringGetTypeID(), string) && isASCIISuperset)
        {
            // Checking for NSString to avoid infinite recursion
            const unsigned char *ptr;
            if ((cString = (const unsigned char *)RSStringGetCStringPtr(string, __RSStringGetEightBitStringEncoding())))
            {
                ptr = (cString += rangeLoc);
                if (__RSStringGetEightBitStringEncoding() == encoding)
                {
                    numCharsProcessed = (rangeLen < max || buffer == NULL ? rangeLen : max);
                    if (buffer) memmove(buffer, cString, numCharsProcessed);
                    if (usedBufLen) *usedBufLen = numCharsProcessed;
                    return numCharsProcessed;
                }
                
                RSIndex uninterestingTailLen = buffer ? (rangeLen - min(max, rangeLen)) : 0;
                while (*ptr < 0x80 && rangeLen > uninterestingTailLen)
                {
                    ++ptr;
                    --rangeLen;
                }
                numCharsProcessed = ptr - cString;
                if (buffer)
                {
                    numCharsProcessed = (numCharsProcessed < max ? numCharsProcessed : max);
                    memmove(buffer, cString, numCharsProcessed);
                    buffer += numCharsProcessed;
                    max -= numCharsProcessed;
                }
                if (!rangeLen || (buffer && (max == 0)))
                {
                    if (usedBufLen) *usedBufLen = numCharsProcessed;
                    return numCharsProcessed;
                }
                rangeLoc += numCharsProcessed;
                totalBytesWritten += numCharsProcessed;
            }
            extern const unsigned char *RSStringGetPascalStringPtr(RSStringRef str, RSStringEncoding encoding);
            if (!cString && (cString = RSStringGetPascalStringPtr(string, __RSStringGetEightBitStringEncoding())))
            {
                ptr = (cString += (rangeLoc + 1));
                if (__RSStringGetEightBitStringEncoding() == encoding)
                {
                    numCharsProcessed = (rangeLen < max || buffer == NULL ? rangeLen : max);
                    if (buffer) memmove(buffer, cString, numCharsProcessed);
                    if (usedBufLen) *usedBufLen = numCharsProcessed;
                    return numCharsProcessed;
                }
                while (*ptr < 0x80 && rangeLen > 0)
                {
                    ++ptr;
                    --rangeLen;
                }
                numCharsProcessed = ptr - cString;
                if (buffer)
                {
                    numCharsProcessed = (numCharsProcessed < max ? numCharsProcessed : max);
                    memmove(buffer, cString, numCharsProcessed);
                    buffer += numCharsProcessed;
                    max -= numCharsProcessed;
                }
                if (!rangeLen || (buffer && (max == 0)))
                {
                    if (usedBufLen) *usedBufLen = numCharsProcessed;
                    return numCharsProcessed;
                }
                rangeLoc += numCharsProcessed;
                totalBytesWritten += numCharsProcessed;
            }
        }
        
        if (!buffer) max = 0;
        
        // Special case for Foundation. When lossByte == 0xFF && encoding RSStringEncodingASCII, we do the default ASCII fallback conversion
        // Aki 11/24/04 __RSGetASCIICompatibleFlag() is called only for non-ASCII superset encodings. Otherwise, it could lead to a deadlock (see 3890536).
        flags = (lossByte ? ((unsigned char)lossByte == 0xFF && encoding == RSStringEncodingASCII ? RSStringEncodingAllowLossyConversion : RSStringEncodingLossyByteToMask(lossByte)) : 0) | (generatingExternalFile ? RSStringEncodingPrependBOM : 0) | (isASCIISuperset ? 0 : __RSGetASCIICompatibleFlag());
        
        if (!cString && (cString = (const unsigned char *)RSStringGetCharactersPtr(string)))
        {
            // Must be Unicode string
            RSStringEncodingUnicodeToBytes(encoding, flags, (const UniChar *)cString + rangeLoc, rangeLen, &numCharsProcessed, buffer, max, &totalBytesWritten);
        }
        else
        {
            UniChar charBuf[RSCharConversionBufferLength];
            RSIndex currentLength;
            RSIndex usedLen;
            RSIndex lastUsedLen = 0, lastNumChars = 0;
            uint32_t result;
            uint32_t streamingMask;
            uint32_t streamID = 0;
#define MAX_DECOMP_LEN (6)
            
            while (rangeLen > 0)
            {
                currentLength = (rangeLen > RSCharConversionBufferLength ? RSCharConversionBufferLength : rangeLen);
                RSStringGetCharacters(string, RSMakeRange(rangeLoc, currentLength), charBuf);
                
                // could be in the middle of surrogate pair; back up.
                if ((rangeLen > RSCharConversionBufferLength) && RSUniCharIsSurrogateHighCharacter(charBuf[RSCharConversionBufferLength - 1])) --currentLength;
                
                streamingMask = ((rangeLen > currentLength) ? RSStringEncodingPartialInput : 0)|RSStringEncodingStreamIDToMask(streamID);
                
                result = RSStringEncodingUnicodeToBytes(encoding, flags|streamingMask, charBuf, currentLength, &numChars, buffer, max, &usedLen);
                streamID = RSStringEncodingStreamIDFromMask(result);
                result &= ~RSStringEncodingStreamIDMask;
                
                if (result != RSStringEncodingConversionSuccess)
                {
                    if (RSStringEncodingInvalidInputStream == result)
                    {
                        RSRange composedRange;
                        extern RSRange RSStringGetRangeOfComposedCharactersAtIndex(RSStringRef theString, RSIndex theIndex);
                        // Check the tail
                        if ((rangeLen > RSCharConversionBufferLength) && ((currentLength - numChars) < MAX_DECOMP_LEN))
                        {
                            composedRange = RSStringGetRangeOfComposedCharactersAtIndex(string, rangeLoc + currentLength);
                            
                            if ((composedRange.length <= MAX_DECOMP_LEN) && (composedRange.location < (rangeLoc + numChars))) {
                                result = RSStringEncodingUnicodeToBytes(encoding, flags|streamingMask, charBuf, composedRange.location - rangeLoc, &numChars, buffer, max, &usedLen);
                                streamID = RSStringEncodingStreamIDFromMask(result);
                                result &= ~RSStringEncodingStreamIDMask;
                            }
                        }
                        
                        // Check the head
                        if ((RSStringEncodingConversionSuccess != result) && (lastNumChars > 0) && (numChars < MAX_DECOMP_LEN)) {
                            composedRange = RSStringGetRangeOfComposedCharactersAtIndex(string, rangeLoc);
                            
                            if ((composedRange.length <= MAX_DECOMP_LEN) && (composedRange.location < rangeLoc)) {
                                // Try if the composed range can be converted
                                RSStringGetCharacters(string, composedRange, charBuf);
                                
                                if (RSStringEncodingUnicodeToBytes(encoding, flags, charBuf, composedRange.length, &numChars, NULL, 0, &usedLen) == RSStringEncodingConversionSuccess) { // OK let's try the last run
                                    RSIndex lastRangeLoc = rangeLoc - lastNumChars;
                                    
                                    currentLength = composedRange.location - lastRangeLoc;
                                    RSStringGetCharacters(string, RSMakeRange(lastRangeLoc, currentLength), charBuf);
                                    
                                    result = RSStringEncodingUnicodeToBytes(encoding, flags|streamingMask, charBuf, currentLength, &numChars, (max ? buffer - lastUsedLen : NULL), (max ? max + lastUsedLen : 0), &usedLen);
                                    streamID = RSStringEncodingStreamIDFromMask(result);
                                    result &= ~RSStringEncodingStreamIDMask;
                                    
                                    if (result == RSStringEncodingConversionSuccess) { // OK let's try the last run
                                        // Looks good. back up
                                        totalBytesWritten -= lastUsedLen;
                                        numCharsProcessed -= lastNumChars;
                                        
                                        rangeLoc = lastRangeLoc;
                                        rangeLen += lastNumChars;
                                        
                                        if (max) {
                                            buffer -= lastUsedLen;
                                            max += lastUsedLen;
                                        }
                                    }
                                }
                            }
                        }
                    }
                    
                    if (RSStringEncodingConversionSuccess != result) { // really failed
                        totalBytesWritten += usedLen;
                        numCharsProcessed += numChars;
                        break;
                    }
                }
                
                totalBytesWritten += usedLen;
                numCharsProcessed += numChars;
                
                rangeLoc += numChars;
                rangeLen -= numChars;
                if (max) {
                    buffer += usedLen;
                    max -= usedLen;
                    if (max <= 0) break;
                }
                lastUsedLen = usedLen; lastNumChars = numChars;
                flags &= ~RSStringEncodingPrependBOM;
            }
        }
    }
    if (usedBufLen) *usedBufLen = totalBytesWritten;
    return numCharsProcessed;
}
