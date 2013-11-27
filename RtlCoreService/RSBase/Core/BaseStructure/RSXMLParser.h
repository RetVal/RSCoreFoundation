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

typedef enum RSXMLDocumentType {
    RSXMLDocumentTidyHTML = 1UL << 9,
    RSXMLDocumentTidyXML = 1UL << 10,
}RSXMLDocumentType;

RSExport RSXMLDocumentRef RSXMLDocumentCreateWithXMLData(RSAllocatorRef allocator, RSDataRef xmlData, RSXMLDocumentType documentType);
RSExport RSXMLDocumentRef RSXMLDocumentCreateWithContentOfFile(RSAllocatorRef allocator, RSStringRef path, RSXMLDocumentType documentType);

RSExport RSXMLDocumentRef RSXMLDocumentWithXMLData(RSDataRef xmlData, RSXMLDocumentType documentType);
RSExport RSXMLDocumentRef RSXMLDocumentWithContentOfFile(RSStringRef path, RSXMLDocumentType documentType);
RSExport RSXMLDocumentRef __RSHTML5Parser(RSDataRef xmlData, RSErrorRef *error);
// RSXMLDocumentRef : RSXMLNodeRef : RSTypeRef
// RSXMLElementRef  : RSXMLNodeRef : RSTypeRef

RS_EXTERN_C_END
#endif 
