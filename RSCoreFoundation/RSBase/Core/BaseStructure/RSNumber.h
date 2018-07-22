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
    RSNumberChar       = 5,
    RSNumberFloat32    = 6,
    
    
    RSNumberFloat64    = 7,
    RSNumberRSRange    = 8,
    RSNumberPointer    = 9,
    RSNumberString     = 10,
    
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
    RSNumberUnsignedChar = RSNumberChar,
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
RSExport RSTypeID RSNumberGetTypeID(void) RS_AVAILABLE(0_0);

/*! @function RSNumberCreate
 @abstract Return the RSNumberRef.
 @discussion This function return the type id of RSNumberRef from the runtime.
 @result A RSTypeID the type id of RSNumberRef.
 */
// RSNumberCreate create the RSNumber object with a constant number
RSExport RSNumberRef RSNumberCreate(RSAllocatorRef allocator, RSNumberType theType, void* value) RS_AVAILABLE(0_0);

RSExport RSNumberRef RSNumberCreateChar(RSAllocatorRef allocator, char value) RS_AVAILABLE(0_4);
RSExport RSNumberRef RSNumberCreateUnsignedChar(RSAllocatorRef allocator, unsigned char value) RS_AVAILABLE(0_4);
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
RSExport RSNumberRef RSNumberCreatePointer(RSAllocatorRef allocator, void *value) RS_AVAILABLE(0_4);
RSExport RSNumberRef RSNumberCreateRange(RSAllocatorRef allocator, RSRange value) RS_AVAILABLE(0_4);
RSExport RSNumberRef RSNumberCreateString(RSAllocatorRef allocator, RSStringRef value) RS_AVAILABLE(0_4);

RSExport char RSNumberCharValue(RSNumberRef aValue) RS_AVAILABLE(0_4);
RSExport unsigned char RSNumberUnsignedCharValue(RSNumberRef aValue) RS_AVAILABLE(0_4);
RSExport short RSNumberShortValue(RSNumberRef aValue) RS_AVAILABLE(0_4);
RSExport unsigned short RSNumberUnsignedShortValue(RSNumberRef aValue) RS_AVAILABLE(0_4);
RSExport int RSNumberIntValue(RSNumberRef aValue) RS_AVAILABLE(0_4);
RSExport unsigned int RSNumberUnsignedIntValue(RSNumberRef aValue) RS_AVAILABLE(0_4);
RSExport long RSNumberLongValue(RSNumberRef aValue) RS_AVAILABLE(0_4);
RSExport unsigned long RSNumberUnsignedLongValue(RSNumberRef aValue) RS_AVAILABLE(0_4);
RSExport long long RSNumberLongLongValue(RSNumberRef aValue) RS_AVAILABLE(0_4);
RSExport unsigned long long RSNumberUnsignedLongLongValue(RSNumberRef aValue) RS_AVAILABLE(0_4);
RSExport float RSNumberFloatValue(RSNumberRef aValue) RS_AVAILABLE(0_4);
RSExport double RSNumberDoubleValue(RSNumberRef aValue) RS_AVAILABLE(0_4);
RSExport BOOL RSNumberBooleanValue(RSNumberRef aValue) RS_AVAILABLE(0_4);
RSExport RSFloat RSNumberRSFloatValue(RSNumberRef aValue) RS_AVAILABLE(0_4);
RSExport RSInteger RSNumberIntegerValue(RSNumberRef aValue) RS_AVAILABLE(0_4);
RSExport RSUInteger RSNumberUnsignedIntegerValue(RSNumberRef aValue) RS_AVAILABLE(0_4);
RSExport void * RSNumberPointerValue(RSNumberRef aValue) RS_AVAILABLE(0_4);
RSExport RSRange RSNumberRangeValue(RSNumberRef aValue) RS_AVAILABLE(0_4);
RSExport RSStringRef RSNumberStringValue(RSNumberRef aValue) RS_AVAILABLE(0_4);


RSExport RSNumberType RSNumberGetType(RSNumberRef aNumber) RS_AVAILABLE(0_0);
RSExport void RSNumberSetValue(RSNumberRef aNumber, RSNumberType numberType, void* value) RS_AVAILABLE(0_0);
RSExport BOOL RSNumberGetValue(RSNumberRef aNumber, void* value) RS_AVAILABLE(0_0);
RSExport RSComparisonResult RSNumberCompare(RSNumberRef aNumber, RSNumberRef other, void *context) RS_AVAILABLE(0_0);

RSExport BOOL RSNumberIsFloatType(RSNumberRef aNumber) RS_AVAILABLE(0_0);
RSExport BOOL RSNumberIsIntegerType(RSNumberRef aNumber) RS_AVAILABLE(0_0);
RSExport BOOL RSNumberIsBooleanType(RSNumberRef aNumber) RS_AVAILABLE(0_0);

RSExport RSBitU64 RSNumberGetUnFloatingValue(RSNumberRef aNum);
RSExport double RSNumberGetFloatingValue(RSNumberRef aNum);
RS_EXTERN_C_END
#endif
