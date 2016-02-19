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
typedef const struct __RSXMLNode *RSXMLNodeRef;

RSExport RSTypeID RSXMLNodeGetTypeID();
RSExport RSStringRef RSXMLNodeGetName(RSXMLNodeRef node);
RSExport void RSXMLNodeSetName(RSXMLNodeRef node, RSStringRef name);
RSExport RSTypeRef RSXMLNodeGetValue(RSXMLNodeRef node);
RSExport void RSXMLNodeSetObjectValue(RSXMLNodeRef node, RSTypeRef value);
RSExport RSXMLNodeRef RSXMLNodeGetParent(RSXMLNodeRef node);
RSExport RSUInteger RSXMLNodeGetNodeTypeId(RSXMLNodeRef node);
RSExport RSStringRef RSXMLNodeGetStringValue(RSXMLNodeRef node);
RSExport RSArrayRef RSXMLNodeGetChildren(RSXMLNodeRef node);

typedef const struct __RSXMLElement *RSXMLElementRef;

RSExport RSTypeID RSXMLElementGetTypeID();
RSExport void RSXMLElementAddAttribute(RSXMLElementRef element, RSXMLNodeRef attribute);
RSExport void RSXMLElementRemoveAttributeForName(RSXMLElementRef element, RSStringRef name);
RSExport RSArrayRef RSXMLElementGetAttributes(RSXMLElementRef element);
RSExport RSXMLNodeRef RSXMLElementGetAttributeForName(RSXMLElementRef element, RSStringRef name);
RSExport void RSXMLElementSetAttributes(RSXMLElementRef element, RSArrayRef attributes);
RSExport RSArrayRef RSXMLElementGetElementsForName(RSXMLElementRef element, RSStringRef name);
RSExport void RSXMLElementInsertChild(RSXMLElementRef element, RSXMLNodeRef child, RSIndex atIndex);
RSExport void RSXMLElementInsertChildren(RSXMLElementRef element, RSArrayRef children, RSIndex atIndex);
RSExport void RSXMLElementRemoveChild(RSXMLElementRef element, RSXMLNodeRef child, RSIndex atIndex);
RSExport void RSXMLElementSetChildren(RSXMLElementRef element, RSArrayRef children);
RSExport void RSXMLElementAddChild(RSXMLElementRef element, RSXMLNodeRef child);
RSExport void RSXMLElementReplaceChild(RSXMLElementRef element, RSXMLNodeRef child, RSIndex atIndex);


typedef const struct __RSXMLDocument *RSXMLDocumentRef;

RSExport RSTypeID RSXMLDocumentGetTypeID();
RSExport void RSXMLDocumentInsertChild(RSXMLDocumentRef document, RSXMLNodeRef child, RSIndex atIndex);
RSExport RSXMLElementRef RSXMLDocumentGetRootElement(RSXMLDocumentRef document);
RSExport void RSXMLDocumentSetRootElement(RSXMLDocumentRef document, RSXMLElementRef rootElement);
RSExport void RSXMLDocumentInsertChildren(RSXMLDocumentRef document, RSArrayRef children, RSIndex atIndex);
RSExport void RSXMLDocumentRemoveChild(RSXMLDocumentRef document, RSIndex atIndex);
RSExport void RSXMLDocumentSetChildren(RSXMLDocumentRef document, RSArrayRef children);
RSExport void RSXMLDocumentAddChild(RSXMLDocumentRef document, RSXMLNodeRef child);
RSExport void RSXMLDocumentReplaceChild(RSXMLDocumentRef document, RSXMLNodeRef child, RSIndex atIndex);


typedef enum RSXMLDocumentType {
    RSXMLDocumentTidyHTML = 1UL << 9,
    RSXMLDocumentTidyXML = 1UL << 10,
}RSXMLDocumentType;

RSExport RSXMLDocumentRef RSXMLDocumentCreateWithXMLData(RSAllocatorRef allocator, RSDataRef xmlData, RSXMLDocumentType documentType);
RSExport RSXMLDocumentRef RSXMLDocumentCreateWithContentOfFile(RSAllocatorRef allocator, RSStringRef path, RSXMLDocumentType documentType);

RSExport RSXMLDocumentRef RSXMLDocumentWithXMLData(RSDataRef xmlData, RSXMLDocumentType documentType);
RSExport RSXMLDocumentRef RSXMLDocumentWithContentOfFile(RSStringRef path, RSXMLDocumentType documentType);
// RSXMLDocumentRef : RSXMLNodeRef : RSTypeRef
// RSXMLElementRef  : RSXMLNodeRef : RSTypeRef

RS_EXTERN_C_END
#endif 
