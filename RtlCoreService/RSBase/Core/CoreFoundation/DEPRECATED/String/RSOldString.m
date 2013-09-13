//
//  __RSOldString.m
//  RSKit
//
//  Created by RetVal on 10/13/12.
//  Copyright (c) 2012 RetVal. All rights reserved.
//

#include <stdio.h>
#include <RSKit/RSMemory.h>
#include <RSKit/RSOldString.h>
RSExport __RSOldStringError __RSOldStringFindMaxSubString(RSBuffer Resource1,RSBuffer Resource2,
                                      RSBufferRef maxSubString)
{
    __RSOldStringError iRet = kSuccess;
    RSUInteger lenOfSource1 = 0;
    RSUInteger lenOfSource2 = 0;
    RSUInteger historyOfSubOffset = 0;
    RSUInteger historyOfSubLength = 0;
    
    //RSUInteger moreOffset = 0;
    RSUInteger lessOffset = 0;
    RSUInteger currentLegnth = 0;
    
    RSUInteger moreLength = 0;
    RSUInteger lessLength = 0;
    
    RSBuffer more = nil;
    RSBuffer less = nil;
    RSBuffer baseMore = nil;
    RSBuffer baseLess = nil;
    
    
    RSBuffer pComprare = nil;
    RSBlock flag = 0;
    BOOL firstOrSecond = NO;
    BOOL bFindSubString = false;
    do{
        RCMP(Resource1, iRet);
        RCMP(Resource2, iRet);
        RCPP(maxSubString, iRet);
        lenOfSource1 = strlen(Resource1);
        lenOfSource2 = strlen(Resource2);
        firstOrSecond = (lenOfSource1 >= lenOfSource2)?(YES):(NO);
        if (firstOrSecond)
        {
            __RSOldStringCopyByLength(Resource1, &more, moreLength = lenOfSource1);
            __RSOldStringCopyByLength(Resource2, &less, lessLength = lenOfSource2);
        }
        else
        {
            __RSOldStringCopyByLength(Resource2, &more, moreLength = lenOfSource2);
            __RSOldStringCopyByLength(Resource1, &less, lessLength = lenOfSource1);
        }
        baseMore = more;
        baseLess = less;
        
        RSUInteger tmpOffset = 0;
        
        for (lessOffset = 0; lessOffset < lessLength; lessOffset++)
        {
            pComprare = less + lessOffset;
            for (currentLegnth = 1; currentLegnth < lessLength; currentLegnth++)
            {
                flag = pComprare[currentLegnth];
                pComprare[currentLegnth] = 0;
                bFindSubString = __RSOldStringGetIndex(more, pComprare, &tmpOffset);
                
                if (bFindSubString)
                {
                    RSUInteger tmpLength = 0;
                    tmpLength = strlen(pComprare);
                    if (tmpLength > historyOfSubLength)
                    {
                        historyOfSubLength = tmpLength;
                        historyOfSubOffset = tmpOffset;
                    }
                }
                else
                {
                    tmpOffset = 0;
                }
                pComprare[currentLegnth] = flag;
            }
        }
        if (historyOfSubLength)
        {
            __RSOldStringCopyByOffset(more, maxSubString, historyOfSubOffset, historyOfSubOffset + historyOfSubLength);
        }
    } while (0);
    RSMmFree((RSZone)&baseMore);
    RSMmFree((RSZone)&baseLess);
    return iRet;
}

