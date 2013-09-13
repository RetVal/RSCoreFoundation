//
//  RSBaseHeap.c
//  RSCoreFoundation
//
//  Created by RetVal on 11/12/12.
//  Copyright (c) 2012 RetVal. All rights reserved.
//

#include <stdio.h>

#include <RSCoreFoundation/RSString.h>
#include <RSCoreFoundation/RSNumber.h>
#include <RSCoreFoundation/RSBaseHeap.h>
#include <RSCoreFoundation/RSNil.h>
#include <RSCoreFoundation/RSRuntime.h>
/*
 1)root position is 0
 2)the position of the left child of node i is 2*i + 1
 3)the position of the right child of node i is 2*i +2
 4)the parent of node i floor((i-1)/2)
 */

struct __RSBaseHeap {
    RSRuntimeBase _base;
    RSIndex _count;
    RSIndex _limit;
    RSBaseHeapCallbacks _callbacks;
    RSTypeRef* _heap;
};
const static RSBaseHeapCallbacks __RSBaseHeapRSStringRefCallBacks = {
    0,
    RSRetain,
    RSRelease,
    RSDescription,
    (RSBaseHeapCompareCallBack)RSStringCompare,
};

const RSBaseHeapCallbacks* const RSBaseHeapRSStringRefCallBacks = &__RSBaseHeapRSStringRefCallBacks;

const static RSBaseHeapCallbacks __RSBaseHeapRSNumberRefCallBacks = {
    0,
    RSRetain,
    RSRelease,
    RSDescription,
    (RSBaseHeapCompareCallBack)RSNumberCompare,
};

const RSBaseHeapCallbacks* const RSBaseHeapRSNumberRefCallBacks = &__RSBaseHeapRSNumberRefCallBacks;
/*
 0: max / min heap.
 
 */
enum
{
    _RSHeapStrongMemory = 1 << 0L,
    _RSHeapMaxheap = 1 << 1L,
    _RSHeapConstantLimit = 1 << 2L,
    _RSHeapStrongMemoryMask = _RSHeapStrongMemory,
    _RSHeapMaxheapMask = _RSHeapMaxheap,
    _RSHeapConstantLimitMask = _RSHeapConstantLimit,
};

RSInline BOOL isStrong(RSBaseHeapRef heap)
{
    return (heap->_base._rsinfo._rsinfo1 & _RSHeapStrongMemoryMask) == _RSHeapStrongMemory;
}
RSInline void markStrong(RSBaseHeapRef heap)
{
    ((struct __RSBaseHeap*)heap)->_base._rsinfo._rsinfo1 |= _RSHeapStrongMemory;
}
RSInline void markWeak(RSBaseHeapRef heap)
{
    ((struct __RSBaseHeap*)heap)->_base._rsinfo._rsinfo1 &= ~_RSHeapStrongMemory;
}

RSInline BOOL isMaxheap(RSBaseHeapRef heap)
{
    return (heap->_base._rsinfo._rsinfo1 & _RSHeapMaxheapMask) == _RSHeapMaxheap;
}

RSInline BOOL isMinheap(RSBaseHeapRef heap)
{
    return (heap->_base._rsinfo._rsinfo1 & _RSHeapMaxheapMask) != _RSHeapMaxheap;
}

RSInline void markMaxHeap(RSBaseHeapRef heap)
{
    ((struct __RSBaseHeap*)heap)->_base._rsinfo._rsinfo1 |= _RSHeapMaxheap;
}

RSInline void markMinHeap(RSBaseHeapRef heap)
{
    ((struct __RSBaseHeap*)heap)->_base._rsinfo._rsinfo1 &= ~_RSHeapMaxheap;
}

RSInline BOOL isConstantLimit(RSBaseHeapRef heap)
{
    return ((heap->_base._rsinfo._rsinfo1 & _RSHeapConstantLimitMask) == _RSHeapConstantLimit);
}

RSInline void markConstantLimit(RSBaseHeapRef heap)
{
    (((struct __RSBaseHeap*)heap)->_base._rsinfo._rsinfo1 |= _RSHeapConstantLimit);
}

RSInline void unMarkConstantLimit(RSBaseHeapRef heap)
{
    (((struct __RSBaseHeap*)heap)->_base._rsinfo._rsinfo1 &= ~_RSHeapConstantLimit);
}

RSInline RSIndex __RSBaseHeapLimit(RSBaseHeapRef heap)
{
    return heap->_limit;
}

RSInline void   __RSBaseHeapSetLimit(RSBaseHeapRef heap, RSIndex capacity)
{
    ((struct __RSBaseHeap*)heap)->_limit = capacity;
}

