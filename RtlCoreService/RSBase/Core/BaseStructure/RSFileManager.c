//
//  RSFileManager.c
//  RSCoreFoundation
//
//  Created by RetVal on 1/29/13.
//  Copyright (c) 2013 RetVal. All rights reserved.
//

#include "RSPrivate/RSFileManager/RSFileManagerEnumSupport.h"
#include "RSPrivate/RSFileManager/RSFileManagerPathSupport.h"
#include "RSPrivate/RSPrivateOperatingSystem/RSPrivateShell.h"
#include <RSCoreFoundation/RSFileManager.h>
#include <RSCoreFoundation/RSRuntime.h>
#include <RSCoreFoundation/RSSet.h>
#include <grp.h>
#include <pwd.h>
#include <stdio.h>
#include <sys/fcntl.h>
#include <sys/stat.h>
#include <sys/param.h>

RS_CONST_STRING_DECL(_RSFileSystemSymbolUserPath, "~/");
RS_CONST_STRING_DECL(_RSFileSystemSymbolRootPath, "/");

RSPrivate BOOL __RSProphylacticAutofsAccess = NO;
RSInline int openAutoFSNoWait() {
#if DEPLOYMENT_TARGET_WINDOWS
    return -1;
#else
    return (__RSProphylacticAutofsAccess ? open("/dev/autofs_nowait", 0) : -1);
#endif
}

RSInline void closeAutoFSNoWait(int fd) {
    if (-1 != fd) close(fd);
}

static BOOL _RSRemoveDirectory(const char *path) {
    int no_hang_fd = openAutoFSNoWait();
    int ret = ((rmdir(path) == 0) ? YES : NO);
    closeAutoFSNoWait(no_hang_fd);
    return ret;
}

static BOOL _RSRemoveFile(const char *path) {
    int no_hang_fd = openAutoFSNoWait();
    int ret = unlink(path) == 0;
    closeAutoFSNoWait(no_hang_fd);
    return ret;
}

static BOOL _RSFileExisting(const char* path) {
    int no_hang_fd = openAutoFSNoWait();
    int ret = (0 == access(path, F_OK));
    closeAutoFSNoWait(no_hang_fd);
    return ret;
}

static int _mkdir(const char *dir) {
    char tmp[RSBufferSize] = {0};
    snprintf(tmp, sizeof(tmp), "%s", dir);
    size_t   i = 1, len = strlen(tmp);
    if(tmp[len-1]!='/') strcat(tmp, "/");
    for(; i < len; i++) {
        if(tmp[i] == '/') {
            tmp[i] = 0;
            if(access(tmp, nil)) {
                if(mkdir(tmp, 0755) == -1) {
                    return -1;
                }  
            }  
            tmp[i]   =   '/';  
        }  
    }
    return   0;  
}

static BOOL _RSCreateDirectory(const char *path) {
    int no_hang_fd = openAutoFSNoWait();
    int ret = ((_mkdir(path) == 0) ? YES : NO);
    closeAutoFSNoWait(no_hang_fd);
    return ret;
}

#if DEPLOYMENT_TARGET_MACOSX
#include <sys/time.h>
#include <sys/signal.h>
#endif
#pragma mark -
#pragma mark RSFileHandle API
struct __RSFileHandle
{
    RSRuntimeBase _base;
    RSStringRef _path;
    RSFileHandle _fp;
};

typedef struct __RSFileHandle* __RSFileHandleRef;
RSInline BOOL isFpAvailable(RSFileHandleRef fh)
{
    return (fh->_fp == -1) ? NO : YES;
}

RSInline void markFileRead(RSFileHandleRef fh)
{
    __RSBitfieldSetValue(RSRuntimeClassBaseFiled(fh), 1, 1, 1);
}

RSInline void unMarkFileRead(RSFileHandleRef fh)
{
    __RSBitfieldSetValue(RSRuntimeClassBaseFiled(fh), 1, 1, 0);
}

RSInline BOOL isFileCanBeRead(RSFileHandleRef fh)
{
    return __RSBitfieldGetValue(RSRuntimeClassBaseFiled(fh), 1, 1);
}

RSInline void markFileWrite(RSFileHandleRef fh)
{
    __RSBitfieldSetValue(RSRuntimeClassBaseFiled(fh), 2, 2, 1);
}

RSInline void unMarkFileWrite(RSFileHandleRef fh)
{
    __RSBitfieldSetValue(RSRuntimeClassBaseFiled(fh), 2, 2, 0);
}

RSInline BOOL isFileCanBeWrote(RSFileHandleRef fh)
{
    return __RSBitfieldGetValue(RSRuntimeClassBaseFiled(fh), 2, 2);
}

RSInline void markFileTemplate(RSFileHandleRef fh)
{
    __RSBitfieldSetValue(RSRuntimeClassBaseFiled(fh), 4, 4, 1);
}

RSInline void unMarkFileTemplate(RSFileHandleRef fh)
{
    __RSBitfieldSetValue(RSRuntimeClassBaseFiled(fh), 4, 4, 0);
}

RSInline BOOL isFileTemplate(RSFileHandleRef fh)
{
    return __RSBitfieldGetValue(RSRuntimeClassBaseFiled(fh), 4, 4);
}

static void __RSFileHandleClassInit(RSTypeRef rs)
{
    __RSFileHandleRef fh = (__RSFileHandleRef)rs;
    fh->_path = nil;
    fh->_fp = -1;
    return;
}

static RSTypeRef __RSFileHandleClassCopy(RSAllocatorRef allocator, RSTypeRef rs, BOOL mutableCopy)
{
    return nil;
}

static void __RSFileHandleClassDeallocate(RSTypeRef rs)
{
    __RSFileHandleRef fh = (__RSFileHandleRef)rs;
    if (fh->_fp)
    {
        close(fh->_fp);
        fh->_fp = -1;
    }
    if (isFileTemplate(fh)) {
        _RSRemoveFile(RSStringGetCStringPtr(fh->_path, __RSDefaultEightBitStringEncoding));
    }
    RSRelease(fh->_path);
    return;
}

static BOOL __RSFileHandleClassEqual(RSTypeRef rs1, RSTypeRef rs2)
{
    return RSEqual(RSFileHandleGetPath(rs1), RSFileHandleGetPath(rs2));
}

static RSHashCode __RSFileHandleClassHash(RSTypeRef rs)
{
    return RSHash(RSFileHandleGetPath(rs));
}

static RSStringRef __RSFileHandleClassDescription(RSTypeRef rs)
{
    return RSCopy(RSAllocatorSystemDefault, RSFileHandleGetPath(rs));
}

static RSRuntimeClass __RSFileHandleClass = {
    _RSRuntimeScannedObject,
    "RSFileHandle",
    __RSFileHandleClassInit,
    __RSFileHandleClassCopy,
    __RSFileHandleClassDeallocate,
    __RSFileHandleClassEqual,
    __RSFileHandleClassHash,
    __RSFileHandleClassDescription,
    nil,
    nil
};

static RSTypeID _RSFileHandleTypeID = _RSRuntimeNotATypeID;
RSPrivate void __RSFileHandleInitialize()
{
    _RSFileHandleTypeID = __RSRuntimeRegisterClass(&__RSFileHandleClass);
    __RSRuntimeSetClassTypeID(&__RSFileHandleClass, _RSFileHandleTypeID);
}

RSExport RSTypeID RSFileHandleGetTypeID()
{
    if (unlikely(_RSFileHandleTypeID == _RSRuntimeNotATypeID))
        __RSFileHandleInitialize();
    return _RSFileHandleTypeID;
}

