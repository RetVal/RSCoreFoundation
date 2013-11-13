//
//  RSDictionary.c
//  RSCoreFoundation
//
//  Created by RetVal on 11/4/12.
//  Copyright (c) 2012 RetVal. All rights reserved.

#include <RSCoreFoundation/RSBase.h>
#include <RSCoreFoundation/RSDictionary.h>
#include <RSCoreFoundation/RSNil.h>
#include <RSCoreFoundation/RSNumber.h>
#include <RSCoreFoundation/RSData.h>
#include <RSCoreFoundation/RSString.h>

#include <RSCoreFoundation/RSRuntime.h>

const static RSDictionaryKeyContext __RSDictionaryRSTypeKeyContext = {
#if RSDictionaryCoreSelector == RSDictionaryBasicHash
    __RSTypeCollectionRetain, __RSTypeCollectionRelease,
#elif RSDictionaryCoreSelector == RSDictionaryRBTree
    RSRetain, RSRelease,
#endif
    RSDescription,
    RSCopy,
    RSHash,
#if RSDictionaryCoreSelector  == RSDictionaryBasicHash
    RSEqual,
#elif RSDictionaryCoreSelector == RSDictionaryRBTree
    (RSDictionaryEqualCallBack)RSStringCompare,
#endif
};

const static RSDictionaryValueContext __RSDictionaryRSTypeValueContext = {
#if RSDictionaryCoreSelector == RSDictionaryBasicHash
    __RSTypeCollectionRetain, __RSTypeCollectionRelease,
#elif RSDictionaryCoreSelector == RSDictionaryRBTree
    RSRetain, RSRelease,
#endif
    RSDescription, RSCopy, RSHash, RSEqual,
};

const static RSDictionaryContext __RSDictionaryRSTypeContext = {
    0,
    &__RSDictionaryRSTypeKeyContext,
    &__RSDictionaryRSTypeValueContext
};

const RSDictionaryContext* RSDictionaryRSTypeContext = &__RSDictionaryRSTypeContext;

const RSDictionaryKeyContext* RSDictionaryRSTypeKeyContext = &__RSDictionaryRSTypeKeyContext;
const RSDictionaryValueContext* RSDictionaryRSTypeValueContext = &__RSDictionaryRSTypeValueContext;

static const void* __RSRetainNothing(const void* value)
{
    return value;
}

static void __RSReleaseNothing(const void* value)
{
    return;
}

static const void* __RSRetainNothingWithAllocator(RSAllocatorRef allocator, const void* value)
{
    return value;
}

static void __RSReleaseNothingWithAllocator(RSAllocatorRef allocator, const void* value)
{
    return;
}

static RSStringRef __RSDescriptionNothing(const void* value)
{
    return RSStringCreateWithFormat(RSAllocatorSystemDefault, RSSTR("%p"), value);
}

static const void* __RSCopyNothing(RSAllocatorRef allocator, const void* value)
{
    return value;
}

static BOOL __RSCompareNothing(const void* value1, const void* value2)
{
    uint32_t v1 = (uint32_t)value1;
    uint32_t v2 = (uint32_t)value2;
#if RSDictionaryCoreSelector == RSDictionaryBasicHash
    return v1 == v2;
#elif RSDictionaryCoreSelector == RSDictionaryRBTree
    if (v1 > v2) return RSCompareGreaterThan;
    if (v2 > v1) return RSCompareLessThan;
    return RSCompareEqualTo;
#endif
}

static RSHashCode __RSHashNothing(const void* value)
{
    return (RSHashCode)value;
}

RSPrivate const RSDictionaryKeyContext ___RSDictionaryNilKeyContext = {
#if RSDictionaryCoreSelector == RSDictionaryBasicHash
    __RSRetainNothingWithAllocator, __RSReleaseNothingWithAllocator,
#elif RSDictionaryCoreSelector == RSDictionaryRBTree
    __RSRetainNothing, __RSReleaseNothing,
#endif
    __RSDescriptionNothing, __RSCopyNothing, __RSHashNothing, __RSCompareNothing
};

RSPrivate const RSDictionaryValueContext ___RSDictionaryNilValueContext = {
#if RSDictionaryCoreSelector == RSDictionaryBasicHash
    __RSRetainNothingWithAllocator, __RSReleaseNothingWithAllocator,
#elif RSDictionaryCoreSelector == RSDictionaryRBTree
    RSRetain, RSRelease,
#endif
    __RSDescriptionNothing, __RSCopyNothing, __RSHashNothing, __RSCompareNothing
};

RSPrivate const RSDictionaryKeyContext ___RSDictionaryRSKeyContext = {
#if RSDictionaryCoreSelector == RSDictionaryBasicHash
    __RSTypeCollectionRetain, __RSTypeCollectionRelease,
#elif RSDictionaryCoreSelector == RSDictionaryRBTree
    RSRetain, RSRelease,
#endif
    RSDescription, RSCopy, RSHash,
#if RSDictionaryCoreSelector  == RSDictionaryBasicHash
    RSEqual,
#elif RSDictionaryCoreSelector == RSDictionaryRBTree
    (RSDictionaryEqualCallBack)RSStringCompare,
#endif
};

RSPrivate const RSDictionaryValueContext ___RSDictionaryRSValueContext = {
#if RSDictionaryCoreSelector == RSDictionaryBasicHash
    __RSTypeCollectionRetain, __RSTypeCollectionRelease,
#elif RSDictionaryCoreSelector == RSDictionaryRBTree
    RSRetain, RSRelease,
#endif
    RSDescription, RSCopy, RSHash,
#if RSDictionaryCoreSelector  == RSDictionaryBasicHash
    RSEqual,
#elif RSDictionaryCoreSelector == RSDictionaryRBTree
    (RSDictionaryEqualCallBack)RSStringCompare,
#endif
};

static const RSDictionaryContext __RSDictionaryNilContext = {
    0,
    &___RSDictionaryNilKeyContext,
    &___RSDictionaryNilValueContext,
};

const RSDictionaryContext* RSDictionaryNilContext = &__RSDictionaryNilContext;

static const RSDictionaryContext __RSDictionaryNilKeyContext = {
    0,
    &___RSDictionaryNilKeyContext,
    &___RSDictionaryRSValueContext
};

static const RSDictionaryContext __RSDictionaryNilValueContext = {
    0,
    &___RSDictionaryRSKeyContext,
    &___RSDictionaryNilValueContext,
};

const RSDictionaryContext* RSDictionaryNilKeyContext = &__RSDictionaryNilKeyContext;
const RSDictionaryContext* RSDictionaryNilValueContext = &__RSDictionaryNilValueContext;

const RSDictionaryKeyContext* RSDictionaryDoNilKeyContext = &___RSDictionaryNilKeyContext;
const RSDictionaryValueContext* RSDictionaryDoNilValueContext = &___RSDictionaryNilValueContext;
const RSDictionaryKeyContext* RSDictionaryRSKeyContext = &___RSDictionaryRSKeyContext;
const RSDictionaryValueContext* RSDictionaryRSValueContext = &___RSDictionaryRSValueContext;

#if RSDictionaryCoreSelector == RSDictionaryRBTree
#include "RSPrivate/RSDictionary/rbtree_container.h"
struct __RSDictionary {
    RSRuntimeBase _base;
    RSIndex _cnt;
    RSIndex _capacity;
    const RSDictionaryContext* _context;
    rbtree_container* _root;
};

enum {
    _RSDictionaryStrongMemory     = 1 << 0L,  // 0 = strong, 1 = weak
    _RSDictionaryMutable        = 1 << 1L,  // 0 = const, 1 = mutable
    _RSDictionaryDeallocated      = 1 << 2L,  // 0 = alive, 1 = dead
    
    _RSDictionaryStrongMemoryMask = _RSDictionaryStrongMemory,
    _RSDictionaryMutableMask = _RSDictionaryMutable,
    _RSDictionaryDeallocatedMask = _RSDictionaryDeallocated,
};

RSInline void __RSDictionarySetContext(RSMutableDictionaryRef dictionary, const RSDictionaryContext* context)
{
    dictionary->_context = context;
}

RSInline BOOL isMutable(RSDictionaryRef dictionary)
{
    return (dictionary->_base._rsinfo._rsinfo1 & _RSDictionaryMutable) == _RSDictionaryMutableMask;
}

RSInline void markMutable(RSMutableDictionaryRef dictionary)
{
    (dictionary->_base._rsinfo._rsinfo1 |= _RSDictionaryMutable);
}

RSInline void unMarkMutable(RSMutableDictionaryRef dictionary)
{
    (dictionary->_base._rsinfo._rsinfo1 &= ~_RSDictionaryMutable);
}

RSInline BOOL   isDeallocated(RSDictionaryRef dictionary)
{
    return (dictionary->_base._rsinfo._rsinfo1 & _RSDictionaryDeallocated) == _RSDictionaryDeallocatedMask;
}

RSInline void   markDeallocated(RSDictionaryRef dictionary)
{
    (((struct __RSDictionary*)dictionary)->_base._rsinfo._rsinfo1 |= _RSDictionaryDeallocatedMask);
}

RSInline BOOL   isStrong(RSDictionaryRef dictionary)
{
    return (dictionary->_base._rsinfo._rsinfo1 & _RSDictionaryStrongMemory) == _RSDictionaryStrongMemoryMask;
}

RSInline void   markStrong(RSDictionaryRef dictionary)
{
    (((struct __RSDictionary*)dictionary)->_base._rsinfo._rsinfo1 |= _RSDictionaryStrongMemory);
}

RSInline void   unMarkStrong(RSDictionaryRef dictionary)
{
    (((struct __RSDictionary*)dictionary)->_base._rsinfo._rsinfo1 &= ~_RSDictionaryStrongMemory);
}

RSInline RSIndex __RSDictionaryCount(RSDictionaryRef dictionary)
{
    return dictionary->_cnt;
}

RSInline void __RSDictionaryCountIncrenment(RSDictionaryRef dictionary)
{
    ((struct __RSDictionary*)dictionary)->_cnt++;
}

RSInline void __RSDictionaryCountDecrenment(RSDictionaryRef dictionary)
{
    if(dictionary->_cnt) ((struct __RSDictionary*)dictionary)->_cnt--;
}

RSInline void __RSDictionarySetCount(RSDictionaryRef dictionary, RSIndex count)
{
    ((struct __RSDictionary*)dictionary)->_cnt = count;
}

RSInline RSIndex __RSDictionaryCapacity(RSDictionaryRef dictionary)
{
    return dictionary->_capacity;
}
RSInline void __RSDictionarySetCapacity(RSDictionaryRef dictionary, RSIndex capacity)
{
    ((struct __RSDictionary*)dictionary)->_capacity = capacity;
}

RSInline rbtree_container* __RSDictionaryRoot(RSDictionaryRef dictionary)
{
    return dictionary->_root;
}
RSPrivate void __RSDictionarySetEqualCallBack(RSMutableDictionaryRef dictionary, RSKeyCompare equal)
{
    if(dictionary && dictionary->_root)
    {
        dictionary->_root->compare = equal;
    }
}

static void __RSDictionaryClassInit(RSTypeRef obj)
{
    RSDictionaryRef dictionary = (RSTypeRef)obj;
    ((RSMutableDictionaryRef)dictionary)->_root = RSAllocatorAllocate(RSAllocatorSystemDefault, sizeof(rbtree_container));
}

static RSTypeRef __RSDictionaryClassCopy(RSAllocatorRef allocator, RSTypeRef rs, BOOL mutbaleCopy)
{
    RSMutableDictionaryRef copy = RSDictionaryCreateMutableCopy(allocator, rs);
    if (mutbaleCopy == NO) unMarkMutable(copy);
    return copy;
}

static void __RSDictionaryClassDeallocate(RSTypeRef obj)
{
    RSDictionaryRef dictionary = (RSTypeRef)obj;
    rbtree_container* tree = __RSDictionaryRoot(dictionary);
    rbtree_container_node* pc = rbtree_container_first(tree);
    rbtree_container_node* tpc = nil;
	while (pc)
	{
		tpc = pc;
        
        dictionary->_context->keyContext->release(tpc->key);
        dictionary->_context->valueContext->release(tpc->value);
		pc = rbtree_container_next(pc);
		rbtree_container_erase(tree, tpc);
		rbtree_container_node_free(tpc);
	}
    RSAllocatorDeallocate(RSAllocatorSystemDefault, dictionary->_root);
}

