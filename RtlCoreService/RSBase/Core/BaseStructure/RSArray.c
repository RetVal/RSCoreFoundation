//
//  RSArray.c
//  RSCoreFoundation
//
//  Created by RetVal on 10/15/12.
//  Copyright (c) 2012 RetVal. All rights reserved.
//

#include <RSCoreFoundation/RSBase.h>
#include <RSCoreFoundation/RSNil.h>
#include <RSCoreFoundation/RSArray.h>
#include <RSCoreFoundation/RSString.h>
#include <RSCoreFoundation/RSRuntime.h>
#include "RSSortFunctions.h"

#define RSARRAY_VERSION   3

#if RSARRAY_VERSION == 1
struct __RSArray {
    RSRuntimeBase _base;    // the runtime system support.
    volatile RSSpinLock lock;// the sync lock for the array.
    RSIndex _cnt;           // the number of the objects in the store.
    RSIndex _capacity;      // the real size of the store.
    RSIndex _storePerSize;  // the size of the objects in the store. sizeof(ISA) || sizeof(RSTypeRef)
    RSComparatorFunction _compare;// the compare call backs for compare objects in the store. support to sort array.
    RSTypeRef* _store;      // the store block for store the objects in the array.
};

/**
 RSRuntimeBaseInfo
 */
enum {
    _RSArrayStrongMemory   = 1 << 0L,  // 0 = strong, 1 = weak
    _RSArrayMutable        = 1 << 1L,  // 0 = const, 1 = mutable
    _RSArrayDeallocated    = 1 << 2L,  // 0 = alive, 1 = dead
    _RSArraySorted         = 1 << 3L,
    _RSArrayAscending      = 1 << 4L,
    _RSArrayNoCallBack     = 1 << 5L,
    
    
    _RSArrayStrongMemoryMask = _RSArrayStrongMemory,
    _RSArrayMutableMask = _RSArrayMutable,
    _RSArrayDeallocatedMask = _RSArrayDeallocated,
    _RSArraySortedMask = _RSArraySorted,
    _RSArrayAscendingMask = _RSArrayAscending,
    _RSArrayNoCallBackMask = _RSArrayNoCallBack,
};
RSInline void __RSArrayLockInstance(RSArrayRef array)
{
    //RSSpinLockLock(&((struct __RSArray*)array)->lock);
}

RSInline void __RSArrayUnLockInstance(RSArrayRef array)
{
    //RSSpinLockUnlock(&((struct __RSArray*)array)->lock);
}

RSInline BOOL   isMutable(RSArrayRef array)
{
    return (array) ? (array->_base._rsinfo._rsinfo1 & _RSArrayMutable) == _RSArrayMutableMask : (NO);
}

RSInline void   markMutable(RSArrayRef array)
{
    //__RSArrayLockInstance(array);
    (((struct __RSArray*)array)->_base._rsinfo._rsinfo1 |= _RSArrayMutable);
    //__RSArrayUnLockInstance(array);
}

RSInline void   unMarkMutable(RSArrayRef array)
{
    //__RSArrayLockInstance(array);
    (((struct __RSArray*)array)->_base._rsinfo._rsinfo1 &= ~_RSArrayMutable);
    //__RSArrayUnLockInstance(array);
}

RSInline BOOL   isDeallocated(RSArrayRef array)
{
    return (array->_base._rsinfo._rsinfo1 & _RSArrayDeallocated) == _RSArrayDeallocatedMask;
}

RSInline void   markDeallocated(RSArrayRef array)
{
    //__RSArrayLockInstance(array);
    (((struct __RSArray*)array)->_base._rsinfo._rsinfo1 |= _RSArrayDeallocated);
    //__RSArrayUnLockInstance(array);
}

RSInline BOOL   isStrong(RSArrayRef array)
{
    return (array->_base._rsinfo._rsinfo1 & _RSArrayStrongMemory) == _RSArrayStrongMemoryMask;
}

RSInline void   markStrong(RSArrayRef array)
{
    //__RSArrayLockInstance(array);
    (((struct __RSArray*)array)->_base._rsinfo._rsinfo1 |= _RSArrayStrongMemory);
    //__RSArrayUnLockInstance(array);
}

RSInline void   unMarkStrong(RSArrayRef array)
{
    //__RSArrayLockInstance(array);
    (((struct __RSArray*)array)->_base._rsinfo._rsinfo1 &= ~_RSArrayStrongMemory);
    //__RSArrayUnLockInstance(array);
}

RSInline BOOL   isSorted(RSArrayRef array)
{
    return (array->_base._rsinfo._rsinfo1 & _RSArraySorted) == _RSArraySortedMask;
}

RSInline void   markSorted(RSArrayRef array)
{
    ((RSMutableArrayRef)array)->_base._rsinfo._rsinfo1 |= _RSArraySorted;
}

RSInline void   unMarkSorted(RSArrayRef array)
{
    ((RSMutableArrayRef)array)->_base._rsinfo._rsinfo1 &= ~_RSArraySorted;
}

RSInline BOOL   isAscending(RSArrayRef array)
{
    return (array->_base._rsinfo._rsinfo1 & _RSArrayAscending) == _RSArrayAscendingMask;
}

RSInline void   markAscending(RSArrayRef arrray)
{
    ((RSMutableArrayRef)arrray)->_base._rsinfo._rsinfo1 |= _RSArrayAscending;
}

RSInline void   markDescending(RSArrayRef array)
{
    ((RSMutableArrayRef)array)->_base._rsinfo._rsinfo1 &= ~_RSArrayAscending;
}

RSInline BOOL isNoCallBack(RSArrayRef array)
{
    return (array->_base._rsinfo._rsinfo1 & _RSArrayNoCallBackMask) == _RSArrayNoCallBack;
}

RSInline BOOL setNoCallBack(RSArrayRef array)
{
    return (((RSMutableArrayRef)array)->_base._rsinfo._rsinfo1 |= _RSArrayNoCallBack);
}


RSInline RSHandle __RSArrayContent(RSArrayRef array)
{
    return array->_store;
}

RSInline RSIndex __RSArrayCount(RSArrayRef array)
{
    return (array) ? array->_cnt : (0);
}
RSInline void __RSArraySetCount(RSArrayRef array, RSIndex count)
{
    //__RSArrayLockInstance(array);
    ((struct __RSArray*)array)->_cnt = count;
    //__RSArrayUnLockInstance(array);
}
RSInline RSIndex __RSArrayCapacity(RSArrayRef array)
{
    return (array) ? array->_capacity : 0;
}
RSInline void __RSArraySetCapacity(RSArrayRef array, RSIndex capacity)
{
    //__RSArrayLockInstance(array);
    ((struct __RSArray*)array)->_capacity = capacity;
    //__RSArrayUnLockInstance(array);
}
RSInline RSIndex __RSArrayStorePerSize(RSArrayRef array)
{
    return array->_storePerSize;
}
RSInline void __RSArraySetStorePerSize(RSArrayRef array, RSIndex persize)
{
    //__RSArrayLockInstance(array);
    ((struct __RSArray*)array)->_storePerSize = persize;
    //__RSArrayUnLockInstance(array);
}



RSInline void __RSArraySetStrongStore(RSArrayRef array, RSTypeRef* store)
{
    //__RSArrayLockInstance(array);
    ((RSMutableArrayRef)array)->_store = store;
    //__RSArrayUnLockInstance(array);
    markStrong(array);
}
RSInline void __RSArraySetWeakStore(RSArrayRef array, RSTypeRef* store)
{
    //__RSArrayLockInstance(array);
    ((RSMutableArrayRef)array)->_store = store;
    //__RSArrayUnLockInstance(array);
    unMarkStrong(array);
}
RSInline RSTypeRef* __RSArrayStore(RSArrayRef array)
{
    return array->_store;
}

RSInline RSComparatorFunction __RSArrayComparator(RSArrayRef array)
{
    return array->_compare;
}

RSInline void __RSArraySetComparator(RSArrayRef array, RSComparatorFunction comparator)
{
    ((RSMutableArrayRef)array)->_compare = comparator;
}

static void __RSArrayAvailable(RSArrayRef array)
{
    if (array == nil) HALTWithError(RSInvalidArgumentException, "array is nil");
    __RSGenericValidInstance(array, RSArrayGetTypeID());
    return;
}

static void __RSArrayAvailableRange(RSArrayRef array, RSRange range)
{
    RSIndex cnt = __RSArrayCount(array);
    if (range.location + range.length > cnt) {
        HALTWithError(RSRangeException, "the range for RSArray is not available");
    }
    return;
}

/*
 __RSArrayCreateTable create a new block of memory, the size of the block is the cnt * __RSArrayStorePerSize,
 and get the new capacity from the RSAllocatorSize, set the new table pointer to the array, set the new capacity to
 the array. return the new table pointer for caller.
 if the array has a old pointer of table, the table will be freed by RSAllocatorDeallocate.
 
 */
static RSTypeRef* __RSArrayCreateTable(RSMutableArrayRef array, RSIndex cnt)
{
    RSTypeRef* table = __RSArrayStore(array);
    if (table) RSAllocatorDeallocate(RSAllocatorSystemDefault, table);
    RSIndex capacity = RSAllocatorSize(RSAllocatorSystemDefault, cnt*__RSArrayStorePerSize(array));
    __RSCLog(RSLogLevelDebug, "%s want new count is : %lld\n","__RSArrayCreateTable", capacity / __RSArrayStorePerSize(array));
    table = RSAllocatorAllocate(RSAllocatorSystemDefault, capacity);
    __RSArraySetStrongStore(array, table);
    capacity = RSAllocatorSize(RSAllocatorSystemDefault, capacity);
    __RSCLog(RSLogLevelDebug, "%s real new count is : %lld\n","__RSArrayCreateTable", capacity / __RSArrayStorePerSize(array));
    __RSArraySetCapacity(array, capacity);
    return table;
}

static void __RSArrayClassInit(RSTypeRef obj)
{
    //RSArrayRef __array = (RSArrayRef)obj;
    
    return;
}

static RSTypeRef __RSArrayClassCopy(RSAllocatorRef allocator, RSTypeRef rs, BOOL mutableCopy)
{
    RSMutableArrayRef copy = RSArrayCreateMutableCopy(allocator, rs);
    if (mutableCopy == NO) unMarkMutable(copy);
    return copy;
}

static void __RSArrayClassDeallocate(RSTypeRef obj)
{
    RSArrayRef array = (RSArrayRef)obj;
    __RSArrayLockInstance(array);
    //RSArrayDebugLogSelf(array);
    
    RSTypeRef* table = __RSArrayStore(array);
    RSIndex cnt = __RSArrayCount(array);
    for (RSIndex idx = 0; idx < cnt; idx++)
    {
        RSTypeRef obj = table[idx];
        RSRelease(obj);
    }
    
    
    __RSArrayUnLockInstance(array);
    if (isStrong(array)) {
        RSAllocatorDeallocate(RSAllocatorSystemDefault, array->_store);
        ((RSMutableArrayRef)array)->_store = nil;
    }
    //RSAllocatorDeallocate(RSAllocatorSystemDefault, array);
}

static RSStringRef __RSArrayClassDescription(RSTypeRef obj)
{
    RSArrayRef array = (RSArrayRef)obj;
    __RSArrayAvailable(array);
    if (isNoCallBack(array))
        return RSStringCreateWithCString(RSAllocatorSystemDefault, "RSArray Not Contains RSTypeRef instance.\n", RSStringEncodingUTF8);
    //__RSArrayLockInstance(array);
    //RSArrayDebugLogSelf(obj);
    extern RSCBuffer __RSStringGetCStringWithNoCopy(RSStringRef aString);
    RSIndex cnt = __RSArrayCount(array);
    RSMutableStringRef string = RSStringCreateMutable(RSAllocatorSystemDefault, 32*cnt);
    RSStringAppendCString(string, "{\n");
    RSTypeRef* table = __RSArrayStore(array);
    for (RSIndex idx = 1; idx < cnt; idx++)
    {
        RSTypeRef item = table[idx-1];
        RSStringRef itemDescription = RSDescription(item);
        RSStringAppendCString(string, "\t\"");
        RSStringAppendCString(string, __RSStringGetCStringWithNoCopy(itemDescription));
        RSStringAppendCString(string, "\",\n");
        RSRelease(itemDescription);
    }
    if (cnt) {
        RSTypeRef item = table[cnt-1];
        RSStringRef itemDescription = RSDescription(item);
        RSStringAppendStringWithFormat(string, RSSTR("\t\"%R\"\n"), itemDescription);
        RSRelease(itemDescription); // bug!
    }
    //RSRelease(format);
    //RSArrayDebugLogSelf(obj);
    RSStringAppendCString(string, "}");
    //    RSStringRef format = RSSTR("%R");
    //    RSLog(format,string);
    //    RSRelease(format);
    //__RSArrayUnLockInstance(array);
    return string;
}
static RSRuntimeClass __RSArrayClass =
{
    _RSRuntimeScannedObject,
    0,
    "RSArray",
    __RSArrayClassInit,
    __RSArrayClassCopy,
    __RSArrayClassDeallocate,
    nil,
    nil,
    __RSArrayClassDescription,
    nil,
    nil,// the runtime will do it for RSArray
};
static RSTypeID _RSArrayTypeID = _RSRuntimeNotATypeID;

#define __RSArraySize     sizeof(struct __RSArray)

RSExport RSTypeID RSArrayGetTypeID()
{
    return _RSArrayTypeID;
}
/*
 typedef void (*RSRuntimeClassInit)(RSTypeRef cf);
 typedef RSTypeRef (*RSRuntimeClassCopy)(RSAllocatorRef allocator, RSTypeRef cf);
 typedef void (*RSRuntimeClassDeallocate)(RSTypeRef cf);
 typedef BOOL (*RSRuntimeClassEqual)(RSTypeRef cf1, RSTypeRef cf2);
 typedef RSHashCode (*RSRuntimeClassHash)(RSTypeRef cf);
 typedef void (*RSRuntimeClassReclaim)(RSTypeRef cf);
 typedef RSUInteger (*RSRuntimeClassRefcount)(intptr_t op, RSTypeRef cf);
 */

RSPrivate void __RSArrayInitialize()
{
    _RSArrayTypeID = __RSRuntimeRegisterClass(&__RSArrayClass);
    __RSRuntimeSetClassTypeID(&__RSArrayClass, _RSArrayTypeID);
}

static void __RSArrayShrinkStore(RSMutableArrayRef array, RSRange range)
{
    __RSArrayAvailableRange(array, range);
    //    if (range.location + range.length > __RSArrayCount(array)) HALTWithError(RSGenericException, "the range is out of the array.");
    RSTypeRef* table = __RSArrayStore(array);
    RSIndex i = 0;
    RSIndex cnt = __RSArrayCount(array);
    for (RSIndex idx = 0 ; idx < range.length ; idx++)
    {
        if (table[range.location + idx] == nil)
        {
            if (cnt - i > (range.location + range.length))
            {
                table[range.location + idx] = table[cnt - i - 1];
                table[cnt - i - 1] = nil;
            }
            i++;
        }
    }
    __RSArraySetCount(array, (__RSArrayCount(array)-i));
}