static void __RSFileHandleUpdate(RSFileHandleRef handle)
{
    __RSFileHandleRef fh = (__RSFileHandleRef)handle;
    const char* path = (const char*)RSStringGetCStringPtr(fh->_path, __RSDefaultEightBitStringEncoding);
    if (0 == access(path, R_OK)) markFileRead(handle);
    if (0 == access(path, W_OK)) markFileWrite(handle);    
}

static BOOL __RSFileHandleVerifyFd(__RSFileHandleRef fh)
{
    if (fh->_fp > 0)
    {
        if (fh->_fp)
        {
            int result = fcntl(fh->_fp, F_GETFL);
            if (result == -1)
                return NO;
        }
        if (fh->_fp > STDERR_FILENO)
        {
            // custom file descriptor
            int result = 0;
            result = fcntl(fh->_fp, F_GETFL, 0);
            if (result & O_RDWR)
            {
                markFileRead(fh);
                markFileWrite(fh);
            }
            else if (result & O_RDONLY)
                markFileRead(fh);
            else if (result & O_WRONLY)
                markFileWrite(fh);
            
        }
        return YES;
    }
    return NO;
}

static RSFileHandleRef __RSFileHandleCreateInstance(RSAllocatorRef allocator, RSStringRef filePath, RSFileOptionMode mode, RSFileHandle cfd)
{
    if (filePath == nil && mode > STDERR_FILENO && cfd == -1) return nil;
    int fdMode = 0;
    BOOL isTempFile = NO;
    static int privilege = 0777;
    RSFileHandle fd = -1;
    switch (mode)
    {
        case RSFileOptionRead:
            fdMode |= (O_RDONLY);
            break;
        case RSFileOptionWrite:
            fdMode |= (O_WRONLY|O_CREAT);
            break;
        case RSFileOptionDefaulte:
        case RSFileOptionUpdate:
            fdMode |= (O_RDWR|O_CREAT);
            break;
        case RSFileOptionTempelate:
            isTempFile = YES;
            fdMode |= O_WRONLY|O_CREAT|O_TRUNC;
            break;
        case STDIN_FILENO:
            fd = STDIN_FILENO;
            break;
        case STDOUT_FILENO:
            fd = STDOUT_FILENO;
            break;
        case STDERR_FILENO:
            fd = STDERR_FILENO;
            break;
        default:
            fd = (RSFileHandle)cfd;
            break;
    }
    char tempName[RSBufferSize] = "RSCoreFoundationXXXXXX";
    const char* fileName = tempName;
    
    if (fd == -1)
    {
        if (isTempFile)
        {
            fileName = mkstemp("RSCoreFoundationXXXXXX");//mktemp();
        }
        else
        {
            fileName = RSStringGetCStringPtr(filePath, __RSDefaultEightBitStringEncoding);
        }
        fd = open(fileName, fdMode, privilege);
    }
    if (fd != -1)
    {
        __RSFileHandleRef fh = (__RSFileHandleRef)__RSRuntimeCreateInstance(allocator, RSFileHandleGetTypeID(), sizeof(struct __RSFileHandle) - sizeof(RSRuntimeBase));
        fh->_fp = fd;
        if (fd > STDERR_FILENO)
        {
            if (isTempFile)
            {
                fh->_path = RSStringCreateWithCString(allocator, fileName, __RSDefaultEightBitStringEncoding);
//                free((void*)fileName);
                fileName = nil;
            }
            else if (filePath)
            {
                fh->_path = RSCopy(allocator, filePath);
                fileName = nil;
            }
            else if (cfd)
            {
                if (!__RSFileHandleVerifyFd(fh))
                {
                    RSRelease(fh);
                    return nil;
                }
            }
        }
        else
        {
            switch (fd)
            {
                case STDIN_FILENO:
                    fh->_path = RSStringCreateWithCString(allocator, "stdin", RSStringEncodingASCII);
                    break;
                case STDOUT_FILENO:
                    fh->_path = RSStringCreateWithCString(allocator, "stdout", RSStringEncodingASCII);
                    break;
                case STDERR_FILENO:
                    fh->_path = RSStringCreateWithCString(allocator, "stderr", RSStringEncodingASCII);
                    break;
                default:
                    break;
            }
        }
        
        switch (mode)
        {
            case RSFileOptionRead:
            case STDIN_FILENO:
                markFileRead(fh);
                break;
            case RSFileOptionWrite:
            case STDOUT_FILENO:
                markFileWrite(fh);
                break;
            case RSFileOptionDefaulte:
            case RSFileOptionUpdate:
            case STDERR_FILENO:
                markFileRead(fh);
                markFileWrite(fh);
                break;
            case RSFileOptionTempelate:
                markFileTemplate(fh);markFileRead(fh);markFileWrite(fh);
                break;
            default:
                break;
        }
        return fh;
    }
//    if (fileName && isTempFile) free((void*)fileName);
    return nil;
}


RSExport RSFileHandleRef RSFileHandleCreateWithStdin() RS_AVAILABLE(0_2)
{
    return __RSFileHandleCreateInstance(RSAllocatorSystemDefault, nil, STDIN_FILENO, -1);
}
RSExport RSFileHandleRef RSFileHandleCreateWithStdout() RS_AVAILABLE(0_2)
{
    return __RSFileHandleCreateInstance(RSAllocatorSystemDefault, nil, STDOUT_FILENO, -1);
}
RSExport RSFileHandleRef RSFileHandleCreateWithStderr() RS_AVAILABLE(0_2)
{
    return __RSFileHandleCreateInstance(RSAllocatorSystemDefault, nil, STDERR_FILENO, -1);
}

RSExport RSFileHandleRef RSFileHandleCreateForReadingAtPath(RSStringRef path)
{
    return __RSFileHandleCreateInstance(RSAllocatorSystemDefault, path, RSFileOptionRead, -1);
}

RSExport RSFileHandleRef RSFileHandleCreateForWritingAtPath(RSStringRef path)
{
    return __RSFileHandleCreateInstance(RSAllocatorSystemDefault, path, RSFileOptionWrite, -1);
}

RSExport RSFileHandleRef RSFileHandleCreateForUpdatingAtPath(RSStringRef path)
{
    return __RSFileHandleCreateInstance(RSAllocatorSystemDefault, path, RSFileOptionUpdate, -1);
}

RSExport RSFileHandleRef RSFileHandleCreateWithFileDescriptor(RSFileHandle fd)
{
    return __RSFileHandleCreateInstance(RSAllocatorSystemDefault, nil, -1, fd);
}

RSExport RSFileLength RSFileHandleGetFileSize(RSFileHandleRef handle)
{
    if (handle == nil) return -1;
    __RSGenericValidInstance(handle, _RSFileHandleTypeID);
    if (isFpAvailable(handle) == NO) return (unsigned long long)-1;
    int fd = handle->_fp;
    RSFileLength org = lseek(fd, 0, SEEK_CUR);
    lseek(fd, 0, SEEK_END);
    RSFileLength size = lseek(fd, 0, SEEK_CUR); // get current file pointer
    lseek(fd, org, SEEK_SET);
    return size + 1;
}

RSExport RSFileLength RSFileHanldeOffsetInFile(RSFileHandleRef handle)
{
    if (handle == nil) return -1;
    __RSGenericValidInstance(handle, _RSFileHandleTypeID);
    if (isFpAvailable(handle) == NO) return (unsigned long long)-1;
    RSFileLength offset = 0;
    offset = lseek(handle->_fp, 0, SEEK_CUR);
    return offset;
}

