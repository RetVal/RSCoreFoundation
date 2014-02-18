//
//  RSNumber.c
//  RSCoreFoundation
//
//  Created by RetVal on 10/16/12.
//  Copyright (c) 2012 RetVal. All rights reserved.
//

#include <stdio.h>
#include <RSCoreFoundation/RSBase.h>
#include <RSCoreFoundation/RSNil.h>
#include <RSCoreFoundation/RSNumber.h>
#include <RSCoreFoundation/RSRuntime.h>

#define RS_IS_TAGGED_INT(OBJ)	(RS_TAGGED_OBJ_TYPE(OBJ) == RSTaggedObjectID_Integer)

RSInline void __RSNumberTaggedSetType(RSNumberRef number, RSNumberType type)
{
    intptr_t value = (intptr_t)number;
    int flag = 0;
    switch (type)
    {
        case RSNumberSInt8:
            flag = 0;
            break;
        case RSNumberSInt16:
            flag = 1;
            break;
        case RSNumberSInt32:
            flag = 2;
            break;
        case RSNumberSInt64:
            flag = 3;
            break;
        default:
            break;
    }
    value >>= 8;
    value = value << 8 | (flag << 6) | RSTaggedObjectID_Integer;
}

struct __RSBaseNumber {
    unsigned int _type;
    union {
        BOOL _bool;
        int8_t _int8;
        int16_t _int16;
        int32_t _int32;
#if __LP64__
        int64_t _int64;
#endif
        uint8_t _uint8;
        uint16_t _uint16;
        uint32_t _uint32;
#if __LP64__
        uint64_t _uint64;
#endif
        RSFloat _rsfloat;
        short _short;
        unsigned short _ushort;
        int _int;
        unsigned int _uint;
        long _long;
        unsigned long _ulong;
        
        float _float;
        double _double;
        
        long long _longlong;
        unsigned long long _ulonglong;
        
        char _char;
        unsigned char _unsignedChar;
        RSRange _range;
        RSStringRef _string;
        void *_pointer;
    }_pay;
};
struct __RSNumber {
    RSRuntimeBase _base;
    struct __RSBaseNumber _number;
};
static  struct __RSNumber __RSBooleanTrue =
{
    {0}
//#if __LP64__
//    {0,3,1,0,0}
//#else
//    {1,0,3,1,0,0}
//#endif
    ,YES
};
static  struct __RSNumber __RSBooleanFalse =
{
    {0}
//#if __LP64__
//    {0,3,1,0,0}
//#else
//    {1,0,3,1,0,0}
//#endif
    ,NO
};

enum {
    _RSNumberBoolean   = 1 << 0,
    _RSNumberShort     = 1 << 1,
    _RSNumberInt       = 1 << 2,
    _RSNumberLong      = 1 << 3,
    _RSNumberFloat     = 1 << 4,
    _RSNumberDouble    = 1 << 5,
    _RSNumberLonglong  = 1 << 6,
    _RSNumberChar      = 1 << 7,
    _RSNumberRSRange   = 1 << 8,
    _RSNumberPointer   = 1 << 9,
    _RSNumberString    = 1 << 10
};

enum {
    _RSNumberIsUnsigned = 1 << 14,
    _RSNumberIsFloat    = 1 << 15,
};

enum {
    _RSNumberBooleanMask    = 1 << 0,
    _RSNumberShortMask      = 1 << 1,
    _RSNumberIntMask        = 1 << 2,
    _RSNumberLongMask       = 1 << 3,
    _RSNumberFloatMask      = 1 << 4,
    _RSNumberDoubleMask     = 1 << 5,
    _RSNumberLonglongMask   = 1 << 6,
    _RSNumberCharMask       = 1 << 7,
    _RSNumberRSRangeMask    = 1 << 8,
    _RSNumberPointerMask    = 1 << 9,
    _RSNumberStringMask     = 1 << 10,
    
    _RSNumberIsUnsignedMask = 1 << 14,
    _RSNumberIsFloatMask = 1 << 15
};

/* unmark
 XXXX X1XX (long)
~1111 1011 (mask)
&_______________
 XXXX X0XX
 */
/* mark
 XXXX X0XX (long)
 0000 0100 (mask)
|_______________
 XXXX X1XX
 */
RSInline BOOL   __RSNumberISFloatType(RSNumberRef number) {
    if (RS_IS_TAGGED_INT(number)) return NO;
    return ((number->_number._type & _RSNumberIsFloat) == _RSNumberIsFloatMask);
}
RSInline BOOL   __RSNumberISUnsignedType(RSNumberRef number) {
    return ((number->_number._type & _RSNumberIsUnsigned) == _RSNumberIsUnsignedMask);
}

RSInline BOOL   __RSNumberIsChar(RSNumberRef number) {
    if (RS_IS_TAGGED_INT(number))
        return RSNumberGetType(number) == RSNumberChar;
    return ((((struct __RSNumber *)number)->_number._type & _RSNumberChar) == _RSNumberCharMask);
}

RSInline void   __RSNumberMarkChar(RSNumberRef number) {
    if (RS_IS_TAGGED_INT(number))
        return __RSNumberTaggedSetType(number, RSNumberChar);
    ((((struct __RSNumber *)number)->_number._type |= _RSNumberCharMask));
}

RSInline void   __RSNumberUnMarkChar(RSNumberRef number) {
    if (RS_IS_TAGGED_INT(number))
        return __RSNumberTaggedSetType(number, RSNumberChar);
    ((((struct __RSNumber *)number)->_number._type &= ~_RSNumberCharMask));
}

RSInline BOOL   __RSNumberIsRange(RSNumberRef number) {
    if (RS_IS_TAGGED_INT(number))
        return NO;
    return ((((struct __RSNumber *)number)->_number._type & _RSNumberRSRange) == _RSNumberRSRangeMask);
}

RSInline void   __RSNumberMarkRange(RSNumberRef number) {
    if (RS_IS_TAGGED_INT(number))
        return __RSNumberTaggedSetType(number, RSNumberRSRange);
    ((((struct __RSNumber *)number)->_number._type |= _RSNumberRSRangeMask));
}

RSInline void   __RSNumberUnMarkRange(RSNumberRef number) {
    if (RS_IS_TAGGED_INT(number))
        return __RSNumberTaggedSetType(number, RSNumberNil);
    ((((struct __RSNumber *)number)->_number._type &= ~_RSNumberRSRangeMask));
}

RSInline BOOL   __RSNumberIsPointer(RSNumberRef number) {
    if (RS_IS_TAGGED_INT(number))
        return RSNumberGetType(number) == RSNumberPointer;
    return ((((struct __RSNumber *)number)->_number._type & _RSNumberPointer) == _RSNumberPointerMask);
}

RSInline void   __RSNumberMarkPointer(RSNumberRef number) {
    if (RS_IS_TAGGED_INT(number))
        return __RSNumberTaggedSetType(number, RSNumberPointer);
    ((((struct __RSNumber *)number)->_number._type |= _RSNumberPointerMask));
}

RSInline void   __RSNumberUnMarkPointer(RSNumberRef number) {
    if (RS_IS_TAGGED_INT(number))
        return __RSNumberTaggedSetType(number, RSNumberNil);
    ((((struct __RSNumber *)number)->_number._type &= ~_RSNumberPointerMask));
}

RSInline BOOL   __RSNumberIsString(RSNumberRef number) {
    if (RS_IS_TAGGED_INT(number))
        return RSNumberGetType(number) == RSNumberString;
    return ((((struct __RSNumber *)number)->_number._type & _RSNumberString) == _RSNumberStringMask);
}

RSInline void   __RSNumberMarkString(RSNumberRef number) {
    if (RS_IS_TAGGED_INT(number))
        return __RSNumberTaggedSetType(number, RSNumberString);
    ((((struct __RSNumber *)number)->_number._type |= _RSNumberStringMask));
}

RSInline void   __RSNumberUnMarkString(RSNumberRef number) {
    if (RS_IS_TAGGED_INT(number))
        return __RSNumberTaggedSetType(number, RSNumberNil);
    ((((struct __RSNumber *)number)->_number._type &= ~_RSNumberStringMask));
}

RSInline BOOL   __RSNumberISBoolean(RSNumberRef number) {
    if (RS_IS_TAGGED_INT(number))
    {
        return RSNumberGetType(number) == RSNumberSInt8;
    }
    return ((number->_number._type & _RSNumberBoolean) == _RSNumberBooleanMask);
}
RSInline void   __RSNumberMarkBoolean (RSNumberRef number) {
    if (RS_IS_TAGGED_INT(number))
    {
        __RSNumberTaggedSetType(number, RSNumberSInt8);
        return;
    }
    ((((struct __RSNumber*)number)->_number._type |= _RSNumberBooleanMask));
}
RSInline void   __RSNumberUnMarkBoolean(RSNumberRef number) {
    if (RS_IS_TAGGED_INT(number))
    {
        __RSNumberTaggedSetType(number, RSNumberSInt8);
        return;
    }
    ((((struct __RSNumber*)number)->_number._type &= ~_RSNumberBooleanMask));
}