static void __RSArrayAddObjectUnit(RSMutableArrayRef array, RSTypeRef obj , RSIndex idx)
{
    RSTypeRef* table = __RSArrayStore(array);
    RSIndex cnt = __RSArrayCount(array);
    table[idx] = RSRetain(obj);
    __RSArraySetCount(array, ++cnt);
}

static void __RSArrayResizeStore(RSMutableArrayRef array, RSIndex newCount)
{
    __RSCLog(RSLogLevelDebug, "%s want new count is : %lld\n","__RSArrayResizeStore", newCount);
    RSTypeRef* table = __RSArrayStore(array);
    RSIndex capacity = __RSArrayCapacity(array);
    RSIndex limit = capacity / __RSArrayStorePerSize(array);
    RSIndex newlimit = newCount ?: 1;
    newCount = RSAllocatorSize(RSAllocatorSystemDefault, newlimit*__RSArrayStorePerSize(array));
    //newlimit = newCount / __RSArrayStorePerSize(array);
    // if the table is nil, means the creator maybe has a zero count.
    // so just create a new capacity for the array and return.
    if (table == nil)
    {
        // assume the new limit is 1, and calculate the new capacity of the new table.
        newlimit = 1;
        capacity = RSAllocatorSize(RSAllocatorSystemDefault, newlimit * __RSArrayStorePerSize(array));
        table = RSAllocatorAllocate(RSAllocatorSystemDefault, capacity);
        // set the member of the array.
        __RSArraySetStrongStore(array, table);
        __RSArraySetCount(array, 0);
        __RSArraySetCapacity(array, capacity);
        __RSCLog(RSLogLevelDebug, "%s real new count is : %lld\n","__RSArrayResizeStore", capacity / __RSArrayStorePerSize(array));
        return;
    }
    
    // the array is too small to store the new object,
    // the limit number of objects in the array is less than the new required count,
    // so should add a new memory block to make the old store bigger.
    // and because the array has a limit, so the table should not be nil.
    if (limit < newCount && table)
    {
        // we get a new capacity of the new store.
        capacity = newCount * __RSArrayStorePerSize(array);
        capacity = RSAllocatorSize(RSAllocatorSystemDefault, capacity);
        
        table = RSAllocatorReallocate(RSAllocatorSystemDefault, table, capacity);
        __RSArraySetStrongStore(array, table);
        __RSArraySetCapacity(array, capacity);
        __RSCLog(RSLogLevelDebug, "%s real new count is : %lld\n","__RSArrayResizeStore", capacity / __RSArrayStorePerSize(array));
    }
}

static void __RSArrayReplaceValues(RSMutableArrayRef array, RSRange range, RSTypeRef* values, RSIndex count)
{
    if (array == nil) HALTWithError(RSInvalidArgumentException, "the array is nil");
    RSIndex capacity = __RSArrayCapacity(array);
    RSIndex limit = capacity / __RSArrayStorePerSize(array);
    RSIndex cnt = __RSArrayCount(array);
    BOOL sortChange = YES;
    if (range.location == cnt - 1 && range.length == 1 && count == 1 && values)
    {
        if (isSorted(array))
        {
            RSComparisonResult result = __RSArrayComparator(array)(array->_store[array->_cnt - 1], values[0]);
            if (isAscending(array) && result == RSCompareLessThan)
            {
                sortChange = NO;
            }
            else if (!isAscending(array) && result == RSCompareGreaterThan)
            {
                sortChange = NO;
            }
            else if (result == RSCompareEqualTo)
            {
                sortChange = NO;
            }
        }
    }
    RSIndex idx = 0;
    if (limit < (range.location + count))
        __RSArrayResizeStore(array, range.location + range.length + count);
    RSTypeRef value = nil;
    RSTypeRef* table = __RSArrayStore(array);
    RSIndex nilCount = 0;
    
    if (range.length == count)
    {
        /*
         * start is the range to replace
         * dot is the new values
         array
         ------------**********-------
         ------------..........-------
         */
        for (idx = 0; idx < count; idx++)
        {
            RSRelease(table[range.location + idx]);
            value = values ? values[idx] : nil ;
            table[range.location + idx] = (value) ? (RSRetain(value)) : (nilCount++, nil);
        }
    }
    else if (range.length < count)
    {
        /*
         * start is the range to replace
         * dot is the new values
         array
         ------------**********-------
         ------------..........XX-----
         */
        for (idx = 0; idx < (count - range.length); idx++)
        {
            table[cnt + count - range.length - idx] = table[cnt - idx];
        }
        for (idx = 0; idx < count; idx++)
        {
            RSRelease(table[range.location + idx]);
            value = values ? values[idx] : nil ;
            table[range.location + idx] = (value) ? (RSRetain(value)) : (nilCount++, nil);
        }
    }
    else if (range.length > count)
    {
        /*
         * start is the range to replace
         * dot is the new values
         array
         ------------**********-------
         ------------........---------
         */
        for (idx = 0; idx < count; idx++)
        {
            //RSRelease(table[range.location + idx]);
            RSRelease(table[range.location + idx]);
            value = values ? values[idx] : nil ;
            table[range.location + idx] = (value) ? (RSRetain(value)) : (nilCount++, nil);
        }
        for (RSIndex idx = count; idx < range.length; idx++)
        {
            RSRelease(table[range.location + idx]);
            table[range.location + idx] = nil;
            nilCount++;
        }
        //__RSArraySetCount(array, cnt + count - range.length);
        
    }
    if (nilCount && (cnt >= nilCount))
    {
        cnt -= nilCount;
    }
    __RSArrayShrinkStore(array, range);
    __RSArraySetCount(array, cnt);
    if (sortChange) unMarkSorted(array);
    return;
}

static RSTypeRef __RSArrayObjectAtIndex(RSArrayRef array, RSIndex idx)
{
    RSTypeRef* table = __RSArrayStore(array);
    RSIndex cnt = __RSArrayCount(array);
    if (cnt <= idx || table == nil) HALTWithError(RSInvalidArgumentException, "the RSArray has not the object at this idx");
    
    RSTypeRef ret = table[idx];
    return ret ;//? RSRetain(ret) : ret;
}

RSPrivate RSTypeRef __RSArrayGetTable(RSArrayRef array)
{
    return __RSArrayStore(array);
}

static void __RSArrayAddObject(RSMutableArrayRef array, RSTypeRef obj)
{
    //__RSArrayLockInstance(array);
    if (array == nil) HALTWithError(RSInvalidArgumentException, "the array is nil");
    RSIndex cnt = __RSArrayCount(array);
    RSIndex capacity = __RSArrayCapacity(array);
    RSIndex limit = capacity / __RSArrayStorePerSize(array);
    BOOL sortChange = YES;
    if (isSorted(array))
    {
        RSComparisonResult result = __RSArrayComparator(array)(array->_store[array->_cnt - 1], obj);
        if (isAscending(array) && result == RSCompareLessThan)
        {
            sortChange = NO;
        }
        else if (!isAscending(array) && result == RSCompareGreaterThan)
        {
            sortChange = NO;
        }
        else if (result == RSCompareEqualTo)
        {
            sortChange = NO;
        }
    }
    if (limit && limit > cnt)
        __RSArrayAddObjectUnit(array, (obj), cnt);
    else
    {
        __RSArrayResizeStore(array, (limit + 1));
        __RSArrayAddObjectUnit(array, (obj), cnt);
    }
    if (sortChange) unMarkSorted(array);
    //__RSArrayUnLockInstance(array);
}

static void __RSArrayRemoveObject(RSMutableArrayRef array, RSTypeRef obj)
{
    //__RSArrayLockInstance(array);
    {
        for (RSIndex idx = 0; idx < __RSArrayCount(array); idx++)
        {
            RSTypeRef item = __RSArrayObjectAtIndex(array, idx);
            
            if (RSEqual(item, obj))
            {
                __RSArrayReplaceValues(array, RSMakeRange(idx, 1), nil, 1);
                idx--;
            }
            //RSRelease(item);
        }
    }
    
    
    //__RSArrayUnLockInstance(array);
}



static RSMutableArrayRef __RSArrayCreateInstance(RSAllocatorRef allocator, RSTypeRef* store, RSIndex count, RSIndex exCount, RSIndex perSize, BOOL _strong, BOOL _mutable, BOOL _needCopy)
{
    if (perSize != sizeof(RSTypeRef)) HALTWithError(RSInvalidArgumentException, "the object size must be equal to the id!");
    
    RSMutableArrayRef mutableArray = (RSMutableArrayRef)__RSRuntimeCreateInstance(allocator, RSArrayGetTypeID(), sizeof(struct __RSArray) - sizeof(struct __RSRuntimeBase));
    __RSArraySetStorePerSize(mutableArray, perSize = sizeof(RSTypeRef));
    
    if (_strong)
    {
        if (_needCopy)
        {
            if (store)
            {
                RSTypeRef* __ptr = nil;
                if (_mutable) // mutable array
                {
                    if (exCount < count *2) exCount *= 2;
                    if (nil == (__ptr = __RSArrayCreateTable(mutableArray, exCount)))
                    {
                        RSAllocatorDeallocate(allocator, mutableArray);
                        return nil;
                    }
                    //__RSArraySetCapacity(mutableArray, RSAllocatorSize(allocator, exCount));
                }
                else    // constant array
                {
                    
                    if (nil == (__ptr = __RSArrayCreateTable(mutableArray, count)))
                    {
                        RSAllocatorDeallocate(allocator, mutableArray);
                        return nil;
                    }
                    //__RSArraySetCapacity(mutableArray, RSAllocatorSize(allocator, count));
                    
                }
                
                //__RSArraySetCount(mutableArray, count);
                __RSArraySetStrongStore(mutableArray, __ptr);
                // deep copy
                for (RSIndex idx = 0; idx < count; idx ++)
                {
                    __RSArrayAddObjectUnit(mutableArray, store[idx], idx);
                }
                
                
            }// store
            else HALTWithError(RSInvalidArgumentException, "the store is nil but want to deep copy!");
        }// _needCopy
        else
        {
            if (store)
            {
                
                __RSArraySetStrongStore(mutableArray, store);
            }
        }
        
        // check the table is allocated or not in strong.
        if (nil == __RSArrayStore(mutableArray)) {
            count = exCount;
            __RSArrayCreateTable(mutableArray, count);
            __RSArraySetCount(mutableArray, 0);
            // the __RSArrayCreateTable do it for caller.
            //__RSArraySetCapacity(mutableArray, RSAllocatorSize(allocator, exCount));
        }
    }// _strong
    if (_mutable) {
        markMutable(mutableArray);
    }
    return mutableArray;
}

RSExport RSIndex    RSArrayGetCount(RSArrayRef array)
{
    __RSArrayAvailable(array);
    __RSArrayLockInstance(array);
    RSIndex cnt = __RSArrayCount(array);
    __RSArrayUnLockInstance(array);
    return cnt;
}

RSExport RSTypeRef   RSArrayObjectAtIndex(RSArrayRef array, RSIndex idx)
{
    __RSArrayAvailable(array);
    if (idx < 0) return nil;
    __RSArrayLockInstance(array);
    RSTypeRef ret = __RSArrayObjectAtIndex(array, idx); // return the object with retain.
    __RSArrayUnLockInstance(array);
    return ret;//RSRetain(ret);
}

RSExport RSIndex    RSArrayIndexOfObject(RSArrayRef array, RSTypeRef obj)
{
    __RSArrayAvailable(array);
    if (obj == nil) return -1;
    __RSArrayLockInstance(array);
    RSTypeRef* table = __RSArrayStore(array);
    RSIndex count = __RSArrayCount(array);
    __RSArrayUnLockInstance(array);
    if (isSorted(array))
    {
        //middle=left + ((right-left)>>1)
        RSIndex left = 0;
        RSIndex right = count - 1;
        RSIndex middle = 0;//(RSIndex)left + ((right-left)>>1);
        
        RSComparatorFunction comparator = __RSArrayComparator(array);
        if (comparator == nil) HALTWithError(RSInvalidArgumentException, "the comparator is nil");
        if (isAscending(array)) // the array is ascending.
        {
            __RSArrayLockInstance(array);
            while (1)
            {
                middle = (RSIndex)left + ((right-left)>>1);
                RSComparisonResult result = comparator(table[middle], obj);
                if (RSCompareGreaterThan == result)
                {
                    //middle = (middle + 0) / 2 ;
                    right = middle-1;
                }
                else if (RSCompareLessThan == result)
                {
                    //middle = (middle + count) / 2 ;
                    left = middle+1;
                }
                else if (RSCompareEqualTo == result)
                {
                    __RSArrayUnLockInstance(array);
                    return middle;
                }
            }
            __RSArrayUnLockInstance(array);
        }
        else
        {
            __RSArrayLockInstance(array);
            while (1)
            {
                middle = (RSIndex)left + ((right-left)>>1);
                RSComparisonResult result = comparator(table[middle], obj);
                if (RSCompareGreaterThan == result)
                {
                    left = middle+1;//middle = (middle + count) / 2 ;
                }
                else if (RSCompareLessThan == result)
                {
                    right = middle-1;//middle = (middle + 0) / 2 ;
                }
                else if (RSCompareEqualTo == result)
                {
                    __RSArrayUnLockInstance(array);
                    return middle;
                }
            }
            __RSArrayUnLockInstance(array);
        }
    }
    else
    {
        __RSArrayLockInstance(array);
        {
            for (RSIndex idx = 0; idx < count; idx++)
            {
                if (RSEqual(table[idx], obj))
                {
                    __RSArrayUnLockInstance(array);
                    return idx;
                }
            }
        }
        
        __RSArrayUnLockInstance(array);
    }
    return -1;
}

RSExport void       RSArraySort(RSArrayRef array, RSComparatorFunction comparator, BOOL ascending)
{
    __RSArrayAvailable(array);
    
    __RSArrayLockInstance(array);
    if (comparator == nil && __RSArrayComparator(array) == nil)
    {
        __RSArrayUnLockInstance(array);
        return;
    }
    RSTypeRef* table = __RSArrayStore(array);
    __RSArraySetComparator(array, comparator);
    RSQSortArray((void**)table, __RSArrayCount(array), sizeof(RSTypeRef), comparator, ascending);
    markSorted(array);
    (ascending) ? markAscending(array) : markDescending(array);
    __RSArrayUnLockInstance(array);
    return;
}

RSExport RSTypeRef  RSArrayLastObject(RSArrayRef array)
{
    if (0 < RSArrayGetCount(array))
        return RSArrayObjectAtIndex(array, RSArrayGetCount(array) - 1);
    else return nil;
}
RSExport void       RSArrayAddObject(RSMutableArrayRef array, RSTypeRef obj)
{
    __RSArrayAvailable(array);
    __RSArrayLockInstance(array);
    if (isMutable(array) == NO) HALTWithError(RSInvalidArgumentException, "the array must be a RSMutableArray");
    __RSArrayAddObject(array, obj);
    __RSArrayUnLockInstance(array);
}

RSExport void       RSArrayRemoveObject(RSMutableArrayRef array, RSTypeRef obj)
{
    __RSArrayAvailable(array);
    __RSArrayLockInstance(array);
    if (isMutable(array) == NO) HALTWithError(RSInvalidArgumentException, "the array must be a RSMutableArray");
    __RSArrayRemoveObject(array, obj);
    __RSArrayUnLockInstance(array);
}