RSExport RSDataRef RSFileHandleReadDataToEndOfFile(RSFileHandleRef handle)
{
    if (handle == nil) return nil;
    __RSGenericValidInstance(handle, _RSFileHandleTypeID);
    if (isFpAvailable(handle) && isFileCanBeRead(handle))
    {
        RSBitU8* bytes = nil;
        RSFileHandle fp = handle->_fp;
        if (fp <= STDERR_FILENO)
        {
            RSMutableDataRef data = RSDataCreateMutable(RSAllocatorSystemDefault, 1024);
            long readlength = 0;
            static const int size = 1024;
            RSBitU8 buffer[1024] = {0};
        read_again:
            readlength = read(fp, buffer, size);
            if (readlength > 0)
            {
                RSDataAppendBytes(data, buffer, readlength);
                if (readlength < size) return data;
                else
                {
                    readlength = 0;
                    __builtin_memset(buffer, 0, size);
                    goto read_again;
                }
            }
            else {
                if (RSDataGetLength(data)) return data;
                else RSRelease(data);
                return nil;
            }
        }
        else
        {
            RSFileLength size = RSFileHandleGetFileSize(handle);
            RSFileLength readSize = 0;
            size -= RSFileHanldeOffsetInFile(handle);
            if ((bytes = RSAllocatorAllocate(RSAllocatorSystemDefault,
                                             size)))
            {
                readSize = read(handle->_fp, bytes, size);
                RSDataRef data = RSDataCreateWithNoCopy(RSAllocatorSystemDefault,
                                                        bytes,
                                                        readSize,
                                                        YES,
                                                        RSAllocatorSystemDefault);
                return data;
            }
        }
    }
    return nil;
}

RSExport void RSFileHandleSeekData(RSFileHandleRef handle, RSFileLength offset)
{
    if (handle == nil) return;
    __RSGenericValidInstance(handle, _RSFileHandleTypeID);
    if (isFpAvailable(handle))
        lseek(handle->_fp, offset, SEEK_CUR);
}

RSExport RSDataRef RSFileHandleReadDataOfLength(RSFileHandleRef handle, RSFileLength length)
{
    if (handle == nil) return nil;
    __RSGenericValidInstance(handle, _RSFileHandleTypeID);
    RSDataRef data = nil;
    if (isFpAvailable(handle) && isFileCanBeRead(handle))
    {
        RSFileLength last = RSFileHandleGetFileSize(handle);
        last -= RSFileHanldeOffsetInFile(handle);
        length = min(last, length);
        RSBitU8* bytes = nil;
        if ((bytes = RSAllocatorAllocate(RSAllocatorSystemDefault, length)))
        {
            read(RSFileHandleGetHandle(handle), bytes, length);
            data = RSDataCreateWithNoCopy(RSAllocatorSystemDefault,
                                          bytes,
                                          length,
                                          YES,
                                          RSAllocatorSystemDefault);
        }
    }
    return data;
}

RSExport BOOL RSFileHandleWriteData(RSFileHandleRef handle ,RSDataRef data)
{
    if (handle == nil || data == nil) return NO;
    __RSGenericValidInstance(handle, _RSFileHandleTypeID);
    if (isFpAvailable(handle) && isFileCanBeWrote(handle))
    {
        RSIndex length = RSDataGetLength(data);
        if (length)
        {
            return (length == write(handle->_fp, RSDataGetBytesPtr(data), length));
        }
    }
    return NO;
}


RSExport void RSFileHandleSeekToFileOffset(RSFileHandleRef handle, RSFileLength offset)
{
    if (handle == nil) return ;
    __RSGenericValidInstance(handle, _RSFileHandleTypeID);
    if (isFpAvailable(handle))
    {
        lseek(handle->_fp, offset, SEEK_SET);
    }
}

RSExport void RSFileHandleCloseFile(RSFileHandleRef handle)
{
    if (handle == nil) return ;
    __RSGenericValidInstance(handle, _RSFileHandleTypeID);
    if (isFpAvailable(handle))
    {
        close(handle->_fp);
        ((__RSFileHandleRef)handle)->_fp = -1;
        RSRelease(handle->_path);
        ((__RSFileHandleRef)handle)->_path = nil;
    }
}

RSExport RSStringRef RSFileHandleGetPath(RSFileHandleRef handle)
{
    if (handle == nil) return RSStringGetEmptyString();
    __RSGenericValidInstance(handle, _RSFileHandleTypeID);
    if (isFpAvailable(handle))
    {
        if (!handle->_path) {
            char c_path[RSBufferSize] = {0};
            int result = fcntl(handle->_fp, F_GETPATH, c_path);
            if (result >= 0)
            {
                ((struct __RSFileHandle *)handle)->_path = RSStringCreateWithCString(RSAllocatorSystemDefault, c_path, __RSDefaultEightBitStringEncoding);
            }
        }
        return handle->_path;
    }
    return RSStringGetEmptyString();
}

RSExport RSFileHandle RSFileHandleGetHandle(RSFileHandleRef handle)
{
    if (handle == nil) return -1;
    __RSGenericValidInstance(handle, _RSFileHandleTypeID);
    return handle->_fp;
}

#pragma mark RSFileHandle End

#pragma mark -
#pragma mark RSFileWrapper API

struct __RSFileWrapper
{
    RSRuntimeBase _base;
    RSStringRef _filePath;
    int _fd;
    RSMutableDictionaryRef _fileAttributes;
    struct stat _fstat;     //lstat
};

typedef struct __RSFileWrapper* __RSFileWrapperRef;
static void __RSFileWrapperClassInit(RSTypeRef rs)
{
    __RSFileWrapperRef fw = (__RSFileWrapperRef)rs;
    fw->_fd = -1;
    fw->_filePath = nil;
    fw->_fileAttributes = RSDictionaryCreateMutable(RSAllocatorSystemDefault, 0, RSDictionaryRSTypeContext);
}

static RSTypeRef __RSFileWrapperClassCopy(RSAllocatorRef allocator, RSTypeRef rs, BOOL mutableCopy)
{
    return nil;
}

static void __RSFileWrapperClassDeallocate(RSTypeRef rs)
{
    __RSFileWrapperRef fw = (__RSFileWrapperRef)rs;
    if (fw->_fd) close(fw->_fd);
    fw->_fd = -1;
    RSRelease(fw->_fileAttributes);
    RSRelease(fw->_filePath);
}

static BOOL __RSFileWrapperClassEqual(RSTypeRef rs1, RSTypeRef rs2)
{
    __RSFileWrapperRef fw1 = (__RSFileWrapperRef)rs1;
    __RSFileWrapperRef fw2 = (__RSFileWrapperRef)rs2;
    return RSEqual(fw1->_filePath, fw2->_filePath);
}

static RSHashCode __RSFileWrapperClassHash(RSTypeRef rs)
{
    __RSFileWrapperRef fw = (__RSFileWrapperRef)rs;
    return RSHash(fw->_filePath);
}

static RSStringRef __RSFileWrapperClassDescription(RSTypeRef rs)
{
    return RSCopy(RSAllocatorSystemDefault, ((RSFileWrapperRef)rs)->_filePath);
}

static RSRuntimeClass __RSFileWrapperClass = {
    _RSRuntimeScannedObject,
    "RSFileWrapper",
    __RSFileWrapperClassInit,
    __RSFileWrapperClassCopy,
    __RSFileWrapperClassDeallocate,
    __RSFileWrapperClassEqual,
    __RSFileWrapperClassHash,
    __RSFileWrapperClassDescription,
    nil,
    nil
};

static RSTypeID _RSFileWrapperTypeID = _RSRuntimeNotATypeID;

RSPrivate void __RSFileWrapperInitialize()
{
    _RSFileWrapperTypeID = __RSRuntimeRegisterClass(&__RSFileWrapperClass);
    __RSRuntimeSetClassTypeID(&__RSFileWrapperClass, _RSFileWrapperTypeID);
}