RSInline RSIndex __RSBaseHeapCount(RSBaseHeapRef heap)
{
    return heap->_count;
}

RSInline void   __RSBaseHeapSetCount(RSBaseHeapRef heap, RSIndex count)
{
    ((struct __RSBaseHeap*)heap)->_count = count;
}

RSInline RSBaseHeapCallbacks* __RSBaseHeapCallbacks(RSBaseHeapRef heap)
{
    return (RSBaseHeapCallbacks*)&heap->_callbacks;
}

RSInline void   __RSBaseHeapSetCallbacks(RSBaseHeapRef heap, const RSBaseHeapCallbacks* callbacks)
{
    memcpy(&((struct __RSBaseHeap*)heap)->_callbacks, callbacks, sizeof(RSBaseHeapCallbacks));
    //((struct __RSBaseHeap*)heap)->_callbacks = (RSBaseHeapCallbacks*)callbacks;
}

RSInline RSTypeRef* __RSBaseHeapStore(RSBaseHeapRef heap)
{
    return heap->_heap;
}

RSInline void __RSBaseHeapSetStore(RSBaseHeapRef heap, RSTypeRef* store)
{
    ((struct __RSBaseHeap*)heap)->_heap = store;
}

static void __RSBaseHeapClassInit(RSTypeRef obj)
{
    RSBaseHeapRef heap = (RSBaseHeapRef)obj;
    markStrong(heap);
    markConstantLimit(heap);
    return;
}

static void __RSBaseHeapClassDeallocate(RSTypeRef obj)
{
    RSBaseHeapRef heap = (RSBaseHeapRef)obj;
    RSIndex count = __RSBaseHeapCount(heap);
    RSTypeRef* table = __RSBaseHeapStore(heap);
    for (RSIndex idx = 0; idx < count; idx++)
    {
        __RSBaseHeapCallbacks(heap)->release(table[idx]);
    }
    
    if (isStrong(heap))
    {
        RSAllocatorDeallocate(RSAllocatorSystemDefault, table);
    }
}

static RSStringRef __RSBaseHeapClassDescription(RSTypeRef obj)
{
    RSBaseHeapRef heap = (RSBaseHeapRef)obj;
    RSIndex cnt = __RSBaseHeapCount(heap);
    RSTypeRef* table = __RSBaseHeapStore(heap);
    RSMutableStringRef description = RSStringCreateMutable(RSAllocatorSystemDefault, 32*cnt);
    RSStringAppendCString(description, "{");
    for (RSIndex idx = 0; idx < cnt; idx++)
    {
        RSStringRef item = RSDescription(table[idx]);
        RSStringAppendCString(description, "\t");
        RSStringAppendString(description, item);
        RSStringAppendCString(description, "\n");
        RSRelease(item);
    }
    RSStringAppendCString(description, "}");
    return description;
}

struct __RSRuntimeClass __RSBaseHeapClass = {
    _RSRuntimeScannedObject,
    "RSBaseHeap",
    __RSBaseHeapClassInit,
    NULL,
    __RSBaseHeapClassDeallocate,
    NULL,
    NULL,
    __RSBaseHeapClassDescription,
    NULL,
    NULL,
};

static RSIndex __RSBaseHeapTypeID = _RSRuntimeNotATypeID;

RSTypeID RSBaseHeapGetTypeID()
{
    return __RSBaseHeapTypeID;
}


static void __RSBaseHeapAvailable(RSBaseHeapRef heap)
{
    if (heap == nil) HALTWithError(RSInvalidArgumentException, "the heap is nil");
    __RSGenericValidInstance(heap, __RSBaseHeapTypeID);
    //if (!isConstantLimit(heap)) HALTWithError(RSGenericException, "not a constant capacity heap");
    return;
}


RSPrivate void __RSBaseHeapInitialize()
{
    __RSBaseHeapTypeID = __RSRuntimeRegisterClass(&__RSBaseHeapClass);
    __RSRuntimeSetClassTypeID(&__RSBaseHeapClass, __RSBaseHeapTypeID);
}
static RSTypeRef* __RSBaseHeapCreateTable(RSAllocatorRef allocator, RSIndex capacity)
{
    RSTypeRef* table = nil;
    table = RSAllocatorAllocate(allocator, capacity * sizeof(ISA));
    return table;
}

static void __RSTypeSwap(RSTypeRef* a, RSTypeRef* b)
{
    RSTypeRef t = *a;
    *a = *b;
    *b = t;
}

