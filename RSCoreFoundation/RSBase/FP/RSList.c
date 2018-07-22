//
//  RSList.c
//  RSCoreFoundation
//
//  Created by closure on 1/30/14.
//  Copyright (c) 2014 RetVal. All rights reserved.
//

#include "RSList.h"

#include <RSCoreFoundation/RSRuntime.h>
#include "RSKVBucket.h"
#include "RSMultidimensionalDictionary.h"

static RSDictionaryRef __RSListPreferences = nil;   // read only

typedef const struct __RSNode *RSNodeRef;
struct __RSNode {
    RSRuntimeBase _base;
    RSTypeRef _value;
    RSNodeRef _next;
};

static RSTypeRef __RSNodeGetValue(RSNodeRef node) {
    return node ? node->_value : nil;
}

static RSNodeRef __RSNodeCreateWithNext(RSTypeRef value, RSNodeRef n, BOOL nIsHead);

static RSSpinLock __RSNodePoolLock = RSSpinLockInit;
static RSMultidimensionalDictionaryRef __RSNodePool = nil;

static void __RSNodePoolInitialize() {
    RSSyncUpdateBlock(&__RSNodePoolLock, ^{
        if (!__RSNodePool) {
            __RSNodePool = RSMultidimensionalDictionaryCreate(RSAllocatorSystemDefault, 2); // create 2 dimensional dictionary [[value, next], node]
            __RSListPreferences = RSDictionaryCreateWithContentOfPath(RSAllocatorSystemDefault, RSFileManagerStandardizingPath(RSSTR("~/Library/Preferences/com.retval.RSCoreFoundation.list.node.plist")));
        }
    });
}

static void __RSNodePoolDeallocate() {
    RSSyncUpdateBlock(&__RSNodePoolLock, ^{
        if (__RSNodePool) {
//            RSShow(__RSNodePool);
            extern void __RSDictionaryCleanAllObjects(RSMutableDictionaryRef dictionary);
//            __RSCLog(RSLogLevelWarning, "RSNil -> %p\n", RSNil);
            RSDictionaryRef dimension = RSMultidimensionalDictionaryGetDimensionEntry(__RSNodePool);
            RSDictionaryApplyBlock(dimension, ^(const void *key1, const void *nd_dimension, BOOL *stop) {
//                RSLog(RSSTR("1 : %r<%p> - %r<%p>"), key1, key1, nd_dimension, nd_dimension);
                RSDictionaryApplyBlock(nd_dimension, ^(const void *key2, const void *value, BOOL *stop) {
//                    RSNodeRef n = value;
//                    RSLog(RSSTR("value <%p> - (%r -> %r)"), n, n, n->_next);
//                    RSLog(RSSTR("dimension1 -> %r, dimension2 -> %r (%r - rc -> %lld)"), key1, key2, value, RSGetRetainCount(value));
                    __RSRuntimeSetInstanceSpecial(value, NO);
                });
            });
//            RSShow(__RSNodePool);
            RSRelease(__RSNodePool);
            __RSNodePool = nil;
            RSRelease(__RSListPreferences);
        }
    });
}

static RSNodeRef __RSNodePoolGetNode(RSTypeRef value, RSNodeRef next, BOOL createAsHead) {
    __block RSNodeRef n = nil;
    RSSyncUpdateBlock(&__RSNodePoolLock, ^{
        n = RSMultidimensionalDictionaryGetValue(__RSNodePool, value, next ? : (RSNodeRef)RSNil);
        if (n && (!(n->_next) || (n->_next && RSEqual(n->_next, next)))) {
            return;
        } else if (n) {
            n = nil;
        }
        if (!n) {
            n = __RSNodeCreateWithNext(value, next, createAsHead);
            RSMultidimensionalDictionarySetValue(__RSNodePool, n, value, next ? : (RSNodeRef)RSNil);
            RSRelease(n);
//            RSLog(RSSTR("%r -> %r, <%p>(%ld) set special"), n, n->_next, n, RSGetRetainCount(n));
            __RSRuntimeSetInstanceSpecial(n, YES);
        }
    });
    return n;
}