RSExport RSTypeID RSFileWrapperGetTypeID()
{
    if (unlikely(_RSFileWrapperTypeID == _RSRuntimeNotATypeID))
        __RSFileWrapperInitialize();
    return _RSFileWrapperTypeID;
}

RSExport BOOL RSFileWrapperIsRegularFile(RSFileWrapperRef file)
{
    __RSGenericValidInstance(file, _RSFileWrapperTypeID);
    if (file->_fstat.st_mode & S_IFREG)
    {
        return YES;
    }
    return NO;
}

RSExport BOOL RSFileWrapperIsDirectory(RSFileWrapperRef file)
{
    __RSGenericValidInstance(file, _RSFileWrapperTypeID);
    if (file->_fstat.st_mode & S_IFDIR)
    {
        return YES;
    }
    return NO;
}

RSExport BOOL RSFileWrapperIsSymbolicLink(RSFileWrapperRef file)
{
    __RSGenericValidInstance(file, _RSFileWrapperTypeID);
    if (file->_fstat.st_mode & S_IFLNK)
    {
        return YES;
    }
    return NO;
}


static RSFileWrapperRef __RSFileWrapperCreateInstance(RSAllocatorRef allocator,  RSStringRef path)
{
    __RSFileWrapperRef fw = nil;
    if (path == nil ||
        RSStringGetCStringPtr(path, __RSDefaultEightBitStringEncoding) == nil ||
        RSStringGetLength(path) == 0) return fw;
    struct stat __fstat ;
    if (0 == lstat(RSStringGetCStringPtr(path, __RSDefaultEightBitStringEncoding), &__fstat))
    {
        fw = (__RSFileWrapperRef)__RSRuntimeCreateInstance(allocator, RSFileWrapperGetTypeID(), sizeof(struct __RSFileWrapper) - sizeof(RSRuntimeBase));
        fw->_filePath = RSCopy(allocator, path);
        fw->_fstat = __fstat;
    }
    
    return fw;
}

RSExport RSFileWrapperRef RSFileWrapperCreateAtPath(RSStringRef path)
{
    return __RSFileWrapperCreateInstance(RSAllocatorSystemDefault, path);
}
#pragma mark RSFileWrapper End

#pragma mark -
#pragma mark RSFileManager API

struct __RSFileManager
{
    RSRuntimeBase _base;
    RSStringRef _fileSystemName;
    RSStringRef _currentPath;
    RSMutableDictionaryRef _fds;
    RSMutableSetRef _fs;
    
};

typedef struct __RSFileManager* __RSFileManagerRef;
static RSSpinLock __RSFileManagerDefaultLock = RSSpinLockInit;
static __RSFileManagerRef __RSFileManagerDefault = nil;

static RSStringRef __RSFileManagerGetCurrentWorkDirectory()
{
    char* cwd = nil;
    size_t capacity = 256;
    if ((cwd = RSAllocatorAllocate(RSAllocatorSystemDefault, capacity)))
    {
        size_t len = strlen(getcwd(cwd, capacity));
        while (capacity == len)
        {
            capacity *= 2;
            cwd = RSAllocatorReallocate(RSAllocatorSystemDefault, cwd, capacity);
            len = strlen(getcwd(cwd, capacity));
        }
        RSStringRef path = RSStringCreateWithCStringNoCopy(RSAllocatorSystemDefault, cwd, __RSDefaultEightBitStringEncoding, RSAllocatorSystemDefault);
        return path;
    }
    return nil;
}

static void __RSFileManagerClassInit(RSTypeRef rs)
{
    __RSFileManagerRef fmg = (__RSFileManagerRef)rs;
    __RSRuntimeSetInstanceSpecial(rs, YES);
    fmg->_currentPath = __RSFileManagerGetCurrentWorkDirectory();
    fmg->_fileSystemName = RSSTR("APPLE HFS+");
}

static RSTypeRef __RSFileManagerClassCopy(RSAllocatorRef allocator, RSTypeRef rs, BOOL mutableCopy)
{
    return rs;
}

static void __RSFileManagerClassDeallocate(RSTypeRef rs)
{
    if (rs != __RSFileManagerDefault) HALTWithError(RSGenericException, "the file manager is not verified");
    
    __RSFileManagerRef fmg = (__RSFileManagerRef)rs;
    
    if (fmg->_fileSystemName) RSRelease(fmg->_fileSystemName);
    if (fmg->_currentPath) RSRelease(fmg->_currentPath);
    if (fmg->_fds) RSRelease(fmg->_fds);
    if (fmg->_fs) RSRelease(fmg->_fs);
}

static BOOL __RSFileManagerClassEqual(RSTypeRef rs1, RSTypeRef rs2)
{
    return rs1 == rs2;
}

static RSHashCode __RSFileManagerClassHash(RSTypeRef rs)
{
    return RSHash(((__RSFileManagerRef)rs)->_currentPath);
}

static RSStringRef __RSFileManagerClassDescription(RSTypeRef rs)
{
    return RSCopy(RSAllocatorSystemDefault,((__RSFileManagerRef)rs)->_currentPath);
}

static RSRuntimeClass __RSFileManagerClass =
{
    _RSRuntimeScannedObject,
    "RSFileManager",
    __RSFileManagerClassInit,
    __RSFileManagerClassCopy,
    __RSFileManagerClassDeallocate,
    __RSFileManagerClassEqual,
    __RSFileManagerClassHash,
    __RSFileManagerClassDescription,
    nil,
    nil
};

static RSTypeID _RSFileManagerTypeID = _RSRuntimeNotATypeID;
RSStringRef _RSTemporaryDirectory = nil;

RSPrivate void __RSFileManagerInitialize()
{
    _RSFileManagerTypeID = __RSRuntimeRegisterClass(&__RSFileManagerClass);
    __RSRuntimeSetClassTypeID(&__RSFileManagerClass, _RSFileManagerTypeID);
}

RSPrivate void __RSFileManagerDeallocate()
{
    RSSyncUpdateBlock(&__RSFileManagerDefaultLock, ^{
        if (_RSTemporaryDirectory) {
            __RSRuntimeSetInstanceSpecial(_RSTemporaryDirectory, NO);
            RSAutorelease(_RSTemporaryDirectory); // maybe use in the later deallocate routines, so just autorelease it here (RSAutoreleasePool is the last deallocate routine).
            _RSTemporaryDirectory = nil;
        }
        if (__RSFileManagerDefault) {
            __RSRuntimeSetInstanceSpecial(__RSFileManagerDefault, NO);
            RSRelease(__RSFileManagerDefault);
            __RSFileManagerDefault = nil;
        }
    });
    return;
}

static RSStringRef ___RSFileManagerGetFilePathDotOperation(RSFileManagerRef fmg)
{
    if (fmg == RSFileManagerGetDefault()) return RSSTR(".");
    return RSStringGetEmptyString();
}

static RSStringRef ___RSFileManagerGetFilePathDelimiterOperation(RSFileManagerRef fmg)
{
    if (fmg == RSFileManagerGetDefault()) return RSSTR("/");
    return RSStringGetEmptyString();
}

RSExport RSTypeID RSFileManagerGetTypeID()
{
    if (unlikely(_RSFileManagerTypeID == _RSRuntimeNotATypeID)) __RSFileManagerInitialize();
    return _RSFileManagerTypeID;
}

static RSFileManagerRef __RSFileManagerCreateInstance(RSAllocatorRef allocator)
{
    __RSFileManagerDefault = (__RSFileManagerRef)__RSRuntimeCreateInstance(allocator, RSFileManagerGetTypeID(), sizeof(struct __RSFileManager) - sizeof(RSRuntimeBase));
    __RSRuntimeSetInstanceSpecial(__RSFileManagerDefault, YES);
    return __RSFileManagerDefault;
}