static BOOL __RSDictionaryClassEqual(RSTypeRef obj1, RSTypeRef obj2)
{
    if (obj1 == obj2) return YES;
    RSDictionaryRef dictionary1 = (RSDictionaryRef)obj1;
    RSDictionaryRef dictionary2 = (RSDictionaryRef)obj2;
    RSRetain(dictionary1); RSRetain(dictionary2);
    if (__RSDictionaryCount(dictionary1) != __RSDictionaryCount(dictionary2))
    {
        RSRelease(dictionary1);RSRelease(dictionary2);
        return NO;
    }
    if (dictionary1->_context != dictionary2->_context) {
        RSRelease(dictionary1);RSRelease(dictionary2);
        return NO;
    }
    rbtree_container *tree1 = __RSDictionaryRoot(dictionary1), *tree2 = __RSDictionaryRoot(dictionary2);
    rbtree_container_node* pc1 = rbtree_container_first(tree1), *pc2 =  rbtree_container_first(tree2);
    rbtree_container_node* tpc1 = nil, *tpc2 = nil;
	while (pc1 && pc2)
	{
		tpc1 = pc1;tpc2 = pc2;
        if( dictionary1->_context->keyContext->equal(tpc1->key, tpc2->key) && dictionary1->_context->valueContext->equal(tpc1->value, tpc2->value))
        {
            pc1 = rbtree_container_next(pc1);
            pc2 = rbtree_container_next(pc2);
        }
		else
        {
            RSRelease(dictionary1);RSRelease(dictionary2);
            return NO;
        }
	}
    RSRelease(dictionary1);RSRelease(dictionary2);
    return YES;
}

static RSStringRef __RSDictionaryClassDescription(RSTypeRef obj)
{
    RSMutableStringRef desc = nil;
    RSDictionaryRef dictionary = (RSDictionaryRef)RSRetain(obj);
    rbtree_container* tree = __RSDictionaryRoot(dictionary);
    rbtree_container_node* pc = rbtree_container_first(tree);
    rbtree_container_node* tpc = nil;
    desc = RSStringCreateMutableCopy(RSAllocatorSystemDefault, 0, RSSTR("{\n"));

	while (pc)
	{
		tpc = pc;
        pc = rbtree_container_next(pc);
        if (pc == nil) break;
        RSStringRef key = dictionary->_context->keyContext->description(tpc->key);
        RSTypeRef value = dictionary->_context->valueContext->description(tpc->value);
        RSStringAppendStringWithFormat(desc, RSSTR("\t%R = %R,\n"), key, value);
        RSRelease(key);
        RSRelease(value);
	}
    if (tpc)
    {
        RSStringRef key = dictionary->_context->keyContext->description(tpc->key);
        RSTypeRef value = dictionary->_context->valueContext->description(tpc->value);
        RSStringAppendStringWithFormat(desc, RSSTR("\t%R = %R\n"), key, value);
        RSRelease(key);
        RSRelease(value);
    }
    RSStringAppendCString(desc, "}", RSStringEncodingASCII);
    //RSRelease(format);
    RSRelease(obj);
    return desc;
}
static RSRuntimeClass __RSDictionaryClass = {
    _RSRuntimeScannedObject,
    "RSDictionary",
    __RSDictionaryClassInit,
    __RSDictionaryClassCopy,
    __RSDictionaryClassDeallocate,
    __RSDictionaryClassEqual,
    nil,
    __RSDictionaryClassDescription,
    nil,
    nil,
};

static RSTypeID _RSDictionaryTypeID = _RSRuntimeNotATypeID;

RSPrivate void __RSDictionaryInitialize()
{
    _RSDictionaryTypeID = __RSRuntimeRegisterClass(&__RSDictionaryClass);
    __RSRuntimeSetClassTypeID(&__RSDictionaryClass, _RSDictionaryTypeID);
}

RSExport RSTypeID RSDictionaryGetTypeID()
{
    return _RSDictionaryTypeID;
}

static void __RSDictionaryAvailable(RSDictionaryRef dictionary)
{
    if (dictionary == nil) HALTWithError(RSInvalidArgumentException, "dictionary is nil");
    __RSGenericValidInstance(dictionary, _RSDictionaryTypeID);
    
}

static void __RSDictionaryAddValue(RSMutableDictionaryRef dictionary, const void** keys, const void** values, RSIndex count)
{
    rbtree_container* rbt = __RSDictionaryRoot(dictionary);
    for (RSIndex idx = 0; idx < count; idx++)
    {
        rbtree_container_node* node = rbtree_container_node_malloc(rbt, 0);
        node->key = dictionary->_context->keyContext->retain((keys[idx]));
        node->value = dictionary->_context->valueContext->retain(values[idx]);
        if (kSuccess == rbtree_container_insert(rbt, node))
            __RSDictionaryCountIncrenment(dictionary);
        else
        {
            // the key is already existing, should replace it.
            rbtree_container_node* find = rbtree_container_replace(rbt, node);
            if (find)
            {
                dictionary->_context->keyContext->release(find->key);
                dictionary->_context->valueContext->release(find->value);
                rbtree_container_node_free(find);
                //__RSDictionaryCountIncrenment(dictionary);
            }
            else
                HALTWithError(RSInvalidArgumentException, "what the hell to find the key-value.");
            // already existing but can not find now. unbelievable! Give up.
        }
    }
}


static void __RSDictionaryRemoveValue(RSMutableDictionaryRef dictionary, RSTypeRef* keys, RSIndex count)
{
    rbtree_container* rbt = __RSDictionaryRoot(dictionary);
    for (RSIndex idx = 0; idx < count; idx++)
    {
        rbtree_container_node* find = rbtree_container_delete(rbt, keys[idx]);
        if (find)
        {
            dictionary->_context->keyContext->release(find->key);
            dictionary->_context->valueContext->release(find->value);
            __RSDictionaryCountDecrenment(dictionary);
            rbtree_container_node_free(find);
        }
    }
}

static RSTypeRef __RSDictionaryObjectForKey(RSDictionaryRef dictionary, RSTypeRef key)
{
    rbtree_container* rbt = __RSDictionaryRoot(dictionary);
    rbtree_container_node* find = rbtree_container_search(rbt, key);
    if (find)
    {
        return (find->value);
    }
    return nil;
}

static void __RSDictionaryRemoveAllObjects(RSMutableDictionaryRef dictionary, BOOL flag)
{
    rbtree_container* rbt = __RSDictionaryRoot(dictionary);
    rbtree_container_node* pc = rbtree_container_first(rbt), *tpc = nil;
    while (pc)
    {
        tpc = pc;
        dictionary->_context->keyContext->release(tpc->key);
        dictionary->_context->valueContext->release(tpc->value);
        pc = rbtree_container_next(pc);
        rbtree_container_erase(rbt, tpc);
		rbtree_container_node_free(tpc);
    }
}



RSPrivate void __RSDictionarySetObjectForCKey(RSMutableDictionaryRef dictionary, RSCBuffer key, RSTypeRef object)
{
    __RSDictionaryAvailable(dictionary);
    if (isMutable(dictionary) == NO) HALTWithError(RSInvalidArgumentException, "the dictionary is not mutable.");
    if (key == nil) HALTWithError(RSInvalidArgumentException, "the key is nil");
    if (__RSDictionaryCapacity(dictionary)) // NOT 0 is means limit.
    {
        if (__RSDictionaryCapacity(dictionary) > __RSDictionaryCount(dictionary)) {
            return __RSDictionaryAddValue(dictionary, (RSTypeRef*)&key, &object, 1);
        }
    }
    __RSDictionaryAddValue(dictionary, (RSTypeRef*)&key, &object, 1);
}

RSPrivate void __RSDictionaryCleanAllObjects(RSMutableDictionaryRef dictionary)
{
    rbtree_container* rbt = __RSDictionaryRoot(dictionary);
    rbtree_container_node* pc = rbtree_container_first(rbt), *tpc = nil;
    //RSIndex refcnt = 0;
    while (pc)
    {
        tpc = pc;
        //__RSCLog(RSLogLevelDebug, "%s\n",tpc->key);
        //1 << 3
        RSDeallocateInstance(tpc->value);
        //RSRelease(tpc->value);
        pc = rbtree_container_next(pc);
        rbtree_container_erase(rbt, tpc);
		rbtree_container_node_free(tpc);
    }
}

RSPrivate void __RSDictionaryCleanAllKeysAndObjects(RSMutableDictionaryRef dictionary)
{
    rbtree_container* rbt = __RSDictionaryRoot(dictionary);
    rbtree_container_node* pc = rbtree_container_first(rbt), *tpc = nil;
    while (pc)
    {
        tpc = pc;
        RSDeallocateInstance(tpc->key);
        //1 << 3
        RSDeallocateInstance(tpc->value);
        //RSRelease(tpc->value);
        pc = rbtree_container_next(pc);
        rbtree_container_erase(rbt, tpc);
		rbtree_container_node_free(tpc);
    }
}

static RSMutableDictionaryRef __RSDictionaryCreateInstance(RSAllocatorRef allocator, const void** keys, const void** values, const RSDictionaryContext* context, RSIndex count, BOOL _strong, BOOL _mutable, BOOL _needCopy, RSIndex capacity)
{
    RSMutableDictionaryRef mutableDictionary = (RSMutableDictionaryRef)__RSRuntimeCreateInstance(allocator, _RSDictionaryTypeID, sizeof(struct __RSDictionary) - sizeof(struct __RSRuntimeBase));
    if (capacity < count) capacity = count;
    if (_mutable) markMutable(mutableDictionary);
    else unMarkMutable(mutableDictionary);
    __RSDictionarySetCapacity(mutableDictionary, capacity);
    if (context == nil) context = RSDictionaryRSTypeContext;
    __RSDictionarySetContext(mutableDictionary, context);
    __RSDictionarySetEqualCallBack(mutableDictionary, context->keyContext->equal);
    if (_strong)    // always strong now (11/4/12)
    {
        if (keys)
        {
            if (values)
            {
                __RSDictionaryAddValue(mutableDictionary, keys, values, count);
            }
            else HALTWithError(RSInvalidArgumentException, "keys and values is not paired.");
        }
        
    }       //strong
    else    // weak.
    {
        
    }
    return mutableDictionary;
}

RSExport RSDictionaryRef RSDictionaryCreate(RSAllocatorRef allocator, const void** keys, const void** values, const RSDictionaryContext* context, RSIndex count)
{
    if ((keys && values && count) || (keys == nil && values == nil && count == 0))
    {
        return __RSDictionaryCreateInstance(allocator, keys, values, context, count, YES, NO, NO, 0);
    }
    return __RSDictionaryCreateInstance(allocator, nil, nil, context, 0, YES, NO, NO, 0);
    HALTWithError(RSInvalidArgumentException, "overflow");
}

RSExport RSMutableDictionaryRef RSDictionaryCreateMutableCopy(RSAllocatorRef allocator, RSDictionaryRef dictionary)
{
    __RSGenericValidInstance(dictionary, RSDictionaryGetTypeID());
    //RSArrayRef keys = RSDictionaryAllKeys(dictionary);
    RSMutableDictionaryRef copy = RSDictionaryCreateMutable(allocator, 0, dictionary->_context);
    
    rbtree_container* rbt = __RSDictionaryRoot(dictionary);
    rbtree_container_node* pc = rbtree_container_first(rbt), *tpc = nil;
    RSIndex cnt = RSDictionaryGetCount(dictionary);
    
    RSTypeRef value = nil, key = nil;
    while (pc && cnt)
    {
        tpc = pc;
        key = copy->_context->keyContext->copy(allocator, tpc->key);
        value = copy->_context->valueContext->copy(allocator, tpc->value);
        RSDictionarySetValue(copy, key, value);
        if (copy->_context->valueContext->release)
            copy->_context->valueContext->release(value);
        if (copy->_context->keyContext->release)
            copy->_context->keyContext->release(key);
        --cnt;
        pc = rbtree_container_next(pc);
    }
    return copy;
}

RSExport RSDictionaryRef RSDictionaryCreateWithArray(RSAllocatorRef allocator, RSArrayRef keys, RSArrayRef values, const RSDictionaryContext* context)
{
    if (!(keys && values)) return __RSDictionaryCreateInstance(allocator, nil, nil, context, 0, YES, NO, NO, 0);
    RSIndex cnt = RSArrayGetCount(keys);
    if (cnt != RSArrayGetCount(values)) HALTWithError(RSInvalidArgumentException, "the number of keys and values is not equal.");
    RSMutableDictionaryRef dictionary = __RSDictionaryCreateInstance(allocator, nil, nil, context, 0, YES, YES, NO, 0);
    RSRetain(keys); RSRetain(values);
    for (RSIndex idx = 0; idx < cnt; idx++)
    {
        RSTypeRef key = RSArrayObjectAtIndex(keys, idx);
        RSTypeRef value = RSArrayObjectAtIndex(values, idx);
        __RSDictionaryAddValue(dictionary, &key, &value, 1);
    }
    RSRelease(keys);RSRelease(values);
    unMarkMutable(dictionary);
    return dictionary;
}

