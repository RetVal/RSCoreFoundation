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

RSNumberRef RSNumberAdd(RSNumberRef n1, RSNumberRef n2) {
    return RSNumberWithInteger(RSNumberIntegerValue(n1) + RSNumberIntegerValue(n2));
}

void test_fn() {
    RSClassRef cls1 = RSClassGetWithUTF8String("RSDictionary");
    RSClassRef cls2 = RSClassGetWithUTF8String("RSArray");
    RSShow(cls1);
    RSShow(cls2);
    RSShow(RSNumberWithBool(RSEqual(cls1, cls2)));
    RSShow(RSNumberWithInteger(RSGetRetainCount(cls1)));
    RSShow(RSRetain(cls1));
    RSShow(RSAutorelease(cls1));
    RSRelease(cls2);
    
    return;
    RSArrayRef lines = RSStringCreateArrayBySeparatingStrings(RSAllocatorDefault, RSStringWithContentOfPath(RSFileManagerStandardizingPath(RSSTR("~/Desktop/B.txt"))), RSSTR("\n"));
    RSDictionaryRef rst_map = RSShow(RSMap(RSDrop(lines, 2), ^RSTypeRef(RSTypeRef obj) {
        RSArrayRef id_name = RSStringCreateArrayBySeparatingStrings(RSAllocatorDefault, obj, RSSTR("\t"));
        RSDictionaryRef dict = RSDictionaryCreateWithObjectsAndOKeys(RSAllocatorDefault, RSArrayObjectAtIndex(id_name, 1), RSArrayObjectAtIndex(id_name, 0), NULL);
        RSRelease(id_name);
        return RSAutorelease(dict);
    }));
    RSRelease(lines);
    
    lines = RSStringCreateArrayBySeparatingStrings(RSAllocatorDefault, RSStringWithContentOfPath(RSFileManagerStandardizingPath(RSSTR("~/Desktop/A.txt"))), RSSTR("\n"));
    RSArrayRef rst_order_a = RSShow(RSMap(RSDrop(lines, 2), ^RSTypeRef(RSTypeRef obj) {
        RSArrayRef x = RSStringCreateArrayBySeparatingStrings(RSAllocatorDefault, obj, RSSTR("\t"));
        RSArrayRef pair = RSArrayCreate(RSAllocatorDefault, RSArrayObjectAtIndex(x, 0), RSAutorelease(RSStringCreateArrayBySeparatingStrings(RSAllocatorDefault, RSArrayLastObject(x), RSSTR(";"))), NULL);
        RSRelease(x);
        return RSAutorelease(pair);
    }));
    
    RSTypeRef result = RSShow(RSMap(rst_order_a, ^RSTypeRef(RSTypeRef obj) {
        RSArrayRef pair = (RSArrayRef)obj;
        return RSAutorelease(RSDictionaryCreateWithObjectsAndOKeys(RSAllocatorDefault, RSMap(RSArrayLastObject(pair), ^RSTypeRef(RSTypeRef obj) {
            return RSDictionaryGetValue(rst_map, obj);
        }), RSArrayObjectAtIndex(pair, 0),NULL));
    }));
    
    RSRelease(lines);
    return;
    
    
    RSMutableDictionaryRef dict = RSDictionaryCreateMutable(RSAllocatorDefault, 0, RSDictionaryRSTypeContext);
    RSDictionarySetValueForKeyPath(dict, RSSTR("a.b.c.d"), RSBooleanTrue);
    RSRelease(RSShow(dict));
    return;
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

    RSListRef list = RSListCreate(RSAllocatorDefault, RSNumberWithInt(0), RSNumberWithInteger(123), RSNumberWithLong(23), nil);
    
    RSShow(RSMap(RSFilter(list, ^BOOL(RSTypeRef x) {
        return RSNumberIntegerValue(x) > 20;
    }), ^RSTypeRef(RSTypeRef obj) {
        return RSShow(obj);
    }));
    
    RSRelease(list);
    
    return;
    
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
    
    RSShow(RSReduce(coll, ^RSTypeRef(RSTypeRef a, RSTypeRef b) {
        return RSNumberWithInteger(RSNumberIntegerValue(a) + RSNumberIntegerValue(b));
    }));
    
    RSRelease(coll);
    return;
}
