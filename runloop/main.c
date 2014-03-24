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
//    RSStringRef content = RSStringWithContentOfPath(RSFileManagerStandardizingPath(RSSTR("~/Desktop/leak.txt")));
//    RSArrayRef lines = RSStringCreateComponentsSeparatedByStrings(RSAllocatorDefault, content, RSSTR("\n"));
//    RSStringRef prefix = RSSTR("RSRuntime debug notice : ");
//    RSArrayRef filterResult = RSFilter(lines, ^BOOL(RSTypeRef x) {
//        return RSStringHasPrefix(x, prefix);
//    });
//    RSRelease(lines);
//    
//    RSShow(RSNumberWithInteger(RSCount(filterResult)));
//    
//    RSArrayRef map = RSMap(filterResult, ^RSTypeRef(RSTypeRef obj) {
//        return RSDrop(RSAutorelease(RSStringCreateComponentsSeparatedByStrings(RSAllocatorDefault, obj, RSSTR(" "))), 4);
//    });
//    
//    RSArrayRef alloc = RSFilter(map, ^BOOL(RSTypeRef x) {
//        return RSEqual(RSSTR("alloc"), RSArrayObjectAtIndex(x, 1));
//    });
//    
//    RSShow(RSNumberWithInteger(RSCount(alloc)));
//    
//    RSArrayRef dealloc = RSFilter(map, ^BOOL(RSTypeRef x) {
//        return RSEqual(RSSTR("dealloc"), RSArrayObjectAtIndex(x, 1));
//    });
//    
//    RSShow(RSNumberWithInteger(RSCount(dealloc)));
//    
//    RSMultidimensionalDictionaryRef allocDict = RSMultidimensionalDictionaryCreate(RSAllocatorDefault, 2);
//    RSArrayApplyBlock(alloc, RSMakeRange(0, RSArrayGetCount(alloc)), ^(const void *value, RSUInteger idx, BOOL *isStop) {
//        RSStringRef key = RSArrayObjectAtIndex(value, 0);
//        RSStringRef address = RSArrayLastObject(value);
//        RSMutableArrayRef store = (RSMutableArrayRef)RSMultidimensionalDictionaryGetValue(allocDict, key, RSSTR("alloc"));
//        if (!store) {
//            store = RSArrayCreateMutable(RSAllocatorDefault, 0);
//            RSMultidimensionalDictionarySetValue(allocDict, store, key, RSSTR("alloc"));
//            RSRelease(store);
//        }
//        RSArrayAddObject(store, address);
//    });
//    
//    RSArrayApplyBlock(dealloc, RSMakeRange(0, RSArrayGetCount(dealloc)), ^(const void *value, RSUInteger idx, BOOL *isStop) {
//        if (RSEqual(RSArrayObjectAtIndex(value, 0), RSSTR("RSSet"))) {
//            RSShow(RSSTR("hit"));
//        }
//        RSStringRef key = RSArrayObjectAtIndex(value, 0);
//        RSStringRef address = RSArrayLastObject(value);
//        RSMutableArrayRef store = (RSMutableArrayRef)RSMultidimensionalDictionaryGetValue(allocDict, key, RSSTR("alloc"));
//        if (!store) {
//            store = (RSMutableArrayRef)RSMultidimensionalDictionaryGetValue(allocDict, key, RSSTR("dealloc"));
//            if (!store) {
//                store = RSArrayCreateMutable(RSAllocatorDefault, 0);
//                RSMultidimensionalDictionarySetValue(allocDict, store, key, RSSTR("dealloc"));
//                RSRelease(store);
//            }
//            RSArrayAddObject(store, address);
//        } else {
//            if (RSArrayIndexOfObject(store, address) != RSNotFound) {
//                RSArrayRemoveObject(store, address);
//                if (!RSArrayGetCount(store)) {
//                    RSMultidimensionalDictionarySetValue(allocDict, nil, key, RSSTR("alloc"));
//                }
//            } else {
//                store = (RSMutableArrayRef)RSMultidimensionalDictionaryGetValue(allocDict, key, RSSTR("dealloc"));
//                if (!store) {
//                    store = RSArrayCreateMutable(RSAllocatorDefault, 0);
//                    RSMultidimensionalDictionarySetValue(allocDict, store, key, RSSTR("dealloc"));
//                    RSRelease(store);
//                } else {
//                    RSArrayAddObject(store, address);
//                }
//            }
//        }
//    });
//    
//    RSMutableArrayRef dictDeallocStore = (RSMutableArrayRef)RSMultidimensionalDictionaryGetValueWithKeyPaths(allocDict, RSSTR("RSDictionary.dealloc"));
//    RSMutableArrayRef basicHashAllocStore = (RSMutableArrayRef)RSMultidimensionalDictionaryGetValueWithKeyPaths(allocDict, RSSTR("RSBasicHash.alloc"));
//    RSMutableArrayRef rst = RSArrayCreateMutable(RSAllocatorDefault, 0);
//    RSArrayApplyBlock(basicHashAllocStore, RSMakeRange(0, RSArrayGetCount(basicHashAllocStore)), ^(const void *value, RSUInteger idx, BOOL *isStop) {
//        if (RSNotFound == RSArrayIndexOfObject(dictDeallocStore, value)) {
//            RSArrayAddObject(rst, value);
//        }
//    });
//    RSArrayWriteToFile(rst, RSFileManagerStandardizingPath(RSSTR("~/Desktop/leak2.plist")), RSWriteFileAutomatically);
//    RSRelease(rst);
//    RSDictionaryWriteToFile(RSMultidimensionalDictionaryGetDimensionEntry(allocDict), RSFileManagerStandardizingPath(RSSTR("~/Desktop/leak.plist")), RSWriteFileAutomatically);
//    RSRelease(allocDict);
//    return 0;
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