RSExport void       RSArrayRemoveLastObject(RSMutableArrayRef array)
{
    __RSArrayAvailable(array);
    __RSArrayLockInstance(array);
    if (__RSArrayCount(array))
        __RSArrayReplaceValues(array, RSMakeRange(__RSArrayCount(array) - 1, 1), nil, 1);
    __RSArrayUnLockInstance(array);
}

RSExport void       RSArrayRemoveAllObjects(RSMutableArrayRef array)
{
    __RSArrayAvailable(array);
    __RSArrayLockInstance(array);
    if (__RSArrayCount(array)) {
        __RSArrayReplaceValues(array, RSMakeRange(0, __RSArrayCount(array)), nil, __RSArrayCount(array));
    }
    __RSArrayUnLockInstance(array);
}

RSExport void       RSArrayRemoveObjectAtIndex(RSMutableArrayRef array, RSIndex idx)
{
    __RSArrayAvailable(array);
    __RSArrayLockInstance(array);
    if (isMutable(array) == NO) HALTWithError(RSInvalidArgumentException, "the array must be a RSMutableArray");
    if (idx >= __RSArrayCount(array)) HALTWithError(RSInvalidArgumentException, "the index of object is unavailable");
    __RSArrayReplaceValues(array, RSMakeRange(idx, 1), nil, 1);
    __RSArrayUnLockInstance(array);
}

RSExport RSArrayRef RSArrayCreateWithObject(RSAllocatorRef allocator, RSTypeRef obj)
{
    if (obj == nil) return RSArrayCreate(allocator, nil);
    RSArrayRef array = __RSArrayCreateInstance(allocator, (&obj), 1, 1, sizeof(ISA), YES, NO, YES);
    return array;
}

RSExport RSArrayRef RSArrayCreateWithObjects(RSAllocatorRef allocator, RSTypeRef* objects, RSIndex count)
{
    if ((objects == nil) && count) HALTWithError(RSInvalidArgumentException, "the member is illegal");
    RSArrayRef array = __RSArrayCreateInstance(allocator, objects, count, count, sizeof(ISA), YES, NO, YES);
    return array;
}

RSExport RSArrayRef RSArrayCreateWithObjectsNoCopy(RSAllocatorRef allocator, RSTypeRef* objects, RSIndex count, BOOL freeWhenDone)
{
    if ((objects == nil) && count) HALTWithError(RSInvalidArgumentException, "the member is illegal");
    RSArrayRef array = __RSArrayCreateInstance(allocator, objects, count, count, sizeof(ISA), YES, NO, NO);
    return array;
}

RSExport RSArrayRef RSArrayCreate(RSAllocatorRef allocator, RSTypeRef object, ...)
{
    RSTypeRef value = object;
    RSMutableArrayRef array = RSArrayCreateMutable(allocator, 0);
    va_list va;
    va_start(va, object);
    value = object;
    for (;;)
    {
        if (value == nil)
        {
            va_end(va);
            return array;
        }
        RSArrayAddObject(array, value);
        value = va_arg(va, RSTypeRef);
    }
    va_end(va);
    return array;
}

RSExport RSMutableArrayRef RSArrayCreateMutable(RSAllocatorRef allocator, RSIndex count)
{
    RSMutableArrayRef array = __RSArrayCreateInstance(allocator, nil, 0, count ?: 1, sizeof(ISA), YES, YES, NO);
    return array;
}

RSExport RSMutableArrayRef RSArrayCreateMutableCopy(RSAllocatorRef allocator, RSArrayRef array)
{
    RSIndex cnt = RSArrayGetCount(array);
    RSMutableArrayRef copyArray = RSArrayCreateMutable(allocator, cnt);
    for (RSIndex idx = 0; idx < cnt; idx++)
    {
        RSTypeRef obj = RSArrayObjectAtIndex(array, idx);   // return with no retain.
        RSTypeRef copy = RSCopy(allocator, obj);
        RSArrayAddObject(copyArray, copy);
        RSRelease(copy);
    }
    return copyArray;
}

RSExport void       RSArrayGetObjects(RSArrayRef array, RSRange range, RSTypeRef* obj)
{
    if (obj == nil) return;
    __RSArrayAvailable(array);
    __RSArrayAvailableRange(array, range);
    
    for (RSIndex idx = range.location; idx < range.length; idx++)
    {
        obj[idx] = array->_store[idx];
    }
}

RSExport void       RSArrayApplyFunction(RSArrayRef array, RSRange range, RSArrayApplierFunction function, void* context)
{
    if (function == nil) return;
    __RSArrayAvailable(array);
    __RSArrayAvailableRange(array, range);
    
    for (RSIndex idx = range.location; idx < range.length; idx++)
    {
        function(array->_store[idx], context);
    }
}

RSPrivate void RSArrayDebugLogSelf(RSArrayRef array)
{
    __RSArrayAvailable(array);
    RSIndex idx = 0, cnt = __RSArrayCount(array);
    RSTypeRef* table = __RSArrayStore(array);
    __RSCLog(RSLogLevelDebug, "array = *%p*\n",array);
    for (idx = 0; idx < cnt; idx++) {
        __RSCLog(RSLogLevelDebug, "[%p]\n",table[idx]);
    }
}
#elif RSARRAY_VERSION == 2
typedef const void *	(*RSArrayRetainCallBack)(RSAllocatorRef allocator, const void *value);
typedef void		(*RSArrayReleaseCallBack)(RSAllocatorRef allocator, const void *value);
typedef RSStringRef	(*RSArrayCopyDescriptionCallBack)(const void *value);
typedef BOOL		(*RSArrayEqualCallBack)(const void *value1, const void *value2);
typedef struct {
    RSIndex				version;
    RSArrayRetainCallBack		retain;
    RSArrayReleaseCallBack		release;
    RSArrayCopyDescriptionCallBack	copyDescription;
    RSArrayEqualCallBack		equal;
} RSArrayCallBacks;

static RSArrayCallBacks __RSArrayRSTypeCallBacks = {0, __RSTypeCollectionRetain, __RSTypeCollectionRelease, RSDescription, RSEqual};
static RSArrayCallBacks __RSArrayNullCallBacks = {0, nil, nil, nil, nil};
struct __RSArrayBucket
{
    union
    {
        uintptr_t _value;
        RSTypeRef _constantObject;
        ISA _obj;
    };
};

enum {
    __RS_MAX_BUCKETS_PER_DEQUE = LONG_MAX
};

RSInline RSIndex __RSArrayDequeRoundUpCapacity(RSIndex capacity)
{
    if (capacity < 4) return 4;
    return min((1 << flsl(capacity)), __RS_MAX_BUCKETS_PER_DEQUE);
}

struct __RSArrayDeque
{
    uintptr_t _idx;
    uintptr_t _capacity;
};

struct __RSArray
{
    RSRuntimeBase _base;
    RSIndex _count;
    void * _store;
};

enum
{
    __RSArrayImmutable = 0,
    __RSArrayDeque = 2,
};

enum
{
    __RSArrayRSCallback = 0,
    __RSArrayNoCallback = 1,
    __RSArrayNullCallback = 1,
    __RSArrayCustomCallback = 2,
};

RSInline RSIndex __RSArrayGetType(RSArrayRef array)
{
    return __RSBitfieldGetObject(RSRuntimeClassBaseFiled(array), 1, 0);
}

RSInline void __RSArraySetType(RSArrayRef array, RSIndex type)
{
    __RSBitfieldSetObject(RSRuntimeClassBaseFiled(array), 1, 0, type);
}

RSInline RSIndex __RSArrayGetCallBackType(RSArrayRef array)
{
    return __RSBitfieldGetObject(RSRuntimeClassBaseFiled(array), 3, 2);
}

RSInline RSIndex __RSArrayGetSizeOfType(RSIndex type)
{
    RSIndex size = sizeof(struct __RSArray);
    if (type == __RSArrayCustomCallback) size += sizeof(RSArrayCallBacks);
    return size;
}

RSInline RSIndex __RSArrayGetCount(RSArrayRef array)
{
    return array->_count;
}

RSInline void __RSArraySetCount(RSArrayRef array, RSIndex count)
{
    ((RSMutableArrayRef)array)->_count = count;
}

RSInline struct __RSArrayBucket* __RSArrayFindBucket(RSArrayRef array)
{
    switch (__RSArrayGetType(array))
    {
        case __RSArrayImmutable:
            return (struct __RSArrayBucket*)((RSBitU8*)(array + __RSArrayGetSizeOfType(__RSArrayGetCallBackType(array))));
        case __RSArrayDeque:
        {
            struct __RSArrayDeque* deque = array->_store;
            return (struct __RSArrayBucket*)((RSBitU8*)(deque + sizeof(deque) + deque->_idx * sizeof(struct __RSArrayBucket)));
        }
    }
    return nil;
}

RSInline struct __RSArrayBucket* __RSArrayGetBucketAtIndex(RSArrayRef array, RSIndex idx)
{
    switch (__RSArrayGetType(array))
    {
        case __RSArrayImmutable:
        case __RSArrayDeque:
            return __RSArrayFindBucket(array) + idx;
    }
    return nil;
}

static const RSArrayCallBacks* __RSArrayGetCallBacks(RSArrayRef array)
{
    switch (__RSArrayGetCallBackType(array))
    {
        case __RSArrayRSCallback:
            return RSArrayRSTypeCallBacks;
        case __RSArrayNoCallback:
            return RSArrayNullCallBacks;
        case __RSArrayCustomCallback:
        {
            switch (__RSArrayGetType(array))
            {
                case __RSArrayImmutable:
                case __RSArrayDeque:
                    return (const RSArrayCallBacks*)(RSBitU8*)array + sizeof(struct __RSArray);
            }
        }
            
    }
    return nil;
}

RSInline BOOL __RSArrayCheckCallBackIsRSType(const RSArrayCallBacks* cb)
{
    return ((cb == RSArrayRSTypeCallBacks) ||
            (cb->retain == RSArrayRSTypeCallBacks->retain &&
             cb->release == RSArrayRSTypeCallBacks->release &&
             cb->copyDescription == RSArrayRSTypeCallBacks->copyDescription &&
             cb->equal == RSArrayRSTypeCallBacks->equal &&
             cb->version == RSArrayRSTypeCallBacks->version));
}

RSInline BOOL __RSArrayCheckCallBackIsNull(const RSArrayCallBacks* cb)
{
    return ((cb == RSArrayNullCallBacks) ||
            (cb->retain == RSArrayNullCallBacks->retain &&
             cb->release == RSArrayNullCallBacks->release &&
             cb->copyDescription == RSArrayNullCallBacks->copyDescription &&
             cb->equal == RSArrayNullCallBacks->equal &&
             cb->version == RSArrayNullCallBacks->version));
}

static void __RSArrayReleaseObjects(RSArrayRef array, RSRange range, BOOL freeBucketIfPossible)
{
    RSIndex count = __RSArrayGetCount(array);
    if (count < range.location) return;
    if (count < range.location + range.length) range.length = count - range.location ;
    const RSArrayCallBacks* cb = __RSArrayGetCallBacks(array);
    if (range.length <= 0) return;
    switch (__RSArrayGetType(array))
    {
        case __RSArrayImmutable:
        {
            
            struct __RSArrayBucket* buckets = __RSArrayFindBucket(array);
            if (cb && cb->release)
            {
                RSAllocatorRef allocator = RSGetAllocator(array);
                for (RSIndex idx = 0; idx < range.length; idx++)
                {
                    cb->release(allocator, buckets[idx + range.location]._obj);
                }
            }
            else
            {
                for (RSIndex idx = 0; idx < range.length; idx++)
                {
                    buckets[idx + range.location]._obj = nil;
                }
            }
        }
            break;
        case __RSArrayDeque:
        {
            struct __RSArrayDeque* deque = (struct __RSArrayDeque*)(RSMutableArrayRef)array->_store;
            RSAllocatorRef allocator = RSGetAllocator(array);
            if (deque && range.length > 0  && range.length < count)
            {
                struct __RSArrayBucket *buckets = __RSArrayFindBucket(array);
                if (cb && cb->release)
                {
                    for (RSIndex idx = 0; idx < range.length; idx++)
                    {
                        cb->release(allocator, buckets[idx + range.location]._obj);
                        buckets[idx + range.location]._obj = nil;
                    }
                }
                else
                {
                    for (RSIndex idx = 0; idx < range.length; idx++)
                    {
                        buckets[idx + range.location]._obj = nil;
                    }
                }
            }
            
            
        }
            break;
    } // switch
    if (freeBucketIfPossible && 0 == range.location && count == range.length)
    {
        RSAllocatorRef allocator = RSGetAllocator(array);
        RSAllocatorDeallocate(allocator, array->_store);
        __RSArraySetCount(array, 0);
        ((RSMutableArrayRef)array)->_store = nil;
    }
    return;
}

