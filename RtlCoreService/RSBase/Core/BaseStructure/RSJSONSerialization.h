//
//  RSJSONSerialization.h
//  RSCoreFoundation
//
//  Created by RetVal on 4/11/13.
//  Copyright (c) 2013 RetVal. All rights reserved.
//

#ifndef RSCoreFoundation_RSJSONSerialization_h
#define RSCoreFoundation_RSJSONSerialization_h

#include <RSCoreFoundation/RSArray.h>
#include <RSCoreFoundation/RSDictionary.h>
#include <RSCoreFoundation/RSData.h>
#include <RSCoreFoundation/RSDate.h>
#include <RSCoreFoundation/RSNumber.h>
#include <RSCoreFoundation/RSString.h>
#include <RSCoreFoundation/RSError.h>
RS_EXTERN_C_BEGIN
typedef RS_ENUM(RSErrorCode, RSJSONSerializationError) {
    RSJSONReadCorruptError = 10051,
};
RSExport RSDataRef RSJSONSerializationCreateData(RSAllocatorRef allocator, RSTypeRef jsonObject);
RSExport RSTypeRef RSJSONSerializationCreateWithJSONData(RSAllocatorRef allocator, RSDataRef jsonData);
RS_EXTERN_C_END
#endif
