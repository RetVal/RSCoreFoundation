//
//  main.c
//  runloop
//
//  Created by Closure on 11/10/13.
//  Copyright (c) 2013 RetVal. All rights reserved.
//

#include <RSFoundation/RSFoundation.h>


using namespace RSFoundation;
using namespace RSFoundation::Basic;

int main(int argc, char **argv) {
    Nullable<ObjectBox<char>> x = ObjectBox<char>('1');
    if (x) {
        printf("%c", x.GetValue().Unbox());
    }
    Date date;
    return 0;
}