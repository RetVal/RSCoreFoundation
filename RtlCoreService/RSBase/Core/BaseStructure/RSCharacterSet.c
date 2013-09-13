//
//  RSCharacterSet.c
//  RSCoreFoundation
//
//  Created by RetVal on 8/5/13.
//  Copyright (c) 2013 RetVal. All rights reserved.
//

#include "RSCharacterSet.h"

#include <RSCoreFoundation/RSRuntime.h>

#include <RSCoreFoundation/RSByteOrder.h>
RS_EXTERN_C_BEGIN

/*!
 @function RSCharacterSetIsSurrogateHighCharacter
 Reports whether or not the character is a high surrogate.
 @param character  The character to be checked.
 @result YES, if character is a high surrogate, otherwise NO.
 */
RSInline BOOL RSCharacterSetIsSurrogateHighCharacter(UniChar character) {
    return ((character >= 0xD800UL) && (character <= 0xDBFFUL) ? YES : NO);
}

/*!
 @function RSCharacterSetIsSurrogateLowCharacter
 Reports whether or not the character is a low surrogate.
 @param character  The character to be checked.
 @result YES, if character is a low surrogate, otherwise NO.
 */
RSInline BOOL RSCharacterSetIsSurrogateLowCharacter(UniChar character) {
    return ((character >= 0xDC00UL) && (character <= 0xDFFFUL) ? YES : NO);
}

/*!
 @function RSCharacterSetGetLongCharacterForSurrogatePair
 Returns the UTF-32 value corresponding to the surrogate pair passed in.
 @param surrogateHigh  The high surrogate character.  If this parameter
 is not a valid high surrogate character, the behavior is undefined.
 @param surrogateLow  The low surrogate character.  If this parameter
 is not a valid low surrogate character, the behavior is undefined.
 @result The UTF-32 value for the surrogate pair.
 */
RSInline UTF32Char RSCharacterSetGetLongCharacterForSurrogatePair(UniChar surrogateHigh, UniChar surrogateLow) {
    return (UTF32Char)(((surrogateHigh - 0xD800UL) << 10) + (surrogateLow - 0xDC00UL) + 0x0010000UL);
}

/* Check to see if the character represented by the surrogate pair surrogateHigh & surrogateLow is in the chraracter set */
RSExport BOOL RSCharacterSetIsSurrogatePairMember(RSCharacterSetRef theSet, UniChar surrogateHigh, UniChar surrogateLow) ;

/* Keyed-coding support
 */
typedef RS_ENUM(RSIndex, RSCharacterSetKeyedCodingType) {
    RSCharacterSetKeyedCodingTypeBitmap = 1,
    RSCharacterSetKeyedCodingTypeBuiltin = 2,
    RSCharacterSetKeyedCodingTypeRange = 3,
    RSCharacterSetKeyedCodingTypeString = 4,
    RSCharacterSetKeyedCodingTypeBuiltinAndBitmap = 5
};
    
RSExport RSCharacterSetKeyedCodingType _RSCharacterSetGetKeyedCodingType(RSCharacterSetRef cset);
RSExport RSCharacterSetPredefinedSet _RSCharacterSetGetKeyedCodingBuiltinType(RSCharacterSetRef cset);
RSExport RSRange _RSCharacterSetGetKeyedCodingRange(RSCharacterSetRef cset);
RSExport RSStringRef _RSCharacterSetCreateKeyedCodingString(RSCharacterSetRef cset);
RSExport BOOL _RSCharacterSetIsInverted(RSCharacterSetRef cset);
RSExport void _RSCharacterSetSetIsInverted(RSCharacterSetRef cset, BOOL flag);

RS_EXTERN_C_END

#include <RSCoreFoundation/RSData.h>
#include <RSCoreFoundation/RSString.h>
#include "RSInternal.h"
#include "RSPrivate/CString/RSString/RSUniChar.h"
#include "RSPrivate/CString/RSString/RSUniCharPrivate.h"
#include <stdlib.h>
#include <string.h>

#define BITSPERBYTE	8	/* (CHAR_BIT * sizeof(unsigned char)) */
#define LOG_BPB		3
#define LOG_BPLW	5
#define NUMCHARACTERS	65536

#define MAX_ANNEX_PLANE	(16)

/* Number of things in the array keeping the bits.
 */
#define __RSBitmapSize		(NUMCHARACTERS / BITSPERBYTE)

/* How many elements max can be in an __RSCharSetClassString RSCharacterSet
 */
#define __RSStringCharSetMax 64

/* The last builtin set ID number
 */
#define __RSLastBuiltinSetID RSCharacterSetNewline

/* How many elements in the "singles" array before we use binary search.
 */
#define __RSSetBreakeven 10

/* This tells us, within 1k or so, whether a thing is POTENTIALLY in the set (in the bitmap blob of the private structure) before we bother to do specific checking.
 */
#define __RSCSetBitsInRange(n, i)	(i[n>>15] & (1L << ((n>>10) % 32)))

/* Compact bitmap params
 */
#define __RSCompactBitmapNumPages (256)

#define __RSCompactBitmapMaxPages (128) // the max pages allocated

#define __RSCompactBitmapPageSize (__RSBitmapSize / __RSCompactBitmapNumPages)

typedef struct {
    RSCharacterSetRef *_nonBMPPlanes;
    unsigned int _validEntriesBitmap;
    unsigned char _numOfAllocEntries;
    unsigned char _isAnnexInverted;
    uint16_t _padding;
} RSCharSetAnnexStruct;

struct __RSCharacterSet {
    RSRuntimeBase _base;
    RSHashCode _hashValue;
    union {
        struct {
            RSIndex _type;
        } _builtin;
        struct {
            UInt32 _firstChar;
            RSIndex _length;
        } _range;
        struct {
            UniChar *_buffer;
            RSIndex _length;
        } _string;
        struct {
            uint8_t *_bits;
        } _bitmap;
        struct {
            uint8_t *_cBits;
        } _compactBitmap;
    } _variants;
    RSCharSetAnnexStruct *_annex;
};

/* _base._info values interesting for RSCharacterSet
 */
enum {
    __RSCharSetClassTypeMask = 0x0070,
    __RSCharSetClassBuiltin = 0x0000,
    __RSCharSetClassRange = 0x0010,
    __RSCharSetClassString = 0x0020,
    __RSCharSetClassBitmap = 0x0030,
    __RSCharSetClassSet = 0x0040,
    __RSCharSetClassCompactBitmap = 0x0040,
    
    __RSCharSetIsInvertedMask = 0x0008,
    __RSCharSetIsInverted = 0x0008,
    
    __RSCharSetHasHashValueMask = 0x00004,
    __RSCharSetHasHashValue = 0x0004,
    
    /* Generic RSBase values */
    __RSCharSetIsMutableMask = 0x0001,
    __RSCharSetIsMutable = 0x0001,
};

/* Inline accessor macros for _base._info
 */
RSInline BOOL __RSCSetIsMutable(RSCharacterSetRef cset) {return (RSRuntimeClassBaseFiled(cset) & __RSCharSetIsMutableMask) == __RSCharSetIsMutable;}
RSInline BOOL __RSCSetIsBuiltin(RSCharacterSetRef cset) {return (RSRuntimeClassBaseFiled(cset) & __RSCharSetClassTypeMask) == __RSCharSetClassBuiltin;}
RSInline BOOL __RSCSetIsRange(RSCharacterSetRef cset) {return (RSRuntimeClassBaseFiled(cset) & __RSCharSetClassTypeMask) == __RSCharSetClassRange;}
RSInline BOOL __RSCSetIsString(RSCharacterSetRef cset) {return (RSRuntimeClassBaseFiled(cset) & __RSCharSetClassTypeMask) == __RSCharSetClassString;}
RSInline BOOL __RSCSetIsBitmap(RSCharacterSetRef cset) {return (RSRuntimeClassBaseFiled(cset) & __RSCharSetClassTypeMask) == __RSCharSetClassBitmap;}
RSInline BOOL __RSCSetIsCompactBitmap(RSCharacterSetRef cset) {return (RSRuntimeClassBaseFiled(cset) & __RSCharSetClassTypeMask) == __RSCharSetClassCompactBitmap;}
RSInline BOOL __RSCSetIsInverted(RSCharacterSetRef cset) {return (RSRuntimeClassBaseFiled(cset) & __RSCharSetIsInvertedMask) == __RSCharSetIsInverted;}
RSInline BOOL __RSCSetHasHashValue(RSCharacterSetRef cset) {return (RSRuntimeClassBaseFiled(cset) & __RSCharSetHasHashValueMask) == __RSCharSetHasHashValue;}
RSInline UInt32 __RSCSetClassType(RSCharacterSetRef cset) {return (RSRuntimeClassBaseFiled(cset) & __RSCharSetClassTypeMask);}

RSInline void __RSCSetPutIsMutable(RSMutableCharacterSetRef cset, BOOL isMutable) {(isMutable ? (RSRuntimeClassBaseFiled(cset) |= __RSCharSetIsMutable) : (RSRuntimeClassBaseFiled(cset) &= ~ __RSCharSetIsMutable));}
RSInline void __RSCSetPutIsInverted(RSMutableCharacterSetRef cset, BOOL isInverted) {(isInverted ? (RSRuntimeClassBaseFiled(cset) |= __RSCharSetIsInverted) : (RSRuntimeClassBaseFiled(cset) &= ~__RSCharSetIsInverted));}
RSInline void __RSCSetPutHasHashValue(RSMutableCharacterSetRef cset, BOOL hasHash) {(hasHash ? (RSRuntimeClassBaseFiled(cset) |= __RSCharSetHasHashValue) : (RSRuntimeClassBaseFiled(cset) &= ~__RSCharSetHasHashValue));}
RSInline void __RSCSetPutClassType(RSMutableCharacterSetRef cset, UInt32 classType) {RSRuntimeClassBaseFiled(cset) &= ~__RSCharSetClassTypeMask;  RSRuntimeClassBaseFiled(cset) |= classType;}

__private_extern__ BOOL __RSCharacterSetIsMutable(RSCharacterSetRef cset) {return __RSCSetIsMutable(cset);}

/* Inline contents accessor macros
 */
RSInline RSCharacterSetPredefinedSet __RSCSetBuiltinType(RSCharacterSetRef cset) {return cset->_variants._builtin._type;}
RSInline UInt32 __RSCSetRangeFirstChar(RSCharacterSetRef cset) {return cset->_variants._range._firstChar;}
RSInline RSIndex __RSCSetRangeLength(RSCharacterSetRef cset) {return cset->_variants._range._length;}
RSInline UniChar *__RSCSetStringBuffer(RSCharacterSetRef cset) {return (UniChar*)(cset->_variants._string._buffer);}
RSInline RSIndex __RSCSetStringLength(RSCharacterSetRef cset) {return cset->_variants._string._length;}
RSInline uint8_t *__RSCSetBitmapBits(RSCharacterSetRef cset) {return cset->_variants._bitmap._bits;}
RSInline uint8_t *__RSCSetCompactBitmapBits(RSCharacterSetRef cset) {return cset->_variants._compactBitmap._cBits;}

RSInline void __RSCSetPutBuiltinType(RSMutableCharacterSetRef cset, RSCharacterSetPredefinedSet type) {cset->_variants._builtin._type = type;}
RSInline void __RSCSetPutRangeFirstChar(RSMutableCharacterSetRef cset, UInt32 first) {cset->_variants._range._firstChar = first;}
RSInline void __RSCSetPutRangeLength(RSMutableCharacterSetRef cset, RSIndex length) {cset->_variants._range._length = length;}
RSInline void __RSCSetPutStringBuffer(RSMutableCharacterSetRef cset, UniChar *theBuffer) {cset->_variants._string._buffer = theBuffer;}
RSInline void __RSCSetPutStringLength(RSMutableCharacterSetRef cset, RSIndex length) {cset->_variants._string._length = length;}
RSInline void __RSCSetPutBitmapBits(RSMutableCharacterSetRef cset, uint8_t *bits) {cset->_variants._bitmap._bits = bits;}
RSInline void __RSCSetPutCompactBitmapBits(RSMutableCharacterSetRef cset, uint8_t *bits) {cset->_variants._compactBitmap._cBits = bits;}

/* Validation funcs
 */
#if defined(RS_ENABLE_ASSERTIONS)
RSInline void __RSCSetValidateBuiltinType(RSCharacterSetPredefinedSet type, const char *func) {
    RSAssert2(type > 0 && type <= __RSLastBuiltinSetID, __RSLogAssertion, "%s: Unknowen builtin type %d", func, type);
}
RSInline void __RSCSetValidateRange(RSRange theRange, const char *func) {
    RSAssert3(theRange.location >= 0 && theRange.location + theRange.length <= 0x1FFFFF, __RSLogAssertion, "%s: Range out of Unicode range (location -> %d length -> %d)", func, theRange.location, theRange.length);
}
RSInline void __RSCSetValidateTypeAndMutability(RSCharacterSetRef cset, const char *func) {
    __RSGenericValidInstance(cset, __RSCharacterSetTypeID);
    RSAssert1(__RSCSetIsMutable(cset), __RSLogAssertion, "%s: Immutable character set passed to mutable function", func);
}
#else
#define __RSCSetValidateBuiltinType(t,f)
#define __RSCSetValidateRange(r,f)
#define __RSCSetValidateTypeAndMutability(r,f)
#endif

/* Inline utility funcs
 */
static BOOL __RSCSetIsEqualBitmap(const UInt32 *bits1, const UInt32 *bits2) {
    RSIndex length = __RSBitmapSize / sizeof(UInt32);
    
    if (bits1 == bits2) {
        return YES;
    } else if (bits1 && bits2) {
        if (bits1 == (const UInt32 *)-1) {
            while (length--) if ((UInt32)-1 != *bits2++) return NO;
        } else if (bits2 == (const UInt32 *)-1) {
            while (length--) if ((UInt32)-1 != *bits1++) return NO;
        } else {
            while (length--) if (*bits1++ != *bits2++) return NO;
        }
        return YES;
    } else if (!bits1 && !bits2) { // empty set
        return YES;
    } else {
        if (bits2) bits1 = bits2;
        if (bits1 == (const UInt32 *)-1) return NO;
        while (length--) if (*bits1++) return NO;
        return YES;
    }
}

RSInline BOOL __RSCSetIsEqualBitmapInverted(const UInt32 *bits1, const UInt32 *bits2) {
    RSIndex length = __RSBitmapSize / sizeof(UInt32);
    
    while (length--) if (*bits1++ != ~(*(bits2++))) return NO;
    return YES;
}

static BOOL __RSCSetIsBitmapEqualToRange(const UInt32 *bits, UniChar firstChar, UniChar lastChar, BOOL isInverted) {
    RSIndex firstCharIndex = firstChar >> LOG_BPB;
    RSIndex lastCharIndex = lastChar >> LOG_BPB;
    RSIndex length;
    UInt32 value;
    
    if (firstCharIndex == lastCharIndex) {
        value = ((((UInt32)0xFF) << (firstChar & (BITSPERBYTE - 1))) & (((UInt32)0xFF) >> ((BITSPERBYTE - 1) - (lastChar & (BITSPERBYTE - 1))))) << (((sizeof(UInt32) - 1) - (firstCharIndex % sizeof(UInt32))) * BITSPERBYTE);
        value = RSSwapInt32HostToBig(value);
        firstCharIndex = lastCharIndex = firstChar >> LOG_BPLW;
        if (*(bits + firstCharIndex) != (isInverted ? ~value : value)) return NO;
    } else {
        UInt32 firstCharMask;
        UInt32 lastCharMask;
        
        length = firstCharIndex % sizeof(UInt32);
        firstCharMask = (((((UInt32)0xFF) << (firstChar & (BITSPERBYTE - 1))) & 0xFF) << (((sizeof(UInt32) - 1) - length) * BITSPERBYTE)) | (((UInt32)0xFFFFFFFF) >> ((length + 1) * BITSPERBYTE));
        
        length = lastCharIndex % sizeof(UInt32);
        lastCharMask = ((((UInt32)0xFF) >> ((BITSPERBYTE - 1) - (lastChar & (BITSPERBYTE - 1)))) << (((sizeof(UInt32) - 1) - length) * BITSPERBYTE)) | (((UInt32)0xFFFFFFFF) << ((sizeof(UInt32) - length) * BITSPERBYTE));
        
        firstCharIndex = firstChar >> LOG_BPLW;
        lastCharIndex = lastChar >> LOG_BPLW;
        
        if (firstCharIndex == lastCharIndex) {
            firstCharMask &= lastCharMask;
            value = RSSwapInt32HostToBig(firstCharMask & lastCharMask);
            if (*(bits + firstCharIndex) != (isInverted ? ~value : value)) return NO;
        } else {
            value = RSSwapInt32HostToBig(firstCharMask);
            if (*(bits + firstCharIndex) != (isInverted ? ~value : value)) return NO;
            
            value = RSSwapInt32HostToBig(lastCharMask);
            if (*(bits + lastCharIndex) != (isInverted ? ~value : value)) return NO;
        }
    }
    
    length = firstCharIndex;
    value = (isInverted ? ((UInt32)0xFFFFFFFF) : 0);
    while (length--) {
        if (*(bits++) != value) return NO;
    }
    
    ++bits; // Skip firstCharIndex
    length = (lastCharIndex - (firstCharIndex + 1));
    value = (isInverted ? 0 : ((UInt32)0xFFFFFFFF));
    while (length-- > 0) {
        if (*(bits++) != value) return NO;
    }
    if (firstCharIndex != lastCharIndex) ++bits;
    
    length = (0xFFFF >> LOG_BPLW) - lastCharIndex;
    value = (isInverted ? ((UInt32)0xFFFFFFFF) : 0);
    while (length--) {
        if (*(bits++) != value) return NO;
    }
    
    return YES;
}

RSInline BOOL __RSCSetIsBitmapSupersetOfBitmap(const UInt32 *bits1, const UInt32 *bits2, BOOL isInverted1, BOOL isInverted2) {
    RSIndex length = __RSBitmapSize / sizeof(UInt32);
    UInt32 val1, val2;
    
    while (length--) {
        val2 = (isInverted2 ? ~(*(bits2++)) : *(bits2++));
        val1 = (isInverted1 ? ~(*(bits1++)) : *(bits1++)) & val2;
        if (val1 != val2) return NO;
    }
    
    return YES;
}

RSInline BOOL __RSCSetHasNonBMPPlane(RSCharacterSetRef cset) { return ((cset)->_annex && (cset)->_annex->_validEntriesBitmap ? YES : NO); }
RSInline BOOL __RSCSetAnnexIsInverted (RSCharacterSetRef cset) { return ((cset)->_annex && (cset)->_annex->_isAnnexInverted ? YES : NO); }
RSInline UInt32 __RSCSetAnnexValidEntriesBitmap(RSCharacterSetRef cset) { return ((cset)->_annex ? (cset)->_annex->_validEntriesBitmap : 0); }

RSInline BOOL __RSCSetIsEmpty(RSCharacterSetRef cset) {
    if (__RSCSetHasNonBMPPlane(cset) || __RSCSetAnnexIsInverted(cset)) return NO;
    
    switch (__RSCSetClassType(cset)) {
        case __RSCharSetClassRange: if (!__RSCSetRangeLength(cset)) return YES; break;
        case __RSCharSetClassString: if (!__RSCSetStringLength(cset)) return YES; break;
        case __RSCharSetClassBitmap: if (!__RSCSetBitmapBits(cset)) return YES; break;
        case __RSCharSetClassCompactBitmap: if (!__RSCSetCompactBitmapBits(cset)) return YES; break;
    }
    return NO;
}

RSInline void __RSCSetBitmapAddCharacter(uint8_t *bitmap, UniChar theChar) {
    bitmap[(theChar) >> LOG_BPB] |= (((unsigned)1) << (theChar & (BITSPERBYTE - 1)));
}

