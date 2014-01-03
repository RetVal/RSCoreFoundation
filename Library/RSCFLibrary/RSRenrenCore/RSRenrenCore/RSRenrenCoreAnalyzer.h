//
//  RSRenrenCoreAnalyzer.h
//  RSRenrenCore
//
//  Created by closure on 1/3/14.
//  Copyright (c) 2014 closure. All rights reserved.
//

#ifndef RSRenrenCore_RSRenrenCoreAnalyzer
#define RSRenrenCore_RSRenrenCoreAnalyzer

#include <RSCoreFoundation/RSCoreFoundation.h>
#include <RSRenrenCore/RSRenrenEvent.h>

RS_EXTERN_C_BEGIN

typedef struct __RSRenrenCoreAnalyzer *RSRenrenCoreAnalyzerRef;

RSExport RSTypeID RSRenrenCoreAnalyzerGetTypeID();
RSExport RSRenrenCoreAnalyzerRef RSRenrenCoreAnalyzerCreate(RSAllocatorRef allocator, RSStringRef email, RSStringRef password, void (^handler)(RSRenrenCoreAnalyzerRef analyzer, RSDataRef data, RSURLResponseRef response, RSErrorRef error));
RSExport void RSRenrenCoreAnalyzerStartLogin(RSRenrenCoreAnalyzerRef analyzer);
RSExport RSArrayRef RSRenrenCoreAnalyzerCreateLikeEvent(RSRenrenCoreAnalyzerRef analyzer, RSStringRef content, BOOL addlike);

RSExport void RSRenrenCoreAnalyzerCreateEventContentsWithUserId(RSRenrenCoreAnalyzerRef analyzer, RSStringRef userId, RSUInteger count, void (^handler)(RSRenrenEventRef event), void (^compelete)());

RSExport RSStringRef RSRenrenCoreAnalyzerGetUserId(RSRenrenCoreAnalyzerRef analyzer);
RSExport RSStringRef RSRenrenCoreAnalyzerGetEmail(RSRenrenCoreAnalyzerRef analyzer);
RSExport RSTypeRef RSRenrenCoreAnalyzerGetToken(RSRenrenCoreAnalyzerRef analyzer);
RSExport RSURLResponseRef RSRenrenCoreAnalyzerGetResponse(RSRenrenCoreAnalyzerRef analyzer);
RS_EXTERN_C_END
#endif 