RSExport BOOL	__RSOldStringExchange(RSBuffer Resources,
                              RSBuffer WantFind,
                              RSBuffer Exchange,
                              RSBuffer put)
{
	RSUInteger lenofResources = 0;
	RSUInteger lenofWantFind = 0;
	RSUInteger i = 0;
	BOOL bRet = 0;
	
	if((!Resources) || (!WantFind) || (!Exchange) || (!put))
		return false;
	if(Resources == NULL || WantFind == NULL || put == NULL)
		return false;
	
	lenofResources = strlen(Resources);
	lenofWantFind = strlen(WantFind);
	if(lenofWantFind > lenofResources)
		return false;
	
	bRet = __RSOldStringGetIndex(Resources,WantFind,&i); //WantFd = Resources[i]  (length = length of wantfd)
	if(!bRet) return bRet;
	
	if(i<lenofResources)
	{
        // 		if(strlen(put) > (lenofResources + strlen(Exchange) - lenofWantFind))
        // 			return false;
		memcpy(put,Resources,i*sizeof(RSBlock));
		memcpy(put + i*sizeof(RSBlock),Exchange,sizeof(RSBlock)*strlen(Exchange));
		Resources += i+lenofWantFind;
		memcpy(	put + i*sizeof(RSBlock) + sizeof(RSBlock)*strlen(Exchange),Resources,
               sizeof(RSBlock)*(lenofResources - i - lenofWantFind));
		Resources -= i+lenofWantFind;
        if (Resources)
        {
            ;//just pass the analysis
        }
	}
	return true;
}
RSExport BOOL	__RSOldStringGetIndex(RSBuffer Resources,
                              RSBuffer WantFind,
                              RSUIntegerRef index)
{
	RSBuffer	tmpToResources = NULL;
	RSBlock	tmpRtlBlock = ~0;
	RSUInteger	i = 0;
	RSUInteger	j = 0;
	RSUInteger	lenofResources = 0;
	RSUInteger	lenofWantFd = 0;
	BOOL	bRet = true;
	if(Resources == NULL || WantFind == NULL || index == NULL)
		return !bRet;
	lenofResources = strlen(Resources);
	lenofWantFd = strlen(WantFind);
    if (lenofResources <= 0 )
    {
        return NO;
    }
	if(lenofWantFd > lenofResources)
		return !bRet;
	if (lenofResources == lenofWantFd)
	{
		if(0 == strcmp(Resources,WantFind))
		{
			*index = 0;
			return	YES;
		}
		*index = 0;
		return NO;
	}
	
	*index = ~0;
	
    RSMmAlloc((RSZone)&tmpToResources, sizeof(RSBlock)*(lenofResources+1));
    
	if(!tmpToResources)
		return !bRet;
	memset((RSHandle)tmpToResources,0,sizeof(RSBlock)*lenofResources+1);
	for(j=0;j<lenofResources;j++)
		tmpToResources[j] = (Resources[j]);
	if (lenofResources == 1)
	{
		if (lenofWantFd == 1)
		{
			if (tmpToResources[0] == WantFind[0])
			{
				bRet = true;
				RSMmFree((RSZone)&tmpToResources);
				return bRet;
			}
		}
		bRet = false;
		RSMmFree((RSZone)&tmpToResources);
		return bRet;
	}
	while(1)
	{
		tmpRtlBlock = tmpToResources[lenofWantFd];
		tmpToResources[lenofWantFd] = '\0';
		if(!strcmp(tmpToResources,WantFind))
			break;
		tmpToResources[lenofWantFd] = tmpRtlBlock;
		i++;
		tmpToResources++;
		if(lenofWantFd >= (lenofResources-i))
		{
			if(strcmp(tmpToResources,WantFind))
				bRet = !bRet;
			break;
		}
	}
	if(tmpToResources)
	{
		tmpToResources[lenofWantFd] = tmpRtlBlock;
		tmpToResources -= i;
		RSMmFree((RSZone)&tmpToResources);
	}
	if(bRet)
		*index = i;
	return bRet;
}
RSExport BOOL	__RSOldStringGetIndexEx(RSBuffer Resources,
                                RSBuffer WantFd,
                                RSUInteger GetLength,
                                RSUIntegerRef dex)
{
	RSBuffer	tmpToResources = NULL;
	RSBlock	tmpRtlBlock = ~0;
	RSBlock	tmpEndRtlBlock = ~0;
	RSUInteger	i = 0;
	RSUInteger	j = 0;
	RSUInteger	lenofResources = 0;
	RSUInteger	lenofWantFind = 0;
	BOOL	bRet = true;
	if(Resources == NULL || WantFd == NULL || dex == NULL || GetLength <= 0)
		return !bRet;
	lenofResources = strlen(Resources);
	lenofWantFind = strlen(WantFd);
	if(lenofWantFind > lenofResources)
		return !bRet;
	if(GetLength < lenofWantFind)
		return !bRet;
	GetLength = min(GetLength,lenofResources);
	
	*dex = ~0;
	
    RSMmAlloc((RSZone)&tmpToResources, sizeof(RSBlock)*(lenofResources+1));
	if(!tmpToResources)
		return !bRet;
	memset((RSHandle)tmpToResources,0,sizeof(RSBlock)*lenofResources+1);
	for(j=0;j<lenofResources;j++)
		tmpToResources[j] = tolower(Resources[j]);
	if (lenofResources == 1)
	{
		if (lenofWantFind == 1)
		{
			if (tmpToResources[0] == WantFd[0])
			{
				bRet = true;
				RSMmFree((RSZone)&tmpToResources);
				return bRet;
			}
		}
		bRet = false;
		RSMmFree((RSZone)&tmpToResources);
		return bRet;
	}
	tmpEndRtlBlock = tmpToResources[GetLength + 1];
	tmpToResources[GetLength + 1] = '\0';
	while(1)
	{
		tmpRtlBlock = tmpToResources[lenofWantFind];
		tmpToResources[lenofWantFind] = '\0';
		if(!strcmp(tmpToResources,WantFd))
			break;
		tmpToResources[lenofWantFind] = tmpRtlBlock;
		i++;
		tmpToResources++;
		if(lenofWantFind >= (lenofResources-i))
		{
			if(strcmp(tmpToResources,WantFd))
				bRet = !bRet;
			break;
		}
	}
	if(tmpToResources)
	{
		tmpToResources[GetLength + 1] = tmpEndRtlBlock;
		tmpToResources[lenofWantFind] = tmpRtlBlock;
		tmpToResources -= i;
        RSMmFree((RSZone)&tmpToResources);
	}
	if(bRet)
		*dex = i;
	return bRet;
}
RSExport BOOL    __RSOldStringGetPreIndex(RSBuffer	Resources,
                                   RSBuffer WantFd,
                                   RSUInteger Sentinel,
                                   RSUIntegerRef index)
{
	BOOL iRet = false;
	do
	{
		if (Resources == nil || WantFd == nil || index == nil)
		{
			return	iRet;
		}
		//RSUInteger nearEst = 0;
		//RSUInteger tmpOffset = 0;
		RSUInteger lenOfRes = strlen(Resources);
		RSUInteger lenOfWfd = strlen(WantFd);
		if (lenOfRes < Sentinel || lenOfRes < lenOfWfd)
		{
			return	iRet;
		}
		RSBuffer	realRes = Resources;
		RSBlock	endRtlBlock = Resources[Sentinel + 1];
		realRes[Sentinel + 1] = '\0';
		if ((iRet = __RSOldStringGetLast(realRes,WantFd,index)))
		{
			;
		}
		realRes[Sentinel + 1] = endRtlBlock;
	} while (0);
	return	iRet;
}
RSExport BOOL    __RSOldStringGetNxtIndex(RSBuffer	Resources,
                                   RSBuffer WantFd,
                                   RSUInteger Sentinel,
                                   RSUIntegerRef index)
{
	BOOL iRet = false;
	do
	{
		if (Resources == nil || WantFd == nil || index == nil)
		{
			return	iRet;
		}
		RSUInteger lenOfRes = strlen(Resources);
		RSUInteger lenOfWfd = strlen(WantFd);
		if (lenOfRes < Sentinel || lenOfRes < lenOfWfd)
		{
			return	iRet;
		}
		RSBuffer	realRes = Resources + Sentinel;
        
		if ((iRet = __RSOldStringGetIndex(realRes,WantFd,index)))
		{
			*index += Sentinel;
		}
	} while (0);
	return	iRet;
}
RSExport BOOL	__RSOldStringGetNextBlock(RSBuffer Resources,
                                  RSBuffer WantFd1,
                                  RSBuffer WantFd2,
                                  RSUInteger * autoOffset,
                                  int * nSwitch,
                                  RSUIntegerRef index)
{
	BOOL iRet = false;
	do
	{
		if (Resources == nil || WantFd1 == nil || WantFd2 == nil || index == nil || autoOffset == nil || nSwitch == nil)
		{
			return iRet;
		}
		RSUInteger lenOfRes = strlen(Resources);
		if (lenOfRes <= *autoOffset)
		{
			return iRet;
		}
		RSUInteger offset1 = 0;
		RSUInteger offset2 = 0;
		BOOL ret1 = false;
		BOOL ret2 = false;
#ifdef	_DEBUG
		RSBuffer tmpDebugPtr = Resources + *autoOffset;
#endif
		*index = 0;
		ret1 = __RSOldStringGetIndex(Resources + *autoOffset,WantFd1,&offset1);
		ret2 = __RSOldStringGetIndex(Resources + *autoOffset,WantFd2,&offset2);
		iRet = true;
		if (ret1 && ret2)
		{
			if (offset1 <= offset2)
			{
				*nSwitch = 1;
				*autoOffset += strlen(WantFd1);
			}
			else
			{
				*nSwitch = 2;
				*autoOffset += strlen(WantFd2);
			}
			*index = min(offset1,offset2);
			*autoOffset += *index;
		}
		else if (ret1 && !ret2)
		{
			*nSwitch = 1;
			*index = offset1;
			*autoOffset += *index + strlen(WantFd1);
		}
		else if (!ret1 && ret2)
		{
			*nSwitch = 2;
			*index = offset2;
			*autoOffset += *index + strlen(WantFd2);
		}
		else if (!ret1 && !ret2)
		{
			*nSwitch = 0;
			*index = ~0;
			iRet = false;
		}
        
	} while (0);
	return	iRet;
}
RSExport BOOL	__RSOldStringGetCounterWithSentinel(RSBuffer Resources,
                                            RSBuffer WantFd,
                                            RSUInteger sentinelS,
                                            RSUInteger sentinelE,
                                            RSUIntegerRef counter)
{
	BOOL	iRet = false;
	RSUInteger lenOfRes = 0;
	//RSBlock	orgSentinel = 0;
	RSBuffer	realPtr = nil;
	do
	{
		if (Resources == nil || WantFd == nil || counter == nil)
		{
			return	iRet;
		}
		if (sentinelS > sentinelE)
		{
			return	iRet;
		}
		lenOfRes = strlen(Resources);
		if (lenOfRes < sentinelE)
		{
			return	iRet;
		}
		//orgSentinel = Resources[sentinelE];
		Resources[sentinelE] = '\0';
		realPtr = Resources + sentinelS;
		iRet = __RSOldStringGetCounter(realPtr,WantFd,counter);
	} while (0);
	return	iRet;
}
RSExport BOOL	__RSOldStringGetLast(RSBuffer Resources,
                             RSBuffer WantFd,
                             RSUIntegerRef dex)
{
	BOOL IsSuccess = false;
	RSUInteger dwdex = 0;
	RSBuffer tmpResources = NULL;
	if(Resources == NULL || WantFd == NULL || dex == NULL)
		return IsSuccess;
	tmpResources = Resources;
	*dex = 0;
	//µ›πÈµ˜”√’“∞°’“£¨÷±µΩ∑µªÿfalse£®√ª’“µΩ£©£¨º¥Œ™◊Ó∫Û“ª∏ˆ
	while(__RSOldStringGetIndex(tmpResources,WantFd,&dwdex))
	{
		IsSuccess = true;
		*dex += ++dwdex;//
		tmpResources += dwdex;
		dwdex = 0;
	}
	return IsSuccess;
}
RSExport BOOL 	__RSOldStringGetCounter(RSBuffer Resources,
                                RSBuffer WantFd,
                                RSUIntegerRef Counter)
{
	BOOL IsSuccess = false;
	RSUInteger dwdex = 0;
	RSBuffer tmpResources = NULL;
	if(Resources == NULL || WantFd == NULL || Counter == NULL)
		return IsSuccess;
	tmpResources = Resources;
	*Counter = 0;
	
	while(__RSOldStringGetIndex(tmpResources,WantFd,&dwdex))
	{
		IsSuccess = true;
		Counter++;
		tmpResources += dwdex;
		dwdex = 0;
	}
	return IsSuccess;
}
RSExport int		__RSOldStringCmp(RSBuffer Dec,
                             RSBuffer Src,
                             RSUInteger uLength,
                             BOOL bIsCase)
{
	RSUInteger nCount = 0;
	if((!Dec) || (!Src) || uLength <= 0)
		return -1;
    
	RSBuffer Dectmp = Dec;
	RSBuffer Srctmp = Src;
	if(bIsCase)
		__RSOldStringToUpper(Dec,uLength);
	else
		__RSOldStringToLowwer(Dec,uLength);
	for(nCount = 0; nCount < uLength ; nCount++)
	{
		if(Dectmp[nCount] != Srctmp[nCount])
			return abs((Dectmp[nCount] - Srctmp[nCount]));
	}
	return  0;
}
RSExport int		__RSOldStringCmpByOffset(RSBuffer Dec,
                                     RSBuffer Src,
                                     RSUInteger uStartOffset,
                                     RSUInteger uEndOffset,
                                     BOOL bIsCase)
{
	int nRet = 0;
	RSUInteger nCount = 0;
	RSUInteger uLength = 0;
	if((!Dec) || (!Src))
		return -1;
    do
    {
        BWI(nRet = ( uEndOffset >= uStartOffset) ? (0) : (-1));
        BWI(nRet = (strlen(Dec) >= uEndOffset  ) ? (0) : (-1));
        
    } while (0);
	if (nRet == -1)
    {
        return nRet;
    }
	RSBuffer Dectmp = Dec;
	RSBuffer Srctmp = Src;
	if(bIsCase)
		__RSOldStringToUpper(Dec,uLength);
	else
		__RSOldStringToLowwer(Dec,uLength);
	uLength = uEndOffset - uStartOffset;
	Dectmp += uStartOffset;
	for(nCount = 0; nCount < uLength ; nCount++)
	{
		if(Dectmp[nCount] != Srctmp[nCount])
			return abs((Dectmp[nCount] - Srctmp[nCount]));
	}
	return nRet;
}
RSExport void	__RSOldStringToUpper(RSBuffer Dec,
                             RSUInteger uLength)
{
	RSUInteger nCount = 0;
	//RSUInteger iuLength = 0;
	RSBuffer Dectmp = Dec;
	for(nCount = 0; nCount < uLength ; nCount++)
		if((Dectmp[nCount] > 'a') && (Dectmp[nCount] < 'z'))
			Dectmp[nCount] &= ~0x20;//ªπ «Ω‚ Õ“ªœ¬∞…’‚¿Ô£¨»•ø¥ø¥2Ω¯÷∆µƒASCII¬Î±Ì
	return;
}
RSExport void	__RSOldStringToLowwer(RSBuffer Dec,
                              RSUInteger uLength)
{
	
	RSUInteger nCount = 0;
	//RSUInteger iuLength = 0;
	RSBuffer Dectmp = Dec;
	for(nCount = 0; nCount < uLength ; nCount++)
        if(Dectmp[nCount] > 'A' && Dectmp[nCount] < 'Z')
            Dectmp[nCount] |= 0x20;//»•ø¥2Ω¯÷∆µƒASCII¬Î±Ì
	return;
}
RSExport __RSOldStringError	__RSOldStringCopyByLength(RSBuffer pszCopyBuffer,
                                  RSBuffer *ppszBuffer,
                                  RSUInteger uCopyLength)
{
	__RSOldStringError	uRet = kSuccess;
	RSUInteger	uStrgLength = 0;
	//RSBlock	cTmpBuffer = 0;
	do
	{
		if(nil == pszCopyBuffer)
		{
			uRet = kErrNil;
			break;
		}
		if(!(ppszBuffer))
		{
			uRet = kErrNil;
			break;
		}
		else
		{
			if(*ppszBuffer)
			{
				uRet = kErrNNil;
				break;
			}
		}
		if(!uCopyLength)
			return uRet;
		uStrgLength = strlen(pszCopyBuffer);
		if(uStrgLength < uCopyLength)
            BWI(uRet = kErrVerify);
		uStrgLength = min(uStrgLength,uCopyLength);
		uStrgLength++;
		if((uRet = RSMmAlloc((RSZone)ppszBuffer,uStrgLength)))
		{
			break;
		}
		memset((RSHandle)*ppszBuffer,0,sizeof(RSBlock)*(uStrgLength));
		memcpy(*ppszBuffer,pszCopyBuffer,uCopyLength);
	}
	while(0);
	return uRet;
}
RSExport __RSOldStringError	__RSOldStringCopyByOffset(RSBuffer pszCopyBuffer,
                                  RSBuffer *ppszBuffer,
                                  RSUInteger uStartOffset,
                                  RSUInteger uEndOffset)
{
	RSUInteger uRet = kSuccess;
	RSUInteger uStrgLength = 0;
	RSUInteger uCopyLength = 0;
	do
	{
		if(!pszCopyBuffer)
		{
			uRet = kErrNil;
			break;
		}
		if(!(ppszBuffer))
		{
			uRet = kErrNil;
			break;
		}
		else
		{
			if(*ppszBuffer)
			{
				uRet = kErrNNil;
				break;
			}
		}
		if(uStartOffset > uEndOffset)
		{
			uRet = kErrVerify;
			break;
		}
		if(!uEndOffset)
			return uRet;
		uCopyLength = uEndOffset - uStartOffset;
		uStrgLength = strlen(pszCopyBuffer);
		if(uStrgLength < uCopyLength)
		{
			uRet = kErrVerify;
			break;
		}
		pszCopyBuffer += uStartOffset;
		BWI(uRet = __RSOldStringCopyByLength(pszCopyBuffer,ppszBuffer,uCopyLength));
		pszCopyBuffer -= uStartOffset;
        if (pszCopyBuffer)
        {
            ;//just pass the analysis
        }
	}
	while(0);
	return uRet;
}
RSExport __RSOldStringError   __RSOldStringFilterCallEx(RSBuffer pFilterBuffer,
                                    RSBuffer pReserved ,
                                    RSBuffer pReservedE ,
                                    RSUIntegerRef puFilterCode,
                                    RSBuffer *ppBuffer,
                                    RSUIntegerRef puStartOffset,
                                    RSUIntegerRef puEndOffset)
{
	static	RSUInteger	uCallCount = 0;
	static	BOOL	bIsHaveChange = false;
	__RSOldStringError	uRet = kSuccess;
	RSUInteger	uSize = 0;
	RSUInteger	uStartOffset = 0;
	RSUInteger	uEndOffset = 0;
	//RSUInteger	uFilterCode = 0;
	RSUInteger	uBufferLength = 0;
	RSBuffer	pBufferCopyBase = NULL;	   //
	RSBuffer	pBufferCopy = NULL;
	RSBuffer	pBufferCopyTmpA = NULL;//
	RSBuffer	pBufferCopyTmpB = NULL;//
	RSBuffer	pBufferCopyTmpC = NULL;//”√”⁄
	RSBuffer	pBufferCopyExcg = NULL;//”√”⁄µ›πÈΩªªªƒ⁄¥ÊøÈ
	RSBlock	cFlagOfEnter[2] = {0};//0x0D;
	RSBlock	cFlagOfSpace[2] = " ";//0x20;
	RSBlock	cFlagOfSemicolon[2] = ";";//0x3B;
	RSBlock	cFlagCall[2] = {0};
	BOOL	bRetA = false;
	BOOL	bRetB = false;
	cFlagOfEnter[0] = 0x0D;
	do
	{
		if(uCallCount)
			goto NEXT1;
		if(!pFilterBuffer)
		{
			uRet = kErrNil;
			break;
		}
		if(!puFilterCode)
		{
			uRet = kErrNil;
			break;
		}
		if(!ppBuffer)
		{
			uRet = kErrNil;
			break;
		}
		if(!(*puFilterCode & KX_FILTER_ANTI_USER))
		{
			if((*puFilterCode & (KX_FILTER_ANTI_SEMICOLON|KX_FILTER_ANTI_ENTER|KX_FILTER_ANTI_SPACE)) == (KX_FILTER_ANTI_SEMICOLON|KX_FILTER_ANTI_ENTER|KX_FILTER_ANTI_SPACE))
			{
				uRet = kErrVerify;
				break;
			}
			if((*puFilterCode & (KX_FILTER_ANTI_SEMICOLON|KX_FILTER_ANTI_ENTER)) == (KX_FILTER_ANTI_SEMICOLON|KX_FILTER_ANTI_ENTER))
			{
				uRet = kErrVerify;
				break;
			}
			if((*puFilterCode & (KX_FILTER_ANTI_SEMICOLON|KX_FILTER_ANTI_SPACE)) == (KX_FILTER_ANTI_SEMICOLON|KX_FILTER_ANTI_SPACE))
			{
				uRet = kErrVerify;
				break;
			}
			if((*puFilterCode & (KX_FILTER_ANTI_ENTER|KX_FILTER_ANTI_SPACE)) == (KX_FILTER_ANTI_ENTER|KX_FILTER_ANTI_SPACE))
			{
				uRet = kErrVerify;
				break;
			}
			if((*puFilterCode & (KX_FILTER_ANTI_ENDSEMICOLON|KX_FILTER_ANTI_ENDENTER|KX_FILTER_ANTI_ENDEND)) == (KX_FILTER_ANTI_ENDSEMICOLON|KX_FILTER_ANTI_ENDENTER|KX_FILTER_ANTI_ENDEND))
			{
				uRet = kErrVerify;
				break;
			}
			if((*puFilterCode & (KX_FILTER_ANTI_ENDSEMICOLON|KX_FILTER_ANTI_ENDENTER)) == (KX_FILTER_ANTI_ENDSEMICOLON|KX_FILTER_ANTI_ENDENTER))
			{
				uRet = kErrVerify;
				break;
			}
			if((*puFilterCode & (KX_FILTER_ANTI_ENDSEMICOLON|KX_FILTER_ANTI_ENDEND)) == (KX_FILTER_ANTI_ENDSEMICOLON|KX_FILTER_ANTI_ENDEND))
			{
				uRet = kErrVerify;
				break;
			}
			if((*puFilterCode & (KX_FILTER_ANTI_ENDENTER|KX_FILTER_ANTI_ENDEND)) == (KX_FILTER_ANTI_ENDENTER|KX_FILTER_ANTI_ENDEND))
			{
				uRet = kErrVerify;
				break;
			}
		}
		else
		{
			if(!pReserved)
			{
				uRet = kErrNil;
				break;
			}
			if(!pReservedE)
			{
				uRet = kErrNil;
				break;
			}
		}
		//≤Œ ˝ºÏ≤‚ÕÍ≥…
    NEXT1:
		uCallCount ++;
		uBufferLength = strlen(pFilterBuffer);
		if(RSMmAlloc((RSZone)&pBufferCopy,uBufferLength+1))
		{
			uRet = kErrMmRes;
			break;
		}
		pBufferCopyBase = pBufferCopy;
		memset((RSHandle)pBufferCopy,0,sizeof(RSBlock)*(uBufferLength+1));
		//strcpy(pBufferCopy,pFilterBuffer);
		memcpy(pBufferCopy,pFilterBuffer,sizeof(RSBlock)*(uBufferLength+1));
		if(!( *puFilterCode & KX_FILTER_ANTI_NOCHANGE))
		{
			if(*puFilterCode & KX_FILTER_ANTI_LOWWER)
				__RSOldStringToLowwer(pBufferCopy,uBufferLength);
			else
				if(*puFilterCode & KX_FILTER_ANTI_UPPER)
					__RSOldStringToUpper(pBufferCopy,uBufferLength);
		}
		if(!(*puFilterCode & KX_FILTER_ANTI_USER))
		{
			if(*puFilterCode & KX_FILTER_ANTI_FLAGS)
			{
				memset(cFlagCall,0,sizeof(RSBlock)*2);
				if(*puFilterCode & KX_FILTER_ANTI_SEMICOLON)
					strcpy(cFlagCall,cFlagOfSemicolon);
				else
					if(*puFilterCode & KX_FILTER_ANTI_ENTER)
						strcpy(cFlagCall,cFlagOfEnter);
                    else
                        if(*puFilterCode & KX_FILTER_ANTI_SPACE)
                            strcpy(cFlagCall,cFlagOfSpace);
				bRetA = __RSOldStringGetIndex(pBufferCopy,(RSBuffer)cFlagCall,&uStartOffset);
				if(bRetA)
				{
					if(uStartOffset <= uBufferLength)
						pBufferCopy += uStartOffset;
				}
			}
			if(*puFilterCode & KX_FILTER_ANTI_ENDFLAGS)
			{
				memset(cFlagCall,0,sizeof(RSBlock)*2);
				if(*puFilterCode & KX_FILTER_ANTI_ENDSEMICOLON)
					strcpy(cFlagCall,cFlagOfSemicolon);
				else
					if(*puFilterCode & KX_FILTER_ANTI_ENDENTER)
						strcpy(cFlagCall,cFlagOfEnter);
                    else
                        if(*puFilterCode & KX_FILTER_ANTI_ENDEND)
                            memset(cFlagCall,0,sizeof(RSBlock)*2);
				if(*cFlagCall)
					bRetB = __RSOldStringGetIndex(pBufferCopy,(RSBuffer)cFlagCall,&uEndOffset);
				else
				{
					bRetB = __RSOldStringGetIndex(pBufferCopy,(RSBuffer)cFlagOfEnter,&uEndOffset);
					if(!bRetB)
					{
						uEndOffset = strlen(pBufferCopy);
						bRetB = true;
					}
				}
				if(bRetB)
					uEndOffset += uStartOffset;
				else
					if(bRetA)
					{
						if(*cFlagCall == '\0')
							strcpy(cFlagCall,cFlagOfEnter);
						else
							if(!strcmp(cFlagCall,cFlagOfEnter))
								memset(cFlagCall,0,sizeof(RSBlock)*2);
						if(*cFlagCall)
							bRetB = __RSOldStringGetIndex(pBufferCopy,(RSBuffer)cFlagCall,&uEndOffset);
						else
						{
							strcpy(cFlagCall,cFlagOfEnter);
							bRetB = __RSOldStringGetIndex(pBufferCopy,(RSBuffer)cFlagCall,&uEndOffset);
							if(!bRetB)
							{
								uEndOffset = strlen(pBufferCopy);
								bRetB = true;
							}
						}
						if(bRetB)
							uEndOffset += uStartOffset;
					}
			}
			if(bRetA)
			{
				if(uStartOffset <= uBufferLength)
					pBufferCopy -= uStartOffset;
			}
			else
			{
				/*if(NULL == *ppBuffer)
                 if(!__RSOldStringCopyByOffset(pBufferCopy,
                 ppBuffer,
                 0,
                 uBufferLength))
                 break;*/
			}
		}
		else
		{
			bRetA = __RSOldStringGetIndex(pBufferCopy,pReserved,&uStartOffset);
			if(bRetA)
			{
				if(uStartOffset <= uBufferLength)
					pBufferCopy += uStartOffset;
			}
			bRetB = __RSOldStringGetIndex(pBufferCopy,pReservedE,&uEndOffset);
			if(bRetB)
				uEndOffset += uStartOffset;
			if(bRetA)
			{
				if(uStartOffset <= uBufferLength)
					pBufferCopy -= uStartOffset;
			}
			break;
		}
	}
	while(0);
	if(bRetA && bRetB)
	{
		if(*ppBuffer == NULL)
		{
			if(RSMmAlloc((RSZone)ppBuffer,uBufferLength - (uEndOffset - uStartOffset) + 2))
			{
				uRet = kErrMmRes;
				goto END;
			}
			else
			{
				memset((RSHandle)*ppBuffer,0,sizeof(RSBlock)*(uBufferLength - (uEndOffset - uStartOffset) + 2));
			}
		}
		else
		{
			if(!RSMmAlloc((RSZone)&pBufferCopyTmpC,(uBufferLength - (uEndOffset - uStartOffset) + 2) + strlen(*ppBuffer) + 2))
			{
				memset(pBufferCopyTmpC,0,sizeof(RSBlock)*(uBufferLength - (uEndOffset - uStartOffset) + 2) + strlen(*ppBuffer) + 2);
				strcpy(pBufferCopyTmpC,*ppBuffer);
				pBufferCopyExcg = *ppBuffer;
				*ppBuffer = pBufferCopyTmpC;
				pBufferCopyTmpC = pBufferCopyExcg;
				pBufferCopyExcg = NULL;//free the TMPC  the end.
			}
			else
			{
				uRet = kErrMmRes;
				goto END;
			}
		}
        
		if(uStartOffset)
			bRetA = __RSOldStringCopyByOffset(      pBufferCopy,
                                      &pBufferCopyTmpA,
                                      0,
                                      uStartOffset);
		if(uEndOffset < uBufferLength)
        {
			if(!(*puFilterCode & KX_FILTER_ANTI_USER))
			{
				bRetB = __RSOldStringCopyByOffset(	pBufferCopy,
                                          &pBufferCopyTmpB,
                                          uEndOffset,
                                          uBufferLength);
			}
			else
			{
				bRetB = __RSOldStringCopyByOffset(	pBufferCopy,
                                          &pBufferCopyTmpB,
                                          uEndOffset + strlen(pReservedE),
                                          uBufferLength);
			}
        }
		if(!bRetA)
			strcpy(*ppBuffer,pBufferCopyTmpA);
		/*if(!strcmp((RSBuffer)cFlagCall,(RSBuffer)cFlagOfEnter))
         strcat(*ppBuffer,(RSBuffer)cFlagOfEnter);*/
		if(!bRetB)
			strcat(*ppBuffer,pBufferCopyTmpB);
		if(!bRetA || !bRetB)
			bIsHaveChange = true;
	}
	else
		uRet = kErrUnknown;
END:
	if(puStartOffset)
		if(uStartOffset)
			*puStartOffset = uStartOffset;
	if(puEndOffset)
		if(uEndOffset)
			*puEndOffset = uEndOffset;
	if(pBufferCopyBase)
		RSMmFree((RSZone)&pBufferCopyBase);
	if(pBufferCopyTmpA)
		RSMmFree((RSZone)&pBufferCopyTmpA);
	if(pBufferCopyTmpB)
		RSMmFree((RSZone)&pBufferCopyTmpB);
	if(pBufferCopyTmpC)
		RSMmFree((RSZone)&pBufferCopyTmpC);
	if(!uRet)
		while(!(uRet = __RSOldStringFilterCallEx(	*ppBuffer,
                                         pReserved,
                                         pReservedE,
                                         puFilterCode,
                                         ppBuffer,
                                         puStartOffset,
                                         puEndOffset)));
	uCallCount --;
	if(!uCallCount)
	{
		if(kErrUnknown == uRet)
		{
			if(!bIsHaveChange)
			{
				if(kSuccess == RSMmAlloc((RSZone)ppBuffer,uSize = strlen(pFilterBuffer) + 1))
				{
					memset(*ppBuffer,0,sizeof(RSBlock)*uSize);
					strcpy(*ppBuffer,pFilterBuffer);
				}
				else
					return kErrMmRes;
			}
			uRet = kSuccess;
			if(*puFilterCode & KX_FILTER_ANTI_SYMBOL)
			{
				RSBuffer tmp = NULL;
				if(uRet == RSMmAlloc((RSZone)&tmp,uSize))
				{
					memset(tmp,0,sizeof(RSBlock)*uSize);
				}
				uRet = __RSOldStringFilterCall(pFilterBuffer,KX_FILTER_ANTI_SYMBOL,&uSize,tmp);
				if(kSuccess == uRet)
				{
					memset(*ppBuffer,0,uSize);
				}
				else
				{
                    
				}
				RSMmFree((RSZone)&tmp);
			}
			return kSuccess;
		}
	}
	uCallCount = 0;
	return uRet;
}
RSExport __RSOldStringError	__RSOldStringFilterCall(RSBuffer pszFilterBuffer,
                                        RSUInteger uFilterCode,
                                        RSUIntegerRef puBufferLength,
                                        RSBuffer pBuffer)
{
	RSUInteger uRet = kSuccess;
	RSUInteger uRealBufferLength = 0;
	RSBuffer pszTmpBuffer = NULL;
	unsigned int i = 0;
	unsigned int j = 0;
	if((!puBufferLength)||(!&pBuffer))
		return kErrNil;
	if(pBuffer == NULL)
		return kErrNNil;
	uRealBufferLength = strlen(pszFilterBuffer);
	*puBufferLength = uRealBufferLength;
	if((uRet = RSMmAlloc((RSZone)&pszTmpBuffer,uRealBufferLength+1)))
		return uRet;
	memset(pszTmpBuffer,0,uRealBufferLength+1);
	
	strcpy(pszTmpBuffer,pszFilterBuffer);
	do
	{
		if(uFilterCode & KX_FILTER_ANTI_ALPHA)
		{
			for(i = 0,j = 0; i < uRealBufferLength+1;i++)
			{
				if((('a' <= pszTmpBuffer[i]) && (pszTmpBuffer[i] <= 'z'))
                   ||(('A' <= pszTmpBuffer[i])&&(pszTmpBuffer[i] <= 'Z'))
                   ||(('0' <= pszTmpBuffer[i])&&(pszTmpBuffer[i] <= '9')))
				{
					pBuffer[j] = pszTmpBuffer[i];
					j++;
				}
			}
		}
		if(uFilterCode & KX_FILTER_ANTI_SYMBOL)
		{
			for(i = 0,j = 0; i < uRealBufferLength+1;i++)
			{
				if((('a' <= pszTmpBuffer[i]) && (pszTmpBuffer[i] <= 'z'))
                   ||(('A' <= pszTmpBuffer[i])&&(pszTmpBuffer[i] <= 'Z'))
                   ||(('0' <= pszTmpBuffer[i])&&(pszTmpBuffer[i] <= '9'))
                   ||(((RSBlock)pszTmpBuffer[i]-'z')))
				{
					if((pszTmpBuffer[i] != '\r')
                       &&(pszTmpBuffer[i] != '-')
                       &&(pszTmpBuffer[i] != 0xD))
					{
						pBuffer[j] = pszTmpBuffer[i];
						j++;
					}
				}
                
			}
		}
	}
	while(0);
	
	
EXIT_FREE:
	RSMmFree((RSZone)&(pszTmpBuffer));
	return uRet;
}
RSExport __RSOldStringError	__RSOldStringAntiCopyCall(RSBuffer pszAntiBuffer,
                                  RSBuffer pszRetBuffer,
                                  RSUIntegerRef uRetBufferLength)
{
	__RSOldStringError uRet = kSuccess;
	RSUInteger uRealBufferLength = 0;
	RSUInteger AntiBufferCounter = 0;
	RSUInteger RetBufferCounter = 0;
	if((!pszAntiBuffer)||(!&pszRetBuffer)||(!&uRetBufferLength))
		return uRet = kErrNil;
	if(!(*uRetBufferLength))
		return uRet = kErrNNil;
	uRealBufferLength = strlen(pszAntiBuffer);
	RetBufferCounter = uRealBufferLength;
	*uRetBufferLength = RetBufferCounter;
	RSZeroMemory(pszRetBuffer,RetBufferCounter+1);
	RetBufferCounter--;
    
	for(AntiBufferCounter = 0;AntiBufferCounter < uRealBufferLength;AntiBufferCounter ++,RetBufferCounter--)
		(pszRetBuffer)[RetBufferCounter] = pszAntiBuffer[AntiBufferCounter];
	(pszRetBuffer)[uRealBufferLength]='\0';
	return uRet;
}