RSInline void __RSCSetBitmapRemoveCharacter(uint8_t *bitmap, UniChar theChar) {
    bitmap[(theChar) >> LOG_BPB] &= ~(((unsigned)1) << (theChar & (BITSPERBYTE - 1)));
}

RSInline BOOL __RSCSetIsMemberBitmap(const uint8_t *bitmap, UniChar theChar) {
    return ((bitmap[(theChar) >> LOG_BPB] & (((unsigned)1) << (theChar & (BITSPERBYTE - 1)))) ? YES : NO);
}

#define NUM_32BIT_SLOTS	(NUMCHARACTERS / 32)

RSInline void __RSCSetBitmapFastFillWithValue(UInt32 *bitmap, uint8_t value) {
    UInt32 mask = (value << 24) | (value << 16) | (value << 8) | value;
    UInt32 numSlots = NUMCHARACTERS / 32;
    
    while (numSlots--) *(bitmap++) = mask;
}

RSInline void __RSCSetBitmapAddCharactersInRange(uint8_t *bitmap, UniChar firstChar, UniChar lastChar) {
    if (firstChar == lastChar) {
        bitmap[firstChar >> LOG_BPB] |= (((unsigned)1) << (firstChar & (BITSPERBYTE - 1)));
    } else {
        UInt32 idx = firstChar >> LOG_BPB;
        UInt32 max = lastChar >> LOG_BPB;
        
        if (idx == max) {
            bitmap[idx] |= (((unsigned)0xFF) << (firstChar & (BITSPERBYTE - 1))) & (((unsigned)0xFF) >> ((BITSPERBYTE - 1) - (lastChar & (BITSPERBYTE - 1))));
        } else {
            bitmap[idx] |= (((unsigned)0xFF) << (firstChar & (BITSPERBYTE - 1)));
            bitmap[max] |= (((unsigned)0xFF) >> ((BITSPERBYTE - 1) - (lastChar & (BITSPERBYTE - 1))));
            
            ++idx;
            while (idx < max) bitmap[idx++] = 0xFF;
        }
    }
}

RSInline void __RSCSetBitmapRemoveCharactersInRange(uint8_t *bitmap, UniChar firstChar, UniChar lastChar) {
    UInt32 idx = firstChar >> LOG_BPB;
    UInt32 max = lastChar >> LOG_BPB;
    
    if (idx == max) {
        bitmap[idx] &= ~((((unsigned)0xFF) << (firstChar & (BITSPERBYTE - 1))) & (((unsigned)0xFF) >> ((BITSPERBYTE - 1) - (lastChar & (BITSPERBYTE - 1)))));
    } else {
        bitmap[idx] &= ~(((unsigned)0xFF) << (firstChar & (BITSPERBYTE - 1)));
        bitmap[max] &= ~(((unsigned)0xFF) >> ((BITSPERBYTE - 1) - (lastChar & (BITSPERBYTE - 1))));
        
        ++idx;
        while (idx < max) bitmap[idx++] = 0;
    }
}

#define __RSCSetAnnexBitmapSetPlane(bitmap,plane)	((bitmap) |= (1 << (plane)))
#define __RSCSetAnnexBitmapClearPlane(bitmap,plane)	((bitmap) &= (~(1 << (plane))))
#define __RSCSetAnnexBitmapGetPlane(bitmap,plane)	((bitmap) & (1 << (plane)))

RSInline void __RSCSetAllocateAnnexForPlane(RSCharacterSetRef cset, int plane) {
    if (cset->_annex == NULL) {
        ((RSMutableCharacterSetRef)cset)->_annex = (RSCharSetAnnexStruct *)RSAllocatorAllocate(RSGetAllocator(cset), sizeof(RSCharSetAnnexStruct));
        cset->_annex->_numOfAllocEntries = plane;
        cset->_annex->_isAnnexInverted = NO;
        cset->_annex->_validEntriesBitmap = 0;
        cset->_annex->_nonBMPPlanes = ((plane > 0) ? (RSCharacterSetRef*)RSAllocatorAllocate(RSGetAllocator(cset), sizeof(RSCharacterSetRef) * plane) : NULL);
    } else if (cset->_annex->_numOfAllocEntries < plane) {
        cset->_annex->_numOfAllocEntries = plane;
        if (NULL == cset->_annex->_nonBMPPlanes) {
            cset->_annex->_nonBMPPlanes = (RSCharacterSetRef*)RSAllocatorAllocate(RSGetAllocator(cset), sizeof(RSCharacterSetRef) * plane);
        } else {
            cset->_annex->_nonBMPPlanes = (RSCharacterSetRef*)RSAllocatorReallocate(RSGetAllocator(cset), (void *)cset->_annex->_nonBMPPlanes, sizeof(RSCharacterSetRef) * plane);
        }
    }
}

RSInline void __RSCSetAnnexSetIsInverted(RSCharacterSetRef cset, BOOL flag) {
    if (flag) __RSCSetAllocateAnnexForPlane(cset, 0);
    if (cset->_annex) ((RSMutableCharacterSetRef)cset)->_annex->_isAnnexInverted = flag;
}

RSInline void __RSCSetPutCharacterSetToAnnexPlane(RSCharacterSetRef cset, RSCharacterSetRef annexCSet, int plane) {
    __RSCSetAllocateAnnexForPlane(cset, plane);
    if (__RSCSetAnnexBitmapGetPlane(cset->_annex->_validEntriesBitmap, plane)) RSRelease(cset->_annex->_nonBMPPlanes[plane - 1]);
    if (annexCSet) {
        cset->_annex->_nonBMPPlanes[plane - 1] = (RSCharacterSetRef)RSRetain(annexCSet);
        __RSCSetAnnexBitmapSetPlane(cset->_annex->_validEntriesBitmap, plane);
    } else {
        __RSCSetAnnexBitmapClearPlane(cset->_annex->_validEntriesBitmap, plane);
    }
}

RSInline RSCharacterSetRef __RSCSetGetAnnexPlaneCharacterSet(RSCharacterSetRef cset, int plane) {
    __RSCSetAllocateAnnexForPlane(cset, plane);
    if (!__RSCSetAnnexBitmapGetPlane(cset->_annex->_validEntriesBitmap, plane)) {
        cset->_annex->_nonBMPPlanes[plane - 1] = (RSCharacterSetRef)RSCharacterSetCreateMutable(RSGetAllocator(cset));
        __RSCSetAnnexBitmapSetPlane(cset->_annex->_validEntriesBitmap, plane);
    }
    return cset->_annex->_nonBMPPlanes[plane - 1];
}

RSInline RSCharacterSetRef __RSCSetGetAnnexPlaneCharacterSetNoAlloc(RSCharacterSetRef cset, int plane) {
    return (cset->_annex && __RSCSetAnnexBitmapGetPlane(cset->_annex->_validEntriesBitmap, plane) ? cset->_annex->_nonBMPPlanes[plane - 1] : NULL);
}

RSInline void __RSCSetDeallocateAnnexPlane(RSCharacterSetRef cset) {
    if (cset->_annex) {
        int idx;
        
        for (idx = 0;idx < MAX_ANNEX_PLANE;idx++) {
            if (__RSCSetAnnexBitmapGetPlane(cset->_annex->_validEntriesBitmap, idx + 1)) {
                RSRelease(cset->_annex->_nonBMPPlanes[idx]);
            }
        }
        RSAllocatorDeallocate(RSGetAllocator(cset), cset->_annex->_nonBMPPlanes);
        RSAllocatorDeallocate(RSGetAllocator(cset), cset->_annex);
        ((RSMutableCharacterSetRef)cset)->_annex = NULL;
    }
}

RSInline uint8_t __RSCSetGetHeaderValue(const uint8_t *bitmap, int *numPages) {
    uint8_t value = *bitmap;
    
    if ((value == 0) || (value == UINT8_MAX)) {
        int numBytes = __RSCompactBitmapPageSize - 1;
        
        while (numBytes > 0) {
            if (*(++bitmap) != value) break;
            --numBytes;
        }
        if (numBytes == 0) return value;
    }
    return (uint8_t)(++(*numPages));
}

RSInline BOOL __RSCSetIsMemberInCompactBitmap(const uint8_t *compactBitmap, UTF16Char character) {
    uint8_t value = compactBitmap[(character >> 8)]; // Assuming __RSCompactBitmapNumPages == 256
    
    if (value == 0) {
        return NO;
    } else if (value == UINT8_MAX) {
        return YES;
    } else {
        compactBitmap += (__RSCompactBitmapNumPages + (__RSCompactBitmapPageSize * (value - 1)));
        character &= 0xFF; // Assuming __RSCompactBitmapNumPages == 256
        return ((compactBitmap[(character / BITSPERBYTE)] & (1 << (character % BITSPERBYTE))) ? YES : NO);
    }
}

RSInline uint32_t __RSCSetGetCompactBitmapSize(const uint8_t *compactBitmap) {
    uint32_t length = __RSCompactBitmapNumPages;
    uint32_t size = __RSCompactBitmapNumPages;
    uint8_t value;
    
    while (length-- > 0) {
        value = *(compactBitmap++);
        if ((value != 0) && (value != UINT8_MAX)) size += __RSCompactBitmapPageSize;
    }
    return size;
}

/* Take a private "set" structure and make a bitmap from it.  Return the bitmap.  THE CALLER MUST RELEASE THE RETURNED MEMORY as necessary.
 */

RSInline void __RSCSetBitmapProcessManyCharacters(unsigned char *map, unsigned n, unsigned m, BOOL isInverted) {
    if (isInverted) {
        __RSCSetBitmapRemoveCharactersInRange(map, n, m);
    } else {
        __RSCSetBitmapAddCharactersInRange(map, n, m);
    }
}

RSInline void __RSExpandCompactBitmap(const uint8_t *src, uint8_t *dst) {
    const uint8_t *srcBody = src + __RSCompactBitmapNumPages;
    int i;
    uint8_t value;
    
    for (i = 0;i < __RSCompactBitmapNumPages;i++) {
        value = *(src++);
        if ((value == 0) || (value == UINT8_MAX)) {
            memset(dst, value, __RSCompactBitmapPageSize);
        } else {
            memmove(dst, srcBody, __RSCompactBitmapPageSize);
            srcBody += __RSCompactBitmapPageSize;
        }
        dst += __RSCompactBitmapPageSize;
    }
}


static void __RSCheckForExpandedSet(RSCharacterSetRef cset) {
    static int8_t __RSNumberOfPlanesForLogging = -1;
    static BOOL warnedOnce = NO;
    
    if (0 > __RSNumberOfPlanesForLogging) {
        const char *envVar = __RSRuntimeGetEnvironment("RSCharacterSetCheckForExpandedSet");
        long value = (envVar ? strtol_l(envVar, NULL, 0, NULL) : 0);
        __RSNumberOfPlanesForLogging = (int8_t)(((value > 0) && (value <= 16)) ? value : 0);
    }
    
    if (__RSNumberOfPlanesForLogging) {
        uint32_t entries = __RSCSetAnnexValidEntriesBitmap(cset);
        int count = 0;
        
        while (entries) {
            if ((entries & 1) && (++count >= __RSNumberOfPlanesForLogging)) {
                if (!warnedOnce) {
                    RSLog(RSLogLevelWarning, RSSTR("An expanded RSMutableCharacter has been detected.  Recommend to compact with RSCharacterSetCreateCopy"));
                    warnedOnce = YES;
                }
                break;
            }
            entries >>= 1;
        }
    }
}

static void __RSCSetGetBitmap(RSCharacterSetRef cset, uint8_t *bits) {
    uint8_t *bitmap;
    RSIndex length = __RSBitmapSize;
    
    if (__RSCSetIsBitmap(cset) && (bitmap = __RSCSetBitmapBits(cset))) {
        memmove(bits, bitmap, __RSBitmapSize);
    } else {
        BOOL isInverted = __RSCSetIsInverted(cset);
        uint8_t value = (isInverted ? (uint8_t)-1 : 0);
        
        bitmap = bits;
        while (length--) *bitmap++ = value; // Initialize the buffer
        
        if (!__RSCSetIsEmpty(cset)) {
            switch (__RSCSetClassType(cset)) {
                case __RSCharSetClassBuiltin: {
                    UInt8 result = RSUniCharGetBitmapForPlane((RSBitU32)__RSCSetBuiltinType(cset), 0, bits, (isInverted != 0));
                    if (result == RSUniCharBitmapEmpty && isInverted) {
                        length = __RSBitmapSize;
                        bitmap = bits;
                        while (length--) *bitmap++ = 0;
                    } else if (result == RSUniCharBitmapAll && !isInverted) {
                        length = __RSBitmapSize;
                        bitmap = bits;
                        while (length--) *bitmap++ = (UInt8)0xFF;
                    }
                }
                    break;
                    
                case __RSCharSetClassRange: {
                    UInt32 theChar = __RSCSetRangeFirstChar(cset);
                    if (theChar < NUMCHARACTERS) { // the range starts in BMP
                        length = __RSCSetRangeLength(cset);
                        if (theChar + length >= NUMCHARACTERS) length = NUMCHARACTERS - theChar;
                        if (isInverted) {
                            __RSCSetBitmapRemoveCharactersInRange(bits, theChar, (UniChar)(theChar + length) - 1);
                        } else {
                            __RSCSetBitmapAddCharactersInRange(bits, theChar, (UniChar)(theChar + length) - 1);
                        }
                    }
                }
                    break;
                    
                case __RSCharSetClassString: {
                    const UniChar *buffer = __RSCSetStringBuffer(cset);
                    length = __RSCSetStringLength(cset);
                    while (length--) (isInverted ? __RSCSetBitmapRemoveCharacter(bits, *buffer++) : __RSCSetBitmapAddCharacter(bits, *buffer++));
                }
                    break;
                    
                case __RSCharSetClassCompactBitmap:
                    __RSExpandCompactBitmap(__RSCSetCompactBitmapBits(cset), bits);
                    break;
            }
        }
    }
}

static BOOL __RSCharacterSetEqual(RSTypeRef cf1, RSTypeRef cf2);

static BOOL __RSCSetIsEqualAnnex(RSCharacterSetRef cf1, RSCharacterSetRef cf2) {
    RSCharacterSetRef subSet1;
    RSCharacterSetRef subSet2;
    BOOL isAnnexInvertStateIdentical = (__RSCSetAnnexIsInverted(cf1) == __RSCSetAnnexIsInverted(cf2) ? YES: NO);
    int idx;
    
    if (isAnnexInvertStateIdentical) {
        if (__RSCSetAnnexValidEntriesBitmap(cf1) != __RSCSetAnnexValidEntriesBitmap(cf2)) return NO;
        for (idx = 1;idx <= MAX_ANNEX_PLANE;idx++) {
            subSet1 = __RSCSetGetAnnexPlaneCharacterSetNoAlloc(cf1, idx);
            subSet2 = __RSCSetGetAnnexPlaneCharacterSetNoAlloc(cf2, idx);
            
            if (subSet1 && !__RSCharacterSetEqual(subSet1, subSet2)) return NO;
        }
    } else {
        uint8_t bitsBuf[__RSBitmapSize];
        uint8_t bitsBuf2[__RSBitmapSize];
        
        for (idx = 1;idx <= MAX_ANNEX_PLANE;idx++) {
            subSet1 = __RSCSetGetAnnexPlaneCharacterSetNoAlloc(cf1, idx);
            subSet2 = __RSCSetGetAnnexPlaneCharacterSetNoAlloc(cf2, idx);
            
            if (subSet1 == NULL && subSet2 == NULL) {
                return NO;
            } else if (subSet1 == NULL) {
                if (__RSCSetIsBitmap(subSet2)) {
                    if (!__RSCSetIsEqualBitmap((const UInt32 *)__RSCSetBitmapBits(subSet2), (const UInt32 *)-1)) {
                        return NO;
                    }
                } else {
                    __RSCSetGetBitmap(subSet2, bitsBuf);
                    if (!__RSCSetIsEqualBitmap((const UInt32 *)bitsBuf, (const UInt32 *)-1)) {
                        return NO;
                    }
                }
            } else if (subSet2 == NULL) {
                if (__RSCSetIsBitmap(subSet1)) {
                    if (!__RSCSetIsEqualBitmap((const UInt32 *)__RSCSetBitmapBits(subSet1), (const UInt32 *)-1)) {
                        return NO;
                    }
                } else {
                    __RSCSetGetBitmap(subSet1, bitsBuf);
                    if (!__RSCSetIsEqualBitmap((const UInt32 *)bitsBuf, (const UInt32 *)-1)) {
                        return NO;
                    }
                }
            } else {
                BOOL isBitmap1 = __RSCSetIsBitmap(subSet1);
                BOOL isBitmap2 = __RSCSetIsBitmap(subSet2);
                
                if (isBitmap1 && isBitmap2) {
                    if (!__RSCSetIsEqualBitmapInverted((const UInt32 *)__RSCSetBitmapBits(subSet1), (const UInt32 *)__RSCSetBitmapBits(subSet2))) {
                        return NO;
                    }
                } else if (!isBitmap1 && !isBitmap2) {
                    __RSCSetGetBitmap(subSet1, bitsBuf);
                    __RSCSetGetBitmap(subSet2, bitsBuf2);
                    if (!__RSCSetIsEqualBitmapInverted((const UInt32 *)bitsBuf, (const UInt32 *)bitsBuf2)) {
                        return NO;
                    }
                } else {
                    if (isBitmap2) {
                        RSCharacterSetRef tmp = subSet2;
                        subSet2 = subSet1;
                        subSet1 = tmp;
                    }
                    __RSCSetGetBitmap(subSet2, bitsBuf);
                    if (!__RSCSetIsEqualBitmapInverted((const UInt32 *)__RSCSetBitmapBits(subSet1), (const UInt32 *)bitsBuf)) {
                        return NO;
                    }
                }
            }
        }
    }
    return YES;
}

/* Compact bitmap
 */
static uint8_t *__RSCreateCompactBitmap(RSAllocatorRef allocator, const uint8_t *bitmap) {
    const uint8_t *src;
    uint8_t *dst;
    int i;
    int numPages = 0;
    uint8_t header[__RSCompactBitmapNumPages];
    
    src = bitmap;
    for (i = 0;i < __RSCompactBitmapNumPages;i++) {
        header[i] = __RSCSetGetHeaderValue(src, &numPages);
        
        // Allocating more pages is probably not interesting enough to be compact
        if (numPages > __RSCompactBitmapMaxPages) return NULL;
        src += __RSCompactBitmapPageSize;
    }
    
    dst = (uint8_t *)RSAllocatorAllocate(allocator, __RSCompactBitmapNumPages + (__RSCompactBitmapPageSize * numPages));
    
    if (numPages > 0) {
        uint8_t *dstBody = dst + __RSCompactBitmapNumPages;
        
        src = bitmap;
        for (i = 0;i < __RSCompactBitmapNumPages;i++) {
            dst[i] = header[i];
            
            if ((dst[i] != 0) && (dst[i] != UINT8_MAX)) {
                memmove(dstBody, src, __RSCompactBitmapPageSize);
                dstBody += __RSCompactBitmapPageSize;
            }
            src += __RSCompactBitmapPageSize;
        }
    } else {
        memmove(dst, header, __RSCompactBitmapNumPages);
    }
    
    return dst;
}

static void __RSCSetMakeCompact(RSMutableCharacterSetRef cset) {
    if (__RSCSetIsBitmap(cset) && __RSCSetBitmapBits(cset)) {
        uint8_t *bitmap = __RSCSetBitmapBits(cset);
        uint8_t *cBitmap = __RSCreateCompactBitmap(RSGetAllocator(cset), bitmap);
        
        if (cBitmap) {
            RSAllocatorDeallocate(RSGetAllocator(cset), bitmap);
            __RSCSetPutClassType(cset, __RSCharSetClassCompactBitmap);
            __RSCSetPutCompactBitmapBits(cset, cBitmap);
        }
    }
}

