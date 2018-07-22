//
//  RSPrivateStringFormat.h
//  RSCoreFoundation
//
//  Created by RetVal on 10/24/12.
//  Copyright (c) 2012 RetVal. All rights reserved.
//

#ifndef RSCoreFoundation_RSPrivateStringFormat_h
#define RSCoreFoundation_RSPrivateStringFormat_h

#include <RSCoreFoundation/RSBase.h>

enum {
    RSFormatLiteralType = 6,
    RSFormatUnsignedLongType = 7,
    RSFormatLongType = 8,
    RSFormatDoubleType = 9,
    RSFormatPointerType = 10,
    RSFormatObjectType = 11,
    RSFormatRSType = 12,
    RSFormatUnicharsType = 13,
    RSFormatCharType = 14,
    RSFormatCharsType = 15,
    RSFormatPascalCharsType = 16,
    RSFormatSingleUnicharType = 17,
    RSFormatDummyPointerType = 18,
};

enum {
    RSFormatStyleDecimal = (1 << 0),
    RSFormatStyleScientific = (1 << 1),
    RSFormatStyleDecimalOrScientific = RSFormatStyleDecimal|RSFormatStyleScientific,
    RSFormatStyleUnsigned = (1 << 2)
};

//enum {
//    RSFormatLiteralType = 32,
//    RSFormatLongType = 33,
//    RSFormatDoubleType = 34,
//    RSFormatPointerType = 35,
//    RSFormatObjectType = 36,		/* handled specially */	/* ??? not used anymore, can be removed? */
//    RSFormatRSType = 37,		/* handled specially */
//    RSFormatUnicharsType = 38,		/* handled specially */
//    RSFormatCharsType = 39,		/* handled specially */
//    RSFormatPascalCharsType = 40,	/* handled specially */
//    RSFormatSingleUnicharType = 41,	/* handled specially */
//    RSFormatDummyPointerType = 42	/* special case for %n */
//};

enum {
    RSFormatDefaultSize = 0,
    RSFormatSize1 = 1,
    RSFormatSize2 = 2,
    RSFormatSize4 = 3,
    RSFormatSize8 = 4,
    RSFormatSize16 = 5,
#if __LP64__
    RSFormatSizeLong = RSFormatSize8,
    RSFormatSizePointer = RSFormatSize8
#else
    RSFormatSizeLong = RSFormatSize4,
    RSFormatSizePointer = RSFormatSize4
#endif
};

typedef RSTypeID RSFormatTypeID;
typedef struct {
    int16_t type;
    int16_t size;
    union {
        int64_t int64Value;
        double doubleValue;
#if LONG_DOUBLE_SUPPORT
        long double longDoubleValue;
#endif
        void* pointerValue;
    } value;
    //char format[32];
} RSPrintValue;
#define RSFormatBuffer  400
RSIndex __RSFormatConvertHex(RSBuffer buffer, RSIndex n, BOOL captial);
RSIndex __RSFormatPrint(RSBuffer* buf, RSIndex* size, RSCUBuffer format, va_list ap);  // the buf should be freed by caller.
#endif
