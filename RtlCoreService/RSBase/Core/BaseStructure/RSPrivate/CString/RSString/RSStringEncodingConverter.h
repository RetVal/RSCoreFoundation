//
//  RSStringEncodingConverter.h
//  RSCoreFoundation
//
//  Created by RetVal on 7/29/13.
//  Copyright (c) 2013 RetVal. All rights reserved.
//

#ifndef RSCoreFoundation_RSStringEncodingConverter_h
#define RSCoreFoundation_RSStringEncodingConverter_h

#if !defined(__COREFOUNDATION_RSSTRINGENCODINGCONVERTER__)
#define __COREFOUNDATION_RSSTRINGENCODINGCONVERTER__ 1

#include <RSCoreFoundation/RSString.h>


RS_EXTERN_C_BEGIN

/* Macro to shift lossByte argument.
 */
#define RSStringEncodingLossyByteToMask(lossByte)	((uint32_t)(lossByte << 24)|RSStringEncodingAllowLossyConversion)
#define RSStringEncodingMaskToLossyByte(flags)		((uint8_t)(flags >> 24))

/* Macros for streaming support
 */
#define RSStringEncodingStreamIDMask                    (0x00FF0000)
#define RSStringEncodingStreamIDFromMask(mask)    ((mask >> 16) & 0xFF)
#define RSStringEncodingStreamIDToMask(identifier)            ((uint32_t)((identifier & 0xFF) << 16))

/* Converts characters into the specified encoding.  Returns the constants defined above.
 If maxByteLen is 0, bytes is ignored. You can pass lossyByte by passing the value in flags argument.
 i.e. RSStringEncodingUnicodeToBytes(encoding, RSStringEncodingLossyByteToMask(lossByte), ....)
 */
RSExport uint32_t RSStringEncodingUnicodeToBytes(uint32_t encoding, uint32_t flags, const UniChar *characters, RSIndex numChars, RSIndex *usedCharLen, uint8_t *bytes, RSIndex maxByteLen, RSIndex *usedByteLen);

/* Converts bytes in the specified encoding into unicode.  Returns the constants defined above.
 maxCharLen & usdCharLen are in UniChar length, not byte length.
 If maxCharLen is 0, characters is ignored.
 */
RSExport uint32_t RSStringEncodingBytesToUnicode(uint32_t encoding, uint32_t flags, const uint8_t *bytes, RSIndex numBytes, RSIndex *usedByteLen, UniChar *characters, RSIndex maxCharLen, RSIndex *usedCharLen);

/* Fallback functions used when allowLossy
 */
typedef RSIndex (*RSStringEncodingToBytesFallbackProc)(const UniChar *characters, RSIndex numChars, uint8_t *bytes, RSIndex maxByteLen, RSIndex *usedByteLen);
typedef RSIndex (*RSStringEncodingToUnicodeFallbackProc)(const uint8_t *bytes, RSIndex numBytes, UniChar *characters, RSIndex maxCharLen, RSIndex *usedCharLen);

RSExport BOOL RSStringEncodingIsValidEncoding(uint32_t encoding);

/* Returns RSStringEncodingInvalidId terminated encoding list
 */
RSExport const RSStringEncoding *RSStringEncodingListOfAvailableEncodings(void);

/* Returns required length of destination buffer for conversion.  These functions are faster than specifying 0 to maxByteLen (maxCharLen), but unnecessarily optimal length
 */
RSExport RSIndex RSStringEncodingCharLengthForBytes(uint32_t encoding, uint32_t flags, const uint8_t *bytes, RSIndex numBytes);
RSExport RSIndex RSStringEncodingByteLengthForCharacters(uint32_t encoding, uint32_t flags, const UniChar *characters, RSIndex numChars);

/* Can register functions used for lossy conversion.  Reregisters default procs if nil
 */
RSExport void RSStringEncodingRegisterFallbackProcedures(uint32_t encoding, RSStringEncodingToBytesFallbackProc toBytes, RSStringEncodingToUnicodeFallbackProc toUnicode);

RS_EXTERN_C_END

#endif /* ! __COREFOUNDATION_RSSTRINGENCODINGCONVERTER__ */



#endif