static void __RSCSetAddNonBMPPlanesInRange(RSMutableCharacterSetRef cset, RSRange range) {
    int firstChar = (range.location & 0xFFFF);
    int maxChar = (int)(range.location + range.length);
    int idx = (int)(range.location >> 16); // first plane
    int maxPlane = (maxChar - 1) >> 16; // last plane
    RSRange planeRange;
    RSMutableCharacterSetRef annexPlane;
    
    maxChar &= 0xFFFF;
    
    for (idx = (idx ? idx : 1);idx <= maxPlane;idx++) {
        planeRange.location = __RSMax(firstChar, 0);
        planeRange.length = (idx == maxPlane && maxChar ? maxChar : 0x10000) - planeRange.location;
        if (__RSCSetAnnexIsInverted(cset)) {
            if ((annexPlane = (RSMutableCharacterSetRef)__RSCSetGetAnnexPlaneCharacterSetNoAlloc(cset, idx))) {
                RSCharacterSetRemoveCharactersInRange(annexPlane, planeRange);
                if (__RSCSetIsEmpty(annexPlane) && !__RSCSetIsInverted(annexPlane)) {
                    RSRelease(annexPlane);
                    __RSCSetAnnexBitmapClearPlane(cset->_annex->_validEntriesBitmap, idx);
                }
            }
        } else {
            RSCharacterSetAddCharactersInRange((RSMutableCharacterSetRef)__RSCSetGetAnnexPlaneCharacterSet(cset, idx), planeRange);
        }
    }
    if (!__RSCSetHasNonBMPPlane(cset) && !__RSCSetAnnexIsInverted(cset)) __RSCSetDeallocateAnnexPlane(cset);
}

static void __RSCSetRemoveNonBMPPlanesInRange(RSMutableCharacterSetRef cset, RSRange range) {
    int firstChar = (range.location & 0xFFFF);
    int maxChar = (int)(range.location + range.length);
    int idx = (int)(range.location >> 16); // first plane
    int maxPlane = (maxChar - 1) >> 16; // last plane
    RSRange planeRange;
    RSMutableCharacterSetRef annexPlane;
    
    maxChar &= 0xFFFF;
    
    for (idx = (idx ? idx : 1);idx <= maxPlane;idx++) {
        planeRange.location = __RSMax(firstChar, 0);
        planeRange.length = (idx == maxPlane && maxChar ? maxChar : 0x10000) - planeRange.location;
        if (__RSCSetAnnexIsInverted(cset)) {
            RSCharacterSetAddCharactersInRange((RSMutableCharacterSetRef)__RSCSetGetAnnexPlaneCharacterSet(cset, idx), planeRange);
        } else {
            if ((annexPlane = (RSMutableCharacterSetRef)__RSCSetGetAnnexPlaneCharacterSetNoAlloc(cset, idx))) {
                RSCharacterSetRemoveCharactersInRange(annexPlane, planeRange);
                if(__RSCSetIsEmpty(annexPlane) && !__RSCSetIsInverted(annexPlane)) {
                    RSRelease(annexPlane);
                    __RSCSetAnnexBitmapClearPlane(cset->_annex->_validEntriesBitmap, idx);
                }
            }
        }
    }
    if (!__RSCSetHasNonBMPPlane(cset) && !__RSCSetAnnexIsInverted(cset)) __RSCSetDeallocateAnnexPlane(cset);
}

static void __RSCSetMakeBitmap(RSMutableCharacterSetRef cset) {
    if (!__RSCSetIsBitmap(cset) || !__RSCSetBitmapBits(cset)) {
        RSAllocatorRef allocator = RSGetAllocator(cset);
        uint8_t *bitmap = (uint8_t *)RSAllocatorAllocate(allocator, __RSBitmapSize);
        __RSCSetGetBitmap(cset, bitmap);
        
        if (__RSCSetIsBuiltin(cset)) {
            RSIndex numPlanes = RSUniCharGetNumberOfPlanes((RSBitU32)__RSCSetBuiltinType(cset));
            
            if (numPlanes > 1) {
                RSMutableCharacterSetRef annexSet;
                uint8_t *annexBitmap = NULL;
                int idx;
                UInt8 result;
                
                __RSCSetAllocateAnnexForPlane(cset, (RSBitU32)numPlanes - 1);
                for (idx = 1;idx < numPlanes;idx++) {
                    if (NULL == annexBitmap) {
                        annexBitmap = (uint8_t *)RSAllocatorAllocate(allocator, __RSBitmapSize);
                    }
                    result = RSUniCharGetBitmapForPlane((RSBitU32)__RSCSetBuiltinType(cset), idx, annexBitmap, NO);
                    if (result == RSUniCharBitmapEmpty) continue;
                    if (result == RSUniCharBitmapAll) {
                        RSIndex bitmapLength = __RSBitmapSize;
                        uint8_t *bytes = annexBitmap;
                        while (bitmapLength-- > 0) *(bytes++) = (uint8_t)0xFF;
                    }
                    annexSet = (RSMutableCharacterSetRef)__RSCSetGetAnnexPlaneCharacterSet(cset, idx);
                    __RSCSetPutClassType(annexSet, __RSCharSetClassBitmap);
                    __RSCSetPutBitmapBits(annexSet, annexBitmap);
                    __RSCSetPutIsInverted(annexSet, NO);
                    __RSCSetPutHasHashValue(annexSet, NO);
                    annexBitmap = NULL;
                }
                if (annexBitmap) RSAllocatorDeallocate(allocator, annexBitmap);
            }
        } else if (__RSCSetIsCompactBitmap(cset) && __RSCSetCompactBitmapBits(cset)) {
            RSAllocatorDeallocate(allocator, __RSCSetCompactBitmapBits(cset));
            __RSCSetPutCompactBitmapBits(cset, NULL);
        } else if (__RSCSetIsString(cset) && __RSCSetStringBuffer(cset)) {
            RSAllocatorDeallocate(allocator, __RSCSetStringBuffer(cset));
            __RSCSetPutStringBuffer(cset, NULL);
        } else if (__RSCSetIsRange(cset)) { // We may have to allocate annex here
            BOOL needsToInvert = (!__RSCSetHasNonBMPPlane(cset) && __RSCSetIsInverted(cset) ? YES : NO);
            __RSCSetAddNonBMPPlanesInRange(cset, RSMakeRange(__RSCSetRangeFirstChar(cset), __RSCSetRangeLength(cset)));
            if (needsToInvert) __RSCSetAnnexSetIsInverted(cset, YES);
        }
        __RSCSetPutClassType(cset, __RSCharSetClassBitmap);
        __RSCSetPutBitmapBits(cset, bitmap);
        __RSCSetPutIsInverted(cset, NO);
    }
}

RSInline RSMutableCharacterSetRef __RSCSetGenericCreate(RSAllocatorRef allocator, UInt32 flags) {
    RSMutableCharacterSetRef cset;
    RSIndex size = sizeof(struct __RSCharacterSet) - sizeof(RSRuntimeBase);
    
    cset = (RSMutableCharacterSetRef)__RSRuntimeCreateInstance(allocator, RSCharacterSetGetTypeID(), size);
    if (NULL == cset) return NULL;
    
    RSRuntimeClassBaseFiled(cset) |= flags;
    cset->_hashValue = 0;
    cset->_annex = NULL;
    
    return cset;
}

static void __RSApplySurrogatesInString(RSMutableCharacterSetRef cset, RSStringRef string, void (*applyer)(RSMutableCharacterSetRef, RSRange)) {
    RSStringInlineBuffer buffer;
    RSIndex index, length = RSStringGetLength(string);
    RSRange range = RSMakeRange(0, 0);
    UTF32Char character;
    
    RSStringInitInlineBuffer(string, &buffer, RSMakeRange(0, length));
    
    for (index = 0;index < length;index++) {
        character = __RSStringGetCharacterFromInlineBufferQuick(&buffer, index);
        
        if (RSStringIsSurrogateHighCharacter(character) && ((index + 1) < length)) {
            UTF16Char other = __RSStringGetCharacterFromInlineBufferQuick(&buffer, index + 1);
            
            if (RSStringIsSurrogateLowCharacter(other)) {
                character = RSStringGetLongCharacterForSurrogatePair(character, other);
                
                if ((range.length + range.location) == character) {
                    ++range.length;
                } else {
                    if (range.length > 0) applyer(cset, range);
                    range.location = character;
                    range.length = 1;
                }
            }
            
            ++index; // skip the low surrogate
        }
    }
    
    if (range.length > 0) applyer(cset, range);
}


/* Bsearch theChar for __RSCharSetClassString
 */
RSInline BOOL __RSCSetBsearchUniChar(const UniChar *theTable, RSIndex length, UniChar theChar) {
    const UniChar *p, *q, *divider;
    
    if ((theChar < theTable[0]) || (theChar > theTable[length - 1])) return NO;
    
    p = theTable;
    q = p + (length - 1);
    while (p <= q) {
        divider = p + ((q - p) >> 1);	/* divide by 2 */
        if (theChar < *divider) q = divider - 1;
        else if (theChar > *divider) p = divider + 1;
        else return YES;
    }
    return NO;
}

/* Array of instantiated builtin set. Note builtin set ID starts with 1 so the array index is ID - 1
 */
static RSCharacterSetRef *__RSBuiltinSets = NULL;

/* Global lock for character set
 */
static RSSpinLock __RSCharacterSetLock = RSSpinLockInit;

/* RSBase API functions
 */
static BOOL __RSCharacterSetEqual(RSTypeRef cf1, RSTypeRef cf2) {
    BOOL isInvertStateIdentical = (__RSCSetIsInverted((RSCharacterSetRef)cf1) == __RSCSetIsInverted((RSCharacterSetRef)cf2) ? YES: NO);
    BOOL isAnnexInvertStateIdentical = (__RSCSetAnnexIsInverted((RSCharacterSetRef)cf1) == __RSCSetAnnexIsInverted((RSCharacterSetRef)cf2) ? YES: NO);
    RSIndex idx;
    RSCharacterSetRef subSet1;
    uint8_t bitsBuf[__RSBitmapSize];
    uint8_t *bits;
    BOOL isBitmap1;
    BOOL isBitmap2;
    
    if (__RSCSetHasHashValue((RSCharacterSetRef)cf1) && __RSCSetHasHashValue((RSCharacterSetRef)cf2) && ((RSCharacterSetRef)cf1)->_hashValue != ((RSCharacterSetRef)cf2)->_hashValue) return NO;
    if (__RSCSetIsEmpty((RSCharacterSetRef)cf1) && __RSCSetIsEmpty((RSCharacterSetRef)cf2) && !isInvertStateIdentical) return NO;
    
    if (__RSCSetClassType((RSCharacterSetRef)cf1) == __RSCSetClassType((RSCharacterSetRef)cf2)) { // Types are identical, we can do it fast
        switch (__RSCSetClassType((RSCharacterSetRef)cf1)) {
            case __RSCharSetClassBuiltin:
                return (__RSCSetBuiltinType((RSCharacterSetRef)cf1) == __RSCSetBuiltinType((RSCharacterSetRef)cf2) && isInvertStateIdentical ? YES : NO);
                
            case __RSCharSetClassRange:
                return (__RSCSetRangeFirstChar((RSCharacterSetRef)cf1) == __RSCSetRangeFirstChar((RSCharacterSetRef)cf2) && __RSCSetRangeLength((RSCharacterSetRef)cf1) && __RSCSetRangeLength((RSCharacterSetRef)cf2) && isInvertStateIdentical ? YES : NO);
                
            case __RSCharSetClassString:
                if (__RSCSetStringLength((RSCharacterSetRef)cf1) == __RSCSetStringLength((RSCharacterSetRef)cf2) && isInvertStateIdentical) {
                    const UniChar *buf1 = __RSCSetStringBuffer((RSCharacterSetRef)cf1);
                    const UniChar *buf2 = __RSCSetStringBuffer((RSCharacterSetRef)cf2);
                    RSIndex length = __RSCSetStringLength((RSCharacterSetRef)cf1);
                    
                    while (length--) if (*buf1++ != *buf2++) return NO;
                } else {
                    return NO;
                }
                break;
                
            case __RSCharSetClassBitmap:
                if (!__RSCSetIsEqualBitmap((const UInt32 *)__RSCSetBitmapBits((RSCharacterSetRef)cf1), (const UInt32 *)__RSCSetBitmapBits((RSCharacterSetRef)cf2))) return NO;
                break;
        }
        return __RSCSetIsEqualAnnex((RSCharacterSetRef)cf1, (RSCharacterSetRef)cf2);
    }
    
    // Check for easy empty cases
    if (__RSCSetIsEmpty((RSCharacterSetRef)cf1) || __RSCSetIsEmpty((RSCharacterSetRef)cf2)) {
        RSCharacterSetRef emptySet = (__RSCSetIsEmpty((RSCharacterSetRef)cf1) ? (RSCharacterSetRef)cf1 : (RSCharacterSetRef)cf2);
        RSCharacterSetRef nonEmptySet = (emptySet == cf1 ? (RSCharacterSetRef)cf2 : (RSCharacterSetRef)cf1);
        
        if (__RSCSetIsBuiltin(nonEmptySet)) {
            return NO;
        } else if (__RSCSetIsRange(nonEmptySet)) {
            if (isInvertStateIdentical) {
                return (__RSCSetRangeLength(nonEmptySet) ? NO : YES);
            } else {
                return (__RSCSetRangeLength(nonEmptySet) == 0x110000 ? YES : NO);
            }
        } else {
            if (__RSCSetAnnexIsInverted(nonEmptySet)) {
                if (__RSCSetAnnexValidEntriesBitmap(nonEmptySet) != 0x1FFFE) return NO;
            } else {
                if (__RSCSetAnnexValidEntriesBitmap(nonEmptySet)) return NO;
            }
            
            if (__RSCSetIsBitmap(nonEmptySet)) {
                bits = __RSCSetBitmapBits(nonEmptySet);
            } else {
                bits = bitsBuf;
                __RSCSetGetBitmap(nonEmptySet, bitsBuf);
            }
            
            if (__RSCSetIsEqualBitmap(NULL, (const UInt32 *)bits)) {
                if (!__RSCSetAnnexIsInverted(nonEmptySet)) return YES;
            } else {
                return NO;
            }
            
            // Annex set has to be RSMakeRange(0x10000, 0xfffff)
            for (idx = 1;idx < MAX_ANNEX_PLANE;idx++) {
                if (__RSCSetIsBitmap(nonEmptySet)) {
                    if (!__RSCSetIsEqualBitmap((__RSCSetAnnexIsInverted(nonEmptySet) ? NULL : (const UInt32 *)-1), (const UInt32 *)bitsBuf)) return NO;
                } else {
                    __RSCSetGetBitmap(__RSCSetGetAnnexPlaneCharacterSetNoAlloc(nonEmptySet, (RSBitU32)idx), bitsBuf);
                    if (!__RSCSetIsEqualBitmap((const UInt32 *)-1, (const UInt32 *)bitsBuf)) return NO;
                }
            }
            return YES;
        }
    }
    
    if (__RSCSetIsBuiltin((RSCharacterSetRef)cf1) || __RSCSetIsBuiltin((RSCharacterSetRef)cf2)) {
        RSCharacterSetRef builtinSet = (__RSCSetIsBuiltin((RSCharacterSetRef)cf1) ? (RSCharacterSetRef)cf1 : (RSCharacterSetRef)cf2);
        RSCharacterSetRef nonBuiltinSet = (builtinSet == cf1 ? (RSCharacterSetRef)cf2 : (RSCharacterSetRef)cf1);
        
        
        if (__RSCSetIsRange(nonBuiltinSet)) {
            UTF32Char firstChar = __RSCSetRangeFirstChar(nonBuiltinSet);
            UTF32Char lastChar = (RSBitU32)(firstChar + __RSCSetRangeLength(nonBuiltinSet) - 1);
            uint8_t firstPlane = (firstChar >> 16) & 0xFF;
            uint8_t lastPlane = (lastChar >> 16) & 0xFF;
            uint8_t result;
            
            for (idx = 0;idx < MAX_ANNEX_PLANE;idx++) {
                result = RSUniCharGetBitmapForPlane((RSBitU32)__RSCSetBuiltinType(builtinSet), (RSBitU32)idx, bitsBuf, (isInvertStateIdentical != 0));
                
                if (idx < firstPlane || idx > lastPlane) {
                    if (result == RSUniCharBitmapAll) {
                        return NO;
                    } else if (result == RSUniCharBitmapFilled) {
                        if (!__RSCSetIsEqualBitmap(NULL, (const UInt32 *)bitsBuf)) return NO;
                    }
                } else if (idx > firstPlane && idx < lastPlane) {
                    if (result == RSUniCharBitmapEmpty) {
                        return NO;
                    } else if (result == RSUniCharBitmapFilled) {
                        if (!__RSCSetIsEqualBitmap((const UInt32 *)-1, (const UInt32 *)bitsBuf)) return NO;
                    }
                } else {
                    if (result == RSUniCharBitmapEmpty) {
                        return NO;
                    } else if (result == RSUniCharBitmapAll) {
                        if (idx == firstPlane) {
                            if (((firstChar & 0xFFFF) != 0) || (firstPlane == lastPlane && ((lastChar & 0xFFFF) != 0xFFFF))) return NO;
                        } else {
                            if (((lastChar & 0xFFFF) != 0xFFFF) || (firstPlane == lastPlane && ((firstChar & 0xFFFF) != 0))) return NO;
                        }
                    } else {
                        if (idx == firstPlane) {
                            if (!__RSCSetIsBitmapEqualToRange((const UInt32 *)bitsBuf, firstChar & 0xFFFF, (firstPlane == lastPlane ? lastChar & 0xFFFF : 0xFFFF), NO)) return NO;
                        } else {
                            if (!__RSCSetIsBitmapEqualToRange((const UInt32 *)bitsBuf, (firstPlane == lastPlane ? firstChar & 0xFFFF : 0), lastChar & 0xFFFF, NO)) return NO;
                        }
                    }
                }
            }
            return YES;
        } else  {
            uint8_t bitsBuf2[__RSBitmapSize];
            uint8_t result;
            
            result = RSUniCharGetBitmapForPlane((RSBitU32)__RSCSetBuiltinType(builtinSet), 0, bitsBuf, (__RSCSetIsInverted(builtinSet) != 0));
            if (result == RSUniCharBitmapFilled) {
                if (__RSCSetIsBitmap(nonBuiltinSet)) {
                    if (!__RSCSetIsEqualBitmap((const UInt32 *)bitsBuf, (const UInt32 *)__RSCSetBitmapBits(nonBuiltinSet))) return NO;
                } else {
                    
                    __RSCSetGetBitmap(nonBuiltinSet, bitsBuf2);
                    if (!__RSCSetIsEqualBitmap((const UInt32 *)bitsBuf, (const UInt32 *)bitsBuf2)) {
                        return NO;
                    }
                }
            } else {
                if (__RSCSetIsBitmap(nonBuiltinSet)) {
                    if (!__RSCSetIsEqualBitmap((result == RSUniCharBitmapAll ? (const UInt32*)-1 : NULL), (const UInt32 *)__RSCSetBitmapBits(nonBuiltinSet))) return NO;
                } else {
                    __RSCSetGetBitmap(nonBuiltinSet, bitsBuf);
                    if (!__RSCSetIsEqualBitmap((result == RSUniCharBitmapAll ? (const UInt32*)-1: NULL), (const UInt32 *)bitsBuf)) return NO;
                }
            }
            
            isInvertStateIdentical = (__RSCSetIsInverted(builtinSet) == __RSCSetAnnexIsInverted(nonBuiltinSet) ? YES : NO);
            
            for (idx = 1;idx < MAX_ANNEX_PLANE;idx++) {
                result = RSUniCharGetBitmapForPlane((RSBitU32)__RSCSetBuiltinType(builtinSet), (RSBitU32)idx, bitsBuf, !isInvertStateIdentical);
                subSet1 = __RSCSetGetAnnexPlaneCharacterSetNoAlloc(nonBuiltinSet, (RSBitU32)idx);
                
                if (result == RSUniCharBitmapFilled) {
                    if (NULL == subSet1) {
                        return NO;
                    } else if (__RSCSetIsBitmap(subSet1)) {
                        if (!__RSCSetIsEqualBitmap((const UInt32*)bitsBuf, (const UInt32*)__RSCSetBitmapBits(subSet1))) {
                            return NO;
                        }
                    } else {
                        
                        __RSCSetGetBitmap(subSet1, bitsBuf2);
                        if (!__RSCSetIsEqualBitmap((const UInt32*)bitsBuf, (const UInt32*)bitsBuf2)) {
                            return NO;
                        }
                    }
                } else {
                    if (NULL == subSet1) {
                        if (result == RSUniCharBitmapAll) {
                            return NO;
                        }
                    } else if (__RSCSetIsBitmap(subSet1)) {
                        if (!__RSCSetIsEqualBitmap((result == RSUniCharBitmapAll ? (const UInt32*)-1: NULL), (const UInt32*)__RSCSetBitmapBits(subSet1))) {
                            return NO;
                        }
                    } else {
                        __RSCSetGetBitmap(subSet1, bitsBuf);
                        if (!__RSCSetIsEqualBitmap((result == RSUniCharBitmapAll ? (const UInt32*)-1: NULL), (const UInt32*)bitsBuf)) {
                            return NO;
                        }
                    }
                }
            }
            return YES;
        }
    }
    
    if (__RSCSetIsRange((RSCharacterSetRef)cf1) || __RSCSetIsRange((RSCharacterSetRef)cf2)) {
        RSCharacterSetRef rangeSet = (__RSCSetIsRange((RSCharacterSetRef)cf1) ? (RSCharacterSetRef)cf1 : (RSCharacterSetRef)cf2);
        RSCharacterSetRef nonRangeSet = (rangeSet == cf1 ? (RSCharacterSetRef)cf2 : (RSCharacterSetRef)cf1);
        UTF32Char firstChar = __RSCSetRangeFirstChar(rangeSet);
        UTF32Char lastChar = (RSBitU32)(firstChar + __RSCSetRangeLength(rangeSet) - 1);
        uint8_t firstPlane = (firstChar >> 16) & 0xFF;
        uint8_t lastPlane = (lastChar >> 16) & 0xFF;
        BOOL isRangeSetInverted = __RSCSetIsInverted(rangeSet);
        
        if (__RSCSetIsBitmap(nonRangeSet)) {
            bits = __RSCSetBitmapBits(nonRangeSet);
        } else {
            bits = bitsBuf;
            __RSCSetGetBitmap(nonRangeSet, bitsBuf);
        }
        if (firstPlane == 0) {
            if (!__RSCSetIsBitmapEqualToRange((const UInt32*)bits, firstChar, (lastPlane == 0 ? lastChar : 0xFFFF), isRangeSetInverted)) return NO;
            firstPlane = 1;
            firstChar = 0;
        } else {
            if (!__RSCSetIsEqualBitmap((const UInt32*)bits, (isRangeSetInverted ? (const UInt32 *)-1 : NULL))) return NO;
            firstChar &= 0xFFFF;
        }
        
        lastChar &= 0xFFFF;
        
        isAnnexInvertStateIdentical = (isRangeSetInverted == __RSCSetAnnexIsInverted(nonRangeSet) ? YES : NO);
        
        for (idx = 1;idx < MAX_ANNEX_PLANE;idx++) {
            subSet1 = __RSCSetGetAnnexPlaneCharacterSetNoAlloc(nonRangeSet, (RSBitU32)idx);
            if (NULL == subSet1) {
                if (idx < firstPlane || idx > lastPlane) {
                    if (!isAnnexInvertStateIdentical) return NO;
                } else if (idx > firstPlane && idx < lastPlane) {
                    if (isAnnexInvertStateIdentical) return NO;
                } else if (idx == firstPlane) {
                    if (isAnnexInvertStateIdentical || firstChar || (idx == lastPlane && lastChar != 0xFFFF)) return NO;
                } else if (idx == lastPlane) {
                    if (isAnnexInvertStateIdentical || (idx == firstPlane && firstChar) || (lastChar != 0xFFFF)) return NO;
                }
            } else {
                if (__RSCSetIsBitmap(subSet1)) {
                    bits = __RSCSetBitmapBits(subSet1);
                } else {
                    __RSCSetGetBitmap(subSet1, bitsBuf);
                    bits = bitsBuf;
                }
                
                if (idx < firstPlane || idx > lastPlane) {
                    if (!__RSCSetIsEqualBitmap((const UInt32*)bits, (isAnnexInvertStateIdentical ? NULL : (const UInt32 *)-1))) return NO;
                } else if (idx > firstPlane && idx < lastPlane) {
                    if (!__RSCSetIsEqualBitmap((const UInt32*)bits, (isAnnexInvertStateIdentical ? (const UInt32 *)-1 : NULL))) return NO;
                } else if (idx == firstPlane) {
                    if (!__RSCSetIsBitmapEqualToRange((const UInt32*)bits, firstChar, (idx == lastPlane ? lastChar : 0xFFFF), !isAnnexInvertStateIdentical)) return NO;
                } else if (idx == lastPlane) {
                    if (!__RSCSetIsBitmapEqualToRange((const UInt32*)bits, (idx == firstPlane ? firstChar : 0), lastChar, !isAnnexInvertStateIdentical)) return NO;
                }
            }
        }
        return YES;
    }
    
    isBitmap1 = __RSCSetIsBitmap((RSCharacterSetRef)cf1);
    isBitmap2 = __RSCSetIsBitmap((RSCharacterSetRef)cf2);
    
    if (isBitmap1 && isBitmap2) {
        if (!__RSCSetIsEqualBitmap((const UInt32 *)__RSCSetBitmapBits((RSCharacterSetRef)cf1), (const UInt32 *)__RSCSetBitmapBits((RSCharacterSetRef)cf2))) return NO;
    } else if (!isBitmap1 && !isBitmap2) {
        uint8_t bitsBuf2[__RSBitmapSize];
        
        __RSCSetGetBitmap((RSCharacterSetRef)cf1, bitsBuf);
        __RSCSetGetBitmap((RSCharacterSetRef)cf2, bitsBuf2);
        
        if (!__RSCSetIsEqualBitmap((const UInt32*)bitsBuf, (const UInt32*)bitsBuf2)) {
            return NO;
        }
    } else {
        if (isBitmap2) {
            RSCharacterSetRef tmp = (RSCharacterSetRef)cf2;
            cf2 = cf1;
            cf1 = tmp;
        }
        
        __RSCSetGetBitmap((RSCharacterSetRef)cf2, bitsBuf);
        
        if (!__RSCSetIsEqualBitmap((const UInt32 *)__RSCSetBitmapBits((RSCharacterSetRef)cf1), (const UInt32 *)bitsBuf)) return NO;
    }
    return __RSCSetIsEqualAnnex((RSCharacterSetRef)cf1, (RSCharacterSetRef)cf2);
}

