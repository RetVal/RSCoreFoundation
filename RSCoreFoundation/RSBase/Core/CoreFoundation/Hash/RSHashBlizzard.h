//
//  RSHashBlizzard.h
//  RSCoreFoundation
//
//  Created by RetVal on 10/14/12.
//  Copyright (c) 2012 RetVal. All rights reserved.
//

#ifndef RSCoreFoundation_RSHashBlizzard_h
#define RSCoreFoundation_RSHashBlizzard_h
#include <RSCoreFoundation/RSBase.h>


//init the hash table.
RSPrivate void __RSBaseHashBlizzardInitCryptTable(void) RS_INIT_ROUTINE;

//release the hash table(reset).
RSPrivate void __RSBaseHashBlizzardReleaseCryptTable(void) RS_FINAL_ROUTINE;

//lpszBuffer : want to hash the content in the buffer.
//dwHashType : hash type, default use 0, second type use 1.
//return code: see RtlError.
RSPrivate RSHashCode __RSBaseHashBlizzardHash(RSTypeRef obj, RSIndex size, RSUInteger type);
#endif
