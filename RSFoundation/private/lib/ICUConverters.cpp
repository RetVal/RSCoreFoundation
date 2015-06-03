//
//  ICUConverters.cpp
//  RSCoreFoundation
//
//  Created by closure on 6/3/15.
//  Copyright (c) 2015 RetVal. All rights reserved.
//

#include <RSFoundation/ICUConverters.h>

#include <RSFoundation/Database.h>
#include <RSFoundation/UniChar.h>
#include <RSFoundation/Allocator.h>
#include <RSFoundation/StringConverter.h>

#include "Internal.h"

#ifndef UCNV_H
#include "unicode/ucnv.h"
#endif

namespace RSFoundation {
    namespace Collection {
        namespace Encoding {
            // Thread data support
            typedef struct {
                uint8_t _numSlots;
                uint8_t _nextSlot;
                UConverter **_converters;
            } __ICUThreadData;
            
            static void __ICUThreadDataDestructor(void *context) {
                __ICUThreadData * data = (__ICUThreadData *)context;
                
                if (nil != data->_converters) { // scan to make sure deallocation
                    UConverter **converter = data->_converters;
                    UConverter **limit = converter + data->_numSlots;
                    
                    while (converter < limit) {
                        if (nil != converter) ucnv_close(*converter);
                        ++converter;
                    }
                    Allocator<decltype(data->_converters)>::AllocatorSystemDefault.Deallocate(data->_converters);
                }
                Allocator<decltype(data)>::AllocatorSystemDefault.Deallocate(data);
            }
            
            inline __ICUThreadData *__StringEncodingICUGetThreadData() {
                __ICUThreadData * data;
                
//                data = (__ICUThreadData *)_GetTSD(__TSDKeyICUConverter);
//                
//                if (nil == data) {
//                    data = (__ICUThreadData *)AllocatorAllocate(nil, sizeof(__ICUThreadData));
//                    memset(data, 0, sizeof(__ICUThreadData));
//                    _SetTSD(__TSDKeyICUConverter, (void *)data, __ICUThreadDataDestructor);
//                }
                
                return data;
            }
            
            Private const char *__StringEncodingGetICUName(String::Encoding encoding) {
#define STACK_BUFFER_SIZE (60)
                char buffer[STACK_BUFFER_SIZE];
                const char *result = nil;
                UErrorCode errorCode = U_ZERO_ERROR;
                uint32_t codepage = 0;
                
                if (String::Encoding::UTF7_IMAP == encoding) return "IMAP-mailbox-name";
                
                const DataBase& db = DataBase::SharedDataBase();
                
                if (String::Encoding::Unicode != (encoding & 0x0F00)) codepage = db.GetWindowsCodePage(encoding); // we don't use codepage for UTF to avoid little endian weirdness of Windows
                
                if ((0 != codepage) && (snprintf(buffer, STACK_BUFFER_SIZE, "windows-%d", codepage) < STACK_BUFFER_SIZE) && (nil != (result = ucnv_getAlias(buffer, 0, &errorCode)))) return result;
                
                if (db.GetCanonicalName(encoding, buffer, STACK_BUFFER_SIZE)) result = ucnv_getAlias(buffer, 0, &errorCode);
                
                return result;
#undef STACK_BUFFER_SIZE
            }
            
