//
//  String.cpp
//  CoreFoundation
//
//  Created by closure on 5/27/15.
//  Copyright (c) 2015 RetVal. All rights reserved.
//

#include <RSFoundation/String.hpp>
#include <RSFoundation/Order.hpp>
#include <RSFoundation/UniChar.hpp>
#include <RSFoundation/StringConverter.hpp>
#include <RSFoundation/StringConverterExt.hpp>

#include "Internal.hpp"

namespace RSFoundation {
    namespace Collection {
        namespace StringPrivate {
            static inline bool _CanBeStoredInEightBit(String::Encoding encoding) {
                switch (encoding & 0xFFF) {
                        // just use encoding base
                    case String::InvalidId:
                    case String::Unicode:
                    case String::NonLossyASCII:
                        return NO;
                        
                    case String::MacRoman:
                    case String::WindowsLatin1:
                    case String::ISOLatin1:
                    case String::NextStepLatin:
                    case String::ASCII:
                        return YES;
                        
                    default: return NO;
                }
            }
            
            static inline bool _BytesInASCII(const uint8_t *bytes, Index len) {
#if __LP64__
                /* A bit of unrolling; go by 32s, 16s, and 8s first */
                while (len >= 32) {
                    uint64_t val = *(const uint64_t *)bytes;
                    uint64_t hiBits = (val & 0x8080808080808080ULL);    // More efficient to collect this rather than do a conditional at every step
                    bytes += 8;
                    val = *(const uint64_t *)bytes;
                    hiBits |= (val & 0x8080808080808080ULL);
                    bytes += 8;
                    val = *(const uint64_t *)bytes;
                    hiBits |= (val & 0x8080808080808080ULL);
                    bytes += 8;
                    val = *(const uint64_t *)bytes;
                    if (hiBits | (val & 0x8080808080808080ULL)) return false;
                    bytes += 8;
                    len -= 32;
                }
                
                while (len >= 16) {
                    uint64_t val = *(const uint64_t *)bytes;
                    uint64_t hiBits = (val & 0x8080808080808080ULL);
                    bytes += 8;
                    val = *(const uint64_t *)bytes;
                    if (hiBits | (val & 0x8080808080808080ULL)) return false;
                    bytes += 8;
                    len -= 16;
                }
                
                while (len >= 8) {
                    uint64_t val = *(const uint64_t *)bytes;
                    if (val & 0x8080808080808080ULL) return false;
                    bytes += 8;
                    len -= 8;
                }
#endif
                /* Go by 4s */
                while (len >= 4) {
                    uint32_t val = *(const uint32_t *)bytes;
                    if (val & 0x80808080U) return false;
                    bytes += 4;
                    len -= 4;
                }
                /* Handle the rest one byte at a time */
                while (len--) {
                    if (*bytes++ & 0x80) return false;
                }
                
                return true;
            }
            
            static String::Encoding __DefaultEightBitStringEncoding;
            
            static String::Encoding _ComputeEightBitStringEncoding(void) {
                if (__DefaultEightBitStringEncoding == String::Encoding::InvalidId) {
                    String::Encoding systemEncoding = String::GetSystemEncoding();
                    if (systemEncoding == String::Encoding::InvalidId) { // We're right in the middle of querying system encoding from default database. Delaying to set until system encoding is determined.
                        return String::Encoding::ASCII;
                    } else if (_CanBeStoredInEightBit(systemEncoding)) {
                        __DefaultEightBitStringEncoding = systemEncoding;
                    } else {
                        __DefaultEightBitStringEncoding = String::Encoding::ASCII;
                    }
                }
                
                return __DefaultEightBitStringEncoding;
            }
            
            static inline String::Encoding _GetEightBitStringEncoding(void) {
                if (__DefaultEightBitStringEncoding == String::Encoding::InvalidId) _ComputeEightBitStringEncoding();
                return __DefaultEightBitStringEncoding;
            }
            
            static inline bool _IsSupersetOfASCII(String::Encoding encoding) {
                switch (encoding & 0x0000FF00) {
                    case 0x0: // MacOS Script range
                        // Symbol & bidi encodings are not ASCII superset
                        if (encoding == String::MacJapanese || encoding == String::MacArabic || encoding == String::MacHebrew || encoding == String::MacUkrainian || encoding == String::MacSymbol || encoding == String::MacDingbats) return NO;
                        return YES;
                        
                    case 0x100: // Unicode range
                        if (encoding != String::UTF8) return NO;
                        return YES;
                        
                    case 0x200: // ISO range
                        if (encoding == String::ISOLatinArabic) return NO;
                        return YES;
                        
                    case 0x600: // National standards range
                        if (encoding != String::ASCII) return NO;
                        return YES;
                        
                    case 0x800: // ISO 2022 range
                        return NO; // It's modal encoding
                        
                    case 0xA00: // Misc standard range
                        if ((encoding == String::ShiftJIS) || (encoding == String::HZ_GB_2312) || (encoding == String::UTF7_IMAP)) return NO;
                        return YES;
                        
                    case 0xB00:
                        if (encoding == String::NonLossyASCII) return NO;
                        return YES;
                        
                    case 0xC00: // EBCDIC
                        return NO;
                        
                    default:
                        return ((encoding & 0x0000FF00) > 0x0C00 ? NO : YES);
                }
            }
            
            static inline bool _CanUseEightBitStringForBytes(const uint8_t *bytes, Index len, String::Encoding encoding) {
                
                // If the encoding is the same as the 8-bit String encoding, we can just use the bytes as-is.
                // One exception is ASCII, which unfortunately needs to mean ISOLatin1 for compatibility reasons <rdar://problem/5458321>.
                if (encoding == _GetEightBitStringEncoding() && encoding != String::ASCII) return YES;
                if (_IsSupersetOfASCII(encoding) && _BytesInASCII(bytes, len)) return YES;
                return NO;
            }
            
            static inline bool _CanUseLengthByte(Index len) {
                return (len <= 255) ? true : false;
            }
            
