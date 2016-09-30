//
//  RSCharacterSet.h
//  RSCoreFoundation
//
//  Created by RetVal on 8/5/13.
//  Copyright (c) 2013 RetVal. All rights reserved.
//

#ifndef RSCoreFoundation_RSCharacterSet
#define RSCoreFoundation_RSCharacterSet

#include <RSCoreFoundation/RSBase.h>

RS_EXTERN_C_BEGIN

typedef const struct __RSCharacterSet *RSCharacterSetRef;
typedef struct __RSCharacterSet *RSMutableCharacterSetRef;

typedef RS_ENUM(RSIndex, RSCharacterSetPredefinedSet) {
    RSCharacterSetControl = 1, /* Control character set (Unicode General Category Cc and Cf) */
    RSCharacterSetWhitespace, /* Whitespace character set (Unicode General Category Zs and U0009 CHARACTER TABULATION) */
    RSCharacterSetWhitespaceAndNewline,  /* Whitespace and Newline character set (Unicode General Category Z*, U000A ~ U000D, and U0085) */
    RSCharacterSetDecimalDigit, /* Decimal digit character set */
    RSCharacterSetLetter, /* Letter character set (Unicode General Category L* & M*) */
    RSCharacterSetLowercaseLetter, /* Lowercase character set (Unicode General Category Ll) */
    RSCharacterSetUppercaseLetter, /* Uppercase character set (Unicode General Category Lu and Lt) */
    RSCharacterSetNonBase, /* Non-base character set (Unicode General Category M*) */
    RSCharacterSetDecomposable, /* Canonically decomposable character set */
    RSCharacterSetAlphaNumeric, /* Alpha Numeric character set (Unicode General Category L*, M*, & N*) */
    RSCharacterSetPunctuation, /* Punctuation character set (Unicode General Category P*) */
    RSCharacterSetCapitalizedLetter = 13, /* Titlecase character set (Unicode General Category Lt) */
    RSCharacterSetSymbol = 14, /* Symbol character set (Unicode General Category S*) */
    RSCharacterSetNewline = 15, /* Newline character set (U000A ~ U000D, U0085, U2028, and U2029) */
    RSCharacterSetIllegal = 12/* Illegal character set */
};
RSExport RSTypeID RSCharacterSetGetTypeID();

/*!
 @function RSCharacterSetGetPredefined
 Returns a predefined RSCharacterSet instance.
 @param theSetIdentifier The RSCharacterSetPredefinedSet selector
 which specifies the predefined character set.  If the
 value is not in RSCharacterSetPredefinedSet, the behavior
 is undefined.
 @result A reference to the predefined immutable RSCharacterSet.
 This instance is owned by RS.
 */
RSExport RSCharacterSetRef RSCharacterSetGetPredefined(RSCharacterSetPredefinedSet theSetIdentifier);

/*!
 @function RSCharacterSetCreateWithCharactersInRange
 Creates a new immutable character set with the values from the given range.
 @param allocator The RSAllocator which should be used to allocate
 memory for the array and its storage for values. This
 parameter may be nil in which case the current default
 RSAllocator is used. If this reference is not a valid
 RSAllocator, the behavior is undefined.
 @param theRange The RSRange which should be used to specify the
 Unicode range the character set is filled with.  It
 accepts the range in 32-bit in the UTF-32 format.  The
 valid character point range is from 0x00000 to 0x10FFFF.
 If the range is outside of the valid Unicode character
 point, the behavior is undefined.
 @result A reference to the new immutable RSCharacterSet.
 */
RSExport RSCharacterSetRef RSCharacterSetCreateWithCharactersInRange(RSAllocatorRef allocator, RSRange theRange);

/*!
 @function RSCharacterSetCreateWithCharactersInString
 Creates a new immutable character set with the values in the given string.
 @param allocator The RSAllocator which should be used to allocate
 memory for the array and its storage for values. This
 parameter may be nil in which case the current default
 RSAllocator is used. If this reference is not a valid
 RSAllocator, the behavior is undefined.
 @param theString The RSString which should be used to specify
 the Unicode characters the character set is filled with.
 If this parameter is not a valid RSString, the behavior
 is undefined.
 @result A reference to the new immutable RSCharacterSet.
 */
RSExport RSCharacterSetRef RSCharacterSetCreateWithCharactersInString(RSAllocatorRef allocator, RSStringRef theString);

/*!
 @function RSCharacterSetCreateWithBitmapRepresentation
 Creates a new immutable character set with the bitmap representtion in the given data.
 @param allocator The RSAllocator which should be used to allocate
 memory for the array and its storage for values. This
 parameter may be nil in which case the current default
 RSAllocator is used. If this reference is not a valid
 RSAllocator, the behavior is undefined.
 @param theData The RSData which should be used to specify the
 bitmap representation of the Unicode character points
 the character set is filled with.  The bitmap
 representation could contain all the Unicode character
 range starting from BMP to Plane 16.  The first 8192 bytes
 of the data represent the BMP range.  The BMP range 8192
 bytes can be followed by zero to sixteen 8192 byte
 bitmaps, each one with the plane index byte prepended.
 For example, the bitmap representing the BMP and Plane 2
 has the size of 16385 bytes (8192 bytes for BMP, 1 byte
 index + 8192 bytes bitmap for Plane 2).  The plane index
 byte, in this case, contains the integer value two.  If
 this parameter is not a valid RSData or it contains a
 Plane index byte outside of the valid Plane range
 (1 to 16), the behavior is undefined.
 @result A reference to the new immutable RSCharacterSet.
 */
