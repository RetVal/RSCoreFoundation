//
//  RSFoundationEncoding.h
//  RSCoreFoundation
//
//  Created by RetVal on 7/28/13.
//  Copyright (c) 2013 RetVal. All rights reserved.
//

#ifndef RSCoreFoundation_RSFoundationEncoding_h
#define RSCoreFoundation_RSFoundationEncoding_h

RSExport RSIndex __RSStringEncodeByteStream(RSStringRef string, RSIndex rangeLoc, RSIndex rangeLen, BOOL generatingExternalFile, RSStringEncoding encoding, char lossByte, UInt8 *buffer, RSIndex max, RSIndex *usedBufLen);

RSExport RSStringRef __RSStringCreateImmutableFunnel2(RSAllocatorRef alloc, const void *bytes, RSIndex numBytes, RSStringEncoding encoding, BOOL possiblyExternalFormat, BOOL tryToReduceUnicode, BOOL hasLengthByte, BOOL hasNullByte, BOOL noCopy, RSAllocatorRef contentsDeallocator);

RSExport void __RSStringAppendBytes(RSMutableStringRef str, const char *cStr, RSIndex appendedLength, RSStringEncoding encoding);

RSInline BOOL __RSStringEncodingIsSupersetOfASCII(RSStringEncoding encoding) {
    switch (encoding & 0x0000FF00) {
        case 0x0: // MacOS Script range
            // Symbol & bidi encodings are not ASCII superset
            if (encoding == RSStringEncodingMacJapanese || encoding == RSStringEncodingMacArabic || encoding == RSStringEncodingMacHebrew || encoding == RSStringEncodingMacUkrainian || encoding == RSStringEncodingMacSymbol || encoding == RSStringEncodingMacDingbats) return NO;
            return YES;
            
        case 0x100: // Unicode range
            if (encoding != RSStringEncodingUTF8) return NO;
            return YES;
            
        case 0x200: // ISO range
            if (encoding == RSStringEncodingISOLatinArabic) return NO;
            return YES;
            
        case 0x600: // National standards range
            if (encoding != RSStringEncodingASCII) return NO;
            return YES;
            
        case 0x800: // ISO 2022 range
            return NO; // It's modal encoding
            
        case 0xA00: // Misc standard range
            if ((encoding == RSStringEncodingShiftJIS) || (encoding == RSStringEncodingHZ_GB_2312) || (encoding == RSStringEncodingUTF7_IMAP)) return NO;
            return YES;
            
        case 0xB00:
            if (encoding == RSStringEncodingNonLossyASCII) return NO;
            return YES;
            
        case 0xC00: // EBCDIC
            return NO;
            
        default:
            return ((encoding & 0x0000FF00) > 0x0C00 ? NO : YES);
    }
}


/* Desperately using extern here */
RSExport RSStringEncoding __RSDefaultEightBitStringEncoding;
RSExport RSStringEncoding __RSStringComputeEightBitStringEncoding(void);
RSExport RSStringEncoding RSStringGetSystemEncoding(void);
RSExport RSStringEncoding RSStringFileSystemEncoding(void);

RSInline RSStringEncoding __RSStringGetEightBitStringEncoding(void) {
    if (__RSDefaultEightBitStringEncoding == RSStringEncodingInvalidId) __RSStringComputeEightBitStringEncoding();
    return __RSDefaultEightBitStringEncoding;
}


#endif