static void __RSArrayRepositionDequeRegions(RSMutableArrayRef array, RSRange range, RSIndex newCount)
{
    // newCount elements are going to replace the range, and the result will fit in the deque
    struct __RSArrayDeque *deque = (struct __RSArrayDeque *)array->_store;
    struct __RSArrayBucket *buckets;
    RSIndex cnt, futureCnt, numNewElems;
    RSIndex L, A, B, C, R;
    
    buckets = (struct __RSArrayBucket *)((RSBitU8 *)deque + sizeof(struct __RSArrayDeque));
    cnt = __RSArrayGetCount(array);
    futureCnt = cnt - range.length + newCount;
    
    L = deque->_idx;		// length of region to left of deque
    A = range.location;			// length of region in deque to left of replaced range
    B = range.length;			// length of replaced range
    C = cnt - B - A;			// length of region in deque to right of replaced range
    R = deque->_capacity - cnt - L;	// length of region to right of deque
    numNewElems = newCount - B;
    
    RSIndex wiggle = deque->_capacity >> 17;
    if (wiggle < 4) wiggle = 4;
    if (deque->_capacity < (RSBit32)futureCnt || (cnt < futureCnt && L + R < wiggle))
    {
        // must be inserting or space is tight, reallocate and re-center everything
        RSAllocatorRef allocator = RSGetAllocator(array);
        RSIndex capacity = RSAllocatorSize(allocator, futureCnt + wiggle);
        RSIndex size = sizeof(struct __RSArrayDeque) + capacity * sizeof(struct __RSArrayBucket);
        BOOL collectableMemory = RS_IS_COLLECTABLE_ALLOCATOR(allocator);
        struct __RSArrayDeque *newDeque = (struct __RSArrayDeque *)RSAllocatorAllocate(allocator, size);
        struct __RSArrayBucket *newBuckets = (struct __RSArrayBucket *)((RSBitU8 *)newDeque + sizeof(struct __RSArrayDeque));
        RSIndex oldL = L;
        RSIndex newL = (capacity - futureCnt) / 2;
        RSIndex oldC0 = oldL + A + B;
        RSIndex newC0 = newL + A + newCount;
        newDeque->_idx = newL;
        newDeque->_capacity = capacity;
        if (0 < A) memmove(newBuckets + newL, buckets + oldL, A * sizeof(struct __RSArrayBucket));
        if (0 < C) memmove(newBuckets + newC0, buckets + oldC0, C * sizeof(struct __RSArrayBucket));
        __RSAssignWithWriteBarrier((void **)&array->_store, (void *)newDeque);
        if (!collectableMemory && deque) RSAllocatorDeallocate(allocator, deque);
        //printf("3:  array %p store is now %p (%lx)\n", array, array->_store, *(unsigned long *)(array->_store));
        return;
    }
    
    if ((numNewElems < 0 && C < A) || (numNewElems <= R && C < A))
    {
        // move C
        // deleting: C is smaller
        // inserting: C is smaller and R has room
        RSIndex oldC0 = L + A + B;
        RSIndex newC0 = L + A + newCount;
        if (0 < C) memmove(buckets + newC0, buckets + oldC0, C * sizeof(struct __RSArrayBucket));
        // GrP GC: zero-out newly exposed space on the right, if any
        if (oldC0 > newC0) memset(buckets + newC0 + C, 0, (oldC0 - newC0) * sizeof(struct __RSArrayBucket));
    }
    else if ((numNewElems < 0) || (numNewElems <= L && A <= C))
    {
        // move A
        // deleting: A is smaller or equal (covers remaining delete cases)
        // inserting: A is smaller and L has room
        RSIndex oldL = L;
        RSIndex newL = L - numNewElems;
        deque->_idx = newL;
        if (0 < A) memmove(buckets + newL, buckets + oldL, A * sizeof(struct __RSArrayBucket));
        // GrP GC: zero-out newly exposed space on the left, if any
        if (newL > oldL) memset(buckets + oldL, 0, (newL - oldL) * sizeof(struct __RSArrayBucket));
    }
    else
    {
        // now, must be inserting, and either:
        //    A<=C, but L doesn't have room (R might have, but don't care)
        //    C<A, but R doesn't have room (L might have, but don't care)
        // re-center everything
        RSIndex oldL = L;
        RSIndex newL = (L + R - numNewElems) / 2;
        newL = newL - newL / 2;
        RSIndex oldC0 = oldL + A + B;
        RSIndex newC0 = newL + A + newCount;
        deque->_idx = newL;
        if (newL < oldL)
        {
            if (0 < A) memmove(buckets + newL, buckets + oldL, A * sizeof(struct __RSArrayBucket));
            if (0 < C) memmove(buckets + newC0, buckets + oldC0, C * sizeof(struct __RSArrayBucket));
            // GrP GC: zero-out newly exposed space on the right, if any
            if (oldC0 > newC0) memset(buckets + newC0 + C, 0, (oldC0 - newC0) * sizeof(struct __RSArrayBucket));
        }
        else
        {
            if (0 < C) memmove(buckets + newC0, buckets + oldC0, C * sizeof(struct __RSArrayBucket));
            if (0 < A) memmove(buckets + newL, buckets + oldL, A * sizeof(struct __RSArrayBucket));
            // GrP GC: zero-out newly exposed space on the left, if any
            if (newL > oldL) memset(buckets + oldL, 0, (newL - oldL) * sizeof(struct __RSArrayBucket));
        }
    }
}


static void __RSArrayReplaceObjects(RSMutableArrayRef array, RSRange range, const void** newObjects, RSIndex newCount)
{
    const RSArrayCallBacks *cb;
    RSIndex idx, cnt, futureCnt;
    const void **newv, *buffer[256];
    cnt = __RSArrayGetCount(array);
    futureCnt = cnt - range.length + newCount;
    cb = __RSArrayGetCallBacks(array);
    RSAllocatorRef allocator = RSGetAllocator(array);
    
    if (nil != cb->retain)
    {
        newv = (newCount <= 256) ? (const void **)buffer : (const void **)RSAllocatorAllocate(RSAllocatorSystemDefault, newCount * sizeof(void *));
        for (idx = 0; idx < newCount; idx++)
        {
            newv[idx] = (void *)INVOKE_CALLBACK2(cb->retain, allocator, (void *)newObjects[idx]);
        }
    }
    else
    {
        newv = newObjects;
    }
    //array->_mutations++;
    if (0 < range.length)
    {
        __RSArrayReleaseObjects(array, range, NO);
    }
    // region B elements are now "dead"
    if (0)
    {
    }
    else if (nil == array->_store)
    {
        if (0)
        {
        }
        else if (0 <= futureCnt)
        {
            struct __RSArrayDeque *deque;
            RSIndex capacity = __RSArrayDequeRoundUpCapacity(futureCnt);
            RSIndex size = sizeof(struct __RSArrayDeque) + capacity * sizeof(struct __RSArrayBucket);
            deque = (struct __RSArrayDeque *)RSAllocatorAllocate(allocator, size);
            deque->_idx = (capacity - newCount) / 2;
            deque->_capacity = capacity;
            __RSAssignWithWriteBarrier((void **)&array->_store, (void *)deque);
        }
    }
    else
    {
        // Deque
        // reposition regions A and C for new region B elements in gap
        if (0)
        {
        }
        else if (range.length != newCount)
        {
            __RSArrayRepositionDequeRegions(array, range, newCount);
        }
    }
    // copy in new region B elements
    if (0 < newCount)
    {
        if (0)
        {
        }
        else
        {
            // Deque
            struct __RSArrayDeque *deque = (struct __RSArrayDeque *)array->_store;
            struct __RSArrayBucket *raw_buckets = (struct __RSArrayBucket *)((uint8_t *)deque + sizeof(struct __RSArrayDeque));
            memmove(raw_buckets + deque->_idx + range.location, newv, newCount * sizeof(struct __RSArrayBucket));
        }
    }
    __RSArraySetCount(array, futureCnt);
    if (newv != buffer && newv != newObjects) RSAllocatorDeallocate(RSAllocatorSystemDefault, newv);
    //    RSIndex count = __RSArrayGetCount(array);
    //    //if (count <= range.location) return;
    //    RSIndex futureCnt = count - range.length + countOfNewObjects, idx = 0;
    //    const RSArrayCallBacks* cb = __RSArrayGetCallBacks(array);
    //    const void ** newv = nil, *buf[256] = {nil};
    //    RSAllocatorRef allocator = RSGetAllocator(array);
    //    if (cb->retain && newObjects && countOfNewObjects)
    //    {
    //        newv = (countOfNewObjects <= 256) ? buf : RSAllocatorAllocate(RSAllocatorSystemDefault, countOfNewObjects * sizeof(RSTypeRef));
    //        while (idx < countOfNewObjects)
    //        {
    //            newv[idx++] = cb->retain(allocator, newObjects[idx]);
    //        }
    //    }
    //    else
    //    {
    //        newv = newObjects;
    //    }
    //
    //    if (range.length > 0) __RSArrayReleaseObjects(array, range, NO);
    //
    //    if (nil == array->_store)
    //    {
    //        if (futureCnt > 0)
    //        {
    //            struct __RSArrayDeque* deque = nil;
    //            RSIndex capacity = __RSArrayDequeRoundUpCapacity(futureCnt);
    //            RSIndex size = sizeof(struct __RSArrayDeque) + sizeof(struct __RSArrayBucket) * capacity;
    //            deque = RSAllocatorAllocate(allocator, size);
    //            deque->_idx = (capacity - countOfNewObjects) / 2;
    //            deque->_capacity = capacity;
    //            array->_store = (void *)deque;
    //        }
    //    }
    //    else
    //    {
    //        if (range.length != countOfNewObjects)
    //            __RSArrayRepositionDequeRegions(array, range, countOfNewObjects);
    //    }
    //
    //    if (0 < countOfNewObjects)
    //    {
    //        // Deque
    //        struct __RSArrayDeque *deque = (struct __RSArrayDeque *)array->_store;
    //        struct __RSArrayBucket *raw_buckets = (struct __RSArrayBucket *)((uint8_t *)deque + sizeof(struct __RSArrayDeque));
    //        memmove(raw_buckets + deque->_idx + range.location,
    //                newv,
    //                countOfNewObjects * sizeof(struct __RSArrayBucket));
    //    }
    //
    //    __RSArraySetCount(array, futureCnt);
    //    if (newv != buf && newv != newObjects)
    //        RSAllocatorDeallocate(RSAllocatorSystemDefault, newv);
    //    return;
}

static RSTypeRef __RSArrayClassCopy(RSAllocatorRef allocator, RSTypeRef rs, BOOL mutableCopy)
{
    if (mutableCopy) return RSArrayCreateMutableCopy(allocator, rs);
    return nil;
}

static void __RSArrayClassDeallocate(RSTypeRef rs)
{
    RSArrayRef array = (RSArrayRef)rs;
    switch (__RSArrayGetType(array))
    {
        case __RSArrayImmutable:
        case __RSArrayDeque:
            if (array->_store) __RSArrayReleaseObjects(array, RSMakeRange(0, __RSArrayGetCount(array)), YES);
            break;
        default:
            break;
    }
    return;
}

static RSHashCode __RSArrayClassHash(RSTypeRef rs)
{
    return __RSArrayGetCount(rs);
}

static RSStringRef __RSArrayClassDescription(RSTypeRef rs)
{
    RSArrayRef array = (RSArrayRef)rs;
    RSAllocatorRef allocator = nil;
    RSMutableStringRef description = RSStringCreateMutable(allocator = RSGetAllocator(array), 0);
    RSIndex cnt = __RSArrayGetCount(rs);
    switch (__RSArrayGetType(array))
    {
        case __RSArrayImmutable:
#ifdef __x86_64__
            RSStringAppendStringWithFormat(description, RSSTR("<RSArray %p [%p]>{type = immutable, count = %lld, values = (%s"), rs, allocator, cnt, cnt ? "\n" : "");
#else
            RSStringAppendStringWithFormat(description, RSSTR("<RSArray %p [%p]>{type = immutable, count = %ld, values = (%s"),  rs, allocator, cnt, cnt ? "\n" : "");
#endif
            break;
        case __RSArrayDeque:
#ifdef __x86_64__
            RSStringAppendStringWithFormat(description, RSSTR("<RSArray %p [%p]>{type = mutable, count = %lld, value = (%s)"), rs, allocator, cnt, cnt ? "\n" : "");
#else
            RSStringAppendStringWithFormat(description, RSSTR("<RSArray %p [%p]>{type = mutable, count = %ld, values = (%s"),  rs, allocator, cnt, cnt ? "\n" : "");
#endif
            break;
        default:
            break;
    }
    const RSArrayCallBacks* cb = __RSArrayGetCallBacks(array);
    if (cb && cb->copyDescription)
    {
        RSStringRef format = RSSTR("\t%R\n");
        for (RSIndex idx = 0; idx < cnt; idx++)
        {
            // part A is slower than part B because of RSSTR get a lock every time.
            
            // A
            //RSStringAppendStringWithFormat(description, RSSTR("\t%R\n"), cb->copyDescription(__RSArrayGetBucketAtIndex(array, idx)->_obj));
            
            // B
            //RSStringAppendCString(description, "\t");
            //RSStringAppendString(description, cb->copyDescription(__RSArrayGetBucketAtIndex(array, idx)->_obj)                                                                                                                                                                                              );
            //RSStringAppendCString(description, "\n");
            
            RSStringAppendStringWithFormat(description, format, cb->copyDescription(__RSArrayGetBucketAtIndex(array, idx)->_obj));
        }
    }
    else
    {
        for (RSIndex idx = 0; idx < cnt; idx++)
        {
            RSStringAppendStringWithFormat(description, RSSTR("\t%lld : <%p>\n"), idx, __RSArrayGetBucketAtIndex(array, idx)->_obj);
        }
    }
    RSStringAppendCString(description, ")};");
    return description;
}

static BOOL __RSArrayClassEqual(RSTypeRef rs1, RSTypeRef rs2)
{
    RSIndex cnt = __RSArrayGetCount(rs1);
    if (cnt != __RSArrayGetCount(rs2)) return NO;
    const RSArrayCallBacks* cb = __RSArrayGetCallBacks(rs1);
    if (cb->equal !=  __RSArrayGetCallBacks(rs2)->equal) return NO;
    if (cb->equal)
    {
        for (RSIndex idx = 0; idx < cnt; idx++)
        {
            if (NO == cb->equal(__RSArrayGetBucketAtIndex(rs1, idx)->_obj,
                                __RSArrayGetBucketAtIndex(rs2, idx)->_obj))
                return NO;
        }
        return YES;
    }
    else
    {
        for (RSIndex idx = 0; idx < cnt; idx++)
        {
            if (__RSArrayGetBucketAtIndex(rs1, idx)->_obj !=
                __RSArrayGetBucketAtIndex(rs2, idx)->_obj)
                return NO;
        }
    }
    return YES;
}

static RSRuntimeClass __RSArrayClass  =
{
    _RSRuntimeScannedObject,
    0,
    "RSArray",
    nil,
    __RSArrayClassCopy,
    __RSArrayClassDeallocate,
    __RSArrayClassEqual,
    __RSArrayClassHash,
    __RSArrayClassDescription,
    nil,
    nil,
};

static RSTypeID _RSArrayTypeID = _RSRuntimeNotATypeID;

RSPrivate void __RSArrayInitialize()
{
    _RSArrayTypeID = __RSRuntimeRegisterClass(&__RSArrayClass);
    __RSRuntimeSetClassTypeID(&__RSArrayClass, _RSArrayTypeID);
}

RSExport RSTypeID RSArrayGetTypeID()
{
    return _RSArrayTypeID;
}

static void __RSArrayAppendObjects(RSMutableArrayRef array, const void** objs, RSIndex count)
{
    __RSArrayReplaceObjects(array, RSMakeRange(__RSArrayGetCount(array), 0), objs, count);
}

static RSArrayRef __RSArrayCreateInstance(RSAllocatorRef allocaotr, const void** objs, RSIndex count, BOOL mutable,  const RSArrayCallBacks* callbacks, RSIndex capacity)
{
    int callbackType = 0;
    if (callbacks == nil) callbacks = RSArrayRSTypeCallBacks;
    if (__RSArrayCheckCallBackIsRSType(callbacks)) callbackType = __RSArrayRSCallback;
    else if (__RSArrayCheckCallBackIsNull(callbacks)) callbackType = __RSArrayNullCallback;
    else callbackType = __RSArrayCustomCallback;
    
    RSIndex size = __RSArrayGetSizeOfType(callbackType) - sizeof(struct __RSRuntimeBase);
    if (!mutable) size += sizeof(struct __RSArrayBucket) * capacity;
    
    RSMutableArrayRef array = (RSMutableArrayRef)__RSRuntimeCreateInstance(allocaotr, _RSArrayTypeID, size);
    if (array == nil) return nil;
    
    __RSArraySetCount(array, 0);
    if (mutable) __RSArraySetType(array, __RSArrayDeque);
    else __RSArraySetType(array, __RSArrayImmutable);
    if (callbackType == __RSArrayCustomCallback) *(RSArrayCallBacks*)__RSArrayGetCallBacks(array) = *callbacks;
    
    if (objs != nil && count > 0) __RSArrayAppendObjects(array, objs, count);
    return array;
}

