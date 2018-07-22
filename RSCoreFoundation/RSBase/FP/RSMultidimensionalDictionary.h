//
//  RSMultidimensionalDictionary.h
//  RSCoreFoundation
//
//  Created by closure on 2/2/14.
//  Copyright (c) 2014 RetVal. All rights reserved.
//

#ifndef RSCoreFoundation_RSMultidimensionalDictionary
#define RSCoreFoundation_RSMultidimensionalDictionary

#include <RSCoreFoundation/RSCoreFoundation.h>

RS_EXTERN_C_BEGIN

typedef struct __RSMultidimensionalDictionary *RSMultidimensionalDictionaryRef;

RSExport RSTypeID RSMultidimensionalDictionaryGetTypeID(void);

RSExport RSMultidimensionalDictionaryRef RSMultidimensionalDictionaryCreate(RSAllocatorRef allocator, RSUInteger dimension);
RSExport RSUInteger RSMultidimensionalDictionaryGetDimension(RSMultidimensionalDictionaryRef dict);
RSExport RSDictionaryRef RSMultidimensionalDictionaryGetDimensionEntry(RSMultidimensionalDictionaryRef dict);

RSExport RSTypeRef RSMultidimensionalDictionaryGetValue(RSMultidimensionalDictionaryRef dict, ...);
RSExport RSTypeRef RSMultidimensionalDictionaryGetValueWithKeyPaths(RSMultidimensionalDictionaryRef dict, RSStringRef keyPath);
RSExport RSTypeRef RSMultidimensionalDictionaryGetValueWithKeys(RSMultidimensionalDictionaryRef dict, RSArrayRef keys);

RSExport void RSMultidimensionalDictionarySetValue(RSMultidimensionalDictionaryRef dict, RSTypeRef value, ...);
RSExport void RSMultidimensionalDictionarySetValueWithKeyPaths(RSMultidimensionalDictionaryRef dict, RSTypeRef value, RSStringRef keyPath);
RSExport void RSMultidimensionalDictionarySetValueWithKeys(RSMultidimensionalDictionaryRef dict, RSTypeRef value, RSArrayRef keys);
RS_EXTERN_C_END
#endif 