            /* rearrangeBlocks() rearranges the blocks of data within the buffer so that they are "evenly spaced". buffer is assumed to have enough room for the result.
             numBlocks is current total number of blocks within buffer.
             blockSize is the size of each block in bytes
             ranges and numRanges hold the ranges that are no longer needed; ranges are stored sorted in increasing order, and don't overlap
             insertLength is the final spacing between the remaining blocks
             
             Example: buffer = A B C D E F G H, blockSize = 1, ranges = { (2,1) , (4,2) }  (so we want to "delete" C and E F), fromEnd = NO
             if insertLength = 4, result = A B ? ? ? ? D ? ? ? ? G H
             if insertLength = 0, result = A B D G H
             
             Example: buffer = A B C D E F G H I J K L M N O P Q R S T U, blockSize = 1, ranges { (1,1), (3,1), (5,11), (17,1), (19,1) }, fromEnd = NO
             if insertLength = 3, result = A ? ? ? C ? ? ? E ? ? ? Q ? ? ? S ? ? ? U
             
             */
            typedef struct _StringDeferredRange {
                Index beginning;
                Index length;
                Index shift;
            } StringDeferredRange;
            
            typedef struct _StringStackInfo {
                Index capacity;		// Capacity (if capacity == count, need to realloc to add another)
                Index count;			// Number of elements actually stored
                StringDeferredRange *stack;
                bool hasMalloced;	// Indicates "stack" is allocated and needs to be deallocated when done
                char _padding[3];
            } StringStackInfo;
            
            inline void pop (StringStackInfo *si, StringDeferredRange *topRange) {
                si->count = si->count - 1;
                *topRange = si->stack[si->count];
            }
            
            inline void push (StringStackInfo *si, const StringDeferredRange *newRange) {
                if (si->count == si->capacity) {
                    // increase size of the stack
                    si->capacity = (si->capacity + 4) * 2;
                    if (si->hasMalloced) {
                        si->stack = (StringDeferredRange *)realloc(si->stack, si->capacity * sizeof(StringDeferredRange));
                        //                        si->stack = (StringDeferredRange *)AllocatorReallocate(SystemDefault, si->stack, si->capacity * sizeof(StringDeferredRange));
                    } else {
                        StringDeferredRange *newStack = (StringDeferredRange *)malloc(si->capacity * sizeof(StringDeferredRange));
                        memmove(newStack, si->stack, si->count * sizeof(StringDeferredRange));
                        si->stack = newStack;
                        si->hasMalloced = YES;
                    }
                }
                si->stack[si->count] = *newRange;
                si->count = si->count + 1;
            }
            
            static void rearrangeBlocks(uint8_t *buffer,
                                        Index numBlocks,
                                        Index blockSize,
                                        const Range *ranges,
                                        Index numRanges,
                                        Index insertLength)
            {
                
#define origStackSize 10
                StringDeferredRange origStack[origStackSize];
                StringStackInfo si = {origStackSize, 0, origStack, NO, {0, 0, 0}};
                StringDeferredRange currentNonRange = {0, 0, 0};
                Index currentRange = 0;
                Index amountShifted = 0;
                
                // must have at least 1 range left.
                
                while (currentRange < numRanges)
                {
                    currentNonRange.beginning = (ranges[currentRange].location + ranges[currentRange].length) * blockSize;
                    if ((numRanges - currentRange) == 1)
                    {
                        // at the end.
                        currentNonRange.length = numBlocks * blockSize - currentNonRange.beginning;
                        if (currentNonRange.length == 0) break;
                    }
                    else
                    {
                        currentNonRange.length = (ranges[currentRange + 1].location * blockSize) - currentNonRange.beginning;
                    }
                    currentNonRange.shift = amountShifted + (insertLength * blockSize) - (ranges[currentRange].length * blockSize);
                    amountShifted = currentNonRange.shift;
                    if (amountShifted <= 0)
                    {
                        // process current item and rest of stack
                        if (currentNonRange.shift && currentNonRange.length) memmove (&buffer[currentNonRange.beginning + currentNonRange.shift], &buffer[currentNonRange.beginning], currentNonRange.length);
                        while (si.count > 0)
                        {
                            pop (&si, &currentNonRange);  // currentNonRange now equals the top element of the stack.
                            if (currentNonRange.shift && currentNonRange.length) memmove (&buffer[currentNonRange.beginning + currentNonRange.shift], &buffer[currentNonRange.beginning], currentNonRange.length);
                        }
                    }
                    else
                    {
                        // add currentNonRange to stack.
                        push (&si, &currentNonRange);
                    }
                    currentRange++;
                }
                
                // no more ranges.  if anything is on the stack, process.
                
                while (si.count > 0)
                {
                    pop (&si, &currentNonRange);  // currentNonRange now equals the top element of the stack.
                    if (currentNonRange.shift && currentNonRange.length) memmove (&buffer[currentNonRange.beginning + currentNonRange.shift], &buffer[currentNonRange.beginning], currentNonRange.length);
                }
                if (si.hasMalloced) free(si.stack);
            }
            
            /* See comments for rearrangeBlocks(); this is the same, but the string is assembled in another buffer (dstBuffer), so the algorithm is much easier. We also take care of the case where the source is not-Unicode but destination is. (The reverse case is not supported.)
             */
            static void copyBlocks(const uint8_t *srcBuffer,
                                   uint8_t *dstBuffer,
                                   Index srcLength,
                                   bool srcIsUnicode,
                                   bool dstIsUnicode,
                                   const Range *ranges,
                                   Index numRanges,
                                   Index insertLength) {
                Index srcLocationInBytes = 0;	// in order to avoid multiplying all the time, this is in terms of bytes, not blocks
                Index dstLocationInBytes = 0;	// ditto
                Index srcBlockSize = srcIsUnicode ? sizeof(UniChar) : sizeof(uint8_t);
                Index insertLengthInBytes = insertLength * (dstIsUnicode ? sizeof(UniChar) : sizeof(uint8_t));
                Index rangeIndex = 0;
                Index srcToDstMultiplier = (srcIsUnicode == dstIsUnicode) ? 1 : (sizeof(UniChar) / sizeof(uint8_t));
                
                // Loop over the ranges, copying the range to be preserved (right before each range)
                while (rangeIndex < numRanges) {
                    Index srcLengthInBytes = ranges[rangeIndex].location * srcBlockSize - srcLocationInBytes;	// srcLengthInBytes is in terms of bytes, not blocks; represents length of region to be preserved
                    if (srcLengthInBytes > 0) {
                        if (srcIsUnicode == dstIsUnicode) {
                            memmove(dstBuffer + dstLocationInBytes, srcBuffer + srcLocationInBytes, srcLengthInBytes);
                        } else {
                            //                            __StrConvertBytesToUnicode(srcBuffer + srcLocationInBytes, (UniChar *)(dstBuffer + dstLocationInBytes), srcLengthInBytes);
                        }
                    }
                    srcLocationInBytes += srcLengthInBytes + ranges[rangeIndex].length * srcBlockSize;	// Skip over the just-copied and to-be-deleted stuff
                    dstLocationInBytes += srcLengthInBytes * srcToDstMultiplier + insertLengthInBytes;
                    rangeIndex++;
                }
                
                // Do last range (the one beyond last range)
                if (srcLocationInBytes < srcLength * srcBlockSize) {
                    if (srcIsUnicode == dstIsUnicode) {
                        memmove(dstBuffer + dstLocationInBytes, srcBuffer + srcLocationInBytes, srcLength * srcBlockSize - srcLocationInBytes);
                    } else {
                        //                        __StrConvertBytesToUnicode(srcBuffer + srcLocationInBytes, (UniChar *)(dstBuffer + dstLocationInBytes), srcLength * srcBlockSize - srcLocationInBytes);
                    }
                }
            }
            
