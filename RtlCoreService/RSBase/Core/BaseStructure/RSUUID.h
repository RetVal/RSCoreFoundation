//
//  RSUUID.h
//  RSCoreFoundation
//
//  Created by RetVal on 11/17/12.
//  Copyright (c) 2012 RetVal. All rights reserved.
//

#ifndef RSCoreFoundation_RSUUID_h
#define RSCoreFoundation_RSUUID_h
RS_EXTERN_C_BEGIN
typedef const struct __RSUUID* RSUUIDRef RS_AVAILABLE(0_0);

typedef struct {
    RSBitU8 byte[16];
} RSUUIDBytes RS_AVAILABLE(0_0);

RSExport RSTypeID RSUUIDGetTypeID() RS_AVAILABLE(0_0);
RSExport RSUUIDRef RSUUIDCreate(RSAllocatorRef allocator) RS_AVAILABLE(0_0);
RSExport RSUUIDRef RSUUIDCreateWithBytes(RSAllocatorRef allocator,
                                         RSBitU8 b1,  RSBitU8 b2,  RSBitU8 b3,  RSBitU8 b4,
                                         RSBitU8 b5,  RSBitU8 b6,  RSBitU8 b7,  RSBitU8 b8,
                                         RSBitU8 b9,  RSBitU8 b10, RSBitU8 b11, RSBitU8 b12,
                                         RSBitU8 b13, RSBitU8 b14, RSBitU8 b15, RSBitU8 b16) RS_AVAILABLE(0_0);
RSExport RSUUIDRef RSUUIDCreateWithString(RSAllocatorRef allocator, RSStringRef uuidString) RS_AVAILABLE(0_0);
RSExport RSUUIDRef RSUUIDCreateWithUUIDBytes(RSAllocatorRef allocator, RSUUIDBytes uuidBytes) RS_AVAILABLE(0_0);

RSExport RSStringRef RSUUIDCreateString(RSAllocatorRef allocator, RSUUIDRef uuid) RS_AVAILABLE(0_0);
RSExport RSUUIDBytes RSUUIDGetBytes(RSUUIDRef uuid) RS_AVAILABLE(0_0);

RSExport RSComparisonResult RSUUIDCompare(RSUUIDRef uuid1, RSUUIDRef uuid2) RS_AVAILABLE(0_0);

RS_EXTERN_C_END
#endif