static void __RSNodeClassDeallocate(RSTypeRef rs)
{
    RSNodeRef node = (RSNodeRef)rs;
    RSRelease(node->_value);
//    RSRelease(node->_next);
}

static BOOL __RSNodeClassEqual(RSTypeRef rs1, RSTypeRef rs2)
{
    RSNodeRef RSNode1 = (RSNodeRef)rs1;
    RSNodeRef RSNode2 = (RSNodeRef)rs2;
    BOOL result = NO;
    
    result = RSEqual(RSNode1->_value, RSNode2->_value);
    if (result) {
        if ((RSNode1->_next && !RSNode2->_next) || (!RSNode1->_next && RSNode2->_next)) {
            result = NO;
        } else if (RSNode1->_next) {
            result = RSEqual(RSNode1->_next->_value, RSNode2->_next->_value) && RSEqual(RSNode1->_next->_next, RSNode2->_next->_next);
//            if (result) {
//                if ((RSNode1->_next->_next && !RSNode2->_next->_next) || (!RSNode1->_next->_next && RSNode2->_next->_next)) {
//                    result = NO;
//                } else if (RSNode1->_next) {
//                    result = RSEqual(RSNode1->_next->_next->_value, RSNode2->_next->_next->_value);
//                }
//            }
        }
    }
    return result;
}

static RSHashCode __RSNodeClassHash(RSTypeRef rs) {
    RSNodeRef node = (RSNodeRef)rs;
    if (node->_next)
        return RSHash(node->_value) + RSHash(node->_next->_value);
    return RSHash(node->_value);
}

static RSStringRef __RSNodeClassDescription(RSTypeRef rs)
{
    RSNodeRef node = (RSNodeRef)rs;
    if (YES == RSNumberBooleanValue(RSDictionaryGetValue(__RSListPreferences, RSSTR("Node")))) {
        RSStringRef description = RSStringCreateWithFormat(RSAllocatorSystemDefault, RSSTR("(%r -> %r)"), node->_value, node->_next);
        return description;
    }
    return RSDescription(node->_value);
}

static RSRuntimeClass __RSNodeClass =
{
    _RSRuntimeScannedObject,
    0,
    "RSNode",
    nil,
    nil,
    __RSNodeClassDeallocate,
    __RSNodeClassEqual,
    __RSNodeClassHash,
    __RSNodeClassDescription,
    nil,
    nil
};

static RSTypeID _RSNodeTypeID = _RSRuntimeNotATypeID;
static void __RSNodeInitialize(void);

RSExport RSTypeID RSNodeGetTypeID(void)
{
    static dispatch_once_t onceToken;
    dispatch_once(&onceToken, ^{
        __RSNodeInitialize();
    });
    return _RSNodeTypeID;
}

static void __RSNodeInitialize(void)
{
    _RSNodeTypeID = __RSRuntimeRegisterClass(&__RSNodeClass);
    __RSRuntimeSetClassTypeID(&__RSNodeClass, _RSNodeTypeID);
}

struct __RSList {
    RSRuntimeBase _base;
    RSUInteger _count;
//    RSUInteger _pp;
    RSNodeRef _head;
};

RSInline RSNodeRef __RSNodeSetupNext(RSNodeRef node, RSNodeRef next) {
    struct __RSNode *mutableNode = (struct __RSNode *)node;
//    if (mutableNode->_next) RSRelease(mutableNode->_next);
//    mutableNode->_next = RSRetain(next);
    if (next && __RSRuntimeInstanceIsSpecial(next)) {
        __RSRuntimeSetInstanceSpecial(next, NO);
        RSRetain(next);
        __RSRuntimeSetInstanceSpecial(next, YES);
    }
    mutableNode->_next = next;
    return node;
}

