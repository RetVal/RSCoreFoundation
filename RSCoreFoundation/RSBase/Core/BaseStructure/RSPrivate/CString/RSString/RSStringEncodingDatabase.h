//
//  RSStringEncodingDatabase.h
//  RSCoreFoundation
//
//  Created by RetVal on 7/29/13.
//  Copyright (c) 2013 RetVal. All rights reserved.
//

#ifndef RSCoreFoundation_RSStringEncodingDatabase_h
#define RSCoreFoundation_RSStringEncodingDatabase_h


#include <RSCoreFoundation/RSString.h>

extern uint16_t __RSStringEncodingGetWindowsCodePage(RSStringEncoding encoding);
extern RSStringEncoding __RSStringEncodingGetFromWindowsCodePage(uint16_t codepage);

extern BOOL __RSStringEncodingGetCanonicalName(RSStringEncoding encoding, char *buffer, RSIndex bufferSize);
extern RSStringEncoding __RSStringEncodingGetFromCanonicalName(const char *canonicalName);

extern RSStringEncoding __RSStringEncodingGetMostCompatibleMacScript(RSStringEncoding encoding);

extern const char *__RSStringEncodingGetName(RSStringEncoding encoding); // Returns simple non-localizd name


#endif
