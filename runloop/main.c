//
//  main.c
//  runloop
//
//  Created by Closure on 11/10/13.
//  Copyright (c) 2013 RetVal. All rights reserved.
//

#include <RSCoreFoundation/RSCoreFoundation.h>
#include "RSClosure.h"

void timer(void (^fn)()) {
    RSAbsoluteTime at1 = RSAbsoluteTimeGetCurrent();
    fn();
    RSLog(RSSTR("Elapsed time %f msecs"), 1000.0 * (RSAbsoluteTimeGetCurrent() - at1));
}

void test_fn();

int main(int argc, const char * argv[]) {
    demo();
}
