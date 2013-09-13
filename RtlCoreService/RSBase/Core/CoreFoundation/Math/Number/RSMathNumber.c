//
//  RSMathNumber.c
//  RSCoreService
//
//  Created by RetVal on 10/13/12.
//  Copyright (c) 2012 RetVal. All rights reserved.
//

#include <stdio.h>

#import <RSCoreFoundation/RSMathNumber.h>

RSExport RSInteger RSMathModulo(RSInteger baseNumber,RSInteger exponential,RSInteger modulus)//a^b mod n
{
    RSInteger ret=1;
    for (; exponential; exponential >>= 1, baseNumber = (RSInteger)((RSBit64)baseNumber)* baseNumber % modulus)
        if (exponential & 1)
            ret=(RSInteger)((RSBit64)ret)*baseNumber%modulus;
    return ret;
}


RSExport RSInteger RSMathGcd(RSInteger a, RSInteger b)
{
    RSInteger t =  a % b;
    while( t > 0 )
    {
        a= b;
        b= t;
        t = a % b;
    }
    return b;
}

RSExport RSInteger RSMathLcm (RSInteger u, RSInteger v)
{
    RSInteger s, h, temp;
    h = u * v;
    while (v != 0)
    {
        temp = u % v;
        u = v;
        v = temp;
    }
    if (u < 0 || v < 0)
        return 0;
    else
        s = h / u;
    return s;
}

RSExport BOOL RSMathIsPrimeNumber(RSBGNumber aNumber)
{
    RSInteger time = 10;
    if (aNumber == 1 || (aNumber!=2&&!(aNumber%2))||(aNumber!=3&&!(aNumber%3))||(aNumber!=5&&!(aNumber%5))||(aNumber!=7&&!(aNumber%7)))
        return 0;
    while (time--)
        if (RSMathModulo(((arc4random()&0x7fff<<16)+arc4random()&0x7fff+arc4random()&0x7fff)%(aNumber-1)+1,aNumber-1,aNumber)!=1)
            return 0;
    return 1;
}


RSExport RSUInteger RSMathGetPrimitiveRoot (RSBGNumber p)
{
    RSUInteger fact[100] = {0};
    RSUInteger phi = p-1,  n = phi;
    RSUInteger idx = 0;
    for (RSUInteger i=2; i*i<=n; ++i)
        if (n % i == 0)
        {
            fact[idx++] = (i);
            while (n % i == 0)
                n /= i;
        }
    if (n > 1)
        fact[idx++] = (n);
    for (RSUInteger res=2; res<=p; ++res)
    {
        bool ok = YES;
        for (size_t i=0; i< idx && ok; ++i)
            ok &= RSMathModulo(res, phi / fact[i], p) != 1;
        if (ok)  return res;
    }
    return -1;
}

RSExport RSBitU64   RSMathNumberFitToBase(RSBitU64 aNumber, RSBitU64 aBase)
{
    return (((aNumber)+(aBase-1)) &~ (aBase-1));
}
RSExport RSBitU64   RSMathNumberTo8(RSBitU64 aNumber)
{
    return RSMathNumberFitToBase(aNumber, 8);
}
RSExport RSBitU64   RSMathNumberTo16(RSBitU64 aNumber)
{
    return RSMathNumberFitToBase(aNumber, 16);
}
RSExport RSBitU64   RSMathNumberTo32(RSBitU64 aNumber)
{
    return RSMathNumberFitToBase(aNumber, 32);
}
RSExport RSBitU64   RSMathNumberTo64(RSBitU64 aNumber)
{
    return RSMathNumberFitToBase(aNumber, 64);
}
RSExport RSBitU64   RSMathNumberTo128(RSBitU64 aNumber)
{
    return RSMathNumberFitToBase(aNumber, 128);
}
RSExport RSBitU64   RSMathNumberTo256(RSBitU64 aNumber)
{
    return RSMathNumberFitToBase(aNumber, 256);
}
RSExport RSBitU64   RSMathNumberTo512(RSBitU64 aNumber)
{
    return RSMathNumberFitToBase(aNumber, 512);
}
RSExport RSBitU64   RSMathNumberTo1024(RSBitU64 aNumber)
{
    return RSMathNumberFitToBase(aNumber, 1024);
}

RSExport RSFloat RSReciprocalSqrt(RSFloat number)
{
	RSInteger i;
	RSFloat x2, y;
	const RSFloat threehalfs = 1.5F;
    
	x2 = number * 0.5F;
	y  = number;
	i  = * ( RSInteger * ) &y;                  // evil floating point bit level hacking
	i  = 0x5f3759df - ( i >> 1 );               // what the fuck?
	y  = * ( RSFloat * ) &i;
	y  = y * ( threehalfs - ( x2 * y * y ) );   // 1st iteration
    //	y  = y * ( threehalfs - ( x2 * y * y ) );   // 2nd iteration, this can be removed
    
#ifndef Q3_VM
#ifdef __linux__
	assert( !isnan(y) ); // bk010122 - FPE?
#endif
#endif
	return y;
}

RSExport RSFloat RSFAbs(RSFloat f)
{
	RSInteger tmp = * ( RSInteger * ) &f;
	tmp &= 0x7FFFFFFF;
	return * ( RSFloat * ) &tmp;
}

RSExport BOOL RSIsPowerOf2(RSBitU64 v)
{
    return 0 == (v & (v - 1));
}