RSExport RSMutableDictionaryRef RSDictionaryCreateWithObjectsAndOKeys(RSAllocatorRef allocator, RSTypeRef obj, ...)
{
    RSStringRef key = nil;
    RSTypeRef value = obj;
    RSMutableDictionaryRef dict = RSDictionaryCreateMutable(allocator, 0, RSDictionaryRSTypeContext);
    va_list va;
    va_start(va, obj);
    value = obj;
    if (value == nil)
    {
        va_end(va);
        return dict;
    }
    for (;;)
    {
        key = va_arg(va, RSStringRef);
        RSDictionarySetValue(dict, key, value);
        value = va_arg(va, RSTypeRef);
        if (value == nil)
        {
            va_end(va);
            return dict;
        }
        
    }
    va_end(va);
    return dict;
}

RSExport RSMutableDictionaryRef RSDictionaryCreateMutable(RSAllocatorRef allocator, RSIndex capacity, const RSDictionaryContext* context)
{
    return __RSDictionaryCreateInstance(allocator, nil, nil, context, 0, YES, YES, NO, capacity);
}

RSExport void RSDictionarySetValue(RSMutableDictionaryRef dictionary, const void* key, const void* value)
{
    __RSDictionaryAvailable(dictionary);
    if (isMutable(dictionary) == NO) HALTWithError(RSInvalidArgumentException, "the dictionary is not mutable.");
    if (key == nil) HALTWithError(RSInvalidArgumentException, "the key is nil");
    if (value == nil || value == RSNil) return RSDictionaryRemoveValue(dictionary, key);
    if (__RSDictionaryCapacity(dictionary)) // NOT 0 is means limit.
    {
        if (__RSDictionaryCapacity(dictionary) > __RSDictionaryCount(dictionary)) {
            return __RSDictionaryAddValue(dictionary, (RSTypeRef*)&key, &value, 1);
        }
        return ;
    }
    __RSDictionaryAddValue(dictionary, (RSTypeRef*)&key, &value, 1);
}
RSExport RSTypeRef RSDictionaryGetValue(RSDictionaryRef dictionary, const void* key)
{
    __RSDictionaryAvailable(dictionary);
    //if (isMutable(dictionary) == NO) HALTWithError(RSGenericException, "the dictionary is not mutable.");
    if (key == nil) HALTWithError(RSInvalidArgumentException, "the key is nil");
    RSTypeRef value = __RSDictionaryObjectForKey(dictionary, key);
    return value;
}

RSExport void RSDictionaryRemoveValue(RSMutableDictionaryRef dictionary, const void* key)
{
    __RSDictionaryAvailable(dictionary);
    if (isMutable(dictionary) == NO) HALTWithError(RSInvalidArgumentException, "the dictionary is not mutable.");
    if (key == nil) HALTWithError(RSInvalidArgumentException, "the key is nil");
    __RSDictionaryRemoveValue(dictionary, (RSTypeRef*)&key, 1);
}

RSExport void RSDictionaryRemoveAllObjects(RSMutableDictionaryRef dictionary)
{
    __RSDictionaryAvailable(dictionary);
    if (isMutable(dictionary) == NO) HALTWithError(RSInvalidArgumentException, "the dictionary is not mutable.");
    __RSDictionaryRemoveAllObjects(dictionary, NO);
}

RSExport RSIndex RSDictionaryGetCount(RSDictionaryRef dictionary)
{
    __RSDictionaryAvailable(dictionary);
    return __RSDictionaryCount(dictionary);
}

RSExport RSArrayRef RSDictionaryAllKeys(RSDictionaryRef dictionary)
{
    __RSDictionaryAvailable(dictionary);
    if (dictionary->_context == RSDictionaryNilKeyContext) return nil;
    RSIndex cnt = __RSDictionaryCount(dictionary);
    RSMutableArrayRef array = RSArrayCreateMutable(RSAllocatorSystemDefault, cnt);
    rbtree_container* rbt = __RSDictionaryRoot(dictionary);
    rbtree_container_node* pc = rbtree_container_first(rbt), *tpc = nil;
    while (pc)
    {
        tpc = pc;
        RSArrayAddObject(array, tpc->key);
        pc = rbtree_container_next(pc);
    }
    
    return array;
    
}

RSExport RSArrayRef RSDictionaryAllValues(RSDictionaryRef dictionary)
{
    __RSDictionaryAvailable(dictionary);
    if (dictionary->_context == RSDictionaryNilValueContext) return nil;
    RSIndex cnt = __RSDictionaryCount(dictionary);
    RSMutableArrayRef array = RSArrayCreateMutable(RSAllocatorSystemDefault, cnt);
    
    rbtree_container* rbt = __RSDictionaryRoot(dictionary);
    rbtree_container_node* pc = rbtree_container_first(rbt), *tpc = nil;
    while (pc)
    {
        tpc = pc;
        RSArrayAddObject(array, tpc->value);
        pc = rbtree_container_next(pc);
    }
    
    return array;
}

RSExport void RSDictionaryGetKeysAndValues(RSDictionaryRef dictionary, RSTypeRef* keys, RSTypeRef* values)
{
    __RSDictionaryAvailable(dictionary);
    if (keys || values)
    {
        rbtree_container* rbt = __RSDictionaryRoot(dictionary);
        rbtree_container_node* pc = rbtree_container_first(rbt), *tpc = nil;
        RSIndex cnt = RSDictionaryGetCount(dictionary), idx = 0;
        while (pc && cnt)
        {
            tpc = pc;
            if (keys)
            {
                keys[idx] = tpc->key;
            }
            if (values)
            {
                values[idx] = tpc->value;
            }
            ++idx;
            --cnt;
            pc = rbtree_container_next(pc);
        }
    }
}

RSExport void RSDictionaryApplyFunction(RSDictionaryRef dictionary, RSDictionaryApplierFunction apply, void* context)
{
    __RSDictionaryAvailable(dictionary);
    RSIndex cnt = RSDictionaryGetCount(dictionary);
    if (cnt && apply)
    {
        rbtree_container* rbt = __RSDictionaryRoot(dictionary);
        rbtree_container_node* pc = rbtree_container_first(rbt), *tpc = nil;
        while (pc && cnt)
        {
            tpc = pc;
            apply(tpc->key, tpc->value, context);
            --cnt;
            pc = rbtree_container_next(pc);
        }
    }
    return;
}
#elif RSDictionaryCoreSelector == RSDictionaryBasicHash

#include "RSBasicHash.h"

#define RSDictionary 0
#define RSSet 0
#define RSBag 0
#undef RSDictionary
#define RSDictionary 1

#if RSDictionary
const RSDictionaryKeyContext RSTypeDictionaryKeyCallBacks = {__RSTypeCollectionRetain, __RSTypeCollectionRelease, RSDescription, RSCopy, RSHash, RSEqual};
const RSDictionaryKeyContext RSCopyStringDictionaryKeyCallBacks = {__RSStringCollectionCopy, __RSTypeCollectionRelease, RSDescription, RSCopy, RSHash, RSEqual};
const RSDictionaryValueContext RSTypeDictionaryValueCallBacks = {__RSTypeCollectionRetain, __RSTypeCollectionRelease, RSDescription, RSCopy, RSHash, RSEqual};
__private_extern__ const RSDictionaryValueContext RSTypeDictionaryValueCompactableCallBacks = { __RSTypeCollectionRetain, __RSTypeCollectionRelease, RSDescription, RSCopy, RSHash, RSEqual};
static const RSDictionaryKeyContext __RSNullDictionaryKeyCallBacks = {0, nil, nil, nil, nil, nil};
static const RSDictionaryValueContext __RSNullDictionaryValueCallBacks = {0, nil, nil, nil, nil};

#define RSHashRef RSDictionaryRef
#define RSMutableHashRef RSMutableDictionaryRef
#define RSHashKeyCallBacks RSDictionaryKeyContext
#define RSHashValueCallBacks RSDictionaryValueContext
#endif

#if RSSet
const RSDictionaryCallBacks RSTypeDictionaryCallBacks = {0, __RSTypeCollectionRetain, __RSTypeCollectionRelease, RSDescription, RSEqual, RSHash};
const RSDictionaryCallBacks RSCopyStringDictionaryCallBacks = {0, __RSStringCollectionCopy, __RSTypeCollectionRelease, RSDescription, RSEqual, RSHash};
static const RSDictionaryCallBacks __RSNullDictionaryCallBacks = {0, nil, nil, nil, nil, nil};

#define RSDictionaryKeyContext RSDictionaryCallBacks
#define RSDictionaryValueContext RSDictionaryCallBacks
#define RSTypeDictionaryKeyCallBacks RSTypeDictionaryCallBacks
#define RSTypeDictionaryValueCallBacks RSTypeDictionaryCallBacks
#define __RSNullDictionaryKeyCallBacks __RSNullDictionaryCallBacks
#define __RSNullDictionaryValueCallBacks __RSNullDictionaryCallBacks

#define RSHashRef RSSetRef
#define RSMutableHashRef RSMutableSetRef
#define RSHashKeyCallBacks RSDictionaryCallBacks
#define RSHashValueCallBacks RSDictionaryCallBacks
#endif

#if RSBag
const RSDictionaryCallBacks RSTypeDictionaryCallBacks = {0, __RSTypeCollectionRetain, __RSTypeCollectionRelease, RSDescription, RSEqual, RSHash};
const RSDictionaryCallBacks RSCopyStringDictionaryCallBacks = {0, __RSStringCollectionCopy, __RSTypeCollectionRelease, RSDescription, RSEqual, RSHash};
static const RSDictionaryCallBacks __RSNullDictionaryCallBacks = {0, nil, nil, nil, nil, nil};

#define RSDictionaryKeyContext RSDictionaryCallBacks
#define RSDictionaryValueContext RSDictionaryCallBacks
#define RSTypeDictionaryKeyCallBacks RSTypeDictionaryCallBacks
#define RSTypeDictionaryValueCallBacks RSTypeDictionaryCallBacks
#define __RSNullDictionaryKeyCallBacks __RSNullDictionaryCallBacks
#define __RSNullDictionaryValueCallBacks __RSNullDictionaryCallBacks

#define RSHashRef RSBagRef
#define RSMutableHashRef RSMutableBagRef
#define RSHashKeyCallBacks RSDictionaryCallBacks
#define RSHashValueCallBacks RSDictionaryCallBacks
#endif


typedef uintptr_t any_t;
typedef const void * const_any_pointer_t;
typedef void * any_pointer_t;

static RSTypeRef __RSDictionaryCopy(RSAllocatorRef allocator, RSTypeRef rs, BOOL mutbaleCopy)
{
    if (mutbaleCopy) return RSDictionaryCreateMutableCopy(allocator, rs);
    return RSDictionaryCreateCopy(allocator, rs);
}

static BOOL __RSDictionaryEqual(RSTypeRef rs1, RSTypeRef rs2) {
    return __RSBasicHashEqual((RSBasicHashRef)rs1, (RSBasicHashRef)rs2);
}

static RSHashCode __RSDictionaryHash(RSTypeRef rs) {
    return __RSBasicHashHash((RSBasicHashRef)rs);
}

static RSStringRef __RSDictionaryCopyDescription(RSTypeRef rs) {
    return RSBasicHashDescription(rs, NO, RSSTR(""), RSSTR("\t"), YES);
}

static void __RSDictionaryDeallocate(RSTypeRef rs) {
    __RSBasicHashDeallocate((RSBasicHashRef)rs);
}

static RSTypeID _RSDictionaryTypeID = _RSRuntimeNotATypeID;

static const RSRuntimeClass __RSDictionaryClass = {
    _RSRuntimeScannedObject,
    "RSDictionary",
    nil,        // init
    __RSDictionaryCopy,        // copy
    __RSDictionaryDeallocate,
    __RSDictionaryEqual,
    __RSDictionaryHash,
    __RSDictionaryCopyDescription
};

RSPrivate void __RSDictionaryInitialize()
{
    RSDictionaryGetTypeID();
}

