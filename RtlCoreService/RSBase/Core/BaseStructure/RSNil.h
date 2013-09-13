//
//  RSNil.h
//  RSCoreFoundation
//
//  Created by RetVal on 10/16/12.
//  Copyright (c) 2012 RetVal. All rights reserved.
//

#ifndef RSCoreFoundation_RSNil_h
#define RSCoreFoundation_RSNil_h
#include <RSCoreFoundation/RSBase.h>
RS_EXTERN_C_BEGIN
typedef const struct __RSNil* RSNilRef RS_AVAILABLE(0_0);
/*! @function RSNilCreate
 @abstract Return the RSNilRef object.
 @discussion This function return a RSNilRef object.
 @result A RSNilRef error code.
 */
RSNilRef RSNilCreate() RS_AVAILABLE(0_0);

/*! @function RSNilGetTypeID
 @abstract Return the RSTypeID type id.
 @discussion This function return the type id of RSNilRef from the runtime.
 @result A RSTypeID the type id of RSNilRef.
 */
RSExport RSTypeID RSNilGetTypeID() RS_AVAILABLE(0_0);

/**
 RSNil : the public RSNilRef instance.
 */
RSExport const RSNilRef RSNil RS_AVAILABLE(0_0);
RSExport const RSNilRef RSNill RS_AVAILABLE(0_0);
RSExport const RSNilRef RSNull RS_AVAILABLE(0_0);

RS_EXTERN_C_END
#endif
