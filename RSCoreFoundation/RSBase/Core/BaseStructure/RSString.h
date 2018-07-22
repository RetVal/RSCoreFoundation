//
//  RSString.h
//  RSCoreFoundation
//
//  Created by RetVal on 10/24/12.
//  Copyright (c) 2012 RetVal. All rights reserved.
//

#ifndef __RSSTRING__
#define __RSSTRING__
#include <RSCoreFoundation/RSArray.h>
#include <RSCoreFoundation/RSAllocator.h>
#include <RSCoreFoundation/RSStringEncoding.h>
#include <RSCoreFoundation/RSData.h>

RS_EXTERN_C_BEGIN
enum
{
    RSCompareCaseInsensitive = 1,  
    RSCompareBackwards = 4,		/* Starting from the end of the string */
    RSCompareAnchored = 8,         /* Only at the specified starting point */
    RSCompareNonliteral = 16,		/* If specified, loose equivalence is performed (o-umlaut == o, umlaut) */
    RSCompareLocalized = 32,		/* User's default locale is used for the comparisons */
    RSCompareNumerically = 64		/* Numeric comparison is used RS_AVAILABLE(0_0); that is, Foo2.txt < Foo7.txt < Foo25.txt */
    ,
    RSCompareDiacriticInsensitive = 128, /* If specified, ignores diacritics (o-umlaut == o) */
    RSCompareWidthInsensitive = 256, /* If specified, ignores width differences ('a' == UFF41) */
    RSCompareForcedOrdering = 512 /* If specified, comparisons are forced to return either RSCompareLessThan or RSCompareGreaterThan if the strings are equivalent but not strictly equal, for stability when sorting (e.g. "aaa" > "AAA" with RSCompareCaseInsensitive specified) */

};
typedef RSIndex RSStringCompareFlags RS_AVAILABLE(0_0);

typedef const struct __RSString* RSStringRef RS_AVAILABLE(0_0);
typedef struct __RSString* RSMutableStringRef RS_AVAILABLE(0_0);

#include <RSCoreFoundation/RSLocale.h>
#include <RSCoreFoundation/RSCharacterSet.h>

RSExport RSTypeID RSStringGetTypeID(void) RS_AVAILABLE(0_0);
RSExport RSStringRef RSStringGetEmptyString(void) RS_AVAILABLE(0_0);
RSExport RSIndex RSStringGetLength(RSStringRef str) RS_AVAILABLE(0_0);
// allocator : put the RSAllocatorGetDefault() to these function below.
RSExport RSStringRef RSStringCreateWithCString(RSAllocatorRef allocator, RSCBuffer cString, RSStringEncoding encoding) RS_AVAILABLE(0_0);
RSExport RSStringRef RSStringCreateWithCStringNoCopy(RSAllocatorRef alloc, const char *cStr, RSStringEncoding encoding, RSAllocatorRef contentsDeallocator) RS_AVAILABLE(0_0);
RSExport RSStringRef RSStringCreateWithFormat(RSAllocatorRef allocator, RSStringRef format, ...) RS_AVAILABLE(0_0);
RSExport RSStringRef RSStringCreateWithFormatAndArguments(RSAllocatorRef allocator, RSIndex capacity, RSStringRef format, va_list arguments) RS_AVAILABLE(0_0);
RSExport RSStringRef RSStringCreateWithBytes(RSAllocatorRef alloc, const void *bytes, RSIndex numBytes, RSStringEncoding encoding, BOOL externalFormat) RS_AVAILABLE(0_0);
RSExport  RSStringRef  RSStringCreateWithBytesNoCopy(RSAllocatorRef alloc, const void *bytes, RSIndex numBytes, RSStringEncoding encoding, BOOL externalFormat, RSAllocatorRef contentsDeallocator) RS_AVAILABLE(0_0);
RSExport void RSStringSetExternalCharactersNoCopy(RSMutableStringRef string, UniChar *chars, RSIndex length, RSIndex capacity) RS_AVAILABLE(0_0);
//
RSExport RSStringRef RSStringCreateWithCharacters(RSAllocatorRef allocator, const UniChar* characters, RSIndex numChars) RS_AVAILABLE(0_0);
RSExport RSStringRef RSStringCreateWithCharactersNoCopy(RSAllocatorRef alloc, const UniChar *chars, RSIndex numChars, RSAllocatorRef contentsDeallocator) RS_AVAILABLE(0_0);
//
RSExport RSStringRef RSStringCreateCopy(RSAllocatorRef alloc, RSStringRef str) RS_AVAILABLE(0_0);
//
RSExport RSStringRef RSStringCreateWithSubstring(RSAllocatorRef allocator, RSStringRef aString, RSRange range) RS_AVAILABLE(0_0);
//
RSExport RSMutableStringRef RSStringCreateMutable(RSAllocatorRef allocator, RSIndex maxLength) RS_AVAILABLE(0_0);
RSExport RSMutableStringRef RSStringCreateMutableCopy(RSAllocatorRef allocator, RSIndex maxLength, RSStringRef aString) RS_AVAILABLE(0_0);
RSExport RSMutableStringRef RSStringCreateMutableWithExternalCharactersNoCopy(RSAllocatorRef alloc, UniChar *chars, RSIndex numChars, RSIndex capacity, RSAllocatorRef externalCharactersAllocator) RS_AVAILABLE(0_0);
//
RSExport RSStringRef RSStringCreateSubStringWithRange(RSAllocatorRef allocator, RSStringRef aString, RSRange range) RS_AVAILABLE(0_0);
RSExport RSMutableStringRef RSStringReplace(RSMutableStringRef mutableString, RSRange range, RSStringRef aString) RS_AVAILABLE(0_0);
RSExport RSMutableStringRef RSStringReplaceAll(RSMutableStringRef mutableString, RSStringRef find, RSStringRef replace) RS_AVAILABLE(0_0);
//RSExport RSMutableStringRef RSStringReplaceAllInRange(RSMutableStringRef mutableString, RSStringRef find, RSStringRef replace, RSRange range, RSIndex *replacedCount) RS_AVAILABLE(0_3);

