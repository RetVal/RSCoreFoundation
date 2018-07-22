//
//  RSHash.c
//  RSCoreFoundation
//
//  Created by RetVal on 10/15/12.
//  Copyright (c) 2012 RetVal. All rights reserved.
//

#include <stdio.h>
#include <RSCoreFoundation/RSBase.h>
#include <RSCoreFoundation/RSBaseHash.h>

/*Private Headers*/
#include "RSHashBlizzard.h"
#include "RSHashSHA-2.h"
//void _RSHashSHA2( RSCUBuffer input, RSUInteger ilen, RSUBlock output[64], BOOL is384 );
RSExport BOOL RSBaseHash(RSHashSelectorID selector, const void* hash, RSUInteger size, RSHashCode* hashCodeExt, RSUInteger codeSize)
{
    if (hashCodeExt == nil) return NO;
    __builtin_memset(hashCodeExt, 0, codeSize);
    RSHashCode hashCode = 0x0;
    RSUBlock output[64] = {0};
    switch (selector)
    {
        case RSDefaultHash:
        case RSBlizzardHash:
            if (codeSize < sizeof(RSHashCode)) return NO;
            hashCode = __RSBaseHashBlizzardHash(hash, size, 0);
            *hashCodeExt = hashCode;
            break;
        case RSSHA2Hash:
            if (codeSize < sizeof(RSUBlock)*64) return NO;
            _RSHashSHA2(hash, size, output, YES);
            __builtin_memcpy(hashCodeExt, output, sizeof(RSUBlock)*64);
            break;
        default:
            break;
    }
    return YES;
}