RSInline BOOL   __RSNumberISInt(RSNumberRef number) {
    if (RS_IS_TAGGED_INT(number))
    {
        return RSNumberGetType(number) == RSNumberSInt32;
    }

    return  (!__RSNumberISFloatType(number)) &&
//            (!__RSNumberISUnsignedType(number)) &&
            ((number->_number._type & _RSNumberInt)    == _RSNumberIntMask);
}
RSInline void   __RSNumberMarkInt(RSNumberRef number) {
    if (RS_IS_TAGGED_INT(number))
    {
        __RSNumberTaggedSetType(number, RSNumberSInt32);
        return;
    }
    ((((struct __RSNumber*)number)->_number._type &= ~_RSNumberIsFloatMask));
//    ((((struct __RSNumber*)number)->_number._type &= ~_RSNumberIsUnsignedMask));
    ((((struct __RSNumber*)number)->_number._type |= _RSNumberIntMask));
}
RSInline void   __RSNumberUnMarkInt(RSNumberRef number) {
    ((((struct __RSNumber*)number)->_number._type &= ~_RSNumberIsFloatMask));
//    ((((struct __RSNumber*)number)->_number._type &= ~_RSNumberIsUnsignedMask));
    ((((struct __RSNumber*)number)->_number._type &= ~_RSNumberIntMask));
}
RSInline BOOL   __RSNumberISUnsignedInt(RSNumberRef number) {
    return __RSNumberISInt(number);
    return  (!__RSNumberISFloatType(number)) &&
//            (__RSNumberISUnsignedType(number)) &&
            ((number->_number._type & _RSNumberInt)    == _RSNumberIntMask);
}
RSInline void   __RSNumberMarkUnsignedInt(RSNumberRef number) {
    return __RSNumberMarkInt(number);
    ((((struct __RSNumber*)number)->_number._type &= ~_RSNumberIsFloatMask));
//    ((((struct __RSNumber*)number)->_number._type |= _RSNumberIsUnsignedMask));
    ((((struct __RSNumber*)number)->_number._type |= _RSNumberIntMask));
}
RSInline void   __RSNumberUnMarkUnsignedInt(RSNumberRef number) {
    return __RSNumberUnMarkInt(number);
    ((((struct __RSNumber*)number)->_number._type &= ~_RSNumberIsFloatMask));
//    ((((struct __RSNumber*)number)->_number._type &= ~_RSNumberIsUnsignedMask));
    ((((struct __RSNumber*)number)->_number._type &= ~_RSNumberIntMask));
}

RSInline BOOL   __RSNumberISShort(RSNumberRef number) {
    if (RS_IS_TAGGED_INT(number))
    {
        return RSNumberGetType(number) == RSNumberSInt16;
    }
    return  (!__RSNumberISFloatType(number)) &&
//    (!__RSNumberISUnsignedType(number)) &&
    ((number->_number._type & _RSNumberShort)    == _RSNumberShortMask);
}
RSInline void   __RSNumberMarkShort(RSNumberRef number) {
    if (RS_IS_TAGGED_INT(number))
    {
        __RSNumberTaggedSetType(number, RSNumberSInt16);
        return;
    }
    ((((struct __RSNumber*)number)->_number._type &= ~_RSNumberIsFloatMask));
//    ((((struct __RSNumber*)number)->_number._type &= ~_RSNumberIsUnsignedMask));
    ((((struct __RSNumber*)number)->_number._type |= _RSNumberShortMask));
}
RSInline void   __RSNumberUnMarkShort(RSNumberRef number) {
    ((((struct __RSNumber*)number)->_number._type &= ~_RSNumberIsFloatMask));
//    ((((struct __RSNumber*)number)->_number._type &= ~_RSNumberIsUnsignedMask));
    ((((struct __RSNumber*)number)->_number._type &= ~_RSNumberShortMask));
}
RSInline BOOL   __RSNumberISUnsignedShort(RSNumberRef number) {
    return __RSNumberISShort(number);
    return  (!__RSNumberISFloatType(number)) &&
//    (__RSNumberISUnsignedType(number)) &&
    ((number->_number._type & _RSNumberShort)    == _RSNumberShortMask);
}
RSInline void   __RSNumberMarkUnsignedShort(RSNumberRef number) {
    return __RSNumberMarkShort(number);
    ((((struct __RSNumber*)number)->_number._type &= ~_RSNumberIsFloatMask));
//    ((((struct __RSNumber*)number)->_number._type |= _RSNumberIsUnsignedMask));
    ((((struct __RSNumber*)number)->_number._type |= _RSNumberShortMask));
}
RSInline void   __RSNumberUnMarkUnsignedShort(RSNumberRef number) {
    return __RSNumberUnMarkShort(number);
    ((((struct __RSNumber*)number)->_number._type &= ~_RSNumberIsFloatMask));
//    ((((struct __RSNumber*)number)->_number._type &= ~_RSNumberIsUnsignedMask));
    ((((struct __RSNumber*)number)->_number._type &= ~_RSNumberShortMask));
}

RSInline BOOL   __RSNumberISLong(RSNumberRef number) {
    if (RS_IS_TAGGED_INT(number))
    {
#if __LP64__
        return RSNumberGetType(number) == RSNumberSInt64;
#else
        return RSNumberGetType(number) == RSNumberSInt32;
#endif
    }
    return  (!__RSNumberISFloatType(number)) &&
//            (!__RSNumberISUnsignedType(number)) &&
            ((number->_number._type & _RSNumberLong) == _RSNumberLongMask);
}
RSInline void   __RSNumberMarkLong(RSNumberRef number) {
    if (RS_IS_TAGGED_INT(number))
    {
#if __LP64__
        __RSNumberTaggedSetType(number, RSNumberSInt64);
#else
        __RSNumberTaggedSetType(number, RSNumberSInt32);
#endif
        return;
    }
    ((((struct __RSNumber*)number)->_number._type &= ~_RSNumberIsFloatMask));
//    ((((struct __RSNumber*)number)->_number._type &= ~_RSNumberIsUnsignedMask));
    ((((struct __RSNumber*)number)->_number._type |= _RSNumberLongMask));
}
RSInline void   __RSNumberUnMarkLong(RSNumberRef number) {
    
    ((((struct __RSNumber*)number)->_number._type &= ~_RSNumberIsFloatMask));
//    ((((struct __RSNumber*)number)->_number._type &= ~_RSNumberIsUnsignedMask));
    ((((struct __RSNumber*)number)->_number._type &= ~_RSNumberLongMask));
}


RSInline BOOL   __RSNumberISUnsignedlong(RSNumberRef number) {
    return __RSNumberISLong(number);
    return  (!__RSNumberISFloatType(number)) &&
//            (__RSNumberISUnsignedType(number)) &&
            ((number->_number._type & _RSNumberLong) == _RSNumberLongMask);
}
RSInline void   __RSNumberMarkUnsignedlong(RSNumberRef number) {
    return __RSNumberMarkLong(number);
    ((((struct __RSNumber*)number)->_number._type &= ~_RSNumberIsFloatMask));
//    ((((struct __RSNumber*)number)->_number._type |= _RSNumberIsUnsignedMask));
    ((((struct __RSNumber*)number)->_number._type |= _RSNumberLongMask));
}
RSInline void   __RSNumberUnMarkUnsignedlong(RSNumberRef number) {
    ((((struct __RSNumber*)number)->_number._type &= ~_RSNumberIsFloatMask));
//    ((((struct __RSNumber*)number)->_number._type &= ~_RSNumberIsUnsignedMask));
    ((((struct __RSNumber*)number)->_number._type &= ~_RSNumberLongMask));
}


RSInline BOOL   __RSNumberISFloat(RSNumberRef number) {
    return  (__RSNumberISFloatType(number)) &&
            //(!__RSNumberISUnsignedType(number)) &&
            ((number->_number._type & _RSNumberFloat) == _RSNumberFloatMask);
}
RSInline void   __RSNumberMarkFloat(RSNumberRef number ) {
    ((((struct __RSNumber*)number)->_number._type |= _RSNumberIsFloatMask));
//    ((number->base._rsinfo._rsinfo1 |= _RSNumberIsUnsignedMask));
    ((((struct __RSNumber*)number)->_number._type |= _RSNumberFloatMask));
}
RSInline void   __RSNumberUnMarkFloat(RSNumberRef number ) {
    ((((struct __RSNumber*)number)->_number._type &= ~_RSNumberIsFloatMask));
//    ((number->base._rsinfo._rsinfo1 |= _RSNumberIsUnsignedMask));
    ((((struct __RSNumber*)number)->_number._type &= ~_RSNumberFloatMask));
}