RSExport RSArrayRef RSArrayCreateWithObject(RSAllocatorRef allocator, RSTypeRef obj)
{
    if (obj == nil) return nil;
    RSArrayRef array = __RSArrayCreateInstance(allocator, &obj, 1, NO, nil, 1);
    return array;
}

RSExport RSArrayRef RSArrayCreateWithObjects(RSAllocatorRef allocator, RSTypeRef* objects, RSIndex count)
{
    if (objects == nil) return nil;
    return __RSArrayCreateInstance(allocator, objects, count, NO, nil, count);
}

RSExport RSArrayRef RSArrayCreateWithObjectsNoCopy(RSAllocatorRef allocator, RSTypeRef* objects, RSIndex count, BOOL freeWhenDone)
{
    RSArrayRef array = RSArrayCreateWithObjects(allocator, objects, count);
    if (array && freeWhenDone) RSAllocatorDeallocate(RSAllocatorSystemDefault, objects);
    return array;
}

RSExport RSArrayRef RSArrayCreate(RSAllocatorRef allocator, RSTypeRef object, ...)
{
    RSMutableArrayRef array = RSArrayCreateMutable(allocator, 0);
    if (object != nil)
    {
        RSArrayAddObject(array, object);
        va_list ap;
        va_start(ap, object);
        RSTypeRef obj = object;
        while (obj)
        {
            obj = va_arg(ap, RSTypeRef);
            if (obj == nil) break;
            RSArrayAddObject(array, obj);
        }
    }
    return array;
}

RSExport RSMutableArrayRef RSArrayCreateMutable(RSAllocatorRef allocator, RSIndex capacity)
{
    return (RSMutableArrayRef)__RSArrayCreateInstance(allocator, nil, 0, YES, nil, 0);
}

RSExport RSMutableArrayRef RSArrayCreateMutableCopy(RSAllocatorRef allocator, RSArrayRef array)
{
    if (array == nil) return nil;
    RSMutableArrayRef copy = RSArrayCreateMutable(allocator, 0);
    RSIndex cnt = RSArrayGetCount(array);
    RSTypeRef obj = nil;
    for (RSIndex idx = 0; idx < cnt; idx++)
    {
        obj = RSCopy(allocator, RSArrayObjectAtIndex(array, idx));
        RSArrayAddObject(copy, obj);
        RSRelease(obj);
    }
    return copy;
}

RSExport RSIndex    RSArrayGetCount(RSArrayRef array)
{
    if (array == nil) return 0;
    __RSGenericValidInstance(array, _RSArrayTypeID);
    return __RSArrayGetCount(array);
}

RSExport RSTypeRef  RSArrayObjectAtIndex(RSArrayRef array, RSIndex idx)
{
    if (array == nil) return nil;
    __RSGenericValidInstance(array, _RSArrayTypeID);
    RSIndex cnt = __RSArrayGetCount(array);
    if (idx < 0 || cnt <= idx) return nil;
    return __RSArrayGetBucketAtIndex(array, idx)->_obj;
}

RSExport RSTypeRef  RSArrayLastObject(RSArrayRef array)
{
    return RSArrayObjectAtIndex(array, RSArrayGetCount(array) - 1);
}

RSExport void       RSArrayAddObject(RSMutableArrayRef array, RSTypeRef obj)
{
    if (array == nil || obj == nil) return;
    __RSGenericValidInstance(array, _RSArrayTypeID);
    __RSArrayAppendObjects(array, &obj, 1);
}

RSExport void       RSArrayRemoveObject(RSMutableArrayRef array, RSTypeRef obj)
{
    if (array == nil || obj == nil) return;
    __RSGenericValidInstance(array, _RSArrayTypeID);
    RSRange range = RSMakeRange(0, 1);
    range.location = RSArrayIndexOfObject(array, obj);
    if (range.location == RSNotFound) return;
    __RSArrayReplaceObjects(array, range, nil, 0);
}

RSExport void       RSArrayRemoveLastObject(RSMutableArrayRef array)
{
    if (array == nil) return;
    __RSGenericValidInstance(array, _RSArrayTypeID);
    RSIndex count = 0;
    if ((count = __RSArrayGetCount(array)))
    {
        __RSArrayReplaceObjects(array, RSMakeRange(count-1, 1), nil, 0);
    }
}

RSExport void       RSArrayRemoveAllObjects(RSMutableArrayRef array)
{
    if (array == nil) return;
    __RSGenericValidInstance(array, _RSArrayTypeID);
    RSIndex count = __RSArrayGetCount(array);
    __RSArrayReleaseObjects(array, RSMakeRange(0, count), YES);
    __RSArraySetCount(array, 0);
}

RSExport void       RSArrayRemoveObjectAtIndex(RSMutableArrayRef array, RSIndex idx)
{
    if (array == nil || idx < 0) return;
    __RSGenericValidInstance(array, _RSArrayTypeID);
    if (idx > __RSArrayGetCount(array)) return;
    __RSArrayReplaceObjects(array, RSMakeRange(idx, 1), nil, 0);
}

RSExport RSIndex    RSArrayIndexOfObject(RSArrayRef array, RSTypeRef obj)
{
    if (array == nil || obj == nil) return RSNotFound;
    __RSGenericValidInstance(array, _RSArrayTypeID);
    const RSArrayCallBacks* cb = __RSArrayGetCallBacks(array);
    RSIndex cnt = __RSArrayGetCount(array);
    if (cb->equal)
    {
        for (RSIndex idx = 0; idx < cnt; idx++)
        {
            id _obj = __RSArrayGetBucketAtIndex(array, idx)->_obj;
            if (obj == _obj || cb->equal(obj, _obj)) return idx;
        }
    }
    else
    {
        for (RSIndex idx = 0; idx < cnt; idx++)
        {
            if (obj == __RSArrayGetBucketAtIndex(array, idx)->_obj) return idx;
        }
    }
    return RSNotFound;
}

RSExport void       RSArraySort(RSArrayRef array, RSComparatorFunction comparator, BOOL ascending)
{
    if (array == nil || comparator == nil) return;
    __RSGenericValidInstance(array, _RSArrayTypeID);
    void ** buf[256], *values= buf;
    RSIndex cnt = __RSArrayGetCount(array);
    values = (cnt < 256) ? buf : RSAllocatorAllocate(RSAllocatorSystemDefault, cnt * sizeof(RSTypeRef));
    RSArrayGetObjects(array, RSMakeRange(0, cnt), values);
    RSQSortArray(values, __RSArrayGetCount(array), sizeof(void*), comparator, ascending);
    if (values != buf) RSAllocatorDeallocate(RSAllocatorSystemDefault, values);
}

RSExport void       RSArrayGetObjects(RSArrayRef array, RSRange range, RSTypeRef* values)
{
    if (array == nil || values == nil) return;
    __RSGenericValidInstance(array, _RSArrayTypeID);
    RSIndex cnt = __RSArrayGetCount(array);
    if (range.location < 0 ||
        range.location > cnt ||
        range.length < 0 ||
        range.location + range.length > cnt)
        return;
    switch (__RSArrayGetType(array))
    {
        case __RSArrayImmutable:
        case __RSArrayDeque:
            memmove(values, __RSArrayFindBucket(array) + range.location, range.length * sizeof(struct __RSArrayBucket));
            break;
        default:
            break;
    }
}

RSExport void       RSArrayApplyFunction(RSArrayRef array, RSRange range, RSArrayApplierFunction function, void* context)
{
    if (array == nil || function == nil) return;
    if (range.location < 0 || range.length < 0) return;
    __RSGenericValidInstance(array, _RSArrayTypeID);
    RSIndex cnt = __RSArrayGetCount(array);
    if (range.location + range.length > cnt) return;
    for (RSIndex idx = 0; idx < cnt; idx++)
    {
        RSTypeRef obj = __RSArrayGetBucketAtIndex(array, idx)->_obj;
        if (obj) function(obj, context);
    }
}

#elif RSARRAY_VERSION == 3


static RSArrayCallBacks __RSArrayRSTypeCallBacks = {0, __RSTypeCollectionRetain, __RSTypeCollectionRelease, RSDescription, RSEqual};
static RSArrayCallBacks __RSArrayNullCallBacks = {0, nil, nil, nil, nil};

const RSArrayCallBacks* RSArrayRSTypeCallBacks = &__RSArrayRSTypeCallBacks;
const RSArrayCallBacks* RSArrayNullCallBacks = &__RSArrayNullCallBacks;
const RSArrayCallBacks RSTypeArrayCallBacks = {0, __RSTypeCollectionRetain, __RSTypeCollectionRelease, RSDescription, RSEqual};
static const RSArrayCallBacks __RSNullArrayCallBacks = {0, nil, nil, nil, nil};

struct __RSArrayBucket {
    const void *_item;
};

enum {
    __RS_MAX_BUCKETS_PER_DEQUE = LONG_MAX
};

RSInline RSIndex __RSArrayDequeRoundUpCapacity(RSIndex capacity)
{
    if (capacity < 4) return 4;
    return min((1 << flsl(capacity)), __RS_MAX_BUCKETS_PER_DEQUE);
}

struct __RSArrayDeque {
    uintptr_t _leftIdx;
    uintptr_t _capacity;
    /* struct __RSArrayBucket buckets follow here */
};

struct __RSArray {
    RSRuntimeBase _base;
    RSIndex _count;		/* number of objects */
    RSIndex _mutations;
    int32_t _mutInProgress;
    __strong void *_store;           /* can be nil when MutableDeque */
};

/* Flag bits */
enum {		/* Bits 0-1 */
    __RSArrayImmutable = 0,
    __RSArrayDeque = 2,
};

enum {		/* Bits 2-3 */
    __RSArrayHasNullCallBacks = 0,
    __RSArrayHasRSTypeCallBacks = 1,
    __RSArrayHasCustomCallBacks = 3	/* callbacks are at end of header */
};

RSInline RSIndex __RSArrayGetType(RSArrayRef array)
{
    return __RSBitfieldGetValue((RSRuntimeClassBaseFiled(array)), 1, 0);
}

RSInline RSIndex __RSArrayGetSizeOfType(RSIndex t)
{
    RSIndex size = 0;
    size += sizeof(struct __RSArray);
    if (__RSBitfieldGetValue(t, 3, 2) == __RSArrayHasCustomCallBacks)
    {
        size += sizeof(RSArrayCallBacks);
    }
    return size;
}

RSInline RSIndex __RSArrayGetCount(RSArrayRef array)
{
    return array->_count;
}

RSInline void __RSArraySetCount(RSArrayRef array, RSIndex v)
{
    ((struct __RSArray *)array)->_count = v;
}

/* Only applies to immutable and mutable-deque-using arrays;
 * Returns the bucket holding the left-most real value in the latter case. */
RSInline struct __RSArrayBucket *__RSArrayGetBucketsPtr(RSArrayRef array)
{
    switch (__RSArrayGetType(array))
    {
        case __RSArrayImmutable:
            return (struct __RSArrayBucket *)((uint8_t *)array + __RSArrayGetSizeOfType(RSRuntimeClassBaseFiled(array)));
        case __RSArrayDeque:
        {
            struct __RSArrayDeque *deque = (struct __RSArrayDeque *)array->_store;
            return (struct __RSArrayBucket *)((uint8_t *)deque + sizeof(struct __RSArrayDeque) + deque->_leftIdx * sizeof(struct __RSArrayBucket));
        }
    }
    return nil;
}

/* This shouldn't be called if the array count is 0. */
RSInline struct __RSArrayBucket *__RSArrayGetBucketAtIndex(RSArrayRef array, RSIndex idx)
{
    switch (__RSArrayGetType(array))
    {
        case __RSArrayImmutable:
        case __RSArrayDeque:
            return __RSArrayGetBucketsPtr(array) + idx;
    }
    return nil;
}

RSPrivate RSArrayCallBacks *__RSArrayGetCallBacks(RSArrayRef array)
{
    RSArrayCallBacks *result = nil;
    switch (__RSBitfieldGetValue(RSRuntimeClassBaseFiled(array), 3, 2))
    {
        case __RSArrayHasNullCallBacks:
            return (RSArrayCallBacks *)&__RSNullArrayCallBacks;
        case __RSArrayHasRSTypeCallBacks:
            return (RSArrayCallBacks *)&RSTypeArrayCallBacks;
        case __RSArrayHasCustomCallBacks:
            break;
    }
    switch (__RSArrayGetType(array))
    {
        case __RSArrayImmutable:
            result = (RSArrayCallBacks *)((uint8_t *)array + sizeof(struct __RSArray));
            break;
        case __RSArrayDeque:
            result = (RSArrayCallBacks *)((uint8_t *)array + sizeof(struct __RSArray));
            break;
    }
    return result;
}

RSInline bool __RSArrayCallBacksMatchNull(const RSArrayCallBacks *c)
{
    return (nil == c ||
            (c->retain == __RSNullArrayCallBacks.retain &&
             c->release == __RSNullArrayCallBacks.release &&
             c->copyDescription == __RSNullArrayCallBacks.copyDescription &&
             c->equal == __RSNullArrayCallBacks.equal));
}

RSInline bool __RSArrayCallBacksMatchRSType(const RSArrayCallBacks *c)
{
    return (&RSTypeArrayCallBacks == c ||
            (c->retain == RSTypeArrayCallBacks.retain &&
             c->release == RSTypeArrayCallBacks.release &&
             c->copyDescription == RSTypeArrayCallBacks.copyDescription &&
             c->equal == RSTypeArrayCallBacks.equal));
}

#if 0
#define CHECK_FOR_MUTATION(A) do { if ((A)->_mutInProgress) RSLog(3, RSSTR("*** %s: function called while the array (%p) is being mutated in this or another thread"), __PRETTY_FUNCTION__, (A)); } while (0)
#define BEGIN_MUTATION(A) do { OSAtomicAdd32Barrier(1, &((struct __RSArray *)(A))->_mutInProgress); } while (0)
#define END_MUTATION(A) do { OSAtomicAdd32Barrier(-1, &((struct __RSArray *)(A))->_mutInProgress); } while (0)
#else
#define CHECK_FOR_MUTATION(A) do { } while (0)
#define BEGIN_MUTATION(A) do { } while (0)
#define END_MUTATION(A) do { } while (0)
#endif

struct _releaseContext {
    void (*release)(RSAllocatorRef, const void *);
    RSAllocatorRef allocator;
};

void _RSArrayReplaceObjects(RSMutableArrayRef array, RSRange range, const void **newObjects, RSIndex newCount);

