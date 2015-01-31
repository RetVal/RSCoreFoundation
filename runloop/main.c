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
        
        if (error) {
            RSShow(error);
        } else if (data) {
            RSShow(RSStringWithData(data, RSStringEncodingUTF8));
        }
        RSRunLoopStop(RSRunLoopGetMain());
    });
    RSShow(RSSTR("start run loop"));
    RSRunLoopRun();
    RSShow(RSSTR("stop run loop"));
    
    for (unsigned i = 0; i < 1000; i++) {
        RSShow(RSNumberWithInt(i));
    }
    
    return 0;
}
