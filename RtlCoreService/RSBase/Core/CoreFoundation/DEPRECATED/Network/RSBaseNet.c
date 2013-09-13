//  Created by RetVal on 10/13/11.
//  Copyright (c) 2012 RetVal. All rights reserved.
//
#include <RSKit/RSBase.h>
#include <RSKit/RSBaseNet.h>
#define wait_time_part 50

//////////////////////////////////////////////////////////////////////////
//VERSION	1.3
//////////////////////////////////////////////////////////////////////////
static char* ltoa(long value,char* buf, int radix)
{
    RSUInteger iRet = kSuccess;
    do
    {
        RCMP(buf,iRet);
        sprintf(buf,"%ld",value);
    } while (0);
    return buf;
}

static char* ultoa(unsigned long value,char* buf, int radix)
{
    RSUInteger iRet = kSuccess;
    do
    {
        RCMP(buf,iRet);
        sprintf(buf,"%u",radix);
    } while (0);
    return buf;
}

RSExport bool	RSNetInitSocketService()
{
	bool	err = false;
	do 
	{
#ifdef	_WIN32
		WORD wVersionRequested;
		WSADATA wsaData;
		wVersionRequested = MAKEWORD( 2, 2 );
		err = WSAStartup( wVersionRequested, &wsaData );
		if ( err != 0 ) 
		{
			return err = false;
		}
		if(LOBYTE( wsaData.wVersion ) != 2 ||
		   HIBYTE( wsaData.wVersion ) != 2 ) 
		{
			WSACleanup( );
			return err = false; 
		}
#else
        err = true;
#endif
	} while (0);
	return	err;
}
RSExport bool	RSNetReleaseSocketService()
{
#ifdef	_WIN32
	WSACleanup();
#else
#endif
	return	true;
}
RSExport SOCKET	RSNetAlloc(int socket_type,int socket_protocol)
{
    if (socket_type == RSDefaultSocketType) {
        socket_type = SOCK_STREAM;
    }
    if (socket_protocol == RSDefaultSocketProtocol) {
        socket_protocol = IPPROTO_TCP;
    }
	return socket(AF_INET,socket_type, socket_protocol);
}
RSExport SOCKET	RSNetAllocWithIpAddress(char *ip,unsigned short port,bool isSender,int socket_type,int socket_protocol)
{
	struct	sockaddr_in	rawAddress = {0};
	SOCKET	s = RSNetAlloc(socket_type,socket_protocol);
	if (RSNetInitRawAddressWithIpAddress(&rawAddress,ip,port))
	{
		if(RSNetCombineRawAddressWithSocket(s,&rawAddress,isSender))
		{
			return	s;
		}
		else
		{
			RSNetFree(&s);
			return 0;
		}
	}
	return 0;
}
RSExport SOCKET	RSNetAllocWithRawAddress(sockaddr_inRef rawAddress,bool isSender,int socket_type,int socket_protocol)
{
	SOCKET s = RSNetAlloc(socket_type,socket_protocol);
	if(RSNetCombineRawAddressWithSocket(s,rawAddress,isSender))
	{
		return	s;
	}
	else
	{
		RSNetFree(&s);
	}
	return 0;
}
RSExport bool	RSNetInitRawAddressWithIpAddress(sockaddr_inRef raw,char* ip,unsigned short port)
{
	bool err = false;
	
	//SOCKET	s = 0;
	do 
	{
		RCMP(raw,err);
		RCMP(ip,err);
		raw->sin_family = AF_INET; 
		raw->sin_port = htons(port); 
		raw->sin_addr.s_addr = (in_addr_t)inet_addr(ip);
	} while (0);
	return !err;
}
RSExport bool	RSNetCombineRawAddressWithSocket(SOCKET s,sockaddr_inRef  rawAddress,bool isSender)
{
	if (isSender)
	{
		//unsigned long	wait_time_part = 50;
#ifndef retry_limit
#define retry_limit	5
#endif
		struct sockaddr_in addr;
		addr.sin_family = rawAddress->sin_family;
		addr.sin_addr.s_addr = rawAddress->sin_addr.s_addr;
		addr.sin_port = rawAddress->sin_port;
		for (unsigned long iX = 0; iX < retry_limit; iX++)
		{
			if(SOCKET_ERROR == connect(s,(struct sockaddr*)rawAddress,sizeof(sockaddr_in)))
			{
#ifdef	_WIN32
				__RSCLog(kRSLogLevelDebug, "connect error code:%d\n",GetLastError());
#endif
				//TMWaitMicro(wait_time_part);
#ifdef __APPLE__
                sleep(wait_time_part);
#endif
			}
			else
			{
				return	true;
			}
		}
		
	}
	else
	{
		if(SOCKET_ERROR == bind(s,(struct sockaddr*)rawAddress,sizeof(sockaddr_in)))
		{
#ifdef	_WIN32
			__RSCLog(kRSLogLevelDebug, "bind error code:%d\n",GetLastError());
#endif
			return false;
		}
		else
		{
			return true;
		}
	}
	return	false;
}
RSExport void	RSNetFree(SOCKET *s)
{
	if (s)
	{
		shutdown(*s,0);
#ifdef _WIN32
		closesocket(*s);
#else
        close(*s);
#endif
		*s = 0;
	}
}

