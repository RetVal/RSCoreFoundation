//
//  RSClosure.h
//  RSCoreFoundation
//
//  Created by closure on 9/18/14.
//  Copyright (c) 2014 RetVal. All rights reserved.
//

#ifndef RSCoreFoundation_RSClosure
#define RSCoreFoundation_RSClosure

#include <RSCoreFoundation/RSCoreFoundation.h>

RS_EXTERN_C_BEGIN

typedef const struct __RSClosure *RSClosureRef;

RSExport RSTypeID RSClosureGetTypeID();
void demo();
RS_EXTERN_C_END
#endif 
