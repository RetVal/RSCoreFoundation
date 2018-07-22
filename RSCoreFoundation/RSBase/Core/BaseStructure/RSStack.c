//
//  RSStack.c
//  RSKit
//
//  Created by RetVal on 12/30/12.
//  Copyright (c) 2012 RetVal. All rights reserved.
//

#include <stdio.h>
#include <RSKit/RSStack.h>
#include <RSKit/RSRuntime.h>
#undef __RSStackIndexInit   
#define __RSStackIndexInit ((RSIndex)-1)
struct __RSStack
{
    RSRuntimeBase _base;
    RSStackContext _context;
    RSIndex _idx;
    RSIndex _cnt;
    RSTypeRef _stackG[0];
    RSTypeRef* _stack[0];
};

RSInline BOOL __RSStackIsEmpty(RSStackRef stack)
{
    return __RSBitfieldGetValue(RSRuntimeClassBaseFiled(stack), 1, 1);
}

RSInline void __RSStackSetEmpty(RSStackRef stack)
{
    __RSBitfieldSetValue(RSRuntimeClassBaseFiled(stack), 1, 1, 1);
}

RSInline void __RSStackUnsetEmpty(RSStackRef stack)
{
    __RSBitfieldSetValue(RSRuntimeClassBaseFiled(stack), 1, 1, 0);
}

RSInline BOOL __RSStackIsGrowing(RSStackRef stack)
{
    return __RSBitfieldGetValue(RSRuntimeClassBaseFiled(stack), 2, 2);
}

RSInline void __RSStackSetGrowing(RSStackRef stack)
{
    __RSBitfieldSetValue(RSRuntimeClassBaseFiled(stack), 2, 2, 1);
}

RSInline void __RSStackUnsetGrowing(RSStackRef stack)
{
    __RSBitfieldSetValue(RSRuntimeClassBaseFiled(stack), 2, 2, 0);
}

RSInline void __RSStackSetContext(RSStackRef stack, RSStackContext* context)
{
    memcpy(&stack->_context, context, sizeof(RSStackContext));
}

RSInline RSStackContext* __RSStackGetContext(RSStackRef stack)
{
    return &stack->_context;
}

RSInline RSTypeRef* __RSStackGetRealStack(RSStackRef stack)
{
    return __RSStackIsGrowing(stack) ? stack->_stack[0] : &stack->_stackG[0];
}

static void __RSStackClassInit(RSTypeRef rs)
{
    RSStackRef stack = (RSStackRef)rs;
    stack->_idx = __RSStackIndexInit;
    stack->_cnt = 0;
    stack->_stack[0] = stack->_stack[1] = nil;
}

static void __RSStackDrain(RSStackRef stack);
static void __RSStackClassDeallocate(RSTypeRef rs)
{
    RSStackRef stack = (RSStackRef)rs;
    if (__RSStackIsGrowing(stack))
    {
        RSTypeRef* _s = __RSStackGetRealStack(stack);
        __RSStackDrain(stack);
        RSAllocatorDeallocate(kRSAllocatorSystemDefault, (RSZone)&_s);
    }
}

static BOOL __RSStackClassEqual(RSTypeRef rs1, RSTypeRef rs2)
{
    RSStackRef stack1 = (RSStackRef)rs1;
    RSStackRef stack2 = (RSStackRef)rs2;
    if (__RSStackIsGrowing(stack1) != __RSStackIsGrowing(stack2)) return NO;
    if (__RSStackIsEmpty(stack1) != __RSStackIsEmpty(stack2)) return NO;
    if (stack1->_cnt != stack2->_cnt) return NO;
    if (stack1->_idx != stack2->_idx) return NO;
    if (0 != memcpy(&stack1->_context, &stack2->_context, sizeof(RSStackContext))) return NO;
    RSTypeRef* st1 = __RSStackGetRealStack(stack1);
    RSTypeRef* st2 = __RSStackGetRealStack(stack2);
    if (st1 == st2) return YES;
    if (st1 && st2)
    {
        for (RSIndex idx = 0; idx < stack1->_idx; idx++)
        {
            if (NO == RSEqual(st1[idx], st2[idx]))
                return NO;
        }
        return YES;
    }
    return NO;
}

