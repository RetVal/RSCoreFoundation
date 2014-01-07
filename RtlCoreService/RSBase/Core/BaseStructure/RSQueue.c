//
//  RSQueue.c
//  RSCoreFoundation
//
//  Created by RetVal on 3/8/13.
//  Copyright (c) 2013 RetVal. All rights reserved.
//

#include <RSCoreFoundation/RSQueue.h>
#include <RSCoreFoundation/RSArray.h>

#include <RSCoreFoundation/RSRuntime.h>

struct __RSQueue
{
    RSRuntimeBase _base;
    RSSpinLock _lock;
    RSIndex _capacity;
    RSMutableArrayRef _queueCore;
};

static RSTypeRef __RSQueueClassCopy(RSAllocatorRef allocator, RSTypeRef rs, BOOL mutableCopy)
{
    return nil;
}

static void __RSQueueClassDeallocate(RSTypeRef rs)
{
    RSQueueRef queue = (RSQueueRef)rs;
    if (queue->_queueCore) RSRelease(queue->_queueCore);
    queue->_queueCore = nil;
    return;
}

static BOOL __RSQueueClassEqual(RSTypeRef rs1, RSTypeRef rs2)
{
    return RSEqual(((RSQueueRef)rs1)->_queueCore, ((RSQueueRef)rs2)->_queueCore);
}

static RSHashCode __RSQueueClassHash(RSTypeRef rs)
{
    return RSHash(((RSQueueRef)rs)->_queueCore);
}

static RSStringRef __RSQueueClassDescription(RSTypeRef rs)
{
    RSQueueRef queue = (RSQueueRef)rs;
    RSMutableStringRef result;
    RSAllocatorRef allocator;
    RSIndex idx, cnt;
    cnt = RSQueueGetCount(queue);
    allocator = RSGetAllocator(queue);
    result = RSStringCreateMutable(allocator, 0);
    RSStringAppendStringWithFormat(result, RSSTR("<RSQueue %p [%p]>{type = mutable-small, count = %u, values = (%s"), rs, allocator, cnt, cnt ? "\n" : "");
    for (idx = 0; idx < cnt; idx++)
    {
        RSStringRef desc = nil;
        
        RSTypeRef val = nil;
        desc = (RSStringRef)INVOKE_CALLBACK1(RSDescription, val = RSArrayObjectAtIndex(queue->_queueCore, idx));
        if (nil != desc)
        {
            RSStringAppendStringWithFormat(result, RSSTR("\t%u : %R\n"), idx, desc);
            RSRelease(desc);
        } else {
            RSStringAppendStringWithFormat(result, RSSTR("\t%u : <%p>\n"), idx, val);
        }
    }
    RSStringAppendString(result, RSSTR(")}"));
    return result;
}

static RSRuntimeClass __RSQueueClass =
{
    _RSRuntimeScannedObject,
    "RSQueue",
    nil,
    __RSQueueClassCopy,
    __RSQueueClassDeallocate,
    __RSQueueClassEqual,
    __RSQueueClassHash,
    __RSQueueClassDescription,
    nil,
    nil
};

static RSTypeID _RSQueueTypeID = _RSRuntimeNotATypeID;

RSExport RSTypeID RSQueueGetTypeID()
{
    return _RSQueueTypeID;
}

RSPrivate void __RSQueueInitialize()
{
    _RSQueueTypeID = __RSRuntimeRegisterClass(&__RSQueueClass);
    __RSRuntimeSetClassTypeID(&__RSQueueClass, _RSQueueTypeID);
}

RSInline BOOL isAtom(RSQueueRef queue)
{
	return __RSBitfieldGetValue(RSRuntimeClassBaseFiled(queue), 1, 1);
}

RSInline void setAtom(RSQueueRef queue)
{
	__RSBitfieldSetValue(RSRuntimeClassBaseFiled(queue), 1, 1, 1);
}

RSInline void unsetAtom(RSQueueRef queue)
{
	__RSBitfieldSetValue(RSRuntimeClassBaseFiled(queue), 1, 1, 0);
}

static RSQueueRef __RSQueueCreateInstance(RSAllocatorRef allocator, RSIndex capacity, RSQueueAtomType atom)
{
    RSQueueRef queue = (RSQueueRef)__RSRuntimeCreateInstance(allocator, _RSQueueTypeID, sizeof(struct __RSQueue) - sizeof(RSRuntimeBase));
    queue->_lock = RSSpinLockInit;
	if (atom == RSQueueAtom) setAtom(queue);
    queue->_capacity = capacity;
    queue->_queueCore = RSArrayCreateMutable(allocator, capacity);
    return queue;
}