RSExport RSFileManagerRef RSFileManagerGetDefault()
{
    RSSyncUpdateBlock(&__RSFileManagerDefaultLock, ^{
        if (!__RSFileManagerDefault) {
           __RSFileManagerCreateInstance(RSAllocatorSystemDefault);
        }
    });
    return __RSFileManagerDefault;
}

RSExport RSStringRef __RSFileManagerStandardizingPath(RSStringRef path)
{
//    if (path == nil || RSStringGetLength(path) == 0 || RSStringGetLength(path) + 64 > RSBufferSize) return nil;
//    char *buf = RSAllocatorAllocate(RSAllocatorSystemDefault, RSBufferSize / 2 > RSStringGetLength(path) ? RSStringGetLength(path) * 2 + 1: RSBufferSize);
//    if (realpath(RSStringGetCStringPtr(path, __RSDefaultEightBitStringEncoding), buf)) {
//        return RSStringCreateWithCStringNoCopy(RSAllocatorSystemDefault, buf, RSStringEncodingUTF8, RSAllocatorSystemDefault);
//    }
//    RSAllocatorDeallocate(RSAllocatorSystemDefault, buf);
//    return nil;
    RSMutableStringRef returnPath = nil;
    if (RSStringHasPrefix(path, _RSFileSystemSymbolRootPath))
    {
        returnPath = RSMutableCopy(RSAllocatorSystemDefault, path);
    }
    else if (RSStringGetLength(path) >= 2 && RSStringHasPrefix(path, _RSFileSystemSymbolUserPath))
    {
        RSMutableStringRef _standardizingPath = RSStringCreateMutable(RSAllocatorSystemDefault, 0);
        RSStringAppendCString(_standardizingPath, __RSRuntimeGetEnvironment("HOME"), __RSDefaultEightBitStringEncoding);
        const char *cp = RSStringGetCStringPtr(path, __RSDefaultEightBitStringEncoding);
        RSStringAppendCString(_standardizingPath, cp+1, __RSDefaultEightBitStringEncoding);
        returnPath = _standardizingPath;
    }
    BOOL isDir = NO;
    RSFileManagerRef fmg = RSFileManagerGetDefault();
    if (YES == RSFileManagerFileExistsAtPath(fmg, returnPath, &isDir))
    {
        if (isDir)
        {
            if (NO == RSStringHasSuffix(returnPath, ___RSFileManagerGetFilePathDelimiterOperation(fmg)))
            {
                RSStringAppendString(returnPath, ___RSFileManagerGetFilePathDelimiterOperation(fmg));
            }
        }
    }
    return returnPath;
}

RSExport __autorelease RSStringRef RSFileManagerStandardizingPath(RSStringRef path)
{
    return RSAutorelease(__RSFileManagerStandardizingPath(path));
}

RSExport __autorelease RSArrayRef RSFileManagerContentsOfDirectory(RSFileManagerRef fmg, RSStringRef path, __autorelease RSErrorRef* error)
{
    if (fmg == nil || path == nil) return nil;
    __RSGenericValidInstance(fmg, _RSFileManagerTypeID);
    struct __RSFileManagerContentsContext context = {0};
    RSStringRef standardizingPath = (path);
    BOOL success = __RSFileManagerContentsDirectory(fmg, standardizingPath, error, &context, NO);
    RSArrayRef contents = nil;
    if (success)
    {
#if __RSFileManagerContentsContextDoNotUseRSArray
        contents = RSArrayCreateWithObjects(RSAllocatorSystemDefault, (RSTypeRef*)context.spaths, context.numberOfContents);
#else
        contents = RSRetain(context.contain);
#endif
        __RSFileManagerContentContextFree(&context);
        
    }
    return contents ? RSAutorelease(contents) : nil;
}

RSExport __autorelease RSArrayRef RSFileManagerSubpathsOfDirectory(RSFileManagerRef fmg, RSStringRef path, __autorelease RSErrorRef* error)
{
    if (fmg == nil || path == nil) return nil;
    __RSGenericValidInstance(fmg, _RSFileManagerTypeID);
    struct __RSFileManagerContentsContext context = {0};
    RSStringRef standardizingPath = (path);
    BOOL success = __RSFileManagerContentsDirectory(fmg, standardizingPath, error, &context, YES);
    RSArrayRef contents = nil;
    if (success)
    {
#if __RSFileManagerContentsContextDoNotUseRSArray
        contents = RSArrayCreateWithObjects(RSAllocatorSystemDefault, (RSTypeRef*)context.spaths, context.numberOfContents);
#else
        contents = RSRetain(context.contain);
#endif
        __RSFileManagerContentContextFree(&context);
        
    }
    return contents ? RSAutorelease(contents) : nil;
}

RSExport __autorelease RSDataRef RSFileManagerContentsAtPath(RSFileManagerRef fmg, RSStringRef path)
{
    if (fmg == nil || path == nil) return nil;
    __RSGenericValidInstance(fmg, _RSFileManagerTypeID);
    RSStringRef standardizingPath = (path);
    RSFileHandleRef fh = RSFileHandleCreateForReadingAtPath(standardizingPath);
    RSDataRef data = RSFileHandleReadDataToEndOfFile(fh);
    RSRelease(fh);
    return RSAutorelease(data);
}

RS_PUBLIC_CONST_STRING_DECL(RSFileCreationDate, "RSFileCreationDate");
RS_PUBLIC_CONST_STRING_DECL(RSFileAccessDate, "RSFileAccessDate");
RS_PUBLIC_CONST_STRING_DECL(RSFileExtensionHidden, "RSFileExtensionHidden");
RS_PUBLIC_CONST_STRING_DECL(RSFileGroupOwnerAccountID, "RSFileGroupOwnerAccountID");
RS_PUBLIC_CONST_STRING_DECL(RSFileGroupOwnerAccountName, "RSFileGroupOwnerAccountName");
RS_PUBLIC_CONST_STRING_DECL(RSFileModificationDate, "RSFileModificationDate");
RS_PUBLIC_CONST_STRING_DECL(RSFileOwnerAccountID, "RSFileOwnerAccountID");
RS_PUBLIC_CONST_STRING_DECL(RSFileOwnerAccountName, "RSFileOwnerAccountName");
RS_PUBLIC_CONST_STRING_DECL(RSFilePosixPermissions, "RSFilePosixPermissions");
RS_PUBLIC_CONST_STRING_DECL(RSFileReferenceCount, "RSFileReferenceCount");
RS_PUBLIC_CONST_STRING_DECL(RSFileSize, "RSFileSize");
RS_PUBLIC_CONST_STRING_DECL(RSFileSystemFileNumber, "RSFileSystemFileNumber");
RS_PUBLIC_CONST_STRING_DECL(RSFileSystemNumber, "RSFileSystemNumber");
RS_PUBLIC_CONST_STRING_DECL(RSFileType, "RSFileType");

RSInline RSGregorianDate __RSDateTranslateFromTM(struct tm local)
{
    struct tm _localModify = local;
    RSGregorianDate gd = {0};
    gd.year = _localModify.tm_year + 1900;
    gd.month = _localModify.tm_mon;
    gd.day = _localModify.tm_mday;
    gd.hour = _localModify.tm_hour;
    gd.minute = _localModify.tm_min;
    gd.second = _localModify.tm_sec;
    return gd;
}