static void __RSBaseHeapCheckCallbacks(const RSBaseHeapCallbacks* cbs)
{
    if (cbs == nil) HALTWithError(RSInvalidArgumentException, "RSBaseHeapCallbacks is nil");
    if (cbs->vs != 0) HALTWithError(RSInvalidArgumentException, "RSBaseHeapCallbacks is not initialize");
    if (cbs->retain == nil) HALTWithError(RSInvalidArgumentException, "RSBaseHeapCallbacks' retain is nil");
    if (cbs->release == nil) HALTWithError(RSInvalidArgumentException, "RSBaseHeapCallbacks' release is nil");
    if (cbs->description == nil) HALTWithError(RSInvalidArgumentException, "RSBaseHeapCallbacks's description is nil");
    if (cbs->comparator == nil) HALTWithError(RSInvalidArgumentException, "RSBaseHeapCallbacks's comparator is nil");
}

/*
 * Max-Heapify
 *
 * l <-- LEFT(i)
 * r <-- RIGHT(i)
 * if(l <= heap-size[A] and A[l] > A[i])
 *  then largest <-- l
 *  else largest <-- i
 * if(r <= heap-size[A] and A[r] < A[largest])
 *  then largest <-- r
 * if largest != i
 *  then exchange A[i] <--> A[largest]
 *       Max-Heapify(A,largest)
 */
static void __RSBaseHeapMaxHeapify(RSTypeRef arraySort[], RSIndex heapSize, RSIndex i, RSBaseHeapCompareCallBack comparator)
{
	RSIndex l = 2*i+1;
	RSIndex r = 2*i+2;
	RSIndex largest = 0;
    
	if (l < heapSize && (RSCompareGreaterThan == comparator(arraySort[l], arraySort[i])))
	{
		largest = l;
	}
	else largest = i;
    
	if (r < heapSize && (RSCompareGreaterThan == comparator(arraySort[r], arraySort[largest])))
	{
		largest = r;
	}
    
	if (largest != i)
	{
		__RSTypeSwap(&arraySort[i], &arraySort[largest]);
		__RSBaseHeapMaxHeapify(arraySort, heapSize, largest, comparator);
	}
}

void __RSBaseHeapMaxSort(RSTypeRef arraySort[],RSIndex arrayLen, RSBaseHeapCompareCallBack comparator)
{
	//build max heap
	RSIndex i = arrayLen / 2;
	RSIndex heapSize = arrayLen;
    
	for ( ; i >= 0 ; i--)
	{
		__RSBaseHeapMaxHeapify(arraySort, heapSize, i, comparator);
	}
    
	//sort
    return;
	for (i = arrayLen - 1 ; i >= 1 ; i--)
	{
		__RSTypeSwap(&arraySort[0], &arraySort[i]);
		heapSize--;
		__RSBaseHeapMaxHeapify(arraySort, heapSize, 0, comparator);
	}
}

static RSMutableBaseHeapRef __RSBaseHeapCreateInstance(RSAllocatorRef allocator, RSIndex capacity, const RSBaseHeapCallbacks* callbacks, BOOL maxHeap, BOOL constantLimit)
{
    __RSBaseHeapCheckCallbacks(callbacks);
    if (allocator == nil) allocator = RSAllocatorSystemDefault;
    struct __RSBaseHeap* heap = (struct __RSBaseHeap*)__RSRuntimeCreateInstance(allocator, __RSBaseHeapTypeID, sizeof(struct __RSBaseHeap) - sizeof(struct __RSRuntimeBase));
    //capacity = RSAllocatorSize(allocator, capacity);
    RSIndex limit = capacity;
    //capacity *= sizeof(ISA);
    __RSBaseHeapSetStore(heap, __RSBaseHeapCreateTable(RSAllocatorSystemDefault, capacity));
    __RSBaseHeapSetCount(heap, 0);
    __RSBaseHeapSetLimit(heap, limit);
    __RSBaseHeapSetCallbacks(heap, callbacks);
    (maxHeap) ? markMaxHeap(heap) : markMinHeap(heap);
    (constantLimit) ? markConstantLimit(heap) : unMarkConstantLimit(heap);
    return heap;
}