static void __RSArrayReleaseObjects(RSArrayRef array, RSRange range, bool releaseStorageIfPossible)
{
    const RSArrayCallBacks *cb = __RSArrayGetCallBacks(array);
    RSAllocatorRef allocator;
    RSIndex idx;
    switch (__RSArrayGetType(array))
    {
        case __RSArrayImmutable:
            if (nil != cb->release && 0 < range.length)
            {
                // if we've been finalized then we know that
                //   1) we're using the standard callback on GC memory
                //   2) the slots don't' need to be zeroed
                struct __RSArrayBucket *buckets = __RSArrayGetBucketsPtr(array);
                allocator = RSGetAllocator(array);
                for (idx = 0; idx < range.length; idx++)
                {
                    INVOKE_CALLBACK2(cb->release, allocator, buckets[idx + range.location]._item);
                    buckets[idx + range.location]._item = nil; // GC:  break strong reference.
                }
            }
            break;
        case __RSArrayDeque: {
            struct __RSArrayDeque *deque = (struct __RSArrayDeque *)array->_store;
            if (0 < range.length && nil != deque)
            {
                struct __RSArrayBucket *buckets = __RSArrayGetBucketsPtr(array);
                if (nil != cb->release)
                {
                    allocator = RSGetAllocator(array);
                    for (idx = 0; idx < range.length; idx++)
                    {
                        INVOKE_CALLBACK2(cb->release, allocator, buckets[idx + range.location]._item);
                        buckets[idx + range.location]._item = nil; // GC:  break strong reference.
                    }
                } else {
                    for (idx = 0; idx < range.length; idx++)
                    {
                        buckets[idx + range.location]._item = nil; // GC:  break strong reference.
                    }
                }
            }
            if (releaseStorageIfPossible && 0 == range.location && __RSArrayGetCount(array) == range.length)
            {
                allocator = RSGetAllocator(array);
                if (nil != deque && !RS_IS_COLLECTABLE_ALLOCATOR(allocator)) RSAllocatorDeallocate(allocator, deque);
                __RSArraySetCount(array, 0);  // GC: _count == 0 ==> _store == nil.
                ((struct __RSArray *)array)->_store = nil;
            }
            break;
        }
    }
}

#if defined(DEBUG)
RSInline void __RSArrayValidateRange(RSArrayRef array, RSRange range, const char *func)
{
    if (range.location + range.length > RSArrayGetCount(array))
        __RSCLog(RSLogLevelDebug, "%s\n", func);
}
#else
#define __RSArrayValidateRange(a,r,f)
#endif

static RSTypeRef __RSArrayClassCopy(RSAllocatorRef allocator, RSTypeRef rs, BOOL mutableCopy)
{
    if (mutableCopy) return RSArrayCreateMutableCopy(allocator, rs);
    else return RSArrayCreateCopy(allocator, rs);
    return nil;
}

static BOOL __RSArrayClassEqual(RSTypeRef rs1, RSTypeRef rs2)
{
    RSArrayRef array1 = (RSArrayRef)rs1;
    RSArrayRef array2 = (RSArrayRef)rs2;
    const RSArrayCallBacks *cb1, *cb2;
    RSIndex idx, cnt;
    if (array1 == array2) return YES;
    cnt = __RSArrayGetCount(array1);
    if (cnt != __RSArrayGetCount(array2)) return NO;
    cb1 = __RSArrayGetCallBacks(array1);
    cb2 = __RSArrayGetCallBacks(array2);
    if (cb1->equal != cb2->equal) return NO;
    if (0 == cnt) return YES;	/* after function comparison! */
    for (idx = 0; idx < cnt; idx++)
    {
        const void *val1 = __RSArrayGetBucketAtIndex(array1, idx)->_item;
        const void *val2 = __RSArrayGetBucketAtIndex(array2, idx)->_item;
        if (val1 != val2)
        {
            if (nil == cb1->equal) return NO;
            if (!INVOKE_CALLBACK2(cb1->equal, val1, val2)) return NO;
        }
    }
    return YES;
}

static RSHashCode __RSArrayClassHash(RSTypeRef rs)
{
    RSArrayRef array = (RSArrayRef)rs;
    return __RSArrayGetCount(array);
}

static RSStringRef __RSArrayClassDescription(RSTypeRef rs)
{
    RSArrayRef array = (RSArrayRef)rs;
    RSMutableStringRef result;
    const RSArrayCallBacks *cb;
    RSAllocatorRef allocator;
    RSIndex idx, cnt;
    cnt = __RSArrayGetCount(array);
    allocator = RSGetAllocator(array);
    result = RSStringCreateMutable(allocator, 0);
//    switch (__RSArrayGetType(array))
//    {
//        case __RSArrayImmutable:
//            RSStringAppendStringWithFormat(result, RSSTR("<RSArray %p [%p]>{type = immutable, count = %u, values = (%s"), rs, allocator, cnt, cnt ? "\n" : "");
//            break;
//        case __RSArrayDeque:
//            RSStringAppendStringWithFormat(result, RSSTR("<RSArray %p [%p]>{type = mutable-small, count = %u, values = (%s"), rs, allocator, cnt, cnt ? "\n" : "");
//            break;
//    }
    cb = __RSArrayGetCallBacks(array);
    RSStringAppendString(result, RSSTR("("));
    for (idx = 0; idx < (cnt - 1); idx++)
    {
        RSStringRef desc = nil;
        const void *val = __RSArrayGetBucketAtIndex(array, idx)->_item;
        if (nil != cb->copyDescription)
        {
            desc = (RSStringRef)INVOKE_CALLBACK1(cb->copyDescription, val);
        }
        if (nil != desc)
        {
//            RSStringAppendStringWithFormat(result, RSSTR("\t%u : %R\n"), idx, desc);
            RSStringAppendStringWithFormat(result, RSSTR("%r "), desc);
            RSRelease(desc);
        } else {
            RSStringAppendStringWithFormat(result, RSSTR("<%p> "), val);
//            RSStringAppendStringWithFormat(result, RSSTR("\t%u : <%p>\n"), idx, val);
        }
    }
    
    if (cnt > 0) {
        RSStringRef desc = nil;
        const void *val = __RSArrayGetBucketAtIndex(array, idx)->_item;
        if (nil != cb->copyDescription)
        {
            desc = (RSStringRef)INVOKE_CALLBACK1(cb->copyDescription, val);
        }
        if (nil != desc)
        {
            //            RSStringAppendStringWithFormat(result, RSSTR("\t%u : %R\n"), idx, desc);
            RSStringAppendStringWithFormat(result, RSSTR("%r"), desc);
            RSRelease(desc);
        } else {
            RSStringAppendStringWithFormat(result, RSSTR("<%p>"), val);
            //            RSStringAppendStringWithFormat(result, RSSTR("\t%u : <%p>\n"), idx, val);
        }
    }
    RSStringAppendString(result, RSSTR(")"));
    return result;
}


static void __RSArrayClassDeallocate(RSTypeRef rs)
{
    RSArrayRef array = (RSArrayRef)rs;
    __RSArrayReleaseObjects(array, RSMakeRange(0, __RSArrayGetCount(array)), YES);
    return;
    BEGIN_MUTATION(array);
#if DEPLOYMENT_TARGET_MACOSX
    // Under GC, keep contents alive when we know we can, either standard callbacks or nil
    // if (__RSBitfieldGetObject(rs->info, 5, 4)) return; // bits only ever set under GC
    RSAllocatorRef allocator = RSGetAllocator(array);
    if (RS_IS_COLLECTABLE_ALLOCATOR(allocator))
    {
        // XXX_PCB keep array intact during finalization.
        const RSArrayCallBacks *cb = __RSArrayGetCallBacks(array);
		if (cb->retain == nil && cb->release == nil)
        {
            END_MUTATION(array);
            return;
        }
        if (cb == &RSTypeArrayCallBacks || cb->release == RSTypeArrayCallBacks.release)
        {
            for (RSIndex idx = 0; idx < __RSArrayGetCount(array); idx++)
            {
                const void *item = RSArrayObjectAtIndex(array, 0 + idx);
    	        RSTypeArrayCallBacks.release(RSAllocatorSystemDefault, item);
            }
            END_MUTATION(array);
            return;
        }
    }
#endif
    
    END_MUTATION(array);
}

static RSTypeID __RSArrayTypeID = _RSRuntimeNotATypeID;

static const RSRuntimeClass __RSArrayClass = {
    _RSRuntimeScannedObject,
    0,
    "RSArray",
    nil,	// init
    __RSArrayClassCopy,
    __RSArrayClassDeallocate,
    __RSArrayClassEqual,
    __RSArrayClassHash,
    __RSArrayClassDescription,
    nil,
    nil,
};

RSPrivate void __RSArrayInitialize(void)
{
    __RSArrayTypeID = __RSRuntimeRegisterClass(&__RSArrayClass);
    __RSRuntimeSetClassTypeID(&__RSArrayClass, __RSArrayTypeID);
}

RSExport RSTypeID RSArrayGetTypeID(void)
{
    return __RSArrayTypeID;
}

static RSArrayRef __RSArrayInit(RSAllocatorRef allocator, UInt32 flags, RSIndex capacity, const RSArrayCallBacks *callBacks)
{
    struct __RSArray *memory;
    RSIndex size;
    __RSBitfieldSetValue(flags, 31, 2, 0);
    if (RS_IS_COLLECTABLE_ALLOCATOR(allocator))
    {
        if (!callBacks || (callBacks->retain == nil && callBacks->release == nil))
        {
            __RSBitfieldSetValue(flags, 4, 4, 1); // setWeak
        }
    }
    if (__RSArrayCallBacksMatchNull(callBacks))
    {
        __RSBitfieldSetValue(flags, 3, 2, __RSArrayHasNullCallBacks);
    } else if (__RSArrayCallBacksMatchRSType(callBacks))
    {
        __RSBitfieldSetValue(flags, 3, 2, __RSArrayHasRSTypeCallBacks);
    } else {
        __RSBitfieldSetValue(flags, 3, 2, __RSArrayHasCustomCallBacks);
    }
    size = __RSArrayGetSizeOfType(flags) - sizeof(RSRuntimeBase);
    switch (__RSBitfieldGetValue(flags, 1, 0))
    {
        case __RSArrayImmutable:
            size += capacity * sizeof(struct __RSArrayBucket);
            break;
        case __RSArrayDeque:
            break;
    }
    memory = (struct __RSArray*)__RSRuntimeCreateInstance(allocator, __RSArrayTypeID, size);
    if (nil == memory)
    {
        return nil;
    }
    __RSBitfieldSetValue(RSRuntimeClassBaseFiled(memory), 6, 0, flags);
    __RSArraySetCount((RSArrayRef)memory, 0);
    switch (__RSBitfieldGetValue(flags, 1, 0))
    {
        case __RSArrayImmutable:
            
            break;
        case __RSArrayDeque:
            ((struct __RSArray *)memory)->_mutations = 1;
            ((struct __RSArray *)memory)->_mutInProgress = 0;
            ((struct __RSArray*)memory)->_store = nil;
            break;
    }
    if (__RSArrayHasCustomCallBacks == __RSBitfieldGetValue(flags, 3, 2))
    {
        RSArrayCallBacks *cb = (RSArrayCallBacks *)__RSArrayGetCallBacks((RSArrayRef)memory);
        *cb = *callBacks;
        FAULT_CALLBACK((void **)&(cb->retain));
        FAULT_CALLBACK((void **)&(cb->release));
        FAULT_CALLBACK((void **)&(cb->copyDescription));
        FAULT_CALLBACK((void **)&(cb->equal));
    }
    return (RSArrayRef)memory;
}

RSPrivate RSArrayRef __RSArrayCreateTransfer(RSAllocatorRef allocator, const void **values, RSIndex numObjects)
{
    UInt32 flags = __RSArrayImmutable;
    __RSBitfieldSetValue(flags, 31, 2, 0);
    __RSBitfieldSetValue(flags, 3, 2, __RSArrayHasRSTypeCallBacks);
    RSIndex size = __RSArrayGetSizeOfType(flags) - sizeof(RSRuntimeBase);
    size += numObjects * sizeof(struct __RSArrayBucket);
    struct __RSArray *memory = (struct __RSArray*)__RSRuntimeCreateInstance(allocator, __RSArrayTypeID, size);
    if (nil == memory)
    {
        return nil;
    }
    __RSBitfieldSetValue(RSRuntimeClassBaseFiled(memory), 6, 0, flags);
    __RSArraySetCount(memory, numObjects);
    memmove(__RSArrayGetBucketsPtr(memory), values, sizeof(void *) * numObjects);
    return (RSArrayRef)memory;
}

RSPrivate RSArrayRef __RSArrayCreate0(RSAllocatorRef allocator, const void **values, RSIndex numObjects, const RSArrayCallBacks *callBacks)
{
    if (values == nil) return nil;
    if (*values == nil) return nil;
    RSArrayRef result;
    const RSArrayCallBacks *cb;
    struct __RSArrayBucket *buckets;
    RSAllocatorRef bucketsAllocator;
    void* bucketsBase;
    RSIndex idx;
    result = __RSArrayInit(allocator, __RSArrayImmutable, numObjects, callBacks);
    cb = __RSArrayGetCallBacks(result);
    buckets = __RSArrayGetBucketsPtr(result);
    bucketsAllocator = allocator;
    bucketsBase = nil;
    
    if (nil != cb->retain)
    {
        for (idx = 0; idx < numObjects; idx++)
        {
            __RSAssignWithWriteBarrier((void **)&buckets->_item, (void *)INVOKE_CALLBACK2(cb->retain, allocator, *values));
            values++;
            buckets++;
        }
    }
    else {
        for (idx = 0; idx < numObjects; idx++)
        {
            __RSAssignWithWriteBarrier((void **)&buckets->_item, (void *)*values);
            values++;
            buckets++;
        }
    }
    __RSArraySetCount(result, numObjects);
    return result;
}

RSExport RSMutableArrayRef __RSArrayCreateMutable0(RSAllocatorRef allocator, RSIndex capacity, const RSArrayCallBacks *callBacks)
{
    return (RSMutableArrayRef)__RSArrayInit(allocator, __RSArrayDeque, capacity, callBacks);
}

RSExport RSArrayRef __RSArrayCreateCopy0(RSAllocatorRef allocator, RSArrayRef array)
{
    if (array == nil) return nil;
    RSArrayRef result;
    const RSArrayCallBacks *cb;
    struct __RSArrayBucket *buckets;
    RSAllocatorRef bucketsAllocator;
    void* bucketsBase;
    RSIndex numObjects = RSArrayGetCount(array);
    RSIndex idx;
    if (RS_IS_OBJC(__RSArrayTypeID, array))
    {
        cb = &RSTypeArrayCallBacks;
    }
    else
    {
        cb = __RSArrayGetCallBacks(array);
    }
    result = __RSArrayInit(allocator, __RSArrayImmutable, numObjects, cb);
    cb = __RSArrayGetCallBacks(result); // GC: use the new array's callbacks so we don't leak.
    buckets = __RSArrayGetBucketsPtr(result);
    bucketsAllocator = allocator;
	bucketsBase = nil;
    for (idx = 0; idx < numObjects; idx++)
    {
        const void *value = RSArrayObjectAtIndex(array, idx);
        if (nil != cb->retain)
        {
            value = (void *)INVOKE_CALLBACK2(cb->retain, allocator, value);
        }
        __RSAssignWithWriteBarrier((void **)&buckets->_item, (void *)value);
        buckets++;
    }
    __RSArraySetCount(result, numObjects);
    return result;
}
void _RSArraySetCapacity(RSMutableArrayRef array, RSIndex cap);
RSExport RSMutableArrayRef __RSArrayCreateMutableCopy0(RSAllocatorRef allocator, RSIndex capacity, RSArrayRef array)
{
    if (capacity <= 0 || array == nil) return nil;
    RSMutableArrayRef result;
    const RSArrayCallBacks *cb;
    RSIndex idx, numObjects = RSArrayGetCount(array);
    UInt32 flags;
    if (RS_IS_OBJC(__RSArrayTypeID, array))
    {
        cb = &RSTypeArrayCallBacks;
    }
    else
    {
        cb = __RSArrayGetCallBacks(array);
    }
    flags = __RSArrayDeque;
    result = (RSMutableArrayRef)__RSArrayInit(allocator, flags, capacity, cb);
    if (0 == capacity) _RSArraySetCapacity(result, numObjects);
    for (idx = 0; idx < numObjects; idx++)
    {
        const void *value = RSArrayObjectAtIndex(array, idx);
        RSArrayAddObject(result, value);
    }
    return result;
}