RSInline BOOL   __RSNumberISDouble(RSNumberRef number) {
    return  (__RSNumberISFloatType(number)) &&
//            ((number->base._rsinfo._rsinfo1 & _RSNumberIsUnsigned) == _RSNumberIsUnsignedMask) &&
            ((number->_number._type & _RSNumberDouble) == _RSNumberDoubleMask);
}
RSInline void   __RSNumberMarkDouble(RSNumberRef number) {
    ((((struct __RSNumber*)number)->_number._type |= _RSNumberIsFloatMask));
//    ((number->base._rsinfo._rsinfo1 |= _RSNumberIsUnsignedMask));
    ((((struct __RSNumber*)number)->_number._type |= _RSNumberDoubleMask));
}
RSInline void   __RSNumberUnMarkDouble(RSNumberRef number) {
    ((((struct __RSNumber*)number)->_number._type &= ~_RSNumberIsFloatMask));
//    ((number->base._rsinfo._rsinfo1 |= _RSNumberIsUnsignedMask));
    ((((struct __RSNumber*)number)->_number._type &= ~_RSNumberDoubleMask));
}

RSInline BOOL   __RSNumberISLonglong(RSNumberRef number) {
    if (RS_IS_TAGGED_INT(number)) {
        return RSNumberGetType(number) == RSNumberSInt64;
    }
    return  (!__RSNumberISFloatType(number)) &&
//            (!__RSNumberISUnsignedType(number)) &&
            ((number->_number._type & _RSNumberLonglong) == _RSNumberLonglongMask);
}
RSInline void   __RSNumberMarkLonglong(RSNumberRef number) {
    if (RS_IS_TAGGED_INT(number))
    {
        __RSNumberTaggedSetType(number, RSNumberSInt64);
        return;
    }
    ((((struct __RSNumber*)number)->_number._type &= ~_RSNumberIsFloatMask));
//    ((((struct __RSNumber*)number)->_number._type &= ~_RSNumberIsUnsignedMask));
    ((((struct __RSNumber*)number)->_number._type |= _RSNumberLonglongMask));
}

RSInline void   __RSNumberUnMarkLonglong(RSNumberRef number) {
    ((((struct __RSNumber*)number)->_number._type &= ~_RSNumberIsFloatMask));
//    ((((struct __RSNumber*)number)->_number._type &= ~_RSNumberIsUnsignedMask));
    ((((struct __RSNumber*)number)->_number._type &= ~_RSNumberLonglongMask));
}


RSInline BOOL   __RSNumberISUnsignedlonglong(RSNumberRef number) {
    return __RSNumberISLonglong(number);
    return  (!__RSNumberISFloatType(number)) &&
//            (__RSNumberISUnsignedType(number)) &&
            ((number->_number._type & _RSNumberLonglong) == _RSNumberLonglongMask);
}
RSInline void   __RSNumberMarkUnsignedlonglong(RSNumberRef number) {
    return __RSNumberMarkLonglong(number);
    ((((struct __RSNumber*)number)->_number._type &= ~_RSNumberIsFloatMask));
//    ((((struct __RSNumber*)number)->_number._type |= _RSNumberIsUnsignedMask));
    ((((struct __RSNumber*)number)->_number._type |= _RSNumberLonglongMask));
}
RSInline void   __RSNumberUnMarkUnsignedlonglong(RSNumberRef number) {
    ((((struct __RSNumber*)number)->_number._type &= ~_RSNumberIsFloatMask));
//    ((((struct __RSNumber*)number)->_number._type &= ~_RSNumberIsUnsignedMask));
    ((((struct __RSNumber*)number)->_number._type &= ~_RSNumberLonglongMask));
}

enum {
    _RSNumberISStrongMemory  = 1 << 18,
    _RSNumberISConstant    = 1 << 17,
    
};

RSInline BOOL   __RSNumberISStrongMemory(RSNumberRef number) {
    if (RS_IS_TAGGED_INT(number)) return NO;
    return ((number->_number._type & _RSNumberISStrongMemory) == _RSNumberISStrongMemory);
}
RSInline void   __RSNumberMarkStrongMemory(RSNumberRef number) {
    if (RS_IS_TAGGED_INT(number)) return;
    ((((struct __RSNumber*)number)->_number._type |= _RSNumberISStrongMemory));
}
RSInline void   __RSNumberUnMarkStrongMemory(RSNumberRef number) {
    if (RS_IS_TAGGED_INT(number)) return;
    ((((struct __RSNumber*)number)->_number._type &= ~_RSNumberISStrongMemory));
}


RSInline BOOL   __RSNumberISConstant(RSNumberRef number) {
    if (RS_IS_TAGGED_INT(number)) return YES;
    return ((number->_number._type & _RSNumberISConstant) == _RSNumberISConstant);
}
RSInline void   __RSNumberMarkConstant(RSNumberRef number) {
    if (RS_IS_TAGGED_INT(number)) return;
    ((((struct __RSNumber*)number)->_number._type |= _RSNumberISConstant));
}
RSInline void   __RSNumberUnMarkConstant(RSNumberRef number) {
    if (RS_IS_TAGGED_INT(number)) return;
    ((((struct __RSNumber*)number)->_number._type &= ~_RSNumberISConstant));
}

RSExport char RSNumberCharValue(RSNumberRef aValue) {
    if (!aValue) return 0;
    char value = 0;
    if (RSNumberGetType(aValue) == RSNumberChar && RSNumberGetValue(aValue, &value)) return value;
    return 0;
}

RSExport unsigned char RSNumberUnsignedCharValue(RSNumberRef aValue) {
    if (!aValue) return 0;
    unsigned char value = 0;
    if (RSNumberIsIntegerType(aValue) && RSNumberGetType(aValue) == RSNumberUnsignedChar && RSNumberGetValue(aValue, &value)) return value;
    return 0;
}

RSExport short RSNumberShortValue(RSNumberRef aValue) {
    if (!aValue) return 0;
    short value = 0;
    if (RSNumberIsIntegerType(aValue) && RSNumberGetType(aValue) == RSNumberShort && RSNumberGetValue(aValue, &value)) return value;
    return 0;
}

RSExport unsigned short RSNumberUnsignedShortValue(RSNumberRef aValue) {
    if (!aValue) return 0;
    unsigned short value = 0;
    if (RSNumberIsIntegerType(aValue) && RSNumberGetType(aValue) == RSNumberUnsignedShort && RSNumberGetValue(aValue, &value)) return value;
    return 0;
}

RSExport int RSNumberIntValue(RSNumberRef aValue) {
    if (!aValue) return 0;
    int value = 0;
    if (RSNumberIsIntegerType(aValue) && RSNumberGetType(aValue) == RSNumberInt && RSNumberGetValue(aValue, &value)) return value;
    return 0;
}

RSExport unsigned int RSNumberUnsignedIntValue(RSNumberRef aValue) {
    if (!aValue) return 0;
    unsigned int value = 0;
    if (RSNumberIsIntegerType(aValue) && RSNumberGetType(aValue) == RSNumberUnsignedInt && RSNumberGetValue(aValue, &value)) return value;
    return 0;
}

RSExport long RSNumberLongValue(RSNumberRef aValue) {
    if (!aValue) return 0;
    long value = 0;
    if (RSNumberIsIntegerType(aValue) && RSNumberGetType(aValue) == RSNumberLong && RSNumberGetValue(aValue, &value)) return value;
    return 0;
}

RSExport unsigned long RSNumberUnsignedLongValue(RSNumberRef aValue) {
    if (!aValue) return 0;
    unsigned long value = 0;
    if (RSNumberIsIntegerType(aValue) && RSNumberGetValue(aValue, &value)) return value;
    return 0;
}

RSExport long long RSNumberLongLongValue(RSNumberRef aValue) {
    if (!aValue) return 0;
    long long value = 0;
    if (RSNumberIsIntegerType(aValue) && RSNumberGetValue(aValue, &value)) return value;
    return 0;
}

RSExport unsigned long long RSNumberUnsignedLongLongValue(RSNumberRef aValue) {
    if (!aValue) return 0;
    unsigned long long value = 0;
    if (RSNumberIsIntegerType(aValue) && RSNumberGetValue(aValue, &value)) return value;
    return 0;
}

RSExport float RSNumberFloatValue(RSNumberRef aValue) {
    if (!aValue) return 0;
    float value = 0;
    if (RSNumberIsFloatType(aValue) && RSNumberGetValue(aValue, &value)) return value;
    return 0;
}

RSExport double RSNumberDoubleValue(RSNumberRef aValue) {
    if (!aValue) return 0;
    double value = 0;
    if (RSNumberIsFloatType(aValue) && RSNumberGetValue(aValue, &value)) return value;
    return 0;
}

