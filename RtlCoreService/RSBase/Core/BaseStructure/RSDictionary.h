//
//  RSDictionary.h
//  RSCoreFoundation
//
//  Created by RetVal on 11/4/12.
//  Copyright (c) 2012 RetVal. All rights reserved.
//

#ifndef RSCoreFoundation_RSDictionary_h
#define RSCoreFoundation_RSDictionary_h
#include <RSCoreFoundation/RSBase.h>
#include <RSCoreFoundation/RSAllocator.h>
#include <RSCoreFoundation/RSArray.h>
RS_EXTERN_C_BEGIN
typedef void (*RSDictionaryApplierFunction)(const void *key, const void* value, void *context);
typedef const struct __RSDictionary* RSDictionaryRef RS_AVAILABLE(0_0);
typedef struct __RSDictionary* RSMutableDictionaryRef RS_AVAILABLE(0_0);


typedef const void *	(*RSDictionaryRetainCallBack)(RSAllocatorRef, const void *value);
typedef void            (*RSDictionaryReleaseCallBack)(RSAllocatorRef, const void *value);
//typedef const void *	(*RSDictionaryRetainCallBack)(const void *value);
//typedef void            (*RSDictionaryReleaseCallBack)(const void *value);

typedef RSStringRef     (*RSDictionaryCopyDescriptionCallBack)(const void *value);
typedef BOOL            (*RSDictionaryEqualCallBack)(const void *value1, const void *value2);
typedef RSHashCode      (*RSDictionaryHashCallBack)(const void *value);
typedef const void *    (*RSDictionaryCopyCallBack)(RSAllocatorRef allocator, const void *value);
typedef struct __RSDictionaryKeyContext {
    RSDictionaryRetainCallBack retain;
    RSDictionaryReleaseCallBack release;
    RSDictionaryCopyDescriptionCallBack description;
    RSDictionaryCopyCallBack copy;
    RSDictionaryHashCallBack hash;
    RSDictionaryEqualCallBack equal;
}RSDictionaryKeyContext RS_AVAILABLE(0_0);

typedef struct __RSDictionaryValueContext {
    RSDictionaryRetainCallBack retain;
    RSDictionaryReleaseCallBack release;
    RSDictionaryCopyDescriptionCallBack description;
    RSDictionaryCopyCallBack copy;
    RSDictionaryHashCallBack hash;
    RSDictionaryEqualCallBack equal;
}RSDictionaryValueContext RS_AVAILABLE(0_0);

typedef struct __RSDictionaryContext {
    RSIndex va;
    const RSDictionaryKeyContext *keyContext;
    const RSDictionaryValueContext *valueContext;
}RSDictionaryContext RS_AVAILABLE(0_0);

RSExport const RSDictionaryKeyContext* RSDictionaryDoNilKeyContext RS_AVAILABLE(0_0);       
RSExport const RSDictionaryValueContext* RSDictionaryDoNilValueContext RS_AVAILABLE(0_0);
RSExport const RSDictionaryKeyContext* RSDictionaryRSKeyContext RS_AVAILABLE(0_0);     
RSExport const RSDictionaryValueContext* RSDictionaryRSValueContext RS_AVAILABLE(0_0); 
RSExport const RSDictionaryContext* RSDictionaryRSTypeContext RS_AVAILABLE(0_0);     // key and value are RSTypeRef.
RSExport const RSDictionaryContext* RSDictionaryNilContext RS_AVAILABLE(0_0);        // do nothing with key and value.
RSExport const RSDictionaryContext* RSDictionaryNilKeyContext RS_AVAILABLE(0_0);     // do nothing with key.
RSExport const RSDictionaryContext* RSDictionaryNilValueContext RS_AVAILABLE(0_0);   // do nothing with value.

RSExport RSTypeID RSDictionaryGetTypeID() RS_AVAILABLE(0_0);

RSExport RSIndex RSDictionaryGetCount(RSDictionaryRef dictionary) RS_AVAILABLE(0_0);

RSExport RSDictionaryRef RSDictionaryCreate(RSAllocatorRef allocator, const void** keys, const void** values, const RSDictionaryContext* context, RSIndex count) RS_AVAILABLE(0_0);
RSExport RSDictionaryRef RSDictionaryCreateCopy(RSAllocatorRef allocator, RSDictionaryRef other) RS_AVAILABLE(0_0);
RSExport RSMutableDictionaryRef RSDictionaryCreateMutableCopy(RSAllocatorRef allocator, RSDictionaryRef dictionary) RS_AVAILABLE(0_0);
RSExport RSDictionaryRef RSDictionaryCreateWithArray(RSAllocatorRef allocator, RSArrayRef keys, RSArrayRef values, const RSDictionaryContext* context) RS_AVAILABLE(0_0);
RSExport RSMutableDictionaryRef RSDictionaryCreateWithObjectsAndOKeys(RSAllocatorRef allocator, RSTypeRef obj, ...) RS_AVAILABLE(0_0) RS_REQUIRES_NIL_TERMINATION;
RSExport RSMutableDictionaryRef RSDictionaryCreateMutable(RSAllocatorRef allocator, RSIndex capacity, const RSDictionaryContext* context) RS_AVAILABLE(0_0);

RSExport void RSDictionarySetValue(RSMutableDictionaryRef dictionary, const void* key, const void* value) RS_AVAILABLE(0_0);
RSExport const void* RSDictionaryGetValue(RSDictionaryRef dictionary, const void* key) RS_AVAILABLE(0_0);
RSExport BOOL RSDictionaryGetValueIfPresent(RSDictionaryRef hc, const void *key, const void **value) RS_AVAILABLE(0_0);
RSExport void RSDictionaryRemoveValue(RSMutableDictionaryRef dictionary, const void* key) RS_AVAILABLE(0_0);

RSExport void RSDictionaryRemoveAllObjects(RSMutableDictionaryRef dictionary) RS_AVAILABLE(0_0);

RSExport RSArrayRef RSDictionaryCopyAllKeys(RSDictionaryRef dictionary) RS_AVAILABLE(0_0);
RSExport RSArrayRef RSDictionaryCopyAllValues(RSDictionaryRef dictionary) RS_AVAILABLE(0_0);
RSExport void RSDictionaryGetKeysAndValues(RSDictionaryRef dictionary, RSTypeRef* keys, RSTypeRef* values) RS_AVAILABLE(0_0);

RSExport void RSDictionaryApplyFunction(RSDictionaryRef dictionary, RSDictionaryApplierFunction apply, void* context) RS_AVAILABLE(0_0);

#if RS_BLOCKS_AVAILABLE
RSExport void RSDictionaryApplyBlock(RSDictionaryRef dictionary, void (^block)(const void* key, const void* value, BOOL *stop)) RS_AVAILABLE(0_1);
#endif
RS_EXTERN_C_END
#endif