RSInline void __RSQueueAddToObject(RSQueueRef queue, RSTypeRef obj)
{
    BOOL shouldLocked = isAtom(queue);
    if (shouldLocked) RSSpinLockLock(&queue->_lock);
    if (queue->_capacity == 0)
    {
        RSArrayAddObject(queue->_queueCore, obj);
        if (shouldLocked) RSSpinLockUnlock(&queue->_lock);
        return;
    }
    RSIndex cnt = RSArrayGetCount(queue->_queueCore);
    if (cnt < queue->_capacity)
        RSArrayAddObject(queue->_queueCore, obj);
    else
        __RSCLog(RSLogLevelNotice, "RSQueue %p is full!\n", queue); 
    if (shouldLocked) RSSpinLockUnlock(&queue->_lock);
}

RSInline RSTypeRef __RSQueueGetObjectFromQueue(RSQueueRef queue, BOOL remove)
{
    BOOL shouldLocked = isAtom(queue);
    if (shouldLocked) RSSpinLockLock(&queue->_lock);
    RSIndex cnt = RSArrayGetCount(queue->_queueCore);
    if (cnt == 0)
    {
        if (shouldLocked) RSSpinLockUnlock(&queue->_lock);
        return nil;
    }
    RSTypeRef obj = RSArrayObjectAtIndex(queue->_queueCore, 0);
    if (remove)
    {
        RSRetain(obj);
        RSArrayRemoveObjectAtIndex(queue->_queueCore, 0);
    }
    if (shouldLocked) RSSpinLockUnlock(&queue->_lock);
    return obj;
}

RSExport RSQueueRef RSQueueCreate(RSAllocatorRef allocator, RSIndex capacity, RSQueueAtomType atom)
{
    return __RSQueueCreateInstance(allocator, capacity, atom);
}

RSExport RSTypeRef RSQueueDequeue(RSQueueRef queue)
{
    if (queue == nil) return nil;
    __RSGenericValidInstance(queue, _RSQueueTypeID);
    return __RSQueueGetObjectFromQueue(queue, YES);
}

RSExport void RSQueueEnqueue(RSQueueRef queue, RSTypeRef object)
{
    if (queue == nil || object == nil) return;
    __RSGenericValidInstance(queue, _RSQueueTypeID);
    __RSQueueAddToObject(queue, object);
}

RSExport RSTypeRef RSQueuePeekObject(RSQueueRef queue)
{
    if (queue == nil) return nil;
    __RSGenericValidInstance(queue, _RSQueueTypeID);
    return __RSQueueGetObjectFromQueue(queue, NO);
}

RSExport RSIndex RSQueueGetCapacity(RSQueueRef queue)
{
    if (queue == nil) return 0;
    __RSGenericValidInstance(queue, _RSQueueTypeID);
    return  queue->_capacity;
}

RSExport RSIndex RSQueueGetCount(RSQueueRef queue)
{
    if (queue == nil) return 0;
    __RSGenericValidInstance(queue, _RSQueueTypeID);
    if (isAtom(queue)) RSSpinLockLock(&queue->_lock);
    RSIndex cnt = RSArrayGetCount(queue->_queueCore);
    if (isAtom(queue)) RSSpinLockUnlock(&queue->_lock);
    return cnt;
}

RSExport RSArrayRef RSQueueCopyCoreQueueUnsafe(RSQueueRef queue) {
    if (!queue) return nil;
    __RSGenericValidInstance(queue, _RSQueueTypeID);
    if (isAtom(queue)) RSSpinLockLock(&queue->_lock);
    RSArrayRef results = RSRetain(queue->_queueCore);
    if (isAtom(queue)) RSSpinLockUnlock(&queue->_lock);
    return results;
}

RSExport RSArrayRef RSQueueCopyCoreQueueSafe(RSQueueRef queue) {
    if (!queue) return nil;
    __RSGenericValidInstance(queue, _RSQueueTypeID);
    if (isAtom(queue)) RSSpinLockLock(&queue->_lock);
    RSArrayRef results = RSCopy(RSAllocatorSystemDefault, queue->_queueCore);
    if (isAtom(queue)) RSSpinLockUnlock(&queue->_lock);
    return results;
}