#define DEFINE_CREATION_METHODS 1

#if DEFINE_CREATION_METHODS

RSExport RSArrayRef RSArrayCreateWithObjects(RSAllocatorRef allocator, const void **values, RSIndex numObjects)
{
    return __RSArrayCreate0(allocator, values, numObjects, RSArrayRSTypeCallBacks);
}

RSExport RSArrayRef RSArrayCreateWithObject(RSAllocatorRef allocator, RSTypeRef object)
{
    return RSArrayCreateWithObjects(allocator, &object, 1);
}

RSExport RSArrayRef RSArrayCreateWithObjectsNoCopy(RSAllocatorRef allocator, RSTypeRef* objects, RSIndex numObjects, BOOL freeWhenDone)
{
    RSArrayRef array = RSArrayCreateWithObjects(allocator, objects, numObjects);
    if (freeWhenDone)
        RSAllocatorDeallocate(RSAllocatorSystemDefault, objects);
    return array;
}

RSExport RSArrayRef RSArrayCreate(RSAllocatorRef allocator, RSTypeRef object, ...)
{
    RSMutableArrayRef array = RSArrayCreateMutable(allocator, 0);
    va_list ap;
    RSTypeRef obj = object;
    va_start(ap, object);
    
    while (obj != nil)
    {
        RSArrayAddObject(array, obj);
        obj = va_arg(ap, RSTypeRef);
    }
    
    va_end(ap);
    return array;
}

RSExport RSMutableArrayRef RSArrayCreateMutable(RSAllocatorRef allocator, RSIndex capacity)
{
    return __RSArrayCreateMutable0(allocator, capacity, RSArrayRSTypeCallBacks);
}

RSExport RSArrayRef RSArrayCreateCopy(RSAllocatorRef allocator, RSArrayRef array)
{
    return __RSArrayCreateCopy0(allocator, array);
}

RSExport RSMutableArrayRef RSArrayCreateMutableCopy(RSAllocatorRef allocator, RSArrayRef array)
{
    return __RSArrayCreateMutableCopy0(allocator, RSArrayGetCount(array) + 1, array);
}

#endif

RSExport RSIndex RSArrayGetCount(RSArrayRef array)
{
    RS_OBJC_FUNCDISPATCHV(__RSArrayTypeID, RSIndex, (NSArray *)array, count);
    if (!array) return 0;
    __RSGenericValidInstance(array, __RSArrayTypeID);
    return __RSArrayGetCount(array);
}


RSExport RSIndex RSArrayGetCountOfObject(RSArrayRef array, RSRange range, const void *object)
{
    RSIndex idx, count = 0;
    __RSGenericValidInstance(array, __RSArrayTypeID);
    __RSArrayValidateRange(array, range, __PRETTY_FUNCTION__);
    
    const RSArrayCallBacks *cb = RS_IS_OBJC(RSArrayGetTypeID(), array) ? &RSTypeArrayCallBacks : __RSArrayGetCallBacks(array);
    for (idx = 0; idx < range.length; idx++)
    {
        const void *item = RSArrayObjectAtIndex(array, range.location + idx);
        if (object == item || (cb->equal && INVOKE_CALLBACK2(cb->equal, object, item)))
        {
            count++;
        }
    }
    return count;
}

RSExport BOOL RSArrayContainsObject(RSArrayRef array, RSRange range, const void *value)
{
    RSIndex idx;
    __RSGenericValidInstance(array, __RSArrayTypeID);
    __RSArrayValidateRange(array, range, __PRETTY_FUNCTION__);
    
    const RSArrayCallBacks *cb = RS_IS_OBJC(RSArrayGetTypeID(), array) ? &RSTypeArrayCallBacks : __RSArrayGetCallBacks(array);
    for (idx = 0; idx < range.length; idx++)
    {
        const void *item = RSArrayObjectAtIndex(array, range.location + idx);
        if (value == item || (cb->equal && INVOKE_CALLBACK2(cb->equal, value, item)))
        {
            return YES;
        }
    }
    return NO;
}

const void *RSArrayObjectAtIndex(RSArrayRef array, RSIndex idx)
{
    if (!array) return nil;
    __RSGenericValidInstance(array, __RSArrayTypeID);
    return __RSArrayGetBucketAtIndex(array, idx)->_item;
}

RSTypeRef RSArrayLastObject(RSArrayRef array)
{
    if (RSArrayGetCount(array))
        return RSArrayObjectAtIndex(array, RSArrayGetCount(array) - 1);
    return nil;
}

void RSArrayRemoveObject(RSMutableArrayRef array, RSTypeRef object)
{
    RSIndex idx = RSArrayIndexOfObject(array, object);
    if (idx == RSNotFound) return;
    RSArrayRemoveObjectAtIndex(array, idx);
}

void RSArrayRemoveLastObject(RSMutableArrayRef array)
{
    if (RSArrayGetCount(array))
        RSArrayRemoveObjectAtIndex(array, RSArrayGetCount(array) - 1);
}

// This is for use by NSRSArray; it avoids ObjC dispatch, and checks for out of bounds
const void *_RSArrayCheckAndGetObjectAtIndex(RSArrayRef array, RSIndex idx)
{
    if (0 <= idx && idx < __RSArrayGetCount(array)) return __RSArrayGetBucketAtIndex(array, idx)->_item;
    return (void *)(-1);
}


void RSArrayGetObjects(RSArrayRef array, RSRange range, const void **values)
{
    __RSGenericValidInstance(array, __RSArrayTypeID);
    __RSArrayValidateRange(array, range, __PRETTY_FUNCTION__);
    if (0 < range.length)
    {
        switch (__RSArrayGetType(array))
        {
            case __RSArrayImmutable:
            case __RSArrayDeque:
                memmove(values, __RSArrayGetBucketsPtr(array) + range.location, range.length * sizeof(struct __RSArrayBucket));
                break;
        }
    }
}

void RSArrayApplyFunction(RSArrayRef array, RSRange range, RSArrayApplierFunction applier, void *context) 
{
    RSIndex idx;
    FAULT_CALLBACK((void **)&(applier));
    __RSGenericValidInstance(array, __RSArrayTypeID);
    __RSArrayValidateRange(array, range, __PRETTY_FUNCTION__);
    
    for (idx = 0; idx < range.length; idx++)
    {
        const void *item = __RSArrayGetBucketAtIndex(array, range.location + idx)->_item;
        INVOKE_CALLBACK2(applier, item, context);
    }
}

#if RS_BLOCKS_AVAILABLE
RSExport void RSArrayApplyBlock(RSArrayRef array, RSRange range, void (^block)(const void*value, RSUInteger idx, BOOL *isStop))
{
    if (!array || range.length == 0) return;
    RSIndex idx;
    FAULT_CALLBACK((void **)&(applier));
    __RSGenericValidInstance(array, __RSArrayTypeID);
    __RSArrayValidateRange(array, range, __PRETTY_FUNCTION__);
    
    __block BOOL isStop = NO;
    for (idx = 0; idx < range.length && !isStop; idx++)
    {
        const void *item = __RSArrayGetBucketAtIndex(array, range.location + idx)->_item;
        block(item, range.location + idx, &isStop);
    }
}
#endif

RSIndex RSArrayGetFirstIndexOfObject(RSArrayRef array, RSRange range, const void *value)
{
    RSIndex idx;
    __RSGenericValidInstance(array, __RSArrayTypeID);
    __RSArrayValidateRange(array, range, __PRETTY_FUNCTION__);
    
    const RSArrayCallBacks *cb = RS_IS_OBJC(RSArrayGetTypeID(), array) ? &RSTypeArrayCallBacks : __RSArrayGetCallBacks(array);
    for (idx = 0; idx < range.length; idx++)
    {
        const void *item = __RSArrayGetBucketAtIndex(array, range.location + idx)->_item; // object at index
        if (value == item || (cb->equal && INVOKE_CALLBACK2(cb->equal, value, item)))
            return idx + range.location;
    }
    return RSNotFound;
}

RSIndex RSArrayGetLastIndexOfObject(RSArrayRef array, RSRange range, const void *value)
{
    RSIndex idx;
    __RSGenericValidInstance(array, __RSArrayTypeID);
    __RSArrayValidateRange(array, range, __PRETTY_FUNCTION__);
    
    const RSArrayCallBacks *cb = RS_IS_OBJC(RSArrayGetTypeID(), array) ? &RSTypeArrayCallBacks : __RSArrayGetCallBacks(array);
    for (idx = range.length; idx--;)
    {
        const void *item = RSArrayObjectAtIndex(array, range.location + idx);
        if (value == item || (cb->equal && INVOKE_CALLBACK2(cb->equal, value, item)))
            return idx + range.location;
    }
    return RSNotFound;
}

void RSArrayAddObject(RSMutableArrayRef array, const void *value)
{
    __RSGenericValidInstance(array, __RSArrayTypeID);
    _RSArrayReplaceObjects(array, RSMakeRange(__RSArrayGetCount(array), 0), &value, 1);
}

RSExport void RSArrayAddObjects(RSMutableArrayRef array, RSArrayRef objects)
{
    __RSGenericValidInstance(array, __RSArrayTypeID);
    __RSGenericValidInstance(objects, __RSArrayTypeID);
    RSRange otherRange = RSMakeRange(0, RSArrayGetCount(objects));
    __RSArrayValidateRange(objects, otherRange, __PRETTY_FUNCTION__);
    // implemented abstractly, careful!
    for (RSIndex idx = otherRange.location; idx < otherRange.location + otherRange.length; idx++)
    {
        RSArrayAddObject(array, RSArrayObjectAtIndex(objects, idx));
    }
}

void RSArraySetObjectAtIndex(RSMutableArrayRef array, RSIndex idx, const void *value)
{
    __RSGenericValidInstance(array, __RSArrayTypeID);
    
    if (idx == __RSArrayGetCount(array))
    {
        _RSArrayReplaceObjects(array, RSMakeRange(idx, 0), &value, 1);
    }
    else
    {
        const void *old_value;
        const RSArrayCallBacks *cb = __RSArrayGetCallBacks(array);
        RSAllocatorRef allocator = RSGetAllocator(array);
        struct __RSArrayBucket *bucket = __RSArrayGetBucketAtIndex(array, idx);
        if (nil != cb->retain)
        {
            value = (void *)INVOKE_CALLBACK2(cb->retain, allocator, value);
        }
        old_value = bucket->_item;
        __RSAssignWithWriteBarrier((void **)&bucket->_item, (void *)value);
        if (nil != cb->release)
        {
            INVOKE_CALLBACK2(cb->release, allocator, old_value);
        }
        array->_mutations++;
    }
}

void RSArrayInsertObjectAtIndex(RSMutableArrayRef array, RSIndex idx, const void *value)
{
    __RSGenericValidInstance(array, __RSArrayTypeID);
    _RSArrayReplaceObjects(array, RSMakeRange(idx, 0), &value, 1);
}

// NB: AddressBook on the Phone is a fragile flower, so this function cannot do anything
// that causes the values to be retained or released.
void RSArrayExchangeObjectsAtIndices(RSMutableArrayRef array, RSIndex idx1, RSIndex idx2)
{
    const void *tmp;
    struct __RSArrayBucket *bucket1, *bucket2;
    __RSGenericValidInstance(array, __RSArrayTypeID);
    
    bucket1 = __RSArrayGetBucketAtIndex(array, idx1);
    bucket2 = __RSArrayGetBucketAtIndex(array, idx2);
    tmp = bucket1->_item;
    
    __RSAssignWithWriteBarrier((void **)&bucket1->_item, (void *)bucket2->_item);
    __RSAssignWithWriteBarrier((void **)&bucket2->_item, (void *)tmp);
}

void RSArrayRemoveObjectAtIndex(RSMutableArrayRef array, RSIndex idx)
{
    __RSGenericValidInstance(array, __RSArrayTypeID);
    _RSArrayReplaceObjects(array, RSMakeRange(idx, 1), nil, 0);
}

void RSArrayRemoveAllObjects(RSMutableArrayRef array)
{
    __RSGenericValidInstance(array, __RSArrayTypeID);
    __RSArrayReleaseObjects(array, RSMakeRange(0, __RSArrayGetCount(array)), YES);
    __RSArraySetCount(array, 0);
}

