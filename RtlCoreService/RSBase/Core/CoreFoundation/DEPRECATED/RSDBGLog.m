//
//  RSDBGLog.m
//  RSKit
//
//  Created by RetVal on 10/13/12.
//  Copyright (c) 2012 RetVal. All rights reserved.
//

#include <stdio.h>
#include <RSKit/RSBase.h>
RSExport void RSDBGLog(NSString* format,...)
{
#if DEBUG
    if(format)
    {
        va_list args;
        va_start(args,format);
        NSString *str = [[NSString alloc] initWithFormat:format arguments:args];
        va_end(args);
        NSLog(@"%@",str);
    }
#endif
    return;
}