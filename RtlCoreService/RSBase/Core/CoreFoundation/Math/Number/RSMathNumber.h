//
//  RSMathNumber.h
//
//
//  Created by RetVal on 10/13/12.
//  Copyright (c) 2012 RetVal. All rights reserved.
//

#ifndef RSCoreFoundation_RSMathNumber_h
#define RSCoreFoundation_RSMathNumber_h
#import <RSCoreFoundation/RSBase.h>
RSExport RSInteger  RSMathModulo(RSInteger baseNumber,RSInteger exponential,RSInteger modulus);
RSExport RSInteger  RSMathGcd(RSInteger a, RSInteger b);
RSExport RSInteger  RSMathLcm (RSInteger u, RSInteger v);
RSExport BOOL       RSMathIsPrimeNumber(RSBGNumber aNumber);
RSExport RSUInteger RSMathGetPrimitiveRoot (RSBGNumber p);


RSExport RSBitU64   RSMathNumberFitToBase(RSBitU64 aNumber, RSBitU64 aBase);

RSExport RSBitU64   RSMathNumberTo8(RSBitU64 aNumber);
RSExport RSBitU64   RSMathNumberTo16(RSBitU64 aNumber);
RSExport RSBitU64   RSMathNumberTo32(RSBitU64 aNumber);
RSExport RSBitU64   RSMathNumberTo64(RSBitU64 aNumber);
RSExport RSBitU64   RSMathNumberTo128(RSBitU64 aNumber);
RSExport RSBitU64   RSMathNumberTo256(RSBitU64 aNumber);
RSExport RSBitU64   RSMathNumberTo512(RSBitU64 aNumber);
RSExport RSBitU64   RSMathNumberTo1024(RSBitU64 aNumber);

RSExport RSFloat    RSReciprocalSqrt(RSFloat number);
RSExport RSFloat    RSFAbs(RSFloat f);
RSExport BOOL       RSIsPowerOf2(RSBitU64 v);
#endif
