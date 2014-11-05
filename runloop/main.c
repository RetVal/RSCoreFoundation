//
//  main.c
//  runloop
//
//  Created by Closure on 11/10/13.
//  Copyright (c) 2013 RetVal. All rights reserved.
//

#include <RSCoreFoundation/RSCoreFoundation.h>

int main(int argc, const char * argv[]) {
    RSURLConnectionSendAsynchronousRequest(RSURLRequestWithURL(RSURLWithString(RSSTR("http://www.baidu.com/"))), RSRunLoopGetCurrent(), ^(RSURLResponseRef response, RSDataRef data, RSErrorRef error) {
        RSShow(response);
//        RSShow(RSStringWithData(data, RSStringEncodingUTF8));
        if (error) RSShow(error);
        RSRunLoopStop(RSRunLoopGetMain());
    });
    RSShow(RSSTR("start run loop"));
    RSRunLoopRun();
    RSShow(RSSTR("stop run loop"));
    return 0;
}