RSTypeID RSDictionaryGetTypeID(void) {
    if (_RSRuntimeNotATypeID == _RSDictionaryTypeID)
    {
        _RSDictionaryTypeID = __RSRuntimeRegisterClass(&__RSDictionaryClass);
        __RSRuntimeSetClassTypeID(&__RSDictionaryClass, _RSDictionaryTypeID);
    }
    return _RSDictionaryTypeID;
}

#if !defined(RSBasicHashVersion) || (RSBasicHashVersion == 1)
#define GCRETAIN(A, B) RSTypeSetCallBacks.retain(A, B)
#define GCRELEASE(A, B) RSTypeSetCallBacks.release(A, B)

static uintptr_t __RSDictionaryStandardRetainValue(RSConstBasicHashRef ht, uintptr_t stack_value) {
    if (RSBasicHashGetSpecialBits(ht) & 0x0100) return stack_value;
    return (RSBasicHashHasStrongValues(ht)) ? (uintptr_t)GCRETAIN(RSAllocatorSystemDefault, (RSTypeRef)stack_value) : (uintptr_t)RSRetain((RSTypeRef)stack_value);
}

static uintptr_t __RSDictionaryStandardRetainKey(RSConstBasicHashRef ht, uintptr_t stack_key) {
    if (RSBasicHashGetSpecialBits(ht) & 0x0001) return stack_key;
    return (RSBasicHashHasStrongKeys(ht)) ? (uintptr_t)GCRETAIN(RSAllocatorSystemDefault, (RSTypeRef)stack_key) : (uintptr_t)RSRetain((RSTypeRef)stack_key);
}

static void __RSDictionaryStandardReleaseValue(RSConstBasicHashRef ht, uintptr_t stack_value) {
    if (RSBasicHashGetSpecialBits(ht) & 0x0200) return;
    if (RSBasicHashHasStrongValues(ht)) GCRELEASE(RSAllocatorSystemDefault, (RSTypeRef)stack_value); else RSRelease((RSTypeRef)stack_value);
}

static void __RSDictionaryStandardReleaseKey(RSConstBasicHashRef ht, uintptr_t stack_key) {
    if (RSBasicHashGetSpecialBits(ht) & 0x0002) return;
    if (RSBasicHashHasStrongKeys(ht)) GCRELEASE(RSAllocatorSystemDefault, (RSTypeRef)stack_key); else RSRelease((RSTypeRef)stack_key);
}

static BOOL __RSDictionaryStandardEquateValues(RSConstBasicHashRef ht, uintptr_t coll_value1, uintptr_t stack_value2) {
    if (RSBasicHashGetSpecialBits(ht) & 0x0400) return coll_value1 == stack_value2;
    return RSEqual((RSTypeRef)coll_value1, (RSTypeRef)stack_value2);
}

static BOOL __RSDictionaryStandardEquateKeys(RSConstBasicHashRef ht, uintptr_t coll_key1, uintptr_t stack_key2) {
    if (RSBasicHashGetSpecialBits(ht) & 0x0004) return coll_key1 == stack_key2;
    return RSEqual((RSTypeRef)coll_key1, (RSTypeRef)stack_key2);
}

static uintptr_t __RSDictionaryStandardHashKey(RSConstBasicHashRef ht, uintptr_t stack_key) {
    if (RSBasicHashGetSpecialBits(ht) & 0x0008) return stack_key;
    return (uintptr_t)RSHash((RSTypeRef)stack_key);
}

static uintptr_t __RSDictionaryStandardGetIndirectKey(RSConstBasicHashRef ht, uintptr_t coll_value) {
    return 0;
}

static RSStringRef __RSDictionaryStandardCopyValueDescription(RSConstBasicHashRef ht, uintptr_t stack_value) {
    if (RSBasicHashGetSpecialBits(ht) & 0x0800) return RSStringCreateWithFormat(RSAllocatorSystemDefault, nil, RSSTR("<%p>"), (void *)stack_value);
    return RSDescription((RSTypeRef)stack_value);
}

static RSStringRef __RSDictionaryStandardCopyKeyDescription(RSConstBasicHashRef ht, uintptr_t stack_key) {
    if (RSBasicHashGetSpecialBits(ht) & 0x0010) return RSStringCreateWithFormat(RSAllocatorSystemDefault, nil, RSSTR("<%p>"), (void *)stack_key);
    return RSDescription((RSTypeRef)stack_key);
}

static RSBasicHashCallbacks *__RSDictionaryStandardCopyCallbacks(RSConstBasicHashRef ht, RSAllocatorRef allocator, RSBasicHashCallbacks *cb);
static void __RSDictionaryStandardFreeCallbacks(RSConstBasicHashRef ht, RSAllocatorRef allocator, RSBasicHashCallbacks *cb);

static const RSBasicHashCallbacks RSDictionaryStandardCallbacks = {
    __RSDictionaryStandardCopyCallbacks,
    __RSDictionaryStandardFreeCallbacks,
    __RSDictionaryStandardRetainValue,
    __RSDictionaryStandardRetainKey,
    __RSDictionaryStandardReleaseValue,
    __RSDictionaryStandardReleaseKey,
    __RSDictionaryStandardEquateValues,
    __RSDictionaryStandardEquateKeys,
    __RSDictionaryStandardHashKey,
    __RSDictionaryStandardGetIndirectKey,
    __RSDictionaryStandardCopyValueDescription,
    __RSDictionaryStandardCopyKeyDescription
};

static RSBasicHashCallbacks *__RSDictionaryStandardCopyCallbacks(RSConstBasicHashRef ht, RSAllocatorRef allocator, RSBasicHashCallbacks *cb) {
    return (RSBasicHashCallbacks *)&RSDictionaryStandardCallbacks;
}

static void __RSDictionaryStandardFreeCallbacks(RSConstBasicHashRef ht, RSAllocatorRef allocator, RSBasicHashCallbacks *cb) {
}


static RSBasicHashCallbacks *__RSDictionaryCopyCallbacks(RSConstBasicHashRef ht, RSAllocatorRef allocator, RSBasicHashCallbacks *cb) {
    RSBasicHashCallbacks *newcb = nil;
    newcb = (RSBasicHashCallbacks *)RSAllocatorAllocate(allocator, sizeof(RSBasicHashCallbacks) + 10 * sizeof(void *));
    if (!newcb) return nil;
    memmove(newcb, (void *)cb, sizeof(RSBasicHashCallbacks) + 10 * sizeof(void *));
    return newcb;
}

static void __RSDictionaryFreeCallbacks(RSConstBasicHashRef ht, RSAllocatorRef allocator, RSBasicHashCallbacks *cb) {
    if (RS_IS_COLLECTABLE_ALLOCATOR(allocator)) {
    } else {
        RSAllocatorDeallocate(allocator, cb);
    }
}

static uintptr_t __RSDictionaryRetainValue(RSConstBasicHashRef ht, uintptr_t stack_value) {
    const RSBasicHashCallbacks *cb = RSBasicHashGetCallbacks(ht);
    const_any_pointer_t (*value_retain)(RSAllocatorRef, const_any_pointer_t) = (const_any_pointer_t (*)(RSAllocatorRef, const_any_pointer_t))cb->context[0];
    if (nil == value_retain) return stack_value;
    return (uintptr_t)INVOKE_CALLBACK2(value_retain, RSGetAllocator(ht), (const_any_pointer_t)stack_value);
}

static uintptr_t __RSDictionaryRetainKey(RSConstBasicHashRef ht, uintptr_t stack_key) {
    const RSBasicHashCallbacks *cb = RSBasicHashGetCallbacks(ht);
    const_any_pointer_t (*key_retain)(RSAllocatorRef, const_any_pointer_t) = (const_any_pointer_t (*)(RSAllocatorRef, const_any_pointer_t))cb->context[1];
    if (nil == key_retain) return stack_key;
    return (uintptr_t)INVOKE_CALLBACK2(key_retain, RSGetAllocator(ht), (const_any_pointer_t)stack_key);
}

static void __RSDictionaryReleaseValue(RSConstBasicHashRef ht, uintptr_t stack_value) {
    const RSBasicHashCallbacks *cb = RSBasicHashGetCallbacks(ht);
    void (*value_release)(RSAllocatorRef, const_any_pointer_t) = (void (*)(RSAllocatorRef, const_any_pointer_t))cb->context[2];
    if (nil != value_release) INVOKE_CALLBACK2(value_release, RSGetAllocator(ht), (const_any_pointer_t) stack_value);
}

static void __RSDictionaryReleaseKey(RSConstBasicHashRef ht, uintptr_t stack_key) {
    const RSBasicHashCallbacks *cb = RSBasicHashGetCallbacks(ht);
    void (*key_release)(RSAllocatorRef, const_any_pointer_t) = (void (*)(RSAllocatorRef, const_any_pointer_t))cb->context[3];
    if (nil != key_release) INVOKE_CALLBACK2(key_release, RSGetAllocator(ht), (const_any_pointer_t) stack_key);
}

static BOOL __RSDictionaryEquateValues(RSConstBasicHashRef ht, uintptr_t coll_value1, uintptr_t stack_value2) {
    const RSBasicHashCallbacks *cb = RSBasicHashGetCallbacks(ht);
    BOOL (*value_equal)(const_any_pointer_t, const_any_pointer_t) = (BOOL (*)(const_any_pointer_t, const_any_pointer_t))cb->context[4];
    if (nil == value_equal) return (coll_value1 == stack_value2);
    return INVOKE_CALLBACK2(value_equal, (const_any_pointer_t) coll_value1, (const_any_pointer_t) stack_value2) ? 1 : 0;
}

static BOOL __RSDictionaryEquateKeys(RSConstBasicHashRef ht, uintptr_t coll_key1, uintptr_t stack_key2) {
    const RSBasicHashCallbacks *cb = RSBasicHashGetCallbacks(ht);
    BOOL (*key_equal)(const_any_pointer_t, const_any_pointer_t) = (BOOL (*)(const_any_pointer_t, const_any_pointer_t))cb->context[5];
    if (nil == key_equal) return (coll_key1 == stack_key2);
    return INVOKE_CALLBACK2(key_equal, (const_any_pointer_t) coll_key1, (const_any_pointer_t) stack_key2) ? 1 : 0;
}

static uintptr_t __RSDictionaryHashKey(RSConstBasicHashRef ht, uintptr_t stack_key) {
    const RSBasicHashCallbacks *cb = RSBasicHashGetCallbacks(ht);
    RSHashCode (*hash)(const_any_pointer_t) = (RSHashCode (*)(const_any_pointer_t))cb->context[6];
    if (nil == hash) return stack_key;
    return (uintptr_t)INVOKE_CALLBACK1(hash, (const_any_pointer_t) stack_key);
}

static uintptr_t __RSDictionaryGetIndirectKey(RSConstBasicHashRef ht, uintptr_t coll_value) {
    return 0;
}

static RSStringRef __RSDictionaryCopyValueDescription(RSConstBasicHashRef ht, uintptr_t stack_value) {
    const RSBasicHashCallbacks *cb = RSBasicHashGetCallbacks(ht);
    RSStringRef (*value_describe)(const_any_pointer_t) = (RSStringRef (*)(const_any_pointer_t))cb->context[8];
    if (nil == value_describe) return RSStringCreateWithFormat(RSAllocatorSystemDefault, nil, RSSTR("<%p>"), (const_any_pointer_t) stack_value);
    return (RSStringRef)INVOKE_CALLBACK1(value_describe, (const_any_pointer_t) stack_value);
}

static RSStringRef __RSDictionaryCopyKeyDescription(RSConstBasicHashRef ht, uintptr_t stack_key) {
    const RSBasicHashCallbacks *cb = RSBasicHashGetCallbacks(ht);
    RSStringRef (*key_describe)(const_any_pointer_t) = (RSStringRef (*)(const_any_pointer_t))cb->context[9];
    if (nil == key_describe) return RSStringCreateWithFormat(RSAllocatorSystemDefault, nil, RSSTR("<%p>"), (const_any_pointer_t) stack_key);
    return (RSStringRef)INVOKE_CALLBACK1(key_describe, (const_any_pointer_t) stack_key);
}

