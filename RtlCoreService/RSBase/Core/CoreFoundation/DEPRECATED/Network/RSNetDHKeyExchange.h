//
//  NSDHKeyExchange.h
//  RSXCL
//
//  Created by RetVal on 9/15/12.
//  Copyright (c) 2012 RetVal. All rights reserved.
//

#ifndef RSXCL_NSDHKeyExchange_h
#define RSKit_RSNetBase_h
#include <RSKit/RSBase.h>
#include <RSKit/RSBaseNet.h>

RSExport RSUInteger NSDHKeyExchangeMain(SOCKET socket, RSUInteger key);
RSExport RSUInteger NSDHKeyExchangeWait(SOCKET socket, RSUInteger key);
#endif
