//
//  RSInternal.h
//  RSCoreFoundation
//
//  Created by RetVal on 11/13/12.
//  Copyright (c) 2012 RetVal. All rights reserved.
//

#ifndef RSCoreFoundation_RSInternal_h
#define RSCoreFoundation_RSInternal_h

RS_EXTERN_C_BEGIN

#include <RSCoreFoundation/RSRuntime.h>
#include <RSCoreFoundation/RSStringEncoding.h>
#include <RSCoreFoundation/RSAvailability.h>
#include <RSCoreFoundation/RSBaseType.h>

#include "RSStringInlineBuffer.h"
#include "RSFoundationEncoding.h"

#include <dispatch/dispatch.h>

#include <RSCoreFoundation/RSPriv.h>
#define RSDictionaryBasicHash   1
#define RSDictionaryRBTree      2
#define RSDictionaryCoreSelector RSDictionaryBasicHash

#ifdef __CONSTANT_RSSTRINGS__

//struct __RSString {
//    RSRuntimeBase _base;
//    // for memory management
//    RSIndex _byteLength;    // bytes
//    RSIndex _capacity;
//    
//    // count of string
//    RSIndex _length;
//    RSStringEncoding _encoding;
//    union
//    {
//        struct __inline01 {
//            void *_buffer;
//        } _inline1;
//        struct {
//            UniChar *_unicodePtr;
//            unsigned long long _size;
//        }_unicode;
//    }_string;
//};
struct __notInlineMutable {
    void *_buffer;
    RSIndex _length;
    RSIndex _capacity;                           // Capacity in bytes
    unsigned int _hasGap:1;                      // Currently unused
    unsigned int _isFixedCapacity:1;
    unsigned int _isExternalMutable:1;
    unsigned int _capacityProvidedExternally:1;
#if __LP64__
    unsigned long _desiredCapacity:60;
#else
    unsigned long _desiredCapacity:28;
#endif
    RSAllocatorRef _contentsAllocator;           // Optional
};

struct __RSString
{
    RSRuntimeBase _base;
    union
    {
        struct __inline1
        {
            RSIndex _length;
        }_inline1;
        
        struct __inline2
        {
            RSIndex _length;
            void *_buffer;
        }_inline2;
        
        struct __notInlineImmutable1
        {
            void *_buffer;
            RSIndex _length;
            RSAllocatorRef _contentsDeallocator;
        }_notInlineImmutable1;
        
        struct __notInlineImmutable2
        {
            void *_buffer;
            RSAllocatorRef _contentsDeallocator;
        }_notInlineImmutable2;
        struct __notInlineMutable _notInlineMutable1;
    }_variants;
};

#if __LP64__
#define RS_IS_TAGGED_OBJ(PTR)	((uintptr_t)(PTR) & 0x1)
#define RS_TAGGED_OBJ_TYPE(PTR)	((uintptr_t)(PTR) & 0xF)
#else
#define RS_IS_TAGGED_OBJ(PTR)	0
#define RS_TAGGED_OBJ_TYPE(PTR)	0
#endif

enum {
    RSTaggedObjectID_Invalid = 0,
    RSTaggedObjectID_Atom = (0 << 1) + 1,
    RSTaggedObjectID_Undefined3 = (1 << 1) + 1,
    RSTaggedObjectID_Undefined2 = (2 << 1) + 1,
    RSTaggedObjectID_Integer = (3 << 1) + 1,
    RSTaggedObjectID_DateTS = (4 << 1) + 1,
    RSTaggedObjectID_ManagedObjectID = (5 << 1) + 1, // Core Data
    RSTaggedObjectID_Date = (6 << 1) + 1,
    RSTaggedObjectID_Undefined7 = (7 << 1) + 1,
};


RSInline BOOL RS_IS_OBJC(RSTypeID typeID, RSTypeRef obj)
{
    return RS_IS_TAGGED_OBJ(obj);
}
#define RS_OBJC_FUNCDISPATCHV(typeID, obj, ...) do { } while (0)
#define RS_OBJC_CALLV(obj, ...) (0)

RSInline uintptr_t __RSISAForTypeID(RSTypeID typeID)
{
    return 0;
}

