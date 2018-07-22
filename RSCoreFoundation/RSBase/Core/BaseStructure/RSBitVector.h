//
//  RSBitVector.h
//  RSCoreFoundation
//
//  Created by RetVal on 6/30/13.
//  Copyright (c) 2013 RetVal. All rights reserved.
//

#ifndef RSCoreFoundation_RSBitVector
#define RSCoreFoundation_RSBitVector

RS_EXTERN_C_BEGIN

typedef const struct __RSBitVector *RSBitVectorRef;
typedef struct __RSBitVector *RSMutableBitVectorRef;
typedef RSBitU8 RSByte;
typedef RSBitU8 RSBit;

RSExport RSTypeID RSBitVectorGetTypeID(void);
RSExport RSBitVectorRef RSBitVectorCreate(RSAllocatorRef allocator, RSByte *bytes, RSIndex countOfBytes);
RSExport RSMutableBitVectorRef RSBitVectorCreateMutable(RSAllocatorRef allocator, RSIndex capacity);
RSExport RSBitVectorRef RSBitVectorCreateCopy(RSAllocatorRef allocator, RSBitVectorRef bv);
RSExport RSMutableBitVectorRef RSBitVectorCreateMutableCopy(RSAllocatorRef allocator, RSBitVectorRef bv);

RSExport RSIndex RSBitVectorGetCount(RSBitVectorRef bv);
RSExport RSIndex RSBitVectorGetCountOfBit(RSBitVectorRef bv, RSRange range, RSBit value);
RSExport BOOL RSBitVectorContainsBit(RSBitVectorRef bv, RSRange range, RSBit value);
RSExport RSBit RSBitVectorGetBitAtIndex(RSBitVectorRef bv, RSIndex idx);
RSExport void RSBitVectorGetBits(RSBitVectorRef bv, RSRange range, RSByte *bytes);

RSExport void		RSBitVectorSetCount(RSMutableBitVectorRef bv, RSIndex count);
RSExport void		RSBitVectorFlipBitAtIndex(RSMutableBitVectorRef bv, RSIndex idx);
RSExport void		RSBitVectorFlipBits(RSMutableBitVectorRef bv, RSRange range);
RSExport void		RSBitVectorSetBitAtIndex(RSMutableBitVectorRef bv, RSIndex idx, RSBit value);
RSExport void		RSBitVectorSetBits(RSMutableBitVectorRef bv, RSRange range, RSBit value);
RSExport void		RSBitVectorSetAllBits(RSMutableBitVectorRef bv, RSBit value);

RS_EXTERN_C_END
#endif 
