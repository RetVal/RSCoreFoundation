//
//  RSPathUtilities.h
//  RSCoreFoundation
//
//  Created by RetVal on 6/5/13.
//  Copyright (c) 2013 RetVal. All rights reserved.
//

#ifndef RSCoreFoundation_RSPathUtilities_h
#define RSCoreFoundation_RSPathUtilities_h

#include <RSCoreFoundation/RSFileManager.h>

RSExport RSStringRef RSTemporaryPath(RSStringRef prefix, RSStringRef suffix) RS_AVAILABLE(0_3);
RSExport RSStringRef RSTemporaryDirectory(void) RS_AVAILABLE(0_3);
#endif