RSExport RSCharacterSetRef RSCharacterSetCreateWithBitmapRepresentation(RSAllocatorRef allocator, RSDataRef theData);

/*!
 @function RSCharacterSetCreateInvertedSet
 Creates a new immutable character set that is the invert of the specified character set.
 @param allocator The RSAllocator which should be used to allocate
 memory for the array and its storage for values. This
 parameter may be nil in which case the current default
 RSAllocator is used. If this reference is not a valid
 RSAllocator, the behavior is undefined.
 @param theSet The RSCharacterSet which is to be inverted.  If this
 parameter is not a valid RSCharacterSet, the behavior is
 undefined.
 @result A reference to the new immutable RSCharacterSet.
 */
RSExport RSCharacterSetRef RSCharacterSetCreateInvertedSet(RSAllocatorRef allocator, RSCharacterSetRef theSet);

/*!
 @function RSCharacterSetIsSupersetOfSet
 Reports whether or not the character set is a superset of the character set specified as the second parameter.
 @param theSet  The character set to be checked for the membership of theOtherSet.
 If this parameter is not a valid RSCharacterSet, the behavior is undefined.
 @param theOtherset  The character set to be checked whether or not it is a subset of theSet.
 If this parameter is not a valid RSCharacterSet, the behavior is undefined.
 */
RSExport BOOL RSCharacterSetIsSupersetOfSet(RSCharacterSetRef theSet, RSCharacterSetRef theOtherset);

/*!
 @function RSCharacterSetHasMemberInPlane
 Reports whether or not the character set contains at least one member character in the specified plane.
 @param theSet  The character set to be checked for the membership.  If this
 parameter is not a valid RSCharacterSet, the behavior is undefined.
 @param thePlane  The plane number to be checked for the membership.
 The valid value range is from 0 to 16.  If the value is outside of the valid
 plane number range, the behavior is undefined.
 */
RSExport BOOL RSCharacterSetHasMemberInPlane(RSCharacterSetRef theSet, RSIndex thePlane);

/*!
 @function RSCharacterSetCreateMutable
 Creates a new empty mutable character set.
 @param allocator The RSAllocator which should be used to allocate
 memory for the array and its storage for values. This
 parameter may be nil in which case the current default
 RSAllocator is used. If this reference is not a valid
 RSAllocator, the behavior is undefined.
 @result A reference to the new mutable RSCharacterSet.
 */
RSExport RSMutableCharacterSetRef RSCharacterSetCreateMutable(RSAllocatorRef allocator);

/*!
 @function RSCharacterSetCreateCopy
 Creates a new character set with the values from the given character set.  This function tries to compact the backing store where applicable.
 @param allocator The RSAllocator which should be used to allocate
 memory for the array and its storage for values. This
 parameter may be nil in which case the current default
 RSAllocator is used. If this reference is not a valid
 RSAllocator, the behavior is undefined.
 @param theSet The RSCharacterSet which is to be copied.  If this
 parameter is not a valid RSCharacterSet, the behavior is
 undefined.
 @result A reference to the new RSCharacterSet.
 */
RSExport RSCharacterSetRef RSCharacterSetCreateCopy(RSAllocatorRef allocator, RSCharacterSetRef theSet);

/*!
 @function RSCharacterSetCreateMutableCopy
 Creates a new mutable character set with the values from the given character set.
 @param allocator The RSAllocator which should be used to allocate
 memory for the array and its storage for values. This
 parameter may be nil in which case the current default
 RSAllocator is used. If this reference is not a valid
 RSAllocator, the behavior is undefined.
 @param theSet The RSCharacterSet which is to be copied.  If this
 parameter is not a valid RSCharacterSet, the behavior is
 undefined.
 @result A reference to the new mutable RSCharacterSet.
 */
RSExport RSMutableCharacterSetRef RSCharacterSetCreateMutableCopy(RSAllocatorRef allocator, RSCharacterSetRef theSet);

/*!
 @function RSCharacterSetIsCharacterMember
 Reports whether or not the Unicode character is in the character set.
 @param theSet The character set to be searched. If this parameter
 is not a valid RSCharacterSet, the behavior is undefined.
 @param theChar The Unicode character for which to test against the
 character set.  Note that this function takes 16-bit Unicode
 character value; hence, it does not support access to the
 non-BMP planes.
 @result YES, if the value is in the character set, otherwise NO.
 */
RSExport BOOL RSCharacterSetIsCharacterMember(RSCharacterSetRef theSet, UniChar theChar);

