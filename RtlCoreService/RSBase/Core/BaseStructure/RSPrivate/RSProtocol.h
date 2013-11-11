//
//  RSProtocol.h
//  RSCoreFoundation
//
//  Created by RetVal on 6/27/13.
//  Copyright (c) 2013 RetVal. All rights reserved.
//

#ifndef RSCoreFoundation_RSProtocol
#define RSCoreFoundation_RSProtocol

RS_EXTERN_C_BEGIN

typedef struct __RSProtocol *RSProtocolRef;

typedef const char* PCE;

struct _RSProtocolItem {
    PCE sel;
    ISA imp;
};
typedef struct _RSProtocolItem RSProtocolItem;

struct _RSProtocolContext {
    const char * delegateName;
    RSUInteger countOfItems;
    RSProtocolItem items[1];
};
typedef struct _RSProtocolContext RSProtocolContext;
RSExport RSTypeID RSProtocolGetTypeID() RS_AVAILABLE(0_4);

RSExport RSProtocolRef RSProtocolCreateWithContext(RSAllocatorRef allocator, RSProtocolContext *context, RSArrayRef supers) RS_AVAILABLE(0_4);

RSExport RSProtocolRef RSProtocolCreateWithName(RSAllocatorRef allocator, RSStringRef name) RS_AVAILABLE(0_4);

RSExport RSStringRef RSProtocolGetName(RSProtocolRef protocol) RS_AVAILABLE(0_4);

RSExport RSArrayRef RSProtocolGetSupers(RSProtocolRef protocol) RS_AVAILABLE(0_4);

RSExport BOOL RSProtocolIsRoot(RSProtocolRef protocol) RS_AVAILABLE(0_4);

RSExport ISA RSProtocolGetImplementationWithPCE(RSProtocolRef protocol, PCE pce) RS_AVAILABLE(0_4);

RSExport BOOL RSProtocolConfirmImplementation(RSProtocolRef protocol, PCE pce) RS_AVAILABLE(0_4);

RS_EXTERN_C_END
#endif 