            Private String::Encoding __StringEncodingGetFromICUName(const char *icuName) {
                uint32_t codepage;
                UErrorCode errorCode = U_ZERO_ERROR;
                const DataBase& db = DataBase::SharedDataBase();
                if ((0 == strncasecmp_l(icuName, "windows-", strlen("windows-"), nil)) && (0 != (codepage = (uint32_t)strtol(icuName + strlen("windows-"), nil, 10)))) return db.GetFromWindowsCodePage(codepage);
                
                if (0 != ucnv_countAliases(icuName, &errorCode)) {
                    String::Encoding encoding;
                    const char *name;
                    
                    // Try WINDOWS platform
                    name = ucnv_getStandardName(icuName, "WINDOWS", &errorCode);
                    
                    if (nil != name) {
                        if ((0 == strncasecmp_l(name, "windows-", strlen("windows-"), nil)) && (0 != (codepage = (uint32_t)strtol(name + strlen("windows-"), nil, 10)))) return db.GetFromWindowsCodePage(codepage);
                        
                        if (strncasecmp_l(icuName, name, strlen(name), nil) && (String::Encoding::InvalidId != (encoding = db.GetFromCanonicalName(name)))) return encoding;
                    }
                    
                    // Try JAVA platform
                    name = ucnv_getStandardName(icuName, "JAVA", &errorCode);
                    if ((nil != name) && strncasecmp_l(icuName, name, strlen(name), nil) && (String::Encoding::InvalidId != (encoding = db.GetFromCanonicalName(name)))) return encoding;
                    
                    // Try MIME platform
                    name = ucnv_getStandardName(icuName, "MIME", &errorCode);
                    if ((nil != name) && strncasecmp_l(icuName, name, strlen(name), nil) && (String::Encoding::InvalidId != (encoding = db.GetFromCanonicalName(name)))) return encoding;
                }
                
                return String::Encoding::InvalidId;
            }
            
            inline UConverter *__StringEncodingConverterCreateICUConverter(const char *icuName, uint32_t flags, BOOL toUnicode) {
                UConverter *converter;
                UErrorCode errorCode = U_ZERO_ERROR;
                uint8_t streamID = StringEncodingStreamIDFromMask(flags);
                
                if (0 != streamID) { // this is a part of streaming previously created
                    __ICUThreadData *data = __StringEncodingICUGetThreadData();
                    
                    --streamID; // map to array index
                    
                    if ((streamID < data->_numSlots) && (nil != data->_converters[streamID])) return data->_converters[streamID];
                }
                
                converter = ucnv_open(icuName, &errorCode);
                
                if (nil != converter) {
                    char lossyByte = StringEncodingMaskToLossyByte(flags);
                    
                    if ((0 == lossyByte) && (0 != (flags & (uint32_t)String::EncodingConfiguration::AllowLossyConversion))) lossyByte = '?';
                    
                    if (0 ==lossyByte) {
                        if (toUnicode) {
                            ucnv_setToUCallBack(converter, &UCNV_TO_U_CALLBACK_STOP, nil, nil, nil, &errorCode);
                        } else {
                            ucnv_setFromUCallBack(converter, &UCNV_FROM_U_CALLBACK_STOP, nil, nil, nil, &errorCode);
                        }
                    } else {
                        ucnv_setSubstChars(converter, &lossyByte, 1, &errorCode);
                    }
                }
                
                return converter;
            }
            
#define ICU_CONVERTER_SLOT_INCREMENT (10)
#define ICU_CONVERTER_MAX_SLOT (255)
            