static RSNodeRef __RSNodeCreateWithNext(RSTypeRef value, RSNodeRef n, BOOL nIsHead) {
    struct __RSNode *node = (struct __RSNode *)__RSRuntimeCreateInstance(RSAllocatorSystemDefault, RSNodeGetTypeID(), sizeof(struct __RSNode) - sizeof(RSRuntimeBase));
    node->_value = RSRetain(value);
    node->_next = nil;
    if (nIsHead == YES && n) {
        struct __RSNode *nn = (struct __RSNode *)n;
        if (nn->_next) {
            __RSNodeSetupNext(node, nn->_next);
        }
        __RSNodeSetupNext(nn, node);
    } else if (nIsHead == NO) {
        __RSNodeSetupNext(node, n);
    }
    return node;
}

static RSNodeRef __RSNodeCreate(RSTypeRef value) {
    return __RSNodePoolGetNode(value, nil, NO);
}

RSInline RSListRef __RSListSetupHead(RSListRef list, RSNodeRef head) {
    struct __RSList *mutableList = (struct __RSList*)list;
//    if (mutableList->_head) RSRelease(mutableList->_head);
    if (head) {
//        mutableList->_pp ++;
//        mutableList->_head = RSRetain(head);
        mutableList->_head = head;
    }
    else mutableList->_head = nil;
    return list;
}

RSInline RSListRef __RSListIncNodeCount(RSListRef list) {
    struct __RSList *coll = (struct __RSList *)list;
    coll->_count ++;
    return coll;
}

static RSListRef __RSNodeConjoin(RSListRef list, RSTypeRef value) {
    RSNodeRef n = __RSNodePoolGetNode(value, list->_head, NO);
    __RSListIncNodeCount(list);
    list = __RSListSetupHead(list, n);
//    RSRelease(n);
    return list;
}

static void __RSNodeRelease(RSNodeRef node, RSUInteger pp) {
    RSNodeRef next = node;
    RSNodeRef tmp = next;
    while (next && pp) {
        next = next->_next;
        RSRelease(tmp);
        tmp = next;
        pp --;
    }
}

static void __RSListClassDeallocate(RSTypeRef rs)
{
    struct __RSList * list = (struct __RSList *)rs;
//    __RSNodeRelease(list->_head, list->_pp);
    list->_head = nil;
}

static BOOL __RSListClassEqual(RSTypeRef rs1, RSTypeRef rs2)
{
    RSListRef RSList1 = (RSListRef)rs1;
    RSListRef RSList2 = (RSListRef)rs2;
    BOOL result = NO;
    
    result = RSList1->_count == RSList2->_count;
    RSNodeRef n1 = RSList1->_head, n2 = RSList2->_head;
    while (result && n1 && n2) {
        result = RSEqual(n1, n2);
        n1 = n1->_next;
        n2 = n2->_next;
    }
    
    return result;
}

static RSStringRef __RSListClassDescription(RSTypeRef rs)
{
    RSListRef list = (RSListRef)rs;
//    RSStringRef description = RSStringCreateWithFormat(RSAllocatorDefault, RSSTR("RSList %p"), rs);
    RSMutableStringRef description = RSStringCreateMutable(RSAllocatorSystemDefault, 0);
    RSStringAppendCString(description, "(", RSStringEncodingASCII);
    RSNodeRef node = list->_head;
    for (RSUInteger idx = 0; idx < list->_count; idx++) {
        RSStringRef desc = RSDescription(node);
        RSStringAppendString(description, desc);
        RSRelease(desc);
        if (node)
            node = node->_next;
        else
            break;
        if (idx < list->_count - 1) {
            RSStringAppendCString(description, " ", RSStringEncodingASCII);
        }
    }
    RSStringAppendCString(description, ")", RSStringEncodingASCII);
    return description;
}

static RSRuntimeClass __RSListClass =
{
    _RSRuntimeScannedObject,
    0,
    "RSList",
    nil,
    nil,
    __RSListClassDeallocate,
    __RSListClassEqual,
    nil,
    __RSListClassDescription,
    nil,
    nil
};

static RSTypeID _RSListTypeID = _RSRuntimeNotATypeID;
static void __RSListInitialize(void);
static void __RSListDeallocate(RSNotificationRef notification);

