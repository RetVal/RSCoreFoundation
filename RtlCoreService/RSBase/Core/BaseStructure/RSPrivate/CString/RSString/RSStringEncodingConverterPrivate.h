//
//  RSStringEncodingConverterPrivate.h
//  RSCoreFoundation
//
//  Created by RetVal on 7/29/13.
//  Copyright (c) 2013 RetVal. All rights reserved.
//

#ifndef RSCoreFoundation_RSStringEncodingConverterPrivate_h
#define RSCoreFoundation_RSStringEncodingConverterPrivate_h

#if !defined(__RSCOREFOUNDATION_RSSTRINGENCODINGCONVERTERPRIV__)
#define __RSCOREFOUNDATION_RSSTRINGENCODINGCONVERTERPRIV__ 1

#include <RSCoreFoundation/RSBase.h>
#include "RSStringEncodingConverterExt.h"

extern  const RSStringEncodingConverter __RSConverterASCII;
extern  const RSStringEncodingConverter __RSConverterISOLatin1;
extern  const RSStringEncodingConverter __RSConverterMacRoman;
extern  const RSStringEncodingConverter __RSConverterWinLatin1;
extern  const RSStringEncodingConverter __RSConverterNextStepLatin;
extern  const RSStringEncodingConverter __RSConverterUTF8;

extern  RSStringEncoding *__RSStringEncodingCreateListOfAvailablePlatformConverters(RSAllocatorRef allocator, RSIndex *numberOfConverters);
extern  const RSStringEncodingConverter *__RSStringEncodingGetExternalConverter(uint32_t encoding);
extern  RSIndex __RSStringEncodingPlatformUnicodeToBytes(uint32_t encoding, uint32_t flags, const UniChar *characters, RSIndex numChars, RSIndex *usedCharLen, uint8_t *bytes, RSIndex maxByteLen, RSIndex *usedByteLen);
extern  RSIndex __RSStringEncodingPlatformBytesToUnicode(uint32_t encoding, uint32_t flags, const uint8_t *bytes, RSIndex numBytes, RSIndex *usedByteLen, UniChar *characters, RSIndex maxCharLen, RSIndex *usedCharLen);
extern  RSIndex __RSStringEncodingPlatformCharLengthForBytes(uint32_t encoding, uint32_t flags, const uint8_t *bytes, RSIndex numBytes);
extern  RSIndex __RSStringEncodingPlatformByteLengthForCharacters(uint32_t encoding, uint32_t flags, const UniChar *characters, RSIndex numChars);

#endif /* ! __RSCOREFOUNDATION_RSSTRINGENCODINGCONVERTERPRIV__ */
#endif