RSExport __RSOldStringError	__RSOldStringFilterTargetInBuffer(RSBuffer filterBuf,
                                          RSBuffer target,
                                          RSBufferRef ppBuf)
{
	__RSOldStringError	iRet = kSuccess;
	do
	{
		RCMP(filterBuf,iRet);
		RCMP(target,iRet);
		RCPP(ppBuf,iRet);
		RSUInteger	len = strlen(filterBuf);
		RSUInteger	lenOfTarget = strlen(target);
		RSUInteger	offset =  0;
		RSBuffer	pBase = filterBuf;
		int	ret = YES;
		RSUInteger	numberOfTarget = 0;
		BWI(iRet = __RSOldStringGetCounter(filterBuf,target,&numberOfTarget));
		BWI(iRet = RSMmAlloc((RSZone)ppBuf,len + lenOfTarget*numberOfTarget + 1));
		if (numberOfTarget == 0)
		{
			strcpy(*ppBuf,filterBuf);
			return	iRet;
		}
		do
		{
			//strcpy(*ppBuf,pBase);
			//pBase = *ppBuf;
			RSUInteger	sumOffset = 0;
			ret = __RSOldStringGetIndex(pBase,target,&offset);
			if (!ret)
			{
				
				goto	SYMBOL_NOT_FOUND;
			}
			while(ret)
			{
				if (offset)
				{
					memcpy(*ppBuf + sumOffset,pBase,offset*sizeof(RSBlock));
					pBase += offset+lenOfTarget;
				}
				sumOffset += offset;
				ret = __RSOldStringGetIndex(pBase,target,&offset);
                //	pBase[offset] = '\0';
                //	strcat(*ppBuf,pBase);
				
				//strcat(*ppBuf,pBase);
				if (ret && !offset)
				{
					pBase += lenOfTarget;
				}
			}
        SYMBOL_FOUND:
            
			//memcpy(*ppBuf + sumOffset,pBase,len - sumOffset - 1);
			strcat(*ppBuf,pBase);
        SYMBOL_NOT_FOUND:
			;
		} while (0);
	} while (0);
	return	iRet;
}