RSExport BOOL RSNumberBooleanValue(RSNumberRef aValue) {
    if (!aValue) return 0;
    BOOL value = 0;
    if (RSNumberIsBooleanType(aValue) && RSNumberGetType(aValue) == RSNumberBoolean && RSNumberGetValue(aValue, &value)) return value;
    return 0;
}

RSExport RSFloat RSNumberRSFloatValue(RSNumberRef aValue) {
    if (!aValue) return 0;
    RSFloat value = 0;
    if (RSNumberIsFloatType(aValue) && RSNumberGetValue(aValue, &value)) return value;
    return 0;
}

RSExport RSInteger RSNumberIntegerValue(RSNumberRef aValue) {
    if (!aValue) return 0;
    RSInteger value = 0;
    if (RSNumberIsIntegerType(aValue) && RSNumberGetValue(aValue, &value)) return value;
    return 0;
}

RSExport RSUInteger RSNumberUnsignedIntegerValue(RSNumberRef aValue) {
    if (!aValue) return 0;
    RSUInteger value = 0;
    if (RSNumberIsIntegerType(aValue) && RSNumberGetValue(aValue, &value)) return value;
    return 0;
}

RSExport void * RSNumberPointerValue(RSNumberRef aValue) {
    if (!aValue) return 0;
    void * value = 0;
    if (RSNumberGetType(aValue) == RSNumberPointer && RSNumberGetValue(aValue, &value)) return value;
    return 0;
}

RSExport RSRange RSNumberRangeValue(RSNumberRef aValue) {
    if (!aValue) return RSMakeRange(RSNotFound, 0);
    RSRange value = {RSNotFound};
    if (RSNumberGetType(aValue) == RSNumberRSRange && RSNumberGetValue(aValue, &value)) return value;
    return RSMakeRange(RSNotFound, 0);
}

RSExport RSStringRef RSNumberStringValue(RSNumberRef aValue) {
    if (!aValue) return 0;
    RSStringRef value = 0;
    if (RSNumberGetType(aValue) == RSNumberString && RSNumberGetValue(aValue, &value)) return value;
    return 0;
}

RSExport RSNumberType RSNumberGetType(RSNumberRef aNumber)
{
    if (aNumber == nil) HALTWithError(RSInvalidArgumentException, "the number is nil");
    if (aNumber == (RSNumberRef)0) return RSNumberNil;
    
    if (RS_IS_TAGGED_INT(aNumber))
    {
        uintptr_t type_bits = ((uintptr_t)aNumber >> 6) & 0x3; // top 2 bits of low byte
        const RSNumberType canonical_types[4] = {RSNumberSInt8, RSNumberSInt16, RSNumberSInt32, RSNumberSInt64};
        return canonical_types[type_bits];
    }
    
    if (__RSNumberISFloatType(aNumber))
    {
        if (__RSNumberISFloat(aNumber)) return RSNumberFloat;
        if (__RSNumberISDouble(aNumber)) return RSNumberDouble;
        HALTWithError(RSInvalidArgumentException, "the number is damaged");
    }
//    else if (__RSNumberISUnsignedType(aNumber)) {
//        if (__RSNumberISUnsignedShort(aNumber)) return RSNumberUnsignedShort;
//        if (__RSNumberISUnsignedInt(aNumber)) return RSNumberUnsignedInt;  
//        if (__RSNumberISUnsignedlong(aNumber)) return RSNumberUnsignedLong;
//        if (__RSNumberISUnsignedlonglong(aNumber)) return RSNumberUnsignedLonglong;
//        HALTWithError(RSInvalidArgumentException, "the number is damaged");
//    }
    else if (__RSNumberISBoolean(aNumber))
        return RSNumberBoolean;
    else
    {
        if (__RSNumberISInt(aNumber)) return RSNumberInt;
        if (__RSNumberISShort(aNumber)) return RSNumberShort;
        if (__RSNumberISLong(aNumber)) return RSNumberLong;
        if (__RSNumberISLonglong(aNumber)) return RSNumberLonglong;
        if (__RSNumberIsChar(aNumber)) return RSNumberChar;
        if (__RSNumberIsRange(aNumber)) return RSNumberRSRange;
        if (__RSNumberIsPointer(aNumber)) return RSNumberPointer;
        if (__RSNumberIsString(aNumber)) return RSNumberString;
        HALTWithError(RSInvalidArgumentException, "the number is damaged");
    }
    
    return RSNumberNil;
}
static void    __RSNumberSetType(RSNumberRef aNumber, RSNumberType aType)
{
    if (nil == aNumber) HALTWithError(RSMallocException, "the RSNumber is nil");
    switch (aType) {
        case RSNumberNil: return;
        case RSNumberBoolean: return __RSNumberMarkBoolean(aNumber);
        case RSNumberInt: return __RSNumberMarkInt(aNumber);
        case RSNumberShort: return __RSNumberMarkShort(aNumber);
        case RSNumberLong: return __RSNumberMarkLong(aNumber);
        case RSNumberFloat: return __RSNumberMarkFloat(aNumber);
        case RSNumberDouble: return __RSNumberMarkDouble(aNumber);
        case RSNumberLonglong: return __RSNumberMarkLonglong(aNumber);
        case RSNumberChar: return __RSNumberMarkChar(aNumber);
        case RSNumberRSRange: return __RSNumberMarkRange(aNumber);
        case RSNumberPointer: return __RSNumberMarkPointer(aNumber);
        case RSNumberString: return __RSNumberMarkString(aNumber);
//        case RSNumberUnsignedShort: return __RSNumberMarkUnsignedShort(aNumber);
//        case RSNumberUnsignedInt: return __RSNumberMarkUnsignedInt(aNumber);
//        case RSNumberUnsignedLong: return __RSNumberMarkUnsignedlong(aNumber);
//        case RSNumberUnsignedLonglong: return __RSNumberMarkUnsignedlonglong(aNumber);
        default:
            HALTWithError(RSInvalidArgumentException, "the RSNumberType is unknown.");
    }
}
const   RSNumberRef RSBooleanTrue = & __RSBooleanTrue;
const   RSNumberRef RSBooleanFalse= & __RSBooleanFalse;
static  RSTypeID __RSNumberTypeID = _RSRuntimeNotATypeID;

static const struct {
    uint16_t canonicalType:5;	// canonical fixed-width type
    uint16_t floatBit:1;	// is float
    uint16_t storageBit:1;	// storage size (0: (float ? 4 : 8), 1: (float ? 8 : 16) bits)
    uint16_t lgByteSize:3;	// base-2 log byte size of public type
    uint16_t unused:6;
} __RSNumberTypeTable[] = {
    /* 0 */			{0, 0, 0, 0},
    
    /* RSNumberSInt8 */     {RSNumberSInt8, 0, 0, 0, 0},
    /* RSNumberSInt16 */	{RSNumberSInt16, 0, 0, 1, 0},
    /* RSNumberSInt32 */	{RSNumberSInt32, 0, 0, 2, 0},
    /* RSNumberSInt64 */	{RSNumberSInt64, 0, 0, 3, 0},
    /* RSNumberFloat32 */	{RSNumberFloat32, 1, 0, 2, 0},
    /* RSNumberFloat64 */	{RSNumberFloat64, 1, 1, 3, 0},
    
    /* RSNumberBoolean */	{RSNumberSInt8, 0, 0, 0, 0},
    /* RSNumberShort */     {RSNumberSInt16, 0, 0, 1, 0},
    /* RSNumberInt */       {RSNumberSInt32, 0, 0, 2, 0},
#if __LP64__
    /* RSNumberLong */      {RSNumberSInt64, 0, 0, 3, 0},
#else
    /* RSNumberLong */      {RSNumberSInt32, 0, 0, 2, 0},
#endif
    /* RSNumberLongLong */	{RSNumberSInt64, 0, 0, 3, 0},
    /* RSNumberFloat */     {RSNumberFloat32, 1, 0, 2, 0},
    /* RSNumberDouble */	{RSNumberFloat64, 1, 1, 3, 0},
    
#if __LP64__
    /* RSNumberRSIndex */	{RSNumberSInt64, 0, 0, 3, 0},
    /* RSNumberNSInteger */ {RSNumberSInt64, 0, 0, 3, 0},
    /* RSNumberCGFloat */	{RSNumberFloat64, 1, 1, 3, 0},
#else
    /* RSNumberRSIndex */	{RSNumberSInt32, 0, 0, 2, 0},
    /* RSNumberNSInteger */ {RSNumberSInt32, 0, 0, 2, 0},
    /* RSNumberCGFloat */	{RSNumberFloat32, 1, 0, 2, 0},
#endif
    
//    /* RSNumberSInt128 */	{RSNumberSInt128, 0, 1, 4, 0},
};

