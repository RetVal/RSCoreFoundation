//
//  RSSet.h
//  RSCoreFoundation
//
//  Created by RetVal on 12/8/12.
//  Copyright (c) 2012 RetVal. All rights reserved.
//

#ifndef RSCoreFoundation_RSSet_h
#define RSCoreFoundation_RSSet_h
RS_EXTERN_C_BEGIN

typedef void (*RSSetApplierFunction)(const void *value, void *context);

typedef const void *	(*RSSetRetainCallBack)(RSAllocatorRef allocator, const void *value);

/*!
 @typedef RSSetReleaseCallBack
 Type of the callback function used by RSSets for releasing a retain on values.
 @param allocator The allocator of the RSSet.
 @param value The value to release.
 */
typedef void		(*RSSetReleaseCallBack)(RSAllocatorRef allocator, const void *value);

/*!
 @typedef RSSetCopyDescriptionCallBack
 Type of the callback function used by RSSets for describing values.
 @param value The value to describe.
 @result A description of the specified value.
 */
typedef RSStringRef	(*RSSetCopyDescriptionCallBack)(const void *value);

/*!
 @typedef RSSetEqualCallBack
 Type of the callback function used by RSSets for comparing values.
 @param value1 The first value to compare.
 @param value2 The second value to compare.
 @result True if the values are equal, otherwise NO.
 */
typedef BOOL		(*RSSetEqualCallBack)(const void *value1, const void *value2);

/*!
 @typedef RSSetHashCallBack
 Type of the callback function used by RSSets for hashing values.
 @param value The value to hash.
 @result The hash of the value.
 */
typedef RSHashCode	(*RSSetHashCallBack)(const void *value);

typedef struct {
    RSIndex				version;
    RSSetRetainCallBack			retain;
    RSSetReleaseCallBack		release;
    RSSetCopyDescriptionCallBack	copyDescription;
    RSSetEqualCallBack			equal;
    RSSetHashCallBack			hash;
} RSSetCallBacks RS_AVAILABLE(0_1);

typedef const struct __RSSet* RSSetRef RS_AVAILABLE(0_1);
typedef struct __RSSet* RSMutableSetRef RS_AVAILABLE(0_1);
extern const RSSetCallBacks RSTypeSetCallBacks;
RSExport
RSTypeID RSSetGetTypeID(void) RS_AVAILABLE(0_1);

/*!
 @function RSSetCreate
 Creates a new immutable set with the given values.
 @param allocator The RSAllocator which should be used to allocate
 memory for the set and its storage for values. This
 parameter may be nil in which case the current default
 RSAllocator is used. If this reference is not a valid
 RSAllocator, the behavior is undefined.
 @param values A C array of the pointer-sized values to be in the
 set.  This C array is not changed or freed by this function.
 If this parameter is not a valid pointer to a C array of at
 least numValues pointers, the behavior is undefined.
 @param numValues The number of values to copy from the values C
 array into the RSSet. This number will be the count of the
 set.  If this parameter is zero, negative, or greater than
 the number of values actually in the values C array, the
 behavior is undefined.
 @param callBacks A C pointer to a RSSetCallBacks structure
 initialized with the callbacks for the set to use on each
 value in the set. A copy of the contents of the
 callbacks structure is made, so that a pointer to a
 structure on the stack can be passed in, or can be reused
 for multiple set creations. If the version field of this
 callbacks structure is not one of the defined ones for
 RSSet, the behavior is undefined. The retain field may be
 nil, in which case the RSSet will do nothing to add a
 retain to the contained values for the set. The release
 field may be nil, in which case the RSSet will do nothing
 to remove the set's retain (if any) on the values when the
 set is destroyed. If the copyDescription field is nil,
 the set will create a simple description for the value. If
 the equal field is nil, the set will use pointer equality
 to test for equality of values. The hash field may be nil,
 in which case the RSSet will determine uniqueness by pointer
 equality. This callbacks parameter
 itself may be nil, which is treated as if a valid structure
 of version 0 with all fields nil had been passed in.
 Otherwise, if any of the fields are not valid pointers to
 functions of the correct type, or this parameter is not a
 valid pointer to a  RSSetCallBacks callbacks structure,
 the behavior is undefined. If any of the values put into the
 set is not one understood by one of the callback functions
 the behavior when that callback function is used is
 undefined.
 @result A reference to the new immutable RSSet.
 */
