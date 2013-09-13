//
//  RSException.h
//  RSCoreFoundation
//
//  Created by RetVal on 5/17/13.
//  Copyright (c) 2013 RetVal. All rights reserved.
//

#ifndef RSCoreFoundation_RSException_h
#define RSCoreFoundation_RSException_h

RS_EXTERN_C_BEGIN
typedef const struct __RSException* RSExceptionRef;
typedef void (^RSExecutableBlock)();
typedef void (^RSExceptionHandler)(RSExceptionRef e);
RSExport RSExceptionRef RSExceptionCreate(RSAllocatorRef allocator,RSStringRef name, RSStringRef reason, RSDictionaryRef userInfo) RS_AVAILABLE(0_3);
RSExport void RSExceptionCreateAndRaise(RSAllocatorRef allocator, RSStringRef name, RSStringRef reason, RSDictionaryRef userInfo) RS_AVAILABLE(0_3);

RSExport RSExceptionRef RSExceptionCreateWithFormat(RSAllocatorRef allocator, RSStringRef name, RSDictionaryRef userInfo, RSStringRef reasonFormat, ...) RS_AVAILABLE(0_3);
RSExport void RSExceptionCreateWithFormatAndRaise(RSAllocatorRef allocator, RSStringRef name, RSDictionaryRef userInfo, RSStringRef reasonFormat, ...) RS_AVAILABLE(0_3);

RSExport RSDictionaryRef RSExceptionGetUserInfo(RSExceptionRef e) RS_AVAILABLE(0_3);
RSExport RSStringRef RSExceptionGetName(RSExceptionRef e) RS_AVAILABLE(0_3);
RSExport RSStringRef RSExceptionGetReason(RSExceptionRef e) RS_AVAILABLE(0_3);
RSExport void **RSExceptionGetFrames(RSExceptionRef e) RS_AVAILABLE(0_3);

RSExport void RSExceptionRaise(RSExceptionRef e) RS_AVAILABLE(0_3);
RSExport void RSExceptionBlock(RSExecutableBlock tryBlock, RSExceptionHandler finalHandler) RS_AVAILABLE(0_3);

RSExport RSStringRef const RSGenericException;
RSExport RSStringRef const RSRangeException;
RSExport RSStringRef const RSInvalidArgumentException;
RSExport RSStringRef const RSInternalInconsistencyException;

RSExport RSStringRef const RSMallocException;

RSExport RSStringRef const RSObjectInaccessibleException;
RSExport RSStringRef const RSObjectNotAvailableException;
RSExport RSStringRef const RSDestinationInvalidException;

RSExport RSStringRef const RSPortTimeoutException;
RSExport RSStringRef const RSInvalidSendPortException;
RSExport RSStringRef const RSInvalidReceivePortException;
RSExport RSStringRef const RSPortSendException;
RSExport RSStringRef const RSPortReceiveException;

RSExport RSStringRef const RSOldStyleException;

RSExport RSStringRef const RSExceptionObject;
RSExport RSStringRef const RSExceptionThreadId;
RSExport RSStringRef const RSExceptionDetailInformation;
RS_EXTERN_C_END

#endif