            static Index __StringEncodingConverterReleaseICUConverter(UConverter *converter, uint32_t flags, String::ConversionResult status) {
                uint8_t streamID = StringEncodingStreamIDFromMask(flags);
                
                if ((String::ConversionResult::InvalidInputStream != status) && ((0 != (flags & (uint32_t)String::EncodingConfiguration::PartialInput)) || ((String::ConversionResult::InsufficientOutputBufferLength == status) && (0 != (flags & (uint32_t)String::EncodingConfiguration::PartialOutput))))) {
                    if (0 == streamID) {
                        __ICUThreadData *data = __StringEncodingICUGetThreadData();
                        
                        if (nil == data->_converters) {
                            data->_converters = Allocator<UConverter *>::AllocatorSystemDefault.Allocate<UConverter *>(ICU_CONVERTER_SLOT_INCREMENT);
                            memset(data->_converters, 0, sizeof(UConverter *) * ICU_CONVERTER_SLOT_INCREMENT);
                            data->_numSlots = ICU_CONVERTER_SLOT_INCREMENT;
                            data->_nextSlot = 0;
                        } else if ((data->_nextSlot >= data->_numSlots) || (nil != data->_converters[data->_nextSlot])) { // Need to find one
                            Index index;
                            
                            for (index = 0;index < data->_numSlots;index++) {
                                if (nil == data->_converters[index]) {
                                    data->_nextSlot = index;
                                    break;
                                }
                            }
                            
                            if (index >= data->_numSlots) { // we're full
                                UConverter **newConverters;
                                Index newSize = data->_numSlots + ICU_CONVERTER_SLOT_INCREMENT;
                                
                                if (newSize > ICU_CONVERTER_MAX_SLOT) { // something is terribly wrong
//                                    Log(LogLevelError, String("Per-thread streaming ID for ICU converters exhausted. Ignoring..."));
                                    ucnv_close(converter);
                                    return 0;
                                }
                                
                                newConverters = Allocator<UConverter *>::AllocatorSystemDefault.Allocate<UConverter *>(newSize);
//                                newConverters = (UConverter **)AllocatorAllocate(nil, sizeof(UConverter *) * newSize);
                                memset(newConverters, 0, sizeof(UConverter *) * newSize);
                                memcpy(newConverters, data->_converters, sizeof(UConverter *) * data->_numSlots);
                                Allocator<decltype(data->_converters)>::AllocatorSystemDefault.Deallocate(data->_converters);
//                                AllocatorDeallocate(nil, data->_converters);
                                data->_converters = newConverters;
                                data->_nextSlot = data->_numSlots;
                                data->_numSlots = newSize;
                            }
                        }
                        
                        data->_converters[data->_nextSlot] = converter;
                        streamID = data->_nextSlot + 1;
                        
                        // now find next slot
                        ++data->_nextSlot;
                        
                        if ((data->_nextSlot >= data->_numSlots) || (nil != data->_converters[data->_nextSlot])) {
                            data->_nextSlot = 0;
                            
                            while ((data->_nextSlot < data->_numSlots) && (nil != data->_converters[data->_nextSlot])) ++data->_nextSlot;
                        }
                    }
                    
                    return StringEncodingStreamIDToMask(streamID);
                }
                
                if (0 != streamID) {
                    __ICUThreadData *data = __StringEncodingICUGetThreadData();
                    
                    --streamID; // map to array index
                    
                    if ((streamID < data->_numSlots) && (converter == data->_converters[streamID])) {
                        data->_converters[streamID] = nil;
                        if (data->_nextSlot > streamID) data->_nextSlot = streamID;
                    }
                }
                
                ucnv_close(converter);
                
                return 0;
            }
            
#define MAX_BUFFER_SIZE (1000)
            
#if DEPLOYMENT_TARGET_MACOSX || DEPLOYMENT_TARGET_EMBEDDED
#if (U_ICU_VEION_MAJOR_NUM > 49)
#warning Unknown ICU version. Check binary compatibility issues for rdar://problem/6024743
#endif
#endif
#define HAS_ICU_BUG_6024743 (1)
#define HAS_ICU_BUG_6025527 (1)
            