static  RSNumberRef __RSNumberCreateInstance(RSAllocatorRef allocator, RSNumberType theType, void* value)
{
    RSNumberRef aNumber = nil;
    aNumber = __RSRuntimeCreateInstance(RSAllocatorSystemDefault, __RSNumberTypeID, sizeof(struct __RSNumber) - sizeof(struct __RSRuntimeBase));
    //aNumber = RSAllocatorAllocate(RSAllocatorSystemDefault, sizeof(struct __RSNumber));
    if(aNumber)
    {
        __RSNumberMarkStrongMemory(aNumber);
        __RSNumberSetType(aNumber, theType);
        RSNumberSetValue(aNumber, theType, value);
        //    ((struct __RSNumber*)aNumber)->_base._rsinfo._customRef = 0;
        __RSNumberMarkStrongMemory(aNumber);
        //    ((struct __RSNumber*)aNumber)->_base._rsinfo._objId = (RSBitU32)RSNumberGetTypeID();
    }
    else
    {
        aNumber = (RSNumberRef)0;
        HALTWithError(RSMallocException, "Malloc RSNumber failed");
    }
    return aNumber;
}

static RSTypeRef __RSNumberClassCopy(RSAllocatorRef allocator, RSTypeRef rs, BOOL mutableCopy)
{
    RSNumberType nt = RSNumberGetType(rs);
    unsigned long long value = 0;
    RSNumberGetValue(rs, &value);
    RSNumberRef copy = RSNumberCreate(allocator, nt, &value);
    return copy;
}

static void    __RSNumberClassDeallocate(RSTypeRef obj)
{
    RSTypeID id = RSGetTypeID(obj);
    if (id != RSNumberGetTypeID()) HALTWithError(RSInvalidArgumentException, "the obj is not an RSNumber");
    if (obj == RSBooleanTrue) return;
    if (obj == RSBooleanFalse) return;
    RSNumberRef aNumber = (RSNumberRef)obj;
    if (!__RSNumberISStrongMemory(aNumber)) {
        return;
    }
    //RSAllocatorDeallocate(RSAllocatorSystemDefault, obj);
}

static BOOL    __RSNumberClassEqual(RSTypeRef obj1, RSTypeRef obj2)
{
    if (obj1 == nil) HALTWithError(RSInvalidArgumentException, "the obj1 is nil");
    if (obj2 == nil) HALTWithError(RSInvalidArgumentException, "the obj2 is nil");
    return RSCompareEqualTo == RSNumberCompare(obj1, obj2, nil);
    // the object is RSNumber
    RSNumberRef num1 = (RSNumberRef)obj1;
    RSNumberRef num2 = (RSNumberRef)obj2;
    RSNumberType type1 = RSNumberGetType(obj1);
    RSNumberType type2 = RSNumberGetType(obj2);
    if (type1 != type2) return NO;
    BOOL tagged1 = RS_IS_TAGGED_INT(obj1);
    BOOL tagged2 = RS_IS_TAGGED_INT(obj2);
    if (tagged1 && tagged2) {
        return num1 == num2;
    } else if (tagged1 == YES)
    switch (type1)
    {
        case RSNumberBoolean:
            return (num1->_number._pay._bool == num2->_number._pay._bool)             ? (YES):(NO);
            break;
        case RSNumberInt:
            return (num1->_number._pay._int == num2->_number._pay._int)               ? (YES):(NO);
        case RSNumberLong:
            return (num1->_number._pay._long == num2->_number._pay._long)             ? (YES):(NO);
            break;
        case RSNumberLonglong:
            return (num1->_number._pay._longlong == num2->_number._pay._longlong)     ? (YES):(NO);
            break;
        case RSNumberFloat:
            return (num1->_number._pay._float == num2->_number._pay._float)           ? (YES):(NO);
            break;
        case RSNumberDouble:
            return (num1->_number._pay._double == num2->_number._pay._double)         ? (YES):(NO);
            break;
        case RSNumberRSRange:
            return (num1->_number._pay._range.location == num2->_number._pay._range.location) && (num1->_number._pay._range.length == num2->_number._pay._range.length)            ? (YES):(NO);
            break;
        case RSNumberPointer:
            return (num1->_number._pay._pointer == num2->_number._pay._pointer)   ? (YES):(NO);
            break;
        case RSNumberString:
            return RSEqual(num1->_number._pay._string , num2->_number._pay._string);
        default:
            return NO;
            break;
    }
    
    return NO;
}

static RSHashCode __RSNumberClassHash(RSTypeRef rs)
{
    if (RS_IS_TAGGED_INT(rs)) {
        RSHashCode hc = 0;
        RSNumberGetValue(rs, &hc);
        return hc;
    }
    return (RSHashCode)(((RSNumberRef)rs)->_number._pay._int64);
}

static RSStringRef __RSNumberClassDescription(RSTypeRef obj)
{
    static RSCBuffer formats[] = {"Nil","%d","%f","%lld","%ld","(%ld, %ld)","0x%x","","","",""};
    RSBlock number[128] = {0};
    RSNumberType type = RSNumberGetType(obj);
    if (RS_IS_TAGGED_INT(obj))
    {
        long long value = 0;
        RSNumberGetValue(obj, &value);
        switch (type)
        {
            case RSNumberSInt8:
                strncpy(number, value ? "YES" : "NO", value ? 3 : 2);
                break;
            case RSNumberSInt16:
                snprintf(number, 128, formats[1], value);
                break;
            case RSNumberSInt32:
                snprintf(number, 128, formats[1], value);
                break;
            case RSNumberSInt64:
                snprintf(number, 128, formats[3], value);
                break;
        }
    }
    else
    {
        switch (type) {
            case RSNumberNil:
                strncpy(number, formats[0], 3);
                break;
            case RSNumberChar:
                number[0] = ((struct __RSNumber*)obj)->_number._pay._char;
                break;
            case RSNumberBoolean:
                strncpy(number, ((struct __RSNumber*)obj)->_number._pay._bool ? "YES" : "NO", ((struct __RSNumber*)obj)->_number._pay._bool ? 3 : 2);
                break;
            case RSNumberShort:
                snprintf(number, 128, formats[1], ((struct __RSNumber*)obj)->_number._pay._short);
                break;
            case RSNumberInt:
                snprintf(number, 128, formats[1], ((struct __RSNumber*)obj)->_number._pay._int);
                break;
            case RSNumberLong:
                snprintf(number, 128, formats[1], ((struct __RSNumber*)obj)->_number._pay._long);
                break;
            case RSNumberFloat:
                snprintf(number, 128, formats[2], ((struct __RSNumber*)obj)->_number._pay._float);
                break;
            case RSNumberDouble:
                snprintf(number, 128, formats[2], ((struct __RSNumber*)obj)->_number._pay._double);
                break;
            case RSNumberLonglong:
                snprintf(number, 128, formats[3], ((struct __RSNumber*)obj)->_number._pay._longlong);
                break;
            case RSNumberRSRange:
                snprintf(number, 128, formats[5], ((struct __RSNumber*)obj)->_number._pay._range.location, ((struct __RSNumber*)obj)->_number._pay._range.length);
                break;
            case RSNumberPointer:
                snprintf(number, 128, formats[6], ((struct __RSNumber*)obj)->_number._pay._pointer);
                break;
            case RSNumberString:
                return RSDescription(((struct __RSNumber*)obj)->_number._pay._string);
            default:
                HALTWithError(RSInvalidArgumentException, "the RSNumber is unknown type.");
                break;
        }
    }
    RSStringRef desciption = RSStringCreateWithCString(RSAllocatorSystemDefault, number, RSStringEncodingUTF8);
    return desciption;
}

static  RSRuntimeClass __RSNumberClass = {
    _RSRuntimeScannedObject,
    0,
    "RSNumber",
    nil,
    __RSNumberClassCopy,
    __RSNumberClassDeallocate,
    __RSNumberClassEqual,
    __RSNumberClassHash,
    __RSNumberClassDescription,
    nil,
    nil,
};

RSTypeID    RSNumberGetTypeID()
{
    return __RSNumberTypeID;
}

static RSNumberRef __RSNumberCreateConstant(RSAllocatorRef allocator);
RSPrivate   void    __RSNumberInitialize()
{
    __RSNumberTypeID = __RSRuntimeRegisterClass(&__RSNumberClass);
    __RSRuntimeSetClassTypeID(&__RSNumberClass, __RSNumberTypeID);
    BOOL ktrue = YES;
    RSNumberRef _skt = nil;
    _skt = (RSNumberRef)__RSNumberCreateConstant(RSAllocatorSystemDefault);
    __RSNumberUnMarkConstant(_skt);
    RSNumberSetValue(_skt, RSNumberBoolean, &ktrue);
    memcpy(&__RSBooleanTrue, _skt, sizeof(struct __RSNumber));
    __RSBooleanTrue._base._rsisa = (RSIndex)&__RSBooleanTrue;
    RSRelease(_skt);
    
    __RSNumberMarkConstant(RSBooleanTrue);
    memcpy((void*)RSBooleanFalse, RSBooleanTrue, sizeof(struct __RSNumber));
    __RSBooleanFalse._base._rsisa = (RSIndex)&__RSBooleanFalse;
    ((struct __RSNumber*)RSBooleanFalse)->_number._pay._bool = NO;
    
    __RSRuntimeSetInstanceAsStackValue(RSBooleanTrue);
    __RSRuntimeSetInstanceAsStackValue(RSBooleanFalse);
}

