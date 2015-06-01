//
//  String.cpp
//  RSCoreFoundation
//
//  Created by closure on 5/27/15.
//  Copyright (c) 2015 RetVal. All rights reserved.
//

#include <RSFoundation/String.h>
#include <string.h>

namespace RSFoundation {
    namespace Collection {
        namespace StringPrivate {
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
            typedef struct _RSStringDeferredRange {
                RSIndex beginning;
                RSIndex length;
                RSIndex shift;
            } RSStringDeferredRange;
            
            typedef struct _RSStringStackInfo {
                RSIndex capacity;		// Capacity (if capacity == count, need to realloc to add another)
                RSIndex count;			// Number of elements actually stored
                RSStringDeferredRange *stack;
                BOOL hasMalloced;	// Indicates "stack" is allocated and needs to be deallocated when done
                char _padding[3];
            } RSStringStackInfo;

            inline void pop (RSStringStackInfo *si, RSStringDeferredRange *topRange) {
                si->count = si->count - 1;
                *topRange = si->stack[si->count];
            }
            
            inline void push (RSStringStackInfo *si, const RSStringDeferredRange *newRange) {
                if (si->count == si->capacity) {
                    // increase size of the stack
                    si->capacity = (si->capacity + 4) * 2;
                    if (si->hasMalloced) {
                        si->stack = (RSStringDeferredRange *)realloc(si->stack, si->capacity * sizeof(RSStringDeferredRange));
//                        si->stack = (RSStringDeferredRange *)RSAllocatorReallocate(RSAllocatorSystemDefault, si->stack, si->capacity * sizeof(RSStringDeferredRange));
                    } else {
                        RSStringDeferredRange *newStack = (RSStringDeferredRange *)malloc(si->capacity * sizeof(RSStringDeferredRange));
                        memmove(newStack, si->stack, si->count * sizeof(RSStringDeferredRange));
                        si->stack = newStack;
                        si->hasMalloced = YES;
                    }
                }
                si->stack[si->count] = *newRange;
                si->count = si->count + 1;
            }