static unsigned int __RSFilePosixPermissions(struct stat _status)
{
    unsigned int _posixPermissions = 0;
    {
        _posixPermissions += (_status.st_mode & S_IRUSR) ? 400 : 0;
        _posixPermissions += (_status.st_mode & S_IWUSR) ? 200 : 0;
        _posixPermissions += (_status.st_mode & S_IXUSR) ? 100 : 0;
        
        _posixPermissions += (_status.st_mode & S_IRGRP) ? 40 : 0;
        _posixPermissions += (_status.st_mode & S_IWGRP) ? 20 : 0;
        _posixPermissions += (_status.st_mode & S_IXGRP) ? 10 : 0;
        
        _posixPermissions += (_status.st_mode & S_IROTH) ? 4 : 0;
        _posixPermissions += (_status.st_mode & S_IWOTH) ? 2 : 0;
        _posixPermissions += (_status.st_mode & S_IXOTH) ? 1 : 0;
    }
    return _posixPermissions;
}

RSExport __autorelease RSDictionaryRef RSFileManagerAttributesOfFileAtPath(RSFileManagerRef fmg, RSStringRef path)
{
    if (fmg == nil || path == nil) return nil;
    __RSGenericValidInstance(fmg, _RSFileManagerTypeID);
    RSDictionaryRef attributes = nil;
    RSStringRef standardizingPath = (path);
    struct stat _status;
    if (kSuccess == lstat(RSStringGetCStringPtr(standardizingPath, __RSDefaultEightBitStringEncoding), &_status))
    {
        struct passwd _passwd = {0};
        memcpy(&_passwd, getpwuid(_status.st_uid), sizeof(struct passwd));
        endpwent();
        
        struct group _group = {0};
        memcpy(&_group, getgrgid(_status.st_gid), sizeof(struct group));
        endgrent();
        
        unsigned int _posixPermissions = __RSFilePosixPermissions(_status);
        unsigned int _rc = _status.st_nlink;
        struct tm _local = {0};
        memcpy(&_local, localtime(&_status.st_birthtimespec.tv_sec), sizeof(struct tm));
        RSGregorianDate gd =  __RSDateTranslateFromTM(_local);
        memset(&_local, 0, sizeof(struct tm));
        RSDateRef createDate = RSDateCreate(RSAllocatorSystemDefault, RSGregorianDateGetAbsoluteTime(gd, nil));
        
        memcpy(&_local, localtime(&_status.st_mtimespec.tv_sec), sizeof(struct tm));
        gd = __RSDateTranslateFromTM(_local);
        RSDateRef modificationDate = RSDateCreate(RSAllocatorSystemDefault, RSGregorianDateGetAbsoluteTime(gd, nil));
        memset(&_local, 0, sizeof(struct tm));
        
        memcpy(&_local, localtime(&_status.st_atimespec.tv_sec), sizeof(struct tm));
        gd = __RSDateTranslateFromTM(_local);
        RSDateRef accessDate = RSDateCreate(RSAllocatorSystemDefault, RSGregorianDateGetAbsoluteTime(gd, nil));
        memset(&_local, 0, sizeof(struct tm));
        
        RSNumberRef groupOwnerID = RSNumberCreateUnsignedInteger(RSAllocatorSystemDefault, _status.st_gid);
        RSStringRef groupOwnerName = RSStringCreateWithCString(RSAllocatorSystemDefault, _group.gr_name, __RSDefaultEightBitStringEncoding);
        RSNumberRef ownerAccountID = RSNumberCreateUnsignedInteger(RSAllocatorSystemDefault, _status.st_uid);
        RSStringRef ownerAccountName = RSStringCreateWithCString(RSAllocatorSystemDefault, _passwd.pw_name, __RSDefaultEightBitStringEncoding);
        RSNumberRef posixPermissions = RSNumberCreateUnsignedInteger(RSAllocatorSystemDefault, _posixPermissions);
        RSNumberRef referenceCount = RSNumberCreateUnsignedInteger(RSAllocatorSystemDefault, _rc);
        RSNumberRef fileSize = RSNumberCreateUnsignedLonglong(RSAllocatorSystemDefault, _status.st_size);
        RSNumberRef systemNumber = RSNumberCreateUnsignedInteger(RSAllocatorSystemDefault, _status.st_dev);
        RSNumberRef systemFileNumber = RSNumberCreateUnsignedLonglong(RSAllocatorSystemDefault, _status.st_ino);
        attributes = RSDictionaryCreateWithObjectsAndOKeys(RSAllocatorSystemDefault,
                                                           createDate, RSFileCreationDate,
                                                           modificationDate, RSFileModificationDate,
                                                           accessDate, RSFileAccessDate,
                                                           groupOwnerID, RSFileGroupOwnerAccountID,
                                                           groupOwnerName, RSFileGroupOwnerAccountName,
                                                           ownerAccountID, RSFileOwnerAccountID,
                                                           ownerAccountName, RSFileOwnerAccountName,
                                                           posixPermissions, RSFilePosixPermissions,
                                                           referenceCount, RSFileReferenceCount,
                                                           fileSize, RSFileSize,
                                                           systemFileNumber, RSFileSystemFileNumber,
                                                           systemNumber, RSFileSystemNumber,
                                                           nil);
        RSRelease(createDate);
        RSRelease(modificationDate);
        RSRelease(accessDate);
        RSRelease(groupOwnerID);
        RSRelease(groupOwnerName);
        RSRelease(ownerAccountID);
        RSRelease(ownerAccountName);
        RSRelease(posixPermissions);
        RSRelease(referenceCount);
        RSRelease(fileSize);
        RSRelease(systemFileNumber);
        RSRelease(systemNumber);
    }
    return attributes ? RSAutorelease(attributes) : nil;
}

RSExport BOOL RSFileManagerCopyFileToPath(RSFileManagerRef fmg, RSStringRef from, RSStringRef to)
{
    if (fmg == nil || from == nil || to == nil) return NO;
    __RSGenericValidInstance(fmg, _RSFileManagerTypeID);
    BOOL result = NO;
    if (RSFileManagerFileExistsAtPath(fmg, from, &result) && result == NO && RSFileManagerIsReadableFileAtPath(fmg, from))
    {
    
//        RSFileHandleRef readHandle = RSFileHandleCreateForReadingAtPath(from);
//        RSFileHandleRef writeHandle = RSFileHandleCreateForWritingAtPath(to);
//        
//        if (readHandle && writeHandle)
//        {
//            RSFileSizeX fileSize = RSFileHandleGetFileSize(readHandle);
//            static const RSFileSizeX step = 20*1024*1024*8;
//            RSDataRef data = nil;
//            while (result && fileSize > step)
//            {
//                data = RSFileHandleReadDataOfLength(readHandle, step);
//                result = RSFileHandleWriteData(writeHandle, data);
//                RSRelease(data);
//                fileSize -= step;
//            }
//            
//            if (result && fileSize && fileSize < step)
//            {
//                data = RSFileHandleReadDataToEndOfFile(readHandle);
//                RSFileHandleWriteData(writeHandle, data);
//                RSRelease(data);
//            }
//        }
//        
//        RSRelease(readHandle);
//        RSRelease(writeHandle);
        const static char* _cmd = "cp";
        //RSStringRef cmd = RSStringCreateWithFormat(RSAllocatorSystemDefault, RSSTR("%s%R %R"), _cmd, from, to);
        if (-1 == __RSPrivateRunShell(_cmd, RSSTR("\"%R\" \"%R\""), from, to))
            result = NO;
        else result = YES;
    }
    return result;
}

RSExport BOOL RSFileManagerMoveFileToPath(RSFileManagerRef fmg, RSStringRef from, RSStringRef to)
{
    BOOL result = RSFileManagerIsDeletableFileAtPath(fmg, from);
    if (result)
    {
//        result = RSFileManagerCopyFileToPath(fmg, from, to);
        BOOL result = NO;
        if (RSFileManagerFileExistsAtPath(fmg, from, &result) && result == NO && RSFileManagerIsReadableFileAtPath(fmg, from))
        {
            const static char *_cmd = "mv";
            if (-1 == __RSPrivateRunShell(_cmd, RSSTR("\"%r\" \"%r\""), from, to))
                result = NO;
            else
                result = YES;
            
        }
//        if (result)
//        {
//            result = _RSRemoveFile(RSStringGetCStringPtr(from));
//        }
    }
    return result;
}

