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
        return part;
    };
    RSAutoreleaseBlock(^{
        RSArrayApplyBlock(lines, RSMakeRange(0, RSArrayGetCount(lines)), ^(const void *value, RSUInteger idx, BOOL *isStop) {
            RSStringRef line = (RSStringRef)value;
            if (RSStringHasPrefix(line, prefix)) {
                RSStringRef nonPrefixLine = RSStringWithSubstring(line, RSMakeRange(RSStringGetLength(prefix), RSStringGetLength(line) - RSStringGetLength(prefix)));
                RSArrayRef parts = RSArrayBySeparatingStrings(nonPrefixLine, RSSTR(" "));
                if (RSArrayGetCount(parts) == 4) {
                    RSMutableArrayRef classPart = (RSMutableArrayRef)RSDictionaryGetValue(map, RSArrayObjectAtIndex(parts, 0));
                    if (!classPart) {
                        classPart = RSArrayCreateMutable(RSAllocatorDefault, 0);
                        RSDictionarySetValue(map, RSArrayObjectAtIndex(parts, 0), classPart);
                        RSRelease(classPart);
                    }
                    RSStringRef address = parserAddress(RSArrayLastObject(parts));
                    if (address) {
                        if (RSEqual(RSArrayObjectAtIndex(parts, 1), RSSTR("alloc"))) {
                            RSArrayAddObject(classPart, address);
                        } else if (RSEqual(RSArrayObjectAtIndex(parts, 1), RSSTR("dealloc"))) {
                            RSArrayRemoveObject(classPart, address);
                            if (0 == RSArrayGetCount(classPart)) {
                                RSDictionaryRemoveValue(map, RSArrayObjectAtIndex(parts, 0));
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

int main(int argc, const char * argv[])
{
    RSStringRef resultPath = RSSTR("~/Desktop/leak.plist");
    RSMutableDictionaryRef map = RSDictionaryCreateMutable(RSAllocatorDefault, 0, RSDictionaryRSTypeContext);
    RSDataRef data = RSDataCreateWithContentOfPath(RSAllocatorDefault, RSStringWithUTF8String(argv[1]));
    parser(map, data);
    RSRelease(data);
    RSDictionaryWriteToFile(map, RSFileManagerStandardizingPath(resultPath), RSWriteFileAutomatically);
    RSRelease(map);
    return 0;
}

