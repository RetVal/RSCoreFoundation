//
//  main.c
//  RSRenrenCoreDemo
//
//  Created by closure on 1/3/14.
//  Copyright (c) 2014 closure. All rights reserved.
//

#include <RSRenrenCore/RSRenrenCore.h>

static int GetG_TK(char * str){
    int hash = 5381;
    
    for(int i = 0, len = strlen(str); i < len; ++i){
        printf("hash << 5 -> %d str[%d] -> %d\n", hash << 5, i, str[i]);
        hash += ((hash << 5) + (int)str[i]);
    }
    return (hash & 0x7fffffff);
}

int main(int argc, const char * argv[])
{
    RSShow(RSStringWithUTF8String(argv[3]));
    RSRenrenCoreAnalyzerRef analyzer = RSRenrenCoreAnalyzerCreate(RSAllocatorDefault, RSStringWithUTF8String(argv[1]), RSStringWithUTF8String(argv[2]), ^(RSRenrenCoreAnalyzerRef analyzer, RSDataRef data, RSURLResponseRef response, RSErrorRef error) {
        if (error) {
            RSShow(error);
            return;
        }
//        extern void dump(RSRenrenCoreAnalyzerRef);
//        dump(analyzer);
//        RSRunLoopStop(RSRunLoopGetMain());
        RSRenrenCoreAnalyzerCreateEventContentsWithUserId(analyzer, RSStringWithUTF8String(argv[3]), 200, NO, ^(RSRenrenEventRef event) {
            RSShow(event);
            
            RSRenrenEventDo(event);
            sleep(1);
        }, ^(void) {
            RSRunLoopStop(RSRunLoopGetMain());
        });
//        RSRenrenCoreAnalyzerUploadImage(analyzer, RSDataWithContentOfPath(RSFileManagerStandardizingPath(RSSTR("~/Desktop/upload.jpg"))), RSSTR("upload by RSCoreFoundation"), ^RSDictionaryRef(RSArrayRef albumList) {
//            BOOL (^PE)(RSDictionaryRef dict) = ^BOOL (RSDictionaryRef dict) {
//                return RSStringHasPrefix(RSDictionaryGetValue(dict, RSSTR("name")), RSSTR("二次元"));
//            };
//            __block RSIndex retIdx = 0;
//            RSArrayApplyBlock(albumList, RSMakeRange(0, RSArrayGetCount(albumList)), ^(const void *value, RSUInteger idx, BOOL *isStop) {
//                *isStop = PE(value) ? retIdx = idx, YES : NO;
//            });
//            return RSArrayObjectAtIndex(albumList, retIdx);
//        }, ^(RSTypeRef photo, BOOL success) {
//            RSShow(photo);
//            RSRunLoopStop(RSRunLoopGetMain());
//        });
//        RSRenrenCoreAnalyzerPublicStatus(analyzer, RSSTR("sent by RSCoreFoundation"));
//        for (RSUInteger idx = 0; idx < 20; idx++) {
//            s(analyzer, RSSTR("test"), RSAutorelease(RSDescription(RSNumberWithInteger(idx))));
//        }
//        RSRenrenCoreAnalyzerReply(analyzer, RSSTR("baga"), RSSTR("status"), RSSTR("5112882888"), RSSTR("254788288"), RSSTR("502"), RSSTR("newsfeed"));
    });
    RSRenrenCoreAnalyzerStartLogin(analyzer);
    RSRunLoopRun();
    sleep(1);
    RSRelease(analyzer);
    return 0;
}