RSExport BOOL RSFileManagerRemoveFile(RSFileManagerRef fmg, RSStringRef fileToDelete)
{
    BOOL result = RSFileManagerIsDeletableFileAtPath(fmg, fileToDelete);
    if (result)
    {
        result = _RSRemoveFile(RSStringGetCStringPtr(fileToDelete, __RSDefaultEightBitStringEncoding));
    }
    return result;
}

RSExport __autorelease RSDictionaryRef RSFileManagerAttributesOfDirectoryAtPath(RSFileManagerRef fmg, RSStringRef dir)
{
    BOOL isDir = NO;
    if (RSFileManagerFileExistsAtPath(fmg, dir, &isDir) && isDir)
        return RSFileManagerAttributesOfFileAtPath(fmg, dir);
    return RSAutorelease(RSDictionaryCreateWithObjectsAndOKeys(RSAllocatorSystemDefault, nil));
}

RSExport BOOL RSFileManagerCopyDirctoryToPath(RSFileManagerRef fmg, RSStringRef from, RSStringRef to)
{
    BOOL isDir = NO;
    BOOL result = NO;
    if ((result = RSFileManagerFileExistsAtPath(fmg, from, &isDir)))
    {
        if (RSFileManagerFileExistsAtPath(fmg, to, &isDir))
        {
            return NO;
        }
        const static char* _cmd = "cp";
        //if (-1 == execl("/bin/cp", "/bin/cp", "-r", RSStringGetCStringPtr(from), RSStringGetCStringPtr(to), nil))
        if (-1 == __RSPrivateRunShell(_cmd, RSSTR("%s \"%R\" \"%R\""), "-r", from, to))
            result = NO;
        else result = YES;
    }
    return result;
}

RSExport BOOL RSFileManagerMoveDirctoryToPath(RSFileManagerRef fmg, RSStringRef from, RSStringRef to)
{
    BOOL isDir = NO;
    BOOL result = NO;
    if ((result = RSFileManagerFileExistsAtPath(fmg, from, &isDir)) && isDir)
    {
        if (RSFileManagerFileExistsAtPath(fmg, to, &isDir)) return NO;
        const static char* _cmd = "mv";
        if (-1 == __RSPrivateRunShell(_cmd, RSSTR("\"%R\" \"%R\""), from, to))
            result = NO;
        else result = YES;
    }
    return result;
}

RSExport BOOL RSFileManagerRemoveDirectory(RSFileManagerRef fmg, RSStringRef dirToDelete)
{
    BOOL isDir = NO;
    BOOL result = NO;
    if ((result = RSFileManagerFileExistsAtPath(fmg, dirToDelete, &isDir)) && isDir)
    {
        const static char* _cmd = "rm -rf";
        if (-1 == __RSPrivateRunShell(_cmd, RSSTR("\"%R\""), dirToDelete))
            result = NO;
        else result = YES;
    }
    return result;
}

RSExport BOOL RSFileManagerCreateDirectoryAtPath(RSFileManagerRef fmg, RSStringRef dirPath)
{
    BOOL isDir = NO;
    BOOL result = RSFileManagerFileExistsAtPath(fmg, dirPath, &isDir);
    if (result && isDir) return YES; // directory is already existing.
    return _RSCreateDirectory(RSStringGetCStringPtr(dirPath, __RSDefaultEightBitStringEncoding));
//    const static char* _cmd = "mkdir";
//    if (-1 == __RSPrivateRunShell(_cmd, RSSTR("%R"), dirPath))
//        result = NO;
//    else result = YES;
//    return result;
}

RSExport BOOL RSFileManagerFileExistsAtPath(RSFileManagerRef fmg, RSStringRef path, BOOL* isDirectory)
{
    if (fmg == nil || path == nil) return NO;
    __RSGenericValidInstance(fmg, _RSFileManagerTypeID);
    RSStringRef standardizingPath = path;
    BOOL result = NO;
    result = _RSFileExisting(RSStringGetCStringPtr(standardizingPath, __RSDefaultEightBitStringEncoding));
    if (result && isDirectory)
    {
        *isDirectory = NO;
        struct stat _status;
        if (stat(RSStringGetCStringPtr(standardizingPath, __RSDefaultEightBitStringEncoding), &_status) == 0)
        {
            *isDirectory = S_ISDIR(_status.st_mode);
        }
    }
    return result;
}

RSExport BOOL RSFileManagerIsReadableFileAtPath(RSFileManagerRef fmg, RSStringRef path)
{
    if (fmg == nil || path == nil) return NO;
    __RSGenericValidInstance(fmg, _RSFileManagerTypeID);
    RSStringRef standardizingPath = (path);
    BOOL result = NO;
    result = (0 == access(RSStringGetCStringPtr(standardizingPath, __RSDefaultEightBitStringEncoding), R_OK)) ? YES : NO;
    return result;
}

RSExport BOOL RSFileManagerIsWritableFileAtPath(RSFileManagerRef fmg, RSStringRef path)
{
    if (fmg == nil || path == nil) return NO;
    __RSGenericValidInstance(fmg, _RSFileManagerTypeID);
    RSStringRef standardizingPath = (path);
    BOOL result = NO;
    result = (0 == access(RSStringGetCStringPtr(standardizingPath, __RSDefaultEightBitStringEncoding), W_OK)) ? YES : NO;
    return result;
}

RSExport BOOL RSFileManagerIsExecutableFileAtPath(RSFileManagerRef fmg, RSStringRef path)
{
    if (fmg == nil || path == nil) return NO;
    __RSGenericValidInstance(fmg, _RSFileManagerTypeID);
    RSStringRef standardizingPath = (path);
    BOOL result = NO;
    result = (0 == access(RSStringGetCStringPtr(standardizingPath, __RSDefaultEightBitStringEncoding), X_OK)) ? YES : NO;
    return result;
}

RSExport BOOL RSFileManagerIsDeletableFileAtPath(RSFileManagerRef fmg, RSStringRef path)
{
    if (fmg == nil || path == nil) return NO;
    __RSGenericValidInstance(fmg, _RSFileManagerTypeID);
    RSStringRef standardizingPath = (path);
    BOOL result = NO;
    result = (0 == access(RSStringGetCStringPtr(standardizingPath, __RSDefaultEightBitStringEncoding), _RMFILE_OK)) ? YES : NO;
    return result;
}

RSExport __autorelease RSStringRef RSFileManagerFileExtension(RSFileManagerRef fmg, RSStringRef path)
{
    __RSGenericValidInstance(fmg, _RSFileManagerTypeID);
    RSRange result = RSMakeRange(RSNotFound, RSNotFound);
    RSRange current = result;
    RSIndex length = RSStringGetLength(path);
    RSRange search = RSMakeRange(0, length);
    BOOL isHasPoint = NO;
    while (RSStringFind(path, ___RSFileManagerGetFilePathDotOperation(fmg), search, &current))
    {
        isHasPoint = YES;
        // update search filed
        result = current;
        search = RSMakeRange(search.location + result.location + result.length,
                             length - search.location - result.location - result.length);
    }
    if (isHasPoint) {
        return RSAutorelease(RSStringCreateSubStringWithRange(RSAllocatorSystemDefault, path, search));
    }
    return RSStringGetEmptyString();
}

