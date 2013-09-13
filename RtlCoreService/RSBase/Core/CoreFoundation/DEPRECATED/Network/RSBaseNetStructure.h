//  Created by RetVal on 10/13/11.
//  Copyright (c) 2012 RetVal. All rights reserved.
//

#include <RSKit/RSBase.h>
#ifndef RSNetKit_import_h
#define RSNetKit_import_h	1

//#include <socket.h>
	#ifdef	_WIN32
		#ifndef include_winsock2_already
		#define include_winsock2_already	2
			#if !defined(WIN32_LEAN_AND_MEAN) && (_WIN32_WINNT >= 0x0400) && !defined(USING_WIN_PSDK)
				#include <windows.h>
			#else
				#include <WINSOCK2.H>
				#pragma comment(lib,"ws2_32.lib")
			#endif
		#endif
	#endif
#ifndef kNSDefaultEncryptKey
    #define kNSDefaultEncryptKey    "kRtlNSDefaultEncryptKey"
#endif
#ifndef kNSDefaultContentEncryptKey
    #define kNSDefaultContentEncryptKey "kRtlNSContentEncryptKey"
#endif

#endif