static RSBasicHashRef __RSDictionaryCreateGeneric(RSAllocatorRef allocator, const RSHashKeyCallBacks *keyCallBacks, const RSHashValueCallBacks *valueCallBacks, BOOL useValueCB) {
    RSOptionFlags flags = RSBasicHashLinearHashing; // RSBasicHashExponentialHashing
    flags |= (RSDictionary ? RSBasicHashHasKeys : 0) | (RSBag ? RSBasicHashHasCounts : 0);
    
    RSBasicHashCallbacks *cb = nil;
    BOOL std_cb = false;
    uint16_t specialBits = 0;
    const_any_pointer_t (*key_retain)(RSAllocatorRef, const_any_pointer_t) = nil;
    void (*key_release)(RSAllocatorRef, const_any_pointer_t) = nil;
    const_any_pointer_t (*value_retain)(RSAllocatorRef, const_any_pointer_t) = nil;
    void (*value_release)(RSAllocatorRef, const_any_pointer_t) = nil;
    
    if ((nil == keyCallBacks) && (!useValueCB || nil == valueCallBacks))
    {
        BOOL keyRetainNull = nil == keyCallBacks || nil == keyCallBacks->retain;
        BOOL keyReleaseNull = nil == keyCallBacks || nil == keyCallBacks->release;
        BOOL keyEquateNull = nil == keyCallBacks || nil == keyCallBacks->equal;
        BOOL keyHashNull = nil == keyCallBacks || nil == keyCallBacks->hash;
        BOOL keyDescribeNull = nil == keyCallBacks || nil == keyCallBacks->description;
        
        BOOL valueRetainNull = (useValueCB && (nil == valueCallBacks || nil == valueCallBacks->retain)) || (!useValueCB && keyRetainNull);
        BOOL valueReleaseNull = (useValueCB && (nil == valueCallBacks || nil == valueCallBacks->release)) || (!useValueCB && keyReleaseNull);
        BOOL valueEquateNull = (useValueCB && (nil == valueCallBacks || nil == valueCallBacks->equal)) || (!useValueCB && keyEquateNull);
        BOOL valueDescribeNull = (useValueCB && (nil == valueCallBacks || nil == valueCallBacks->description)) || (!useValueCB && keyDescribeNull);
        
        BOOL keyRetainStd = keyRetainNull || __RSTypeCollectionRetain == keyCallBacks->retain;
        BOOL keyReleaseStd = keyReleaseNull || __RSTypeCollectionRelease == keyCallBacks->release;
        BOOL keyEquateStd = keyEquateNull || RSEqual == keyCallBacks->equal;
        BOOL keyHashStd = keyHashNull || RSHash == keyCallBacks->hash;
        BOOL keyDescribeStd = keyDescribeNull || RSDescription == keyCallBacks->description;
        
        BOOL valueRetainStd = (useValueCB && (valueRetainNull || __RSTypeCollectionRetain == valueCallBacks->retain)) || (!useValueCB && keyRetainStd);
        BOOL valueReleaseStd = (useValueCB && (valueReleaseNull || __RSTypeCollectionRelease == valueCallBacks->release)) || (!useValueCB && keyReleaseStd);
        BOOL valueEquateStd = (useValueCB && (valueEquateNull || RSEqual == valueCallBacks->equal)) || (!useValueCB && keyEquateStd);
        BOOL valueDescribeStd = (useValueCB && (valueDescribeNull || RSDescription == valueCallBacks->description)) || (!useValueCB && keyDescribeStd);
        
        if (keyRetainStd && keyReleaseStd && keyEquateStd && keyHashStd && keyDescribeStd && valueRetainStd && valueReleaseStd && valueEquateStd && valueDescribeStd) {
            cb = (RSBasicHashCallbacks *)&RSDictionaryStandardCallbacks;
            if (!(keyRetainNull || keyReleaseNull || keyEquateNull || keyHashNull || keyDescribeNull || valueRetainNull || valueReleaseNull || valueEquateNull || valueDescribeNull)) {
                std_cb = true;
            } else {
                // just set these to tickle the GC Strong logic below in a way that mimics past practice
                key_retain = keyCallBacks ? keyCallBacks->retain : nil;
                key_release = keyCallBacks ? keyCallBacks->release : nil;
                if (useValueCB) {
                    value_retain = valueCallBacks ? valueCallBacks->retain : nil;
                    value_release = valueCallBacks ? valueCallBacks->release : nil;
                } else {
                    value_retain = key_retain;
                    value_release = key_release;
                }
            }
            if (keyRetainNull) specialBits |= 0x0001;
            if (keyReleaseNull) specialBits |= 0x0002;
            if (keyEquateNull) specialBits |= 0x0004;
            if (keyHashNull) specialBits |= 0x0008;
            if (keyDescribeNull) specialBits |= 0x0010;
            if (valueRetainNull) specialBits |= 0x0100;
            if (valueReleaseNull) specialBits |= 0x0200;
            if (valueEquateNull) specialBits |= 0x0400;
            if (valueDescribeNull) specialBits |= 0x0800;
        }
    }
    
    if (!cb) {
        BOOL (*key_equal)(const_any_pointer_t, const_any_pointer_t) = nil;
        BOOL (*value_equal)(const_any_pointer_t, const_any_pointer_t) = nil;
        RSStringRef (*key_describe)(const_any_pointer_t) = nil;
        RSStringRef (*value_describe)(const_any_pointer_t) = nil;
        RSHashCode (*hash_key)(const_any_pointer_t) = nil;
        key_retain = keyCallBacks ? keyCallBacks->retain : nil;
        key_release = keyCallBacks ? keyCallBacks->release : nil;
        key_equal = keyCallBacks ? keyCallBacks->equal : nil;
        key_describe = keyCallBacks ? keyCallBacks->description : nil;
        if (useValueCB) {
            value_retain = valueCallBacks ? valueCallBacks->retain : nil;
            value_release = valueCallBacks ? valueCallBacks->release : nil;
            value_equal = valueCallBacks ? valueCallBacks->equal : nil;
            value_describe = valueCallBacks ? valueCallBacks->description : nil;
        } else {
            value_retain = key_retain;
            value_release = key_release;
            value_equal = key_equal;
            value_describe = key_describe;
        }
        hash_key = keyCallBacks ? keyCallBacks->hash : nil;
        
        RSBasicHashCallbacks *newcb = nil;
        newcb = (RSBasicHashCallbacks *)RSAllocatorAllocate(allocator, sizeof(RSBasicHashCallbacks) + 10 * sizeof(void *));
        if (!newcb) return nil;
        newcb->copyCallbacks = __RSDictionaryCopyCallbacks;
        newcb->freeCallbacks = __RSDictionaryFreeCallbacks;
        newcb->retainValue = __RSDictionaryRetainValue;
        newcb->retainKey = __RSDictionaryRetainKey;
        newcb->releaseValue = __RSDictionaryReleaseValue;
        newcb->releaseKey = __RSDictionaryReleaseKey;
        newcb->equateValues = __RSDictionaryEquateValues;
        newcb->equateKeys = __RSDictionaryEquateKeys;
        newcb->hashKey = __RSDictionaryHashKey;
        newcb->getIndirectKey = __RSDictionaryGetIndirectKey;
        newcb->copyValueDescription = __RSDictionaryCopyValueDescription;
        newcb->copyKeyDescription = __RSDictionaryCopyKeyDescription;
        newcb->context[0] = (uintptr_t)value_retain;
        newcb->context[1] = (uintptr_t)key_retain;
        newcb->context[2] = (uintptr_t)value_release;
        newcb->context[3] = (uintptr_t)key_release;
        newcb->context[4] = (uintptr_t)value_equal;
        newcb->context[5] = (uintptr_t)key_equal;
        newcb->context[6] = (uintptr_t)hash_key;
        newcb->context[8] = (uintptr_t)value_describe;
        newcb->context[9] = (uintptr_t)key_describe;
        cb = newcb;
    }
    
    if (RS_IS_COLLECTABLE_ALLOCATOR(allocator)) {
        if (std_cb || value_retain != nil || value_release != nil) {
            flags |= RSBasicHashStrongValues;
        }
        if (std_cb || key_retain != nil || key_release != nil) {
            flags |= RSBasicHashStrongKeys;
        }
#if RSDictionary
        if (valueCallBacks == &RSTypeDictionaryValueCompactableCallBacks) {
            // Foundation allocated collections will have compactable values
            flags |= RSBasicHashCompactableValues;
        }
#endif
    }
    
    RSBasicHashRef ht = RSBasicHashCreate(allocator, flags, cb);
    if (ht) RSBasicHashSetSpecialBits(ht, specialBits);
    if (!ht && !RS_IS_COLLECTABLE_ALLOCATOR(allocator)) RSAllocatorDeallocate(allocator, cb);
    return ht;
}

