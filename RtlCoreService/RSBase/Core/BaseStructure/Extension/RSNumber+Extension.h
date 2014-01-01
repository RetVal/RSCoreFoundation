//
//  RSNumber+Extension.h
//  RSCoreFoundation
//
//  Created by RetVal on 6/5/13.
//  Copyright (c) 2013 RetVal. All rights reserved.
//

#ifndef RSCoreFoundation_RSNumber_Extension_h
#define RSCoreFoundation_RSNumber_Extension_h

#include <RSCoreFoundation/RSNumber.h>

RS_EXTERN_C_BEGIN

RSExport RSNumberRef RSNumberWithChar(char value) RS_AVAILABLE(0_4);
RSExport RSNumberRef RSNumberWithUnsignedChar(unsigned char value) RS_AVAILABLE(0_4);
RSExport RSNumberRef RSNumberWithShort(short value) RS_AVAILABLE(0_3);
RSExport RSNumberRef RSNumberWithUnsignedShort(unsigned short value) RS_AVAILABLE(0_3);
RSExport RSNumberRef RSNumberWithInt(int value) RS_AVAILABLE(0_3);
RSExport RSNumberRef RSNumberWithUnsignedInt(unsigned int value) RS_AVAILABLE(0_3);
RSExport RSNumberRef RSNumberWithLong(long value) RS_AVAILABLE(0_3);
RSExport RSNumberRef RSNumberWithUnsignedLong(unsigned long value) RS_AVAILABLE(0_3);
RSExport RSNumberRef RSNumberWithFloat(float value) RS_AVAILABLE(0_3);
RSExport RSNumberRef RSNumberWithDouble(double value) RS_AVAILABLE(0_3);
RSExport RSNumberRef RSNumberWithBool(BOOL value) RS_AVAILABLE(0_3);
RSExport RSNumberRef RSNumberWithInteger(RSInteger value) RS_AVAILABLE(0_3);
RSExport RSNumberRef RSNumberWithUnsignedInteger(RSUInteger value) RS_AVAILABLE(0_3);
RSExport RSNumberRef RSNumberWithLonglong(long long value) RS_AVAILABLE(0_3);
RSExport RSNumberRef RSNumberWithUnsignedLonglong(unsigned long long value) RS_AVAILABLE(0_3);
RSExport RSNumberRef RSNumberWithRSFloat(RSFloat value) RS_AVAILABLE(0_3);
RSExport RSNumberRef RSNumberWithRange(RSRange value) RS_AVAILABLE(0_4);
RSExport RSNumberRef RSNumberWithPointer(void *value) RS_AVAILABLE(0_4);
RSExport RSNumberRef RSNumberWithString(RSStringRef value) RS_AVAILABLE(0_4);

RS_EXTERN_C_END
#endif
