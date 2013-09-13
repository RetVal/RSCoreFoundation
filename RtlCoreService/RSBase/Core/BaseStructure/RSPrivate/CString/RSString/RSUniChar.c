//
//  RSUniChar.c
//  RSCoreFoundation
//
//  Created by RetVal on 7/28/13.
//  Copyright (c) 2013 RetVal. All rights reserved.
//

#include <RSCoreFoundation/RSByteOrder.h>
#include "RSInternal.h"
#include "RSUniChar.h"
//#include "RSStringEncodingConverterExt.h" // #define MAX_DECOMPOSED_LENGTH (10)
#ifndef MAX_DECOMPOSED_LENGTH
#define MAX_DECOMPOSED_LENGTH (10)
#endif
#include "RSUnicodeDecomposition.h"
#include "RSUniCharPrivate.h"
#if DEPLOYMENT_TARGET_MACOSX || DEPLOYMENT_TARGET_EMBEDDED || DEPLOYMENT_TARGET_EMBEDDED_MINI || DEPLOYMENT_TARGET_LINUX || DEPLOYMENT_TARGET_FREEBSD
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/param.h>
#include <sys/mman.h>
#include <unistd.h>
#include <stdlib.h>
#endif
#if DEPLOYMENT_TARGET_MACOSX || DEPLOYMENT_TARGET_EMBEDDED
#include <mach/mach.h>
#endif

#if DEPLOYMENT_TARGET_WINDOWS
extern void _RSGetFrameworkPath(wchar_t *path, int maxLength);
#endif

#if DEPLOYMENT_TARGET_MACOSX || DEPLOYMENT_TARGET_EMBEDDED
#define __RSCharacterSetDir "/System/Library/CoreServices"
#elif DEPLOYMENT_TARGET_LINUX || DEPLOYMENT_TARGET_FREEBSD || DEPLOYMENT_TARGET_EMBEDDED_MINI
#define __RSCharacterSetDir "/usr/local/share/CoreFoundation"
#elif DEPLOYMENT_TARGET_WINDOWS
#define __RSCharacterSetDir "\\Windows\\CoreFoundation"
#endif

#if DEPLOYMENT_TARGET_MACOSX || DEPLOYMENT_TARGET_EMBEDDED
#define USE_MACHO_SEGMENT 1
#endif

enum {
    RSUniCharLastExternalSet = RSUniCharNewlineCharacterSet,
    RSUniCharFirstInternalSet = RSUniCharCompatibilityDecomposableCharacterSet,
    RSUniCharLastInternalSet = RSUniCharGraphemeExtendCharacterSet,
    RSUniCharFirstBitmapSet = RSUniCharDecimalDigitCharacterSet
};

RSInline uint32_t __RSUniCharMapExternalSetToInternalIndex(uint32_t cset) { return ((RSUniCharFirstInternalSet <= cset) ? ((cset - RSUniCharFirstInternalSet) + RSUniCharLastExternalSet) : cset) - RSUniCharFirstBitmapSet; }
RSInline uint32_t __RSUniCharMapCompatibilitySetID(uint32_t cset) { return ((cset == RSUniCharControlCharacterSet) ? RSUniCharControlAndFormatterCharacterSet : (((cset > RSUniCharLastExternalSet) && (cset < RSUniCharFirstInternalSet)) ? ((cset - RSUniCharLastExternalSet) + RSUniCharFirstInternalSet) : cset)); }

#if (DEPLOYMENT_TARGET_MACOSX || DEPLOYMENT_TARGET_EMBEDDED) && USE_MACHO_SEGMENT
#include <mach-o/getsect.h>
#include <mach-o/dyld.h>
#include <mach-o/ldsyms.h>

static const void *__RSGetSectDataPtr(const char *segname, const char *sectname, uint64_t *sizep) {
    uint32_t idx, cnt = _dyld_image_count();
    for (idx = 0; idx < cnt; idx++) {
        void *mh = (void *)_dyld_get_image_header(idx);
        if (mh != &_mh_dylib_header) continue;
#if __LP64__
        const struct section_64 *sect = getsectbynamefromheader_64((struct mach_header_64 *)mh, segname, sectname);
#else
        const struct section *sect = getsectbynamefromheader((struct mach_header *)mh, segname, sectname);
#endif
        if (!sect) break;
        if (sizep) *sizep = (uint64_t)sect->size;
        return (char *)sect->addr + _dyld_get_image_vmaddr_slide(idx);
    }
    if (sizep) *sizep = 0ULL;
    return NULL;
}
#endif

#if !USE_MACHO_SEGMENT

// Memory map the file