static RSNumberRef __RSNumberCreateConstant(RSAllocatorRef allocator)
{
    //RSNumberRef aNumber = RSNumberInit(RSNumberAlloc());
    RSNumberRef aNumber = __RSRuntimeCreateInstance(allocator, __RSNumberTypeID, sizeof(struct __RSNumber) - sizeof(struct __RSRuntimeBase));
    if (aNumber && aNumber != (RSNumberRef)0) {
        __RSNumberMarkConstant(aNumber);
        
        return aNumber;
    }
    return (RSNumberRef)0;
}

RSExport RSNumberRef RSNumberCreateInt(RSAllocatorRef allocator, int value)
{
    return RSNumberCreate(allocator, RSNumberInt, &value);
    RSNumberRef aNumber = __RSNumberCreateConstant(allocator);
    if (aNumber == (RSNumberRef)0) return aNumber;
    RSNumberSetValue(aNumber, RSNumberInt, &value);
    return aNumber;
}

RSExport RSNumberRef RSNumberCreateUnsignedInt(RSAllocatorRef allocator, unsigned int value)
{
    return RSNumberCreate(allocator, RSNumberUnsignedInt, &value);
    RSNumberRef aNumber = __RSNumberCreateConstant(allocator);
    if (aNumber == (RSNumberRef)0) return aNumber;
    RSNumberSetValue(aNumber, RSNumberUnsignedInt, &value);
    return aNumber;
}

RSExport RSNumberRef RSNumberCreateShort(RSAllocatorRef allocator, short value)
{
    return RSNumberCreate(allocator, RSNumberShort, &value);
    RSNumberRef aNumber = __RSNumberCreateConstant(allocator);
    if (aNumber == (RSNumberRef)0) return aNumber;
    RSNumberSetValue(aNumber, RSNumberShort, &value);
    return aNumber;
}

RSExport RSNumberRef RSNumberCreateUnsignedShort(RSAllocatorRef allocator, unsigned short value)
{
    return RSNumberCreate(allocator, RSNumberUnsignedShort, &value);
    RSNumberRef aNumber = __RSNumberCreateConstant(allocator);
    if (aNumber == (RSNumberRef)0) return aNumber;
    RSNumberSetValue(aNumber, RSNumberUnsignedShort, &value);
    return aNumber;
}

RSExport RSNumberRef RSNumberCreateInteger(RSAllocatorRef allocator, RSInteger value)
{
    return RSNumberCreate(allocator, RSNumberInteger, &value);
    RSNumberRef aNumber = __RSNumberCreateConstant(allocator);
    if (aNumber == (RSNumberRef)0) return aNumber;
    RSNumberSetValue(aNumber, RSNumberInteger, &value);
    return aNumber;
}
RSExport RSNumberRef RSNumberCreateUnsignedInteger(RSAllocatorRef allocator, RSUInteger value)
{
    return RSNumberCreate(allocator, RSNumberUnsignedInteger, &value);
    RSNumberRef aNumber = __RSNumberCreateConstant(allocator);
    if (aNumber == (RSNumberRef)0) return aNumber;
    RSNumberSetValue(aNumber, RSNumberUnsignedInteger, &value);
    return aNumber;
}

RSExport RSNumberRef RSNumberCreateBoolean(RSAllocatorRef allocator, BOOL value)
{
    if (value) return RSBooleanTrue;
    else return RSBooleanFalse;
}

RSExport RSNumberRef RSNumberCreateLong(RSAllocatorRef allocator, long value)
{
    return RSNumberCreate(allocator, RSNumberLong, &value);
    RSNumberRef aNumber = __RSNumberCreateConstant(allocator);
    if (aNumber == (RSNumberRef)0) return aNumber;
    RSNumberSetValue(aNumber, RSNumberLong, &value);
    return aNumber;
}
RSExport RSNumberRef RSNumberCreateLonglong(RSAllocatorRef allocator, long long value)
{
    return RSNumberCreate(allocator, RSNumberLonglong, &value);
    RSNumberRef aNumber = __RSNumberCreateConstant(allocator);
    if (aNumber == (RSNumberRef)0) return aNumber;
    RSNumberSetValue(aNumber, RSNumberLonglong, &value);
    return aNumber;
}

RSExport RSNumberRef RSNumberCreateUnsignedLong(RSAllocatorRef allocator, unsigned long value)
{
    return RSNumberCreate(allocator, RSNumberUnsignedLong, &value);
    RSNumberRef aNumber = __RSNumberCreateConstant(allocator);
    if (aNumber == (RSNumberRef)0) return aNumber;
    RSNumberSetValue(aNumber, RSNumberUnsignedLong, &value);
    return aNumber;
}
RSExport RSNumberRef RSNumberCreateUnsignedLonglong(RSAllocatorRef allocator, unsigned long long value)
{
    return RSNumberCreate(allocator, RSNumberUnsignedLonglong, &value);
    RSNumberRef aNumber = __RSNumberCreateConstant(allocator);
    if (aNumber == (RSNumberRef)0) return aNumber;
    RSNumberSetValue(aNumber, RSNumberUnsignedLonglong, &value);
    return aNumber;
}
RSExport RSNumberRef RSNumberCreateFloat(RSAllocatorRef allocator, float value)
{
    return RSNumberCreate(allocator, RSNumberFloat, &value);
    RSNumberRef aNumber = __RSNumberCreateConstant(allocator);
    if (aNumber == (RSNumberRef)0) return aNumber;
    RSNumberSetValue(aNumber, RSNumberFloat, &value);
    return aNumber;
}
RSExport RSNumberRef RSNumberCreateDouble(RSAllocatorRef allocator, double value)
{
    return RSNumberCreate(allocator, RSNumberDouble, &value);
    RSNumberRef aNumber = __RSNumberCreateConstant(allocator);
    if (aNumber == (RSNumberRef)0) return aNumber;
    RSNumberSetValue(aNumber, RSNumberDouble, &value);
    return aNumber;
}

RSExport RSNumberRef RSNumberCreateRSFloat(RSAllocatorRef allocator, RSFloat value)
{
    return RSNumberCreate(allocator, RSNumberRSFloat, &value);
    RSNumberRef aNumber = __RSNumberCreateConstant(allocator);
    if (aNumber == (RSNumberRef)0) return aNumber;
    RSNumberSetValue(aNumber, RSNumberRSFloat, &value);
    return aNumber;
}

RSExport RSNumberRef RSNumberCreateRange(RSAllocatorRef allocator, RSRange value) {
    return RSNumberCreate(allocator, RSNumberRSRange, &value);
}

RSExport RSNumberRef RSNumberCreatePointer(RSAllocatorRef allocator, void *value) {
    return RSNumberCreate(allocator, RSNumberPointer, &value);
}

RSExport RSNumberRef RSNumberCreateString(RSAllocatorRef allocator, RSStringRef value) {
    return RSNumberCreate(allocator, RSNumberString, (void *)&value);
}

RSExport RSNumberRef RSNumberCreateChar(RSAllocatorRef allocator, char value) {
    return RSNumberCreate(allocator, RSNumberChar, &value);
}

RSExport RSNumberRef RSNumberCreateUnsignedChar(RSAllocatorRef allocator, unsigned char value) {
    return RSNumberCreate(allocator, RSNumberUnsignedChar, &value);
}

