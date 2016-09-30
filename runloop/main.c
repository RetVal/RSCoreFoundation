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
        RSRelease(local);
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

int main(int argc, char **argv) {
    RSStringRef str = RSStringCreateWithCString(RSAllocatorDefault, "/SourceCache/Library/Frameworks/Foundation/RSImageIO/RSImageIOTests/UnitTest/1.exr", RSStringEncodingUTF8);
    RSRelease(str);
    RSMessagePortRef MessagePort = RSMessagePortCreateLocal(RSAllocatorDefault, RSSTR("com.retval.runloop.server"), ServerMessagePortCallBack, NULL, NULL);
    RSRunLoopRef RL = RSRunLoopGetCurrent();
    RSRunLoopSourceRef Source = RSMessagePortCreateRunLoopSource(RSAllocatorDefault, MessagePort, 0);
    RSRunLoopAddSource(RL, Source, RSRunLoopDefaultMode);
    RSRelease(Source);
//    RSPerformBlockAfterDelay(2.0, ^{
//        RSRunLoopStop(RL);
//        RSShow(RSSTR("stop rl"));
//    });
    RSRunLoopRun();
    RSRelease(MessagePort);
//    RSURLConnectionSendAsynchronousRequest(RSURLRequestWithURL(RSURLWithString(RSSTR("http://www.baidu.com/"))), RSRunLoopGetCurrent(), ^(RSURLResponseRef response, RSDataRef data, RSErrorRef error) {
//        RSAutoreleasePoolPush();
//        RSLog(RSSTR("%R"), response);
//        RSXMLDocumentRef document = RSAutorelease(RSXMLDocumentCreateWithXMLData(RSAllocatorDefault, data, RSXMLDocumentTidyHTML));
//        RSLog(RSSTR("%R"), document);
//        RSLog(RSSTR("%R"), RSStringWithData(data, RSStringEncodingUTF8));
//        RSAutoreleasePoolPop();
//    });
    return 0;
}