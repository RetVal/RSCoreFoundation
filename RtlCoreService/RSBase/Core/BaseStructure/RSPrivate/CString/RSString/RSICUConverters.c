//
//  RSICUConverters.c
//  RSCoreFoundation
//
//  Created by RetVal on 7/29/13.
//  Copyright (c) 2013 RetVal. All rights reserved.
//

#include <stdio.h>
#include "RSStringEncodingDatabase.h"
#include "RSStringEncodingConverterPrivate.h"
#include "RSICUConverters.h"
#include <RSCoreFoundation/RSStringEncoding.h>
//#include "RSStringEncodingExt.h"
#include "RSUniChar.h"
#include "unicode/ucnv.h"
#include "unicode/uversion.h"
#include "RSInternal.h"

// Thread data support
typedef struct {
    uint8_t _numSlots;
    uint8_t _nextSlot;
    UConverter **_converters;
} __RSICUThreadData;

static void __RSICUThreadDataDestructor(void *context) {
    __RSICUThreadData * data = (__RSICUThreadData *)context;
    
    if (NULL != data->_converters) { // scan to make sure deallocation
        UConverter **converter = data->_converters;
        UConverter **limit = converter + data->_numSlots;
        
        while (converter < limit) {
            if (NULL != converter) ucnv_close(*converter);
            ++converter;
        }
        RSAllocatorDeallocate(NULL, data->_converters);
    }
    
    RSAllocatorDeallocate(NULL, data);
}

RSInline __RSICUThreadData *__RSStringEncodingICUGetThreadData() {
    __RSICUThreadData * data;
    
    data = (__RSICUThreadData *)_RSGetTSD(__RSTSDKeyICUConverter);
    
    if (NULL == data) {
        data = (__RSICUThreadData *)RSAllocatorAllocate(NULL, sizeof(__RSICUThreadData));
        memset(data, 0, sizeof(__RSICUThreadData));
        _RSSetTSD(__RSTSDKeyICUConverter, (void *)data, __RSICUThreadDataDestructor);
    }
    
    return data;
}

RSPrivate const char *__RSStringEncodingGetICUName(RSStringEncoding encoding) {
#define STACK_BUFFER_SIZE (60)
    char buffer[STACK_BUFFER_SIZE];
    const char *result = NULL;
    UErrorCode errorCode = U_ZERO_ERROR;
    uint32_t codepage = 0;
    
    if (RSStringEncodingUTF7_IMAP == encoding) return "IMAP-mailbox-name";
    
    if (RSStringEncodingUnicode != (encoding & 0x0F00)) codepage = __RSStringEncodingGetWindowsCodePage(encoding); // we don't use codepage for UTF to avoid little endian weirdness of Windows
    
    if ((0 != codepage) && (snprintf(buffer, STACK_BUFFER_SIZE, "windows-%d", codepage) < STACK_BUFFER_SIZE) && (NULL != (result = ucnv_getAlias(buffer, 0, &errorCode)))) return result;
    
    if (__RSStringEncodingGetCanonicalName(encoding, buffer, STACK_BUFFER_SIZE)) result = ucnv_getAlias(buffer, 0, &errorCode);
    
    return result;
#undef STACK_BUFFER_SIZE
}

RSPrivate RSStringEncoding __RSStringEncodingGetFromICUName(const char *icuName) {
    uint32_t codepage;
    UErrorCode errorCode = U_ZERO_ERROR;
    
    if ((0 == strncasecmp_l(icuName, "windows-", strlen("windows-"), NULL)) && (0 != (codepage = (uint32_t)strtol(icuName + strlen("windows-"), NULL, 10)))) return __RSStringEncodingGetFromWindowsCodePage(codepage);
    
    if (0 != ucnv_countAliases(icuName, &errorCode)) {
        RSStringEncoding encoding;
        const char *name;
        
        // Try WINDOWS platform
        name = ucnv_getStandardName(icuName, "WINDOWS", &errorCode);
        
        if (NULL != name) {
            if ((0 == strncasecmp_l(name, "windows-", strlen("windows-"), NULL)) && (0 != (codepage = (uint32_t)strtol(name + strlen("windows-"), NULL, 10)))) return __RSStringEncodingGetFromWindowsCodePage(codepage);
            
            if (strncasecmp_l(icuName, name, strlen(name), NULL) && (RSStringEncodingInvalidId != (encoding = __RSStringEncodingGetFromCanonicalName(name)))) return encoding;
        }
        
        // Try JAVA platform
        name = ucnv_getStandardName(icuName, "JAVA", &errorCode);
        if ((NULL != name) && strncasecmp_l(icuName, name, strlen(name), NULL) && (RSStringEncodingInvalidId != (encoding = __RSStringEncodingGetFromCanonicalName(name)))) return encoding;
        
        // Try MIME platform
        name = ucnv_getStandardName(icuName, "MIME", &errorCode);
        if ((NULL != name) && strncasecmp_l(icuName, name, strlen(name), NULL) && (RSStringEncodingInvalidId != (encoding = __RSStringEncodingGetFromCanonicalName(name)))) return encoding;
    }
    
    return RSStringEncodingInvalidId;
}

