//
//  main.c
//  RSRenrenCoreDemo
//
//  Created by closure on 1/3/14.
//  Copyright (c) 2014 closure. All rights reserved.
//

#include <RSRenrenCore/RSRenrenCore.h>

int main(int argc, const char * argv[])
{
    if (4 != argc) {
        RSShow(RSSTR("RSRenrenDemo email password target-id"));
        return -1;
    }
    RSShow(RSStringWithUTF8String(argv[3]));
    RSRenrenCoreAnalyzerRef analyzer = RSRenrenCoreAnalyzerCreate(RSAllocatorDefault, RSStringWithUTF8String(argv[1]), RSStringWithUTF8String(argv[2]), ^(RSRenrenCoreAnalyzerRef analyzer, RSDataRef data, RSURLResponseRef response, RSErrorRef error) {
        if (error) {
            RSShow(error);
            return;
        }
        RSShow(RSSTR("login success"));
        extern void dump(RSRenrenCoreAnalyzerRef);
        dump(analyzer);
        RSRunLoopStop(RSRunLoopGetMain());
//        RSRenrenCoreAnalyzerCreateEventContentsWithUserId(analyzer, RSStringWithUTF8String(argv[3]), 150, NO, ^(RSRenrenEventRef event) {
//            RSShow(event);
//            
//            RSRenrenEventDo(event);
//            sleep(2);
//        }, ^(void) {
//            RSRunLoopStop(RSRunLoopGetMain());
//        });
    });
    RSRenrenCoreAnalyzerStartLogin(analyzer);
    RSRunLoopRun();
    sleep(1);
    RSRelease(analyzer);
    return 0;
}

