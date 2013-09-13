//
//  RSMemory.c
//  RSCoreService
//
//  Created by RetVal on 10/13/12.
//  Copyright (c) 2012 RetVal. All rights reserved.
//

#include <RSKit/RSMemory.h>
#include <RSKit/RSMathNumber.h>
#if DEPLOYMENT_TARGET_MACOSX
#include <malloc/malloc.h>
#endif
RSPrivate RSBGNumber __MSCount = 0;
RSPrivate RSSpinLock __MSLock = RSSpinLockInit;
#define __kMSSmallSize  512

RSExport RSBuffer RSMmAllocateIOUnit()
{
    RSHandle ret = nil;
    RSMmAlloc(&ret, kRSBufferSize);
    return ret;
}

RSExport RSBitU64  RSMmZoneRealSize(RSBitU64 size)
{
#if __RSRuntimeAllocatorResizeAutomatically
    #if DEPLOYMENT_TARGET_WINDOWS || DEPLOYMENT_TARGET_LINUX
        RSBitU64 retSize = 0;
        if (size <= __kMSSmallSize)
        {
    #if __LP64__
            retSize = RSMathNumberTo32(size);   // 64 bits
    #else
            retSize = RSMathNumberTo16(size);   // 32 bits
    #endif
        }
        else if (size > __kMSSmallSize)
        {
            retSize = RSMathNumberTo512(size);
        }
        return retSize;
    #elif DEPLOYMENT_TARGET_MACOSX
        size = malloc_good_size(size);
    #endif
#endif
    return size;
}

RSExport RSMmERR RSMmAlloc(RSZone addPtr,RSUInteger size)
{
	RSMmERR	iRet = kSuccess;
	do
	{
		RCMP((addPtr),iRet);
		RCM((!*addPtr),iRet,kSuccess,kErrNNil);//should be nil but not.
		RCM((size > 0),iRet,kSuccess,kErrVerify);
#if __RSRuntimeAllocatorResizeAutomatically
        RSBitU64 _realSize = RSMmZoneRealSize(size);
#else
        RSBitU64 _realSize = size;
#endif
        RCO(_realSize, iRet);
#if DEPLOYMENT_TARGET_MACOSX
		if ((*addPtr = malloc_zone_calloc(malloc_default_zone(), 1, size)))
#elif DEPOLOYMENT_TARGET_WINDOWS || DEPOLOYMENT_TARGET_LINUX
        if ((*addPtr = calloc(_realSize)))
#endif
        {
#if __RSRuntimeInstanceAllocFreeWatcher
            __RSCLog(kRSLogLevelDebug, "RSAllocator alloc - <%p>\n", *addPtr);
#endif
			//RSZeroMemory(*addPtr, size);
            OSSpinLockLock(&__MSLock);
            __MSCount++;
            OSSpinLockUnlock(&__MSLock);
		}
		else
		{
			iRet = kErrMmRes;	//no more memory resource.
		}
	} while (0);
	return	iRet;
}

RSExport RSMmERR	RSMmReAlloc(RSZone addPtr,RSUInteger size)
{
    RSMmERR iRet = kSuccess;
    do {
        RCMP(addPtr, iRet);
        if (*addPtr)
        {
#if __RSRuntimeInstanceAllocFreeWatcher
            __RSCLog(kRSLogLevelDebug, "RSAllocator realloc 1 - <%p>\n", *addPtr);
#endif
#if DEPLOYMENT_TARGET_MACOSX
            *addPtr = malloc_zone_realloc(malloc_default_zone(), *addPtr, size);
#elif DEPOLOYMENT_TARGET_WINDOWS || DEPOLOYMENT_TARGET_LINUX
            *addPtr = realloc(*addPtr, size);
#endif
#if __RSRuntimeInstanceAllocFreeWatcher
            __RSCLog(kRSLogLevelDebug, "RSAllocator realloc 2 - <%p>\n", *addPtr);
#endif
            //RSZeroMemory(*addPtr, size);
            //[__MSCountLock lock];
            OSSpinLockLock(&__MSLock);
            //__MSCount++;
            OSSpinLockUnlock(&__MSLock);
            //[__MSCountLock unlock];
        }
    } while (0);
    return iRet;
}

RSExport RSMmERR RSMmFree(RSZone addPtr)
{
	RSMmERR	iRet  = kSuccess;
	do
	{
		RCMP(addPtr,iRet);
		RCMP(*addPtr,iRet);
#if __RSRuntimeInstanceAllocFreeWatcher
        __RSCLog(kRSLogLevelDebug, "RSAllocator free - <%p>\n", *addPtr);
#endif
#if DEPLOYMENT_TARGET_MACOSX
        malloc_zone_free(malloc_default_zone(), *addPtr);
#elif DEPOLOYMENT_TARGET_WINDOWS || DEPOLOYMENT_TARGET_LINUX
        free(*addPtr);
#endif
		
        //[__MSCountLock lock];
        OSSpinLockLock(&__MSLock);
        __MSCount--;
        OSSpinLockUnlock(&__MSLock);
        //[__MSCountLock unlock];
		*addPtr = nil;
	} while (0);
	return	iRet;
}

RSExport RSMmERR RSMmCopyZone(RSZone retZone, RSUInteger capacity, RSHandle needCopy, RSUInteger copyZoneSize)
{
    RSMmERR   iRet = kSuccess;
    do {
        RCPP(retZone, iRet);
        RCMP(needCopy, iRet);
        if (capacity < copyZoneSize) BWI(iRet = kErrMmSmall);
        BWI(iRet = RSMmAlloc(retZone, capacity));
        memcpy(*retZone, needCopy, copyZoneSize);
    } while (0);
    return  iRet;
}