RSExport
RSSetRef RSSetCreate(RSAllocatorRef allocator, const void **values, RSIndex numValues, const RSSetCallBacks *callBacks) RS_AVAILABLE(0_1);

/*!
 @function RSSetCreateCopy
 Creates a new immutable set with the values from the given set.
 @param allocator The RSAllocator which should be used to allocate
 memory for the set and its storage for values. This
 parameter may be nil in which case the current default
 RSAllocator is used. If this reference is not a valid
 RSAllocator, the behavior is undefined.
 @param theSet The set which is to be copied. The values from the
 set are copied as pointers into the new set (that is,
 the values themselves are copied, not that which the values
 point to, if anything). However, the values are also
 retained by the new set. The count of the new set will
 be the same as the copied set. The new set uses the same
 callbacks as the set to be copied. If this parameter is
 not a valid RSSet, the behavior is undefined.
 @result A reference to the new immutable RSSet.
 */
RSExport
RSSetRef RSSetCreateCopy(RSAllocatorRef allocator, RSSetRef theSet) RS_AVAILABLE(0_1);

/*!
 @function RSSetCreateMutable
 Creates a new empty mutable set.
 @param allocator The RSAllocator which should be used to allocate
 memory for the set and its storage for values. This
 parameter may be nil in which case the current default
 RSAllocator is used. If this reference is not a valid
 RSAllocator, the behavior is undefined.
 @param capacity A hint about the number of values that will be held
 by the RSSet. Pass 0 for no hint. The implementation may
 ignore this hint, or may use it to optimize various
 operations. A set's actual capacity is only limited by
 address space and available memory constraints). If this
 parameter is negative, the behavior is undefined.
 @param callBacks A C pointer to a RSSetCallBacks structure
 initialized with the callbacks for the set to use on each
 value in the set. A copy of the contents of the
 callbacks structure is made, so that a pointer to a
 structure on the stack can be passed in, or can be reused
 for multiple set creations. If the version field of this
 callbacks structure is not one of the defined ones for
 RSSet, the behavior is undefined. The retain field may be
 nil, in which case the RSSet will do nothing to add a
 retain to the contained values for the set. The release
 field may be nil, in which case the RSSet will do nothing
 to remove the set's retain (if any) on the values when the
 set is destroyed. If the copyDescription field is nil,
 the set will create a simple description for the value. If
 the equal field is nil, the set will use pointer equality
 to test for equality of values. The hash field may be nil,
 in which case the RSSet will determine uniqueness by pointer
 equality. This callbacks parameter
 itself may be nil, which is treated as if a valid structure
 of version 0 with all fields nil had been passed in.
 Otherwise, if any of the fields are not valid pointers to
 functions of the correct type, or this parameter is not a
 valid pointer to a  RSSetCallBacks callbacks structure,
 the behavior is undefined. If any of the values put into the
 set is not one understood by one of the callback functions
 the behavior when that callback function is used is
 undefined.
 @result A reference to the new mutable RSSet.
 */
RSExport
RSMutableSetRef RSSetCreateMutable(RSAllocatorRef allocator, RSIndex capacity, const RSSetCallBacks *callBacks) RS_AVAILABLE(0_1);

/*!
 @function RSSetCreateMutableCopy
 Creates a new immutable set with the values from the given set.
 @param allocator The RSAllocator which should be used to allocate
 memory for the set and its storage for values. This
 parameter may be nil in which case the current default
 RSAllocator is used. If this reference is not a valid
 RSAllocator, the behavior is undefined.
 @param capacity A hint about the number of values that will be held
 by the RSSet. Pass 0 for no hint. The implementation may
 ignore this hint, or may use it to optimize various
 operations. A set's actual capacity is only limited by
 address space and available memory constraints).
 This parameter must be greater than or equal
 to the count of the set which is to be copied, or the
 behavior is undefined. If this parameter is negative, the
 behavior is undefined.
 @param theSet The set which is to be copied. The values from the
 set are copied as pointers into the new set (that is,
 the values themselves are copied, not that which the values
 point to, if anything). However, the values are also
 retained by the new set. The count of the new set will
 be the same as the copied set. The new set uses the same
 callbacks as the set to be copied. If this parameter is
 not a valid RSSet, the behavior is undefined.
 @result A reference to the new mutable RSSet.
 */
