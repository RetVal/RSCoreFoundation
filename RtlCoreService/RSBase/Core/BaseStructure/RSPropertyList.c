//
//  RSPropertyList.c
//  RSCoreFoundation
//
//  Created by RetVal on 12/13/12.
//  Copyright (c) 2012 RetVal. All rights reserved.
//

#include <RSCoreFoundation/RSBinaryPropertyList.h>
#include <RSCoreFoundation/RSPropertyList.h>
#include <RSCoreFoundation/RSRuntime.h>
#include <RSCoreFoundation/RSSet.h>
#include <RSCoreFoundation/RSData+Extension.h>
#include <dispatch/dispatch.h>

#define PLIST_IX    0
#define ARRAY_IX    1
#define DICT_IX     2
#define KEY_IX      3
#define STRING_IX   4
#define DATA_IX     5
#define DATE_IX     6
#define REAL_IX     7
#define INTEGER_IX  8
#define TRUE_IX     9
#define FALSE_IX    10
#define DOCTYPE_IX  11
#define CDSECT_IX   12

#define PLIST_TAG_LENGTH	5
#define ARRAY_TAG_LENGTH	5
#define DICT_TAG_LENGTH		4
#define KEY_TAG_LENGTH		3
#define STRING_TAG_LENGTH	6
#define DATA_TAG_LENGTH		4
#define DATE_TAG_LENGTH		4
#define REAL_TAG_LENGTH		4
#define INTEGER_TAG_LENGTH	7
#define TRUE_TAG_LENGTH		4
#define FALSE_TAG_LENGTH	5
#define DOCTYPE_TAG_LENGTH	7
#define CDSECT_TAG_LENGTH	9

static const RSBlock RSXMLPlistTags[13][10]= {
    {'p', 'l', 'i', 's', 't',   '\0', '\0', '\0', '\0', '\0'},
    {'a', 'r', 'r', 'a', 'y',   '\0', '\0', '\0', '\0', '\0'},
    {'d', 'i', 'c', 't',  '\0', '\0', '\0', '\0', '\0', '\0'},
    {'k', 'e', 'y', '\0', '\0', '\0', '\0', '\0', '\0', '\0'},
    {'s', 't', 'r', 'i', 'n', 'g',    '\0', '\0', '\0', '\0'},
    {'d', 'a', 't', 'a',  '\0', '\0', '\0', '\0', '\0', '\0'},
    {'d', 'a', 't', 'e',  '\0', '\0', '\0', '\0', '\0', '\0'},
    {'r', 'e', 'a', 'l',  '\0', '\0', '\0', '\0', '\0', '\0'},
    {'i', 'n', 't', 'e', 'g', 'e', 'r',     '\0', '\0', '\0'},
    {'t', 'r', 'u', 'e',  '\0', '\0', '\0', '\0', '\0', '\0'},
    {'f', 'a', 'l', 's', 'e',   '\0', '\0', '\0', '\0', '\0'},
    {'D', 'O', 'C', 'T', 'Y', 'P', 'E',     '\0', '\0', '\0'},
    {'<', '!', '[', 'C', 'D', 'A', 'T', 'A', '[',       '\0'}
};

static const UniChar RSXMLPlistTagsUnicode[13][10]= {
    {'p', 'l', 'i', 's', 't',   '\0', '\0', '\0', '\0', '\0'},
    {'a', 'r', 'r', 'a', 'y',   '\0', '\0', '\0', '\0', '\0'},
    {'d', 'i', 'c', 't',  '\0', '\0', '\0', '\0', '\0', '\0'},
    {'k', 'e', 'y', '\0', '\0', '\0', '\0', '\0', '\0', '\0'},
    {'s', 't', 'r', 'i', 'n', 'g',    '\0', '\0', '\0', '\0'},
    {'d', 'a', 't', 'a',  '\0', '\0', '\0', '\0', '\0', '\0'},
    {'d', 'a', 't', 'e',  '\0', '\0', '\0', '\0', '\0', '\0'},
    {'r', 'e', 'a', 'l',  '\0', '\0', '\0', '\0', '\0', '\0'},
    {'i', 'n', 't', 'e', 'g', 'e', 'r',     '\0', '\0', '\0'},
    {'t', 'r', 'u', 'e',  '\0', '\0', '\0', '\0', '\0', '\0'},
    {'f', 'a', 'l', 's', 'e',   '\0', '\0', '\0', '\0', '\0'},
    {'D', 'O', 'C', 'T', 'Y', 'P', 'E',     '\0', '\0', '\0'},
    {'<', '!', '[', 'C', 'D', 'A', 'T', 'A', '[',       '\0'}
};

#pragma mark -
#pragma mark RSPropertyList Private Core API
static RSTypeID stringtype, datatype, numbertype, datetype;
static RSTypeID booltype __unused, nulltype, dicttype, arraytype, settype;

RSPrivate void __RSPropertyListInitializeInitStatics() {
    static dispatch_once_t once;
    dispatch_once(&once, ^{
        stringtype = RSStringGetTypeID();
        datatype = RSDataGetTypeID();
        numbertype = RSNumberGetTypeID();
        //booltype = RSBooleanGetTypeID();
        datetype = RSDateGetTypeID();
        dicttype = RSDictionaryGetTypeID();
        arraytype = RSArrayGetTypeID();
        settype = RSSetGetTypeID();
        nulltype = RSNilGetTypeID();
    });
}

static void _plistAppendUTF8CString(RSMutableDataRef mData, const char *cString)
{
    RSDataAppendBytes (mData, (const RSBitU8 *)cString, strlen(cString));
}

static void _plistAppendCharacters(RSMutableDataRef xmlData, const UniChar* characters, RSIndex length)
{
    RSIndex curLoc = 0;
    
    do
    {
        // Flush out ASCII chars, BUFLEN at a time
#define BUFLEN 400
        UInt8 buf[BUFLEN], *bufPtr = buf;
        RSIndex cnt = 0;
        while (cnt < length && (cnt - curLoc < BUFLEN) && (characters[cnt] < 128)) *bufPtr++ = (UInt8)(characters[cnt++]);
        if (cnt > curLoc)
        {	// Flush any ASCII bytes
            RSDataAppendBytes(xmlData, buf, cnt - curLoc);
            curLoc = cnt;
        }
    } while (curLoc < length && (characters[curLoc] < 128));	// We will exit out of here when we run out of chars or hit a non-ASCII char
    
    if (curLoc < length) {	// Now deal with non-ASCII chars
        RSDataRef data = nil;
        RSStringRef str = nil;
        if ((str = RSStringCreateWithCharactersNoCopy(RSAllocatorSystemDefault, characters + curLoc, length - curLoc, RSAllocatorNull))) {
            if ((data = RSStringCreateExternalRepresentation(RSAllocatorSystemDefault, str, RSStringEncodingUTF8, 0))) {
                RSDataAppendBytes (xmlData, RSDataGetBytesPtr(data), RSDataGetLength(data));
                RSRelease(data);
            }
            RSRelease(str);
        }
        RSAssert1(str && data, __RSLogAssertion, "%s(): Error writing plist", __PRETTY_FUNCTION__);
    }

}

static void _plistAppendString(RSMutableDataRef xmlData, RSStringRef append)
{
    const UniChar *chars;
    const char *cStr;
    RSDataRef data;
    if ((chars = RSStringGetCharactersPtr(append)))
    {
        _plistAppendCharacters(xmlData, chars, RSStringGetLength(append));
    }
    else if ((cStr = RSStringGetCStringPtr(append, RSStringEncodingASCII)) || (cStr = RSStringGetCStringPtr(append, __RSDefaultEightBitStringEncoding)))
    {
        _plistAppendUTF8CString(xmlData, cStr);
    }
    else if ((data = RSStringCreateExternalRepresentation(RSAllocatorSystemDefault, append, RSStringEncodingUTF8, 0)))
    {
        RSDataAppendBytes (xmlData, RSDataGetBytesPtr(data), RSDataGetLength(data));
        RSRelease(data);
    }
    else
    {
        RSAssert1(TRUE, __RSLogAssertion, "%s(): Error in plist writing", __PRETTY_FUNCTION__);
    }
}

static void _plistAppendFormat(RSMutableDataRef xmlData, RSStringRef format, ...)
{
    va_list list;
    va_start(list, format);
    RSStringRef append = RSStringCreateWithFormatAndArguments(RSAllocatorSystemDefault, 128, format, list);
    va_end(list);
    _plistAppendString(xmlData, append);
    RSRelease(append);
}

static void _appendIndents(RSIndex numIndents, RSMutableDataRef str)
{
#define NUMTABS 4
    static const UniChar tabs[NUMTABS] = {'\t','\t','\t','\t'};
    for (; numIndents > 0; numIndents -= NUMTABS) _plistAppendCharacters(str, tabs, (numIndents >= NUMTABS) ? NUMTABS : numIndents);
}