RSExport char*	RSNetGetLocalName()
{
	static char name[kRSBufferSize] = {0};
	gethostname(name,kRSBufferSize);
	return name;
}
RSExport char*	RSNetGetLocalIp()
{
	static char	ipAddress[3*4+3+1 + 30] = {0};
	struct hostent* host = nil;
	host = (struct hostent*)gethostbyname((const char*)RSNetGetLocalName());
    //char* hostAddrlist = (char*)*host->h_addr_list;
	sprintf(ipAddress,"%d.%d.%d.%d",
			(int)(host->h_addr_list[0][0]&0x00ff),
			(int)(host->h_addr_list[0][1]&0x00ff),
			(int)(host->h_addr_list[0][2]&0x00ff),
			(int)(host->h_addr_list[0][3]&0x00ff));
	return	ipAddress;
}

RSExport bool	RSNetSend(SOCKET s,RSHandle data,RSUInteger len)
{
    RSInteger bRet = 0;
    RSUInteger needSendSize = len;
    RSUInteger alreadySendSize = 0;
ReSent:
    alreadySendSize += bRet;
    needSendSize -= bRet;
    bRet = send(s, (RSBuffer)data + alreadySendSize, needSendSize,0);
    if (bRet == SOCKET_ERROR)
    {
        return false;
    }
    else if (needSendSize == 0)
    {
        return true;
    }
    else
        goto ReSent;
	return false;
}
RSExport bool	RSNetRecv(SOCKET s,RSHandle data,RSUInteger len)
{
	RSInteger bRet = 0;
    RSUInteger needRecvSize = len;
    RSUInteger alreadyRecvSize = 0;
ReRecv:
    alreadyRecvSize += bRet;
    needRecvSize -= bRet;
    bRet = recv(s, (RSBuffer)data + alreadyRecvSize, needRecvSize,0);
    if (bRet == SOCKET_ERROR)
    {
        return false;
    }
    else if (needRecvSize == 0)
    {
        return true;
    }
    else
        goto ReRecv;
	if (needRecvSize == 0)
	{
		return true;
	}
	return false;
}

RSExport bool	RSNetAutoWaitConnect(SOCKET * s,SOCKET*newSocket,int number,sockaddr_inRef linkAddress,int size)
{
	//SOCKET	sOld = *s;
	if(RSNetSetAccpetNumber(s,number))
	{
		SOCKET	s_new = 0;
		if(-1 == (s_new = accept(*s,(struct sockaddr*)linkAddress,(unsigned int*)&size)))
		{
		//	RSNetFree(&sOld);
			return false;
		}
		//RSNetFree(&sOld);
		if (newSocket)
		{
			*newSocket = s_new;
		}
		else
		{
            RSNetFree(s);
			*s = s_new;
		}
		return true;
	}
	else
	{
		return false;
	}
	
}

RSExport bool	RSNetSetAccpetNumber(SOCKET * s,int number)
{
	ssize_t err = listen(*s,number);
	if (err != SOCKET_ERROR)
	{
		return true;
	}
	else
	{
		return false;
	}
	return false;
}