static RSHashCode __RSStackClassHash(RSTypeRef rs)
{
    RSStackRef stack = (RSStackRef)rs;
    return stack->_cnt^stack->_idx;
}

static RSStringRef __RSStackClassDescription(RSTypeRef rs)
{
    RSStackRef stack = (RSStackRef)rs;
#if __LP64__
    RSStringRef description = RSStringCreateWithFormat(kRSAllocatorDefault, RSSTR("RSStack <%p> - (%p, %lld, %lld)"), stack, __RSStackGetRealStack(stack), stack->_idx, stack->_cnt);
#else
    RSStringRef description = RSStringCreateWithFormat(kRSAllocatorDefault, RSSTR("RSStack <%p> - (%p, %ld, %ld)"), stack, __RSStackGetRealStack(stack), stack->_idx, stack->_cnt);
#endif
    return description;
}

static const RSRuntimeClass __RSStackClass = {
    _kRSRuntimeScannedObject,
    "RSStack",
    __RSStackClassInit,
    nil,
    __RSStackClassDeallocate,
    __RSStackClassEqual,
    __RSStackClassHash,
    __RSStackClassDescription,
    nil,
    nil,
};

static RSTypeID _kRSStackTypeID = _kRSRuntimeNotATypeID;

static void __RSStackInitialize()
{
    _kRSStackTypeID = __RSRuntimeRegisterClass(&__RSStackClass);
    __RSRuntimeSetClassTypeID(&__RSStackClass, _kRSStackTypeID);
}

RSExport RSTypeID RSStackGetTypeID()
{
    if (unlikely(_kRSStackTypeID == _kRSRuntimeNotATypeID))
        __RSStackInitialize();
    return _kRSStackTypeID;
}

static RSComparisonResult __RSStackCompare(RSTypeRef rs1, RSTypeRef rs2)
{
    RSStackRef stack1 = (RSStackRef)rs1;
    RSStackRef stack2 = (RSStackRef)rs2;
    if (stack1 == nil || stack2 == nil) HALTWithError("stack is nil.");
    if (stack1->_cnt + stack1->_idx > stack2->_cnt + stack2->_idx)
        return kRSCompareGreaterThan;
    else if (stack1->_cnt + stack1->_idx < stack2->_cnt + stack2->_idx)
        return kRSCompareLessThan;
    return kRSCompareEqualTo;
}

static RSStackContext __kRSStackRSTypeContext = {
    0,
    RSRetain,
    RSRelease,
    RSDescription,
    __RSStackCompare,
    RSHash,
};

const RSStackContext* kRSStackRSTypeContext = &__kRSStackRSTypeContext;

static void __RSStackEjectTop(RSStackRef stack)
{
    if (stack->_idx == __RSStackIndexInit) return;
    RSTypeRef* bucket = __RSStackGetRealStack(stack);
    if (bucket)
    {
        if (stack->_context.release)
        {
            stack->_context.release(bucket[stack->_idx--]);
        }
        bucket[stack->_idx + 1] = nil;
    }
}


static void __RSStackDrain(RSStackRef stack)
{
    RSTypeRef* st = __RSStackGetRealStack(stack);
    if (st == nil) return;
    if (likely(stack->_context.release != nil))
    {
        for (RSIndex idx = 0; idx < stack->_idx; idx++)
        {
            RSTypeRef item = st[idx];
            if (likely(item != nil)) stack->_context.release(item);
            st[idx] = nil;
        }
        stack->_idx = 0;
        return;
    }
    for (RSIndex idx = 0; idx < stack->_idx; idx++)
    {
        RSTypeRef item = st[idx];
        if (likely(item != nil)) st[idx] = nil;
    }
    stack->_idx = 0;
    return;
}