#define RSUseCollectableAllocator NO

RSInline ISA objc_assign_strongCast(ISA val, ISA *dest)
{ return (*dest = val); }

RSInline void __RSAssignWithWriteBarrier(void **location, void *value) {
    if (RSUseCollectableAllocator) {
        objc_assign_strongCast((ISA)value, (ISA *)location);
    } else if (location) {
        *location = value;
    }
}

#if __LP64__
//#define RS_CONST_STRING_DECL(S, V) static const struct __RSString ___##S = (const struct __RSString){{0,{10,0,1,12,1,7},1},sizeof(V) - 1,sizeof(V), sizeof(V) - 1, RSStringEncodingUTF8, V}; static const RSStringRef S = &___##S;
#define RS_CONST_STRING_DECL(S, V) static const struct __RSString ___##S = (const struct __RSString){\
    ._base._rsisa = 0,\
    ._base._rc = 1,\
    ._base._rsinfo._reserved = 0,\
    ._base._rsinfo._special = 1,\
    ._base._rsinfo._objId = 7,\
    ._base._rsinfo._rsinfo1 = 72,\
    ._base._rsinfo._rsinfo = (1 << 1) | (1 << 3),\
    ._variants._notInlineImmutable1._buffer = V,\
    ._variants._notInlineImmutable1._length = sizeof(V) - 1};\
    static const RSStringRef S = &___##S;
#else
//#define RS_CONST_STRING_DECL(S, V) static const struct __RSString ___##S = (const struct __RSString){{0,{1,10,0,1,12,1,7}},sizeof(V) - 1,sizeof(V), sizeof(V) - 1, RSStringEncodingUTF8, V}; static const RSStringRef S = &___##S;
#define RS_CONST_STRING_DECL(S, V) static const struct __RSString ___##S = (const struct __RSString){\
    ._base._rsisa = 0,\
    ._base._rsinfo._rc = 1,\
    ._base._rsinfo._reserved = 0,\
    ._base._rsinfo._special = 1,\
    ._base._rsinfo._objId = 7,\
    ._base._rsinfo._rsinfo1 = 72,\
    ._base._rsinfo._rsinfo = (1 << 1) | (1 << 3),\
    ._variants._notInlineImmutable1._buffer = V,\
    ._variants._notInlineImmutable1._length = sizeof(V) - 1};\
    static const RSStringRef S = &___##S;
#endif

#if __LP64__

//#define RS_PUBLIC_CONST_STRING_DECL(S, V) static const struct __RSString ___##S = (const struct __RSString){{0,{10,0,1,12,1,7},1},sizeof(V) - 1,sizeof(V), sizeof(V) - 1, RSStringEncodingUTF8, V}; const RSStringRef S = &___##S;
#define RS_PUBLIC_CONST_STRING_DECL(S, V) const struct __RSString ___##S = (const struct __RSString){\
    ._base._rsisa = 0,\
    ._base._rc = 1,\
    ._base._rsinfo._reserved = 0,\
    ._base._rsinfo._special = 1,\
    ._base._rsinfo._objId = 7,\
    ._base._rsinfo._rsinfo1 = 72,\
    ._base._rsinfo._rsinfo = (1 << 1) | (1 << 3),\
    ._variants._notInlineImmutable1._buffer = V,\
    ._variants._notInlineImmutable1._length = sizeof(V) - 1};\
    const RSStringRef S = &___##S;
#else

//#define RS_PUBLIC_CONST_STRING_DECL(S, V) static const struct __RSString ___##S = (const struct __RSString){{0,{1,10,0,1,12,1,7}},sizeof(V) - 1,sizeof(V), sizeof(V) - 1, RSStringEncodingUTF8, V}; const RSStringRef S = &___##S;
#define RS_PUBLIC_CONST_STRING_DECL(S, V) const struct __RSString ___##S = (const struct __RSString){\
    ._base._rsisa = 0,\
    ._base._rsinfo._rc = 1,\
    ._base._rsinfo._reserved = 0,\
    ._base._rsinfo._special = 1,\
    ._base._rsinfo._objId = 7,\
    ._base._rsinfo._rsinfo1 = 72,\
    ._base._rsinfo._rsinfo = (1 << 1) | (1 << 3),\
    ._variants._notInlineImmutable1._buffer = V,\
    ._variants._notInlineImmutable1._length = sizeof(V) - 1};\
    const RSStringRef S = &___##S;
