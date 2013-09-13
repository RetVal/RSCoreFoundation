//
//  RSFileWrapper.h
//  RSCoreFoundation
//
//  Created by RetVal on 3/11/13.
//  Copyright (c) 2013 RetVal. All rights reserved.
//

#ifndef RSCoreFoundation_RSFileWrapper_h
#define RSCoreFoundation_RSFileWrapper_h
RS_EXTERN_C_BEGIN
typedef const struct __RSFileWrapper* RSFileWrapperRef;
RSExport RSTypeID RSFileWrapperGetTypeID() RS_AVAILABLE(0_0);
RSExport BOOL RSFileWrapperIsRegularFile(RSFileWrapperRef file) RS_AVAILABLE(0_0);
RSExport BOOL RSFileWrapperIsDirectory(RSFileWrapperRef file) RS_AVAILABLE(0_0);
RSExport BOOL RSFileWrapperIsSymbolicLink(RSFileWrapperRef file) RS_AVAILABLE(0_0);
RS_EXTERN_C_END
#endif
