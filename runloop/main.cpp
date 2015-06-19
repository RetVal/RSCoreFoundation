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
    
//    std::auto_ptr<String> ptr(new String());
    
    
    auto allocator = &Allocator<String>::SystemDefault;
    const String* str = String::Create("str", String::Encoding::UTF8);
    allocator->Deallocate(str);
//    Index length = str->GetLength();
    const String* copy = str->Copy();
    allocator->Deallocate(copy);
//    assert(length == copy->GetLength());
//    const String* copy2 = copy->Copy();
//    assert(length == copy2->GetLength());
    
    return 0;
}