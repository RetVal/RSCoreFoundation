//
//  RSPrivateFileSystem.h
//  RSCoreFoundation
//
//  Created by RetVal on 5/14/13.
//  Copyright (c) 2013 RetVal. All rights reserved.
//

#ifndef RSCoreFoundation_RSPrivateFileSystem_h
#define RSCoreFoundation_RSPrivateFileSystem_h

BOOL _RSGetCurrentDirectory(char *path, int maxlen);
BOOL _RSGetExecutablePath(char *path, int maxlen);

RSHandle __RSLoadLibrary(RSStringRef dir, RSStringRef lib);
RSHandle _RSLoadLibrary(RSStringRef library);
void *_RSGetImplementAddress(RSHandle handle, const char* key);
void _RSCloseHandle(RSHandle handle);
#endif