RSPrivate RSHashRef __RSDictionaryCreateTransfer(RSAllocatorRef allocator, const_any_pointer_t *klist, const_any_pointer_t *vlist, RSIndex numValues)
{
    RSTypeID typeID = RSDictionaryGetTypeID();
    RSAssert2(0 <= numValues, __RSLogAssertion, "%s(): numValues (%ld) cannot be less than zero", __PRETTY_FUNCTION__, numValues);
    RSOptionFlags flags = RSBasicHashLinearHashing; // RSBasicHashExponentialHashing
    flags |= (RSDictionary ? RSBasicHashHasKeys : 0) | (RSBag ? RSBasicHashHasCounts : 0);
    RSBasicHashCallbacks *cb = (RSBasicHashCallbacks *)&RSDictionaryStandardCallbacks;
    RSBasicHashRef ht = RSBasicHashCreate(allocator, flags, cb);
    RSBasicHashSetSpecialBits(ht, 0x0303);
    if (0 < numValues) RSBasicHashSetCapacity(ht, numValues);
    for (RSIndex idx = 0; idx < numValues; idx++) {
        RSBasicHashAddValue(ht, (uintptr_t)klist[idx], (uintptr_t)vlist[idx]);
    }
    RSBasicHashSetSpecialBits(ht, 0x0000);
    RSBasicHashMakeImmutable(ht);
    //    *(uintptr_t *)ht = __RSISAForTypeID(typeID);
    __RSRuntimeSetInstanceTypeID(ht, typeID);
    //    if (__RSOASafe) __RSSetLastAllocationEventName(ht, "RSDictionary (immutable)");
    return (RSHashRef)ht;
}
#elif (RSBasicHashVersion == 2)
static RSBasicHashRef __RSDictionaryCreateGeneric(RSAllocatorRef allocator, const RSHashKeyCallBacks *keyCallBacks, const RSHashValueCallBacks *valueCallBacks, BOOL useValueCB) {
    RSOptionFlags flags = RSBasicHashLinearHashing; // RSBasicHashExponentialHashing
    flags |= (RSDictionary ? RSBasicHashHasKeys : 0) | (RSBag ? RSBasicHashHasCounts : 0);
    
    if (RS_IS_COLLECTABLE_ALLOCATOR(allocator)) { // all this crap is just for figuring out two flags for GC in the way done historically; it probably simplifies down to three lines, but we let the compiler worry about that
        BOOL set_cb = false;
        BOOL std_cb = false;
        const_any_pointer_t (*key_retain)(RSAllocatorRef, const_any_pointer_t) = nil;
        void (*key_release)(RSAllocatorRef, const_any_pointer_t) = nil;
        const_any_pointer_t (*value_retain)(RSAllocatorRef, const_any_pointer_t) = nil;
        void (*value_release)(RSAllocatorRef, const_any_pointer_t) = nil;
        
        if ((nil == keyCallBacks) && (!useValueCB || nil == valueCallBacks)) {
            BOOL keyRetainNull = nil == keyCallBacks || nil == keyCallBacks->retain;
            BOOL keyReleaseNull = nil == keyCallBacks || nil == keyCallBacks->release;
            BOOL keyEquateNull = nil == keyCallBacks || nil == keyCallBacks->equal;
            BOOL keyHashNull = nil == keyCallBacks || nil == keyCallBacks->hash;
            BOOL keyDescribeNull = nil == keyCallBacks || nil == keyCallBacks->description;
            
            BOOL valueRetainNull = (useValueCB && (nil == valueCallBacks || nil == valueCallBacks->retain)) || (!useValueCB && keyRetainNull);
            BOOL valueReleaseNull = (useValueCB && (nil == valueCallBacks || nil == valueCallBacks->release)) || (!useValueCB && keyReleaseNull);
            BOOL valueEquateNull = (useValueCB && (nil == valueCallBacks || nil == valueCallBacks->equal)) || (!useValueCB && keyEquateNull);
            BOOL valueDescribeNull = (useValueCB && (nil == valueCallBacks || nil == valueCallBacks->description)) || (!useValueCB && keyDescribeNull);
            
            BOOL keyRetainStd = keyRetainNull || __RSTypeCollectionRetain == keyCallBacks->retain;
            BOOL keyReleaseStd = keyReleaseNull || __RSTypeCollectionRelease == keyCallBacks->release;
            BOOL keyEquateStd = keyEquateNull || RSEqual == keyCallBacks->equal;
            BOOL keyHashStd = keyHashNull || RSHash == keyCallBacks->hash;
            BOOL keyDescribeStd = keyDescribeNull || RSDescription == keyCallBacks->description;
            
            BOOL valueRetainStd = (useValueCB && (valueRetainNull || __RSTypeCollectionRetain == valueCallBacks->retain)) || (!useValueCB && keyRetainStd);
            BOOL valueReleaseStd = (useValueCB && (valueReleaseNull || __RSTypeCollectionRelease == valueCallBacks->release)) || (!useValueCB && keyReleaseStd);
            BOOL valueEquateStd = (useValueCB && (valueEquateNull || RSEqual == valueCallBacks->equal)) || (!useValueCB && keyEquateStd);
            BOOL valueDescribeStd = (useValueCB && (valueDescribeNull || RSDescription == valueCallBacks->description)) || (!useValueCB && keyDescribeStd);
            
            if (keyRetainStd && keyReleaseStd && keyEquateStd && keyHashStd && keyDescribeStd && valueRetainStd && valueReleaseStd && valueEquateStd && valueDescribeStd) {
                set_cb = true;
                if (!(keyRetainNull || keyReleaseNull || keyEquateNull || keyHashNull || keyDescribeNull || valueRetainNull || valueReleaseNull || valueEquateNull || valueDescribeNull)) {
                    std_cb = true;
                } else {
                    // just set these to tickle the GC Strong logic below in a way that mimics past practice
                    key_retain = keyCallBacks ? keyCallBacks->retain : nil;
                    key_release = keyCallBacks ? keyCallBacks->release : nil;
                    if (useValueCB) {
                        value_retain = valueCallBacks ? valueCallBacks->retain : nil;
                        value_release = valueCallBacks ? valueCallBacks->release : nil;
                    } else {
                        value_retain = key_retain;
                        value_release = key_release;
                    }
                }
            }
        }
        
        if (!set_cb) {
            key_retain = keyCallBacks ? keyCallBacks->retain : nil;
            key_release = keyCallBacks ? keyCallBacks->release : nil;
            if (useValueCB) {
                value_retain = valueCallBacks ? valueCallBacks->retain : nil;
                value_release = valueCallBacks ? valueCallBacks->release : nil;
            } else {
                value_retain = key_retain;
                value_release = key_release;
            }
        }
        
        if (std_cb || value_retain != nil || value_release != nil) {
            flags |= RSBasicHashStrongValues;
        }
        if (std_cb || key_retain != nil || key_release != nil) {
            flags |= RSBasicHashStrongKeys;
        }
    }
    
    
    RSBasicHashCallbacks callbacks;
    callbacks.retainKey = keyCallBacks ? (uintptr_t (*)(RSAllocatorRef, uintptr_t))keyCallBacks->retain : nil;
    callbacks.releaseKey = keyCallBacks ? (void (*)(RSAllocatorRef, uintptr_t))keyCallBacks->release : nil;
    callbacks.equateKeys = keyCallBacks ? (BOOL (*)(uintptr_t, uintptr_t))keyCallBacks->equal : nil;
    callbacks.hashKey = keyCallBacks ? (RSHashCode (*)(uintptr_t))keyCallBacks->hash : nil;
    callbacks.getIndirectKey = nil;
    callbacks.copyKeyDescription = keyCallBacks ? (RSStringRef (*)(uintptr_t))keyCallBacks->description : nil;
    callbacks.retainValue = useValueCB ? (valueCallBacks ? (uintptr_t (*)(RSAllocatorRef, uintptr_t))valueCallBacks->retain : nil) : (callbacks.retainKey);
    callbacks.releaseValue = useValueCB ? (valueCallBacks ? (void (*)(RSAllocatorRef, uintptr_t))valueCallBacks->release : nil) : (callbacks.releaseKey);
    callbacks.equateValues = useValueCB ? (valueCallBacks ? (BOOL (*)(uintptr_t, uintptr_t))valueCallBacks->equal : nil) : (callbacks.equateKeys);
    callbacks.copyValueDescription = useValueCB ? (valueCallBacks ? (RSStringRef (*)(uintptr_t))valueCallBacks->description : nil) : (callbacks.copyKeyDescription);
    
    RSBasicHashRef ht = RSBasicHashCreate(allocator, flags, &callbacks);
    return ht;
}

#if RSDictionary
RSPrivate RSHashRef __RSDictionaryCreateTransfer(RSAllocatorRef allocator, const_any_pointer_t *klist, const_any_pointer_t *vlist, RSIndex numValues)
#endif
#if RSSet || RSBag
RSPrivate RSHashRef __RSDictionaryCreateTransfer(RSAllocatorRef allocator, const_any_pointer_t *klist, RSIndex numValues)

#endif
{
#if RSSet || RSBag
    const_any_pointer_t *vlist = klist;
#endif
    RSTypeID typeID = RSDictionaryGetTypeID();
    RSAssert2(0 <= numValues, __RSLogAssertion, "%s(): numValues (%ld) cannot be less than zero", __PRETTY_FUNCTION__, numValues);
    RSOptionFlags flags = RSBasicHashLinearHashing; // RSBasicHashExponentialHashing
    flags |= (RSDictionary ? RSBasicHashHasKeys : 0) | (RSBag ? RSBasicHashHasCounts : 0);
    
    RSBasicHashCallbacks callbacks;
    callbacks.retainKey = (uintptr_t (*)(RSAllocatorRef, uintptr_t))RSTypeDictionaryKeyCallBacks.retain;
    callbacks.releaseKey = (void (*)(RSAllocatorRef, uintptr_t))RSTypeDictionaryKeyCallBacks.release;
    callbacks.equateKeys = (BOOL (*)(uintptr_t, uintptr_t))RSTypeDictionaryKeyCallBacks.equal;
    callbacks.hashKey = (RSHashCode (*)(uintptr_t))RSTypeDictionaryKeyCallBacks.hash;
    callbacks.getIndirectKey = nil;
    callbacks.copyKeyDescription = (RSStringRef (*)(uintptr_t))RSTypeDictionaryKeyCallBacks.description;
    callbacks.retainValue = RSDictionary ? (uintptr_t (*)(RSAllocatorRef, uintptr_t))RSTypeDictionaryValueCallBacks.retain : callbacks.retainKey;
    callbacks.releaseValue = RSDictionary ? (void (*)(RSAllocatorRef, uintptr_t))RSTypeDictionaryValueCallBacks.release : callbacks.releaseKey;
    callbacks.equateValues = RSDictionary ? (BOOL (*)(uintptr_t, uintptr_t))RSTypeDictionaryValueCallBacks.equal : callbacks.equateKeys;
    callbacks.copyValueDescription = RSDictionary ? (RSStringRef (*)(uintptr_t))RSTypeDictionaryValueCallBacks.description : callbacks.copyKeyDescription;
    
    RSBasicHashRef ht = RSBasicHashCreate(allocator, flags, &callbacks);
    RSBasicHashSuppressRC(ht);
    if (0 < numValues) RSBasicHashSetCapacity(ht, numValues);
    for (RSIndex idx = 0; idx < numValues; idx++) {
        RSBasicHashAddValue(ht, (uintptr_t)klist[idx], (uintptr_t)vlist[idx]);
    }
    RSBasicHashUnsuppressRC(ht);
    RSBasicHashMakeImmutable(ht);
    __RSRuntimeSetInstanceTypeID(ht, typeID);
    //        if (__RSOASafe) __RSSetLastAllocationEventName(ht, "RSDictionary (immutable)");
    return (RSHashRef)ht;
}
#endif
RSExport RSHashRef RSDictionaryCreate(RSAllocatorRef allocator, const_any_pointer_t *klist, const_any_pointer_t *vlist, const RSDictionaryContext *context, RSIndex numValues) {
    RSTypeID typeID = RSDictionaryGetTypeID();
    RSAssert2(0 <= numValues, __RSLogAssertion, "%s(): numValues (%ld) cannot be less than zero", __PRETTY_FUNCTION__, numValues);
    RSBasicHashRef ht = __RSDictionaryCreateGeneric(allocator, context->keyContext, context->valueContext, RSDictionary);
    if (!ht) return nil;
    if (0 < numValues) RSBasicHashSetCapacity(ht, numValues);
    for (RSIndex idx = 0; idx < numValues; idx++) {
        RSBasicHashAddValue(ht, (uintptr_t)klist[idx], (uintptr_t)vlist[idx]);
    }
    RSBasicHashMakeImmutable(ht);
    //    *(uintptr_t *)ht = __RSISAForTypeID(typeID);
    __RSRuntimeSetInstanceTypeID(ht, typeID);
    //    if (__RSOASafe) __RSSetLastAllocationEventName(ht, "RSDictionary (immutable)");
    return (RSHashRef)ht;
}

RSExport RSMutableHashRef RSDictionaryCreateMutable(RSAllocatorRef allocator, RSIndex capacity, const RSDictionaryContext *context) {
    RSTypeID typeID = RSDictionaryGetTypeID();
    RSAssert2(0 <= capacity, __RSLogAssertion, "%s(): capacity (%ld) cannot be less than zero", __PRETTY_FUNCTION__, capacity);
    RSBasicHashRef ht = __RSDictionaryCreateGeneric(allocator, context ? context->keyContext : nil, context ? context->valueContext : nil, RSDictionary);
    if (!ht) return nil;
    //    *(uintptr_t *)ht = __RSISAForTypeID(typeID);
    __RSRuntimeSetInstanceTypeID(ht, typeID);
    //    if (__RSOASafe) __RSSetLastAllocationEventName(ht, "RSDictionary (mutable)");
    return (RSMutableHashRef)ht;
}

RSExport RSHashRef RSDictionaryCreateCopy(RSAllocatorRef allocator, RSHashRef other)
{
    RSTypeID typeID = RSDictionaryGetTypeID();
    RSAssert1(other, __RSLogAssertion, "%s(): other RSDictionary cannot be nil", __PRETTY_FUNCTION__);
    __RSGenericValidInstance(other, typeID);
    RSBasicHashRef ht = nil;
    if (RS_IS_OBJC(typeID, other)) {
        RSIndex numValues = RSDictionaryGetCount(other);
        const_any_pointer_t vbuffer[256], kbuffer[256];
        const_any_pointer_t *vlist = (numValues <= 256) ? vbuffer : (const_any_pointer_t *)RSAllocatorAllocate(RSAllocatorSystemDefault, numValues * sizeof(const_any_pointer_t));
        const_any_pointer_t *klist = (numValues <= 256) ? kbuffer : (const_any_pointer_t *)RSAllocatorAllocate(RSAllocatorSystemDefault, numValues * sizeof(const_any_pointer_t));
        RSDictionaryGetKeysAndValues(other, klist, vlist);
        ht = __RSDictionaryCreateGeneric(allocator, & RSTypeDictionaryKeyCallBacks, RSDictionary ? & RSTypeDictionaryValueCallBacks : nil, RSDictionary);
        if (ht && 0 < numValues) RSBasicHashSetCapacity(ht, numValues);
        for (RSIndex idx = 0; ht && idx < numValues; idx++) {
            RSBasicHashAddValue(ht, (uintptr_t)klist[idx], (uintptr_t)vlist[idx]);
        }
        if (klist != kbuffer && klist != vlist) RSAllocatorDeallocate(RSAllocatorSystemDefault, klist);
        if (vlist != vbuffer) RSAllocatorDeallocate(RSAllocatorSystemDefault, vlist);
    } else {
        ht = RSBasicHashCreateCopy(allocator, (RSBasicHashRef)other);
    }
    if (!ht) return nil;
    RSBasicHashMakeImmutable(ht);
    //    *(uintptr_t *)ht = __RSISAForTypeID(typeID);
    __RSRuntimeSetInstanceTypeID(ht, typeID);
    //    if (__RSOASafe) __RSSetLastAllocationEventName(ht, "RSDictionary (immutable)");
    return (RSHashRef)ht;
}

