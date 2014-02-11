//
//  RSFunctional.h
//  RSCoreFoundation
//
//  Created by closure on 1/29/14.
//  Copyright (c) 2014 RetVal. All rights reserved.
//

#ifndef RSCoreFoundation_RSFunctional_h
#define RSCoreFoundation_RSFunctional_h

RS_EXTERN_C_BEGIN
#include <RSCoreFoundation/RSCoreFoundation.h>
#include <RSCoreFoundation/RSList.h>
#include <RSCoreFoundation/RSMultidimensionalDictionary.h>

typedef RSTypeRef RSCollectionRef;

RSExport RSCollectionRef RSMap(RSCollectionRef coll, RSTypeRef (^fn)(RSTypeRef obj));
RSExport RSTypeRef RSReduce(RSTypeRef (^fn)(RSTypeRef a, RSTypeRef b), RSCollectionRef coll);

RSExport RSCollectionRef RSNext(RSCollectionRef coll);
RSExport RSTypeRef RSFirst(RSCollectionRef coll);
RSExport RSTypeRef RSSecond(RSCollectionRef coll);

RSExport RSCollectionRef RSConjoin(RSCollectionRef coll, RSTypeRef value);
RSExport RSCollectionRef RSReverse(RSCollectionRef coll);
RS_EXTERN_C_END

#endif