RSExport
RSMutableSetRef RSSetCreateMutableCopy(RSAllocatorRef allocator, RSIndex capacity, RSSetRef theSet) RS_AVAILABLE(0_1);

/*!
 @function RSSetGetCount
 Returns the number of values currently in the set.
 @param theSet The set to be queried. If this parameter is not a valid
 RSSet, the behavior is undefined.
 @result The number of values in the set.
 */
RSExport
RSIndex RSSetGetCount(RSSetRef theSet) RS_AVAILABLE(0_1);

/*!
 @function RSSetGetCountOfValue
 Counts the number of times the given value occurs in the set. Since
 sets by definition contain only one instance of a value, this function
 is synonymous to RSSetContainsValue.
 @param theSet The set to be searched. If this parameter is not a
 valid RSSet, the behavior is undefined.
 @param value The value for which to find matches in the set. The
 equal() callback provided when the set was created is
 used to compare. If the equal() callback was nil, pointer
 equality (in C, ==) is used. If value, or any of the values
 in the set, are not understood by the equal() callback,
 the behavior is undefined.
 @result The number of times the given value occurs in the set.
 */
RSExport
RSIndex RSSetGetCountOfValue(RSSetRef theSet, const void *value) RS_AVAILABLE(0_1);

/*!
 @function RSSetContainsValue
 Reports whether or not the value is in the set.
 @param theSet The set to be searched. If this parameter is not a
 valid RSSet, the behavior is undefined.
 @param value The value for which to find matches in the set. The
 equal() callback provided when the set was created is
 used to compare. If the equal() callback was nil, pointer
 equality (in C, ==) is used. If value, or any of the values
 in the set, are not understood by the equal() callback,
 the behavior is undefined.
 @result YES, if the value is in the set, otherwise NO.
 */
RSExport
BOOL RSSetContainsValue(RSSetRef theSet, const void *value) RS_AVAILABLE(0_1);

/*!
 @function RSSetGetValue
 Retrieves a value in the set which hashes the same as the specified value.
 @param theSet The set to be queried. If this parameter is not a
 valid RSSet, the behavior is undefined.
 @param value The value to retrieve. The equal() callback provided when
 the set was created is used to compare. If the equal() callback
 was nil, pointer equality (in C, ==) is used. If a value, or
 any of the values in the set, are not understood by the equal()
 callback, the behavior is undefined.
 @result The value in the set with the given hash.
 */
RSExport
const void *RSSetGetValue(RSSetRef theSet, const void *value) RS_AVAILABLE(0_1);

/*!
 @function RSSetGetValueIfPresent
 Retrieves a value in the set which hashes the same as the specified value,
 if present.
 @param theSet The set to be queried. If this parameter is not a
 valid RSSet, the behavior is undefined.
 @param candidate This value is hashed and compared with values in the
 set to determine which value to retrieve. The equal() callback provided when
 the set was created is used to compare. If the equal() callback
 was nil, pointer equality (in C, ==) is used. If a value, or
 any of the values in the set, are not understood by the equal()
 callback, the behavior is undefined.
 @param value A pointer to memory which should be filled with the
 pointer-sized value if a matching value is found. If no
 match is found, the contents of the storage pointed to by
 this parameter are undefined. This parameter may be nil,
 in which case the value from the dictionary is not returned
 (but the return value of this function still indicates
 whether or not the value was present).
 @result True if the value was present in the set, otherwise NO.
 */
RSExport
BOOL RSSetGetValueIfPresent(RSSetRef theSet, const void *candidate, const void **value) RS_AVAILABLE(0_1);

/*!
 @function RSSetGetValues
 Fills the buffer with values from the set.
 @param theSet The set to be queried. If this parameter is not a
 valid RSSet, the behavior is undefined.
 @param values A C array of pointer-sized values to be filled with
 values from the set. The values in the C array are ordered
 in the same order in which they appear in the set. If this
 parameter is not a valid pointer to a C array of at least
 RSSetGetCount() pointers, the behavior is undefined.
 */
RSExport
void RSSetGetValues(RSSetRef theSet, const void **values) RS_AVAILABLE(0_1);