static void _XMLPlistAppendDataUsingBase64(RSMutableDataRef mData, RSDataRef inputData, RSIndex indent) {
    static const char __RSPLDataEncodeTable[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
#define MAXLINELEN 76
    char buf[MAXLINELEN + 4 + 2];	// For the slop and carriage return and terminating nil
    
    const uint8_t *bytes = RSDataGetBytesPtr(inputData);
    RSIndex length = RSDataGetLength(inputData);
    RSIndex i, pos;
    const uint8_t *p;
    
    if (indent > 8) indent = 8; // refuse to indent more than 64 characters
    
    pos = 0;		// position within buf
    
    for (i = 0, p = bytes; i < length; i++, p++) {
        /* 3 bytes are encoded as 4 */
        switch (i % 3) {
            case 0:
                buf[pos++] = __RSPLDataEncodeTable [ ((p[0] >> 2) & 0x3f)];
                break;
            case 1:
                buf[pos++] = __RSPLDataEncodeTable [ ((((p[-1] << 8) | p[0]) >> 4) & 0x3f)];
                break;
            case 2:
                buf[pos++] = __RSPLDataEncodeTable [ ((((p[-1] << 8) | p[0]) >> 6) & 0x3f)];
                buf[pos++] = __RSPLDataEncodeTable [ (p[0] & 0x3f)];
                break;
        }
        /* Flush the line out every 76 (or fewer) chars --- indents count against the line length*/
        if (pos >= MAXLINELEN - 8 * indent) {
            buf[pos++] = '\n';
            buf[pos++] = 0;
            _appendIndents(indent, mData);
            _plistAppendUTF8CString(mData, buf);
            pos = 0;
        }
    }
    
    switch (i % 3) {
        case 0:
            break;
        case 1:
            buf[pos++] = __RSPLDataEncodeTable [ ((p[-1] << 4) & 0x30)];
            buf[pos++] = '=';
            buf[pos++] = '=';
            break;
        case 2:
            buf[pos++] =  __RSPLDataEncodeTable [ ((p[-1] << 2) & 0x3c)];
            buf[pos++] = '=';
            break;
    }
    
    if (pos > 0) {
        buf[pos++] = '\n';
        buf[pos++] = 0;
        _appendIndents(indent, mData);
        _plistAppendUTF8CString(mData, buf);
    }
}

static RSCode __RSPropertyListTagID(RSStringRef tag)
{
    const UniChar* buf = (const UniChar*)RSStringGetCStringPtr(tag, RSStringEncodingUnicode);
    if (0 == __builtin_memcmp(buf, RSXMLPlistTagsUnicode[KEY_IX], KEY_TAG_LENGTH))
    {
        return KEY_IX;
    }
    if (0 == __builtin_memcmp(buf, RSXMLPlistTagsUnicode[STRING_IX], STRING_TAG_LENGTH))
    {
        return STRING_IX;
    }
    if (0 == __builtin_memcmp(buf, RSXMLPlistTagsUnicode[DICT_IX], DICT_TAG_LENGTH))
    {
        return DICT_IX;
    }
    if (0 == __builtin_memcmp(buf, RSXMLPlistTagsUnicode[ARRAY_IX], ARRAY_TAG_LENGTH))
    {
        return ARRAY_IX;
    }
    if (0 == __builtin_memcmp(buf, RSXMLPlistTagsUnicode[DATA_IX], DATA_TAG_LENGTH))
    {
        return DATA_IX;
    }
    if (0 == __builtin_memcmp(buf, RSXMLPlistTagsUnicode[DATE_IX], DATE_TAG_LENGTH))
    {
        return DATE_IX;
    }
    if (0 == __builtin_memcmp(buf, RSXMLPlistTagsUnicode[TRUE_IX], TRUE_TAG_LENGTH))
    {
        return TRUE_IX;
    }
    if (0 == __builtin_memcmp(buf, RSXMLPlistTagsUnicode[FALSE_IX], FALSE_TAG_LENGTH))
    {
        return FALSE_IX;
    }
    if (0 == __builtin_memcmp(buf, RSXMLPlistTagsUnicode[INTEGER_IX], INTEGER_TAG_LENGTH))
    {
        return INTEGER_IX;
    }
    if (0 == __builtin_memcmp(buf, RSXMLPlistTagsUnicode[REAL_IX], REAL_TAG_LENGTH))
    {
        return REAL_IX;
    }
    
    if (0 == __builtin_memcmp(buf, RSXMLPlistTagsUnicode[PLIST_IX], PLIST_TAG_LENGTH))
    {
        return PLIST_IX;
    }
    return kSuccess;
}

static RSNumberRef __RSPropertyListTagNumberWithID(RSCode tagID)
{
    return RSNumberCreateInteger(RSAllocatorSystemDefault, tagID);
}

#if !defined(new_rstype_array)
#define new_rstype_array(N, C) \
size_t N ## _count__ = (C); \
if (N ## _count__ > LONG_MAX / sizeof(RSTypeRef)) { \
HALTWithError(RSGenericException, "RSPropertyList ran out of memory while attempting to allocate temporary storage."); \
} \
BOOL N ## _is_stack__ = (N ## _count__ <= 256); \
if (N ## _count__ == 0) N ## _count__ = 1; \
STACK_BUFFER_DECL(RSTypeRef, N ## _buffer__, N ## _is_stack__ ? N ## _count__ : 1); \
if (N ## _is_stack__) memset(N ## _buffer__, 0, N ## _count__ * sizeof(RSTypeRef)); \
RSTypeRef * N = nil;\
if (N ## _is_stack__) \
    N = (N ## _buffer__);\
else \
    N = RSAllocatorAllocate(RSAllocatorSystemDefault, (N ## _count__) * sizeof(RSTypeRef)); \
if (! N) { \
HALTWithError(RSGenericException, "RSPropertyList ran out of memory while attempting to allocate temporary storage."); \
} \
do {} while (0)
#endif

#if !defined(free_rstype_array)
#define free_rstype_array(N) \
if (! N ## _is_stack__) { \
RSAllocatorDeallocate(RSAllocatorSystemDefault, N); \
} \
do {} while (0)
#endif
extern RSStringRef __RSNumberCopyFormattingDescriptionAsFloat64(RSTypeRef obj);
static RSMutableStringRef __RSNormalStringToXMLFormat(RSStringRef normal)
{
    RSMutableStringRef newString = RSMutableCopy(RSAllocatorSystemDefault, normal);
    RSStringReplaceAll(RSStringReplaceAll(RSStringReplaceAll(RSStringReplaceAll(RSStringReplaceAll(newString,
                                                                                                   RSSTR("&"),
                                                                                                   RSSTR("&amp;")),
                                                                                RSSTR("<"),
                                                                                RSSTR("&lt;")),
                                                             RSSTR(">"),
                                                             RSSTR("&gt;")),
                                          RSSTR("\'"),
                                          RSSTR("&apos;")),
                       RSSTR("\""),
                       RSSTR("&quot;"));
    
    return newString;
}

static RSMutableStringRef __RSNormalStringFromXMLFormat(RSStringRef xmlString)
{
    RSMutableStringRef newString = RSMutableCopy(RSAllocatorSystemDefault, xmlString);
    RSStringReplaceAll(RSStringReplaceAll(RSStringReplaceAll(RSStringReplaceAll(RSStringReplaceAll(newString,
                                                                                                   RSSTR("&lt;"),
                                                                                                   RSSTR("<")),
                                                                                RSSTR("&gt;"),
                                                                                RSSTR(">")),
                                                             RSSTR("&apos;"),
                                                             RSSTR("\'")),
                                          RSSTR("&quot;"),
                                          RSSTR("\"")),
                       RSSTR("&amp;"),
                       RSSTR("&"));
    return newString;
}
static BOOL __RSPlistHaltHandler(const char* errorCStr, __autorelease RSErrorRef* error)
{
    return NO;
}
static RSErrorCode __RSAppendXML0(RSTypeRef object, RSBitU32 indentation, RSMutableDataRef xmlString)
{
    RSTypeID typeID = RSGetTypeID(object);
    _appendIndents(indentation, xmlString);
    if (typeID == stringtype)
    {
        _plistAppendUTF8CString(xmlString, "<");
        _plistAppendCharacters(xmlString, RSXMLPlistTagsUnicode[STRING_IX], STRING_TAG_LENGTH);
        _plistAppendUTF8CString(xmlString, ">");
        
        RSStringRef xmlObj = __RSNormalStringToXMLFormat((RSStringRef)object);
        _plistAppendString(xmlString, xmlObj);
        RSRelease(xmlObj);
        
        _plistAppendUTF8CString(xmlString, "</");
        _plistAppendCharacters(xmlString, RSXMLPlistTagsUnicode[STRING_IX], STRING_TAG_LENGTH);
        _plistAppendUTF8CString(xmlString, ">\n");
        
        return kSuccess;
    }
    else if (typeID == arraytype)
    {
        RSBitU32 i;
        RSIndex count = RSArrayGetCount((RSArrayRef)object);
        if (count == 0)
        {
            _plistAppendUTF8CString(xmlString, "<");
            _plistAppendCharacters(xmlString, RSXMLPlistTagsUnicode[ARRAY_IX], ARRAY_TAG_LENGTH);
            _plistAppendUTF8CString(xmlString, "/>\n");
            return kSuccess;
        }
        _plistAppendUTF8CString(xmlString, "<");
        _plistAppendCharacters(xmlString, RSXMLPlistTagsUnicode[ARRAY_IX], ARRAY_TAG_LENGTH);
        _plistAppendUTF8CString(xmlString, ">\n");
        RSErrorCode errorCode = kSuccess;
        for (i = 0; i < count; i ++) {
            RSTypeRef oinArray = RSArrayObjectAtIndex((RSArrayRef)object, i);
            errorCode = __RSAppendXML0(oinArray, indentation+1, xmlString);
            if (errorCode) break;
            //RSRelease(oinArray);
        }
        _appendIndents(indentation, xmlString);
        _plistAppendUTF8CString(xmlString, "</");
        _plistAppendCharacters(xmlString, RSXMLPlistTagsUnicode[ARRAY_IX], ARRAY_TAG_LENGTH);
        _plistAppendUTF8CString(xmlString, ">\n");
        return errorCode;
    }
    else if (typeID == dicttype)
    {
        RSBitU32 i;
        RSIndex count = RSDictionaryGetCount((RSDictionaryRef)object);
        RSArrayRef keyArray;
        if (count == 0)
        {
            _plistAppendUTF8CString(xmlString, "<");
            _plistAppendCharacters(xmlString, RSXMLPlistTagsUnicode[DICT_IX], DICT_TAG_LENGTH);
            _plistAppendUTF8CString(xmlString, "/>\n");
            return kSuccess;
        }
        _plistAppendUTF8CString(xmlString, "<");
        _plistAppendCharacters(xmlString, RSXMLPlistTagsUnicode[DICT_IX], DICT_TAG_LENGTH);
        _plistAppendUTF8CString(xmlString, ">\n");
        new_rstype_array(keys, count);
        //RSArrayRef keys = RSDictionaryCopyAllKeys(object);
        RSDictionaryGetKeysAndValues((RSDictionaryRef)object, keys, nil);
//        keyArray = RSArrayCreateWithObjects(RSAllocatorSystemDefault, keys, count);
        keyArray = RSArrayCreateMutable(RSAllocatorSystemDefault, 0);
        for (RSUInteger idx = 0; idx < count; idx++) {
            RSArrayAddObject((RSMutableArrayRef)keyArray, keys[idx]);
        }
        
        RSArraySort(keyArray, RSOrderedAscending, (RSComparatorFunction)RSStringCompare, nil);
        RSArrayGetObjects(keyArray, RSMakeRange(0, count), keys);
        
        RSErrorCode errCode = kSuccess;
        for (i = 0; i < count; i ++)
        {
            RSTypeRef key = keys[i];
            _appendIndents(indentation+1, xmlString);
            _plistAppendUTF8CString(xmlString, "<");
            _plistAppendCharacters(xmlString, RSXMLPlistTagsUnicode[KEY_IX], KEY_TAG_LENGTH);
            _plistAppendUTF8CString(xmlString, ">");
            
            RSStringRef _xmlKey = __RSNormalStringToXMLFormat((RSStringRef)key);
            _plistAppendString(xmlString, _xmlKey);
            RSRelease(_xmlKey);
            
            _plistAppendUTF8CString(xmlString, "</");
            _plistAppendCharacters(xmlString, RSXMLPlistTagsUnicode[KEY_IX], KEY_TAG_LENGTH);
            _plistAppendUTF8CString(xmlString, ">\n");
            RSTypeRef value = RSDictionaryGetValue((RSDictionaryRef)object, key);
            errCode = __RSAppendXML0(value, indentation+1, xmlString);
            if (errCode) break;
            //RSRelease(value);
        }
        RSRelease(keyArray);
        free_rstype_array(keys);
        _appendIndents(indentation, xmlString);
        _plistAppendUTF8CString(xmlString, "</");
        _plistAppendCharacters(xmlString, RSXMLPlistTagsUnicode[DICT_IX], DICT_TAG_LENGTH);
        _plistAppendUTF8CString(xmlString, ">\n");
        return errCode;
    }
    else if (typeID == datatype)
    {
        _plistAppendUTF8CString(xmlString, "<");
        _plistAppendCharacters(xmlString, RSXMLPlistTagsUnicode[DATA_IX], DATA_TAG_LENGTH);
        _plistAppendUTF8CString(xmlString, ">\n");
        _XMLPlistAppendDataUsingBase64(xmlString, (RSDataRef)object, indentation);
        _appendIndents(indentation, xmlString);
        _plistAppendUTF8CString(xmlString, "</");
        _plistAppendCharacters(xmlString, RSXMLPlistTagsUnicode[DATA_IX], DATA_TAG_LENGTH);
        _plistAppendUTF8CString(xmlString, ">\n");
        return kSuccess;
    }
    else if (typeID == datetype)
    {
        // YYYY '-' MM '-' DD 'T' hh ':' mm ':' ss 'Z'
//        RSBit32 y = 0, M = 0, d = 0, H = 0, m = 0, s = 0;

        RSGregorianDate date = RSAbsoluteTimeGetGregorianDate(RSDateGetAbsoluteTime((RSDateRef)object), nil);
//        y = date.year;
//        M = date.month;
//        d = date.day;
//        H = date.hour;
//        m = date.minute;
//        s = (int32_t)date.second;

        _plistAppendUTF8CString(xmlString, "<");
        _plistAppendCharacters(xmlString, RSXMLPlistTagsUnicode[DATE_IX], DATE_TAG_LENGTH);
        _plistAppendUTF8CString(xmlString, ">");
        _plistAppendFormat(xmlString, RSSTR("%04d-%02d-%02dT%02d:%02d:%02dZ"), date.year, date.month, date.day, date.hour, date.minute, (int32_t)date.second);
        _plistAppendUTF8CString(xmlString, "</");
        _plistAppendCharacters(xmlString, RSXMLPlistTagsUnicode[DATE_IX], DATE_TAG_LENGTH);
        _plistAppendUTF8CString(xmlString, ">\n");
        return kSuccess;
    }
    else if (typeID == numbertype)
    {
        if (RSNumberIsFloatType((RSNumberRef)object))
        {
            _plistAppendUTF8CString(xmlString, "<");
            _plistAppendCharacters(xmlString, RSXMLPlistTagsUnicode[REAL_IX], REAL_TAG_LENGTH);
            _plistAppendUTF8CString(xmlString, ">");
            RSStringRef s = __RSNumberCopyFormattingDescriptionAsFloat64(object);
            _plistAppendString(xmlString, s);
            RSRelease(s);
            _plistAppendUTF8CString(xmlString, "</");
            _plistAppendCharacters(xmlString, RSXMLPlistTagsUnicode[REAL_IX], REAL_TAG_LENGTH);
            _plistAppendUTF8CString(xmlString, ">\n");
        }
        else if (RSNumberIsBooleanType((RSNumberRef)object))
        {
            BOOL boolean = NO;
            RSNumberGetValue(object, &boolean);
            if (boolean)
            {
                _plistAppendUTF8CString(xmlString, "<");
                _plistAppendCharacters(xmlString, RSXMLPlistTagsUnicode[TRUE_IX], TRUE_TAG_LENGTH);
                _plistAppendUTF8CString(xmlString, "/>\n");
            }
            else
            {
                _plistAppendUTF8CString(xmlString, "<");
                _plistAppendCharacters(xmlString, RSXMLPlistTagsUnicode[FALSE_IX], FALSE_TAG_LENGTH);
                _plistAppendUTF8CString(xmlString, "/>\n");
            }
        }
        else
        {
            _plistAppendUTF8CString(xmlString, "<");
            _plistAppendCharacters(xmlString, RSXMLPlistTagsUnicode[INTEGER_IX], INTEGER_TAG_LENGTH);
            _plistAppendUTF8CString(xmlString, ">");
            
            _plistAppendFormat(xmlString, RSSTR("%R"), object);
            
            _plistAppendUTF8CString(xmlString, "</");
            _plistAppendCharacters(xmlString, RSXMLPlistTagsUnicode[INTEGER_IX], INTEGER_TAG_LENGTH);
            _plistAppendUTF8CString(xmlString, ">\n");
        }
        return kSuccess;
    }
    return kErrVerify;
}

RSPrivate RSMutableDataRef __RSPropertyListCreateWithError(RSTypeRef root, __autorelease RSErrorRef* error)
{
    RSMutableDataRef __xmlData = RSDataCreateMutable(RSAllocatorSystemDefault, 128);
    RSErrorCode errCode = __RSAppendXML0(root, 0, __xmlData);
    if (errCode == kErrVerify)
    {
        RSRelease(__xmlData);
        if (error)
        {
            *error = RSErrorCreate(RSAllocatorSystemDefault, RSErrorDomainRSCoreFoundation, errCode, nil);
            RSAutorelease(*error);
        }
        return nil;
    }
    return __xmlData;
}

static RSDataRef __RSPropertyListHeader()
{
    /*
     <?xml version=\"1.0\" encoding=\"UTF-8\"?>
     <!DOCTYPE plist PUBLIC "-//Apple//DTD PLIST 1.0//EN" "http://www.apple.com/DTDs/PropertyList-1.0.dtd">
     */
    static const RSBitU8 __xmlHeader[] = "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\r\n\
<!DOCTYPE plist PUBLIC \"-//Apple//DTD PLIST 1.0//EN\" \"http://www.apple.com/DTDs/PropertyList-1.0.dtd\">\r\n\
<plist version=\"1.0\">\n";
    return RSDataCreate(RSAllocatorSystemDefault, __xmlHeader, sizeof(__xmlHeader)/sizeof(RSBitU8) - 1);
}

static RSDataRef __RSPropertyListTailer()
{
    static const RSBitU8 __xmlTailer[] = "</plist>";
    return RSDataCreate(RSAllocatorSystemDefault, __xmlTailer, sizeof(__xmlTailer)/sizeof(RSBitU8) - 1);
}

static BOOL __RSPropertyListParseHeader(RSDataRef plistData, RSIndex* offset)
{
    if (plistData == nil) return NO;
    const UniChar* bytes = RSDataGetBytesPtr(plistData);
    UniChar* c = (UniChar*)bytes;
    BOOL inTag = NO;
    BOOL inComment = NO;
    BOOL inNextTag __unused = NO;
    BOOL inHeader = NO;
    BOOL isParserHeader = NO;
    BOOL isParserComment = NO;
    RSIndex idx = 0;
    while (*c)
    {
        switch (*c)
        {
            case '<':
                inTag = YES;
                inNextTag = YES;
                break;
            case '>':
                if (inTag)
                {
                    inTag = NO;
                    if (inComment)
                    {
                        if (*(c - 1) != '\"')
                        {
                            return __RSPlistHaltHandler("", nil);
                        }
                        inComment = NO;
                        isParserComment = YES;
                        break;
                    }
                    
                    if (inHeader)
                    {
                        if (*(c-1) != '?')
                        {
                            return __RSPlistHaltHandler("", nil);
                        }
                        inHeader = NO;
                        isParserHeader = YES;
                        break;
                    }
                }
                break;
            default:
                if (inTag)
                {
                    if (inNextTag)
                    {
                        switch (*c)
                        {
                            case '?':
                                if (inTag && inNextTag)
                                    inHeader = YES;
                                else
                                    return __RSPlistHaltHandler("xml is not available", nil);
                                break;
                            case '!':
                                if (inTag && inNextTag)
                                    inComment = YES;
                                else
                                    return __RSPlistHaltHandler("xml is not available", nil);
                                break;
                            default:
                                break;
                        }
                    }
                    inNextTag = NO;
                }
                inNextTag = NO;
                break;
        }
        if (inTag == NO && inComment == NO && inNextTag == NO && inHeader == NO
            && isParserHeader == YES && isParserComment == YES)
        {
            idx++;
            if (offset) *offset = idx;
            return YES;
        }
        c++;idx++;
    }
    return NO;
}

static RSDataRef __plistRestoreData(RSAllocatorRef allocator, RSStringRef value)
{
    /*the data is using base64 encoding. should restore it.*/
    const char *base = RSStringGetCStringPtr(value, RSStringEncodingASCII);
    char* curr = (char*)base, *end = curr + RSStringGetLength(value);
    static const signed char dataDecodeTable[128] =
    {
        /* 000 */ -1, -1, -1, -1, -1, -1, -1, -1,
        /* 010 */ -1, -1, -1, -1, -1, -1, -1, -1,
        /* 020 */ -1, -1, -1, -1, -1, -1, -1, -1,
        /* 030 */ -1, -1, -1, -1, -1, -1, -1, -1,
        /* ' ' */ -1, -1, -1, -1, -1, -1, -1, -1,
        /* '(' */ -1, -1, -1, 62, -1, -1, -1, 63,
        /* '0' */ 52, 53, 54, 55, 56, 57, 58, 59,
        /* '8' */ 60, 61, -1, -1, -1,  0, -1, -1,
        /* '@' */ -1,  0,  1,  2,  3,  4,  5,  6,
        /* 'H' */  7,  8,  9, 10, 11, 12, 13, 14,
        /* 'P' */ 15, 16, 17, 18, 19, 20, 21, 22,
        /* 'X' */ 23, 24, 25, -1, -1, -1, -1, -1,
        /* '`' */ -1, 26, 27, 28, 29, 30, 31, 32,
        /* 'h' */ 33, 34, 35, 36, 37, 38, 39, 40,
        /* 'p' */ 41, 42, 43, 44, 45, 46, 47, 48,
        /* 'x' */ 49, 50, 51, -1, -1, -1, -1, -1
    };
    
    int tmpbufpos = 0;
    int tmpbuflen = 256;
    uint8_t *tmpbuf = (uint8_t *)RSAllocatorAllocate(RSAllocatorSystemDefault, tmpbuflen);
    int numeq = 0;
    int acc = 0;
    int cntr = 0;
    
    for (; curr < end; curr++)
    {
        signed char c = *(curr);
        if (c == '<')
        {
            break;
        }
        if ('=' == c)
        {
            numeq++;
        }
        else if (!isspace(c))
        {
            numeq = 0;
        }
        if (dataDecodeTable[c] < 0)
            continue;
        cntr++;
        acc <<= 6;
        acc += dataDecodeTable[c];
        if (0 == (cntr & 0x3))
        {
            if (tmpbuflen <= tmpbufpos + 2)
            {
                if (tmpbuflen < 256 * 1024)
                {
                    tmpbuflen *= 4;
                }
                else if (tmpbuflen < 16 * 1024 * 1024)
                {
                    tmpbuflen *= 2;
                }
                else
                {
                    // once in this stage, this will be really slow
                    // and really potentially fragment memory
                    tmpbuflen += 256 * 1024;
                }
                tmpbuf = (uint8_t *)RSAllocatorReallocate(RSAllocatorSystemDefault, tmpbuf, tmpbuflen);
                if (!tmpbuf) __HALT(); // out of memory
            }
            tmpbuf[tmpbufpos++] = (acc >> 16) & 0xff;
            if (numeq < 2) tmpbuf[tmpbufpos++] = (acc >> 8) & 0xff;
            if (numeq < 1) tmpbuf[tmpbufpos++] = acc & 0xff;
        }
    }
    
    RSDataRef _v = nil;
    
    _v = RSDataCreateWithNoCopy(allocator, tmpbuf, tmpbufpos, YES, RSAllocatorSystemDefault);
    if (!_v)
    {
        curr = (char*)base;
        RSAllocatorDeallocate(allocator, tmpbuf);
        return nil;
    }
    return _v;
}

static RSNumberRef __plistRestoreNumber(RSAllocatorRef allocator, RSStringRef value, int tagID)
{
    RSNumberRef _v = nil;
    if (tagID == TRUE_IX)
    {
        _v = RSNumberCreateBoolean(allocator, YES);
    }
    else if (tagID == FALSE_IX)
    {
        _v = RSNumberCreateBoolean(allocator, NO);
    }
    else if (tagID == INTEGER_IX)
    {
        int n = 0;
        const char* ptr = RSStringGetCStringPtr(value, RSStringEncodingASCII);
        assert(strlen((char*)ptr) == RSStringGetLength(value));
        n = atoi((char*)ptr);
        _v = RSNumberCreateInteger(allocator, n);
    }
    else if (tagID == REAL_IX)
    {
        double n = .0f;
        const char* ptr = RSStringGetCStringPtr(value, RSStringEncodingASCII);
        assert(strlen((char*)ptr) == RSStringGetLength(value));
        n = atof((char*)ptr);
        _v = RSNumberCreateDouble(allocator, n);
    }
    return _v == nil ? _v = RSNumberCreateBoolean(allocator, NO) : _v;
}

static RSTypeRef __RSPropertyListCopyValue(RSAllocatorRef allocator, RSTypeRef value, int tagID)
{
    /*
     RSTypeID typeID = RSGetTypeID(value);
     static RSTypeID stringtype, datatype, numbertype, datetype;
     static RSTypeID booltype __unused, nulltype, dicttype, arraytype, settype;
     */
    RSTypeRef _v = nil;
    switch (tagID)
    {
        case -1:
        case PLIST_IX:
            break;
        case ARRAY_IX:
            _v = RSCopy(allocator, value);
            break;
        case DICT_IX:
            _v = RSCopy(allocator, value);
            break;
        case STRING_IX:
            _v = __RSNormalStringFromXMLFormat(value);
            break;
        case DATA_IX:
            _v = __plistRestoreData(allocator, value);
            break;
        case DATE_IX:
            _v = RSDateCreateWithString(allocator, value);
            break;
        case REAL_IX:
        case INTEGER_IX:
        case TRUE_IX:
        case FALSE_IX:
            _v = __plistRestoreNumber(allocator, value, tagID);
            break;
        default:
            break;
    }
    
    return _v;
}

#pragma mark -
#pragma mark RSPropertyListParserCore   New Version API

RSErrorRef __RSPropertyListCreateError(RSErrorCode errorCode, RSStringRef descriptionFormat, ...)
{
    va_list ap;
    va_start(ap, descriptionFormat);
    RSStringRef description = RSStringCreateWithFormatAndArguments(RSAllocatorSystemDefault, 0, descriptionFormat, ap);
    va_end(ap);
    RSDictionaryRef userInfo = RSDictionaryCreateWithObjectsAndOKeys(RSAllocatorSystemDefault, description, RSErrorDebugDescriptionKey, nil);
    RSRelease(description);
    RSErrorRef error = RSErrorCreate(RSAllocatorSystemDefault, RSErrorDomainRSCoreFoundation, errorCode, userInfo);
    RSRelease(userInfo);
    return error;
}

typedef struct __RSPlistParserInfo
{
    const char *begin; // first character of the XML to be parsed
    const char *curr;  // current parse location
    const char *end;   // the first character _after_ the end of the XML
    RSErrorRef error;
    RSAllocatorRef allocator;
    UInt32 mutabilityOption;
    //RSBurstTrieRef stringTrie; // map of cached strings
    RSMutableArrayRef stringCache; // retaining array of strings
    BOOL allowNewTypes; // Whether to allow the new types supported by XML property lists, but not by the old, OPENSTEP ASCII property lists (RSNumber, RSBoolean, RSDate)
    BOOL skip; // if YES, do not create any objects.
}__RSPlistParserInfo;

static UInt32 lineNumber(__RSPlistParserInfo *pInfo)
{
    const char *p = pInfo->begin;
    UInt32 count = 1;
    while (p < pInfo->curr)
    {
        if (*p == '\r')
        {
            count ++;
            if (*(p + 1) == '\n')
                p ++;
        }
        else if (*p == '\n')
        {
            count ++;
        }
        p ++;
    }
    return count;
}

RSInline void skipWhitespace(__RSPlistParserInfo *pInfo)
{
    while (pInfo->curr < pInfo->end)
    {
        switch (*(pInfo->curr))
        {
            case ' ':
            case '\t':
            case '\n':
            case '\r':
                pInfo->curr ++;
                continue;
            default:
                return;
        }
    }
}

// info should be just past "<!--"
static void skipXMLComment(__RSPlistParserInfo *pInfo)
{
    const char *p = pInfo->curr;
    const char *end = pInfo->end - 3; // Need at least 3 characters to compare against
    while (p < end)
    {
        if (*p == '-' && *(p+1) == '-' && *(p+2) == '>')
        {
            pInfo->curr = p+3;
            return;
        }
        p ++;
    }
    pInfo->error = __RSPropertyListCreateError(RSPropertyListReadCorruptError, RSSTR("Unterminated comment started on line %d"), lineNumber(pInfo));
}

// pInfo should be set to the first character after "<?"
static void skipXMLProcessingInstruction(__RSPlistParserInfo *pInfo)
{
    const char *begin = pInfo->curr, *end = pInfo->end - 2; // Looking for "?>" so we need at least 2 characters
    while (pInfo->curr < end)
    {
        if (*(pInfo->curr) == '?' && *(pInfo->curr+1) == '>')
        {
            pInfo->curr += 2;
            return;
        }
        pInfo->curr ++;
    }
    pInfo->curr = begin;
    pInfo->error = __RSPropertyListCreateError(RSPropertyListReadCorruptError, RSSTR("Encountered unexpected EOF while parsing the processing instruction begun on line %d"), lineNumber(pInfo));
}

static void skipPERef(__RSPlistParserInfo *pInfo)
{
    const char *p = pInfo->curr;
    while (p < pInfo->end)
    {
        if (*p == ';')
        {
            pInfo->curr = p+1;
            return;
        }
        p ++;
    }
    pInfo->error = __RSPropertyListCreateError(RSPropertyListReadCorruptError, RSSTR("Encountered unexpected EOF while parsing percent-escape sequence begun on line %d"), lineNumber(pInfo));
}

// First character should be just past '['
static void skipInlineDTD(__RSPlistParserInfo *pInfo)
{
    while (!pInfo->error && pInfo->curr < pInfo->end)
    {
        UniChar ch;
        skipWhitespace(pInfo);
        ch = *pInfo->curr;
        if (ch == '%')
        {
            pInfo->curr ++;
            skipPERef(pInfo);
        }
        else if (ch == '<')
        {
            pInfo->curr ++;
            if (pInfo->curr >= pInfo->end)
            {
                pInfo->error = __RSPropertyListCreateError(RSPropertyListReadCorruptError, RSSTR("Encountered unexpected EOF while parsing inline DTD"));
                return;
            }
            ch = *(pInfo->curr);
            if (ch == '?')
            {
                pInfo->curr ++;
                skipXMLProcessingInstruction(pInfo);
            }
            else if (ch == '!')
            {
                if (pInfo->curr + 2 < pInfo->end && (*(pInfo->curr+1) == '-' && *(pInfo->curr+2) == '-'))
                {
                    pInfo->curr += 3;
                    skipXMLComment(pInfo);
                }
                else
                {
                    // Skip the myriad of DTD declarations of the form "<!string" ... ">"
                    pInfo->curr ++; // Past both '<' and '!'
                    while (pInfo->curr < pInfo->end)
                    {
                        if (*(pInfo->curr) == '>') break;
                        pInfo->curr ++;
                    }
                    if (*(pInfo->curr) != '>')
                    {
                        pInfo->error = __RSPropertyListCreateError(RSPropertyListReadCorruptError, RSSTR("Encountered unexpected EOF while parsing inline DTD"));
                        return;
                    }
                    pInfo->curr ++;
                }
            }
            else
            {
                pInfo->error = __RSPropertyListCreateError(RSPropertyListReadCorruptError, RSSTR("Encountered unexpected character %c on line %d while parsing inline DTD"), ch, lineNumber(pInfo));
                return;
            }
        }
        else if (ch == ']')
        {
            pInfo->curr ++;
            return;
        }
        else
        {
            pInfo->error = __RSPropertyListCreateError(RSPropertyListReadCorruptError, RSSTR("Encountered unexpected character %c on line %d while parsing inline DTD"), ch, lineNumber(pInfo));
            return;
        }
    }
    if (!pInfo->error)
    {
        pInfo->error = __RSPropertyListCreateError(RSPropertyListReadCorruptError, RSSTR("Encountered unexpected EOF while parsing inline DTD"));
    }
}

// first character should be immediately after the "<!"
static void skipDTD(__RSPlistParserInfo *pInfo)
{
    // First pass "DOCTYPE"
    if (pInfo->end - pInfo->curr < DOCTYPE_TAG_LENGTH ||
        memcmp(pInfo->curr, RSXMLPlistTags[DOCTYPE_IX], DOCTYPE_TAG_LENGTH))
    {
        pInfo->error = __RSPropertyListCreateError(RSPropertyListReadCorruptError, RSSTR("Malformed DTD on line %d"), lineNumber(pInfo));
        return;
    }
    pInfo->curr += DOCTYPE_TAG_LENGTH;
    skipWhitespace(pInfo);
    
    // Look for either the beginning of a complex DTD or the end of the DOCTYPE structure
    while (pInfo->curr < pInfo->end)
    {
        char ch = *(pInfo->curr);
        if (ch == '[') break; // inline DTD
        if (ch == '>')
        {
            // End of the DTD
            pInfo->curr ++;
            return;
        }
        pInfo->curr ++;
    }
    if (pInfo->curr == pInfo->end)
    {
        pInfo->error = __RSPropertyListCreateError(RSPropertyListReadCorruptError, RSSTR("Encountered unexpected EOF while parsing DTD"));
        return;
    }
    
    // *Sigh* Must parse in-line DTD
    skipInlineDTD(pInfo);
    if (pInfo->error)  return;
    skipWhitespace(pInfo);
    if (pInfo->error) return;
    if (pInfo->curr < pInfo->end)
    {
        if (*(pInfo->curr) == '>')
        {
            pInfo->curr ++;
        }
        else
        {
            pInfo->error = __RSPropertyListCreateError(RSPropertyListReadCorruptError, RSSTR("Encountered unexpected character %c on line %d while parsing DTD"), *(pInfo->curr), lineNumber(pInfo));
        }
    }
    else
    {
        pInfo->error = __RSPropertyListCreateError(RSPropertyListReadCorruptError, RSSTR("Encountered unexpected EOF while parsing DTD"));
    }
}

static BOOL parseStringTag(__RSPlistParserInfo *pInfo, RSStringRef *out);
static BOOL parseXMLElement(__RSPlistParserInfo *pInfo, BOOL *isKey, RSTypeRef *out);
// content ::== (element | CharData | Reference | CDSect | PI | Comment)*
// In the context of a plist, CharData, Reference and CDSect are not legal (they all resolve to strings).  Skipping whitespace, then, the next character should be '<'.  From there, we figure out which of the three remaining cases we have (element, PI, or Comment).
static BOOL getContentObject(__RSPlistParserInfo *pInfo, BOOL *isKey, RSTypeRef *out)
{
    if (isKey) *isKey = NO;
    while (!pInfo->error && pInfo->curr < pInfo->end)
    {
        skipWhitespace(pInfo);
        if (pInfo->curr >= pInfo->end)
        {
            pInfo->error = __RSPropertyListCreateError(RSPropertyListReadCorruptError, RSSTR("Encountered unexpected EOF"));
            return NO;
        }
        if (*(pInfo->curr) != '<')
        {
            pInfo->error = __RSPropertyListCreateError(RSPropertyListReadCorruptError, RSSTR("Encountered unexpected character %c on line %d while looking for open tag"), *(pInfo->curr), lineNumber(pInfo));
            return NO;
        }
        pInfo->curr ++;
        if (pInfo->curr >= pInfo->end)
        {
            pInfo->error = __RSPropertyListCreateError(RSPropertyListReadCorruptError, RSSTR("Encountered unexpected EOF"));
            return NO;
        }
        switch (*(pInfo->curr))
        {
            case '?':
                // Processing instruction
                skipXMLProcessingInstruction(pInfo);
                break;
            case '!':
                // Could be a comment
                if (pInfo->curr+2 >= pInfo->end)
                {
                    pInfo->error = __RSPropertyListCreateError(RSPropertyListReadCorruptError, RSSTR("Encountered unexpected EOF"));
                    return NO;
                }
                if (*(pInfo->curr+1) == '-' && *(pInfo->curr+2) == '-')
                {
                    pInfo->curr += 2;
                    skipXMLComment(pInfo);
                }
                else
                {
                    pInfo->error = __RSPropertyListCreateError(RSPropertyListReadCorruptError, RSSTR("Encountered unexpected EOF"));
                    return NO;
                }
                break;
            case '/':
                // Whoops!  Looks like we got to the end tag for the element whose content we're parsing
                pInfo->curr --; // Back off to the '<'
                return NO;
            default:
                // Should be an element
                return parseXMLElement(pInfo, isKey, out);
        }
    }
    // Do not set the error string here; if it wasn't already set by one of the recursive parsing calls, the caller will quickly detect the failure (b/c pInfo->curr >= pInfo->end) and provide a more useful one of the form "end tag for <blah> not found"
    return NO;
}

#define GET_CH	if (pInfo->curr == pInfo->end) \
                {	\
                    pInfo->error = __RSPropertyListCreateError(RSPropertyListReadCorruptError, RSSTR("Premature end of file after <integer> on line %d"), lineNumber(pInfo)); \
                    return NO;			\
                }					\
                ch = *(pInfo->curr)

RSInline BOOL isWhitespace(const char *utf8bytes, const char *end)
{
    // Converted UTF-16 isWhitespace from RSString to UTF8 bytes to get full list of UTF8 whitespace
    /*
     0020 -> <20>
     0009 -> <09>
     00a0 -> <c2a0>
     1680 -> <e19a80>
     2000 -> <e28080>
     2001 -> <e28081>
     2002 -> <e28082>
     2003 -> <e28083>
     2004 -> <e28084>
     2005 -> <e28085>
     2006 -> <e28086>
     2007 -> <e28087>
     2008 -> <e28088>
     2009 -> <e28089>
     200a -> <e2808a>
     200b -> <e2808b>
     202f -> <e280af>
     205f -> <e2819f>
     3000 -> <e38080>
     */
    // Except we consider some additional values from 0x0 to 0x21 and 0x7E to 0xA1 as whitespace, for compatability
    char byte1 = *utf8bytes;
    if (byte1 < 0x21 || (byte1 > 0x7E && byte1 < 0xA1)) return YES;
    if ((byte1 == 0xe2 || byte1 == 0xe3) && (end - utf8bytes >= 3))
    {
        // Check other possibilities in the 3-bytes range
        char byte2 = *(utf8bytes + 1);
        char byte3 = *(utf8bytes + 2);
        if (byte1 == 0xe2 && byte2 == 0x80)
        {
            return ((byte3 >= 80 && byte3 <= 0x8b) || byte3 == 0xaf);
        }
        else if (byte1 == 0xe2 && byte2 == 0x81)
        {
            return byte3 == 0x9f;
        }
        else if (byte1 == 0xe3 && byte2 == 0x80 && byte3 == 0x80)
        {
            return YES;
        }
    }
    return NO;
}

static BOOL checkForCloseTag(__RSPlistParserInfo *pInfo, const char *tag, RSIndex tagLen)
{
    if (pInfo->end - pInfo->curr < tagLen + 3)
    {
        if (!pInfo->error)
        {
            pInfo->error = __RSPropertyListCreateError(RSPropertyListReadCorruptError, RSSTR("Encountered unexpected EOF"));
        }
        return NO;
    }
    if (*(pInfo->curr) != '<' || *(++pInfo->curr) != '/')
    {

        if (!pInfo->error) {
            pInfo->error = __RSPropertyListCreateError(RSPropertyListReadCorruptError, RSSTR("Encountered unexpected character %c on line %d while looking for close tag"), *(pInfo->curr), lineNumber(pInfo));
        }
        return NO;
    }
    pInfo->curr ++;
    if (memcmp(pInfo->curr, tag, tagLen))
    {
        RSStringRef str = RSStringCreateWithBytes(RSAllocatorSystemDefault, (const UInt8 *)tag, tagLen, __RSDefaultEightBitStringEncoding, NO);
        if (!pInfo->error)
        {
            pInfo->error = __RSPropertyListCreateError(RSPropertyListReadCorruptError, RSSTR("Close tag on line %d does not match open tag %@"), lineNumber(pInfo), str);
        }
        RSRelease(str);
        return NO;
    }
    pInfo->curr += tagLen;
    skipWhitespace(pInfo);
    if (pInfo->curr == pInfo->end)
    {
        if (!pInfo->error)
        {
            pInfo->error = __RSPropertyListCreateError(RSPropertyListReadCorruptError, RSSTR("Encountered unexpected EOF"));
        }
        return NO;
    }
    if (*(pInfo->curr) != '>')
    {
        if (!pInfo->error)
        {
            pInfo->error = __RSPropertyListCreateError(RSPropertyListReadCorruptError, RSSTR("Encountered unexpected character %c on line %d while looking for close tag"), *(pInfo->curr), lineNumber(pInfo));
        }
        return NO;
    }
    pInfo->curr ++;
    return YES;
}

static BOOL parseIntegerTag(__RSPlistParserInfo *pInfo, RSTypeRef *out)
{
    BOOL isHex = NO, isNeg = NO, hadLeadingZero = NO;
    char ch = 0;
    
    // decimal_constant         S*(-|+)?S*[0-9]+		(S == space)
    // hex_constant		S*(-|+)?S*0[xX][0-9a-fA-F]+	(S == space)
    
    while (pInfo->curr < pInfo->end && isWhitespace(pInfo->curr, pInfo->end)) pInfo->curr++;
    GET_CH;
    if ('<' == ch)
    {
        pInfo->error = __RSPropertyListCreateError(RSPropertyListReadCorruptError, RSSTR("Encountered empty <integer> on line %d"), lineNumber(pInfo));
        return NO;
    }
    if ('-' == ch || '+' == ch)
    {
        isNeg = ('-' == ch);
        pInfo->curr++;
        while (pInfo->curr < pInfo->end && isWhitespace(pInfo->curr, pInfo->end)) pInfo->curr++;
    }
    GET_CH;
    if ('0' == ch)
    {
        if (pInfo->curr + 1 < pInfo->end && ('x' == *(pInfo->curr + 1) || 'X' == *(pInfo->curr + 1)))
        {
            pInfo->curr++;
            isHex = YES;
        }
        else
        {
            hadLeadingZero = YES;
        }
        pInfo->curr++;
    }
    GET_CH;
    while ('0' == ch)
    {
        hadLeadingZero = YES;
        pInfo->curr++;
        GET_CH;
    }
    if ('<' == ch && hadLeadingZero)
    {	// nothing but zeros
        long long val = 0;
        if (!checkForCloseTag(pInfo, RSXMLPlistTags[INTEGER_IX], INTEGER_TAG_LENGTH))
        {
            // checkForCloseTag() sets error string
            return NO;
        }
        if (pInfo->skip)
        {
            *out = nil;
        }
        else
        {
            *out = RSNumberCreate(pInfo->allocator, RSNumberLonglong, &val);
        }
        return YES;
    }
    if ('<' == ch)
    {
        pInfo->error = __RSPropertyListCreateError(RSPropertyListReadCorruptError, RSSTR("Incomplete <integer> on line %d"), lineNumber(pInfo));
        return NO;
    }
    uint64_t value = 0;
    uint32_t multiplier = (isHex ? 16 : 10);
    while ('<' != ch)
    {
        uint32_t new_digit = 0;
        switch (ch)
        {
            case '0':
            case '1':
            case '2':
            case '3':
            case '4':
            case '5':
            case '6':
            case '7':
            case '8':
            case '9':
                new_digit = (ch - '0');
                break;
            case 'a':
            case 'b':
            case 'c':
            case 'd':
            case 'e':
            case 'f':
                new_digit = (ch - 'a' + 10);
                break;
            case 'A':
            case 'B':
            case 'C':
            case 'D':
            case 'E':
            case 'F':
                new_digit = (ch - 'A' + 10);
                break;
            default:	// other character
                pInfo->error = __RSPropertyListCreateError(RSPropertyListReadCorruptError, RSSTR("Unknown character '%c' (0x%x) in <integer> on line %d"), ch, ch, lineNumber(pInfo));
                return NO;
        }
        if (!isHex && new_digit > 9)
        {
            pInfo->error = __RSPropertyListCreateError(RSPropertyListReadCorruptError, RSSTR("Hex digit in non-hex <integer> on line %d"), lineNumber(pInfo));
            return NO;
        }
        if (UINT64_MAX / multiplier < value)
        {
            pInfo->error = __RSPropertyListCreateError(RSPropertyListReadCorruptError, RSSTR("Integer overflow in <integer> on line %d"), lineNumber(pInfo));
            return NO;
        }
        value = multiplier * value;
        if (UINT64_MAX - new_digit < value)
        {
            pInfo->error = __RSPropertyListCreateError(RSPropertyListReadCorruptError, RSSTR("Integer overflow in <integer> on line %d"), lineNumber(pInfo));
            return NO;
        }
        value = value + new_digit;
        if (isNeg && (uint64_t)INT64_MAX + 1 < value)
        {
            pInfo->error = __RSPropertyListCreateError(RSPropertyListReadCorruptError, RSSTR("Integer underflow in <integer> on line %d"), lineNumber(pInfo));
            return NO;
        }
        pInfo->curr++;
        GET_CH;
    }
    if (!checkForCloseTag(pInfo, RSXMLPlistTags[INTEGER_IX], INTEGER_TAG_LENGTH))
    {
        // checkForCloseTag() sets error string
        return NO;
    }
    
    if (pInfo->skip)
    {
        *out = nil;
    }
    else
    {
        if (isNeg || value <= INT64_MAX)
        {
            int64_t v = value;
            if (isNeg) v = -v;	// no-op if INT64_MIN
            *out = RSNumberCreate(pInfo->allocator, RSNumberLonglong, &v);
        }
        else
        {
            int64_t v = value;
            *out = RSNumberCreate(pInfo->allocator, RSNumberLonglong, &v);
        }
    }
    return YES;
}

#undef GET_CH

// Returned object is retained; caller must free.  pInfo->curr expected to point to the first character after the '<'

static void __RSPListRelease(RSTypeRef obj, RSAllocatorRef allocator)
{
    if (allocator == RSAllocatorSystemDefault ||
        allocator == RSAllocatorSystemDefault)
        RSRelease(obj);
}

static BOOL parsePListTag(__RSPlistParserInfo *pInfo, RSTypeRef *out)
{
    RSTypeRef result = nil;
    if (!getContentObject(pInfo, nil, &result))
    {
        if (!pInfo->error) pInfo->error = __RSPropertyListCreateError(RSPropertyListReadCorruptError, RSSTR("Encountered empty plist tag"));
        return NO;
    }
    const char *save = pInfo->curr; // Save this in case the next step fails
    RSTypeRef tmp = nil;
    if (getContentObject(pInfo, nil, &tmp))
    {
        // Got an extra object
        __RSPListRelease(tmp, pInfo->allocator);
        __RSPListRelease(result, pInfo->allocator);
        pInfo->curr = save;
        pInfo->error = __RSPropertyListCreateError(RSPropertyListReadCorruptError, RSSTR("Encountered unexpected element at line %d (plist can only include one object)"), lineNumber(pInfo));
        return NO;
    }
    if (pInfo->error)
    {
        // Parse failed catastrophically
        __RSPListRelease(result, pInfo->allocator);
        return NO;
    }
    if (!checkForCloseTag(pInfo, RSXMLPlistTags[PLIST_IX], PLIST_TAG_LENGTH))
    {
        __RSPListRelease(result, pInfo->allocator);
        return NO;
    }
    *out = result;
    return YES;
}

static int allowImmutableCollections = -1;

static void checkImmutableCollections(void)
{
    allowImmutableCollections = (nil == __RSRuntimeGetEnvironment("RSPropertyListAllowImmutableCollections")) ? 0 : 1;
}

static BOOL parseArrayTag(__RSPlistParserInfo *pInfo, RSTypeRef *out) {
    RSTypeRef tmp = nil;
    
    if (pInfo->skip) {
        BOOL result = getContentObject(pInfo, nil, &tmp);
        while (result) {
            if (tmp) {
                // Shouldn't happen (if skipping, all content values should be null), but just in case
                __RSPListRelease(tmp, pInfo->allocator);
            }
            result = getContentObject(pInfo, nil, &tmp);
        }
        
        if (pInfo->error) {
            // getContentObject encountered a parse error
            return NO;
        }
        if (!checkForCloseTag(pInfo, RSXMLPlistTags[ARRAY_IX], ARRAY_TAG_LENGTH)) {
            return NO;
        } else {
            *out = nil;
            return YES;
        }
    }
    
    RSMutableArrayRef array = RSArrayCreateMutable(pInfo->allocator, 0);
    BOOL result;
    
//    RSIndex count = 0;
//    RSSetRef oldKeyPaths = pInfo->keyPaths;
//    RSSetRef newKeyPaths, keys;
//    //__RSPropertyListCreateSplitKeypaths(pInfo->allocator, pInfo->keyPaths, &keys, &newKeyPaths);
//    
//    if (keys)
//    {
//        RSStringRef countString = RSStringCreateWithFormat(pInfo->allocator, nil, RSSTR("%ld"), count);
//        if (!RSSetContainsValue(keys, countString)) pInfo->skip = YES;
//        __RSPListRelease(countString, pInfo->allocator);
//        count++;
//        pInfo->keyPaths = newKeyPaths;
//    }
    result = getContentObject(pInfo, nil, &tmp);
//    if (keys)
//    {
//        pInfo->keyPaths = oldKeyPaths;
//        pInfo->skip = NO;
//    }
    
    while (result)
    {
        if (tmp)
        {
            RSArrayAddObject(array, tmp);
            __RSPListRelease(tmp, pInfo->allocator);
        }
        
//        if (keys)
//        {
//            // prep for getting next object
//            RSStringRef countString = RSStringCreateWithFormat(pInfo->allocator, nil, RSSTR("%ld"), count);
//            if (!RSSetContainsValue(keys, countString)) pInfo->skip = YES;
//            __RSPListRelease(countString, pInfo->allocator);
//            count++;
//            pInfo->keyPaths = newKeyPaths;
//        }
        result = getContentObject(pInfo, nil, &tmp);
//        if (keys)
//        {
//            // reset after getting object
//            pInfo->keyPaths = oldKeyPaths;
//            pInfo->skip = NO;
//        }
        
    }
    
//    __RSPListRelease(newKeyPaths, pInfo->allocator);
//    __RSPListRelease(keys, pInfo->allocator);
    
    if (pInfo->error)
    {
        // getContentObject encountered a parse error
        __RSPListRelease(array, pInfo->allocator);
        return NO;
    }
    if (!checkForCloseTag(pInfo, RSXMLPlistTags[ARRAY_IX], ARRAY_TAG_LENGTH))
    {
        __RSPListRelease(array, pInfo->allocator);
        return NO;
    }
    if (-1 == allowImmutableCollections)
    {
        checkImmutableCollections();
    }
    if (1 == allowImmutableCollections)
    {
        if (pInfo->mutabilityOption == RSPropertyListImmutable)
        {
            RSArrayRef newArray = RSArrayCreateCopy(pInfo->allocator, array);
            __RSPListRelease(array, pInfo->allocator);
            array = (RSMutableArrayRef)newArray;
        }
    }
    *out = array;
    return YES;
}

static BOOL parseDictTag(__RSPlistParserInfo *pInfo, RSTypeRef *out)
{
    BOOL gotKey;
    BOOL result;
    RSTypeRef key = nil, value = nil;
    
    if (pInfo->skip)
    {
        result = getContentObject(pInfo, &gotKey, &key);
        while (result)
        {
            if (!gotKey)
            {
                if (!pInfo->error) pInfo->error = __RSPropertyListCreateError(RSPropertyListReadCorruptError, RSSTR("Found non-key inside <dict> at line %d"), lineNumber(pInfo));
                return NO;
            }
            result = getContentObject(pInfo, nil, &value);
            if (!result)
            {
                if (!pInfo->error) pInfo->error = __RSPropertyListCreateError(RSPropertyListReadCorruptError, RSSTR("Value missing for key inside <dict> at line %d"), lineNumber(pInfo));
                return NO;
            }
            // key and value should be null, but we'll release just in case here
            __RSPListRelease(key, pInfo->allocator);
            key = nil;
            __RSPListRelease(value, pInfo->allocator);
            value = nil;
            result = getContentObject(pInfo, &gotKey, &key);
        }
        if (checkForCloseTag(pInfo, RSXMLPlistTags[DICT_IX], DICT_TAG_LENGTH))
        {
            *out = nil;
            return YES;
        }
        else
        {
            return NO;
        }
    }
    
//    RSSetRef oldKeyPaths = pInfo->keyPaths;
//    RSSetRef nextKeyPaths, theseKeyPaths;
//    __RSPropertyListCreateSplitKeypaths(pInfo->allocator, pInfo->keyPaths, &theseKeyPaths, &nextKeyPaths);
//    
    RSMutableDictionaryRef dict = nil;
    
    result = getContentObject(pInfo, &gotKey, &key);
    while (result && key)
    {
        if (!gotKey)
        {
            if (!pInfo->error) pInfo->error = __RSPropertyListCreateError(RSPropertyListReadCorruptError, RSSTR("Found non-key inside <dict> at line %d"), lineNumber(pInfo));
            __RSPListRelease(key, pInfo->allocator);
//            __RSPListRelease(nextKeyPaths, pInfo->allocator);
//            __RSPListRelease(theseKeyPaths, pInfo->allocator);
            __RSPListRelease(dict, pInfo->allocator);
            return NO;
        }
        
//        if (theseKeyPaths)
//        {
//            if (!RSSetContainsValue(theseKeyPaths, key)) pInfo->skip = YES;
//            pInfo->keyPaths = nextKeyPaths;
//        }
        result = getContentObject(pInfo, nil, &value);
//        if (theseKeyPaths)
//        {
//            pInfo->keyPaths = oldKeyPaths;
//            pInfo->skip = NO;
//        }
        
        if (!result)
        {
            if (!pInfo->error) pInfo->error = __RSPropertyListCreateError(RSPropertyListReadCorruptError, RSSTR("Value missing for key inside <dict> at line %d"), lineNumber(pInfo));
            __RSPListRelease(key, pInfo->allocator);
//            __RSPListRelease(nextKeyPaths, pInfo->allocator);
//            __RSPListRelease(theseKeyPaths, pInfo->allocator);
            __RSPListRelease(dict, pInfo->allocator);
            return NO;
        }
        
        if (key && value)
        {
            if (nil == dict)
            {
                dict = RSDictionaryCreateMutable(pInfo->allocator, 0, RSDictionaryRSTypeContext);
            }
            RSDictionarySetValue(dict, key, value);
        }
        
        __RSPListRelease(key, pInfo->allocator);
        key = nil;
        __RSPListRelease(value, pInfo->allocator);
        value = nil;
        
        result = getContentObject(pInfo, &gotKey, &key);
    }
    
//    __RSPListRelease(nextKeyPaths, pInfo->allocator);
//    __RSPListRelease(theseKeyPaths, pInfo->allocator);
    
    if (checkForCloseTag(pInfo, RSXMLPlistTags[DICT_IX], DICT_TAG_LENGTH))
    {
        if (nil == dict)
        {
            if (pInfo->mutabilityOption == RSPropertyListImmutable)
            {
                dict = (RSMutableDictionaryRef)RSDictionaryCreate(pInfo->allocator, nil, nil, 0, RSDictionaryRSTypeContext);
            }
            else
            {
                dict = RSDictionaryCreateMutable(pInfo->allocator, 0, RSDictionaryRSTypeContext);
            }
        }
        else
        {
//            RSIndex cnt = RSDictionaryGetCount(dict);
//            if (1 == cnt)
//            {
//                RSTypeRef val = RSDictionaryGetValue(dict, RSSTR("RS$UID"));
//                if (val && RSGetTypeID(val) == numbertype)
//                {
//                    RSTypeRef uid;
//                    uint32_t v;
//                    RSNumberGetValue((RSNumberRef)val, &v);
//                    uid = (RSTypeRef)_RSKeyedArchiverUIDCreate(pInfo->allocator, v);
//                    __RSPListRelease(dict, pInfo->allocator);
//                    *out = uid;
//                    return YES;
//                }
//            }
            if (-1 == allowImmutableCollections) checkImmutableCollections();
            if (1 == allowImmutableCollections)
            {
                if (pInfo->mutabilityOption == RSPropertyListImmutable)
                {
                    RSDictionaryRef newDict = RSCopy(pInfo->allocator, dict);
                    __RSPListRelease(dict, pInfo->allocator);
                    dict = (RSMutableDictionaryRef)newDict;
                }
            }
        }
        *out = dict;
        return YES;
    }
    
    return NO;
}

static BOOL parseDataTag(__RSPlistParserInfo *pInfo, RSTypeRef *out)
{
    const char *base = pInfo->curr;
    static const signed char dataDecodeTable[128] =
    {
        /* 000 */ -1, -1, -1, -1, -1, -1, -1, -1,
        /* 010 */ -1, -1, -1, -1, -1, -1, -1, -1,
        /* 020 */ -1, -1, -1, -1, -1, -1, -1, -1,
        /* 030 */ -1, -1, -1, -1, -1, -1, -1, -1,
        /* ' ' */ -1, -1, -1, -1, -1, -1, -1, -1,
        /* '(' */ -1, -1, -1, 62, -1, -1, -1, 63,
        /* '0' */ 52, 53, 54, 55, 56, 57, 58, 59,
        /* '8' */ 60, 61, -1, -1, -1,  0, -1, -1,
        /* '@' */ -1,  0,  1,  2,  3,  4,  5,  6,
        /* 'H' */  7,  8,  9, 10, 11, 12, 13, 14,
        /* 'P' */ 15, 16, 17, 18, 19, 20, 21, 22,
        /* 'X' */ 23, 24, 25, -1, -1, -1, -1, -1,
        /* '`' */ -1, 26, 27, 28, 29, 30, 31, 32,
        /* 'h' */ 33, 34, 35, 36, 37, 38, 39, 40,
        /* 'p' */ 41, 42, 43, 44, 45, 46, 47, 48,
        /* 'x' */ 49, 50, 51, -1, -1, -1, -1, -1
    };
    
    int tmpbufpos = 0;
    int tmpbuflen = 256;
    uint8_t *tmpbuf = pInfo->skip ? nil : (uint8_t *)RSAllocatorAllocate(pInfo->allocator, tmpbuflen);
    int numeq = 0;
    int acc = 0;
    int cntr = 0;
    
    for (; pInfo->curr < pInfo->end; pInfo->curr++)
    {
        signed char c = *(pInfo->curr);
        if (c == '<') {
            break;
        }
        if ('=' == c) {
            numeq++;
        } else if (!isspace(c)) {
            numeq = 0;
        }
        if (dataDecodeTable[c] < 0)
            continue;
        cntr++;
        acc <<= 6;
        acc += dataDecodeTable[c];
        if (!pInfo->skip && 0 == (cntr & 0x3))
        {
            if (tmpbuflen <= tmpbufpos + 2)
            {
                if (tmpbuflen < 256 * 1024)
                {
                    tmpbuflen *= 4;
                }
                else if (tmpbuflen < 16 * 1024 * 1024)
                {
                    tmpbuflen *= 2;
                }
                else
                {
                    // once in this stage, this will be really slow
                    // and really potentially fragment memory
                    tmpbuflen += 256 * 1024;
                }
                tmpbuf = (uint8_t *)RSAllocatorReallocate(pInfo->allocator, tmpbuf, tmpbuflen);
                if (!tmpbuf) __HALT(); // out of memory
            }
            tmpbuf[tmpbufpos++] = (acc >> 16) & 0xff;
            if (numeq < 2) tmpbuf[tmpbufpos++] = (acc >> 8) & 0xff;
            if (numeq < 1) tmpbuf[tmpbufpos++] = acc & 0xff;
        }
    }
    
    RSDataRef result = nil;
    if (!pInfo->skip)
    {
        if (pInfo->mutabilityOption == RSPropertyListMutableContainersAndLeaves)
        {
            result = (RSDataRef)RSDataCreateMutable(pInfo->allocator, 0);
            RSDataAppendBytes((RSMutableDataRef)result, tmpbuf, tmpbufpos);
            RSAllocatorDeallocate(pInfo->allocator, tmpbuf);
        }
        else
        {
            result = RSDataCreateWithNoCopy(pInfo->allocator, tmpbuf, tmpbufpos, YES, pInfo->allocator);
        }
        if (!result)
        {
            pInfo->curr = base;
            pInfo->error = __RSPropertyListCreateError(RSPropertyListReadCorruptError, RSSTR("Could not interpret <data> at line %d (should be base64-encoded)"), lineNumber(pInfo));
            return NO;
        }
    }
    
    if (checkForCloseTag(pInfo, RSXMLPlistTags[DATA_IX], DATA_TAG_LENGTH))
    {
        *out = result;
        return YES;
    }
    else
    {
        __RSPListRelease(result, pInfo->allocator);
        return NO;
    }
}

static BOOL parseRealTag(__RSPlistParserInfo *pInfo, RSTypeRef *out)
{
    RSStringRef str = nil;
    if (!parseStringTag(pInfo, &str))
    {
        if (!pInfo->error) pInfo->error = __RSPropertyListCreateError(RSPropertyListReadCorruptError, RSSTR("Encountered empty <real> on line %d"), lineNumber(pInfo));
        return NO;
    }
    
    RSNumberRef result = nil;
    
    if (!pInfo->skip)
    {
//        if (RSCompareEqualTo == RSStringCompare(str, RSSTR("nan"), RSCompareCaseInsensitive)) result = RSNumberNaN;
//        else if (RSCompareEqualTo == RSStringCompare(str, RSSTR("+infinity"), RSCompareCaseInsensitive)) result = RSNumberPositiveInfinity;
//        else if (RSCompareEqualTo == RSStringCompare(str, RSSTR("-infinity"), RSCompareCaseInsensitive)) result = RSNumberNegativeInfinity;
//        else if (RSCompareEqualTo == RSStringCompare(str, RSSTR("infinity"), RSCompareCaseInsensitive)) result = RSNumberPositiveInfinity;
//        else if (RSCompareEqualTo == RSStringCompare(str, RSSTR("-inf"), RSCompareCaseInsensitive)) result = RSNumberNegativeInfinity;
//        else if (RSCompareEqualTo == RSStringCompare(str, RSSTR("inf"), RSCompareCaseInsensitive)) result = RSNumberPositiveInfinity;
//        else if (RSCompareEqualTo == RSStringCompare(str, RSSTR("+inf"), RSCompareCaseInsensitive)) result = RSNumberPositiveInfinity;
        
        if (result)
        {
            RSRetain(result);
        }
        else
        {
//            RSIndex len = RSStringGetLength(str);
//            RSStringInlineBuffer buf;
//            RSStringInitInlineBuffer(str, &buf, RSRangeMake(0, len));
//            SInt32 idx = 0;
//            double val;
//            if (!__RSStringScanDouble(&buf, nil, &idx, &val) || idx != len)
//            {
//                __RSPListRelease(str, pInfo->allocator);
//                pInfo->error = __RSPropertyListCreateError(RSPropertyListReadCorruptError, RSSTR("Encountered misformatted real on line %d"), lineNumber(pInfo));
//                return NO;
//            }
            result = __plistRestoreNumber(pInfo->allocator, str, REAL_IX);
//            RSNumberCreate(pInfo->allocator, RSNumberDoubleType, &val);
        }
    }
    
    __RSPListRelease(str, pInfo->allocator);
    if (checkForCloseTag(pInfo, RSXMLPlistTags[REAL_IX], REAL_TAG_LENGTH))
    {
        *out = result;
        return YES;
    }
    else
    {
        __RSPListRelease(result, pInfo->allocator);
        return NO;
    }
}

RSInline BOOL read2DigitNumber(__RSPlistParserInfo *pInfo, int32_t *result)
{
    char ch1, ch2;
    if (pInfo->curr + 2 >= pInfo->end) return NO;
    ch1 = *pInfo->curr;
    ch2 = *(pInfo->curr + 1);
    pInfo->curr += 2;
    if (!isdigit(ch1) || !isdigit(ch2)) return NO;
    *result = (ch1 - '0')*10 + (ch2 - '0');
    return YES;
}

static BOOL parseDateTag(__RSPlistParserInfo *pInfo, RSTypeRef *out)
{
    int32_t year = 0, month = 0, day = 0, hour = 0, minute = 0, second = 0;
    int32_t num = 0;
    BOOL badForm = NO;
    BOOL yearIsNegative = NO;
    
    if (pInfo->curr < pInfo->end && *pInfo->curr == '-')
    {
        yearIsNegative = YES;
        pInfo->curr++;
    }
    
    while (pInfo->curr < pInfo->end && isdigit(*pInfo->curr))
    {
        year = 10*year + (*pInfo->curr) - '0';
        pInfo->curr ++;
    }
    if (pInfo->curr >= pInfo->end || *pInfo->curr != '-')
    {
        badForm = YES;
    }
    else
    {
        pInfo->curr ++;
    }
    
    if (!badForm && read2DigitNumber(pInfo, &month) && pInfo->curr < pInfo->end && *pInfo->curr == '-')
    {
        pInfo->curr ++;
    }
    else
    {
        badForm = YES;
    }
    
    if (!badForm && read2DigitNumber(pInfo, &day) && pInfo->curr < pInfo->end && *pInfo->curr == 'T')
    {
        pInfo->curr ++;
    }
    else
    {
        badForm = YES;
    }
    
    if (!badForm && read2DigitNumber(pInfo, &hour) && pInfo->curr < pInfo->end && *pInfo->curr == ':')
    {
        pInfo->curr ++;
    }
    else
    {
        badForm = YES;
    }
    
    if (!badForm && read2DigitNumber(pInfo, &minute) && pInfo->curr < pInfo->end && *pInfo->curr == ':')
    {
        pInfo->curr ++;
    }
    else
    {
        badForm = YES;
    }
    
    if (!badForm && read2DigitNumber(pInfo, &num) && pInfo->curr < pInfo->end && *pInfo->curr == 'Z')
    {
        second = num;
        pInfo->curr ++;
    }
    else
    {
        badForm = YES;
    }
    
    if (badForm || !checkForCloseTag(pInfo, RSXMLPlistTags[DATE_IX], DATE_TAG_LENGTH))
    {
        pInfo->error = __RSPropertyListCreateError(RSPropertyListReadCorruptError, RSSTR("Could not interpret <date> at line %d"), lineNumber(pInfo));
        return NO;
    }
    
    RSAbsoluteTime at = 0.0;
#if 1
    RSGregorianDate date = {yearIsNegative ? -year : year, month, day, hour, minute, second};
    at = RSGregorianDateGetAbsoluteTime(date, nil);
#else
    // this doesn't work
    RSCalendarRef calendar = RSCalendarCreateWithIdentifier(RSAllocatorSystemDefault, RSCalendarIdentifierGregorian);
    RSTimeZoneRef tz = RSTimeZoneCreateWithName(RSAllocatorSystemDefault, RSSTR("GMT"), YES);
    RSCalendarSetTimeZone(calendar, tz);
    RSCalendarComposeAbsoluteTime(calendar, &at, (const uint8_t *)"yMdHms", year, month, day, hour, minute, second);
    RSRelease(calendar);
    RSRelease(tz);
#endif
    if (pInfo->skip)
    {
        *out = nil;
    }
    else
    {
        *out = RSDateCreate(pInfo->allocator, at);
    }
    return YES;
}

static void parseCDSect_pl(__RSPlistParserInfo *pInfo, RSMutableDataRef stringData)
{
    const char *end, *begin;
    if (pInfo->end - pInfo->curr < CDSECT_TAG_LENGTH)
    {
        pInfo->error = __RSPropertyListCreateError(RSPropertyListReadCorruptError, RSSTR("Encountered unexpected EOF"));
        return;
    }
    if (memcmp(pInfo->curr, RSXMLPlistTags[CDSECT_IX], CDSECT_TAG_LENGTH))
    {
        pInfo->error = __RSPropertyListCreateError(RSPropertyListReadCorruptError, RSSTR("Encountered improper CDATA opening at line %d"), lineNumber(pInfo));
        return;
    }
    pInfo->curr += CDSECT_TAG_LENGTH;
    begin = pInfo->curr; // Marks the first character of the CDATA content
    end = pInfo->end-2; // So we can safely look 2 characters beyond p
    while (pInfo->curr < end)
    {
        if (*(pInfo->curr) == ']' && *(pInfo->curr+1) == ']' && *(pInfo->curr+2) == '>')
        {
            // Found the end!
            RSDataAppendBytes(stringData, (const UInt8 *)begin, pInfo->curr-begin);
            pInfo->curr += 3;
            return;
        }
        pInfo->curr ++;
    }
    // Never found the end mark
    pInfo->curr = begin;
    pInfo->error = __RSPropertyListCreateError(RSPropertyListReadCorruptError, RSSTR("Could not find end of CDATA started on line %d"), lineNumber(pInfo));
}

// Only legal references are {lt, gt, amp, apos, quote, #ddd, #xAAA}
static void parseEntityReference_pl(__RSPlistParserInfo *pInfo, RSMutableDataRef stringData)
{
    int len;
    pInfo->curr ++; // move past the '&';
    len = (int)(pInfo->end - pInfo->curr); // how many bytes we can safely scan
    if (len < 1)
    {
        pInfo->error = __RSPropertyListCreateError(RSPropertyListReadCorruptError, RSSTR("Encountered unexpected EOF"));
        return;
    }
    
    char ch;
    switch (*(pInfo->curr))
    {
        case 'l':  // "lt"
            if (len >= 3 && *(pInfo->curr+1) == 't' && *(pInfo->curr+2) == ';')
            {
                ch = '<';
                pInfo->curr += 3;
                break;
            }
            pInfo->error = __RSPropertyListCreateError(RSPropertyListReadCorruptError, RSSTR("Encountered unknown ampersand-escape sequence at line %d"), lineNumber(pInfo));
            return;
        case 'g': // "gt"
            if (len >= 3 && *(pInfo->curr+1) == 't' && *(pInfo->curr+2) == ';')
            {
                ch = '>';
                pInfo->curr += 3;
                break;
            }
            pInfo->error = __RSPropertyListCreateError(RSPropertyListReadCorruptError, RSSTR("Encountered unknown ampersand-escape sequence at line %d"), lineNumber(pInfo));
            return;
        case 'a': // "apos" or "amp"
            if (len < 4)
            {
                // Not enough characters for either conversion
                pInfo->error = __RSPropertyListCreateError(RSPropertyListReadCorruptError, RSSTR("Encountered unexpected EOF"));
                return;
            }
            if (*(pInfo->curr+1) == 'm')
            {
                // "amp"
                if (*(pInfo->curr+2) == 'p' && *(pInfo->curr+3) == ';') {
                    ch = '&';
                    pInfo->curr += 4;
                    break;
                }
            }
            else if (*(pInfo->curr+1) == 'p')
            {
                // "apos"
                if (len > 4 && *(pInfo->curr+2) == 'o' && *(pInfo->curr+3) == 's' && *(pInfo->curr+4) == ';') {
                    ch = '\'';
                    pInfo->curr += 5;
                    break;
                }
            }
            pInfo->error = __RSPropertyListCreateError(RSPropertyListReadCorruptError, RSSTR("Encountered unknown ampersand-escape sequence at line %d"), lineNumber(pInfo));
            return;
        case 'q':  // "quote"
            if (len >= 5 && *(pInfo->curr+1) == 'u' && *(pInfo->curr+2) == 'o' && *(pInfo->curr+3) == 't' && *(pInfo->curr+4) == ';')
            {
                ch = '\"';
                pInfo->curr += 5;
                break;
            }
            pInfo->error = __RSPropertyListCreateError(RSPropertyListReadCorruptError, RSSTR("Encountered unknown ampersand-escape sequence at line %d"), lineNumber(pInfo));
            return;
        case '#':
        {
            uint16_t num = 0;
            BOOL isHex = NO;
            if ( len < 4)
            {
                // Not enough characters to make it all fit!  Need at least "&#d;"
                pInfo->error = __RSPropertyListCreateError(RSPropertyListReadCorruptError, RSSTR("Encountered unexpected EOF"));
                return;
            }
            pInfo->curr ++;
            if (*(pInfo->curr) == 'x')
            {
                isHex = YES;
                pInfo->curr ++;
            }
            while (pInfo->curr < pInfo->end)
            {
                ch = *(pInfo->curr);
                pInfo->curr ++;
                if (ch == ';')
                {
                    // The value in num always refers to the unicode code point. We'll have to convert since the calling function expects UTF8 data.
                    RSStringRef oneChar = RSStringCreateWithBytes(pInfo->allocator, (const uint8_t *)&num, 2, __RSDefaultEightBitStringEncoding, NO);
                    UniChar tmpBuf[6]; // max of 6 bytes for UTF8
                    RSIndex tmpBufLength = 0;
                    RSStringGetCharacters(oneChar, RSMakeRange(1, 6), (UniChar*)tmpBuf);
//                    RSStringGetBytes(oneChar, RSRangeMake(0, 1), RSStringEncodingUTF8, 0, NO, tmpBuf, 6, &tmpBufLength);
                    RSDataAppendBytes(stringData, tmpBuf, tmpBufLength);
                    __RSPListRelease(oneChar, pInfo->allocator);
                    return;
                }
                if (!isHex) num = num*10;
                else num = num << 4;
                if (ch <= '9' && ch >= '0')
                {
                    num += (ch - '0');
                }
                else if (!isHex)
                {
                    pInfo->error = __RSPropertyListCreateError(RSPropertyListReadCorruptError, RSSTR("Encountered unexpected character %c at line %d while parsing data"), ch, lineNumber(pInfo));
                    return;
                }
                else if (ch >= 'a' && ch <= 'f')
                {
                    num += 10 + (ch - 'a');
                }
                else if (ch >= 'A' && ch <= 'F')
                {
                    num += 10 + (ch - 'A');
                }
                else
                {
                    pInfo->error = __RSPropertyListCreateError(RSPropertyListReadCorruptError, RSSTR("Encountered unexpected character %c at line %d while parsing data"), ch, lineNumber(pInfo));
                    return;
                }
            }
            pInfo->error = __RSPropertyListCreateError(RSPropertyListReadCorruptError, RSSTR("Encountered unexpected EOF"));
            return;
        }
        default:
            pInfo->error = __RSPropertyListCreateError(RSPropertyListReadCorruptError, RSSTR("Encountered unknown ampersand-escape sequence at line %d"), lineNumber(pInfo));
            return;
    }
    RSDataAppendBytes(stringData, (const UInt8 *)&ch, 1);
}

static RSStringRef _uniqueStringForUTF8Bytes(__RSPlistParserInfo *pInfo, const char *base, RSIndex length)
{
    if (length == 0)
        return RSSTR("");
    
    RSStringRef result = nil;
//    RSIndex payload = 0;
//    BOOL uniqued = RSBurstTrieContainsUTF8String(pInfo->stringTrie, (UInt8 *)base, length, &payload);
//    if (uniqued)
//    {
//        result = (RSStringRef)RSArrayGetValueAtIndex(pInfo->stringCache, (RSIndex)payload);
//        if (!_RSAllocatorIsGCRefZero(pInfo->allocator)) RSRetain(result);
//    }
//    else
//    {
//        result = RSStringCreateWithBytes(pInfo->allocator, (const UInt8 *)base, length, RSStringEncodingUTF8, NO);
//        if (!result) return nil;
//        payload = RSArrayGetCount(pInfo->stringCache);
//        RSArrayAppendValue(pInfo->stringCache, result);
//        //RSBurstTrieAddUTF8String(pInfo->stringTrie, (UInt8 *)base, length, payload);
//    }
    result = RSStringCreateWithBytes(pInfo->allocator, (const RSBitU8 *)base, length, __RSDefaultEightBitStringEncoding, NO);
    return result;
}

static BOOL parseStringTag(__RSPlistParserInfo *pInfo, RSStringRef *out)
{
    const char *mark = pInfo->curr;
    RSMutableDataRef stringData = nil;
    while (!pInfo->error && pInfo->curr < pInfo->end)
    {
        char ch = *(pInfo->curr);
        if (ch == '<')
        {
            if (pInfo->curr + 1 >= pInfo->end) break;
            // Could be a CDSect; could be the end of the string
            if (*(pInfo->curr+1) != '!') break; // End of the string
            if (!stringData) stringData = RSDataCreateMutable(pInfo->allocator, 0);
            RSDataAppendBytes(stringData, (const UInt8 *)mark, pInfo->curr - mark);
            parseCDSect_pl(pInfo, stringData); // TODO: move to return BOOL
            mark = pInfo->curr;
        }
        else if (ch == '&')
        {
            if (!stringData) stringData = RSDataCreateMutable(pInfo->allocator, 0);
            if (pInfo->curr - mark > 0) RSDataAppendBytes(stringData, (const UInt8 *)mark, pInfo->curr - mark);
            parseEntityReference_pl(pInfo, stringData); // TODO: move to return BOOL
            mark = pInfo->curr;
        }
        else
        {
            pInfo->curr ++;
        }
    }
    
    if (pInfo->error)
    {
        __RSPListRelease(stringData, pInfo->allocator);
        return NO;
    }
    
    if (!stringData)
    {
        if (pInfo->skip)
        {
            *out = nil;
        }
        else
        {
            if (pInfo->mutabilityOption != RSPropertyListMutableContainersAndLeaves)
            {
                RSStringRef s = _uniqueStringForUTF8Bytes(pInfo, mark, pInfo->curr - mark);
                if (!s)
                {
                    pInfo->error = __RSPropertyListCreateError(RSPropertyListReadCorruptError, RSSTR("Unable to convert string to correct encoding"));
                    return NO;
                }
                *out = s;
            }
            else
            {
//                RSStringRef s = RSStringCreateWithBytes(pInfo->allocator, (const UInt8 *)mark, pInfo->curr - mark, RSStringEncodingUTF8);
                RSStringRef s = _uniqueStringForUTF8Bytes(pInfo, mark, pInfo->curr - mark);
                if (!s) {
                    pInfo->error = __RSPropertyListCreateError(RSPropertyListReadCorruptError, RSSTR("Unable to convert string to correct encoding"));
                    return NO;
                }
                *out = RSStringCreateMutableCopy(pInfo->allocator, 0, s);
                __RSPListRelease(s, pInfo->allocator);
            }
        }
        return YES;
    }
    else
    {
        if (pInfo->skip)
        {
            *out = nil;
        }
        else
        {
            if (pInfo->curr - mark > 0) RSDataAppendBytes(stringData, (const UInt8 *)mark, pInfo->curr - mark);
            if (pInfo->mutabilityOption != RSPropertyListMutableContainersAndLeaves)
            {
                RSStringRef s = _uniqueStringForUTF8Bytes(pInfo, (const char *)RSDataGetBytesPtr(stringData), RSDataGetLength(stringData));
                if (!s)
                {
                    pInfo->error = __RSPropertyListCreateError(RSPropertyListReadCorruptError, RSSTR("Unable to convert string to correct encoding"));
                    return NO;
                }
                *out = s;
            }
            else
            {
                RSStringRef s = RSStringCreateWithBytes(pInfo->allocator, (const UInt8 *)RSDataGetBytesPtr(stringData), RSDataGetLength(stringData), __RSDefaultEightBitStringEncoding, NO);
                if (!s)
                {
                    pInfo->error = __RSPropertyListCreateError(RSPropertyListReadCorruptError, RSSTR("Unable to convert string to correct encoding"));
                    return NO;
                }
                *out = RSStringCreateMutableCopy(pInfo->allocator, 0, s);
                __RSPListRelease(s, pInfo->allocator);
            }
        }
        __RSPListRelease(stringData, pInfo->allocator);
        return YES;
    }
}

static BOOL parseXMLElement(__RSPlistParserInfo *pInfo, BOOL *isKey, RSTypeRef *out)
{
    const char *marker = pInfo->curr;
    int markerLength = -1;
    BOOL isEmpty;
    int markerIx = -1;
    
    if (isKey) *isKey = NO;
    while (pInfo->curr < pInfo->end)
    {
        char ch = *(pInfo->curr);
        if (ch == ' ' || ch ==  '\t' || ch == '\n' || ch =='\r')
        {
            if (markerLength == -1) markerLength = (int)(pInfo->curr - marker);
        }
        else if (ch == '>')
        {
            break;
        }
        pInfo->curr ++;
    }
    if (pInfo->curr >= pInfo->end) return NO;
    isEmpty = (*(pInfo->curr-1) == '/');
    if (markerLength == -1)
        markerLength = (int)(pInfo->curr - (isEmpty ? 1 : 0) - marker);
    pInfo->curr ++; // Advance past '>'
    if (markerLength == 0)
    {
        // Back up to the beginning of the marker
        pInfo->curr = marker;
        pInfo->error = __RSPropertyListCreateError(RSPropertyListReadCorruptError, RSSTR("Malformed tag on line %d"), lineNumber(pInfo));
        return NO;
    }
    switch (*marker)
    {
        case 'a':   // Array
            if (markerLength == ARRAY_TAG_LENGTH && !memcmp(marker, RSXMLPlistTags[ARRAY_IX], ARRAY_TAG_LENGTH))
                markerIx = ARRAY_IX;
            break;
        case 'd': // Dictionary, data, or date; Fortunately, they all have the same marker length....
            if (markerLength != DICT_TAG_LENGTH)
                break;
            if (!memcmp(marker, RSXMLPlistTags[DICT_IX], DICT_TAG_LENGTH))
                markerIx = DICT_IX;
            else if (!memcmp(marker, RSXMLPlistTags[DATA_IX], DATA_TAG_LENGTH))
                markerIx = DATA_IX;
            else if (!memcmp(marker, RSXMLPlistTags[DATE_IX], DATE_TAG_LENGTH))
                markerIx = DATE_IX;
            break;
        case 'f': // NO (BOOL)
            if (markerLength == FALSE_TAG_LENGTH && !memcmp(marker, RSXMLPlistTags[FALSE_IX], FALSE_TAG_LENGTH)) {
                markerIx = FALSE_IX;
            }
            break;
        case 'i': // integer
            if (markerLength == INTEGER_TAG_LENGTH && !memcmp(marker, RSXMLPlistTags[INTEGER_IX], INTEGER_TAG_LENGTH))
                markerIx = INTEGER_IX;
            break;
        case 'k': // Key of a dictionary
            if (markerLength == KEY_TAG_LENGTH && !memcmp(marker, RSXMLPlistTags[KEY_IX], KEY_TAG_LENGTH)) {
                markerIx = KEY_IX;
                if (isKey) *isKey = YES;
            }
            break;
        case 'p': // Plist
            if (markerLength == PLIST_TAG_LENGTH && !memcmp(marker, RSXMLPlistTags[PLIST_IX], PLIST_TAG_LENGTH))
                markerIx = PLIST_IX;
            break;
        case 'r': // real
            if (markerLength == REAL_TAG_LENGTH && !memcmp(marker, RSXMLPlistTags[REAL_IX], REAL_TAG_LENGTH))
                markerIx = REAL_IX;
            break;
        case 's': // String
            if (markerLength == STRING_TAG_LENGTH && !memcmp(marker, RSXMLPlistTags[STRING_IX], STRING_TAG_LENGTH))
                markerIx = STRING_IX;
            break;
        case 't': // YES (BOOL)
            if (markerLength == TRUE_TAG_LENGTH && !memcmp(marker, RSXMLPlistTags[TRUE_IX], TRUE_TAG_LENGTH))
                markerIx = TRUE_IX;
            break;
    }
    
    if (!pInfo->allowNewTypes && markerIx != PLIST_IX && markerIx != ARRAY_IX && markerIx != DICT_IX && markerIx != STRING_IX && markerIx != KEY_IX && markerIx != DATA_IX)
    {
        pInfo->error = __RSPropertyListCreateError(RSPropertyListReadCorruptError, RSSTR("Encountered new tag when expecting only old-style property list objects"));
        return NO;
    }
    
    switch (markerIx)
    {
        case PLIST_IX:
            if (isEmpty)
            {
                pInfo->error = __RSPropertyListCreateError(RSPropertyListReadCorruptError, RSSTR("Encountered empty plist tag"));
                return NO;
            }
            return parsePListTag(pInfo, out);
        case ARRAY_IX:
            if (isEmpty)
            {
                if (pInfo->skip)
                {
                    *out = nil;
                }
                else
                {
                    if (pInfo->mutabilityOption == RSPropertyListImmutable)
                    {
                        *out = RSArrayCreate(pInfo->allocator, nil);
                    }
                    else
                    {
                        *out = RSArrayCreateMutable(pInfo->allocator, 0);
                    }
                }
                return YES;
            }
            else
            {
                return parseArrayTag(pInfo, out);
            }
        case DICT_IX:
            if (isEmpty)
            {
                if (pInfo->skip)
                {
                    *out = nil;
                }
                else
                {
                    if (pInfo->mutabilityOption == RSPropertyListImmutable)
                    {
                        *out = RSDictionaryCreate(pInfo->allocator, nil, nil, 0, RSDictionaryRSTypeContext);
                    }
                    else
                    {
                        *out = RSDictionaryCreateMutable(pInfo->allocator, 0, RSDictionaryRSTypeContext);
                    }
                }
                return YES;
            }
            else
            {
                return parseDictTag(pInfo, out);
            }
        case KEY_IX:
        case STRING_IX:
        {
            int tagLen = (markerIx == KEY_IX) ? KEY_TAG_LENGTH : STRING_TAG_LENGTH;
            if (isEmpty)
            {
                if (pInfo->skip)
                {
                    *out = nil;
                }
                else
                {
                    if (pInfo->mutabilityOption == RSPropertyListMutableContainersAndLeaves)
                    {
                        *out = RSStringCreateMutable(pInfo->allocator, 0);
                    }
                    else
                    {
                        *out = RSStringCreateWithCharacters(pInfo->allocator, nil, 0);
                    }
                }
                return YES;
            }
            if (!parseStringTag(pInfo, (RSStringRef *)out))
            {
                return NO; // parseStringTag will already have set the error string
            }
            if (!checkForCloseTag(pInfo, RSXMLPlistTags[markerIx], tagLen))
            {
                __RSPListRelease(*out, pInfo->allocator);
                return NO;
            }
            else
            {
                return YES;
            }
        }
        case DATA_IX:
            if (isEmpty)
            {
                pInfo->error = __RSPropertyListCreateError(RSPropertyListReadCorruptError, RSSTR("Encountered empty <data> on line %d"), lineNumber(pInfo));
                return NO;
            }
            else
            {
                return parseDataTag(pInfo, out);
            }
        case DATE_IX:
            if (isEmpty)
            {
                pInfo->error = __RSPropertyListCreateError(RSPropertyListReadCorruptError, RSSTR("Encountered empty <date> on line %d"), lineNumber(pInfo));
                return NO;
            }
            else
            {
                return parseDateTag(pInfo, out);
            }
        case TRUE_IX:
            if (!isEmpty)
            {
                if (!checkForCloseTag(pInfo, RSXMLPlistTags[TRUE_IX], TRUE_TAG_LENGTH))
                {
                    pInfo->error = __RSPropertyListCreateError(RSPropertyListReadCorruptError, RSSTR("Encountered non-empty <YES> on line %d"), lineNumber(pInfo));
                    return NO;
                }
            }
            if (pInfo->skip)
            {
                *out = nil;
            }
            else
            {
                *out = RSRetain(RSBooleanTrue);
            }
            return YES;
        case FALSE_IX:
            if (!isEmpty)
            {
                if (!checkForCloseTag(pInfo, RSXMLPlistTags[FALSE_IX], FALSE_TAG_LENGTH))
                {
                    pInfo->error = __RSPropertyListCreateError(RSPropertyListReadCorruptError, RSSTR("Encountered non-empty <NO> on line %d"), lineNumber(pInfo));
                    return NO;
                }
            }
            if (pInfo->skip)
            {
                *out = nil;
            }
            else
            {
                *out = RSRetain(RSBooleanFalse);
            }
            return YES;
        case REAL_IX:
            if (isEmpty)
            {
                pInfo->error = __RSPropertyListCreateError(RSPropertyListReadCorruptError, RSSTR("Encountered empty <real> on line %d"), lineNumber(pInfo));
                return NO;
            }
            else
            {
                return parseRealTag(pInfo, out);
            }
        case INTEGER_IX:
            if (isEmpty)
            {
                pInfo->error = __RSPropertyListCreateError(RSPropertyListReadCorruptError, RSSTR("Encountered empty <integer> on line %d"), lineNumber(pInfo));
                return NO;
            }
            else
            {
                return parseIntegerTag(pInfo, out);
            }
        default:
        {
            RSStringRef markerStr = RSStringCreateWithBytes(RSAllocatorSystemDefault, (const UInt8 *)marker, markerLength, __RSDefaultEightBitStringEncoding, NO);
            pInfo->curr = marker;
            pInfo->error = __RSPropertyListCreateError(RSPropertyListReadCorruptError, RSSTR("Encountered unknown tag %@ on line %d"), markerStr ? markerStr : RSSTR("<unknown>"), lineNumber(pInfo));
            if (markerStr) RSRelease(markerStr);
            return NO;
        }
    }
}

static BOOL parseXMLPropertyList(__RSPlistParserInfo *pInfo, RSTypeRef *out)
{
    while (!pInfo->error && pInfo->curr < pInfo->end)
    {
        UniChar ch;
        skipWhitespace(pInfo);
        if (pInfo->curr+1 >= pInfo->end)
        {
            pInfo->error = __RSPropertyListCreateError(RSPropertyListReadCorruptError, RSSTR("No XML content found"));
            return NO;
        }
        if (*(pInfo->curr) != '<')
        {
            pInfo->error = __RSPropertyListCreateError(RSPropertyListReadCorruptError, RSSTR("Unexpected character %c at line %d"), *(pInfo->curr), lineNumber(pInfo));
            return NO;
        }
        ch = *(++ pInfo->curr);
        if (ch == '!')
        {
            // Comment or DTD
            ++ pInfo->curr;
            if (pInfo->curr+1 < pInfo->end && *pInfo->curr == '-' && *(pInfo->curr+1) == '-')
            {
                // Comment
                pInfo->curr += 2;
                skipXMLComment(pInfo);
            }
            else
            {
                skipDTD(pInfo);
            }
        }
        else if (ch == '?')
        {
            // Processing instruction
            pInfo->curr++;
            skipXMLProcessingInstruction(pInfo);
        }
        else
        {
            // Tag or malformed
            return parseXMLElement(pInfo, nil, out);
            // Note we do not verify that there was only one element, so a file that has garbage after the first element will nonetheless successfully parse
        }
    }
    // Should never get here
    if (!(pInfo->error))
    {
        pInfo->error = __RSPropertyListCreateError(RSPropertyListReadCorruptError, RSSTR("Encountered unexpected EOF"));
    }
    return NO;
}

#pragma mark New Version End
#pragma mark -

typedef struct __RSPropertyListParseContext
{
    RSMutableArrayRef passStack;
    BOOL shouldParseHeader;
    BOOL previousTagIsKEYIX;
    char padding[2];
}__RSPropertyListParseContext;
static RSTypeRef __RSPropertyListCore0(RSDataRef plistData, RSIndex* offset,__RSPropertyListParseContext* context);
static RSTypeRef __RSPropertyListCore1(RSDataRef plistData, RSIndex* offset,__RSPropertyListParseContext* context);

static RSTypeRef __RSPropertyListCore1(RSDataRef plistData, RSIndex* offset,__RSPropertyListParseContext* context)
{
    RSMutableArrayRef _plist = nil;
    
    if (nil == plistData)
        return nil;
    UniChar* data = (UniChar*)RSDataGetBytesPtr(plistData);
    RSIndex cnt = RSDataGetLength(plistData);
    if (data == nil || cnt == 0) return nil;
    RSIndex offsetHeader = 0, idx = 0;
    if (context && context->shouldParseHeader)
    {
        if (NO == __RSPropertyListParseHeader(plistData, &offsetHeader))
        {
            return nil;
        }
        idx = offsetHeader;
    }
    else _plist = RSArrayCreateMutable(RSAllocatorSystemDefault, 0);
    if (offset) idx += *offset;
    
    BOOL inTag = NO;
    BOOL inEndTag = NO;
    BOOL inSelfEndTag = NO;
    BOOL inQuote = NO;
    
    BOOL previousTagIsKEYIX = YES;
    BOOL shouldFlushCTD = NO;
    UniChar c = 0;
    RSMutableStringRef  tagName = RSStringCreateMutable(RSAllocatorSystemDefault, 0);
    //RSMutableStringRef key = RSStringCreateMutable(RSAllocatorSystemDefault, 0);
    RSTypeRef value = nil;
    int tagID, previousTagID = 0;
    int currentTagID = -1;
    RSMutableArrayRef tagIDPairStack = context ? (context->passStack ? (RSMutableArrayRef)RSRetain(context->passStack) : RSArrayCreateMutable(RSAllocatorSystemDefault, 64)) : RSArrayCreateMutable(RSAllocatorSystemDefault, 64);
    RSNumberRef tagIDRef = nil;     // current tag id
    RSNumberRef preTagIDRef = nil;  // stack top tag id
    
    BOOL shouldContinue = NO;
    for (; idx < cnt || shouldContinue ; idx++)
    {
        if (shouldFlushCTD) currentTagID = -1;
        shouldFlushCTD = NO;
        if (likely(NO == shouldContinue)) c = data[idx];
        switch (c)
        {
            case '\"':
                inQuote = !inQuote;
                break;
            case '<':
                if (NO == inQuote)
                {
                    inTag = YES;
                    if (data[idx+1] == '/')
                    {
                        // the tag is an end tag
                    }
                }
                break;
            case '>':
                if (NO == inQuote)
                {
                    inTag = NO;
                    currentTagID = tagID = __RSPropertyListTagID(tagName);
                    tagIDRef = __RSPropertyListTagNumberWithID(tagID);
                    
                    if (NO == inEndTag)
                    {
                        preTagIDRef = RSArrayLastObject(tagIDPairStack);
                        if (preTagIDRef)
                        {
                            RSNumberGetValue(preTagIDRef, &previousTagID);
                        }
                        else previousTagID = -1;
                        RSArrayAddObject(tagIDPairStack, tagIDRef); // push to stack
                        switch (tagID)
                        {
                            case PLIST_IX:
                                if (previousTagID != -1)
                                {
                                    __RSPlistHaltHandler("xml is not available", nil);
                                    goto PLIST_PARSE_RELEASE;
                                }
                                previousTagIsKEYIX = NO;
                                break;
                            case ARRAY_IX:
                                if (previousTagID == ARRAY_IX || previousTagIsKEYIX)
                                {
                                    RSIndex _idx = idx + 1;
                                    __RSPropertyListParseContext context = {tagIDPairStack, NO, previousTagIsKEYIX, {0,0}};
                                    //__RSCLog(RSLogLevelDebug, "%s\n", "entry __RSPropertyListCore1");
                                    if (value) RSRelease(value);
                                    value = __RSPropertyListCore1(plistData, &_idx, &context);
                                    //__RSCLog(RSLogLevelDebug, "%s\n", "leave __RSPropertyListCore1");
                                    idx = _idx;
                                    goto UPDATE_BLOCK;
                                }
                                else
                                {
                                    __RSPlistHaltHandler("xml is not available", nil);
                                    goto PLIST_PARSE_RELEASE;
                                }
                                break;
                            case DICT_IX:
                                if (previousTagIsKEYIX)
                                {
                                    RSIndex _idx = idx + 1;
                                    __RSPropertyListParseContext context = {tagIDPairStack, NO, previousTagIsKEYIX, {0,0}};
                                    //__RSCLog(RSLogLevelDebug, "%s\n", "entry __RSPropertyListCore0");
                                    if (value) RSRelease(value);
                                    value = __RSPropertyListCore0(plistData, &_idx, &context);
                                    //__RSCLog(RSLogLevelDebug, "%s\n", "leave __RSPropertyListCore0");
                                    idx = _idx;
                                    goto UPDATE_BLOCK;
                                }
                                else if (previousTagID == PLIST_IX)
                                {
                                    _plist = RSArrayCreateMutable(RSAllocatorSystemDefault, 0);
                                }
                                else
                                {
                                    __RSPlistHaltHandler("dict is not available", nil);
                                    goto PLIST_PARSE_RELEASE;
                                }
                                break;
                            case KEY_IX:
                                __RSPlistHaltHandler("the key should not be in an array.", nil);
                                goto PLIST_PARSE_RELEASE;
                                if (previousTagIsKEYIX) return __RSPlistHaltHandler("the previous tag is not key.", nil);
                                //if (key) RSStringReplace(key, RSMakeRange(0, RSStringGetLength(key)), RSSTR(""));
                                previousTagIsKEYIX = YES;
                                break;
                                
                            case STRING_IX:
                            case DATE_IX:
                            case DATA_IX:
                            case REAL_IX:
                            case INTEGER_IX:
                            case TRUE_IX:
                            case FALSE_IX:
                                if (previousTagID == ARRAY_IX || previousTagIsKEYIX)
                                {
                                    if (value != nil) RSRelease(value);
                                    value = nil;
                                    value = RSStringCreateMutable(RSAllocatorSystemDefault, 32);
                                }
                                else
                                {
                                    __RSPlistHaltHandler("the previous tag is not array.", nil);
                                    goto PLIST_PARSE_RELEASE;
                                }
                                break;
                            default:
                                __RSPlistHaltHandler("the tag is not defined.", nil);
                                goto PLIST_PARSE_RELEASE;
                                break;
                        } // switch (currentTagID)
                    } // if (NO == inEndTag)
                    else
                    {
                        inEndTag = NO;
                        shouldFlushCTD = YES;
                        RSNumberRef _tagPare = RSArrayLastObject(tagIDPairStack);
                        int _id = -1;
                        if (nil == _tagPare) goto UPDATE_BLOCK;
                        if (NO == inSelfEndTag)
                        {
                            if (RSCompareEqualTo == RSNumberCompare(_tagPare, tagIDRef, nil))
                            {
                                RSNumberGetValue(_tagPare, &_id);
                                RSArrayRemoveLastObject(tagIDPairStack);
                                //previousTagIsKEYIX = (_id == KEY_IX);
                            }
                            else
                            {
                                __RSPlistHaltHandler("", nil);
                                goto PLIST_PARSE_RELEASE;
                            }
                        }
                        else
                        {
                            if (value) RSRelease(value);
                            switch (currentTagID)
                            {
                                case DICT_IX:
                                    value = RSDictionaryCreateWithObjectsAndOKeys(RSAllocatorSystemDefault, nil);
                                    break;
                                case ARRAY_IX:
                                    value = RSArrayCreateWithObject(RSAllocatorSystemDefault, nil);
                                default:
                                    break;
                            }
                        }
                        // update the KVC
                        if (ARRAY_IX == _id && inSelfEndTag == NO)
                        {
                            RSRelease(tagIDRef);
                            goto END;
                        }
                        inSelfEndTag = NO;
                        if (previousTagIsKEYIX && _id != KEY_IX)
                        {
                        UPDATE_BLOCK:
                            //previousTagIsKEYIX = NO;
                            
                            //RSStringRef _k = __RSNormalStringFromXMLFormat(key);
                            RSNumberGetValue(tagIDRef, &currentTagID);
                            RSTypeRef _v = __RSPropertyListCopyValue(RSAllocatorSystemDefault, value, currentTagID);
                            if (_v) RSArrayAddObject(_plist, _v);//RSDictionarySetValue(_plist, _k, _v);
                            else //__RSCLog(RSLogLevelNotice, "__RSPropertyListCopyValue return nil!\n");
                            //RSRelease(_k);
                            RSRelease(_v);
                            //if (value) RSStringReplace((RSMutableStringRef)value, RSMakeRange(0, RSStringGetLength(value)), RSStringGetEmptyString());
                            if (value) RSRelease(value);
                            value = nil;
                            //previousTagIsKEYIX = NO;
                        }
                    }
                    RSRelease(tagIDRef);
                    //RSStringReplace(tagName, RSMakeRange(0, RSStringGetLength(tagName)), RSSTR(""));
                    RSRelease(tagName);
                    tagName = RSStringCreateMutable(RSAllocatorSystemDefault, 128);
                }
                else
                {
                }
                break;
            case '/':
                if (inTag)
                {
                    inEndTag = YES;
                    if (data[idx+1] == '>')
                    {
                        // the tag is an end tag
                        inSelfEndTag = YES;
                    }
                    break;
                }
            default:
                if (YES == inQuote)
                {
                    break;
                }
                if (YES == inTag)
                {
                    RSStringAppendCharacters(tagName, &c, 1);
                }
                else
                {
                    switch (currentTagID)
                    {
                        case -1:
                            break;
                        case KEY_IX:
                            //RSStringAppendUTF8Characters(key, &c, 1);
                            break;
                            
                        case STRING_IX:
                        case DATE_IX:
                        case DATA_IX:
                        case REAL_IX:
                        case INTEGER_IX:
                        case TRUE_IX:
                        case FALSE_IX:
                            RSStringAppendCharacters((RSMutableStringRef)value, &c, 1);
                            break;
                        case ARRAY_IX:
                            break;
                        case DICT_IX:
                            break;
                        default:
                            break;
                    }
                }
                break;  //default
        }//switch (c)
    }
PLIST_PARSE_RELEASE:
END:
    RSRelease(tagIDPairStack);
    RSRelease(tagName);
    //RSRelease(key);
    RSRelease(value);
    if (offset) *offset = idx;
    return _plist;
}


static RSTypeRef __RSPropertyListCore0 (RSDataRef plistData, RSIndex* offset,__RSPropertyListParseContext* context)
{
    RSMutableDictionaryRef _plist = nil;
    
    if (nil == plistData)
        return nil;
    UniChar* data = (UniChar*)RSDataGetBytesPtr(plistData);
    RSIndex cnt = RSDataGetLength(plistData);
    if (data == nil || cnt == 0) return nil;
    RSIndex offsetHeader = 0, idx = 0;
    if (context && context->shouldParseHeader)
    {
        if (NO == __RSPropertyListParseHeader(plistData, &offsetHeader))
        {
            return nil;
        }
        idx = offsetHeader;
    }
    else _plist = RSDictionaryCreateMutable(RSAllocatorSystemDefault, 0, RSDictionaryRSTypeContext);
    if (offset) idx += *offset;
    
    BOOL inTag = NO;
    BOOL inEndTag = NO;
    BOOL inSelfEndTag = NO; // <dict/> define a empty dictionary is the self end tag
    BOOL inQuote = NO;
    
    BOOL previousTagIsKEYIX = NO;
    BOOL shouldFlushCTD = NO;
    UniChar c = 0;
    RSMutableStringRef  tagName = RSStringCreateMutable(RSAllocatorSystemDefault, 0);
    RSMutableStringRef key = RSStringCreateMutable(RSAllocatorSystemDefault, 0);
    RSTypeRef value = nil;
    int tagID, previousTagID = 0;
    int currentTagID = -1;
    RSMutableArrayRef tagIDPairStack = context ? (context->passStack ? (RSMutableArrayRef)RSRetain(context->passStack) : RSArrayCreateMutable(RSAllocatorSystemDefault, 64)) : RSArrayCreateMutable(RSAllocatorSystemDefault, 64);
    RSNumberRef tagIDRef = nil;     // current tag id
    RSNumberRef preTagIDRef = nil;  // stack top tag id
    
    RSMutableDictionaryRef cpl __unused = nil;
    BOOL shouldContinue = NO;
    for (; idx < cnt || shouldContinue ; idx++)
    {
        if (shouldFlushCTD) currentTagID = -1;
        shouldFlushCTD = NO;
        if (likely(NO == shouldContinue)) c = data[idx];
        switch (c)
        {
            case '\"':
                inQuote = !inQuote;
                break;
            case '<':
                if (NO == inQuote)
                {
                    inTag = YES;
                    if (data[idx+1] == '/')
                    {
                        // the tag is an end tag
                    }
                }
                break;
            case '>':
                if (NO == inQuote)
                {
                    inTag = NO;
                    currentTagID = tagID = __RSPropertyListTagID(tagName);
                    tagIDRef = __RSPropertyListTagNumberWithID(tagID);
                    
                    if (NO == inEndTag)
                    {
                        preTagIDRef = RSArrayLastObject(tagIDPairStack);
                        if (preTagIDRef)
                        {
                            RSNumberGetValue(preTagIDRef, &previousTagID);
                        }
                        else previousTagID = -1;
                        RSArrayAddObject(tagIDPairStack, tagIDRef); // push to stack
                        switch (tagID)
                        {
                            case PLIST_IX:
                                if (inSelfEndTag)
                                {
                                    if (previousTagID != 0)
                                    {
                                        __RSPlistHaltHandler("plist tag is not available.", nil);
                                        goto PLIST_PARSE_RELEASE;
                                    }
                                }
                                else if (previousTagID != -1)
                                    return __RSPlistHaltHandler("plist tag is not available.", nil);
                                previousTagIsKEYIX = NO;
                                break;
                            case ARRAY_IX:
                                if (previousTagIsKEYIX)
                                {
                                    RSIndex _idx = idx + 1;
                                    __RSPropertyListParseContext context = {tagIDPairStack, NO, previousTagIsKEYIX, {0,0}};
                                    //__RSCLog(RSLogLevelDebug, "%s\n", "entry __RSPropertyListCore1");
                                    if (value) RSRelease(value);
                                    value = nil;
                                    value = __RSPropertyListCore1(plistData, &_idx, &context);
                                    //__RSCLog(RSLogLevelDebug, "%s\n", "leave __RSPropertyListCore1");
                                    idx = _idx;
                                    goto UPDATE_BLOCK;
                                }
                                else if (previousTagID == PLIST_IX)
                                {
                                    __RSPropertyListParseContext context = {tagIDPairStack, NO, previousTagIsKEYIX, {0,0}};
                                    RSIndex _idx = idx + 1;
                                    _plist = (RSMutableTypeRef)__RSPropertyListCore1(plistData, &_idx, &context);
                                    idx = _idx;
                                    RSRelease(tagIDRef);
                                    tagIDRef = nil;
                                    goto PLIST_PARSE_RELEASE;
                                }
                                else
                                {
                                    __RSPlistHaltHandler("previous tag is not key tag.", nil);
                                    goto PLIST_PARSE_RELEASE;
                                }
                                break;
                            case DICT_IX:
                                if (previousTagIsKEYIX)
                                {
                                    RSIndex _idx = idx + 1;
                                    __RSPropertyListParseContext context = {tagIDPairStack, NO, previousTagIsKEYIX, {0,0}};
                                    //__RSCLog(RSLogLevelDebug, "%s\n", "entry __RSPropertyListCore0");
                                    if (value) RSRelease(value);
                                    value = nil;
                                    value = __RSPropertyListCore0(plistData, &_idx, &context);
                                    //__RSCLog(RSLogLevelDebug, "%s\n", "leave __RSPropertyListCore0");
                                    idx = _idx;
                                    goto UPDATE_BLOCK;
                                }
                                else if (previousTagID == PLIST_IX)
                                {
                                    _plist = RSDictionaryCreateMutable(RSAllocatorSystemDefault, 0, RSDictionaryRSTypeContext);
                                }
                                else
                                {
                                    __RSPlistHaltHandler("previous tag is not key tag.", nil);
                                    goto PLIST_PARSE_RELEASE;
                                }
                                break;
                            case KEY_IX:
                                if (previousTagIsKEYIX) return __RSPlistHaltHandler("previous tag is not key tag.", nil);
                                if (key) RSRelease(key);
                                key = RSStringCreateMutable(RSAllocatorSystemDefault, 32);
                                previousTagIsKEYIX = YES;
                                break;
                                
                            case STRING_IX:
                            case DATE_IX:
                            case DATA_IX:
                            case REAL_IX:
                            case INTEGER_IX:
                            case TRUE_IX:
                            case FALSE_IX:
                                if (previousTagIsKEYIX)
                                {
                                    if (value != nil) RSRelease(value);
                                    value = nil;
                                    value = RSStringCreateMutable(RSAllocatorSystemDefault, 32);
                                }
                                else
                                {
                                    __RSPlistHaltHandler("previous tag is not key tag.", nil);
                                    goto PLIST_PARSE_RELEASE;
                                }
                                break;
                            default:
                                __RSPlistHaltHandler("the tag is not defined.", nil);
                                goto PLIST_PARSE_RELEASE;
                                break;
                        } // switch (currentTagID)
                    } // if (NO == inEndTag || NO == inSelfEndTag)
                    else
                    {
                        inEndTag = NO;
                        shouldFlushCTD = YES;
                        RSNumberRef _tagPare = RSArrayLastObject(tagIDPairStack);

                        if (nil == _tagPare) goto UPDATE_BLOCK;
                        int _id = -1;
                        if (NO == inSelfEndTag)
                        {
                            if (RSCompareEqualTo == RSNumberCompare(_tagPare, tagIDRef, nil))
                            {
                                RSNumberGetValue(_tagPare, &_id);
                                RSArrayRemoveLastObject(tagIDPairStack);
                                //previousTagIsKEYIX = (_id == KEY_IX);
                            }
                            else
                            {
                                __RSPlistHaltHandler("tag is not pare.", nil);
                                goto PLIST_PARSE_RELEASE;
                            }
                        }
                        else
                        {
                            if (value) RSRelease(value);
                            value = nil;
                            switch (currentTagID)
                            {
                                case DICT_IX:
                                    value = RSDictionaryCreateWithObjectsAndOKeys(RSAllocatorSystemDefault, nil);
                                    break;
                                case ARRAY_IX:
                                    value = RSArrayCreateWithObject(RSAllocatorSystemDefault, nil);
                                    break;
                                default:
                                    break;
                            }
                        }
                        // update the KVC
                        if (DICT_IX == _id && inSelfEndTag == NO)
                        {
                            RSRelease(tagIDRef);
                            tagIDRef = nil;
                            goto PLIST_PARSE_RELEASE;
                        }
                        inSelfEndTag = NO;

                        inSelfEndTag = NO;
                        if (previousTagIsKEYIX && _id != KEY_IX)
                        {
                        UPDATE_BLOCK:
                            previousTagIsKEYIX = NO;
                            
                            RSStringRef _k = __RSNormalStringFromXMLFormat(key);
                            RSNumberGetValue(tagIDRef, &currentTagID);
                            RSTypeRef _v = __RSPropertyListCopyValue(RSAllocatorSystemDefault, value, currentTagID);
                            if (_k && _v && _plist) RSDictionarySetValue(_plist, _k, _v);
                            else //__RSCLog(RSLogLevelNotice, "__RSPropertyListCopyValue return nil!\n");
                            RSRelease(_k);RSRelease(_v);
                            previousTagIsKEYIX = NO;
                        }
                        if (preTagIDRef && _id != KEY_IX) {
                            previousTagIsKEYIX = NO;
                        }
                    }
                    RSRelease(tagIDRef);
                    tagIDRef = nil;
                    RSRelease(tagName);
                    tagName = RSStringCreateMutable(RSAllocatorSystemDefault, 32);
                }
                else
                {
                }
                break;
            case '/':
                if (inTag)
                {
                    inEndTag = YES;
                    if (data[idx+1] == '>')
                    {
                        // the tag is an end tag
                        inSelfEndTag = YES;
                    }

                    break;
                }
            default:
                if (YES == inQuote)
                {
                    break;
                }
                if (YES == inTag)
                {
                    RSStringAppendCharacters(tagName, &c, 1);
                }
                else
                {
                    switch (currentTagID)
                    {
                        case -1:
                            break;
                        case KEY_IX:
                            RSStringAppendCharacters(key, &c, 1);
                            break;
                            
                        case STRING_IX:
                        case DATE_IX:
                        case REAL_IX:
                        case INTEGER_IX:
                        case TRUE_IX:
                        case FALSE_IX:
                            RSStringAppendCharacters((RSMutableStringRef)value, &c, 1);
                            break;
                        case DATA_IX:
                            if (!isspace(c))
                                RSStringAppendCharacters((RSMutableStringRef)value, &c, 1);
                            break;
                        case ARRAY_IX:
                            break;
                        case DICT_IX:
                            break;
                        default:
                            break;
                    }
                }
            break;  //default
        }//switch (c)
    }
PLIST_PARSE_RELEASE:
    RSRelease(tagIDRef);
    RSRelease(tagIDPairStack);
    RSRelease(tagName);
    RSRelease(key);
    RSRelease(value);
    if (offset) *offset = idx;
    return _plist;
}
extern BOOL __RSBPLCheckHeader(RSDataRef data);
extern RSTypeRef __RSBPLCreateWithData(RSAllocatorRef allocator, RSDataRef data);
// entry of Plist parser
RSPrivate RSTypeRef __RSPropertyListParser(RSAllocatorRef allocator, RSDataRef plistData, RSIndex* offset, __autorelease RSErrorRef *error)
{
    if (plistData == nil) return nil;
    if (RSDataGetLength(plistData) >= 8)
    {
        if (__RSBPLCheckHeader(plistData))
        {
            return __RSBPLCreateWithData(RSAllocatorSystemDefault, plistData);
        }
    }
    
    RSTypeRef output = nil;
    __RSPlistParserInfo info = {0};
    __RSPlistParserInfo *pInfo = &info;
    RSIndex length = RSDataGetLength(plistData);
    const char *buf = (const char *)RSDataGetBytesPtr(plistData);
    pInfo->begin = buf;
    pInfo->end = buf+length;
    pInfo->curr = buf;
    pInfo->allocator = allocator;
    pInfo->error = nil;
    //_createStringMap(pInfo);
    pInfo->mutabilityOption = RSPropertyListMutableContainersAndLeaves;
    pInfo->allowNewTypes = YES;
    pInfo->skip = NO;
    
    parseXMLPropertyList(pInfo, &output);
    if (pInfo->stringCache) RSRelease(pInfo->stringCache);
    if (pInfo->error)
    {
        if (error) *error = (RSErrorRef)RSAutorelease(pInfo->error);
        else {
            __RSLogShowWarning(pInfo->error);
            RSRelease(pInfo->error);
        }
    }
    return output;
    __RSPropertyListParseContext context = {nil, YES, -1, {0,0}};
    if (offset)
        return __RSPropertyListCore0(plistData, offset, &context);
    else
    {
        RSIndex _inOffset = 0;
        return __RSPropertyListParser(allocator, plistData, &_inOffset, error);
    }
}

#pragma mark -
#pragma mark __RSPLCContext API
enum __RSPropertyListCreateContextFlag
{
    __RSPlistCreateWithPath = 1,
    __RSPlistCreateWithData = 2,
    __RSPlistCreateWithDictionary = 3,
    __RSPlistCreateWithArray = 4,
    __RSPlistCreateWithHandle = 5,
    __RSPlistCreateWithURL = 6
};
struct __RSPLCContext
{
    int _flag;
    union
    {
        RSMutableDictionaryRef _dict;
        RSMutableArrayRef _array;
        RSDataRef _data;
        RSStringRef _path;
        RSFileHandleRef _handle;
//        RSSocketRef _socket;
    }_context;
};

static void __RSPLCCCreateContext(struct __RSPLCContext* cc, int flag, const void* rstype)
{
    if (cc)
    {
        cc->_flag = flag;
        switch (flag)
        {
            case __RSPlistCreateWithArray:
                cc->_context._array = (RSMutableArrayRef)rstype;
                break;
            case __RSPlistCreateWithDictionary:
                cc->_context._dict = (RSMutableDictionaryRef)rstype;
                break;
            case __RSPlistCreateWithData:
                cc->_context._data = rstype;
                break;
            case __RSPlistCreateWithHandle:
                cc->_context._handle = rstype;
                break;
            case __RSPlistCreateWithPath:
                cc->_context._path = rstype;
                break;
            case __RSPlistCreateWithURL: // not support right now.
                break;
            default:
                break;
        }
    }
}

static RSMutableDictionaryRef __RSPLCCCreatePlistWithCContext(RSAllocatorRef allocator, struct __RSPLCContext* context)
{
    RSFileHandleRef handle = nil, debug_handle __unused = nil;
    RSDataRef data = nil;
    RSMutableTypeRef _plist = nil;
    RSDictionaryRef dict = nil;
    switch (context->_flag)
    {
        case __RSPlistCreateWithURL: // not support right now
            break;
        case __RSPlistCreateWithPath:
            handle = RSFileHandleCreateForReadingAtPath(context->_context._path);
            if (handle == nil) break;
        case __RSPlistCreateWithHandle:
            handle = handle ?: RSRetain(context->_context._handle);
            if (handle == nil) break;
        case __RSPlistCreateWithData:
            data = (handle ? (RSFileHandleReadDataToEndOfFile(handle)) : (RSCopy(allocator, context->_context._data)));
            if (data == nil) break;
        case __RSPlistCreateWithDictionary:
        case __RSPlistCreateWithArray:
            _plist = data ? (RSMutableTypeRef)__RSPropertyListParser(allocator, data, nil, nil) : RSMutableCopy(allocator, (__RSPlistCreateWithArray == context->_flag) ?  (RSTypeRef)context->_context._array :  (RSTypeRef)context->_context._dict);
            break;
        default:
            break;
    }
    if (handle) RSRelease(handle);
    if (data) RSRelease(data);
    if (dict) RSRelease(dict);
    return _plist;
}

#pragma mark -
#pragma mark RSPropertyList

struct __RSPropertyList
{
    RSRuntimeBase _base;
    RSTypeRef _plist;   // RSDictionary, RSArray.
    RSDataRef _data;
};

RSInline BOOL __RSPropertyListAutoSave(RSPropertyListRef plist)
{
    return NO;
}

static void __RSPropertyListClassInit(RSTypeRef rs)
{
    //RSPropertyListRef pl = (RSPropertyListRef)rs;
    //pl->_plist = RSDictionaryCreateMutable(RSAllocatorSystemDefault, 0, RSDictionaryRSTypeContext);
}

static RSTypeRef __RSPropertyListClassCopy(RSAllocatorRef allocator, RSTypeRef rs, BOOL mutableCopy)
{
    RSPropertyListRef pl = (RSPropertyListRef)rs;
    return RSRetain(pl);
}

static void __RSPropertyListClassDeallocate(RSTypeRef rs)
{
    RSPropertyListRef pl = (RSPropertyListRef)rs;
    if (pl->_plist)
    {
        RSRelease(pl->_plist);
        pl->_plist = nil;
    }
    
    if (pl->_data)
    {
        RSRelease(pl->_data);
        pl->_data = nil;
    }
}

static BOOL __RSPropertyListClassEqual(RSTypeRef rs1, RSTypeRef rs2)
{
    RSPropertyListRef pl1 = (RSPropertyListRef)rs1;
    RSPropertyListRef pl2 = (RSPropertyListRef)rs2;
    if (NO == RSEqual(pl1->_plist, pl2->_plist)) return NO;
    return YES;
}

static RSHashCode __RSPropertyListClassHash(RSTypeRef rs)
{
    return RSHash(((RSPropertyListRef)rs)->_plist);
}

static RSStringRef __RSPropertyListClassDescription(RSTypeRef rs)
{
    return RSDescription(((RSPropertyListRef)rs)->_plist);
}

static RSRuntimeClass __RSPropertyListClass =
{
    _RSRuntimeScannedObject,
    "RSPropertyList",
    __RSPropertyListClassInit,
    __RSPropertyListClassCopy,
    __RSPropertyListClassDeallocate,
    __RSPropertyListClassEqual,
    __RSPropertyListClassHash,
    __RSPropertyListClassDescription,
    nil,
    nil
};

static RSTypeID _RSPropertyListTypeID = _RSRuntimeNotATypeID;
RSPrivate void __RSPropertyListInitialize()
{
    _RSPropertyListTypeID = __RSRuntimeRegisterClass(&__RSPropertyListClass);
    __RSRuntimeSetClassTypeID(&__RSPropertyListClass, _RSPropertyListTypeID);
}

RSExport RSTypeID RSPropertyListGetTypeID()
{
    return _RSPropertyListTypeID;
}

static RSPropertyListRef __RSPropertyListCreateInstance(RSAllocatorRef allocator, struct __RSPLCContext* ccontext)
{
    RSMutableDictionaryRef _plist = __RSPLCCCreatePlistWithCContext(allocator, ccontext);
    if (_plist)
    {
        RSPropertyListRef plist = (RSPropertyListRef)__RSRuntimeCreateInstance(allocator, _RSPropertyListTypeID, sizeof(struct __RSPropertyList) - sizeof(RSRuntimeBase));
        plist->_plist = _plist;
        return plist;
    }
    return nil;
}

RSExport RSPropertyListRef RSPropertyListCreateWithXMLData(RSAllocatorRef allocator, RSDataRef data)
{
    if (data == nil) return nil;
    __RSGenericValidInstance(data, RSDataGetTypeID());
    struct __RSPLCContext ccContext = {0};
    __RSPLCCCreateContext(&ccContext, __RSPlistCreateWithData, data);
    RSPropertyListRef plist = __RSPropertyListCreateInstance(RSAllocatorSystemDefault, &ccContext);
    return plist;
}

RSExport RSPropertyListRef RSPropertyListCreateWithPath(RSAllocatorRef allocator, RSStringRef filePath)
{
    if (filePath == nil) return nil;
    __RSGenericValidInstance(filePath, RSStringGetTypeID());
    struct __RSPLCContext ccontext = {0};
    __RSPLCCCreateContext(&ccontext, __RSPlistCreateWithPath, filePath);
    RSPropertyListRef plist = __RSPropertyListCreateInstance(RSAllocatorSystemDefault, &ccontext);
    return plist;
}

RSExport RSPropertyListRef RSPropertyListCreateWithHandle(RSAllocatorRef allocator, RSFileHandleRef handle)
{
    if (handle == nil) return nil;
    __RSGenericValidInstance(handle, RSFileHandleGetTypeID());
    struct __RSPLCContext ccontext = {0};
    __RSPLCCCreateContext(&ccontext, __RSPlistCreateWithHandle, handle);
    RSPropertyListRef plist = __RSPropertyListCreateInstance(allocator, &ccontext);
    return plist;
}

RSExport RSPropertyListRef RSPropertyListCreateWithContent(RSAllocatorRef allocator, RSTypeRef content)
{
    if (content == nil) return nil;
    RSTypeID contentID = RSGetTypeID(content);
    if (contentID != arraytype && contentID != dicttype)
        return nil;
    __RSGenericValidInstance(content, contentID);   // just check runtime base.
    struct __RSPLCContext ccontext = {0};
    __RSPLCCCreateContext(&ccontext, (contentID == dicttype) ? __RSPlistCreateWithDictionary : __RSPlistCreateWithArray, content);
    RSPropertyListRef plist = __RSPropertyListCreateInstance(allocator, &ccontext);
    return plist;
}
//RSExport RSPropertyListRef RSPropertyListCreateWithURL(RSAllocatorRef allocator, RSURLRef url)

RSExport RSTypeRef RSPropertyListGetContent(RSPropertyListRef plist)
{
    if (plist == nil) return nil;
    __RSGenericValidInstance(plist, _RSPropertyListTypeID);
    return plist->_plist;
}

RSExport RSTypeRef RSPropertyListCopyContent(RSAllocatorRef allocator, RSPropertyListRef plist)
{
    if (plist == nil) return nil;
    __RSGenericValidInstance(plist, _RSPropertyListTypeID);
    return RSCopy(allocator, plist->_plist);
}

RSExport RSDataRef RSPropertyListGetData(RSPropertyListRef plist, __autorelease RSErrorRef* error)
{
    if (plist == nil) return nil;
    __RSGenericValidInstance(plist, _RSPropertyListTypeID);
    if (plist->_data) return plist->_data;
    if (error) *error = nil;
    RSMutableDataRef xmlData = __RSPropertyListCreateWithError(plist->_plist, error);
    if (xmlData == nil) return nil;
    
    RSDataRef padding = __RSPropertyListHeader();
    
    RSMutableDataRef allData = RSDataCreateMutable(RSAllocatorSystemDefault, RSDataGetLength(xmlData) + RSDataGetLength(padding));
    RSDataAppend(allData, padding);
    RSRelease(padding);
    
    RSDataAppend(allData, xmlData);
    RSRelease(xmlData);
    
    padding = __RSPropertyListTailer();
    RSDataAppend(allData, padding);
    RSRelease(padding);
    
    return plist->_data = allData;
}

RSExport RSDataRef RSPropertyListCreateXMLData(RSAllocatorRef allocator, RSDictionaryRef requestDictionary)
{
	RSPropertyListRef plist = RSPropertyListCreateWithContent(allocator, requestDictionary);
	if (plist)
	{
		RSDataRef xmlData = RSRetain(RSPropertyListGetData(plist, nil));
		RSRelease(plist);
		return xmlData;
	}
	return nil;
}

RSExport BOOL RSPropertyListWriteToFile(RSPropertyListRef plist, RSStringRef filePath, __autorelease RSErrorRef* error)
{
    BOOL result = NO;
    if (error && *error) return result;
    if (!plist || !filePath || !RSPropertyListGetData(plist, error)) return result;
    result = RSDataWriteToFile(RSPropertyListGetData(plist, error), filePath, YES);
    return result;
}

RSExport BOOL RSPropertyListWriteToFileWithMode(RSPropertyListRef plist, RSStringRef filePath, RSPropertyListWriteMode mode, __autorelease RSErrorRef* error)
{
    BOOL result = NO;
    switch (mode)
    {
        case RSPropertyListWriteDefault:
        case RSPropertyListWriteNormal:
            result = RSPropertyListWriteToFile(plist, filePath, error);
            break;
        case RSPropertyListWriteBinary:
            result = RSBinaryPropertyListWriteToFile(plist, filePath, error);
            break;
    }
    return result;
}
