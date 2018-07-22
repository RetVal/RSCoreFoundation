//
//  RSRegularExpression.h.h
//  RSCoreFoundation
//
//  Created by closure on 12/26/13.
//  Copyright (c) 2013 RetVal. All rights reserved.
//

#ifndef RSCoreFoundation_RSRegularExpression
#define RSCoreFoundation_RSRegularExpression

#include <RSCoreFoundation/RSCoreFoundation.h>

RS_EXTERN_C_BEGIN

typedef const struct __RSRegularExpression *RSRegularExpressionRef;

RSExport RSTypeID RSRegularExpressionGetTypeID(void);

RSExport RSRegularExpressionRef RSRegularExpressionCreate(RSAllocatorRef allocator, RSStringRef expression);

RSExport RSArrayRef RSRegularExpressionApplyOnString(RSRegularExpressionRef regexExpression, RSStringRef str, __autorelease RSErrorRef *error);
RS_EXTERN_C_END
#endif 