#endif


//#if __LP64__
//#define RS_PUBLIC_CONST_STRING_DECL(S, V) const struct __RSString ___##S = (const struct __RSString){{0,{10,0,1,12,1,7},1},sizeof(V) - 1,sizeof(V), sizeof(V) - 1, RSStringEncodingUTF8, V};  const RSStringRef S = &___##S;
//#else
//#define RS_PUBLIC_CONST_STRING_DECL(S, V)  const struct __RSString ___##S = (const struct __RSString){{0,{1,10,0,1,12,1,7},sizeof(V) - 1,sizeof(V), sizeof(V) - 1, RSStringEncodingUTF8, V};  const RSStringRef S = &___##S;
//#endif
#endif

#if DEPLOYMENT_TARGET_WINDOWS
#define RSMaxPathSize ((RSIndex)262)
#define RSMaxPathLength ((RSIndex)260)
#define PATH_SEP '\\'
#define PATH_SEP_STR RSSTR("\\")
#define PATH_MAX MAX_PATH
#else
#define RSMaxPathSize ((RSIndex)1026)
#define RSMaxPathLength ((RSIndex)1024)
#define PATH_SEP '/'
#define PATH_SEP_STR RSSTR("/")
#endif


#if DEPLOYMENT_TARGET_MACOSX
#include <xlocale.h>
#endif

RSHashCode RSHashBytes(RSBitU8* bytes, RSBit32 length);

#if DEPLOYMENT_TARGET_WINDOWS
#define kNilPthreadT  { nil, nil }
#else
#define kNilPthreadT  (pthread_t)0
#endif

#if defined(__BIG_ENDIAN__)
#define __RS_BIG_ENDIAN__ 1
#define __RS_LITTLE_ENDIAN__ 0
#endif

#if defined(__LITTLE_ENDIAN__)
#define __RS_LITTLE_ENDIAN__ 1
#define __RS_BIG_ENDIAN__ 0
#endif

enum {
	__RSTSDKeyAllocator = 1,
	__RSTSDKeyIsInRSLog = 2,
	__RSTSDKeyIsInNSCache = 3,
	__RSTSDKeyIsInGCDMainQ = 4,
	__RSTSDKeyICUConverter = 7,
	__RSTSDKeyCollatorLocale = 8,
	__RSTSDKeyCollatorUCollator = 9,
	__RSTSDKeyRunLoop = 10,
	__RSTSDKeyRunLoopCntr = 11,
	// autorelease pool stuff must be higher than run loop constants
	__RSTSDKeyAutoreleaseData2 = 61,
	__RSTSDKeyAutoreleaseData1 = 62,
	__RSTSDKeyExceptionData = 63,
};

RSExtern int       pthread_key_init_np(int, void (*)(void *));
RSExtern pthread_t _RSMainPThread;
RSExtern void *_RSGetTSD(uint32_t slot);
typedef void (*tsdDestructor)(void *);
RSExtern void *_RSSetTSD(uint32_t slot, void *newVal, tsdDestructor destructor);


RSExtern RSAllocatorRef RSAllocatorSystemDefault;
#define RSAllocatorNull ((RSAllocatorRef)nil)

RSExtern const void *__RSStringCollectionCopy(RSAllocatorRef allocator, const void *ptr);
RSExtern const void *__RSTypeCollectionRetain(RSAllocatorRef allocator, const void *ptr);
RSExtern void __RSTypeCollectionRelease(RSAllocatorRef allocator, const void *ptr);

RSExtern RSTypeRef __RSAllocatorSetHeader(RSAllocatorRef allocator, RSHandle* zone, RSIndex size);