static RSHashCode __RSCharacterSetHash(RSTypeRef cf) {
    if (!__RSCSetHasHashValue((RSCharacterSetRef)cf)) {
        if (__RSCSetIsEmpty((RSCharacterSetRef)cf)) {
            ((RSMutableCharacterSetRef)cf)->_hashValue = (__RSCSetIsInverted((RSCharacterSetRef)cf) ? ((UInt32)0xFFFFFFFF) : 0);
        } else if (__RSCSetIsBitmap( (RSCharacterSetRef) cf  )) {
            ((RSMutableCharacterSetRef)cf)->_hashValue = RSHashBytes(__RSCSetBitmapBits((RSCharacterSetRef)cf), __RSBitmapSize);
        } else {
            uint8_t bitsBuf[__RSBitmapSize];
            __RSCSetGetBitmap((RSCharacterSetRef)cf, bitsBuf);
            ((RSMutableCharacterSetRef)cf)->_hashValue = RSHashBytes(bitsBuf, __RSBitmapSize);
        }
        __RSCSetPutHasHashValue((RSMutableCharacterSetRef)cf, YES);
    }
    return ((RSCharacterSetRef)cf)->_hashValue;
}

static RSStringRef  __RSCharacterSetCopyDescription(RSTypeRef cf) {
    RSMutableStringRef string;
    RSIndex idx;
    RSIndex length;
    
    if (__RSCSetIsEmpty((RSCharacterSetRef)cf)) {
        return (RSStringRef)(__RSCSetIsInverted((RSCharacterSetRef)cf) ? RSRetain(RSSTR("<RSCharacterSet All>")) : RSRetain(RSSTR("<RSCharacterSet Empty>")));
    }
    
    switch (__RSCSetClassType((RSCharacterSetRef)cf)) {
        case __RSCharSetClassBuiltin:
            switch (__RSCSetBuiltinType((RSCharacterSetRef)cf)) {
                case RSCharacterSetControl: return (RSStringRef)RSRetain(RSSTR("<RSCharacterSet Predefined Control Set>"));
                case RSCharacterSetWhitespace : return (RSStringRef)RSRetain(RSSTR("<RSCharacterSet Predefined Whitespace Set>"));
                case RSCharacterSetWhitespaceAndNewline: return (RSStringRef)RSRetain(RSSTR("<RSCharacterSet Predefined WhitespaceAndNewline Set>"));
                case RSCharacterSetDecimalDigit: return (RSStringRef)RSRetain(RSSTR("<RSCharacterSet Predefined DecimalDigit Set>"));
                case RSCharacterSetLetter: return (RSStringRef)RSRetain(RSSTR("<RSCharacterSet Predefined Letter Set>"));
                case RSCharacterSetLowercaseLetter: return (RSStringRef)RSRetain(RSSTR("<RSCharacterSet Predefined LowercaseLetter Set>"));
                case RSCharacterSetUppercaseLetter: return (RSStringRef)RSRetain(RSSTR("<RSCharacterSet Predefined UppercaseLetter Set>"));
                case RSCharacterSetNonBase: return (RSStringRef)RSRetain(RSSTR("<RSCharacterSet Predefined NonBase Set>"));
                case RSCharacterSetDecomposable: return (RSStringRef)RSRetain(RSSTR("<RSCharacterSet Predefined Decomposable Set>"));
                case RSCharacterSetAlphaNumeric: return (RSStringRef)RSRetain(RSSTR("<RSCharacterSet Predefined AlphaNumeric Set>"));
                case RSCharacterSetPunctuation: return (RSStringRef)RSRetain(RSSTR("<RSCharacterSet Predefined Punctuation Set>"));
                case RSCharacterSetIllegal: return (RSStringRef)RSRetain(RSSTR("<RSCharacterSet Predefined Illegal Set>"));
                case RSCharacterSetCapitalizedLetter: return (RSStringRef)RSRetain(RSSTR("<RSCharacterSet Predefined CapitalizedLetter Set>"));
                case RSCharacterSetSymbol: return (RSStringRef)RSRetain(RSSTR("<RSCharacterSet Predefined Symbol Set>"));
                case RSCharacterSetNewline: return (RSStringRef)RSRetain(RSSTR("<RSCharacterSet Predefined Newline Set>"));
            }
            break;
            
        case __RSCharSetClassRange:
            return RSStringCreateWithFormat(RSGetAllocator((RSCharacterSetRef)cf), RSSTR("<RSCharacterSet Range(%d, %d)>"), __RSCSetRangeFirstChar((RSCharacterSetRef)cf), __RSCSetRangeLength((RSCharacterSetRef)cf));
            
        case __RSCharSetClassString: {
            RSStringRef format = RSSTR("<RSCharacterSet Items(");
            
            length = __RSCSetStringLength((RSCharacterSetRef)cf);
            string = RSStringCreateMutable(RSGetAllocator(cf), RSStringGetLength(format) + 7 * length + 2); // length of format + "U+XXXX "(7) * length + ")>"(2)
            RSStringAppendString(string, format);
            for (idx = 0;idx < length;idx++) {
                RSStringAppendStringWithFormat(string, RSSTR("%sU+%04X"), (idx > 0 ? " " : ""), (UInt32)((__RSCSetStringBuffer((RSCharacterSetRef)cf))[idx]));
            }
            RSStringAppendString(string, RSSTR(")>"));
            return string;
        }
            
        case __RSCharSetClassBitmap:
        case __RSCharSetClassCompactBitmap:
            return (RSStringRef)RSRetain(RSSTR("<RSCharacterSet Bitmap>")); // ??? Should generate description for 8k bitmap ?
    }
    RSAssert1(0, __RSLogAssertion, "%s: Internal inconsistency error: unknown character set type", __PRETTY_FUNCTION__); // We should never come here
    return NULL;
}

static void __RSCharacterSetDeallocate(RSTypeRef cf) {
    RSAllocatorRef allocator = RSGetAllocator(cf);
    
    if (__RSCSetIsBuiltin((RSCharacterSetRef)cf) && !__RSCSetIsMutable((RSCharacterSetRef)cf) && !__RSCSetIsInverted((RSCharacterSetRef)cf)) {
        RSCharacterSetRef sharedSet = RSCharacterSetGetPredefined(__RSCSetBuiltinType((RSCharacterSetRef)cf));
        if (sharedSet == cf) { // We're trying to dealloc the builtin set
            RSAssert1(0, __RSLogAssertion, "%s: Trying to deallocate predefined set. The process is likely to crash.", __PRETTY_FUNCTION__);
            return; // We never deallocate builtin set
        }
    }
    
    if (__RSCSetIsString((RSCharacterSetRef)cf) && __RSCSetStringBuffer((RSCharacterSetRef)cf)) RSAllocatorDeallocate(allocator, __RSCSetStringBuffer((RSCharacterSetRef)cf));
    else if (__RSCSetIsBitmap((RSCharacterSetRef)cf) && __RSCSetBitmapBits((RSCharacterSetRef)cf)) RSAllocatorDeallocate(allocator, __RSCSetBitmapBits((RSCharacterSetRef)cf));
    else if (__RSCSetIsCompactBitmap((RSCharacterSetRef)cf) && __RSCSetCompactBitmapBits((RSCharacterSetRef)cf)) RSAllocatorDeallocate(allocator, __RSCSetCompactBitmapBits((RSCharacterSetRef)cf));
    __RSCSetDeallocateAnnexPlane((RSCharacterSetRef)cf);
}

static RSTypeID __RSCharacterSetTypeID = _RSRuntimeNotATypeID;

static const RSRuntimeClass __RSCharacterSetClass = {
    0,
    "RSCharacterSet",
    NULL,      // init
    NULL,      // copy
    __RSCharacterSetDeallocate,
    __RSCharacterSetEqual,
    __RSCharacterSetHash,
    __RSCharacterSetCopyDescription
};

static BOOL __RSCheckForExapendedSet = NO;

__private_extern__ void __RSCharacterSetInitialize(void) {
    const char *checkForExpandedSet = __RSRuntimeGetEnvironment("__RS_DEBUG_EXPANDED_SET");
    
    __RSCharacterSetTypeID = __RSRuntimeRegisterClass(&__RSCharacterSetClass);
    __RSRuntimeSetClassTypeID(&__RSCharacterSetClass, __RSCharacterSetTypeID);
    if (checkForExpandedSet && (*checkForExpandedSet == 'Y')) __RSCheckForExapendedSet = YES;
}

/* Public functions
 */

RSTypeID RSCharacterSetGetTypeID(void) {
    return __RSCharacterSetTypeID;
}

/*** CharacterSet creation ***/
/* Functions to create basic immutable characterset.
 */
RSCharacterSetRef RSCharacterSetGetPredefined(RSCharacterSetPredefinedSet theSetIdentifier) {
    RSCharacterSetRef cset;
    
    __RSCSetValidateBuiltinType(theSetIdentifier, __PRETTY_FUNCTION__);
    
    RSSpinLockLock(&__RSCharacterSetLock);
    cset = ((NULL != __RSBuiltinSets) ? __RSBuiltinSets[theSetIdentifier - 1] : NULL);
    RSSpinLockUnlock(&__RSCharacterSetLock);
    
    if (NULL != cset) return cset;
    
    if (!(cset = __RSCSetGenericCreate(RSAllocatorSystemDefault, __RSCharSetClassBuiltin))) return NULL;
    __RSCSetPutBuiltinType((RSMutableCharacterSetRef)cset, theSetIdentifier);
    
    RSSpinLockLock(&__RSCharacterSetLock);
    if (!__RSBuiltinSets) {
        __RSBuiltinSets = (RSCharacterSetRef *)RSAllocatorAllocate((RSAllocatorRef)RSRetain(RSAllocatorGetDefault()), sizeof(RSCharacterSetRef) * __RSLastBuiltinSetID);
        memset(__RSBuiltinSets, 0, sizeof(RSCharacterSetRef) * __RSLastBuiltinSetID);
    }
    
    __RSBuiltinSets[theSetIdentifier - 1] = cset;
    RSSpinLockUnlock(&__RSCharacterSetLock);
    
    return cset;
}

RSCharacterSetRef RSCharacterSetCreateWithCharactersInRange(RSAllocatorRef allocator, RSRange theRange) {
    RSMutableCharacterSetRef cset;
    
    __RSCSetValidateRange(theRange, __PRETTY_FUNCTION__);
    
    if (theRange.length) {
        if (!(cset = __RSCSetGenericCreate(allocator, __RSCharSetClassRange))) return NULL;
        __RSCSetPutRangeFirstChar(cset, (RSBitU32)theRange.location);
        __RSCSetPutRangeLength(cset, theRange.length);
    } else {
        if (!(cset = __RSCSetGenericCreate(allocator, __RSCharSetClassBitmap))) return NULL;
        __RSCSetPutBitmapBits(cset, NULL);
        __RSCSetPutHasHashValue(cset, YES); // _hashValue is 0
    }
    
    return cset;
}

static int chcompar(const void *a, const void *b) {
    return -(int)(*(UniChar *)b - *(UniChar *)a);
}

RSCharacterSetRef RSCharacterSetCreateWithCharactersInString(RSAllocatorRef allocator, RSStringRef theString) {
    RSIndex length;
    
    length = RSStringGetLength(theString);
    if (length < __RSStringCharSetMax) {
        RSMutableCharacterSetRef cset;
        
        if (!(cset = __RSCSetGenericCreate(allocator, __RSCharSetClassString))) return NULL;
        __RSCSetPutStringBuffer(cset, (UniChar *)RSAllocatorAllocate(RSGetAllocator(cset), __RSStringCharSetMax * sizeof(UniChar)));
        __RSCSetPutStringLength(cset, length);
        RSStringGetCharacters(theString, RSMakeRange(0, length), __RSCSetStringBuffer(cset));
        qsort(__RSCSetStringBuffer(cset), length, sizeof(UniChar), chcompar);
        
        if (0 == length) {
            __RSCSetPutHasHashValue(cset, YES); // _hashValue is 0
        } else if (length > 1) { // Check for surrogate
            const UTF16Char *characters = __RSCSetStringBuffer(cset);
            const UTF16Char *charactersLimit = characters + length;
            
            if ((*characters < 0xDC00UL) && (*(charactersLimit - 1) > 0xDBFFUL)) { // might have surrogate chars
                while (characters < charactersLimit) {
                    if (RSStringIsSurrogateHighCharacter(*characters) || RSStringIsSurrogateLowCharacter(*characters)) {
                        RSRelease(cset);
                        cset = NULL;
                        break;
                    }
                    ++characters;
                }
            }
        }
        if (NULL != cset) return cset;
    }
    
    RSMutableCharacterSetRef mcset = RSCharacterSetCreateMutable(allocator);
    RSCharacterSetAddCharactersInString(mcset, theString);
    __RSCSetMakeCompact(mcset);
    __RSCSetPutIsMutable(mcset, NO);
    return mcset;
}

