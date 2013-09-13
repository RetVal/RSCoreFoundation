//
//  RSBaseHeap.h
//  RSCoreFoundation
//
//  Created by RetVal on 11/12/12.
//  Copyright (c) 2012 RetVal. All rights reserved.
//

#ifndef RSCoreFoundation_RSBaseHeap_h
#define RSCoreFoundation_RSBaseHeap_h
RS_EXTERN_C_BEGIN
typedef const void *	(*RSBaseHeapRetainCallBack)(const void *value);
typedef void            (*RSBaseHeapReleaseCallBack)(const void *value);
typedef RSStringRef     (*RSBaseHeapCopyDescriptionCallBack)(const void *value);
typedef RSComparisonResult (*RSBaseHeapCompareCallBack)(const void *value1, const void *value2);
//typedef RSHashCode      (*RSBaseHeapHashCallBack)(const void *value);
typedef struct {
    RSIndex vs;
    RSBaseHeapRetainCallBack retain;
    RSBaseHeapReleaseCallBack release;
    RSBaseHeapCopyDescriptionCallBack description;
    RSBaseHeapCompareCallBack comparator;
}RSBaseHeapCallbacks;

extern const RSBaseHeapCallbacks* const RSBaseHeapRSStringRefCallBacks;  //only for the RSStringRef.
extern const RSBaseHeapCallbacks* const RSBaseHeapRSNumberRefCallBacks;  //only for the RSNumberRef.
typedef const struct __RSBaseHeap* RSBaseHeapRef;
typedef struct __RSBaseHeap* RSMutableBaseHeapRef;
RSTypeID RSBaseHeapGetTypeID();

RSExport RSBaseHeapRef RSBaseHeapCreateWithArray(RSAllocatorRef allocator, RSArrayRef objects, RSBaseHeapCompareCallBack comparator, BOOL maxRootHeap);
RSExport RSMutableBaseHeapRef RSBaseHeapCreateMutable(RSAllocatorRef allocator, RSIndex capacity, const RSBaseHeapCallbacks* const callbacks, BOOL maxRootHeap);
RSExport void RSBaseHeapSetCallBacks(RSBaseHeapRef heap, RSBaseHeapCallbacks* callbacks);
RSExport RSBaseHeapCallbacks* RSBaseHeapGetCallBacks(RSBaseHeapRef heap);
RSExport void RSBaseHeapAddObject(RSBaseHeapRef heap, RSTypeRef obj);
RSExport void RSBaseHeapRemoveObject(RSBaseHeapRef heap, RSTypeRef obj);
RSExport RSIndex RSBaseHeapGetCount(RSBaseHeapRef heap);
RS_EXTERN_C_END
#endif
