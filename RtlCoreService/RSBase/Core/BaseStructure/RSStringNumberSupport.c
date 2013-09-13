//
//  RSStringNumberSupport.c
//  RSCoreFoundation
//
//  Created by RetVal on 2/27/13.
//  Copyright (c) 2013 RetVal. All rights reserved.
//

#include <RSCoreFoundation/RSStringNumberSupport.h>

#define __RSStringOSEncoding __RSDefaultEightBitStringEncoding

RSExport int RSStringIntValue(RSStringRef str)
{
    int _num = 0;
    if (str)
    {
        const char *ptr = RSStringGetCStringPtr(str, __RSStringOSEncoding);
        _num = ptr ? atoi(ptr) : 0;
    }
    return _num;
}

RSExport unsigned int RSStringUnsigendIntValue(RSStringRef str)
{
    unsigned int _num = 0;
    if (str)
    {
        const char *ptr = RSStringGetCStringPtr(str, __RSStringOSEncoding);
        _num = ptr ? atoi(ptr) : 0;
    }
    return _num;
}

RSExport RSInteger RSStringIntegerValue(RSStringRef str)
{
#if __LP64__
    return RSStringLongValue(str);
#else
    return RSStringIntValue(str);
#endif
    return 0;
}

RSExport RSUInteger RSStringUnsignedIntegerValue(RSStringRef str)
{
#if __LP64__
    return RSStringUnsignedLongValue(str);
#else
    return RSStringUnsigendIntValue(str);
#endif
    return 0;
}

RSExport long RSStringLongValue(RSStringRef str)
{
    long _num = 0;
    if (str)
    {
        const char *ptr = RSStringGetCStringPtr(str, __RSStringOSEncoding);
        _num = ptr ? strtol(ptr, nil, 10) : 0;
    }
    return _num;
}

RSExport unsigned long RSStringUnsignedLongValue(RSStringRef str)
{
    unsigned long _num = 0;
    if (str)
    {
        const char *ptr = RSStringGetCStringPtr(str, __RSStringOSEncoding);
        _num = ptr ? strtoul(ptr, nil, 10) : 0;
    }
    return _num;
}

RSExport long long RSStringLongLongValue(RSStringRef str)
{
    long long _num = 0;
    if (str)
    {
        const char *ptr = RSStringGetCStringPtr(str, __RSStringOSEncoding);
        _num = ptr ? strtoll(ptr, nil, 10) : 0;
    }
    return _num;
}

RSExport unsigned long long RSStringUnsignedLongLongValue(RSStringRef str)
{
    unsigned long long _num = 0;
    if (str)
    {
        const char *ptr = RSStringGetCStringPtr(str, __RSStringOSEncoding);
        _num = ptr ? strtoull(ptr, nil, 10) : 0;
    }
    return _num;
}

RSExport BOOL RSStringBooleanValue(RSStringRef str)
{
    BOOL _boolean = -1;
    if (str)
    {
        do
        {
            if (RSEqual(str, RSSTR("YES")))
            {
                _boolean = YES;
                break;
            }
            if (RSEqual(str, RSSTR("yes")))
            {
                _boolean = YES;
                break;
            }
            if (RSEqual(str, RSSTR("NO")))
            {
                _boolean = NO;
                break;
            }
            if (RSEqual(str, RSSTR("no")))
            {
                _boolean = NO;
                break;
            }
            const char *ptr = RSStringGetCStringPtr(str, __RSStringOSEncoding);
            _boolean = ptr ? (atol(ptr) ? YES : NO) : -1;
        } while (0);
    }
    return _boolean == -1 ? NO : _boolean;
}

RSExport float RSStringFloatValue(RSStringRef str)
{
    double _num = 0.0f;
    if (str)
    {
        const char *ptr = RSStringGetCStringPtr(str, __RSStringOSEncoding);
        _num = ptr ? strtof(ptr, nil) : 0.0f;
    }
    return _num;
}

RSExport double RSStringDoubleValue(RSStringRef str)
{
    double _num = 0.0f;
    if (str)
    {
        const char *ptr = RSStringGetCStringPtr(str, __RSStringOSEncoding);
        _num = ptr ? strtod(ptr, nil) : 0.0f;
    }
    return _num;
}

RSExport RSFloat RSStringRSFloatValue(RSStringRef str)
{
#if __LP64__
    return RSStringDoubleValue(str);
#else
    return RSStringFloatValue(str);
#endif
    return 0.0f;
}
