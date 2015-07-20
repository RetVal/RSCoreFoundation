//
//  main.c
//  runloop
//
//  Created by Closure on 11/10/13.
//  Copyright (c) 2013 RetVal. All rights reserved.
//

#include <RSFoundation/RSFoundation.h>

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
    using namespace RSCF;
    String *s = new String("RSFoundation");
    s->release();

    RSMessagePortRef messagePort = RSMessagePortCreateLocal(RSAllocatorDefault, RSSTR("com.retval.rl.server"), ServerMessagePortCallBack, nil, nil);
    
    RSRunLoopSourceRef source = RSMessagePortCreateRunLoopSource(RSAllocatorDefault, messagePort, 0);
    RSRunLoopRef rl = RSRunLoopGetCurrent();
    RSRunLoopAddSource(rl, source, RSRunLoopDefaultMode);
    RSRelease(source);
    RSRunLoopRun();
    
    sleep(1);
    return 0;
}