RSExport RSNumberRef RSNumberCreate(RSAllocatorRef allocator, RSNumberType theType, void* valuePtr)
{
    if (valuePtr == nil) HALTWithError(RSInvalidArgumentException, "the value is nil");
    if (theType == RSNumberNil) return (RSNumberRef)0;
    if (theType == RSNumberBoolean) return *((BOOL*)valuePtr) ? RSBooleanTrue : RSBooleanFalse;
    
    if (theType < 6 || theType == RSNumberLonglong)
    {
        switch (__RSNumberTypeTable[theType].canonicalType)
        {
            // canonicalized client-desired type
            case RSNumberChar:
            case RSNumberSInt8:
            {
                int8_t value = *(int8_t *)valuePtr;
                return (RSNumberRef)((uintptr_t)((intptr_t)value << 8) | (0 << 6) | RSTaggedObjectID_Integer);
            }
            case RSNumberSInt16:
            {
                int16_t value = *(int16_t *)valuePtr;
                return (RSNumberRef)((uintptr_t)((intptr_t)value << 8) | (1 << 6) | RSTaggedObjectID_Integer);
            }
            case RSNumberSInt32:
            {
                int32_t value = *(int32_t *)valuePtr;
#if !__LP64__
                // We don't bother allowing the min 24-bit integer -2^23 to also be fast-pathed;
                // tell anybody that complains about that to go ... hang.
                int32_t limit = (1L << 23);
                if (value <= -limit || limit <= value) break;
#endif
                uintptr_t ptr_val = ((uintptr_t)((intptr_t)value << 8) | (2 << 6) | RSTaggedObjectID_Integer);
                return (RSNumberRef)ptr_val;
            }
            case RSNumberSInt64:
            {
                int64_t value = *(int64_t *)valuePtr;
#if __LP64__
                // We don't bother allowing the min 56-bit integer -2^55 to also be fast-pathed;
                // tell anybody that complains about that to go ... hang.
                int64_t limit = (1L << 55);
                if (value <= -limit || limit <= value) break;
#else
                // We don't bother allowing the min 24-bit integer -2^23 to also be fast-pathed;
                // tell anybody that complains about that to go ... hang.
                int64_t limit = (1L << 23);
                if (value <= -limit || limit <= value) break;
#endif
                uintptr_t ptr_val = ((uintptr_t)((intptr_t)value << 8) | (3 << 6) | RSTaggedObjectID_Integer);
                return (RSNumberRef)ptr_val;
            }
        }
    }
    RSNumberRef aNumber = __RSNumberCreateInstance(allocator, theType, valuePtr);
    return aNumber ? (aNumber) : ((RSNumberRef)0);
}

RSExport void RSNumberSetValue(RSNumberRef aNumber, RSNumberType numberType, void* value)
{
    if (aNumber == nil || value == nil) HALTWithError(RSInvalidArgumentException, "the number or value is nil");
    if (aNumber == (RSNumberRef)0) return;
    if (numberType == RSNumberNil) return;
    if (__RSNumberISConstant(aNumber)) return;
//    if (RS_IS_TAGGED_INT(aNumber))
//    {
//        aNumber = (RSNumberRef)((uintptr_t)((intptr_t)value << 8) | (0 << 6) | RSTaggedObjectID_Integer);
//        return;
//    }
    ((struct __RSNumber*)aNumber)->_number._type &= 0x00;// clean the number type
    __RSNumberSetType(aNumber, numberType);
    switch (numberType)
    {
        case RSNumberChar:
            ((struct __RSNumber*)aNumber)->_number._pay._char = *(char *)value; return;
            break;
        case RSNumberBoolean:
            ((struct __RSNumber*)aNumber)->_number._pay._bool = *(BOOL*)value; return;
            break;
        case RSNumberShort:
            ((struct __RSNumber*)aNumber)->_number._pay._short = *(short*)value; return;
            break;
        case RSNumberInt:
            ((struct __RSNumber*)aNumber)->_number._pay._int = *(int*)value; return;
            break;
        case RSNumberLong:
            ((struct __RSNumber*)aNumber)->_number._pay._long = *(long*)value;
            return;
            break;
        case RSNumberFloat:
            ((struct __RSNumber*)aNumber)->_number._pay._float = *(float*)value;
            return;
            break;
        case RSNumberDouble:
            ((struct __RSNumber*)aNumber)->_number._pay._double = *(double*)value;
            return;
            break;
        case RSNumberLonglong:
            ((struct __RSNumber*)aNumber)->_number._pay._longlong = *(long long*)value;
            return;
            break;
        case RSNumberString:
            ((struct __RSNumber*)aNumber)->_number._pay._string = *(RSStringRef *)value; return;
            break;
        case RSNumberPointer:
            ((struct __RSNumber*)aNumber)->_number._pay._pointer = *(void **)value; return;
            break;
        case RSNumberRSRange:
            ((struct __RSNumber*)aNumber)->_number._pay._range = *(RSRange *)value; return;
            break;
//        case RSNumberUnsignedShort:
//            ((struct __RSNumber*)aNumber)->_number._pay._ushort = *(unsigned short*)value; return;
//        case RSNumberUnsignedInt:
//            ((struct __RSNumber*)aNumber)->_number._pay._uint = *(unsigned int*)value; return;
//            break;
//        case RSNumberUnsignedLong:
//            ((struct __RSNumber*)aNumber)->_number._pay._ulong = *(unsigned long*)value;
//            return;
//            break;
//        case RSNumberUnsignedLonglong:
//            ((struct __RSNumber*)aNumber)->_number._pay._ulonglong = *(unsigned long long*)value;
//            return;
        default:
            HALTWithError(RSInvalidArgumentException, "the RSNumber is unknown type.");
            break;
    }
}

#define CVT_COMPAT(SRC_TYPE, DST_TYPE, FT) do { \
        SRC_TYPE sv; memmove(&sv, data, sizeof(SRC_TYPE)); \
        DST_TYPE dv = (DST_TYPE)(sv); \
        memmove(valuePtr, &dv, sizeof(DST_TYPE)); \
        SRC_TYPE vv = (SRC_TYPE)dv; return (FT) || (vv == sv); \
    } while (0)

RSExport BOOL RSNumberGetValue(RSNumberRef aNumber, void *value)
{
    if (aNumber == nil || value == nil) HALTWithError(RSInvalidArgumentException, "");
    if (aNumber == (RSNumberRef)0) return NO;

    RSNumberType theType = RSNumberGetType(aNumber);
    if (theType == RSNumberNil) return NO;
    
    if (RS_IS_TAGGED_INT(aNumber))
    {
        uint8_t localMemory[128];
        if (!value) value = localMemory;
        intptr_t taggedInteger = (intptr_t)aNumber;
        taggedInteger = taggedInteger >> 8;
        switch (__RSNumberTypeTable[theType].canonicalType)
        {
            // canonicalized client-desired type
            case RSNumberSInt8:
#if 0
                if (taggedInteger < INT8_MIN) {
                    *(int8_t *)value = INT8_MIN;
                    return NO;
                }
                if (INT8_MAX < taggedInteger) {
                    *(int8_t *)value = INT8_MAX;
                    return NO;
                }
#endif
                *(int8_t *)value = (int8_t)taggedInteger;
                return YES;
            case RSNumberSInt16:
#if 0
                if (taggedInteger < INT16_MIN) {
                    *(int16_t *)value = INT16_MIN;
                    return NO;
                }
                if (INT16_MAX < taggedInteger) {
                    *(int16_t *)value = INT16_MAX;
                    return NO;
                }
#endif
                *(int16_t *)value = (int16_t)taggedInteger;
                return YES;
            case RSNumberSInt32:
                *(int32_t *)value = (int32_t)taggedInteger;
                return YES;
#if __LP64__
            case RSNumberSInt64:
                *(int64_t *)value = (int64_t)taggedInteger;
                return YES;
#endif
        }
    }
    
    switch (theType)
    {
        case RSNumberChar:
            *(char*)value = aNumber->_number._pay._char;
            break;
        case RSNumberSInt8:
            *(BOOL*)value = aNumber->_number._pay._bool;
            break;
        case RSNumberInt:
            *(int*)value = aNumber->_number._pay._int;
            break;
        case RSNumberLong:
            *(long*)value = aNumber->_number._pay._long;
            break;
        case RSNumberFloat:
            *(float*)value = aNumber->_number._pay._float;
            break;
        case RSNumberDouble:
            *(double*)value = aNumber->_number._pay._double;
            break;
        case RSNumberLonglong:
            *(long long*)value = aNumber->_number._pay._longlong;
            break;
        case RSNumberRSRange:
            *(RSRange*)value = aNumber->_number._pay._range;
            break;
        case RSNumberPointer:
            *(void**)value = aNumber->_number._pay._pointer;
            break;
        case RSNumberString:
            *(RSStringRef*)value = aNumber->_number._pay._string;
            break;
//        case RSNumberUnsignedInt:
//            *(unsigned int*)value = aNumber->_number._pay._uint;
//            break;
//        case RSNumberUnsignedLong:
//            *(unsigned long*)value = aNumber->_number._pay._ulong;
//            break;
//        case RSNumberUnsignedLonglong:
//            *(unsigned long long*)value = aNumber->_number._pay._ulonglong;
//            break;
        default:
            HALTWithError(RSInvalidArgumentException, "the RSNumber is unknown type.");
            break;
    }
    return YES;
}

RSInline BOOL __RSNumberTypeIsFloatGroup(RSNumberType t) {
    return (t == RSNumberFloat || t == RSNumberFloat32 || t == RSNumberFloat64 || t == RSNumberDouble);
}