            static void rearrangeBlocks(uint8_t *buffer,
                                        RSIndex numBlocks,
                                        RSIndex blockSize,
                                        const RSRange *ranges,
                                        RSIndex numRanges,
                                        RSIndex insertLength)
            {
                
#define origStackSize 10
                RSStringDeferredRange origStack[origStackSize];
                RSStringStackInfo si = {origStackSize, 0, origStack, NO, {0, 0, 0}};
                RSStringDeferredRange currentNonRange = {0, 0, 0};
                RSIndex currentRange = 0;
                RSIndex amountShifted = 0;
                
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
                                   RSIndex srcLength,
                                   BOOL srcIsUnicode,
                                   BOOL dstIsUnicode,
                                   const RSRange *ranges,
                                   RSIndex numRanges,
                                   RSIndex insertLength) {
                RSIndex srcLocationInBytes = 0;	// in order to avoid multiplying all the time, this is in terms of bytes, not blocks
                RSIndex dstLocationInBytes = 0;	// ditto
                RSIndex srcBlockSize = srcIsUnicode ? sizeof(UniChar) : sizeof(uint8_t);
                RSIndex insertLengthInBytes = insertLength * (dstIsUnicode ? sizeof(UniChar) : sizeof(uint8_t));
                RSIndex rangeIndex = 0;
                RSIndex srcToDstMultiplier = (srcIsUnicode == dstIsUnicode) ? 1 : (sizeof(UniChar) / sizeof(uint8_t));
                
                // Loop over the ranges, copying the range to be preserved (right before each range)
                while (rangeIndex < numRanges) {
                    RSIndex srcLengthInBytes = ranges[rangeIndex].location * srcBlockSize - srcLocationInBytes;	// srcLengthInBytes is in terms of bytes, not blocks; represents length of region to be preserved
                    if (srcLengthInBytes > 0) {
                        if (srcIsUnicode == dstIsUnicode) {
                            memmove(dstBuffer + dstLocationInBytes, srcBuffer + srcLocationInBytes, srcLengthInBytes);
                        } else {
//                            __RSStrConvertBytesToUnicode(srcBuffer + srcLocationInBytes, (UniChar *)(dstBuffer + dstLocationInBytes), srcLengthInBytes);
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
//                        __RSStrConvertBytesToUnicode(srcBuffer + srcLocationInBytes, (UniChar *)(dstBuffer + dstLocationInBytes), srcLength * srcBlockSize - srcLocationInBytes);
                    }
                }
            }

            void _HandleOutOfMemory() {
                String msg("Out of memory. We suggest restarting the application. If you have an unsaved document, create a backup copy in Finder, then try to save."); {
//                    String("%r"), msg);
                }
            }
            
            enum {
                __RSVarWidthLocalBufferSize = 1008
            };
            
            typedef struct {      /* A simple struct to maintain ASCII/Unicode versions of the same buffer. */
                union {
                    UInt8 *ascii;
                    UniChar *unicode;
                } chars;
                BOOL isASCII;	/* This really does mean 7-bit ASCII, not _NSDefaultCStringEncoding() */
                BOOL shouldFreeChars;	/* If the number of bytes exceeds __RSVarWidthLocalBufferSize, bytes are allocated */
                BOOL _unused1;
                BOOL _unused2;
                Allocator<String> allocator;	/* Use this allocator to allocate, reallocate, and deallocate the bytes */
                RSIndex numChars;	/* This is in terms of ascii or unicode; that is, if isASCII, it is number of 7-bit chars; otherwise it is number of UniChars; note that the actual allocated space might be larger */
                UInt8 localBuffer[__RSVarWidthLocalBufferSize];	/* private; 168 ISO2022JP chars, 504 Unicode chars, 1008 ASCII chars */
            } RSVarWidthCharBuffer;
            enum {_RSStringErrNone = 0, _RSStringErrNotMutable = 1, _RSStringErrNilArg = 2, _RSStringErrBounds = 3};
            
            String &&_CreateInstanceImmutable(Allocator<String> *allocator,
                                              const void *bytes,
                                              RSIndex numBytes,
                                              String::Encoding encoding,
                                              bool possiblyExternalFormat,
                                              bool tryToReduceUnicode,
                                              bool hasLengthByte,
                                              bool hasNullByte,
                                              bool noCopy,
                                              Allocator<String> *contentsDeallocator,
                                              RSUInt32 reservedFlags) {
                if (!bytes) {
                    return MoveValue(String(""));
                }
                
                String str;
                RSVarWidthCharBuffer vBuf = {0};
                
                RSIndex size = 0;
                bool useLengthByte = NO;
                bool useNullByte = NO;
                bool useInlineData = NO;
                if (allocator == nullptr) {
                    allocator = &Allocator<String>::AllocatorDefault;
                }
                
#define ALLOCATORSFREEFUNC ((Allocator<String>*)-1)
                
                if (contentsDeallocator == ALLOCATORSFREEFUNC) {
                    contentsDeallocator = &Allocator<String>::AllocatorDefault;
                } else if (contentsDeallocator == nullptr) {
                    contentsDeallocator = &Allocator<String>::AllocatorDefault;
                }
                
                if ((numBytes == 0) && (allocator == &Allocator<String>::AllocatorSystemDefault)) {
                    // If we are using the system default allocator, and the string is empty, then use the empty string!
                    if (noCopy) {
                        // See 2365208... This change was done after Sonata; before we didn't free the bytes at all (leak).
                        contentsDeallocator->Deallocate((void *)bytes);
                    }
//                    return (RSStringRef)RSRetain(_RSEmptyString);	// Quick exit; won't catch all empty strings, but most
                }
                return MoveValue(str);
            }
        }
        
        String::String(const char *cStr) {
            bool isASCII = true;
            // Given this code path is rarer these days, OK to do this extra work to verify the strings
            const char *tmp = cStr;
            while (*tmp) {
                if (*(tmp++) & 0x80) {
                    isASCII = false;
                    break;
                }
            }
            
            if (!isASCII) {
                
            } else {
                
            }
        }
    }
}