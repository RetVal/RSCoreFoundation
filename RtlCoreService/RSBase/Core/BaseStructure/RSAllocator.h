//
//  RSAllocator.h
//  RSCoreFoundation
//
//  Created by RetVal on 10/23/12.
//  Copyright (c) 2012 RetVal. All rights reserved.
//

#ifndef RSCoreFoundation_RSAllocator_h
#define RSCoreFoundation_RSAllocator_h
RS_EXTERN_C_BEGIN
/**
 RSAllocatorRef is an RSType that use for RSRuntime system.
 */
typedef const struct __RSAllocator* RSAllocatorRef RS_AVAILABLE(0_0);
/**
 RSAllocatorDefault is a RSAllocatorRef use for runtime as default.
 @warning RSAllocatorDefault is used for runtime and other RSTypeRef as default. 
 */
extern  RSAllocatorRef RSAllocatorDefault  RS_AVAILABLE(0_0);

/*! @function RSAllocatorGetTypeID
 @abstract Return the RSTypeID of RSAllocatorRef.
 @discussion This function return the RSAllocatorRef type ID from runtime.
 @result A RSTypeID type ID. 
 */
RSTypeID RSAllocatorGetTypeID() RS_AVAILABLE(0_0);

/*! @function RSAllocatorGetDefault
 @abstract Return RSAllocatorDefault as default.
 @discussion if not set the default allocator, it returns the RSAllocatorDefault.
 @result A RSAllocatorRef.
 */
RSAllocatorRef RSAllocatorGetDefault() RS_AVAILABLE(0_0);

/*! @function RSAllocatorSetDefault
 @abstract Return nothing.
 @discussion This function set a new allocator as the default allocator, if push nil, the RSAllocatorDefault will be set.
 @param allocator The RSAllocatorRef object.
 */
void RSAllocatorSetDefault(RSAllocatorRef allocator) RS_AVAILABLE(0_0);
void *RSAllocatorAllocate(RSAllocatorRef allocator, RSIndex size) __attribute__ ((__malloc__));
void *RSAllocatorCallocate(RSAllocatorRef allocator, RSIndex part, RSIndex size);
void  RSAllocatorDeallocate(RSAllocatorRef allocator, const void *ptr);
void *RSAllocatorCopy(RSAllocatorRef allocator, RSIndex newsize, void *ptr, RSIndex needCopy);
void *RSAllocatorReallocate(RSAllocatorRef allocator, void *ptr, RSIndex newsize);
void *RSAllocatorVallocate(RSAllocatorRef allocator, RSIndex size);
RSIndex RSAllocatorSize(RSAllocatorRef allocator, RSIndex size);
RSIndex RSAllocatorInstanceSize(RSAllocatorRef allocator, RSTypeRef rs);

RS_EXTERN_C_END
#endif
