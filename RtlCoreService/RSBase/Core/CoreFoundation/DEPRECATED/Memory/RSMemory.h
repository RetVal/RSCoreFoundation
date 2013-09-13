//
//  RSMemory.h
//  RSKit
//
//  Created by RetVal on 10/13/12.
//  Copyright (c) 2012 RetVal. All rights reserved.
//

#ifndef RSKit_RSMemory_h
#define RSKit_RSMemory_h

#import <RSKit/RSBase.h>
typedef RSErrorCode RSMmERR;
#ifndef __ETMSKitCopyMode
#define __ETMSKitCopyMode
enum
{
    RSMmKitCopyNormalMode = 0,
    RSMmKitCopyBySize     = 1,
    RSMmKitCopyByBlock    = 2
};
typedef RSUInteger RSMmKitCopyMode;
#endif


// allocate a block memory from system, should be freed by caller after use or memory leaks.
RSExport RSMmERR	RSMmAlloc(RSZone addPtr,RSUInteger size);

// re-allocate a block memory from system, should be freed by caller after use or memory leaks.
RSExport RSMmERR	RSMmReAlloc(RSZone addPtr,RSUInteger size);

// free the memory block that allocate by RSMmAlloc.
RSExport RSMmERR	RSMmFree(RSZone addPtr);

// allocate a new block of memory and copy the content of needCopy to the new zone, the copy size is copyZoneSize.
// the retZone should be freed by RSMmFree after use or memory leaks.
RSExport RSMmERR RSMmCopyZone(RSZone retZone, RSUInteger capacity, RSHandle needCopy, RSUInteger copyZoneSize);

// get the number of units
RSExport RSBGNumber RSMmGetUnitNumber();

// print the current number of units
RSExport void RSMmLogUnitNumber();

// allocate a 2d block memory from system, w is x, h is y, size is one element size.
// how to use the return block ? It's very easy to use, just use it as a 2d array, but this one is dynamically.
// should be freed by RSMmFree2d after use.
RSExport RSMmERR RSMmAlloc2d(RSZone matrix,RSUInteger w, RSUInteger h, RSUInteger size);

// free the memory block from RSMmAlloc2d.
RSExport RSMmERR	RSMmFree2d(RSZone addPtr);

// write the memory block(pAddress is the start address of memory block).
// the content is stored in CBWrite, the offset can not be nil, just give it a member and initialized with 0,
// it means the offset pointer of this memory block.
// writeLen is what length you want to write as this time.
// originalLen is realWrite size.
RSExport RSMmERR	RSMmWrite(RSBuffer* pAddress,RSBuffer CBWrite,RSUIntegerRef offset,size_t writeLen,RSUIntegerRef originalLen);

// same as IOReadFile, the handle is a block,
// CBRead is returned with content, readLen is what length you want to read.
// offset is the offset of the start read address in handle.
// CBRead should be freed by RSMmFree after use or memory leaks.
RSExport RSMmERR	RSMmRead(RSBuffer handle,RSBuffer* CBRead,RSUIntegerRef offset,size_t readLen);

// same as RSMmRead, the CBRead is provided by caller.
RSExport RSMmERR	RSMmFRead(RSBuffer handle,RSBuffer CBRead,RSUIntegerRef offset,size_t readLen);

// do not use these two functions anyway.
RSExport RSMmERR	RSMmAutoSwapPointer(RSZone swapPtrA,RSZone swapPtrB);
RSExport RSMmERR	RSMmAutoSwspAndFree(RSZone outPtr,RSZone freePtr);

// just do for IOKit, do not call if you are a third part developer.
RSExport RSBuffer RSMmAllocateIOUnit();

RSExport RSBitU64  RSMmZoneRealSize(RSBitU64 size);
#endif