#if DEPLOYMENT_TARGET_MACOSX || DEPLOYMENT_TARGET_EMBEDDED || DEPLOYMENT_TARGET_EMBEDDED_MINI || DEPLOYMENT_TARGET_LINUX
RSInline void __RSUniCharCharacterSetPath(char *cpath)
{
#elif DEPLOYMENT_TARGET_WINDOWS
RSInline void __RSUniCharCharacterSetPath(wchar_t *wpath)
{
#else
#error Unknown or unspecified DEPLOYMENT_TARGET
#endif
#if DEPLOYMENT_TARGET_MACOSX || DEPLOYMENT_TARGET_EMBEDDED
    strlcpy(cpath, __RSCharacterSetDir, MAXPATHLEN);
#elif DEPLOYMENT_TARGET_LINUX
    strlcpy(cpath, __RSCharacterSetDir, MAXPATHLEN);
#elif DEPLOYMENT_TARGET_WINDOWS
    wchar_t frameworkPath[MAXPATHLEN];
    _RSGetFrameworkPath(frameworkPath, MAXPATHLEN);
    wcsncpy(wpath, frameworkPath, MAXPATHLEN);
    wcsncat(wpath, L"\\CoreFoundation.resources\\", MAXPATHLEN - wcslen(wpath));
#else
    strlcpy(cpath, __RSCharacterSetDir, MAXPATHLEN);
    strlcat(cpath, "/CharacterSets/", MAXPATHLEN);
#endif
}
    
#if DEPLOYMENT_TARGET_WINDOWS
#define MAX_BITMAP_STATE 512
//
//  If a string is placed into this array, then it has been previously
//  determined that the bitmap-file cannot be found.  Thus, we make
//  the assumption it won't be there in future calls and we avoid
//  hitting the disk un-necessarily.  This assumption isn't 100%
//  correct, as bitmap-files can be added.  We would have to re-start
//  the application in order to pick-up the new bitmap info.
//
//  We should probably re-visit this.
//
static wchar_t *mappedBitmapState[MAX_BITMAP_STATE];
static int __nNumStateEntries = -1;
CRITICAL_SECTION __bitmapStateLock = {0};

BOOL __GetBitmapStateForName(const wchar_t *bitmapName) {
    if (NULL == __bitmapStateLock.DebugInfo)
        InitializeCriticalSection(&__bitmapStateLock);
    EnterCriticalSection(&__bitmapStateLock);
    if (__nNumStateEntries >= 0) {
        for (int i = 0; i < __nNumStateEntries; i++) {
            if (wcscmp(mappedBitmapState[i], bitmapName) == 0) {
                LeaveCriticalSection(&__bitmapStateLock);
                return YES;
            }
        }
    }
    LeaveCriticalSection(&__bitmapStateLock);
    return NO;
}
void __AddBitmapStateForName(const wchar_t *bitmapName) {
    if (NULL == __bitmapStateLock.DebugInfo)
        InitializeCriticalSection(&__bitmapStateLock);
    EnterCriticalSection(&__bitmapStateLock);
    __nNumStateEntries++;
    mappedBitmapState[__nNumStateEntries] = (wchar_t *)malloc((lstrlenW(bitmapName)+1) * sizeof(wchar_t));
    lstrcpyW(mappedBitmapState[__nNumStateEntries], bitmapName);
    LeaveCriticalSection(&__bitmapStateLock);
}
#endif
    
#if DEPLOYMENT_TARGET_MACOSX || DEPLOYMENT_TARGET_EMBEDDED || DEPLOYMENT_TARGET_EMBEDDED_MINI || DEPLOYMENT_TARGET_LINUX
    static BOOL __RSUniCharLoadBytesFromFile(const char *fileName, const void **bytes, int64_t *fileSize) {
#elif DEPLOYMENT_TARGET_WINDOWS
static BOOL __RSUniCharLoadBytesFromFile(const wchar_t *fileName, const void **bytes, int64_t *fileSize) {
#else
#error Unknown or unspecified DEPLOYMENT_TARGET
#endif
#if DEPLOYMENT_TARGET_WINDOWS
    HANDLE bitmapFileHandle = NULL;
    HANDLE mappingHandle = NULL;
    
    if (__GetBitmapStateForName(fileName)) {
        // The fileName has been tried in the past, so just return NO
        // and move on.
        *bytes = NULL;
        return NO;
    }
    mappingHandle = OpenFileMappingW(FILE_MAP_READ, TRUE, fileName);
    if (NULL == mappingHandle) {
        if ((bitmapFileHandle = CreateFileW(fileName, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL)) == INVALID_HANDLE_VALUE) {
            // We tried to get the bitmap file for mapping, but it's not there.  Add to list of non-existant bitmap-files so
            // we don't have to try this again in the future.
            __AddBitmapStateForName(fileName);
            return NO;
        }
        mappingHandle = CreateFileMapping(bitmapFileHandle, NULL, PAGE_READONLY, 0, 0, NULL);
        CloseHandle(bitmapFileHandle);
        if (!mappingHandle) return NO;
    }
    
    *bytes = MapViewOfFileEx(mappingHandle, FILE_MAP_READ, 0, 0, 0, 0);
    
    if (NULL != fileSize) {
        MEMORY_BASIC_INFORMATION memoryInfo;
        
        if (0 == VirtualQueryEx(mappingHandle, *bytes, &memoryInfo, sizeof(memoryInfo))) {
            *fileSize = 0; // This indicates no checking. Is it right ?
        } else {
            *fileSize = memoryInfo.RegionSize;
        }
    }
    
    CloseHandle(mappingHandle);
    
    return (*bytes ? YES : NO);
#else
    struct stat statBuf;
    int fd = -1;
    
    if ((fd = open(fileName, O_RDONLY, 0)) < 0) {
        return NO;
    }
    if (fstat(fd, &statBuf) < 0 || (*bytes = mmap(0, statBuf.st_size, PROT_READ, MAP_PRIVATE, fd, 0)) == (void *)-1) {
        close(fd);
        return NO;
    }
    close(fd);
    
    if (NULL != fileSize) *fileSize = statBuf.st_size;
    
    return YES;
#endif
}
        
#endif // USE_MACHO_SEGMENT
        
#if DEPLOYMENT_TARGET_MACOSX || DEPLOYMENT_TARGET_EMBEDDED || DEPLOYMENT_TARGET_EMBEDDED_MINI || DEPLOYMENT_TARGET_LINUX
static BOOL __RSUniCharLoadFile(const char *bitmapName, const void **bytes, int64_t *fileSize) {
#elif DEPLOYMENT_TARGET_WINDOWS
static BOOL __RSUniCharLoadFile(const wchar_t *bitmapName, const void **bytes, int64_t *fileSize) {
#else
#error Unknown or unspecified DEPLOYMENT_TARGET
#endif
#if USE_MACHO_SEGMENT
    *bytes = __RSGetSectDataPtr("__UNICODE", bitmapName, NULL);
    
    if (NULL != fileSize) *fileSize = 0;
    
    return *bytes ? YES : NO;
#else
#if DEPLOYMENT_TARGET_MACOSX || DEPLOYMENT_TARGET_EMBEDDED || DEPLOYMENT_TARGET_EMBEDDED_MINI || DEPLOYMENT_TARGET_LINUX
    char cpath[MAXPATHLEN];
    __RSUniCharCharacterSetPath(cpath);
    strlcat(cpath, bitmapName, MAXPATHLEN);
    return __RSUniCharLoadBytesFromFile(cpath, bytes, fileSize);
#elif DEPLOYMENT_TARGET_WINDOWS
    wchar_t wpath[MAXPATHLEN];
    __RSUniCharCharacterSetPath(wpath);
    wcsncat(wpath, bitmapName, MAXPATHLEN);
    return __RSUniCharLoadBytesFromFile(wpath, bytes, fileSize);
#else
#error Unknown or unspecified DEPLOYMENT_TARGET
#endif
#endif
}

// Bitmap functions
RSInline BOOL isControl(UTF32Char theChar, uint16_t charset, const void *data) { // ISO Control
    return (((theChar <= 0x001F) || (theChar >= 0x007F && theChar <= 0x009F)) ? YES : NO);
}

RSInline BOOL isWhitespace(UTF32Char theChar, uint16_t charset, const void *data) { // Space
    return (((theChar == 0x0020) || (theChar == 0x0009) || (theChar == 0x00A0) || (theChar == 0x1680) || (theChar >= 0x2000 && theChar <= 0x200B) || (theChar == 0x202F) || (theChar == 0x205F) || (theChar == 0x3000)) ? YES : NO);
}

RSInline BOOL isNewline(UTF32Char theChar, uint16_t charset, const void *data) { // White space
    return (((theChar >= 0x000A && theChar <= 0x000D) || (theChar == 0x0085) || (theChar == 0x2028) || (theChar == 0x2029)) ? YES : NO);
}

RSInline BOOL isWhitespaceAndNewline(UTF32Char theChar, uint16_t charset, const void *data) { // White space
    return ((isWhitespace(theChar, charset, data) || isNewline(theChar, charset, data)) ? YES : NO);
}
            
#if USE_MACHO_SEGMENT
RSInline BOOL __RSSimpleFileSizeVerification(const void *bytes, int64_t fileSize) { return YES; }
#elif 1
// <rdar://problem/8961744> __RSSimpleFileSizeVerification is broken
static BOOL __RSSimpleFileSizeVerification(const void *bytes, int64_t fileSize) { return YES; }
#else
static BOOL __RSSimpleFileSizeVerification(const void *bytes, int64_t fileSize) {
    BOOL result = YES;
    
    if (fileSize > 0) {
        if ((sizeof(uint32_t) * 2) > fileSize) {
            result = NO;
        } else {
            uint32_t headerSize = RSSwapInt32BigToHost(*((uint32_t *)((char *)bytes + 4)));
            
            if ((headerSize < (sizeof(uint32_t) * 4)) || (headerSize > fileSize)) {
                result = NO;
            } else {
                const uint32_t *lastElement = (uint32_t *)(((uint8_t *)bytes) + headerSize) - 2;
                
                if ((headerSize + RSSwapInt32BigToHost(lastElement[0]) + RSSwapInt32BigToHost(lastElement[1])) > headerSize) result = NO;
            }
        }
    }
    
    if (!result) RSLog(RSLogLevelCritical, RSSTR("File size verification for Unicode database file failed."));
    
    return result;
}
#endif // USE_MACHO_SEGMENT

typedef struct {
    uint32_t _numPlanes;
    const uint8_t **_planes;
} __RSUniCharBitmapData;

static char __RSUniCharUnicodeVersionString[8] = {0, 0, 0, 0, 0, 0, 0, 0};

static uint32_t __RSUniCharNumberOfBitmaps = 0;
static __RSUniCharBitmapData *__RSUniCharBitmapDataArray = NULL;

static RSSpinLock __RSUniCharBitmapLock = RSSpinLockInit;
            
#if DEPLOYMENT_TARGET_MACOSX || DEPLOYMENT_TARGET_EMBEDDED || DEPLOYMENT_TARGET_EMBEDDED_MINI || DEPLOYMENT_TARGET_LINUX
#if !defined(RS_UNICHAR_BITMAP_FILE)
#if USE_MACHO_SEGMENT
#define RS_UNICHAR_BITMAP_FILE "__csbitmaps"
#else
#define RS_UNICHAR_BITMAP_FILE "/RSCharacterSetBitmaps.bitmap"
#endif
#endif
#elif DEPLOYMENT_TARGET_WINDOWS
#if !defined(RS_UNICHAR_BITMAP_FILE)
#define RS_UNICHAR_BITMAP_FILE L"RSCharacterSetBitmaps.bitmap"
#endif
#else
#error Unknown or unspecified DEPLOYMENT_TARGET
#endif
            
static BOOL __RSUniCharLoadBitmapData(void) {
    __RSUniCharBitmapData *array;
    uint32_t headerSize;
    uint32_t bitmapSize;
    int numPlanes;
    uint8_t currentPlane;
    const void *bytes;
    const void *bitmapBase;
    const void *bitmap;
    int idx, bitmapIndex;
    int64_t fileSize;
    
    RSSpinLockLock(&__RSUniCharBitmapLock);
    
    if (__RSUniCharBitmapDataArray || !__RSUniCharLoadFile(RS_UNICHAR_BITMAP_FILE, &bytes, &fileSize) || !__RSSimpleFileSizeVerification(bytes, fileSize)) {
        RSSpinLockUnlock(&__RSUniCharBitmapLock);
        return NO;
    }
    
    for (idx = 0;idx < 4 && ((const uint8_t *)bytes)[idx];idx++) {
        __RSUniCharUnicodeVersionString[idx * 2] = ((const uint8_t *)bytes)[idx];
        __RSUniCharUnicodeVersionString[idx * 2 + 1] = '.';
    }
    __RSUniCharUnicodeVersionString[(idx < 4 ? idx * 2 - 1 : 7)] = '\0';
    
    headerSize = RSSwapInt32BigToHost(*((uint32_t *)((char *)bytes + 4)));
    
    bitmapBase = (uint8_t *)bytes + headerSize;
    bytes = (uint8_t *)bytes + (sizeof(uint32_t) * 2);
    headerSize -= (sizeof(uint32_t) * 2);
    
    __RSUniCharNumberOfBitmaps = headerSize / (sizeof(uint32_t) * 2);
    
    array = (__RSUniCharBitmapData *)RSAllocatorAllocate(RSAllocatorSystemDefault, sizeof(__RSUniCharBitmapData) * __RSUniCharNumberOfBitmaps);
    
    for (idx = 0;idx < (int)__RSUniCharNumberOfBitmaps;idx++) {
        bitmap = (uint8_t *)bitmapBase + RSSwapInt32BigToHost(*((uint32_t *)bytes)); bytes = (uint8_t *)bytes + sizeof(uint32_t);
        bitmapSize = RSSwapInt32BigToHost(*((uint32_t *)bytes)); bytes = (uint8_t *)bytes + sizeof(uint32_t);
        
        numPlanes = bitmapSize / (8 * 1024);
        numPlanes = *(const uint8_t *)((char *)bitmap + (((numPlanes - 1) * ((8 * 1024) + 1)) - 1)) + 1;
        array[idx]._planes = (const uint8_t **)RSAllocatorAllocate(RSAllocatorSystemDefault, sizeof(const void *) * numPlanes);
        array[idx]._numPlanes = numPlanes;
        
        currentPlane = 0;
        for (bitmapIndex = 0;bitmapIndex < numPlanes;bitmapIndex++) {
            if (bitmapIndex == currentPlane) {
                array[idx]._planes[bitmapIndex] = (const uint8_t *)bitmap;
                bitmap = (uint8_t *)bitmap + (8 * 1024);
#if defined (__cplusplus)
                currentPlane = *(((const uint8_t*&)bitmap)++);
#else
                currentPlane = *((const uint8_t *)bitmap++);
#endif
                
            } else {
                array[idx]._planes[bitmapIndex] = NULL;
            }
        }
    }
    
    __RSUniCharBitmapDataArray = array;
    
    RSSpinLockUnlock(&__RSUniCharBitmapLock);
    
    return YES;
}
            
RSPrivate const char *__RSUniCharGetUnicodeVersionString(void) {
    if (NULL == __RSUniCharBitmapDataArray) __RSUniCharLoadBitmapData();
    return __RSUniCharUnicodeVersionString;
}
            
BOOL RSUniCharIsMemberOf(UTF32Char theChar, uint32_t charset) {
    charset = __RSUniCharMapCompatibilitySetID(charset);
    
    switch (charset) {
        case RSUniCharWhitespaceCharacterSet:
            return isWhitespace(theChar, charset, NULL);
            
        case RSUniCharWhitespaceAndNewlineCharacterSet:
            return isWhitespaceAndNewline(theChar, charset, NULL);
            
        case RSUniCharNewlineCharacterSet:
            return isNewline(theChar, charset, NULL);
            
        default: {
            uint32_t tableIndex = __RSUniCharMapExternalSetToInternalIndex(charset);
            
            if (NULL == __RSUniCharBitmapDataArray) __RSUniCharLoadBitmapData();
            
            if (tableIndex < __RSUniCharNumberOfBitmaps) {
                __RSUniCharBitmapData *data = __RSUniCharBitmapDataArray + tableIndex;
                uint8_t planeNo = (theChar >> 16) & 0xFF;
                
                // The bitmap data for RSUniCharIllegalCharacterSet is actually LEGAL set less Plane 14 ~ 16
                if (charset == RSUniCharIllegalCharacterSet) {
                    if (planeNo == 0x0E) { // Plane 14
                        theChar &= 0xFF;
                        return (((theChar == 0x01) || ((theChar > 0x1F) && (theChar < 0x80))) ? NO : YES);
                    } else if (planeNo == 0x0F || planeNo == 0x10) { // Plane 15 & 16
                        return ((theChar & 0xFF) > 0xFFFD ? YES : NO);
                    } else {
                        return (planeNo < data->_numPlanes && data->_planes[planeNo] ? !RSUniCharIsMemberOfBitmap(theChar, data->_planes[planeNo]) : YES);
                    }
                } else if (charset == RSUniCharControlAndFormatterCharacterSet) {
                    if (planeNo == 0x0E) { // Plane 14
                        theChar &= 0xFF;
                        return (((theChar == 0x01) || ((theChar > 0x1F) && (theChar < 0x80))) ? YES : NO);
                    } else {
                        return (planeNo < data->_numPlanes && data->_planes[planeNo] ? RSUniCharIsMemberOfBitmap(theChar, data->_planes[planeNo]) : NO);
                    }
                } else {
                    return (planeNo < data->_numPlanes && data->_planes[planeNo] ? RSUniCharIsMemberOfBitmap(theChar, data->_planes[planeNo]) : NO);
                }
            }
            return NO;
        }
    }
}
            
const uint8_t *RSUniCharGetBitmapPtrForPlane(uint32_t charset, uint32_t plane) {
    if (NULL == __RSUniCharBitmapDataArray) __RSUniCharLoadBitmapData();
    
    charset = __RSUniCharMapCompatibilitySetID(charset);
    
    if ((charset > RSUniCharWhitespaceAndNewlineCharacterSet) && (charset != RSUniCharIllegalCharacterSet) && (charset != RSUniCharNewlineCharacterSet)) {
        uint32_t tableIndex = __RSUniCharMapExternalSetToInternalIndex(charset);
        
        if (tableIndex < __RSUniCharNumberOfBitmaps) {
            __RSUniCharBitmapData *data = __RSUniCharBitmapDataArray + tableIndex;
            
            return (plane < data->_numPlanes ? data->_planes[plane] : NULL);
        }
    }
    return NULL;
}
            
RSPrivate uint8_t RSUniCharGetBitmapForPlane(uint32_t charset, uint32_t plane, void *bitmap, BOOL isInverted) {
    const uint8_t *src = RSUniCharGetBitmapPtrForPlane(charset, plane);
    int numBytes = (8 * 1024);
    
    if (src) {
        if (isInverted) {
#if defined (__cplusplus)
            while (numBytes-- > 0) *(((uint8_t *&)bitmap)++) = ~(*(src++));
#else
            while (numBytes-- > 0) *((uint8_t *)bitmap++) = ~(*(src++));
#endif
        } else {
#if defined (__cplusplus)
            while (numBytes-- > 0) *(((uint8_t *&)bitmap)++) = *(src++);
#else
            while (numBytes-- > 0) *((uint8_t *)bitmap++) = *(src++);
#endif
        }
        return RSUniCharBitmapFilled;
    } else if (charset == RSUniCharIllegalCharacterSet) {
        __RSUniCharBitmapData *data = __RSUniCharBitmapDataArray + __RSUniCharMapExternalSetToInternalIndex(__RSUniCharMapCompatibilitySetID(charset));
        
        if (plane < data->_numPlanes && (src = data->_planes[plane])) {
            if (isInverted) {
#if defined (__cplusplus)
                while (numBytes-- > 0) *(((uint8_t *&)bitmap)++) = *(src++);
#else
                while (numBytes-- > 0) *((uint8_t *)bitmap++) = *(src++);
#endif
            } else {
#if defined (__cplusplus)
                while (numBytes-- > 0) *(((uint8_t *&)bitmap)++) = ~(*(src++));
#else
                while (numBytes-- > 0) *((uint8_t *)bitmap++) = ~(*(src++));
#endif
            }
            return RSUniCharBitmapFilled;
        } else if (plane == 0x0E) { // Plane 14
            int idx;
            uint8_t asciiRange = (isInverted ? (uint8_t)0xFF : (uint8_t)0);
            uint8_t otherRange = (isInverted ? (uint8_t)0 : (uint8_t)0xFF);
            
#if defined (__cplusplus)
            *(((uint8_t *&)bitmap)++) = 0x02; // UE0001 LANGUAGE TAG
#else
            *((uint8_t *)bitmap++) = 0x02; // UE0001 LANGUAGE TAG
#endif
            for (idx = 1;idx < numBytes;idx++) {
#if defined (__cplusplus)
                *(((uint8_t *&)bitmap)++) = ((idx >= (0x20 / 8) && (idx < (0x80 / 8))) ? asciiRange : otherRange);
#else
                *((uint8_t *)bitmap++) = ((idx >= (0x20 / 8) && (idx < (0x80 / 8))) ? asciiRange : otherRange);
#endif
            }
            return RSUniCharBitmapFilled;
        } else if (plane == 0x0F || plane == 0x10) { // Plane 15 & 16
            uint32_t value = (isInverted ? ~0 : 0);
            numBytes /= 4; // for 32bit
            
            while (numBytes-- > 0) {
                *((uint32_t *)bitmap) = value;
#if defined (__cplusplus)
                bitmap = (uint8_t *)bitmap + sizeof(uint32_t);
#else
                bitmap += sizeof(uint32_t);
#endif
            }
            *(((uint8_t *)bitmap) - 5) = (isInverted ? 0x3F : 0xC0); // 0xFFFE & 0xFFFF
            return RSUniCharBitmapFilled;
        }
        return (isInverted ? RSUniCharBitmapEmpty : RSUniCharBitmapAll);
    } else if ((charset < RSUniCharDecimalDigitCharacterSet) || (charset == RSUniCharNewlineCharacterSet)) {
        if (plane) return (isInverted ? RSUniCharBitmapAll : RSUniCharBitmapEmpty);
        
        uint8_t *bitmapBase = (uint8_t *)bitmap;
        RSIndex idx;
        uint8_t nonFillValue = (isInverted ? (uint8_t)0xFF : (uint8_t)0);
        
#if defined (__cplusplus)
        while (numBytes-- > 0) *(((uint8_t *&)bitmap)++) = nonFillValue;
#else
        while (numBytes-- > 0) *((uint8_t *)bitmap++) = nonFillValue;
#endif
        
        if ((charset == RSUniCharWhitespaceAndNewlineCharacterSet) || (charset == RSUniCharNewlineCharacterSet)) {
            const UniChar newlines[] = {0x000A, 0x000B, 0x000C, 0x000D, 0x0085, 0x2028, 0x2029};
            
            for (idx = 0;idx < (int)(sizeof(newlines) / sizeof(*newlines)); idx++) {
                if (isInverted) {
                    RSUniCharRemoveCharacterFromBitmap(newlines[idx], bitmapBase);
                } else {
                    RSUniCharAddCharacterToBitmap(newlines[idx], bitmapBase);
                }
            }
            
            if (charset == RSUniCharNewlineCharacterSet) return RSUniCharBitmapFilled;
        }
        
        if (isInverted) {
            RSUniCharRemoveCharacterFromBitmap(0x0009, bitmapBase);
            RSUniCharRemoveCharacterFromBitmap(0x0020, bitmapBase);
            RSUniCharRemoveCharacterFromBitmap(0x00A0, bitmapBase);
            RSUniCharRemoveCharacterFromBitmap(0x1680, bitmapBase);
            RSUniCharRemoveCharacterFromBitmap(0x202F, bitmapBase);
            RSUniCharRemoveCharacterFromBitmap(0x205F, bitmapBase);
            RSUniCharRemoveCharacterFromBitmap(0x3000, bitmapBase);
        } else {
            RSUniCharAddCharacterToBitmap(0x0009, bitmapBase);
            RSUniCharAddCharacterToBitmap(0x0020, bitmapBase);
            RSUniCharAddCharacterToBitmap(0x00A0, bitmapBase);
            RSUniCharAddCharacterToBitmap(0x1680, bitmapBase);
            RSUniCharAddCharacterToBitmap(0x202F, bitmapBase);
            RSUniCharAddCharacterToBitmap(0x205F, bitmapBase);
            RSUniCharAddCharacterToBitmap(0x3000, bitmapBase);
        }
        
        for (idx = 0x2000;idx <= 0x200B;idx++) {
            if (isInverted) {
                RSUniCharRemoveCharacterFromBitmap(idx, bitmapBase);
            } else {
                RSUniCharAddCharacterToBitmap(idx, bitmapBase);
            }
        }
        return RSUniCharBitmapFilled;
    }
    return (isInverted ? RSUniCharBitmapAll : RSUniCharBitmapEmpty);
}
            
RSPrivate uint32_t RSUniCharGetNumberOfPlanes(uint32_t charset) {
    if ((charset == RSUniCharControlCharacterSet) || (charset == RSUniCharControlAndFormatterCharacterSet)) {
        return 15; // 0 to 14
    } else if (charset < RSUniCharDecimalDigitCharacterSet) {
        return 1;
    } else if (charset == RSUniCharIllegalCharacterSet) {
        return 17;
    } else {
        uint32_t numPlanes;
        
        if (NULL == __RSUniCharBitmapDataArray) __RSUniCharLoadBitmapData();
        
        numPlanes = __RSUniCharBitmapDataArray[__RSUniCharMapExternalSetToInternalIndex(__RSUniCharMapCompatibilitySetID(charset))]._numPlanes;
        
        return numPlanes;
    }
}

// Mapping data loading
static const void **__RSUniCharMappingTables = NULL;

static RSSpinLock __RSUniCharMappingTableLock = RSSpinLockInit;
            
#if DEPLOYMENT_TARGET_MACOSX || DEPLOYMENT_TARGET_EMBEDDED || DEPLOYMENT_TARGET_EMBEDDED_MINI || DEPLOYMENT_TARGET_LINUX
#if __RS_BIG_ENDIAN__
#if USE_MACHO_SEGMENT
#define MAPPING_TABLE_FILE "__data"
#else
#define MAPPING_TABLE_FILE "/RSUnicodeData-B.mapping"
#endif
#else
#if USE_MACHO_SEGMENT
#define MAPPING_TABLE_FILE "__data"
#else
#define MAPPING_TABLE_FILE "/RSUnicodeData-L.mapping"
#endif
#endif
#elif DEPLOYMENT_TARGET_WINDOWS
#if __RS_BIG_ENDIAN__
#if USE_MACHO_SEGMENT
#define MAPPING_TABLE_FILE "__data"
#else
#define MAPPING_TABLE_FILE L"RSUnicodeData-B.mapping"
#endif
#else
#if USE_MACHO_SEGMENT
#define MAPPING_TABLE_FILE "__data"
#else
#define MAPPING_TABLE_FILE L"RSUnicodeData-L.mapping"
#endif
#endif
#else
#error Unknown or unspecified DEPLOYMENT_TARGET
#endif
            
RSPrivate const void *RSUniCharGetMappingData(uint32_t type) {
    
    
    RSSpinLockLock(&__RSUniCharMappingTableLock);
    
    if (NULL == __RSUniCharMappingTables) {
        const void *bytes;
        const void *bodyBase;
        int headerSize;
        int idx, count;
        int64_t fileSize;
        
        if (!__RSUniCharLoadFile(MAPPING_TABLE_FILE, &bytes, &fileSize) || !__RSSimpleFileSizeVerification(bytes, fileSize)) {
            RSSpinLockUnlock(&__RSUniCharMappingTableLock);
            return NULL;
        }
        
#if defined (__cplusplus)
        bytes = (uint8_t *)bytes + 4; // Skip Unicode version
        headerSize = *((uint8_t *)bytes); bytes = (uint8_t *)bytes + sizeof(uint32_t);
#else
        bytes += 4; // Skip Unicode version
        headerSize = *((uint32_t *)bytes); bytes += sizeof(uint32_t);
#endif
        headerSize -= (sizeof(uint32_t) * 2);
        bodyBase = (char *)bytes + headerSize;
        
        count = headerSize / sizeof(uint32_t);
        
        __RSUniCharMappingTables = (const void **)RSAllocatorAllocate(RSAllocatorSystemDefault, sizeof(const void *) * count);
        
        for (idx = 0;idx < count;idx++) {
#if defined (__cplusplus)
            __RSUniCharMappingTables[idx] = (char *)bodyBase + *((uint32_t *)bytes); bytes = (uint8_t *)bytes + sizeof(uint32_t);
#else
            __RSUniCharMappingTables[idx] = (char *)bodyBase + *((uint32_t *)bytes); bytes += sizeof(uint32_t);
#endif
        }
    }
    
    RSSpinLockUnlock(&__RSUniCharMappingTableLock);
    
    return __RSUniCharMappingTables[type];
}
            
            // Case mapping functions
#define DO_SPECIAL_CASE_MAPPING 1
            
static uint32_t *__RSUniCharCaseMappingTableCounts = NULL;
static uint32_t **__RSUniCharCaseMappingTable = NULL;
static const uint32_t **__RSUniCharCaseMappingExtraTable = NULL;
            
typedef struct {
    uint32_t _key;
    uint32_t _value;
} __RSUniCharCaseMappings;
            
            /* Binary searches RSStringEncodingUnicodeTo8BitCharMap */
static uint32_t __RSUniCharGetMappedCase(const __RSUniCharCaseMappings *theTable, uint32_t numElem, UTF32Char character) {
    const __RSUniCharCaseMappings *p, *q, *divider;
    
    if ((character < theTable[0]._key) || (character > theTable[numElem-1]._key)) {
        return 0;
    }
    p = theTable;
    q = p + (numElem-1);
    while (p <= q) {
        divider = p + ((q - p) >> 1);	/* divide by 2 */
        if (character < divider->_key) { q = divider - 1; }
        else if (character > divider->_key) { p = divider + 1; }
        else { return divider->_value; }
    }
    return 0;
}
            
#define NUM_CASE_MAP_DATA (RSUniCharCaseFold + 1)
            
static BOOL __RSUniCharLoadCaseMappingTable(void) {
    uint32_t *countArray;
    int idx;
    
    if (NULL == __RSUniCharMappingTables) (void)RSUniCharGetMappingData(RSUniCharToLowercase);
    if (NULL == __RSUniCharMappingTables) return NO;
    
    RSSpinLockLock(&__RSUniCharMappingTableLock);
    
    if (__RSUniCharCaseMappingTableCounts) {
        RSSpinLockUnlock(&__RSUniCharMappingTableLock);
        return YES;
    }
    
    countArray = (uint32_t *)RSAllocatorAllocate(RSAllocatorSystemDefault, sizeof(uint32_t) * NUM_CASE_MAP_DATA + sizeof(uint32_t *) * NUM_CASE_MAP_DATA * 2);
    __RSUniCharCaseMappingTable = (uint32_t **)((char *)countArray + sizeof(uint32_t) * NUM_CASE_MAP_DATA);
    __RSUniCharCaseMappingExtraTable = (const uint32_t **)__RSUniCharCaseMappingTable + NUM_CASE_MAP_DATA;
    
    for (idx = 0;idx < NUM_CASE_MAP_DATA;idx++) {
        countArray[idx] = *((uint32_t *)__RSUniCharMappingTables[idx]) / (sizeof(uint32_t) * 2);
        __RSUniCharCaseMappingTable[idx] = ((uint32_t *)__RSUniCharMappingTables[idx]) + 1;
        __RSUniCharCaseMappingExtraTable[idx] = (const uint32_t *)((char *)__RSUniCharCaseMappingTable[idx] + *((uint32_t *)__RSUniCharMappingTables[idx]));
    }
    
    __RSUniCharCaseMappingTableCounts = countArray;
    
    RSSpinLockUnlock(&__RSUniCharMappingTableLock);
    return YES;
}
            
#if __RS_BIG_ENDIAN__
#define TURKISH_LANG_CODE	(0x7472) // tr
#define LITHUANIAN_LANG_CODE	(0x6C74) // lt
#define AZERI_LANG_CODE		(0x617A) // az
#define DUTCH_LANG_CODE		(0x6E6C) // nl
#define GREEK_LANG_CODE		(0x656C) // el
#else
#define TURKISH_LANG_CODE	(0x7274) // tr
#define LITHUANIAN_LANG_CODE	(0x746C) // lt
#define AZERI_LANG_CODE		(0x7A61) // az
#define DUTCH_LANG_CODE		(0x6C6E) // nl
#define GREEK_LANG_CODE		(0x6C65) // el
#endif
            
RSIndex RSUniCharMapCaseTo(UTF32Char theChar, UTF16Char *convertedChar, RSIndex maxLength, uint32_t ctype, uint32_t flags, const uint8_t *langCode) {
    __RSUniCharBitmapData *data;
    uint8_t planeNo = (theChar >> 16) & 0xFF;
    
caseFoldRetry:
    
#if DO_SPECIAL_CASE_MAPPING
    if (flags & RSUniCharCaseMapFinalSigma) {
        if (theChar == 0x03A3) { // Final sigma
            *convertedChar = (ctype == RSUniCharToLowercase ? 0x03C2 : 0x03A3);
            return 1;
        }
    }
    
    if (langCode) {
        if (flags & RSUniCharCaseMapGreekTonos) { // localized Greek uppercasing
            if (theChar == 0x0301) { // GREEK TONOS
                return 0;
            } else if (theChar == 0x0344) {// COMBINING GREEK DIALYTIKA TONOS
                *convertedChar = 0x0308; // COMBINING GREEK DIALYTIKA
                return 1;
            } else if (RSUniCharIsMemberOf(theChar, RSUniCharDecomposableCharacterSet)) {
                UTF32Char buffer[MAX_DECOMPOSED_LENGTH];
                RSIndex length = RSUniCharDecomposeCharacter(theChar, buffer, MAX_DECOMPOSED_LENGTH);
                
                if (length > 1) {
                    UTF32Char *characters = buffer + 1;
                    UTF32Char *tail = buffer + length;
                    
                    while (characters < tail) {
                        if (*characters == 0x0301) break;
                        ++characters;
                    }
                    
                    if (characters < tail) { // found a tonos
                        RSIndex convertedLength = RSUniCharMapCaseTo(*buffer, convertedChar, maxLength, ctype, 0, langCode);
                        
                        if (convertedLength == 0) {
                            *convertedChar = (UTF16Char)*buffer;
                            convertedLength = 1;
                        }
                        
                        characters = buffer + 1;
                        
                        while (characters < tail) {
                            if (*characters != 0x0301) { // not tonos
                                if (*characters < 0x10000) { // BMP
                                    convertedChar[convertedLength] = (UTF16Char)*characters;
                                    ++convertedLength;
                                } else {
                                    UTF32Char character = *characters - 0x10000;
                                    convertedChar[convertedLength++] = (UTF16Char)((character >> 10) + 0xD800UL);
                                    convertedChar[convertedLength++] = (UTF16Char)((character & 0x3FF) + 0xDC00UL);
                                }
                            }
                            ++characters;
                        }
                        
                        return convertedLength;
                    }
                }
            }
        }
        switch (*(uint16_t *)langCode) {
            case LITHUANIAN_LANG_CODE:
                if (theChar == 0x0307 && (flags & RSUniCharCaseMapAfter_i)) {
                    return 0;
                } else if (ctype == RSUniCharToLowercase) {
                    if (flags & RSUniCharCaseMapMoreAbove) {
                        switch (theChar) {
                            case 0x0049: // LATIN CAPITAL LETTER I
                                *(convertedChar++) = 0x0069;
                                *(convertedChar++) = 0x0307;
                                return 2;
                                
                            case 0x004A: // LATIN CAPITAL LETTER J
                                *(convertedChar++) = 0x006A;
                                *(convertedChar++) = 0x0307;
                                return 2;
                                
                            case 0x012E: // LATIN CAPITAL LETTER I WITH OGONEK
                                *(convertedChar++) = 0x012F;
                                *(convertedChar++) = 0x0307;
                                return 2;
                                
                            default: break;
                        }
                    }
                    switch (theChar) {
                        case 0x00CC: // LATIN CAPITAL LETTER I WITH GRAVE
                            *(convertedChar++) = 0x0069;
                            *(convertedChar++) = 0x0307;
                            *(convertedChar++) = 0x0300;
                            return 3;
                            
                        case 0x00CD: // LATIN CAPITAL LETTER I WITH ACUTE
                            *(convertedChar++) = 0x0069;
                            *(convertedChar++) = 0x0307;
                            *(convertedChar++) = 0x0301;
                            return 3;
                            
                        case 0x0128: // LATIN CAPITAL LETTER I WITH TILDE
                            *(convertedChar++) = 0x0069;
                            *(convertedChar++) = 0x0307;
                            *(convertedChar++) = 0x0303;
                            return 3;
                            
                        default: break;
                    }
                }
                break;
                
            case TURKISH_LANG_CODE:
            case AZERI_LANG_CODE:
                if ((theChar == 0x0049) || (theChar == 0x0131)) { // LATIN CAPITAL LETTER I & LATIN SMALL LETTER DOTLESS I
                    *convertedChar = (((ctype == RSUniCharToLowercase) || (ctype == RSUniCharCaseFold))  ? ((RSUniCharCaseMapMoreAbove & flags) ? 0x0069 : 0x0131) : 0x0049);
                    return 1;
                } else if ((theChar == 0x0069) || (theChar == 0x0130)) { // LATIN SMALL LETTER I & LATIN CAPITAL LETTER I WITH DOT ABOVE
                    *convertedChar = (((ctype == RSUniCharToLowercase) || (ctype == RSUniCharCaseFold)) ? 0x0069 : 0x0130);
                    return 1;
                } else if (theChar == 0x0307 && (RSUniCharCaseMapAfter_i & flags)) { // COMBINING DOT ABOVE AFTER_i
                    if (ctype == RSUniCharToLowercase) {
                        return 0;
                    } else {
                        *convertedChar = 0x0307;
                        return 1;
                    }
                }
                break;
                
            case DUTCH_LANG_CODE:
                if ((theChar == 0x004A) || (theChar == 0x006A)) {
                    *convertedChar = (((ctype == RSUniCharToUppercase) || (ctype == RSUniCharToTitlecase) || (RSUniCharCaseMapDutchDigraph & flags)) ? 0x004A  : 0x006A);
                    return 1;
                }
                break;
                
            default: break;
        }
    }
#endif // DO_SPECIAL_CASE_MAPPING
    
    if (NULL == __RSUniCharBitmapDataArray) __RSUniCharLoadBitmapData();
    
    data = __RSUniCharBitmapDataArray + __RSUniCharMapExternalSetToInternalIndex(__RSUniCharMapCompatibilitySetID(ctype + RSUniCharHasNonSelfLowercaseCharacterSet));
    
    if (planeNo < data->_numPlanes && data->_planes[planeNo] && RSUniCharIsMemberOfBitmap(theChar, data->_planes[planeNo]) && (__RSUniCharCaseMappingTableCounts || __RSUniCharLoadCaseMappingTable())) {
        uint32_t value = __RSUniCharGetMappedCase((const __RSUniCharCaseMappings *)__RSUniCharCaseMappingTable[ctype], __RSUniCharCaseMappingTableCounts[ctype], theChar);
        
        if (!value && ctype == RSUniCharToTitlecase) {
            value = __RSUniCharGetMappedCase((const __RSUniCharCaseMappings *)__RSUniCharCaseMappingTable[RSUniCharToUppercase], __RSUniCharCaseMappingTableCounts[RSUniCharToUppercase], theChar);
            if (value) ctype = RSUniCharToUppercase;
        }
        
        if (value) {
            RSIndex count = RSUniCharConvertFlagToCount(value);
            
            if (count == 1) {
                if (value & RSUniCharNonBmpFlag) {
                    if (maxLength > 1) {
                        value = (value & 0xFFFFFF) - 0x10000;
                        *(convertedChar++) = (UTF16Char)(value >> 10) + 0xD800UL;
                        *(convertedChar++) = (UTF16Char)(value & 0x3FF) + 0xDC00UL;
                        return 2;
                    }
                } else {
                    *convertedChar = (UTF16Char)value;
                    return 1;
                }
            } else if (count < maxLength) {
                const uint32_t *extraMapping = __RSUniCharCaseMappingExtraTable[ctype] + (value & 0xFFFFFF);
                
                if (value & RSUniCharNonBmpFlag) {
                    RSIndex copiedLen = 0;
                    
                    while (count-- > 0) {
                        value = *(extraMapping++);
                        if (value > 0xFFFF) {
                            if (copiedLen + 2 >= maxLength) break;
                            value = (value & 0xFFFFFF) - 0x10000;
                            convertedChar[copiedLen++] = (UTF16Char)(value >> 10) + 0xD800UL;
                            convertedChar[copiedLen++] = (UTF16Char)(value & 0x3FF) + 0xDC00UL;
                        } else {
                            if (copiedLen + 1 >= maxLength) break;
                            convertedChar[copiedLen++] = value;
                        }
                    }
                    if (!count) return copiedLen;
                } else {
                    RSIndex idx;
                    
                    for (idx = 0;idx < count;idx++) *(convertedChar++) = (UTF16Char)*(extraMapping++);
                    return count;
                }
            }
        }
    } else if (ctype == RSUniCharCaseFold) {
        ctype = RSUniCharToLowercase;
        goto caseFoldRetry;
    }
    
    if (theChar > 0xFFFF) { // non-BMP
        theChar = (theChar & 0xFFFFFF) - 0x10000;
        *(convertedChar++) = (UTF16Char)(theChar >> 10) + 0xD800UL;
        *(convertedChar++) = (UTF16Char)(theChar & 0x3FF) + 0xDC00UL;
        return 2;
    } else {
        *convertedChar = theChar;
        return 1;
    }
}
            
RSIndex RSUniCharMapTo(UniChar theChar, UniChar *convertedChar, RSIndex maxLength, uint16_t ctype, uint32_t flags) {
    if (ctype == RSUniCharCaseFold + 1) { // RSUniCharDecompose
        if (RSUniCharIsDecomposableCharacter(theChar, NO)) {
            UTF32Char buffer[MAX_DECOMPOSED_LENGTH];
            RSIndex usedLength = RSUniCharDecomposeCharacter(theChar, buffer, MAX_DECOMPOSED_LENGTH);
            RSIndex idx;
            
            for (idx = 0;idx < usedLength;idx++) *(convertedChar++) = buffer[idx];
            return usedLength;
        } else {
            *convertedChar = theChar;
            return 1;
        }
    } else {
        return RSUniCharMapCaseTo(theChar, convertedChar, maxLength, ctype, flags, NULL);
    }
}
            
RSInline BOOL __RSUniCharIsMoreAbove(UTF16Char *buffer, RSIndex length) {
    UTF32Char currentChar;
    uint32_t property;
    
    while (length-- > 0) {
        currentChar = *(buffer)++;
        if (RSUniCharIsSurrogateHighCharacter(currentChar) && (length > 0) && RSUniCharIsSurrogateLowCharacter(*(buffer + 1))) {
            currentChar = RSUniCharGetLongCharacterForSurrogatePair(currentChar, *(buffer++));
            --length;
        }
        if (!RSUniCharIsMemberOf(currentChar, RSUniCharNonBaseCharacterSet)) break;
        
        property = RSUniCharGetCombiningPropertyForCharacter(currentChar, (const uint8_t *)RSUniCharGetUnicodePropertyDataForPlane(RSUniCharCombiningProperty, (currentChar >> 16) & 0xFF));
        
        if (property == 230) return YES; // Above priority
    }
    return NO;
}
            
RSInline BOOL __RSUniCharIsAfter_i(UTF16Char *buffer, RSIndex length) {
    UTF32Char currentChar = 0;
    uint32_t property;
    UTF32Char decomposed[MAX_DECOMPOSED_LENGTH];
    RSIndex decompLength;
    RSIndex idx;
    
    if (length < 1) return 0;
    
    buffer += length;
    while (length-- > 1) {
        currentChar = *(--buffer);
        if (RSUniCharIsSurrogateLowCharacter(currentChar)) {
            if ((length > 1) && RSUniCharIsSurrogateHighCharacter(*(buffer - 1))) {
                currentChar = RSUniCharGetLongCharacterForSurrogatePair(*(--buffer), currentChar);
                --length;
            } else {
                break;
            }
        }
        if (!RSUniCharIsMemberOf(currentChar, RSUniCharNonBaseCharacterSet)) break;
        
        property = RSUniCharGetCombiningPropertyForCharacter(currentChar, (const uint8_t *)RSUniCharGetUnicodePropertyDataForPlane(RSUniCharCombiningProperty, (currentChar >> 16) & 0xFF));
        
        if (property == 230) return NO; // Above priority
    }
    if (length == 0) {
        currentChar = *(--buffer);
    } else if (RSUniCharIsSurrogateLowCharacter(currentChar) && RSUniCharIsSurrogateHighCharacter(*(--buffer))) {
        currentChar = RSUniCharGetLongCharacterForSurrogatePair(*buffer, currentChar);
    }
    
    decompLength = RSUniCharDecomposeCharacter(currentChar, decomposed, MAX_DECOMPOSED_LENGTH);
    currentChar = *decomposed;
    
    
    for (idx = 1;idx < decompLength;idx++) {
        currentChar = decomposed[idx];
        property = RSUniCharGetCombiningPropertyForCharacter(currentChar, (const uint8_t *)RSUniCharGetUnicodePropertyDataForPlane(RSUniCharCombiningProperty, (currentChar >> 16) & 0xFF));
        
        if (property == 230) return NO; // Above priority
    }
    return YES;
}
            
RSPrivate uint32_t RSUniCharGetConditionalCaseMappingFlags(UTF32Char theChar, UTF16Char *buffer, RSIndex currentIndex, RSIndex length, uint32_t type, const uint8_t *langCode, uint32_t lastFlags) {
    if (theChar == 0x03A3) { // GREEK CAPITAL LETTER SIGMA
        if ((type == RSUniCharToLowercase) && (currentIndex > 0)) {
            UTF16Char *start = buffer;
            UTF16Char *end = buffer + length;
            UTF32Char otherChar;
            
            // First check if we're after a cased character
            buffer += (currentIndex - 1);
            while (start <= buffer) {
                otherChar = *(buffer--);
                if (RSUniCharIsSurrogateLowCharacter(otherChar) && (start <= buffer) && RSUniCharIsSurrogateHighCharacter(*buffer)) {
                    otherChar = RSUniCharGetLongCharacterForSurrogatePair(*(buffer--), otherChar);
                }
                if (!RSUniCharIsMemberOf(otherChar, RSUniCharCaseIgnorableCharacterSet)) {
                    if (!RSUniCharIsMemberOf(otherChar, RSUniCharUppercaseLetterCharacterSet) && !RSUniCharIsMemberOf(otherChar, RSUniCharLowercaseLetterCharacterSet)) return 0; // Uppercase set contains titlecase
                    break;
                }
            }
            
            // Next check if we're before a cased character
            buffer = start + currentIndex + 1;
            while (buffer < end) {
                otherChar = *(buffer++);
                if (RSUniCharIsSurrogateHighCharacter(otherChar) && (buffer < end) && RSUniCharIsSurrogateLowCharacter(*buffer)) {
                    otherChar = RSUniCharGetLongCharacterForSurrogatePair(otherChar, *(buffer++));
                }
                if (!RSUniCharIsMemberOf(otherChar, RSUniCharCaseIgnorableCharacterSet)) {
                    if (RSUniCharIsMemberOf(otherChar, RSUniCharUppercaseLetterCharacterSet) || RSUniCharIsMemberOf(otherChar, RSUniCharLowercaseLetterCharacterSet)) return 0; // Uppercase set contains titlecase
                    break;
                }
            }
            return RSUniCharCaseMapFinalSigma;
        }
    } else if (langCode) {
        if (*((const uint16_t *)langCode) == LITHUANIAN_LANG_CODE) {
            if ((theChar == 0x0307) && ((RSUniCharCaseMapAfter_i|RSUniCharCaseMapMoreAbove) & lastFlags) == (RSUniCharCaseMapAfter_i|RSUniCharCaseMapMoreAbove)) {
                return (__RSUniCharIsAfter_i(buffer, currentIndex) ? RSUniCharCaseMapAfter_i : 0);
            } else if (type == RSUniCharToLowercase) {
                if ((theChar == 0x0049) || (theChar == 0x004A) || (theChar == 0x012E)) {
                    ++currentIndex;
                    return (__RSUniCharIsMoreAbove(buffer + (currentIndex), length - currentIndex) ? RSUniCharCaseMapMoreAbove : 0);
                }
            } else if ((theChar == 'i') || (theChar == 'j')) {
                ++currentIndex;
                return (__RSUniCharIsMoreAbove(buffer + (currentIndex), length - currentIndex) ? (RSUniCharCaseMapAfter_i|RSUniCharCaseMapMoreAbove) : 0);
            }
        } else if ((*((const uint16_t *)langCode) == TURKISH_LANG_CODE) || (*((const uint16_t *)langCode) == AZERI_LANG_CODE)) {
            if (type == RSUniCharToLowercase) {
                if (theChar == 0x0307) {
                    return (RSUniCharCaseMapMoreAbove & lastFlags ? RSUniCharCaseMapAfter_i : 0);
                } else if (theChar == 0x0049) {
                    return (((++currentIndex < length) && (buffer[currentIndex] == 0x0307)) ? RSUniCharCaseMapMoreAbove : 0);
                }
            }
        } else if (*((const uint16_t *)langCode) == DUTCH_LANG_CODE) {
            if (RSUniCharCaseMapDutchDigraph & lastFlags) {
                return (((theChar == 0x006A) || (theChar == 0x004A)) ? RSUniCharCaseMapDutchDigraph : 0);
            } else {
                if ((type == RSUniCharToTitlecase) && ((theChar == 0x0069) || (theChar == 0x0049))) {
                    return (((++currentIndex < length) && ((buffer[currentIndex] == 0x006A) || (buffer[currentIndex] == 0x004A))) ? RSUniCharCaseMapDutchDigraph : 0);
                }
            }
        }
        
        if (RSUniCharCaseMapGreekTonos & lastFlags) { // still searching for tonos
            if (RSUniCharIsMemberOf(theChar, RSUniCharNonBaseCharacterSet)) {
                return RSUniCharCaseMapGreekTonos;
            }
        }
        if (((theChar >= 0x0370) && (theChar < 0x0400)) || ((theChar >= 0x1F00) && (theChar < 0x2000))) { // Greek/Coptic & Greek extended ranges
            if (((type == RSUniCharToUppercase) || (type == RSUniCharToTitlecase))&& (RSUniCharIsMemberOf(theChar, RSUniCharLetterCharacterSet))) return RSUniCharCaseMapGreekTonos;
        }
    }
    return 0;
}
            
// Unicode property database
static __RSUniCharBitmapData *__RSUniCharUnicodePropertyTable = NULL;
static int __RSUniCharUnicodePropertyTableCount = 0;

static RSSpinLock __RSUniCharPropTableLock = RSSpinLockInit;
            
#if DEPLOYMENT_TARGET_MACOSX || DEPLOYMENT_TARGET_EMBEDDED || DEPLOYMENT_TARGET_EMBEDDED_MINI || DEPLOYMENT_TARGET_LINUX
#if USE_MACHO_SEGMENT
#define PROP_DB_FILE "__properties"
#else
#define PROP_DB_FILE "/RSUniCharPropertyDatabase.data"
#endif
#elif DEPLOYMENT_TARGET_WINDOWS
#if USE_MACHO_SEGMENT
#define PROP_DB_FILE "__properties"
#else
#define PROP_DB_FILE L"RSUniCharPropertyDatabase.data"
#endif
#else
#error Unknown or unspecified DEPLOYMENT_TARGET
#endif
            
const void *RSUniCharGetUnicodePropertyDataForPlane(uint32_t propertyType, uint32_t plane) {
    
    RSSpinLockLock(&__RSUniCharPropTableLock);
    
    if (NULL == __RSUniCharUnicodePropertyTable) {
        __RSUniCharBitmapData *table;
        const void *bytes;
        const void *bodyBase;
        const void *planeBase;
        int headerSize;
        int idx, count;
        int planeIndex, planeCount;
        int planeSize;
        int64_t fileSize;
        
        if (!__RSUniCharLoadFile(PROP_DB_FILE, &bytes, &fileSize) || !__RSSimpleFileSizeVerification(bytes, fileSize)) {
            RSSpinLockUnlock(&__RSUniCharPropTableLock);
            return NULL;
        }
        
#if defined (__cplusplus)
        bytes = (uint8_t*)bytes + 4; // Skip Unicode version
        headerSize = RSSwapInt32BigToHost(*((uint32_t *)bytes)); bytes = (uint8_t *)bytes + sizeof(uint32_t);
#else
        bytes += 4; // Skip Unicode version
        headerSize = RSSwapInt32BigToHost(*((uint32_t *)bytes)); bytes += sizeof(uint32_t);
#endif
        
        headerSize -= (sizeof(uint32_t) * 2);
        bodyBase = (char *)bytes + headerSize;
        
        count = headerSize / sizeof(uint32_t);
        __RSUniCharUnicodePropertyTableCount = count;
        
        table = (__RSUniCharBitmapData *)RSAllocatorAllocate(RSAllocatorSystemDefault, sizeof(__RSUniCharBitmapData) * count);
        
        for (idx = 0;idx < count;idx++) {
            planeCount = *((const uint8_t *)bodyBase);
            planeBase = (char *)bodyBase + planeCount + (planeCount % 4 ? 4 - (planeCount % 4) : 0);
            table[idx]._planes = (const uint8_t **)RSAllocatorAllocate(RSAllocatorSystemDefault, sizeof(const void *) * planeCount);
            
            for (planeIndex = 0;planeIndex < planeCount;planeIndex++) {
                if ((planeSize = ((const uint8_t *)bodyBase)[planeIndex + 1])) {
                    table[idx]._planes[planeIndex] = (const uint8_t *)planeBase;
#if defined (__cplusplus)
                    planeBase = (char*)planeBase + (planeSize * 256);
#else
                    planeBase += (planeSize * 256);
#endif
                } else {
                    table[idx]._planes[planeIndex] = NULL;
                }
            }
            
            table[idx]._numPlanes = planeCount;
#if defined (__cplusplus)
            bodyBase = (const uint8_t *)bodyBase + (RSSwapInt32BigToHost(*(uint32_t *)bytes));
            ((uint32_t *&)bytes) ++;
#else
            bodyBase += (RSSwapInt32BigToHost(*((uint32_t *)bytes++)));
#endif
        }
        
        __RSUniCharUnicodePropertyTable = table;
    }
    
    RSSpinLockUnlock(&__RSUniCharPropTableLock);
    
    return (plane < __RSUniCharUnicodePropertyTable[propertyType]._numPlanes ? __RSUniCharUnicodePropertyTable[propertyType]._planes[plane] : NULL);
}
            
RSPrivate uint32_t RSUniCharGetNumberOfPlanesForUnicodePropertyData(uint32_t propertyType) {
    (void)RSUniCharGetUnicodePropertyDataForPlane(propertyType, 0);
    return __RSUniCharUnicodePropertyTable[propertyType]._numPlanes;
}
            
RSPrivate uint32_t RSUniCharGetUnicodeProperty(UTF32Char character, uint32_t propertyType) {
    if (propertyType == RSUniCharCombiningProperty) {
        return RSUniCharGetCombiningPropertyForCharacter(character, (const uint8_t *)RSUniCharGetUnicodePropertyDataForPlane(propertyType, (character >> 16) & 0xFF));
    } else if (propertyType == RSUniCharBidiProperty) {
        return RSUniCharGetBidiPropertyForCharacter(character, (const uint8_t *)RSUniCharGetUnicodePropertyDataForPlane(propertyType, (character >> 16) & 0xFF));
    } else {
        return 0;
    }
}
            
            
            
            /*
             The UTF8 conversion in the following function is derived from ConvertUTF.c
             */
            /*
             * Copyright 2001 Unicode, Inc.
             *
             * Disclaimer
             *
             * This source code is provided as is by Unicode, Inc. No claims are
             * made as to fitness for any particular purpose. No warranties of any
             * kind are expressed or implied. The recipient agrees to determine
             * applicability of information provided. If this file has been
             * purchased on magnetic or optical media from Unicode, Inc., the
             * sole remedy for any claim will be exchange of defective media
             * within 90 days of receipt.
             *
             * Limitations on Rights to Redistribute This Code
             *
             * Unicode, Inc. hereby grants the right to freely use the information
             * supplied in this file in the creation of products supporting the
             * Unicode Standard, and to make copies of this file in any form
             * for internal or external distribution as long as this notice
             * remains attached.
             */
#define UNI_REPLACEMENT_CHAR (0x0000FFFDUL)
            
BOOL RSUniCharFillDestinationBuffer(const UTF32Char *src, RSIndex srcLength, void **dst, RSIndex dstLength, RSIndex *filledLength, uint32_t dstFormat)
{
    UTF32Char currentChar;
    RSIndex usedLength = *filledLength;
    
    if (dstFormat == RSUniCharUTF16Format) {
        UTF16Char *dstBuffer = (UTF16Char *)*dst;
        
        while (srcLength-- > 0) {
            currentChar = *(src++);
            
            if (currentChar > 0xFFFF) { // Non-BMP
                usedLength += 2;
                if (dstLength) {
                    if (usedLength > dstLength) return NO;
                    currentChar -= 0x10000;
                    *(dstBuffer++) = (UTF16Char)((currentChar >> 10) + 0xD800UL);
                    *(dstBuffer++) = (UTF16Char)((currentChar & 0x3FF) + 0xDC00UL);
                }
            } else {
                ++usedLength;
                if (dstLength) {
                    if (usedLength > dstLength) return NO;
                    *(dstBuffer++) = (UTF16Char)currentChar;
                }
            }
        }
        
        *dst = dstBuffer;
    } else if (dstFormat == RSUniCharUTF8Format) {
        uint8_t *dstBuffer = (uint8_t *)*dst;
        uint16_t bytesToWrite = 0;
        const UTF32Char byteMask = 0xBF;
        const UTF32Char byteMark = 0x80;
        static const uint8_t firstByteMark[7] = { 0x00, 0x00, 0xC0, 0xE0, 0xF0, 0xF8, 0xFC };
        
        while (srcLength-- > 0) {
            currentChar = *(src++);
            
            /* Figure out how many bytes the result will require */
            if (currentChar < (UTF32Char)0x80) {
                bytesToWrite = 1;
            } else if (currentChar < (UTF32Char)0x800) {
                bytesToWrite = 2;
            } else if (currentChar < (UTF32Char)0x10000) {
                bytesToWrite = 3;
            } else if (currentChar < (UTF32Char)0x200000) {
                bytesToWrite = 4;
            } else {
                bytesToWrite = 2;
                currentChar = UNI_REPLACEMENT_CHAR;
            }
            
            usedLength += bytesToWrite;
            
            if (dstLength) {
                if (usedLength > dstLength) return NO;
                
                dstBuffer += bytesToWrite;
                switch (bytesToWrite) {	/* note: everything falls through. */
                    case 4:	*--dstBuffer = (currentChar | byteMark) & byteMask; currentChar >>= 6;
                    case 3:	*--dstBuffer = (currentChar | byteMark) & byteMask; currentChar >>= 6;
                    case 2:	*--dstBuffer = (currentChar | byteMark) & byteMask; currentChar >>= 6;
                    case 1:	*--dstBuffer =  currentChar | firstByteMark[bytesToWrite];
                }
                dstBuffer += bytesToWrite;
            }
        }
        
        *dst = dstBuffer;
    } else {
        UTF32Char *dstBuffer = (UTF32Char *)*dst;
        
        while (srcLength-- > 0) {
            currentChar = *(src++);
            
            ++usedLength;
            if (dstLength) {
                if (usedLength > dstLength) return NO;
                *(dstBuffer++) = currentChar;
            }
        }
        
        *dst = dstBuffer;
    }
    
    *filledLength = usedLength;
    
    return YES;
}
            
#if DEPLOYMENT_TARGET_WINDOWS
void __RSUniCharCleanup(void)
{
    int	idx;
    
    // cleanup memory allocated by __RSUniCharLoadBitmapData()
    RSSpinLockLock(&__RSUniCharBitmapLock);
    
    if (__RSUniCharBitmapDataArray != NULL) {
        for (idx = 0; idx < (int)__RSUniCharNumberOfBitmaps; idx++) {
            RSAllocatorDeallocate(RSAllocatorSystemDefault, __RSUniCharBitmapDataArray[idx]._planes);
            __RSUniCharBitmapDataArray[idx]._planes = NULL;
        }
        
        RSAllocatorDeallocate(RSAllocatorSystemDefault, __RSUniCharBitmapDataArray);
        __RSUniCharBitmapDataArray = NULL;
        __RSUniCharNumberOfBitmaps = 0;
    }
    
    RSSpinLockUnlock(&__RSUniCharBitmapLock);
    
    // cleanup memory allocated by RSUniCharGetMappingData()
    RSSpinLockLock(&__RSUniCharMappingTableLock);
    
    if (__RSUniCharMappingTables != NULL) {
        RSAllocatorDeallocate(RSAllocatorSystemDefault, __RSUniCharMappingTables);
        __RSUniCharMappingTables = NULL;
    }
    
    // cleanup memory allocated by __RSUniCharLoadCaseMappingTable()
    if (__RSUniCharCaseMappingTableCounts != NULL) {
        RSAllocatorDeallocate(RSAllocatorSystemDefault, __RSUniCharCaseMappingTableCounts);
        __RSUniCharCaseMappingTableCounts = NULL;
        
        __RSUniCharCaseMappingTable = NULL;
        __RSUniCharCaseMappingExtraTable = NULL;
    }
    
    RSSpinLockUnlock(&__RSUniCharMappingTableLock);
    
    // cleanup memory allocated by RSUniCharGetUnicodePropertyDataForPlane()
    RSSpinLockLock(&__RSUniCharPropTableLock);
    
    if (__RSUniCharUnicodePropertyTable != NULL) {
        for (idx = 0; idx < __RSUniCharUnicodePropertyTableCount; idx++) {
            RSAllocatorDeallocate(RSAllocatorSystemDefault, __RSUniCharUnicodePropertyTable[idx]._planes);
            __RSUniCharUnicodePropertyTable[idx]._planes = NULL;
        }
        
        RSAllocatorDeallocate(RSAllocatorSystemDefault, __RSUniCharUnicodePropertyTable);
        __RSUniCharUnicodePropertyTable = NULL;
        __RSUniCharUnicodePropertyTableCount = 0;
    }
    
    RSSpinLockUnlock(&__RSUniCharPropTableLock);
}
#endif
            
#undef USE_MACHO_SEGMENT
            
            
