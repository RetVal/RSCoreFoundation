//
//  RSPrivateBuffer.h
//  RSCoreFoundation
//
//  Created by RetVal on 10/30/12.
//  Copyright (c) 2012 RetVal. All rights reserved.
//

#ifndef RSCoreFoundation_RSPrivateBuffer_h
#define RSCoreFoundation_RSPrivateBuffer_h
#include <RSCoreFoundation/RSBase.h>

void __RSBufferMakeUpper(RSBuffer buffer, RSIndex length);        // make buffer capital
void __RSBufferMakeLower(RSBuffer buffer, RSIndex length);          // make buffer lower
void __RSBufferMakeCapital(RSBuffer buffer, RSIndex length);
void __RSBufferReverse(RSBuffer buffer, RSIndex start, RSIndex end);// reverse the buffer from start to end
void __RSBufferTrunLeft(RSBuffer buffer,RSIndex sizeToMove,RSIndex length); // trun buffer left.
void __RSBufferTrunRight(RSBuffer buffer,RSIndex sizeToMove,RSIndex length);// trun buffer right.
BOOL __RSBufferDeleteSubBuffer(RSBuffer str,RSCBuffer substr );     // delete substr from str (maybe not test)
BOOL __RSBufferDeleteRange(RSBuffer str, RSIndex length,  RSRange range);            // delete str in range
BOOL __RSBufferMove(RSBuffer aMutableString, RSIndex length, RSRange rangeFrom, RSRange rangeTo);
void __RSBufferPermutation(RSCBuffer dumpPath, RSBuffer* result, RSBuffer pStr); // the result should be freed by caller, or memory leaks.
BOOL __RSBufferIsPalindrome(RSCBuffer s, RSInteger n);      // check s is palindrome or not.
RSIndex __RSBufferFindSubString(RSCBuffer str1, RSCBuffer str2);    // find substring(str2) in str1
RSRange *__RSBufferFindSubStrings(RSCBuffer cStr, RSCBuffer strToFind, RSIndex *numberOfRanges);
RSIndex __RSBufferToNumber16(RSCBuffer str, RSIndex length);        // make str(HEX) to a number
RSIndex __RSBufferToNumber10(RSCBuffer str, RSIndex length);        // make str(DEC) to a number

BOOL __RSBufferContainString_low_hash(RSCBuffer str1, RSCBuffer str2);
BOOL __RSBufferContainString_high_hash(RSCBuffer str1, RSCBuffer str2);
BOOL __RSBufferContainString_low_bitmap(RSCBuffer str1, RSCBuffer str2);
BOOL __RSBufferContainString_high_bitmap(RSCBuffer str1, RSCBuffer str2);
BOOL __RSBufferMatchString_low(RSCBuffer str1, RSCBuffer str2);
BOOL __RSBufferMatchString_high(RSCBuffer str1, RSCBuffer str2);
RSInline RSIndex __RSUTF8StepMove(RSUBuffer str);
RSPrivate RSIndex __RSGetUTF8Length(RSCUBuffer str);
#endif