            void _HandleOutOfMemory() {
                //                String msg("Out of memory. We suggest restarting the application. If you have an unsaved document, create a backup copy in Finder, then try to save."); {
                //                    String("%r"), msg);
            }
        }
        
        enum {
            __VarWidthLocalBufferSize = 1008
        };
        
        typedef struct {      /* A simple struct to maintain ASCII/Unicode versions of the same buffer. */
            union {
                UInt8 *ascii;
                UniChar *unicode;
            } chars;
            bool isASCII;	/* This really does mean 7-bit ASCII, not _NSDefaultCStringEncoding() */
            bool shouldFreeChars;	/* If the number of bytes exceeds __VarWidthLocalBufferSize, bytes are allocated */
            bool _unused1;
            bool _unused2;
            Allocator<String> *allocator;	/* Use this allocator to allocate, reallocate, and deallocate the bytes */
            Index numChars;	/* This is in terms of ascii or unicode; that is, if isASCII, it is number of 7-bit chars; otherwise it is number of UniChars; note that the actual allocated space might be larger */
            UInt8 localBuffer[__VarWidthLocalBufferSize];	/* private; 168 ISO2022JP chars, 504 Unicode chars, 1008 ASCII chars */
        } VarWidthCharBuffer;
        
        enum {_StringErrNone = 0, _StringErrNotMutable = 1, _StringErrNilArg = 2, _StringErrBounds = 3};
        
        namespace StringEncodeDecode {
            
            /* The minimum length the output buffers should be in the above functions
             */
#define CharConversionBufferLength 512
            
            
#define MAX_LOCAL_CHA		(sizeof(buffer->localBuffer) / sizeof(uint8_t))
#define MAX_LOCAL_UNICHA	(sizeof(buffer->localBuffer) / sizeof(UniChar))
            
            enum  CodingMode : Index {
                NonLossyErrorMode = -1,
                NonLossyASCIIMode = 0,
                NonLossyBackslashMode = 1,
                NonLossyHexInitialMode = NonLossyBackslashMode + 1,
                NonLossyHexFinalMode = NonLossyHexInitialMode + 4,
                NonLossyOctalInitialMode = NonLossyHexFinalMode + 1,
                NonLossyOctalFinalMode = NonLossyHexFinalMode + 3
            };
            
            static bool __WantsToUseASCIICompatibleConversion = NO;
            inline UInt32 __GetASCIICompatibleFlag(void) { return __WantsToUseASCIICompatibleConversion; }
            
