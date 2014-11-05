//
//  main.m
//  RSNoteSR
//
//  Created by closure on 9/30/14.
//  Copyright (c) 2014 closure. All rights reserved.
//

#import <Foundation/Foundation.h>
#import "RSNote.h"
#import "RSNoteManager.h"

int main(int argc, const char * argv[]) {
    @autoreleasepool {
        // insert code here...
        NSLog(@"Hello, World!");
        NSString *htmlFile = [[NSString alloc] initWithContentsOfFile:[@"~/Desktop/cszg-index.html" stringByStandardizingPath] encoding:NSUTF8StringEncoding error:nil];
        NSString *baseURLString  = @"http://book.kanunu.org/book4/10571/";
        RSNote *note = [[RSNote alloc] initWithContent:htmlFile];
        [note setBaseURL:baseURLString];
//        [[RSNoteManager defaultManager] writeNote:note];
    }
    return 0;
}