RSInline UConverter *__RSStringEncodingConverterCreateICUConverter(const char *icuName, uint32_t flags, BOOL toUnicode) {
    UConverter *converter;
    UErrorCode errorCode = U_ZERO_ERROR;
    uint8_t streamID = RSStringEncodingStreamIDFromMask(flags);
    
    if (0 != streamID) { // this is a part of streaming previously created
        __RSICUThreadData *data = __RSStringEncodingICUGetThreadData();
        
        --streamID; // map to array index
        
        if ((streamID < data->_numSlots) && (NULL != data->_converters[streamID])) return data->_converters[streamID];
    }
    
    converter = ucnv_open(icuName, &errorCode);
    
    if (NULL != converter) {
        char lossyByte = RSStringEncodingMaskToLossyByte(flags);
        
        if ((0 == lossyByte) && (0 != (flags & RSStringEncodingAllowLossyConversion))) lossyByte = '?';
        
        if (0 ==lossyByte) {
            if (toUnicode) {
                ucnv_setToUCallBack(converter, &UCNV_TO_U_CALLBACK_STOP, NULL, NULL, NULL, &errorCode);
            } else {
                ucnv_setFromUCallBack(converter, &UCNV_FROM_U_CALLBACK_STOP, NULL, NULL, NULL, &errorCode);
            }
        } else {
            ucnv_setSubstChars(converter, &lossyByte, 1, &errorCode);
        }
    }
    
    return converter;
}

#define ICU_CONVERTER_SLOT_INCREMENT (10)
#define ICU_CONVERTER_MAX_SLOT (255)

static RSIndex __RSStringEncodingConverterReleaseICUConverter(UConverter *converter, uint32_t flags, RSIndex status) {
    uint8_t streamID = RSStringEncodingStreamIDFromMask(flags);
    
    if ((RSStringEncodingInvalidInputStream != status) && ((0 != (flags & RSStringEncodingPartialInput)) || ((RSStringEncodingInsufficientOutputBufferLength == status) && (0 != (flags & RSStringEncodingPartialOutput))))) {
        if (0 == streamID) {
            __RSICUThreadData *data = __RSStringEncodingICUGetThreadData();
            
            if (NULL == data->_converters) {
                data->_converters = (UConverter **)RSAllocatorAllocate(NULL, sizeof(UConverter *) * ICU_CONVERTER_SLOT_INCREMENT);
                memset(data->_converters, 0, sizeof(UConverter *) * ICU_CONVERTER_SLOT_INCREMENT);
                data->_numSlots = ICU_CONVERTER_SLOT_INCREMENT;
                data->_nextSlot = 0;
            } else if ((data->_nextSlot >= data->_numSlots) || (NULL != data->_converters[data->_nextSlot])) { // Need to find one
                RSIndex index;
                
                for (index = 0;index < data->_numSlots;index++) {
                    if (NULL == data->_converters[index]) {
                        data->_nextSlot = index;
                        break;
                    }
                }
                
                if (index >= data->_numSlots) { // we're full
                    UConverter **newConverters;
                    RSIndex newSize = data->_numSlots + ICU_CONVERTER_SLOT_INCREMENT;
                    
                    if (newSize > ICU_CONVERTER_MAX_SLOT) { // something is terribly wrong
                        RSLog(RSLogLevelError, RSSTR("Per-thread streaming ID for ICU converters exhausted. Ignoring..."));
                        ucnv_close(converter);
                        return 0;
                    }
                    
                    newConverters = (UConverter **)RSAllocatorAllocate(NULL, sizeof(UConverter *) * newSize);
                    memset(newConverters, 0, sizeof(UConverter *) * newSize);
                    memcpy(newConverters, data->_converters, sizeof(UConverter *) * data->_numSlots);
                    RSAllocatorDeallocate(NULL, data->_converters);
                    data->_converters = newConverters;
                    data->_nextSlot = data->_numSlots;
                    data->_numSlots = newSize;
                }
            }
            
            data->_converters[data->_nextSlot] = converter;
            streamID = data->_nextSlot + 1;
            
            // now find next slot
            ++data->_nextSlot;
            
            if ((data->_nextSlot >= data->_numSlots) || (NULL != data->_converters[data->_nextSlot])) {
                data->_nextSlot = 0;
                
                while ((data->_nextSlot < data->_numSlots) && (NULL != data->_converters[data->_nextSlot])) ++data->_nextSlot;
            }
        }
        
        return RSStringEncodingStreamIDToMask(streamID);
    }
    
    if (0 != streamID) {
        __RSICUThreadData *data = __RSStringEncodingICUGetThreadData();
        
        --streamID; // map to array index
        
        if ((streamID < data->_numSlots) && (converter == data->_converters[streamID])) {
            data->_converters[streamID] = NULL;
            if (data->_nextSlot > streamID) data->_nextSlot = streamID;
        }
    }
    
    ucnv_close(converter);
    
    return 0;
}

