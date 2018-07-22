//
//  RSNumber+Extension.c
//  RSCoreFoundation
//
//  Created by RetVal on 6/5/13.
//  Copyright (c) 2013 RetVal. All rights reserved.
//

#include <RSCoreFoundation/RSNumber+Extension.h>
RSExport RSNumberRef RSNumberWithChar(char value) {
    return RSAutorelease(RSNumberCreateChar(RSAllocatorSystemDefault, value));
}

RSExport RSNumberRef RSNumberWithUnsignedChar(unsigned char value) {
    return RSAutorelease(RSNumberCreateUnsignedChar(RSAllocatorSystemDefault, value));
}

RSExport RSNumberRef RSNumberWithShort(short value)
{
    return RSAutorelease(RSNumberCreateShort(RSAllocatorSystemDefault, value));
}

RSExport RSNumberRef RSNumberWithUnsignedShort(unsigned short value)
{
    return RSAutorelease(RSNumberCreateUnsignedShort(RSAllocatorSystemDefault, value));
}


RSExport RSNumberRef RSNumberWithInt(int value)
{
    return RSAutorelease(RSNumberCreateInt(RSAllocatorSystemDefault, value));
}


RSExport RSNumberRef RSNumberWithUnsignedInt(unsigned int value)
{
    return RSAutorelease(RSNumberCreateUnsignedInt(RSAllocatorSystemDefault, value));
}


RSExport RSNumberRef RSNumberWithLong(long value)
{
    return RSAutorelease(RSNumberCreateLong(RSAllocatorSystemDefault, value));
}


RSExport RSNumberRef RSNumberWithUnsignedLong(unsigned long value)
{
    return RSAutorelease(RSNumberCreateUnsignedLong(RSAllocatorSystemDefault, value));
}


RSExport RSNumberRef RSNumberWithFloat(float value)
{
    return RSAutorelease(RSNumberCreateFloat(RSAllocatorSystemDefault, value));
}


RSExport RSNumberRef RSNumberWithDouble(double value)
{
    return RSAutorelease(RSNumberCreateDouble(RSAllocatorSystemDefault, value));
}


RSExport RSNumberRef RSNumberWithBool(BOOL value)
{
    return RSAutorelease(RSNumberCreateBoolean(RSAllocatorSystemDefault, value));
}


RSExport RSNumberRef RSNumberWithInteger(RSInteger value)
{
    return RSAutorelease(RSNumberCreateInteger(RSAllocatorSystemDefault, value));
}


RSExport RSNumberRef RSNumberWithUnsignedInteger(RSUInteger value)
{
    return RSAutorelease(RSNumberCreateUnsignedInteger(RSAllocatorSystemDefault, value));
}


RSExport RSNumberRef RSNumberWithLonglong(long long value)
{
    return RSAutorelease(RSNumberCreateLonglong(RSAllocatorSystemDefault, value));
}


RSExport RSNumberRef RSNumberWithUnsignedLonglong(unsigned long long value)
{
    return RSAutorelease(RSNumberCreateUnsignedLonglong(RSAllocatorSystemDefault, value));
}


RSExport RSNumberRef RSNumberWithRSFloat(RSFloat value)
{
    return RSAutorelease(RSNumberCreateRSFloat(RSAllocatorSystemDefault, value));
}

RSExport RSNumberRef RSNumberWithRange(RSRange value) {
    return RSAutorelease(RSNumberCreateRange(RSAllocatorSystemDefault, value));
}

RSExport RSNumberRef RSNumberWithPointer(void *value) {
    return RSAutorelease(RSNumberCreatePointer(RSAllocatorSystemDefault, value));
}

RSExport RSNumberRef RSNumberWithString(RSStringRef value) {
    return RSAutorelease(RSNumberCreateString(RSAllocatorSystemDefault, value));
}