RSCharacterSetRef RSCharacterSetCreateWithBitmapRepresentation(RSAllocatorRef allocator, RSDataRef theData) {
    RSMutableCharacterSetRef cset;
    RSIndex length;
    
    if (!(cset = __RSCSetGenericCreate(allocator, __RSCharSetClassBitmap))) return NULL;
    
    if (theData && (length = RSDataGetLength(theData)) > 0) {
        uint8_t *bitmap;
        uint8_t *cBitmap;
        
        if (length < __RSBitmapSize) {
            bitmap = (uint8_t *)RSAllocatorAllocate(allocator, __RSBitmapSize);
            memmove(bitmap, RSDataGetBytesPtr(theData), length);
            memset(bitmap + length, 0, __RSBitmapSize - length);
            
            cBitmap = __RSCreateCompactBitmap(allocator, bitmap);
            
            if (cBitmap == NULL) {
                __RSCSetPutBitmapBits(cset, bitmap);
            } else {
                RSAllocatorDeallocate(allocator, bitmap);
                __RSCSetPutCompactBitmapBits(cset, cBitmap);
                __RSCSetPutClassType(cset, __RSCharSetClassCompactBitmap);
            }
        } else {
            cBitmap = __RSCreateCompactBitmap(allocator, RSDataGetBytesPtr(theData));
            
            if (cBitmap == NULL) {
                bitmap = (uint8_t *)RSAllocatorAllocate(allocator, __RSBitmapSize);
                memmove(bitmap, RSDataGetBytesPtr(theData), __RSBitmapSize);
                
                __RSCSetPutBitmapBits(cset, bitmap);
            } else {
                __RSCSetPutCompactBitmapBits(cset, cBitmap);
                __RSCSetPutClassType(cset, __RSCharSetClassCompactBitmap);
            }
            
            if (length > __RSBitmapSize) {
                RSMutableCharacterSetRef annexSet;
                const uint8_t *bytes = RSDataGetBytesPtr(theData) + __RSBitmapSize;
                
                length -= __RSBitmapSize;
                
                while (length > 1) {
                    annexSet = (RSMutableCharacterSetRef)__RSCSetGetAnnexPlaneCharacterSet(cset, *(bytes++));
                    --length; // Decrement the plane no byte
                    
                    if (length < __RSBitmapSize) {
                        bitmap = (uint8_t *)RSAllocatorAllocate(allocator, __RSBitmapSize);
                        memmove(bitmap, bytes, length);
                        memset(bitmap + length, 0, __RSBitmapSize - length);
                        
                        cBitmap = __RSCreateCompactBitmap(allocator, bitmap);
                        
                        if (cBitmap == NULL) {
                            __RSCSetPutBitmapBits(annexSet, bitmap);
                        } else {
                            RSAllocatorDeallocate(allocator, bitmap);
                            __RSCSetPutCompactBitmapBits(annexSet, cBitmap);
                            __RSCSetPutClassType(annexSet, __RSCharSetClassCompactBitmap);
                        }
                    } else {
                        cBitmap = __RSCreateCompactBitmap(allocator, bytes);
                        
                        if (cBitmap == NULL) {
                            bitmap = (uint8_t *)RSAllocatorAllocate(allocator, __RSBitmapSize);
                            memmove(bitmap, bytes, __RSBitmapSize);
                            
                            __RSCSetPutBitmapBits(annexSet, bitmap);
                        } else {
                            __RSCSetPutCompactBitmapBits(annexSet, cBitmap);
                            __RSCSetPutClassType(annexSet, __RSCharSetClassCompactBitmap);
                        }
                    }
                    length -= __RSBitmapSize;
                    bytes += __RSBitmapSize;
                }
            }
        }
    } else {
        __RSCSetPutBitmapBits(cset, NULL);
        __RSCSetPutHasHashValue(cset, YES); // Hash value is 0
    }
    
    return cset;
}

RSCharacterSetRef RSCharacterSetCreateInvertedSet(RSAllocatorRef alloc, RSCharacterSetRef theSet) {
    RSMutableCharacterSetRef cset;
    
    RS_OBJC_FUNCDISPATCHV(__RSCharacterSetTypeID, RSCharacterSetRef , (NSCharacterSet *)theSet, invertedSet);
    
    cset = RSCharacterSetCreateMutableCopy(alloc, theSet);
    RSCharacterSetInvert(cset);
    __RSCSetPutIsMutable(cset, NO);
    
    return cset;
}

/* Functions to create mutable characterset.
 */
RSMutableCharacterSetRef RSCharacterSetCreateMutable(RSAllocatorRef allocator) {
    RSMutableCharacterSetRef cset;
    
    if (!(cset = __RSCSetGenericCreate(allocator, __RSCharSetClassBitmap| __RSCharSetIsMutable))) return NULL;
    __RSCSetPutBitmapBits(cset, NULL);
    __RSCSetPutHasHashValue(cset, YES); // Hash value is 0
    
    return cset;
}

static RSMutableCharacterSetRef __RSCharacterSetCreateCopy(RSAllocatorRef alloc, RSCharacterSetRef theSet, BOOL isMutable) {
    RSMutableCharacterSetRef cset;
    
    RS_OBJC_FUNCDISPATCHV(__RSCharacterSetTypeID, RSMutableCharacterSetRef , (NSCharacterSet *)theSet, mutableCopy);
    
    __RSGenericValidInstance(theSet, __RSCharacterSetTypeID);
    
    if (!isMutable && !__RSCSetIsMutable(theSet)) {
        return (RSMutableCharacterSetRef)RSRetain(theSet);
    }
    
    cset = RSCharacterSetCreateMutable(alloc);
    
    __RSCSetPutClassType(cset, __RSCSetClassType(theSet));
    __RSCSetPutHasHashValue(cset, __RSCSetHasHashValue(theSet));
    __RSCSetPutIsInverted(cset, __RSCSetIsInverted(theSet));
    cset->_hashValue = theSet->_hashValue;
    
    switch (__RSCSetClassType(theSet)) {
        case __RSCharSetClassBuiltin:
            __RSCSetPutBuiltinType(cset, __RSCSetBuiltinType(theSet));
            break;
            
        case __RSCharSetClassRange:
            __RSCSetPutRangeFirstChar(cset, __RSCSetRangeFirstChar(theSet));
            __RSCSetPutRangeLength(cset, __RSCSetRangeLength(theSet));
            break;
            
        case __RSCharSetClassString:
			__RSCSetPutStringBuffer(cset, (UniChar *)RSAllocatorAllocate(alloc, __RSStringCharSetMax * sizeof(UniChar)));
            
            __RSCSetPutStringLength(cset, __RSCSetStringLength(theSet));
            memmove(__RSCSetStringBuffer(cset), __RSCSetStringBuffer(theSet), __RSCSetStringLength(theSet) * sizeof(UniChar));
            break;
            
        case __RSCharSetClassBitmap:
            if (__RSCSetBitmapBits(theSet)) {
                uint8_t * bitmap = (isMutable ? NULL : __RSCreateCompactBitmap(alloc, __RSCSetBitmapBits(theSet)));
                
                if (bitmap == NULL) {
                    bitmap = (uint8_t *)RSAllocatorAllocate(alloc, sizeof(uint8_t) * __RSBitmapSize);
                    memmove(bitmap, __RSCSetBitmapBits(theSet), __RSBitmapSize);
                    __RSCSetPutBitmapBits(cset, bitmap);
                } else {
                    __RSCSetPutCompactBitmapBits(cset, bitmap);
                    __RSCSetPutClassType(cset, __RSCharSetClassCompactBitmap);
                }
            } else {
                __RSCSetPutBitmapBits(cset, NULL);
            }
            break;
            
        case __RSCharSetClassCompactBitmap: {
            const uint8_t *compactBitmap = __RSCSetCompactBitmapBits(theSet);
            
            if (compactBitmap) {
                uint32_t size = __RSCSetGetCompactBitmapSize(compactBitmap);
                uint8_t *newBitmap = (uint8_t *)RSAllocatorAllocate(alloc, size);
                
                memmove(newBitmap, compactBitmap, size);
                __RSCSetPutCompactBitmapBits(cset, newBitmap);
            }
        }
            break;
            
        default:
            RSAssert1(0, __RSLogAssertion, "%s: Internal inconsistency error: unknown character set type", __PRETTY_FUNCTION__); // We should never come here
    }
    if (__RSCSetHasNonBMPPlane(theSet)) {
        RSMutableCharacterSetRef annexPlane;
        int idx;
        
        for (idx = 1;idx <= MAX_ANNEX_PLANE;idx++) {
            if ((annexPlane = (RSMutableCharacterSetRef)__RSCSetGetAnnexPlaneCharacterSetNoAlloc(theSet, idx))) {
                annexPlane = __RSCharacterSetCreateCopy(alloc, annexPlane, isMutable);
                __RSCSetPutCharacterSetToAnnexPlane(cset, annexPlane, idx);
                RSRelease(annexPlane);
            }
        }
        __RSCSetAnnexSetIsInverted(cset, __RSCSetAnnexIsInverted(theSet));
    } else if (__RSCSetAnnexIsInverted(theSet)) {
        __RSCSetAnnexSetIsInverted(cset, YES);
    }
    
    return cset;
}

RSCharacterSetRef RSCharacterSetCreateCopy(RSAllocatorRef alloc, RSCharacterSetRef theSet) {
    return __RSCharacterSetCreateCopy(alloc, theSet, NO);
}

RSMutableCharacterSetRef RSCharacterSetCreateMutableCopy(RSAllocatorRef alloc, RSCharacterSetRef theSet) {
    return __RSCharacterSetCreateCopy(alloc, theSet, YES);
}

/*** Basic accessors ***/
BOOL RSCharacterSetIsCharacterMember(RSCharacterSetRef theSet, UniChar theChar) {
    RSIndex length;
    BOOL isInverted;
    BOOL result = NO;
    
    RS_OBJC_FUNCDISPATCHV(__RSCharacterSetTypeID, BOOL, (NSCharacterSet *)theSet, longCharacterIsMember:(UTF32Char)theChar);
    
    __RSGenericValidInstance(theSet, __RSCharacterSetTypeID);
    
    isInverted = __RSCSetIsInverted(theSet);
    
    switch (__RSCSetClassType(theSet)) {
        case __RSCharSetClassBuiltin:
            result = (RSUniCharIsMemberOf(theChar, (RSBitU32)__RSCSetBuiltinType(theSet)) ? !isInverted : isInverted);
            break;
            
        case __RSCharSetClassRange:
            length = __RSCSetRangeLength(theSet);
            result = (length && __RSCSetRangeFirstChar(theSet) <= theChar && theChar < __RSCSetRangeFirstChar(theSet) + length ? !isInverted : isInverted);
            break;
            
        case __RSCharSetClassString:
            result = ((length = __RSCSetStringLength(theSet)) ? (__RSCSetBsearchUniChar(__RSCSetStringBuffer(theSet), length, theChar) ? !isInverted : isInverted) : isInverted);
            break;
            
        case __RSCharSetClassBitmap:
            result = (__RSCSetCompactBitmapBits(theSet) ? (__RSCSetIsMemberBitmap(__RSCSetBitmapBits(theSet), theChar) ? YES : NO) : isInverted);
            break;
            
        case __RSCharSetClassCompactBitmap:
            result = (__RSCSetCompactBitmapBits(theSet) ? (__RSCSetIsMemberInCompactBitmap(__RSCSetCompactBitmapBits(theSet), theChar) ? YES : NO) : isInverted);
            break;
            
        default:
            RSAssert1(0, __RSLogAssertion, "%s: Internal inconsistency error: unknown character set type", __PRETTY_FUNCTION__); // We should never come here
            break;
    }
    
    return result;
}

BOOL RSCharacterSetIsLongCharacterMember(RSCharacterSetRef theSet, UTF32Char theChar) {
    RSIndex length;
    UInt32 plane = (theChar >> 16);
    BOOL isAnnexInverted = NO;
    BOOL isInverted;
    BOOL result = NO;
    
    RS_OBJC_FUNCDISPATCHV(__RSCharacterSetTypeID, BOOL, (NSCharacterSet *)theSet, longCharacterIsMember:(UTF32Char)theChar);
    
    __RSGenericValidInstance(theSet, __RSCharacterSetTypeID);
    
    if (plane) {
        RSCharacterSetRef annexPlane;
        
        if (__RSCSetIsBuiltin(theSet)) {
            isInverted = __RSCSetIsInverted(theSet);
            return (RSUniCharIsMemberOf(theChar, (RSBitU32)__RSCSetBuiltinType(theSet)) ? !isInverted : isInverted);
        }
        
        isAnnexInverted = __RSCSetAnnexIsInverted(theSet);
        
        if ((annexPlane = __RSCSetGetAnnexPlaneCharacterSetNoAlloc(theSet, plane)) == NULL) {
            if (!__RSCSetHasNonBMPPlane(theSet) && __RSCSetIsRange(theSet)) {
                isInverted = __RSCSetIsInverted(theSet);
                length = __RSCSetRangeLength(theSet);
                return (length && __RSCSetRangeFirstChar(theSet) <= theChar && theChar < __RSCSetRangeFirstChar(theSet) + length ? !isInverted : isInverted);
            } else {
                return (isAnnexInverted ? YES : NO);
            }
        } else {
            theSet = annexPlane;
            theChar &= 0xFFFF;
        }
    }
    
    isInverted = __RSCSetIsInverted(theSet);
    
    switch (__RSCSetClassType(theSet)) {
        case __RSCharSetClassBuiltin:
            result = (RSUniCharIsMemberOf(theChar, (RSBitU32)__RSCSetBuiltinType(theSet)) ? !isInverted : isInverted);
            break;
            
        case __RSCharSetClassRange:
            length = __RSCSetRangeLength(theSet);
            result = (length && __RSCSetRangeFirstChar(theSet) <= theChar && theChar < __RSCSetRangeFirstChar(theSet) + length ? !isInverted : isInverted);
            break;
            
        case __RSCharSetClassString:
            result = ((length = __RSCSetStringLength(theSet)) ? (__RSCSetBsearchUniChar(__RSCSetStringBuffer(theSet), length, theChar) ? !isInverted : isInverted) : isInverted);
            break;
            
        case __RSCharSetClassBitmap:
            result = (__RSCSetCompactBitmapBits(theSet) ? (__RSCSetIsMemberBitmap(__RSCSetBitmapBits(theSet), theChar) ? YES : NO) : isInverted);
            break;
            
        case __RSCharSetClassCompactBitmap:
            result = (__RSCSetCompactBitmapBits(theSet) ? (__RSCSetIsMemberInCompactBitmap(__RSCSetCompactBitmapBits(theSet), theChar) ? YES : NO) : isInverted);
            break;
            
        default:
            RSAssert1(0, __RSLogAssertion, "%s: Internal inconsistency error: unknown character set type", __PRETTY_FUNCTION__); // We should never come here
            return NO; // To make compiler happy
    }
    
    return (result ? !isAnnexInverted : isAnnexInverted);
}

BOOL RSCharacterSetIsSurrogatePairMember(RSCharacterSetRef theSet, UniChar surrogateHigh, UniChar surrogateLow) {
    return RSCharacterSetIsLongCharacterMember(theSet, RSCharacterSetGetLongCharacterForSurrogatePair(surrogateHigh, surrogateLow));
}


static inline RSCharacterSetRef __RSCharacterSetGetExpandedSetForNSCharacterSet(const void *characterSet) {
    RS_OBJC_FUNCDISPATCHV(__RSCharacterSetTypeID, RSCharacterSetRef , (NSCharacterSet *)characterSet, _expandedRSCharacterSet);
    return NULL;
}

BOOL RSCharacterSetIsSupersetOfSet(RSCharacterSetRef theSet, RSCharacterSetRef theOtherSet) {
    RSMutableCharacterSetRef copy;
    RSCharacterSetRef expandedSet = NULL;
    RSCharacterSetRef expandedOtherSet = NULL;
    BOOL result;
    
    if ((!RS_IS_OBJC(__RSCharacterSetTypeID, theSet) || (expandedSet = __RSCharacterSetGetExpandedSetForNSCharacterSet(theSet))) && (!RS_IS_OBJC(__RSCharacterSetTypeID, theOtherSet) || (expandedOtherSet = __RSCharacterSetGetExpandedSetForNSCharacterSet(theOtherSet)))) { // Really RS, we can do some trick here
        if (expandedSet) theSet = expandedSet;
        if (expandedOtherSet) theOtherSet = expandedOtherSet;
        
        __RSGenericValidInstance(theSet, __RSCharacterSetTypeID);
        __RSGenericValidInstance(theOtherSet, __RSCharacterSetTypeID);
        
        if (__RSCSetIsEmpty(theSet)) {
            if (__RSCSetIsInverted(theSet)) {
                return YES; // Inverted empty set covers all range
            } else if (!__RSCSetIsEmpty(theOtherSet) || __RSCSetIsInverted(theOtherSet)) {
                return NO;
            }
        } else if (__RSCSetIsEmpty(theOtherSet) && !__RSCSetIsInverted(theOtherSet)) {
            return YES;
        } else {
            if (__RSCSetIsBuiltin(theSet) || __RSCSetIsBuiltin(theOtherSet)) {
                if (__RSCSetClassType(theSet) == __RSCSetClassType(theOtherSet) && __RSCSetBuiltinType(theSet) == __RSCSetBuiltinType(theOtherSet) && !__RSCSetIsInverted(theSet) && !__RSCSetIsInverted(theOtherSet)) return YES;
            } else if (__RSCSetIsRange(theSet) || __RSCSetIsRange(theOtherSet)) {
                if (__RSCSetClassType(theSet) == __RSCSetClassType(theOtherSet)) {
                    if (__RSCSetIsInverted(theSet)) {
                        if (__RSCSetIsInverted(theOtherSet)) {
                            return (__RSCSetRangeFirstChar(theOtherSet) > __RSCSetRangeFirstChar(theSet) || (__RSCSetRangeFirstChar(theSet) + __RSCSetRangeLength(theSet)) > (__RSCSetRangeFirstChar(theOtherSet) + __RSCSetRangeLength(theOtherSet)) ? NO : YES);
                        } else {
                            return ((__RSCSetRangeFirstChar(theOtherSet) + __RSCSetRangeLength(theOtherSet)) <= __RSCSetRangeFirstChar(theSet) || (__RSCSetRangeFirstChar(theSet) + __RSCSetRangeLength(theSet)) <= __RSCSetRangeFirstChar(theOtherSet) ? YES : NO);
                        }
                    } else {
                        if (__RSCSetIsInverted(theOtherSet)) {
                            return ((__RSCSetRangeFirstChar(theSet) == 0 && __RSCSetRangeLength(theSet) == 0x110000) || (__RSCSetRangeFirstChar(theOtherSet) == 0 && (UInt32)__RSCSetRangeLength(theOtherSet) <= __RSCSetRangeFirstChar(theSet)) || ((__RSCSetRangeFirstChar(theSet) + __RSCSetRangeLength(theSet)) <= __RSCSetRangeFirstChar(theOtherSet) && (__RSCSetRangeFirstChar(theOtherSet) + __RSCSetRangeLength(theOtherSet)) == 0x110000) ? YES : NO);
                        } else {
                            return (__RSCSetRangeFirstChar(theOtherSet) < __RSCSetRangeFirstChar(theSet) || (__RSCSetRangeFirstChar(theSet) + __RSCSetRangeLength(theSet)) < (__RSCSetRangeFirstChar(theOtherSet) + __RSCSetRangeLength(theOtherSet)) ? NO : YES);
                        }
                    }
                }
            } else {
                UInt32 theSetAnnexMask = __RSCSetAnnexValidEntriesBitmap(theSet);
                UInt32 theOtherSetAnnexMask = __RSCSetAnnexValidEntriesBitmap(theOtherSet);
                BOOL isTheSetAnnexInverted = __RSCSetAnnexIsInverted(theSet);
                BOOL isTheOtherSetAnnexInverted = __RSCSetAnnexIsInverted(theOtherSet);
                uint8_t theSetBuffer[__RSBitmapSize];
                uint8_t theOtherSetBuffer[__RSBitmapSize];
                
                // We mask plane 1 to plane 16
                if (isTheSetAnnexInverted) theSetAnnexMask = (~theSetAnnexMask) & (0xFFFF << 1);
                if (isTheOtherSetAnnexInverted) theOtherSetAnnexMask = (~theOtherSetAnnexMask) & (0xFFFF << 1);
                
                __RSCSetGetBitmap(theSet, theSetBuffer);
                __RSCSetGetBitmap(theOtherSet, theOtherSetBuffer);
                
                if (!__RSCSetIsBitmapSupersetOfBitmap((const UInt32 *)theSetBuffer, (const UInt32 *)theOtherSetBuffer, NO, NO)) return NO;
                
                if (theOtherSetAnnexMask) {
                    RSCharacterSetRef theSetAnnex;
                    RSCharacterSetRef theOtherSetAnnex;
                    uint32_t idx;
                    
                    if ((theSetAnnexMask & theOtherSetAnnexMask) != theOtherSetAnnexMask) return NO;
                    
                    for (idx = 1;idx <= 16;idx++) {
                        theSetAnnex = __RSCSetGetAnnexPlaneCharacterSetNoAlloc(theSet, idx);
                        if (NULL == theSetAnnex) continue; // This case is already handled by the mask above
                        
                        theOtherSetAnnex = __RSCSetGetAnnexPlaneCharacterSetNoAlloc(theOtherSet, idx);
                        
                        if (NULL == theOtherSetAnnex) {
                            if (isTheOtherSetAnnexInverted) {
                                __RSCSetGetBitmap(theSetAnnex, theSetBuffer);
                                if (!__RSCSetIsEqualBitmap((const UInt32 *)theSetBuffer, (isTheSetAnnexInverted ? NULL : (const UInt32 *)-1))) return NO;
                            }
                        } else {
                            __RSCSetGetBitmap(theSetAnnex, theSetBuffer);
                            __RSCSetGetBitmap(theOtherSetAnnex, theOtherSetBuffer);
                            if (!__RSCSetIsBitmapSupersetOfBitmap((const UInt32 *)theSetBuffer, (const UInt32 *)theOtherSetBuffer, isTheSetAnnexInverted, isTheOtherSetAnnexInverted)) return NO;
                        }
                    }
                }
                
                return YES;
            }
        }
    }
    
    copy = RSCharacterSetCreateMutableCopy(RSAllocatorSystemDefault, theSet);
    RSCharacterSetIntersect(copy, theOtherSet);
    result = __RSCharacterSetEqual(copy, theOtherSet);
    RSRelease(copy);
    
    return result;
}