/*!
 @function RSCharacterSetIsLongCharacterMember
 Reports whether or not the UTF-32 character is in the character set.
 @param theSet The character set to be searched. If this parameter
 is not a valid RSCharacterSet, the behavior is undefined.
 @param theChar The UTF-32 character for which to test against the
 character set.
 @result YES, if the value is in the character set, otherwise NO.
 */
RSExport BOOL RSCharacterSetIsLongCharacterMember(RSCharacterSetRef theSet, UTF32Char theChar);

/*!
 @function RSCharacterSetCreateBitmapRepresentation
 Creates a new immutable data with the bitmap representation from the given character set.
 @param allocator The RSAllocator which should be used to allocate
 memory for the array and its storage for values. This
 parameter may be nil in which case the current default
 RSAllocator is used. If this reference is not a valid
 RSAllocator, the behavior is undefined.
 @param theSet The RSCharacterSet which is to be used create the
 bitmap representation from.  Refer to the comments for
 RSCharacterSetCreateWithBitmapRepresentation for the
 detailed discussion of the bitmap representation format.
 If this parameter is not a valid RSCharacterSet, the
 behavior is undefined.
 @result A reference to the new immutable RSData.
 */
RSExport RSDataRef RSCharacterSetCreateBitmapRepresentation(RSAllocatorRef allocator, RSCharacterSetRef theSet);

/*!
 @function RSCharacterSetAddCharactersInRange
 Adds the given range to the charaacter set.
 @param theSet The character set to which the range is to be added.
 If this parameter is not a valid mutable RSCharacterSet,
 the behavior is undefined.
 @param theRange The range to add to the character set.  It accepts
 the range in 32-bit in the UTF-32 format.  The valid
 character point range is from 0x00000 to 0x10FFFF.  If the
 range is outside of the valid Unicode character point,
 the behavior is undefined.
 */
RSExport void RSCharacterSetAddCharactersInRange(RSMutableCharacterSetRef theSet, RSRange theRange);

/*!
 @function RSCharacterSetRemoveCharactersInRange
 Removes the given range from the charaacter set.
 @param theSet The character set from which the range is to be
 removed.  If this parameter is not a valid mutable
 RSCharacterSet, the behavior is undefined.
 @param theRange The range to remove from the character set.
 It accepts the range in 32-bit in the UTF-32 format.
 The valid character point range is from 0x00000 to 0x10FFFF.
 If the range is outside of the valid Unicode character point,
 the behavior is undefined.
 */
RSExport void RSCharacterSetRemoveCharactersInRange(RSMutableCharacterSetRef theSet, RSRange theRange);

/*!
 @function RSCharacterSetAddCharactersInString
 Adds the characters in the given string to the charaacter set.
 @param theSet The character set to which the characters in the
 string are to be added.  If this parameter is not a
 valid mutable RSCharacterSet, the behavior is undefined.
 @param theString The string to add to the character set.
 If this parameter is not a valid RSString, the behavior
 is undefined.
 */
RSExport void RSCharacterSetAddCharactersInString(RSMutableCharacterSetRef theSet,  RSStringRef theString);

/*!
 @function RSCharacterSetRemoveCharactersInString
 Removes the characters in the given string from the charaacter set.
 @param theSet The character set from which the characters in the
 string are to be remove.  If this parameter is not a
 valid mutable RSCharacterSet, the behavior is undefined.
 @param theString The string to remove from the character set.
 If this parameter is not a valid RSString, the behavior
 is undefined.
 */
RSExport void RSCharacterSetRemoveCharactersInString(RSMutableCharacterSetRef theSet, RSStringRef theString);

/*!
 @function RSCharacterSetUnion
 Forms the union with the given character set.
 @param theSet The destination character set into which the
 union of the two character sets is stored.  If this
 parameter is not a valid mutable RSCharacterSet, the
 behavior is undefined.
 @param theOtherSet The character set with which the union is
 formed.  If this parameter is not a valid RSCharacterSet,
 the behavior is undefined.
 */
RSExport void RSCharacterSetUnion(RSMutableCharacterSetRef theSet, RSCharacterSetRef theOtherSet);

/*!
 @function RSCharacterSetIntersect
 Forms the intersection with the given character set.
 @param theSet The destination character set into which the
 intersection of the two character sets is stored.
 If this parameter is not a valid mutable RSCharacterSet,
 the behavior is undefined.
 @param theOtherSet The character set with which the intersection
 is formed.  If this parameter is not a valid RSCharacterSet,
 the behavior is undefined.
 */
RSExport void RSCharacterSetIntersect(RSMutableCharacterSetRef theSet, RSCharacterSetRef theOtherSet);

/*!
 @function RSCharacterSetInvert
 Inverts the content of the given character set.
 @param theSet The character set to be inverted.
 If this parameter is not a valid mutable RSCharacterSet,
 the behavior is undefined.
 */
RSExport void RSCharacterSetInvert(RSMutableCharacterSetRef theSet);

RS_EXTERN_C_END
#endif 