RSExtern BOOL __RSStringEncodingAvailable(RSStringEncoding encoding);
//RSPrivate void RSStringSetEncoding(RSStringRef string, RSStringEncoding encoding);
RSPrivate void __RSSetCharToUniCharFunc(BOOL (*func)(UInt32 flags, UInt8 ch, UniChar *unicodeChar));
RSPrivate void __RSStrConvertBytesToUnicode(const uint8_t *bytes, UniChar *buffer, RSIndex numChars);
enum {
    __RSVarWidthLocalBufferSize = 1008
};

typedef struct {      /* A simple struct to maintain ASCII/Unicode versions of the same buffer. */
    union {
        UInt8 *ascii;
        UniChar *unicode;
    } chars;
    BOOL isASCII;	/* This really does mean 7-bit ASCII, not _NSDefaultCStringEncoding() */
    BOOL shouldFreeChars;	/* If the number of bytes exceeds __RSVarWidthLocalBufferSize, bytes are allocated */
    BOOL _unused1;
    BOOL _unused2;
    RSAllocatorRef allocator;	/* Use this allocator to allocate, reallocate, and deallocate the bytes */
    RSIndex numChars;	/* This is in terms of ascii or unicode; that is, if isASCII, it is number of 7-bit chars; otherwise it is number of UniChars; note that the actual allocated space might be larger */
    UInt8 localBuffer[__RSVarWidthLocalBufferSize];	/* private; 168 ISO2022JP chars, 504 Unicode chars, 1008 ASCII chars */
} RSVarWidthCharBuffer;
enum {_RSStringErrNone = 0, _RSStringErrNotMutable = 1, _RSStringErrNilArg = 2, _RSStringErrBounds = 3};

/* Convert a byte stream to ASCII (7-bit!) or Unicode, with a RSVarWidthCharBuffer struct on the stack. NO return indicates an error occured during the conversion. Depending on .isASCII, follow .chars.ascii or .chars.unicode.  If .shouldFreeChars is returned as YES, free the returned buffer when done with it.  If useClientsMemoryPtr is provided as non-NULL, and the provided memory can be used as is, this is set to YES, and the .ascii or .unicode buffer in RSVarWidthCharBuffer is set to bytes.
 !!! If the stream is Unicode and has no BOM, the data is assumed to be big endian! Could be trouble on Intel if someone didn't follow that assumption.
 !!! __RSStringDecodeByteStream2() needs to be deprecated and removed post-Jaguar.
 */
RSExport BOOL __RSStringDecodeByteStream2(const UInt8 *bytes, UInt32 len, RSStringEncoding encoding, BOOL alwaysUnicode, RSVarWidthCharBuffer *buffer, BOOL *useClientsMemoryPtr);
RSExport BOOL __RSStringDecodeByteStream3(const UInt8 *bytes, RSIndex len, RSStringEncoding encoding, BOOL alwaysUnicode, RSVarWidthCharBuffer *buffer, BOOL *useClientsMemoryPtr, UInt32 converterFlags);

#include <RSCoreFoundation/RSDictionary.h>
RSExtern void __RSDictionaryCleanAllKeysAndObjects(RSMutableDictionaryRef dictionary);
#include <RSCoreFoundation/RSAutoreleasePool.h>

#define RSExceptionKey          205
#define RSAutoreleasePageKey    204
#define RSRunLoopKey            203
RSExtern void __RSAutoreleasePoolReleaseUntil(RSAutoreleasePoolRef pool, RSTypeRef *stop);

RSExtern RSStringRef __RSRunLoopThreadToString(pthread_t t);

RSExtern const RSDictionaryKeyContext ___RSDictionaryNilKeyContext;
RSExtern const RSDictionaryValueContext ___RSDictionaryNilValueContext;
RSExtern const RSDictionaryKeyContext ___RSDictionaryRSKeyContext;
RSExtern const RSDictionaryValueContext ___RSDictionaryRSValueContext;
RSExtern const RSDictionaryKeyContext ___RSConstantCStringKeyContext;

RSExtern void __RSRuntimeMemoryBarrier(void);
RSExtern void RSAllocatorLogUnitCount(void);
RSExtern void *__RSStartSimpleThread(void (*func)(void *), void *arg);
RSExtern void _RSRunLoopRunBackGround(void);

