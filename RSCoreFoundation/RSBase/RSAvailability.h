//
//  RSAvailability.h
//  RSCoreFoundation
//
//  Created by RetVal on 12/27/12.
//  Copyright (c) 2012 RetVal. All rights reserved.
//

#ifndef RSCoreFoundation_RSAvailability_h
#define RSCoreFoundation_RSAvailability_h
#include <RSCoreFoundation/RSDeploymentAvailability.h>

#ifndef RS_DEPRECATED
#define __RS_DEPRECATED(_rscorefoundation)    __RS_AVAILABLE_STARTING(_##_rscorefoundation)
#define RS_DEPRECATED(_rscorefoundation) __RS_DEPRECATED(_rscorefoundation)
#endif



#ifndef RS_AVAILABLE
#define RS_AVAILABLE(_rscorefoundation) AVAILABLE_RSCF_VERSION_##_rscorefoundation##_AND_LATER
#endif

#ifndef RS_AVAILABLE_BUT_DEPRECATED
#define RS_AVAILABLE_BUT_DEPRECATED(start,dep)    __RS_AVAILABLE_BUT_DEPRECATED(start , dep)
#endif

#ifndef RS_UNAVAILABLE
//#define __RS_UNAVAILABLE(_rscorefoundation)    __RS_UNAVAILABLE_STARTING(_##_rscorefoundation)
//#define RS_UNAVAILABLE(_rscorefoundation)  __RS_UNAVAILABLE(_rscorefoundation)
#define RS_UNAVAILABLE  RS_UNAVAILABLE_ATTRIBUTE
#endif

#endif
