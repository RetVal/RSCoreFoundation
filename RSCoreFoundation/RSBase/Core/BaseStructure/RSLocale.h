//
//  RSLocale.h
//  RSCoreFoundation
//
//  Created by RetVal on 8/4/13.
//  Copyright (c) 2013 RetVal. All rights reserved.
//

#ifndef RSCoreFoundation_RSLocale
#define RSCoreFoundation_RSLocale

#include <RSCoreFoundation/RSBase.h>
#include <RSCoreFoundation/RSArray.h>
#include <RSCoreFoundation/RSString.h>

RS_EXTERN_C_BEGIN

typedef const struct __RSLocale *RSLocaleRef;

RSExport RSTypeID RSLocaleGetTypeID(void);
RSExport RSLocaleRef RSLocaleCopyCurrent(void);
RSExport RSLocaleRef RSLocaleCreate(RSAllocatorRef allocator, RSStringRef identifier);
RSExport RSStringRef RSLocaleGetIdentifier(RSLocaleRef locale);
RSExport RSTypeRef RSLocaleGetValue(RSLocaleRef locale, RSStringRef key);

typedef RS_ENUM(RSIndex, RSLocaleLanguageDirection) {
    RSLocaleLanguageDirectionUnknown = 0,
    RSLocaleLanguageDirectionLeftToRight = 1,
    RSLocaleLanguageDirectionRightToLeft = 2,
    RSLocaleLanguageDirectionTopToBottom = 3,
    RSLocaleLanguageDirectionBottomToTop = 4
};
RS_EXTERN_C_END
#endif 