RSExport RSTypeID RSListGetTypeID()
{
    static dispatch_once_t onceToken;
    dispatch_once(&onceToken, ^{
        __RSNodePoolInitialize();
        __RSListInitialize();
        RSNotificationCenterAddObserver(RSNotificationCenterGetDefault(), RSAutorelease(RSObserverCreate(RSAllocatorSystemDefault, RSCoreFoundationWillDeallocateNotification, __RSListDeallocate, nil)));
    });
    return _RSListTypeID;
}

static struct __RSList *__RSListCreateEmpty(RSAllocatorRef allocator);

RSExport RSCollectionRef RSNext(RSCollectionRef coll) {
    if (coll == nil || coll == RSNil) return nil;
    if (RSInstanceIsMemberOfClass(coll, &__RSListClass)) {
        RSListRef list = (RSListRef)coll;
        
        // (-> '(1 2 3) next next) ; '(3)
        // (-> '(1 2 3) next next next) ; nil
        // return nil if list is empty
        if ((list->_count < 2) || !list->_head)
            return nil;
        
        RSNodeRef newHead = list->_head->_next;
        struct __RSList *next = __RSListCreateEmpty(RSAllocatorSystemDefault);
        __RSListSetupHead(next, newHead);
        next->_count = newHead ? list->_count - 1 : 0;
        return RSAutorelease(next);
    }
    return nil;
}

static void __RSListInitialize()
{
    _RSListTypeID = __RSRuntimeRegisterClass(&__RSListClass);
    __RSRuntimeSetClassTypeID(&__RSListClass, _RSListTypeID);
}

static void __RSListDeallocate(RSNotificationRef notification)
{
    __RSNodePoolDeallocate();
}

static struct __RSList *__RSListCreateEmpty(RSAllocatorRef allocator) {
    return (struct __RSList *)__RSRuntimeCreateInstance(allocator, RSListGetTypeID(), sizeof(struct __RSList) - sizeof(RSRuntimeBase));
}

static RSListRef __RSListCreateInstance(RSAllocatorRef allocator, RSTypeRef a, va_list arguments)
{
    RSListRef instance = __RSListCreateEmpty(allocator);
    
    RSTypeRef n = a;
    
    while (n) {
        instance = __RSNodeConjoin(instance, n);
        n = va_arg(arguments, RSTypeRef);
    }
    
    return instance;
}

RSExport RSListRef RSListCreate(RSAllocatorRef allocator, RSTypeRef a, ...) {
    if (!a) return __RSListCreateEmpty(allocator);
    va_list ap;
    va_start(ap, a);
    RSArrayRef objs = __RSArrayCreateWithArguments(ap, -1);
    RSListRef list = (RSListRef)RSConjoin(RSListCreateWithArray(allocator, objs), a);
    RSRelease(objs);
    va_end(ap);
    return list;
}

RSInline void __RSListValidateRange(RSListRef list, RSRange range, const char *func)
{
    if (range.location + range.length > RSListGetCount(list))
        __RSCLog(RSLogLevelDebug, "%s\n", func);
}

RSExport void RSListApplyBlock(RSListRef list, RSRange range, void (^fn)(RSTypeRef value, BOOL *isStop)) {
    if (!list || !fn) return;
    __RSGenericValidInstance(list, _RSListTypeID);
    __RSListValidateRange(list, range, __PRETTY_FUNCTION__);
    RSNodeRef node = list->_head;
    for (RSUInteger idx = 0; node && idx < range.location; idx++) {
        node = node->_next;
    }
    if (!node)
        return;
    BOOL stop = NO;
    for (RSUInteger idx = 0; node && idx < range.length && !stop; idx++) {
        fn(__RSNodeGetValue(node), &stop);
        if (node->_next)
            node = node->_next;
        else
            break;
    }
}