#include "../../Risp/Risp/BIF/RSNumberOperation.h"
#include "../../Risp/Risp/BIF/RSNumberOperation.c"
#include <dispatch/dispatch.h>
void test_fn() {
    if (1) {
        RSURLConnectionSendAsynchronousRequest(RSURLRequestWithURL(RSURLWithString(RSSTR("http://localhost"))), nil, ^(RSURLResponseRef response, RSDataRef data, RSErrorRef error) {
            RSShow(response);
            RSRunLoopStop(RSRunLoopGetMain());
        });
        RSRunLoopRun();
        RSShow(RSSTR("么么哒"));
        return;
    }
    if (0) {
        RSShow(RSReduceWithRange(RSAutorelease(RSListCreate(RSAllocatorDefault, RSNumberWithInt(1), RSNumberWithInt(2), RSNumberWithInt(3), nil)), RSMakeRange(1, 2), ^RSTypeRef(RSTypeRef a, RSTypeRef b) {
            RSShow(a);
            RSShow(b);
            return a;
        }));
        return;
    }
    if (1) {
        RSArrayRef range = RSPerformBlockConcurrentCopyResults(400000, ^RSTypeRef(RSIndex idx) {
            return RSNumberWithInteger(idx);
        });
        RSURLRequestRef request = RSURLRequestWithURL(RSURLWithString(RSSTR("http://www.taobao.com")));
        dispatch_group_t g = dispatch_group_create();
//        __block RSSpinLock lock = RSSpinLockInit;
        RSArrayApplyBlock(range, RSMakeRange(0, RSArrayGetCount(range)), ^(const void *value, RSUInteger idx, BOOL *isStop) {
            dispatch_group_async(g, dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_HIGH, 2), ^{
                RSURLResponseRef response = nil;
                RSErrorRef error = nil;
                RSDataRef data = RSURLConnectionSendSynchronousRequest(request, &response, &error);
                if (data) {
                    RSShow(response);
                } else {
                    RSShow(error);
                }
            });
        });
        dispatch_group_wait(g, DISPATCH_TIME_FOREVER);
//        RSArrayWriteToFile(result, RSFileManagerStandardizingPath(RSSTR("~/Desktop/result.plist")), RSWriteFileAutomatically);
        return;
    }
    if (0) {
        RSListRef list = RSShow(RSAutorelease(RSListCreateWithArray(RSAllocatorDefault, RSAutorelease(RSArrayCreate(RSAllocatorDefault, RSNumberWithInt(0),RSNumberWithInt(1),RSNumberWithInt(2),RSNumberWithInt(3),RSNumberWithInt(4),RSNumberWithInt(5),RSNumberWithInt(6),RSNumberWithInt(7),RSNumberWithInt(8),RSNumberWithInt(9) ,NULL)))));
        RSListRef new = RSMap(list, ^RSTypeRef(RSTypeRef obj) {
            return numberAdd(obj, RSNumberWithInt(1));
        });
        RSShow(new);
        return;
    }
    if (0) {
        
        // 2014/02/13 23:00:02 [error] 20572#0: *598658985 recv() failed (104: Connection reset by peer) while reading response header from upstream, client: 60.49.107.153, server: api.zank.mobi, request: "POST /snowball/api/client/devicePush/bindDevicetoken.json HTTP/1.1", upstream: "fastcgi://unix:/tmp/php-fpm.sock:", host: "api.zank.mobi"
        RSArrayRef content = RSAutorelease(RSStringCreateComponentsSeparatedByStrings(RSAllocatorDefault, RSStringWithContentOfPath(RSFileManagerStandardizingPath(RSSTR("~/Desktop/error.2014-02-17.txt"))), RSSTR("\n")));
        RSMutableArrayRef rst = RSArrayCreateMutable(RSAllocatorDefault, 0);
        RSCollectionRef coll = RSMap(content, ^RSTypeRef(RSTypeRef obj) {
            RSArrayRef line = RSStringCreateComponentsSeparatedByStrings(RSAllocatorDefault, obj, RSSTR(" "));
            RSArrayRef r = RSArrayCreate(RSAllocatorDefault, RSArrayObjectAtIndex(line, 0), RSArrayObjectAtIndex(line, 1), RSArrayObjectAtIndex(line, RSArrayGetCount(line) - 6), NULL);
            RSRelease(line);
            return RSAutorelease(r);
        });
        RSRelease(rst);
        RSMutableDictionaryRef dict = RSDictionaryCreateMutable(RSAllocatorDefault, 0, RSDictionaryRSTypeContext);
        RSCollectionRef api = RSFilter(RSMap(coll, ^RSTypeRef(RSTypeRef obj) {
            return RSArrayLastObject(RSAutorelease(RSStringCreateComponentsSeparatedByStrings(RSAllocatorDefault, RSArrayObjectAtIndex(RSAutorelease(RSStringCreateComponentsSeparatedByStrings(RSAllocatorDefault, RSArrayLastObject(obj), RSSTR("?"))), 0), RSSTR("/"))));
        }), ^BOOL(RSTypeRef x) {
            return RSStringHasSuffix(x, RSSTR(".json"));
        });
        RSArrayApplyBlock(api, RSMakeRange(0, RSArrayGetCount(api)), ^(const void *value, RSUInteger idx, BOOL *isStop) {
            RSNumberRef n = RSDictionaryGetValue(dict, value);
            if (!n) {
                n = RSNumberWithInteger(1);
            } else {
                n = RSNumberWithInteger(RSNumberIntegerValue(n) + 1);
            }
            RSDictionarySetValue(dict, value, n);
        });
        RSDictionaryWriteToFile(dict, RSFileManagerStandardizingPath(RSSTR("~/Desktop/error.2014-02-17.plist")), RSWriteFileAutomatically);
        RSRelease(dict);
        return;
    }
    if (0) {
        RSArrayRef content = RSArrayWithContentOfPath(RSFileManagerStandardizingPath(RSSTR("~/Desktop/log.1.plist")));
        RSCollectionRef rst = RSFilter(content, ^BOOL(RSTypeRef x) {
            return RSStringIntegerValue(RSArrayObjectAtIndex(x, 5)) >= 500;
        });
        RSStringRef (^transform)(RSStringRef url) = ^RSStringRef (RSStringRef url) {
            return RSArrayObjectAtIndex(RSAutorelease(RSStringCreateComponentsSeparatedByStrings(RSAllocatorDefault, url, RSSTR("?"))), 0);
        };
        RSMutableDictionaryRef dict = RSDictionaryCreateMutable(RSAllocatorDefault, 0, RSDictionaryRSTypeContext);
        rst = RSMap(rst, ^RSTypeRef(RSTypeRef obj) {
            return transform(RSArrayObjectAtIndex(obj, 3)); //RSAutorelease(RSArrayCreate(RSAllocatorDefault, RSArrayObjectAtIndex(obj, 0), , NULL));
        });
        RSArrayApplyBlock(rst, RSMakeRange(0, RSArrayGetCount(rst)), ^(const void *value, RSUInteger idx, BOOL *isStop) {
//            RSArrayRef keys = RSAutorelease(RSArrayCreate(RSAllocatorDefault, key1, key2, transform(value), NULL));
            
            RSNumberRef n = RSDictionaryGetValue(dict, value);
            if (!n) {
                n = RSNumberWithInteger(1);
            } else {
                n = RSNumberWithInteger(RSNumberIntegerValue(n) + 1);
            }
            RSDictionarySetValue(dict, value, n);
        });
        RSDictionaryWriteToFile(dict, RSFileManagerStandardizingPath(RSSTR("~/Desktop/log.2.plist")), RSWriteFileAutomatically);
        RSRelease(dict);
        return;
    }
    
    if (1) {
        RSStringRef content = RSStringWithContentOfPath(RSFileManagerStandardizingPath(RSSTR("~/Desktop/log.txt")));
        RSArrayRef lines = RSAutorelease(RSStringCreateComponentsSeparatedByStrings(RSAllocatorDefault, content, RSSTR("\n")));
        RSArrayRef rst_1 = RSMap(lines, ^RSTypeRef(RSTypeRef obj) {
            RSMutableArrayRef parts = RSMutableCopy(RSAllocatorDefault, RSAutorelease(RSStringCreateComponentsSeparatedByStrings(RSAllocatorDefault, obj, RSSTR(" "))));
            RSArrayRemoveObjectAtIndex(parts, 1);
            RSArrayRemoveObjectAtIndex(parts, 1);
            RSArrayRemoveObjectAtIndex(parts, 2);
            RSArrayRemoveLastObject(parts);
            RSMutableStringRef str = RSMutableCopy(RSAllocatorDefault, RSArrayObjectAtIndex(parts, 1));
            RSStringDelete(str, RSMakeRange(0, 1));
            RSArraySetObjectAtIndex(parts, 1, str);
            RSRelease(str);
            return RSAutorelease(parts);
        });
        
        rst_1 = RSFilter(RSFilter(rst_1, ^BOOL(RSTypeRef x) {
            return !RSStringHasPrefix(RSArrayObjectAtIndex(x, 3), RSSTR("/snowball/api/user/user/updateUserStatus.json"));
        }), ^BOOL(RSTypeRef x) {
            return RSStringIntegerValue(RSArrayObjectAtIndex(x, 5)) == 500;
        });
        RSArrayWriteToFile(rst_1, RSFileManagerStandardizingPath(RSSTR("~/Desktop/log.3.plist")), RSWriteFileAutomatically);
        return;
    }
    if (0) {
        RSShow(RSSTR("begin"));
        RSDictionaryRef log = RSDictionaryWithContentOfPath(RSFileManagerStandardizingPath(RSSTR("~/Desktop/function-user-2.plist")));
        __block RSNumberRef sum = RSNumberWithInteger(0);
        RSDictionaryApplyBlock(log, ^(const void *key, const void *value, BOOL *stop) {
            RSDictionaryApplyBlock(value, ^(const void *key, const void *value, BOOL *stop) {
                RSDictionaryApplyBlock(value, ^(const void *key, const void *value, BOOL *stop) {
                    sum = RSNumberWithInteger(RSNumberIntegerValue(sum) + RSNumberIntegerValue(value));
                });
            });
        });
        RSShow(sum);
        RSShow(RSSTR("end"));
        return;
    }
    if (0) {
        RSDictionaryRef log = RSDictionaryWithContentOfPath(RSFileManagerStandardizingPath(RSSTR("~/Desktop/function-user-1.plist")));
        RSMutableDictionaryRef result = RSDictionaryCreateMutable(RSAllocatorDefault, 0, RSDictionaryRSTypeContext);
        RSStringRef (^transform)(RSStringRef x) = ^RSStringRef (RSStringRef x) {
            RSMutableStringRef str = RSMutableCopy(RSAllocatorDefault, x);
            RSStringDelete(str, RSMakeRange(RSStringGetLength(x) - 3, 3));
            return RSAutorelease(str);
        };
        RSDictionaryApplyBlock(log, ^(const void *key1, const void *value, BOOL *stop) {
            RSDictionaryApplyBlock(value, ^(const void *key2, const void *value2, BOOL *stop) {
                RSArrayApplyBlock(value2, RSMakeRange(0, RSArrayGetCount(value2)), ^(const void *value, RSUInteger idx, BOOL *isStop) {
                    RSArrayRef keys = RSAutorelease(RSArrayCreate(RSAllocatorDefault, key1, key2, transform(value), NULL));
                    RSNumberRef n = RSDictionaryGetValueForKeys(result, keys);
                    if (!n) {
                        n = RSNumberWithInteger(1);
                    } else {
                        n = RSNumberWithInteger(RSNumberIntegerValue(n) + 1);
                    }
                    RSDictionarySetValueForKeys(result, keys, n);
                });
            });
        });
        RSDictionaryWriteToFile(result, RSFileManagerStandardizingPath(RSSTR("~/Desktop/function-user-2.plist")), RSWriteFileAutomatically);
        RSRelease(result);
        return;
    }
    if (0) {
        //    [13-Feb-2014 01:07:46 Asia/Shanghai] PHP Fatal error:  Call to a member function query() on a non-object in /feizan/vhost/zank.mobi/api/snowball/source/function_user.php on line 1130
        RSArrayRef lines = RSAutorelease(RSStringCreateComponentsSeparatedByStrings(RSAllocatorDefault, RSStringWithContentOfPath(RSFileManagerStandardizingPath(RSSTR("~/Desktop/1.log"))), RSSTR("\n")));
        RSArrayRef rst = RSMap(lines, ^RSTypeRef(RSTypeRef obj) {
            RSArrayRef parts = RSStringCreateComponentsSeparatedByStrings(RSAllocatorDefault, obj, RSSTR(" "));
            return RSAutorelease(parts);
        });
        rst = (RSMap(rst, ^RSTypeRef(RSTypeRef obj) {
            return RSAutorelease(RSArrayCreate(RSAllocatorDefault, RSArrayObjectAtIndex(obj, 1), RSArrayObjectAtIndex(obj, 17), RSArrayLastObject(obj), NULL));
        }));
        
        RSMutableDictionaryRef result = RSDictionaryCreateMutable(RSAllocatorDefault, 0, RSDictionaryRSTypeContext);
        RSMap(RSFilter(rst, ^BOOL(RSTypeRef x) {
            return RSArrayGetCount(x) > 2;
        }), ^RSTypeRef(RSTypeRef value) {
            RSMutableArrayRef array = (RSMutableArrayRef)RSDictionaryGetValueForKeys(result, RSDrop(value, 1));
            if (!array) {
                array = RSArrayCreateMutable(RSAllocatorDefault, 0);
                RSDictionarySetValueForKeys(result, RSDrop(value, 1), array);
                RSRelease(array);
            }
            RSArrayAddObject(array, RSArrayObjectAtIndex(value, 0));
            return RSNil;
        });
        
        RSDictionaryWriteToFile(result, RSFileManagerStandardizingPath(RSSTR("~/Desktop/function-user-1.plist")), RSWriteFileAutomatically);
        RSRelease(result);
    }
    return ;
    if (1) {
        RSListRef list = RSListCreate(RSAllocatorDefault, RSNumberWithInt(0), RSNumberWithInt(1), RSNumberWithInt(2), RSNumberWithInt(3), RSNumberWithInt(0), nil);
        return;
    }
    if (0) {
        RSArrayRef rst_array = RSMap(RSDrop(RSAutorelease(RSStringCreateComponentsSeparatedByStrings(RSAllocatorDefault, RSStringWithContentOfPath(RSFileManagerStandardizingPath(RSSTR("~/Desktop/B.txt"))), RSSTR("\n"))), 2), ^RSTypeRef(RSTypeRef obj) {
            RSArrayRef id_name = RSAutorelease(RSStringCreateComponentsSeparatedByStrings(RSAllocatorDefault, obj, RSSTR("\t")));
            return RSAutorelease(RSDictionaryCreateWithObjectsAndOKeys(RSAllocatorDefault, RSArrayObjectAtIndex(id_name, 1), RSArrayObjectAtIndex(id_name, 0), NULL));
        });
        RSDictionaryRef map = RSMerge(RSArrayWithObject(RSArrayObjectAtIndex(rst_array, 0)), RSDrop(rst_array, 1), nil);
        RSShow(RSMap(RSMap(RSDrop(RSAutorelease(RSStringCreateComponentsSeparatedByStrings(RSAllocatorDefault, RSStringWithContentOfPath(RSFileManagerStandardizingPath(RSSTR("~/Desktop/A.txt"))), RSSTR("\n"))), 2), ^RSTypeRef(RSTypeRef obj) {
            RSArrayRef x = RSAutorelease(RSStringCreateComponentsSeparatedByStrings(RSAllocatorDefault, obj, RSSTR("\t")));
            return RSAutorelease(RSArrayCreate(RSAllocatorDefault, RSArrayObjectAtIndex(x, 0), RSAutorelease(RSStringCreateComponentsSeparatedByStrings(RSAllocatorDefault, RSArrayLastObject(x), RSSTR(";"))), NULL));
        }), ^RSTypeRef(RSTypeRef obj) {
            RSArrayRef pair = (RSArrayRef)obj;
            return RSAutorelease(RSDictionaryCreateWithObjectsAndOKeys(RSAllocatorDefault, RSMap(RSArrayLastObject(pair), ^RSTypeRef(RSTypeRef obj) {
                return RSDictionaryGetValue(map, obj);
            }), RSArrayObjectAtIndex(pair, 0),NULL));
        }));
        return;
    }
}
