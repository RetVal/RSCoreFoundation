//
//  RSHash.h
//  RSCoreFoundation
//
//  Created by RetVal on 10/15/12.
//  Copyright (c) 2012 RetVal. All rights reserved.
//

#ifndef RSCoreFoundation_RSHash_h
#define RSCoreFoundation_RSHash_h

#include <RSCoreFoundation/RSBase.h>
enum {
    RSDefaultHash = 0,
    RSBlizzardHash = 1,
    RSSHA2Hash = 2
};
typedef RSUInteger RSHashSelectorID;
RSExport BOOL RSBaseHash(RSHashSelectorID selector, const void* hash, RSUInteger size, RSHashCode* hashCodeExt, RSUInteger codeSize) RS_AVAILABLE(0_0);
#endif
