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
using namespace RSFoundation::Collection;

extern "C" int __main ();
int main(int argc, char **argv) {
    Nullable<ObjectBox<char>> x = ObjectBox<char>('1');
    if (x) {
        printf("%c\n", x.GetValue().Unbox());
    }
    Date date;
    const String* str = String::Create("str", String::Encoding::UTF8);
//    Allocator<String>::AllocatorSystemDefault.Deallocate(str);
    
    __main();
    return 0;
}