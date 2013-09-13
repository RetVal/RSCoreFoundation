//
//  RSArchiverRoutine.h
//  RSCoreFoundation
//
//  Created by RetVal on 9/12/13.
//  Copyright (c) 2013 RetVal. All rights reserved.
//

#ifndef RSCoreFoundation_RSArchiverRoutine_h
#define RSCoreFoundation_RSArchiverRoutine_h

#include <RSCoreFoundation/RSBase.h>
#include <RSCoreFoundation/RSArchiver.h>
RS_EXTERN_C_BEGIN
RSExport BOOL RSArchiverRegisterCallbacks(const RSArchiverCallBacks * callbacks) RS_AVAILABLE(0_0);

RSExport void RSArchiverUnregisterCallbacks(RSTypeID classId) RS_AVAILABLE(0_0);
RS_EXTERN_C_END
#endif