            void _StringEncodingSetForceASCIICompatibility(bool flag) {
                __WantsToUseASCIICompatibleConversion = (flag ? (UInt32)true : (UInt32)false);
            }
            
            
            bool __StringDecodeByteStream3(const uint8_t *bytes, Index len, String::Encoding encoding, bool alwaysUnicode, VarWidthCharBuffer *buffer, bool *useClientsMemoryPtr, UInt32 converterFlags) {
                Index idx;
                const uint8_t *chars = (const uint8_t *)bytes;
                const uint8_t *end = chars + len;
                bool result = YES;
                
                if (useClientsMemoryPtr) *useClientsMemoryPtr = NO;
                
                buffer->isASCII = !alwaysUnicode;
                buffer->shouldFreeChars = NO;
                buffer->numChars = 0;
                
                if (0 == len) return YES;
                
                buffer->allocator = (buffer->allocator ? buffer->allocator : &Allocator<String>::SystemDefault);
                
                if ((encoding == String::Encoding::UTF16) || (encoding == String::Encoding::UTF16BE) || (encoding == String::Encoding::UTF16LE)) {
                    // UTF-16
                    const UTF16Char *src = (const UTF16Char *)bytes;
                    const UTF16Char *limit = src + (len / sizeof(UTF16Char)); // <rdar://problem/7854378> avoiding odd len issue
                    bool swap = NO;
                    
                    if (String::Encoding::UTF16 == encoding) {
                        UTF16Char bom = ((*src == 0xFFFE) || (*src == 0xFEFF) ? *(src++) : 0);
                        
#if ___BIG_ENDIAN__
                        if (bom == 0xFFFE) swap = YES;
#else
                        if (bom != 0xFEFF) swap = YES;
#endif
                        if (bom) useClientsMemoryPtr = nil;
                    } else {
#if ___BIG_ENDIAN__
                        if (String::Encoding::UTF16LE == encoding) swap = YES;
#else
                        if (String::Encoding::UTF16BE == encoding) swap = YES;
#endif
                    }
                    
                    buffer->numChars = limit - src;
                    
                    if (useClientsMemoryPtr && !swap) {
                        // If the caller is ready to deal with no-copy situation, and the situation is possible, indicate it...
                        *useClientsMemoryPtr = YES;
                        buffer->chars.unicode = (UniChar *)src;
                        buffer->isASCII = NO;
                    } else {
                        if (buffer->isASCII) {
                            // Let's see if we can reduce the Unicode down to ASCII...
                            const UTF16Char *characters = src;
                            UTF16Char mask = (swap ? 0x80FF : 0xFF80);
                            
                            while (characters < limit) {
                                if (*(characters++) & mask) {
                                    buffer->isASCII = NO;
                                    break;
                                }
                            }
                        }
                        
                        if (buffer->isASCII) {
                            uint8_t *dst;
                            if (nil == buffer->chars.ascii) {
                                // we never reallocate when buffer is supplied
                                if (buffer->numChars > MAX_LOCAL_CHA) {
                                    buffer->chars.ascii = buffer->allocator->Allocate<UInt8>((buffer->numChars));
                                    if (!buffer->chars.ascii) goto memoryErrorExit;
                                    buffer->shouldFreeChars = YES;
                                } else {
                                    buffer->chars.ascii = (uint8_t *)buffer->localBuffer;
                                }
                            }
                            dst = buffer->chars.ascii;
                            
                            if (swap) {
                                while (src < limit) *(dst++) = (*(src++) >> 8);
                            } else {
                                while (src < limit) *(dst++) = (uint8_t)*(src++);
                            }
                        } else {
                            UTF16Char *dst;
                            
                            if (nil == buffer->chars.unicode) {
                                // we never reallocate when buffer is supplied
                                if (buffer->numChars > MAX_LOCAL_UNICHA) {
                                    buffer->chars.unicode = buffer->allocator->Allocate<UniChar>(buffer->numChars);
                                    if (!buffer->chars.unicode) goto memoryErrorExit;
                                    buffer->shouldFreeChars = YES;
                                } else {
                                    buffer->chars.unicode = (UTF16Char *)buffer->localBuffer;
                                }
                            }
                            dst = buffer->chars.unicode;
                            
                            if (swap) {
                                while (src < limit) *(dst++) = SwapInt16(*(src++));
                            } else {
                                memmove(dst, src, buffer->numChars * sizeof(UTF16Char));
                            }
                        }
                    }
                } else if ((encoding == String::Encoding::UTF32) || (encoding == String::Encoding::UTF32BE) || (encoding == String::Encoding::UTF32LE)) {
                    const UTF32Char *src = (const UTF32Char *)bytes;
                    const UTF32Char *limit =  src + (len / sizeof(UTF32Char)); // <rdar://problem/7854378> avoiding odd len issue
                    bool swap = NO;
                    static bool strictUTF32 = (bool)-1;
                    
                    if ((bool)-1 == strictUTF32) strictUTF32 = (1 != 0);
                    
                    if (String::Encoding::UTF32 == encoding) {
                        UTF32Char bom = ((*src == 0xFFFE0000) || (*src == 0x0000FEFF) ? *(src++) : 0);
                        
#if ___BIG_ENDIAN__
                        if (bom == 0xFFFE0000) swap = YES;
#else
                        if (bom != 0x0000FEFF) swap = YES;
#endif
                    } else {
#if ___BIG_ENDIAN__
                        if (Encoding::UTF32LE == encoding) swap = YES;
#else
                        if (String::Encoding::UTF32BE == encoding) swap = YES;
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
                                    if (strictUTF32 && ((swap ? (UTF32Char)SwapInt32(*characters) : *characters) > 0x10FFFF)) return NO; // outside of Unicode Scaler Value. Haven't allocated buffer, yet.
                                    ++(buffer->numChars);
                                }
                            }
                            ++characters;
                        }
                    }
                    