RSExport RSBaseHeapRef RSBaseHeapCreateWithArray(RSAllocatorRef allocator, RSArrayRef objects, RSBaseHeapCompareCallBack comparator, BOOL maxRootHeap)
{
    RSBaseHeapCallbacks cbs = {0, RSRetain, RSRelease, RSDescription, comparator};
    RSIndex capacity = RSArrayGetCount(objects);
    RSBaseHeapRef bh = __RSBaseHeapCreateInstance(allocator, capacity, &cbs, maxRootHeap, YES);
    RSTypeRef* heap = __RSBaseHeapStore(bh);
    for (RSIndex idx = 0; idx < capacity; idx++)
    {
        heap[idx] = RSRetain(RSArrayObjectAtIndex(objects, idx)); // return with not retain
    }
    __RSBaseHeapSetCount(bh, capacity);
    if (maxRootHeap)
        __RSBaseHeapMaxSort(heap, capacity, comparator);
    else {};
    return bh;
}

RSExport RSMutableBaseHeapRef RSBaseHeapCreateMutable(RSAllocatorRef allocator, RSIndex capacity, const RSBaseHeapCallbacks* callbacks, BOOL maxRootHeap)
{
    RSMutableBaseHeapRef bh = __RSBaseHeapCreateInstance(allocator, capacity, callbacks, maxRootHeap, NO);
    return bh;
}


RSExport void RSBaseHeapSetCallBacks(RSBaseHeapRef heap, RSBaseHeapCallbacks* callbacks)
{
    __RSBaseHeapAvailable(heap);
    __RSBaseHeapSetCallbacks(heap, callbacks);
}

RSExport RSBaseHeapCallbacks* RSBaseHeapGetCallBacks(RSBaseHeapRef heap)
{
    __RSBaseHeapAvailable(heap);
    return __RSBaseHeapCallbacks(heap);
}

RSExport RSIndex RSBaseHeapGetCount(RSBaseHeapRef heap)
{
    __RSBaseHeapAvailable(heap);
    return __RSBaseHeapCount(heap);
}

static void __RSBaseHeapAddObject(RSBaseHeapRef heap, RSTypeRef obj)
{
    RSIndex count = __RSBaseHeapCount(heap);
    RSIndex limit = __RSBaseHeapLimit(heap);
    RSTypeRef* __heap = __RSBaseHeapStore(heap);
    if (__heap == nil || obj == nil) return;
    if (isConstantLimit(heap))
    {
        if (limit <= count)
        {
            __heap[0] = obj;
            if (isMaxheap(heap))
            {
                __RSBaseHeapCallbacks(heap)->retain(obj);
                //__RSBaseHeapMaxHeapify(__heap, limit, 0, __RSBaseHeapCallbacks(heap)->comparator);
                __RSBaseHeapMaxSort(__heap, limit, __RSBaseHeapCallbacks(heap)->comparator);
            }
            else
            {
                __RSBaseHeapCallbacks(heap)->retain(obj);
            }
        }
        else
        {
            __heap[count] = obj;
            count++;
            if (isMaxheap(heap))
            {
                __RSBaseHeapCallbacks(heap)->retain(obj);
                //__RSBaseHeapMaxHeapify(__heap, count, count-1, __RSBaseHeapCallbacks(heap)->comparator);
                __RSBaseHeapMaxSort(__heap, count, __RSBaseHeapCallbacks(heap)->comparator);
            }
            else
            {
                __RSBaseHeapCallbacks(heap)->retain(obj);
            }
            __RSBaseHeapSetCount(heap, count);
        }
    }
    else
    {
        if (limit <= count)
        {
            __heap = RSAllocatorReallocate(RSAllocatorSystemDefault, __heap, sizeof(ISA) * (count + 1));
            __RSBaseHeapSetStore(heap, __heap);
            __RSBaseHeapSetLimit(heap, count+1);
            __heap[count] = obj;
            ++count;
            if (isMaxheap(heap)) {
                __RSBaseHeapCallbacks(heap)->retain(obj);
                //__RSBaseHeapMaxHeapify(__heap, count, count-1, __RSBaseHeapCallbacks(heap)->comparator);
                __RSBaseHeapMaxSort(__heap, count, __RSBaseHeapCallbacks(heap)->comparator);
            }
            else {
                __RSBaseHeapCallbacks(heap)->retain(obj);
            }
        }
        __RSBaseHeapSetCount(heap, count);
    }
}

static void __RSBaseHeapRemoveObject(RSBaseHeapRef heap, RSTypeRef obj)
{
    
}

RSExport void RSBaseHeapAddObject(RSBaseHeapRef heap, RSTypeRef obj)
{
    __RSBaseHeapAvailable(heap);
    if (obj == nil) return;
    __RSBaseHeapAddObject(heap, obj);
    return;
}

RSExport void RSBaseHeapRemoveObject(RSBaseHeapRef heap, RSTypeRef obj)
{
    __RSBaseHeapAvailable(heap);
    if (obj == nil) return;
    
    return;
}