RSExport RSRange RSStringGetRange(RSStringRef aString) RS_AVAILABLE(0_0);

RSExport RSComparisonResult RSStringCompare(RSStringRef aString, RSStringRef other, RSStringCompareFlags compareFlags) RS_AVAILABLE(0_0);
RSExport RSComparisonResult RSStringCompareCaseInsensitive(RSStringRef aString, RSStringRef other) RS_AVAILABLE(0_0);

RSExport RSComparisonResult RSStringCompareWithOptionsAndLocale(RSStringRef string, RSStringRef string2, RSRange rangeToCompare, RSStringCompareFlags compareOptions, const void* locale) RS_AVAILABLE(0_0);
RSExport BOOL RSStringFindWithOptions(RSStringRef aString, RSStringRef stringToFind, RSRange rangeToSearch, RSStringCompareFlags compareFlags, RSRange* result) RS_AVAILABLE(0_0);

RSExport BOOL RSStringFind(RSStringRef aString, RSStringRef stringToFind, RSRange rangeToSearch, RSRange* result) RS_AVAILABLE(0_0);
RSExport RSRange *RSStringFindAll(RSStringRef aString, RSStringRef stringToFind, RSIndex *numberOfResult) RS_AVAILABLE(0_3);

RSExport BOOL RSStringFindCharacterFromSet(RSStringRef theString, RSCharacterSetRef theSet, RSRange rangeToSearch, RSStringCompareFlags searchOptions, RSRange *result) RS_AVAILABLE(0_4);

RSExport void RSStringDelete(RSMutableStringRef aString, RSRange range) RS_AVAILABLE(0_0);

RSExport void RSStringTrim(RSMutableStringRef string, RSStringRef trimString) RS_AVAILABLE(0_4);
RSExport void RSStringTrimWhitespace(RSMutableStringRef string) RS_AVAILABLE(0_4);
RSExport void RSStringTrimInCharacterSet(RSMutableStringRef string, RSCharacterSetRef characterSet) RS_AVAILABLE(0_4);
RSExport void RSStringLowercase(RSMutableStringRef string, RSLocaleRef locale) RS_AVAILABLE(0_4);
RSExport void RSStringUppercase(RSMutableStringRef string, RSLocaleRef locale) RS_AVAILABLE(0_4);