RSExport __autorelease RSStringRef RSFileManagerFileFullName(RSFileManagerRef fmg, RSStringRef path)
{
    __RSGenericValidInstance(fmg, _RSFileManagerTypeID);
    RSRange result = RSMakeRange(RSNotFound, RSNotFound);
    RSRange current = result;
    RSIndex length = RSStringGetLength(path);
    RSRange search = RSMakeRange(0, length);
    BOOL isHasSlash = NO;
    while (RSStringFind(path, ___RSFileManagerGetFilePathDelimiterOperation(fmg), search, &current))
    {
        isHasSlash = YES;
        result = current;
        search = RSMakeRange(search.location + result.location + result.length, length - search.location - result.location - result.length);
    }
    if (isHasSlash)
    {
        return RSAutorelease(RSStringCreateSubStringWithRange(RSAllocatorSystemDefault, path, search));
    }
    return path ? RSAutorelease(RSRetain(path)) : RSStringGetEmptyString();
}

RSExport __autorelease RSStringRef RSFileManagerFileName(RSFileManagerRef fmg, RSStringRef path)
{
    RSMutableStringRef name = RSMutableCopy(RSAllocatorSystemDefault, path);
    RSStringRef extension = RSFileManagerFileExtension(fmg, path);
    if (extension && RSStringGetLength(extension) > 0) {
        RSStringDelete(name, RSMakeRange(RSStringGetLength(name) - RSStringGetLength(extension) - 1, RSStringGetLength(extension) + 1));
//        BUG : RSFileManagerFileExtension return autorelease object, should not release it 
//        RSRelease(extension);
    }
    RSIndex numberOfResults = 0;
    RSRange *rangesPtr = RSStringFindAll(name, RSSTR("/"), &numberOfResults);
    if (rangesPtr) {
        RSRange range = rangesPtr[numberOfResults - 1];
        RSIndex offset = range.location + range.length;
        RSStringDelete(name, RSMakeRange(0, offset));
        RSAllocatorDeallocate(RSAllocatorSystemDefault, rangesPtr);
    }
    return name ? RSAutorelease(name) : nil;
}

RSExport RSMutableStringRef RSFileManagerAppendFileName(RSFileManagerRef fmg, RSMutableStringRef parent, RSStringRef name)
{
    if (fmg == nil || parent == nil || name == nil) return nil;
    __RSGenericValidInstance(fmg, _RSFileManagerTypeID);
    if (NO == RSStringHasSuffix(parent, ___RSFileManagerGetFilePathDelimiterOperation(fmg)))
        RSStringAppendString(parent, ___RSFileManagerGetFilePathDelimiterOperation(fmg));
    RSStringAppendString(parent, name);
    return parent;
}

#pragma mark -
#pragma mark RSFileManager FileWatcher

#if DEPLOYMENT_TARGET_MACOSX
#include <sys/event.h>

typedef BOOL (*RSFileWatcherCallBack)(RSFileManagerRef fileManager, RSFileHandleRef handle, RSDictionaryRef property);
typedef RSFileHandle RSEventQueue;
struct __RSFileWatcher
{
    RSRuntimeBase _base;
    RSMutableArrayRef _paths;       //RSStringRefs
    RSEventQueue _queue;            //kqueue
    RSFileWatcherCallBack _delegate;//callback
};
typedef struct __RSFileWatcher* RSFileWatcherRef;
static void __RSFileWatcherClassInit(RSTypeRef rs)
{
    RSFileWatcherRef wt = (RSFileWatcherRef)rs;
    wt->_queue = kqueue();
}

static void __RSFileWatcherClassDeallocate(RSTypeRef rs)
{
    RSFileWatcherRef wt = (RSFileWatcherRef)rs;
    if (wt->_queue != -1) close(wt->_queue);
    wt->_queue = -1;
}

static RSTypeRef __RSFileWatcherClassCopy(RSAllocatorRef allocator, RSTypeRef rs, BOOL mutableCopy)
{
    return nil;
}

static BOOL __RSFileWatcherClassEqual(RSTypeRef rs1, RSTypeRef rs2)
{
    return RSEqual(((RSFileWatcherRef)rs1)->_paths, ((RSFileWatcherRef)rs2)->_paths);
}

static RSStringRef __RSFileWatcherClassDescription(RSTypeRef rs)
{
    RSFileWatcherRef wt = (RSFileWatcherRef)rs;
    RSStringRef description = RSStringCreateWithFormat(RSAllocatorSystemDefault, RSSTR("RSFileWatcher watch files = %R\n"), wt->_paths);
    return description;
}

static RSRuntimeClass __RSFileWatcherClass =
{
    _RSRuntimeScannedObject,
    "RSFileWatcher",
    __RSFileWatcherClassInit,
    __RSFileWatcherClassCopy,
    __RSFileWatcherClassDeallocate,
    __RSFileWatcherClassEqual,
    nil,
    __RSFileWatcherClassDescription,
    nil,
    nil
};

static RSTypeID _RSFileWatcherTypeID = _RSRuntimeNotATypeID;

RSPrivate void __RSFileWatcherInitialize()
{
    _RSFileWatcherTypeID = __RSRuntimeRegisterClass(&__RSFileWatcherClass);
    __RSRuntimeSetClassTypeID(&__RSFileWatcherClass, _RSFileWatcherTypeID);
}

RSExport RSTypeID RSFileWatcherGetTypeID()
{
    if (unlikely(_RSFileWatcherTypeID == _RSRuntimeNotATypeID))
    {
        __RSFileWatcherInitialize();
    }
    return _RSFileWatcherTypeID;
}
/*
 typedef struct {
     RSIndex	version;
     void *	info;
     const void *(*retain)(const void *info);
     void	(*release)(const void *info);
     RSStringRef	(*description)(const void *info);
     BOOL	(*equal)(const void *info1, const void *info2);
     RSHashCode	(*hash)(const void *info);
     void	(*schedule)(void *info, RSRunLoopRef runloop, RSStringRef mode); // call when source add to the RSRunLoop
     void	(*cancel)(void *info, RSRunLoopRef runloop, RSStringRef mode);   // call when source remove from the RSRunLoop
     void	(*perform)(void *info);
 } RSRunLoopSourceContext RS_AVAILABLE(0_0);
 
 */
static void __RSFWAddFDToQueue(RSFileWatcherRef fw, RSFileHandle fd, int mode, RSAbsoluteTime timeout)
{
    struct kevent event;
    EV_SET(&event, fd, EVFILT_VNODE,
           EV_ADD | EV_ENABLE | EV_ONESHOT,
           NOTE_DELETE | NOTE_EXTEND | NOTE_WRITE | NOTE_ATTRIB,
           0, 0);
    struct timespec _to ;
    _to.tv_sec = timeout;
    kevent(fw->_queue, &event, 1, nil, 0, &_to);
//    RSRunLoopSourceContext context = {};
//    RSRunLoopSourceRef source = RSRunLoopSourceCreate(RSAllocatorSystemDefault, 0, nil);
//    RSRunLoopAddSource(RSRunLoopGetCurrent(), source, RSRunLoopQueue);
//    RSRelease(source);
}

//RSExport void __RSFileWatcherCreateInstance(RSAllocatorRef allocator)
//{
//    RSFileWatcherRef fw = __RSRuntimeCreateInstance(allocator, RSFileWatcherGetTypeID(), sizeof(struct __RSFileWater) - sizeof(RSRuntimeBase));
//    return fw;
//}

RSExport void RSFileWatcherAddFileHandle(RSFileWatcherRef fileWatcher, RSFileHandleRef handle, int mode, RSAbsoluteTime timeout)
{
    __RSGenericValidInstance(fileWatcher, _RSFileWatcherTypeID);
    RSFileHandle fd = RSFileHandleGetHandle(handle);
    __RSFWAddFDToQueue(fileWatcher, fd, mode, timeout);
}

#endif
