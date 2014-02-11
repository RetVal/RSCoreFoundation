//
//  RSFunctional.c
//  RSCoreFoundation
//
//  Created by closure on 1/29/14.
//  Copyright (c) 2014 RetVal. All rights reserved.
//

#include <RSCoreFoundation/RSFunctional.h>
#include <RSCoreFoundation/RSRuntime.h>

#pragma mark -
#pragma mark Map API Group

static RSArrayRef __RSMapArray(RSArrayRef coll, RSTypeRef (^fn)(RSTypeRef obj)) {
    RSMutableArrayRef result = RSArrayCreateMutable(RSAllocatorSystemDefault, 0);
    RSArrayApplyBlock(coll, RSMakeRange(0, RSArrayGetCount(coll)), ^(const void *value, RSUInteger idx, BOOL *isStop) {
        RSTypeRef obj = fn(value);
        RSArrayAddObject(result, obj);
        RSRelease(obj);
    });
    return RSAutorelease(result);
}

static RSListRef __RSMapList(RSListRef coll, RSTypeRef (^fn)(RSTypeRef obj)) {
    __block RSCollectionRef rst = RSAutorelease(RSListCreate(RSAllocatorSystemDefault, fn(RSFirst(coll)), nil));
    __block RSCollectionRef cache = coll;
    RSAutoreleaseBlock(^{
        while ((cache = RSNext(cache))) {
            rst = RSConjoin(rst, fn(RSFirst(cache)));
        }
        RSRetain(rst);
    });
    return (RSListRef)RSAutorelease(rst);
}

RSExport RSCollectionRef RSMap(RSCollectionRef coll, RSTypeRef (^fn)(RSTypeRef obj)) {
    if (RSInstanceIsMemberOfClass(coll, RSClassGetWithName(RSSTR("RSArray")))) return __RSMapArray(coll, fn);
    else if (RSInstanceIsMemberOfClass(coll, RSClassGetWithName(RSSTR("RSList")))) return __RSMapList(coll, fn);
    return nil;
}

#pragma mark -
#pragma mark Reduce API Group

static RSTypeRef __RSReduceArray(RSTypeRef (^fn)(RSTypeRef a, RSTypeRef b), RSArrayRef coll) {
    __block RSTypeRef result = RSArrayObjectAtIndex(coll, 0);
    if (RSArrayGetCount(coll) == 1)
        return RSAutorelease(fn(result, nil));
    else if (RSArrayGetCount(coll) == 2) {
        return RSAutorelease(fn(result, RSArrayObjectAtIndex(coll, 1)));
    }
    
    RSAutoreleaseBlock(^{
        RSArrayApplyBlock(coll, RSMakeRange(1, RSArrayGetCount(coll) - 1), ^(const void *value, RSUInteger idx, BOOL *isStop) {
            result = RSAutorelease(fn(result, value));
        });
        RSRetain(result);
    });
    return RSAutorelease(result);
}

RSExport RSTypeRef RSReduce(RSTypeRef (^fn)(RSTypeRef a, RSTypeRef b), RSCollectionRef coll) {
    if (RSInstanceIsMemberOfClass(coll, RSClassGetWithName(RSSTR("RSArray")))) return __RSReduceArray(fn, coll);
    return nil;
}

#pragma mark -
#pragma mark Reverse API Group

static RSListRef __RSReverseList(RSListRef list) {
    return __RSMapList(list, ^RSTypeRef(RSTypeRef obj) {
        return obj;
    });
}

static RSArrayRef __RSReverseArray(RSArrayRef array) {
    RSUInteger cnt = RSArrayGetCount(array);
    RSMutableArrayRef coll = RSArrayCreateMutable(RSAllocatorSystemDefault, cnt);
    for (RSUInteger idx = 0; idx < cnt; idx++) {
        RSArrayAddObject(coll, RSArrayObjectAtIndex(array, cnt - idx - 1));
    }
    return RSAutorelease(coll);
}

RSExport RSCollectionRef RSReverse(RSCollectionRef coll) {
    if (RSInstanceIsMemberOfClass(coll, RSClassGetWithName(RSSTR("RSList")))) return __RSReverseList(coll);
    else if (RSInstanceIsMemberOfClass(coll, RSClassGetWithName(RSSTR("RSArray")))) return __RSReverseArray(coll);
    return coll;
}