BOOL RSCharacterSetHasMemberInPlane(RSCharacterSetRef theSet, RSIndex thePlane) {
    BOOL isInverted = __RSCSetIsInverted(theSet);
    
    RS_OBJC_FUNCDISPATCHV(__RSCharacterSetTypeID, BOOL, (NSCharacterSet *)theSet, hasMemberInPlane:(uint8_t)thePlane);
    
    if (__RSCSetIsEmpty(theSet)) {
        return (isInverted ? YES : NO);
    } else if (__RSCSetIsBuiltin(theSet)) {
        RSCharacterSetPredefinedSet type = __RSCSetBuiltinType(theSet);
        
        if (type == RSCharacterSetControl) {
            if (isInverted || (thePlane == 14)) {
                return YES; // There is no plane that covers all values || Plane 14 has language tags
            } else {
                return (RSUniCharGetBitmapPtrForPlane((RSBitU32)type, (RSBitU32)thePlane) ? YES : NO);
            }
        } else if ((type < RSCharacterSetDecimalDigit) || (type == RSCharacterSetNewline)) {
            return (thePlane && !isInverted ? NO : YES);
        } else if (__RSCSetBuiltinType(theSet) == RSCharacterSetIllegal) {
            return (isInverted ? (thePlane < 3 || thePlane > 13 ? YES : NO) : YES); // This is according to Unicode 3.1
        } else {
            if (isInverted) {
                return YES; // There is no plane that covers all values
            } else {
                return (RSUniCharGetBitmapPtrForPlane((RSBitU32)type, (RSBitU32)thePlane) ? YES : NO);
            }
        }
    } else if (__RSCSetIsRange(theSet)) {
        UTF32Char firstChar = __RSCSetRangeFirstChar(theSet);
        UTF32Char lastChar = (firstChar + (RSBitU32)__RSCSetRangeLength(theSet) - 1);
        RSIndex firstPlane = firstChar >> 16;
        RSIndex lastPlane = lastChar >> 16;
        
        if (isInverted) {
            if (thePlane < firstPlane || thePlane > lastPlane) {
                return YES;
            } else if (thePlane > firstPlane && thePlane < lastPlane) {
                return NO;
            } else {
                firstChar &= 0xFFFF;
                lastChar &= 0xFFFF;
                if (thePlane == firstPlane) {
                    return (firstChar || (firstPlane == lastPlane && lastChar != 0xFFFF) ? YES : NO);
                } else {
                    return (lastChar != 0xFFFF || (firstPlane == lastPlane && firstChar) ? YES : NO);
                }
            }
        } else {
            return (thePlane < firstPlane || thePlane > lastPlane ? NO : YES);
        }
    } else {
        if (thePlane == 0) {
            switch (__RSCSetClassType(theSet)) {
                case __RSCharSetClassString: if (!__RSCSetStringLength(theSet)) return isInverted; break;
                case __RSCharSetClassCompactBitmap: return (__RSCSetCompactBitmapBits(theSet) ? YES : NO); break;
                case __RSCharSetClassBitmap: return (__RSCSetBitmapBits(theSet) ? YES : NO); break;
            }
            return YES;
        } else {
            RSCharacterSetRef annex = __RSCSetGetAnnexPlaneCharacterSetNoAlloc(theSet, (RSBitU32)thePlane);
            if (annex) {
                if (__RSCSetIsRange(annex)) {
                    return (__RSCSetAnnexIsInverted(theSet) && (__RSCSetRangeFirstChar(annex) == 0) && (__RSCSetRangeLength(annex) == 0x10000) ? NO : YES);
                } else if (__RSCSetIsBitmap(annex)) {
                    return (__RSCSetAnnexIsInverted(theSet) && __RSCSetIsEqualBitmap((const UInt32 *)__RSCSetBitmapBits(annex), (const UInt32 *)-1) ? NO : YES);
                } else {
                    uint8_t bitsBuf[__RSBitmapSize];
                    __RSCSetGetBitmap(annex, bitsBuf);
                    return (__RSCSetAnnexIsInverted(theSet) && __RSCSetIsEqualBitmap((const UInt32 *)bitsBuf, (const UInt32 *)-1) ? NO : YES);
                }
            } else {
                return __RSCSetAnnexIsInverted(theSet);
            }
        }
    }
    
    return NO;
}


RSDataRef RSCharacterSetCreateBitmapRepresentation(RSAllocatorRef alloc, RSCharacterSetRef theSet) {
    RSMutableDataRef data;
    int numNonBMPPlanes = 0;
    int planeIndices[MAX_ANNEX_PLANE];
    int idx;
    int length;
    BOOL isAnnexInverted;
    
    RS_OBJC_FUNCDISPATCHV(__RSCharacterSetTypeID, RSDataRef , (NSCharacterSet *)theSet, _retainedBitmapRepresentation);
    
    __RSGenericValidInstance(theSet, __RSCharacterSetTypeID);
    
    isAnnexInverted = (__RSCSetAnnexIsInverted(theSet) != 0);
    
    if (__RSCSetHasNonBMPPlane(theSet)) {
        for (idx = 1;idx <= MAX_ANNEX_PLANE;idx++) {
            if (isAnnexInverted || __RSCSetGetAnnexPlaneCharacterSetNoAlloc(theSet, idx)) {
                planeIndices[numNonBMPPlanes++] = idx;
            }
        }
    } else if (__RSCSetIsBuiltin(theSet)) {
        numNonBMPPlanes = (__RSCSetIsInverted(theSet) ? MAX_ANNEX_PLANE : RSUniCharGetNumberOfPlanes((RSBitU32)__RSCSetBuiltinType(theSet)) - 1);
    } else if (__RSCSetIsRange(theSet)) {
        UInt32 firstChar = __RSCSetRangeFirstChar(theSet);
        UInt32 lastChar = (RSBitU32)(__RSCSetRangeFirstChar(theSet) + __RSCSetRangeLength(theSet) - 1);
        int firstPlane = (firstChar >> 16);
        int lastPlane = (lastChar >> 16);
        BOOL isInverted = (__RSCSetIsInverted(theSet) != 0);
        
        if (lastPlane > 0) {
            if (firstPlane == 0) {
                firstPlane = 1;
                firstChar = 0x10000;
            }
            numNonBMPPlanes = (lastPlane - firstPlane) + 1;
            if (isInverted) {
                numNonBMPPlanes = MAX_ANNEX_PLANE - numNonBMPPlanes;
                if (firstPlane == lastPlane) {
                    if (((firstChar & 0xFFFF) > 0) || ((lastChar & 0xFFFF) < 0xFFFF)) ++numNonBMPPlanes;
                } else {
                    if ((firstChar & 0xFFFF) > 0) ++numNonBMPPlanes;
                    if ((lastChar & 0xFFFF) < 0xFFFF) ++numNonBMPPlanes;
                }
            }
        } else if (isInverted) {
            numNonBMPPlanes = MAX_ANNEX_PLANE;
        }
    } else if (isAnnexInverted) {
        numNonBMPPlanes = MAX_ANNEX_PLANE;
    }
    
    length = __RSBitmapSize + ((__RSBitmapSize + 1) * numNonBMPPlanes);
    data = RSDataCreateMutable(alloc, length);
    RSDataSetLength(data, length);

    __RSCSetGetBitmap(theSet, RSDataGetMutableBytesPtr(data));
    
    if (numNonBMPPlanes > 0) {
        uint8_t *bytes = RSDataGetMutableBytesPtr(data) + __RSBitmapSize;
        
        if (__RSCSetHasNonBMPPlane(theSet)) {
            RSCharacterSetRef subset;
            
            for (idx = 0;idx < numNonBMPPlanes;idx++) {
                *(bytes++) = planeIndices[idx];
                if ((subset = __RSCSetGetAnnexPlaneCharacterSetNoAlloc(theSet, planeIndices[idx])) == NULL) {
                    __RSCSetBitmapFastFillWithValue((UInt32 *)bytes, (isAnnexInverted ? 0xFF : 0));
                } else {
                    __RSCSetGetBitmap(subset, bytes);
                    if (isAnnexInverted) {
                        uint32_t count = __RSBitmapSize / sizeof(uint32_t);
                        uint32_t *bits = (uint32_t *)bytes;
                        
                        while (count-- > 0) {
                            *bits = ~(*bits);
                            ++bits;
                        }
                    }
                }
                bytes += __RSBitmapSize;
            }
        } else if (__RSCSetIsBuiltin(theSet)) {
            UInt8 result;
            RSIndex delta;
            BOOL isInverted = __RSCSetIsInverted(theSet);
            
            for (idx = 0;idx < numNonBMPPlanes;idx++) {
                if ((result = RSUniCharGetBitmapForPlane((RSBitU32)__RSCSetBuiltinType(theSet), idx + 1, bytes + 1,  (isInverted != 0))) == RSUniCharBitmapEmpty) continue;
                *(bytes++) = idx + 1;
                if (result == RSUniCharBitmapAll) {
                    RSIndex bitmapLength = __RSBitmapSize;
                    while (bitmapLength-- > 0) *(bytes++) = (uint8_t)0xFF;
                } else {
                    bytes += __RSBitmapSize;
                }
            }
            delta = bytes - (const uint8_t *)RSDataGetBytesPtr(data);
            if (delta < length) RSDataIncreaseLength(data, delta);
        } else if (__RSCSetIsRange(theSet)) {
            UInt32 firstChar = __RSCSetRangeFirstChar(theSet);
            UInt32 lastChar = (RSBitU32)(__RSCSetRangeFirstChar(theSet) + __RSCSetRangeLength(theSet) - 1);
            int firstPlane = (firstChar >> 16);
            int lastPlane = (lastChar >> 16);
            
            if (firstPlane == 0) {
                firstPlane = 1;
                firstChar = 0x10000;
            }
            if (__RSCSetIsInverted(theSet)) {
                // Mask out the plane byte
                firstChar &= 0xFFFF;
                lastChar &= 0xFFFF;
                
                for (idx = 1;idx < firstPlane;idx++) { // Fill up until the first plane
                    *(bytes++) = idx;
                    __RSCSetBitmapFastFillWithValue((UInt32 *)bytes, 0xFF);
                    bytes += __RSBitmapSize;
                }
                if (firstPlane == lastPlane) {
                    if ((firstChar > 0) || (lastChar < 0xFFFF)) {
                        *(bytes++) = idx;
                        __RSCSetBitmapFastFillWithValue((UInt32 *)bytes, 0xFF);
                        __RSCSetBitmapRemoveCharactersInRange(bytes, firstChar, lastChar);
                        bytes += __RSBitmapSize;
                    }
                } else if (firstPlane < lastPlane) {
                    if (firstChar > 0) {
                        *(bytes++) = idx;
                        __RSCSetBitmapFastFillWithValue((UInt32 *)bytes, 0);
                        __RSCSetBitmapAddCharactersInRange(bytes, 0, firstChar - 1);
                        bytes += __RSBitmapSize;
                    }
                    if (lastChar < 0xFFFF) {
                        *(bytes++) = idx;
                        __RSCSetBitmapFastFillWithValue((UInt32 *)bytes, 0);
                        __RSCSetBitmapAddCharactersInRange(bytes, lastChar, 0xFFFF);
                        bytes += __RSBitmapSize;
                    }
                }
                for (idx = lastPlane + 1;idx <= MAX_ANNEX_PLANE;idx++) {
                    *(bytes++) = idx;
                    __RSCSetBitmapFastFillWithValue((UInt32 *)bytes, 0xFF);
                    bytes += __RSBitmapSize;
                }
            } else {
                for (idx = firstPlane;idx <= lastPlane;idx++) {
                    *(bytes++) = idx;
                    __RSCSetBitmapAddCharactersInRange(bytes, (idx == firstPlane ? firstChar : 0), (idx == lastPlane ? lastChar : 0xFFFF));
                    bytes += __RSBitmapSize;
                }
            }
        } else if (isAnnexInverted) {
            for (idx = 1;idx <= MAX_ANNEX_PLANE;idx++) {
                *(bytes++) = idx;
                __RSCSetBitmapFastFillWithValue((UInt32 *)bytes, 0xFF);
                bytes += __RSBitmapSize;
            }
        }
    }
    
    return data;
}

/*** MutableCharacterSet functions ***/
void RSCharacterSetAddCharactersInRange(RSMutableCharacterSetRef theSet, RSRange theRange) {
    RS_OBJC_FUNCDISPATCHV(__RSCharacterSetTypeID, void, (NSMutableCharacterSet *)theSet, addCharactersInRange:NSMakeRange(theRange.location, theRange.length));
    
    __RSCSetValidateTypeAndMutability(theSet, __PRETTY_FUNCTION__);
    __RSCSetValidateRange(theRange, __PRETTY_FUNCTION__);
    
    if (!theRange.length || (__RSCSetIsInverted(theSet) && __RSCSetIsEmpty(theSet))) return; // Inverted && empty set contains all char
    
    if (!__RSCSetIsInverted(theSet)) {
        if (__RSCSetIsEmpty(theSet)) {
            __RSCSetPutClassType(theSet, __RSCharSetClassRange);
            __RSCSetPutRangeFirstChar(theSet, (RSBitU32)theRange.location);
            __RSCSetPutRangeLength(theSet, theRange.length);
            __RSCSetPutHasHashValue(theSet, NO);
            return;
        } else if (__RSCSetIsRange(theSet)) {
            RSIndex firstChar = __RSCSetRangeFirstChar(theSet);
            RSIndex length = __RSCSetRangeLength(theSet);
            
            if (firstChar == theRange.location) {
                __RSCSetPutRangeLength(theSet, __RSMax(length, theRange.length));
                __RSCSetPutHasHashValue(theSet, NO);
                return;
            } else if (firstChar < theRange.location && theRange.location <= firstChar + length) {
                if (firstChar + length < theRange.location + theRange.length) __RSCSetPutRangeLength(theSet, theRange.length + (theRange.location - firstChar));
                __RSCSetPutHasHashValue(theSet, NO);
                return;
            } else if (theRange.location < firstChar && firstChar <= theRange.location + theRange.length) {
                __RSCSetPutRangeFirstChar(theSet, (RSBitU32)theRange.location);
                __RSCSetPutRangeLength(theSet, length + (firstChar - theRange.location));
                __RSCSetPutHasHashValue(theSet, NO);
                return;
            }
        } else if (__RSCSetIsString(theSet) && __RSCSetStringLength(theSet) + theRange.length < __RSStringCharSetMax) {
            UniChar *buffer;
            if (!__RSCSetStringBuffer(theSet))
				__RSCSetPutStringBuffer(theSet, (UniChar *)RSAllocatorAllocate(RSGetAllocator(theSet), __RSStringCharSetMax * sizeof(UniChar)));
            buffer = __RSCSetStringBuffer(theSet) + __RSCSetStringLength(theSet);
            __RSCSetPutStringLength(theSet, __RSCSetStringLength(theSet) + theRange.length);
            while (theRange.length--) *buffer++ = (UniChar)theRange.location++;
            qsort(__RSCSetStringBuffer(theSet), __RSCSetStringLength(theSet), sizeof(UniChar), chcompar);
            __RSCSetPutHasHashValue(theSet, NO);
            return;
        }
    }
    
    // OK, I have to be a bitmap
    __RSCSetMakeBitmap(theSet);
    __RSCSetAddNonBMPPlanesInRange(theSet, theRange);
    if (theRange.location < 0x10000) { // theRange is in BMP
        if (theRange.location + theRange.length >= NUMCHARACTERS) theRange.length = NUMCHARACTERS - theRange.location;
        __RSCSetBitmapAddCharactersInRange(__RSCSetBitmapBits(theSet), (UniChar)theRange.location, (UniChar)(theRange.location + theRange.length - 1));
    }
    __RSCSetPutHasHashValue(theSet, NO);
    
    if (__RSCheckForExapendedSet) __RSCheckForExpandedSet(theSet);
}