static void __RSStackCreateGrowingStack(RSStackRef stack, RSIndex newCount)
{
    if (__RSStackIsGrowing(stack) == NO) return;
    if (stack->_stack[0] == nil)
    {
        stack->_cnt = RSAllocatorSize(kRSAllocatorSystemDefault, stack->_cnt);
        RSAllocatorAllocate(kRSAllocatorSystemDefault, (RSZone)&stack->_stack[0], stack->_cnt);
        return;
    }
    else if (newCount)
    {
        RSIndex newCapacity = 0;
        newCapacity = RSAllocatorSize(kRSAllocatorSystemDefault, newCount) ;
        if (newCapacity > stack->_cnt)
        {
            stack->_cnt = newCapacity;
            RSAllocatorReAllocate(kRSAllocatorSystemDefault, (RSZone)&stack->_stack[0], stack->_cnt);
        }
    }
}

static void __RSStackPush(RSStackRef stack, RSTypeRef obj)
{
    if (obj == nil) return;
    RSTypeRef* bucket = __RSStackGetRealStack(stack);
    if (stack->_idx < stack->_cnt) bucket[stack->_idx++] = obj;
    else if (stack->_idx == stack->_cnt && __RSStackIsGrowing(stack))
    {
        __RSStackCreateGrowingStack(stack, stack->_cnt*2);
        __RSStackPush(stack, obj);
    }
}

static void __RSStackAddValue(RSStackRef stack, RSTypeRef* obj, RSIndex numberOfObjs)
{
    if (stack->_cnt - stack->_idx < numberOfObjs)
    {
        if (NO == __RSStackIsGrowing(stack)) return;
        
    }
    for (RSIndex idx = 0; idx < numberOfObjs; idx++)
    {
        __RSStackPush(stack, obj[idx]);
    }
}

static RSStackRef __RSStackCreateInstance(RSAllocatorRef allocator, RSIndex stackCapacity, BOOL growing, const RSStackContext* context, RSTypeRef* objs, RSIndex numberOfObjs)
{
    RSStackRef stack = nil;
    RSIndex extraSize = sizeof(struct __RSStack) - sizeof(struct __RSRuntimeBase);
    if (NO == growing)
    {
        extraSize = RSAllocatorSize(allocator, sizeof(struct __RSStack) + stackCapacity);
        extraSize -= sizeof(struct __RSRuntimeBase);
    }
    stack = (RSStackRef)__RSRuntimeCreateInstance(allocator, RSStackGetTypeID(), extraSize);
    if (growing) __RSStackSetGrowing(stack);
    else __RSStackCreateGrowingStack(stack, stackCapacity);
    
    if (context) memcpy(&stack->_context, context, sizeof(RSStackContext));
    if (objs && numberOfObjs > 0)
    {
        __RSStackAddValue(stack, objs, numberOfObjs);
    }
    return stack;
}

RSExport RSStackRef RSStackCreateWithObjects(RSAllocatorRef allocator, RSIndex stackCapacity, BOOL growing, const RSStackContext* context, RSTypeRef* objs, RSIndex numberOfObjs)
{
    return __RSStackCreateInstance(allocator, stackCapacity, growing,context, objs, numberOfObjs);
}

RSExport RSStackRef RSStackCreateWithArray(RSAllocatorRef allocator, RSIndex stackCapacity, RSArrayRef array)
{
    RSIndex cnt = RSArrayGetCount(array);
    RSStackRef stack = __RSStackCreateInstance(allocator, stackCapacity > cnt ? :cnt, NO, kRSStackRSTypeContext, nil, 0);
    for (RSIndex idx = 0; idx < cnt; idx++)
    {
        RSTypeRef obj = RSArrayObjectAtIndex(array, idx);
        __RSStackPush(stack, obj);
        RSRelease(obj);
    }
    return stack;
}
