//
//  RSNumber.h
//  RSCoreFoundation
//
//  Created by RetVal on 10/16/12.
//  Copyright (c) 2012 RetVal. All rights reserved.
//

#ifndef RSCoreFoundation_RSNumber_h
#define RSCoreFoundation_RSNumber_h
#include <RSCoreFoundation/RSBase.h>
RS_EXTERN_C_BEGIN
enum {
    RSNumberNil        = 0,
    
    RSNumberSInt8      = 1,
    RSNumberSInt16     = 2,
    RSNumberSInt32     = 3,
    RSNumberSInt64     = 4,
    RSNumberFloat32    = 5,
    RSNumberFloat64    = 6,
    
    RSNumberBoolean    = RSNumberSInt8,
    RSNumberShort      = RSNumberSInt16,
    RSNumberInt        = RSNumberSInt32,
#if __LP64__
    RSNumberLong       = RSNumberSInt64,
#else
    RSNumberLong       = RSNumberSInt32,
#endif
    RSNumberLonglong   = 11,
    RSNumberFloat      = RSNumberFloat32,
    RSNumberDouble     = RSNumberFloat64,
    RSNumberUnsignedShort = RSNumberShort,
    RSNumberUnsignedInt = RSNumberInt,
    RSNumberUnsignedLong = RSNumberLong,
    RSNumberUnsignedLonglong = RSNumberLonglong,
#if __LP64__
    RSNumberRSIndex    = RSNumberLonglong,
    RSNumberInteger    = RSNumberLong,
    RSNumberUnsignedInteger = RSNumberUnsignedLong,
    RSNumberRSFloat    = RSNumberDouble,
#else
    RSNumberRSIndex    = RSNumberLong,
    RSNumberInteger    = RSNumberInt,
    RSNumberUnsignedInteger = RSNumberUnsignedInt,
    RSNumberRSFloat    = RSNumberFloat,
#endif
} RS_AVAILABLE(0_0);
typedef RSTypeID RSNumberType RS_AVAILABLE(0_0);

typedef const struct __RSNumber* RSNumberRef RS_AVAILABLE(0_0);

RSExport const RSNumberRef RSBooleanTrue;
RSExport const RSNumberRef RSBooleanFalse;
/*! @function RSNumberGetTypeID
 @abstract Return the RSTypeID type id.
 @discussion This function return the type id of RSNumberRef from the runtime.
 @result A RSTypeID the type id of RSNumberRef.
 */
RSExport RSTypeID RSNumberGetTypeID() RS_AVAILABLE(0_0);

/*! @function RSNumberCreate
 @abstract Return the RSNumberRef.
 @discussion This function return the type id of RSNumberRef from the runtime.
 @result A RSTypeID the type id of RSNumberRef.
 */
RSExport RSNumberRef RSNumberCreate(RSAllocatorRef allocator, RSNumberType theType, void* value) RS_AVAILABLE(0_0);
// RSNumberCreate create the RSNumber object with a constant number
RSExport RSNumberRef RSNumberCreateInt(RSAllocatorRef allocator, int value) RS_AVAILABLE(0_3);
RSExport RSNumberRef RSNumberCreateUnsignedInt(RSAllocatorRef allocator, unsigned int value) RS_AVAILABLE(0_3);
RSExport RSNumberRef RSNumberCreateShort(RSAllocatorRef allocator, short value) RS_AVAILABLE(0_3);
RSExport RSNumberRef RSNumberCreateUnsignedShort(RSAllocatorRef allocator, unsigned short value) RS_AVAILABLE(0_3);
RSExport RSNumberRef RSNumberCreateInteger(RSAllocatorRef allocator, RSInteger value) RS_AVAILABLE(0_0);
RSExport RSNumberRef RSNumberCreateUnsignedInteger(RSAllocatorRef allocator, RSUInteger value) RS_AVAILABLE(0_0);
RSExport RSNumberRef RSNumberCreateBoolean(RSAllocatorRef allocator, BOOL value) RS_AVAILABLE(0_0);
RSExport RSNumberRef RSNumberCreateLong(RSAllocatorRef allocator, long value) RS_AVAILABLE(0_0);
RSExport RSNumberRef RSNumberCreateLonglong(RSAllocatorRef allocator, long long value) RS_AVAILABLE(0_0);
RSExport RSNumberRef RSNumberCreateUnsignedLong(RSAllocatorRef allocator, unsigned long value) RS_AVAILABLE(0_0);
RSExport RSNumberRef RSNumberCreateUnsignedLonglong(RSAllocatorRef allocator, unsigned long long value) RS_AVAILABLE(0_0);
RSExport RSNumberRef RSNumberCreateFloat(RSAllocatorRef allocator, float value) RS_AVAILABLE(0_0);
RSExport RSNumberRef RSNumberCreateDouble(RSAllocatorRef allocator, double value) RS_AVAILABLE(0_0);
RSExport RSNumberRef RSNumberCreateRSFloat(RSAllocatorRef allocator, RSFloat value) RS_AVAILABLE(0_3);

RSExport RSNumberType RSNumberGetType(RSNumberRef aNumber) RS_AVAILABLE(0_0);
RSExport void RSNumberSetValue(RSNumberRef aNumber, RSNumberType numberType, void* value) RS_AVAILABLE(0_0);
RSExport BOOL RSNumberGetValue(RSNumberRef aNumber, void* value) RS_AVAILABLE(0_0);
RSExport RSComparisonResult RSNumberCompare(RSNumberRef aNumber, RSNumberRef other, void *context) RS_AVAILABLE(0_0);

RSExport BOOL RSNumberIsFloatType(RSNumberRef aNumber) RS_AVAILABLE(0_0);
RSExport BOOL RSNumberIsBooleanType(RSNumberRef aNumber) RS_AVAILABLE(0_0);

RS_EXTERN_C_END
#endif