            Private Index __StringEncodingICUToBytes(const char *icuName, uint32_t flags, const UniChar *characters, Index numChars, Index *usedCharLen, uint8_t *bytes, Index maxByteLen, Index *usedByteLen) {
                UConverter *converter;
                UErrorCode errorCode = U_ZERO_ERROR;
                const UTF16Char *source = characters;
                const UTF16Char *sourceLimit = source + numChars;
                char *destination = (char *)bytes;
                const char *destinationLimit = destination + maxByteLen;
                BOOL flush = ((0 == (flags & (uint32_t)String::EncodingConfiguration::PartialInput)) ? YES : NO);
                Index status;
                
                if (nil == (converter = __StringEncodingConverterCreateICUConverter(icuName, flags, NO))) return String::ConversionResult::Unavailable;
                
                if (0 == maxByteLen) {
                    char buffer[MAX_BUFFER_SIZE];
                    Index totalLength = 0;
                    
                    while ((source < sourceLimit) && (U_ZERO_ERROR == errorCode)) {
                        destination = buffer;
                        destinationLimit = destination + MAX_BUFFER_SIZE;
                        
                        ucnv_fromUnicode(converter, &destination, destinationLimit, (const UChar **)&source, (const UChar *)sourceLimit, nil, flush, &errorCode);
                        
                        totalLength += (destination - buffer);
                        
                        if (U_BUFFER_OVERFLOW_ERROR == errorCode) errorCode = U_ZERO_ERROR;
                    }
                    
                    if (nil != usedByteLen) *usedByteLen = totalLength;
                } else {
                    ucnv_fromUnicode(converter, &destination, destinationLimit, (const UChar **)&source, (const UChar *)sourceLimit, nil, flush, &errorCode);
                    
#if HAS_ICU_BUG_6024743
                    /* Another critical ICU design issue. Similar to conversion error, source pointer returned from U_BUFFER_OVERFLOW_ERROR is already beyond the last valid character position. It renders the returned value from source entirely unusable. We have to manually back up until succeeding <rdar://problem/7183045> Intrestingly, this issue doesn't apply to ucnv_toUnicode. The asynmmetric nature makes this more dangerous */
                    if (U_BUFFER_OVERFLOW_ERROR == errorCode) {
                        const uint8_t *bitmap = UniCharGetBitmapPtrForPlane(NonBaseCharacterSet, 0);
                        const uint8_t *nonBase;
                        UTF32Char character;
                        
                        do {
                            // Since the output buffer is filled, we can assume no invalid chars (including stray surrogates)
                            do {
                                sourceLimit = (source - 1);
                                character = *sourceLimit;
                                nonBase = bitmap;
                                
                                if (UniCharIsSurrogateLowCharacter(character)) {
                                    --sourceLimit;
                                    character = UniCharGetLongCharacterForSurrogatePair(*sourceLimit, character);
                                    nonBase = UniCharGetBitmapPtrForPlane(NonBaseCharacterSet, (character >> 16) & 0x000F);
                                    character &= 0xFFFF;
                                }
                            } while ((sourceLimit > characters) && UniCharIsMemberOfBitmap(character, nonBase));
                            
                            if (sourceLimit > characters) {
                                source = characters;
                                destination = (char *)bytes;
                                errorCode = U_ZERO_ERROR;
                                
                                ucnv_resetFromUnicode(converter);
                                
                                ucnv_fromUnicode(converter, &destination, destinationLimit, (const UChar **)&source, (const UChar *)sourceLimit, nil, flush, &errorCode);
                            }
                        } while (U_BUFFER_OVERFLOW_ERROR == errorCode);
                        
                        errorCode = U_BUFFER_OVERFLOW_ERROR;
                    }
#endif
                    if (nil != usedByteLen) *usedByteLen = destination - (const char *)bytes;
                }
                
                status = ((U_ZERO_ERROR == errorCode) ? String::ConversionResult::Success : ((U_BUFFER_OVERFLOW_ERROR == errorCode) ? String::ConversionResult::InsufficientOutputBufferLength : String::ConversionResult::InvalidInputStream));
                
                if (nil != usedCharLen) {
#if HAS_ICU_BUG_6024743
                    /* ICU has a serious behavioral inconsistency issue that the source pointer returned from ucnv_fromUnicode() is after illegal input. We have to keep track of any changes in this area in order to prevent future binary compatiibility issues */
                    if (String::ConversionResult::InvalidInputStream == status) {
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
                            if (nil != usedByteLen) *usedByteLen = 0;
                        }
                    }
#endif
                    *usedCharLen = source - characters;
                }
                
                status |= __StringEncodingConverterReleaseICUConverter(converter, flags, (String::ConversionResult)status);
                
                return status;
            }
            
