//
//  RSPrivateTickTock.c
//  RSCoreFoundation
//
//  Created by RetVal on 5/10/13.
//  Copyright (c) 2013 RetVal. All rights reserved.
//

RSPrivate unsigned long long __RSTicktock()
{
#if defined(__clang__)
    #if __has_builtin(__builtin_readcyclecounter)
        return __builtin_readcyclecounter();
    #else
    
    #endif
#elif defined(__MSV__)
    return 0;
#endif
    return 0;
}
