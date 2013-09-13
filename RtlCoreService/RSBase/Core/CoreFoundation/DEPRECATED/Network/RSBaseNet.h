//  Created by RetVal on 10/13/11.
//  Copyright (c) 2012 RetVal. All rights reserved.
//

//  Created by RetVal on 10/13/11.
//  Copyright (c) 2012 RetVal. All rights reserved.
//
#ifndef RSCoreService_RSNetBase_h
#define RSCoreService_RSNetBase_h	1.3
#include <fcntl.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <stdio.h>
#include <netinet/in.h>
#include <resolv.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <sys/types.h>
#include <netdb.h>

#include <RSKit/RSBase.h>
#include <RSKit/RSBaseNetStructure.h>
#include <RSKit/RSBaseNetDefine.h>
#include <RSKit/RSNetDHKeyExchange.h>


// if you want to use RSNetKit, anyway, should make sure call the RSNetInitSocketService at first, otherwise some error may happen.
bool	RSNetInitSocketService();

// if you use RSNetKit in your application anywhere, you should make sure call the RSNetReleaseSocketService in your app exit time.
bool	RSNetReleaseSocketService();

// socket_type : socket type,for instance SOCK_STREAM.
// socket_protocol : socket protocol,for instance IPPROTO_TCP.
// return the new socket you could use. Warnning: the socket is too new to be used in anyway.
SOCKET	RSNetAlloc(int socket_type,int socket_protocol);// = SOCK_STREAM . = 0

// free the socket resource and reset the pointer to nil automatically.
void	RSNetFree(SOCKET *s);

// allocate a socket and bind the ip address, port information.
// ip : ip address of this socket.
// port : port addresss of this socket.
// isSender : if sender is true, the socket will be used for connect, otherwise for bind.
// socket_type : socket type.
// socket protocol : socket protocol.
// return a new socket , and this socket can be used in most way.
// if is sender, the ip address is the target address, else is the listener ip address.
SOCKET	RSNetAllocWithIpAddress(RSBuffer ip,unsigned short port,bool isSender,int socket_type,int socket_protocol);// = SOCK_STREAM . = 0

// you should initialize the raw address first by your self, or call RSNetInitRawAddressWithIpAddress
// return a new socket and bind with address.and this socket can be used in most way.
SOCKET	RSNetAllocWithRawAddress(sockaddr_inRef rawAddress,bool isSender,int socket_type,int socket_protocol);// = SOCK_STREAM . = 0

// initialize a raw address with your ip address and port information.
bool	RSNetInitRawAddressWithIpAddress(sockaddr_inRef raw,RSBuffer ip,unsigned short port);

// if sender, the address is self, otherwise is server.
bool	RSNetCombineRawAddressWithSocket(SOCKET s,sockaddr_inRef rawAddress,bool isSender);

// return the local host name.
RSBuffer	RSNetGetLocalName();

// return the local host ip address.
RSBuffer	RSNetGetLocalIp();

// send the data with length by socket, will wait for send all data before return.
bool	RSNetSend(SOCKET s,RSHandle data,RSUInteger len);

// recv the data from 'remote' by socket, the data buffer length must be enough, will wait for recv all data before return.
bool	RSNetRecv(SOCKET s,RSHandle data,RSUInteger len);

// will wait for a connection coming, and return the client address in the linkAddress.
// the new connection will use the newSocket to send or recv information.
// the socket s must be not sender.
bool	RSNetAutoWaitConnect(SOCKET*  s,SOCKET* newSocket,int number,sockaddr_inRef  linkAddress,int size);

// set the socket accept number. The socket s must be not sender.
bool	RSNetSetAccpetNumber(SOCKET*  s,int number);

#endif