RSExport RSMutableHashRef RSDictionaryCreateMutableCopy(RSAllocatorRef allocator, RSHashRef other) {
    RSTypeID typeID = RSDictionaryGetTypeID();
    RSAssert1(other, __RSLogAssertion, "%s(): other RSDictionary cannot be nil", __PRETTY_FUNCTION__);
    __RSGenericValidInstance(other, typeID);
    //    RSAssert2(0 <= capacity, __RSLogAssertion, "%s(): capacity (%ld) cannot be less than zero", __PRETTY_FUNCTION__, capacity);
    RSBasicHashRef ht = nil;
    if (RS_IS_OBJC(typeID, other)) {
        RSIndex numValues = RSDictionaryGetCount(other);
        const_any_pointer_t vbuffer[256], kbuffer[256];
        const_any_pointer_t *vlist = (numValues <= 256) ? vbuffer : (const_any_pointer_t *)RSAllocatorAllocate(RSAllocatorSystemDefault, numValues * sizeof(const_any_pointer_t));
        const_any_pointer_t *klist = (numValues <= 256) ? kbuffer : (const_any_pointer_t *)RSAllocatorAllocate(RSAllocatorSystemDefault, numValues * sizeof(const_any_pointer_t));
        RSDictionaryGetKeysAndValues(other, klist, vlist);
        ht = __RSDictionaryCreateGeneric(allocator, & RSTypeDictionaryKeyCallBacks, RSDictionary ? & RSTypeDictionaryValueCallBacks : nil, RSDictionary);
        if (ht && 0 < numValues) RSBasicHashSetCapacity(ht, numValues);
        for (RSIndex idx = 0; ht && idx < numValues; idx++) {
            RSBasicHashAddValue(ht, (uintptr_t)klist[idx], (uintptr_t)vlist[idx]);
        }
        if (klist != kbuffer && klist != vlist) RSAllocatorDeallocate(RSAllocatorSystemDefault, klist);
        if (vlist != vbuffer) RSAllocatorDeallocate(RSAllocatorSystemDefault, vlist);
    } else {
        ht = RSBasicHashCreateCopy(allocator, (RSBasicHashRef)other);
    }
    if (!ht) return nil;
    //    *(uintptr_t *)ht = __RSISAForTypeID(typeID);
    __RSRuntimeSetInstanceTypeID(ht, typeID);
    //    if (__RSOASafe) __RSSetLastAllocationEventName(ht, "RSDictionary (mutable)");
    return (RSMutableHashRef)ht;
}

RSExport RSIndex RSDictionaryGetCount(RSHashRef hc) {
    if (RSDictionary) RS_OBJC_FUNCDISPATCHV(__RSDictionaryTypeID, RSIndex, (NSDictionary *)hc, count);
    if (RSSet) RS_OBJC_FUNCDISPATCHV(__RSDictionaryTypeID, RSIndex, (NSSet *)hc, count);
    __RSGenericValidInstance(hc, _RSDictionaryTypeID);
    return RSBasicHashGetCount((RSBasicHashRef)hc);
}

RSExport RSIndex RSDictionaryGetCountOfKey(RSHashRef hc, const_any_pointer_t key) {
    if (RSDictionary) RS_OBJC_FUNCDISPATCHV(__RSDictionaryTypeID, RSIndex, (NSDictionary *)hc, countForKey:(id)key);
    if (RSSet) RS_OBJC_FUNCDISPATCHV(__RSDictionaryTypeID, RSIndex, (NSSet *)hc, countForObject:(id)key);
    __RSGenericValidInstance(hc, _RSDictionaryTypeID);
    return RSBasicHashGetCountOfKey((RSBasicHashRef)hc, (uintptr_t)key);
}


RSExport BOOL RSDictionaryContainsKey(RSHashRef hc, const_any_pointer_t key) {
    
    if (RSDictionary) RS_OBJC_FUNCDISPATCHV(__RSDictionaryTypeID, char, (NSDictionary *)hc, containsKey:(id)key);
    if (RSSet) RS_OBJC_FUNCDISPATCHV(__RSDictionaryTypeID, char, (NSSet *)hc, containsObject:(id)key);
    __RSGenericValidInstance(hc, _RSDictionaryTypeID);
    return (0 < RSBasicHashGetCountOfKey((RSBasicHashRef)hc, (uintptr_t)key));
}

RSExport const_any_pointer_t RSDictionaryGetValue(RSHashRef hc, const_any_pointer_t key) {
    if (RSDictionary) RS_OBJC_FUNCDISPATCHV(__RSDictionaryTypeID, const_any_pointer_t, (NSDictionary *)hc, objectForKey:(id)key);
    if (RSSet) RS_OBJC_FUNCDISPATCHV(__RSDictionaryTypeID, const_any_pointer_t, (NSSet *)hc, member:(id)key);
    __RSGenericValidInstance(hc, _RSDictionaryTypeID);
    RSBasicHashBucket bkt = RSBasicHashFindBucket((RSBasicHashRef)hc, (uintptr_t)key);
    return (0 < bkt.count ? (const_any_pointer_t)bkt.weak_value : 0);
}

RSExport BOOL RSDictionaryGetValueIfPresent(RSHashRef hc, const_any_pointer_t key, const_any_pointer_t *value) {
    if (RSDictionary) RS_OBJC_FUNCDISPATCHV(__RSDictionaryTypeID, BOOL, (NSDictionary *)hc, __getValue:(id *)value forKey:(id)key);
    if (RSSet) RS_OBJC_FUNCDISPATCHV(__RSDictionaryTypeID, BOOL, (NSSet *)hc, __getValue:(id *)value forObj:(id)key);
    __RSGenericValidInstance(hc, _RSDictionaryTypeID);
    RSBasicHashBucket bkt = RSBasicHashFindBucket((RSBasicHashRef)hc, (uintptr_t)key);
    if (0 < bkt.count) {
        if (value) {
            if (RSUseCollectableAllocator && (RSBasicHashGetFlags((RSBasicHashRef)hc) & RSBasicHashStrongValues)) {
                __RSAssignWithWriteBarrier((void **)value, (void *)bkt.weak_value);
            } else {
                *value = (const_any_pointer_t)bkt.weak_value;
            }
        }
        return true;
    }
    return false;
}

RSExport RSIndex RSDictionaryGetCountOfValue(RSHashRef hc, const_any_pointer_t value) {
    RS_OBJC_FUNCDISPATCHV(__RSDictionaryTypeID, RSIndex, (NSDictionary *)hc, countForObject:(id)value);
    __RSGenericValidInstance(hc, _RSDictionaryTypeID);
    return RSBasicHashGetCountOfValue((RSBasicHashRef)hc, (uintptr_t)value);
}

RSExport BOOL RSDictionaryContainsValue(RSHashRef hc, const_any_pointer_t value) {
    RS_OBJC_FUNCDISPATCHV(__RSDictionaryTypeID, char, (NSDictionary *)hc, containsObject:(id)value);
    __RSGenericValidInstance(hc, _RSDictionaryTypeID);
    return (0 < RSBasicHashGetCountOfValue((RSBasicHashRef)hc, (uintptr_t)value));
}

RSExport BOOL RSDictionaryGetKeyIfPresent(RSHashRef hc, const_any_pointer_t key, const_any_pointer_t *actualkey) {
    __RSGenericValidInstance(hc, _RSDictionaryTypeID);
    RSBasicHashBucket bkt = RSBasicHashFindBucket((RSBasicHashRef)hc, (uintptr_t)key);
    if (0 < bkt.count) {
        if (actualkey) {
            if (RSUseCollectableAllocator && (RSBasicHashGetFlags((RSBasicHashRef)hc) & RSBasicHashStrongKeys)) {
                __RSAssignWithWriteBarrier((void **)actualkey, (void *)bkt.weak_key);
            } else {
                *actualkey = (const_any_pointer_t)bkt.weak_key;
            }
        }
        return true;
    }
    return false;
}

RSExport void RSDictionaryGetKeysAndValues(RSHashRef hc, const_any_pointer_t *keybuf, const_any_pointer_t *valuebuf)
{
    if (RSDictionary) RS_OBJC_FUNCDISPATCHV(__RSDictionaryTypeID, void, (NSDictionary *)hc, getObjects:(id *)valuebuf andKeys:(id *)keybuf);
    if (RSSet) RS_OBJC_FUNCDISPATCHV(__RSDictionaryTypeID, void, (NSSet *)hc, getObjects:(id *)keybuf);
    __RSGenericValidInstance(hc, _RSDictionaryTypeID);
    if (RSUseCollectableAllocator) {
        RSOptionFlags flags = RSBasicHashGetFlags((RSBasicHashRef)hc);
        __block const_any_pointer_t *keys = keybuf;
        __block const_any_pointer_t *values = valuebuf;
        RSBasicHashApply((RSBasicHashRef)hc, ^(RSBasicHashBucket bkt) {
            for (RSIndex cnt = bkt.count; cnt--;) {
                if (keybuf && (flags & RSBasicHashStrongKeys)) { __RSAssignWithWriteBarrier((void **)keys, (void *)bkt.weak_key); keys++; }
                if (keybuf && !(flags & RSBasicHashStrongKeys)) { *keys++ = (const_any_pointer_t)bkt.weak_key; }
                if (valuebuf && (flags & RSBasicHashStrongValues)) { __RSAssignWithWriteBarrier((void **)values, (void *)bkt.weak_value); values++; }
                if (valuebuf && !(flags & RSBasicHashStrongValues)) { *values++ = (const_any_pointer_t)bkt.weak_value; }
            }
            return (BOOL)true;
        });
    } else {
        RSBasicHashGetElements((RSBasicHashRef)hc, RSDictionaryGetCount(hc), (uintptr_t *)valuebuf, (uintptr_t *)keybuf);
    }
}

RSExport void RSDictionaryApplyFunction(RSHashRef hc, RSDictionaryApplierFunction applier, any_pointer_t context) {
    FAULT_CALLBACK((void **)&(applier));
    if (RSDictionary) RS_OBJC_FUNCDISPATCHV(__RSDictionaryTypeID, void, (NSDictionary *)hc, __apply:(void (*)(const void *, const void *, void *))applier context:(void *)context);
    if (RSSet) RS_OBJC_FUNCDISPATCHV(__RSDictionaryTypeID, void, (NSSet *)hc, __applyValues:(void (*)(const void *, void *))applier context:(void *)context);
    __RSGenericValidInstance(hc, _RSDictionaryTypeID);
    RSBasicHashApply((RSBasicHashRef)hc, ^(RSBasicHashBucket bkt) {
        INVOKE_CALLBACK3(applier, (const_any_pointer_t)bkt.weak_key, (const_any_pointer_t)bkt.weak_value, context);
        return (BOOL)true;
    });
}

RSExport void RSDictionaryApply(RSHashRef hc, void (^applier)(const void *key, const void* value, void *context), any_pointer_t context) {
    FAULT_CALLBACK((void **)&(applier));
    if (RSDictionary) RS_OBJC_FUNCDISPATCHV(__RSDictionaryTypeID, void, (NSDictionary *)hc, __apply:(void (*)(const void *, const void *, void *))applier context:(void *)context);
    if (RSSet) RS_OBJC_FUNCDISPATCHV(__RSDictionaryTypeID, void, (NSSet *)hc, __applyValues:(void (*)(const void *, void *))applier context:(void *)context);
    __RSGenericValidInstance(hc, _RSDictionaryTypeID);
    RSBasicHashApply((RSBasicHashRef)hc, ^(RSBasicHashBucket bkt) {
        INVOKE_CALLBACK3(applier, (const_any_pointer_t)bkt.weak_key, (const_any_pointer_t)bkt.weak_value, context);
        return (BOOL)true;
    });
}

// This function is for Foundation's benefit; no one else should use it.
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wvisibility"
RSExport unsigned long _RSDictionaryFastEnumeration(RSHashRef hc, struct __objcFastEnumerationStateEquivalent *state, void *stackbuffer, unsigned long count) {
    if (RS_IS_OBJC(_RSDictionaryTypeID, hc)) return 0;
    __RSGenericValidInstance(hc, _RSDictionaryTypeID);
    return __RSBasicHashFastEnumeration((RSBasicHashRef)hc, (struct __RSFastEnumerationStateEquivalent2 *)state, stackbuffer, count);
}
#pragma clang diagnostic pop

// This function is for Foundation's benefit; no one else should use it.
RSExport BOOL _RSDictionaryIsMutable(RSHashRef hc) {
    if (RS_IS_OBJC(_RSDictionaryTypeID, hc)) return false;
    __RSGenericValidInstance(hc, _RSDictionaryTypeID);
    return RSBasicHashIsMutable((RSBasicHashRef)hc);
}