            Private Index __StringEncodingICUToUnicode(const char *icuName, uint32_t flags, const uint8_t *bytes, Index numBytes, Index *usedByteLen, UniChar *characters, Index maxCharLen, Index *usedCharLen) {
                UConverter *converter;
                UErrorCode errorCode = U_ZERO_ERROR;
                const char *source = (const char *)bytes;
                const char *sourceLimit = source + numBytes;
                UTF16Char *destination = characters;
                const UTF16Char *destinationLimit = destination + maxCharLen;
                BOOL flush = ((0 == (flags & (uint32_t)String::EncodingConfiguration::PartialInput)) ? YES : NO);
                Index status;
                
                if (nil == (converter = __StringEncodingConverterCreateICUConverter(icuName, flags, YES))) return String::ConversionResult::Unavailable;
                
                if (0 == maxCharLen) {
                    UTF16Char buffer[MAX_BUFFER_SIZE];
                    Index totalLength = 0;
                    
                    while ((source < sourceLimit) && (U_ZERO_ERROR == errorCode)) {
                        destination = buffer;
                        destinationLimit = destination + MAX_BUFFER_SIZE;
                        
                        ucnv_toUnicode(converter, (UChar **)&destination, (const UChar *)destinationLimit, &source, sourceLimit, nil, flush, &errorCode);
                        
                        totalLength += (destination - buffer);
                        
                        if (U_BUFFER_OVERFLOW_ERROR == errorCode) errorCode = U_ZERO_ERROR;
                    }
                    
                    if (nil != usedCharLen) *usedCharLen = totalLength;
                } else {
                    ucnv_toUnicode(converter, (UChar **)&destination, (const UChar *)destinationLimit, &source, sourceLimit, nil, flush, &errorCode);
                    
                    if (nil != usedCharLen) *usedCharLen = destination - characters;
                }
                
                status = ((U_ZERO_ERROR == errorCode) ? String::ConversionResult::Success : ((U_BUFFER_OVERFLOW_ERROR == errorCode) ? String::ConversionResult::InsufficientOutputBufferLength : String::ConversionResult::InvalidInputStream));
                
                if (nil != usedByteLen) {
#if HAS_ICU_BUG_6024743
                    /* ICU has a serious behavioral inconsistency issue that the source pointer returned from ucnv_toUnicode() is after illegal input. We have to keep track of any changes in this area in order to prevent future binary compatiibility issues */
                    if (String::ConversionResult::InvalidInputStream == status) {
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
                            if (nil != usedCharLen) *usedCharLen = 0;
                        }
                    }
#endif
                    
                    *usedByteLen = source - (const char *)bytes;
                }
                
                status |= __StringEncodingConverterReleaseICUConverter(converter, flags, (String::ConversionResult)status);
                
                return status;
            }
            
            Private Index __StringEncodingICUCharLength(const char *icuName, uint32_t flags, const uint8_t *bytes, Index numBytes) {
                Index usedCharLen;
                return (__StringEncodingICUToUnicode(icuName, flags, bytes, numBytes, nil, nil, 0, &usedCharLen) == String::ConversionResult::Success ? usedCharLen : 0);
            }
            
            Private Index __StringEncodingICUByteLength(const char *icuName, uint32_t flags, const UniChar *characters, Index numChars) {
                Index usedByteLen;
                return (__StringEncodingICUToBytes(icuName, flags, characters, numChars, nil, nil, 0, &usedByteLen) == String::ConversionResult::Success ? usedByteLen : 0);
            }
            
            Private String::Encoding *__StringEncodingCreateICUEncodings(Index *numberOfIndex) {
                Index count = ucnv_countAvailable();
                Index numEncodings = 0;
                String::Encoding *encodings;
                String::Encoding encoding;
                Index index;
                
                if (0 == count) return nil;
                auto allocator = &Allocator<String::Encoding>::AllocatorSystemDefault;
                encodings = allocator->Allocate<String::Encoding>(count);
                
                for (index = 0;index < count;index++) {
                    encoding = __StringEncodingGetFromICUName(ucnv_getAvailableName((UInt32)index));
                    
                    if (String::Encoding::InvalidId != encoding) encodings[numEncodings++] = encoding;
                }
                
                if (0 == numEncodings) {
                    allocator->Deallocate(encodings);
                    encodings = nil;
                }
                
                *numberOfIndex = numEncodings;
                
                return encodings;
            }

        }
    }
}

