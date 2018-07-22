//
//  RSArray.h
//  RSCoreFoundation
//
//  Created by RetVal on 10/15/12.
//  Copyright (c) 2012 RetVal. All rights reserved.
//

#ifndef RSCoreFoundation_RSArray_h
#define RSCoreFoundation_RSArray_h

#include <RSCoreFoundation/RSBase.h>

RS_EXTERN_C_BEGIN
typedef const struct __RSArray* RSArrayRef;
typedef struct __RSArray * RSMutableArrayRef;

#include <RSCoreFoundation/RSString.h>

typedef const void *	(*RSArrayRetainCallBack)(RSAllocatorRef allocator, const void *value);
typedef void		(*RSArrayReleaseCallBack)(RSAllocatorRef allocator, const void * RS_RELEASES_ARGUMENT value);
typedef RSStringRef	(*RSArrayCopyDescriptionCallBack)(const void *value);
typedef BOOL		(*RSArrayEqualCallBack)(const void *value1, const void *value2);
typedef struct {
    RSIndex				version;
    RSArrayRetainCallBack		retain;
    RSArrayReleaseCallBack		release;
    RSArrayCopyDescriptionCallBack	copyDescription;
    RSArrayEqualCallBack		equal;
} RSArrayCallBacks;

/*! @function RSArrayGetTypeID
 @abstract Return the RSTypeID type id.
 @discussion This function return the type id of RSArrayRef from the runtime.
 @result A RSTypeID the type id of RSArrayRef.
 */
RSExport RSTypeID RSArrayGetTypeID(void) RS_AVAILABLE(0_0);

/*! @function RSArrayCreateWithObject
 @abstract Return a RSArrayRef object.
 @discussion This function return a new RSArrayRef object with a object included in the array.
 @param allocator push RSAllocatorSystemDefault.
 @param obj a RSTypeRef object will be add to a new array.
 @result A RSArrayRef a new array object.
 */
RSExport RSArrayRef RSArrayCreateWithObject(RSAllocatorRef allocator, RSTypeRef obj) RS_AVAILABLE(0_0);

/*! @function RSArrayCreateWithObjects
 @abstract Return a RSArrayRef object.
 @discussion This function return a new RSArrayRef object with a object included in the array.
 @param allocator push RSAllocatorSystemDefault.
 @param objects a RSTypeRef* object will be add to a new array.
 @param count the number of objects in objects.
 @result A RSArrayRef a new array object.
 */
RSExport RSArrayRef RSArrayCreateWithObjects(RSAllocatorRef allocator, RSTypeRef* objects, RSIndex count) RS_AVAILABLE(0_0);

RSExport RSArrayRef RSArrayCreateWithCallBacks(RSAllocatorRef allocator, const void **values, RSIndex numObjects, const RSArrayCallBacks *cb) RS_AVAILABLE(0_5);

RSExport RSArrayRef RSArrayCreateWithObjectsNoCopy(RSAllocatorRef allocator, RSTypeRef* objects, RSIndex count, BOOL freeWhenDone) RS_AVAILABLE(0_0);

RSExport RSArrayRef RSArrayCreate(RSAllocatorRef allocator, RSTypeRef object, ...) RS_REQUIRES_NIL_TERMINATION RS_AVAILABLE(0_0);

RSExport RSArrayRef RSArrayCreateCopy(RSAllocatorRef allocator, RSArrayRef array) RS_AVAILABLE(0_2);

/*! @function RSArrayCreateMutable
 @abstract Return a RSArrayRef object.
 @discussion This function return a new RSMutableArrayRef object with the set capacity.
 @param allocator push RSAllocatorSystemDefault.
 @param capacity a initialize capacity for the mutable array.
 @result A RSMutableArrayRef a new mutable array object.
 */
RSExport RSMutableArrayRef RSArrayCreateMutable(RSAllocatorRef allocator, RSIndex capacity) RS_AVAILABLE(0_0);

/*! @function RSArrayCreateMutableCopy
 @abstract Return a RSMutableArrayRef object.
 @discussion This function return a RSMutableArrayRef object which is deep copy of array.
 @param allocator push RSAllocatorSystemDefault.
 @param array a initialize capacity for the mutable array.
 @result A RSMutableArrayRef a new mutable array object.
 */
RSExport RSMutableArrayRef RSArrayCreateMutableCopy(RSAllocatorRef allocator, RSArrayRef array) RS_AVAILABLE(0_0);

/*! @function RSArrayGetCount
 @abstract Return the RSIndex the count of the array.
 @discussion This function return the count of the objects in the array.
 @param array a RSArrayRef array.
 @result A RSIndex the count of the array.
 */
RSExport RSIndex    RSArrayGetCount(RSArrayRef array) RS_AVAILABLE(0_0);

/*! @function RSArrayObjectAtIndex
 @abstract Return the RSTypeRef that the object in the array at the index.
 @discussion This function return the object in the array at the index.
 @param array a RSArrayRef array.
 @param idx the index of the object in the array.
 @result A RSTypeRef the object at the index of the array with not retain.
 */
RSExport RSTypeRef  RSArrayObjectAtIndex(RSArrayRef array, RSIndex idx) RS_AVAILABLE(0_0);
RSExport void RSArraySetObjectAtIndex(RSMutableArrayRef array, RSIndex idx, const void *value) RS_AVAILABLE(0_0);
RSExport void RSArrayInsertObjectAtIndex(RSMutableArrayRef array, RSIndex idx, const void *value) RS_AVAILABLE(0_3);

