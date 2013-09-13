//
//  RSFileManagerEnumSupport.h
//  RSCoreFoundation
//
//  Created by RetVal on 1/31/13.
//  Copyright (c) 2013 RetVal. All rights reserved.
//

#ifndef RSCoreFoundation_RSFileManagerEnumSupport_h
#define RSCoreFoundation_RSFileManagerEnumSupport_h
#include <RSCoreFoundation/RSFileManager.h>
#include <RSCoreFoundation/RSError.h>
#define __RSFileManagerContentsContextDoNotUseRSArray    0
typedef struct __RSFileManagerContentsContext
{
#if __RSFileManagerContentsContextDoNotUseRSArray
    RSIndex numberOfContents;
    RSIndex capacity;
    RSStringRef* spaths;
#else
    RSMutableStringRef currentPrefix;
    RSMutableArrayRef dirStack;
    RSMutableArrayRef contain;
#endif
}__RSFileManagerContentsContext;

void __RSFileManagerContentContextFree(__RSFileManagerContentsContext* context);
BOOL __RSFileManagerContentsDirectory(RSFileManagerRef fmg, RSStringRef path, __autorelease RSErrorRef* error, __RSFileManagerContentsContext* context, BOOL shouldRecursion);
#endif