/*!
 @function RSSetApplyFunction
 Calls a function once for each value in the set.
 @param theSet The set to be operated upon. If this parameter is not
 a valid RSSet, the behavior is undefined.
 @param applier The callback function to call once for each value in
 the given set. If this parameter is not a
 pointer to a function of the correct prototype, the behavior
 is undefined. If there are values in the set which the
 applier function does not expect or cannot properly apply
 to, the behavior is undefined.
 @param context A pointer-sized user-defined value, which is passed
 as the second parameter to the applier function, but is
 otherwise unused by this function. If the context is not
 what is expected by the applier function, the behavior is
 undefined.
 */
RSExport
void RSSetApplyFunction(RSSetRef theSet, RSSetApplierFunction applier, void *context) RS_AVAILABLE(0_1);

#if RS_BLOCKS_AVAILABLE
RSExport void RSSetApplyBlock(RSSetRef theSet, void (^block)(const void* value, BOOL *stop)) RS_AVAILABLE(0_4);
#endif
/*!
 @function RSSetAddValue
 Adds the value to the set if it is not already present.
 @param theSet The set to which the value is to be added. If this
 parameter is not a valid mutable RSSet, the behavior is
 undefined.
 @param value The value to add to the set. The value is retained by
 the set using the retain callback provided when the set
 was created. If the value is not of the sort expected by the
 retain callback, the behavior is undefined. The count of the
 set is increased by one.
 */
RSExport
void RSSetAddValue(RSMutableSetRef theSet, const void *value) RS_AVAILABLE(0_1);

/*!
 @function RSSetReplaceValue
 Replaces the value in the set if it is present.
 @param theSet The set to which the value is to be replaced. If this
 parameter is not a valid mutable RSSet, the behavior is
 undefined.
 @param value The value to replace in the set. The equal() callback provided when
 the set was created is used to compare. If the equal() callback
 was nil, pointer equality (in C, ==) is used. If a value, or
 any of the values in the set, are not understood by the equal()
 callback, the behavior is undefined. The value is retained by
 the set using the retain callback provided when the set
 was created. If the value is not of the sort expected by the
 retain callback, the behavior is undefined. The count of the
 set is increased by one.
 */
RSExport
void RSSetReplaceValue(RSMutableSetRef theSet, const void *value) RS_AVAILABLE(0_1);

/*!
 @function RSSetSetValue
 Replaces the value in the set if it is present, or adds the value to
 the set if it is absent.
 @param theSet The set to which the value is to be replaced. If this
 parameter is not a valid mutable RSSet, the behavior is
 undefined.
 @param value The value to set in the RSSet. The equal() callback provided when
 the set was created is used to compare. If the equal() callback
 was nil, pointer equality (in C, ==) is used. If a value, or
 any of the values in the set, are not understood by the equal()
 callback, the behavior is undefined. The value is retained by
 the set using the retain callback provided when the set
 was created. If the value is not of the sort expected by the
 retain callback, the behavior is undefined. The count of the
 set is increased by one.
 */
RSExport
void RSSetSetValue(RSMutableSetRef theSet, const void *value) RS_AVAILABLE(0_1);

/*!
 @function RSSetRemoveValue
 Removes the specified value from the set.
 @param theSet The set from which the value is to be removed.
 If this parameter is not a valid mutable RSSet,
 the behavior is undefined.
 @param value The value to remove. The equal() callback provided when
 the set was created is used to compare. If the equal() callback
 was nil, pointer equality (in C, ==) is used. If a value, or
 any of the values in the set, are not understood by the equal()
 callback, the behavior is undefined.
 */
RSExport
void RSSetRemoveValue(RSMutableSetRef theSet, const void *value) RS_AVAILABLE(0_1);

/*!
 @function RSSetRemoveAllValues
 Removes all the values from the set, making it empty.
 @param theSet The set from which all of the values are to be
 removed. If this parameter is not a valid mutable RSSet,
 the behavior is undefined.
 */
RSExport
void RSSetRemoveAllValues(RSMutableSetRef theSet) RS_AVAILABLE(0_1);

RSExport RSArrayRef RSSetCopyAllValues(RSSetRef hc) RS_AVAILABLE(0_4);
RS_EXTERN_C_END



#endif