#define MAX_BUFFER_SIZE (1000)

#if DEPLOYMENT_TARGET_MACOSX || DEPLOYMENT_TARGET_EMBEDDED
#if (U_ICU_VERSION_MAJOR_NUM > 49)
#warning Unknown ICU version. Check binary compatibility issues for rdar://problem/6024743
#endif
#endif
#define HAS_ICU_BUG_6024743 (1)
#define HAS_ICU_BUG_6025527 (1)

RSPrivate RSIndex __RSStringEncodingICUToBytes(const char *icuName, uint32_t flags, const UniChar *characters, RSIndex numChars, RSIndex *usedCharLen, uint8_t *bytes, RSIndex maxByteLen, RSIndex *usedByteLen) {
    UConverter *converter;
    UErrorCode errorCode = U_ZERO_ERROR;
    const UTF16Char *source = characters;
    const UTF16Char *sourceLimit = source + numChars;
    char *destination = (char *)bytes;
    const char *destinationLimit = destination + maxByteLen;
    BOOL flush = ((0 == (flags & RSStringEncodingPartialInput)) ? YES : NO);
    RSIndex status;
    
    if (NULL == (converter = __RSStringEncodingConverterCreateICUConverter(icuName, flags, NO))) return RSStringEncodingConverterUnavailable;
    
    if (0 == maxByteLen) {
        char buffer[MAX_BUFFER_SIZE];
        RSIndex totalLength = 0;
        
        while ((source < sourceLimit) && (U_ZERO_ERROR == errorCode)) {
            destination = buffer;
            destinationLimit = destination + MAX_BUFFER_SIZE;
            
            ucnv_fromUnicode(converter, &destination, destinationLimit, (const UChar **)&source, (const UChar *)sourceLimit, NULL, flush, &errorCode);
            
            totalLength += (destination - buffer);
            
            if (U_BUFFER_OVERFLOW_ERROR == errorCode) errorCode = U_ZERO_ERROR;
        }
        
        if (NULL != usedByteLen) *usedByteLen = totalLength;
    } else {
        ucnv_fromUnicode(converter, &destination, destinationLimit, (const UChar **)&source, (const UChar *)sourceLimit, NULL, flush, &errorCode);
        
#if HAS_ICU_BUG_6024743
        /* Another critical ICU design issue. Similar to conversion error, source pointer returned from U_BUFFER_OVERFLOW_ERROR is already beyond the last valid character position. It renders the returned value from source entirely unusable. We have to manually back up until succeeding <rdar://problem/7183045> Intrestingly, this issue doesn't apply to ucnv_toUnicode. The asynmmetric nature makes this more dangerous */
        if (U_BUFFER_OVERFLOW_ERROR == errorCode) {
            const uint8_t *bitmap = RSUniCharGetBitmapPtrForPlane(RSUniCharNonBaseCharacterSet, 0);
            const uint8_t *nonBase;
            UTF32Char character;
            
            do {
                // Since the output buffer is filled, we can assume no invalid chars (including stray surrogates)
                do {
                    sourceLimit = (source - 1);
                    character = *sourceLimit;
                    nonBase = bitmap;
                    
                    if (RSUniCharIsSurrogateLowCharacter(character)) {
                        --sourceLimit;
                        character = RSUniCharGetLongCharacterForSurrogatePair(*sourceLimit, character);
                        nonBase = RSUniCharGetBitmapPtrForPlane(RSUniCharNonBaseCharacterSet, (character >> 16) & 0x000F);
                        character &= 0xFFFF;
                    }
                } while ((sourceLimit > characters) && RSUniCharIsMemberOfBitmap(character, nonBase));
                
                if (sourceLimit > characters) {
                    source = characters;
                    destination = (char *)bytes;
                    errorCode = U_ZERO_ERROR;
                    
                    ucnv_resetFromUnicode(converter);
                    
                    ucnv_fromUnicode(converter, &destination, destinationLimit, (const UChar **)&source, (const UChar *)sourceLimit, NULL, flush, &errorCode);
                }
            } while (U_BUFFER_OVERFLOW_ERROR == errorCode);
            
            errorCode = U_BUFFER_OVERFLOW_ERROR;
        }
#endif
        if (NULL != usedByteLen) *usedByteLen = destination - (const char *)bytes;
    }
    
    status = ((U_ZERO_ERROR == errorCode) ? RSStringEncodingConversionSuccess : ((U_BUFFER_OVERFLOW_ERROR == errorCode) ? RSStringEncodingInsufficientOutputBufferLength : RSStringEncodingInvalidInputStream));
    
    if (NULL != usedCharLen) {
#if HAS_ICU_BUG_6024743
        /* ICU has a serious behavioral inconsistency issue that the source pointer returned from ucnv_fromUnicode() is after illegal input. We have to keep track of any changes in this area in order to prevent future binary compatiibility issues */
        if (RSStringEncodingInvalidInputStream == status) {
#define MAX_ERROR_BUFFER_LEN (32)
            UTF16Char errorBuffer[MAX_ERROR_BUFFER_LEN];
            int8_t errorLength = MAX_ERROR_BUFFER_LEN;
#undef MAX_ERROR_BUFFER_LEN
            
            errorCode = U_ZERO_ERROR;
            
            ucnv_getInvalidUChars(converter, (UChar *)errorBuffer, &errorLength, &errorCode);
            
            if (U_ZERO_ERROR == errorCode) {
                source -= errorLength;
            } else {
                // Gah, something is terribly wrong. Reset everything
                source = characters; // 0 length
                if (NULL != usedByteLen) *usedByteLen = 0;
            }
        }
#endif
        *usedCharLen = source - characters;
    }
    
    status |= __RSStringEncodingConverterReleaseICUConverter(converter, flags, status);
    
    return status;
}

