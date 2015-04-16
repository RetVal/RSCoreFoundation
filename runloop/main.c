//
//  main.c
//  runloop
//
//  Created by Closure on 11/10/13.
//  Copyright (c) 2013 RetVal. All rights reserved.
//

#include <RSCoreFoundation/RSCoreFoundation.h>
#include <RSCoder/RSCoder.h>

#include <RSCoreFoundation/RSInternal.h>
RS_PUBLIC_CONST_STRING_DECL(User, "zachtest@feizan.com")
RS_PUBLIC_CONST_STRING_DECL(Password, "zachtest")

int main(int argc, const char * argv[]) {
    RSDataRef data = RSDataCreateWithString(RSAllocatorDefault, User, RSStringEncodingUTF8);
    RSDataRef encoded = RSCoderRoutineInvoke(RSMD5Coder, RSCoderInvokeEncode, data);
    RSRelease(data);
    
    RSStringRef encodedString = RSStringCreateWithData(RSAllocatorDefault, encoded, RSStringEncodingUTF8);
    RSShow(encodedString);
    RSRelease(encodedString);
    RSRelease(encoded);
    return 0;
}
