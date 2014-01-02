//
//  RSStringInlineBuffer.h
//  RSCoreFoundation
//
//  Created by RetVal on 5/14/13.
//  Copyright (c) 2013 RetVal. All rights reserved.
//

#ifndef RSCoreFoundation_RSStringInlineBuffer_h
#define RSCoreFoundation_RSStringInlineBuffer_h


#define __RSStringInlineBufferLength 128
typedef struct {
    UniChar buffer[__RSStringInlineBufferLength];
    RSStringRef theString;
    const UniChar *directUniCharBuffer;
    const char *directCStringBuffer;
    RSRange rangeToBuffer;		/* Range in string to buffer */
    RSIndex bufferedRangeStart;		/* Start of range currently buffered (relative to rangeToBuffer.location) */
    RSIndex bufferedRangeEnd;		/* bufferedRangeStart + number of chars actually buffered */
} RSStringInlineBuffer;

#if defined(RSInline)

RSExport const char* RSStringGetCStringPtr(RSStringRef str, RSStringEncoding encoding);
RSInline void RSStringInitInlineBuffer(RSStringRef str, RSStringInlineBuffer *buf, RSRange range) {
    buf->theString = str;
    buf->rangeToBuffer = range;
    buf->directCStringBuffer = (buf->directUniCharBuffer = RSStringGetCharactersPtr(str)) ? NULL : RSStringGetCStringPtr(str, RSStringEncodingASCII);
    buf->bufferedRangeStart = buf->bufferedRangeEnd = 0;
}

RSExport void RSStringGetCharacters(RSStringRef str, RSRange range, UniChar *buffer);
RSInline UniChar RSStringGetCharacterFromInlineBuffer(RSStringInlineBuffer *buf, RSIndex idx) {
    if (buf->directUniCharBuffer) {
        if (idx < 0 || idx >= buf->rangeToBuffer.length) return 0;
        return buf->directUniCharBuffer[idx + buf->rangeToBuffer.location];
    }
    if (idx >= buf->bufferedRangeEnd || idx < buf->bufferedRangeStart) {
        if (idx < 0 || idx >= buf->rangeToBuffer.length) return 0;
        if ((buf->bufferedRangeStart = idx - 4) < 0) buf->bufferedRangeStart = 0;
        buf->bufferedRangeEnd = buf->bufferedRangeStart + __RSStringInlineBufferLength;
        if (buf->bufferedRangeEnd > buf->rangeToBuffer.length) buf->bufferedRangeEnd = buf->rangeToBuffer.length;
        RSStringGetCharacters(buf->theString, RSMakeRange(buf->rangeToBuffer.location + buf->bufferedRangeStart, buf->bufferedRangeEnd - buf->bufferedRangeStart), buf->buffer);
    }
    return buf->buffer[idx - buf->bufferedRangeStart];
}

/* Same as RSStringGetCharacterFromInlineBuffer() but returns 0xFFFF on out of bounds access
 */
RSInline UniChar __RSStringGetCharacterFromInlineBufferAux(RSStringInlineBuffer *buf, RSIndex idx) {
    if (buf->directUniCharBuffer) {
        if (idx < 0 || idx >= buf->rangeToBuffer.length) return 0xFFFF;
        return buf->directUniCharBuffer[idx + buf->rangeToBuffer.location];
    }
    if (idx >= buf->bufferedRangeEnd || idx < buf->bufferedRangeStart) {
        if (idx < 0 || idx >= buf->rangeToBuffer.length) return 0xFFFF;
        if ((buf->bufferedRangeStart = idx - 4) < 0) buf->bufferedRangeStart = 0;
        buf->bufferedRangeEnd = buf->bufferedRangeStart + __RSStringInlineBufferLength;
        if (buf->bufferedRangeEnd > buf->rangeToBuffer.length) buf->bufferedRangeEnd = buf->rangeToBuffer.length;
        RSStringGetCharacters(buf->theString, RSMakeRange(buf->rangeToBuffer.location + buf->bufferedRangeStart, buf->bufferedRangeEnd - buf->bufferedRangeStart), buf->buffer);
    }
    return buf->buffer[idx - buf->bufferedRangeStart];
}

/* Same as RSStringGetCharacterFromInlineBuffer(), but without the bounds checking (will return garbage or crash)
 */
RSInline UniChar __RSStringGetCharacterFromInlineBufferQuick(RSStringInlineBuffer *buf, RSIndex idx) {
    if (buf->directUniCharBuffer) return buf->directUniCharBuffer[idx + buf->rangeToBuffer.location];
    if (idx >= buf->bufferedRangeEnd || idx < buf->bufferedRangeStart)
    {
        if ((buf->bufferedRangeStart = idx - 4) < 0)
            buf->bufferedRangeStart = 0;
        buf->bufferedRangeEnd = buf->bufferedRangeStart + __RSStringInlineBufferLength;
        if (buf->bufferedRangeEnd > buf->rangeToBuffer.length)
            buf->bufferedRangeEnd = buf->rangeToBuffer.length;
        RSStringGetCharacters(buf->theString, RSMakeRange(buf->rangeToBuffer.location + buf->bufferedRangeStart, buf->bufferedRangeEnd - buf->bufferedRangeStart), buf->buffer);
    }
    return buf->buffer[idx - buf->bufferedRangeStart];
}

#else
/* If INLINE functions are not available, we do somewhat less powerful macros that work similarly (except be aware that the buf argument is evaluated multiple times).
 */
#define RSStringInitInlineBuffer(str, buf, range) \
do {(buf)->theString = str; (buf)->rangeToBuffer = range; (buf)->directBuffer = RSStringGetUTF8CharactersPtr(str);} while (0)

#define RSStringGetCharacterFromInlineBuffer(buf, idx) \
(((idx) < 0 || (idx) >= (buf)->rangeToBuffer.length) ? 0 : ((buf)->directBuffer ? (buf)->directBuffer[(idx) + (buf)->rangeToBuffer.location] : RSStringGetCharacterAtIndex((buf)->theString, (idx) + (buf)->rangeToBuffer.location)))

#endif /* RS_INLINE */

#endif