// This function is for Foundation's benefit; no one else should use it.
RSExport void _RSDictionarySetCapacity(RSMutableHashRef hc, RSIndex cap) {
    if (RS_IS_OBJC(_RSDictionaryTypeID, hc)) return;
    __RSGenericValidInstance(hc, _RSDictionaryTypeID);
    RSAssert2(RSBasicHashIsMutable((RSBasicHashRef)hc), __RSLogAssertion, "%s(): immutable collection %p passed to mutating operation", __PRETTY_FUNCTION__, hc);
    RSAssert3(RSDictionaryGetCount(hc) <= cap, __RSLogAssertion, "%s(): desired capacity (%ld) is less than count (%ld)", __PRETTY_FUNCTION__, cap, RSDictionaryGetCount(hc));
    RSBasicHashSetCapacity((RSBasicHashRef)hc, cap);
}

RSInline RSIndex __RSDictionaryGetKVOBit(RSHashRef hc) {
    return __RSBitfieldGetValue(RSRuntimeClassBaseFiled(hc), 0, 0);
}

RSInline void __RSDictionarySetKVOBit(RSHashRef hc, RSIndex bit) {
    __RSBitfieldSetValue(RSRuntimeClassBaseFiled(hc), 0, 0, ((uintptr_t)bit & 0x1));
}

// This function is for Foundation's benefit; no one else should use it.
RSExport RSIndex _RSDictionaryGetKVOBit(RSHashRef hc) {
    return __RSDictionaryGetKVOBit(hc);
}

// This function is for Foundation's benefit; no one else should use it.
RSExport void _RSDictionarySetKVOBit(RSHashRef hc, RSIndex bit) {
    __RSDictionarySetKVOBit(hc, bit);
}


#if !defined(RS_OBJC_KVO_WILLCHANGE)
#define RS_OBJC_KVO_WILLCHANGE(obj, key)
#define RS_OBJC_KVO_DIDCHANGE(obj, key)
#define RS_OBJC_KVO_WILLCHANGEALL(obj)
#define RS_OBJC_KVO_DIDCHANGEALL(obj)
#endif

void RSDictionaryAddValue(RSMutableHashRef hc, const_any_pointer_t key, const_any_pointer_t value) {
    
    if (RSDictionary) RS_OBJC_FUNCDISPATCHV(__RSDictionaryTypeID, void, (NSMutableDictionary *)hc, __addObject:(id)value forKey:(id)key);
    if (RSSet) RS_OBJC_FUNCDISPATCHV(__RSDictionaryTypeID, void, (NSMutableSet *)hc, addObject:(id)key);
    __RSGenericValidInstance(hc, _RSDictionaryTypeID);
    RSAssert2(RSBasicHashIsMutable((RSBasicHashRef)hc), __RSLogAssertion, "%s(): immutable collection %p passed to mutating operation", __PRETTY_FUNCTION__, hc);
    if (!RSBasicHashIsMutable((RSBasicHashRef)hc)) {
        RSLog(RSSTR("%s(): immutable collection %p given to mutating function"), __PRETTY_FUNCTION__, hc);
    }
    RS_OBJC_KVO_WILLCHANGE(hc, key);
    RSBasicHashAddValue((RSBasicHashRef)hc, (uintptr_t)key, (uintptr_t)value);
    RS_OBJC_KVO_DIDCHANGE(hc, key);
}


void RSDictionaryReplaceValue(RSMutableHashRef hc, const_any_pointer_t key, const_any_pointer_t value) {
    if (RSDictionary) RS_OBJC_FUNCDISPATCHV(__RSDictionaryTypeID, void, (NSMutableDictionary *)hc, replaceObject:(id)value forKey:(id)key);
    if (RSSet) RS_OBJC_FUNCDISPATCHV(__RSDictionaryTypeID, void, (NSMutableSet *)hc, replaceObject:(id)key);
    __RSGenericValidInstance(hc, _RSDictionaryTypeID);
    RSAssert2(RSBasicHashIsMutable((RSBasicHashRef)hc), __RSLogAssertion, "%s(): immutable collection %p passed to mutating operation", __PRETTY_FUNCTION__, hc);
    if (!RSBasicHashIsMutable((RSBasicHashRef)hc)) {
        RSLog(RSSTR("%s(): immutable collection %p given to mutating function"), __PRETTY_FUNCTION__, hc);
    }
    RS_OBJC_KVO_WILLCHANGE(hc, key);
    RSBasicHashReplaceValue((RSBasicHashRef)hc, (uintptr_t)key, (uintptr_t)value);
    RS_OBJC_KVO_DIDCHANGE(hc, key);
}

void RSDictionarySetValue(RSMutableHashRef hc, const_any_pointer_t key, const_any_pointer_t value) {
    
    if (RSDictionary) RS_OBJC_FUNCDISPATCHV(__RSDictionaryTypeID, void, (NSMutableDictionary *)hc, __setObject:(id)value forKey:(id)key);
    if (RSSet) RS_OBJC_FUNCDISPATCHV(__RSDictionaryTypeID, void, (NSMutableSet *)hc, setObject:(id)key);
    __RSGenericValidInstance(hc, _RSDictionaryTypeID);
    RSAssert2(RSBasicHashIsMutable((RSBasicHashRef)hc), __RSLogAssertion, "%s(): immutable collection %p passed to mutating operation", __PRETTY_FUNCTION__, hc);
    if (!RSBasicHashIsMutable((RSBasicHashRef)hc)) {
        RSLog(RSSTR("%s(): immutable collection %p given to mutating function"), __PRETTY_FUNCTION__, hc);
    }
    RS_OBJC_KVO_WILLCHANGE(hc, key);
    //#warning this for a dictionary used to not replace the key
    RSBasicHashSetValue((RSBasicHashRef)hc, (uintptr_t)key, (uintptr_t)value);
    RS_OBJC_KVO_DIDCHANGE(hc, key);
}

void RSDictionaryRemoveValue(RSMutableHashRef hc, const_any_pointer_t key) {
    if (RSDictionary) RS_OBJC_FUNCDISPATCHV(__RSDictionaryTypeID, void, (NSMutableDictionary *)hc, removeObjectForKey:(id)key);
    if (RSSet) RS_OBJC_FUNCDISPATCHV(__RSDictionaryTypeID, void, (NSMutableSet *)hc, removeObject:(id)key);
    __RSGenericValidInstance(hc, _RSDictionaryTypeID);
    RSAssert2(RSBasicHashIsMutable((RSBasicHashRef)hc), __RSLogAssertion, "%s(): immutable collection %p passed to mutating operation", __PRETTY_FUNCTION__, hc);
    if (!RSBasicHashIsMutable((RSBasicHashRef)hc)) {
        RSLog(RSSTR("%s(): immutable collection %p given to mutating function"), __PRETTY_FUNCTION__, hc);
    }
    RS_OBJC_KVO_WILLCHANGE(hc, key);
    RSBasicHashRemoveValue((RSBasicHashRef)hc, (uintptr_t)key);
    RS_OBJC_KVO_DIDCHANGE(hc, key);
}

void RSDictionaryRemoveAllObjects(RSMutableHashRef hc) {
    if (RSDictionary) RS_OBJC_FUNCDISPATCHV(__RSDictionaryTypeID, void, (NSMutableDictionary *)hc, removeAllObjects);
    if (RSSet) RS_OBJC_FUNCDISPATCHV(__RSDictionaryTypeID, void, (NSMutableSet *)hc, removeAllObjects);
    __RSGenericValidInstance(hc, _RSDictionaryTypeID);
    RSAssert2(RSBasicHashIsMutable((RSBasicHashRef)hc), __RSLogAssertion, "%s(): immutable collection %p passed to mutating operation", __PRETTY_FUNCTION__, hc);
    if (!RSBasicHashIsMutable((RSBasicHashRef)hc)) {
        RSLog(RSSTR("%s(): immutable collection %p given to mutating function"), __PRETTY_FUNCTION__, hc);
    }
    RS_OBJC_KVO_WILLCHANGEALL(hc);
    RSBasicHashRemoveAllValues((RSBasicHashRef)hc);
    RS_OBJC_KVO_DIDCHANGEALL(hc);
}

RSExport RSArrayRef RSDictionaryAllKeys(RSHashRef hc)
{
    RSIndex cnt = RSDictionaryGetCount(hc);
    if (cnt == 0) return RSArrayCreate(RSAllocatorSystemDefault, nil);
    STACK_BUFFER_DECL(RSTypeRef, keys, cnt);
    RSDictionaryGetKeysAndValues(hc, keys, nil);
    RSMutableArrayRef array = RSArrayCreateMutable(RSAllocatorSystemDefault, 0);
    for (RSUInteger idx = 0; idx < cnt; ++idx) {
        RSArrayAddObject(array, keys[idx]);
    }
    return array;
}

RSExport RSArrayRef RSDictionaryAllValues(RSHashRef hc)
{
    RSIndex cnt = RSDictionaryGetCount(hc);
    if (cnt == 0) return RSArrayCreate(RSAllocatorSystemDefault, nil);
    STACK_BUFFER_DECL(RSTypeRef, values, cnt);
    RSDictionaryGetKeysAndValues(hc, nil, values);
    RSMutableArrayRef array = RSArrayCreateMutable(RSAllocatorSystemDefault, 0);
    for (RSUInteger idx = 0; idx < cnt; ++idx) {
        RSArrayAddObject(array, values[idx]);
    }
    return array;
}

RSExport RSHashRef RSDictionaryCreateWithArray(RSAllocatorRef allocator, RSArrayRef keys, RSArrayRef values, const RSDictionaryContext* context)
{
    if (!(keys && values)) return RSDictionaryCreate(allocator, nil, nil, context, 0);
    RSIndex cnt = RSArrayGetCount(keys);
    if (cnt != RSArrayGetCount(values)) HALTWithError(RSInvalidArgumentException, "the number of keys and values is not equal.");
    RSMutableDictionaryRef dictionary = RSDictionaryCreateMutable(RSAllocatorSystemDefault, 0, context);
    RSRetain(keys); RSRetain(values);
    for (RSIndex idx = 0; idx < cnt; idx++)
    {
        RSTypeRef key = RSArrayObjectAtIndex(keys, idx);
        RSTypeRef value = RSArrayObjectAtIndex(values, idx);
        RSDictionarySetValue(dictionary, key, value);
    }
    RSRelease(keys);RSRelease(values);
    RSBasicHashMakeImmutable((RSBasicHashRef)dictionary);
    return dictionary;
}

RSExport RSMutableDictionaryRef RSDictionaryCreateWithObjectsAndOKeys(RSAllocatorRef allocator, RSTypeRef obj, ...)
{
    RSStringRef key = nil;
    RSTypeRef value = obj;
    RSMutableDictionaryRef dict = RSDictionaryCreateMutable(allocator, 0, RSDictionaryRSTypeContext);
    va_list va;
    va_start(va, obj);
    value = obj;
    if (value == nil)
    {
        va_end(va);
        return dict;
    }
    for (;;)
    {
        key = va_arg(va, RSStringRef);
        RSDictionarySetValue(dict, key, value);
        value = va_arg(va, RSTypeRef);
        if (value == nil)
        {
            va_end(va);
            return dict;
        }
    }
    va_end(va);
    return dict;
}

RSPrivate void __RSDictionaryCleanAllObjects(RSMutableDictionaryRef dictionary)
{
    //    rbtree_container* rbt = __RSDictionaryRoot(dictionary);
    //    rbtree_container_node* pc = rbtree_container_first(rbt), *tpc = nil;
    //    //RSIndex refcnt = 0;
    //    while (pc)
    //    {
    //        tpc = pc;
    //        //__RSCLog(RSLogLevelDebug, "%s\n",tpc->key);
    //        //1 << 3
    //        RSDeallocateInstance(tpc->value);
    //        //RSRelease(tpc->value);
    //        pc = rbtree_container_next(pc);
    //        rbtree_container_erase(rbt, tpc);
    //		rbtree_container_node_free(tpc);
    //    }
    RSDictionaryApply(dictionary, ^(const void *key, const void *value, void *context) {
        __RSRuntimeSetInstanceSpecial(value, NO);
    }, nil);
    
    RSDictionaryRemoveAllObjects(dictionary);
}

#undef RS_OBJC_KVO_WILLCHANGE
#undef RS_OBJC_KVO_DIDCHANGE
#undef RS_OBJC_KVO_WILLCHANGEALL
#undef RS_OBJC_KVO_DIDCHANGEALL

#endif