void RSCharacterSetRemoveCharactersInRange(RSMutableCharacterSetRef theSet, RSRange theRange) {
    RS_OBJC_FUNCDISPATCHV(__RSCharacterSetTypeID, void, (NSMutableCharacterSet *)theSet, removeCharactersInRange:NSMakeRange(theRange.location, theRange.length));
    
    __RSCSetValidateTypeAndMutability(theSet, __PRETTY_FUNCTION__);
    __RSCSetValidateRange(theRange, __PRETTY_FUNCTION__);
    
    if (!theRange.length || (!__RSCSetIsInverted(theSet) && __RSCSetIsEmpty(theSet))) return; // empty set
    
    if (__RSCSetIsInverted(theSet)) {
        if (__RSCSetIsEmpty(theSet)) {
            __RSCSetPutClassType(theSet, __RSCharSetClassRange);
            __RSCSetPutRangeFirstChar(theSet, (RSBitU32)theRange.location);
            __RSCSetPutRangeLength(theSet, theRange.length);
            __RSCSetPutHasHashValue(theSet, NO);
            return;
        } else if (__RSCSetIsRange(theSet)) {
            RSIndex firstChar = __RSCSetRangeFirstChar(theSet);
            RSIndex length = __RSCSetRangeLength(theSet);
            
            if (firstChar == theRange.location) {
                __RSCSetPutRangeLength(theSet, __RSMin(length, theRange.length));
                __RSCSetPutHasHashValue(theSet, NO);
                return;
            } else if (firstChar < theRange.location && theRange.location <= firstChar + length) {
                if (firstChar + length < theRange.location + theRange.length) __RSCSetPutRangeLength(theSet, theRange.length + (theRange.location - firstChar));
                __RSCSetPutHasHashValue(theSet, NO);
                return;
            } else if (theRange.location < firstChar && firstChar <= theRange.location + theRange.length) {
                __RSCSetPutRangeFirstChar(theSet, (RSBitU32)theRange.location);
                __RSCSetPutRangeLength(theSet, length + (firstChar - theRange.location));
                __RSCSetPutHasHashValue(theSet, NO);
                return;
            }
        } else if (__RSCSetIsString(theSet) && __RSCSetStringLength(theSet) + theRange.length < __RSStringCharSetMax) {
            UniChar *buffer;
            if (!__RSCSetStringBuffer(theSet))
				__RSCSetPutStringBuffer(theSet, (UniChar *)RSAllocatorAllocate(RSGetAllocator(theSet), __RSStringCharSetMax * sizeof(UniChar)));
            buffer = __RSCSetStringBuffer(theSet) + __RSCSetStringLength(theSet);
            __RSCSetPutStringLength(theSet, __RSCSetStringLength(theSet) + theRange.length);
            while (theRange.length--) *buffer++ = (UniChar)theRange.location++;
            qsort(__RSCSetStringBuffer(theSet), __RSCSetStringLength(theSet), sizeof(UniChar), chcompar);
            __RSCSetPutHasHashValue(theSet, NO);
            return;
        }
    }
    
    // OK, I have to be a bitmap
    __RSCSetMakeBitmap(theSet);
    __RSCSetRemoveNonBMPPlanesInRange(theSet, theRange);
    if (theRange.location < 0x10000) { // theRange is in BMP
        if (theRange.location + theRange.length > NUMCHARACTERS) theRange.length = NUMCHARACTERS - theRange.location;
        if (theRange.location == 0 && theRange.length == NUMCHARACTERS) { // Remove all
            RSAllocatorDeallocate(RSGetAllocator(theSet), __RSCSetBitmapBits(theSet));
            __RSCSetPutBitmapBits(theSet, NULL);
        } else {
            __RSCSetBitmapRemoveCharactersInRange(__RSCSetBitmapBits(theSet), (UniChar)theRange.location, (UniChar)(theRange.location + theRange.length - 1));
        }
    }
    
    __RSCSetPutHasHashValue(theSet, NO);
    if (__RSCheckForExapendedSet) __RSCheckForExpandedSet(theSet);
}

void RSCharacterSetAddCharactersInString(RSMutableCharacterSetRef theSet,  RSStringRef theString) {
    UniChar *buffer;
    RSIndex length;
    BOOL hasSurrogate = NO;
    
    RS_OBJC_FUNCDISPATCHV(__RSCharacterSetTypeID, void, (NSMutableCharacterSet *)theSet, addCharactersInString:(NSString *)theString);
    
    __RSCSetValidateTypeAndMutability(theSet, __PRETTY_FUNCTION__);
    
    if ((__RSCSetIsEmpty(theSet) && __RSCSetIsInverted(theSet)) || !(length = RSStringGetLength(theString))) return;
    
    if (!__RSCSetIsInverted(theSet)) {
        RSIndex newLength = length + (__RSCSetIsEmpty(theSet) ? 0 : (__RSCSetIsString(theSet) ? __RSCSetStringLength(theSet) : __RSStringCharSetMax));
        
        if (newLength < __RSStringCharSetMax) {
            buffer = __RSCSetStringBuffer(theSet);
            
            if (NULL == buffer) {
                buffer = (UniChar *)RSAllocatorAllocate(RSGetAllocator(theSet), __RSStringCharSetMax * sizeof(UniChar));
            } else {
                buffer += __RSCSetStringLength(theSet);
            }
            
            RSStringGetCharacters(theString, RSMakeRange(0, length), (UniChar*)buffer);
            
            if (length > 1) {
                UTF16Char *characters = buffer;
                const UTF16Char *charactersLimit = characters + length;
                
                while (characters < charactersLimit) {
                    if (RSStringIsSurrogateHighCharacter(*characters) || RSStringIsSurrogateLowCharacter(*characters)) {
                        memmove(characters, characters + 1, (charactersLimit - (characters + 1)) * sizeof(*characters));
                        --charactersLimit;
                        hasSurrogate = YES;
                    } else {
                        ++characters;
                    }
                }
                
                newLength -= (length - (charactersLimit - buffer));
            }
            
            if (0 == newLength) {
                if (NULL == __RSCSetStringBuffer(theSet)) RSAllocatorDeallocate(RSGetAllocator(theSet), buffer);
            } else {
                if (NULL == __RSCSetStringBuffer(theSet)) {
                    __RSCSetPutClassType(theSet, __RSCharSetClassString);
                    __RSCSetPutStringBuffer(theSet, buffer);
                }
                __RSCSetPutStringLength(theSet, newLength);
                qsort(__RSCSetStringBuffer(theSet), newLength, sizeof(UniChar), chcompar);
            }
            __RSCSetPutHasHashValue(theSet, NO);
            
            if (hasSurrogate) __RSApplySurrogatesInString(theSet, theString, &RSCharacterSetAddCharactersInRange);
            
            return;
        }
    }
    
    // OK, I have to be a bitmap
    __RSCSetMakeBitmap(theSet);
    RSStringInlineBuffer inlineBuffer;
    RSIndex idx;
    
    RSStringInitInlineBuffer(theString, &inlineBuffer, RSMakeRange(0, length));
    
    for (idx = 0;idx < length;idx++) {
        UTF16Char character = __RSStringGetCharacterFromInlineBufferQuick(&inlineBuffer, idx);
        
        if (RSStringIsSurrogateHighCharacter(character) || RSStringIsSurrogateLowCharacter(character)) {
            hasSurrogate = YES;
        } else {
            __RSCSetBitmapAddCharacter(__RSCSetBitmapBits(theSet), character);
        }
    }
    
    __RSCSetPutHasHashValue(theSet, NO);
    
    if (__RSCheckForExapendedSet) __RSCheckForExpandedSet(theSet);
    
    if (hasSurrogate) __RSApplySurrogatesInString(theSet, theString, &RSCharacterSetAddCharactersInRange);
}

void RSCharacterSetRemoveCharactersInString(RSMutableCharacterSetRef theSet, RSStringRef theString) {
    UniChar *buffer;
    RSIndex length;
    BOOL hasSurrogate = NO;
    
    RS_OBJC_FUNCDISPATCHV(__RSCharacterSetTypeID, void, (NSMutableCharacterSet *)theSet, removeCharactersInString:(NSString *)theString);
    
    __RSCSetValidateTypeAndMutability(theSet, __PRETTY_FUNCTION__);
    
    if ((__RSCSetIsEmpty(theSet) && !__RSCSetIsInverted(theSet)) || !(length = RSStringGetLength(theString))) return;
    
    if (__RSCSetIsInverted(theSet)) {
        RSIndex newLength = length + (__RSCSetIsEmpty(theSet) ? 0 : (__RSCSetIsString(theSet) ? __RSCSetStringLength(theSet) : __RSStringCharSetMax));
        
        if (newLength < __RSStringCharSetMax) {
            buffer = __RSCSetStringBuffer(theSet);
            
            if (NULL == buffer) {
                buffer = (UniChar *)RSAllocatorAllocate(RSGetAllocator(theSet), __RSStringCharSetMax * sizeof(UniChar));
            } else {
                buffer += __RSCSetStringLength(theSet);
            }
            
            RSStringGetCharacters(theString, RSMakeRange(0, length), (UniChar*)buffer);
            
            if (length > 1) {
                UTF16Char *characters = buffer;
                const UTF16Char *charactersLimit = characters + length;
                
                while (characters < charactersLimit) {
                    if (RSStringIsSurrogateHighCharacter(*characters) || RSStringIsSurrogateLowCharacter(*characters)) {
                        memmove(characters, characters + 1, charactersLimit - (characters + 1));
                        --charactersLimit;
                        hasSurrogate = YES;
                    }
                    ++characters;
                }
                
                newLength -= (length - (charactersLimit - buffer));
            }
            
            if (NULL == __RSCSetStringBuffer(theSet)) {
                __RSCSetPutClassType(theSet, __RSCharSetClassString);
                __RSCSetPutStringBuffer(theSet, buffer);
            }
            __RSCSetPutStringLength(theSet, newLength);
            qsort(__RSCSetStringBuffer(theSet), newLength, sizeof(UniChar), chcompar);
            __RSCSetPutHasHashValue(theSet, NO);
            
            if (hasSurrogate) __RSApplySurrogatesInString(theSet, theString, &RSCharacterSetRemoveCharactersInRange);
            
            return;
        }
    }
    
    // OK, I have to be a bitmap
    __RSCSetMakeBitmap(theSet);
    RSStringInlineBuffer inlineBuffer;
    RSIndex idx;
    
    RSStringInitInlineBuffer(theString, &inlineBuffer, RSMakeRange(0, length));
    
    for (idx = 0;idx < length;idx++) {
        UTF16Char character = __RSStringGetCharacterFromInlineBufferQuick(&inlineBuffer, idx);
        
        if (RSStringIsSurrogateHighCharacter(character) || RSStringIsSurrogateLowCharacter(character)) {
            hasSurrogate = YES;
        } else {
            __RSCSetBitmapRemoveCharacter(__RSCSetBitmapBits(theSet), character);
        }
    }
    
    __RSCSetPutHasHashValue(theSet, NO);
    if (__RSCheckForExapendedSet) __RSCheckForExpandedSet(theSet);
    
    if (hasSurrogate) __RSApplySurrogatesInString(theSet, theString, &RSCharacterSetRemoveCharactersInRange);
}

void RSCharacterSetUnion(RSMutableCharacterSetRef theSet, RSCharacterSetRef theOtherSet) {
    RSCharacterSetRef expandedSet = NULL;
    
    RS_OBJC_FUNCDISPATCHV(__RSCharacterSetTypeID, void, (NSMutableCharacterSet *)theSet, formUnionWithCharacterSet:(NSCharacterSet *)theOtherSet);
    
    __RSCSetValidateTypeAndMutability(theSet, __PRETTY_FUNCTION__);
    
    if (__RSCSetIsEmpty(theSet) && __RSCSetIsInverted(theSet)) return; // Inverted empty set contains all char
    
    if (!RS_IS_OBJC(__RSCharacterSetTypeID, theOtherSet) || (expandedSet = __RSCharacterSetGetExpandedSetForNSCharacterSet(theOtherSet))) { // Really RS, we can do some trick here
        if (expandedSet) theOtherSet = expandedSet;
        
        if (__RSCSetIsEmpty(theOtherSet)) {
            if (__RSCSetIsInverted(theOtherSet)) {
                if (__RSCSetIsString(theSet) && __RSCSetStringBuffer(theSet)) {
                    RSAllocatorDeallocate(RSGetAllocator(theSet), __RSCSetStringBuffer(theSet));
                } else if (__RSCSetIsBitmap(theSet) && __RSCSetBitmapBits(theSet)) {
                    RSAllocatorDeallocate(RSGetAllocator(theSet), __RSCSetBitmapBits(theSet));
                } else if (__RSCSetIsCompactBitmap(theSet) && __RSCSetCompactBitmapBits(theSet)) {
                    RSAllocatorDeallocate(RSGetAllocator(theSet), __RSCSetCompactBitmapBits(theSet));
                }
                __RSCSetPutClassType(theSet, __RSCharSetClassRange);
                __RSCSetPutRangeLength(theSet, 0);
                __RSCSetPutIsInverted(theSet, YES);
                __RSCSetPutHasHashValue(theSet, NO);
                __RSCSetDeallocateAnnexPlane(theSet);
            }
        } else if (__RSCSetIsBuiltin(theOtherSet) && __RSCSetIsEmpty(theSet)) { // theSet can be builtin set
            __RSCSetPutClassType(theSet, __RSCharSetClassBuiltin);
            __RSCSetPutBuiltinType(theSet, __RSCSetBuiltinType(theOtherSet));
            if (__RSCSetIsInverted(theOtherSet)) __RSCSetPutIsInverted(theSet, YES);
            if (__RSCSetAnnexIsInverted(theOtherSet)) __RSCSetAnnexSetIsInverted(theSet, YES);
            __RSCSetPutHasHashValue(theSet, NO);
        } else {
            if (__RSCSetIsRange(theOtherSet)) {
                if (__RSCSetIsInverted(theOtherSet)) {
                    UTF32Char firstChar = __RSCSetRangeFirstChar(theOtherSet);
                    RSIndex length = __RSCSetRangeLength(theOtherSet);
                    
                    if (firstChar > 0) RSCharacterSetAddCharactersInRange(theSet, RSMakeRange(0, firstChar));
                    firstChar += length;
                    length = 0x110000 - firstChar;
                    RSCharacterSetAddCharactersInRange(theSet, RSMakeRange(firstChar, length));
                } else {
                    RSCharacterSetAddCharactersInRange(theSet, RSMakeRange(__RSCSetRangeFirstChar(theOtherSet), __RSCSetRangeLength(theOtherSet)));
                }
            } else if (__RSCSetIsString(theOtherSet)) {
                RSStringRef string = RSStringCreateWithCharactersNoCopy(RSGetAllocator(theSet), __RSCSetStringBuffer(theOtherSet), __RSCSetStringLength(theOtherSet), RSAllocatorNull);
                RSCharacterSetAddCharactersInString(theSet, string);
                RSRelease(string);
            } else {
                __RSCSetMakeBitmap(theSet);
                if (__RSCSetIsBitmap(theOtherSet)) {
                    UInt32 *bitmap1 = (UInt32*)__RSCSetBitmapBits(theSet);
                    UInt32 *bitmap2 = (UInt32*)__RSCSetBitmapBits(theOtherSet);
                    RSIndex length = __RSBitmapSize / sizeof(UInt32);
                    while (length--) *bitmap1++ |= *bitmap2++;
                } else {
                    UInt32 *bitmap1 = (UInt32*)__RSCSetBitmapBits(theSet);
                    UInt32 *bitmap2;
                    RSIndex length = __RSBitmapSize / sizeof(UInt32);
                    uint8_t bitmapBuffer[__RSBitmapSize];
                    __RSCSetGetBitmap(theOtherSet, bitmapBuffer);
                    bitmap2 = (UInt32*)bitmapBuffer;
                    while (length--) *bitmap1++ |= *bitmap2++;
                }
                __RSCSetPutHasHashValue(theSet, NO);
            }
            if (__RSCSetHasNonBMPPlane(theOtherSet)) {
                RSMutableCharacterSetRef otherSetPlane;
                int idx;
                
                for (idx = 1;idx <= MAX_ANNEX_PLANE;idx++) {
                    if ((otherSetPlane = (RSMutableCharacterSetRef)__RSCSetGetAnnexPlaneCharacterSetNoAlloc(theOtherSet, idx))) {
                        RSCharacterSetUnion((RSMutableCharacterSetRef)__RSCSetGetAnnexPlaneCharacterSet(theSet, idx), otherSetPlane);
                    }
                }
            } else if (__RSCSetAnnexIsInverted(theOtherSet)) {
                if (__RSCSetHasNonBMPPlane(theSet)) __RSCSetDeallocateAnnexPlane(theSet);
                __RSCSetAnnexSetIsInverted(theSet, YES);
            } else if (__RSCSetIsBuiltin(theOtherSet)) {
                RSMutableCharacterSetRef annexPlane;
                uint8_t bitmapBuffer[__RSBitmapSize];
                uint8_t result;
                int planeIndex;
                BOOL isOtherAnnexPlaneInverted = __RSCSetAnnexIsInverted(theOtherSet);
                UInt32 *bitmap1;
                UInt32 *bitmap2;
                RSIndex length;
                
                for (planeIndex = 1;planeIndex <= MAX_ANNEX_PLANE;planeIndex++) {
                    result = RSUniCharGetBitmapForPlane((RSBitU32)__RSCSetBuiltinType(theOtherSet), planeIndex, bitmapBuffer, (isOtherAnnexPlaneInverted != 0));
                    if (result != RSUniCharBitmapEmpty) {
                        annexPlane = (RSMutableCharacterSetRef)__RSCSetGetAnnexPlaneCharacterSet(theSet, planeIndex);
                        if (result == RSUniCharBitmapAll) {
                            RSCharacterSetAddCharactersInRange(annexPlane, RSMakeRange(0x0000, 0x10000));
                        } else {
                            __RSCSetMakeBitmap(annexPlane);
                            bitmap1 = (UInt32 *)__RSCSetBitmapBits(annexPlane);
                            length = __RSBitmapSize / sizeof(UInt32);
                            bitmap2 = (UInt32*)bitmapBuffer;
                            while (length--) *bitmap1++ |= *bitmap2++;
                        }
                    }
                }
            }
        }
        if (__RSCheckForExapendedSet) __RSCheckForExpandedSet(theSet);
    } else { // It's NSCharacterSet
        RSDataRef bitmapRep = RSCharacterSetCreateBitmapRepresentation(RSAllocatorSystemDefault, theOtherSet);
        const UInt32 *bitmap2 = (bitmapRep && RSDataGetLength(bitmapRep) ? (const UInt32 *)RSDataGetBytesPtr(bitmapRep) : NULL);
        if (bitmap2) {
            UInt32 *bitmap1;
            RSIndex length = __RSBitmapSize / sizeof(UInt32);
            __RSCSetMakeBitmap(theSet);
            bitmap1 = (UInt32*)__RSCSetBitmapBits(theSet);
            while (length--) *bitmap1++ |= *bitmap2++;
            __RSCSetPutHasHashValue(theSet, NO);
        }
        RSRelease(bitmapRep);
    }
}