                    if (buffer->isASCII) {
                        uint8_t *dst;
                        if (nil == buffer->chars.ascii) {
                            // we never reallocate when buffer is supplied
                            if (buffer->numChars > MAX_LOCAL_CHA) {
                                buffer->chars.ascii = buffer->allocator->Allocate<UInt8>(buffer->numChars);
                                if (!buffer->chars.ascii) goto memoryErrorExit;
                                buffer->shouldFreeChars = YES;
                            } else {
                                buffer->chars.ascii = (uint8_t *)buffer->localBuffer;
                            }
                        }
                        dst = buffer->chars.ascii;
                        
                        if (swap) {
                            while (src < limit) *(dst++) = (*(src++) >> 24);
                        } else {
                            while (src < limit) *(dst++) = *(src++);
                        }
                    } else {
                        if (nil == buffer->chars.unicode) {
                            // we never reallocate when buffer is supplied
                            if (buffer->numChars > MAX_LOCAL_UNICHA) {
                                buffer->chars.unicode = buffer->allocator->Allocate<UTF16Char>(buffer->numChars);
                                if (!buffer->chars.unicode) goto memoryErrorExit;
                                buffer->shouldFreeChars = YES;
                            } else {
                                buffer->chars.unicode = (UTF16Char *)buffer->localBuffer;
                            }
                        }
                        result = (Encoding::UniCharFromUTF32(src, limit - src, buffer->chars.unicode, (strictUTF32 ? NO : YES), __RS_BIG_ENDIAN__ ? !swap : swap) ? YES : NO);
                    }
                } else if (String::Encoding::UTF8 == encoding) {
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
                    if (buffer->isASCII) {
                        buffer->numChars = len;
                        buffer->shouldFreeChars = !buffer->chars.ascii && (len <= MAX_LOCAL_CHA) ? NO : YES;
                        buffer->chars.ascii = (buffer->chars.ascii ? buffer->chars.ascii : (len <= MAX_LOCAL_CHA) ? (uint8_t *)buffer->localBuffer : buffer->allocator->Allocate<UInt8>(len));
                        if (!buffer->chars.ascii) goto memoryErrorExit;
                        memmove(buffer->chars.ascii, chars, len * sizeof(uint8_t));
                    } else {
                        Index numDone;
                        static Encoding::StringEncodingToUnicodeProc __FromUTF8 = nil;
                        
                        if (!__FromUTF8) {
                            const Encoding::StringEncodingConverter *converter = Encoding::StringEncodingGetConverter(String::Encoding::UTF8);
                            __FromUTF8 = (Encoding::StringEncodingToUnicodeProc)converter->toUnicode;
                        }
                        
                        buffer->shouldFreeChars = !buffer->chars.unicode && (len <= MAX_LOCAL_UNICHA) ? NO : YES;
                        buffer->chars.unicode = (buffer->chars.unicode ? buffer->chars.unicode : (len <= MAX_LOCAL_UNICHA) ? (UniChar *)buffer->localBuffer : buffer->allocator->Allocate<UniChar>(len));
                        if (!buffer->chars.unicode) goto memoryErrorExit;
                        buffer->numChars = 0;
                        while (chars < end) {
                            numDone = 0;
                            chars += __FromUTF8(converterFlags, chars, end - chars, &(buffer->chars.unicode[buffer->numChars]), len - buffer->numChars, &numDone);
                            
                            if (0 == numDone)
                            {
                                result = NO;
                                break;
                            }
                            buffer->numChars += numDone;
                        }
                    }
                } else if (String::Encoding::NonLossyASCII == encoding) {
                    UTF16Char currentValue = 0;
                    uint8_t character;
                    StringEncodeDecode::CodingMode mode = StringEncodeDecode::CodingMode::NonLossyASCIIMode;
                    
                    buffer->isASCII = NO;
                    buffer->shouldFreeChars = !buffer->chars.unicode && (len <= MAX_LOCAL_UNICHA) ? NO : YES;
                    buffer->chars.unicode = (buffer->chars.unicode ? buffer->chars.unicode : (len <= MAX_LOCAL_UNICHA) ? (UniChar *)buffer->localBuffer : (UniChar *)buffer->allocator->Allocate<UniChar>(len));
                    if (!buffer->chars.unicode) goto memoryErrorExit;
                    buffer->numChars = 0;
                    
                    while (chars < end) {
                        character = (*chars++);
                        
                        switch (mode) {
                            case CodingMode::NonLossyASCIIMode:
                                if (character == '\\') {
                                    mode = CodingMode::NonLossyBackslashMode;
                                } else if (character < 0x80) {
                                    currentValue = character;
                                } else {
                                    mode = CodingMode::NonLossyErrorMode;
                                }
                                break;
                                
                            case CodingMode::NonLossyBackslashMode:
                                if ((character == 'U') || (character == 'u')) {
                                    mode = CodingMode::NonLossyHexInitialMode;
                                    currentValue = 0;
                                } else if ((character >= '0') && (character <= '9')) {
                                    mode = CodingMode::NonLossyOctalInitialMode;
                                    currentValue = character - '0';
                                } else if (character == '\\') {
                                    mode = CodingMode::NonLossyASCIIMode;
                                    currentValue = character;
                                } else {
                                    mode = CodingMode::NonLossyErrorMode;
                                }
                                break;
                                
                            default:
                                if (mode < CodingMode::NonLossyHexFinalMode) {
                                    if ((character >= '0') && (character <= '9')) {
                                        currentValue = (currentValue << 4) | (character - '0');
                                        Index mod = Index(mode);
                                        if (CodingMode(++mod) == CodingMode::NonLossyHexFinalMode) mode = CodingMode::NonLossyASCIIMode;
                                        else mode = CodingMode(mod);
                                    } else {
                                        if (character >= 'a') character -= ('a' - 'A');
                                        if ((character >= 'A') && (character <= 'F')) {
                                            currentValue = (currentValue << 4) | ((character - 'A') + 10);
                                            Index mod = Index(mode);
                                            if (CodingMode(++mod) == CodingMode::NonLossyHexFinalMode) mode = CodingMode::NonLossyASCIIMode;
                                            else mode = CodingMode(mod);
                                        } else {
                                            mode = CodingMode::NonLossyErrorMode;
                                        }
                                    }
                                } else {
                                    if ((character >= '0') && (character <= '9')) {
                                        currentValue = (currentValue << 3) | (character - '0');
                                        Index mod = Index(mode);
                                        if (CodingMode(++mod) == CodingMode::NonLossyOctalFinalMode) mode = CodingMode::NonLossyASCIIMode;
                                        else mode = CodingMode(mod);
                                    } else {
                                        mode = CodingMode::NonLossyErrorMode;
                                    }
                                }
                                break;
                        }
                        
                        if (mode == CodingMode::NonLossyASCIIMode) {
                            buffer->chars.unicode[buffer->numChars++] = currentValue;
                        } else if (mode == CodingMode::NonLossyErrorMode) {
                            break;
                        }
                    }
                    result = ((mode == CodingMode::NonLossyASCIIMode) ? YES : NO);
                } else {
                    const Encoding::StringEncodingConverter *converter = Encoding::StringEncodingGetConverter(encoding);
                    
                    if (!converter) return NO;
                    
                    bool isASCIISuperset = StringPrivate::_IsSupersetOfASCII(encoding);
                    
                    if (!isASCIISuperset) buffer->isASCII = NO;
                    
                    if (buffer->isASCII) {
                        for (idx = 0; idx < len; idx++) {
                            if (128 <= chars[idx]) {
                                buffer->isASCII = NO;
                                break;
                            }
                        }
                    }
                    
                    if (converter->encodingClass == Encoding::StringEncodingConverterCheapEightBit) {
                        if (buffer->isASCII) {
                            buffer->numChars = len;
                            buffer->shouldFreeChars = !buffer->chars.ascii && (len <= MAX_LOCAL_CHA) ? NO : YES;
                            buffer->chars.ascii = (buffer->chars.ascii ? buffer->chars.ascii : (len <= MAX_LOCAL_CHA) ? (uint8_t *)buffer->localBuffer : buffer->allocator->Allocate<uint8_t>(len));
                            if (!buffer->chars.ascii) goto memoryErrorExit;
                            memmove(buffer->chars.ascii, chars, len * sizeof(uint8_t));
                        } else {
                            buffer->shouldFreeChars = !buffer->chars.unicode && (len <= MAX_LOCAL_UNICHA) ? NO : YES;
                            buffer->chars.unicode = (buffer->chars.unicode ? buffer->chars.unicode : (len <= MAX_LOCAL_UNICHA) ? (UniChar *)buffer->localBuffer : buffer->allocator->Allocate<UniChar>(len));
                            if (!buffer->chars.unicode) goto memoryErrorExit;
                            buffer->numChars = len;
                            if (String::Encoding::ASCII == encoding || String::Encoding::ISOLatin1 == encoding) {
                                for (idx = 0; idx < len; idx++) buffer->chars.unicode[idx] = (UniChar)chars[idx];
                            } else {
                                for (idx = 0; idx < len; idx++) {
                                    if (chars[idx] < 0x80 && isASCIISuperset) {
                                        buffer->chars.unicode[idx] = (UniChar)chars[idx];
                                    }
                                    else if (!((Encoding::StringEncodingCheapEightBitToUnicodeProc)converter->toUnicode)(0, chars[idx], buffer->chars.unicode + idx)) {
                                        result = NO;
                                        break;
                                    }
                                }
                            }
                        }
                    } else {
                        if (buffer->isASCII) {
                            buffer->numChars = len;
                            buffer->shouldFreeChars = !buffer->chars.ascii && (len <= MAX_LOCAL_CHA) ? NO : YES;
                            buffer->chars.ascii = (buffer->chars.ascii ? buffer->chars.ascii : (len <= MAX_LOCAL_CHA) ? (uint8_t *)buffer->localBuffer : buffer->allocator->Allocate<uint8_t>(len));
                            if (!buffer->chars.ascii) goto memoryErrorExit;
                            memmove(buffer->chars.ascii, chars, len * sizeof(uint8_t));
                        } else {
                            Index guessedLength = Encoding::StringEncodingCharLengthForBytes(encoding, 0, bytes, len);
                            static UInt32 lossyFlag = (UInt32)-1;
                            
                            buffer->shouldFreeChars = !buffer->chars.unicode && (guessedLength <= MAX_LOCAL_UNICHA) ? NO : YES;
                            buffer->chars.unicode = (buffer->chars.unicode ? buffer->chars.unicode : (guessedLength <= MAX_LOCAL_UNICHA) ? (UniChar *)buffer->localBuffer : buffer->allocator->Allocate<UniChar>(guessedLength));
                            if (!buffer->chars.unicode) goto memoryErrorExit;
                            
                            if (lossyFlag == (UInt32)-1) lossyFlag = 0;
                            
                            if (Encoding::StringEncodingBytesToUnicode(encoding, lossyFlag|__GetASCIICompatibleFlag(), bytes, len, nil, buffer->chars.unicode, (guessedLength > MAX_LOCAL_UNICHA ? guessedLength : MAX_LOCAL_UNICHA), &(buffer->numChars))) result = NO;
                        }
                    }
                }
                