RSPrivate RSIndex __RSStringEncodingICUToUnicode(const char *icuName, uint32_t flags, const uint8_t *bytes, RSIndex numBytes, RSIndex *usedByteLen, UniChar *characters, RSIndex maxCharLen, RSIndex *usedCharLen) {
    UConverter *converter;
    UErrorCode errorCode = U_ZERO_ERROR;
    const char *source = (const char *)bytes;
    const char *sourceLimit = source + numBytes;
    UTF16Char *destination = characters;
    const UTF16Char *destinationLimit = destination + maxCharLen;
    BOOL flush = ((0 == (flags & RSStringEncodingPartialInput)) ? YES : NO);
    RSIndex status;
    
    if (NULL == (converter = __RSStringEncodingConverterCreateICUConverter(icuName, flags, YES))) return RSStringEncodingConverterUnavailable;
    
    if (0 == maxCharLen) {
        UTF16Char buffer[MAX_BUFFER_SIZE];
        RSIndex totalLength = 0;
        
        while ((source < sourceLimit) && (U_ZERO_ERROR == errorCode)) {
            destination = buffer;
            destinationLimit = destination + MAX_BUFFER_SIZE;
            
            ucnv_toUnicode(converter, (UChar **)&destination, (const UChar *)destinationLimit, &source, sourceLimit, NULL, flush, &errorCode);
            
            totalLength += (destination - buffer);
            
            if (U_BUFFER_OVERFLOW_ERROR == errorCode) errorCode = U_ZERO_ERROR;
        }
        
        if (NULL != usedCharLen) *usedCharLen = totalLength;
    } else {
        ucnv_toUnicode(converter, (UChar **)&destination, (const UChar *)destinationLimit, &source, sourceLimit, NULL, flush, &errorCode);
        
        if (NULL != usedCharLen) *usedCharLen = destination - characters;
    }
    
    status = ((U_ZERO_ERROR == errorCode) ? RSStringEncodingConversionSuccess : ((U_BUFFER_OVERFLOW_ERROR == errorCode) ? RSStringEncodingInsufficientOutputBufferLength : RSStringEncodingInvalidInputStream));
    
    if (NULL != usedByteLen) {
#if HAS_ICU_BUG_6024743
        /* ICU has a serious behavioral inconsistency issue that the source pointer returned from ucnv_toUnicode() is after illegal input. We have to keep track of any changes in this area in order to prevent future binary compatiibility issues */
        if (RSStringEncodingInvalidInputStream == status) {
#define MAX_ERROR_BUFFER_LEN (32)
            char errorBuffer[MAX_ERROR_BUFFER_LEN];
            int8_t errorLength = MAX_ERROR_BUFFER_LEN;
#undef MAX_ERROR_BUFFER_LEN
            
            errorCode = U_ZERO_ERROR;
            
            ucnv_getInvalidChars(converter, errorBuffer, &errorLength, &errorCode);
            
            if (U_ZERO_ERROR == errorCode) {
#if HAS_ICU_BUG_6025527
                // Another ICU oddness here. ucnv_getInvalidUChars() writes the '\0' terminator, and errorLength includes the extra byte.
                if ((errorLength > 0) && ('\0' == errorBuffer[errorLength - 1])) --errorLength;
#endif
                source -= errorLength;
            } else {
                // Gah, something is terribly wrong. Reset everything
                source = (const char *)bytes; // 0 length
                if (NULL != usedCharLen) *usedCharLen = 0;
            }
        }
#endif
        
        *usedByteLen = source - (const char *)bytes;
    }
    
    status |= __RSStringEncodingConverterReleaseICUConverter(converter, flags, status);
    
    return status;
}