struct _RSRuntimeLogPreference {
#define DEBUG_PERFERENCE(Prefix, Name, BitWidth, Default, Comment) RSUInteger Prefix##Name : BitWidth ;
#include <RSCoreFoundation/RSRuntimeDebugSupport.h>
#if __LP64__
    RSUInteger _RSReserved : 64 - 12;
#else
    RSUInteger _RSReserved : 32 - 12;
#endif
};
RSExtern struct _RSRuntimeLogPreference ___RSDebugLogPreference;

#include <RSCoreFoundation/RSRunLoop.h>
RSExtern void *__RSRunLoopGetQueue(RSRunLoopRef rl);

RSExtern BOOL _RSStringGetFileSystemRepresentation(RSStringRef string, uint8_t *buffer, RSIndex maxBufLen);
//RSExtern BOOL __RSStringScanInteger(RSStringInlineBuffer *buf, RSTypeRef locale, SInt32 *indexPtr, BOOL doLonglong, void *result);

RSExtern BOOL _RSExecutableLinkedOnOrAfter(RSUInteger v);

RSExtern RSUInteger RSRunLoopRunSpecific(RSRunLoopRef rl, RSStringRef modeName, RSTimeInterval seconds, BOOL returnAfterSourceHandled);
#ifndef __RSNotificationIPCName
#define __RSNotificationIPCName "com.retval.RSCoreFoundation.RSNotificationCenter"
#endif

#define FAULT_CALLBACK(V)
#define INVOKE_CALLBACK1(P, A) (P)(A)
#define INVOKE_CALLBACK2(P, A, B) (P)(A, B)
#define INVOKE_CALLBACK3(P, A, B, C) (P)(A, B, C)
#define INVOKE_CALLBACK4(P, A, B, C, D) (P)(A, B, C, D)
#define INVOKE_CALLBACK5(P, A, B, C, D, E) (P)(A, B, C, D, E)
#define UNFAULT_CALLBACK(V) do { } while (0)

#define RS_IS_COLLECTABLE_ALLOCATOR(allocator)    (nil && (NULL == (allocator) || RSAllocatorSystemDefault == (allocator) || nil))

#if DEPLOYMENT_TARGET_WINDOWS
#define STACK_BUFFER_DECL(T, N, C) T *N = (T *)_alloca((C) * sizeof(T))
#else
#define STACK_BUFFER_DECL(T, N, C) T N[C]
#endif

#if defined(__GNUC__) && !defined(__STRICT_ANSI__)
#define __RSMin(A,B) ({__typeof__(A) __a = (A); __typeof__(B) __b = (B); __a < __b ? __a : __b; })
#define __RSMax(A,B) ({__typeof__(A) __a = (A); __typeof__(B) __b = (B); __a < __b ? __b : __a; })
#else /* __GNUC__ */
#define __RSMin(A,B) ((A) < (B) ? (A) : (B))
#define __RSMax(A,B) ((A) > (B) ? (A) : (B))
#endif /* __GNUC__ */

#include <RSCoreFoundation/RSCharacterSet.h>

typedef struct {
    RSCharacterSetRef cset;
    uint32_t flags;
    uint32_t rangeStart;
    uint32_t rangeLimit;
    const uint8_t *bitmap;
} RSCharacterSetInlineBuffer;

// Bits for flags field
enum {
    RSCharacterSetIsCompactBitmap = (1UL << 0),
    RSCharacterSetNoBitmapAvailable = (1UL << 1),
    RSCharacterSetIsInverted = (1UL << 2)
};

/*!
 @function RSCharacterSetInitInlineBuffer
 Initializes buffer with cset.
 @param cset The character set used to initialized the buffer.
 If this parameter is not a valid RSCharacterSet, the behavior is undefined.
 @param buffer The reference to the inline buffer to be initialized.
 */
RS_EXPORT
void RSCharacterSetInitInlineBuffer(RSCharacterSetRef cset, RSCharacterSetInlineBuffer *buffer);

/*!
 @function RSCharacterSetInlineBufferIsLongCharacterMember
 Reports whether or not the UTF-32 character is in the character set.
 @param buffer The reference to the inline buffer to be searched.
 @param character The UTF-32 character for which to test against the
 character set.
 @result YES, if the value is in the character set, otherwise NO.
 */
