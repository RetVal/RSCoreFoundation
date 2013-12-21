//
//  RSStringNumberSupport.h
//  RSCoreFoundation
//
//  Created by RetVal on 2/27/13.
//  Copyright (c) 2013 RetVal. All rights reserved.
//

#ifndef RSCoreFoundation_RSStringNumberSupport_h
#define RSCoreFoundation_RSStringNumberSupport_h

#include <RSCoreFoundation/RSString.h>
RS_EXTERN_C_BEGIN
RSExport int RSStringIntValue(RSStringRef str);
RSExport RSInteger RSStringIntegerValue(RSStringRef str);
RSExport unsigned int RSStringUnsignedIntValue(RSStringRef str);
RSExport RSUInteger RSStringUnsignedIntegerValue(RSStringRef str);
RSExport long RSStringLongValue(RSStringRef str);
RSExport unsigned long RSStringUnsignedLongValue(RSStringRef str);
RSExport long long RSStringLongLongValue(RSStringRef str);
RSExport unsigned long long RSStringUnsignedLongLongValue(RSStringRef str);
RSExport BOOL RSStringBooleanValue(RSStringRef str);
RSExport float RSStringFloatValue(RSStringRef str);
RSExport double RSStringDoubleValue(RSStringRef str);
RSExport RSFloat RSStringRSFloatValue(RSStringRef str);
RS_EXTERN_C_END
#endif
