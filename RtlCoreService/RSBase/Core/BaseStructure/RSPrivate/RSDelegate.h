//
//  RSDelegate.h
//  RSCoreFoundation
//
//  Created by RetVal on 6/26/13.
//  Copyright (c) 2013 RetVal. All rights reserved.
//

#ifndef RSCoreFoundation_RSDelegate
#define RSCoreFoundation_RSDelegate

#include <RSCoreFoundation/RSProtocol.h>

RS_EXTERN_C_BEGIN

typedef struct __RSDelegate *RSDelegateRef;

RSExport RSTypeID RSDelegateGetTypeID();

RSExport RSStringRef RSDelegateGetName(RSDelegateRef delegate);

RSExport BOOL RSDelegateConfirmProtocol(RSDelegateRef delegate, RSStringRef protocol);

RSExport RSProtocolRef RSDelegateGetProtocolWithName(RSDelegateRef delegate, RSStringRef name);

RSExport BOOL RSDelegateConfirmProtocol(RSDelegateRef delegate, RSStringRef protocol);

RSExport RSDelegateRef RSDelegateCreateWithProtocols(RSAllocatorRef allocator, RSStringRef delegateName, RSArrayRef protocols);
RS_EXTERN_C_END
#endif 