RSExport RSTypeRef  RSArrayFirstObject(RSArrayRef array) RS_AVAILABLE(0_4);

/*! @function RSArrayLastObject
 @abstract Return the RSTypeRef that the object in the array at the last index.
 @discussion This function return the object in the array at the last index.
 @param array a RSArrayRef array.
 @result A RSTypeRef the object at the last index of the array with retain.
 */
RSExport RSTypeRef  RSArrayLastObject(RSArrayRef array) RS_AVAILABLE(0_0);

/*! @function RSArrayAddObject
 @abstract Return nothing.
 @discussion This function add the object to the array.
 @param array a RSMutableArrayRef array.
 @param obj a RSTypeRef that add to the array, array will retain it.
 */
RSExport void       RSArrayAddObject(RSMutableArrayRef array, RSTypeRef obj) RS_AVAILABLE(0_0);
RSExport void       RSArrayAddObjects(RSMutableArrayRef array, RSArrayRef objects) RS_AVAILABLE(0_0);

/*! @function RSArrayRemoveObject
 @abstract Return nothing.
 @discussion This function remove the object from the array.
 @param array a RSMutableArrayRef array.
 @param obj a RSTypeRef that want to remove from the array, array will release it.
 */
RSExport void       RSArrayRemoveObject(RSMutableArrayRef array, RSTypeRef obj) RS_AVAILABLE(0_0);

/*! @function RSArrayRemoveLastObject
 @abstract Return nothing.
 @discussion This function remove the last object from the array. if the array is empty, do nothing.
 @param array a RSMutableArrayRef array.
 */
RSExport void       RSArrayRemoveLastObject(RSMutableArrayRef array) RS_AVAILABLE(0_0);

/*! @function RSArrayRemoveAllObjects
 @abstract Return nothing.
 @discussion This function remove all objects from the array. if the array is empty, do nothing.
 @param array a RSMutableArrayRef array.
 */
RSExport void       RSArrayRemoveAllObjects(RSMutableArrayRef array) RS_AVAILABLE(0_0);

/*! @function RSArrayRemoveObject
 @abstract Return nothing.
 @discussion This function remove the object from the array.
 @param array a RSMutableArrayRef array.
 @param idx a RSTypeRef that want to remove from the array, array will release it.
 */
RSExport void       RSArrayRemoveObjectAtIndex(RSMutableArrayRef array, RSIndex idx) RS_AVAILABLE(0_0);

RSExport void       RSArrayReplaceObject(RSMutableArrayRef array, RSRange range, const void **newObjects, RSIndex newCount) RS_AVAILABLE(0_0);

/*! @function RSArrayIndexOfObject
 @abstract Return index of the object in the array, RSNotFound(-1) is not found.
 @discussion This function try to find the obj in the array.
 @param array a RSArrayRef array.
 @param obj a RSTypeRef that want to find.
 @result RSIndex.
 */
RSExport RSIndex    RSArrayIndexOfObject(RSArrayRef array, RSTypeRef obj) RS_AVAILABLE(0_0);
RSExport RSIndex RSArrayGetFirstIndexOfObject(RSArrayRef array, RSRange range, const void *value) RS_AVAILABLE(0_0);
/*! @function RSArrayExchangeObjectsAtIndices
 @abstract Return nothing.
 @discussion This function exchange objects at two given indexes
 @param array a RSArrayRef array.
 @param idx1 idx2 two indexes
 */
RSExport void RSArrayExchangeObjectsAtIndices(RSMutableArrayRef array, RSIndex idx1, RSIndex idx2) RS_AVAILABLE(0_0);

/*! @function RSArraySort
 @abstract Return nothing.
 @discussion This function use the comparator to sort the array.
 @param array a RSArrayRef array.
 @param order a BOOL sort the array ascending or descending.
 @param comparator a RSComparatorFunction comparator.
 @param context user context
 */
RSExport void RSArraySort(RSArrayRef array, RSComparisonOrder order, RSComparatorFunction comparator, void *context) RS_AVAILABLE(0_0);
RSExport void RSArraySortUsingComparaotr(RSArrayRef array, RSComparisonResult (^comparator)(const void *val1, const void *val2)) RS_AVAILABLE(0_4);
/*! @function RSArrayGetObjects
 @abstract Return nothing.
 @discussion This function get the objects in range of array, all objects are returned with weak link.
 @param array a RSArrayRef array.
 @param range a RSRange range.
 @param obj a RSTypeRef* obj.
 */
RSExport void RSArrayGetObjects(RSArrayRef array, RSRange range, RSTypeRef* obj) RS_AVAILABLE(0_0);
typedef void (*RSArrayApplierFunction)(const void *value, void *context);

/*! @function RSArrayApplyFunction
 @abstract Return nothing.
 @discussion This function apply the function with context to all objects in the range from array.
 @param array a RSArrayRef array.
 @param range a RSRange range.
 @param function RSArrayApplierFunction function.
 @param context void* context.
 */
RSExport void RSArrayApplyFunction(RSArrayRef array, RSRange range, RSArrayApplierFunction function, void* context) RS_AVAILABLE(0_1);

#if RS_BLOCKS_AVAILABLE
RSExport void RSArrayApplyBlock(RSArrayRef array, RSRange range, void (^block)(const void*value, RSUInteger idx, BOOL *isStop)) RS_AVAILABLE(0_1);
#endif

RSExport BOOL RSArrayContainsObject(RSArrayRef array, RSRange range, const void *value) RS_AVAILABLE(0_3);
RS_EXTERN_C_END
#endif
