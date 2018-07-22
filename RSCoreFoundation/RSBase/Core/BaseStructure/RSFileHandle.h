//
//  RSFileHandle.h
//  RSCoreFoundation
//
//  Created by RetVal on 3/11/13.
//  Copyright (c) 2013 RetVal. All rights reserved.
//

#ifndef RSCoreFoundation_RSFileHandle_h
#define RSCoreFoundation_RSFileHandle_h
RS_EXTERN_C_BEGIN
typedef const struct __RSFileHandle*  RSFileHandleRef;
RSExport RSTypeID RSFileHandleGetTypeID(void) RS_AVAILABLE(0_0);
typedef RS_ENUM(RSUInteger, RSFileOptionMode) {
    RSFileOptionDefaulte = 0x10,
    RSFileOptionRead = 0x20,
    RSFileOptionWrite = 0x40,
    RSFileOptionUpdate = 0x60,
    RSFileOptionDeleteWhenDone = 0x80,
    RSFileOptionTempelate = 0xE0,
};

typedef RSBitU64 RSFileLength;
typedef int RSFileHandle;
RSExport RSFileHandleRef RSFileHandleCreateForReadingAtPath(RSStringRef path) RS_AVAILABLE(0_0);
RSExport RSFileHandleRef RSFileHandleCreateForWritingAtPath(RSStringRef path) RS_AVAILABLE(0_0);
RSExport RSFileHandleRef RSFileHandleCreateForUpdatingAtPath(RSStringRef path) RS_AVAILABLE(0_0);
RSExport RSFileHandleRef RSFileHandleCreateWithFileDescriptor(RSFileHandle fd) RS_AVAILABLE(0_4);
    
RSExport RSFileHandleRef RSFileHandleCreateWithStdin(void) RS_AVAILABLE(0_2);
RSExport RSFileHandleRef RSFileHandleCreateWithStdout(void) RS_AVAILABLE(0_2);
RSExport RSFileHandleRef RSFileHandleCreateWithStderr(void) RS_AVAILABLE(0_2);

RSExport RSFileLength RSFileHandleGetFileSize(RSFileHandleRef handle) RS_AVAILABLE(0_0);
RSExport RSFileLength RSFileHanldeOffsetInFile(RSFileHandleRef handle) RS_AVAILABLE(0_0);
    
// read data from handle with retain!!!
RSExport RSDataRef RSFileHandleReadDataOfLength(RSFileHandleRef handle, RSFileLength length) RS_AVAILABLE(0_0);
RSExport RSDataRef RSFileHandleReadDataToEndOfFile(RSFileHandleRef handle) RS_AVAILABLE(0_0);
    
RSExport void RSFileHandleSeekToFileOffset(RSFileHandleRef handle, RSFileLength offset) RS_AVAILABLE(0_0);
RSExport BOOL RSFileHandleWriteData(RSFileHandleRef handle ,RSDataRef data) RS_AVAILABLE(0_0);
RSExport void RSFileHandleCloseFile(RSFileHandleRef handle) RS_AVAILABLE(0_0);
RSExport RSStringRef RSFileHandleGetPath(RSFileHandleRef handle) RS_AVAILABLE(0_0);
RSExport RSFileHandle RSFileHandleGetHandle(RSFileHandleRef handle) RS_AVAILABLE(0_0);

RS_EXTERN_C_END
#endif
