//
//  RSJSONSerialization.c
//  RSCoreFoundation
//
//  Created by RetVal on 4/11/13.
//  Copyright (c) 2013 RetVal. All rights reserved.
//

#include <RSCoreFoundation/RSJSONSerialization.h>
#include <RSCoreFoundation/RSNil.h>
#include <RSCoreFoundation/RSStringNumberSupport.h>

#ifndef __RSJSON_TRUE
#define __RSJSON_TRUE   "true"
#endif

#ifndef __RSJSON_FALSE
#define __RSJSON_FALSE   "false"
#endif

static RSTypeID arraytype = _RSRuntimeNotATypeID;
static RSTypeID datetype = _RSRuntimeNotATypeID;
static RSTypeID datatype = _RSRuntimeNotATypeID;
static RSTypeID dicttype = _RSRuntimeNotATypeID;
static RSTypeID numbertype = _RSRuntimeNotATypeID;
static RSTypeID stringtype = _RSRuntimeNotATypeID;
static RSTypeID nulltype = _RSRuntimeNotATypeID;

RSPrivate void __RSJSONSerializationInitialize()
{
    arraytype = RSArrayGetTypeID();
    datatype = RSDataGetTypeID();
    datetype = RSDateGetTypeID();
    dicttype = RSDictionaryGetTypeID();
    numbertype = RSNumberGetTypeID();
    stringtype = RSStringGetTypeID();
    nulltype = RSNilGetTypeID();
}

RSExport BOOL RSJSONSerializationIsValidJSONObject(RSTypeRef jsonObject)
{
    if (nil == jsonObject || RSNil == jsonObject) return NO;
    RSTypeID objectID = RSGetTypeID(jsonObject);
    if (objectID == arraytype) return YES;
    else if (objectID == dicttype) return YES;
    else if (objectID == stringtype) return YES;
    else if (objectID == datatype) return YES;
    else if (objectID == datetype) return YES;
    else if (objectID == numbertype) return YES;
    return NO;
}

static RSErrorRef __RSJSONCreateError(RSErrorCode errorCode, RSStringRef descriptionFormat, ...)
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

#pragma mark -
#pragma mark __RSJSONCreation API

static void _jsonAppendUTF8CString(RSMutableDataRef mData, const char *cString)
{
    RSDataAppendBytes(mData, (const RSBitU8 *)cString, strlen(cString));
}
static void _jsonAppendCharacters(RSMutableDataRef xmlData, const UniChar* characters, RSIndex length);

static void _jsonAppendString(RSMutableDataRef xmlData, RSStringRef append)
{
    const UniChar *chars;
    const char *cStr;
    RSDataRef data;
    if ((chars = RSStringGetCharactersPtr(append)))
    {
        _jsonAppendCharacters(xmlData, chars, RSStringGetLength(append));
    }
    else if ((cStr = RSStringGetCStringPtr(append, RSStringEncodingASCII)) || (cStr = RSStringGetCStringPtr(append, __RSDefaultEightBitStringEncoding)))
    {
        _jsonAppendUTF8CString(xmlData, cStr);
    }
    else if ((data = RSStringCreateExternalRepresentation(RSAllocatorSystemDefault, append, RSStringEncodingUTF8, 0)))
    {
        RSDataAppendBytes (xmlData, RSDataGetBytesPtr(data), RSDataGetLength(data));
        RSRelease(data);
    }
    else
    {
        RSAssert1(YES, __RSLogAssertion, "%s(): Error in plist writing", __PRETTY_FUNCTION__);
    }
}

static void _jsonAppendFormat(RSMutableDataRef xmlData, RSStringRef format, ...)
{
    va_list list;
    va_start(list, format);
    RSStringRef append = RSStringCreateWithFormatAndArguments(RSAllocatorSystemDefault, 128, format, list);
    va_end(list);
    _jsonAppendString(xmlData, append);
    RSRelease(append);
}

