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
#include <RSCoreFoundation/RSKVBucket.h>

typedef RSTypeRef RSCollectionRef;

// RSList, RSArray
RSExport RSTypeRef RSApply(RSCollectionRef coll, void (^fn)(RSTypeRef obj));
RSExport RSTypeRef RSApplyWithRange(RSCollectionRef coll, RSRange range, void (^fn)(RSTypeRef obj));

RSExport RSCollectionRef RSMap(RSCollectionRef coll, RSTypeRef (^fn)(RSTypeRef obj));
RSExport RSCollectionRef RSMapWithRange(RSCollectionRef coll, RSRange range, RSTypeRef (^fn)(RSTypeRef obj));

RSExport RSTypeRef RSReduce(RSCollectionRef coll, RSTypeRef (^fn)(RSTypeRef a, RSTypeRef b));
RSExport RSTypeRef RSReduceWithRange(RSCollectionRef coll, RSRange range, RSTypeRef (^fn)(RSTypeRef a, RSTypeRef b));

RSExport RSCollectionRef RSNext(RSCollectionRef coll);
RSExport RSTypeRef RSFirst(RSCollectionRef coll);
RSExport RSTypeRef RSSecond(RSCollectionRef coll);
RSExport RSTypeRef RSNth(RSCollectionRef coll, RSIndex idx);

RSExport RSCollectionRef RSConjoin(RSCollectionRef coll, RSTypeRef value);
RSExport RSCollectionRef RSReverse(RSCollectionRef coll);

RSExport RSCollectionRef RSFilter(RSCollectionRef coll, BOOL (^pred)(RSTypeRef x));
RSExport RSCollectionRef RSDrop(RSCollectionRef coll, RSIndex n);

RSExport RSCollectionRef RSMerge(RSCollectionRef a, RSCollectionRef b, ...);

RSExport RSIndex RSCollPred(RSTypeRef coll);
RSExport RSIndex RSCount(RSTypeRef coll);
RS_EXTERN_C_END

#endif