RSExport RSBGNumber RSMmGetUnitNumber()
{
    return __MSCount;
}

RSExport void RSMmLogUnitNumber()
{
    __RSCLog(kRSLogLevelDebug, "RSKit(RSMemory) - alloc unit : %lld\n",RSMmGetUnitNumber());
}

RSZone malloc2d(RSUInteger width, RSUInteger height, size_t size, int align)
{
    // alloc all spaces: index area, largest padding area and data area
    RSUInteger index_size = sizeof(void *) * height;
    RSUInteger row_size = size * width;
    RSZone p = (RSZone) malloc(index_size + align + height * row_size);
    // compute padding
    RSBuffer ptr_index_ending = (RSBuffer) p + index_size;
    RSUInteger padding = (align - ((RSUInteger) ptr_index_ending + 1) % align) % align;
    // link index area and data area
    // starting address of data area
    RSBuffer ptr_data = ptr_index_ending + 1 + padding;
    for (RSUInteger i = 0; i < height; i++)
    {
        p[i] = ptr_data;
        ptr_data += size * width;
    }
    return p;
}

RSInside RSZone _MSAlloc2d(RSUInteger row, RSUInteger col, RSUInteger size)
{
	RSZone arr;
	arr = (RSZone) malloc(sizeof(RSHandle) * row + size * row * col);
    //[__MSCountLock lock];
        //[__MSCountLock unlock];
	if (arr != NULL)
	{
        OSSpinLockLock(&__MSLock);
        __MSCount++;
        OSSpinLockUnlock(&__MSLock);

		RSHandle head;
		head = (RSHandle) arr + sizeof(RSHandle) * row;
		memset(arr, 0, sizeof(RSHandle) * row + size * row * col);
		while (row--)
			arr[row] = head + size * row * col;
	}
	return arr;
}
RSExport RSMmERR RSMmAlloc2d(RSZone matrix,RSUInteger w, RSUInteger h, RSUInteger size)
{
    RSMmERR iRet = kSuccess;
    do {
        RCMP((matrix),iRet);
		RCM((!*matrix),iRet,kSuccess,kErrNNil);//should be nil but not.
		RCM((size > 0),iRet,kSuccess,kErrVerify);
        RSZone a = NULL;
        a = _MSAlloc2d(w, h, size);
        *matrix = (RSHandle)a;
    } while (0);
    
    return iRet;
}
RSExport RSMmERR	RSMmFree2d(RSZone addPtr)
{
    return RSMmFree(addPtr);
}
RSExport RSMmERR RSMmWrite(RSBuffer* pAddress,RSBuffer CBWrite,RSUIntegerRef offset,size_t writeLen, RSUIntegerRef originalLen)
{
	RSMmERR iRet = kSuccess;
	RSBuffer CBExg = nil;
	RSBuffer CBTmp = nil;
	do
	{
		if (nil == pAddress || nil == *pAddress
			|| nil == CBWrite || 0 == writeLen
			|| nil == originalLen || 0 == *originalLen)
		{
			BWI(iRet = kErrNil);
		}
		if ((*offset + writeLen) > *originalLen)
		{
			BWI(iRet = RSMmAlloc((RSZone)&CBExg,*offset + writeLen + *originalLen));
			memcpy(CBExg,*pAddress,*originalLen);
			*originalLen += *offset + writeLen;
			CBTmp = *pAddress;
			*pAddress = CBExg;
			CBExg = CBTmp;
		}
		memcpy((*pAddress)+*offset,CBWrite,writeLen);
		*offset += writeLen;
	} while (0);
	RSMmFree((RSZone)&CBExg);
	return iRet;
}
RSExport RSMmERR RSMmFRead(RSBuffer handle,RSBuffer CBRead,RSUInteger* offset,size_t readLen)
{
	RSUInteger iRet = kSuccess;
	do
	{
		RCMP(handle,iRet);
		RCMP(CBRead,iRet);
		memcpy(CBRead,handle + *offset,readLen);
		*offset += readLen;
	} while (0);
	return iRet;
}
RSExport RSMmERR RSMmRead(RSBuffer handle,RSBuffer* CBRead,RSUInteger * offset,size_t readLen)
{
	RSUInteger iRet = kSuccess;
	do
	{
		RCMP(handle,iRet);
		RCMP(CBRead,iRet);
		RCPP(CBRead,iRet);
		BWI(iRet = RSMmAlloc((RSZone)CBRead,readLen + 1));
		memcpy(*CBRead,handle + *offset,readLen);
		*offset += readLen;
	} while (0);
	return iRet;
}
RSExport RSMmERR RSMmAutoSwapPointer(RSZone swapPtrA,RSZone swapPtrB)
{
	RSMmERR	iRet = kSuccess;
	do
	{
		RCMP(swapPtrA,iRet);
		RCMP(swapPtrB,iRet);
		RSBuffer	ptr = nil;
		ptr = (RSBuffer)*swapPtrB;
		*swapPtrB = *swapPtrA;
		*(RSBuffer*)(swapPtrA) = ptr;
		ptr = nil;
	} while (0);
	
	return  iRet;
}

RSExport RSMmERR RSMmAutoSwspAndFree(RSZone outPtr,RSZone freePtr)
{
	RSMmERR iRet = kSuccess;
	do
	{
		BWI(iRet = RSMmAutoSwapPointer(outPtr,freePtr));
		RSMmFree(freePtr);
	} while (0);
	return	iRet;
}