static void _jsonAppendCharacters(RSMutableDataRef xmlData, const UniChar* characters, RSIndex length)
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
    
    if (curLoc < length)
    {
        // Now deal with non-ASCII chars
        RSDataRef data = nil;
        RSStringRef str = nil;
        if ((str = RSStringCreateWithCharactersNoCopy(RSAllocatorSystemDefault, characters + curLoc, length - curLoc, RSAllocatorNull)))
        {
            if ((data = RSStringCreateExternalRepresentation(RSAllocatorSystemDefault, str, RSStringEncodingUTF8, 0))) {
                RSDataAppendBytes (xmlData, RSDataGetBytesPtr(data), RSDataGetLength(data));
                RSRelease(data);
            }
            RSRelease(str);
        }
        RSAssert1(str && data, __RSLogAssertion, "%s(): Error writing plist", __PRETTY_FUNCTION__);
    }
}

#if !defined(new_rstype_array)
    #define new_rstype_array(N, C) \
    size_t N ## _count__ = (C); \
    if (N ## _count__ > LONG_MAX / sizeof(RSTypeRef)) { \
        HALTWithError(RSMallocException, "RSJSONSerialization ran out of memory while attempting to allocate temporary storage."); \
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
        HALTWithError(RSMallocException, "RSJSONSerialization ran out of memory while attempting to allocate temporary storage."); \
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

static RSMutableStringRef __RSNormalStringToJSONFormat(RSStringRef normal)
{
    return RSMutableCopy(RSGetAllocator(normal), normal);
}

static RSMutableStringRef __RSNormalStringFromJSONFormat(RSStringRef jsonString)
{
    return RSMutableCopy(RSGetAllocator(jsonString), jsonString);
}

static void _appendIndents(RSIndex numIndents, RSMutableDataRef str)
{
#define NUMTABS 4
    static const UniChar tabs[NUMTABS] = {'\t','\t','\t','\t'};
    for (; numIndents > 0; numIndents -= NUMTABS) _jsonAppendCharacters(str, tabs, (numIndents >= NUMTABS) ? NUMTABS : numIndents);
}

static RSErrorCode __RSAppendJSON0(RSTypeRef object, RSBitU32 indentation, RSMutableDataRef jsonString)
{
    RSTypeID typeID = RSGetTypeID(object);
    if (NO == RSJSONSerializationIsValidJSONObject(object)) return kErrVerify;
    _appendIndents(indentation, jsonString);
    if (typeID == stringtype)
    {
        RSStringRef jsonStr = __RSNormalStringToJSONFormat(object);
        _jsonAppendUTF8CString(jsonString, "\"");
        _jsonAppendString(jsonString, jsonStr);
        _jsonAppendUTF8CString(jsonString, "\"");
        RSRelease(jsonStr);
        return kSuccess;
    }
    else if (typeID == arraytype)
    {
        RSBitU32 i;
        RSIndex count = RSArrayGetCount((RSArrayRef)object);
        if (count == 0)
        {
            // empty array
            _jsonAppendUTF8CString(jsonString, "[]");
            return kSuccess;
        }
        _jsonAppendUTF8CString(jsonString, "\n");
        _appendIndents(indentation, jsonString);
        _jsonAppendUTF8CString(jsonString, "[");
        _jsonAppendUTF8CString(jsonString, "\n");
        for (i = 0; i < count; i ++)
        {
            RSTypeRef oinArray = RSArrayObjectAtIndex((RSArrayRef)object, i);
            __RSAppendJSON0(oinArray, indentation+1, jsonString);
            if (i < count - 1) _jsonAppendUTF8CString(jsonString, ",");
            _jsonAppendUTF8CString(jsonString, "\n");
        }
        _appendIndents(indentation, jsonString);
        _jsonAppendUTF8CString(jsonString, "]");
        return kSuccess;
    }
    else if (typeID == dicttype)
    {
        RSBitU32 i;
        RSIndex count = RSDictionaryGetCount((RSDictionaryRef)object);
        RSArrayRef keyArray;
        if (count == 0)
        {
            // empty dictionary
            _jsonAppendUTF8CString(jsonString, "{}");
            return kSuccess;
        }
        
        new_rstype_array(keys, count);
        RSDictionaryGetKeysAndValues((RSDictionaryRef)object, keys, nil);
        keyArray = RSArrayCreateWithObjects(RSAllocatorSystemDefault, keys, count);
        
        RSArraySort(keyArray, RSOrderedDescending, (RSComparatorFunction)RSStringCompare, nil);
        RSArrayGetObjects(keyArray, RSMakeRange(0, count), keys);
        
        _jsonAppendUTF8CString(jsonString, "\n");
        _appendIndents(indentation, jsonString);
        _jsonAppendUTF8CString(jsonString, "{");
        _jsonAppendUTF8CString(jsonString, "\n");
        RSErrorCode errCode = kSuccess;
        for (i = 0; i < count; i ++)
        {
            RSTypeRef key = keys[i];
            _appendIndents(indentation+1, jsonString);
            _jsonAppendUTF8CString(jsonString, "\"");
            RSStringRef _jsonKey = __RSNormalStringToJSONFormat(key);
            _jsonAppendString(jsonString, _jsonKey);
            RSRelease(_jsonKey);
            _jsonAppendUTF8CString(jsonString, "\"");
            
            _jsonAppendUTF8CString(jsonString, ":");
            
            RSTypeRef value = RSDictionaryGetValue((RSDictionaryRef)object, key);
            errCode = __RSAppendJSON0(value, indentation+1, jsonString);
            if (i < count - 1) _jsonAppendUTF8CString(jsonString, ",");
            _jsonAppendUTF8CString(jsonString, "\n");
        }
        RSRelease(keyArray);
        free_rstype_array(keys);
        _appendIndents(indentation, jsonString);
        _jsonAppendUTF8CString(jsonString, "}");
        _jsonAppendUTF8CString(jsonString, "\n");
        return errCode;
    }
    else if (typeID == datatype)
    {
        return kErrVerify; // should I write data as base64 encoded stirng or not...
    }
    else if (typeID == datetype)
    {
        // YYYY '-' MM '-' DD 'T' hh ':' mm ':' ss 'Z'
        //        RSBit32 y = 0, M = 0, d = 0, H = 0, m = 0, s = 0;
        RSGregorianDate date = RSAbsoluteTimeGetGregorianDate(RSDateGetAbsoluteTime((RSDateRef)object), nil);
        _jsonAppendFormat(jsonString, RSSTR("\"%04d-%02d-%02dT%02d:%02d:%02dZ\""), date.year, date.month, date.day, date.hour, date.minute, (int32_t)date.second);
        return kSuccess;    // date is translated to string.
    }
    else if (typeID == numbertype)
    {
        if (RSNumberIsFloatType((RSNumberRef)object))
        {
            extern RSStringRef __RSNumberCopyFormattingDescriptionAsFloat64(RSTypeRef obj);
            RSStringRef s = __RSNumberCopyFormattingDescriptionAsFloat64(object);
            _jsonAppendString(jsonString, s);
            RSRelease(s);
        }
        else if (RSNumberIsBooleanType((RSNumberRef)object))
        {
            BOOL boolean = NO;
            RSNumberGetValue(object, &boolean);
            if (boolean)
            {
                _jsonAppendUTF8CString(jsonString, __RSJSON_TRUE);
            }
            else
            {
                _jsonAppendUTF8CString(jsonString, __RSJSON_FALSE);
            }
        }
        else
        {
            _jsonAppendFormat(jsonString, RSSTR("%r"), object);
        }
        return kSuccess;
    }
    else if (typeID == nulltype)
    {
        _jsonAppendUTF8CString(jsonString, "null");
        return kSuccess;
    }
    return kErrVerify;
}

#pragma mark -
#pragma mark __RSJSONParse API

typedef struct __RSJSONParserContext
{
    RSDataRef _jsonData;
    
    const char *_begin;
    const char *_curr;
    const char *_end;
    
    RSErrorRef _error;
    BOOL _inContents;
    
    BOOL _skip;
    RSAllocatorRef _allocator;
}__RSJSONParserContext;

static UInt32 lineNumber(__RSJSONParserContext *pInfo)
{
    const char *p = pInfo->_begin;
    UInt32 count = 1;
    while (p < pInfo->_curr)
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

RSInline void skipWhitespace(__RSJSONParserContext *pInfo)
{
    while (pInfo->_curr < pInfo->_end)
    {
        switch (*(pInfo->_curr))
        {
            case ' ':
            case '\t':
            case '\n':
            case '\r':
                pInfo->_curr ++;
                continue;
            default:
                return;
        }
    }
}

static const RSBlock __RSJSONNumber[] = {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '.', '+', '-'};
static const RSBlock __RSJSONAllowContentWithOutQuotation[] = {'\t', '\n', ':'};
static const RSBlock __RSJSONKeywords[][10] = {
    {'n','u','l','l','\0','\0','\0','\0','\0','\0'},
    {'t','r','u','e','\0','\0','\0','\0','\0','\0'},
    {'f','a','l','s','e','\0','\0','\0','\0','\0'},
};

static BOOL ___RSJSONCharInWhiteList(const RSBlock *list, unsigned int size, UniChar c)
{
    for (unsigned int idx = 0; idx < size; ++idx)
    {
        if (c == list[idx]) return YES;
    }
    return NO;
}

static BOOL ___RSJSONWordInWhiteList(const RSBlock *list[], unsigned int size, unsigned int items, const RSBlock *word)
{
    for (unsigned int idx = 0; idx < items; idx++) {
        unsigned int j = 0;
        for (unsigned int i = 0; i < size; i++) {
            if (list[idx][i] == word[i]) j ++;
            else j = 0;
        }
        if (j == size) return YES;
    }
    return NO;
}

RSInline BOOL __RSJSONCharIsNumber(UniChar c)
{
    return ((c >= '0' && c <= '9') || c == '.' || c == '-' || c == '+') ? YES : NO;
}
static BOOL __RSJSONCharInWhiteList(UniChar c)
{
    static unsigned int size = sizeof(__RSJSONAllowContentWithOutQuotation) / sizeof(RSBlock);
    return ___RSJSONCharInWhiteList(__RSJSONAllowContentWithOutQuotation, size, c);
}

static BOOL __RSJSONContextInit(__RSJSONParserContext *context, RSTypeRef jsonData, RSIndex offset)
{
    if (context && jsonData)
    {
        context->_allocator = RSGetAllocator(jsonData);
        context->_jsonData = RSRetain(jsonData);
        context->_begin = (const char*)RSDataGetBytesPtr(context->_jsonData);
        context->_curr = (const char*)RSDataGetBytesPtr(context->_jsonData) + offset;
        context->_end = context->_begin + RSDataGetLength(context->_jsonData);
        if (context->_curr > context->_end) return NO;
        return YES;
    }
    return NO;
}

static void __RSJSONContextRelease(__RSJSONParserContext *context)
{
    if (context)
    {
        if (context->_jsonData)
        {
            RSRelease(context->_jsonData);
            context->_jsonData = nil;
        }
        if (context->_error)
        {
            __RSLogShowWarning(context->_error);
            RSAutorelease(context->_error);
        }
    }
}

static BOOL parserJSON(__RSJSONParserContext *context, BOOL *isKey, RSTypeRef *object);
static BOOL parserJSONElement(__RSJSONParserContext *context, BOOL *isKey, RSTypeRef *object);
static BOOL getContentObject(__RSJSONParserContext *pInfo, BOOL *isKey, RSTypeRef *out)
{
    if (isKey) *isKey = NO;
    while (!pInfo->_error && pInfo->_curr < pInfo->_end)
    {
        skipWhitespace(pInfo);
        if (pInfo->_curr >= pInfo->_end)
        {
            pInfo->_error = __RSJSONCreateError(RSJSONReadCorruptError, RSSTR("Encountered unexpected EOF"));
            return NO;
        }
    
        if (!(*(pInfo->_curr) == '{' ||
              *(pInfo->_curr) == '[' ||
              *(pInfo->_curr) == '\"'||
              __RSJSONCharIsNumber(*pInfo->_curr)))
        {
            
            // here is last chance for checking if is keywords (null, true, NO)
            if (pInfo->_end - pInfo->_curr >= 4 &&
                ('n' == *(pInfo->_curr + 0)) &&
                ('u' == *(pInfo->_curr + 1)) &&
                ('l' == *(pInfo->_curr + 2)) &&
                ('l' == *(pInfo->_curr + 3)))
            {
                *out = RSNil;
                pInfo->_curr += 4;
                return YES;
            }
            else if (pInfo->_end - pInfo->_curr >= 4 &&
                     ('t' == *(pInfo->_curr + 0)) &&
                     ('r' == *(pInfo->_curr + 1)) &&
                     ('u' == *(pInfo->_curr + 2)) &&
                     ('e' == *(pInfo->_curr + 3)))
            {
                *out = RSBooleanTrue;
                pInfo->_curr += 4;
                return YES;
            }
            else if (pInfo->_end - pInfo->_curr >= 5 &&
                     ('f' == *(pInfo->_curr + 0)) &&
                     ('a' == *(pInfo->_curr + 1)) &&
                     ('l' == *(pInfo->_curr + 2)) &&
                     ('s' == *(pInfo->_curr + 3)) &&
                     ('e' == *(pInfo->_curr + 4)))
            {
                *out = RSBooleanFalse;
                pInfo->_curr += 5;
                return YES;
            }
            
            
            pInfo->_error = __RSJSONCreateError(RSJSONReadCorruptError, RSSTR("Encountered unexpected character %c on line %d while looking for open tag"), *(pInfo->_curr), lineNumber(pInfo));
            return NO;
        }
        pInfo->_curr ++;
        if (pInfo->_curr >= pInfo->_end)
        {
            pInfo->_error = __RSJSONCreateError(RSJSONReadCorruptError, RSSTR("Encountered unexpected EOF"));
            return NO;
        }
        pInfo->_curr--;
        char ch = *pInfo->_curr;
        switch (ch)
        {
            default:
                // Should be an element
                return parserJSONElement(pInfo, isKey, out);
        }
    }
    return NO;
}

static RSStringRef _uniqueStringForUTF8Bytes(__RSJSONParserContext *pInfo, const char *base, RSIndex length)
{
    if (length == 0) return RSSTR("");
    
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
    result = RSStringCreateWithBytes(pInfo->_allocator, (const RSBitU8*)base, length, RSStringEncodingUTF8, NO);
    return result;
}

static BOOL parserStringElement(__RSJSONParserContext *context, RSTypeRef *obj)
{
    const char *marker = context->_curr;
    RSMutableDataRef stringData = nil;
    while (!context->_error && context->_curr < context->_end)
    {
        char ch = *(context->_curr);
        if (ch == '\"')
        {
            if (context->_curr + 1 >= context->_end) break;
            if (!stringData) stringData = RSDataCreateMutable(context->_allocator, 0);
            RSDataAppendBytes(stringData, (const RSBitU8 *)marker, context->_curr - marker);
            marker = ++context->_curr;
            break;
        }
        else
        {
            context->_curr ++;
        }
    }
    if (context->_error)
    {
        RSRelease(stringData);
        return NO;
    }
    if (!stringData)
    {
        RSStringRef s = _uniqueStringForUTF8Bytes(context, marker, context->_curr - marker);
        if (!s)
        {
            context->_error = __RSJSONCreateError(RSJSONReadCorruptError, RSSTR("Unable to convert string to correct encoding"));
            RSRelease(stringData);
            return NO;
        }
        *obj = s;
    }
    else
    {
        if (context->_curr - marker > 0) RSDataAppendBytes(stringData, (const RSBitU8 *)marker, context->_curr - marker);
        RSStringRef s = RSStringCreateWithBytes(context->_allocator, (const RSBitU8*)RSDataGetBytesPtr(stringData), RSDataGetLength(stringData), RSStringEncodingUTF8, NO);
        RSRelease(stringData); stringData = nil;
        *obj = s;
        return YES;
    }
    return NO;
}

static BOOL parserDictElement(__RSJSONParserContext *context, RSTypeRef *object)
{
    RSMutableDictionaryRef dict = nil;
    BOOL gotKey = NO;
    RSTypeRef key = nil, value = nil;
    BOOL result = getContentObject(context, &gotKey, &key);
    while (result && key)
    {
        if (!gotKey)
        {
            if (!context->_error) context->_error = __RSJSONCreateError(RSJSONReadCorruptError, RSSTR("Found non-key inside { at line %d"), lineNumber(context));
            RSRelease(key);
            RSRelease(dict);
            return NO;
        }
        skipWhitespace(context);
        if (*context->_curr == ':')
        {
            context->_curr++;
            skipWhitespace(context);
        }
        else
        {
            if (!context->_error) context->_error = __RSJSONCreateError(RSJSONReadCorruptError, RSSTR("Found non-value inside { at line %d"), lineNumber(context));
            RSRelease(key);
            RSRelease(dict);
            return NO;
        }
        result = getContentObject(context, nil, &value);
        if (!result)
        {
            if (!context->_error) context->_error = __RSJSONCreateError(RSJSONReadCorruptError, RSSTR("Value missing for key inside { at line %d"), lineNumber(context));
            RSRelease(key); key = nil;
            RSRelease(value); value = nil;
            RSRelease(dict); dict = nil;
            return NO;
        }
        else
        {
            if (key && value)
            {
                if (nil == dict) dict = RSDictionaryCreateMutable(context->_allocator, 0, RSDictionaryRSTypeContext);
                RSDictionarySetValue(dict, key, value);
            }
            RSRelease(key); key = nil;
            RSRelease(value); value = nil;
            
            skipWhitespace(context);
            if (*context->_curr == ',')
            {
                context->_curr++;
                skipWhitespace(context);
            }
            else if (*context->_curr == '}')
            {
                context->_curr++;
                goto ENDL;
            }
            else
            {
                if (!context->_error) context->_error = __RSJSONCreateError(RSJSONReadCorruptError, RSSTR("Found non-value inside { at line %d"), lineNumber(context));
                RSRelease(key);
                RSRelease(value);
                RSRelease(dict);
                return NO;
            }
        }
        
        result = getContentObject(context, &gotKey, &key);
    }
    
    if (1)
    {
    ENDL:
        if (nil == dict)
        {
            dict = RSDictionaryCreateMutable(RSAllocatorSystemDefault, 0, RSDictionaryRSTypeContext);
        }
        else
        {
            
        }
        *object = dict;
        return YES;
    }
    return NO;
}

static BOOL parserArrayElement(__RSJSONParserContext *context, RSTypeRef *object)
{
    RSMutableArrayRef array = nil;
    RSTypeRef value = nil;
    BOOL result = NO;
    //const char *marker = context->_curr;
    
    while (!context->_error && context->_curr < context->_end)
    {
        result = getContentObject(context, nil, &value);
        if (!result)
        {
            if (!context->_error) context->_error = __RSJSONCreateError(RSJSONReadCorruptError, RSSTR("Value missing for key inside { at line %d"), lineNumber(context));
            RSRelease(value);
            RSRelease(array);
            return NO;
        }
        if (value)
        {
            if (array == nil) array = RSArrayCreateMutable(context->_allocator, 0);
            RSArrayAddObject(array, value);
            RSRelease(value); value = nil;
        }
        
        skipWhitespace(context);
        if (*context->_curr == ',')
        {
            context->_curr++;
            skipWhitespace(context);
        }
        else if (*context->_curr == ']')
        {
            context->_curr++;
            goto ENDL;
        }
        else
        {
            if (!context->_error) context->_error = __RSJSONCreateError(RSJSONReadCorruptError, RSSTR("Found non-value inside { at line %d"), lineNumber(context));
            RSRelease(value);
            RSRelease(array);
            return NO;
        }
    }
    
    if (1)
    {
    ENDL:
        if (nil == array)
        {
            array = RSArrayCreateMutable(context->_allocator, 0);
        }
        else
        {
            
        }
        *object = array;
        return YES;
    }
    return result;
}

static BOOL parserNumberElement(__RSJSONParserContext *context, RSTypeRef *object)
{
    BOOL result = NO;
    const char *marker = context->_curr;
    while (!context->_error && context->_curr < context->_end)
    {
        if (__RSJSONCharIsNumber(*context->_curr)) context->_curr++;
        else
        {
            RSStringRef s = RSStringCreateWithBytes(context->_allocator, (const RSBitU8 *)marker, context->_curr - marker, RSStringEncodingUTF8, NO);
            double x = RSStringFloatValue(s);
            RSRelease(s);
            *object = RSNumberCreateDouble(context->_allocator, x);
            
            return result = YES;
        }
    }
    return result ;
}

static BOOL parserJSONElement(__RSJSONParserContext *context, BOOL *isKey, RSTypeRef *object)
{
    UniChar ch = *(context->_curr);
    context->_curr++;
    BOOL result = NO;
    if (ch == '\"')
    {
        result = parserStringElement(context, object);
        if (result && isKey)
            if (RSGetTypeID(*object) == RSStringGetTypeID())
                *isKey = YES;
        return result;
    }
    else if (ch == '{')
    {
        return parserDictElement(context, object);
    }
    else if (ch == '[')
    {
        return parserArrayElement(context, object);
    }
    else if (__RSJSONCharIsNumber(ch))
    {
        context->_curr--;
        return parserNumberElement(context, object);
    }
    return NO;
}

static BOOL parserJSON(__RSJSONParserContext *context, BOOL *isKey, RSTypeRef *object)
{
    while (!context->_error && context->_curr < context->_end)
    {
        
        skipWhitespace(context);
        if (context->_curr + 1 >= context->_end)
        {
            context->_error = __RSJSONCreateError(RSJSONReadCorruptError, RSSTR("NO JSON Content found"));
            return NO;
        }
        return parserJSONElement(context, isKey, object);
    }
    
    return kSuccess;
}

static RSTypeRef __RSJSONParserEntry(RSDataRef obj, RSIndex *offset)
{
    if (obj == nil) return nil;
    else if (offset == nil)
    {
        RSIndex _offset = 0;
        return __RSJSONParserEntry(obj, &_offset);
    }
    RSTypeRef result = nil;
    __RSJSONParserContext context = {0};
    __RSJSONContextInit(&context, obj, *offset);
    parserJSON(&context, nil, &result);
    __RSJSONContextRelease(&context);
    return result;
}

RSExport RSDataRef RSJSONSerializationCreateData(RSAllocatorRef allocator, RSTypeRef jsonObject)
{
    if (NO == RSJSONSerializationIsValidJSONObject(jsonObject)) return nil;
    RSMutableDataRef json = RSDataCreateMutable(RSAllocatorSystemDefault, 0);
    if (kSuccess == __RSAppendJSON0(jsonObject, 0, json)) return json;
    RSRelease(json);
    return nil;
}

RSExport RSTypeRef RSJSONSerializationCreateWithJSONData(RSAllocatorRef allocator, RSDataRef jsonData)
{
    return __RSJSONParserEntry(jsonData, 0);
}
