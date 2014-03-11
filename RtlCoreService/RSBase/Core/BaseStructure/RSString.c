//
//  RSString.c
//  RSCoreFoundation
//
//  Created by RetVal on 10/24/12.
//  Copyright (c) 2012 RetVal. All rights reserved.
//

#include <strings.h>
#include <wchar.h>
#include <RSCoreFoundation/RSBase.h>
#include <RSCoreFoundation/RSAllocator.h>
#include <RSCoreFoundation/RSNil.h>
#include <RSCoreFoundation/RSString.h>
#include <RSCoreFoundation/RSDictionary.h>
#include <RSCoreFoundation/RSNumber.h>

#include <RSCharacterSet.h>

#include <RSCoreFoundation/RSRuntime.h>
#include "RSPrivate/CString/RSPrivateBuffer.h"
#include "RSPrivate/CString/RSPrivateStringFormat.h"
#include "RSPrivate/CString/RSString/RSUnicodeDecomposition.h"
#include "RSPrivate/CString/RSString/RSUnicodePrecomposition.h"
#include "RSPrivate/CString/RSString/RSStringEncodingConverter.h"
#include "RSPrivate/CString/RSString/RSFoundationEncoding.h"

//struct __RSString {
//    RSRuntimeBase _base;
//    
//    RSIndex _length;
//    RSIndex _capacity;
//    union {
//        RSCBuffer _inline_store;    // the memory is in stack, can not be released by self.
//        RSBuffer _bytes_store;     // the memory is in heap, but I do not know if strong or weak.
//        RSWBuffer _unicode_store;
//    }_string;
//    
//};

/* (1,0)
 1 : mutable or constant
 2 : strong or weak
 3 : inline or not inline
 4 : 
 5 :
 6 :
 7 :
 8 :
 */

static BOOL __RSStringAvailable(RSStringRef aString)
{
    if (aString == nil) HALTWithError(RSInvalidArgumentException, "the RSStringRef is nil.");
    if (aString == (RSStringRef)RSNil) return NO;
    __RSGenericValidInstance(aString, 7);
    return YES;
}
//
//enum {
//    _RSStringMutable   = 1 << 0, _RSStringMutableMask = _RSStringMutable,
//    _RSStringStrong    = 1 << 1, _RSStringStrongMask = _RSStringStrong,
//    _RSStringInline    = 1 << 2, _RSStringInlineMask = _RSStringInline,
//    _RSStringSpecial   = 1 << 3, _RSStringSpecialMask = _RSStringSpecial,
//    _RSStringCString   = 1 << 4, _RSStringCStringMask = _RSStringCString,
//    _RSStringUnicodeString = 1 << 5,
//    _RSStringUTF8String = 1 << 6,
//};
//
//RSInline BOOL  isMutable(RSStringRef string) {
//    return ((string->_base._rsinfo._rsinfo1 & _RSStringMutable) == _RSStringMutableMask);
//}
//
//RSInline void   markMutable(RSStringRef string) {
//    (((RSMutableStringRef)string)->_base._rsinfo._rsinfo1 |= _RSStringMutableMask);
//}
//
//RSInline void   unmarkMutable(RSMutableStringRef string) {
//    (string->_base._rsinfo._rsinfo1 &= ~_RSStringMutableMask);
//}
//
//RSInline BOOL  isSpecial(RSStringRef string) {
//    return ((string->_base._rsinfo._rsinfo1 & _RSStringSpecial) == _RSStringSpecialMask);
//}
//
//RSInline void   markSpecial(RSStringRef string) {
//    (((RSMutableStringRef)string)->_base._rsinfo._rsinfo1 |= _RSStringSpecialMask);
//}
//
//RSInline void   unmarkSpecial(RSMutableStringRef string) {
//    (string->_base._rsinfo._rsinfo1 &= ~_RSStringSpecialMask);
//}
//
//RSInline BOOL  isStrong(RSStringRef string) {
//    return ((string->_base._rsinfo._rsinfo1 & _RSStringStrong) == _RSStringStrongMask);
//}
//
//RSInline void   markStrong(RSStringRef string) {
//    (((RSMutableStringRef)string)->_base._rsinfo._rsinfo1 |= _RSStringStrongMask);
//}
//
//RSInline void   unMarkStrong(RSMutableStringRef string) {
//    (string->_base._rsinfo._rsinfo1 &= ~_RSStringStrongMask);
//}
//
//RSInline BOOL  isInline(RSStringRef string) {
//    return ((string->_base._rsinfo._rsinfo1 & _RSStringInline) == _RSStringInlineMask);
//}
//
//RSInline void   markInline(RSStringRef string) {
//    (((RSMutableStringRef)string)->_base._rsinfo._rsinfo1 |= _RSStringInlineMask);
//}
//
//RSInline void   unmarkInline(RSMutableStringRef string) {
//    (string->_base._rsinfo._rsinfo1 &= ~_RSStringInlineMask);
//}
//
//RSInline BOOL   isCString(RSStringRef string)
//{
//    return ((string->_base._rsinfo._rsinfo1 & _RSStringCStringMask) == _RSStringCString);
//}
//
//RSInline void   setCString(RSStringRef string)
//{
//    (((RSMutableStringRef)string)->_base._rsinfo._rsinfo1 |= _RSStringCStringMask);
//}
//
//RSInline void   unSetCString(RSStringRef string)
//{
//    (((RSMutableStringRef)string)->_base._rsinfo._rsinfo1 &= ~_RSStringCStringMask);
//}
//
//RSInline BOOL isUnicode(RSStringRef string)
//{
//    return ((string->_base._rsinfo._rsinfo1 & _RSStringUnicodeString) == _RSStringUnicodeString);
//}
//
//RSInline void   setUnicode(RSStringRef string)
//{
//    (((RSMutableStringRef)string)->_base._rsinfo._rsinfo1 |= _RSStringUnicodeString);
//}
//
//RSInline void   unSetUnicode(RSStringRef string)
//{
//    (((RSMutableStringRef)string)->_base._rsinfo._rsinfo1 &= ~_RSStringUnicodeString);
//}
//
//RSInline BOOL isUTF8String(RSStringRef string)
//{
//    return ((string->_base._rsinfo._rsinfo1 & _RSStringUTF8String) == _RSStringUTF8String);
//}
//
//RSInline void   setUTF8String(RSStringRef string)
//{
//    (((RSMutableStringRef)string)->_base._rsinfo._rsinfo1 |= _RSStringUTF8String);
//}
//
//RSInline void   unSetUTF8String(RSStringRef string)
//{
//    (((RSMutableStringRef)string)->_base._rsinfo._rsinfo1 &= ~_RSStringUTF8String);
//}
//
//RSInline void* __RSStringContent(RSStringRef string)
//{
//    if (isInline(string)) {
//        return (void*)string->_string._inline1._buffer;
//    }
//    return string->_string._unicode._unicodePtr;
//}
//
//RSInline void __RSStringSetStrongPtr(RSMutableStringRef aString, RSCUBuffer cString)
//{
//    aString->_string._inline1._buffer = (RSUBuffer)cString;
//    markStrong(aString);
//}
//
//RSInline RSIndex __RSStringByteLength(RSStringRef aString)
//{
//    return aString->_byteLength;
//}
//
//static void __RSStringUpdateCount(RSStringRef aString);
//RSInline RSIndex __RSStringCharactersLength(RSStringRef aString)
//{
//    if (aString->_length == 0)
//        __RSStringUpdateCount(aString);
//    return aString->_length;
//}
//
//RSInline RSIndex __RSStringCapacity(RSStringRef aString)
//{
//    return aString->_capacity;
//}
//
//RSInline void __RSStringSetByteLength(RSMutableStringRef aString, RSIndex length)
//{
//    ((RSMutableStringRef)aString)->_byteLength = length;
//    
//    if (NO == isInline(aString)) {
//        UTF8Char* p = __RSStringContent(aString);
//        p[length] = '\0';
//    }
//    
//    __RSStringUpdateCount(aString);
//}
//
//RSInline void __RSStringSetCapacity(RSMutableStringRef aString, RSIndex capacity)
//{
//    ((RSMutableStringRef)aString)->_capacity = capacity;
//}
//
//
//
RS_CONST_STRING_DECL(_RSEmptyString, "");
//// class methods
//
//static RSCUBuffer   __RSStringCreateCopyStrongBlock(RSAllocatorRef allocator, RSIndex capacity, RSCUBuffer cString, RSIndex length)
//{
//    RSCUBuffer _cString = nil;
//    _cString = RSAllocatorCopy(allocator, capacity, (RSHandle)cString, length);
//    return _cString;
//}
//
//static void     __RSStringResizeCapacity(RSMutableStringRef aString, RSAllocatorRef allocator, RSIndex needSpace)
//{
//    if (isStrong(aString))
//    {
//        RSIndex capacity = __RSStringCapacity(aString);
//        RSIndex newCapacity = RSAllocatorSize(RSAllocatorSystemDefault, needSpace);;
//        if (newCapacity == needSpace) newCapacity += 1; // for the terminal flag
//        if (capacity < newCapacity)
//        {
//            RSUBuffer ptr = (RSUBuffer)__RSStringContent(aString);
//            ptr = RSAllocatorReallocate(RSAllocatorSystemDefault, ptr, newCapacity);
//            if (ptr != nil)
//            {
//                __RSStringSetCapacity(aString, newCapacity);
//                __RSStringSetStrongPtr(aString, ptr);
//            }
//            else HALTWithError(RSInvalidArgumentException, "__RSStringResizeCapacity:RSAllocatorReallocate failed.");
//        }
//        else
//        {
//            RSIndex length = __RSStringByteLength(aString);
//            length = length < newCapacity ? length : newCapacity;   // get the small one to copy the content.
//            RSUBuffer ptr = nil;
//            RSUBuffer oldPtr = (RSUBuffer)__RSStringContent(aString);
//            ptr = RSAllocatorCopy(RSAllocatorSystemDefault, length + 1, (RSHandle)oldPtr, length + 1);
//            RSAllocatorDeallocate(RSAllocatorSystemDefault, oldPtr);      // release the old strong buffer pointer
//            __RSStringSetStrongPtr(aString, ptr);                   // set the new strong buffer pointer
//            
//            __RSStringSetByteLength(aString, length);                   // set the new length of the string
//            __RSStringSetCapacity(aString, newCapacity); // set the new capacity of buffer
//        }
//    }
//}
//
//static void __RSStringClassInit(RSTypeRef obj)
//{
//    
//    ((RSRuntimeBase*)obj)->_rsinfo._customRef = 0;
//    return;
//}
//
//static RSTypeRef __RSStringClassCopy(RSAllocatorRef allocator, RSTypeRef rs, BOOL mutableCopy)
//{
//    if (mutableCopy) {
//        RSMutableStringRef copy = RSStringCreateMutableCopy(allocator, 0, rs);
//        return copy;
//    }
//    return RSStringCreateCopy(allocator, rs);
//}
//
//static void __RSStringClassDeallocate(RSTypeRef obj)
//{
//    if (obj == nil) HALTWithError(RSInvalidArgumentException, "the object is nil");
//    if (obj == _RSEmptyString) return;
//    RSStringRef aString = (RSStringRef)obj;
//    //__RSCLog(RSLogLevelDebug, "RSStringClassDeallocate");__RSCLog(RSLogLevelDebug, "(%s)\t",__RSStringContent(aString));
//    if (isStrong(aString)) {
//        RSAllocatorDeallocate(RSAllocatorSystemDefault, aString->_string._inline1._buffer);
//    }
//    //RSAllocatorDeallocate(RSAllocatorSystemDefault, aString);
//    //RSMmLogUnitNumber();
//}
//
//static BOOL __RSStringClassEqual(RSTypeRef obj1, RSTypeRef obj2)
//{
//    if (obj1 == nil || obj2 == nil) HALTWithError(RSInvalidArgumentException, "the obj1 or obj2 is nil");
//    if (obj1 == (RSTypeRef)RSNil || obj2 == (RSTypeRef)RSNil) return NO;
//    if (obj1 == obj2) return YES;
//    RSStringRef string1 = (RSStringRef)obj1;
//    RSStringRef string2 = (RSStringRef)obj2;
//    RSCUBuffer buf1 = __RSStringContent(string1);
//    RSCUBuffer buf2 = __RSStringContent(string2);
//    if (buf1 == buf2) return YES;
//    if (buf1 == nil || buf2 == nil) return NO;
//    RSIndex  length1 = __RSStringByteLength(string1);
//    if (length1 != __RSStringByteLength(string2)) return NO;
//    if (strncmp((RSCBuffer)buf1, (RSCBuffer)buf2, length1) == 0) return YES;
//    return NO;
//}
////extern RSStringRef _RSEmptyString;
//static RSStringRef __RSStringClassDescription(RSTypeRef obj)
//{
//    RSStringRef str = (RSStringRef)obj;
//    if (str == _RSEmptyString) return RSSTR("");//return RSSTR("\"\"");
//    if (RSStringEncodingUTF8 == RSStringGetEncoding(str) || RSStringEncodingASCII == RSStringGetEncoding(str))
//    {
//        return RSRetain(str);
//    }
//    else
//    {
//        RSStringRef conv = RSStringCreateConvert(str, RSStringEncodingUTF8);
//        if (conv) {
//            return conv;
//        }
//        return RSRetain(str);
//    }
//    HALTWithError(RSInvalidArgumentException, "the object is not a string");
//    return RSStringGetEmptyString();
//}
//static RSUInteger __RSStringClassRefcount(intptr_t op, RSTypeRef obj)
//{
//    if (isSpecial(obj)) return 1;
//    RSBitU32 ref = 0;
//#if __LP64__
//    
//    ref = ((RSRuntimeBase*)obj)->_rc;
//#else
//    
//    ref = ((RSRuntimeBase*)obj)->_rsinfo._rc;
//#endif
//    if (op == -1 && ref == 1)
//        return 0;
//    switch (op) {
//        case +1:
//            __RSRuntimeRetain(obj, NO);
//            break;
//        case -1:
//            __RSRuntimeRelease(obj, NO);
//            break;
//            
//        default:
//            break;
//    }
//    return __RSRuntimeGetRetainCount(obj);
//}
//
//#include "RSBaseHash.h"
//static RSHashCode __RSStringClassHash(RSTypeRef obj)
//{
//    RSStringRef str = (RSStringRef)obj;
//    RSHashCode code = 0;
//    if (RSBaseHash(RSBlizzardHash, (void*)RSStringGetCStringPtr(str), RSStringGetLength(str), &code, sizeof(RSHashCode)))
//        return code;
//    return (RSHashCode)obj;
//}
//
//static RSRuntimeClass __RSStringClass = {
//    _RSRuntimeScannedObject,
//    0,
//    "RSString",
//    __RSStringClassInit,
//    __RSStringClassCopy,
//    __RSStringClassDeallocate,
//    __RSStringClassEqual,
//    __RSStringClassHash,
//    __RSStringClassDescription,
//    nil,
//    __RSStringClassRefcount,
//};
//
//static RSTypeID __RSStringTypeID = _RSRuntimeNotATypeID;
//
//
//
//RSTypeID RSStringGetTypeID()
//{
//    return __RSStringTypeID;
//}
//
//RSExport RSStringRef RSStringGetEmptyString()
//{
//    return (_RSEmptyString);
//}
//
////#include <langinfo.h>
//extern void __RSStringInitialize();
//RSPrivate void __RSStringInitialize()
//{
//    __RSStringTypeID = __RSRuntimeRegisterClass(&__RSStringClass);
//    __RSRuntimeSetClassTypeID(&__RSStringClass, __RSStringTypeID);
//    __RSStringInitialize();
////    __RSCLog(RSLogLevelNotice, "%s", nl_langinfo(CODESET));
//}
//
//static void __RSStringUpdateCount(RSStringRef aString)
//{
//    if (nil == aString || (RSStringRef)RSNil == aString) return;
//    switch (aString->_encoding)
//    {
//        case RSStringEncodingUTF8:
//            ((struct __RSString *)aString)->_length = __RSGetUTF8Length(aString->_string._inline1._buffer);
//            break;
//        case RSStringEncodingASCII:
//            ((struct __RSString *)aString)->_length = aString->_byteLength;
//            break;
//        case RSStringEncodingUTF16:
//            ((struct __RSString*)aString)->_string._unicode._size = 0;
//            //FIXME: UNICODE SIZE CALCULATE
//            ((struct __RSString*)aString)->_length = aString->_string._unicode._size;
//            break;
//        default:
//            break;
//    }
//}
//
//static void __RSStringUpdateEncodingMask(RSStringRef aString)
//{
//    switch (RSStringGetEncoding(aString))
//    {
//        case RSStringEncodingUTF16:
//            setUnicode(aString);
//            break;
//        case RSStringEncodingASCII:
//            setCString(aString);
//            break;
//        case RSStringEncodingUTF8:
//        default:
//            setUTF8String(aString);
//            break;
//    }
//}
//
//RSPrivate void RSStringSetEncoding(RSStringRef string, RSStringEncoding encoding)
//{
//    __RSGenericValidInstance(string, RSStringGetTypeID());
//    if (RSStringEncodingASCII == encoding)
//        encoding = RSStringEncodingUTF8;
//    ((RSMutableStringRef)string)->_encoding = encoding;
//    __RSStringUpdateEncodingMask(string);
//}
//
//static RSMutableStringRef __RSStringCreateInstance(RSAllocatorRef allocator, RSCUBuffer cString, RSIndex length, RSIndex capacity, BOOL _strong, BOOL _mutable, BOOL _inline, BOOL _needCopy, RSStringEncoding _encoding)
//{
//    if (cString == nil) HALTWithError(RSInvalidArgumentException, "the cString is nil.");
//    //if (strlen((RSCBuffer)cString) < length && capacity == 0) HALTWithError(RSGenericException, "the length is not allowed."); // capacity default as 0 and mean just use the length,
//    /*
//     the cString is bytes, if the bytes is mesns a c string, the length will be strlen(cString), otherwise, the length is the bytes length.
//     */
//    // otherwise should use the capacity to allocate the memory block by allocator
//    if (_mutable && _inline) HALTWithError(RSInvalidArgumentException, "mutable and inline flag is confilecct");
//    RSMutableStringRef  mutableString = (RSMutableStringRef)__RSRuntimeCreateInstance(allocator, __RSStringTypeID, sizeof(struct __RSString) - sizeof(struct __RSRuntimeBase));  //create the string instance.
//    if (mutableString == nil)
//    {
//        return nil;
//    }
//    if (YES == __RSStringEncodingAvailable(_encoding))
//    {
//        mutableString->_encoding = _encoding;
//    }
//    if (_strong)
//    {
//        if (_needCopy)  // if need deep copy or not
//        {
//            RSIndex zoneSize = (capacity == 0) ? ((capacity > length) ? (capacity) : (length + 1)) : (capacity);
//            if (zoneSize == length) zoneSize ++;
//            zoneSize = RSAllocatorSize(RSAllocatorSystemDefault, zoneSize);
//            __RSStringSetStrongPtr(mutableString, __RSStringCreateCopyStrongBlock(RSAllocatorSystemDefault, zoneSize, cString, length));
//            __RSStringSetCapacity(mutableString, zoneSize);
//        }
//        else
//        {
//            if (_inline && !_mutable) // is constant ?
//            {
//                mutableString->_string._inline1._buffer = (void *)cString;
//                markInline(mutableString);
//                __RSStringSetCapacity(mutableString, length);
//            }
//            else
//            {
//                mutableString->_string._inline1._buffer = (RSUBuffer)cString;
//                __RSStringSetCapacity(mutableString, capacity);
//            }
//            markStrong(mutableString);
//        }
//        
//        __RSStringSetByteLength(mutableString, length);
//    }else
//    {
//        if (_inline)
//        {
//            mutableString->_string._inline1._buffer = (RSBuffer)cString;
//            markInline(mutableString);
//        }
//        else mutableString->_string._inline1._buffer = (RSUBuffer)cString;
//        unMarkStrong(mutableString);
//        
//        __RSStringSetCapacity(mutableString, length);
//        __RSStringSetByteLength(mutableString, length);
//    }
//    markMutable(mutableString);
//    switch (_encoding)
//    {
//        case RSStringEncodingASCII:
//            setCString(mutableString);
//            break;
//        case RSStringEncodingUnicode:
//            setUnicode(mutableString);
//            break;
//        case RSStringEncodingUTF8:
//            setUTF8String(mutableString);
//            break;
//        default:
//            RSMutableStringConvert(mutableString, RSStringEncodingUnicode);
//            setUnicode(mutableString);
//            break;
//    }
//    if (!_mutable)
//    {
//        unmarkMutable(mutableString);
//    }
//    //__RSCLog(RSLogLevelDebug, "RSStringClassInit");__RSCLog(RSLogLevelDebug, "(%s)\t",__RSStringContent(mutableString));RSMmLogUnitNumber();
//    return mutableString;
//}
//
//// strong
//// mutable
//// inline
//// need copy
//
//
////warning   inline && mutable => HALTWithError(RSGenericException, "")
////          only strong instance will see the need copy flag.
//
//static volatile RSSpinLock __RSStringConstantTableLock = RSSpinLockInit;
//static RSMutableDictionaryRef __RSStringConstantTable = nil;
//// redesign the dictionary, so the string constant table need not use the private APIs which designed for the table.
//// just set its context for the dictionary, the dictionary know how to do with the key - value pairs.
////extern void __RSDictionarySetEqualCallBack(RSMutableDictionaryRef dictionary, RSKeyCompare compare);
////extern void __RSDictionarySetObjectForCKey(RSMutableDictionaryRef dictionary, RSCBuffer key, RSTypeRef object);
//
//static const void* __RSStringCStringRetain(const void* value)
//{
//    return value;   // just return itself, it just a c string.
//}
//
//static void __RSStringCStringRelease(const void* value)
//{
//    return ;        // do nothing because the c string is constant inline string.
//}
//
//static RSStringRef __RSStringCStringCopyDescription(const void *value)
//{
//    return RSStringCreateWithCString(RSAllocatorSystemDefault, value, RSStringEncodingUTF8);
//}
//
//static const void* __RSStringCStringCreateCopy(RSAllocatorRef allocator, const void* value)
//{
//    void* copy = nil;
//    if (value) {
//        copy = RSAllocatorAllocate(RSAllocatorSystemDefault, strlen(value) + 1);
//    }
//    return copy;
//}
//
//static RSHashCode __RSStringCStringHash(const void *value)
//{
//    return (RSHashCode)value;
//}
//
//static RSComparisonResult __RSStringCStringCompare(const void *value1, const void *value2)
//{
//    if (value1 && value2)
//    {
//        RSCBuffer str1 = value1;
//        RSCBuffer str2 = value2;
//        int result = strcmp(str1, str2);
//        if (result == 0) return RSCompareEqualTo;
//        else if (result < 0) return RSCompareLessThan;
//        return RSCompareGreaterThan;
//    }
//    HALTWithError(RSInvalidArgumentException, "the compare objects have nil object");   // never run here because the __RSStringMakeConstantString return nil if the c string is nil. so, the keys in table are always not nil.
//    return RSCompareEqualTo;
//}
//
//RSPrivate const RSDictionaryKeyContext ___RSConstantCStringKeyContext = {
//    __RSStringCStringRetain,
//    __RSStringCStringRelease,
//    __RSStringCStringCopyDescription,
//    __RSStringCStringCreateCopy,
//    __RSStringCStringHash,
//    __RSStringCStringCompare
//};
//
//static const RSDictionaryValueContext ___RSConstantStringValueContext = {
//    RSRetain,
//    RSRelease,
//    RSDescription,
//    RSCopy,
//    RSHash,
//    (RSDictionaryEqualCallBack)RSStringCompare
//};
//
//// the key is always a c string.
//// the value is always a RSStringRef.
//static const RSDictionaryContext ___RSStringConstantTableContext = {
//    0,
//    &___RSConstantCStringKeyContext,
//    &___RSConstantStringValueContext
//};
//
//static const RSDictionaryContext* __RSStringConstantTableContext = &___RSStringConstantTableContext;
//
//static  void __RSStringAvailableRange(RSStringRef aString, RSRange range)
//{
//    RSIndex length = __RSStringByteLength(aString);
//    if (length >= (range.location + range.length)) return;
//    HALTWithError(RSRangeException, "the range is not allowed.");
//}
//
//RSExport RSStringRef __RSStringMakeConstantString(RSCBuffer cStr)
//{
//    if (nil == __RSStringConstantTable) {
//        __RSStringConstantTable = RSDictionaryCreateMutable(RSAllocatorSystemDefault,2500,__RSStringConstantTableContext);
//        //__RSDictionarySetEqualCallBack(__RSStringConstantTable, __RSStringCStringCompare);
////        char* key = "%R";
////        RSStringRef str = RSStringCreateWithCString(RSAllocatorSystemDefault, key);
////        RSDictionarySetValue(__RSStringConstantTable, key, str);
////        RSRelease(str);
//    }
//    if (nil == cStr) {
//        return nil;
//    }
//
//    RSSpinLockLock(&__RSStringConstantTableLock);
//    
//    RSStringRef string = (RSStringRef)RSDictionaryGetValue(__RSStringConstantTable, cStr);
//    
//    RSSpinLockUnlock(&__RSStringConstantTableLock);
//    if (string)
//    {
//        // string return from RSDictionaryGetValue with retain. so we should release it first
//        //__RSCLog(RSLogLevelDebug, "{%s} from %s\n",__RSStringContent(string),"return from __RSStringConstantTable");
//        //RSRelease(string);
//        //__RSCLog(RSLogLevelDebug, "retain count is %ld\n",RSGetRetainCount(string));
//        return string;
//    }
//    //__RSCLog(RSLogLevelDebug, "{%s} create\n",cStr);
//    string = __RSStringCreateInstance(RSAllocatorSystemDefault, (RSCUBuffer)cStr, strlen(cStr), 0, NO, NO, YES, NO,  RSStringEncodingUTF8);
//    if (string == nil) HALTWithError(RSInvalidArgumentException, "RSRuntime halt with no more memory to use.");
//    markSpecial(string);
//    __RSRuntimeSetInstanceSpecial(string, YES);
//    //__RSCLog(RSLogLevelDebug, "retain count is %ld\n",RSGetRetainCount(string));
//    RSSpinLockLock(&__RSStringConstantTableLock);
//    RSDictionarySetValue(__RSStringConstantTable, cStr, string);RSRelease(string);
//    RSSpinLockUnlock(&__RSStringConstantTableLock);
//    
//    return string;
//}
//
//RSExport RSIndex RSStringGetLength(RSStringRef aString)
//{
//    if (__RSStringAvailable(aString) == NO) return 0;
//    return __RSStringCharactersLength(aString);
//}
//
//RSExport RSIndex RSStringGetLength(RSStringRef aString)
//{
//    if (__RSStringAvailable(aString) == NO) return 0;
//    return __RSStringByteLength(aString);
//}
//
//RSExport RSStringRef RSStringCreateWithCString(RSAllocatorRef allocator, RSCBuffer cString, RSStringEncoding encoding)
//{
//    if (cString == nil) HALTWithError(RSInvalidArgumentException, "the cString is nil");
//    if ('\0' == cString) return (RSStringRef)RSRetain(_RSEmptyString);
//    RSStringRef str = __RSStringCreateInstance(allocator, (RSCUBuffer)cString, strlen(cString), 0, YES, NO, NO, YES,  encoding);
//    return str;
//}
//
//RSExport RSStringRef RSStringCreateWithCStringNoCopy(RSAllocatorRef allocator, RSCBuffer cString, RSStringEncoding encoding)
//{
//    if (cString == nil) HALTWithError(RSInvalidArgumentException, "the cString is nil");
//    if ('\0' == cString) return (RSStringRef)RSRetain(_RSEmptyString);
//    RSStringRef str = __RSStringCreateInstance(allocator, (RSCUBuffer)cString, strlen(cString), 0, YES, NO, NO, NO,  encoding);
//    return str;
//}
//static RSStringRef  __RSStringCreateWithFormateAndArguments(RSAllocatorRef allocator, RSIndex size, RSStringRef format, va_list arguments)
//{
//    RSRetain(format);
//    RSCUBuffer __format = __RSStringContent(format);
//    RSIndex length = size ?: 64;
//    RSBuffer __formatString = nil;
//    
//    RSIndex result = 0;
//    va_list copy ;
//    //RSIndex moreLength = 0;
//    while (1)    //core
//    {
//        va_copy(copy, arguments);
//        result = __RSFormatPrint(&__formatString, &length, __format, copy);
//        //result = __RSFormatPrint(__formatString, length, &moreLength, __format, copy);
//        if (result > -1 && result < length )
//        {
//            RSRelease(format);
//            va_end(copy);
//            return __RSStringCreateInstance(allocator, (RSUBuffer)__formatString, strlen(__formatString), length, YES, NO, NO, NO  , RSStringEncodingUTF8);//RSStringCreateWithCStringNoCopy(allocator, __formatString);
//        }
//        va_end(copy);
//        //length += moreLength;
//        //length += 1; if (length == RSUIntegerMax) break;   // fix the overflow bug.
//        //RSAllocatorDeallocate(allocator, __formatString);
//        //__formatString = RSAllocatorReallocate(allocator, __formatString, length);
//        //length = RSAllocatorSize(allocator, length);
//    }
//    RSAllocatorDeallocate(allocator, __formatString);
//    RSRelease(format);
//    return (RSStringRef)RSNil;
//}
//
//RSStringRef __RSStringCreateWithFormat(RSAllocatorRef allocator, RSIndex size, RSStringRef format, ...)
//{
//    va_list args ;
//    va_start(args, format);
//    RSStringRef aString = __RSStringCreateWithFormateAndArguments(allocator, size, format, args);
//    va_end(args);
//    return aString;
//}
//
//RSExport RSStringRef RSStringCreateWithFormat(RSAllocatorRef allocator, RSStringRef format, ...)
//{
//    if (format == nil) HALTWithError(RSInvalidArgumentException, "the format is nil");
//    va_list args ;
//    va_start(args, format);
//    RSStringRef aString = RSStringCreateWithFormatAndArguments(allocator, 64, format, args);
//    va_end(args);
//    return aString;
//}
//
//RSExport RSStringRef RSStringCreateWithFormatAndArguments(RSAllocatorRef allocator, RSIndex capacity, RSStringRef format, va_list arguments)
//{
//    if (format == nil) HALTWithError(RSInvalidArgumentException, "the format is nil");
//    return __RSStringCreateWithFormateAndArguments(allocator, capacity, format, arguments);
//}
//
//RSExport RSStringRef RSStringCreateWithBytes(RSAllocatorRef allocator, RSCUBuffer bytes, RSIndex numBytes, RSStringEncoding encoding)
//{
//    if (bytes == nil || numBytes == 0) return nil;
//    return __RSStringCreateInstance(allocator, bytes, numBytes, 0, YES, NO, NO, YES, encoding);
//}
//
//RSExport RSStringRef RSStringCreateWithBytesNoCopy(RSAllocatorRef allocator, RSCUBuffer bytes, RSIndex numBytes, RSStringEncoding encoding)
//{
//    if (bytes == nil || numBytes == 0) return nil;
//    return __RSStringCreateInstance(allocator, bytes, numBytes, 0, YES, NO, NO, NO, encoding);
//}
//
//RSExport RSStringRef RSStringCreateWithCharacters(RSAllocatorRef allocator, const UniChar* characters, RSIndex numChars)
//{
//    if (characters == nil || numChars == 0) return nil;
//    return __RSStringCreateInstance(allocator, (RSCUBuffer)characters, numChars, 0, YES, NO, NO, YES, RSStringEncodingUnicode);
//}
//
//RSExport RSStringRef RSStringCreateWithCharactersWithNoCopy(RSAllocatorRef allocator, const UniChar* characters, RSIndex numChars)
//{
//    if (characters == nil || numChars == 0) return nil;
//    return __RSStringCreateInstance(allocator, (RSCUBuffer)characters, numChars, 0, YES, NO, NO, NO, RSStringEncodingUnicode);
//}
//
//static RSStringRef __RSStringCreateCopy(RSAllocatorRef allocator, RSStringRef aString, BOOL mutableCopy)
//{
//    RSStringRef copy = nil;
//    copy = __RSStringCreateInstance(allocator, __RSStringContent(aString), __RSStringByteLength(aString), 0, YES, mutableCopy, NO, YES, RSStringEncodingUTF8);
//    return copy;
//}
//RSExport RSStringRef RSStringCreateCopy(RSAllocatorRef allocator, RSStringRef aString)
//{
//    if (aString == nil) HALTWithError(RSInvalidArgumentException, "the string is nil");
//    if (aString == (RSStringRef)RSNil) return (RSStringRef)RSNil;
//    RSStringRef copy = nil;
//    return copy = __RSStringCreateCopy(allocator, aString, NO);
//}
//
//RSExport RSStringRef RSStringCreateWithSubstring(RSAllocatorRef allocator, RSStringRef aString, RSRange range)
//{
//    if (aString == nil) HALTWithError(RSInvalidArgumentException, "the string is nil");
//    __RSStringAvailable(aString);
//    __RSStringAvailableRange(aString, range);
//    
//    RSStringRef sub = RSStringCreateWithBytes(allocator, __RSStringContent(aString) + range.location, range.length, aString->_encoding);
//    return sub;
//}
//
//RSExport RSMutableStringRef RSStringCreateMutable(RSAllocatorRef allocator, RSIndex maxLength)
//{
//    if (maxLength == 0) maxLength = 32;
//    RSMutableStringRef mutableString = __RSStringCreateInstance(allocator, (RSCUBuffer)"", 0, maxLength, YES, YES, NO, YES, RSStringEncodingUTF8);
//    return mutableString;
//}
//
//
//
//
//
//static  void __RSStringMoveString(RSMutableStringRef aMutableString, RSRange rangeFrom, RSRange rangeTo)
//{
////    RSRetain(aMutableString);
////    RSBuffer base = (RSBuffer)__RSStringContent(aMutableString);
////    RSIndex length = __RSStringLength(aMutableString);
////    RSIndex endOfFrom = rangeFrom.location + rangeFrom.length;
////    RSIndex endOfTo = rangeTo.location + rangeTo.length;
////    if (endOfFrom < rangeTo.location || rangeFrom.location > endOfTo) // 不重叠
////    {
////        
////    }
////    RSRelease(aMutableString);
//}
//
//static  void __RSStringRepalceString(RSMutableStringRef aMutableString, RSRange range, RSStringRef replacement)
//{
//    if (aMutableString == nil) return;
//    if (replacement == nil) return;
//    if (replacement == aMutableString) {
//        //do something
//    }
//    //RSStringRef copy = RSStringCreateCopy(RSAllocatorSystemDefault, replacement);
//    RSRetain(replacement);
//    RSIndex capcity = __RSStringCapacity(aMutableString);
//    RSIndex length = __RSStringByteLength(aMutableString);
//    RSIndex replaceLength = __RSStringByteLength(replacement);
//    RSIndex overWriteLength = 0;
//    if (length < range.location + range.length)
//    {
//        if (range.location == 0 && length == 0)
//        {
//            //__RSCLog(RSLogLevelNotice, "%s", "__RSStringRepalceString : the range is out of the mutable string.\n");
//            overWriteLength = range.location + range.length - length;
//        }
//        else HALTWithError(RSInvalidArgumentException, "the operation is no meaning.");
//    }
//    
//    RSIndex newLength = length - (range.length) + replaceLength;   // 1 is the size of the end flag.
//    newLength = newLength ? newLength + overWriteLength: overWriteLength;
//    if (newLength > capcity)
//    {
//        __RSStringResizeCapacity(aMutableString, RSAllocatorSystemDefault, newLength + 1);
//        //capcity = __RSStringCapacity(aMutableString);
//    }
//
//    // update the mutable string buffer, and it has the enough capacity to contain the content
//    RSBuffer __mutableBuffer = (RSBuffer)__RSStringContent(aMutableString);
//    
//    if (range.length == replaceLength)
//    {
//        __builtin_memmove(__mutableBuffer + range.location, __RSStringContent(replacement), replaceLength);
//    }
//    else if (range.length < replaceLength)
//    {
//        RSIndex offset = replaceLength - range.length;
//        __builtin_memmove( __mutableBuffer + range.location + range.length + offset, __mutableBuffer + range.location + range.length, length - range.location - range.length);
//        __builtin_memmove(__mutableBuffer + range.location, __RSStringContent(replacement), replaceLength);
//        
//    }
//    else //if (range.length > replaceLength)
//    {
//        //RSIndex offset = range.length - replaceLength;
//        __builtin_memset(__mutableBuffer + range.location, 0, range.length);
//        __builtin_memcpy(__mutableBuffer + range.location, __RSStringContent(replacement), replaceLength);
//        // 10.9 strncat can not do the work
////        strncat(__mutableBuffer + range.location + replaceLength, __mutableBuffer + range.location + range.length, newLength - replaceLength);
//        __builtin_memmove(__mutableBuffer + range.location + replaceLength, __mutableBuffer + range.location + range.length, newLength - replaceLength);
//        //memmove(__mutableBuffer + range.location, __RSStringContent(replacement), replaceLength);
//    }
//    //RSRelease(copy);
//    RSRelease(replacement);
//    __RSStringSetByteLength(aMutableString, length - range.length + replaceLength + overWriteLength);
//}
//
//RSExport RSMutableStringRef RSStringCreateMutableCopy(RSAllocatorRef allocator, RSIndex maxLength, RSStringRef aString)
//{
//    if (maxLength == 0 || maxLength < RSStringGetLength(aString)) maxLength = RSStringGetLength(aString) + 1;
//    RSMutableStringRef  mutableString = RSStringCreateMutable(allocator, maxLength);
//    RSStringSetEncoding(mutableString, RSStringGetEncoding(aString));
//    __RSStringRepalceString(mutableString, RSMakeRange(0, RSStringGetLength(aString)), aString);
//    return mutableString;
//}
//
//RSExport RSMutableStringRef RSStringReplace(RSMutableStringRef mutableString, RSRange range, RSStringRef aString)
//{
//    if (mutableString == aString) return mutableString;
//    if (isMutable(mutableString) == NO) HALTWithError(RSInvalidArgumentException, "the string is not mutable");
//    __RSStringAvailableRange(mutableString, range);
//    __RSStringRepalceString(mutableString, range, aString);
//    return mutableString;
//}
//
//RSExport RSMutableStringRef RSStringReplaceAll(RSMutableStringRef mutableString, RSStringRef find, RSStringRef replace)
//{
//    RSIndex numberOfResult = 0;
//    RSRange *rgs = RSStringFindAll(mutableString, find, &numberOfResult);
//    if (rgs)
//    {
//        RSIndex pos = RSStringGetLength(replace) - RSStringGetLength(find);
//        for (RSIndex idx = 0; idx < numberOfResult; idx++)
//        {
//            RSRange range = rgs[idx];
//            range.location += pos * idx;
//            __RSStringRepalceString(mutableString, range, replace);
//        }
//        RSAllocatorDeallocate(RSAllocatorSystemDefault, rgs);
//    }
//    return mutableString;
//}
//
//RSExport RSMutableStringRef RSStringReplaceAllInRange(RSMutableStringRef mutableString, RSStringRef find, RSStringRef replace, RSRange range, RSIndex *replacedCount)
//{
//    RSMutableStringRef source = nil;
//    if (range.location == 0 && range.length == RSStringGetLength(mutableString))
//        source = (RSMutableStringRef)(mutableString);
//    else
//    {
//        RSStringRef sub = RSStringCreateWithSubstring(RSAllocatorSystemDefault, mutableString, range);
//        source = RSMutableCopy(RSAllocatorSystemDefault, sub);
//        RSRelease(sub);
//    }
//    RSIndex numberOfResult = 0;
//    RSRange *rgs = RSStringFindAll(source, find, &numberOfResult);
//    if (rgs)
//    {
//        RSIndex pos = RSStringGetLength(replace) - RSStringGetLength(find);
//        for (RSIndex idx = 0; idx < numberOfResult; idx++)
//        {
//            RSRange _range = rgs[idx];
//            _range.location += pos * idx + range.location;
//            __RSStringRepalceString(mutableString, _range, replace);
//        }
//        RSAllocatorDeallocate(RSAllocatorSystemDefault, rgs);
//    }
//    if (replacedCount) *replacedCount = numberOfResult;
//    if (source != mutableString) RSRelease(source);
//    return mutableString;
//}
//static RSStringRef __RSStringGetSubStringWithRange(RSAllocatorRef allocator, RSStringRef aString, RSRange range)
//{
//    //RSIndex length = __RSStringLength(aString);
//
//    __RSStringAvailableRange(aString, range);
//    RSStringRef str = nil;
//    RSBuffer ptr = (RSBuffer)__RSStringContent(aString);
//    str = __RSStringCreateInstance(allocator, (RSUBuffer)ptr + range.location, range.length, 0, YES, NO, NO, YES, RSStringEncodingUTF8);
//    return str;
//    //return (RSStringRef)RSRetain(_RSEmptyString);
//}
//RSExport RSStringRef RSStringCreateSubStringWithRange(RSAllocatorRef allocator, RSStringRef aString, RSRange range)
//{
//    if (__RSStringAvailable(aString)) {
//        return __RSStringGetSubStringWithRange(allocator, aString, range);
//    }
//    return (RSStringRef)RSRetain(_RSEmptyString);
//}
//
//static BOOL __RSStringFindWithOptions(RSStringRef aString, RSStringRef stringToFind, RSRange rangeToSearch, RSStringCompareFlags compareFlags, RSRange* result)
//{
//    BOOL caseInsensitive = (compareFlags & RSCompareCaseInsensitive) ? (YES) : (NO);
//    BOOL backwards = (compareFlags & RSCompareBackwards) ? (YES) : (NO);
//    BOOL numberically = (compareFlags & RSCompareNumerically) ? (YES) : (NO);
//    BOOL anchored = (compareFlags & RSCompareAnchored) ? (YES) : (NO);
//    BOOL findResult = NO;
//    RSIndex _numberOfRanges __unused = 0;
//    RSAllocatorRef allocator = RSGetAllocator(aString);
//    RSMutableStringRef srcStr = nil, findString = RSStringCreateMutableCopy(allocator, 0, stringToFind);;
//    if (anchored)
//    {
//        RSStringRef tmp = RSStringCreateSubStringWithRange(RSGetAllocator(aString), aString, rangeToSearch);
//        srcStr = RSMutableCopy(RSAllocatorSystemDefault, tmp);
//        RSRelease(tmp);
//    }
//    else
//    {
//        srcStr = RSStringCreateMutableCopy(allocator, 0, aString);
//    }
//    // src, find is not nil now.
//    RSIndex length = __RSStringByteLength(srcStr);
//    RSIndex lengthOfFind = __RSStringByteLength(findString);
//    do
//    {
//        if (length < lengthOfFind)
//        {
//            findResult = NO;
//            break;
//        }
//        if (caseInsensitive)
//        {
//            RSStringMakeLower(srcStr, RSMakeRange(0, length));
//            RSStringMakeLower(findString, RSMakeRange(0, lengthOfFind));
//        }
//        if (backwards)
//        {
//            RSStringReverse(srcStr, RSStringGetRange(srcStr), length);
//            RSStringReverse(findString, RSStringGetRange(findString), lengthOfFind);
//        }
//        RSStringEncoding encd1 = RSStringGetEncoding(srcStr);
//        RSStringEncoding encd2 = RSStringGetEncoding(findString);
//        
//        if (!((encd1 == RSStringEncodingUTF8 || encd1 == RSStringEncodingASCII) &&
//            (encd2 == RSStringEncodingUTF8 || encd2 == RSStringEncodingASCII)))
//        {
//            RSMutableStringConvert(srcStr, RSStringEncodingUTF8);
//            RSMutableStringConvert(findString, RSStringEncodingUTF8);
//        }
//        findResult = RSStringFind(srcStr, findString, RSStringGetRange(srcStr), result);
//        if (findResult)
//        {
//        }
//        if (numberically)
//        {
//            
//        }
//        else
//        {
//            
//        }
//    } while (0);
//    
//    if (likely(srcStr != nil)) RSRelease(srcStr);
//    if (likely(findString != nil)) RSRelease(findString);
//    return findResult;
//}
//
//RSExport BOOL RSStringFind(RSStringRef aString, RSStringRef stringToFind, RSRange rangeToSearch, RSRange* result)
//{
//    if (result == nil) return NO;
//    if (!(__RSStringAvailable(aString) && __RSStringAvailable(stringToFind))) return NO;
//    __RSStringAvailableRange(aString, rangeToSearch);
//    BOOL find = NO;
//    register RSIndex length = __RSStringByteLength(stringToFind);
//    RSBuffer cStr = (RSBuffer)__RSStringContent(aString);
//    RSCUBuffer search = __RSStringContent(stringToFind);
//    RSUBlock cache = cStr[rangeToSearch.location + rangeToSearch.length];
//    if (cache != '\0') cStr[rangeToSearch.location + rangeToSearch.length] = 0;
//    RSIndex currentOffset = rangeToSearch.location;
//    register RSIndex offset = 0;
//    offset = __RSBufferFindSubString(cStr + currentOffset, (RSCBuffer)search);
//    if (offset != RSNotFound) find = YES;
//    if (result)
//    {
//        if (offset != RSNotFound)
//        {
//            result->location = offset;
//            result->length = length;
//        }
//        else {
//            *result = RSMakeRange(RSNotFound, RSNotFound);
//        }
//    }
//    if (cache != '\0') cStr[rangeToSearch.location + rangeToSearch.length] = cache;
//    return find;
//}
//
//static RSRange * __RSStringFindToRanges(RSStringRef string, RSStringRef separatorString, RSIndex *num)
//{
//    if (string && separatorString && num && __RSStringAvailable(string) && __RSStringAvailable(separatorString))
//    {
//        return __RSBufferFindSubStrings(__RSStringContent(string), __RSStringContent(separatorString), num);
//    }
//    return nil;
//}
//
//RSExport RSRange *RSStringFindAll(RSStringRef aString, RSStringRef stringToFind, RSIndex *numberOfResult)
//{
//    return __RSStringFindToRanges(aString, stringToFind, numberOfResult);
//}
//
//RSExport BOOL RSStringFindWithOptions(RSStringRef aString, RSStringRef stringToFind, RSRange rangeToSearch, RSStringCompareFlags compareFlags, RSRange* result)
//{
//    if ((__RSStringAvailable(stringToFind) & __RSStringAvailable(aString)) == NO) return NO;
//    if (result == nil) return NO;
//    return __RSStringFindWithOptions(aString, stringToFind, rangeToSearch, compareFlags, result);
//}
//
//RSExport RSRange RSStringGetRange(RSStringRef aString)
//{
//    if (__RSStringAvailable(aString)) {
//        return RSMakeRange(0, __RSStringByteLength(aString));
//    }
//    return RSMakeRange(0, 0);
//}
//
//
//
//RSExport void RSStringMakeUpper(RSMutableStringRef aString, RSRange range)
//{
//    if (__RSStringAvailable(aString) && isMutable(aString))
//    {
//        __RSStringAvailableRange(aString, range);
//        RSBuffer ptr = (RSBuffer)__RSStringContent(aString);
//        if (aString->_encoding == RSStringEncodingUTF8 || aString->_encoding == RSStringEncodingASCII)__RSBufferMakeUpper(ptr + range.location, range.length);
//        return;
//    }
//    HALTWithError(RSInvalidArgumentException, "string is unavailable/string is not mutable");
//}
//
//RSExport void RSStringMakeLower(RSMutableStringRef aString, RSRange range)
//{
//    if (__RSStringAvailable(aString) && isMutable(aString))
//    {
//        __RSStringAvailableRange(aString, range);
//        RSBuffer ptr = (RSBuffer)__RSStringContent(aString);
//        if (aString->_encoding == RSStringEncodingUTF8 || aString->_encoding == RSStringEncodingASCII)__RSBufferMakeLower(ptr + range.location, range.length);
//        return;
//    }
//    HALTWithError(RSInvalidArgumentException, "string is unavailable/string is not mutable");
//}
//
//RSExport void RSStringMakeCapital(RSMutableStringRef aString, RSRange range)
//{
//    if (__RSStringAvailable(aString) && isMutable(aString))
//    {
//        __RSStringAvailableRange(aString, range);
//        RSBuffer ptr = (RSBuffer)__RSStringContent(aString);
//        if (aString->_encoding == RSStringEncodingUTF8 || aString->_encoding == RSStringEncodingASCII)__RSBufferMakeCapital(ptr + range.location, range.length);
//        return;
//    }
//    HALTWithError(RSInvalidArgumentException, "string is unavailable/string is not mutable");
//
//}
//
//RSExport void RSStringDelete(RSMutableStringRef aString, RSRange range)
//{
//    if (__RSStringAvailable(aString) && isMutable(aString)) {
//        __RSStringAvailableRange(aString, range);
//        RSIndex length = __RSStringByteLength(aString);
//        RSBuffer buf = (RSBuffer)__RSStringContent(aString);
//        __RSBufferDeleteRange(buf, length, range);
//        if (range.location == 0 && range.length == length)
//        {
//            __RSStringSetByteLength(aString, 0);
//        }
//        else if (range.location + range.length == length)
//        {
//            __RSStringSetByteLength(aString, length - range.length);
//        }
//        else {
//            __RSStringSetByteLength(aString, length - range.length);
//        }
//        return;
//    }
//    HALTWithError(RSInvalidArgumentException, "string is unavailable/string is not mutable");
//}
//
//RSExport RSRange RSStringGetRangeWithSubString(RSStringRef aString, RSStringRef subString)
//{
//    RSRange result = {RSNotFound, RSNotFound};
//    RSStringFind(aString, subString, RSMakeRange(0, RSStringGetLength(aString)), &result);
//    return result;
//}
//
//static void __RSStringAppendCString(RSMutableStringRef aString, RSCBuffer buffer, RSIndex size)
//{
//    RSBuffer ptr = (RSBuffer)__RSStringContent(aString);
////    if (size > strlen(buffer)) {
////        HALTWithError(RSGenericException, "the size that want to append is error.");
////    }
//    //strncat(ptr, buffer, size);
//    RSIndex cur = __RSStringByteLength(aString);
//    strncpy(ptr + cur, buffer, size);
//    *(RSBuffer)(ptr + cur + size) = (RSBlock)0;
//    __RSStringSetByteLength(aString, cur + size);
//}
//
//static void __RSStringAppendCharacters(RSMutableStringRef aString, UniChar *buffer, RSIndex size)
//{
//    
//}
//
//static void __RSStringAppendString(RSMutableStringRef aString, RSStringRef append)
//{
//    RSRetain(aString);
//    RSRetain(append);
//    RSIndex capacity = __RSStringCapacity(aString);
//    RSIndex appendLength = __RSStringByteLength(append);
//    RSIndex length = __RSStringByteLength(aString);
//    RSIndex overLength = 0;
//    if (capacity == (length + appendLength)) overLength = 1;
//    if (capacity < (length + appendLength + overLength))
//    {
//        RSIndex needSpace = length + appendLength + overLength; // here is not reserved space for the terminal flag '\0'
//        __RSStringResizeCapacity(aString, RSAllocatorSystemDefault, needSpace);  // check here
//    }
//
//    RSBuffer appStr = (RSBuffer)__RSStringContent(append);
//    __RSStringAppendCString(aString, appStr, appendLength);
//    RSRelease(aString);
//    RSRelease(append);
//    
//}
//
//RSExport void RSStringAppendString(RSMutableStringRef aString, RSStringRef append)
//{
//    if (__RSStringAvailable(aString) && isMutable(aString) && __RSStringAvailable(append)) {
//        return __RSStringAppendString(aString, append);
//    }
//}
//
//RSExport void RSStringAppendStringWithFormat(RSMutableStringRef aString, RSStringRef format,...)
//{
//    if (__RSStringAvailable(aString) && isMutable(aString)) {
//        va_list ap;
//        va_start(ap, format);
//        RSStringRef append = RSStringCreateWithFormatAndArguments(RSAllocatorSystemDefault, 64, format, ap);
//        RSStringAppendString(aString, append);
//        RSRelease(append);
//        va_end(ap);
//        return;
//    }
//    HALTWithError(RSInvalidArgumentException, "string is unavailable/string is not mutable");
//    return;
//}
//
//RSExport void RSStringAppendCString(RSMutableStringRef aString, RSCBuffer cString)
//{
////    RSStringRef append = RSStringCreateWithCString(RSAllocatorSystemDefault, cString);
////    RSStringAppendString(aString, append);
////    RSRelease(append);
//    if (__RSStringAvailable(aString) && isMutable(aString) && cString) {
//        RSIndex capacity = __RSStringCapacity(aString);
//        RSIndex appendLength = strlen(cString);
//        RSIndex length = __RSStringByteLength(aString);
//        RSIndex overLength = 0;
//        if (capacity == (length + appendLength)) overLength = 1;
//        if (capacity < (length + appendLength + overLength))
//        {
//            __RSStringResizeCapacity(aString, RSAllocatorSystemDefault, length + appendLength + overLength);
//        }
//        __RSStringAppendCString(aString, cString, appendLength);
//    }
//}
//
//RSExport void RSStringAppendCharacters(RSMutableStringRef aString, const UniChar* characters, RSIndex numChars)
//{
//    if (__RSStringAvailable(aString) && isMutable(aString) && characters)
//    {
//        RSIndex capacity = __RSStringCapacity(aString);
//        RSIndex appendLength = numChars;
//        RSIndex length = __RSStringByteLength(aString);
//        
//        RSIndex overLength = 0;
//        if (capacity == (length + appendLength)) overLength = 1;
//        if (capacity < (length + appendLength + overLength))
//        {
//            __RSStringResizeCapacity(aString, RSAllocatorSystemDefault, length + appendLength + overLength);
//        }
//        __RSStringAppendCString(aString, (RSCBuffer)characters, appendLength);
//    }
//}
//
//RSExport void RSStringInsert(RSMutableStringRef aString, RSIndex location, RSStringRef insert)
//{
//    if (__RSStringAvailable(aString) && isMutable(aString) && insert)
//    {
//        RSIndex length = RSStringGetLength(aString);
//        if (length < location) return;
//#if 1
//        RSIndex insertLength = RSStringGetLength(insert);
//        RSIndex capacity = __RSStringCapacity(aString);
//        RSIndex overLength = 0;
//        if (capacity == (length + insertLength)) overLength = 1;
//        if (capacity < insertLength + length + overLength)
//        {
//            __RSStringResizeCapacity(aString, RSAllocatorSystemDefault, insertLength + length + overLength);
//        }
//        memmove(__RSStringContent(aString) + location + insertLength, __RSStringContent(aString) + location, location);
//        __builtin_memcpy(__RSStringContent(aString) + location, __RSStringContent(insert), insertLength);
//#else
//        RSStringReplace(aString, RSMakeRange(location, length - location), RSAutorelease(RSStringCreateWithFormat(RSAllocatorSystemDefault, RSSTR("%R%R"), insert, RSAutorelease(RSStringCreateWithSubstring(RSAllocatorSystemDefault, aString, RSMakeRange(location, length - location))))));
//#endif
//    }
//}
//
//RSExport RSArrayRef RSStringCreateComponentsSeparatedByStrings(RSAllocatorRef allocator, RSStringRef string, RSStringRef separatorString)
//{
//    RSMutableArrayRef array = nil;
//    RSIndex number = 0;
//    RSRange *rgs = __RSStringFindToRanges(string, separatorString, &number);
//    if (rgs)
//    {
//        RSIndex numChars = 0;
//        RSIndex startIndex = 0;
//        array = RSArrayCreateMutable(allocator, number + 2);
//        RSStringRef subString = nil;
//        for (RSIndex idx = 0; idx < number; idx++)
//        {
//            RSRange range = rgs[idx];
//            numChars = range.location - startIndex;
//            subString = RSStringCreateWithSubstring(allocator, string, RSMakeRange(startIndex, numChars));
//            if (subString) RSArrayAddObject(array, subString);
//            startIndex = range.location + range.length;
//            RSRelease(subString);
//        }
//        subString = RSStringCreateWithSubstring(allocator, string, RSMakeRange(startIndex, RSStringGetLength(string) - startIndex));
//        if (subString) RSArrayAddObject(array, subString);
//        RSRelease(subString);
//        RSAllocatorDeallocate(RSAllocatorSystemDefault, rgs);
//    }
//    else
//    {
//        array = (RSMutableArrayRef)RSArrayCreateWithObject(allocator, string);
//    }
//    return array;
//}
//
//RSExport RSStringRef RSStringCreateByCombiningStrings(RSAllocatorRef allocator, RSArrayRef array, RSStringRef separatorString)
//{
//    if (array == nil) return nil;
//    RSIndex cnt = RSArrayGetCount(array);
//    RSMutableStringRef s = RSStringCreateMutable(allocator, 0);
//    for (RSIndex idx = 0; idx < cnt; idx++)
//    {
//        RSStringRef s1 = RSArrayObjectAtIndex(array, idx);
//        if (NO == __RSStringAvailable(s1)) continue;
//        RSStringAppendString(s, s1);
//        if (separatorString) RSStringAppendString(s, separatorString);
//    }
//    return s;
//}
//
//RSExport RSDataRef RSStringCreateExternalRepresentation(RSAllocatorRef allocator, RSStringRef aString, RSStringEncoding encoding, RSIndex n)
//{
//    RSStringRef s = RSStringCreateConvert(aString, encoding);
//    RSDataRef data = RSDataCreate(allocator, (const UTF8Char *)RSStringGetCStringPtr(s), RSStringGetLength(s));
//    RSRelease(s);
//    return data;
//}
//
//RSExport void RSStringCyclicShift(RSMutableStringRef aString, RSRange range, RSIndex n, BOOL left)
//{
//    if (__RSStringAvailable(aString) && isMutable(aString)) {
//        __RSStringAvailableRange(aString, range);
//        RSBuffer buf = (RSBuffer)__RSStringContent(aString);
//        return (left) ? (__RSBufferTrunLeft(buf + range.location, n, range.length)) : (__RSBufferTrunRight(buf + range.location, n, range.length));
//    }
//    HALTWithError(RSInvalidArgumentException, "string is unavailable/string is not mutable");
//}
//
//RSExport void RSStringReverse(RSMutableStringRef aString, RSRange range, RSIndex n)
//{
//    if (__RSStringAvailable(aString) && isMutable(aString))
//    {
//        RSIndex left = n % __RSStringByteLength(aString);
//        if (left == 0) left = __RSStringByteLength(aString);
//        __RSBufferReverse(__RSStringContent(aString) + range.location,0,left-1);
//    }
//}
//
//RSExport RSCBuffer RSStringCopyCString(RSStringRef aString)
//{
//    if (__RSStringAvailable(aString)) {
//        RSBuffer ptr = nil;
//        if ((ptr = RSAllocatorCopy(RSAllocatorSystemDefault, __RSStringByteLength(aString)+1, (RSHandle)__RSStringContent(aString), __RSStringByteLength(aString))))
//            return ptr;
//        else return nil;
//    }
//    return nil;
//}
//RSExport RSCBuffer RSStringGetCStringPtr(RSStringRef aString)
//{
//    if (__RSStringAvailable(aString)) {
//        return (RSCBuffer)__RSStringContent(aString);
//    }
//    return nil;
//}
//
//RSPrivate RSCBuffer __RSStringGetCStringWithNoCopy(RSStringRef aString)
//{
//    return (RSCBuffer)__RSStringContent(aString);
//}
//
//RSExport RSComparisonResult RSStringCompare(RSStringRef aString, RSStringRef other)
//{
//    if (__RSStringAvailable(aString) && __RSStringAvailable(other))
//    {
//       // RSIndex length = min(__RSStringLength(aString), __RSStringLength(other));
//        int result = strcmp((RSCBuffer)__RSStringContent(aString), (RSCBuffer)__RSStringContent(other));
//        if (result > 0) return RSCompareGreaterThan;
//        else if (result < 0)return RSCompareLessThan;
//        else if (result == 0) return RSCompareEqualTo;
//    }
//    HALTWithError(RSInvalidArgumentException, "nevere got here.");// if the string is not available, the __RSStringAvailable will raise an exception and asm (int 3).
//    return RSCompareEqualTo;
//}
//
//RSExport RSComparisonResult RSStringCompareCaseInsensitive(RSStringRef aString, RSStringRef other) RS_AVAILABLE(0_0)
//{
//    if ((RSStringGetEncoding(aString) == RSStringGetEncoding(other)) && RSStringGetEncoding(aString) != RSStringEncodingUTF8)
//        HALTWithError(RSInvalidArgumentException, "go away without UTF8.");
//    if (__RSStringAvailable(aString) && __RSStringAvailable(other) )
//    {
//        int result = strcasecmp((RSCBuffer)__RSStringContent(aString), (RSCBuffer)__RSStringContent(other));
//        //FIXME: compare with other routine
//        if (result > 0) return RSCompareGreaterThan;
//        else if (result < 0) return RSCompareLessThan;
//        else if (result == 0) return RSCompareEqualTo;
//    }
//    HALTWithError(RSInvalidArgumentException, "never got here.");
//    return RSCompareEqualTo;
//}
//
//RSExport UniChar RSStringGetCharacterAtIndex(RSStringRef aString, RSIndex index)
//{
//    if (__RSStringAvailable(aString))
//    {
//        if (index >= 0 && __RSStringByteLength(aString)+1 >= index)
//        {
//            if (isUTF8String(aString) || isCString(aString))
//                return (UniChar)((UniChar*)__RSStringContent(aString))[index];
//            else if (isUnicode(aString))
//            {
//                return (UniChar)((UniChar*)__RSStringContent(aString))[index];
//            }
//        }
//    }
//    return (UniChar)'\0';
//}
//static void __RSStringGetCharacters(RSStringRef aString, RSRange range, UniChar* buffer)
//{
//    __builtin_memcpy(buffer, ((UniChar *)__RSStringContent(aString)) + range.location, range.length);
//}
//
//RSExport void RSStringGetCharacters(RSStringRef aString, RSRange range, UniChar* buffer)
//{
//    if (__RSStringAvailable(aString) && buffer)
//    {
//        __RSStringAvailableRange(aString, range);
//        __RSStringGetCharacters(aString, range, buffer);
//    }
//    return;
//}
//
//RSExport BOOL RSStringHasPrefix(RSStringRef aString, RSStringRef prefix)
//{
//    if (__RSStringAvailable(aString) && __RSStringAvailable(prefix))
//    {
//        if (RSStringGetEncoding(aString) == RSStringGetEncoding(prefix) &&
//            RSStringGetEncoding(prefix) == RSStringEncodingUTF8)
//        {
//            RSIndex len = 0;
//            if ((len = __RSStringByteLength(prefix)) > __RSStringByteLength(aString)) return NO;
//            return 0 == __builtin_memcmp(__RSStringContent(aString), __RSStringContent(prefix), len);
//        }
//    }
//    return NO;
//}
//
//RSExport BOOL RSStringHasSuffix(RSStringRef aString, RSStringRef suffix) RS_AVAILABLE(0_0)
//{
//    if (__RSStringAvailable(aString) && __RSStringAvailable(suffix))
//    {
//        if (RSStringGetEncoding(aString) == RSStringGetEncoding(suffix) &&
//            RSStringGetEncoding(suffix) == RSStringEncodingUTF8)
//        {
//            RSIndex len = 0;
//            if ((len = __RSStringByteLength(suffix)) > __RSStringByteLength(aString)) return NO;
//            if (len == __RSStringByteLength(aString))
//            {
//                return RSStringCompare(aString, suffix);
//            }
//            /*
//             abcdefg, fg
//             trun right with 2 length, get fgabcde
//             compare fgabcde, fg
//             trun left with 2 length, restore the aString to abcdefg
//             */
//            BOOL result = NO;
//            //                __RSBufferTrunRight(__RSStringContent(aString), len, __RSStringLength(aString));
//            //                result = 0 == __builtin_memcmp(__RSStringContent(aString), __RSStringContent(suffix), len);
//            //                __RSBufferTrunLeft(__RSStringContent(aString), len, __RSStringLength(aString));
//            result = 0 == __builtin_memcmp(__RSStringContent(aString) + __RSStringByteLength(aString) - len,
//                                           __RSStringContent(suffix),
//                                           len);
//            return result;
//        }
//    }
//    return NO;
//
//}
//
//RSExport BOOL RSStringIsIpAddress(RSStringRef aString)
//{
//    __RSStringAvailable(aString);
//    RSBuffer ipaddr = (RSBuffer)__RSStringContent(aString);
//    if (__RSStringByteLength(aString) > 15)
//        return NO;
//    // 3 + 1 + 3 + 1 + 3 + 1 + 3
//    char *pnum,*pdot = ipaddr;
//    for(;*ipaddr;ipaddr=pdot++)
//    {
//        int t=0,e=1;
//        if(*(pnum=pdot)=='.')return 0;
//        for(; *pdot!='.'&&*pdot; ++pdot);
//        for(ipaddr=pdot-1;ipaddr>=pnum;t+=e*(*ipaddr---'0'),e*=10);
//        if(t<0||t>255||(pdot-pnum==3&&t<100)||(pdot-pnum==2&&t<10))
//            return NO;
//    }
//    return YES;
//}
//
//static void __RSStringShow(RSStringRef aString, BOOL detail)
//{
//    if (detail)
//    {
//        if(NO == __RSStringAvailable(aString)) {fprintf(stdout,"unavailable String");return;}
//        if (__RSStringByteLength(aString))
//        {
//            if (aString->_encoding == RSStringEncodingUTF16 || aString->_encoding == RSStringEncodingUnicode)
//                fwprintf(stdout, L"%S\n", (wchar_t*)__RSStringContent(aString));
//            else
//                fprintf(stdout, "%s\n",__RSStringContent(aString));
//        }
//        else fprintf(stdout, "\n");
//        if (isStrong(aString)) fprintf(stdout, "Strong");
//        else fprintf(stdout, "Weak");
//        fprintf(stdout, "Memory\n");
//        fprintf(stdout, "%s string\n", isMutable(aString) ? ("mutable") :("constant"));
//        fprintf(stdout, "Length %lld\n", __RSStringCharactersLength(aString));
//        fprintf(stdout, "Byte %lld, Capacity %lld\n", __RSStringByteLength(aString), __RSStringCapacity(aString));
//    }
//    else
//    {
//        if (__RSStringByteLength(aString)) fprintf(stdout, "%s\n",__RSStringContent(aString));
//        else fprintf(stdout, "");
//        fprintf(stdout, "\n");
//    }
//}
//
//#if defined(DEBUG)
//RSExport void RSStringShowContent(RSStringRef aString)
//{
//    return __RSStringShow(aString, NO);
//}
//RSExport void RSStringShow(RSStringRef aString)
//{
//    return __RSStringShow(aString, YES);
//}
//
//#endif
//
//RSExport void RSStringCacheRelease()
//{
//    if(__RSStringConstantTable == nil) return;
//    RSSpinLockLock(&__RSStringConstantTableLock);
//    extern void __RSDictionaryCleanAllObjects(RSMutableDictionaryRef dictionary);
//    __RSDictionaryCleanAllObjects(__RSStringConstantTable);
//    RSRelease(__RSStringConstantTable);
//    __RSStringConstantTable = nil;
//    RSSpinLockUnlock(&__RSStringConstantTableLock);
//}

#include "RSPrivate/CString/RSString/RSUniChar.h"

#include "RSPrivate/CString/RSString/RSFoundationEncoding.h"

#define ALLOCATORSFREEFUNC ((RSAllocatorRef)-1)
static RSTypeID __RSStringTypeID = _RSRuntimeNotATypeID;
//RSInline BOOL __RSCanUseLengthByte(RSIndex len) {
//#define __RSMaxPascalStrLen 255
//    return (len <= __RSMaxPascalStrLen) ? YES : NO;
//}

enum {
    __RSFreeContentsWhenDoneMask = 0x020,
    __RSFreeContentsWhenDone = 0x020,
    __RSContentsMask = 0x060,
	__RSHasInlineContents = 0x000,
	__RSNotInlineContentsNoFree = 0x040,		// Don't free
	__RSNotInlineContentsDefaultFree = 0x020,	// Use allocator's free function
	__RSNotInlineContentsCustomFree = 0x060,		// Use a specially provided free function
    __RSHasContentsAllocatorMask = 0x060,
    __RSHasContentsAllocator = 0x060,		// (For mutable strings) use a specially provided allocator
    __RSHasContentsDeallocatorMask = 0x060,
    __RSHasContentsDeallocator = 0x060,
    __RSIsMutableMask = 0x01,
	__RSIsMutable = 0x01,
    __RSIsUnicodeMask = 0x10,
	__RSIsUnicode = 0x10,
    __RSHasNullByteMask = 0x08,
	__RSHasNullByte = 0x08,
    __RSHasLengthByteMask = 0x04,
	__RSHasLengthByte = 0x04,
    
};

RSInline BOOL __RSStrIsMutable(RSStringRef str)            { return (RSRuntimeClassBaseFiled(str) & __RSIsMutableMask) == __RSIsMutable;}
RSInline BOOL __RSStrIsInline(RSStringRef str)             { return (RSRuntimeClassBaseFiled(str) & __RSContentsMask) == __RSHasInlineContents;}
RSInline BOOL __RSStrFreeContentsWhenDone(RSStringRef str) { return (RSRuntimeClassBaseFiled(str) & __RSFreeContentsWhenDoneMask) == __RSFreeContentsWhenDone;}
RSInline BOOL __RSStrHasContentsDeallocator(RSStringRef str){ return (RSRuntimeClassBaseFiled(str) & __RSHasContentsDeallocatorMask) == __RSHasContentsDeallocator;}
RSInline BOOL __RSStrIsUnicode(RSStringRef str)            { return (RSRuntimeClassBaseFiled(str) & __RSIsUnicodeMask) == __RSIsUnicode;}
RSInline BOOL __RSStrIsEightBit(RSStringRef str)           { return (RSRuntimeClassBaseFiled(str) & __RSIsUnicodeMask) != __RSIsUnicode;}
RSInline BOOL __RSStrHasNullByte(RSStringRef str)          { return (RSRuntimeClassBaseFiled(str) & __RSHasNullByteMask) == __RSHasNullByte;}
RSInline BOOL __RSStrHasLengthByte(RSStringRef str)        { return (RSRuntimeClassBaseFiled(str) & __RSHasLengthByteMask) == __RSHasLengthByte;}
RSInline BOOL __RSStrHasExplicitLength(RSStringRef str)    { return (RSRuntimeClassBaseFiled(str) & (__RSIsMutableMask | __RSHasLengthByteMask)) != __RSHasLengthByte;}    // Has explicit length if (1) mutable or (2) not mutable and no length byte
RSInline BOOL __RSStrIsConstant(RSStringRef str)           { return __RSRuntimeInstanceIsSpecial(str);}
RSInline RSBitU32 __RSStrSkipAnyLengthByte(RSStringRef str){ return ((RSRuntimeClassBaseFiled(str) & __RSHasLengthByteMask) == __RSHasLengthByte) ? 1 : 0;}    // Number of bytes to skip over the length byte in the contents
RSInline const void* __RSStrContents(RSStringRef str) {
    if (__RSStrIsInline(str)) {
        return (const void *)(((uintptr_t)&(str->_variants)) + (__RSStrHasExplicitLength(str) ? sizeof(RSIndex) : 0));
    }
    return str->_variants._notInlineImmutable1._buffer;
}

static RSAllocatorRef *__RSStrContentsDeallocatorPtr(RSStringRef str) {
    return __RSStrHasExplicitLength(str) ? &(((RSMutableStringRef)str)->_variants._notInlineImmutable1._contentsDeallocator) : &(((RSMutableStringRef)str)->_variants._notInlineImmutable2._contentsDeallocator); }

// Assumption: Called with immutable strings only, and on strings that are known to have a contentsDeallocator
RSInline RSAllocatorRef __RSStrContentsDeallocator(RSStringRef str) {
    return *__RSStrContentsDeallocatorPtr(str);
}

RSInline void __RSStrSetContentsDeallocator(RSStringRef str, RSAllocatorRef allocator) {
    RSRetain(allocator);
    *__RSStrContentsDeallocatorPtr(str) = allocator;
}

static RSAllocatorRef *__RSStrContentsAllocatorPtr(RSStringRef str) {
    RSAssert(!__RSStrIsInline(str), __RSLogAssertion, "Asking for contents allocator of inline string");
    RSAssert(__RSStrIsMutable(str), __RSLogAssertion, "Asking for contents allocator of an immutable string");
    return (RSAllocatorRef *)&(str->_variants._notInlineMutable1._contentsAllocator);
}

RSInline RSAllocatorRef __RSStrContentsAllocator(RSMutableStringRef str) {
    return *(__RSStrContentsAllocatorPtr(str));
}

// Assumption: Called with strings that have a contents allocator; also, contents allocator follows custom
RSInline void __RSStrSetContentsAllocator(RSMutableStringRef str, RSAllocatorRef allocator) {
    RSRetain(allocator);
    *(__RSStrContentsAllocatorPtr(str)) = allocator;
}

/* Returns length; use __RSStrLength2 if contents buffer pointer has already been computed.
 */

RSInline RSIndex __RSStrLength(RSStringRef str)
{
    if (__RSStrHasExplicitLength(str))
    {
        if (__RSStrIsInline(str))
        {
            return str->_variants._inline1._length;
        }
        else
        {
            return str->_variants._notInlineImmutable1._length;
        }
    }
    else
    {
        return (RSIndex)(*((uint8_t *)__RSStrContents(str)));
    }
}

RSInline RSIndex __RSStrLength2(RSStringRef str, const void *buffer)
{
    if (__RSStrHasExplicitLength(str))
    {
        if (__RSStrIsInline(str))
        {
            return str->_variants._inline1._length;
        }
        else
        {
            return str->_variants._notInlineImmutable1._length;
        }
    }
    else
    {
        return (RSIndex)(*((uint8_t *)buffer));
    }
}

BOOL __RSStringIsEightBit(RSStringRef str) {
    return __RSStrIsEightBit(str);
}

/* Sets the content pointer for immutable or mutable strings.
 */
RSInline void __RSStrSetContentPtr(RSStringRef str, const void *p)
{
    // XXX_PCB catch all writes for mutable string case.
    __RSAssignWithWriteBarrier((void **)&((RSMutableStringRef)str)->_variants._notInlineImmutable1._buffer, (void *)p);
}
RSInline void __RSStrSetInfoBits(RSStringRef str, UInt32 v)		{__RSBitfieldSetValue(RSRuntimeClassBaseFiled(str), 6, 0, v);}

RSInline void __RSStrSetExplicitLength(RSStringRef str, RSIndex v)
{
    if (__RSStrIsInline(str))
    {
        ((RSMutableStringRef)str)->_variants._inline1._length = v;
    }
    else
    {
        ((RSMutableStringRef)str)->_variants._notInlineImmutable1._length = v;
    }
}

RSInline void __RSStrSetUnicode(RSMutableStringRef str)		    {RSRuntimeClassBaseFiled(str) |= __RSIsUnicode;}
RSInline void __RSStrClearUnicode(RSMutableStringRef str)		    {RSRuntimeClassBaseFiled(str) &= ~__RSIsUnicode;}
RSInline void __RSStrSetHasLengthAndNullBytes(RSMutableStringRef str)	    {RSRuntimeClassBaseFiled(str) |= (__RSHasLengthByte | __RSHasNullByte);}
RSInline void __RSStrClearHasLengthAndNullBytes(RSMutableStringRef str)    {RSRuntimeClassBaseFiled(str) &= ~(__RSHasLengthByte | __RSHasNullByte);}

// Assumption: The following set of inlines (using str->variants.notInlineMutable) are called with mutable strings only
RSInline BOOL __RSStrIsFixed(RSStringRef str)   		{return str->_variants._notInlineMutable1._isFixedCapacity;}
RSInline BOOL __RSStrIsExternalMutable(RSStringRef str)	{return str->_variants._notInlineMutable1._isExternalMutable;}
RSInline BOOL __RSStrHasContentsAllocator(RSStringRef str)	{return (RSRuntimeClassBaseFiled(str) & __RSHasContentsAllocatorMask) == __RSHasContentsAllocator;}
RSInline void __RSStrSetIsFixed(RSMutableStringRef str)		    {str->_variants._notInlineMutable1._isFixedCapacity = 1;}
RSInline void __RSStrSetIsExternalMutable(RSMutableStringRef str)	    {str->_variants._notInlineMutable1._isExternalMutable = 1;}
RSInline void __RSStrSetHasGap(RSMutableStringRef str)			    {str->_variants._notInlineMutable1._hasGap = 1;}

// If capacity is provided externally, we only change it when we need to grow beyond it
RSInline BOOL __RSStrCapacityProvidedExternally(RSStringRef str)   		{return str->_variants._notInlineMutable1._capacityProvidedExternally;}
RSInline void __RSStrSetCapacityProvidedExternally(RSMutableStringRef str)	{str->_variants._notInlineMutable1._capacityProvidedExternally = 1;}
RSInline void __RSStrClearCapacityProvidedExternally(RSMutableStringRef str)	{str->_variants._notInlineMutable1._capacityProvidedExternally = 0;}

// "Capacity" is stored in number of bytes, not characters. It indicates the total number of bytes in the contents buffer.
RSInline RSIndex __RSStrCapacity(RSStringRef str)				{return str->_variants._notInlineMutable1._capacity;}
RSInline void __RSStrSetCapacity(RSMutableStringRef str, RSIndex cap)		{str->_variants._notInlineMutable1._capacity = cap;}

// "Desired capacity" is in number of characters; it is the client requested capacity; if fixed, it is the upper bound on the mutable string backing store.
RSInline RSIndex __RSStrDesiredCapacity(RSStringRef str)			{return str->_variants._notInlineMutable1._desiredCapacity;}
RSInline void __RSStrSetDesiredCapacity(RSMutableStringRef str, RSIndex size)	{str->_variants._notInlineMutable1._desiredCapacity = size;}


static void *__RSStrAllocateMutableContents(RSMutableStringRef str, RSIndex size) {
    void *ptr;
    RSAllocatorRef alloc = (__RSStrHasContentsAllocator(str)) ? __RSStrContentsAllocator(str) : RSGetAllocator(str);
    ptr = RSAllocatorAllocate(alloc, size);
//    if (__RSOASafe) __RSSetLastAllocationEventName(ptr, "RSString (store)");
    return ptr;
}

#define __RSGetAllocator(obj) RSGetAllocator(obj)
#define _RSAllocatorIsGCRefZero(allocator) NO

static void __RSStrDeallocateMutableContents(RSMutableStringRef str, void *buffer)
{
    RSAllocatorRef alloc = (__RSStrHasContentsAllocator(str)) ? __RSStrContentsAllocator(str) : __RSGetAllocator(str);
    if (__RSStrIsMutable(str) && __RSStrHasContentsAllocator(str) && _RSAllocatorIsGCRefZero(alloc))
    {
        // do nothing
    }
    else if (RS_IS_COLLECTABLE_ALLOCATOR(alloc))
    {
        // GC:  for finalization safety, let collector reclaim the buffer in the next GC cycle.
//        auto_zone_release(objc_collectableZone(), buffer);
    }
    else
    {
        RSAllocatorDeallocate(alloc, buffer);
    }
//    __RSStrEncodingCanBeStoredInEightBit();
}

/* Returns whether the indicated encoding can be stored in 8-bit chars
 */
RSInline BOOL __RSStrEncodingCanBeStoredInEightBit(RSStringEncoding encoding)
{
    switch (encoding & 0xFFF)
    {
        // just use encoding base
        case RSStringEncodingInvalidId:
        case RSStringEncodingUnicode:
        case RSStringEncodingNonLossyASCII:
            return NO;
            
        case RSStringEncodingMacRoman:
        case RSStringEncodingWindowsLatin1:
        case RSStringEncodingISOLatin1:
        case RSStringEncodingNextStepLatin:
        case RSStringEncodingASCII:
            return YES;
            
        default: return NO;
    }
}

RSInline BOOL __RSBytesInASCII(const uint8_t *bytes, RSIndex len) {
#if __LP64__
    /* A bit of unrolling; go by 32s, 16s, and 8s first */
    while (len >= 32) {
        uint64_t val = *(const uint64_t *)bytes;
        uint64_t hiBits = (val & 0x8080808080808080ULL);    // More efficient to collect this rather than do a conditional at every step
        bytes += 8;
        val = *(const uint64_t *)bytes;
        hiBits |= (val & 0x8080808080808080ULL);
        bytes += 8;
        val = *(const uint64_t *)bytes;
        hiBits |= (val & 0x8080808080808080ULL);
        bytes += 8;
        val = *(const uint64_t *)bytes;
        if (hiBits | (val & 0x8080808080808080ULL)) return NO;
        bytes += 8;
        len -= 32;
    }
    
    while (len >= 16) {
        uint64_t val = *(const uint64_t *)bytes;
        uint64_t hiBits = (val & 0x8080808080808080ULL);
        bytes += 8;
        val = *(const uint64_t *)bytes;
        if (hiBits | (val & 0x8080808080808080ULL)) return NO;
        bytes += 8;
        len -= 16;
    }
    
    while (len >= 8) {
        uint64_t val = *(const uint64_t *)bytes;
        if (val & 0x8080808080808080ULL) return NO;
        bytes += 8;
        len -= 8;
    }
#endif
    /* Go by 4s */
    while (len >= 4) {
        uint32_t val = *(const uint32_t *)bytes;
        if (val & 0x80808080U) return NO;
        bytes += 4;
        len -= 4;
    }
    /* Handle the rest one byte at a time */
    while (len--) {
        if (*bytes++ & 0x80) return NO;
    }
    
    return YES;
}

/* Returns whether the provided 8-bit string in the specified encoding can be stored in an 8-bit RSString.
 */
RSInline BOOL __RSCanUseEightBitRSStringForBytes(const uint8_t *bytes, RSIndex len, RSStringEncoding encoding)
{
    // If the encoding is the same as the 8-bit RSString encoding, we can just use the bytes as-is.
    // One exception is ASCII, which unfortunately needs to mean ISOLatin1 for compatibility reasons <rdar://problem/5458321>.
    if (encoding == __RSStringGetEightBitStringEncoding() && encoding != RSStringEncodingASCII) return YES;
    if (__RSStringEncodingIsSupersetOfASCII(encoding) && __RSBytesInASCII(bytes, len)) return YES;
    return NO;
}

/* Returns whether a length byte can be tacked on to a string of the indicated length.
 */
RSInline BOOL __RSCanUseLengthByte(RSIndex len) {
#define __RSMaxPascalStrLen 255
    return (len <= __RSMaxPascalStrLen) ? YES : NO;
}

/* Various string assertions
 */

#define __RSAssertIsString(RS) __RSGenericValidInstance(RS, __RSStringTypeID)
#define __RSAssertIndexIsInStringBounds(RS, idx) RSAssert3((idx) >= 0 && (idx) < __RSStrLength(RS), __RSLogAssertion, "%s(): string index %d out of bounds (length %d)", __PRETTY_FUNCTION__, idx, __RSStrLength(RS))
#define __RSAssertRangeIsInStringBounds(RS, idx, count) RSAssert4((idx) >= 0 && (idx + count) <= __RSStrLength(RS), __RSLogAssertion, "%s(): string range %d,%d out of bounds (length %d)", __PRETTY_FUNCTION__, idx, count, __RSStrLength(RS))
#define __RSAssertIsStringAndMutable(RS) {__RSGenericValidInstance(RS, __RSStringTypeID); RSAssert1(__RSStrIsMutable(RS), __RSLogAssertion, "%s(): string not mutable", __PRETTY_FUNCTION__);}
#define __RSAssertIsStringAndExternalMutable(RS) {__RSGenericValidInstance(RS, __RSStringTypeID); RSAssert1(__RSStrIsMutable(RS) && __RSStrIsExternalMutable(RS), __RSLogAssertion, "%s(): string not external mutable", __PRETTY_FUNCTION__);}
#define __RSAssertIsNotNegative(idx) RSAssert2(idx >= 0, __RSLogAssertion, "%s(): index %d is negative", __PRETTY_FUNCTION__, idx)
#define __RSAssertIfFixedLengthIsOK(RS, reqLen) RSAssert2(!__RSStrIsFixed(RS) || (reqLen <= __RSStrDesiredCapacity(RS)), __RSLogAssertion, "%s(): length %d too large", __PRETTY_FUNCTION__, reqLen)

/* Basic algorithm is to shrink memory when capacity is SHRINKFACTOR times the required capacity or to allocate memory when the capacity is less than GROWFACTOR times the required capacity.  This function will return -1 if the new capacity is just too big (> LONG_MAX).
 Additional complications are applied in the following order:
 - desiredCapacity, which is the minimum (except initially things can be at zero)
 - rounding up to factor of 8
 - compressing (to fit the number if 16 bits), which effectively rounds up to factor of 256
 - we need to make sure GROWFACTOR computation doesn't suffer from overflow issues on 32-bit, hence the casting to unsigned. Normally for required capacity of C bytes, the allocated space is (3C+1)/2. If C > ULONG_MAX/3, we instead simply return LONG_MAX
 */
#define SHRINKFACTOR(c) (c / 2)

#if __LP64__
#define GROWFACTOR(c) ((c * 3 + 1) / 2)
#else
#define GROWFACTOR(c) (((c) >= (ULONG_MAX / 3UL)) ? __RSMax(LONG_MAX - 4095, (c)) : (((unsigned long)c * 3 + 1) / 2))
#endif

RSInline RSIndex __RSStrNewCapacity(RSMutableStringRef str, unsigned long reqCapacity, RSIndex capacity, BOOL leaveExtraRoom, RSIndex charSize)
{
    if (capacity != 0 || reqCapacity != 0)
    {
        /* If initially zero, and space not needed, leave it at that... */
        if ((capacity < reqCapacity) ||		/* We definitely need the room... */
            (!__RSStrCapacityProvidedExternally(str) && 	/* Assuming we control the capacity... */
             ((reqCapacity < SHRINKFACTOR(capacity)) ||		/* ...we have too much room! */
              (!leaveExtraRoom && (reqCapacity < capacity)))))
        {
            /* ...we need to eliminate the extra space... */
            if (reqCapacity > LONG_MAX) return -1;  /* Too big any way you cut it */
            unsigned long newCapacity = leaveExtraRoom ? GROWFACTOR(reqCapacity) : reqCapacity;	/* Grow by 3/2 if extra room is desired */
            RSIndex desiredCapacity = __RSStrDesiredCapacity(str) * charSize;
            if (newCapacity < desiredCapacity)
            {
                /* If less than desired, bump up to desired */
                newCapacity = desiredCapacity;
            }
            else if (__RSStrIsFixed(str))
            {
                /* Otherwise, if fixed, no need to go above the desired (fixed) capacity */
                newCapacity = __RSMax(desiredCapacity, reqCapacity);	/* !!! So, fixed is not really fixed, but "tight" */
            }
            if (__RSStrHasContentsAllocator(str))
            {
                /* Also apply any preferred size from the allocator  */
                newCapacity = RSAllocatorSize(__RSStrContentsAllocator(str), newCapacity);
#if DEPLOYMENT_TARGET_MACOSX || DEPLOYMENT_TARGET_EMBEDDED || DEPLOYMENT_TARGET_EMBEDDED_MINI
            }
            else
            {
                #include <malloc/malloc.h>
                newCapacity = malloc_good_size(newCapacity);
#endif
            }
            return (newCapacity > LONG_MAX) ? -1 : (RSIndex)newCapacity; // If packing: __RSStrUnpackNumber(__RSStrPackNumber(newCapacity));
        }
    }
    return capacity;
}

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

RSInline void pop (RSStringStackInfo *si, RSStringDeferredRange *topRange)
{
    si->count = si->count - 1;
    *topRange = si->stack[si->count];
}

RSInline void push (RSStringStackInfo *si, const RSStringDeferredRange *newRange)
{
    if (si->count == si->capacity)
    {
        // increase size of the stack
        si->capacity = (si->capacity + 4) * 2;
        if (si->hasMalloced)
        {
            si->stack = (RSStringDeferredRange *)RSAllocatorReallocate(RSAllocatorSystemDefault, si->stack, si->capacity * sizeof(RSStringDeferredRange));
        }
        else
        {
            RSStringDeferredRange *newStack = (RSStringDeferredRange *)RSAllocatorAllocate(RSAllocatorSystemDefault, si->capacity * sizeof(RSStringDeferredRange));
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
    if (si.hasMalloced) RSAllocatorDeallocate (RSAllocatorSystemDefault, si.stack);
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
                       RSIndex insertLength)
{
    
    RSIndex srcLocationInBytes = 0;	// in order to avoid multiplying all the time, this is in terms of bytes, not blocks
    RSIndex dstLocationInBytes = 0;	// ditto
    RSIndex srcBlockSize = srcIsUnicode ? sizeof(UniChar) : sizeof(uint8_t);
    RSIndex insertLengthInBytes = insertLength * (dstIsUnicode ? sizeof(UniChar) : sizeof(uint8_t));
    RSIndex rangeIndex = 0;
    RSIndex srcToDstMultiplier = (srcIsUnicode == dstIsUnicode) ? 1 : (sizeof(UniChar) / sizeof(uint8_t));
    
    // Loop over the ranges, copying the range to be preserved (right before each range)
    while (rangeIndex < numRanges)
    {
        RSIndex srcLengthInBytes = ranges[rangeIndex].location * srcBlockSize - srcLocationInBytes;	// srcLengthInBytes is in terms of bytes, not blocks; represents length of region to be preserved
        if (srcLengthInBytes > 0)
        {
            if (srcIsUnicode == dstIsUnicode)
            {
                memmove(dstBuffer + dstLocationInBytes, srcBuffer + srcLocationInBytes, srcLengthInBytes);
            }
            else
            {
                __RSStrConvertBytesToUnicode(srcBuffer + srcLocationInBytes, (UniChar *)(dstBuffer + dstLocationInBytes), srcLengthInBytes);
            }
        }
        srcLocationInBytes += srcLengthInBytes + ranges[rangeIndex].length * srcBlockSize;	// Skip over the just-copied and to-be-deleted stuff
        dstLocationInBytes += srcLengthInBytes * srcToDstMultiplier + insertLengthInBytes;
        rangeIndex++;
    }
    
    // Do last range (the one beyond last range)
    if (srcLocationInBytes < srcLength * srcBlockSize)
    {
        if (srcIsUnicode == dstIsUnicode)
        {
            memmove(dstBuffer + dstLocationInBytes, srcBuffer + srcLocationInBytes, srcLength * srcBlockSize - srcLocationInBytes);
        }
        else
        {
            __RSStrConvertBytesToUnicode(srcBuffer + srcLocationInBytes, (UniChar *)(dstBuffer + dstLocationInBytes), srcLength * srcBlockSize - srcLocationInBytes);
        }
    }
}

static void __RSStringHandleOutOfMemory(RSTypeRef obj)
{
    RSStringRef msg = RSSTR("Out of memory. We suggest restarting the application. If you have an unsaved document, create a backup copy in Finder, then try to save.");
    {
        RSLog(RSSTR("%r"), msg);
    }
}

/* Reallocates the backing store of the string to accomodate the new length. Space is reserved or characters are deleted as indicated by insertLength and the ranges in deleteRanges. The length is updated to reflect the new state. Will also maintain a length byte and a null byte in 8-bit strings. If length cannot fit in length byte, the space will still be reserved, but will be 0. (Hence the reason the length byte should never be looked at as length unless there is no explicit length.)
 */
static void __RSStringChangeSizeMultiple(RSMutableStringRef str, const RSRange *deleteRanges, RSIndex numDeleteRanges, RSIndex insertLength, BOOL makeUnicode)
{
    const uint8_t *curContents = (uint8_t *)__RSStrContents(str);
    RSIndex curLength = curContents ? __RSStrLength2(str, curContents) : 0;
    unsigned long newLength;	// We use unsigned to better keep track of overflow
    
    // Compute new length of the string
    if (numDeleteRanges == 1)
    {
        newLength = curLength + insertLength - deleteRanges[0].length;
    }
    else
    {
        RSIndex cnt;
        newLength = curLength + insertLength * numDeleteRanges;
        for (cnt = 0; cnt < numDeleteRanges; cnt++) newLength -= deleteRanges[cnt].length;
    }
    
    __RSAssertIfFixedLengthIsOK(str, newLength);
    
    if (newLength == 0)
    {
        // An somewhat optimized code-path for this special case, with the following implicit values:
        // newIsUnicode = NO
        // useLengthAndNullBytes = NO
        // newCharSize = sizeof(uint8_t)
        // If the newCapacity happens to be the same as the old, we don't free the buffer; otherwise we just free it totally
        // instead of doing a potentially useless reallocation (as the needed capacity later might turn out to be different anyway)
        RSIndex curCapacity = __RSStrCapacity(str);
        RSIndex newCapacity = __RSStrNewCapacity(str, 0, curCapacity, YES, sizeof(uint8_t));
        if (newCapacity != curCapacity)
        {
            // If we're reallocing anyway (larger or smaller --- larger could happen if desired capacity was changed in the meantime), let's just free it all
            if (curContents) __RSStrDeallocateMutableContents(str, (uint8_t *)curContents);
            __RSStrSetContentPtr(str, nil);
            __RSStrSetCapacity(str, 0);
            __RSStrClearCapacityProvidedExternally(str);
            __RSStrClearHasLengthAndNullBytes(str);
            if (!__RSStrIsExternalMutable(str)) __RSStrClearUnicode(str);	// External mutable implies Unicode
        }
        else
        {
            if (!__RSStrIsExternalMutable(str))
            {
                __RSStrClearUnicode(str);
                if (curCapacity >= (int)(sizeof(uint8_t) * 2))
                {
                    // If there's room
                    __RSStrSetHasLengthAndNullBytes(str);
                    ((uint8_t *)curContents)[0] = ((uint8_t *)curContents)[1] = 0;
                }
                else
                {
                    __RSStrClearHasLengthAndNullBytes(str);
                }
            }
        }
        __RSStrSetExplicitLength(str, 0);
    }
    else
    {
        /* This else-clause assumes newLength > 0 */
        BOOL oldIsUnicode = __RSStrIsUnicode(str);
        BOOL newIsUnicode = makeUnicode || (oldIsUnicode /* && (newLength > 0) - implicit */ ) || __RSStrIsExternalMutable(str);
        RSIndex newCharSize = newIsUnicode ? sizeof(UniChar) : sizeof(uint8_t);
        BOOL useLengthAndNullBytes = !newIsUnicode /* && (newLength > 0) - implicit */;
        RSIndex numExtraBytes = useLengthAndNullBytes ? 2 : 0;	/* 2 extra bytes to keep the length byte & null... */
        RSIndex curCapacity = __RSStrCapacity(str);
        if (newLength > (LONG_MAX - numExtraBytes) / newCharSize) __RSStringHandleOutOfMemory(str);	// Does not return
        RSIndex newCapacity = __RSStrNewCapacity(str, newLength * newCharSize + numExtraBytes, curCapacity, YES, newCharSize);
        
        if (newCapacity == -1) __RSStringHandleOutOfMemory(str);	// Does not return
        BOOL allocNewBuffer = (newCapacity != curCapacity) || (curLength > 0 && !oldIsUnicode && newIsUnicode);	/* We alloc new buffer if oldIsUnicode != newIsUnicode because the contents have to be copied */
        uint8_t *newContents;
        if (allocNewBuffer)
        {
            newContents = (uint8_t *)__RSStrAllocateMutableContents(str, newCapacity);
            if (!newContents)
            {
                // Try allocating without extra room
                newCapacity = __RSStrNewCapacity(str, newLength * newCharSize + numExtraBytes, curCapacity, NO, newCharSize);
                // Since we checked for this above, it shouldn't be the case here, but just in case
                if (newCapacity == -1) __RSStringHandleOutOfMemory(str);    // Does not return
                newContents = (uint8_t *)__RSStrAllocateMutableContents(str, newCapacity);
                if (!newContents) __RSStringHandleOutOfMemory(str);	    // Does not return
            }
        }
        else
        {
            newContents = (uint8_t *)curContents;
        }
        
        BOOL hasLengthAndNullBytes = __RSStrHasLengthByte(str);

        RSAssert1(hasLengthAndNullBytes == __RSStrHasNullByte(str), __RSLogAssertion, "%s(): Invalid state in 8-bit string", __PRETTY_FUNCTION__);
        
        // Calculate pointers to the actual string content (skipping over the length byte, if present).  Note that keeping a reference to the base is needed for newContents under GC, since the copy may take a long time.
        const uint8_t *curContentsBody = hasLengthAndNullBytes ? (curContents+1) : curContents;
        uint8_t *newContentsBody = useLengthAndNullBytes ? (newContents+1) : newContents;
        
        if (curContents)
        {
            if (oldIsUnicode == newIsUnicode)
            {
                if (newContentsBody == curContentsBody)
                {
                    rearrangeBlocks(newContentsBody, curLength, newCharSize, deleteRanges, numDeleteRanges, insertLength);
                }
                else
                {
                    copyBlocks(curContentsBody, newContentsBody, curLength, oldIsUnicode, newIsUnicode, deleteRanges, numDeleteRanges, insertLength);
                }
            }
            else if (newIsUnicode)
            {
                /* this implies we have a new buffer */
                copyBlocks(curContentsBody, newContentsBody, curLength, oldIsUnicode, newIsUnicode, deleteRanges, numDeleteRanges, insertLength);
            }
            if (allocNewBuffer && __RSStrFreeContentsWhenDone(str)) __RSStrDeallocateMutableContents(str, (void *)curContents);
        }
        
        if (!newIsUnicode)
        {
            if (useLengthAndNullBytes && newContentsBody)
            {
                newContentsBody[newLength] = 0;	/* Always have null byte, if not unicode */
                newContents[0] = __RSCanUseLengthByte(newLength) ? (uint8_t)newLength : 0;
                if (!hasLengthAndNullBytes) __RSStrSetHasLengthAndNullBytes(str);
            }
            else
            {
                if (hasLengthAndNullBytes) __RSStrClearHasLengthAndNullBytes(str);
            }
            if (oldIsUnicode) __RSStrClearUnicode(str);
        }
        else
        {
            // New is unicode...
            if (!oldIsUnicode) __RSStrSetUnicode(str);
            if (hasLengthAndNullBytes) __RSStrClearHasLengthAndNullBytes(str);
        }
        __RSStrSetExplicitLength(str, newLength);
        
        if (allocNewBuffer)
        {
            __RSStrSetCapacity(str, newCapacity);
            __RSStrClearCapacityProvidedExternally(str);
            __RSStrSetContentPtr(str, newContents);
        }
    }
}

/* Same as above, but takes one range (very common case)
 */
RSInline void __RSStringChangeSize(RSMutableStringRef str, RSRange range, RSIndex insertLength, BOOL makeUnicode)
{
    __RSStringChangeSizeMultiple(str, &range, 1, insertLength, makeUnicode);
}
#if defined(DEBUG)

// We put this into C & Pascal strings if we can't convert
#define CONVERSIONFAILURESTR "RSString conversion failed"

// We set this to YES when purging the constant string table, so RSStringDeallocate doesn't assert
static BOOL __RSConstantStringTableBeingFreed = NO;

#endif

#if defined(DEBUG)
static BOOL __RSStrIsConstantString(RSStringRef str);
#endif

static void __RSStringDeallocate(RSTypeRef RS)
{
    RSStringRef str = (RSStringRef)RS;
    
    // If in DEBUG mode, check to see if the string a RSSTR, and complain.
//    RSAssert1(__RSConstantStringTableBeingFreed || !__RSStrIsConstantString((RSStringRef)RS), __RSLogAssertion, "Tried to deallocate RSSTR(\"%R\")", str);
//    if (!__RSConstantStringTableBeingFreed)
//    {
//        if (!(__RSRuntimeInstanceIsStackValue(str) || __RSRuntimeInstanceIsSpecial(str)) && __RSStrIsConstantString(str))
//            __RSCLog(RSLogLevelNotice, "constant string deallocate\n");
//    }
    if (!__RSStrIsInline(str))
    {
        uint8_t *contents;
        BOOL isMutable = __RSStrIsMutable(str);
        if (__RSStrFreeContentsWhenDone(str) && (contents = (uint8_t *)__RSStrContents(str)))
        {
            if (isMutable)
            {
                __RSStrDeallocateMutableContents((RSMutableStringRef)str, contents);
            }
            else
            {
                if (__RSStrHasContentsDeallocator(str))
                {
                    RSAllocatorRef allocator = __RSStrContentsDeallocator(str);
                    RSAllocatorDeallocate(allocator, contents);
                    RSRelease(allocator);
                }
                else
                {
                    RSAllocatorRef alloc = __RSGetAllocator(str);
                    RSAllocatorDeallocate(alloc, contents);
                }
            }
        }
        if (isMutable && __RSStrHasContentsAllocator(str))
        {
            RSAllocatorRef allocator = __RSStrContentsAllocator((RSMutableStringRef)str);
//            if (!(RSAllocatorSystemDefaultGCRefZero == allocator || RSAllocatorDefaultGCRefZero == allocator))
            RSRelease(allocator);
        }
    }
}

RSExport const char* RSStringGetCStringPtr(RSStringRef str, RSStringEncoding encoding)
{
    if (encoding != __RSStringGetEightBitStringEncoding() && (RSStringEncodingASCII != __RSStringGetEightBitStringEncoding() || !__RSStringEncodingIsSupersetOfASCII(encoding)))
        return nil;
    // ??? Also check for encoding = SystemEncoding and perhaps bytes are all ASCII?
    
    RS_OBJC_FUNCDISPATCHV(__RSStringTypeID, const char *, (NSString *)str, _fastCStringContents:YES);
    
    __RSAssertIsString(str);
    
    if (__RSStrHasNullByte(str)) {
        // Note: this is called a lot, 27000 times to open a small xcode project with one file open.
        // Of these uses about 1500 are for cStrings/utf8strings.
#if 0
        // Only sometimes when the stars are aligned will this call return a gc pointer
        // under GC we can only really return a pointer to the start of a GC buffer for cString use
        // (Is there a simpler way to ask if contents isGC?)
        RSAllocatorRef alloc = (__RSStrHasContentsAllocator(str)) ? __RSStrContentsAllocator(str) : __RSGetAllocator(str);
        if (RS_IS_COLLECTABLE_ALLOCATOR(alloc)) {
            if (__RSStrSkipAnyLengthByte(str) != 0 || !__RSStrIsMutable(str)) {
                static int counter = 0;
                printf("RSString %dth unsafe safe string %s\n", ++counter, __RSStrContents(str) + __RSStrSkipAnyLengthByte(str));
                return nil;
            }
        }
#endif
        const void *contents = nil;
        if (__RSStrIsInline(str)) {
            contents = (const void *)(((uintptr_t)&(str->_variants)));
            contents += (__RSStrHasExplicitLength(str) ? sizeof(RSIndex) : 0);
        }
        else
            contents = str->_variants._notInlineImmutable1._buffer;
        contents += __RSStrSkipAnyLengthByte(str);
        
        return (const char *)__RSStrContents(str) + __RSStrSkipAnyLengthByte(str);
    }
    else
    {
        return nil;
    }
}

const unsigned char * RSStringGetPascalStringPtr (RSStringRef str, RSStringEncoding encoding)
{
    if (!RS_IS_OBJC(__RSStringTypeID, str))
    {
        /* ??? Hope the compiler optimizes this away if OBJC_MAPPINGS is not on */
        __RSAssertIsString(str);
        if (__RSStrHasLengthByte(str) && __RSStrIsEightBit(str) && ((__RSStringGetEightBitStringEncoding() == encoding) || (__RSStringGetEightBitStringEncoding() == RSStringEncodingASCII && __RSStringEncodingIsSupersetOfASCII(encoding))))
        {
            // Requested encoding is equal to the encoding in string || the contents is in ASCII
            const uint8_t *contents = (const uint8_t *)__RSStrContents(str);
            if (__RSStrHasExplicitLength(str) && (__RSStrLength2(str, contents) != (SInt32)(*contents))) return nil;	// Invalid length byte
            return (ConstStringPtr)contents;
        }
        // ??? Also check for encoding = SystemEncoding and perhaps bytes are all ASCII?
    }
    return nil;
}

/* Guts of RSStringGetCharacters(); called from the two functions below. Don't call it from elsewhere.
 */
RSInline void __RSStringGetCharactersGuts(RSStringRef str, RSRange range, UniChar *buffer, const uint8_t *contents)
{
    if (__RSStrIsEightBit(str))
    {
        __RSStrConvertBytesToUnicode(((uint8_t *)contents) + (range.location + __RSStrSkipAnyLengthByte(str)), buffer, range.length);
    }
    else
    {
        const UniChar *uContents = ((UniChar *)contents) + range.location;
        memmove(buffer, uContents, range.length * sizeof(UniChar));
    }
}

RSExport void RSStringGetCharacters(RSStringRef str, RSRange range, UniChar *buffer)
{
    RS_OBJC_FUNCDISPATCHV(__RSStringTypeID, void, (NSString *)str, getCharacters:(unichar *)buffer range:NSMakeRange(range.location, range.length));
    
    __RSAssertIsString(str);
    __RSAssertRangeIsInStringBounds(str, range.location, range.length);
    __RSStringGetCharactersGuts(str, range, buffer, (const uint8_t *)__RSStrContents(str));
}

// This function determines whether errors which would cause string exceptions should
// be ignored or not

BOOL __RSStringNoteErrors(void) {
    return YES;
}

/* This one is for NSRSString usage; it doesn't do ObjC dispatch; but it does do range check
 */
int _RSStringCheckAndGetCharacters(RSStringRef str, RSRange range, UniChar *buffer)
{
    const uint8_t *contents = (const uint8_t *)__RSStrContents(str);
    if (range.location + range.length > __RSStrLength2(str, contents) && __RSStringNoteErrors()) return _RSStringErrBounds;
    __RSStringGetCharactersGuts(str, range, buffer, contents);
    return _RSStringErrNone;
}

#define __RSStringInlineBufferLength 128
//typedef struct {
//    UniChar buffer[__RSStringInlineBufferLength];
//    RSStringRef theString;
//    const UniChar *directBuffer;
//    RSRange rangeToBuffer;		/* Range in string to buffer */
//    RSIndex bufferedRangeStart;		/* Start of range currently buffered (relative to rangeToBuffer.location) */
//    RSIndex bufferedRangeEnd;		/* bufferedRangeStart + number of chars actually buffered */
//} RSStringInlineBuffer;
//
//#if defined(RSInline)
//RSInline void RSStringInitInlineBuffer(RSStringRef str, RSStringInlineBuffer *buf, RSRange range) {
//    buf->theString = str;
//    buf->rangeToBuffer = range;
//    buf->directBuffer = (UniChar *)RSStringGetCStringPtr(str, RSStringEncodingUnicode);
//    buf->bufferedRangeStart = buf->bufferedRangeEnd = 0;
//}
//
//RSInline UTF8Char RSStringGetCharacterFromInlineBuffer(RSStringInlineBuffer *buf, RSIndex idx) {
//    if (buf->directBuffer) {
//        if (idx < 0 || idx >= buf->rangeToBuffer.length) return 0;
//        return buf->directBuffer[idx + buf->rangeToBuffer.location];
//    }
//    if (idx >= buf->bufferedRangeEnd || idx < buf->bufferedRangeStart) {
//        if (idx < 0 || idx >= buf->rangeToBuffer.length) return 0;
//        if ((buf->bufferedRangeStart = idx - 4) < 0) buf->bufferedRangeStart = 0;
//        buf->bufferedRangeEnd = buf->bufferedRangeStart + __RSStringInlineBufferLength;
//        if (buf->bufferedRangeEnd > buf->rangeToBuffer.length) buf->bufferedRangeEnd = buf->rangeToBuffer.length;
//        RSStringGetCharacters(buf->theString, RSMakeRange(buf->rangeToBuffer.location + buf->bufferedRangeStart, buf->bufferedRangeEnd - buf->bufferedRangeStart), buf->buffer);
//    }
//    return buf->buffer[idx - buf->bufferedRangeStart];
//}
//
//#else
///* If INLINE functions are not available, we do somewhat less powerful macros that work similarly (except be aware that the buf argument is evaluated multiple times).
// */
//#define RSStringInitInlineBuffer(str, buf, range) \
//do {(buf)->theString = str; (buf)->rangeToBuffer = range; (buf)->directBuffer = RSStringGetUTF8CharactersPtr(str);} while (0)
//
//#define RSStringGetCharacterFromInlineBuffer(buf, idx) \
//(((idx) < 0 || (idx) >= (buf)->rangeToBuffer.length) ? 0 : ((buf)->directBuffer ? (buf)->directBuffer[(idx) + (buf)->rangeToBuffer.location] : RSStringGetCharacterAtIndex((buf)->theString, (idx) + (buf)->rangeToBuffer.location)))
//
//#endif /* RSInline */



static BOOL __RSStringEqual(RSTypeRef RS1, RSTypeRef RS2)
{
    RSStringRef str1 = (RSStringRef)RS1;
    RSStringRef str2 = (RSStringRef)RS2;
    const uint8_t *contents1;
    const uint8_t *contents2;
    RSIndex len1;
    
    /* !!! We do not need IsString assertions, as the RSBase runtime assures this */
    /* !!! We do not need == test, as the RSBase runtime assures this */
    
    contents1 = (uint8_t *)__RSStrContents(str1);
    contents2 = (uint8_t *)__RSStrContents(str2);
    len1 = __RSStrLength2(str1, contents1);
    
    if (len1 != __RSStrLength2(str2, contents2)) return NO;
    
    contents1 += __RSStrSkipAnyLengthByte(str1);
    contents2 += __RSStrSkipAnyLengthByte(str2);
    
    if (__RSStrIsEightBit(str1) && __RSStrIsEightBit(str2))
    {
        return memcmp((const char *)contents1, (const char *)contents2, len1) ? NO : YES;
    }
    else if (__RSStrIsEightBit(str1))
    {
        /* One string has Unicode contents */
        RSStringInlineBuffer buf;
        RSIndex buf_idx = 0;
        
        RSStringInitInlineBuffer(str1, &buf, RSMakeRange(0, len1));
        for (buf_idx = 0; buf_idx < len1; buf_idx++)
        {
            if (__RSStringGetCharacterFromInlineBufferQuick(&buf, buf_idx) != ((UniChar *)contents2)[buf_idx]) return NO;
        }
    }
    else if (__RSStrIsEightBit(str2))
    {
        /* One string has Unicode contents */
        RSStringInlineBuffer buf;
        RSIndex buf_idx = 0;
        
        RSStringInitInlineBuffer(str2, &buf, RSMakeRange(0, len1));
        for (buf_idx = 0; buf_idx < len1; buf_idx++) {
            if (__RSStringGetCharacterFromInlineBufferQuick(&buf, buf_idx) != ((UniChar *)contents1)[buf_idx]) return NO;
        }
    }
    else
    {
        /* Both strings have Unicode contents */
        RSIndex idx;
        for (idx = 0; idx < len1; idx++)
        {
            if (((UniChar *)contents1)[idx] != ((UniChar *)contents2)[idx]) return NO;
        }
    }
    return YES;
}

/* String hashing: Should give the same results whatever the encoding; so we hash UniChars.
 If the length is less than or equal to 96, then the hash function is simply the
 following (n is the nth UniChar character, starting from 0):
 
 hash(-1) = length
 hash(n) = hash(n-1) * 257 + unichar(n);
 Hash = hash(length-1) * ((length & 31) + 1)
 
 If the length is greater than 96, then the above algorithm applies to
 characters 0..31, (length/2)-16..(length/2)+15, and length-32..length-1, inclusive;
 thus the first, middle, and last 32 characters.
 
 Note that the loops below are unrolled; and: 257^2 = 66049; 257^3 = 16974593; 257^4 = 4362470401;  67503105 is 257^4 - 256^4
 If hashcode is changed from UInt32 to something else, this last piece needs to be readjusted.
 !!! We haven't updated for LP64 yet
 
 NOTE: The hash algorithm used to be duplicated in RS and Foundation; but now it should only be in the four functions below.
 
 Hash function was changed between Panther and Tiger, and Tiger and Leopard.
 */
#define HashEverythingLimit 96

#define HashNextFourUniChars(accessStart, accessEnd, pointer) \
{result = result * 67503105 + (accessStart 0 accessEnd) * 16974593  + (accessStart 1 accessEnd) * 66049  + (accessStart 2 accessEnd) * 257 + (accessStart 3 accessEnd); pointer += 4;}

#define HashNextUniChar(accessStart, accessEnd, pointer) \
{result = result * 257 + (accessStart 0 accessEnd); pointer++;}


/* In this function, actualLen is the length of the original string; but len is the number of characters in buffer. The buffer is expected to contain the parts of the string relevant to hashing.
 */
RSInline RSHashCode __RSStrHashCharacters(const UniChar *uContents, RSIndex len, RSIndex actualLen)
{
    RSHashCode result = actualLen;
    if (len <= HashEverythingLimit)
    {
        const UniChar *end4 = uContents + (len & ~3);
        const UniChar *end = uContents + len;
        while (uContents < end4) HashNextFourUniChars(uContents[, ], uContents); 	// First count in fours
        while (uContents < end) HashNextUniChar(uContents[, ], uContents);		// Then for the last <4 chars, count in ones...
    }
    else
    {
        const UniChar *contents, *end;
        contents = uContents;
        end = contents + 32;
        while (contents < end) HashNextFourUniChars(contents[, ], contents);
        contents = uContents + (len >> 1) - 16;
        end = contents + 32;
        while (contents < end) HashNextFourUniChars(contents[, ], contents);
        end = uContents + len;
        contents = end - 32;
        while (contents < end) HashNextFourUniChars(contents[, ], contents);
    }
    return result + (result << (actualLen & 31));
}

/* This hashes cString in the eight bit string encoding. It also includes the little debug-time sanity check.
 */
extern BOOL (*__RSCharToUniCharFunc)(UInt32 flags, uint8_t ch, UniChar *unicodeChar);
extern UniChar __RSCharToUniCharTable[256];
RSInline RSHashCode __RSStrHashEightBit(const uint8_t *cContents, RSIndex len)
{
#if defined(DEBUG)
    if (!__RSCharToUniCharFunc)
    {	// A little sanity verification: If this is not set, trying to hash high byte chars would be a bad idea
        RSIndex cnt;
        BOOL err = NO;
        if (len <= HashEverythingLimit)
        {
            for (cnt = 0; cnt < len; cnt++) if (cContents[cnt] >= 128) err = YES;
        }
        else
        {
            for (cnt = 0; cnt < 32; cnt++) if (cContents[cnt] >= 128) err = YES;
            for (cnt = (len >> 1) - 16; cnt < (len >> 1) + 16; cnt++) if (cContents[cnt] >= 128) err = YES;
            for (cnt = (len - 32); cnt < len; cnt++) if (cContents[cnt] >= 128) err = YES;
        }
        if (err)
        {
            // Can't do log here, as it might be too early
            fprintf(stderr, "Warning: RSHash() attempting to hash RSString containing high bytes before properly initialized to do so\n");
        }
    }
#endif
    RSHashCode result = len;
    if (len <= HashEverythingLimit)
    {
        const uint8_t *end4 = cContents + (len & ~3);
        const uint8_t *end = cContents + len;
        while (cContents < end4) HashNextFourUniChars(__RSCharToUniCharTable[cContents[, ]], cContents); 	// First count in fours
        while (cContents < end) HashNextUniChar(__RSCharToUniCharTable[cContents[, ]], cContents);		// Then for the last <4 chars, count in ones...
    }
    else
    {
        const uint8_t *contents, *end;
        contents = cContents;
        end = contents + 32;
        while (contents < end) HashNextFourUniChars(__RSCharToUniCharTable[contents[, ]], contents);
        contents = cContents + (len >> 1) - 16;
        end = contents + 32;
        while (contents < end) HashNextFourUniChars(__RSCharToUniCharTable[contents[, ]], contents);
        end = cContents + len;
        contents = end - 32;
        while (contents < end) HashNextFourUniChars(__RSCharToUniCharTable[contents[, ]], contents);
    }
    return result + (result << (len & 31));
}

RSHashCode RSStringHashISOLatin1CString(const uint8_t *bytes, RSIndex len)
{
    RSHashCode result = len;
    if (len <= HashEverythingLimit)
    {
        const uint8_t *end4 = bytes + (len & ~3);
        const uint8_t *end = bytes + len;
        while (bytes < end4) HashNextFourUniChars(bytes[, ], bytes); 	// First count in fours
        while (bytes < end) HashNextUniChar(bytes[, ], bytes);		// Then for the last <4 chars, count in ones...
    }
    else
    {
        const uint8_t *contents, *end;
        contents = bytes;
        end = contents + 32;
        while (contents < end) HashNextFourUniChars(contents[, ], contents);
        contents = bytes + (len >> 1) - 16;
        end = contents + 32;
        while (contents < end) HashNextFourUniChars(contents[, ], contents);
        end = bytes + len;
        contents = end - 32;
        while (contents < end) HashNextFourUniChars(contents[, ], contents);
    }
    return result + (result << (len & 31));
}

RSHashCode RSStringHashCString(const uint8_t *bytes, RSIndex len)
{
    return __RSStrHashEightBit(bytes, len);
}

RSHashCode RSStringHashCharacters(const UniChar *characters, RSIndex len)
{
    return __RSStrHashCharacters(characters, len, len);
}

/* This is meant to be called from NSString or subclassers only. It is an error for this to be called without the ObjC runtime or an argument which is not an NSString or subclass. It can be called with NSRSString, although that would be inefficient (causing indirection) and won't normally happen anyway, as NSRSString overrides hash.
 */

#define RS_OBJC_FUNCDISPATCHV(typeID, obj, ...) do { } while (0)
#define RS_OBJC_CALLV(obj, ...) (0)

static RSStringRef __RSStringCreateInstanceImmutable(RSAllocatorRef allocator,
                                                      const void *bytes,
                                                      RSIndex numBytes,
                                                      RSStringEncoding encoding,
                                                      BOOL possiblyExternalFormat,
                                                      BOOL tryToReduceUnicode,
                                                      BOOL hasLengthByte,
                                                      BOOL hasNullByte,
                                                      BOOL noCopy,
                                                      RSAllocatorRef contentsDeallocator,
                                                      RSBitU32 reservedFlags);

#define __RSStringCreateImmutableFunnel3(a, b, c, d, e, f, g, h, i, j, k)     __RSStringCreateInstanceImmutable((a), (b), (c), (d), (e), (f), (g), (h), (i),  (j), (k))

RSHashCode RSStringHashNSString(RSStringRef str)
{
    UniChar buffer[HashEverythingLimit];
    RSIndex bufLen;		// Number of characters in the buffer for hashing
    RSIndex len = 0;	// Actual length of the string
    
    len = RS_OBJC_CALLV((NSString *)str, length);
    if (len <= HashEverythingLimit)
    {
        (void)RS_OBJC_CALLV((NSString *)str, getCharacters:buffer range:NSMakeRange(0, len));
        bufLen = len;
    }
    else
    {
        (void)RS_OBJC_CALLV((NSString *)str, getCharacters:buffer range:NSMakeRange(0, 32));
        (void)RS_OBJC_CALLV((NSString *)str, getCharacters:buffer+32 range:NSMakeRange((len >> 1) - 16, 32));
        (void)RS_OBJC_CALLV((NSString *)str, getCharacters:buffer+64 range:NSMakeRange(len - 32, 32));
        bufLen = HashEverythingLimit;
    }
    return __RSStrHashCharacters(buffer, bufLen, len);
}

RSHashCode __RSStringHash(RSTypeRef RS)
{
    /* !!! We do not need an IsString assertion here, as this is called by the RSBase runtime only */
    RSStringRef str = (RSStringRef)RS;
    const uint8_t *contents = (uint8_t *)__RSStrContents(str);
    RSIndex len = __RSStrLength2(str, contents);
    
    if (__RSStrIsEightBit(str))
    {
        contents += __RSStrSkipAnyLengthByte(str);
        return __RSStrHashEightBit(contents, len);
    }
    else
    {
        return __RSStrHashCharacters((const UniChar *)contents, len, len);
    }
}
RSStringRef  _RSStringCreateWithFormatAndArgumentsAux(RSAllocatorRef alloc, RSStringRef (*copyDesRSunc)(void *, const void *), RSIndex, RSStringRef format, va_list arguments);

RSExport RSStringRef RSStringCreateCopy(RSAllocatorRef alloc, RSStringRef str)
{
    //  RS_OBJC_FUNCDISPATCHV(__RSStringTypeID, RSStringRef, (NSString *)str, copy);
    
    __RSAssertIsString(str);
    if (!__RSStrIsMutable((RSStringRef)str) && 								// If the string is not mutable
        ((alloc ? alloc : RSAllocatorGetDefault()) == __RSGetAllocator(str)) &&		//  and it has the same allocator as the one we're using
        (__RSStrIsInline((RSStringRef)str) || __RSStrFreeContentsWhenDone((RSStringRef)str) || __RSStrIsConstant((RSStringRef)str)))
    {	//  and the characters are inline, or are owned by the string, or the string is constant
        RSRetain(str);			// Then just retain instead of making a YES copy
        return str;
    }
    
    if (__RSStrIsEightBit((RSStringRef)str))
    {
        const uint8_t *contents = (const uint8_t *)__RSStrContents((RSStringRef)str);
        return __RSStringCreateImmutableFunnel3(alloc, contents + __RSStrSkipAnyLengthByte((RSStringRef)str), __RSStrLength2((RSStringRef)str, contents), __RSStringGetEightBitStringEncoding(), NO, NO, NO, NO, NO, ALLOCATORSFREEFUNC, 0);
    }
    else
    {
        const UniChar *contents = (const UniChar *)(__RSStrContents((RSStringRef)str));
//        return __RSStringCreateImmutableFunnel3(alloc, (((uint8_t*)contents) + __RSStrSkipAnyLengthByte(str)) , __RSStrLength2((RSStringRef)str, contents) * sizeof(UniChar), RSStringEncodingUnicode, NO, YES, NO, NO, NO, ALLOCATORSFREEFUNC, 0);
        return __RSStringCreateImmutableFunnel3(alloc, contents , __RSStrLength2((RSStringRef)str, contents) * sizeof(UniChar), RSStringEncodingUnicode, NO, YES, __RSStrHasLengthByte(str), NO, NO, ALLOCATORSFREEFUNC, 0);
    }
}

static RSTypeRef __RSStringCopy(RSAllocatorRef allocator, RSTypeRef rs, BOOL mutableCopy)
{
    if (mutableCopy) return RSStringCreateMutableCopy(allocator, 0, rs);
    return RSStringCreateCopy(allocator, rs);
}


enum {
	RSStringFormatZeroFlag = (1 << 0),         // if not, padding is space char
	RSStringFormatMinusFlag = (1 << 1),        // if not, no flag implied
	RSStringFormatPlusFlag = (1 << 2),         // if not, no flag implied, overrides space
	RSStringFormatSpaceFlag = (1 << 3),        // if not, no flag implied
	RSStringFormatExternalSpecFlag = (1 << 4), // using config dict
    RSStringFormatLocalizable = (1 << 5)       // explicitly mark the specs we can localize
};


typedef struct {
    int16_t size;
    int16_t type;
    RSIndex loc;
    RSIndex len;
    RSIndex widthArg;
    RSIndex precArg;
    uint32_t flags;
    int8_t mainArgNum;
    int8_t precArgNum;
    int8_t widthArgNum;
    int8_t configDictIndex;
    int8_t numericFormatStyle;        // Only set for localizable numeric quantities
} RSFormatSpec;

#define BUFFER_LEN 512

RSInline void __RSParseFormatSpec(const UniChar *uformat, const uint8_t *cformat, RSIndex *fmtIdx, RSIndex fmtLen, RSFormatSpec *spec, RSStringRef *configKeyPointer)
{
    BOOL seenDot = NO;
    BOOL seenSharp = NO;
    RSIndex keyIndex = RSNotFound;
    
    for (;;)
    {
        UniChar ch;
        if (fmtLen <= *fmtIdx) return;	/* no type */
        if (cformat) ch = (UniChar)cformat[(*fmtIdx)++]; else ch = uformat[(*fmtIdx)++];
        
        if (keyIndex >= 0)
        {
            if (ch == 'R' || ch == 'r') // ch == '@'
            {
                // found the key
                RSIndex length = (*fmtIdx) - 1 - keyIndex;
                
                spec->flags |= RSStringFormatExternalSpecFlag;
                spec->type = RSFormatRSType;
                spec->size = RSFormatSizePointer;  // 4 or 8 depending on LP64
                
                if ((nil != configKeyPointer) && (length > 0))
                {
                    if (cformat)
                    {
                        *configKeyPointer = RSStringCreateWithBytes(nil, cformat + keyIndex, length, __RSStringGetEightBitStringEncoding(), FALSE);
                    }
                    else
                    {
                        *configKeyPointer = RSStringCreateWithCharactersNoCopy(nil, uformat + keyIndex, length, RSAllocatorNull);
                    }
                }
                return;
            }
            if ((ch < '0') || ((ch > '9') && (ch < 'A')) || ((ch > 'Z') && (ch < 'a') && (ch != '_')) || (ch > 'z'))
            {
//                if (ch == 'R' || ch == 'r') // ch == '@'
//                {
//                    // found the key
//                    RSIndex length = (*fmtIdx) - 1 - keyIndex;
//                    
//                    spec->flags |= RSStringFormatExternalSpecFlag;
//                    spec->type = RSFormatRSType;
//                    spec->size = RSFormatSizePointer;  // 4 or 8 depending on LP64
//                    
//                    if ((nil != configKeyPointer) && (length > 0))
//                    {
//                        if (cformat)
//                        {
//                            *configKeyPointer = RSStringCreateWithBytes(nil, cformat + keyIndex, length, __RSStringGetEightBitStringEncoding(), FALSE);
//                        }
//                        else
//                        {
//                            *configKeyPointer = RSStringCreateWithCharactersNoCopy(nil, uformat + keyIndex, length, RSAllocatorNull);
//                        }
//                    }
//                    return;
//                }
                keyIndex = RSNotFound;
            }
            continue;
        }
        
    reswtch:
        switch (ch)
        {
            case '#':	// ignored for now
                seenSharp = YES;
                break;
            case 0x20:
                if (!(spec->flags & RSStringFormatPlusFlag)) spec->flags |= RSStringFormatSpaceFlag;
                break;
            case '-':
                spec->flags |= RSStringFormatMinusFlag;
                spec->flags &= ~RSStringFormatZeroFlag;	// remove zero flag
                break;
            case '+':
                spec->flags |= RSStringFormatPlusFlag;
                spec->flags &= ~RSStringFormatSpaceFlag;	// remove space flag
                break;
            case '0':
                if (seenDot) {    // after we see '.' and then we see '0', it is 0 precision. We should not see '.' after '0' if '0' is the zero padding flag
                    spec->precArg = 0;
                    break;
                }
                if (!(spec->flags & RSStringFormatMinusFlag)) spec->flags |= RSStringFormatZeroFlag;
                break;
            case 'h':
                if (*fmtIdx < fmtLen) {
                    // fetch next character, don't increment fmtIdx
                    if (cformat) ch = (UniChar)cformat[(*fmtIdx)]; else ch = uformat[(*fmtIdx)];
                    if ('h' == ch) {	// 'hh' for char, like 'c'
                        (*fmtIdx)++;
                        spec->size = RSFormatSize1;
                        break;
                    }
                }
                spec->size = RSFormatSize2;
                break;
            case 'l':
                if (*fmtIdx < fmtLen) {
                    // fetch next character, don't increment fmtIdx
                    if (cformat) ch = (UniChar)cformat[(*fmtIdx)]; else ch = uformat[(*fmtIdx)];
                    if ('l' == ch) {	// 'll' for long long, like 'q'
                        (*fmtIdx)++;
                        spec->size = RSFormatSize8;
                        break;
                    }
                }
                spec->size = RSFormatSizeLong;  // 4 or 8 depending on LP64
                break;
#if LONG_DOUBLE_SUPPORT
            case 'L':
                spec->size = RSFormatSize16;
                break;
#endif
            case 'q':
                spec->size = RSFormatSize8;
                break;
            case 't': case 'z':
                spec->size = RSFormatSizeLong;  // 4 or 8 depending on LP64
                break;
            case 'j':
                spec->size = RSFormatSize8;
                break;
            case 'c':
                spec->type = RSFormatLongType;
                spec->size = RSFormatSize1;
                return;
            case 'D': case 'd': case 'i': case 'U': case 'u':
                // we can localize all but octal or hex
//                if (_RSExecutableLinkedOnOrAfter(RSSystemVersionMountainLion))
                spec->flags |= RSStringFormatLocalizable;
                spec->numericFormatStyle = RSFormatStyleDecimal;
                if (ch == 'u' || ch == 'U') spec->numericFormatStyle = RSFormatStyleUnsigned;
                // fall thru
            case 'O': case 'o': case 'x': case 'X':
                spec->type = RSFormatLongType;
                // Seems like if spec->size == 0, we should spec->size = RSFormatSize4. However, 0 is handled correctly.
                return;
            case 'f': case 'F': case 'g': case 'G': case 'e': case 'E': {
                // we can localize all but hex float output
//                if (_RSExecutableLinkedOnOrAfter(RSSystemVersionMountainLion))
                spec->flags |= RSStringFormatLocalizable;
                char lch = (ch >= 'A' && ch <= 'Z') ? (ch - 'A' + 'a') : ch;
                spec->numericFormatStyle = ((lch == 'e' || lch == 'g') ? RSFormatStyleScientific : 0) | ((lch == 'f' || lch == 'g') ? RSFormatStyleDecimal : 0);
                if (seenDot && spec->precArg == -1 && spec->precArgNum == -1) { // for the cases that we have '.' but no precision followed, not even '*'
                    spec->precArg = 0;
                }
            }
                // fall thru
            case 'a': case 'A':
                spec->type = RSFormatDoubleType;
                if (spec->size != RSFormatSize16) spec->size = RSFormatSize8;
                return;
            case 'n':		/* %n is not handled correctly; for Leopard or newer apps, we disable it further */
                spec->type = 1 ? RSFormatDummyPointerType : RSFormatPointerType;
                spec->size = RSFormatSizePointer;  // 4 or 8 depending on LP64
                return;
            case 'p':
                spec->type = RSFormatPointerType;
                spec->size = RSFormatSizePointer;  // 4 or 8 depending on LP64
                return;
            case 's':
                spec->type = RSFormatCharsType;
                spec->size = RSFormatSizePointer;  // 4 or 8 depending on LP64
                return;
            case 'S':
                spec->type = RSFormatUnicharsType;
                spec->size = RSFormatSizePointer;  // 4 or 8 depending on LP64
                return;
            case 'C':
                spec->type = RSFormatSingleUnicharType;
                spec->size = RSFormatSize2;
                return;
            case 'P':
                spec->type = RSFormatPascalCharsType;
                spec->size = RSFormatSizePointer;  // 4 or 8 depending on LP64
                return;
//            case '@':
            case 'R':
            case 'r':
                if (seenSharp) {
                    seenSharp = NO;
                    keyIndex = *fmtIdx;
                    break;
                } else {
                    spec->type = RSFormatRSType;
                    spec->size = RSFormatSizePointer;  // 4 or 8 depending on LP64
                    return;
                }
            case '1': case '2': case '3': case '4': case '5': case '6': case '7': case '8': case '9': {
                int64_t number = 0;
                do {
                    number = 10 * number + (ch - '0');
                    if (cformat) ch = (UniChar)cformat[(*fmtIdx)++]; else ch = uformat[(*fmtIdx)++];
                } while ((UInt32)(ch - '0') <= 9);
                if ('$' == ch) {
                    if (-2 == spec->precArgNum) {
                        spec->precArgNum = (int8_t)number - 1;	// Arg numbers start from 1
                    } else if (-2 == spec->widthArgNum) {
                        spec->widthArgNum = (int8_t)number - 1;	// Arg numbers start from 1
                    } else {
                        spec->mainArgNum = (int8_t)number - 1;	// Arg numbers start from 1
                    }
                    break;
                } else if (seenDot) {	/* else it's either precision or width */
                    spec->precArg = (SInt32)number;
                } else {
                    spec->widthArg = (SInt32)number;
                }
                goto reswtch;
            }
            case '*':
                spec->widthArgNum = -2;
                break;
            case '.':
                seenDot = YES;
                if (cformat) ch = (UniChar)cformat[(*fmtIdx)++]; else ch = uformat[(*fmtIdx)++];
                if ('*' == ch) {
                    spec->precArgNum = -2;
                    break;
                }
                goto reswtch;
            default:
                spec->type = RSFormatLiteralType;
                return;
        }
    }
}

void __RSStringAppendBytes(RSMutableStringRef str, const char *cStr, RSIndex appendedLength, RSStringEncoding encoding)
{
    BOOL appendedIsUnicode = NO;
    BOOL freeCStrWhenDone = NO;
    BOOL demoteAppendedUnicode = NO;
    RSVarWidthCharBuffer vBuf;
    
    __RSAssertIsNotNegative(appendedLength);
    
    if (encoding == RSStringEncodingASCII || encoding == __RSStringGetEightBitStringEncoding()) {
        // appendedLength now denotes length in UniChars
    } else if (encoding == RSStringEncodingUnicode) {
        UniChar *chars = (UniChar *)cStr;
        RSIndex idx, length = appendedLength / sizeof(UniChar);
        bool isASCII = YES;
        for (idx = 0; isASCII && idx < length; idx++) isASCII = (chars[idx] < 0x80);
        if (!isASCII) {
            appendedIsUnicode = YES;
        } else {
            demoteAppendedUnicode = YES;
        }
        appendedLength = length;
    } else {
        BOOL usingPassedInMemory = NO;
        
        vBuf.allocator = RSAllocatorGetDefault();	// We don't want to use client's allocator for temp stuff
        vBuf.chars.unicode = nil;	// This will cause the decode function to allocate memory if necessary
        
        if (!__RSStringDecodeByteStream3((const uint8_t *)cStr, appendedLength, encoding, __RSStrIsUnicode(str), &vBuf, &usingPassedInMemory, 0)) {
            RSAssert1(0, __RSLogAssertion, "Supplied bytes could not be converted specified encoding %d", encoding);
            return;
        }
        
        // If not ASCII, appendedLength now denotes length in UniChars
        appendedLength = vBuf.numChars;
        appendedIsUnicode = !vBuf.isASCII;
        cStr = (const char *)vBuf.chars.ascii;
        freeCStrWhenDone = !usingPassedInMemory && vBuf.shouldFreeChars;
    }
    
    if (RS_IS_OBJC(__RSStringTypeID, str)) {
        if (!appendedIsUnicode && !demoteAppendedUnicode) {
            RS_OBJC_FUNCDISPATCHV(__RSStringTypeID, void, (NSMutableString *)str, _cfAppendCString:(const unsigned char *)cStr length:(NSInteger)appendedLength);
        } else {
            RS_OBJC_FUNCDISPATCHV(__RSStringTypeID, void, (NSMutableString *)str, appendCharacters:(const unichar *)cStr length:(NSUInteger)appendedLength);
        }
    } else {
        RSIndex strLength;
        __RSAssertIsStringAndMutable(str);
        strLength = __RSStrLength(str);
        
        __RSStringChangeSize(str, RSMakeRange(strLength, 0), appendedLength, appendedIsUnicode || __RSStrIsUnicode(str));
        
        if (__RSStrIsUnicode(str)) {
            UniChar *contents = (UniChar *)__RSStrContents(str);
            if (appendedIsUnicode) {
                memmove(contents + strLength, cStr, appendedLength * sizeof(UniChar));
            } else {
                __RSStrConvertBytesToUnicode((const uint8_t *)cStr, contents + strLength, appendedLength);
            }
        } else {
            if (demoteAppendedUnicode) {
                UniChar *chars = (UniChar *)cStr;
                RSIndex idx;
                uint8_t *contents = (uint8_t *)__RSStrContents(str) + strLength + __RSStrSkipAnyLengthByte(str);
                for (idx = 0; idx < appendedLength; idx++) contents[idx] = (uint8_t)chars[idx];
            } else {
                uint8_t *contents = (uint8_t *)__RSStrContents(str);
                memmove(contents + strLength + __RSStrSkipAnyLengthByte(str), cStr, appendedLength);
            }
        }
    }
    
    if (freeCStrWhenDone) RSAllocatorDeallocate(RSAllocatorGetDefault(), (void *)cStr);
}

void RSStringAppendPascalString(RSMutableStringRef str, const unsigned char* pStr, RSStringEncoding encoding) {
    __RSStringAppendBytes(str, (const char *)(pStr + 1), (RSIndex)*pStr, encoding);
}

void RSStringAppendCString(RSMutableStringRef str, const char *cStr, RSStringEncoding encoding) {
    __RSStringAppendBytes(str, cStr, strlen(cStr), encoding);
}

RSInline void __RSStringReplace(RSMutableStringRef str, RSRange range, RSStringRef replacement);

RSExport void RSStringAppendString(RSMutableStringRef str, RSStringRef appended) {
    RS_OBJC_FUNCDISPATCHV(__RSStringTypeID, void, (NSMutableString *)str, appendString:(NSString *)appended);
    __RSAssertIsStringAndMutable(str);
    __RSStringReplace(str, RSMakeRange(__RSStrLength(str), 0), appended);
}

RSExport RSMutableStringRef RSStringReplace(RSMutableStringRef str, RSRange range, RSStringRef replacement) {
    RS_OBJC_FUNCDISPATCHV(__RSStringTypeID, void, (NSMutableString *)str, replaceCharactersInRange:NSMakeRange(range.location, range.length) withString:(NSString *)replacement);
    __RSAssertIsStringAndMutable(str);
    __RSAssertRangeIsInStringBounds(str, range.location, range.length);
    __RSStringReplace(str, range, replacement);
    return str;
}


RSExport RSMutableStringRef RSStringReplaceAll(RSMutableStringRef str, RSStringRef find, RSStringRef replacement) {
    RS_OBJC_FUNCDISPATCHV(__RSStringTypeID, void, (NSMutableString *)str, setString:(NSString *)replacement);
    __RSAssertIsStringAndMutable(str);

    RSIndex numberOfResult = 0;
    RSRange *rgs = RSStringFindAll(str, find, &numberOfResult);
    if (rgs)
    {
        RSIndex pos = RSStringGetLength(replacement) - RSStringGetLength(find);
        for (RSIndex idx = 0; idx < numberOfResult; ++idx) {
            RSRange range = rgs[idx];
            range.location += pos * idx;
            __RSStringReplace(str, range, replacement);
        }
        RSAllocatorDeallocate(RSAllocatorDefault, rgs);
    }
    return str;
}

static void __RSStringAppendFormatCore(RSMutableStringRef outputString,
                                       RSStringRef (*copyDescfunc)(void *, const void *),
                                       RSDictionaryRef formatOptions,
                                       RSStringRef formatString,
                                       RSIndex initialArgPosition,
                                       const void *origValues,
                                       RSIndex originalValuesSize,
                                       va_list args);

RSExport void RSStringAppendFormatAndArguments(RSMutableStringRef str, RSStringRef format, va_list args)
{
    __RSStringAppendFormatCore(str, nil, nil, format, 0, nil, 0, args);
}

RSExport void RSStringAppendStringWithFormat(RSMutableStringRef str, RSStringRef format, ...)
{
    va_list argList;
    
    va_start(argList, format);
    RSStringAppendFormatAndArguments(str, format, argList);
    va_end(argList);

}

RSExport void RSStringAppendCharacters(RSMutableStringRef str, const UniChar *chars, RSIndex appendedLength) {
    RSIndex strLength, idx;
    
    __RSAssertIsNotNegative(appendedLength);
    
    RS_OBJC_FUNCDISPATCHV(__RSStringTypeID, void, (NSMutableString *)str, appendCharacters:chars length:(NSUInteger)appendedLength);
    
    __RSAssertIsStringAndMutable(str);
    
    strLength = __RSStrLength(str);
    if (__RSStrIsUnicode(str)) {
        __RSStringChangeSize(str, RSMakeRange(strLength, 0), appendedLength, YES);
        memmove((UniChar *)__RSStrContents(str) + strLength, chars, appendedLength * sizeof(UniChar));
    } else {
        uint8_t *contents;
        bool isASCII = YES;
        for (idx = 0; isASCII && idx < appendedLength; idx++) isASCII = (chars[idx] < 0x80);
        __RSStringChangeSize(str, RSMakeRange(strLength, 0), appendedLength, !isASCII);
        if (!isASCII) {
            memmove((UniChar *)__RSStrContents(str) + strLength, chars, appendedLength * sizeof(UniChar));
        } else {
            contents = (uint8_t *)__RSStrContents(str) + strLength + __RSStrSkipAnyLengthByte(str);
            for (idx = 0; idx < appendedLength; idx++) contents[idx] = (uint8_t)chars[idx];
        }
    }
}

void __RSStringAppendBytes(RSMutableStringRef str, const char *cStr, RSIndex appendedLength, RSStringEncoding encoding);

#if DEPLOYMENT_TARGET_MACOSX || DEPLOYMENT_TARGET_EMBEDDED || DEPLOYMENT_TARGET_EMBEDDED_MINI
#define SNPRINTF(TYPE, WHAT) {				\
TYPE value = (TYPE) WHAT;				\
if (-1 != specs[curSpec].widthArgNum) {		\
if (-1 != specs[curSpec].precArgNum) {		\
snprintf_l(buffer, BUFFER_LEN-1, nil, formatBuffer, width, precision, value); \
} else {					\
snprintf_l(buffer, BUFFER_LEN-1, nil, formatBuffer, width, value); \
}						\
} else {						\
if (-1 != specs[curSpec].precArgNum) {		\
snprintf_l(buffer, BUFFER_LEN-1, nil, formatBuffer, precision, value); \
} else {					\
snprintf_l(buffer, BUFFER_LEN-1, nil, formatBuffer, value);	\
}						\
}}
#else
#define SNPRINTF(TYPE, WHAT) {				\
TYPE value = (TYPE) WHAT;				\
if (-1 != specs[curSpec].widthArgNum) {		\
if (-1 != specs[curSpec].precArgNum) {		\
sprintf(buffer, formatBuffer, width, precision, value); \
} else {					\
sprintf(buffer, formatBuffer, width, value); \
}						\
} else {						\
if (-1 != specs[curSpec].precArgNum) {		\
sprintf(buffer, formatBuffer, precision, value); \
} else {					\
sprintf(buffer, formatBuffer, value);	\
}						\
}}
#endif

static void __RSStringAppendFormatCore(RSMutableStringRef outputString,
                                       RSStringRef (*copyDescfunc)(void *, const void *),
                                       RSDictionaryRef formatOptions,
                                       RSStringRef formatString,
                                       RSIndex initialArgPosition,
                                       const void *origValues,
                                       RSIndex originalValuesSize,
                                       va_list args)
{
    RSIndex numSpecs, sizeSpecs, sizeArgNum, formatIdx, curSpec, argNum;
    RSIndex formatLen;
#define FORMAT_BUFFER_LEN 400
    const uint8_t *cformat = nil;
    const UniChar *uformat = nil;
    UniChar *formatChars = nil;
    UniChar localFormatBuffer[FORMAT_BUFFER_LEN];
    
#define VPRINTF_BUFFER_LEN 61
    RSFormatSpec localSpecsBuffer[VPRINTF_BUFFER_LEN];
    RSFormatSpec *specs;
    RSPrintValue localValuesBuffer[VPRINTF_BUFFER_LEN];
    RSPrintValue *values;
    const RSPrintValue *originalValues = (const RSPrintValue *)origValues;
    RSDictionaryRef localConfigs[VPRINTF_BUFFER_LEN];
    RSDictionaryRef *configs;
    RSIndex numConfigs;
    RSAllocatorRef tmpAlloc = nil;
    intmax_t dummyLocation;	    // A place for %n to do its thing in; should be the widest possible int value
    
    numSpecs = 0;
    sizeSpecs = 0;
    sizeArgNum = 0;
    numConfigs = 0;
    specs = nil;
    values = nil;
    configs = nil;
    
    
    formatLen = RSStringGetLength(formatString);
    if (!RS_IS_OBJC(__RSStringTypeID, formatString)) {
        __RSAssertIsString(formatString);
        if (!__RSStrIsUnicode(formatString)) {
            cformat = (const uint8_t *)__RSStrContents(formatString);
            if (cformat) cformat += __RSStrSkipAnyLengthByte(formatString);
        } else {
            uformat = (const UniChar *)__RSStrContents(formatString);
        }
    }
    if (!cformat && !uformat) {
        formatChars = (formatLen > FORMAT_BUFFER_LEN) ? (UniChar *)RSAllocatorAllocate(tmpAlloc = RSAllocatorGetDefault(), formatLen * sizeof(UniChar)) : localFormatBuffer;
//        if (formatChars != localFormatBuffer && __RSOASafe) __RSSetLastAllocationEventName(formatChars, "RSString (temp)");
        RSStringGetCharacters(formatString, RSMakeRange(0, formatLen), formatChars);
        uformat = formatChars;
    }
    
    /* Compute an upper bound for the number of format specifications */
    if (cformat) {
        for (formatIdx = 0; formatIdx < formatLen; formatIdx++) if ('%' == cformat[formatIdx]) sizeSpecs++;
    } else {
        for (formatIdx = 0; formatIdx < formatLen; formatIdx++) if ('%' == uformat[formatIdx]) sizeSpecs++;
    }
    tmpAlloc = RSAllocatorGetDefault();
    specs = ((2 * sizeSpecs + 1) > VPRINTF_BUFFER_LEN) ? (RSFormatSpec *)RSAllocatorAllocate(tmpAlloc, (2 * sizeSpecs + 1) * sizeof(RSFormatSpec)) : localSpecsBuffer;
//    if (specs != localSpecsBuffer && __RSOASafe) __RSSetLastAllocationEventName(specs, "RSString (temp)");
    
    configs = ((sizeSpecs < VPRINTF_BUFFER_LEN) ? localConfigs : (RSDictionaryRef *)RSAllocatorAllocate(tmpAlloc, sizeof(RSStringRef) * sizeSpecs));
    
    /* Collect format specification information from the format string */
    for (curSpec = 0, formatIdx = 0; formatIdx < formatLen; curSpec++) {
        RSIndex newFmtIdx;
        specs[curSpec].loc = formatIdx;
        specs[curSpec].len = 0;
        specs[curSpec].size = 0;
        specs[curSpec].type = 0;
        specs[curSpec].flags = 0;
        specs[curSpec].widthArg = -1;
        specs[curSpec].precArg = -1;
        specs[curSpec].mainArgNum = -1;
        specs[curSpec].precArgNum = -1;
        specs[curSpec].widthArgNum = -1;
        specs[curSpec].configDictIndex = -1;
        if (cformat) {
            for (newFmtIdx = formatIdx; newFmtIdx < formatLen && '%' != cformat[newFmtIdx]; newFmtIdx++);
        } else {
            for (newFmtIdx = formatIdx; newFmtIdx < formatLen && '%' != uformat[newFmtIdx]; newFmtIdx++);
        }
        if (newFmtIdx != formatIdx) {	/* Literal chunk */
            specs[curSpec].type = RSFormatLiteralType;
            specs[curSpec].len = newFmtIdx - formatIdx;
        } else {
            RSStringRef configKey = nil;
            newFmtIdx++;	/* Skip % */
            __RSParseFormatSpec(uformat, cformat, &newFmtIdx, (RSBitU32)formatLen, &(specs[curSpec]), &configKey);
            if (RSFormatLiteralType == specs[curSpec].type) {
                specs[curSpec].loc = formatIdx + 1;
                specs[curSpec].len = 1;
            } else {
                specs[curSpec].len = newFmtIdx - formatIdx;
            }
        }
        formatIdx = newFmtIdx;
        
        // fprintf(stderr, "specs[%d] = {\n  size = %d,\n  type = %d,\n  loc = %d,\n  len = %d,\n  mainArgNum = %d,\n  precArgNum = %d,\n  widthArgNum = %d\n}\n", curSpec, specs[curSpec].size, specs[curSpec].type, specs[curSpec].loc, specs[curSpec].len, specs[curSpec].mainArgNum, specs[curSpec].precArgNum, specs[curSpec].widthArgNum);
        
    }
    numSpecs = curSpec;
    
    // Max of three args per spec, reasoning thus: 1 width, 1 prec, 1 value
    sizeArgNum = ((nil == originalValues) ? (3 * sizeSpecs + 1) : originalValuesSize);
    
    values = (sizeArgNum > VPRINTF_BUFFER_LEN) ? (RSPrintValue *)RSAllocatorAllocate(tmpAlloc, sizeArgNum * sizeof(RSPrintValue)) : localValuesBuffer;
//    if (values != localValuesBuffer && __RSOASafe) __RSSetLastAllocationEventName(values, "RSString (temp)");
    memset(values, 0, sizeArgNum * sizeof(RSPrintValue));
    
#if DEPLOYMENT_TARGET_MACOSX || DEPLOYMENT_TARGET_EMBEDDED || DEPLOYMENT_TARGET_EMBEDDED_MINI || DEPLOYMENT_TARGET_LINUX || DEPLOYMENT_TARGET_FREEBSD
    // va_copy is a C99 extension. No support on Windows
    va_list copiedArgs;
    if (numConfigs > 0) va_copy(copiedArgs, args); // we need to preserve the original state for passing down
#endif
    
    /* Compute values array */
    argNum = initialArgPosition;
    for (curSpec = 0; curSpec < numSpecs; curSpec++) {
        RSIndex newMaxArgNum;
        if (0 == specs[curSpec].type) continue;
        if (RSFormatLiteralType == specs[curSpec].type) continue;
        newMaxArgNum = sizeArgNum;
        if (newMaxArgNum < specs[curSpec].mainArgNum) {
            newMaxArgNum = specs[curSpec].mainArgNum;
        }
        if (newMaxArgNum < specs[curSpec].precArgNum) {
            newMaxArgNum = specs[curSpec].precArgNum;
        }
        if (newMaxArgNum < specs[curSpec].widthArgNum) {
            newMaxArgNum = specs[curSpec].widthArgNum;
        }
        if (sizeArgNum < newMaxArgNum) {
            if (specs != localSpecsBuffer) RSAllocatorDeallocate(tmpAlloc, specs);
            if (values != localValuesBuffer) RSAllocatorDeallocate(tmpAlloc, values);
            if (formatChars && (formatChars != localFormatBuffer)) RSAllocatorDeallocate(tmpAlloc, formatChars);
            return;  // more args than we expected!
        }
        /* It is actually incorrect to reorder some specs and not all; we just do some random garbage here */
        if (-2 == specs[curSpec].widthArgNum) {
            specs[curSpec].widthArgNum = argNum++;
        }
        if (-2 == specs[curSpec].precArgNum) {
            specs[curSpec].precArgNum = argNum++;
        }
        if (-1 == specs[curSpec].mainArgNum) {
            specs[curSpec].mainArgNum = argNum++;
        }
        
        values[specs[curSpec].mainArgNum].size = specs[curSpec].size;
        values[specs[curSpec].mainArgNum].type = specs[curSpec].type;
        
        
        if (-1 != specs[curSpec].widthArgNum) {
            values[specs[curSpec].widthArgNum].size = 0;
            values[specs[curSpec].widthArgNum].type = RSFormatLongType;
        }
        if (-1 != specs[curSpec].precArgNum) {
            values[specs[curSpec].precArgNum].size = 0;
            values[specs[curSpec].precArgNum].type = RSFormatLongType;
        }
    }
    
    /* Collect the arguments in correct type from vararg list */
    for (argNum = 0; argNum < sizeArgNum; argNum++) {
        if ((nil != originalValues) && (0 == values[argNum].type)) values[argNum] = originalValues[argNum];
        switch (values[argNum].type) {
            case 0:
            case RSFormatLiteralType:
                break;
            case RSFormatLongType:
            case RSFormatSingleUnicharType:
                if (RSFormatSize1 == values[argNum].size) {
                    values[argNum].value.int64Value = (int64_t)(int8_t)va_arg(args, int);
                } else if (RSFormatSize2 == values[argNum].size) {
                    values[argNum].value.int64Value = (int64_t)(int16_t)va_arg(args, int);
                } else if (RSFormatSize4 == values[argNum].size) {
                    values[argNum].value.int64Value = (int64_t)va_arg(args, int32_t);
                } else if (RSFormatSize8 == values[argNum].size) {
                    values[argNum].value.int64Value = (int64_t)va_arg(args, int64_t);
                } else {
                    values[argNum].value.int64Value = (int64_t)va_arg(args, int);
                }
                break;
            case RSFormatDoubleType:
#if LONG_DOUBLE_SUPPORT
                if (RSFormatSize16 == values[argNum].size) {
                    values[argNum].value.longDoubleValue = va_arg(args, long double);
                } else
#endif
                {
                    values[argNum].value.doubleValue = va_arg(args, double);
                }
                break;
            case RSFormatPointerType:
            case RSFormatObjectType:
            case RSFormatRSType:
            case RSFormatUnicharsType:
            case RSFormatCharsType:
            case RSFormatPascalCharsType:
                values[argNum].value.pointerValue = va_arg(args, void *);
                break;
            case RSFormatDummyPointerType:
                (void)va_arg(args, void *);	    // Skip the provided argument
                values[argNum].value.pointerValue = &dummyLocation;
                break;
        }
    }
    va_end(args);
    
    /* Format the pieces together */
    
    if (nil == originalValues) {
        originalValues = values;
        originalValuesSize = sizeArgNum;
    }
    
    for (curSpec = 0; curSpec < numSpecs; curSpec++) {
        RSIndex width = 0, precision = 0;
        UniChar *up, ch;
        BOOL hasWidth = NO, hasPrecision = NO;
        
        // widthArgNum and widthArg are never set at the same time; same for precArg*
        if (-1 != specs[curSpec].widthArgNum) {
            width = (SInt32)values[specs[curSpec].widthArgNum].value.int64Value;
            hasWidth = YES;
        }
        if (-1 != specs[curSpec].precArgNum) {
            precision = (SInt32)values[specs[curSpec].precArgNum].value.int64Value;
            hasPrecision = YES;
        }
        if (-1 != specs[curSpec].widthArg) {
            width = specs[curSpec].widthArg;
            hasWidth = YES;
        }
        if (-1 != specs[curSpec].precArg) {
            precision = specs[curSpec].precArg;
            hasPrecision = YES;
        }
        
        switch (specs[curSpec].type) {
            case RSFormatLongType:
            case RSFormatDoubleType:
#if DEPLOYMENT_TARGET_MACOSX || DEPLOYMENT_TARGET_EMBEDDED || DEPLOYMENT_TARGET_WINDOWS
//                if (formatOptions && (specs[curSpec].flags & RSStringFormatLocalizable) && (RSGetTypeID(formatOptions) == RSLocaleGetTypeID())) {    // We have a locale, so we do localized formatting
//                    if (__RSStringFormatLocalizedNumber(outputString, (RSLocaleRef)formatOptions, values, &specs[curSpec], width, precision, hasPrecision)) break;
//                }
                /* Otherwise fall-thru to the next case! */
#endif
            case RSFormatPointerType: {
                char formatBuffer[128];
#if defined(__GNUC__)
                char buffer[BUFFER_LEN + width + precision];
#else
                char stackBuffer[BUFFER_LEN];
                char *dynamicBuffer = nil;
                char *buffer = stackBuffer;
                if (256+width+precision > BUFFER_LEN) {
                    dynamicBuffer = (char *)RSAllocatorAllocate(RSAllocatorSystemDefault, 256+width+precision, 0);
                    buffer = dynamicBuffer;
                }
#endif
                RSIndex cidx, idx, loc;
                BOOL appended = NO;
                loc = specs[curSpec].loc;
                // In preparation to call snprintf(), copy the format string out
                if (cformat) {
                    for (idx = 0, cidx = 0; cidx < specs[curSpec].len; idx++, cidx++) {
                        if ('$' == cformat[loc + cidx]) {
                            for (idx--; '0' <= formatBuffer[idx] && formatBuffer[idx] <= '9'; idx--);
                        } else {
                            formatBuffer[idx] = cformat[loc + cidx];
                        }
                    }
                } else {
                    for (idx = 0, cidx = 0; cidx < specs[curSpec].len; idx++, cidx++) {
                        if ('$' == uformat[loc + cidx]) {
                            for (idx--; '0' <= formatBuffer[idx] && formatBuffer[idx] <= '9'; idx--);
                        } else {
                            formatBuffer[idx] = (int8_t)uformat[loc + cidx];
                        }
                    }
                }
                formatBuffer[idx] = '\0';
                // Should modify format buffer here if necessary; for example, to translate %qd to
                // the equivalent, on architectures which do not have %q.
                buffer[sizeof(buffer) - 1] = '\0';
                switch (specs[curSpec].type) {
                    case RSFormatLongType:
                        if (RSFormatSize8 == specs[curSpec].size) {
                            SNPRINTF(int64_t, values[specs[curSpec].mainArgNum].value.int64Value)
                        } else {
                            SNPRINTF(SInt32, values[specs[curSpec].mainArgNum].value.int64Value)
                        }
                        break;
                    case RSFormatPointerType:
                    case RSFormatDummyPointerType:
                        SNPRINTF(void *, values[specs[curSpec].mainArgNum].value.pointerValue)
                        break;
                        
                    case RSFormatDoubleType:
#if LONG_DOUBLE_SUPPORT
                        if (RSFormatSize16 == specs[curSpec].size) {
                            SNPRINTF(long double, values[specs[curSpec].mainArgNum].value.longDoubleValue)
                        } else
#endif
                        {
                            SNPRINTF(double, values[specs[curSpec].mainArgNum].value.doubleValue)
                        }
                        // See if we need to localize the decimal point
                        if (formatOptions) {	// We have localization info
#if DEPLOYMENT_TARGET_MACOSX || DEPLOYMENT_TARGET_EMBEDDED || DEPLOYMENT_TARGET_EMBEDDED_MINI || DEPLOYMENT_TARGET_WINDOWS || DEPLOYMENT_TARGET_LINUX
//                            RSStringRef decimalSeparator = (RSGetTypeID(formatOptions) == RSLocaleGetTypeID()) ? (RSStringRef)RSLocaleGetValue((RSLocaleRef)formatOptions, RSLocaleDecimalSeparatorKey) : (RSStringRef)RSDictionaryGetValue(formatOptions, RSSTR("NSDecimalSeparator"));
#else
                            RSStringRef decimalSeparator = RSSTR(".");
#endif
                            RSStringRef decimalSeparator = RSSTR(".");
                            if (decimalSeparator != nil)
                            {
                                // We have a decimal separator in there
                                RSIndex decimalPointLoc = 0;
                                while (buffer[decimalPointLoc] != 0 && buffer[decimalPointLoc] != '.') decimalPointLoc++;
                                if (buffer[decimalPointLoc] == '.') {	// And we have a decimal point in the formatted string
                                    buffer[decimalPointLoc] = 0;
                                    RSStringAppendCString(outputString, (const char *)buffer, __RSStringGetEightBitStringEncoding());
                                    RSStringAppendString(outputString, decimalSeparator);
                                    RSStringAppendCString(outputString, (const char *)(buffer + decimalPointLoc + 1), __RSStringGetEightBitStringEncoding());
                                    appended = YES;
                                }
                            }
                        }
                        break;
                }
                if (!appended) RSStringAppendCString(outputString, (const char *)buffer, __RSStringGetEightBitStringEncoding());
#if !defined(__GNUC__)
                if (dynamicBuffer) {
                    RSAllocatorDeallocate(RSAllocatorSystemDefault, dynamicBuffer);
                }
#endif
            }
                break;
            case RSFormatLiteralType:
                if (cformat) {
                    __RSStringAppendBytes(outputString, (const char *)(cformat+specs[curSpec].loc), specs[curSpec].len, __RSStringGetEightBitStringEncoding());
                } else {
                    RSStringAppendCharacters(outputString, uformat+specs[curSpec].loc, specs[curSpec].len);
                }
                break;
            case RSFormatPascalCharsType:
            case RSFormatCharsType:
                if (values[specs[curSpec].mainArgNum].value.pointerValue == nil) {
                    RSStringAppendCString(outputString, "(null)", RSStringEncodingASCII);
                } else {
                    RSUInteger len = 0;
                    const char *str = (const char *)values[specs[curSpec].mainArgNum].value.pointerValue;
                    if (specs[curSpec].type == RSFormatPascalCharsType)
                    {
                        // Pascal string case
                        len = ((unsigned char *)str)[0];
                        str++;
                        if (hasPrecision && precision < len) len = precision;
                    }
                    else
                    {
                        // C-string case
                        if (!hasPrecision)
                        {
                            // No precision, so rely on the terminating null character
                            len = strlen(str);
                        }
                        else
                        {
                            // Don't blindly call strlen() if there is a precision; the string might not have a terminating null (3131988)
                            const char *terminatingNull = (const char *)memchr(str, 0, precision);	// Basically strlen() on only the first precision characters of str
                            if (terminatingNull) {	// There was a null in the first precision characters
                                len = terminatingNull - str;
                            } else {
                                len = precision;
                            }
                        }
                    }
                    // Since the spec says the behavior of the ' ', '0', '#', and '+' flags is undefined for
                    // '%s', and since we have ignored them in the past, the behavior is hereby cast in stone
                    // to ignore those flags (and, say, never pad with '0' instead of space).
                    if (specs[curSpec].flags & RSStringFormatMinusFlag) {
                        __RSStringAppendBytes(outputString, str, len, RSStringGetSystemEncoding());
                        if (hasWidth && width > len) {
                            RSIndex w = width - len;	// We need this many spaces; do it ten at a time
                            do {__RSStringAppendBytes(outputString, "          ", (w > 10 ? 10 : w), RSStringEncodingASCII);} while ((w -= 10) > 0);
                        }
                    } else {
                        if (hasWidth && width > len) {
                            RSIndex w = width - len;	// We need this many spaces; do it ten at a time
                            do {__RSStringAppendBytes(outputString, "          ", (w > 10 ? 10 : w), RSStringEncodingASCII);} while ((w -= 10) > 0);
                        }
                        __RSStringAppendBytes(outputString, str, len, RSStringGetSystemEncoding());
                    }
                }
                break;
            case RSFormatSingleUnicharType:
                ch = (UniChar)values[specs[curSpec].mainArgNum].value.int64Value;
                RSStringAppendCharacters(outputString, &ch, 1);
                break;
            case RSFormatUnicharsType:
                //??? need to handle width, precision, and padding arguments
                up = (UniChar *)values[specs[curSpec].mainArgNum].value.pointerValue;
                if (nil == up) {
                    RSStringAppendCString(outputString, "(null)", RSStringEncodingASCII);
                } else {
                    RSUInteger len;
                    for (len = 0; 0 != up[len]; len++);
                    // Since the spec says the behavior of the ' ', '0', '#', and '+' flags is undefined for
                    // '%s', and since we have ignored them in the past, the behavior is hereby cast in stone
                    // to ignore those flags (and, say, never pad with '0' instead of space).
                    if (hasPrecision && precision < len) len = precision;
                    if (specs[curSpec].flags & RSStringFormatMinusFlag) {
                        RSStringAppendCharacters(outputString, up, len);
                        if (hasWidth && width > len) {
                            RSUInteger w = width - len;	// We need this many spaces; do it ten at a time
                            do {__RSStringAppendBytes(outputString, "          ", (w > 10 ? 10 : w), RSStringEncodingASCII);} while ((w -= 10) > 0);
                        }
                    } else {
                        if (hasWidth && width > len) {
                            RSUInteger w = width - len;	// We need this many spaces; do it ten at a time
                            do {__RSStringAppendBytes(outputString, "          ", (w > 10 ? 10 : w), RSStringEncodingASCII);} while ((w -= 10) > 0);
                        }
                        RSStringAppendCharacters(outputString, up, len);
                    }
                }
                break;
            case RSFormatRSType:
            case RSFormatObjectType:
                if (specs[curSpec].configDictIndex != -1) { // config dict
                    RSTypeRef object = nil;
//                    RSStringRef innerFormat = nil;
                    
                    switch (values[specs[curSpec].mainArgNum].type) {
                        case RSFormatLongType:
                            object = RSNumberCreate(tmpAlloc, RSNumberLong, &(values[specs[curSpec].mainArgNum].value.int64Value));
                            break;
                            
                        case RSFormatDoubleType:
#if LONG_DOUBLE_SUPPORT
                            if (RSFormatSize16 == values[specs[curSpec].mainArgNum].size)
                            {
                                double aValue = values[specs[curSpec].mainArgNum].value.longDoubleValue; // losing precision
                                
                                object = RSNumberCreate(tmpAlloc, RSNumberDouble, &aValue);
                            }
                            else
#endif
                            {
                                object = RSNumberCreate(tmpAlloc, RSNumberDouble, &(values[specs[curSpec].mainArgNum].value.doubleValue));
                            }
                            break;
                            
                        case RSFormatPointerType:
                            object = RSNumberCreate(tmpAlloc, RSNumberLonglong, &(values[specs[curSpec].mainArgNum].value.pointerValue));
                            break;
                            
                        case RSFormatPascalCharsType:
                        case RSFormatCharsType:
                            if (nil != values[specs[curSpec].mainArgNum].value.pointerValue)
                            {
                                RSMutableStringRef aString = RSStringCreateMutable(tmpAlloc, 0);
                                RSUInteger len;
                                const char *str = (const char *)values[specs[curSpec].mainArgNum].value.pointerValue;
                                if (specs[curSpec].type == RSFormatPascalCharsType)
                                {
                                    // Pascal string case
                                    len = ((unsigned char *)str)[0];
                                    str++;
                                    if (hasPrecision && precision < len) len = precision;
                                }
                                else
                                {
                                    // C-string case
                                    if (!hasPrecision)
                                    {
                                        // No precision, so rely on the terminating null character
                                        len = strlen(str);
                                    }
                                    else
                                    {
                                        // Don't blindly call strlen() if there is a precision; the string might not have a terminating null (3131988)
                                        const char *terminatingNull = (const char *)memchr(str, 0, precision);	// Basically strlen() on only the first precision characters of str
                                        if (terminatingNull)
                                        {
                                            // There was a null in the first precision characters
                                            len = terminatingNull - str;
                                        }
                                        else
                                        {
                                            len = precision;
                                        }
                                    }
                                }
                                // Since the spec says the behavior of the ' ', '0', '#', and '+' flags is undefined for
                                // '%s', and since we have ignored them in the past, the behavior is hereby cast in stone
                                // to ignore those flags (and, say, never pad with '0' instead of space).
                                if (specs[curSpec].flags & RSStringFormatMinusFlag) {
                                    __RSStringAppendBytes(aString, str, len, RSStringGetSystemEncoding());
                                    if (hasWidth && width > len) {
                                        RSUInteger w = width - len;	// We need this many spaces; do it ten at a time
                                        do {__RSStringAppendBytes(aString, "          ", (w > 10 ? 10 : w), RSStringEncodingASCII);} while ((w -= 10) > 0);
                                    }
                                } else {
                                    if (hasWidth && width > len) {
                                        RSUInteger w = width - len;	// We need this many spaces; do it ten at a time
                                        do {__RSStringAppendBytes(aString, "          ", (w > 10 ? 10 : w), RSStringEncodingASCII);} while ((w -= 10) > 0);
                                    }
                                    __RSStringAppendBytes(aString, str, len, RSStringGetSystemEncoding());
                                }
                                
                                object = aString;
                            }
                            break;
                            
                        case RSFormatSingleUnicharType:
                            ch = (UniChar)values[specs[curSpec].mainArgNum].value.int64Value;
                            object = RSStringCreateWithCharactersNoCopy(tmpAlloc, &ch, 1, RSAllocatorNull);
                            break;
                            
                        case RSFormatUnicharsType:
                            //??? need to handle width, precision, and padding arguments
                            up = (UniChar *)values[specs[curSpec].mainArgNum].value.pointerValue;
                            if (nil != up) {
                                RSMutableStringRef aString = RSStringCreateMutable(tmpAlloc, 0);
                                RSUInteger len;
                                for (len = 0; 0 != up[len]; len++);
                                // Since the spec says the behavior of the ' ', '0', '#', and '+' flags is undefined for
                                // '%s', and since we have ignored them in the past, the behavior is hereby cast in stone
                                // to ignore those flags (and, say, never pad with '0' instead of space).
                                if (hasPrecision && precision < len) len = precision;
                                if (specs[curSpec].flags & RSStringFormatMinusFlag) {
                                    RSStringAppendCharacters(aString, up, len);
                                    if (hasWidth && width > len) {
                                        RSUInteger w = width - len;	// We need this many spaces; do it ten at a time
                                        do {__RSStringAppendBytes(aString, "          ", (w > 10 ? 10 : w), RSStringEncodingASCII);} while ((w -= 10) > 0);
                                    }
                                } else {
                                    if (hasWidth && width > len) {
                                        RSUInteger w = width - len;	// We need this many spaces; do it ten at a time
                                        do {__RSStringAppendBytes(aString, "          ", (w > 10 ? 10 : w), RSStringEncodingASCII);} while ((w -= 10) > 0);
                                    }
                                    RSStringAppendCharacters(aString, up, len);
                                }
                                object = aString;
                            }
                            break;
                            
                        case RSFormatRSType:
                        case RSFormatObjectType:
                            if (nil != values[specs[curSpec].mainArgNum].value.pointerValue) object = RSRetain(values[specs[curSpec].mainArgNum].value.pointerValue);
                            break;
                    }
                    
                    if (nil != object) RSRelease(object);
                    
                } else if (nil != values[specs[curSpec].mainArgNum].value.pointerValue)
                {
                    RSStringRef str = nil;
                    if (copyDescfunc)
                    {
                        str = copyDescfunc(values[specs[curSpec].mainArgNum].value.pointerValue, formatOptions);
                    }
                    else
                    {
//                        str = __RSCopyFormattingDescription(values[specs[curSpec].mainArgNum].value.pointerValue, formatOptions);
//                        if (nil == str)
//                        {
                            str = (RSStringRef)RSDescription(values[specs[curSpec].mainArgNum].value.pointerValue);
//                        }
                    }
                    if (str)
                    {

                        RSStringAppendString(outputString, str);                        
                        RSRelease(str);
                    }
                    else
                    {
                        RSStringAppendCString(outputString, "(null description)", RSStringEncodingASCII);
                    }
                }
                else
                {
                    RSStringAppendCString(outputString, "(null)", RSStringEncodingASCII);
                }
                break;
        }
    }
    
#if DEPLOYMENT_TARGET_MACOSX || DEPLOYMENT_TARGET_EMBEDDED || DEPLOYMENT_TARGET_EMBEDDED_MINI || DEPLOYMENT_TARGET_LINUX || DEPLOYMENT_TARGET_FREEBSD
    // va_copy is a C99 extension. No support on Windows
    if (numConfigs > 0) va_end(copiedArgs);
#endif
    if (specs != localSpecsBuffer) RSAllocatorDeallocate(tmpAlloc, specs);
    if (values != localValuesBuffer) RSAllocatorDeallocate(tmpAlloc, values);
    if (formatChars && (formatChars != localFormatBuffer)) RSAllocatorDeallocate(tmpAlloc, formatChars);
    if (configs != localConfigs) RSAllocatorDeallocate(tmpAlloc, configs);
}

#undef SNPRINTF

RSExport RSStringRef  RSStringCreateWithBytesNoCopy(RSAllocatorRef alloc, const void *bytes, RSIndex numBytes, RSStringEncoding encoding, BOOL externalFormat, RSAllocatorRef contentsDeallocator);
static RSStringRef __RSStringFormatAndArguments(RSAllocatorRef allocator, RSIndex size, RSStringRef format, va_list arguments)
{
    RSRetain(format);
    RSCUBuffer __format = __RSStrContents(format) + __RSStrSkipAnyLengthByte(format);
    RSIndex length = size ?: 64;
    RSBuffer __formatString = nil;
    
    RSIndex result = 0;
    va_list copy ;
    //RSIndex moreLength = 0;
    while (1)    //core
    {
        va_copy(copy, arguments);
        result = __RSFormatPrint(&__formatString, &length, __format, copy);
        //result = __RSFormatPrint(__formatString, length, &moreLength, __format, copy);
        if (result > -1 && result < length )
        {
            RSRelease(format);
            va_end(copy);
//            return __RSStringCreateInstance(allocator, (RSUBuffer)__formatString, strlen(__formatString), length, YES, NO, NO, NO  , RSStringEncodingUTF8);//RSStringCreateWithCStringNoCopy(allocator, __formatString);
            return RSStringCreateWithBytesNoCopy(allocator, (uint8_t *)__formatString, length, RSStringEncodingUTF8, NO, RSAllocatorSystemDefault);
        }
        va_end(copy);
        //length += moreLength;
        //length += 1; if (length == RSUIntegerMax) break;   // fix the overflow bug.
        //RSAllocatorDeallocate(allocator, __formatString);
        //__formatString = RSAllocatorReallocate(allocator, __formatString, length);
        //length = RSAllocatorSize(allocator, length);
    }
    RSAllocatorDeallocate(allocator, __formatString);
    RSRelease(format);
    return nil;
}

RSStringRef  _RSStringCreateWithFormatAndArgumentsAux(RSAllocatorRef alloc, RSStringRef (*copyDescfunc)(void *, const void *), RSIndex capacity, RSStringRef format, va_list arguments)
{
    RSStringRef str;
    RSMutableStringRef outputString = RSStringCreateMutable(RSAllocatorSystemDefault, 0); //should use alloc if no copy/release
    
    __RSStrSetDesiredCapacity(outputString, capacity);	// Given this will be tightened later, choosing a larger working string is fine
    __RSStringAppendFormatCore(outputString, copyDescfunc, nil, format, 0, nil, 0, arguments);

    // ??? copy/release should not be necessary here -- just make immutable, compress if possible
    // (However, this does make the string inline, and cause the supplied allocator to be used...)
    str = RSCopy(alloc, outputString); // RSStringCreateCopy
    RSRelease(outputString);
    return str;
}

RSExport RSStringRef  RSStringCreateWithFormat(RSAllocatorRef allocator, RSStringRef format, ...)
{
    RSStringRef result;
    va_list argList;
    
    va_start(argList, format);
    result = RSStringCreateWithFormatAndArguments(allocator, 120, format, argList);
    va_end(argList);
    
    return result;
}

enum {
	VALID = 1,
	UNRESERVED = 2,
	PATHVALID = 4,
	SCHEME = 8,
	HEXDIGIT = 16,
    NONQUOT = 32
};

static const unsigned char sStringDescriptionValidCharacters[128] = {
    /* nul   0 */   0,
    /* soh   1 */   0,
    /* stx   2 */   0,
    /* etx   3 */   0,
    /* eot   4 */   0,
    /* enq   5 */   0,
    /* ack   6 */   0,
    /* bel   7 */   0,
    /* bs    8 */   0,
    /* ht    9 */   NONQUOT,
    /* nl   10 */   0,
    /* vt   11 */   0,
    /* np   12 */   0,
    /* cr   13 */   0,
    /* so   14 */   0,
    /* si   15 */   0,
    /* dle  16 */   0,
    /* dc1  17 */   0,
    /* dc2  18 */   0,
    /* dc3  19 */   0,
    /* dc4  20 */   0,
    /* nak  21 */   0,
    /* syn  22 */   0,
    /* etb  23 */   0,
    /* can  24 */   0,
    /* em   25 */   0,
    /* sub  26 */   0,
    /* esc  27 */   0,
    /* fs   28 */   0,
    /* gs   29 */   0,
    /* rs   30 */   0,
    /* us   31 */   0,
    /* sp   32 */   0,
    /* '!'  33 */   VALID | UNRESERVED | PATHVALID ,
    /* '"'  34 */   NONQUOT,
    /* '#'  35 */   0,
    /* '$'  36 */   VALID | PATHVALID ,
    /* '%'  37 */   0,
    /* '&'  38 */   VALID | PATHVALID ,
    /* '''  39 */   VALID | UNRESERVED | PATHVALID | NONQUOT,
    /* '('  40 */   VALID | UNRESERVED | PATHVALID ,
    /* ')'  41 */   VALID | UNRESERVED | PATHVALID ,
    /* '*'  42 */   VALID | UNRESERVED | PATHVALID ,
    /* '+'  43 */   VALID | SCHEME | PATHVALID ,
    /* ','  44 */   VALID | PATHVALID ,
    /* '-'  45 */   VALID | UNRESERVED | SCHEME | PATHVALID ,
    /* '.'  46 */   VALID | UNRESERVED | SCHEME | PATHVALID ,
    /* '/'  47 */   VALID | PATHVALID ,
    /* '0'  48 */   VALID | UNRESERVED | SCHEME | PATHVALID | NONQUOT |HEXDIGIT,
    /* '1'  49 */   VALID | UNRESERVED | SCHEME | PATHVALID | NONQUOT |HEXDIGIT,
    /* '2'  50 */   VALID | UNRESERVED | SCHEME | PATHVALID | NONQUOT |HEXDIGIT,
    /* '3'  51 */   VALID | UNRESERVED | SCHEME | PATHVALID | NONQUOT |HEXDIGIT,
    /* '4'  52 */   VALID | UNRESERVED | SCHEME | PATHVALID | NONQUOT |HEXDIGIT,
    /* '5'  53 */   VALID | UNRESERVED | SCHEME | PATHVALID | NONQUOT |HEXDIGIT,
    /* '6'  54 */   VALID | UNRESERVED | SCHEME | PATHVALID | NONQUOT |HEXDIGIT,
    /* '7'  55 */   VALID | UNRESERVED | SCHEME | PATHVALID | NONQUOT |HEXDIGIT,
    /* '8'  56 */   VALID | UNRESERVED | SCHEME | PATHVALID | NONQUOT |HEXDIGIT,
    /* '9'  57 */   VALID | UNRESERVED | SCHEME | PATHVALID | NONQUOT |HEXDIGIT,
    /* ':'  58 */   VALID ,
    /* ';'  59 */   VALID ,
    /* '<'  60 */   0,
    /* '='  61 */   VALID | PATHVALID ,
    /* '>'  62 */   0,
    /* '?'  63 */   VALID ,
    /* '@'  64 */   VALID ,
    /* 'A'  65 */   VALID | UNRESERVED | SCHEME | PATHVALID | NONQUOT | HEXDIGIT ,
    /* 'B'  66 */   VALID | UNRESERVED | SCHEME | PATHVALID | NONQUOT | HEXDIGIT ,
    /* 'C'  67 */   VALID | UNRESERVED | SCHEME | PATHVALID | NONQUOT | HEXDIGIT ,
    /* 'D'  68 */   VALID | UNRESERVED | SCHEME | PATHVALID | NONQUOT | HEXDIGIT ,
    /* 'E'  69 */   VALID | UNRESERVED | SCHEME | PATHVALID | NONQUOT | HEXDIGIT ,
    /* 'F'  70 */   VALID | UNRESERVED | SCHEME | PATHVALID | NONQUOT | HEXDIGIT ,
    /* 'G'  71 */   VALID | UNRESERVED | SCHEME | PATHVALID | NONQUOT,
    /* 'H'  72 */   VALID | UNRESERVED | SCHEME | PATHVALID | NONQUOT,
    /* 'I'  73 */   VALID | UNRESERVED | SCHEME | PATHVALID | NONQUOT,
    /* 'J'  74 */   VALID | UNRESERVED | SCHEME | PATHVALID | NONQUOT,
    /* 'K'  75 */   VALID | UNRESERVED | SCHEME | PATHVALID | NONQUOT,
    /* 'L'  76 */   VALID | UNRESERVED | SCHEME | PATHVALID | NONQUOT,
    /* 'M'  77 */   VALID | UNRESERVED | SCHEME | PATHVALID | NONQUOT,
    /* 'N'  78 */   VALID | UNRESERVED | SCHEME | PATHVALID | NONQUOT,
    /* 'O'  79 */   VALID | UNRESERVED | SCHEME | PATHVALID | NONQUOT,
    /* 'P'  80 */   VALID | UNRESERVED | SCHEME | PATHVALID | NONQUOT,
    /* 'Q'  81 */   VALID | UNRESERVED | SCHEME | PATHVALID | NONQUOT,
    /* 'R'  82 */   VALID | UNRESERVED | SCHEME | PATHVALID | NONQUOT,
    /* 'S'  83 */   VALID | UNRESERVED | SCHEME | PATHVALID | NONQUOT,
    /* 'T'  84 */   VALID | UNRESERVED | SCHEME | PATHVALID | NONQUOT,
    /* 'U'  85 */   VALID | UNRESERVED | SCHEME | PATHVALID | NONQUOT,
    /* 'V'  86 */   VALID | UNRESERVED | SCHEME | PATHVALID | NONQUOT,
    /* 'W'  87 */   VALID | UNRESERVED | SCHEME | PATHVALID | NONQUOT,
    /* 'X'  88 */   VALID | UNRESERVED | SCHEME | PATHVALID | NONQUOT,
    /* 'Y'  89 */   VALID | UNRESERVED | SCHEME | PATHVALID | NONQUOT,
    /* 'Z'  90 */   VALID | UNRESERVED | SCHEME | PATHVALID | NONQUOT,
    /* '['  91 */   0,
    /* '\'  92 */   0,
    /* ']'  93 */   0,
    /* '^'  94 */   0,
    /* '_'  95 */   VALID | UNRESERVED | PATHVALID ,
    /* '`'  96 */   0,
    /* 'a'  97 */   VALID | UNRESERVED | SCHEME | PATHVALID | NONQUOT | HEXDIGIT ,
    /* 'b'  98 */   VALID | UNRESERVED | SCHEME | PATHVALID | NONQUOT | HEXDIGIT ,
    /* 'c'  99 */   VALID | UNRESERVED | SCHEME | PATHVALID | NONQUOT | HEXDIGIT ,
    /* 'd' 100 */   VALID | UNRESERVED | SCHEME | PATHVALID | NONQUOT | HEXDIGIT ,
    /* 'e' 101 */   VALID | UNRESERVED | SCHEME | PATHVALID | NONQUOT | HEXDIGIT ,
    /* 'f' 102 */   VALID | UNRESERVED | SCHEME | PATHVALID | NONQUOT | HEXDIGIT ,
    /* 'g' 103 */   VALID | UNRESERVED | SCHEME | PATHVALID | NONQUOT,
    /* 'h' 104 */   VALID | UNRESERVED | SCHEME | PATHVALID | NONQUOT,
    /* 'i' 105 */   VALID | UNRESERVED | SCHEME | PATHVALID | NONQUOT,
    /* 'j' 106 */   VALID | UNRESERVED | SCHEME | PATHVALID | NONQUOT,
    /* 'k' 107 */   VALID | UNRESERVED | SCHEME | PATHVALID | NONQUOT,
    /* 'l' 108 */   VALID | UNRESERVED | SCHEME | PATHVALID | NONQUOT,
    /* 'm' 109 */   VALID | UNRESERVED | SCHEME | PATHVALID | NONQUOT,
    /* 'n' 110 */   VALID | UNRESERVED | SCHEME | PATHVALID | NONQUOT,
    /* 'o' 111 */   VALID | UNRESERVED | SCHEME | PATHVALID | NONQUOT,
    /* 'p' 112 */   VALID | UNRESERVED | SCHEME | PATHVALID | NONQUOT,
    /* 'q' 113 */   VALID | UNRESERVED | SCHEME | PATHVALID | NONQUOT,
    /* 'r' 114 */   VALID | UNRESERVED | SCHEME | PATHVALID | NONQUOT,
    /* 's' 115 */   VALID | UNRESERVED | SCHEME | PATHVALID | NONQUOT,
    /* 't' 116 */   VALID | UNRESERVED | SCHEME | PATHVALID | NONQUOT,
    /* 'u' 117 */   VALID | UNRESERVED | SCHEME | PATHVALID | NONQUOT,
    /* 'v' 118 */   VALID | UNRESERVED | SCHEME | PATHVALID | NONQUOT,
    /* 'w' 119 */   VALID | UNRESERVED | SCHEME | PATHVALID | NONQUOT,
    /* 'x' 120 */   VALID | UNRESERVED | SCHEME | PATHVALID | NONQUOT,
    /* 'y' 121 */   VALID | UNRESERVED | SCHEME | PATHVALID | NONQUOT,
    /* 'z' 122 */   VALID | UNRESERVED | SCHEME | PATHVALID | NONQUOT,
    /* '{' 123 */   0,
    /* '|' 124 */   0,
    /* '}' 125 */   0,
    /* '~' 126 */   VALID | UNRESERVED | PATHVALID ,
    /* del 127 */   0,
};

RSInline BOOL __RSStringDescriptionShouldHasQuotationMask(RSStringRef str) {
    const uint8_t *bytes = (const uint8_t*)__RSStrContents(str) + __RSStrSkipAnyLengthByte(str);
    RSIndex len = (RSIndex)__RSStrLength(str);
    if (bytes[0] == '"' && bytes[len - 1] == '"') return NO;
    for (RSIndex idx = 0; idx < len; idx++) {
        if (bytes[idx] < 128) {
            if ((sStringDescriptionValidCharacters[bytes[idx]] & NONQUOT) == 0) return YES;
            continue;
        }
        return YES;
    }
    return NO;
}

RSInline RSStringRef __RSStringDescriptionEx(RSTypeRef rs) {
    if (__RSStringDescriptionShouldHasQuotationMask(rs))
    {
        RSStringRef description = nil;
        RSMutableStringRef tmp = RSStringCreateMutable(RSAllocatorSystemDefault, 0);
        RSStringAppendCString(tmp, "\"", RSStringEncodingASCII);
        RSStringAppendString(tmp, rs);
        RSStringAppendCString(tmp, "\"", RSStringEncodingASCII);
        description = RSCopy(RSAllocatorSystemDefault, tmp);
        RSRelease(tmp);
        
        return description;
    }
    return RSCopy(RSAllocatorSystemDefault, rs);
}

RSExport RSStringRef RSStringCopyDetailDescription(RSStringRef str) {
    if (nil == str || (RSStringRef)RSNil == str) {
        return RSRetain(_RSEmptyString);
    }
    __RSGenericValidInstance(str, __RSStringTypeID);
    if (__RSStrLength(str)) {
        return __RSStringDescriptionEx(str);
    }
    return RSRetain(_RSEmptyString);
}

static RSStringRef __RSStringDescription(RSTypeRef rs) {
    if (__RSStrLength(rs))
    {
        return RSCopy(RSAllocatorSystemDefault, rs);
//        return __RSStringDescriptionEx(rs);
    }
    return RSRetain(_RSEmptyString);
}

static RSStringRef __RSStringCopyFormattingDescription(RSTypeRef RS, RSDictionaryRef formatOptions) {
    return (RSStringRef)RSStringCreateCopy(__RSGetAllocator(RS), (RSStringRef)RS);
}

typedef RSTypeRef (*RS_STRING_CREATE_COPY)(RSAllocatorRef alloc, RSTypeRef theString);



static const RSRuntimeClass __RSStringClass = {
    _RSRuntimeScannedObject,
    0,
    "RSString",
    nil,      // init
    __RSStringCopy,
    __RSStringDeallocate,
    __RSStringEqual,
    __RSStringHash,
    __RSStringDescription
};

RSPrivate void __RSStringInitialize(void) {
    __RSStringTypeID = __RSRuntimeRegisterClass(&__RSStringClass);
    __RSRuntimeSetClassTypeID(&__RSStringClass, __RSStringTypeID);

}

RSTypeID RSStringGetTypeID(void) {
    return __RSStringTypeID;
}

RSExport RSStringRef RSStringGetEmptyString()
{
    return RSRetain(_RSEmptyString);
}

static BOOL RSStrIsUnicode(RSStringRef str) {
    RS_OBJC_FUNCDISPATCHV(__RSStringTypeID, BOOL, (NSString *)str, _encodingCantBeStoredInEightBitRSString);
    return __RSStrIsUnicode(str);
}

// Can pass in NSString as replacement string

RSStringRef RSStringCreateCopy(RSAllocatorRef alloc, RSStringRef str);
RSExport RSIndex RSStringGetLength(RSStringRef str)
{
    __RSGenericValidInstance(str, __RSStringTypeID);
    return __RSStrLength(str);
}

RSExport RSIndex RSStringGetBytes(RSStringRef str, RSRange range, RSStringEncoding encoding, uint8_t lossByte, BOOL isExternalRepresentation, void *buffer, RSIndex maxBufLen, RSIndex *usedBufLen)
{
    
    /* No objc dispatch needed here since __RSStringEncodeByteStream works with both RSString and NSString */
    __RSAssertIsNotNegative(maxBufLen);
    
    if (!RS_IS_OBJC(__RSStringTypeID, str))
    {
        // If we can grope the ivars, let's do it...
        __RSAssertIsString(str);
        __RSAssertRangeIsInStringBounds(str, range.location, range.length);
//        if (!((range.location) >= 0 && (range.location + range.length) <= __RSStrLength(str)))
//        {
//            __RSCLog(RSLogLevelNotice, "");
//        }
        if (__RSStrIsEightBit(str) && ((__RSStringGetEightBitStringEncoding() == encoding) || (__RSStringGetEightBitStringEncoding() == RSStringEncodingASCII && __RSStringEncodingIsSupersetOfASCII(encoding))))
        {
            // Requested encoding is equal to the encoding in string
            const unsigned char *contents = (const unsigned char *)__RSStrContents(str);
            RSIndex cLength = range.length;
            
            if (buffer)
            {
                if (cLength > maxBufLen) cLength = maxBufLen;
                memmove(buffer, contents + __RSStrSkipAnyLengthByte(str) + range.location, cLength);
            }
            if (usedBufLen) *usedBufLen = cLength;
            
            return cLength;
        }
    }
    
    return __RSStringEncodeByteStream(str, range.location, range.length, isExternalRepresentation, encoding, lossByte, buffer, maxBufLen, usedBufLen);
}

RSInline void __RSStringReplace(RSMutableStringRef str, RSRange range, RSStringRef replacement)
{
    RSStringRef copy = nil;
    if (replacement == str) copy = replacement = (RSStringRef)RSStringCreateCopy(RSAllocatorSystemDefault, replacement);   // Very special and hopefully rare case
    else if (replacement == nil) replacement = _RSEmptyString;
    RSIndex replacementLength = RSStringGetLength(replacement);

    __RSStringChangeSize(str, range, replacementLength, (replacementLength > 0) && RSStrIsUnicode(replacement));
    if (__RSStrIsUnicode(str)) {
        UniChar *contents = (UniChar *)__RSStrContents(str);
        RSStringGetCharacters(replacement, RSMakeRange(0, replacementLength), contents + range.location);
    } else {
        uint8_t *contents = (uint8_t *)__RSStrContents(str);
        RSStringGetBytes(replacement, RSMakeRange(0, replacementLength), __RSStringGetEightBitStringEncoding(), 0, NO, contents + range.location + __RSStrSkipAnyLengthByte(str), replacementLength, nil);
    }
    
    if (copy) RSRelease(copy);
}

/* If client does not provide a minimum capacity
 */
#define DEFAULTMINCAPACITY 32

RSInline RSMutableStringRef __RSStringCreateMutableFunnel(RSAllocatorRef alloc, RSIndex maxLength, RSBitU32 additionalInfoBits)
{
    RSMutableStringRef str;
    if (_RSAllocatorIsGCRefZero(alloc)) additionalInfoBits |= __RSHasContentsAllocator;
    BOOL hasExternalContentsAllocator = (additionalInfoBits & __RSHasContentsAllocator) ? YES : NO;
    
    if (alloc == nil) alloc = RSAllocatorGetDefault();
    
    // Note that if there is an externalContentsAllocator, then we also have the storage for the string allocator...
    str = (RSMutableStringRef)__RSRuntimeCreateInstance(alloc, __RSStringTypeID, sizeof(struct __notInlineMutable) - (hasExternalContentsAllocator ? 0 : sizeof(RSAllocatorRef)));
    if (str)
    {
//        if (__RSOASafe) __RSSetLastAllocationEventName(str, "RSString (mutable)");
        
        __RSStrSetInfoBits(str, __RSIsMutable | additionalInfoBits);
        str->_variants._notInlineMutable1._buffer = nil;
        __RSStrSetExplicitLength(str, 0);
        str->_variants._notInlineMutable1._hasGap = str->_variants._notInlineMutable1._isFixedCapacity = str->_variants._notInlineMutable1._isExternalMutable = str->_variants._notInlineMutable1._capacityProvidedExternally = 0;
        if (maxLength != 0) __RSStrSetIsFixed(str);
        __RSStrSetDesiredCapacity(str, (maxLength == 0) ? DEFAULTMINCAPACITY : maxLength);
        __RSStrSetCapacity(str, 0);
        if (__RSStrHasContentsAllocator(str))
        {
            // contents allocator starts out as the string's own allocator
            __RSStrSetContentsAllocator(str, alloc);
        }
    }
    return str;
}

/*** Mutable functions... ***/

RSExport void RSStringSetExternalCharactersNoCopy(RSMutableStringRef string, UniChar *chars, RSIndex length, RSIndex capacity)
{
    __RSAssertIsNotNegative(length);
    __RSAssertIsStringAndExternalMutable(string);
    RSAssert4((length <= capacity) && ((capacity == 0) || ((capacity > 0) && chars)), __RSLogAssertion, "%s(): Invalid args: characters %p length %d capacity %d", __PRETTY_FUNCTION__, chars, length, capacity);
    __RSStrSetContentPtr(string, chars);
    __RSStrSetExplicitLength(string, length);
    __RSStrSetCapacity(string, capacity * sizeof(UniChar));
    __RSStrSetCapacityProvidedExternally(string);
}


RSExport RSMutableStringRef RSStringCreateMutableWithExternalCharactersNoCopy(RSAllocatorRef alloc, UniChar *chars, RSIndex numChars, RSIndex capacity, RSAllocatorRef externalCharactersAllocator)
{
    RSOptionFlags contentsAllocationBits = externalCharactersAllocator ? ((externalCharactersAllocator == RSAllocatorNull) ? __RSNotInlineContentsNoFree : __RSHasContentsAllocator) : __RSNotInlineContentsDefaultFree;
    RSMutableStringRef string = __RSStringCreateMutableFunnel(alloc, 0, (RSBitU32)contentsAllocationBits | __RSIsUnicode);
    if (string)
    {
        __RSStrSetIsExternalMutable(string);
        if (__RSStrHasContentsAllocator(string)) {
            RSAllocatorRef allocator = __RSStrContentsAllocator((RSMutableStringRef)string);
//            if (!(RSAllocatorSystemDefaultGCRefZero == allocator || RSAllocatorDefaultGCRefZero == allocator)) RSRelease(allocator);
            RSRelease(allocator);
            __RSStrSetContentsAllocator(string, externalCharactersAllocator);
        }
        RSStringSetExternalCharactersNoCopy(string, chars, numChars, capacity);
    }
    return string;
}

RSExport RSMutableStringRef RSStringCreateMutable(RSAllocatorRef alloc, RSIndex maxLength)
{
    return __RSStringCreateMutableFunnel(alloc, maxLength, __RSNotInlineContentsDefaultFree);
}

RSExport RSMutableStringRef  RSStringCreateMutableCopy(RSAllocatorRef alloc, RSIndex maxLength, RSStringRef string)
{
    RSMutableStringRef newString;
    
    //  RS_OBJC_FUNCDISPATCHV(__RSStringTypeID, RSMutableStringRef, (NSString *)string, mutableCopy);
    
    __RSAssertIsString(string);
    
    newString = RSStringCreateMutable(alloc, maxLength);
//    if (__RSStrLength(string))
        __RSStringReplace(newString, RSMakeRange(0, 0), string);
//    else
//    {
//        __RSStrSetContentPtr(newString, RSAllocatorAllocate(RSAllocatorSystemDefault, 1));
//        __RSStrClearUnicode(newString);
//        __RSStrSetExplicitLength(newString, 0);
//        __RSStrClearHasLengthAndNullBytes(newString);
//    }
    return newString;
}

static RSStringRef __RSStringCreateInstanceImmutable(RSAllocatorRef allocator,
                                                      const void *bytes,
                                                      RSIndex numBytes,
                                                      RSStringEncoding encoding,
                                                      BOOL possiblyExternalFormat,
                                                      BOOL tryToReduceUnicode,
                                                      BOOL hasLengthByte,
                                                      BOOL hasNullByte,
                                                      BOOL noCopy,
                                                      RSAllocatorRef contentsDeallocator,
                                                      RSBitU32 reservedFlags)
{
//    if (!bytes || numBytes == 0) return nil;
    if (!bytes) return RSRetain(_RSEmptyString);
    RSMutableStringRef str = nil;
    
    RSVarWidthCharBuffer vBuf = {0};
    
    RSIndex size = 0;
    BOOL useLengthByte = NO;
    BOOL useNullByte = NO;
    BOOL useInlineData = NO;
    
    if (allocator == nil) allocator = RSAllocatorGetDefault();

    if (contentsDeallocator == ALLOCATORSFREEFUNC) {
        contentsDeallocator = allocator;
    } else if (contentsDeallocator == nil) {
        contentsDeallocator = RSAllocatorGetDefault();
    }
    
    if ((nil != _RSEmptyString) && (numBytes == 0) && (allocator == RSAllocatorSystemDefault))
    {
        // If we are using the system default allocator, and the string is empty, then use the empty string!
        if (noCopy)
        {
            // See 2365208... This change was done after Sonata; before we didn't free the bytes at all (leak).
            RSAllocatorDeallocate(contentsDeallocator, (void *)bytes);
        }
        return (RSStringRef)RSRetain(_RSEmptyString);	// Quick exit; won't catch all empty strings, but most
    }
    
    // At this point, contentsDeallocator is either same as allocator, or RSAllocatorNull(do nothing with memeory), or something else, but not nil
   
    vBuf.shouldFreeChars = NO;	// We use this to remember to free the buffer possibly allocated by decode

    BOOL stringSupportsEightBitRSRepresentation = encoding != RSStringEncodingUnicode && __RSCanUseEightBitRSStringForBytes((const uint8_t *)bytes, numBytes, encoding);
    
    BOOL stringROMShouldIgnoreNoCopy = NO;
    
    if ((encoding == RSStringEncodingUnicode && possiblyExternalFormat) ||
        (encoding != RSStringEncodingUnicode && !stringSupportsEightBitRSRepresentation))
    {
        const void *realBytes = (uint8_t *) bytes + (hasLengthByte ? 1 : 0);
        RSIndex realNumBytes = numBytes - (hasLengthByte ? 1 : 0);
        BOOL usingPassedInMemory = NO;
        
        vBuf.allocator = RSAllocatorSystemDefault;
        vBuf.chars.unicode = nil;
        
        if (!__RSStringDecodeByteStream3((const uint8_t *)realBytes, realNumBytes, encoding, NO, &vBuf, &usingPassedInMemory, reservedFlags))
        {
            return nil;
        }
        encoding = vBuf.isASCII ? RSStringEncodingASCII : RSStringEncodingUnicode;
        stringSupportsEightBitRSRepresentation = vBuf.isASCII;
        
        if (!usingPassedInMemory) {
            stringROMShouldIgnoreNoCopy = YES;
            numBytes = vBuf.isASCII ? vBuf.numChars : (vBuf.numChars * sizeof(UniChar));
            hasLengthByte = hasNullByte = NO;
            
            if (noCopy && contentsDeallocator != nil) {
                RSAllocatorDeallocate(contentsDeallocator, bytes);
            }
            
            contentsDeallocator = allocator;
            
            if (vBuf.shouldFreeChars && (allocator == vBuf.allocator) && encoding == RSStringEncodingUnicode)
            {
                vBuf.shouldFreeChars = NO;
                bytes = RSAllocatorReallocate(vBuf.allocator, (void *)vBuf.chars.unicode, numBytes);
                
                noCopy = YES;
                
            }
            else
            {
                bytes = vBuf.chars.unicode;
                noCopy = NO;
            }
        }
    }
    else if (encoding == RSStringEncodingUnicode && tryToReduceUnicode)
    {
        RSIndex cnt = 0, len = numBytes / sizeof(UniChar);
        BOOL allASCII = YES;
        for (cnt = 0; cnt < len; cnt ++)
        {
            if (((const UniChar *)bytes)[cnt] > 127)
            {
                allASCII = NO;
                break;
            }
        }
        
        if (allASCII) {
            uint8_t *ptr = nil, *mem = nil;
            BOOL newHasLengthByte = __RSCanUseLengthByte(len);
            numBytes = (len + 1 + (newHasLengthByte ? 1 : 0) * sizeof(uint8_t));
            
            if (numBytes >= __RSVarWidthLocalBufferSize) {
                mem = ptr = (uint8_t *)RSAllocatorAllocate(allocator, numBytes);
            }
            else {
                mem = ptr = (uint8_t *)(vBuf.localBuffer);
            }
            
            if (mem) {
                hasLengthByte = newHasLengthByte;
                hasNullByte = YES;
                if (hasLengthByte) *ptr++ =(uint8_t)len;
                
                for (cnt = 0; cnt < len; cnt++) {
                    ptr[cnt] = (uint8_t)(((const UniChar *)bytes)[cnt]);
                }
                
                ptr[len] = 0;
                
                if (noCopy && (contentsDeallocator != RSAllocatorNull)) {
                    RSAllocatorDeallocate(contentsDeallocator, bytes);
                }
                
                bytes = mem;
                encoding = RSStringEncodingASCII;
                
                contentsDeallocator = allocator;
                
                noCopy = (numBytes >= __RSVarWidthLocalBufferSize);
                
                numBytes--;
                
                stringSupportsEightBitRSRepresentation = YES;
                
                stringROMShouldIgnoreNoCopy = YES;
            }
        }
    }
    
    RSStringRef romResult = nil;
    
    if (! romResult) {
        if (noCopy) {
            size = sizeof(void *);      // Pointer to the buffer
            if (contentsDeallocator != allocator && contentsDeallocator != RSAllocatorNull){
                size += sizeof(void *); // the content deallocator
            }
            if (!hasLengthByte) size += sizeof(RSIndex); // explicit
            
            useLengthByte = hasLengthByte;
            useNullByte = hasNullByte;
        }
        else
        {
            // inline data, reserve space for it
            useInlineData = YES;
            size = numBytes;
            
            if (hasLengthByte || (encoding != RSStringEncodingUnicode && __RSCanUseLengthByte(numBytes)))
            {
                useLengthByte = YES;
                if (!hasLengthByte) size += 1;
            }
            else
            {
                size += sizeof(RSIndex);
            }
            
            if (hasNullByte || encoding != RSStringEncodingUnicode)
            {
                useNullByte = YES;
                size += 1;
            }
        }
        
        str = (RSMutableStringRef)__RSRuntimeCreateInstance(allocator, __RSStringTypeID, size);
        if (str) {
            RSOptionFlags allocBits = (contentsDeallocator == allocator ? __RSNotInlineContentsDefaultFree : (contentsDeallocator == RSAllocatorNull ? __RSNotInlineContentsNoFree : __RSNotInlineContentsCustomFree));
            __RSStrSetInfoBits(str,
                               (UInt32)(useInlineData ? __RSHasInlineContents : allocBits) |
                               ((encoding == RSStringEncodingUnicode) ? __RSIsUnicode : 0) |
                               (useNullByte ? __RSHasNullByte : 0) |
                               (useLengthByte ? __RSHasLengthByte : 0));
            
            if (!useLengthByte)
            {
                RSIndex length = numBytes - (hasLengthByte ? 1 : 0);
                if (encoding == RSStringEncodingUnicode) length /= sizeof(UniChar);
                __RSStrSetExplicitLength(str, length);
            }
            
            if (useInlineData)
            {
                uint8_t *contents = (uint8_t *)__RSStrContents(str);
                if (useLengthByte && !hasLengthByte) *contents++ = (uint8_t)numBytes;
                memmove(contents, bytes, numBytes);
                if (useNullByte) contents[numBytes] = 0;
            }
            else
            {
                __RSStrSetContentPtr(str, bytes);
                if (__RSStrHasContentsDeallocator(str)) __RSStrSetContentsDeallocator(str, contentsDeallocator);
            }
        }
        else
        {
            if (noCopy && (contentsDeallocator != RSAllocatorNull))
            {
                RSAllocatorDeallocate(contentsDeallocator, (void *)bytes);
            }
        }
    }
    if (vBuf.shouldFreeChars) RSAllocatorDeallocate(vBuf.allocator, (void *)bytes);
    
    return str;
}

RSExport const UniChar *RSStringGetCharactersPtr(RSStringRef str)
{
    
    RS_OBJC_FUNCDISPATCHV(__RSStringTypeID, const UniChar *, (NSString *)str, _fastCharacterContents);
    
    __RSAssertIsString(str);
    if (__RSStrIsUnicode(str)) return (const UniChar *)__RSStrContents(str);
    return nil;
}

RSExport RSRange RSStringGetRange(RSStringRef str)
{
    return RSMakeRange(0, RSStringGetLength(str));
}

#define MAX_TRANSCODING_LENGTH 4

#define HANGUL_JONGSEONG_COUNT (28)
#define MAX_CASE_MAPPING_BUF (8)
#define ZERO_WIDTH_JOINER (0x200D)
#define COMBINING_GRAPHEME_JOINER (0x034F)
// Hangul ranges
#define HANGUL_CHOSEONG_START (0x1100)
#define HANGUL_CHOSEONG_END (0x115F)
#define HANGUL_JUNGSEONG_START (0x1160)
#define HANGUL_JUNGSEONG_END (0x11A2)
#define HANGUL_JONGSEONG_START (0x11A8)
#define HANGUL_JONGSEONG_END (0x11F9)

#define HANGUL_SYLLABLE_START (0xAC00)
#define HANGUL_SYLLABLE_END (0xD7AF)

RSInline bool _RSStringIsHangulLVT(UTF32Char character) {
    return (((character - HANGUL_SYLLABLE_START) % HANGUL_JONGSEONG_COUNT) ? YES : NO);
}

static uint8_t __RSTranscodingHintLength[] = {
    2, 3, 4, 4, 4, 4, 4, 2, 2, 2, 2, 4, 0, 0, 0, 0
};

enum {
    RSStringHangulStateL,
    RSStringHangulStateV,
    RSStringHangulStateT,
    RSStringHangulStateLV,
    RSStringHangulStateLVT,
    RSStringHangulStateBreak
};

static RSRange _RSStringInlineBufferGetComposedRange(RSStringInlineBuffer *buffer, RSIndex start, RSStringCharacterClusterType type, const uint8_t *bmpBitmap, RSIndex csetType) {
    RSIndex end = start + 1;
    const uint8_t *bitmap = bmpBitmap;
    UTF32Char character;
    UTF16Char otherSurrogate;
    uint8_t step;
    
    character = RSStringGetCharacterFromInlineBuffer(buffer, start);
    
    // We don't combine characters in Armenian ~ Limbu range for backward deletion
    if ((type != RSStringBackwardDeletionCluster) || (character < 0x0530) || (character > 0x194F)) {
        // Check if the current is surrogate
        if (RSUniCharIsSurrogateHighCharacter(character) && RSUniCharIsSurrogateLowCharacter((otherSurrogate = RSStringGetCharacterFromInlineBuffer(buffer, start + 1)))) {
            ++end;
            character = RSUniCharGetLongCharacterForSurrogatePair(character, otherSurrogate);
            bitmap = RSUniCharGetBitmapPtrForPlane((RSBitU32)csetType, (character >> 16));
        }
        
        // Extend backward
        while (start > 0) {
            if ((type == RSStringBackwardDeletionCluster) && (character >= 0x0530) && (character < 0x1950)) break;
            
            if (character < 0x10000) { // the first round could be already be non-BMP
                if (RSUniCharIsSurrogateLowCharacter(character) && RSUniCharIsSurrogateHighCharacter((otherSurrogate = RSStringGetCharacterFromInlineBuffer(buffer, start - 1)))) {
                    character = RSUniCharGetLongCharacterForSurrogatePair(otherSurrogate, character);
                    bitmap = RSUniCharGetBitmapPtrForPlane((RSBitU32)csetType, (character >> 16));
                    if (--start == 0) break; // starting with non-BMP combining mark
                } else {
                    bitmap = bmpBitmap;
                }
            }
            
            if (!RSUniCharIsMemberOfBitmap(character, bitmap) && (character != 0xFF9E) && (character != 0xFF9F) && ((character & 0x1FFFF0) != 0xF870)) break;
            
            --start;
            
            character = RSStringGetCharacterFromInlineBuffer(buffer, start);
        }
    }
    
    // Hangul
    if (((character >= HANGUL_CHOSEONG_START) && (character <= HANGUL_JONGSEONG_END)) || ((character >= HANGUL_SYLLABLE_START) && (character <= HANGUL_SYLLABLE_END))) {
        uint8_t state;
        uint8_t initialState;
        
        if (character < HANGUL_JUNGSEONG_START) {
            state = RSStringHangulStateL;
        } else if (character < HANGUL_JONGSEONG_START) {
            state = RSStringHangulStateV;
        } else if (character < HANGUL_SYLLABLE_START) {
            state = RSStringHangulStateT;
        } else {
            state = (_RSStringIsHangulLVT(character) ? RSStringHangulStateLVT : RSStringHangulStateLV);
        }
        initialState = state;
        
        // Extend backward
        while (((character = RSStringGetCharacterFromInlineBuffer(buffer, start - 1)) >= HANGUL_CHOSEONG_START) && (character <= HANGUL_SYLLABLE_END) && ((character <= HANGUL_JONGSEONG_END) || (character >= HANGUL_SYLLABLE_START))) {
            switch (state) {
                case RSStringHangulStateV:
                    if (character <= HANGUL_CHOSEONG_END) {
                        state = RSStringHangulStateL;
                    } else if ((character >= HANGUL_SYLLABLE_START) && (character <= HANGUL_SYLLABLE_END) && !_RSStringIsHangulLVT(character)) {
                        state = RSStringHangulStateLV;
                    } else if (character > HANGUL_JUNGSEONG_END) {
                        state = RSStringHangulStateBreak;
                    }
                    break;
                    
                case RSStringHangulStateT:
                    if ((character >= HANGUL_JUNGSEONG_START) && (character <= HANGUL_JUNGSEONG_END)) {
                        state = RSStringHangulStateV;
                    } else if ((character >= HANGUL_SYLLABLE_START) && (character <= HANGUL_SYLLABLE_END)) {
                        state = (_RSStringIsHangulLVT(character) ? RSStringHangulStateLVT : RSStringHangulStateLV);
                    } else if (character < HANGUL_JUNGSEONG_START) {
                        state = RSStringHangulStateBreak;
                    }
                    break;
                    
                default:
                    state = ((character < HANGUL_JUNGSEONG_START) ? RSStringHangulStateL : RSStringHangulStateBreak);
                    break;
            }
            
            if (state == RSStringHangulStateBreak) break;
            --start;
        }
        
        // Extend forward
        state = initialState;
        while (((character = RSStringGetCharacterFromInlineBuffer(buffer, end)) > 0) && (((character >= HANGUL_CHOSEONG_START) && (character <= HANGUL_JONGSEONG_END)) || ((character >= HANGUL_SYLLABLE_START) && (character <= HANGUL_SYLLABLE_END)))) {
            switch (state) {
                case RSStringHangulStateLV:
                case RSStringHangulStateV:
                    if ((character >= HANGUL_JUNGSEONG_START) && (character <= HANGUL_JONGSEONG_END)) {
                        state = ((character < HANGUL_JONGSEONG_START) ? RSStringHangulStateV : RSStringHangulStateT);
                    } else {
                        state = RSStringHangulStateBreak;
                    }
                    break;
                    
                case RSStringHangulStateLVT:
                case RSStringHangulStateT:
                    state = (((character >= HANGUL_JONGSEONG_START) && (character <= HANGUL_JONGSEONG_END)) ? RSStringHangulStateT : RSStringHangulStateBreak);
                    break;
                    
                default:
                    if (character < HANGUL_JUNGSEONG_START) {
                        state = RSStringHangulStateL;
                    } else if (character < HANGUL_JONGSEONG_START) {
                        state = RSStringHangulStateV;
                    } else if (character >= HANGUL_SYLLABLE_START) {
                        state = (_RSStringIsHangulLVT(character) ? RSStringHangulStateLVT : RSStringHangulStateLV);
                    } else {
                        state = RSStringHangulStateBreak;
                    }
                    break;
            }
            
            if (state == RSStringHangulStateBreak) break;
            ++end;
        }
    }
    
    // Extend forward
    while ((character = RSStringGetCharacterFromInlineBuffer(buffer, end)) > 0) {
        if ((type == RSStringBackwardDeletionCluster) && (character >= 0x0530) && (character < 0x1950)) break;
        
        if (RSUniCharIsSurrogateHighCharacter(character) && RSUniCharIsSurrogateLowCharacter((otherSurrogate = RSStringGetCharacterFromInlineBuffer(buffer, end + 1)))) {
            character = RSUniCharGetLongCharacterForSurrogatePair(character, otherSurrogate);
            bitmap = RSUniCharGetBitmapPtrForPlane((RSBitU32)csetType, (character >> 16));
            step = 2;
        } else {
            bitmap = bmpBitmap;
            step  = 1;
        }
        
        if (!RSUniCharIsMemberOfBitmap(character, bitmap) && (character != 0xFF9E) && (character != 0xFF9F) && ((character & 0x1FFFF0) != 0xF870)) break;
        
        end += step;
    }
    
    return RSMakeRange(start, end - start);
}

RSInline bool _RSStringIsVirama(UTF32Char character, const uint8_t *combClassBMP) {
    return ((character == COMBINING_GRAPHEME_JOINER) || (RSUniCharGetCombiningPropertyForCharacter(character, (const uint8_t *)((character < 0x10000) ? combClassBMP : RSUniCharGetUnicodePropertyDataForPlane(RSUniCharCombiningProperty, (character >> 16)))) == 9) ? YES : NO);
}

RSRange RSStringGetRangeOfCharacterClusterAtIndex(RSStringRef string, RSIndex charIndex, RSStringCharacterClusterType type) {
    RSRange range;
    RSIndex currentIndex;
    RSIndex length = RSStringGetLength(string);
    RSIndex csetType = ((RSStringGraphemeCluster == type) ? RSUniCharGraphemeExtendCharacterSet : RSUniCharNonBaseCharacterSet);
    RSStringInlineBuffer stringBuffer;
    const uint8_t *bmpBitmap;
    const uint8_t *letterBMP;
    static const uint8_t *combClassBMP = nil;
    UTF32Char character;
    UTF16Char otherSurrogate;
    
    if (charIndex >= length) return RSMakeRange(RSNotFound, 0);
    
    /* Fast case.  If we're eight-bit, it's either the default encoding is cheap or the content is all ASCII.  Watch out when (or if) adding more 8bit Mac-scripts in RSStringEncodingConverters
     */
    if (!RS_IS_OBJC(__RSStringTypeID, string) && __RSStrIsEightBit(string)) return RSMakeRange(charIndex, 1);
    
    bmpBitmap = RSUniCharGetBitmapPtrForPlane((RSBitU32)csetType, 0);
    letterBMP = RSUniCharGetBitmapPtrForPlane(RSUniCharLetterCharacterSet, 0);
    if (nil == combClassBMP) combClassBMP = (const uint8_t *)RSUniCharGetUnicodePropertyDataForPlane(RSUniCharCombiningProperty, 0);
    
    RSStringInitInlineBuffer(string, &stringBuffer, RSMakeRange(0, length));
    
    // Get composed character sequence first
    range = _RSStringInlineBufferGetComposedRange(&stringBuffer, charIndex, type, bmpBitmap, csetType);
    
    // Do grapheme joiners
    if (type < RSStringCursorMovementCluster) {
        const uint8_t *letter = letterBMP;
        
        // Check to see if we have a letter at the beginning of initial cluster
        character = RSStringGetCharacterFromInlineBuffer(&stringBuffer, range.location);
        
        if ((range.length > 1) && RSUniCharIsSurrogateHighCharacter(character) && RSUniCharIsSurrogateLowCharacter((otherSurrogate = RSStringGetCharacterFromInlineBuffer(&stringBuffer, range.location + 1)))) {
            character = RSUniCharGetLongCharacterForSurrogatePair(character, otherSurrogate);
            letter = RSUniCharGetBitmapPtrForPlane(RSUniCharLetterCharacterSet, (character >> 16));
        }
        
        if ((character == ZERO_WIDTH_JOINER) || RSUniCharIsMemberOfBitmap(character, letter)) {
            RSRange otherRange;
            
            // Check if preceded by grapheme joiners (U034F and viramas)
            otherRange.location = currentIndex = range.location;
            
            while (currentIndex > 1) {
                character = RSStringGetCharacterFromInlineBuffer(&stringBuffer, --currentIndex);
                
                // ??? We're assuming viramas only in BMP
                if ((_RSStringIsVirama(character, combClassBMP) || ((character == ZERO_WIDTH_JOINER) && _RSStringIsVirama(RSStringGetCharacterFromInlineBuffer(&stringBuffer, --currentIndex), combClassBMP))) && (currentIndex > 0)) {
                    --currentIndex;
                } else {
                    break;
                }
                
                currentIndex = _RSStringInlineBufferGetComposedRange(&stringBuffer, currentIndex, type, bmpBitmap, csetType).location;
                
                character = RSStringGetCharacterFromInlineBuffer(&stringBuffer, currentIndex);
                
                if (RSUniCharIsSurrogateLowCharacter(character) && RSUniCharIsSurrogateHighCharacter((otherSurrogate = RSStringGetCharacterFromInlineBuffer(&stringBuffer, currentIndex - 1)))) {
                    character = RSUniCharGetLongCharacterForSurrogatePair(character, otherSurrogate);
                    letter = RSUniCharGetBitmapPtrForPlane(RSUniCharLetterCharacterSet, (character >> 16));
                    --currentIndex;
                } else {
                    letter = letterBMP;
                }
                
                if (!RSUniCharIsMemberOfBitmap(character, letter)) break;
                range.location = currentIndex;
            }
            
            range.length += otherRange.location - range.location;
            
            // Check if followed by grapheme joiners
            if ((range.length > 1) && ((range.location + range.length) < length)) {
                otherRange = range;
                currentIndex = otherRange.location + otherRange.length;
                
                do {
                    character = RSStringGetCharacterFromInlineBuffer(&stringBuffer, currentIndex - 1);
                    
                    // ??? We're assuming viramas only in BMP
                    if ((character != ZERO_WIDTH_JOINER) && !_RSStringIsVirama(character, combClassBMP)) break;
                    
                    character = RSStringGetCharacterFromInlineBuffer(&stringBuffer, currentIndex);
                    
                    if (character == ZERO_WIDTH_JOINER) character = RSStringGetCharacterFromInlineBuffer(&stringBuffer, ++currentIndex);
                    
                    if (RSUniCharIsSurrogateHighCharacter(character) && RSUniCharIsSurrogateLowCharacter((otherSurrogate = RSStringGetCharacterFromInlineBuffer(&stringBuffer, currentIndex + 1)))) {
                        character = RSUniCharGetLongCharacterForSurrogatePair(character, otherSurrogate);
                        letter = RSUniCharGetBitmapPtrForPlane(RSUniCharLetterCharacterSet, (character >> 16));
                    } else {
                        letter = letterBMP;
                    }
                    
                    // We only conjoin letters
                    if (!RSUniCharIsMemberOfBitmap(character, letter)) break;
                    otherRange = _RSStringInlineBufferGetComposedRange(&stringBuffer, currentIndex, type, bmpBitmap, csetType);
                    currentIndex = otherRange.location + otherRange.length;
                } while ((otherRange.location + otherRange.length) < length);
                range.length = currentIndex - range.location;
            }
        }
    }
    
    // Check if we're part of prefix transcoding hints
    RSIndex otherIndex;
    
    currentIndex = (range.location + range.length) - (MAX_TRANSCODING_LENGTH + 1);
    if (currentIndex < 0) currentIndex = 0;
    
    while (currentIndex <= range.location) {
        character = RSStringGetCharacterFromInlineBuffer(&stringBuffer, currentIndex);
        
        if ((character & 0x1FFFF0) == 0xF860) { // transcoding hint
            otherIndex = currentIndex + __RSTranscodingHintLength[(character - 0xF860)] + 1;
            if (otherIndex >= (range.location + range.length)) {
                if (otherIndex <= length) {
                    range.location = currentIndex;
                    range.length = otherIndex - currentIndex;
                }
                break;
            }
        }
        ++currentIndex;
    }
    
    return range;
}

RSRange RSStringGetRangeOfComposedCharactersAtIndex(RSStringRef theString, RSIndex theIndex) {
    return RSStringGetRangeOfCharacterClusterAtIndex(theString, theIndex, RSStringComposedCharacterCluster);
}

RSStringRef  RSStringCreateWithPascalString(RSAllocatorRef alloc, ConstStringPtr pStr, RSStringEncoding encoding) {
    if (!pStr) return RSStringGetEmptyString();
    RSIndex len = (RSIndex)(*(uint8_t *)pStr);
    return __RSStringCreateImmutableFunnel3(alloc, pStr, len+1, encoding, NO, NO, YES, NO, NO, ALLOCATORSFREEFUNC, 0);
}


RSExport RSStringRef  RSStringCreateWithCString(RSAllocatorRef alloc, const char *cStr, RSStringEncoding encoding) {
    if (!cStr) return RSStringGetEmptyString();
    RSIndex len = strlen(cStr);
    return __RSStringCreateImmutableFunnel3(alloc, cStr, len, encoding, NO, NO, NO, YES, NO, ALLOCATORSFREEFUNC, 0);
}

RSStringRef  RSStringCreateWithPascalStringNoCopy(RSAllocatorRef alloc, ConstStringPtr pStr, RSStringEncoding encoding, RSAllocatorRef contentsDeallocator) {
    if (!pStr) return RSStringGetEmptyString();
    RSIndex len = (RSIndex)(*(uint8_t *)pStr);
    return __RSStringCreateImmutableFunnel3(alloc, pStr, len+1, encoding, NO, NO, YES, NO, YES, contentsDeallocator, 0);
}


RSStringRef  RSStringCreateWithCStringNoCopy(RSAllocatorRef alloc, const char *cStr, RSStringEncoding encoding, RSAllocatorRef contentsDeallocator) {
    if (!cStr) return RSStringGetEmptyString();
    RSIndex len = strlen(cStr);
    return __RSStringCreateImmutableFunnel3(alloc, cStr, len, encoding, NO, NO, NO, YES, YES, contentsDeallocator, 0);
}


RSStringRef  RSStringCreateWithCharacters(RSAllocatorRef alloc, const UniChar *chars, RSIndex numChars) {
    return __RSStringCreateImmutableFunnel3(alloc, chars, numChars * sizeof(UniChar), RSStringEncodingUnicode, NO, YES, NO, NO, NO, ALLOCATORSFREEFUNC, 0);
}


RSExport RSStringRef  RSStringCreateWithCharactersNoCopy(RSAllocatorRef alloc, const UniChar *chars, RSIndex numChars, RSAllocatorRef contentsDeallocator) {
    return __RSStringCreateImmutableFunnel3(alloc, chars, numChars * sizeof(UniChar), RSStringEncodingUnicode, NO, NO, NO, NO, YES, contentsDeallocator, 0);
}


RSExport RSStringRef  RSStringCreateWithBytes(RSAllocatorRef alloc, const void *bytes, RSIndex numBytes, RSStringEncoding encoding, BOOL externalFormat)
{
    return __RSStringCreateImmutableFunnel3(alloc, bytes, numBytes, encoding, externalFormat, YES, NO, NO, NO, ALLOCATORSFREEFUNC, 0);
}

RSStringRef  _RSStringCreateWithBytesNoCopy(RSAllocatorRef alloc, const void *bytes, RSIndex numBytes, RSStringEncoding encoding, BOOL externalFormat, RSAllocatorRef contentsDeallocator) {
    return __RSStringCreateImmutableFunnel3(alloc, bytes, numBytes, encoding, externalFormat, YES, NO, NO, YES, contentsDeallocator, 0);
}

RSExport RSStringRef  RSStringCreateWithBytesNoCopy(RSAllocatorRef alloc, const void *bytes, RSIndex numBytes, RSStringEncoding encoding, BOOL externalFormat, RSAllocatorRef contentsDeallocator) {
    return __RSStringCreateImmutableFunnel3(alloc, bytes, numBytes, encoding, externalFormat, YES, NO, NO, YES, contentsDeallocator, 0);
}

RSExport RSStringRef  RSStringCreateWithFormatAndArguments(RSAllocatorRef alloc, RSIndex capacity, RSStringRef format, va_list arguments) {
    return _RSStringCreateWithFormatAndArgumentsAux(alloc, nil, capacity, format, arguments);
}

RSExport RSStringRef RSStringCreateWithSubstring(RSAllocatorRef alloc, RSStringRef str, RSRange range)
{
    //      RS_OBJC_FUNCDISPATCHV(__RSStringTypeID, RSStringRef , (NSString *)str, _createSubstringWithRange:NSMakeRange(range.location, range.length));
    
    __RSAssertIsString(str);
    __RSAssertRangeIsInStringBounds(str, range.location, range.length);
    
    if ((range.location == 0) && (range.length == __RSStrLength(str)))
    {
        /* The substring is the whole string... */
        return (RSStringRef)RSStringCreateCopy(alloc, str);
    }
    else if (__RSStrIsEightBit(str))
    {
        const uint8_t *contents = (const uint8_t *)__RSStrContents(str);
        return __RSStringCreateImmutableFunnel3(alloc, contents + range.location + __RSStrSkipAnyLengthByte(str), range.length, __RSStringGetEightBitStringEncoding(), NO, NO, NO, NO, NO, ALLOCATORSFREEFUNC, 0);
    }
    else
    {
        const UniChar *contents = (UniChar *)__RSStrContents(str);
        return __RSStringCreateImmutableFunnel3(alloc, contents + range.location, range.length * sizeof(UniChar), RSStringEncodingUnicode, NO, YES, NO, NO, NO, ALLOCATORSFREEFUNC, 0);
    }
}

// Returns the length of characters filled into outCharacters. If no change, returns 0. maxBufLen shoule be at least 8
static RSIndex __RSStringFoldCharacterClusterAtIndex(UTF32Char character, RSStringInlineBuffer *buffer, RSIndex index, RSOptionFlags flags, const uint8_t *langCode, UTF32Char *outCharacters, RSIndex maxBufferLength, RSIndex *consumedLength) {
    RSIndex filledLength = 0, currentIndex = index;
    
    if (0 != character) {
        UTF16Char lowSurrogate;
        RSIndex planeNo = (character >> 16);
        bool isTurkikCapitalI = NO;
        static const uint8_t *decompBMP = nil;
        static const uint8_t *graphemeBMP = nil;
        
        if (nil == decompBMP) {
            decompBMP = RSUniCharGetBitmapPtrForPlane(RSUniCharCanonicalDecomposableCharacterSet, 0);
            graphemeBMP = RSUniCharGetBitmapPtrForPlane(RSUniCharGraphemeExtendCharacterSet, 0);
        }
        
        ++currentIndex;
        
        if ((character < 0x0080) && ((nil == langCode) || (character != 'I'))) { // ASCII
            if ((flags & RSCompareCaseInsensitive) && (character >= 'A') && (character <= 'Z')) {
                character += ('a' - 'A');
                *outCharacters = character;
                filledLength = 1;
            }
        } else {
            // do width-insensitive mapping
            if ((flags & RSCompareWidthInsensitive) && (character >= 0xFF00) && (character <= 0xFFEF)) {
                (void)RSUniCharCompatibilityDecompose(&character, 1, 1);
                *outCharacters = character;
                filledLength = 1;
            }
            
            // map surrogates
            if ((0 == planeNo) && RSUniCharIsSurrogateHighCharacter(character) && RSUniCharIsSurrogateLowCharacter((lowSurrogate = RSStringGetCharacterFromInlineBuffer(buffer, currentIndex)))) {
                character = RSUniCharGetLongCharacterForSurrogatePair(character, lowSurrogate);
                ++currentIndex;
                planeNo = (character >> 16);
            }
            
            // decompose
            if (flags & (RSCompareDiacriticInsensitive|RSCompareNonliteral)) {
                if (RSUniCharIsMemberOfBitmap(character, ((0 == planeNo) ? decompBMP : RSUniCharGetBitmapPtrForPlane(RSUniCharCanonicalDecomposableCharacterSet, (RSBitU32)planeNo)))) {
                    UTF32Char original = character;
                    
                    filledLength = RSUniCharDecomposeCharacter(character, outCharacters, maxBufferLength);
                    character = *outCharacters;
                    
                    if ((flags & RSCompareDiacriticInsensitive) && (character < 0x0510)) {
                        filledLength = 1; // reset if Roman, Greek, Cyrillic
                    } else if (0 == (flags & RSCompareNonliteral)) {
                        character = original;
                        filledLength = 0;
                    }
                }
            }
            
            // fold case
            if (flags & RSCompareCaseInsensitive) {
                const uint8_t *nonBaseBitmap;
                bool filterNonBase = (((flags & RSCompareDiacriticInsensitive) && (character < 0x0510)) ? YES : NO);
                static const uint8_t *lowerBMP = nil;
                static const uint8_t *caseFoldBMP = nil;
                
                if (nil == lowerBMP) {
                    lowerBMP = RSUniCharGetBitmapPtrForPlane(RSUniCharHasNonSelfLowercaseCharacterSet, 0);
                    caseFoldBMP = RSUniCharGetBitmapPtrForPlane(RSUniCharHasNonSelfCaseFoldingCharacterSet, 0);
                }
                
                if ((nil != langCode) && ('I' == character) && ((0 == strcmp((const char *)langCode, "tr")) || (0 == strcmp((const char *)langCode, "az")))) { // do Turkik special-casing
                    if (filledLength > 1) {
                        if (0x0307 == outCharacters[1]) {
                            if (--filledLength > 1) memmove((outCharacters + 1), (outCharacters + 2), sizeof(UTF32Char) * (filledLength - 1));
                            character = *outCharacters = 'i';
                            isTurkikCapitalI = YES;
                        }
                    } else if (0x0307 == RSStringGetCharacterFromInlineBuffer(buffer, currentIndex)) {
                        character = *outCharacters = 'i';
                        filledLength = 1;
                        ++currentIndex;
                        isTurkikCapitalI = YES;
                    }
                }
                if (!isTurkikCapitalI && (RSUniCharIsMemberOfBitmap(character, ((0 == planeNo) ? lowerBMP : RSUniCharGetBitmapPtrForPlane(RSUniCharHasNonSelfLowercaseCharacterSet, (RSBitU32)planeNo))) || RSUniCharIsMemberOfBitmap(character, ((0 == planeNo) ? caseFoldBMP : RSUniCharGetBitmapPtrForPlane(RSUniCharHasNonSelfCaseFoldingCharacterSet, (RSBitU32)planeNo))))) {
                    UTF16Char caseFoldBuffer[MAX_CASE_MAPPING_BUF];
                    const UTF16Char *bufferP = caseFoldBuffer, *bufferLimit;
                    UTF32Char *outCharactersP = outCharacters;
                    uint32_t bufferLength = (RSBitU32)RSUniCharMapCaseTo(character, caseFoldBuffer, MAX_CASE_MAPPING_BUF, RSUniCharCaseFold, 0, langCode);
                    
                    bufferLimit = bufferP + bufferLength;
                    
                    if (filledLength > 0) --filledLength; // decrement filledLength (will add back later)
                    
                    // make space for casefold characters
                    if ((filledLength > 0) && (bufferLength > 1)) {
                        RSIndex totalScalerLength = 0;
                        
                        while (bufferP < bufferLimit) {
                            if (RSUniCharIsSurrogateHighCharacter(*(bufferP++)) && (bufferP < bufferLimit) && RSUniCharIsSurrogateLowCharacter(*bufferP)) ++bufferP;
                            ++totalScalerLength;
                        }
                        memmove(outCharacters + totalScalerLength, outCharacters + 1, filledLength * sizeof(UTF32Char));
                        bufferP = caseFoldBuffer;
                    }
                    
                    // fill
                    while (bufferP < bufferLimit) {
                        character = *(bufferP++);
                        if (RSUniCharIsSurrogateHighCharacter(character) && (bufferP < bufferLimit) && RSUniCharIsSurrogateLowCharacter(*bufferP)) {
                            character = RSUniCharGetLongCharacterForSurrogatePair(character, *(bufferP++));
                            nonBaseBitmap = RSUniCharGetBitmapPtrForPlane(RSUniCharGraphemeExtendCharacterSet, (character >> 16));
                        } else {
                            nonBaseBitmap = graphemeBMP;
                        }
                        
                        if (!filterNonBase || !RSUniCharIsMemberOfBitmap(character, nonBaseBitmap)) {
                            *(outCharactersP++) = character;
                            ++filledLength;
                        }
                    }
                }
            }
        }
        
        // collect following combining marks
        if (flags & (RSCompareDiacriticInsensitive|RSCompareNonliteral)) {
            const uint8_t *nonBaseBitmap;
            const uint8_t *decompBitmap;
            bool doFill = (((flags & RSCompareDiacriticInsensitive) && (character < 0x0510)) ? NO : YES);
            
            if (0 == filledLength) {
                *outCharacters = character; // filledLength will be updated below on demand
                
                if (doFill) { // check if really needs to fill
                    UTF32Char nonBaseCharacter = RSStringGetCharacterFromInlineBuffer(buffer, currentIndex);
                    
                    if (RSUniCharIsSurrogateHighCharacter(nonBaseCharacter) && RSUniCharIsSurrogateLowCharacter((lowSurrogate = RSStringGetCharacterFromInlineBuffer(buffer, currentIndex + 1)))) {
                        nonBaseCharacter = RSUniCharGetLongCharacterForSurrogatePair(nonBaseCharacter, lowSurrogate);
                        nonBaseBitmap = RSUniCharGetBitmapPtrForPlane(RSUniCharGraphemeExtendCharacterSet, (nonBaseCharacter >> 16));
                        decompBitmap = RSUniCharGetBitmapPtrForPlane(RSUniCharCanonicalDecomposableCharacterSet, (nonBaseCharacter >> 16));
                    } else {
                        nonBaseBitmap = graphemeBMP;
                        decompBitmap = decompBMP;
                    }
                    
                    if (RSUniCharIsMemberOfBitmap(nonBaseCharacter, nonBaseBitmap)) {
                        filledLength = 1; // For the base character
                        
                        if ((0 == (flags & RSCompareDiacriticInsensitive)) || (nonBaseCharacter > 0x050F)) {
                            if (RSUniCharIsMemberOfBitmap(nonBaseCharacter, decompBitmap)) {
                                filledLength += RSUniCharDecomposeCharacter(nonBaseCharacter, &(outCharacters[filledLength]), maxBufferLength - filledLength);
                            } else {
                                outCharacters[filledLength++] = nonBaseCharacter;
                            }
                        }
                        currentIndex += ((nonBaseBitmap == graphemeBMP) ? 1 : 2);
                    } else {
                        doFill = NO;
                    }
                }
            }
            
            while (filledLength < maxBufferLength) { // do the rest
                character = RSStringGetCharacterFromInlineBuffer(buffer, currentIndex);
                
                if (RSUniCharIsSurrogateHighCharacter(character) && RSUniCharIsSurrogateLowCharacter((lowSurrogate = RSStringGetCharacterFromInlineBuffer(buffer, currentIndex + 1)))) {
                    character = RSUniCharGetLongCharacterForSurrogatePair(character, lowSurrogate);
                    nonBaseBitmap = RSUniCharGetBitmapPtrForPlane(RSUniCharGraphemeExtendCharacterSet, (character >> 16));
                    decompBitmap = RSUniCharGetBitmapPtrForPlane(RSUniCharCanonicalDecomposableCharacterSet, (character >> 16));
                } else {
                    nonBaseBitmap = graphemeBMP;
                    decompBitmap = decompBMP;
                }
                if (isTurkikCapitalI) {
                    isTurkikCapitalI = NO;
                } else if (RSUniCharIsMemberOfBitmap(character, nonBaseBitmap)) {
                    if (doFill) {
                        if (RSUniCharIsMemberOfBitmap(character, decompBitmap)) {
                            RSIndex currentLength = RSUniCharDecomposeCharacter(character, &(outCharacters[filledLength]), maxBufferLength - filledLength);
                            
                            if (0 == currentLength) break; // didn't fit
                            
                            filledLength += currentLength;
                        } else {
                            outCharacters[filledLength++] = character;
                        }
                    } else if (0 == filledLength) {
                        filledLength = 1; // For the base character
                    }
                    currentIndex += ((nonBaseBitmap == graphemeBMP) ? 1 : 2);
                } else {
                    break;
                }
            }
            
            if (filledLength > 1) {
                UTF32Char *sortCharactersLimit = outCharacters + filledLength;
                UTF32Char *sortCharacters = sortCharactersLimit - 1;
                
                while ((outCharacters < sortCharacters) && RSUniCharIsMemberOfBitmap(*sortCharacters, ((*sortCharacters < 0x10000) ? graphemeBMP : RSUniCharGetBitmapPtrForPlane(RSUniCharGraphemeExtendCharacterSet, (*sortCharacters >> 16))))) --sortCharacters;
                
                if ((sortCharactersLimit - sortCharacters) > 1) RSUniCharPrioritySort(sortCharacters, (sortCharactersLimit - sortCharacters)); // priority sort
            }
        }
    }
    
    if ((filledLength > 0) && (nil != consumedLength)) *consumedLength = (currentIndex - index);
    
    return filledLength;
}

#define SURROGATE_START 0xD800
#define SURROGATE_END 0xDFFF

RSExport BOOL RSStringFindCharacterFromSet(RSStringRef theString, RSCharacterSetRef theSet, RSRange rangeToSearch, RSStringCompareFlags searchOptions, RSRange *result) {
    RSStringInlineBuffer stringBuffer;
    RSCharacterSetInlineBuffer csetBuffer;
    UniChar ch;
    RSIndex step;
    RSIndex fromLoc, toLoc, cnt;	// fromLoc and toLoc are inclusive
    BOOL found = NO;
    BOOL done = NO;
    
    //#warning FIX ME !! Should support RSCompareNonliteral
    
    if ((rangeToSearch.location + rangeToSearch.length > RSStringGetLength(theString)) || (rangeToSearch.length == 0)) return NO;
    
    if (searchOptions & RSCompareBackwards) {
        fromLoc = rangeToSearch.location + rangeToSearch.length - 1;
        toLoc = rangeToSearch.location;
    } else {
        fromLoc = rangeToSearch.location;
        toLoc = rangeToSearch.location + rangeToSearch.length - 1;
    }
    if (searchOptions & RSCompareAnchored) {
        toLoc = fromLoc;
    }
    
    step = (fromLoc <= toLoc) ? 1 : -1;
    cnt = fromLoc;
    
    RSStringInitInlineBuffer(theString, &stringBuffer, rangeToSearch);
    RSCharacterSetInitInlineBuffer(theSet, &csetBuffer);
    
    do {
        ch = RSStringGetCharacterFromInlineBuffer(&stringBuffer, cnt - rangeToSearch.location);
        if ((ch >= SURROGATE_START) && (ch <= SURROGATE_END)) {
            RSIndex otherCharIndex = cnt + step;
            
            if (((step < 0) && (otherCharIndex < toLoc)) || ((step > 0) && (otherCharIndex > toLoc))) {
                done = YES;
            } else {
                UniChar highChar;
                UniChar lowChar = RSStringGetCharacterFromInlineBuffer(&stringBuffer, otherCharIndex - rangeToSearch.location);
                
                if (cnt < otherCharIndex) {
                    highChar = ch;
                } else {
                    highChar = lowChar;
                    lowChar = ch;
                }
                
                if (RSUniCharIsSurrogateHighCharacter(highChar) && RSUniCharIsSurrogateLowCharacter(lowChar) && RSCharacterSetInlineBufferIsLongCharacterMember(&csetBuffer, RSUniCharGetLongCharacterForSurrogatePair(highChar, lowChar))) {
                    if (result) *result = RSMakeRange((cnt < otherCharIndex ? cnt : otherCharIndex), 2);
                    return YES;
                } else if (otherCharIndex == toLoc) {
                    done = YES;
                } else {
                    cnt = otherCharIndex + step;
                }
            }
        } else if (RSCharacterSetInlineBufferIsLongCharacterMember(&csetBuffer, ch)) {
            done = found = YES;
        } else if (cnt == toLoc) {
            done = YES;
        } else {
            cnt += step;
        }
    } while (!done);
    
    if (found && result) *result = RSMakeRange(cnt, 1);
    return found;
}

#include "unicode/ucol.h"
#include "unicode/ucoleitr.h"

RSInline RSIndex __extendLocationBackward(RSIndex location, RSStringInlineBuffer *str, const uint8_t *nonBaseBMP, const uint8_t *punctBMP) {
    while (location > 0) {
        UTF32Char ch = RSStringGetCharacterFromInlineBuffer(str, location);
        UTF32Char otherChar;
        if (RSUniCharIsSurrogateLowCharacter(ch) && RSUniCharIsSurrogateHighCharacter((otherChar = RSStringGetCharacterFromInlineBuffer(str, location - 1)))) {
            ch = RSUniCharGetLongCharacterForSurrogatePair(ch, otherChar);
            uint8_t planeNo = (ch >> 16);
            if ((planeNo > 1) || (!RSUniCharIsMemberOfBitmap(ch, RSUniCharGetBitmapPtrForPlane(RSUniCharNonBaseCharacterSet, planeNo)) && !RSUniCharIsMemberOfBitmap(ch, RSUniCharGetBitmapPtrForPlane(RSUniCharPunctuationCharacterSet, planeNo)))) break;
            location -= 2;
        } else {
            if ((!RSUniCharIsMemberOfBitmap(ch, nonBaseBMP) && !RSUniCharIsMemberOfBitmap(ch, punctBMP)) || ((ch >= 0x2E80) && (ch < 0xAC00))) break;
            --location;
        }
    }
    
    return location;
}

RSInline RSIndex __extendLocationForward(RSIndex location, RSStringInlineBuffer *str, const uint8_t *alnumBMP, const uint8_t *punctBMP, const uint8_t *controlBMP, RSIndex strMax) {
    do {
        UTF32Char ch = RSStringGetCharacterFromInlineBuffer(str, location);
        UTF32Char otherChar;
        if (RSUniCharIsSurrogateHighCharacter(ch) && RSUniCharIsSurrogateLowCharacter((otherChar = RSStringGetCharacterFromInlineBuffer(str, location + 1)))) {
            ch = RSUniCharGetLongCharacterForSurrogatePair(ch, otherChar);
            location += 2;
            uint8_t planeNo = (ch >> 16);
            if (!RSUniCharIsMemberOfBitmap(ch, RSUniCharGetBitmapPtrForPlane(RSUniCharAlphaNumericCharacterSet, planeNo)) && !RSUniCharIsMemberOfBitmap(ch, RSUniCharGetBitmapPtrForPlane(RSUniCharPunctuationCharacterSet, planeNo)) && !RSUniCharIsMemberOfBitmap(ch, RSUniCharGetBitmapPtrForPlane(RSUniCharControlAndFormatterCharacterSet, planeNo))) break;
        } else {
            ++location;
            if ((!RSUniCharIsMemberOfBitmap(ch, alnumBMP) && !RSUniCharIsMemberOfBitmap(ch, punctBMP) && !RSUniCharIsMemberOfBitmap(ch, controlBMP)) || ((ch >= 0x2E80) && (ch < 0xAC00))) break;
        }
    } while (location < strMax);
    return location;
}

RSExport RSIndex RSStringGetCString(RSStringRef str, char *buffer, RSIndex bufferSize, RSStringEncoding encoding)
{
    const uint8_t *contents;
    RSIndex len;
    
    __RSAssertIsNotNegative(bufferSize);
    if (bufferSize < 1) return NO;
    
    RS_OBJC_FUNCDISPATCHV(__RSStringTypeID, BOOL, (NSString *)str, _getCString:buffer maxLength:(NSUInteger)bufferSize - 1 encoding:encoding);
    
    __RSAssertIsString(str);
    
    contents = (const uint8_t *)__RSStrContents(str);
    len = __RSStrLength2(str, contents);
    
    if (__RSStrIsEightBit(str) && ((__RSStringGetEightBitStringEncoding() == encoding) || (__RSStringGetEightBitStringEncoding() == RSStringEncodingASCII && __RSStringEncodingIsSupersetOfASCII(encoding)))) {	// Requested encoding is equal to the encoding in string
        if (len >= bufferSize) return RSNotFound;
        RSBitU32 sk = __RSStrSkipAnyLengthByte(str);
        memmove(buffer, contents + sk, len);
        buffer[len] = 0;
        return len;
    } else {
        RSIndex usedLen;
        
        if (__RSStringEncodeByteStream(str, 0, len, NO, encoding, NO, (unsigned char*) buffer, bufferSize - 1, &usedLen) == len) {
            buffer[usedLen] = '\0';
            return usedLen;
        } else {
#if defined(DEBUG)
            strlcpy(buffer, CONVERSIONFAILURESTR, bufferSize);
#else
            if (bufferSize > 0) buffer[0] = 0;
#endif
            return RSNotFound;
        }
    }
}

#include "RSLocale.h"
extern RSStringRef __RSLocaleCollatorID;
static UCollator *__RSStringCreateCollator(const void *compareLocale) {
    RSStringRef canonLocaleRSStr = (RSStringRef)RSLocaleGetValue(compareLocale, (RSStringRef)__RSLocaleCollatorID);
    char icuLocaleStr[128] = {0};
    RSStringGetCString(canonLocaleRSStr, icuLocaleStr, sizeof(icuLocaleStr), RSStringEncodingASCII);
    UErrorCode icuStatus = U_ZERO_ERROR;
    UCollator * collator = ucol_open(icuLocaleStr, &icuStatus);
    ucol_setAttribute(collator, UCOL_NORMALIZATION_MODE, UCOL_OFF, &icuStatus);
    ucol_setAttribute(collator, UCOL_ALTERNATE_HANDLING, UCOL_NON_IGNORABLE, &icuStatus);
    ucol_setAttribute(collator, UCOL_STRENGTH, UCOL_PRIMARY, &icuStatus);
    ucol_setAttribute(collator, UCOL_CASE_LEVEL, UCOL_OFF, &icuStatus);
    ucol_setAttribute(collator, UCOL_NUMERIC_COLLATION, UCOL_OFF, &icuStatus);
    return collator;
}



#define RSMaxCachedDefaultCollators (8)
static UCollator *__RSDefaultCollators[RSMaxCachedDefaultCollators];
static RSIndex __RSDefaultCollatorsCount = 0;
static const void *__RSDefaultCollatorLocale = nil;
static RSSpinLock __RSDefaultCollatorLock = RSSpinLockInit;

static UCollator *__RSStringCopyDefaultCollator(RSLocaleRef compareLocale)
{
    RSLocaleRef currentLocale = nil;
    UCollator * collator = nil;
    
    if (compareLocale != __RSDefaultCollatorLocale) {
        currentLocale = RSLocaleCopyCurrent();
        if (compareLocale != currentLocale) {
            RSRelease(currentLocale);
            return nil;
        }
    }
    
    RSSpinLockLock(&__RSDefaultCollatorLock);
    if ((nil != currentLocale) && (__RSDefaultCollatorLocale != currentLocale)) {
        while (__RSDefaultCollatorsCount > 0) ucol_close(__RSDefaultCollators[--__RSDefaultCollatorsCount]);
        __RSDefaultCollatorLocale = RSRetain(currentLocale);
    }
    
    if (__RSDefaultCollatorsCount > 0) collator = __RSDefaultCollators[--__RSDefaultCollatorsCount];
    RSSpinLockUnlock(&__RSDefaultCollatorLock);
    
    if (nil == collator) {
        collator = __RSStringCreateCollator(compareLocale);
    }
    
    if (nil != currentLocale) RSRelease(currentLocale);
    
    return collator;
}

#if DEPLOYMENT_TARGET_MACOSX || DEPLOYMENT_TARGET_EMBEDDED
static void __collatorFinalize(UCollator *collator)
{
    RSLocaleRef locale = _RSGetTSD(__RSTSDKeyCollatorLocale);
    _RSSetTSD(__RSTSDKeyCollatorUCollator, nil, nil);
    _RSSetTSD(__RSTSDKeyCollatorLocale, nil, nil);
    RSSpinLockLock(&__RSDefaultCollatorLock);
    if ((__RSDefaultCollatorLocale == locale) && (__RSDefaultCollatorsCount < RSMaxCachedDefaultCollators)) {
        __RSDefaultCollators[__RSDefaultCollatorsCount++] = collator;
        collator = nil;
    }
    RSSpinLockUnlock(&__RSDefaultCollatorLock);
    if (nil != collator) ucol_close(collator);
    if (locale) RSRelease(locale);
}
#endif


#if defined(RSInline)
RSInline const UniChar *RSStringGetCharactersPtrFromInlineBuffer(RSStringInlineBuffer *buf, RSRange desiredRange) {
    if ((desiredRange.location < 0) || ((desiredRange.location + desiredRange.length) > buf->rangeToBuffer.length)) return NULL;
    
    if (buf->directUniCharBuffer) {
        return buf->directUniCharBuffer + buf->rangeToBuffer.location + desiredRange.location;
    } else {
        if (desiredRange.length > __RSStringInlineBufferLength) return NULL;
        
        if (((desiredRange.location + desiredRange.length) > buf->bufferedRangeEnd) || (desiredRange.location < buf->bufferedRangeStart)) {
            buf->bufferedRangeStart = desiredRange.location;
            buf->bufferedRangeEnd = buf->bufferedRangeStart + __RSStringInlineBufferLength;
            if (buf->bufferedRangeEnd > buf->rangeToBuffer.length) buf->bufferedRangeEnd = buf->rangeToBuffer.length;
            RSIndex location = buf->rangeToBuffer.location + buf->bufferedRangeStart;
            RSIndex length = buf->bufferedRangeEnd - buf->bufferedRangeStart;
            if (buf->directCStringBuffer) {
                UniChar *bufPtr = buf->buffer;
                while (length--) *bufPtr++ = (UniChar)buf->directCStringBuffer[location++];
            } else {
                RSStringGetCharacters(buf->theString, RSMakeRange(location, length), buf->buffer);
            }
        }
        
        return buf->buffer + (desiredRange.location - buf->bufferedRangeStart);
    }
}

RSInline void RSStringGetCharactersFromInlineBuffer(RSStringInlineBuffer *buf, RSRange desiredRange, UniChar *outBuf) {
    if (buf->directUniCharBuffer) {
        memmove(outBuf, buf->directUniCharBuffer + buf->rangeToBuffer.location + desiredRange.location, desiredRange.length * sizeof(UniChar));
    } else {
        if ((desiredRange.location >= buf->bufferedRangeStart) && (desiredRange.location < buf->bufferedRangeEnd)) {
            RSIndex bufLen = desiredRange.length;
            
            if (bufLen > (buf->bufferedRangeEnd - desiredRange.location)) bufLen = (buf->bufferedRangeEnd - desiredRange.location);
            
            memmove(outBuf, buf->buffer + (desiredRange.location - buf->bufferedRangeStart), bufLen * sizeof(UniChar));
            outBuf += bufLen; desiredRange.location += bufLen; desiredRange.length -= bufLen;
        } else {
            RSIndex desiredRangeMax = (desiredRange.location + desiredRange.length);
            
            if ((desiredRangeMax > buf->bufferedRangeStart) && (desiredRangeMax < buf->bufferedRangeEnd)) {
                desiredRange.length = (buf->bufferedRangeStart - desiredRange.location);
                memmove(outBuf + desiredRange.length, buf->buffer, (desiredRangeMax - buf->bufferedRangeStart) * sizeof(UniChar));
            }
        }
        
        if (desiredRange.length > 0) {
            RSIndex location = buf->rangeToBuffer.location + desiredRange.location;
            RSIndex length = desiredRange.length;
            if (buf->directCStringBuffer) {
                UniChar *bufPtr = outBuf;
                while (length--) *bufPtr++ = (UniChar)buf->directCStringBuffer[location++];
            } else {
                RSStringGetCharacters(buf->theString, RSMakeRange(location, length), outBuf);
            }
        }
    }
}

#else
#define RSStringGetCharactersPtrFromInlineBuffer(buf, desiredRange) ((buf)->directUniCharBuffer ? (buf)->directUniCharBuffer + (buf)->rangeToBuffer.location + desiredRange.location : NULL)

#define RSStringGetCharactersFromInlineBuffer(buf, desiredRange, outBuf) \
if (buf->directUniCharBuffer) memmove(outBuf, (buf)->directUniCharBuffer + (buf)->rangeToBuffer.location + desiredRange.location, desiredRange.length * sizeof(UniChar)); \
else RSStringGetCharacters((buf)->theString, RSMakeRange((buf)->rangeToBuffer.location + desiredRange.location, desiredRange.length), outBuf);

#endif /* RSInline */

enum {
	kUpperCaseWeightMin	= 0x80 | 0x0F,
	kUpperCaseWeightMax	= 0x80 | 0x17,
	kUpperToLowerDelta	= 0x80 | 0x0A,	// 0x0A = 0x0F - 0x05
	kMaskPrimarySecondary	= 0xFFFFFF00,
	kMaskPrimaryOnly	= 0xFFFF0000,
	kMaskSecondaryOnly	= 0x0000FF00,
	kMaskCaseTertiary	= 0x000000FF	// 2 hi bits case, 6 lo bits tertiary
};

static SInt32 __CompareSpecials(const UCollator *collator, RSOptionFlags options, const UniChar *text1Ptr, UniCharCount text1Length, const UniChar *text2Ptr, UniCharCount text2Length)
{
	UErrorCode icuStatus = U_ZERO_ERROR;
	SInt32	orderWidth = 0;
	SInt32	orderCompos = 0;
    
	UCollationElements * collElems1 = ucol_openElements(collator, (const UChar *)text1Ptr, (int)text1Length, &icuStatus);
	UCollationElements * collElems2 = ucol_openElements(collator, (const UChar *)text2Ptr, (int)text2Length, &icuStatus);
	if (U_SUCCESS(icuStatus)) {
		int32_t	startOffset1 = 0;
		int32_t	startOffset2 = 0;
		
		while (YES) {
			int32_t	elemOrder1, elemOrder2;
			int32_t	offset1, offset2;
			
			elemOrder1 = ucol_next(collElems1, &icuStatus);
			elemOrder2 = ucol_next(collElems2, &icuStatus);
			if ( U_FAILURE(icuStatus) || elemOrder1 == (int32_t)UCOL_NULLORDER || elemOrder2 == (int32_t)UCOL_NULLORDER ) {
				break;
			}
            
			offset1 = ucol_getOffset(collElems1);
			offset2 = ucol_getOffset(collElems2);
			if ( (elemOrder1 & kMaskPrimarySecondary) == (elemOrder2 & kMaskPrimarySecondary) ) {
				if ( (elemOrder1 & kMaskPrimaryOnly) != 0 ) {
					// keys may differ in case, width, circling, etc.
                    
					int32_t	tertiary1 = (elemOrder1 & kMaskCaseTertiary);
					int32_t tertiary2 = (elemOrder2 & kMaskCaseTertiary);
					// fold upper to lower case
					if (tertiary1 >= kUpperCaseWeightMin && tertiary1 <= kUpperCaseWeightMax) {
						tertiary1 -= kUpperToLowerDelta;
					}
					if (tertiary2 >= kUpperCaseWeightMin && tertiary2 <= kUpperCaseWeightMax) {
						tertiary2 -= kUpperToLowerDelta;
					}
					// now compare
					if (tertiary1 != tertiary2) {
						orderWidth = (tertiary1 < tertiary2)? -1: 1;
						break;
					}
                    
				} else if ( (elemOrder1 & kMaskSecondaryOnly) != 0 ) {
					// primary weights are both zero, but secondaries are not.
					if ( orderCompos == 0 && (options & RSCompareNonliteral) == 0 ) {
						// We have a code element which is a diacritic.
						// It may have come from a composed char or a combining char.
						// If it came from a combining char (longer element length) it sorts first.
						// This is only an approximation to what the Mac OS 9 code did, but this is an
						// unusual case anyway.
						int32_t	elem1Length = offset1 - startOffset1;
						int32_t	elem2Length = offset2 - startOffset2;
						if (elem1Length != elem2Length) {
							orderCompos = (elem1Length > elem2Length)? -1: 1;
						}
					}
				}
			}
			
			startOffset1 = offset1;
			startOffset2 = offset2;
		}
		ucol_closeElements(collElems1);
		ucol_closeElements(collElems2);
	}
	
	return (orderWidth != 0)? orderWidth: orderCompos;
}

static SInt32 __CompareCodePoints(const UniChar *text1Ptr, UniCharCount text1Length, const UniChar *text2Ptr, UniCharCount text2Length ) {
	const UniChar *	text1P = text1Ptr;
	const UniChar *	text2P = text2Ptr;
	UInt32		textLimit = (text1Length <= text2Length)? (UInt32)text1Length: (UInt32)text2Length;
	UInt32		textCounter;
	SInt32		orderResult = 0;
    
	// Loop through either string...the first difference differentiates this.
	for (textCounter = 0; textCounter < textLimit && *text1P == *text2P; textCounter++) {
		text1P++;
		text2P++;
	}
	if (textCounter < textLimit) {
		// code point difference
		orderResult = (*text1P < *text2P) ? -1 : 1;
	} else if (text1Length != text2Length) {
		// one string has extra stuff at end
		orderResult = (text1Length < text2Length) ? -1 : 1;
	}
	return orderResult;
}

static OSStatus __CompareTextDefault(UCollator *collator, RSOptionFlags options, const UniChar *text1Ptr, UniCharCount text1Length, const UniChar *text2Ptr, UniCharCount text2Length, BOOL *equivalentP, SInt32 *orderP) {
    
	// collator must have default settings restored on exit from this function
    
	*equivalentP = YES;
	*orderP = 0;
    
	if (options & RSCompareNumerically) {
	    UErrorCode icuStatus = U_ZERO_ERROR;
	    ucol_setAttribute(collator, UCOL_NUMERIC_COLLATION, UCOL_ON, &icuStatus);
	}
    
	// Most string differences are Primary. Do a primary check first, then if there
	// are no differences do a comparison with the options in the collator.
	UCollationResult icuResult = ucol_strcoll(collator, (const UChar *)text1Ptr, (int)text1Length, (const UChar *)text2Ptr, (int)text2Length);
	if (icuResult != UCOL_EQUAL) {
		*orderP = (icuResult == UCOL_LESS) ? -2 : 2;
	}
	if (*orderP == 0) {
		UErrorCode icuStatus = U_ZERO_ERROR;
        ucol_setAttribute(collator, UCOL_NORMALIZATION_MODE, UCOL_ON, &icuStatus);
        ucol_setAttribute(collator, UCOL_STRENGTH, (options & RSCompareDiacriticInsensitive) ? UCOL_PRIMARY : UCOL_SECONDARY, &icuStatus);
        ucol_setAttribute(collator, UCOL_CASE_LEVEL, (options & RSCompareCaseInsensitive) ? UCOL_OFF : UCOL_ON, &icuStatus);
		if (!U_SUCCESS(icuStatus)) {
		    icuStatus = U_ZERO_ERROR;
		    ucol_setAttribute(collator, UCOL_NORMALIZATION_MODE, UCOL_OFF, &icuStatus);
		    ucol_setAttribute(collator, UCOL_STRENGTH, UCOL_PRIMARY, &icuStatus);
		    ucol_setAttribute(collator, UCOL_CASE_LEVEL, UCOL_OFF, &icuStatus);
		    ucol_setAttribute(collator, UCOL_NUMERIC_COLLATION, UCOL_OFF, &icuStatus);
		    return 666;
		}
        
		// We don't have a primary difference. Recompare with standard collator.
		icuResult = ucol_strcoll(collator, (const UChar *)text1Ptr, (int)text1Length, (const UChar *)text2Ptr, (int)text2Length);
		if (icuResult != UCOL_EQUAL) {
			*orderP = (icuResult == UCOL_LESS) ? -1 : 1;
		}
		icuStatus = U_ZERO_ERROR;
        ucol_setAttribute(collator, UCOL_NORMALIZATION_MODE, UCOL_OFF, &icuStatus);
		ucol_setAttribute(collator, UCOL_STRENGTH, UCOL_PRIMARY, &icuStatus);
		ucol_setAttribute(collator, UCOL_CASE_LEVEL, UCOL_OFF, &icuStatus);
	}
	if (*orderP == 0 && (options & RSCompareNonliteral) == 0) {
		*orderP = __CompareSpecials(collator, options, text1Ptr, text1Length, text2Ptr, text2Length);
	}
    
	*equivalentP = (*orderP == 0);
    
	// If strings are equivalent but we care about order and have not yet checked
	// to the level of code point order, then do some more checks for order
	if (*orderP == 0) {
		UErrorCode icuStatus = U_ZERO_ERROR;
		// First try to see if ICU can find any differences above code point level
        ucol_setAttribute(collator, UCOL_NORMALIZATION_MODE, UCOL_ON, &icuStatus);
		ucol_setAttribute(collator, UCOL_STRENGTH, UCOL_TERTIARY, &icuStatus);
		ucol_setAttribute(collator, UCOL_CASE_LEVEL, UCOL_ON, &icuStatus);
		if (!U_SUCCESS(icuStatus)) {
		    icuStatus = U_ZERO_ERROR;
		    ucol_setAttribute(collator, UCOL_NORMALIZATION_MODE, UCOL_OFF, &icuStatus);
		    ucol_setAttribute(collator, UCOL_STRENGTH, UCOL_PRIMARY, &icuStatus);
		    ucol_setAttribute(collator, UCOL_CASE_LEVEL, UCOL_OFF, &icuStatus);
		    ucol_setAttribute(collator, UCOL_NUMERIC_COLLATION, UCOL_OFF, &icuStatus);
		    return 666;
		}
		icuResult = ucol_strcoll(collator, (const UChar *)text1Ptr, (int)text1Length, (const UChar *)text2Ptr, (int)text2Length);
		if (icuResult != UCOL_EQUAL) {
			*orderP = (icuResult == UCOL_LESS) ? -1 : 1;
		} else {
			// no ICU differences above code point level, compare code points
			*orderP = __CompareCodePoints( text1Ptr, text1Length, text2Ptr, text2Length );
		}
		icuStatus = U_ZERO_ERROR;
        ucol_setAttribute(collator, UCOL_NORMALIZATION_MODE, UCOL_OFF, &icuStatus);
		ucol_setAttribute(collator, UCOL_STRENGTH, UCOL_PRIMARY, &icuStatus);
		ucol_setAttribute(collator, UCOL_CASE_LEVEL, UCOL_OFF, &icuStatus);
	}
    
	if (options & RSCompareNumerically) {
	    UErrorCode icuStatus = U_ZERO_ERROR;
	    ucol_setAttribute(collator, UCOL_NUMERIC_COLLATION, UCOL_OFF, &icuStatus);
	}
	return 0; // noErr
}


#define RSStringCompareAllocationIncrement (128)

RSPrivate RSComparisonResult _RSCompareStringsWithLocale(RSStringInlineBuffer *str1, RSRange str1Range, RSStringInlineBuffer *str2, RSRange str2Range, RSOptionFlags options, const void *compareLocale)
{
    const UniChar *characters1;
    const UniChar *characters2;
    RSComparisonResult compResult = RSCompareEqualTo;
    RSRange range1 = str1Range;
    RSRange range2 = str2Range;
    SInt32 order;
#if DEPLOYMENT_TARGET_MACOSX || DEPLOYMENT_TARGET_EMBEDDED || DEPLOYMENT_TARGET_WINDOWS || DEPLOYMENT_TARGET_LINUX
    BOOL isEqual;
    bool forcedOrdering = ((options & RSCompareForcedOrdering) ? YES : NO);
    
    UCollator *collator = nil;
    bool defaultCollator = YES;
#endif
    static const uint8_t *alnumBMP = nil;
    static const uint8_t *nonBaseBMP = nil;
    static const uint8_t *punctBMP = nil;
    static const uint8_t *controlBMP = nil;
    
    if (nil == alnumBMP) {
        alnumBMP = RSUniCharGetBitmapPtrForPlane(RSUniCharAlphaNumericCharacterSet, 0);
        nonBaseBMP = RSUniCharGetBitmapPtrForPlane(RSUniCharNonBaseCharacterSet, 0);
        punctBMP = RSUniCharGetBitmapPtrForPlane(RSUniCharPunctuationCharacterSet, 0);
        controlBMP = RSUniCharGetBitmapPtrForPlane(RSUniCharControlAndFormatterCharacterSet, 0);
    }
    
    // Determine the range of characters surrodiing the current index significant for localized comparison. The range is extended backward and forward as long as they are contextual. Contextual characters include all letters and punctuations. Since most control/format characters are ignorable in localized comparison, we also include them extending forward.
    
    range1.location = str1Range.location;
    range2.location = str2Range.location;
    
    // go backward
    // The characters upto the current index are already determined to be equal by the RSString's standard character folding algorithm. Extend as long as truly contextual (all letters and punctuations).
    if (range1.location > 0) {
        range1.location = __extendLocationBackward(range1.location - 1, str1, nonBaseBMP, punctBMP);
    }
    
    if (range2.location > 0) {
        range2.location = __extendLocationBackward(range2.location - 1, str2, nonBaseBMP, punctBMP);
    }
    
#if DEPLOYMENT_TARGET_MACOSX || DEPLOYMENT_TARGET_EMBEDDED || DEPLOYMENT_TARGET_WINDOWS || DEPLOYMENT_TARGET_LINUX
#if DEPLOYMENT_TARGET_MACOSX || DEPLOYMENT_TARGET_EMBEDDED
    // First we try to use the last one used on this thread, if the locale is the same,
    // otherwise we try to check out a default one, or then we create one.
    UCollator *threadCollator = _RSGetTSD(__RSTSDKeyCollatorUCollator);
    RSLocaleRef threadLocale = _RSGetTSD(__RSTSDKeyCollatorLocale);
    if (compareLocale == threadLocale) {
        collator = threadCollator;
    } else {
#endif
        collator = __RSStringCopyDefaultCollator(compareLocale);
        defaultCollator = YES;
        if (nil == collator) {
            collator = __RSStringCreateCollator(compareLocale);
            defaultCollator = NO;
        }
#if DEPLOYMENT_TARGET_MACOSX || DEPLOYMENT_TARGET_EMBEDDED
    }
#endif
#endif
    
    characters1 = RSStringGetCharactersPtrFromInlineBuffer(str1, range1);
    characters2 = RSStringGetCharactersPtrFromInlineBuffer(str2, range2);
    
    if ((nil != characters1) && (nil != characters2)) { // do fast
        range1.length = (str1Range.location + str1Range.length) - range1.location;
        range2.length = (str2Range.location + str2Range.length) - range2.location;
        
#if DEPLOYMENT_TARGET_MACOSX || DEPLOYMENT_TARGET_EMBEDDED || DEPLOYMENT_TARGET_WINDOWS || DEPLOYMENT_TARGET_LINUX
        if ((nil != collator) && (__CompareTextDefault(collator, options, characters1, range1.length, characters2, range2.length, &isEqual, &order) == 0 /* noErr */)) {
            compResult = ((isEqual && !forcedOrdering) ? RSCompareEqualTo : ((order < 0) ? RSCompareLessThan : RSCompareGreaterThan));
        } else
#endif
        {
            compResult = ((memcmp(characters1, characters2, sizeof(UniChar) * range1.length) < 0) ? RSCompareLessThan : RSCompareGreaterThan);
        }
    } else {
        UniChar *buffer1 = nil;
        UniChar *buffer2 = nil;
        UTF16Char sBuffer1[RSStringCompareAllocationIncrement];
        UTF16Char sBuffer2[RSStringCompareAllocationIncrement];
        RSIndex buffer1Len = 0, buffer2Len = 0;
        RSIndex str1Max = str1Range.location + str1Range.length;
        RSIndex str2Max = str2Range.location + str2Range.length;
        RSIndex bufferSize;
        
        // Extend forward and compare until the result is deterministic. The result is indeterministic if the differences are weak and can be resolved by character folding. For example, comparision between "abc" and "ABC" is considered to be indeterministic.
        do {
            if (str1Range.location < str1Max) {
                str1Range.location = __extendLocationForward(str1Range.location, str1, alnumBMP, punctBMP, controlBMP, str1Max);
                range1.length = (str1Range.location - range1.location);
                characters1 = RSStringGetCharactersPtrFromInlineBuffer(str1, range1);
                
                if (nil == characters1) {
                    if ((0 > buffer1Len) || (range1.length > RSStringCompareAllocationIncrement)) {
                        if (buffer1Len < range1.length) {
                            bufferSize = range1.length + (RSStringCompareAllocationIncrement - (range1.length % RSStringCompareAllocationIncrement));
                            if (0 == buffer1Len) {
                                buffer1 = (UniChar *)RSAllocatorAllocate(RSAllocatorSystemDefault, sizeof(UTF16Char) * bufferSize);
                            } else if (buffer1Len < range1.length) {
                                buffer1 = (UniChar *)RSAllocatorReallocate(RSAllocatorSystemDefault, buffer1, sizeof(UTF16Char) * bufferSize);
                            }
                            buffer1Len = bufferSize;
                        }
                    } else {
                        buffer1 = sBuffer1;
                    }
                    
                    RSStringGetCharactersFromInlineBuffer(str1, range1, buffer1);
                    characters1 = buffer1;
                }
            }
            
            if (str2Range.location < str2Max) {
                str2Range.location = __extendLocationForward(str2Range.location, str2, alnumBMP, punctBMP, controlBMP, str2Max);
                range2.length = (str2Range.location - range2.location);
                characters2 = RSStringGetCharactersPtrFromInlineBuffer(str2, range2);
                
                if (nil == characters2) {
                    if ((0 > buffer2Len) || (range2.length > RSStringCompareAllocationIncrement)) {
                        if (buffer2Len < range2.length) {
                            bufferSize = range2.length + (RSStringCompareAllocationIncrement - (range2.length % RSStringCompareAllocationIncrement));
                            if (0 == buffer2Len) {
                                buffer2 = (UniChar *)RSAllocatorAllocate(RSAllocatorSystemDefault, sizeof(UTF16Char) * bufferSize);
                            } else if (buffer2Len < range2.length) {
                                buffer2 = (UniChar *)RSAllocatorReallocate(RSAllocatorSystemDefault, buffer2, sizeof(UTF16Char) * bufferSize);
                            }
                            buffer2Len = bufferSize;
                        }
                    } else {
                        buffer2 = sBuffer2;
                    }
                    
                    RSStringGetCharactersFromInlineBuffer(str2, range2, buffer2);
                    characters2 = buffer2;
                }
            }
            
#if DEPLOYMENT_TARGET_MACOSX || DEPLOYMENT_TARGET_EMBEDDED || DEPLOYMENT_TARGET_WINDOWS || DEPLOYMENT_TARGET_LINUX
            if ((nil != collator) && (__CompareTextDefault(collator, options, characters1, range1.length, characters2, range2.length, &isEqual, &order) ==  0 /* noErr */)) {
                if (isEqual) {
                    if (forcedOrdering && (RSCompareEqualTo == compResult) && (0 != order)) compResult = ((order < 0) ? RSCompareLessThan : RSCompareGreaterThan);
                    order = 0;
                }
            } else
#endif
            {
                order = memcmp(characters1, characters2, sizeof(UTF16Char) * ((range1.length < range2.length) ? range1.length : range2.length));
                if (0 == order) {
                    if (range1.length < range2.length) {
                        order = -2;
                    } else if (range2.length < range1.length) {
                        order = 2;
                    }
                } else if (order < 0) {
                    --order;
                } else if (order > 0) {
                    ++order;
                }
            }
            
            if ((order < -1) || (order > 1)) break; // the result is deterministic
            
            if (0 == order) {
                range1.location = str1Range.location;
                range2.location = str2Range.location;
            }
        } while ((str1Range.location < str1Max) || (str2Range.location < str2Max));
        
        if (0 != order) compResult = ((order < 0) ? RSCompareLessThan : RSCompareGreaterThan);
        
        if (buffer1Len > 0) RSAllocatorDeallocate(RSAllocatorSystemDefault, buffer1);
        if (buffer2Len > 0) RSAllocatorDeallocate(RSAllocatorSystemDefault, buffer2);
    }
    
#if DEPLOYMENT_TARGET_MACOSX || DEPLOYMENT_TARGET_EMBEDDED
    if (collator == threadCollator) {
        // do nothing, already cached
    } else {
        if (threadLocale) __collatorFinalize((UCollator *)_RSGetTSD(__RSTSDKeyCollatorUCollator)); // need to dealloc collators
        
        _RSSetTSD(__RSTSDKeyCollatorUCollator, collator, (void *)__collatorFinalize);
        _RSSetTSD(__RSTSDKeyCollatorLocale, (void *)RSRetain(compareLocale), nil);
    }
#endif
    
    return compResult;
}

extern BOOL __RSLocaleGetNullLocale(struct __RSLocale *locale);
extern void __RSLocaleSetNullLocale(struct __RSLocale *locale);

static const char *_RSStrGetLanguageIdentifierForLocale(RSLocaleRef locale, BOOL collatorOnly) {
    RSStringRef localeID;
    const char *langID = NULL;
    static const void *lastLocale = NULL;
    static const char *lastLangID = NULL;
    static RSSpinLock lock = RSSpinLockInit;
    
    if (__RSLocaleGetNullLocale((struct __RSLocale *)locale)) return NULL;
    
    RSSpinLockLock(&lock);
    if ((NULL != lastLocale) && (lastLocale == locale)) {
        RSSpinLockUnlock(&lock);
        return lastLangID;
    }
    RSSpinLockUnlock(&lock);
    
    localeID = (RSStringRef)RSLocaleGetValue(locale, __RSLocaleCollatorID);
    
    if (!collatorOnly) {
        RSIndex length = __RSStrLength(localeID);
        if ((length < 2) || ((4 == length) && RSEqual(localeID, RSSTR("root"))))
            localeID = (RSStringRef)RSLocaleGetIdentifier(locale);
    }
    
    // This is somewhat depending on RSLocale implementation always creating RSString for locale identifer ???
    if (__RSStrLength(localeID) > 1) {
        const void *contents = __RSStrContents(localeID);
        const char *string;
        char buffer[2];
        
        if (__RSStrIsEightBit(localeID)) {
            string = ((const char *)contents) + __RSStrSkipAnyLengthByte(localeID);
        } else {
            const UTF16Char *characters = (const UTF16Char *)contents;
            
            buffer[0] = (char)*(characters++);
            buffer[1] = (char)*characters;
            string = buffer;
        }
        
        if (!strncmp(string, "az", 2)) { // Azerbaijani
            langID = "az";
        } else if (!strncmp(string, "lt", 2)) { // Lithuanian
            langID = "lt";
        } else if (!strncmp(string, "tr", 2)) { // Turkish
            langID = "tr";
        } else if (!strncmp(string, "nl", 2)) { // Dutch
            langID = "nl";
        } else if (!strncmp(string, "el", 2)) { // Greek
            langID = "el";
        }
    }
    
    if (langID == NULL) __RSLocaleSetNullLocale((struct __RSLocale *)locale);
    
    RSSpinLockLock(&lock);
    lastLocale = locale;
    lastLangID = langID;
    RSSpinLockUnlock(&lock);
    
    return langID;
}

static bool __RSStringFillCharacterSetInlineBuffer(RSCharacterSetInlineBuffer *buffer, RSStringCompareFlags compareOptions) {
    if (0 != (compareOptions & RSCompareIgnoreNonAlphanumeric)) {
        static RSCharacterSetRef nonAlnumChars = nil;
        
        if (nil == nonAlnumChars) {
            RSMutableCharacterSetRef cset = RSCharacterSetCreateMutableCopy(RSAllocatorSystemDefault, RSCharacterSetGetPredefined(RSCharacterSetAlphaNumeric));
            RSCharacterSetInvert(cset);
            if (!OSAtomicCompareAndSwapPtrBarrier(nil, cset, (void **)&nonAlnumChars)) RSRelease(cset);
        }
        
        RSCharacterSetInitInlineBuffer(nonAlnumChars, buffer);
        
        return YES;
    }
    
    return NO;
}

RSExport RSComparisonResult RSStringCompare(RSStringRef string, RSStringRef string2, void *context)
{
    __RSGenericValidInstance(string, __RSStringTypeID);
    __RSGenericValidInstance(string2, __RSStringTypeID);
    return RSStringCompareWithOptionsAndLocale(string, string2, RSStringGetRange(string), RSCompareAnchored, nil);
//    return __builtin_memcmp(__RSStrContents(string), __RSStrContents(string2), min(__RSStrLength(string), __RSStrLength(string2)));
}

#define RSStringStackBufferLength (__RSStringInlineBufferLength)
RSExport RSComparisonResult RSStringCompareWithOptionsAndLocale(RSStringRef string, RSStringRef string2, RSRange rangeToCompare, RSStringCompareFlags compareOptions, const void* locale)
{
    /* No objc dispatch needed here since RSStringInlineBuffer works with both RSString and NSString */
    UTF32Char strBuf1[RSStringStackBufferLength];
    UTF32Char strBuf2[RSStringStackBufferLength];
    RSStringInlineBuffer inlineBuf1, inlineBuf2;
    UTF32Char str1Char, str2Char;
    RSIndex str1UsedLen, str2UsedLen;
    RSIndex str1Index = 0, str2Index = 0, strBuf1Index = 0, strBuf2Index = 0, strBuf1Len = 0, strBuf2Len = 0;
    RSIndex str1LocalizedIndex = 0, str2LocalizedIndex = 0;
    RSIndex forcedIndex1 = 0, forcedIndex2 = 0;
    RSIndex str2Len = RSStringGetLength(string2);
    bool caseInsensitive = ((compareOptions & RSCompareCaseInsensitive) ? YES : NO);
    bool diacriticsInsensitive = ((compareOptions & RSCompareDiacriticInsensitive) ? YES : NO);
    bool equalityOptions = ((compareOptions & (RSCompareCaseInsensitive|RSCompareNonliteral|RSCompareDiacriticInsensitive|RSCompareWidthInsensitive)) ? YES : NO);
    bool numerically = ((compareOptions & RSCompareNumerically) ? YES : NO);
    bool forceOrdering = ((compareOptions & RSCompareForcedOrdering) ? YES : NO);
    const uint8_t *graphemeBMP = RSUniCharGetBitmapPtrForPlane(RSUniCharGraphemeExtendCharacterSet, 0);
    const uint8_t *langCode;
    RSComparisonResult compareResult = RSCompareEqualTo;
    UTF16Char otherChar;
    BOOL freeLocale = NO;
    RSCharacterSetInlineBuffer *ignoredChars = nil;
    RSCharacterSetInlineBuffer csetBuffer;
    bool numericEquivalence = NO;
//
    if ((compareOptions & RSCompareLocalized) && (nil == locale)) {
        locale = RSLocaleCopyCurrent();
        freeLocale = YES;
    }
    
    langCode = ((nil == locale) ? nil : (const uint8_t *)_RSStrGetLanguageIdentifierForLocale(locale, YES));
    
    if (__RSStringFillCharacterSetInlineBuffer(&csetBuffer, compareOptions)) {
        ignoredChars = &csetBuffer;
        equalityOptions = YES;
    }
    
    if (!numerically) { // could do binary comp (be careful when adding new flags) (nil == locale) && (nil == ignoredChars) &&
        RSStringEncoding eightBitEncoding = __RSStringGetEightBitStringEncoding();
        const uint8_t *str1Bytes = (const uint8_t *)RSStringGetCStringPtr(string, eightBitEncoding);
        const uint8_t *str2Bytes = (const uint8_t *)RSStringGetCStringPtr(string2, eightBitEncoding);
        RSIndex factor = sizeof(uint8_t);
        
        if ((nil != str1Bytes) && (nil != str2Bytes)) {
            compareOptions &= ~RSCompareNonliteral; // remove non-literal
            
            if ((RSStringEncodingASCII == eightBitEncoding) && (NO == forceOrdering)) {
                if (caseInsensitive) {
                    int cmpResult = strncasecmp_l((const char *)str1Bytes + rangeToCompare.location, (const char *)str2Bytes, __RSMin(rangeToCompare.length, str2Len), nil);
                    
                    if (0 == cmpResult) cmpResult = (int)(rangeToCompare.length - str2Len);
                    
                    return ((0 == cmpResult) ? RSCompareEqualTo : ((cmpResult < 0) ? RSCompareLessThan : RSCompareGreaterThan));
                }
            } else if (caseInsensitive || diacriticsInsensitive) {
                RSIndex limitLength = __RSMin(rangeToCompare.length, str2Len);
                
                str1Bytes += rangeToCompare.location;
                
                while (str1Index < limitLength) {
                    str1Char = str1Bytes[str1Index];
                    str2Char = str2Bytes[str1Index];
                    
                    if (str1Char != str2Char) {
                        if ((str1Char < 0x80) && (str2Char < 0x80)) {
                            if (forceOrdering && (RSCompareEqualTo == compareResult) && (str1Char != str2Char)) compareResult = ((str1Char < str2Char) ? RSCompareLessThan : RSCompareGreaterThan);
                            if (caseInsensitive) {
                                if ((str1Char >= 'A') && (str1Char <= 'Z')) str1Char += ('a' - 'A');
                                if ((str2Char >= 'A') && (str2Char <= 'Z')) str2Char += ('a' - 'A');
                            }
                            
                            if (str1Char != str2Char) return ((str1Char < str2Char) ? RSCompareLessThan : RSCompareGreaterThan);
                        } else {
                            str1Bytes = nil;
                            break;
                        }
                    }
                    ++str1Index;
                }
                
                str2Index = str1Index;
                
                if (str1Index == limitLength) {
                    int cmpResult = (int)(rangeToCompare.length - str2Len);
                    
                    return ((0 == cmpResult) ? compareResult : ((cmpResult < 0) ? RSCompareLessThan : RSCompareGreaterThan));
                }
            }
        } else if (!equalityOptions && (nil == str1Bytes) && (nil == str2Bytes)) {
            str1Bytes = (const uint8_t *)RSStringGetCharactersPtr(string);
            str2Bytes = (const uint8_t *)RSStringGetCharactersPtr(string2);
            factor = sizeof(UTF16Char);
#if __LITTLE_ENDIAN__
            if ((nil != str1Bytes) && (nil != str2Bytes)) { // we cannot use memcmp
                const UTF16Char *str1 = ((const UTF16Char *)str1Bytes) + rangeToCompare.location;
                const UTF16Char *str1Limit = str1 + __RSMin(rangeToCompare.length, str2Len);
                const UTF16Char *str2 = (const UTF16Char *)str2Bytes;
                RSIndex cmpResult = 0;
                
                while ((0 == cmpResult) && (str1 < str1Limit)) cmpResult = (RSIndex)*(str1++) - (RSIndex)*(str2++);
                
                if (0 == cmpResult) cmpResult = rangeToCompare.length - str2Len;
                
                return ((0 == cmpResult) ? RSCompareEqualTo : ((cmpResult < 0) ? RSCompareLessThan : RSCompareGreaterThan));
            }
#endif /* __LITTLE_ENDIAN__ */
        }
        if ((nil != str1Bytes) && (nil != str2Bytes)) {
            int cmpResult = memcmp(str1Bytes + (rangeToCompare.location * factor), str2Bytes, __RSMin(rangeToCompare.length, str2Len) * factor);
            
            if (0 == cmpResult) cmpResult = (int)(rangeToCompare.length - str2Len);
            
            return ((0 == cmpResult) ? RSCompareEqualTo : ((cmpResult < 0) ? RSCompareLessThan : RSCompareGreaterThan));
        }
    }
    
    RSStringInitInlineBuffer(string, &inlineBuf1, rangeToCompare);
    RSStringInitInlineBuffer(string2, &inlineBuf2, RSMakeRange(0, str2Len));
    
    if (nil != locale) {
        str1LocalizedIndex = str1Index;
        str2LocalizedIndex = str2Index;
        
        // We temporarily disable RSCompareDiacriticInsensitive for SL <rdar://problem/6767096>. Should be revisited in NMOS <rdar://problem/7003830>
        if (forceOrdering) {
            diacriticsInsensitive = NO;
            compareOptions &= ~RSCompareDiacriticInsensitive;
        }
    }
    while ((str1Index < rangeToCompare.length) && (str2Index < str2Len)) {
        if (strBuf1Len == 0) {
            str1Char = RSStringGetCharacterFromInlineBuffer(&inlineBuf1, str1Index);
            if (caseInsensitive && (str1Char >= 'A') && (str1Char <= 'Z') && ((nil == langCode) || (str1Char != 'I')) && ((NO == forceOrdering) || (RSCompareEqualTo != compareResult))) str1Char += ('a' - 'A');
            str1UsedLen = 1;
        } else {
            str1Char = strBuf1[strBuf1Index++];
        }
        if (strBuf2Len == 0) {
            str2Char = RSStringGetCharacterFromInlineBuffer(&inlineBuf2, str2Index);
            if (caseInsensitive && (str2Char >= 'A') && (str2Char <= 'Z') && ((nil == langCode) || (str2Char != 'I')) && ((NO == forceOrdering) || (RSCompareEqualTo != compareResult))) str2Char += ('a' - 'A');
            str2UsedLen = 1;
        } else {
            str2Char = strBuf2[strBuf2Index++];
        }
        
        if (numerically && ((0 == strBuf1Len) && (str1Char <= '9') && (str1Char >= '0')) && ((0 == strBuf2Len) && (str2Char <= '9') && (str2Char >= '0'))) { // If both are not ASCII digits, then don't do numerical comparison here
            uint64_t intValue1 = 0, intValue2 = 0;	// !!! Doesn't work if numbers are > max uint64_t
            RSIndex str1NumRangeIndex = str1Index;
            RSIndex str2NumRangeIndex = str2Index;
            
            do {
                intValue1 = (intValue1 * 10) + (str1Char - '0');
                str1Char = RSStringGetCharacterFromInlineBuffer(&inlineBuf1, ++str1Index);
            } while ((str1Char <= '9') && (str1Char >= '0'));
            
            do {
                intValue2 = intValue2 * 10 + (str2Char - '0');
                str2Char = RSStringGetCharacterFromInlineBuffer(&inlineBuf2, ++str2Index);
            } while ((str2Char <= '9') && (str2Char >= '0'));
            
            if (intValue1 == intValue2) {
                if (forceOrdering && (RSCompareEqualTo == compareResult) && ((str1Index - str1NumRangeIndex) != (str2Index - str2NumRangeIndex))) {
                    compareResult = (((str1Index - str1NumRangeIndex) < (str2Index - str2NumRangeIndex)) ? RSCompareLessThan : RSCompareGreaterThan);
                    numericEquivalence = YES;
                    forcedIndex1 = str1NumRangeIndex;
                    forcedIndex2 = str2NumRangeIndex;
                }
                
                continue;
            } else if (intValue1 < intValue2) {
                if (freeLocale && locale) {
                    RSRelease(locale);
                }
                return RSCompareLessThan;
            } else {
                if (freeLocale && locale) {
                    RSRelease(locale);
                }
                return RSCompareGreaterThan;
            }
        }
        
        if (str1Char != str2Char) {
            if (!equalityOptions) {
                compareResult = ((nil == locale) ? ((str1Char < str2Char) ? RSCompareLessThan : RSCompareGreaterThan) : _RSCompareStringsWithLocale(&inlineBuf1, RSMakeRange(str1Index, rangeToCompare.length - str1Index), &inlineBuf2, RSMakeRange(str2Index, str2Len - str2Index), compareOptions, locale));
                if (freeLocale && locale) {
                    RSRelease(locale);
                }
                return compareResult;
            }
            
            if (forceOrdering && (RSCompareEqualTo == compareResult)) {
                compareResult = ((str1Char < str2Char) ? RSCompareLessThan : RSCompareGreaterThan);
                forcedIndex1 = str1LocalizedIndex;
                forcedIndex2 = str2LocalizedIndex;
            }
            
            if ((str1Char < 0x80) && (str2Char < 0x80) && (nil == ignoredChars))
            {
                if (nil != locale)
                {
                    compareResult = _RSCompareStringsWithLocale(&inlineBuf1, RSMakeRange(str1Index, rangeToCompare.length - str1Index), &inlineBuf2, RSMakeRange(str2Index, str2Len - str2Index), compareOptions, locale);
                    if (freeLocale && locale)
                    {
                        RSRelease(locale);
                    }
                    return compareResult;
                }
                else
                {
                    if (!caseInsensitive)
                    {
                        if (freeLocale && locale) {
                            RSRelease(locale);
                        }
                        return ((str1Char < str2Char) ? RSCompareLessThan : RSCompareGreaterThan);
                    }
                }
            }
            
            if (RSUniCharIsSurrogateHighCharacter(str1Char) && RSUniCharIsSurrogateLowCharacter((otherChar = RSStringGetCharacterFromInlineBuffer(&inlineBuf1, str1Index + 1)))) {
                str1Char = RSUniCharGetLongCharacterForSurrogatePair(str1Char, otherChar);
                str1UsedLen = 2;
            }
            
            if (RSUniCharIsSurrogateHighCharacter(str2Char) && RSUniCharIsSurrogateLowCharacter((otherChar = RSStringGetCharacterFromInlineBuffer(&inlineBuf2, str2Index + 1)))) {
                str2Char = RSUniCharGetLongCharacterForSurrogatePair(str2Char, otherChar);
                str2UsedLen = 2;
            }
            
            if (nil != ignoredChars) {
                if (RSCharacterSetInlineBufferIsLongCharacterMember(ignoredChars, str1Char)) {
                    if ((strBuf1Len > 0) && (strBuf1Index == strBuf1Len)) strBuf1Len = 0;
                    if (strBuf1Len == 0) str1Index += str1UsedLen;
                    if (strBuf2Len > 0) --strBuf2Index;
                    continue;
                }
                if (RSCharacterSetInlineBufferIsLongCharacterMember(ignoredChars, str2Char)) {
                    if ((strBuf2Len > 0) && (strBuf2Index == strBuf2Len)) strBuf2Len = 0;
                    if (strBuf2Len == 0) str2Index += str2UsedLen;
                    if (strBuf1Len > 0) -- strBuf1Index;
                    continue;
                }
            }
            
            if (diacriticsInsensitive && (str1Index > 0)) {
                bool str1Skip = NO;
                bool str2Skip = NO;
                
                if ((0 == strBuf1Len) && RSUniCharIsMemberOfBitmap(str1Char, ((str1Char < 0x10000) ? graphemeBMP : RSUniCharGetBitmapPtrForPlane(RSUniCharGraphemeExtendCharacterSet, (str1Char >> 16))))) {
                    str1Char = str2Char;
                    str1Skip = YES;
                }
                if ((0 == strBuf2Len) && RSUniCharIsMemberOfBitmap(str2Char, ((str2Char < 0x10000) ? graphemeBMP : RSUniCharGetBitmapPtrForPlane(RSUniCharGraphemeExtendCharacterSet, (str2Char >> 16))))) {
                    str2Char = str1Char;
                    str2Skip = YES;
                }
                
                if (str1Skip != str2Skip) {
                    if (str1Skip) str2Index -= str2UsedLen;
                    if (str2Skip) str1Index -= str1UsedLen;
                }
            }
            
            if (str1Char != str2Char) {
                if (0 == strBuf1Len) {
                    strBuf1Len = __RSStringFoldCharacterClusterAtIndex(str1Char, &inlineBuf1, str1Index, compareOptions, langCode, strBuf1, RSStringStackBufferLength, &str1UsedLen);
                    if (strBuf1Len > 0) {
                        str1Char = *strBuf1;
                        strBuf1Index = 1;
                    }
                }
                
                if ((0 == strBuf1Len) && (0 < strBuf2Len)) {
                    compareResult =  ((nil == locale) ? ((str1Char < str2Char) ? RSCompareLessThan : RSCompareGreaterThan) : _RSCompareStringsWithLocale(&inlineBuf1, RSMakeRange(str1LocalizedIndex, rangeToCompare.length - str1LocalizedIndex), &inlineBuf2, RSMakeRange(str2LocalizedIndex, str2Len - str2LocalizedIndex), compareOptions, locale));
                    if (freeLocale && locale) {
                        RSRelease(locale);
                    }
                    return compareResult;
                }
                
                if ((0 == strBuf2Len) && ((0 == strBuf1Len) || (str1Char != str2Char))) {
                    strBuf2Len = __RSStringFoldCharacterClusterAtIndex(str2Char, &inlineBuf2, str2Index, compareOptions, langCode, strBuf2, RSStringStackBufferLength, &str2UsedLen);
                    if (strBuf2Len > 0) {
                        str2Char = *strBuf2;
                        strBuf2Index = 1;
                    }
                    if ((0 == strBuf2Len) || (str1Char != str2Char)) {
                        compareResult = ((nil == locale) ? ((str1Char < str2Char) ? RSCompareLessThan : RSCompareGreaterThan) : _RSCompareStringsWithLocale(&inlineBuf1, RSMakeRange(str1LocalizedIndex, rangeToCompare.length - str1LocalizedIndex), &inlineBuf2, RSMakeRange(str2LocalizedIndex, str2Len - str2LocalizedIndex), compareOptions, locale));
                        if (freeLocale && locale) {
                            RSRelease(locale);
                        }
                        return compareResult;
                    }
                }
            }
            
            if ((strBuf1Len > 0) && (strBuf2Len > 0)) {
                while ((strBuf1Index < strBuf1Len) && (strBuf2Index < strBuf2Len)) {
                    if (strBuf1[strBuf1Index] != strBuf2[strBuf2Index]) break;
                    ++strBuf1Index; ++strBuf2Index;
                }
                if ((strBuf1Index < strBuf1Len) && (strBuf2Index < strBuf2Len)) {
                    RSComparisonResult res = ((nil == locale) ? ((strBuf1[strBuf1Index] < strBuf2[strBuf2Index]) ? RSCompareLessThan : RSCompareGreaterThan) : _RSCompareStringsWithLocale(&inlineBuf1, RSMakeRange(str1LocalizedIndex, rangeToCompare.length - str1LocalizedIndex), &inlineBuf2, RSMakeRange(str2LocalizedIndex, str2Len - str2LocalizedIndex), compareOptions, locale));
                    if (freeLocale && locale) {
                        RSRelease(locale);
                    }
                    return res;
                }
            }
        }
        
        if ((strBuf1Len > 0) && (strBuf1Index == strBuf1Len)) strBuf1Len = 0;
        if ((strBuf2Len > 0) && (strBuf2Index == strBuf2Len)) strBuf2Len = 0;
        
        if (strBuf1Len == 0) str1Index += str1UsedLen;
        if (strBuf2Len == 0) str2Index += str2UsedLen;
        if ((strBuf1Len == 0) && (strBuf2Len == 0)) {
            str1LocalizedIndex = str1Index;
            str2LocalizedIndex = str2Index;
        }
    }
    
    if (diacriticsInsensitive || nil != ignoredChars)
    {
        while (str1Index < rangeToCompare.length) {
            str1Char = RSStringGetCharacterFromInlineBuffer(&inlineBuf1, str1Index);
            if ((str1Char < 0x80) && (nil == ignoredChars)) break; // found ASCII
            
            if (RSUniCharIsSurrogateHighCharacter(str1Char) && RSUniCharIsSurrogateLowCharacter((otherChar = RSStringGetCharacterFromInlineBuffer(&inlineBuf1, str1Index + 1)))) str1Char = RSUniCharGetLongCharacterForSurrogatePair(str1Char, otherChar);
            
            if ((!diacriticsInsensitive || !RSUniCharIsMemberOfBitmap(str1Char, ((str1Char < 0x10000) ? graphemeBMP : RSUniCharGetBitmapPtrForPlane(RSUniCharGraphemeExtendCharacterSet, (str1Char >> 16))))) && ((nil == ignoredChars) || !RSCharacterSetInlineBufferIsLongCharacterMember(ignoredChars, str1Char))) break;
            
            str1Index += ((str1Char < 0x10000) ? 1 : 2);
        }
        
        while (str2Index < str2Len) {
            str2Char = RSStringGetCharacterFromInlineBuffer(&inlineBuf2, str2Index);
            if ((str2Char < 0x80) && (nil == ignoredChars)) break; // found ASCII
            
            if (RSUniCharIsSurrogateHighCharacter(str2Char) && RSUniCharIsSurrogateLowCharacter((otherChar = RSStringGetCharacterFromInlineBuffer(&inlineBuf2, str2Index + 1)))) str2Char = RSUniCharGetLongCharacterForSurrogatePair(str2Char, otherChar);
            
            if ((!diacriticsInsensitive || !RSUniCharIsMemberOfBitmap(str2Char, ((str2Char < 0x10000) ? graphemeBMP : RSUniCharGetBitmapPtrForPlane(RSUniCharGraphemeExtendCharacterSet, (str2Char >> 16))))) && ((nil == ignoredChars) || !RSCharacterSetInlineBufferIsLongCharacterMember(ignoredChars, str2Char))) break;
            
            str2Index += ((str2Char < 0x10000) ? 1 : 2);
        }
    }
    // Need to recalc localized result here for forced ordering, ICU cannot do numericEquivalence
    if (!numericEquivalence && (nil != locale) && (RSCompareEqualTo != compareResult) && (str1Index == rangeToCompare.length) && (str2Index == str2Len)) compareResult = _RSCompareStringsWithLocale(&inlineBuf1, RSMakeRange(forcedIndex1, rangeToCompare.length - forcedIndex1), &inlineBuf2, RSMakeRange(forcedIndex2, str2Len - forcedIndex2), compareOptions, locale);
    
    if (freeLocale && locale) {
        RSRelease(locale);
    }
    
    return ((str1Index < rangeToCompare.length) ? RSCompareGreaterThan : ((str2Index < str2Len) ? RSCompareLessThan : compareResult));
}

/* Guts of RSStringGetCharacterAtIndex(); called from the two functions below. Don't call it from elsewhere.
 */
RSInline UniChar __RSStringGetCharacterAtIndexGuts(RSStringRef str, RSIndex idx, const uint8_t *contents) {
    if (__RSStrIsEightBit(str)) {
        contents += __RSStrSkipAnyLengthByte(str);
#if defined(DEBUG)
        if (!__RSCharToUniCharFunc && (contents[idx] >= 128)) {
            // Can't do log here, as it might be too early
            fprintf(stderr, "Warning: RSStringGetCharacterAtIndex() attempted on RSString containing high bytes before properly initialized to do so\n");
        }
#endif
        return __RSCharToUniCharTable[contents[idx]];
    }
    
    return ((UniChar *)contents)[idx];
}


RSExport UniChar RSStringGetCharacterAtIndex(RSStringRef str, RSIndex idx)
{
    RS_OBJC_FUNCDISPATCHV(__RSStringTypeID, UniChar, (NSString *)str, characterAtIndex:(NSUInteger)idx);
    
    __RSAssertIsString(str);
    __RSAssertIndexIsInStringBounds(str, idx);
    return __RSStringGetCharacterAtIndexGuts(str, idx, (const uint8_t *)__RSStrContents(str));
}


static RSMutableDictionaryRef constantStringTable = nil;
static RSSpinLock _RSSTRLock = RSSpinLockInit;

#if RSDictionaryCoreSelector == RSDictionaryBasicHash
static const void* __RSStringCStringRetain(RSAllocatorRef allocator, const void* value)
{
    return value;   // just return itself, it just a c string.
}

static void __RSStringCStringRelease(RSAllocatorRef allocator, const void* value)
{
    return ;        // do nothing because the c string is constant inline string.
}
static BOOL __RSStringCStringEqual(const void *value1, const void *value2)
{
    if (value1 && value2)
    {
        RSCBuffer str1 = value1;
        RSCBuffer str2 = value2;
        int result = strcmp(str1, str2);
        if (result == 0) return YES;
        return NO;
    }
    HALTWithError(RSInvalidArgumentException, "the compare objects have nil object");   // never run here because the __RSStringMakeConstantString return nil if the c string is nil. so, the keys in table are always not nil.
    return RSCompareEqualTo;
}

#else

static const void* __RSStringCStringRetain(const void* value)
{
    return value;   // just return itself, it just a c string.
}

static void __RSStringCStringRelease(const void* value)
{
    return ;        // do nothing because the c string is constant inline string.
}

static RSComparisonResult __RSStringCStringCompare(const void *value1, const void *value2)
{
    if (value1 && value2)
    {
        RSCBuffer str1 = value1;
        RSCBuffer str2 = value2;
        int result = strcmp(str1, str2);
        if (result == 0) return RSCompareEqualTo;
        else if (result < 0) return RSCompareLessThan;
        return RSCompareGreaterThan;
    }
    HALTWithError(RSInvalidArgumentException, "the compare objects have nil object");   // never run here because the __RSStringMakeConstantString return nil if the c string is nil. so, the keys in table are always not nil.
    return RSCompareEqualTo;
}
#endif

static RSHashCode __RSStringCStringHash(const void *value)
{
    return RSStringHashCString(value, strlen(value));
}

static RSStringRef __RSStringCStringCopyDescription(const void *value)
{
    return (RSStringRef)RSStringCreateWithCString(RSAllocatorSystemDefault, value, RSStringEncodingUTF8);
}

static const void* __RSStringCStringCreateCopy(RSAllocatorRef allocator, const void* value)
{
    void* copy = nil;
    if (value) {
        copy = RSAllocatorAllocate(RSAllocatorSystemDefault, strlen(value) + 1);
    }
    return copy;
}

RSPrivate const RSDictionaryKeyContext ___RSConstantCStringKeyContext = {
    __RSStringCStringRetain,
    __RSStringCStringRelease,
    __RSStringCStringCopyDescription,
    __RSStringCStringCreateCopy,
    __RSStringCStringHash,
#if RSDictionaryCoreSelector == RSDictionaryBasicHash
    __RSStringCStringEqual
#else
    __RSStringCStringCompare
#endif
};

static const RSDictionaryValueContext ___RSConstantStringValueContext = {
#if RSDictionaryCoreSelector == RSDictionaryBasicHash
    __RSTypeCollectionRetain,
    __RSTypeCollectionRelease,
#else
    RSRetain,
    RSRelease,
#endif
    RSDescription,
    RSCopy,
    RSHash,
    (RSDictionaryEqualCallBack)RSEqual
};

// the key is always a c string.
// the value is always a RSStringRef.
static const RSDictionaryContext ___RSStringConstantTableContext = {
    0,
    &___RSConstantCStringKeyContext,
    &___RSConstantStringValueContext
};

RSPrivate const RSDictionaryContext* __RSStringConstantTableContext = &___RSStringConstantTableContext;

RSExport RSStringRef __RSStringMakeConstantString(RSCBuffer cStr)
{
    if (nil == cStr) {
        return nil;
    }
    if (nil == constantStringTable) {
        constantStringTable = RSDictionaryCreateMutable(RSAllocatorSystemDefault,2500,__RSStringConstantTableContext);
        __RSRuntimeSetInstanceSpecial(constantStringTable, YES);
        //__RSDictionarySetEqualCallBack(__RSStringConstantTable, __RSStringCStringCompare);
        //        char* key = "%R";
        //        RSStringRef str = RSStringCreateWithCString(RSAllocatorSystemDefault, key);
        //        RSDictionarySetValue(__RSStringConstantTable, key, str);
        //        RSRelease(str);
    }

    RSSpinLockLock(&_RSSTRLock);
    
    RSStringRef string = (RSStringRef)RSDictionaryGetValue(constantStringTable, cStr);
    
    RSSpinLockUnlock(&_RSSTRLock);
    if (string)
    {
        // string return from RSDictionaryGetValue with retain. so we should release it first
        //__RSCLog(RSLogLevelDebug, "{%s} from %s\n",__RSStringContent(string),"return from __RSStringConstantTable");
        //RSRelease(string);
        //__RSCLog(RSLogLevelDebug, "retain count is %ld\n",RSGetRetainCount(string));
        return string;
    }
    //__RSCLog(RSLogLevelDebug, "{%s} create\n",cStr);
#if __RSRuntimeDebugPreference
    if (___RSDebugLogPreference._RSStringNoticeWhenConstantStringAddToTable) {
        __RSCLog(RSLogLevelNotice, "\"%s\" = ", cStr);
    }
#endif
//    char *key;
    BOOL isASCII = YES;
    // Given this code path is rarer these days, OK to do this extra work to verify the strings
    const char *tmp = cStr;
    while (*tmp) {
        if (*(tmp++) & 0x80) {
            isASCII = NO;
            break;
        }
    }
    if (!isASCII) {
//        RSMutableStringRef ms = RSStringCreateMutable(RSAllocatorSystemDefault, 0);
//        tmp = cStr;
//        while (*tmp) {
//            RSStringAppendStringWithFormat(ms, (*tmp & 0x80) ? RSSTR("\\%3o") : RSSTR("%1c"), *tmp);
//            tmp++;
//        }
//        RSLog(RSSTR("WARNING: RSSTR(\"%R\") has non-7 bit chars, interpreting using MacOS Roman encoding for now, but this will change. Please eliminate usages of non-7 bit chars (including escaped characters above \\177 octal) in RSSTR()."), ms);
//        RSRelease(ms);
        string = RSStringCreateWithBytes(RSAllocatorSystemDefault, (const UInt8*)cStr, strlen(cStr), RSStringEncodingUTF8, YES);
    }
    else
    {
        string = RSStringCreateWithCString(RSAllocatorSystemDefault, cStr, __RSDefaultEightBitStringEncoding);
    }
//    if (__RSStrIsEightBit(string)) {
//        key = (char *)__RSStrContents(string) + __RSStrSkipAnyLengthByte(string);
//    } else {	// For some reason the string is not 8-bit!
//        RSIndex keySize = strlen(cStr) + 1;
//        key = (char *)RSAllocatorAllocate(RSAllocatorSystemDefault, keySize);
//        strlcpy(key, cStr, keySize);	// !!! We will leak this, if the string is removed from the table (or table is freed)
//    }
    
    if (string == nil) HALTWithError(RSInvalidArgumentException, "RSRuntime halt with no more memory to use.");
#if __RSRuntimeDebugPreference
    if (___RSDebugLogPreference._RSStringNoticeWhenConstantStringAddToTable) {
        __RSCLog(RSLogLevelNotice, "<%p>\n", string);
    }
#endif
    __RSRuntimeSetInstanceSpecial(string, YES);
    //__RSCLog(RSLogLevelDebug, "retain count is %ld\n",RSGetRetainCount(string));
    RSSpinLockLock(&_RSSTRLock);
    RSDictionarySetValue(constantStringTable, cStr, string);
    RSSpinLockUnlock(&_RSSTRLock);
    
    return string;
}

#if defined(DEBUG)
static BOOL __RSStrIsConstantString(RSStringRef str)
{
    RSTypeRef found = nil;
    if (constantStringTable)
    {
        void *ptr = (void *)RSStringGetCharactersPtr(str);
        if (!ptr) ptr = (void *)RSStringGetCStringPtr(str, RSStringEncodingUTF8);
        if (!ptr) ptr = (void *)RSStringGetCStringPtr(str, RSStringEncodingASCII);
        if (!ptr) ptr = (void *)RSStringGetCStringPtr(str, RSStringEncodingUnicode);
        if (!ptr) ptr = (void *)RSStringGetCStringPtr(str, __RSDefaultEightBitStringEncoding);
        RSSpinLockLock(&_RSSTRLock);
        found = RSDictionaryGetValue(constantStringTable, ptr);
        RSSpinLockUnlock(&_RSSTRLock);
    }
    return found ? YES : NO;
}
#endif

RSExport void RSStringCacheRelease()
{
//    RSDictionaryApplyBlock(constantStringTable, ^(const void *key, const void *value, BOOL *stop) {
//        __RSCLog(RSLogLevelWarning, "%s <%p>\n", RSStringGetCStringPtr(value, RSStringEncodingMacRoman), value);
//    });
    if(constantStringTable == nil) return;
    RSSpinLockLock(&_RSSTRLock);
#if defined(DEBUG)
    __RSConstantStringTableBeingFreed = YES;
#endif
    extern void __RSDictionaryCleanAllObjects(RSMutableDictionaryRef dictionary);
    
    __RSDictionaryCleanAllObjects(constantStringTable);
    __RSRuntimeSetInstanceSpecial(constantStringTable, NO);
    RSRelease(constantStringTable);
    constantStringTable = nil;
#if defined(DEBUG)
    __RSConstantStringTableBeingFreed = NO;
#endif
    RSSpinLockUnlock(&_RSSTRLock);
}

BOOL RSStringFindWithOptionsAndLocale(RSStringRef string, RSStringRef stringToFind, RSRange rangeToSearch, RSStringCompareFlags compareOptions, RSLocaleRef locale, RSRange *result)
{
    /* No objc dispatch needed here since RSStringInlineBuffer works with both RSString and NSString */
    RSIndex findStrLen = RSStringGetLength(stringToFind);
    BOOL didFind = NO;
    bool lengthVariants = ((compareOptions & (RSCompareCaseInsensitive|RSCompareNonliteral|RSCompareDiacriticInsensitive)) ? YES : NO);
    RSCharacterSetInlineBuffer *ignoredChars = nil;
    RSCharacterSetInlineBuffer csetBuffer;
    
    if (__RSStringFillCharacterSetInlineBuffer(&csetBuffer, compareOptions))
    {
        ignoredChars = &csetBuffer;
        lengthVariants = YES;
    }
    
    if ((findStrLen > 0) && (rangeToSearch.length > 0) && ((findStrLen <= rangeToSearch.length) || lengthVariants))
    {
        UTF32Char strBuf1[RSStringStackBufferLength];
        UTF32Char strBuf2[RSStringStackBufferLength];
        RSStringInlineBuffer inlineBuf1, inlineBuf2;
        UTF32Char str1Char = 0, str2Char = 0;
        RSStringEncoding eightBitEncoding = __RSStringGetEightBitStringEncoding();
        const uint8_t *str1Bytes = (const uint8_t *)RSStringGetCStringPtr(string, eightBitEncoding);
        const uint8_t *str2Bytes = (const uint8_t *)RSStringGetCStringPtr(stringToFind, eightBitEncoding);
        const UTF32Char *characters, *charactersLimit;
        const uint8_t *langCode = nil;
        RSIndex fromLoc, toLoc;
        RSIndex str1Index, str2Index;
        RSIndex strBuf1Len, strBuf2Len;
        RSIndex maxStr1Index = (rangeToSearch.location + rangeToSearch.length);
        bool equalityOptions = ((lengthVariants || (compareOptions & RSCompareWidthInsensitive)) ? YES : NO);
        bool caseInsensitive = ((compareOptions & RSCompareCaseInsensitive) ? YES : NO);
        bool forwardAnchor = ((RSCompareAnchored == (compareOptions & (RSCompareBackwards|RSCompareAnchored))) ? YES : NO);
        bool backwardAnchor = (((RSCompareBackwards|RSCompareAnchored) == (compareOptions & (RSCompareBackwards|RSCompareAnchored))) ? YES : NO);
        int8_t delta;
        
        if (nil == locale)
        {
            if (compareOptions & RSCompareLocalized)
            {
                RSLocaleRef currentLocale = RSLocaleCopyCurrent();
                langCode = (const uint8_t *)_RSStrGetLanguageIdentifierForLocale(currentLocale, YES);
                RSRelease(currentLocale);
            }
        }
        else
        {
            langCode = (const uint8_t *)_RSStrGetLanguageIdentifierForLocale(locale, YES);
        }
        
        RSStringInitInlineBuffer(string, &inlineBuf1, RSMakeRange(0, rangeToSearch.location + rangeToSearch.length));
        RSStringInitInlineBuffer(stringToFind, &inlineBuf2, RSMakeRange(0, findStrLen));
        
        if (compareOptions & RSCompareBackwards)
        {
            fromLoc = rangeToSearch.location + rangeToSearch.length - (lengthVariants ? 1 : findStrLen);
            toLoc = (((compareOptions & RSCompareAnchored) && !lengthVariants) ? fromLoc : rangeToSearch.location);
        }
        else
        {
            fromLoc = rangeToSearch.location;
            toLoc = ((compareOptions & RSCompareAnchored) ? fromLoc : rangeToSearch.location + rangeToSearch.length - (lengthVariants ? 1 : findStrLen));
        }
        
        delta = ((fromLoc <= toLoc) ? 1 : -1);
        
        if ((nil != str1Bytes) && (nil != str2Bytes))
        {
            uint8_t str1Byte, str2Byte;
            while (1)
            {
                str1Index = fromLoc;
                str2Index = 0;
                while ((str1Index < maxStr1Index) && (str2Index < findStrLen))
                {
                    str1Byte = str1Bytes[str1Index];
                    str2Byte = str2Bytes[str2Index];
                    if (str1Byte != str2Byte)
                    {
                        if (equalityOptions)
                        {
                            if ((str1Byte < 0x80) && ((nil == langCode) || ('I' != str1Byte)))
                            {
                                if (caseInsensitive && (str1Byte >= 'A') && (str1Byte <= 'Z'))
                                    str1Byte += ('a' - 'A');
                                *strBuf1 = str1Byte;
                                strBuf1Len = 1;
                            }
                            else
                            {
                                str1Char = RSStringGetCharacterFromInlineBuffer(&inlineBuf1, str1Index);
                                strBuf1Len = __RSStringFoldCharacterClusterAtIndex(str1Char, &inlineBuf1, str1Index, compareOptions, langCode, strBuf1, RSStringStackBufferLength, nil);
                                if (1 > strBuf1Len)
                                {
                                    *strBuf1 = str1Char;
                                    strBuf1Len = 1;
                                }
                            }
                            
                            if ((nil != ignoredChars) &&
                                (forwardAnchor || (str1Index != fromLoc)) &&
                                RSCharacterSetInlineBufferIsLongCharacterMember(ignoredChars,
                                                                                ((str1Byte < 0x80) ? str1Byte : str1Char)))
                            {
                                ++str1Index;
                                continue;
                            }
                            
                            if ((str2Byte < 0x80) &&
                                ((nil == langCode) || ('I' != str2Byte)))
                            {
                                if (caseInsensitive && (str2Byte >= 'A') && (str2Byte <= 'Z'))
                                    str2Byte += ('a' - 'A');
                                *strBuf2 = str2Byte;
                                strBuf2Len = 1;
                            }
                            else
                            {
                                str2Char = RSStringGetCharacterFromInlineBuffer(&inlineBuf2, str2Index);
                                strBuf2Len = __RSStringFoldCharacterClusterAtIndex(str2Char, &inlineBuf2, str2Index, compareOptions, langCode, strBuf2, RSStringStackBufferLength, nil);
                                if (1 > strBuf2Len)
                                {
                                    *strBuf2 = str2Char;
                                    strBuf2Len = 1;
                                }
                            }
                            
                            if ((nil != ignoredChars) &&
                                RSCharacterSetInlineBufferIsLongCharacterMember(ignoredChars,
                                                                                ((str2Byte < 0x80) ? str2Byte : str2Char)))
                            {
                                ++str2Index;
                                continue;
                            }
                            if ((1 == strBuf1Len) && (1 == strBuf2Len))
                            { // normal case
                                if (*strBuf1 != *strBuf2)
                                    break;
                            }
                            else
                            {
                                RSIndex delta;
                                
                                if (!caseInsensitive && (strBuf1Len != strBuf2Len))
                                    break;
                                if (memcmp(strBuf1, strBuf2, sizeof(UTF32Char) * __RSMin(strBuf1Len, strBuf2Len)))
                                    break;
                                if (strBuf1Len < strBuf2Len)
                                {
                                    delta = strBuf2Len - strBuf1Len;
                                    if ((str1Index + strBuf1Len + delta) > maxStr1Index)
                                        break;
                                    characters = &(strBuf2[strBuf1Len]);
                                    charactersLimit = characters + delta;
                                    while (characters < charactersLimit)
                                    {
                                        strBuf1Len = __RSStringFoldCharacterClusterAtIndex(RSStringGetCharacterFromInlineBuffer(&inlineBuf1, str1Index + 1),
                                                                                           &inlineBuf1,
                                                                                           str1Index + 1,
                                                                                           compareOptions,
                                                                                           langCode,
                                                                                           strBuf1,
                                                                                           RSStringStackBufferLength,
                                                                                           nil);
                                        if ((strBuf1Len > 0) || (*characters != *strBuf1))
                                            break;
                                        ++characters;
                                        ++str1Index;
                                    }
                                    if (characters < charactersLimit)
                                        break;
                                }
                                else if (strBuf2Len < strBuf1Len)
                                {
                                    delta = strBuf1Len - strBuf2Len;
                                    if ((str2Index + strBuf2Len + delta) > findStrLen)
                                        break;
                                    characters = &(strBuf1[strBuf2Len]);
                                    charactersLimit = characters + delta;
                                    while (characters < charactersLimit)
                                    {
                                        strBuf2Len = __RSStringFoldCharacterClusterAtIndex(RSStringGetCharacterFromInlineBuffer(&inlineBuf2, str1Index + 1),
                                                                                           &inlineBuf2,
                                                                                           str2Index + 1,
                                                                                           compareOptions,
                                                                                           langCode,
                                                                                           strBuf2,
                                                                                           RSStringStackBufferLength,
                                                                                           nil);
                                        if ((strBuf2Len > 0) || (*characters != *strBuf2))
                                            break;
                                        ++characters;
                                        ++str2Index;
                                    }
                                    if (characters < charactersLimit)
                                        break;
                                }
                            }
                        }
                        else
                        {
                            break;
                        }
                    }
                    ++str1Index;
                    ++str2Index;
                }
                if ((nil != ignoredChars) &&
                    (str1Index == maxStr1Index) &&
                    (str2Index < findStrLen))
                {
                    // Process the stringToFind tail
                    while (str2Index < findStrLen)
                    {
                        str2Char = RSStringGetCharacterFromInlineBuffer(&inlineBuf2, str2Index);
                        
                        if (!RSCharacterSetInlineBufferIsLongCharacterMember(ignoredChars, str2Char))
                            break;
                        ++str2Index;
                    }
                }
                
                if (str2Index == findStrLen)
                {
                    if ((nil != ignoredChars) &&
                        backwardAnchor &&
                        (str1Index < maxStr1Index))
                    {
                        // Process the anchor tail
                        while (str1Index < maxStr1Index)
                        {
                            str1Char = RSStringGetCharacterFromInlineBuffer(&inlineBuf1, str1Index);
                            if (!RSCharacterSetInlineBufferIsLongCharacterMember(ignoredChars, str1Char))
                                break;
                            ++str1Index;
                        }
                    }
                    if (!backwardAnchor || (str1Index == maxStr1Index))
                    {
                        didFind = YES;
                        if (nil != result)
                            *result = RSMakeRange(fromLoc, str1Index - fromLoc);
                    }
                    break;
                }
                if (fromLoc == toLoc)
                    break;
                fromLoc += delta;
            }
        }
        else if (equalityOptions)
        {
            UTF16Char otherChar;
            RSIndex str1UsedLen, str2UsedLen, strBuf1Index = 0, strBuf2Index = 0;
            bool diacriticsInsensitive = ((compareOptions & RSCompareDiacriticInsensitive) ? YES : NO);
            const uint8_t *graphemeBMP = RSUniCharGetBitmapPtrForPlane(RSUniCharGraphemeExtendCharacterSet, 0);
            const uint8_t *combClassBMP = (const uint8_t *)RSUniCharGetUnicodePropertyDataForPlane(RSUniCharCombiningProperty, 0);
            
            while (1)
            {
                str1Index = fromLoc;
                str2Index = 0;
                strBuf1Len = strBuf2Len = 0;
                while (str2Index < findStrLen)
                {
                    if (strBuf1Len == 0)
                    {
                        str1Char = RSStringGetCharacterFromInlineBuffer(&inlineBuf1, str1Index);
                        if (caseInsensitive && (str1Char >= 'A') && (str1Char <= 'Z') && ((nil == langCode) || (str1Char != 'I'))) str1Char += ('a' - 'A');
                        str1UsedLen = 1;
                    }
                    else
                    {
                        str1Char = strBuf1[strBuf1Index++];
                    }
                    if (strBuf2Len == 0)
                    {
                        str2Char = RSStringGetCharacterFromInlineBuffer(&inlineBuf2, str2Index);
                        if (caseInsensitive && (str2Char >= 'A') && (str2Char <= 'Z') && ((nil == langCode) || (str2Char != 'I'))) str2Char += ('a' - 'A');
                        str2UsedLen = 1;
                    }
                    else
                    {
                        str2Char = strBuf2[strBuf2Index++];
                    }
                    if (str1Char != str2Char)
                    {
                        if ((str1Char < 0x80) &&
                            (str2Char < 0x80) &&
                            (nil == ignoredChars) &&
                            ((nil == langCode) || !caseInsensitive))
                            break;
                        
                        if (RSUniCharIsSurrogateHighCharacter(str1Char) &&
                            RSUniCharIsSurrogateLowCharacter((otherChar = RSStringGetCharacterFromInlineBuffer(&inlineBuf1, str1Index + 1))))
                        {
                            str1Char = RSUniCharGetLongCharacterForSurrogatePair(str1Char, otherChar);
                            str1UsedLen = 2;
                        }
                        
                        if (RSUniCharIsSurrogateHighCharacter(str2Char) &&
                            RSUniCharIsSurrogateLowCharacter((otherChar = RSStringGetCharacterFromInlineBuffer(&inlineBuf2, str2Index + 1))))
                        {
                            str2Char = RSUniCharGetLongCharacterForSurrogatePair(str2Char, otherChar);
                            str2UsedLen = 2;
                        }
                        if (nil != ignoredChars)
                        {
                            if ((forwardAnchor ||
                                 (str1Index != fromLoc)) &&
                                (str1Index < maxStr1Index) &&
                                RSCharacterSetInlineBufferIsLongCharacterMember(ignoredChars, str1Char))
                            {
                                if ((strBuf1Len > 0) && (strBuf1Index == strBuf1Len))
                                    strBuf1Len = 0;
                                if (strBuf1Len == 0)
                                    str1Index += str1UsedLen;
                                if (strBuf2Len > 0)
                                    --strBuf2Index;
                                continue;
                            }
                            if (RSCharacterSetInlineBufferIsLongCharacterMember(ignoredChars, str2Char))
                            {
                                if ((strBuf2Len > 0) && (strBuf2Index == strBuf2Len))
                                    strBuf2Len = 0;
                                if (strBuf2Len == 0)
                                    str2Index += str2UsedLen;
                                if (strBuf1Len > 0)
                                    -- strBuf1Index;
                                continue;
                            }
                        }
                        if (diacriticsInsensitive && (str1Index > fromLoc))
                        {
                            bool str1Skip = NO;
                            bool str2Skip = NO;
                            if ((0 == strBuf1Len) &&
                                RSUniCharIsMemberOfBitmap(str1Char, ((str1Char < 0x10000) ? graphemeBMP : RSUniCharGetBitmapPtrForPlane(RSUniCharGraphemeExtendCharacterSet, (str1Char >> 16)))))
                            {
                                str1Char = str2Char;
                                str1Skip = YES;
                            }
                            if ((0 == strBuf2Len) && RSUniCharIsMemberOfBitmap(str2Char, ((str2Char < 0x10000) ? graphemeBMP : RSUniCharGetBitmapPtrForPlane(RSUniCharGraphemeExtendCharacterSet, (str2Char >> 16)))))
                            {
                                str2Char = str1Char;
                                str2Skip = YES;
                            }
                            if (str1Skip != str2Skip)
                            {
                                if (str1Skip) str2Index -= str2UsedLen;
                                if (str2Skip) str1Index -= str1UsedLen;
                            }
                        }
                        if (str1Char != str2Char)
                        {
                            if (0 == strBuf1Len)
                            {
                                strBuf1Len = __RSStringFoldCharacterClusterAtIndex(str1Char,
                                                                                   &inlineBuf1,
                                                                                   str1Index,
                                                                                   compareOptions,
                                                                                   langCode,
                                                                                   strBuf1,
                                                                                   RSStringStackBufferLength,
                                                                                   &str1UsedLen);
                                if (strBuf1Len > 0)
                                {
                                    str1Char = *strBuf1;
                                    strBuf1Index = 1;
                                }
                            }
                            
                            if ((0 == strBuf1Len) && (0 < strBuf2Len))
                                break;
                            
                            if ((0 == strBuf2Len) && ((0 == strBuf1Len) || (str1Char != str2Char)))
                            {
                                strBuf2Len = __RSStringFoldCharacterClusterAtIndex(str2Char, &inlineBuf2, str2Index, compareOptions, langCode, strBuf2, RSStringStackBufferLength, &str2UsedLen);
                                if ((0 == strBuf2Len) || (str1Char != *strBuf2))
                                    break;
                                strBuf2Index = 1;
                            }
                        }
                        
                        if ((strBuf1Len > 0) && (strBuf2Len > 0))
                        {
                            while ((strBuf1Index < strBuf1Len) && (strBuf2Index < strBuf2Len))
                            {
                                if (strBuf1[strBuf1Index] != strBuf2[strBuf2Index])
                                    break;
                                ++strBuf1Index; ++strBuf2Index;
                            }
                            if ((strBuf1Index < strBuf1Len) && (strBuf2Index < strBuf2Len))
                                break;
                        }
                    }
                    
                    if ((strBuf1Len > 0) && (strBuf1Index == strBuf1Len)) strBuf1Len = 0;
                    if ((strBuf2Len > 0) && (strBuf2Index == strBuf2Len)) strBuf2Len = 0;
                    
                    if (strBuf1Len == 0) str1Index += str1UsedLen;
                    if (strBuf2Len == 0) str2Index += str2UsedLen;
                }
                
                if ((nil != ignoredChars) && (str1Index == maxStr1Index) && (str2Index < findStrLen))
                {
                    // Process the stringToFind tail
                    while (str2Index < findStrLen)
                    {
                        str2Char = RSStringGetCharacterFromInlineBuffer(&inlineBuf2, str2Index);
                        if (RSUniCharIsSurrogateHighCharacter(str2Char) &&
                            RSUniCharIsSurrogateLowCharacter((otherChar = RSStringGetCharacterFromInlineBuffer(&inlineBuf2, str2Index + 1))))
                        {
                            str2Char = RSUniCharGetLongCharacterForSurrogatePair(str2Char, otherChar);
                        }
                        if (!RSCharacterSetInlineBufferIsLongCharacterMember(ignoredChars, str2Char))
                            break;
                        str2Index += ((str2Char < 0x10000) ? 1 : 2);
                    }
                }
                
                if (str2Index == findStrLen)
                {
                    bool match = YES;
                    if (strBuf1Len > 0)
                    {
                        match = NO;
                        
                        if (diacriticsInsensitive && (strBuf1[0] < 0x0510))
                        {
                            while (strBuf1Index < strBuf1Len)
                            {
                                if (!RSUniCharIsMemberOfBitmap(strBuf1[strBuf1Index], ((strBuf1[strBuf1Index] < 0x10000) ? graphemeBMP : RSUniCharGetBitmapPtrForPlane(RSUniCharCanonicalDecomposableCharacterSet, (strBuf1[strBuf1Index] >> 16)))))
                                    break;
                                ++strBuf1Index;
                            }
                            if (strBuf1Index == strBuf1Len)
                            {
                                str1Index += str1UsedLen;
                                match = YES;
                            }
                        }
                    }
                    
                    if (match && (compareOptions & (RSCompareDiacriticInsensitive|RSCompareNonliteral)) && (str1Index < maxStr1Index))
                    {
                        const uint8_t *nonBaseBitmap;
                        str1Char = RSStringGetCharacterFromInlineBuffer(&inlineBuf1, str1Index);
                        if (RSUniCharIsSurrogateHighCharacter(str1Char) &&
                            RSUniCharIsSurrogateLowCharacter((otherChar = RSStringGetCharacterFromInlineBuffer(&inlineBuf1, str1Index + 1))))
                        {
                            str1Char = RSUniCharGetLongCharacterForSurrogatePair(str1Char, otherChar);
                            nonBaseBitmap = RSUniCharGetBitmapPtrForPlane(RSUniCharGraphemeExtendCharacterSet, (str1Char >> 16));
                        }
                        else
                        {
                            nonBaseBitmap = graphemeBMP;
                        }
                        if (RSUniCharIsMemberOfBitmap(str1Char, nonBaseBitmap))
                        {
                            if (diacriticsInsensitive)
                            {
                                if (str1Char < 0x10000)
                                {
                                    RSIndex index = str1Index;
                                    do
                                    {
                                        str1Char = RSStringGetCharacterFromInlineBuffer(&inlineBuf1, --index);
                                    }
                                    while (RSUniCharIsMemberOfBitmap(str1Char, graphemeBMP), (rangeToSearch.location < index));
                                    
                                    if (str1Char < 0x0510)
                                    {
                                        while (++str1Index < maxStr1Index)
                                            if (!RSUniCharIsMemberOfBitmap(RSStringGetCharacterFromInlineBuffer(&inlineBuf1, str1Index), graphemeBMP))
                                                break;
                                    }
                                }
                            }
                            else
                            {
                                match = NO;
                            }
                        }
                        else if (!diacriticsInsensitive)
                        {
                            otherChar = RSStringGetCharacterFromInlineBuffer(&inlineBuf1, str1Index - 1);
                            
                            // this is assuming viramas are only in BMP ???
                            if ((str1Char == COMBINING_GRAPHEME_JOINER) ||
                                (otherChar == COMBINING_GRAPHEME_JOINER) ||
                                (otherChar == ZERO_WIDTH_JOINER) ||
                                ((otherChar >= HANGUL_CHOSEONG_START) && (otherChar <= HANGUL_JONGSEONG_END)) ||
                                (RSUniCharGetCombiningPropertyForCharacter(otherChar, combClassBMP) == 9))
                            {
                                RSRange clusterRange = RSStringGetRangeOfCharacterClusterAtIndex(string, str1Index - 1, RSStringGraphemeCluster);
                                if (str1Index < (clusterRange.location + clusterRange.length)) match = NO;
                            }
                        }
                    }
                    
                    if (match)
                    {
                        if ((nil != ignoredChars) && backwardAnchor && (str1Index < maxStr1Index))
                        { // Process the anchor tail
                            while (str1Index < maxStr1Index)
                            {
                                str1Char = RSStringGetCharacterFromInlineBuffer(&inlineBuf1, str1Index);
                                if (RSUniCharIsSurrogateHighCharacter(str1Char) &&
                                    RSUniCharIsSurrogateLowCharacter((otherChar = RSStringGetCharacterFromInlineBuffer(&inlineBuf1, str1Index + 1))))
                                {
                                    str1Char = RSUniCharGetLongCharacterForSurrogatePair(str1Char, otherChar);
                                }
                                if (!RSCharacterSetInlineBufferIsLongCharacterMember(ignoredChars, str1Char))
                                    break;
                                str1Index += ((str1Char < 0x10000) ? 1 : 2);
                            }
                        }
                        if (!backwardAnchor || (str1Index == maxStr1Index))
                        {
                            didFind = YES;
                            if (nil != result)
                                *result = RSMakeRange(fromLoc, str1Index - fromLoc);
                        }
                        break;
                    }
                }
                
                if (fromLoc == toLoc) break;
                fromLoc += delta;
            }
        }
        else
        {
            while (1)
            {
                str1Index = fromLoc;
                str2Index = 0;
                
                while (str2Index < findStrLen)
                {
                    if (RSStringGetCharacterFromInlineBuffer(&inlineBuf1, str1Index) !=
                        RSStringGetCharacterFromInlineBuffer(&inlineBuf2, str2Index))
                        break;
                    ++str1Index; ++str2Index;
                }
                
                if (str2Index == findStrLen)
                {
                    didFind = YES;
                    if (nil != result)
                        *result = RSMakeRange(fromLoc, findStrLen);
                    break;
                }
                if (fromLoc == toLoc) break;
                fromLoc += delta;
            }
        }
    }
    
    return didFind;
}

RSExport BOOL RSStringFindWithOptions(RSStringRef string, RSStringRef stringToFind, RSRange rangeToSearch, RSStringCompareFlags compareOptions, RSRange *result)
{
    return RSStringFindWithOptionsAndLocale(string, stringToFind, rangeToSearch, compareOptions, nil, result);
}

RSExport BOOL RSStringHasPrefix(RSStringRef string, RSStringRef prefix)
{
    return RSStringFindWithOptions(string, prefix, RSMakeRange(0, RSStringGetLength(string)), RSCompareAnchored, nil);
}

RSExport BOOL RSStringHasSuffix(RSStringRef string, RSStringRef suffix)
{
    return RSStringFindWithOptions(string, suffix, RSMakeRange(0, RSStringGetLength(string)), RSCompareAnchored|RSCompareBackwards, nil);
}

RSExport RSStringRef RSStringCreateSubStringWithRange(RSAllocatorRef allocator, RSStringRef aString, RSRange range)
{
    return RSStringCreateWithSubstring(allocator, aString, range);
}

static RSRange * __RSStringFindToRanges(RSStringRef string, RSStringRef separatorString, RSIndex *num)
{
    if (string && separatorString && num && __RSStringAvailable(string) && __RSStringAvailable(separatorString))
    {
        RSBitU8 *s1 = nil, *s2 = nil;
        RSIndex l1 = 0, l2 = 0, r1 = 0, r2 =0;
        r1 = RSStringGetBytes(string, RSStringGetRange(string), RSStringEncodingUTF8, 0, NO, s1, RSStringGetLength(string), &l1);
        r2 = RSStringGetBytes(separatorString, RSStringGetRange(separatorString), RSStringEncodingUTF8, 0, NO, s2, RSStringGetLength(separatorString), &l2);
        if (r1 && r2)
        {
            s1 = RSAllocatorAllocate(RSGetAllocator(string), l1 + 2);
            s2 = RSAllocatorAllocate(RSGetAllocator(separatorString), l2 + 2);
            RSStringGetBytes(string, RSStringGetRange(string), RSStringEncodingUTF8, 0, NO, s1, RSStringGetLength(string), 0);
            RSStringGetBytes(separatorString, RSStringGetRange(separatorString), RSStringEncodingUTF8, 0, NO, s2, RSStringGetLength(separatorString), 0);
            RSRange *rgs = __RSBufferFindSubStrings((RSCBuffer)s1, (RSCBuffer)s2, num);
            RSAllocatorDeallocate(RSGetAllocator(string), s1);
            RSAllocatorDeallocate(RSGetAllocator(separatorString), s2);
            return rgs;
        }
        return nil;
    }
    return nil;
}

RSExport BOOL RSStringFind(RSStringRef aString, RSStringRef stringToFind, RSRange rangeToSearch, RSRange* result)
{
    if (result == nil) return NO;
    if (!(__RSStringAvailable(aString) && __RSStringAvailable(stringToFind))) return NO;
    __RSAssertRangeIsInStringBounds(aString, rangeToSearch.location, rangeToSearch.length);
    return RSStringFindWithOptions(aString, stringToFind, rangeToSearch, RSCompareCaseInsensitive, result);
}

RSExport RSComparisonResult RSStringCompareCaseInsensitive(RSStringRef aString, RSStringRef other)
{
    return RSStringCompareWithOptionsAndLocale(aString, other, RSStringGetRange(aString), RSCompareCaseInsensitive, nil);
}

RSExport RSRange *RSStringFindAll(RSStringRef aString, RSStringRef stringToFind, RSIndex *numberOfResult)
{
    return __RSStringFindToRanges(aString, stringToFind, numberOfResult);
}

// Functions to deal with special arrays of RSRange, RSDataRef, created by RSStringCreateArrayWithFindResults()

static const void *__rangeRetain(RSAllocatorRef allocator, const void *ptr) {
    RSRetain(*(RSDataRef *)((uint8_t *)ptr + sizeof(RSRange)));
    return ptr;
}

static void __rangeRelease(RSAllocatorRef allocator, const void *ptr) {
    RSRelease(*(RSDataRef *)((uint8_t *)ptr + sizeof(RSRange)));
}

static RSStringRef __rangeCopyDescription(const void *ptr) {
    RSRange range = *(RSRange *)ptr;
    return RSStringCreateWithFormat(RSAllocatorSystemDefault, nil, RSSTR("{%d, %d}"), range.location, range.length);
}

static BOOL	__rangeEqual(const void *ptr1, const void *ptr2) {
    RSRange range1 = *(RSRange *)ptr1;
    RSRange range2 = *(RSRange *)ptr2;
    return (range1.location == range2.location) && (range1.length == range2.length);
}



RSArrayRef RSStringCreateArrayWithFindResults(RSAllocatorRef alloc, RSStringRef string, RSStringRef stringToFind, RSRange rangeToSearch, RSStringCompareFlags compareOptions)
{
    RSRange foundRange;
    BOOL backwards = ((compareOptions & RSCompareBackwards) != 0);
    UInt32 endIndex = (RSBitU32)(rangeToSearch.location + rangeToSearch.length);
    RSMutableDataRef rangeStorage = nil;	// Basically an array of RSRange, RSDataRef (packed)
    uint8_t *rangeStorageBytes = nil;
    RSIndex foundCount = 0;
    RSIndex capacity = 0;		// Number of RSRange, RSDataRef element slots in rangeStorage
    
    if (alloc == nil) alloc = RSAllocatorGetDefault();
    
    while ((rangeToSearch.length > 0) && RSStringFindWithOptions(string, stringToFind, rangeToSearch, compareOptions, &foundRange))
    {
        // Determine the next range
        if (backwards) {
            rangeToSearch.length = foundRange.location - rangeToSearch.location;
        } else {
            rangeToSearch.location = foundRange.location + foundRange.length;
            rangeToSearch.length = endIndex - rangeToSearch.location;
        }
        
        // If necessary, grow the data and squirrel away the found range
        if (foundCount >= capacity) {
            // Note that rangeStorage is not allowed to be allocated from one of the GCRefZero allocators
            if (rangeStorage == nil) rangeStorage = RSDataCreateMutable((alloc), 0);
            capacity = (capacity + 4) * 2;
            RSDataSetLength(rangeStorage, capacity * (sizeof(RSRange) + sizeof(RSDataRef)));
            capacity = RSDataGetCapacity(rangeStorage) / (sizeof(RSRange) + sizeof(RSDataRef));
            rangeStorageBytes = (uint8_t *)RSDataGetMutableBytesPtr(rangeStorage) + foundCount * (sizeof(RSRange) + sizeof(RSDataRef));
        }
        memmove(rangeStorageBytes, &foundRange, sizeof(RSRange));	// The range
        memmove(rangeStorageBytes + sizeof(RSRange), &rangeStorage, sizeof(RSDataRef));	// The data
        rangeStorageBytes += (sizeof(RSRange) + sizeof(RSDataRef));
        foundCount++;
    }
    
    if (foundCount > 0) {
        RSIndex cnt;
        RSMutableArrayRef array;
        const RSArrayCallBacks callbacks = {0, __rangeRetain, __rangeRelease, __rangeCopyDescription, __rangeEqual};
        
        RSDataIncreaseLength(rangeStorage, foundCount * (sizeof(RSRange) + sizeof(RSDataRef)));	// Tighten storage up
        rangeStorageBytes = (uint8_t *)RSDataGetMutableBytesPtr(rangeStorage);
        
        array = __RSArrayCreateMutable0(alloc, foundCount * sizeof(RSRange *), &callbacks);
        for (cnt = 0; cnt < foundCount; cnt++) {
            // Each element points to the appropriate RSRange in the RSData
            RSArrayAddObject(array, rangeStorageBytes + cnt * (sizeof(RSRange) + sizeof(RSDataRef)));
        }
        RSRelease(rangeStorage);		// We want the data to go away when all RSRanges inside it are released...
        return array;
    } else {
        return nil;
    }
}

RSExport RSStringRef RSStringCreateByCombiningStrings(RSAllocatorRef alloc, RSArrayRef array, RSStringRef separatorString)
{
    RSIndex numChars;
    RSIndex separatorNumByte;
    RSIndex stringCount = RSArrayGetCount(array);
    BOOL isSepRSString = !RS_IS_OBJC(__RSStringTypeID, separatorString);
    BOOL canBeEightbit = isSepRSString && __RSStrIsEightBit(separatorString);
    RSIndex idx;
    RSStringRef otherString;
    void *buffer;
    uint8_t *bufPtr;
    const void *separatorContents = nil;
    
    if (stringCount == 0) {
        return RSStringCreateWithCharacters(alloc, nil, 0);
    } else if (stringCount == 1) {
        return (RSStringRef)RSStringCreateCopy(alloc, (RSStringRef)RSArrayObjectAtIndex(array, 0));
    }
    
    if (alloc == nil) alloc = RSAllocatorSystemDefault;
    
    numChars = RSStringGetLength(separatorString) * (stringCount - 1);
    for (idx = 0; idx < stringCount; idx++)
    {
        otherString = (RSStringRef)RSArrayObjectAtIndex(array, idx);
        numChars += RSStringGetLength(otherString);
        // canBeEightbit is already NO if the separator is an NSString...
        if (RS_IS_OBJC(__RSStringTypeID, otherString) || ! __RSStrIsEightBit(otherString)) canBeEightbit = NO;
    }
    
    buffer = (uint8_t *)RSAllocatorAllocate(alloc, canBeEightbit ? ((numChars + 1) * sizeof(uint8_t)) : (numChars * sizeof(UniChar)));
	bufPtr = (uint8_t *)buffer;
//    if (__RSOASafe) __RSSetLastAllocationEventName(buffer, "RSString (store)");
    separatorNumByte = RSStringGetLength(separatorString) * (canBeEightbit ? sizeof(uint8_t) : sizeof(UniChar));
    
    for (idx = 0; idx < stringCount; idx++) {
        if (idx) { // add separator here unless first string
            if (separatorContents) {
                memmove(bufPtr, separatorContents, separatorNumByte);
            } else {
                if (!isSepRSString) { // NSString
                    RSStringGetCharacters(separatorString, RSMakeRange(0, RSStringGetLength(separatorString)), (UniChar *)bufPtr);
                } else if (canBeEightbit) {
                    memmove(bufPtr, (const uint8_t *)__RSStrContents(separatorString) + __RSStrSkipAnyLengthByte(separatorString), separatorNumByte);
                } else {
                    __RSStrConvertBytesToUnicode((uint8_t *)__RSStrContents(separatorString) + __RSStrSkipAnyLengthByte(separatorString), (UniChar *)bufPtr, __RSStrLength(separatorString));
                }
                separatorContents = bufPtr;
            }
            bufPtr += separatorNumByte;
        }
        
        otherString = (RSStringRef )RSArrayObjectAtIndex(array, idx);
        if (RS_IS_OBJC(__RSStringTypeID, otherString)) {
            RSIndex otherLength = RSStringGetLength(otherString);
            RSStringGetCharacters(otherString, RSMakeRange(0, otherLength), (UniChar *)bufPtr);
            bufPtr += otherLength * sizeof(UniChar);
        } else {
            const uint8_t * otherContents = (const uint8_t *)__RSStrContents(otherString);
            RSIndex otherNumByte = __RSStrLength2(otherString, otherContents) * (canBeEightbit ? sizeof(uint8_t) : sizeof(UniChar));
            
            if (canBeEightbit || __RSStrIsUnicode(otherString)) {
                memmove(bufPtr, otherContents + __RSStrSkipAnyLengthByte(otherString), otherNumByte);
            } else {
                __RSStrConvertBytesToUnicode(otherContents + __RSStrSkipAnyLengthByte(otherString), (UniChar *)bufPtr, __RSStrLength2(otherString, otherContents));
            }
            bufPtr += otherNumByte;
        }
    }
    if (canBeEightbit) *bufPtr = 0; // nil byte;
    
    return canBeEightbit ?
    RSStringCreateWithCStringNoCopy(alloc, (const char*)buffer, __RSStringGetEightBitStringEncoding(), alloc) :
    RSStringCreateWithCharactersNoCopy(alloc, (UniChar *)buffer, numChars, alloc);
}


RSExport RSArrayRef RSStringCreateComponentsSeparatedByStrings(RSAllocatorRef alloc, RSStringRef string, RSStringRef separatorString) {
    RSArrayRef separatorRanges;
    RSIndex length = RSStringGetLength(string);
    /* No objc dispatch needed here since RSStringCreateArrayWithFindResults() works with both RSString and NSString */
    if (!(separatorRanges = RSStringCreateArrayWithFindResults(alloc, string, separatorString, RSMakeRange(0, length), 0))) {
        return RSArrayCreateWithObject(alloc, string);
    } else {
        RSIndex idx;
        RSIndex count = RSArrayGetCount(separatorRanges);
        RSIndex startIndex = 0;
        RSIndex numChars;
        RSMutableArrayRef array = RSArrayCreateMutable(alloc, count + 2);
        const RSRange *currentRange;
        RSStringRef substring;
        
        for (idx = 0;idx < count;idx++) {
            currentRange = (const RSRange *)RSArrayObjectAtIndex(separatorRanges, idx);
            numChars = currentRange->location - startIndex;
            substring = RSStringCreateWithSubstring(alloc, string, RSMakeRange(startIndex, numChars));
            RSArrayAddObject(array, substring);
            if (!_RSAllocatorIsGCRefZero(alloc)) RSRelease(substring);
            startIndex = currentRange->location + currentRange->length;
        }
        substring = RSStringCreateWithSubstring(alloc, string, RSMakeRange(startIndex, length - startIndex));
        RSArrayAddObject(array, substring);
        if (!_RSAllocatorIsGCRefZero(alloc)) RSRelease(substring);
        
        if (!_RSAllocatorIsGCRefZero(alloc)) RSRelease(separatorRanges);
        
        return array;
    }
}

RSExport RSArrayRef RSStringCreateComponentsSeparatedByCharactersInSet(RSAllocatorRef allocator, RSStringRef string, RSCharacterSetRef charactersSet) {
    if (!string || !charactersSet) return nil;
    RSRange searchRange = RSStringGetRange(string);
    RSRange range;
    if (!RSStringFindCharacterFromSet(string, charactersSet, searchRange, 0, &range)) {
        return RSArrayCreateWithObject(allocator, string);
    } else {
        RSMutableArrayRef array = RSArrayCreateMutable(allocator, 0);
        RSIndex location = 0;
        RSStringRef part = RSStringCreateSubStringWithRange(allocator, string, RSMakeRange(location, range.location));
        RSArrayAddObject(array, part);
        RSRelease(part);
        
        searchRange.location = range.location + range.length;
        searchRange.length = RSStringGetLength(string) - searchRange.location;
        location = searchRange.location;
        while (RSStringFindCharacterFromSet(string, charactersSet, searchRange, 0, &range)) {
            if (range.location > location) {
                part = RSStringCreateSubStringWithRange(allocator, string, RSMakeRange(location, range.location - location));
                RSArrayAddObject(array, part);
                RSRelease(part);
            }
            searchRange.location = range.location + range.length;
            searchRange.length = RSStringGetLength(string) - searchRange.location;
            location = searchRange.location;
        }
        
        return array;
    }
    return nil;
}

RSExport RSDataRef RSStringCreateExternalRepresentation(RSAllocatorRef alloc, RSStringRef string, RSStringEncoding encoding, RSIndex lossByte)
{
    RSIndex length;
    RSIndex guessedByteLength;
    uint8_t *bytes;
    RSIndex usedLength;
    SInt32 result;
    
    if (RS_IS_OBJC(__RSStringTypeID, string)) {	/* ??? Hope the compiler optimizes this away if OBJC_MAPPINGS is not on */
        length = RSStringGetLength(string);
    } else {
        __RSAssertIsString(string);
        length = __RSStrLength(string);
        if (__RSStrIsEightBit(string) && ((__RSStringGetEightBitStringEncoding() == encoding) || (__RSStringGetEightBitStringEncoding() == RSStringEncodingASCII && __RSStringEncodingIsSupersetOfASCII(encoding)))) {	// Requested encoding is equal to the encoding in string
            return RSDataCreate(alloc, ((uint8_t *)__RSStrContents(string) + __RSStrSkipAnyLengthByte(string)), __RSStrLength(string));
        }
    }
    
    if (alloc == nil) alloc = RSAllocatorSystemDefault;
    
    if (((encoding & 0x0FFF) == RSStringEncodingUnicode) && ((encoding == RSStringEncodingUnicode) || ((encoding > RSStringEncodingUTF8) && (encoding <= RSStringEncodingUTF32LE)))) {
        guessedByteLength = (length + 1) * ((((encoding >> 26)  & 2) == 0) ? sizeof(UTF16Char) : sizeof(UTF32Char)); // UTF32 format has the bit set
    } else if (((guessedByteLength = RSStringGetMaximumSizeForEncoding(length, encoding)) > length) && !RS_IS_OBJC(__RSStringTypeID, string)) { // Multi byte encoding
#if DEPLOYMENT_TARGET_MACOSX || DEPLOYMENT_TARGET_EMBEDDED || DEPLOYMENT_TARGET_EMBEDDED_MINI || DEPLOYMENT_TARGET_LINUX || DEPLOYMENT_TARGET_FREEBSD
        if (__RSStrIsUnicode(string)) {
            RSIndex aLength = RSStringEncodingByteLengthForCharacters(encoding, RSStringEncodingPrependBOM, __RSStrContents(string), __RSStrLength(string));
            if (aLength > 0) guessedByteLength = aLength;
        } else {
#endif
            result = (int)__RSStringEncodeByteStream(string, 0, length, YES, encoding, (RSBitU8)lossByte, nil, LONG_MAX, &guessedByteLength);
            // if result == length, we always succeed
            //   otherwise, if result == 0, we fail
            //   otherwise, if there was a lossByte but still result != length, we fail
            if ((result != length) && (!result || !lossByte)) return nil;
            if (guessedByteLength == length && __RSStrIsEightBit(string) && __RSStringEncodingIsSupersetOfASCII(encoding)) { // It's all ASCII !!
                return RSDataCreate(alloc, ((uint8_t *)__RSStrContents(string) + __RSStrSkipAnyLengthByte(string)), __RSStrLength(string));
            }
#if DEPLOYMENT_TARGET_MACOSX || DEPLOYMENT_TARGET_EMBEDDED || DEPLOYMENT_TARGET_EMBEDDED_MINI || DEPLOYMENT_TARGET_LINUX || DEPLOYMENT_TARGET_FREEBSD
        }
#endif
    }
    bytes = (uint8_t *)RSAllocatorAllocate(alloc, guessedByteLength);
//    if (__RSOASafe) __RSSetLastAllocationEventName(bytes, "RSData (store)");
    
    result = (int)__RSStringEncodeByteStream(string, 0, length, YES, encoding, (RSBitU8)lossByte, bytes, guessedByteLength, &usedLength);
    
    if ((result != length) && (!result || !lossByte)) {		// see comment above about what this means
        RSAllocatorDeallocate(alloc, bytes);
        return nil;
    }
    return RSDataCreateWithNoCopy(alloc, bytes, usedLength, YES, alloc);
}

RSExport void RSStringInsert(RSMutableStringRef str, RSIndex idx, RSStringRef insertedStr)
{
    RS_OBJC_FUNCDISPATCHV(__RSStringTypeID, void, (NSMutableString *)str, insertString:(NSString *)insertedStr atIndex:(NSUInteger)idx);
    __RSAssertIsStringAndMutable(str);
    RSAssert3(idx >= 0 && idx <= __RSStrLength(str), __RSLogAssertion, "%s(): string index %d out of bounds (length %d)", __PRETTY_FUNCTION__, idx, __RSStrLength(str));
    __RSStringReplace(str, RSMakeRange(idx, 0), insertedStr);
}

RSExport void RSStringDelete(RSMutableStringRef str, RSRange range)
{
    RS_OBJC_FUNCDISPATCHV(__RSStringTypeID, void, (NSMutableString *)str, deleteCharactersInRange:NSMakeRange(range.location, range.length));
    __RSAssertIsStringAndMutable(str);
    __RSAssertRangeIsInStringBounds(str, range.location, range.length);
    __RSStringChangeSize(str, range, 0, NO);
}

RSExport void RSStringTrim(RSMutableStringRef string, RSStringRef trimString) {
    RSRange range;
    RSIndex newStartIndex;
    RSIndex length;
    
    RS_OBJC_FUNCDISPATCHV(__RSStringTypeID, void, (NSMutableString *)string, _cfTrim:trimString);
    
    __RSAssertIsStringAndMutable(string);
    __RSAssertIsString(trimString);
    
    newStartIndex = 0;
    length = __RSStrLength(string);
    
    while (RSStringFindWithOptions(string, trimString, RSMakeRange(newStartIndex, length - newStartIndex), RSCompareAnchored, &range)) {
        newStartIndex = range.location + range.length;
    }
    
    if (newStartIndex < length) {
        RSIndex charSize = __RSStrIsUnicode(string) ? sizeof(UniChar) : sizeof(uint8_t);
        uint8_t *contents = (uint8_t *)__RSStrContents(string) + __RSStrSkipAnyLengthByte(string);
        
        length -= newStartIndex;
        if (__RSStrLength(trimString) < length) {
            while (RSStringFindWithOptions(string, trimString, RSMakeRange(newStartIndex, length), RSCompareAnchored|RSCompareBackwards, &range)) {
                length = range.location - newStartIndex;
            }
        }
        memmove(contents, contents + newStartIndex * charSize, length * charSize);
        __RSStringChangeSize(string, RSMakeRange(length, __RSStrLength(string) - length), 0, NO);
    } else { // Only trimString in string, trim all
        __RSStringChangeSize(string, RSMakeRange(0, length), 0, NO);
    }
}

RSExport void RSStringTrimWhitespace(RSMutableStringRef string) {
    RSIndex newStartIndex;
    RSIndex length;
    RSStringInlineBuffer buffer;
    
    RS_OBJC_FUNCDISPATCHV(__RSStringTypeID, void, (NSMutableString *)string, _cfTrimWS);
    
    __RSAssertIsStringAndMutable(string);
    
    newStartIndex = 0;
    length = __RSStrLength(string);
    
    RSStringInitInlineBuffer(string, &buffer, RSMakeRange(0, length));
    RSIndex buffer_idx = 0;
    
    while (buffer_idx < length && RSUniCharIsMemberOf(__RSStringGetCharacterFromInlineBufferQuick(&buffer, buffer_idx), RSUniCharWhitespaceAndNewlineCharacterSet))
        buffer_idx++;
    newStartIndex = buffer_idx;
    
    if (newStartIndex < length) {
        uint8_t *contents = (uint8_t *)__RSStrContents(string) + __RSStrSkipAnyLengthByte(string);
        RSIndex charSize = (__RSStrIsUnicode(string) ? sizeof(UniChar) : sizeof(uint8_t));
        
        buffer_idx = length - 1;
        while (0 <= buffer_idx && RSUniCharIsMemberOf(__RSStringGetCharacterFromInlineBufferQuick(&buffer, buffer_idx), RSUniCharWhitespaceAndNewlineCharacterSet))
            buffer_idx--;
        length = buffer_idx - newStartIndex + 1;
        
        memmove(contents, contents + newStartIndex * charSize, length * charSize);
        __RSStringChangeSize(string, RSMakeRange(length, __RSStrLength(string) - length), 0, NO);
    } else { // Whitespace only string
        __RSStringChangeSize(string, RSMakeRange(0, length), 0, NO);
    }
}

RSExport void RSStringTrimInCharacterSet(RSMutableStringRef string, RSCharacterSetRef characterSet) {
    if (!characterSet || !string) return;
    __RSAssertIsStringAndMutable(string);
    RSIndex newStartIndex = 0;
    RSStringInlineBuffer buffer;
    RSIndex length = RSStringGetLength(string);
    RSStringInitInlineBuffer(string, &buffer, RSMakeRange(0, length));
    RSIndex buffer_idx = 0;
    while (buffer_idx < length && RSCharacterSetIsCharacterMember(characterSet, __RSStringGetCharacterFromInlineBufferQuick(&buffer, buffer_idx)))
        buffer_idx++;
    newStartIndex = buffer_idx;
    if (newStartIndex < length) {
        uint8_t *contents = (uint8_t *)__RSStrContents(string) + __RSStrSkipAnyLengthByte(string);
        RSIndex charSize = (__RSStrIsUnicode(string) ? sizeof(UniChar) : sizeof(uint8_t));
        
        buffer_idx = length - 1;
        while (0 <= buffer_idx && RSCharacterSetIsCharacterMember(characterSet, __RSStringGetCharacterFromInlineBufferQuick(&buffer, buffer_idx)))
            buffer_idx--;
        length = buffer_idx - newStartIndex + 1;
        
        memmove(contents, contents + newStartIndex * charSize, length * charSize);
        __RSStringChangeSize(string, RSMakeRange(length, __RSStrLength(string) - length), 0, NO);
    } else { // Whitespace only string
        __RSStringChangeSize(string, RSMakeRange(0, length), 0, NO);
    }
}

//- (NSString *) stringByTrimmingCharactersInSet:(NSCharacterSet *) set
//{ // not at all optimized for speed!
//	unsigned from=0, to=[self length];
//	while(from < to)
//    {
//		if(![set characterIsMember:[self characterAtIndex:from]])
//			break;
//		from++;
//    }
//	while(to > from)
//    {
//		if(![set characterIsMember:[self characterAtIndex:to-1]])
//			break;
//		to--;
//    }
//	return [self substringWithRange:NSMakeRange(from, to-from)];
//}

RSInline BOOL _RSCanUseLocale(RSLocaleRef locale) {
    if (locale) {
        return YES;
    }
    return NO;
}

RSExport void RSStringLowercase(RSMutableStringRef string, RSLocaleRef locale) {
    RSIndex currentIndex = 0;
    RSIndex length;
    const uint8_t *langCode;
    BOOL isEightBit = __RSStrIsEightBit(string);
    
    RS_OBJC_FUNCDISPATCHV(__RSStringTypeID, void, (NSMutableString *)string, _cfLowercase:(const void *)locale);
    
    __RSAssertIsStringAndMutable(string);
    
    length = __RSStrLength(string);
    
    langCode = (const uint8_t *)(_RSCanUseLocale(locale) ? _RSStrGetLanguageIdentifierForLocale(locale, NO) : NULL);
    
    if (!langCode && isEightBit) {
        uint8_t *contents = (uint8_t *)__RSStrContents(string) + __RSStrSkipAnyLengthByte(string);
        for (;currentIndex < length;currentIndex++) {
            if (contents[currentIndex] >= 'A' && contents[currentIndex] <= 'Z') {
                contents[currentIndex] += 'a' - 'A';
            } else if (contents[currentIndex] > 127) {
                break;
            }
        }
    }
    
    if (currentIndex < length) {
        UTF16Char *contents;
        UniChar mappedCharacters[MAX_CASE_MAPPING_BUF];
        RSIndex mappedLength;
        UTF32Char currentChar;
        UInt32 flags = 0;
        
        if (isEightBit) __RSStringChangeSize(string, RSMakeRange(0, 0), 0, YES);
        
        contents = (UniChar *)__RSStrContents(string);
        
        for (;currentIndex < length;currentIndex++) {
            
            if (RSUniCharIsSurrogateHighCharacter(contents[currentIndex]) && (currentIndex + 1 < length) && RSUniCharIsSurrogateLowCharacter(contents[currentIndex + 1])) {
                currentChar = RSUniCharGetLongCharacterForSurrogatePair(contents[currentIndex], contents[currentIndex + 1]);
            } else {
                currentChar = contents[currentIndex];
            }
            flags = ((langCode || (currentChar == 0x03A3)) ? RSUniCharGetConditionalCaseMappingFlags(currentChar, contents, currentIndex, length, RSUniCharToLowercase, langCode, flags) : 0);
            
            mappedLength = RSUniCharMapCaseTo(currentChar, mappedCharacters, MAX_CASE_MAPPING_BUF, RSUniCharToLowercase, flags, langCode);
            if (mappedLength > 0) contents[currentIndex] = *mappedCharacters;
            
            if (currentChar > 0xFFFF) { // Non-BMP char
                switch (mappedLength) {
                    case 0:
                        __RSStringChangeSize(string, RSMakeRange(currentIndex, 2), 0, YES);
                        contents = (UniChar *)__RSStrContents(string);
                        length -= 2;
                        break;
                        
                    case 1:
                        __RSStringChangeSize(string, RSMakeRange(currentIndex + 1, 1), 0, YES);
                        contents = (UniChar *)__RSStrContents(string);
                        --length;
                        break;
                        
                    case 2:
                        contents[++currentIndex] = mappedCharacters[1];
                        break;
                        
                    default:
                        --mappedLength; // Skip the current char
                        __RSStringChangeSize(string, RSMakeRange(currentIndex + 1, 0), mappedLength - 1, YES);
                        contents = (UniChar *)__RSStrContents(string);
                        memmove(contents + currentIndex + 1, mappedCharacters + 1, mappedLength * sizeof(UniChar));
                        length += (mappedLength - 1);
                        currentIndex += mappedLength;
                        break;
                }
            } else if (mappedLength == 0) {
                __RSStringChangeSize(string, RSMakeRange(currentIndex, 1), 0, YES);
                contents = (UniChar *)__RSStrContents(string);
                --length;
            } else if (mappedLength > 1) {
                --mappedLength; // Skip the current char
                __RSStringChangeSize(string, RSMakeRange(currentIndex + 1, 0), mappedLength, YES);
                contents = (UniChar *)__RSStrContents(string);
                memmove(contents + currentIndex + 1, mappedCharacters + 1, mappedLength * sizeof(UniChar));
                length += mappedLength;
                currentIndex += mappedLength;
            }
        }
    }
}

RSExport void RSStringUppercase(RSMutableStringRef string, RSLocaleRef locale) {
    RSIndex currentIndex = 0;
    RSIndex length;
    const uint8_t *langCode;
    BOOL isEightBit = __RSStrIsEightBit(string);
    
    RS_OBJC_FUNCDISPATCHV(__RSStringTypeID, void, (NSMutableString *)string, _cfUppercase:(const void *)locale);
    
    __RSAssertIsStringAndMutable(string);
    
    length = __RSStrLength(string);
    
    langCode = (const uint8_t *)(_RSCanUseLocale(locale) ? _RSStrGetLanguageIdentifierForLocale(locale, NO) : NULL);
    
    if (!langCode && isEightBit) {
        uint8_t *contents = (uint8_t *)__RSStrContents(string) + __RSStrSkipAnyLengthByte(string);
        for (;currentIndex < length;currentIndex++) {
            if (contents[currentIndex] >= 'a' && contents[currentIndex] <= 'z') {
                contents[currentIndex] -= 'a' - 'A';
            } else if (contents[currentIndex] > 127) {
                break;
            }
        }
    }
    
    if (currentIndex < length) {
        UniChar *contents;
        UniChar mappedCharacters[MAX_CASE_MAPPING_BUF];
        RSIndex mappedLength;
        UTF32Char currentChar;
        UInt32 flags = 0;
        
        if (isEightBit) __RSStringChangeSize(string, RSMakeRange(0, 0), 0, YES);
        
        contents = (UniChar *)__RSStrContents(string);
        
        for (;currentIndex < length;currentIndex++) {
            if (RSUniCharIsSurrogateHighCharacter(contents[currentIndex]) && (currentIndex + 1 < length) && RSUniCharIsSurrogateLowCharacter(contents[currentIndex + 1])) {
                currentChar = RSUniCharGetLongCharacterForSurrogatePair(contents[currentIndex], contents[currentIndex + 1]);
            } else {
                currentChar = contents[currentIndex];
            }
            
            flags = (langCode ? RSUniCharGetConditionalCaseMappingFlags(currentChar, contents, currentIndex, length, RSUniCharToUppercase, langCode, flags) : 0);
            
            mappedLength = RSUniCharMapCaseTo(currentChar, mappedCharacters, MAX_CASE_MAPPING_BUF, RSUniCharToUppercase, flags, langCode);
            if (mappedLength > 0) contents[currentIndex] = *mappedCharacters;
            
            if (currentChar > 0xFFFF) { // Non-BMP char
                switch (mappedLength) {
                    case 0:
                        __RSStringChangeSize(string, RSMakeRange(currentIndex, 2), 0, YES);
                        contents = (UniChar *)__RSStrContents(string);
                        length -= 2;
                        break;
                        
                    case 1:
                        __RSStringChangeSize(string, RSMakeRange(currentIndex + 1, 1), 0, YES);
                        contents = (UniChar *)__RSStrContents(string);
                        --length;
                        break;
                        
                    case 2:
                        contents[++currentIndex] = mappedCharacters[1];
                        break;
                        
                    default:
                        --mappedLength; // Skip the current char
                        __RSStringChangeSize(string, RSMakeRange(currentIndex + 1, 0), mappedLength - 1, YES);
                        contents = (UniChar *)__RSStrContents(string);
                        memmove(contents + currentIndex + 1, mappedCharacters + 1, mappedLength * sizeof(UniChar));
                        length += (mappedLength - 1);
                        currentIndex += mappedLength;
                        break;
                }
            } else if (mappedLength == 0) {
                __RSStringChangeSize(string, RSMakeRange(currentIndex, 1), 0, YES);
                contents = (UniChar *)__RSStrContents(string);
                --length;
            } else if (mappedLength > 1) {
                --mappedLength; // Skip the current char
                __RSStringChangeSize(string, RSMakeRange(currentIndex + 1, 0), mappedLength, YES);
                contents = (UniChar *)__RSStrContents(string);
                memmove(contents + currentIndex + 1, mappedCharacters + 1, mappedLength * sizeof(UniChar));
                length += mappedLength;
                currentIndex += mappedLength;
            }
        }
    }
}

RSExport const char* RSStringCopyUTF8String(RSStringRef str)
{
    if (!str) return nil;
    const char *utf8 = nil;
    RSIndex usedLength = 0, maxSize = RSStringGetMaximumSizeForEncoding(RSStringGetLength(str), RSStringEncodingUTF8);
    if (maxSize)
    {
        if ((utf8 = RSAllocatorAllocate(RSAllocatorSystemDefault, ++maxSize)))
        {
            usedLength = RSStringGetCString(str, (char *)utf8, maxSize, RSStringEncodingUTF8) + 1; // add end flag
            char *tmp = RSAllocatorAllocate(RSAllocatorSystemDefault, usedLength);
            if (tmp)
            {
                __builtin_memcpy(tmp, utf8, usedLength);
                RSAllocatorDeallocate(RSAllocatorSystemDefault, utf8);
                utf8 = tmp;
            }
        }
    }
    return utf8;
}

RSExport const char* RSStringGetUTF8String(RSStringRef str)
{
    const char *utf8 = nil;
    if ((utf8 = RSStringGetCStringPtr(str, RSStringEncodingUTF8))) return utf8;
    if ((utf8 = RSStringCopyUTF8String(str))) __RSAutoReleaseISA(RSAllocatorSystemDefault, (ISA)utf8);
    return utf8;
}

#if defined(RSStringEncodingSupport)
RSExport RSStringRef RSStringCreateConvert(RSStringRef aString, RSStringEncoding encoding)
{
    if (!aString) return nil;
    __RSAssertIsString(aString);
    if (RSStringGetCStringPtr(aString, encoding) || (encoding == RSStringEncodingUnicode && RSStringGetCharactersPtr(aString)))
        return (RSCopy(RSAllocatorSystemDefault, aString));
    RSIndex len = RSStringGetMaximumSizeForEncoding(RSStringGetLength(aString), encoding);
    if (len)
    {
        char *buf = RSAllocatorAllocate(RSAllocatorSystemDefault, len);
        if (buf) {
            RSIndex success = RSStringGetCString(aString, buf, len, encoding);
            if (success >= 0)
            {
                return RSStringCreateWithBytesNoCopy(RSAllocatorSystemDefault, (uint8_t*)buf, success, encoding, YES, RSAllocatorSystemDefault);
            }
            RSAllocatorDeallocate(RSAllocatorSystemDefault, buf);
        }
    }
    return nil;
}

RSExport void RSMutableStringConvert(RSMutableStringRef aString, RSStringEncoding encoding)
{
    if (!aString) return ;
    __RSAssertIsStringAndMutable(aString);
    if (RSStringGetCStringPtr(aString, encoding) || (encoding == RSStringEncodingUnicode && RSStringGetCharactersPtr(aString))) return;
    RSIndex strLen = __RSStrLength(aString);
    RSIndex len = RSStringGetMaximumSizeForEncoding(strLen, encoding);
    if (len)
    {
        RSIndex usedLen = 0;
        if (strLen < len)
            __RSStringChangeSize(aString, RSMakeRange(__RSStrLength(aString), 0), len - strLen, NO);
        __RSStringEncodeByteStream(aString, 0, strLen, NO, encoding, NO, (RSUBlock *)__RSStrContents(aString) + __RSStrSkipAnyLengthByte(aString), len - 1, &usedLen);
        __RSStrSetExplicitLength(aString, usedLen);
        __RSStringChangeSize(aString, RSMakeRange(0, __RSStrLength(aString)), usedLen, NO);
    }
    return;
}

#endif
