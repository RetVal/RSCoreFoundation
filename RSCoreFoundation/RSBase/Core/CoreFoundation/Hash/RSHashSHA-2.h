//
//  RSHashSHA-2.h
//  RSCoreFoundation
//
//  Created by RetVal on 10/15/12.
//  Copyright (c) 2012 RetVal. All rights reserved.
//

#ifndef RSCoreFoundation_RSHashSHA_2_h
#define RSCoreFoundation_RSHashSHA_2_h
#include <RSCoreFoundation/RSBase.h>
typedef struct __RSHashSHA2Context
{
    RSBitU64 total[2];          /*!< number of bytes processed  */
    RSBitU64 state[8];          /*!< intermediate digest state  */
    RSUBlock buffer[128];  /*!< data block being processed */

    RSUBlock ipad[128];    /*!< HMAC: inner padding        */
    RSUBlock opad[128];    /*!< HMAC: outer padding        */
    BOOL is384;                  /*!< 0 => SHA-512, else SHA-384 */
}
RSHashSHA2Context;
typedef RSHashSHA2Context* RSHashSHA2ContextRef;
void _RSHashSHA2( RSCUBuffer input, RSUInteger ilen, RSUBlock output[64], BOOL is384 );
void _RSHashSHA2HMAC(RSCUBuffer key, RSUInteger keylen,
                     RSCUBuffer input, RSUInteger ilen,
                     RSUBlock output[64], BOOL is384 );
static int sha4_self_test( int verbose );
#endif
