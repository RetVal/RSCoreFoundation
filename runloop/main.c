//
//  main.c
//  runloop
//
//  Created by Closure on 11/10/13.
//  Copyright (c) 2013 RetVal. All rights reserved.
//

#include <RSCoreFoundation/RSCoreFoundation.h>
#include <RSRenrenCore/RSRenrenCore.h>

void timer(void (^fn)()) {
    RSAbsoluteTime at1 = RSAbsoluteTimeGetCurrent();
    fn();
    RSLog(RSSTR("Elapsed time %f msecs"), 1000.0 * (RSAbsoluteTimeGetCurrent() - at1));
}

void test_fn();

int main(int argc, const char * argv[])
{
    test_fn();
    return 0;
    if (4 != argc) {
        RSShow(RSSTR("RSRenrenDemo email password target-id"));
        RSShow(RSSTR("RSRenrenDemo 登陆邮箱 登陆密码 目标用户ID // 给目标用户点150个赞"));
        return -1;
    }
    RSShow(RSStringWithUTF8String(argv[3]));
    RSRenrenCoreAnalyzerRef analyzer = RSRenrenCoreAnalyzerCreate(RSAllocatorDefault, RSStringWithUTF8String(argv[1]), RSStringWithUTF8String(argv[2]), ^(RSRenrenCoreAnalyzerRef analyzer, RSDataRef data, RSURLResponseRef response, RSErrorRef error) {
        if (error) {
            RSShow(error);
            return;
        }
        RSShow(response);
        RSShow(RSSTR("login success"));
        RSRunLoopStop(RSRunLoopGetMain());
        //        extern void dump(RSRenrenCoreAnalyzerRef);
        //        dump(analyzer);
        //        RSRunLoopStop(RSRunLoopGetMain());
//            RSRenrenCoreAnalyzerCreateEventContentsWithUserId(analyzer, RSStringWithUTF8String(argv[3]), 200, YES, ^(RSRenrenEventRef event) {
//                RSRenrenEventDo(event);
//                sleep(3);
//            }, ^(void) {
//                RSRunLoopStop(RSRunLoopGetMain());
//            });
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
    });
    RSRenrenCoreAnalyzerStartLogin(analyzer);
    RSRunLoopRun();
    sleep(1);
    RSRelease(analyzer);
    return 0;
}

void test_fn() {
//    RSMutableDictionaryRef dict = RSDictionaryCreateMutable(RSAllocatorDefault, 0, RSDictionaryRSTypeContext);
//    RSMutableDictionaryRef dict2 = RSDictionaryCreateMutable(RSAllocatorDefault, 0, RSDictionaryRSTypeContext);
//    RSDictionarySetValue(dict, RSNumberWithInteger(0), dict2);
//    
//    RSShow(dict);
//    RSDictionarySetValue(dict2, RSNumberWithInteger(1), RSSTR("value"));
//    RSShow(dict);
//    
//    RSShow(RSDictionaryGetValue(RSDictionaryGetValue(dict, RSNumberWithInteger(0)), RSNumberWithInteger(1)));
//    
//    RSRelease(dict2);
//    RSRelease(dict);

    RSListRef list = RSListCreate(RSAllocatorDefault, RSNumberWithInt(0), RSNumberWithLong(1), RSNumberWithInteger(2), nil);
    
    RSMap(list, ^RSTypeRef(RSTypeRef obj) {
        RSShow(obj);
        return obj;
    });
    
    RSMap(RSReverse(list), ^RSTypeRef(RSTypeRef obj) {
        RSShow(obj);
        return obj;
    });
    
    RSRelease(list);
    return;
    
    RSListRef rst = RSMap(list, ^RSTypeRef(RSTypeRef obj) {
        return RSNumberWithInteger(RSNumberIntegerValue(obj) + 1);
    });

    RSMap(rst, ^RSTypeRef(RSTypeRef obj) {
        RSShow(obj);
        return obj;
    });
    
    RSRelease(list);
    return;
    
    
    RSMutableArrayRef coll = RSArrayCreateMutable(RSAllocatorDefault, 0);
    for (RSUInteger idx = 0; idx < 10; idx++) {
        RSArrayAddObject(coll, RSNumberWithInteger(idx));
    }
    
    RSShow(RSMap(coll, ^RSTypeRef(RSTypeRef obj) {
        return RSNumberWithInteger(RSNumberIntegerValue(obj) + 1);
    }));
    
    RSShow(RSReduce(^RSTypeRef(RSTypeRef a, RSTypeRef b) {
        return RSNumberWithInteger(RSNumberIntegerValue(a) + RSNumberIntegerValue(b));
    }, coll));
    
    RSRelease(coll);
    return;
}