RSPrivate RSIndex __RSStringEncodingICUCharLength(const char *icuName, uint32_t flags, const uint8_t *bytes, RSIndex numBytes) {
    RSIndex usedCharLen;
    return (__RSStringEncodingICUToUnicode(icuName, flags, bytes, numBytes, NULL, NULL, 0, &usedCharLen) == RSStringEncodingConversionSuccess ? usedCharLen : 0);
}

RSPrivate RSIndex __RSStringEncodingICUByteLength(const char *icuName, uint32_t flags, const UniChar *characters, RSIndex numChars) {
    RSIndex usedByteLen;
    return (__RSStringEncodingICUToBytes(icuName, flags, characters, numChars, NULL, NULL, 0, &usedByteLen) == RSStringEncodingConversionSuccess ? usedByteLen : 0);
}

RSPrivate RSStringEncoding *__RSStringEncodingCreateICUEncodings(RSAllocatorRef allocator, RSIndex *numberOfIndex) {
    RSIndex count = ucnv_countAvailable();
    RSIndex numEncodings = 0;
    RSStringEncoding *encodings;
    RSStringEncoding encoding;
    RSIndex index;
    
    if (0 == count) return NULL;
    
    encodings = (RSStringEncoding *)RSAllocatorAllocate(NULL, sizeof(RSStringEncoding) * count);
    
    for (index = 0;index < count;index++) {
        encoding = __RSStringEncodingGetFromICUName(ucnv_getAvailableName((RSBit32)index));
        
        if (RSStringEncodingInvalidId != encoding) encodings[numEncodings++] = encoding;
    }
    
    if (0 == numEncodings) {
        RSAllocatorDeallocate(allocator, encodings);
        encodings = NULL;
    }
    
    *numberOfIndex = numEncodings;
    
    return encodings;
}
