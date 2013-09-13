//
//  RSNetDHKeyExchange.c
//  RSKit
//
//  Created by RetVal on 9/15/12.
//  Copyright (c) 2012 RetVal. All rights reserved.
//

#include <stdio.h>
#include <RSKit/RSNetDHKeyExchange.h>

RSExport RSUInteger RSNetDHKeyExchangeMain(SOCKET socket, RSUInteger key)
{
    RSUInteger remotePublicKey = 0;
    RSNetSend(socket, &key, sizeof(RSUInteger));
    RSNetRecv(socket, &remotePublicKey, sizeof(RSUInteger));
    return remotePublicKey;
}

RSExport RSUInteger RSNetDHKeyExchangeWait(SOCKET socket, RSUInteger key)
{
    RSUInteger remotePublicKey = 0;
    RSNetRecv(socket, &remotePublicKey, sizeof(RSUInteger));
    RSNetSend(socket, &key, sizeof(RSUInteger));
    return remotePublicKey;
}
