//
//  RSURL+Extension.c
//  RSCoreFoundation
//
//  Created by closure on 1/3/14.
//  Copyright (c) 2014 RetVal. All rights reserved.
//

#include <RSCoreFoundation/RSAutoreleasePool.h>
#include <RSCoreFoundation/RSURL.h>
#include <RSCoreFoundation/RSURLResponse.h>
#include <RSCoreFoundation/RSURLRequest.h>
#include <RSCoreFoundation/RSURLConnection.h>
#include <RSCoreFoundation/RSData+Extension.h>

RSExport RSDataRef RSDataWithURL(RSURLRef URL) {
    if (!URL) return nil;
    if (RSEqual(RSSTR("file"), RSAutorelease(RSURLCopyScheme(URL)))) {
        return RSDataWithContentOfPath(RSAutorelease(RSURLCopyFileSystemPath(URL, RSURLPOSIXPathStyle)));
    }
    return RSURLConnectionSendSynchronousRequest(RSURLRequestWithURL(URL), nil, nil);
}
