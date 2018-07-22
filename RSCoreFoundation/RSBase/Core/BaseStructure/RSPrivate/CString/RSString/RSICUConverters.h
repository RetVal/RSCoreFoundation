//
//  RSICUConverters.h
//  RSCoreFoundation
//
//  Created by RetVal on 7/29/13.
//  Copyright (c) 2013 RetVal. All rights reserved.
//

#ifndef RSCoreFoundation_RSICUConverters_h
#define RSCoreFoundation_RSICUConverters_h

#include <RSCoreFoundation/RSString.h>

extern const char *__RSStringEncodingGetICUName(RSStringEncoding encoding);

extern RSStringEncoding __RSStringEncodingGetFromICUName(const char *icuName);


extern RSIndex __RSStringEncodingICUToBytes(const char *icuName, uint32_t flags, const UniChar *characters, RSIndex numChars, RSIndex *usedCharLen, uint8_t *bytes, RSIndex maxByteLen, RSIndex *usedByteLen);
extern RSIndex __RSStringEncodingICUToUnicode(const char *icuName, uint32_t flags, const uint8_t *bytes, RSIndex numBytes, RSIndex *usedByteLen, UniChar *characters, RSIndex maxCharLen, RSIndex *usedCharLen);
extern RSIndex __RSStringEncodingICUCharLength(const char *icuName, uint32_t flags, const uint8_t *bytes, RSIndex numBytes);
extern RSIndex __RSStringEncodingICUByteLength(const char *icuName, uint32_t flags, const UniChar *characters, RSIndex numChars);

// The caller is responsible for freeing the memory (use RSAllocatorDeallocate)
extern RSStringEncoding *__RSStringEncodingCreateICUEncodings(RSAllocatorRef allocator, RSIndex *numberOfIndex);

#endif
