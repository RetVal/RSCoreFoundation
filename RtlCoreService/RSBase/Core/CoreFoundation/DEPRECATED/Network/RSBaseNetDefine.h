//
//  RSNetBaseDefine.h
//  RSKit
//
//  Created by RetVal on 9/15/12.
//  Copyright (c) 2012 RetVal. All rights reserved.
//

#ifndef RSKit_RSNetBaseDefine_h
#define RSKit_RSNetBaseDefine_h
#include <RSKit/RSBase.h>
#include <sys/socket.h>
typedef RSUInteger RSNetERR;


typedef int SOCKET;
typedef struct sockaddr_in sockaddr_in;
typedef sockaddr_in* sockaddr_inRef;
#define SOCKET_ERROR (-1)
#define SOCKET_SUCCESS  kSuccess

#define RSDefaultSocketType     SOCK_STREAM         
#define RSDefaultSocketProtocol IPPROTO_TCP        

#endif