                if (NO == result) {
                memoryErrorExit:	// Added for <rdar://problem/6581621>, but it's not clear whether an exception would be a better option
                    result = NO;	// In case we come here from a goto
                    if (buffer->shouldFreeChars && buffer->chars.unicode) buffer->allocator->Deallocate(buffer->chars.unicode);
                    buffer->isASCII = !alwaysUnicode;
                    buffer->shouldFreeChars = NO;
                    buffer->chars.ascii = nil;
                    buffer->numChars = 0;
                }
                return result;
            }
        }
        
        String *_CreateInstanceImmutable( Allocator<String> *allocator,
                                         const void *bytes,
                                         Index numBytes,
                                         String::Encoding encoding,
                                         bool possiblyExternalFormat,
                                         bool tryToReduceUnicode,
                                         bool hasLengthByte,
                                         bool hasNullByte,
                                         bool noCopy,
                                         Allocator<String> *contentsDeallocator,
                                         UInt32 reservedFlags) {
            if (!bytes) {
                return &String::Empty;
            }
            
            String *str;
            VarWidthCharBuffer vBuf = {0};
            
            Index size = 0;
            bool useLengthByte = NO;
            bool useNullByte = NO;
            bool useInlineData = NO;
            if (allocator == nullptr) {
                allocator = &Allocator<String>::Default;
            }
            
#define ALLOCATOFREEFUNC ((Allocator<String>*)-1)
            
            if (contentsDeallocator == ALLOCATOFREEFUNC) {
                contentsDeallocator = &Allocator<String>::Default;
            } else if (contentsDeallocator == nullptr) {
                contentsDeallocator = &Allocator<String>::Default;
            }
            
            if ((numBytes == 0) && (allocator == &Allocator<String>::SystemDefault)) {
                // If we are using the system default allocator, and the string is empty, then use the empty string!
                if (noCopy) {
                    // See 2365208... This change was done after Sonata; before we didn't free the bytes at all (leak).
                    contentsDeallocator->Deallocate((void *)bytes);
                }
                return &String::Empty;
                //                    return (StringRef)Retain(_EmptyString);	// Quick exit; won't catch all empty strings, but most
            }
            vBuf.shouldFreeChars = false;
            bool stringSupportsEightBitRepresentation = encoding != String::Encoding::Unicode && StringPrivate::_CanUseEightBitStringForBytes((const uint8_t *)bytes, numBytes, encoding);
            bool stringROMShouldIgnoreNoCopy = false;
            if ((encoding == String::Encoding::Unicode && possiblyExternalFormat) ||
                (encoding != String::Encoding::Unicode && !stringSupportsEightBitRepresentation)) {
                const void *realBytes = (uint8_t *)bytes + (hasLengthByte ? 1 : 0);
                Index realNumBytes = numBytes - (hasLengthByte ? 1 : 0);
                bool usingPassedInMemory = false;
                vBuf.allocator = &Allocator<String>::SystemDefault;
                vBuf.chars.unicode = nullptr;
                if (!StringEncodeDecode::__StringDecodeByteStream3((const uint8_t *)realBytes, realNumBytes, encoding, NO, &vBuf, &usingPassedInMemory, reservedFlags)) {
                    return nullptr;
                }
                
                encoding = vBuf.isASCII ? String::Encoding::ASCII : String::Encoding::Unicode;
                stringSupportsEightBitRepresentation = vBuf.isASCII;
                if (!usingPassedInMemory) {
                    stringROMShouldIgnoreNoCopy = YES;
                    numBytes = vBuf.isASCII ? vBuf.numChars : (vBuf.numChars * sizeof(UniChar));
                    hasLengthByte = hasNullByte = NO;
                    if (noCopy && contentsDeallocator != nullptr) {
                        contentsDeallocator->Deallocate((void*)(bytes));
                    }
                    contentsDeallocator = allocator;
                    
                    if (vBuf.shouldFreeChars && (allocator == vBuf.allocator) && encoding == String::Encoding::Unicode) {
                        vBuf.shouldFreeChars = false;
                        bytes = vBuf.allocator->Reallocate<UInt8>((void*)vBuf.chars.unicode, numBytes);
                        noCopy = true;
                    } else {
                        bytes = vBuf.chars.unicode;
                        noCopy = false;
                    }
                }
            } else if (encoding == String::Encoding::Unicode && tryToReduceUnicode) {
                Index cnt = 0, len = numBytes / sizeof(UniChar);
                bool allASCII = true;
                for (cnt = 0; cnt < len; cnt ++) {
                    if (((const UniChar *)bytes)[cnt] > 127) {
                        allASCII = false;
                        break;
                    }
                }
                
                if (allASCII) {
                    uint8_t *ptr = nil, *mem = nil;
                    BOOL newHasLengthByte = StringPrivate::_CanUseLengthByte(len);
                    numBytes = (len + 1 + (newHasLengthByte ? 1 : 0) * sizeof(uint8_t));
                    
                    if (numBytes >= __VarWidthLocalBufferSize) {
                        mem = ptr = allocator->Allocate<UInt8>(numBytes);
                    } else {
                        mem = ptr = (uint8_t *)(vBuf.localBuffer);
                    }
                    
                    if (mem) {
                        hasLengthByte = newHasLengthByte;
                        hasNullByte = YES;
                        if (hasLengthByte) *ptr++ =(uint8_t)len;
                        
                        for (cnt = 0; cnt < len; cnt++) {
                            ptr[cnt] = (uint8_t)(((const UniChar *)bytes)[cnt]);
                        }
                        
                        ptr[len] = 0;
                        
                        if (noCopy && (contentsDeallocator != nullptr)) {
                            contentsDeallocator->Deallocate((void*)bytes);
                        }
                        
                        bytes = mem;
                        encoding = String::Encoding::ASCII;
                        
                        contentsDeallocator = allocator;
                        
                        noCopy = (numBytes >= __VarWidthLocalBufferSize);
                        
                        numBytes--;
                        
                        stringSupportsEightBitRepresentation = YES;
                        
                        stringROMShouldIgnoreNoCopy = YES;
                    }
                }
            }
            
            String *romResult = nullptr;
            
            if (romResult == nullptr) {
                if (noCopy) {
                    size = sizeof(void *); // Pointer to the buffer
                    if (contentsDeallocator != allocator &&
                        contentsDeallocator != nullptr) {
                        size += sizeof(void *); // the content deallocator
                    }
                    if (!hasLengthByte) {
                        size += sizeof(Index); // explicit
                    }
                    
                    useLengthByte = hasLengthByte;
                    useNullByte = hasNullByte;
                } else {
                    useInlineData = YES;
                    size = numBytes;
                    
                    if (hasLengthByte || (encoding != String::Encoding::Unicode && StringPrivate::_CanUseLengthByte(numBytes))) {
                        useLengthByte = YES;
                        if (!hasLengthByte) size += 1;
                    } else {
                        size += sizeof(Index);
                    }
                    
                    if (hasNullByte || encoding != String::Encoding::Unicode) {
                        useNullByte = YES;
                        size += 1;
                    }
                }
                
                str = allocator->Allocate<String>(size + sizeof(UInt32));
                if (nullptr != str) {
                    OptionFlags allocBits = (contentsDeallocator == allocator ? String::NotInlineContentsDefaultFree : (contentsDeallocator == nullptr ? String::NotInlineContentsNoFree : String::NotInlineContentsCustomFree));
                    UInt32 info = (UInt32)(useInlineData ? String::HasInlineContents : allocBits) |
                     ((encoding == String::Encoding::Unicode) ? String::IsUnicode : 0) |
                     (useNullByte ? String::HasNullByte : 0) |
                     (useLengthByte ? String::HasLengthByte : 0);
                    str->_SetInfo(info);
                    if (!useLengthByte) {
                        Index length = numBytes - (hasLengthByte ? 1 : 0);
                        if (encoding == String::Encoding::Unicode) length /= sizeof(UniChar);
                        str->_SetExplicitLength(length);
                    }
                    
                    if (useInlineData) {
                        UInt8 *contents = (UInt8 *)str->_Contents();
                        if (useLengthByte && !hasLengthByte) {
                            *contents++ = (UInt8)numBytes;
                        }
                        memmove(contents, bytes, numBytes);
                        if (useNullByte) contents[numBytes] = 0;
                    } else {
                        str->_SetContentPtr(bytes);
                        if (str->_HasContentsDeallocator()) {
                            str->_SetContentsDeallocator(contentsDeallocator);
                        }
                    }
                } else {
                    if (noCopy && (contentsDeallocator != nullptr)) {
                        contentsDeallocator->Deallocate((void *)bytes);
                    }
                }
            }
            if (vBuf.shouldFreeChars) {
                vBuf.allocator->Deallocate((void *)bytes);
            }
            return str;
        }
        
        String::String(const char *cStr) {
//            bool isASCII = true;
//            // Given this code path is rarer these days, OK to do this extra work to verify the strings
//            const char *tmp = cStr;
//            while (*tmp) {
//                if (*(tmp++) & 0x80) {
//                    isASCII = false;
//                    break;
//                }
//            }
//            
//            if (!isASCII) {
//                
//            } else {
//                
//            }
        }
        
        void String::_DeallocateMutableContents(void *buffer) {
            auto alloc = _HasContentsAllocator() ? _ContentsAllocator() : &Allocator<String>::SystemDefault;
            
            if (_IsMutable() && _HasContentsAllocator() && alloc->IsGC()) {
                // do nothing
            } else if (alloc->IsGC()) {
                // GC:  for finalization safety, let collector reclaim the buffer in the next GC cycle.
                //        auto_zone_release(objc_collectableZone(), buffer);
            } else {
                alloc->Deallocate(buffer);
            }
        }
        
        String::~String() {
            
            // If in DEBUG mode, check to see if the string a RSSTR, and complain.
            //    RSAssert1(__RSConstantStringTableBeingFreed || !__RSStrIsConstantString((RSStringRef)RS), __RSLogAssertion, "Tried to deallocate RSSTR(\"%R\")", str);
            //    if (!__RSConstantStringTableBeingFreed)
            //    {
            //        if (!(__RSRuntimeInstanceIsStackValue(str) || __RSRuntimeInstanceIsSpecial(str)) && __RSStrIsConstantString(str))
            //            __RSCLog(RSLogLevelNotice, "constant string deallocate\n");
            //    }
            if (!_IsInline()) {
                uint8_t *contents;
                bool isMutable = _IsMutable();
                if (_IsFreeContentsWhenDone() && (contents = (uint8_t *)_Contents())) {
                    if (isMutable) {
//                        _DeallocateMutableContents(contents);
                    } else {
                        if (_HasContentsDeallocator()) {
                            Allocator<String> *allocator = _ContentsDeallocator();
                            allocator->Deallocate(contents);
//                            RSAllocatorDeallocate(allocator, contents);
//                            RSRelease(allocator);
                        } else {
                            Allocator<String> *allocator = &Allocator<String>::SystemDefault;
//                            RSAllocatorRef alloc = __RSGetAllocator();
//                            RSAllocatorDeallocate(alloc, contents);
                            allocator->Deallocate(contents);
                        }
                    }
                }
                if (isMutable && _HasContentsAllocator()) {
                    auto allocator __unused = _ContentsAllocator();
                    
                    //            if (!(RSAllocatorSystemDefaultGCRefZero == allocator || RSAllocatorDefaultGCRefZero == allocator))
//                    RSRelease(allocator);
                }
            }
        }
        
        const String *String::Create() {
            return &String::Empty;
        }
        
        const String *String::Create(const char * cStr, String::Encoding encoding) {
            if (!cStr) return Create();
            Index len = strlen(cStr);
            Allocator<String> &allocator = Allocator<String>::SystemDefault;
            return _CreateInstanceImmutable(&allocator, cStr, len, encoding, false, false, false, true, false, nullptr, 0);
        }
        
        String::Encoding String::GetSystemEncoding() {
            return String::UTF8;
        }
        
        const String *String::Copy() const {
            if (GetLength() == 0) {
                return &Empty;
            }
            
            if (_IsMutable() && (_IsInline() || _IsFreeContentsWhenDone() || _IsConstant())) {
                return this;
            }
            if (_IsEightBit()) {
                const UInt8 *contents = (const UInt8 *)_Contents();
                return _CreateInstanceImmutable(nullptr, contents + _SkipAnyLengthByte(), _Length2(contents), StringPrivate::_GetEightBitStringEncoding(), false, false, false, false, false, nullptr, 0);
            } else {
                const UniChar *contents = (const UniChar *)_Contents();
                return _CreateInstanceImmutable(nullptr, contents, _Length2(contents) * sizeof(UniChar), UTF8, false, true, _HasLengthByte(), false, false, nullptr, 0);
            }
            return this;
        }
        
        Index String::GetLength() const {
            return _Length();
        }
        