RSExport RSIndex RSStringGetBytes(RSStringRef str, RSRange range, RSStringEncoding encoding, uint8_t lossByte, BOOL isExternalRepresentation, void *buffer, RSIndex maxBufLen, RSIndex *usedBufLen) RS_AVAILABLE(0_0);
RSExport void RSStringAppendString(RSMutableStringRef aString, RSStringRef append) RS_AVAILABLE(0_0);
RSExport void RSStringAppendStringWithFormat(RSMutableStringRef aString, RSStringRef format,...) RS_AVAILABLE(0_0);
RSExport void RSStringAppendCString(RSMutableStringRef aString, RSCBuffer cString, RSStringEncoding encoding) RS_AVAILABLE(0_0);
RSExport void RSStringAppendCharacters(RSMutableStringRef aString, const UniChar* characters, RSIndex numChars) RS_AVAILABLE(0_0);
RSExport void RSStringInsert(RSMutableStringRef aString, RSIndex location, RSStringRef insert) RS_AVAILABLE(0_3);
//
//RSExport void RSStringCyclicShift(RSMutableStringRef aString, RSRange range, RSIndex n, BOOL left) RS_AVAILABLE(0_0);
//RSExport void RSStringReverse(RSMutableStringRef aString, RSRange range, RSIndex n) RS_AVAILABLE(0_0);
//
//
RSExport const char* RSStringCopyUTF8String(RSStringRef str) RS_AVAILABLE(0_0);
RSExport const char* RSStringGetUTF8String(RSStringRef str) RS_AVAILABLE(0_0);
RSExport RSIndex RSStringGetCString(RSStringRef str, char *buffer, RSIndex bufferSize, RSStringEncoding encoding) RS_AVAILABLE(0_0);
RSExport const char* RSStringGetCStringPtr(RSStringRef str, RSStringEncoding encoding) RS_AVAILABLE(0_0);
RSExport const UniChar *RSStringGetCharactersPtr(RSStringRef str) RS_AVAILABLE(0_0);
RSExport UniChar RSStringGetCharacterAtIndex(RSStringRef aString, RSIndex index) RS_AVAILABLE(0_0);
RSExport void RSStringGetCharacters(RSStringRef aString, RSRange range, UniChar* buffer) RS_AVAILABLE(0_0);
//
RSExport BOOL RSStringHasPrefix(RSStringRef aString, RSStringRef prefix) RS_AVAILABLE(0_0);
RSExport BOOL RSStringHasSuffix(RSStringRef aString, RSStringRef suffix) RS_AVAILABLE(0_0);
//
//
//RSExport BOOL RSStringIsIpAddress(RSStringRef aString) RS_AVAILABLE(0_0);
//RSExport RSStringEncoding RSStringFileSystemEncoding() RS_AVAILABLE(0_3);
RSExport RSArrayRef RSStringCreateComponentsSeparatedByStrings(RSAllocatorRef allocator, RSStringRef string, RSStringRef separatorString) RS_AVAILABLE(0_3);
RSExport RSArrayRef RSStringCreateComponentsSeparatedByCharactersInSet(RSAllocatorRef allocator, RSStringRef string, RSCharacterSetRef charactersSet) RS_AVAILABLE(0_4);

RSExport RSStringRef RSStringCreateByCombiningStrings(RSAllocatorRef allocator, RSArrayRef array, RSStringRef separatorString) RS_AVAILABLE(0_3);
RSExport RSDataRef RSStringCreateExternalRepresentation(RSAllocatorRef allocator, RSStringRef aString, RSStringEncoding encoding, RSIndex n) RS_AVAILABLE(0_3);
RSExport RSIndex RSStringGetMaximumSizeForEncoding(RSIndex length, RSStringEncoding encoding) RS_AVAILABLE(0_0);
//
//
#if defined(DEBUG)
//RSExport void RSStringShow(RSStringRef aString) RS_AVAILABLE(0_0);
//RSExport void RSStringShowContent(RSStringRef aString) RS_AVAILABLE(0_0);
RSExport void RSStringTest(RSStringRef s);
#endif

RSExport RSStringRef RSStringCopyDetailDescription(RSStringRef str) RS_AVAILABLE(0_4);
//
RSExport void RSStringCacheRelease(void) RS_AVAILABLE(0_0);

RSExport RSStringRef __RSStringMakeConstantString(const char * cStr) RS_AVAILABLE(0_0);
#define  RSSTR(cStr) __RSStringMakeConstantString("" cStr "")

#if defined(RSStringEncodingSupport)
RSExport RSStringRef RSStringCreateConvert(RSStringRef aString, RSStringEncoding toEncoding) RS_AVAILABLE(0_0);
RSExport void RSMutableStringConvert(RSMutableStringRef aString, RSStringEncoding toEncoding) RS_AVAILABLE(0_0);
#endif

/* UTF-16 surrogate support
 */
RSInline BOOL RSStringIsSurrogateHighCharacter(UniChar character) {
    return ((character >= 0xD800UL) && (character <= 0xDBFFUL) ? YES : NO);
}

RSInline BOOL RSStringIsSurrogateLowCharacter(UniChar character) {
    return ((character >= 0xDC00UL) && (character <= 0xDFFFUL) ? YES : NO);
}

RSInline UTF32Char RSStringGetLongCharacterForSurrogatePair(UniChar surrogateHigh, UniChar surrogateLow) {
    return (UTF32Char)(((surrogateHigh - 0xD800UL) << 10) + (surrogateLow - 0xDC00UL) + 0x0010000UL);
}

// Maps a UTF-32 character to a pair of UTF-16 surrogate characters. The buffer pointed by surrogates has to have space for at least 2 UTF-16 characters. Returns YES if mapped to a surrogate pair.
RSInline BOOL RSStringGetSurrogatePairForLongCharacter(UTF32Char character, UniChar *surrogates) {
    if ((character > 0xFFFFUL) && (character < 0x110000UL)) { // Non-BMP character
        character -= 0x10000;
        if (nil != surrogates) {
            surrogates[0] = (UniChar)((character >> 10) + 0xD800UL);
            surrogates[1] = (UniChar)((character & 0x3FF) + 0xDC00UL);
        }
        return YES;
    } else {
        if (nil != surrogates) *surrogates = (UniChar)character;
        return NO;
    }
}


RS_EXTERN_C_END
#endif
