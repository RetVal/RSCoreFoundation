//
//  RSXMLParser.h
//  RSCoreFoundation
//
//  Created by RetVal on 7/8/13.
//  Copyright (c) 2013 RetVal. All rights reserved.
//

#ifndef RSCoreFoundation_RSXMLParser
#define RSCoreFoundation_RSXMLParser

#include <RSCoreFoundation/RSCoreFoundation.h>

RS_EXTERN_C_BEGIN
typedef struct __RSXMLNode *RSXMLNodeRef;

RSExport RSTypeID RSXMLNodeGetTypeID();

typedef struct __RSXMLElement *RSXMLElementRef;

RSExport RSTypeID RSXMLElementGetTypeID();

typedef struct __RSXMLDocument *RSXMLDocumentRef;

RSExport RSTypeID RSXMLDocumentGetTypeID();

RSExport RSXMLDocumentRef RSXMLDocumentCreateWithXMLData(RSAllocatorRef allocator, RSDataRef xmlData);
RSExport RSXMLDocumentRef RSXMLDocumentCreateWithContentOfFile(RSAllocatorRef allocator, RSStringRef path);

RSExport RSXMLDocumentRef RSXMLDocumentWithXMLData(RSDataRef xmlData);
RSExport RSXMLDocumentRef RSXMLDocumentWithContentOfFile(RSStringRef path);

// RSXMLDocumentRef : RSXMLNodeRef : RSTypeRef
// RSXMLElementRef  : RSXMLNodeRef : RSTypeRef

RS_EXTERN_C_END
#endif 
