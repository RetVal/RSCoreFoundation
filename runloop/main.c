//
//  main.c
//  runloop
//
//  Created by Closure on 11/10/13.
//  Copyright (c) 2013 RetVal. All rights reserved.
//

#include <RSCoreFoundation/RSCoreFoundation.h>

RSExtern RSDataRef ServerMessagePortCallBack(RSMessagePortRef local, SInt32 msgid, RSDataRef data, void *info) {
    if (msgid == 0xbeaf) {
        RSDataRef data = RSDataWithString(RSSTR("end"), RSStringEncodingUTF8);
        RSRunLoopRef rl = RSRunLoopGetCurrent();
        RSRunLoopStop(rl);
        RSMessagePortInvalidate(local);
        return (RSDataRef)RSRetain(data);
    }
    RSLog(RSSTR("get message id = %ld"), msgid);
    
    if (data) {
        RSUnarchiverRef unarchiver = RSUnarchiverCreate(RSAllocatorDefault, data);
        RSDictionaryRef getDict = (RSDictionaryRef)RSUnarchiverDecodeObjectForKey(unarchiver, RSKeyedArchiveRootObjectKey);
        RSLog(RSSTR("message content = %R"), getDict);
        RSRelease(getDict);
        RSRelease(unarchiver);
    }

    RSMutableDictionaryRef dict = RSDictionaryCreateMutable(RSAllocatorDefault, 32, RSDictionaryRSTypeContext);
    RSDictionarySetValue(dict, RSSTR("n1"), RSSTR("v1"));
    RSDictionarySetValue(dict, RSSTR("n2"), RSSTR("v2"));
    RSDictionarySetValue(dict, RSSTR("n3"), RSSTR("v3"));
    RSLog(RSSTR("%R"), dict);
    RSArchiverRef archiver = RSArchiverCreate(RSAllocatorDefault);
    RSArchiverEncodeObjectForKey(archiver, RSKeyedArchiveRootObjectKey, dict);
    RSDataRef archivedData = RSArchiverCopyData(archiver);
    RSRelease(dict);
    RSRelease(archiver);
    
    return archivedData;
}

//static BOOL unit_test_array_insert(RSErrorRef *error) {
//    RSMutableArrayRef mutableArray = RSArrayCreateMutable(RSAllocatorDefault, 0);
//    RSArrayAddObject(mutableArray, RSSTR("value0"));
//    RSArrayAddObject(mutableArray, RSSTR("value2"));
//    RSArrayInsertObjectAtIndex(mutableArray, 1, RSSTR("value1"));
//    RSLog(RSSTR("%R"), mutableArray);
//    RSRelease(mutableArray);
//    return YES;
//}

int main(int argc, char **argv) {
    RSMessagePortRef serverMessagePort = RSMessagePortCreateLocal(RSAllocatorDefault, RSSTR("com.retval.runloop.server"), ServerMessagePortCallBack, NULL, NULL);
    RSRunLoopRef RL = RSRunLoopGetCurrent();
    RSRunLoopSourceRef serverPortSource = RSMessagePortCreateRunLoopSource(RSAllocatorDefault, serverMessagePort, 0);
    RSRunLoopAddSource(RL, serverPortSource, RSRunLoopDefaultMode);
    RSRelease(serverPortSource);
//    RSPerformBlockAfterDelay(2.0, ^{
//        RSRunLoopStop(RL);
//        RSShow(RSSTR("stop rl"));
//    });
    
    
    RSRelease(serverMessagePort);
    
    dispatch_async(dispatch_get_global_queue(0, 0), ^{
        RSMessagePortRef clientMessagePort = RSMessagePortCreateRemote(RSAllocatorDefault, RSSTR("com.retval.runloop.server"));
        RSArchiverRef archiver = RSArchiverCreate(RSAllocatorDefault);
        RSArchiverEncodeObjectForKey(archiver, RSKeyedArchiveRootObjectKey, RSSTR("hello, world"));
        RSDataRef requestData = RSArchiverCopyData(archiver);
        RSRelease(archiver);
        
        RSDataRef returnData = nil;
        RSMessagePortSendRequest(clientMessagePort, 0, requestData, 5, 5, RSSTR("SYN"), &returnData);
        
        if (returnData != nil) {
            RSUnarchiverRef unarchiver = RSUnarchiverCreate(RSAllocatorDefault, returnData);
            if (unarchiver != nil) {
                RSDictionaryRef result = RSUnarchiverDecodeObjectForKey(unarchiver, RSKeyedArchiveRootObjectKey);
                if (result != nil) {
                    RSLog(RSSTR("result: %R"), result);
                    RSRelease(result);
                }
                RSRelease(unarchiver);
            }
            RSRelease(returnData);
        }
        
        RSURLRequestRef URLRequest = RSURLRequestWithURL(RSURLWithString(RSSTR("https://www.baidu.com/")));
        
        RSURLConnectionSendAsynchronousRequest(URLRequest, RSRunLoopGetMain(), ^(RSURLResponseRef response, RSDataRef data, RSErrorRef error) {
            RSAutoreleasePoolPush();
            RSLog(RSSTR("%R"), response);
            RSXMLDocumentRef document = RSAutorelease(RSXMLDocumentCreateWithXMLData(RSAllocatorDefault, data, RSXMLDocumentTidyHTML));
            RSLog(RSSTR("%R"), document);
            RSLog(RSSTR("%R"), RSStringWithData(data, RSStringEncodingUTF8));
            RSAutoreleasePoolPop();
            dispatch_async(dispatch_get_global_queue(0, 0), ^{
                RSDataRef returnData = nil;
                RSMessagePortSendRequest(clientMessagePort, 0xbeaf, requestData, 5, 5, RSSTR("SYN"), &returnData);
                RSRelease(clientMessagePort);
            });
        });
    });
    
    RSRunLoopRun();
    return 0;
}