#define HashEverythingLimit 96
        
#define HashNextFourUniChars(accessStart, accessEnd, pointer) \
{result = result * 67503105 + (accessStart 0 accessEnd) * 16974593  + (accessStart 1 accessEnd) * 66049  + (accessStart 2 accessEnd) * 257 + (accessStart 3 accessEnd); pointer += 4;}
        
#define HashNextUniChar(accessStart, accessEnd, pointer) \
{result = result * 257 + (accessStart 0 accessEnd); pointer++;}
        inline HashCode _HashCharacters(const UniChar *uContents, Index len, Index actualLen) {
            HashCode result = actualLen;
            if (len <= HashEverythingLimit)
            {
                const UniChar *end4 = uContents + (len & ~3);
                const UniChar *end = uContents + len;
                while (uContents < end4) HashNextFourUniChars(uContents[, ], uContents); 	// First count in fours
                while (uContents < end) HashNextUniChar(uContents[, ], uContents);		// Then for the last <4 chars, count in ones...
            } else {
                const UniChar *contents, *end;
                contents = uContents;
                end = contents + 32;
                while (contents < end) HashNextFourUniChars(contents[, ], contents);
                contents = uContents + (len >> 1) - 16;
                end = contents + 32;
                while (contents < end) HashNextFourUniChars(contents[, ], contents);
                end = uContents + len;
                contents = end - 32;
                while (contents < end) HashNextFourUniChars(contents[, ], contents);
            }
            return result + (result << (actualLen & 31));
        }
        
        extern "C" bool (*__RSCharToUniCharFunc)(UInt32 flags, uint8_t ch, UniChar *unicodeChar);
        extern "C" UniChar __RSCharToUniCharTable[256];
        inline HashCode _HashEightBit(const UInt8 *cContents, Index len) {
#if defined(DEBUG)
            if (!__RSCharToUniCharFunc) {	// A little sanity verification: If this is not set, trying to hash high byte chars would be a bad idea
                Index cnt;
                BOOL err = NO;
                if (len <= HashEverythingLimit) {
                    for (cnt = 0; cnt < len; cnt++) if (cContents[cnt] >= 128) err = YES;
                } else {
                    for (cnt = 0; cnt < 32; cnt++) if (cContents[cnt] >= 128) err = YES;
                    for (cnt = (len >> 1) - 16; cnt < (len >> 1) + 16; cnt++) if (cContents[cnt] >= 128) err = YES;
                    for (cnt = (len - 32); cnt < len; cnt++) if (cContents[cnt] >= 128) err = YES;
                }
                if (err) {
                    // Can't do log here, as it might be too early
                    fprintf(stderr, "Warning: String::Hash() attempting to hash RSFoundation::Collection::String containing high bytes before properly initialized to do so\n");
                }
            }
#endif
            HashCode result = len;
            if (len <= HashEverythingLimit) {
                const uint8_t *end4 = cContents + (len & ~3);
                const uint8_t *end = cContents + len;
                while (cContents < end4) HashNextFourUniChars(__RSCharToUniCharTable[cContents[, ]], cContents); 	// First count in fours
                while (cContents < end) HashNextUniChar(__RSCharToUniCharTable[cContents[, ]], cContents);		// Then for the last <4 chars, count in ones...
            } else {
                const uint8_t *contents, *end;
                contents = cContents;
                end = contents + 32;
                while (contents < end) HashNextFourUniChars(__RSCharToUniCharTable[contents[, ]], contents);
                contents = cContents + (len >> 1) - 16;
                end = contents + 32;
                while (contents < end) HashNextFourUniChars(__RSCharToUniCharTable[contents[, ]], contents);
                end = cContents + len;
                contents = end - 32;
                while (contents < end) HashNextFourUniChars(__RSCharToUniCharTable[contents[, ]], contents);
            }
            return result + (result << (len & 31));
        }
        
        HashCode String::Hash() const {
            const UInt8 *contents = (const UInt8 *)_Contents();
            Index len = _Length2(contents);
            if (_IsEightBit()) {
                contents += _SkipAnyLengthByte();
                return _HashEightBit(contents, len);
            }
            return _HashCharacters((const UniChar *)contents, len, len);
        }
        
        String String::Empty;
        
    }
}