void RSCharacterSetIntersect(RSMutableCharacterSetRef theSet, RSCharacterSetRef theOtherSet) {
    RSCharacterSetRef expandedSet = NULL;
    
    RS_OBJC_FUNCDISPATCHV(__RSCharacterSetTypeID, void, (NSMutableCharacterSet *)theSet, formIntersectionWithCharacterSet:(NSCharacterSet *)theOtherSet);
    
    __RSCSetValidateTypeAndMutability(theSet, __PRETTY_FUNCTION__);
    
    if (__RSCSetIsEmpty(theSet) && !__RSCSetIsInverted(theSet)) return; // empty set
    
    if (!RS_IS_OBJC(__RSCharacterSetTypeID, theOtherSet) || (expandedSet = __RSCharacterSetGetExpandedSetForNSCharacterSet(theOtherSet))) { // Really RS, we can do some trick here
        if (expandedSet) theOtherSet = expandedSet;
        
        if (__RSCSetIsEmpty(theOtherSet)) {
            if (!__RSCSetIsInverted(theOtherSet)) {
                if (__RSCSetIsString(theSet) && __RSCSetStringBuffer(theSet)) {
                    RSAllocatorDeallocate(RSGetAllocator(theSet), __RSCSetStringBuffer(theSet));
                } else if (__RSCSetIsBitmap(theSet) && __RSCSetBitmapBits(theSet)) {
                    RSAllocatorDeallocate(RSGetAllocator(theSet), __RSCSetBitmapBits(theSet));
                } else if (__RSCSetIsCompactBitmap(theSet) && __RSCSetCompactBitmapBits(theSet)) {
                    RSAllocatorDeallocate(RSGetAllocator(theSet), __RSCSetCompactBitmapBits(theSet));
                }
                __RSCSetPutClassType(theSet, __RSCharSetClassBitmap);
                __RSCSetPutBitmapBits(theSet, NULL);
                __RSCSetPutIsInverted(theSet, NO);
                theSet->_hashValue = 0;
                __RSCSetPutHasHashValue(theSet, YES);
                __RSCSetDeallocateAnnexPlane(theSet);
            }
        } else if (__RSCSetIsEmpty(theSet)) { // non inverted empty set contains all character
            __RSCSetPutClassType(theSet, __RSCSetClassType(theOtherSet));
            __RSCSetPutHasHashValue(theSet, __RSCSetHasHashValue(theOtherSet));
            __RSCSetPutIsInverted(theSet, __RSCSetIsInverted(theOtherSet));
            theSet->_hashValue = theOtherSet->_hashValue;
            if (__RSCSetHasNonBMPPlane(theOtherSet)) {
                RSMutableCharacterSetRef otherSetPlane;
                int idx;
                for (idx = 1;idx <= MAX_ANNEX_PLANE;idx++) {
                    if ((otherSetPlane = (RSMutableCharacterSetRef)__RSCSetGetAnnexPlaneCharacterSetNoAlloc(theOtherSet, idx))) {
                        otherSetPlane = (RSMutableCharacterSetRef)RSCharacterSetCreateMutableCopy(RSGetAllocator(theSet), otherSetPlane);
                        __RSCSetPutCharacterSetToAnnexPlane(theSet, otherSetPlane, idx);
                        RSRelease(otherSetPlane);
                    }
                }
                __RSCSetAnnexSetIsInverted(theSet, __RSCSetAnnexIsInverted(theOtherSet));
            }
            
            switch (__RSCSetClassType(theOtherSet)) {
                case __RSCharSetClassBuiltin:
                    __RSCSetPutBuiltinType(theSet, __RSCSetBuiltinType(theOtherSet));
                    break;
                    
                case __RSCharSetClassRange:
                    __RSCSetPutRangeFirstChar(theSet, __RSCSetRangeFirstChar(theOtherSet));
                    __RSCSetPutRangeLength(theSet, __RSCSetRangeLength(theOtherSet));
                    break;
                    
                case __RSCharSetClassString:
                    __RSCSetPutStringLength(theSet, __RSCSetStringLength(theOtherSet));
                    if (!__RSCSetStringBuffer(theSet))
						__RSCSetPutStringBuffer(theSet, (UniChar *)RSAllocatorAllocate(RSGetAllocator(theSet), __RSStringCharSetMax * sizeof(UniChar)));
                    memmove(__RSCSetStringBuffer(theSet), __RSCSetStringBuffer(theOtherSet), __RSCSetStringLength(theSet) * sizeof(UniChar));
                    break;
                    
                case __RSCharSetClassBitmap:
					__RSCSetPutBitmapBits(theSet, (uint8_t *)RSAllocatorAllocate(RSGetAllocator(theSet), sizeof(uint8_t) * __RSBitmapSize));
                    memmove(__RSCSetBitmapBits(theSet), __RSCSetBitmapBits(theOtherSet), __RSBitmapSize);
                    break;
                    
                case __RSCharSetClassCompactBitmap: {
                    const uint8_t *cBitmap = __RSCSetCompactBitmapBits(theOtherSet);
                    uint8_t *newBitmap;
                    uint32_t size = __RSCSetGetCompactBitmapSize(cBitmap);
                    newBitmap = (uint8_t *)RSAllocatorAllocate(RSGetAllocator(theSet), sizeof(uint8_t) * size);
                    __RSCSetPutBitmapBits(theSet, newBitmap);
                    memmove(newBitmap, cBitmap, size);
                }
                    break;
                    
                default:
                    RSAssert1(0, __RSLogAssertion, "%s: Internal inconsistency error: unknown character set type", __PRETTY_FUNCTION__); // We should never come here
            }
        } else {
            __RSCSetMakeBitmap(theSet);
            if (__RSCSetIsBitmap(theOtherSet)) {
                UInt32 *bitmap1 = (UInt32*)__RSCSetBitmapBits(theSet);
                UInt32 *bitmap2 = (UInt32*)__RSCSetBitmapBits(theOtherSet);
                RSIndex length = __RSBitmapSize / sizeof(UInt32);
                while (length--) *bitmap1++ &= *bitmap2++;
            } else {
                UInt32 *bitmap1 = (UInt32*)__RSCSetBitmapBits(theSet);
                UInt32 *bitmap2;
                RSIndex length = __RSBitmapSize / sizeof(UInt32);
                uint8_t bitmapBuffer[__RSBitmapSize];
                __RSCSetGetBitmap(theOtherSet, bitmapBuffer);
                bitmap2 = (UInt32*)bitmapBuffer;
                while (length--) *bitmap1++ &= *bitmap2++;
            }
            __RSCSetPutHasHashValue(theSet, NO);
            if (__RSCSetHasNonBMPPlane(theOtherSet)) {
                RSMutableCharacterSetRef annexPlane;
                RSMutableCharacterSetRef otherSetPlane;
                int idx;
                for (idx = 1;idx <= MAX_ANNEX_PLANE;idx++) {
                    if ((otherSetPlane = (RSMutableCharacterSetRef)__RSCSetGetAnnexPlaneCharacterSetNoAlloc(theOtherSet, idx))) {
                        annexPlane = (RSMutableCharacterSetRef)__RSCSetGetAnnexPlaneCharacterSet(theSet, idx);
                        RSCharacterSetIntersect(annexPlane, otherSetPlane);
                        if (__RSCSetIsEmpty(annexPlane) && !__RSCSetIsInverted(annexPlane)) __RSCSetPutCharacterSetToAnnexPlane(theSet, NULL, idx);
                    } else if (__RSCSetGetAnnexPlaneCharacterSetNoAlloc(theSet, idx)) {
                        __RSCSetPutCharacterSetToAnnexPlane(theSet, NULL, idx);
                    }
                }
                if (!__RSCSetHasNonBMPPlane(theSet)) __RSCSetDeallocateAnnexPlane(theSet);
            } else if (__RSCSetIsBuiltin(theOtherSet) && !__RSCSetAnnexIsInverted(theOtherSet)) {
                RSMutableCharacterSetRef annexPlane;
                uint8_t bitmapBuffer[__RSBitmapSize];
                uint8_t result;
                int planeIndex;
                UInt32 *bitmap1;
                UInt32 *bitmap2;
                RSIndex length;
                
                for (planeIndex = 1;planeIndex <= MAX_ANNEX_PLANE;planeIndex++) {
                    annexPlane = (RSMutableCharacterSetRef)__RSCSetGetAnnexPlaneCharacterSetNoAlloc(theSet, planeIndex);
                    if (annexPlane) {
                        result = RSUniCharGetBitmapForPlane((RSBitU32)__RSCSetBuiltinType(theOtherSet), planeIndex, bitmapBuffer, NO);
                        if (result == RSUniCharBitmapEmpty) {
                            __RSCSetPutCharacterSetToAnnexPlane(theSet, NULL, planeIndex);
                        } else if (result == RSUniCharBitmapFilled) {
                            BOOL isEmpty = YES;
                            
                            __RSCSetMakeBitmap(annexPlane);
                            bitmap1 = (UInt32 *)__RSCSetBitmapBits(annexPlane);
                            length = __RSBitmapSize / sizeof(UInt32);
                            bitmap2 = (UInt32*)bitmapBuffer;
                            
                            while (length--) {
                                if ((*bitmap1++ &= *bitmap2++)) isEmpty = NO;
                            }
                            if (isEmpty) __RSCSetPutCharacterSetToAnnexPlane(theSet, NULL, planeIndex);
                        }
                    }
                }
                if (!__RSCSetHasNonBMPPlane(theSet)) __RSCSetDeallocateAnnexPlane(theSet);
            } else if (__RSCSetIsRange(theOtherSet)) {
                RSMutableCharacterSetRef tempOtherSet = RSCharacterSetCreateMutable(RSGetAllocator(theSet));
                RSMutableCharacterSetRef annexPlane;
                RSMutableCharacterSetRef otherSetPlane;
                int idx;
                
                __RSCSetAddNonBMPPlanesInRange(tempOtherSet, RSMakeRange(__RSCSetRangeFirstChar(theOtherSet), __RSCSetRangeLength(theOtherSet)));
                
                for (idx = 1;idx <= MAX_ANNEX_PLANE;idx++) {
                    if ((otherSetPlane = (RSMutableCharacterSetRef)__RSCSetGetAnnexPlaneCharacterSetNoAlloc(tempOtherSet, idx))) {
                        annexPlane = (RSMutableCharacterSetRef)__RSCSetGetAnnexPlaneCharacterSet(theSet, idx);
                        RSCharacterSetIntersect(annexPlane, otherSetPlane);
                        if (__RSCSetIsEmpty(annexPlane) && !__RSCSetIsInverted(annexPlane)) __RSCSetPutCharacterSetToAnnexPlane(theSet, NULL, idx);
                    } else if (__RSCSetGetAnnexPlaneCharacterSetNoAlloc(theSet, idx)) {
                        __RSCSetPutCharacterSetToAnnexPlane(theSet, NULL, idx);
                    }
                }
                if (!__RSCSetHasNonBMPPlane(theSet)) __RSCSetDeallocateAnnexPlane(theSet);
                RSRelease(tempOtherSet);
            } else if ((__RSCSetHasNonBMPPlane(theSet) || __RSCSetAnnexIsInverted(theSet)) && !__RSCSetAnnexIsInverted(theOtherSet)) {
                __RSCSetDeallocateAnnexPlane(theSet);
            }
        }
        if (__RSCheckForExapendedSet) __RSCheckForExpandedSet(theSet);
    } else { // It's NSCharacterSet
        RSDataRef bitmapRep = RSCharacterSetCreateBitmapRepresentation(RSAllocatorSystemDefault, theOtherSet);
        const UInt32 *bitmap2 = (bitmapRep && RSDataGetLength(bitmapRep) ? (const UInt32 *)RSDataGetBytesPtr(bitmapRep) : NULL);
        if (bitmap2) {
            UInt32 *bitmap1;
            RSIndex length = __RSBitmapSize / sizeof(UInt32);
            __RSCSetMakeBitmap(theSet);
            bitmap1 = (UInt32*)__RSCSetBitmapBits(theSet);
            while (length--) *bitmap1++ &= *bitmap2++;
            __RSCSetPutHasHashValue(theSet, NO);
        }
        RSRelease(bitmapRep);
    }
}

void RSCharacterSetInvert(RSMutableCharacterSetRef theSet) {
    
    RS_OBJC_FUNCDISPATCHV(__RSCharacterSetTypeID, void, (NSMutableCharacterSet *)theSet, invert);
    
    __RSCSetValidateTypeAndMutability(theSet, __PRETTY_FUNCTION__);
    
    __RSCSetPutHasHashValue(theSet, NO);
    
    if (__RSCSetClassType(theSet) == __RSCharSetClassBitmap) {
        RSIndex idx;
        RSIndex count = __RSBitmapSize / sizeof(UInt32);
        UInt32 *bitmap = (UInt32*) __RSCSetBitmapBits(theSet);
        
        if (NULL == bitmap) {
            bitmap =  (UInt32 *)RSAllocatorAllocate(RSGetAllocator(theSet), __RSBitmapSize);
            __RSCSetPutBitmapBits(theSet, (uint8_t *)bitmap);
            for (idx = 0;idx < count;idx++) bitmap[idx] = ((UInt32)0xFFFFFFFF);
        } else {
            for (idx = 0;idx < count;idx++) bitmap[idx] = ~(bitmap[idx]);
        }
        __RSCSetAllocateAnnexForPlane(theSet, 0); // We need to alloc annex to invert
    } else if (__RSCSetClassType(theSet) == __RSCharSetClassCompactBitmap) {
        uint8_t *bitmap = __RSCSetCompactBitmapBits(theSet);
        int idx;
        int length = 0;
        uint8_t value;
        
        for (idx = 0;idx < __RSCompactBitmapNumPages;idx++) {
            value = bitmap[idx];
            
            if (value == 0) {
                bitmap[idx] = UINT8_MAX;
            } else if (value == UINT8_MAX) {
                bitmap[idx] = 0;
            } else {
                length += __RSCompactBitmapPageSize;
            }
        }
        bitmap += __RSCompactBitmapNumPages;
        for (idx = 0;idx < length;idx++) bitmap[idx] = ~(bitmap[idx]);
        __RSCSetAllocateAnnexForPlane(theSet, 0); // We need to alloc annex to invert
    } else {
        __RSCSetPutIsInverted(theSet, !__RSCSetIsInverted(theSet));
    }
    __RSCSetAnnexSetIsInverted(theSet, !__RSCSetAnnexIsInverted(theSet));
}

void RSCharacterSetCompact(RSMutableCharacterSetRef theSet) {
    if (__RSCSetIsBitmap(theSet) && __RSCSetBitmapBits(theSet)) __RSCSetMakeCompact(theSet);
    if (__RSCSetHasNonBMPPlane(theSet)) {
        RSMutableCharacterSetRef annex;
        int idx;
        
        for (idx = 1;idx <= MAX_ANNEX_PLANE;idx++) {
            if ((annex = (RSMutableCharacterSetRef)__RSCSetGetAnnexPlaneCharacterSetNoAlloc(theSet, idx)) && __RSCSetIsBitmap(annex) && __RSCSetBitmapBits(annex)) {
                __RSCSetMakeCompact(annex);
            }
        }
    }
}

void RSCharacterSetFast(RSMutableCharacterSetRef theSet) {
    if (__RSCSetIsCompactBitmap(theSet) && __RSCSetCompactBitmapBits(theSet)) __RSCSetMakeBitmap(theSet);
    if (__RSCSetHasNonBMPPlane(theSet)) {
        RSMutableCharacterSetRef annex;
        int idx;
        
        for (idx = 1;idx <= MAX_ANNEX_PLANE;idx++) {
            if ((annex = (RSMutableCharacterSetRef)__RSCSetGetAnnexPlaneCharacterSetNoAlloc(theSet, idx)) && __RSCSetIsCompactBitmap(annex) && __RSCSetCompactBitmapBits(annex)) {
                __RSCSetMakeBitmap(annex);
            }
        }
    }
}

/* Keyed-coding support
 */
RSCharacterSetKeyedCodingType _RSCharacterSetGetKeyedCodingType(RSCharacterSetRef cset) {
    if (RS_IS_OBJC(__RSCharacterSetTypeID, cset)) return RSCharacterSetKeyedCodingTypeBitmap;
    
    switch (__RSCSetClassType(cset)) {
        case __RSCharSetClassBuiltin: return ((__RSCSetBuiltinType(cset) < RSCharacterSetSymbol) ? RSCharacterSetKeyedCodingTypeBuiltin : RSCharacterSetKeyedCodingTypeBuiltinAndBitmap);
        case __RSCharSetClassRange: return RSCharacterSetKeyedCodingTypeRange;
            
        case __RSCharSetClassString: // We have to check if we have non-BMP here
            if (!__RSCSetHasNonBMPPlane(cset) && !__RSCSetAnnexIsInverted(cset)) return RSCharacterSetKeyedCodingTypeString; // BMP only. we can archive the string
            /* fallthrough */
            
        default:
            return RSCharacterSetKeyedCodingTypeBitmap;
    }
}

RSCharacterSetPredefinedSet _RSCharacterSetGetKeyedCodingBuiltinType(RSCharacterSetRef cset) { return __RSCSetBuiltinType(cset); }
RSRange _RSCharacterSetGetKeyedCodingRange(RSCharacterSetRef cset) { return RSMakeRange(__RSCSetRangeFirstChar(cset), __RSCSetRangeLength(cset)); }
RSStringRef _RSCharacterSetCreateKeyedCodingString(RSCharacterSetRef cset) { return RSStringCreateWithCharacters(RSAllocatorSystemDefault, __RSCSetStringBuffer(cset), __RSCSetStringLength(cset)); }

BOOL _RSCharacterSetIsInverted(RSCharacterSetRef cset) { return (__RSCSetIsInverted(cset) != 0); }
void _RSCharacterSetSetIsInverted(RSCharacterSetRef cset, BOOL flag) { __RSCSetPutIsInverted((RSMutableCharacterSetRef)cset, flag); }

/* Inline buffer support
 */
void RSCharacterSetInitInlineBuffer(RSCharacterSetRef cset, RSCharacterSetInlineBuffer *buffer) {
    memset(buffer, 0, sizeof(RSCharacterSetInlineBuffer));
    buffer->cset = cset;
    buffer->rangeLimit = 0x10000;
    
    if (RS_IS_OBJC(__RSCharacterSetTypeID, cset)) {
        RSCharacterSetRef expandedSet = __RSCharacterSetGetExpandedSetForNSCharacterSet(cset);
        
        if (NULL == expandedSet) {
            buffer->flags = RSCharacterSetNoBitmapAvailable;
            buffer->rangeLimit = 0x110000;
            
            return;
        } else {
            cset = expandedSet;
        }
    }
    
    switch (__RSCSetClassType(cset)) {
        case __RSCharSetClassBuiltin:
            buffer->bitmap = RSUniCharGetBitmapPtrForPlane((RSBitU32)__RSCSetBuiltinType(cset), 0);
            buffer->rangeLimit = 0x110000;
            if (NULL == buffer->bitmap) {
                buffer->flags = RSCharacterSetNoBitmapAvailable;
            } else {
                if (__RSCSetIsInverted(cset)) buffer->flags = RSCharacterSetIsInverted;
            }
            break;
            
        case __RSCharSetClassRange:
            buffer->rangeStart = __RSCSetRangeFirstChar(cset);
            buffer->rangeLimit = (RSBitU32)(__RSCSetRangeFirstChar(cset) + __RSCSetRangeLength(cset));
            if (__RSCSetIsInverted(cset)) buffer->flags = RSCharacterSetIsInverted;
            return;
            
        case __RSCharSetClassString:
            buffer->flags = RSCharacterSetNoBitmapAvailable;
            if (__RSCSetStringLength(cset) > 0) {
                buffer->rangeStart = *__RSCSetStringBuffer(cset);
                buffer->rangeLimit = *(__RSCSetStringBuffer(cset) + __RSCSetStringLength(cset) - 1) + 1;
                
                if (__RSCSetIsInverted(cset)) {
                    if (0 == buffer->rangeStart) {
                        buffer->rangeStart = buffer->rangeLimit;
                        buffer->rangeLimit = 0x10000;
                    } else if (0x10000 == buffer->rangeLimit) {
                        buffer->rangeLimit = buffer->rangeStart;
                        buffer->rangeStart = 0;
                    } else {
                        buffer->rangeStart = 0;
                        buffer->rangeLimit = 0x10000;
                    }
                }
            }
            break;
            
        case __RSCharSetClassBitmap:
        case __RSCharSetClassCompactBitmap:
            buffer->bitmap = __RSCSetCompactBitmapBits(cset);
            if (NULL == buffer->bitmap) {
                buffer->flags = RSCharacterSetIsCompactBitmap;
                if (__RSCSetIsInverted(cset)) buffer->flags |= RSCharacterSetIsInverted;
            } else {
                if (__RSCharSetClassCompactBitmap == __RSCSetClassType(cset)) buffer->flags = RSCharacterSetIsCompactBitmap;
            }
            break;
            
        default:
            RSAssert1(0, __RSLogAssertion, "%s: Internal inconsistency error: unknown character set type", __PRETTY_FUNCTION__); // We should never come here
            return;
    }
    
    if (__RSCSetAnnexIsInverted(cset)) {
        buffer->rangeLimit = 0x110000;
    } else if (__RSCSetHasNonBMPPlane(cset)) {
        RSIndex index;
        
        for (index = MAX_ANNEX_PLANE;index > 0;index--) {
            if (NULL != __RSCSetGetAnnexPlaneCharacterSetNoAlloc(cset, (int)index)) {
                buffer->rangeLimit = (RSBitU32)((index + 1) << 16);
                break;
            }
        }
    }
}
