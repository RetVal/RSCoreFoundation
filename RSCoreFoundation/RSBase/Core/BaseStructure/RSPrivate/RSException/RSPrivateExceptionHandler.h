//
//  RSPrivateExceptionHandler.h
//  RSCoreFoundation
//
//  Created by RetVal on 5/18/13.
//  Copyright (c) 2013 RetVal. All rights reserved.
//

#ifndef RSCoreFoundation_RSPrivateExceptionHandler_h
#define RSCoreFoundation_RSPrivateExceptionHandler_h

#include <RSCoreFoundation/RSException.h>
typedef void (^_execute_block)(void);
typedef void (^_exception_handler_block)(RSExceptionRef e);

RSPrivate void __tls_add_exception_handler(_exception_handler_block *handler);
RSPrivate void __tls_cls_exception_handler(_exception_handler_block *handler);
RSPrivate BOOL __tls_do_exception_with_handler(RSExceptionRef e);
#endif