RSInline BOOL __RSNumberTypeIsIntGroup(RSNumberType t) {
    return (t == RSNumberSInt16 || t == RSNumberSInt32 || t == RSNumberSInt64 || t == RSNumberSInt8 || t == RSNumberChar);
}

RSInline BOOL __RSNumberIsSameGroup(RSNumberType t1, RSNumberType t2) {
    if (t1 == t2) return YES;
    BOOL is_t1_floating = __RSNumberTypeIsFloatGroup(t1);
    if (is_t1_floating && __RSNumberTypeIsFloatGroup(t2)) return YES;
    else if (!__RSNumberTypeIsFloatGroup(t2) && t2 != RSNumberBoolean) return YES;
    return NO;
}

RSInline BOOL __RSNumberTypeIsSpecialPayload(RSNumberType t) {
    return (t == RSNumberRSRange || t == RSNumberPointer || t == RSNumberString);
}

RSExport RSComparisonResult RSNumberCompare(RSNumberRef aNumber, RSNumberRef other, void *context)
{
    if (aNumber == nil || other == nil) HALTWithError(RSInvalidArgumentException, "");
    __RSGenericValidInstance(aNumber, __RSNumberTypeID);
    __RSGenericValidInstance(other, __RSNumberTypeID);
    RSNumberType numberType = RSNumberGetType(aNumber);
    if (RS_IS_TAGGED_INT(aNumber) && RS_IS_TAGGED_INT(other)) {
        int64_t value1 = 0, value2 = 0;
        BOOL f1 = RSNumberGetValue(aNumber, &value1);
        BOOL f2 = RSNumberGetValue(other, &value2);
        if (f1 && f2) {
            if (value1 > value2) return RSCompareGreaterThan;
            if (value1 < value2) return RSCompareLessThan;
            return RSCompareEqualTo;
        }
    }
    
    RSNumberType otherNumberType = RSNumberGetType(other);
    BOOL isSameGroup = __RSNumberIsSameGroup(numberType, otherNumberType);
    if (isSameGroup) {
        if (__RSNumberTypeIsFloatGroup(numberType)) {
            double n1 = 0.0f, n2 = 0.0f;
            if (RSNumberGetValue(aNumber, &n1) && RSNumberGetValue(other, &n2)) {
                if (isnan(n1) && isnan(n2)) return RSCompareEqualTo;
                double s1 = copysign(1.0, n1);
                double s2 = copysign(1.0, n2);
                if (isnan(n1)) return (s2 < 0.0) ? RSCompareGreaterThan : RSCompareLessThan;
                if (isnan(n2)) return (s1 < 0.0) ? RSCompareLessThan : RSCompareGreaterThan;
                // at this point, we know we don't have any NaNs
                if (s1 < s2) return RSCompareLessThan;
                if (s2 < s1) return RSCompareGreaterThan;
                // at this point, we know the signs are the same; do not combine these tests
                if (n1 < n2) return RSCompareLessThan;
                if (n2 < n1) return RSCompareGreaterThan;
                return RSCompareEqualTo;
            }
        } else if (__RSNumberTypeIsIntGroup(numberType)){
            int64_t n1 = 0, n2 = 0;
            if (RSNumberGetValue(aNumber, &n1) && RSNumberGetValue(other, &n2)) {
                if (n1 > n2) return RSCompareGreaterThan;
                if (n1 < n2) return RSCompareLessThan;
                return RSCompareEqualTo;
            }
        } else {
            struct __RSBaseNumber pay1 = aNumber->_number;
            struct __RSBaseNumber pay2 = other->_number;
            if (otherNumberType == RSNumberRSRange) {
                if (pay1._pay._range.location > pay2._pay._range.location) return RSCompareGreaterThan;
                else if (pay1._pay._range.location < pay2._pay._range.location) return RSCompareLessThan;
                return RSCompareEqualTo;
            } else if (otherNumberType == RSNumberPointer) {
                if (pay1._pay._pointer > pay2._pay._pointer) return RSCompareGreaterThan;
                else if (pay1._pay._pointer < pay2._pay._pointer) return RSCompareLessThan;
                return RSCompareEqualTo;
            } else if (otherNumberType == RSNumberString) {
                return RSStringCompare(pay1._pay._string, pay2._pay._string, nil);
            }
        }
    } else {
        if (!(__RSNumberTypeIsSpecialPayload(numberType) || __RSNumberTypeIsSpecialPayload(otherNumberType))) {
            if (__RSNumberTypeIsFloatGroup(numberType)) {
                double n1 = 0.0f, n2 = 0.0f;
                RSNumberGetValue(aNumber, &n1);
                int64_t l1 = 0;
                RSNumberGetValue(other, &l1);
                n2 = (double)l1;
                
                if (isnan(n1) && isnan(n2)) return RSCompareEqualTo;
                double s1 = copysign(1.0, n1);
                double s2 = copysign(1.0, n2);
                if (isnan(n1)) return (s2 < 0.0) ? RSCompareGreaterThan : RSCompareLessThan;
                if (isnan(n2)) return (s1 < 0.0) ? RSCompareLessThan : RSCompareGreaterThan;
                // at this point, we know we don't have any NaNs
                if (s1 < s2) return RSCompareLessThan;
                if (s2 < s1) return RSCompareGreaterThan;
                // at this point, we know the signs are the same; do not combine these tests
                if (n1 < n2) return RSCompareLessThan;
                if (n2 < n1) return RSCompareGreaterThan;
                return RSCompareEqualTo;
            } else if (__RSNumberTypeIsFloatGroup(otherNumberType)) {
                double n1 = 0.0f, n2 = 0.0f;
                RSNumberGetValue(other, &n1);
                int64_t l1 = 0;
                RSNumberGetValue(aNumber, &l1);
                n2 = (double)l1;
                
                if (isnan(n1) && isnan(n2)) return RSCompareEqualTo;
                double s1 = copysign(1.0, n1);
                double s2 = copysign(1.0, n2);
                if (isnan(n1)) return (s2 < 0.0) ? RSCompareGreaterThan : RSCompareLessThan;
                if (isnan(n2)) return (s1 < 0.0) ? RSCompareLessThan : RSCompareGreaterThan;
                // at this point, we know we don't have any NaNs
                if (s1 < s2) return RSCompareLessThan;
                if (s2 < s1) return RSCompareGreaterThan;
                // at this point, we know the signs are the same; do not combine these tests
                if (n1 < n2) return RSCompareLessThan;
                if (n2 < n1) return RSCompareGreaterThan;
                return RSCompareEqualTo;
            }
        }
        int result = __builtin_memcmp(&aNumber->_number, &other->_number, sizeof(struct __RSBaseNumber));
        if (result > 0) return RSCompareGreaterThan;
        else if (result < 0) return RSCompareLessThan;
        return RSCompareEqualTo;
    }
    HALTWithError(RSInvalidArgumentException, "overflow");
}

RSExport BOOL RSNumberIsFloatType(RSNumberRef aNumber)
{
    __RSGenericValidInstance(aNumber, __RSNumberTypeID);
    return __RSNumberTypeIsFloatGroup(RSNumberGetType(aNumber));
}

RSExport BOOL RSNumberIsIntegerType(RSNumberRef aNumber) {
    __RSGenericValidInstance(aNumber, __RSNumberTypeID);
    return __RSNumberTypeIsIntGroup(RSNumberGetType(aNumber));
}

RSExport BOOL RSNumberIsBooleanType(RSNumberRef aNumber)
{
    __RSGenericValidInstance(aNumber, __RSNumberTypeID);
    return __RSNumberISBoolean(aNumber);
}

// This function separated out from __RSNumberCopyFormattingDescription() so the plist creation can use it as well.

static RSStringRef __RSNumberCreateFormattingDescriptionAsFloat64(RSAllocatorRef allocator, RSTypeRef obj) {
    double d = 0.0f;
    RSNumberGetValue((RSNumberRef)obj, &d);
    if (isnan(d)) {
        return (RSStringRef)RSRetain(RSSTR("nan"));
    }
    if (isinf(d)) {
        return (RSStringRef)RSRetain((0.0 < d) ? RSSTR("+infinity") : RSSTR("-infinity"));
    }
    if (0.0 == d) {
        return (RSStringRef)RSRetain(RSSTR("0.0"));
    }
    // if %g is used here, need to use DBL_DIG + 2 on Mac OS X, but %f needs +1
    return RSStringCreateWithFormat(allocator, RSSTR("%.*g"), DBL_DIG + 2, d);
}

RSPrivate RSStringRef __RSNumberCopyFormattingDescriptionAsFloat64(RSTypeRef obj)
{
    return __RSNumberCreateFormattingDescriptionAsFloat64(RSAllocatorSystemDefault, obj);
}

//for(n=0; b; n++) b &= b-1;