//
//  main.c
//  RSObjectLeakParser
//
//  Created by closure on 12/19/13.
//  Copyright (c) 2013 closure. All rights reserved.
//

#include <RSCoreFoundation/RSCoreFoundation.h>

static void parser(RSMutableDictionaryRef map, RSDataRef data) {
    if (!map || !data) return;
    RSStringRef const prefix = RSSTR("RSRuntime debug notice : ");
    RSStringRef content = RSStringWithData(data, RSStringEncodingUTF8);
    RSArrayRef lines = RSArrayBySeparatingStrings(content, RSSTR("\n"));
    
    RSStringRef (^parserAddress)(RSStringRef part) = ^RSStringRef (RSStringRef part) {
        return RSStringWithSubstring(part, RSMakeRange(1, RSStringGetLength(part) - 2));
    };
    RSAutoreleaseBlock(^{
        RSArrayApplyBlock(lines, RSMakeRange(0, RSArrayGetCount(lines)), ^(const void *value, RSUInteger idx, BOOL *isStop) {
            RSStringRef line = (RSStringRef)value;
            if (RSStringHasPrefix(line, prefix)) {
                RSStringRef nonPrefixLine = RSStringWithSubstring(line, RSMakeRange(RSStringGetLength(prefix), RSStringGetLength(line) - RSStringGetLength(prefix)));
                RSArrayRef parts = RSArrayBySeparatingStrings(nonPrefixLine, RSSTR(" "));
                if (RSArrayGetCount(parts) == 4) {
                    RSStringRef className = RSArrayObjectAtIndex(parts, 0);
                    if (RSEqual(className, RSSTR("RSDictionary")) ||
                        RSEqual(className, RSSTR("RSSet")) ||
                        RSEqual(className, RSSTR("RSBag"))) {
                        className = RSSTR("RSBasicHash");
                    }
                    RSUInteger sw = RSEqual(RSArrayObjectAtIndex(parts, 1), RSSTR("alloc")) == YES ? 1 : 0;
                    if (!sw) sw = RSEqual(RSArrayObjectAtIndex(parts, 1), RSSTR("dealloc")) == YES ? 2 : 0;
                    if (sw) {
                        RSMutableArrayRef classPart = (RSMutableArrayRef)RSDictionaryGetValue(map, className);
                        if (!classPart) {
                            classPart = RSArrayCreateMutable(RSAllocatorDefault, 0);
                            RSDictionarySetValue(map, className, classPart);
                            RSRelease(classPart);
                        }
                        RSStringRef address = parserAddress(RSArrayLastObject(parts));
                        if (sw == 1) {
                            RSArrayAddObject(classPart, address);
                        } else if (sw == 2) {
                            RSArrayRemoveObject(classPart, address);
                            if (0 == RSArrayGetCount(classPart)) {
                                RSDictionaryRemoveValue(map, className);
                            }
                        }
                    }
                } else {
                    RSLog(RSSTR("%s %r"), __func__, parts);
                }
            }
        });
    });
}

int main(int argc, const char * argv[]) {
    RSStringRef resultPath = RSSTR("~/Desktop/leak.plist");
    RSMutableDictionaryRef map = RSDictionaryCreateMutable(RSAllocatorDefault, 0, RSDictionaryRSTypeContext);
    RSDataRef data = RSDataCreateWithContentOfPath(RSAllocatorDefault, RSStringWithUTF8String(argv[1]));
    parser(map, data);
    RSRelease(data);
    RSDictionaryWriteToFile(map, RSFileManagerStandardizingPath(resultPath), RSWriteFileAutomatically);
    RSRelease(map);
    return 0;
}