#if defined(RSInline)
RSInline bool RSCharacterSetInlineBufferIsLongCharacterMember(RSCharacterSetInlineBuffer *buffer, UTF32Char character) {
    bool isInverted = ((0 == (buffer->flags & RSCharacterSetIsInverted)) ? NO : YES);
    
    if ((character >= buffer->rangeStart) && (character < buffer->rangeLimit)) {
        if ((character > 0xFFFF) || (0 != (buffer->flags & RSCharacterSetNoBitmapAvailable))) return (RSCharacterSetIsLongCharacterMember(buffer->cset, character) != 0);
        if (NULL == buffer->bitmap) {
            if (0 == (buffer->flags & RSCharacterSetIsCompactBitmap)) isInverted = !isInverted;
        } else if (0 == (buffer->flags & RSCharacterSetIsCompactBitmap)) {
            if (buffer->bitmap[character >> 3] & (1UL << (character & 7))) isInverted = !isInverted;
        } else {
            uint8_t value = buffer->bitmap[character >> 8];
            
            if (value == 0xFF) {
                isInverted = !isInverted;
            } else if (value > 0) {
                const uint8_t *segment = buffer->bitmap + (256 + (32 * (value - 1)));
                character &= 0xFF;
                if (segment[character >> 3] & (1UL << (character % 8))) isInverted = !isInverted;
            }
        }
    }
    return isInverted;
}
#else /* RSInline */
#define RSCharacterSetInlineBufferIsLongCharacterMember(buffer, character) (RSCharacterSetIsLongCharacterMember(buffer->cset, character))
#endif /* RSInline */

#if DEPLOYMENT_TARGET_MACOSX || DEPLOYMENT_TARGET_EMBEDDED || DEPLOYMENT_TARGET_EMBEDDED_MINI
RSExtern uint8_t __RS120293;
RSExtern uint8_t __RS120290;
RSExtern void __THE_PROCESS_HAS_FORKED_AND_YOU_CANNOT_USE_THIS_COREFOUNDATION_FUNCTIONALITY___YOU_MUST_EXEC__(void);
#define CHECK_FOR_FORK() do { __RS120290 = true; if (__RS120293) __THE_PROCESS_HAS_FORKED_AND_YOU_CANNOT_USE_THIS_COREFOUNDATION_FUNCTIONALITY___YOU_MUST_EXEC__(); } while (0)
#define CHECK_FOR_FORK_RET(...) do { CHECK_FOR_FORK(); if (__RS120293) return __VA_ARGS__; } while (0)
#define HAS_FORKED() (__RS120293)
#endif

#if !defined(CHECK_FOR_FORK)
#define CHECK_FOR_FORK() do { } while (0)
#endif

#if !defined(CHECK_FOR_FORK_RET)
#define CHECK_FOR_FORK_RET(...) do { } while (0)
#endif

#if !defined(HAS_FORKED)
#define HAS_FORKED() 0
#endif

RSExport RSMutableArrayRef __RSArrayCreateMutable0(RSAllocatorRef allocator, RSIndex capacity, const RSArrayCallBacks *callBacks);
RSPrivate RSArrayRef __RSArrayCreateWithArguments(va_list arguments, RSInteger maxCount);
enum {
    RSCompareIgnoreNonAlphanumeric = (1UL << 16), // Ignores characters NOT in kRSCharacterSetAlphaNumeric
};

typedef RS_ENUM(RSIndex, RSStringCharacterClusterType) {
    RSStringGraphemeCluster = 1, /* Unicode Grapheme Cluster */
    RSStringComposedCharacterCluster = 2, /* Compose all non-base (including spacing marks) */
    RSStringCursorMovementCluster = 3, /* Cluster suitable for cursor movements */
    RSStringBackwardDeletionCluster = 4 /* Cluster suitable for backward deletion */
};

RSExport void __RSDumpAllPointerLocations(uintptr_t ptr) RS_AVAILABLE(0_5);
RS_EXTERN_C_END
#endif