// may move deque storage, as it may need to grow deque
static void __RSArrayRepositionDequeRegions(RSMutableArrayRef array, RSRange range, RSIndex newCount)
{
    // newCount elements are going to replace the range, and the result will fit in the deque
    struct __RSArrayDeque *deque = (struct __RSArrayDeque *)array->_store;
    struct __RSArrayBucket *buckets;
    RSIndex cnt, futureCnt, numNewElems;
    RSIndex L, A, B, C, R;
    
    buckets = (struct __RSArrayBucket *)((uint8_t *)deque + sizeof(struct __RSArrayDeque));
    cnt = __RSArrayGetCount(array);
    futureCnt = cnt - range.length + newCount;
    
    L = deque->_leftIdx;		// length of region to left of deque
    A = range.location;			// length of region in deque to left of replaced range
    B = range.length;			// length of replaced range
    C = cnt - B - A;			// length of region in deque to right of replaced range
    R = deque->_capacity - cnt - L;	// length of region to right of deque
    numNewElems = newCount - B;
    
    RSIndex wiggle = deque->_capacity >> 17;
    if (wiggle < 4) wiggle = 4;
    if (deque->_capacity < (uint32_t)futureCnt || (cnt < futureCnt && L + R < wiggle))
    {
        // must be inserting or space is tight, reallocate and re-center everything
        RSIndex capacity = __RSArrayDequeRoundUpCapacity(futureCnt + wiggle);
        RSIndex size = sizeof(struct __RSArrayDeque) + capacity * sizeof(struct __RSArrayBucket);
        RSAllocatorRef allocator = RSGetAllocator(array);
        allocator = allocator;
        BOOL collectableMemory = RS_IS_COLLECTABLE_ALLOCATOR(allocator);
        struct __RSArrayDeque *newDeque = (struct __RSArrayDeque *)RSAllocatorAllocate(allocator, size);
        struct __RSArrayBucket *newBuckets = (struct __RSArrayBucket *)((uint8_t *)newDeque + sizeof(struct __RSArrayDeque));
        RSIndex oldL = L;
        RSIndex newL = (capacity - futureCnt) / 2;
        RSIndex oldC0 = oldL + A + B;
        RSIndex newC0 = newL + A + newCount;
        newDeque->_leftIdx = newL;
        newDeque->_capacity = capacity;
        if (0 < A) memmove(newBuckets + newL, buckets + oldL, A * sizeof(struct __RSArrayBucket));
        if (0 < C) memmove(newBuckets + newC0, buckets + oldC0, C * sizeof(struct __RSArrayBucket));
        __RSAssignWithWriteBarrier((void **)&array->_store, (void *)newDeque);
        if (!collectableMemory && deque) RSAllocatorDeallocate(allocator, deque);
        //printf("3:  array %p store is now %p (%lx)\n", array, array->_store, *(unsigned long *)(array->_store));
        return;
    }
    
    if ((numNewElems < 0 && C < A) || (numNewElems <= R && C < A))
    {
        // move C
        // deleting: C is smaller
        // inserting: C is smaller and R has room
        RSIndex oldC0 = L + A + B;
        RSIndex newC0 = L + A + newCount;
        if (0 < C) memmove(buckets + newC0, buckets + oldC0, C * sizeof(struct __RSArrayBucket));
        // GrP GC: zero-out newly exposed space on the right, if any
        if (oldC0 > newC0) memset(buckets + newC0 + C, 0, (oldC0 - newC0) * sizeof(struct __RSArrayBucket));
    }
    else if ((numNewElems < 0) || (numNewElems <= L && A <= C))
    {
        // move A
        // deleting: A is smaller or equal (covers remaining delete cases)
        // inserting: A is smaller and L has room
        RSIndex oldL = L;
        RSIndex newL = L - numNewElems;
        deque->_leftIdx = newL;
        if (0 < A) memmove(buckets + newL, buckets + oldL, A * sizeof(struct __RSArrayBucket));
        // GrP GC: zero-out newly exposed space on the left, if any
        if (newL > oldL) memset(buckets + oldL, 0, (newL - oldL) * sizeof(struct __RSArrayBucket));
    }
    else
    {
        // now, must be inserting, and either:
        //    A<=C, but L doesn't have room (R might have, but don't care)
        //    C<A, but R doesn't have room (L might have, but don't care)
        // re-center everything
        RSIndex oldL = L;
        RSIndex newL = (L + R - numNewElems) / 2;
        newL = newL - newL / 2;
        RSIndex oldC0 = oldL + A + B;
        RSIndex newC0 = newL + A + newCount;
        deque->_leftIdx = newL;
        if (newL < oldL)
        {
            if (0 < A) memmove(buckets + newL, buckets + oldL, A * sizeof(struct __RSArrayBucket));
            if (0 < C) memmove(buckets + newC0, buckets + oldC0, C * sizeof(struct __RSArrayBucket));
            // GrP GC: zero-out newly exposed space on the right, if any
            if (oldC0 > newC0) memset(buckets + newC0 + C, 0, (oldC0 - newC0) * sizeof(struct __RSArrayBucket));
        }
        else
        {
            if (0 < C) memmove(buckets + newC0, buckets + oldC0, C * sizeof(struct __RSArrayBucket));
            if (0 < A) memmove(buckets + newL, buckets + oldL, A * sizeof(struct __RSArrayBucket));
            // GrP GC: zero-out newly exposed space on the left, if any
            if (newL > oldL) memset(buckets + oldL, 0, (newL - oldL) * sizeof(struct __RSArrayBucket));
        }
    }
}

static void __RSArrayHandleOutOfMemory(RSTypeRef obj, RSIndex numBytes)
{
    RSStringRef msg = RSStringCreateWithFormat(RSAllocatorSystemDefault, RSSTR("Attempt to allocate %ld bytes for RSArray failed"), numBytes);
    {
        RSLog(RSSTR("%R"), msg);
        HALTWithError(RSInvalidArgumentException, "Attempt to allocate n bytes for RSArray failed");
    }
    RSRelease(msg);
}

// This function is for Foundation's benefit; no one else should use it.
void _RSArraySetCapacity(RSMutableArrayRef array, RSIndex cap)
{
    if (RS_IS_OBJC(__RSArrayTypeID, array)) return;
    __RSGenericValidInstance(array, __RSArrayTypeID);
    
    // Currently, attempting to set the capacity of an array which is the RSStorage
    // variant, or set the capacity larger than __RS_MAX_BUCKETS_PER_DEQUE, has no
    // effect.  The primary purpose of this API is to help avoid a bunch of the
    // resizes at the small capacities 4, 8, 16, etc.
    if (__RSArrayGetType(array) == __RSArrayDeque)
    {
        struct __RSArrayDeque *deque = (struct __RSArrayDeque *)array->_store;
        RSIndex capacity = __RSArrayDequeRoundUpCapacity(cap);
        RSIndex size = sizeof(struct __RSArrayDeque) + capacity * sizeof(struct __RSArrayBucket);
        RSAllocatorRef allocator = RSGetAllocator(array);
        allocator = allocator;
        BOOL collectableMemory = RS_IS_COLLECTABLE_ALLOCATOR(allocator);
        if (nil == deque)
        {
            deque = (struct __RSArrayDeque *)RSAllocatorAllocate(allocator, size);
            if (nil == deque) __RSArrayHandleOutOfMemory(array, size);
            deque->_leftIdx = capacity / 2;
        }
        else
        {
            struct __RSArrayDeque *olddeque = deque;
            RSIndex oldcap = deque->_capacity;
            deque = (struct __RSArrayDeque *)RSAllocatorAllocate(allocator, size);
            if (nil == deque) __RSArrayHandleOutOfMemory(array, size);
            memmove(deque, olddeque, sizeof(struct __RSArrayDeque) + oldcap * sizeof(struct __RSArrayBucket));
            if (!collectableMemory) RSAllocatorDeallocate(allocator, olddeque);
        }
        deque->_capacity = capacity;
        __RSAssignWithWriteBarrier((void **)&array->_store, (void *)deque);
    }
    END_MUTATION(array);
}


void RSArrayReplaceObject(RSMutableArrayRef array, RSRange range, const void **newObjects, RSIndex newCount)
{
    __RSGenericValidInstance(array, __RSArrayTypeID);
    __RSArrayValidateRange(array, range, __PRETTY_FUNCTION__);
    return _RSArrayReplaceObjects(array, range, newObjects, newCount);
}

// This function does no ObjC dispatch or argument checking;
// It should only be called from places where that dispatch and check has already been done, or NSRSArray
void _RSArrayReplaceObjects(RSMutableArrayRef array, RSRange range, const void **newObjects, RSIndex newCount)
{
    const RSArrayCallBacks *cb;
    RSIndex idx, cnt, futureCnt;
    const void **newv, *buffer[256];
    cnt = __RSArrayGetCount(array);
    futureCnt = cnt - range.length + newCount;
    cb = __RSArrayGetCallBacks(array);
    RSAllocatorRef allocator = RSGetAllocator(array);
    
    /* Retain new values if needed, possibly allocating a temporary buffer for them */
    if (nil != cb->retain)
    {
        newv = (newCount <= 256) ? (const void **)buffer : (const void **)RSAllocatorAllocate(RSAllocatorSystemDefault, newCount * sizeof(void *)); // GC OK
        for (idx = 0; idx < newCount; idx++)
        {
            newv[idx] = (void *)INVOKE_CALLBACK2(cb->retain, allocator, (void *)newObjects[idx]);
        }
    }
    else
    {
        newv = newObjects;
    }
    
    /* Now, there are three regions of interest, each of which may be empty:
     *   A: the region from index 0 to one less than the range.location
     *   B: the region of the range
     *   C: the region from range.location + range.length to the end
     * Note that index 0 is not necessarily at the lowest-address edge
     * of the available storage. The values in region B need to get
     * released, and the values in regions A and C (depending) need
     * to get shifted if the number of new values is different from
     * the length of the range being replaced.
     */
    if (0 < range.length)
    {
        __RSArrayReleaseObjects(array, range, NO);
    }
    // region B elements are now "dead"
    if (nil == array->_store)
    {
        if (0 <= futureCnt)
        {
            struct __RSArrayDeque *deque;
            RSIndex capacity = __RSArrayDequeRoundUpCapacity(futureCnt);
            RSIndex size = sizeof(struct __RSArrayDeque) + capacity * sizeof(struct __RSArrayBucket);
            deque = (struct __RSArrayDeque *)RSAllocatorAllocate((allocator), size);
            deque->_leftIdx = (capacity - newCount) / 2;
            deque->_capacity = capacity;
            __RSAssignWithWriteBarrier((void **)&array->_store, (void *)deque);
        }
    }
    else
    {
		// Deque
        // reposition regions A and C for new region B elements in gap
        if (range.length != newCount)
        {
            __RSArrayRepositionDequeRegions(array, range, newCount);
        }
    }
    // copy in new region B elements
    if (0 < newCount)
    {
        // Deque
        struct __RSArrayDeque *deque = (struct __RSArrayDeque *)array->_store;
        struct __RSArrayBucket *raw_buckets = (struct __RSArrayBucket *)((uint8_t *)deque + sizeof(struct __RSArrayDeque));
        memmove(raw_buckets + deque->_leftIdx + range.location, newv, newCount * sizeof(struct __RSArrayBucket));
    }
    
    __RSArraySetCount(array, futureCnt);
    if (newv != buffer && newv != newObjects) RSAllocatorDeallocate(RSAllocatorSystemDefault, newv);
}

struct _RSArrayCompareObjectContext {
    RSComparatorFunction func;
    void *context;
};

static RSComparisonResult __RSArrayCompareObjects(const void *v1, const void *v2, void *context)
{
    const void **val1 = (const void **)v1;
    const void **val2 = (const void **)v2;
    struct _RSArrayCompareObjectContext *_ctx = (struct _RSArrayCompareObjectContext *)context;
    return (RSComparisonResult)(INVOKE_CALLBACK3(_ctx->func, *val1, *val2, _ctx->context));
}

RSInline void __RSZSort(RSMutableArrayRef array, RSRange range, RSComparatorFunction comparator, RSComparisonOrder order, void *context)
{
    RSIndex cnt = range.length;
#if RS_BLOCKS_AVAILABLE
    BOOL (^forward)(RSComparisonResult result) = nil;
    forward = order == RSOrderedAscending ? ^BOOL (RSComparisonResult result) {
        return result > 0 ? YES : NO;
    } : ^BOOL (RSComparisonResult result) {
        return result < 0 ? YES : NO;
    };
#else
    BOOL (*forward)(RSComparisonResult result) = nil;
    forward = order == RSOrderedAscending ? ___RSZSortComparisonOver0ReturnYes : ___RSZSortComparisonBelow0ReturnYes;
#endif
    while (1 < cnt)
    {
        for (RSIndex idx = range.location; idx < range.location + cnt - 1; idx++)
        {
            const void *a = RSArrayObjectAtIndex(array, idx);
            const void *b = RSArrayObjectAtIndex(array, idx + 1);
            if (forward((RSComparisonResult)(INVOKE_CALLBACK3(comparator, b, a, context))))
            {
                RSArrayExchangeObjectsAtIndices(array, idx, idx + 1);
            }
        }
        cnt--;
    }
}


RSExport void RSArraySort(RSArrayRef array, RSComparisonOrder order, RSComparatorFunction comparator, void *context)
{
    FAULT_CALLBACK((void **)&(comparator));
    RSRange range = RSMakeRange(0, RSArrayGetCount(array));
    __RSArrayValidateRange(array, range, __PRETTY_FUNCTION__);
    
    BOOL immutable = NO;
    if (NO && RS_IS_OBJC(__RSArrayTypeID, array))
    {
        BOOL result;
        //result = RS_OBJC_CALLV((NSMutableArray *)array, isKindOfClass:[NSMutableArray class]);
        immutable = !result;
    }
    else if (__RSArrayImmutable == __RSArrayGetType(array))
    {
        immutable = YES;
    }
    const RSArrayCallBacks *cb = nil;
    if (RS_IS_OBJC(__RSArrayTypeID, array))
    {
        cb = &RSTypeArrayCallBacks;
    }
    else
    {
        cb = __RSArrayGetCallBacks(array);
    }
    if (!immutable && ((cb->retain && !cb->release) || (!cb->retain && cb->release)))
    {
        __RSZSort((RSMutableArrayRef)array, range, comparator, order, nil);
        return;
    }
    if (range.length < 2)
    {
        return;
    }
    // implemented abstractly, careful!
    const void **values, *buffer[256];
    values = (range.length <= 256) ? (const void **)buffer : (const void **)RSAllocatorAllocate(RSAllocatorSystemDefault, range.length * sizeof(void *)); // GC OK
    RSArrayGetObjects(array, range, values);
    struct _RSArrayCompareObjectContext ctx;
    ctx.func = comparator;
    ctx.context = context;
    RSSortArray(values, range.length, sizeof(void *), order, (RSComparatorFunction)__RSArrayCompareObjects, &ctx);
    if (!immutable) RSArrayReplaceObject((RSMutableArrayRef)array, range, values, range.length);
    if (values != buffer) RSAllocatorDeallocate(RSAllocatorSystemDefault, values);
}

RSExport void RSArraySortUsingComparaotr(RSArrayRef array, RSComparisonResult (^comparator)(const void *val1, const void *val2)) {
    
}

RSIndex RSArrayBSearchObjects(RSArrayRef array, RSRange range, const void *value, RSComparatorFunction comparator, void *context)
{
    FAULT_CALLBACK((void **)&(comparator));
    __RSArrayValidateRange(array, range, __PRETTY_FUNCTION__);
    // implemented abstractly, careful!
    if (range.length <= 0) return range.location;
    const void *item = RSArrayObjectAtIndex(array, range.location + range.length - 1);
    if ((RSComparisonResult)(INVOKE_CALLBACK3(comparator, item, value, context)) < 0)
    {
        return range.location + range.length;
    }
    item = RSArrayObjectAtIndex(array, range.location);
    if ((RSComparisonResult)(INVOKE_CALLBACK3(comparator, value, item, context)) < 0)
    {
        return range.location;
    }
    SInt32 lg = flsl(range.length) - 1;	// lg2(range.length)
    item = RSArrayObjectAtIndex(array, range.location + -1 + (1 << lg));
    // idx will be the current probe index into the range
    RSIndex idx = (comparator(item, value, context) < 0) ? range.length - (1 << lg) : -1;
    while (lg--)
    {
        item = RSArrayObjectAtIndex(array, range.location + idx + (1 << lg));
        if (comparator(item, value, context) < 0)
        {
            idx += (1 << lg);
        }
    }
    idx++;
    return idx + range.location;
}

RSIndex RSArrayIndexOfObject(RSArrayRef array, RSTypeRef object)
{
    return RSArrayGetFirstIndexOfObject(array, RSMakeRange(0, RSArrayGetCount(array)), object);
}

RSPrivate RSArrayRef __RSArrayCreateWithArguments(va_list arguments, RSInteger maxCount) {
    RSTypeRef key = va_arg(arguments, RSTypeRef);
    RSMutableArrayRef keys = key ? RSArrayCreateMutable(RSAllocatorSystemDefault, 0) : nil;
    RSUInteger idx = 0;
    for (; key && idx < maxCount; idx++) {
        RSArrayAddObject(keys, key);
        key = va_arg(arguments, RSTypeRef);
    }
    return keys;
}
#endif