RSExport RSListRef RSListCreateWithArray(RSAllocatorRef allocator, RSArrayRef array) {
    if (!array || !RSArrayGetCount(array)) return __RSListCreateEmpty(allocator);
    RSListRef instance = __RSListCreateEmpty(allocator);
    for (RSIndex idx = RSArrayGetCount(array) - 1; idx >= 0; idx--) {
        RSTypeRef value = RSArrayObjectAtIndex(array, idx);
        instance = __RSNodeConjoin(instance, value);
    }
    return instance;
}

RSExport RSIndex RSListGetCount(RSListRef list) {
    if (!list) return 0;
    __RSGenericValidInstance(list, _RSListTypeID);
    return list->_count;
}

RSExport RSListRef RSListCreateDrop(RSAllocatorRef allocator, RSListRef list, RSIndex n) {
    if (!list || RSListGetCount(list) < n) return nil;
    else if (RSListGetCount(list) == n) return __RSListCreateEmpty(allocator);
    struct __RSList *rst = __RSListCreateEmpty(allocator);
    rst->_count = list->_count - n;
    rst->_head = list->_head;
    while (n) {
        rst->_head = rst->_head->_next;
        n--;
    }
    return rst;
}

RSExport RSArrayRef RSListCreateArray(RSListRef list) {
    if (!list || list == (RSListRef)RSNil) return nil;
    RSMutableArrayRef array = RSArrayCreateMutable(RSAllocatorSystemDefault, list->_count);
    RSNodeRef node = list->_head;
    for (RSUInteger idx = 0; idx < list->_count; idx++) {
        RSArrayAddObject(array, __RSNodeGetValue(node));
        node = node->_next;
    }
    return array;
}

#pragma mark -

RSExport RSTypeRef RSFirst(RSCollectionRef coll) {
    if (coll == nil || coll == RSNil) return nil;
    if (RSInstanceIsMemberOfClass(coll, &__RSListClass)) {
        RSListRef list = (RSListRef)coll;
        return __RSNodeGetValue(list->_head);
    } else if (RSArrayGetTypeID() == RSGetTypeID(coll)) {
        return RSArrayObjectAtIndex(coll, 0);
    }
    return nil;
}

RSExport RSTypeRef RSSecond(RSCollectionRef coll) {
    return RSFirst(RSNext(coll));
}

RSExport RSTypeRef RSNth(RSCollectionRef coll, RSIndex idx) {
    if (!coll || coll == RSNil) return nil;
    if (RSArrayGetTypeID() == RSGetTypeID(coll)) {
        return RSArrayObjectAtIndex(coll, idx);
    } else if (RSListGetTypeID() == RSGetTypeID(coll)) {
        if (RSListGetCount(coll) >= idx) return nil;
        __block RSIndex offset = 0;
        __block RSTypeRef v = nil;
        RSListApplyBlock(coll, RSMakeRange(0, RSListGetCount(coll)), ^(RSTypeRef value, BOOL *stop) {
            if (offset == idx) {
                v = value;
                *stop = YES;
            }
            offset ++;
        });
        return v;
    }
    return nil;
}

RSExport RSCollectionRef RSConjoinM(RSCollectionRef coll, RSTypeRef value) {
    if (coll == nil || coll == RSNil) return nil;
    if (RSInstanceIsMemberOfClass(coll, &__RSListClass)) {
        RSListRef list = (RSListRef)coll;
        return __RSNodeConjoin(list, value);
    } else if (RSArrayGetTypeID() == RSGetTypeID(coll)) {
        RSArrayAddObject((RSMutableArrayRef)coll, value);
        return coll;
    }
    return nil;
}

RSExport RSCollectionRef RSConjoin(RSCollectionRef coll, RSTypeRef value) {
    if (coll == nil || coll == RSNil) return nil;
    if (RSInstanceIsMemberOfClass(coll, &__RSListClass)) {
        RSListRef list = (RSListRef)coll;
        return __RSNodeConjoin(list, value);
    } else if (RSArrayGetTypeID() == RSGetTypeID(coll)) {
        RSMutableArrayRef copy = RSMutableCopy(RSAllocatorSystemDefault, coll);
        RSArrayAddObject(copy, value);
        return RSAutorelease(copy);
    }
    return nil;
}
