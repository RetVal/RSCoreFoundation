//
//  Unichar.cpp
//  RSCoreFoundation
//
//  Created by closure on 6/2/15.
//  Copyright (c) 2015 RetVal. All rights reserved.
//

#include <RSFoundation/Allocator.h>
#include <RSFoundation/Unichar.h>
#include <RSFoundation/Order.h>
#include <RSFoundation/Lock.h>
#include <RSFoundation/UnicodeDecomposition.h>

#include "Internal.h"
#include <RSFoundation/UniCharPrivate.h>

#ifndef MAX_DECOMPOSED_LENGTH
#define MAX_DECOMPOSED_LENGTH (10)
#endif

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

namespace RSFoundation {
    namespace Collection {
        
        namespace Encoding {
#if DEPLOYMENT_TARGET_MACOSX || DEPLOYMENT_TARGET_EMBEDDED
#define USE_MACHO_SEGMENT 1
#endif
            
#if (DEPLOYMENT_TARGET_MACOSX || DEPLOYMENT_TARGET_EMBEDDED) && USE_MACHO_SEGMENT
            extern "C" {
#include <mach-o/getsect.h>
#include <mach-o/dyld.h>
#include <mach-o/ldsyms.h>
                
#if (DEPLOYMENT_TARGET_MACOSX || DEPLOYMENT_TARGET_EMBEDDED) && USE_MACHO_SEGMENT
                static const void *__GetSectDataPtr(const char *segname, const char *sectname, uint64_t *sizep) {
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
                    return nullptr;
                }
#endif
            }
#endif

            enum {
                LastExternalSet = NewlineCharacterSet,
                FirstInternalSet = CompatibilityDecomposableCharacterSet,
                LastInternalSet = GraphemeExtendCharacterSet,
                FirstBitmapSet = DecimalDigitCharacterSet
            };
            
            inline CharacterSet __MapExternalSetToInternalIndex(CharacterSet cset) { return CharacterSet(((FirstInternalSet <= cset) ? ((cset - FirstInternalSet) + LastExternalSet) : cset) - FirstBitmapSet); }
            inline CharacterSet __MapCompatibilitySetID(CharacterSet cset) { return ((cset == ControlCharacterSet) ? ControlAndFormatterCharacterSet : (((cset > LastExternalSet) && (cset < FirstInternalSet)) ? CharacterSet((cset - LastExternalSet) + FirstInternalSet) : cset)); }
        
            
#if !USE_MACHO_SEGMENT
            
            // Memory map the file
            
#if DEPLOYMENT_TARGET_MACOSX || DEPLOYMENT_TARGET_EMBEDDED || DEPLOYMENT_TARGET_EMBEDDED_MINI || DEPLOYMENT_TARGET_LINUX
            inline void __UniCharCharacterSetPath(char *cpath)
            {
#elif DEPLOYMENT_TARGET_WINDOWS
                inline void __UniCharCharacterSetPath(wchar_t *wpath)
                {
#else
#error Unknown or unspecified DEPLOYMENT_TARGET
#endif
#if DEPLOYMENT_TARGET_MACOSX || DEPLOYMENT_TARGET_EMBEDDED
                    strlcpy(cpath, __CharacterSetDir, MAXPATHLEN);
#elif DEPLOYMENT_TARGET_LINUX
                    strlcpy(cpath, __CharacterSetDir, MAXPATHLEN);
#elif DEPLOYMENT_TARGET_WINDOWS
                    wchar_t frameworkPath[MAXPATHLEN];
                    _GetFrameworkPath(frameworkPath, MAXPATHLEN);
                    wcsncpy(wpath, frameworkPath, MAXPATHLEN);
                    wcsncat(wpath, L"\\CoreFoundation.resources\\", MAXPATHLEN - wcslen(wpath));
#else
                    strlcpy(cpath, __CharacterSetDir, MAXPATHLEN);
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
                
                bool __GetBitmapStateForName(const wchar_t *bitmapName) {
                    if (nullptr == __bitmapStateLock.DebugInfo)
                        InitializeCriticalSection(&__bitmapStateLock);
                    EnterCriticalSection(&__bitmapStateLock);
                    if (__nNumStateEntries >= 0) {
                        for (int i = 0; i < __nNumStateEntries; i++) {
                            if (wcscmp(mappedBitmapState[i], bitmapName) == 0) {
                                LeaveCriticalSection(&__bitmapStateLock);
                                return true;
                            }
                        }
                    }
                    LeaveCriticalSection(&__bitmapStateLock);
                    return false;
                }
                void __AddBitmapStateForName(const wchar_t *bitmapName) {
                    if (nullptr == __bitmapStateLock.DebugInfo)
                        InitializeCriticalSection(&__bitmapStateLock);
                    EnterCriticalSection(&__bitmapStateLock);
                    __nNumStateEntries++;
                    mappedBitmapState[__nNumStateEntries] = (wchar_t *)malloc((lstrlenW(bitmapName)+1) * sizeof(wchar_t));
                    lstrcpyW(mappedBitmapState[__nNumStateEntries], bitmapName);
                    LeaveCriticalSection(&__bitmapStateLock);
                }
#endif
                
#if DEPLOYMENT_TARGET_MACOSX || DEPLOYMENT_TARGET_EMBEDDED || DEPLOYMENT_TARGET_EMBEDDED_MINI || DEPLOYMENT_TARGET_LINUX
                static bool __UniCharLoadBytesFromFile(const char *fileName, const void **bytes, int64_t *fileSize) {
#elif DEPLOYMENT_TARGET_WINDOWS
                    static bool __UniCharLoadBytesFromFile(const wchar_t *fileName, const void **bytes, int64_t *fileSize) {
#else
#error Unknown or unspecified DEPLOYMENT_TARGET
#endif
#if DEPLOYMENT_TARGET_WINDOWS
                        HANDLE bitmapFileHandle = nullptr;
                        HANDLE mappingHandle = nullptr;
                        
                        if (__GetBitmapStateForName(fileName)) {
                            // The fileName has been tried in the past, so just return false
                            // and move on.
                            *bytes = nullptr;
                            return false;
                        }
                        mappingHandle = OpenFileMappingW(FILE_MAP_READ, TRUE, fileName);
                        if (nullptr == mappingHandle) {
                            if ((bitmapFileHandle = CreateFileW(fileName, GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_falseRMAL, nullptr)) == INVALID_HANDLE_VALUE) {
                                // We tried to get the bitmap file for mapping, but it's not there.  Add to list of non-existant bitmap-files so
                                // we don't have to try this again in the future.
                                __AddBitmapStateForName(fileName);
                                return false;
                            }
                            mappingHandle = CreateFileMapping(bitmapFileHandle, nullptr, PAGE_READONLY, 0, 0, nullptr);
                            CloseHandle(bitmapFileHandle);
                            if (!mappingHandle) return false;
                        }
                        
                        *bytes = MapViewOfFileEx(mappingHandle, FILE_MAP_READ, 0, 0, 0, 0);
                        
                        if (nullptr != fileSize) {
                            MEMORY_BASIC_INFORMATION memoryInfo;
                            
                            if (0 == VirtualQueryEx(mappingHandle, *bytes, &memoryInfo, sizeof(memoryInfo))) {
                                *fileSize = 0; // This indicates no checking. Is it right ?
                            } else {
                                *fileSize = memoryInfo.RegionSize;
                            }
                        }
                        
                        CloseHandle(mappingHandle);
                        
                        return (*bytes ? true : false);
#else
                        struct stat statBuf;
                        int fd = -1;
                        
                        if ((fd = open(fileName, O_RDONLY, 0)) < 0) {
                            return false;
                        }
                        if (fstat(fd, &statBuf) < 0 || (*bytes = mmap(0, statBuf.st_size, PROT_READ, MAP_PRIVATE, fd, 0)) == (void *)-1) {
                            close(fd);
                            return false;
                        }
                        close(fd);
                        
                        if (nullptr != fileSize) *fileSize = statBuf.st_size;
                        
                        return true;
#endif
                    }
                    
#endif // USE_MACHO_SEGMENT
                    
#if DEPLOYMENT_TARGET_MACOSX || DEPLOYMENT_TARGET_EMBEDDED || DEPLOYMENT_TARGET_EMBEDDED_MINI || DEPLOYMENT_TARGET_LINUX
                    static bool __UniCharLoadFile(const char *bitmapName, const void **bytes, int64_t *fileSize) {
#elif DEPLOYMENT_TARGET_WINDOWS
                        static bool __UniCharLoadFile(const wchar_t *bitmapName, const void **bytes, int64_t *fileSize) {
#else
#error Unknown or unspecified DEPLOYMENT_TARGET
#endif
#if USE_MACHO_SEGMENT
                            *bytes = __GetSectDataPtr("__UNICODE", bitmapName, nullptr);
                            
                            if (nullptr != fileSize) *fileSize = 0;
                            
                            return *bytes ? true : false;
#else
#if DEPLOYMENT_TARGET_MACOSX || DEPLOYMENT_TARGET_EMBEDDED || DEPLOYMENT_TARGET_EMBEDDED_MINI || DEPLOYMENT_TARGET_LINUX
                            char cpath[MAXPATHLEN];
                            __UniCharCharacterSetPath(cpath);
                            strlcat(cpath, bitmapName, MAXPATHLEN);
                            return __UniCharLoadBytesFromFile(cpath, bytes, fileSize);
#elif DEPLOYMENT_TARGET_WINDOWS
                            wchar_t wpath[MAXPATHLEN];
                            __UniCharCharacterSetPath(wpath);
                            wcsncat(wpath, bitmapName, MAXPATHLEN);
                            return __UniCharLoadBytesFromFile(wpath, bytes, fileSize);
#else
#error Unknown or unspecified DEPLOYMENT_TARGET
#endif
#endif
                        }
                        
                        // Bitmap functions
                        inline bool isControl(UTF32Char theChar, uint16_t charset, const void *data) { // ISO Control
                            return (((theChar <= 0x001F) || (theChar >= 0x007F && theChar <= 0x009F)) ? true : false);
                        }
                        
                        inline bool isWhitespace(UTF32Char theChar, uint16_t charset, const void *data) { // Space
                            return (((theChar == 0x0020) || (theChar == 0x0009) || (theChar == 0x00A0) || (theChar == 0x1680) || (theChar >= 0x2000 && theChar <= 0x200B) || (theChar == 0x202F) || (theChar == 0x205F) || (theChar == 0x3000)) ? true : false);
                        }
                        
                        inline bool isNewline(UTF32Char theChar, uint16_t charset, const void *data) { // White space
                            return (((theChar >= 0x000A && theChar <= 0x000D) || (theChar == 0x0085) || (theChar == 0x2028) || (theChar == 0x2029)) ? true : false);
                        }
                        
                        inline bool isWhitespaceAndNewline(UTF32Char theChar, uint16_t charset, const void *data) { // White space
                            return ((isWhitespace(theChar, charset, data) || isNewline(theChar, charset, data)) ? true : false);
                        }
                        
#if USE_MACHO_SEGMENT
                        inline bool __SimpleFileSizeVerification(const void *bytes, int64_t fileSize) { return true; }
#elif 1
                        // <rdar://problem/8961744> __SimpleFileSizeVerification is broken
                        static bool __SimpleFileSizeVerification(const void *bytes, int64_t fileSize) { return true; }
#else
                        static bool __SimpleFileSizeVerification(const void *bytes, int64_t fileSize) {
                            bool result = true;
                            
                            if (fileSize > 0) {
                                if ((sizeof(uint32_t) * 2) > fileSize) {
                                    result = false;
                                } else {
                                    uint32_t headerSize = SwapInt32BigToHost(*((uint32_t *)((char *)bytes + 4)));
                                    
                                    if ((headerSize < (sizeof(uint32_t) * 4)) || (headerSize > fileSize)) {
                                        result = false;
                                    } else {
                                        const uint32_t *lastElement = (uint32_t *)(((uint8_t *)bytes) + headerSize) - 2;
                                        
                                        if ((headerSize + SwapInt32BigToHost(lastElement[0]) + SwapInt32BigToHost(lastElement[1])) > headerSize) result = false;
                                    }
                                }
                            }
                            
                            if (!result) Log(LogLevelCritical, STR("File size verification for Unicode database file failed."));
                            
                            return result;
                        }
#endif // USE_MACHO_SEGMENT
                        
                        typedef struct {
                            uint32_t _numPlanes;
                            const uint8_t **_planes;
                        } __UniCharBitmapData;
                        
                        static char __UniCharUnicodeVersionString[8] = {0, 0, 0, 0, 0, 0, 0, 0};
                        
                        static uint32_t __UniCharNumberOfBitmaps = 0;
                        static __UniCharBitmapData *__UniCharBitmapDataArray = nullptr;
                        
                        static SpinLock __UniCharBitmapLock;
                        
#if DEPLOYMENT_TARGET_MACOSX || DEPLOYMENT_TARGET_EMBEDDED || DEPLOYMENT_TARGET_EMBEDDED_MINI || DEPLOYMENT_TARGET_LINUX
#if !defined(_UNICHAR_BITMAP_FILE)
#if USE_MACHO_SEGMENT
#define _UNICHAR_BITMAP_FILE "__csbitmaps"
#else
#define _UNICHAR_BITMAP_FILE "/CharacterSetBitmaps.bitmap"
#endif
#endif
#elif DEPLOYMENT_TARGET_WINDOWS
#if !defined(_UNICHAR_BITMAP_FILE)
#define _UNICHAR_BITMAP_FILE L"CharacterSetBitmaps.bitmap"
#endif
#else
#error Unknown or unspecified DEPLOYMENT_TARGET
#endif
                        
                        static bool __UniCharLoadBitmapData(void) {
                            __UniCharBitmapData *array;
                            uint32_t headerSize;
                            uint32_t bitmapSize;
                            int numPlanes;
                            uint8_t currentPlane;
                            const void *bytes;
                            const void *bitmapBase;
                            const void *bitmap;
                            int idx, bitmapIndex;
                            int64_t fileSize;
                            
                            __UniCharBitmapLock.Acquire();
                            
                            if (__UniCharBitmapDataArray || !__UniCharLoadFile(_UNICHAR_BITMAP_FILE, &bytes, &fileSize) || !__SimpleFileSizeVerification(bytes, fileSize)) {
                                __UniCharBitmapLock.Release();
                                return false;
                            }
                            
                            for (idx = 0;idx < 4 && ((const uint8_t *)bytes)[idx];idx++) {
                                __UniCharUnicodeVersionString[idx * 2] = ((const uint8_t *)bytes)[idx];
                                __UniCharUnicodeVersionString[idx * 2 + 1] = '.';
                            }
                            __UniCharUnicodeVersionString[(idx < 4 ? idx * 2 - 1 : 7)] = '\0';
                            
                            headerSize = SwapInt32BigToHost(*((uint32_t *)((char *)bytes + 4)));
                            
                            bitmapBase = (uint8_t *)bytes + headerSize;
                            bytes = (uint8_t *)bytes + (sizeof(uint32_t) * 2);
                            headerSize -= (sizeof(uint32_t) * 2);
                            
                            __UniCharNumberOfBitmaps = headerSize / (sizeof(uint32_t) * 2);
                            array = Allocator<__UniCharBitmapData>::AllocatorSystemDefault.Allocate<__UniCharBitmapData>(__UniCharNumberOfBitmaps);
                            
                            for (idx = 0;idx < (int)__UniCharNumberOfBitmaps;idx++) {
                                bitmap = (uint8_t *)bitmapBase + SwapInt32BigToHost(*((uint32_t *)bytes)); bytes = (uint8_t *)bytes + sizeof(uint32_t);
                                bitmapSize = SwapInt32BigToHost(*((uint32_t *)bytes)); bytes = (uint8_t *)bytes + sizeof(uint32_t);
                                
                                numPlanes = bitmapSize / (8 * 1024);
                                numPlanes = *(const uint8_t *)((char *)bitmap + (((numPlanes - 1) * ((8 * 1024) + 1)) - 1)) + 1;
                                //                            array[idx]._planes = (const uint8_t **)AllocatorAllocate(AllocatorSystemDefault, sizeof(const void *) * numPlanes);
                                array[idx]._planes = (const uint8_t**)(Allocator<uint8_t*>::AllocatorSystemDefault.Allocate<uint8_t *>(numPlanes));
                                
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
                                        array[idx]._planes[bitmapIndex] = nullptr;
                                    }
                                }
                            }
                            
                            __UniCharBitmapDataArray = array;
                            
                            __UniCharBitmapLock.Release();
                            
                            return true;
                        }
                        
                        Private const char *__UniCharGetUnicodeVersionString(void) {
                            if (nullptr == __UniCharBitmapDataArray) __UniCharLoadBitmapData();
                            return __UniCharUnicodeVersionString;
                        }
                        
                        bool UniCharIsMemberOf(UTF32Char theChar, CharacterSet charset) {
                            charset = __MapCompatibilitySetID(charset);
                            
                            switch (charset) {
                                case WhitespaceCharacterSet:
                                    return isWhitespace(theChar, charset, nullptr);
                                    
                                case WhitespaceAndNewlineCharacterSet:
                                    return isWhitespaceAndNewline(theChar, charset, nullptr);
                                    
                                case NewlineCharacterSet:
                                    return isNewline(theChar, charset, nullptr);
                                    
                                default: {
                                    uint32_t tableIndex = __MapExternalSetToInternalIndex(charset);
                                    
                                    if (nullptr == __UniCharBitmapDataArray) __UniCharLoadBitmapData();
                                    
                                    if (tableIndex < __UniCharNumberOfBitmaps) {
                                        __UniCharBitmapData *data = __UniCharBitmapDataArray + tableIndex;
                                        uint8_t planeNo = (theChar >> 16) & 0xFF;
                                        
                                        // The bitmap data for UniCharIllegalCharacterSet is actually LEGAL set less Plane 14 ~ 16
                                        if (charset == IllegalCharacterSet) {
                                            if (planeNo == 0x0E) { // Plane 14
                                                theChar &= 0xFF;
                                                return (((theChar == 0x01) || ((theChar > 0x1F) && (theChar < 0x80))) ? false : true);
                                            } else if (planeNo == 0x0F || planeNo == 0x10) { // Plane 15 & 16
                                                return ((theChar & 0xFF) > 0xFFFD ? true : false);
                                            } else {
                                                return (planeNo < data->_numPlanes && data->_planes[planeNo] ? !UniCharIsMemberOfBitmap(theChar, data->_planes[planeNo]) : true);
                                            }
                                        } else if (charset == ControlAndFormatterCharacterSet) {
                                            if (planeNo == 0x0E) { // Plane 14
                                                theChar &= 0xFF;
                                                return (((theChar == 0x01) || ((theChar > 0x1F) && (theChar < 0x80))) ? true : false);
                                            } else {
                                                return (planeNo < data->_numPlanes && data->_planes[planeNo] ? UniCharIsMemberOfBitmap(theChar, data->_planes[planeNo]) : false);
                                            }
                                        } else {
                                            return (planeNo < data->_numPlanes && data->_planes[planeNo] ? UniCharIsMemberOfBitmap(theChar, data->_planes[planeNo]) : false);
                                        }
                                    }
                                    return false;
                                }
                            }
                        }
                        
                        const uint8_t *UniCharGetBitmapPtrForPlane(CharacterSet charset, uint32_t plane) {
                            if (nullptr == __UniCharBitmapDataArray) __UniCharLoadBitmapData();
                            
                            charset = __MapCompatibilitySetID(charset);
                            
                            if ((charset > WhitespaceAndNewlineCharacterSet) && (charset != IllegalCharacterSet) && (charset != NewlineCharacterSet)) {
                                uint32_t tableIndex = __MapExternalSetToInternalIndex(charset);
                                
                                if (tableIndex < __UniCharNumberOfBitmaps) {
                                    __UniCharBitmapData *data = __UniCharBitmapDataArray + tableIndex;
                                    
                                    return (plane < data->_numPlanes ? data->_planes[plane] : nullptr);
                                }
                            }
                            return nullptr;
                        }
                        
                        uint8_t UniCharGetBitmapForPlane(CharacterSet charset, uint32_t plane, void *bitmap, bool isInverted) {
                            const uint8_t *src = UniCharGetBitmapPtrForPlane(charset, plane);
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
                                return UniCharBitmapFilled;
                            } else if (charset == IllegalCharacterSet) {
                                __UniCharBitmapData *data = __UniCharBitmapDataArray + __MapExternalSetToInternalIndex(__MapCompatibilitySetID(charset));
                                
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
                                    return UniCharBitmapFilled;
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
                                    return UniCharBitmapFilled;
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
                                    return UniCharBitmapFilled;
                                }
                                return (isInverted ? UniCharBitmapEmpty : UniCharBitmapAll);
                            } else if ((charset < DecimalDigitCharacterSet) || (charset == NewlineCharacterSet)) {
                                if (plane) return (isInverted ? UniCharBitmapAll : UniCharBitmapEmpty);
                                
                                uint8_t *bitmapBase = (uint8_t *)bitmap;
                                Index idx;
                                uint8_t nonFillValue = (isInverted ? (uint8_t)0xFF : (uint8_t)0);
                                
#if defined (__cplusplus)
                                while (numBytes-- > 0) *(((uint8_t *&)bitmap)++) = nonFillValue;
#else
                                while (numBytes-- > 0) *((uint8_t *)bitmap++) = nonFillValue;
#endif
                                
                                if ((charset == WhitespaceAndNewlineCharacterSet) || (charset == NewlineCharacterSet)) {
                                    const UniChar newlines[] = {0x000A, 0x000B, 0x000C, 0x000D, 0x0085, 0x2028, 0x2029};
                                    
                                    for (idx = 0;idx < (int)(sizeof(newlines) / sizeof(*newlines)); idx++) {
                                        if (isInverted) {
                                            UniCharRemoveCharacterFromBitmap(newlines[idx], bitmapBase);
                                        } else {
                                            UniCharAddCharacterToBitmap(newlines[idx], bitmapBase);
                                        }
                                    }
                                    
                                    if (charset == NewlineCharacterSet) return UniCharBitmapFilled;
                                }
                                
                                if (isInverted) {
                                    UniCharRemoveCharacterFromBitmap(0x0009, bitmapBase);
                                    UniCharRemoveCharacterFromBitmap(0x0020, bitmapBase);
                                    UniCharRemoveCharacterFromBitmap(0x00A0, bitmapBase);
                                    UniCharRemoveCharacterFromBitmap(0x1680, bitmapBase);
                                    UniCharRemoveCharacterFromBitmap(0x202F, bitmapBase);
                                    UniCharRemoveCharacterFromBitmap(0x205F, bitmapBase);
                                    UniCharRemoveCharacterFromBitmap(0x3000, bitmapBase);
                                } else {
                                    UniCharAddCharacterToBitmap(0x0009, bitmapBase);
                                    UniCharAddCharacterToBitmap(0x0020, bitmapBase);
                                    UniCharAddCharacterToBitmap(0x00A0, bitmapBase);
                                    UniCharAddCharacterToBitmap(0x1680, bitmapBase);
                                    UniCharAddCharacterToBitmap(0x202F, bitmapBase);
                                    UniCharAddCharacterToBitmap(0x205F, bitmapBase);
                                    UniCharAddCharacterToBitmap(0x3000, bitmapBase);
                                }
                                
                                for (idx = 0x2000;idx <= 0x200B;idx++) {
                                    if (isInverted) {
                                        UniCharRemoveCharacterFromBitmap(idx, bitmapBase);
                                    } else {
                                        UniCharAddCharacterToBitmap(idx, bitmapBase);
                                    }
                                }
                                return UniCharBitmapFilled;
                            }
                            return (isInverted ? UniCharBitmapAll : UniCharBitmapEmpty);
                        }
                        
                        Private uint32_t UniCharGetNumberOfPlanes(CharacterSet charset) {
                            if ((charset == ControlCharacterSet) || (charset == ControlAndFormatterCharacterSet)) {
                                return 15; // 0 to 14
                            } else if (charset < DecimalDigitCharacterSet) {
                                return 1;
                            } else if (charset == IllegalCharacterSet) {
                                return 17;
                            } else {
                                uint32_t numPlanes;
                                
                                if (nullptr == __UniCharBitmapDataArray) __UniCharLoadBitmapData();
                                
                                numPlanes = __UniCharBitmapDataArray[__MapExternalSetToInternalIndex(__MapCompatibilitySetID(charset))]._numPlanes;
                                
                                return numPlanes;
                            }
                        }
                        
                        // Mapping data loading
                        static const void **__UniCharMappingTables = nullptr;
                        
                        static SpinLock __UniCharMappingTableLock;
                        
#if DEPLOYMENT_TARGET_MACOSX || DEPLOYMENT_TARGET_EMBEDDED || DEPLOYMENT_TARGET_EMBEDDED_MINI || DEPLOYMENT_TARGET_LINUX
#if ___BIG_ENDIAN__
#if USE_MACHO_SEGMENT
#define MAPPING_TABLE_FILE "__data"
#else
#define MAPPING_TABLE_FILE "/UnicodeData-B.mapping"
#endif
#else
#if USE_MACHO_SEGMENT
#define MAPPING_TABLE_FILE "__data"
#else
#define MAPPING_TABLE_FILE "/UnicodeData-L.mapping"
#endif
#endif
#elif DEPLOYMENT_TARGET_WINDOWS
#if ___BIG_ENDIAN__
#if USE_MACHO_SEGMENT
#define MAPPING_TABLE_FILE "__data"
#else
#define MAPPING_TABLE_FILE L"UnicodeData-B.mapping"
#endif
#else
#if USE_MACHO_SEGMENT
#define MAPPING_TABLE_FILE "__data"
#else
#define MAPPING_TABLE_FILE L"UnicodeData-L.mapping"
#endif
#endif
#else
#error Unknown or unspecified DEPLOYMENT_TARGET
#endif
                        const void *UniCharEncodingPrivate::GetMappingData(UnicharMappingType type) {
                            __UniCharMappingTableLock.Acquire();
                            
                            if (nullptr == __UniCharMappingTables) {
                                const void *bytes;
                                const void *bodyBase;
                                int headerSize;
                                int idx, count;
                                int64_t fileSize;
                                
                                if (!__UniCharLoadFile(MAPPING_TABLE_FILE, &bytes, &fileSize) || !__SimpleFileSizeVerification(bytes, fileSize)) {
                                    __UniCharMappingTableLock.Release();
                                    return nullptr;
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
                                
                                __UniCharMappingTables = Allocator<const void *>::AllocatorSystemDefault.Allocate<const void *>(count);
                                
                                for (idx = 0;idx < count;idx++) {
#if defined (__cplusplus)
                                    __UniCharMappingTables[idx] = (char *)bodyBase + *((uint32_t *)bytes); bytes = (uint8_t *)bytes + sizeof(uint32_t);
#else
                                    __UniCharMappingTables[idx] = (char *)bodyBase + *((uint32_t *)bytes); bytes += sizeof(uint32_t);
#endif
                                }
                            }
                            
                            __UniCharMappingTableLock.Release();
                            
                            return __UniCharMappingTables[(Index)type];
                        }
                        
                        // Case mapping functions
#define DO_SPECIAL_CASE_MAPPING 1
                        
                        static uint32_t *__UniCharCaseMappingTableCounts = nullptr;
                        static uint32_t **__UniCharCaseMappingTable = nullptr;
                        static const uint32_t **__UniCharCaseMappingExtraTable = nullptr;
                        
                        typedef struct {
                            uint32_t _key;
                            uint32_t _value;
                        } __UniCharCaseMappings;
                        
                        /* Binary searches StringEncodingUnicodeTo8BitCharMap */
                        static uint32_t __UniCharGetMappedCase(const __UniCharCaseMappings *theTable, uint32_t numElem, UTF32Char character) {
                            const __UniCharCaseMappings *p, *q, *divider;
                            
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
                        
#define NUM_CASE_MAP_DATA (Index(UniCharCaseFold + 1))
                        
                        static bool __UniCharLoadCaseMappingTable(void) {
                            uint32_t *countArray;
                            int idx;
                            
                            if (nullptr == __UniCharMappingTables) (void)UniCharEncodingPrivate::GetMappingData(UnicharMappingType(UniCharToLowercase));
                            if (nullptr == __UniCharMappingTables) return false;
                            
                            __UniCharMappingTableLock.Acquire();
                            
                            if (__UniCharCaseMappingTableCounts) {
                                __UniCharMappingTableLock.Release();
                                return true;
                            }
                            
                            countArray = Allocator<uint32_t*>::AllocatorSystemDefault.Allocate<uint32_t>(NUM_CASE_MAP_DATA + NUM_CASE_MAP_DATA * 2);
                            __UniCharCaseMappingTable = (uint32_t **)((char *)countArray + sizeof(uint32_t) * NUM_CASE_MAP_DATA);
                            __UniCharCaseMappingExtraTable = (const uint32_t **)(__UniCharCaseMappingTable + NUM_CASE_MAP_DATA);
                            
                            for (idx = 0;idx < NUM_CASE_MAP_DATA;idx++) {
                                countArray[idx] = *((uint32_t *)__UniCharMappingTables[idx]) / (sizeof(uint32_t) * 2);
                                __UniCharCaseMappingTable[idx] = ((uint32_t *)__UniCharMappingTables[idx]) + 1;
                                __UniCharCaseMappingExtraTable[idx] = (const uint32_t *)((char *)__UniCharCaseMappingTable[idx] + *((uint32_t *)__UniCharMappingTables[idx]));
                            }
                            
                            __UniCharCaseMappingTableCounts = countArray;
                            
                            __UniCharMappingTableLock.Release();
                            return true;
                        }
                        
#if ___BIG_ENDIAN__
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
                        
                        Index UniCharMapCaseTo(UTF32Char theChar, UTF16Char *convertedChar, Index maxLength, uint32_t ctype, uint32_t flags, const uint8_t *langCode) {
                            __UniCharBitmapData *data;
                            uint8_t planeNo = (theChar >> 16) & 0xFF;
                            
                        caseFoldRetry:
                            
#if DO_SPECIAL_CASE_MAPPING
                            if (flags & UniCharCaseMapFinalSigma) {
                                if (theChar == 0x03A3) { // Final sigma
                                    *convertedChar = (ctype == UniCharToLowercase ? 0x03C2 : 0x03A3);
                                    return 1;
                                }
                            }
                            
                            if (langCode) {
                                if (flags & UniCharCaseMapGreekTonos) { // localized Greek uppercasing
                                    if (theChar == 0x0301) { // GREEK TOfalseS
                                        return 0;
                                    } else if (theChar == 0x0344) {// COMBINING GREEK DIALYTIKA TOfalseS
                                        *convertedChar = 0x0308; // COMBINING GREEK DIALYTIKA
                                        return 1;
                                    } else if (UniCharIsMemberOf(theChar, DecomposableCharacterSet)) {
                                        UTF32Char buffer[MAX_DECOMPOSED_LENGTH];
                                        Index length = Encoding::UnicodeDecoposition::DecomposeCharacter(theChar, buffer, MAX_DECOMPOSED_LENGTH);
                                        
                                        if (length > 1) {
                                            UTF32Char *characters = buffer + 1;
                                            UTF32Char *tail = buffer + length;
                                            
                                            while (characters < tail) {
                                                if (*characters == 0x0301) break;
                                                ++characters;
                                            }
                                            
                                            if (characters < tail) { // found a tonos
                                                Index convertedLength = UniCharMapCaseTo(*buffer, convertedChar, maxLength, ctype, 0, langCode);
                                                
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
                                        if (theChar == 0x0307 && (flags & UniCharCaseMapAfter_i)) {
                                            return 0;
                                        } else if (ctype == UniCharToLowercase) {
                                            if (flags & UniCharCaseMapMoreAbove) {
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
                                            *convertedChar = (((ctype == UniCharToLowercase) || (ctype == UniCharCaseFold))  ? ((UniCharCaseMapMoreAbove & flags) ? 0x0069 : 0x0131) : 0x0049);
                                            return 1;
                                        } else if ((theChar == 0x0069) || (theChar == 0x0130)) { // LATIN SMALL LETTER I & LATIN CAPITAL LETTER I WITH DOT ABOVE
                                            *convertedChar = (((ctype == UniCharToLowercase) || (ctype == UniCharCaseFold)) ? 0x0069 : 0x0130);
                                            return 1;
                                        } else if (theChar == 0x0307 && (UniCharCaseMapAfter_i & flags)) { // COMBINING DOT ABOVE AFTER_i
                                            if (ctype == UniCharToLowercase) {
                                                return 0;
                                            } else {
                                                *convertedChar = 0x0307;
                                                return 1;
                                            }
                                        }
                                        break;
                                        
                                    case DUTCH_LANG_CODE:
                                        if ((theChar == 0x004A) || (theChar == 0x006A)) {
                                            *convertedChar = (((ctype == UniCharToUppercase) || (ctype == UniCharToTitlecase) || (UniCharCaseMapDutchDigraph & flags)) ? 0x004A  : 0x006A);
                                            return 1;
                                        }
                                        break;
                                        
                                    default: break;
                                }
                            }
#endif // DO_SPECIAL_CASE_MAPPING
                            
                            if (nullptr == __UniCharBitmapDataArray) __UniCharLoadBitmapData();
                            
                            data = __UniCharBitmapDataArray + __MapExternalSetToInternalIndex(__MapCompatibilitySetID(CharacterSet((ctype + (UInt32)HasNonSelfLowercaseCharacterSet))));
                            
                            if (planeNo < data->_numPlanes && data->_planes[planeNo] && UniCharIsMemberOfBitmap(theChar, data->_planes[planeNo]) && (__UniCharCaseMappingTableCounts || __UniCharLoadCaseMappingTable())) {
                                uint32_t value = __UniCharGetMappedCase((const __UniCharCaseMappings *)__UniCharCaseMappingTable[ctype], __UniCharCaseMappingTableCounts[ctype], theChar);
                                
                                if (!value && ctype == UniCharToTitlecase) {
                                    value = __UniCharGetMappedCase((const __UniCharCaseMappings *)__UniCharCaseMappingTable[UniCharToUppercase], __UniCharCaseMappingTableCounts[UniCharToUppercase], theChar);
                                    if (value) ctype = UniCharToUppercase;
                                }
                                
                                if (value) {
                                    Index count = UniCharConvertFlagToCount(value);
                                    
                                    if (count == 1) {
                                        if (value & UniCharNonBmpFlag) {
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
                                        const uint32_t *extraMapping = __UniCharCaseMappingExtraTable[ctype] + (value & 0xFFFFFF);
                                        
                                        if (value & UniCharNonBmpFlag) {
                                            Index copiedLen = 0;
                                            
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
                                            Index idx;
                                            
                                            for (idx = 0;idx < count;idx++) *(convertedChar++) = (UTF16Char)*(extraMapping++);
                                            return count;
                                        }
                                    }
                                }
                            } else if (ctype == UniCharCaseFold) {
                                ctype = UniCharToLowercase;
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
                        
                        Index UniCharMapTo(UniChar theChar, UniChar *convertedChar, Index maxLength, uint16_t ctype, uint32_t flags) {
                            if (ctype == UniCharCaseFold + 1) { // UniCharDecompose
                                if (UnicodeDecoposition::IsDecomposableCharacter(theChar, false)) {
                                    UTF32Char buffer[MAX_DECOMPOSED_LENGTH];
                                    Index usedLength = UnicodeDecoposition::DecomposeCharacter(theChar, buffer, MAX_DECOMPOSED_LENGTH);
                                    Index idx;
                                    
                                    for (idx = 0;idx < usedLength;idx++) *(convertedChar++) = buffer[idx];
                                    return usedLength;
                                } else {
                                    *convertedChar = theChar;
                                    return 1;
                                }
                            } else {
                                return UniCharMapCaseTo(theChar, convertedChar, maxLength, ctype, flags, nullptr);
                            }
                        }
                        
                        inline bool __UniCharIsMoreAbove(UTF16Char *buffer, Index length) {
                            UTF32Char currentChar;
                            uint32_t property;
                            
                            while (length-- > 0) {
                                currentChar = *(buffer)++;
                                if (UniCharIsSurrogateHighCharacter(currentChar) && (length > 0) && UniCharIsSurrogateLowCharacter(*(buffer + 1))) {
                                    currentChar = UniCharGetLongCharacterForSurrogatePair(currentChar, *(buffer++));
                                    --length;
                                }
                                if (!UniCharIsMemberOf(currentChar, NonBaseCharacterSet)) break;
                                
                                property = UniCharGetCombiningPropertyForCharacter(currentChar, (const uint8_t *)UniCharGetUnicodePropertyDataForPlane(UniCharCombiningProperty, (currentChar >> 16) & 0xFF));
                                
                                if (property == 230) return true; // Above priority
                            }
                            return false;
                        }
                        
                        inline bool __UniCharIsAfter_i(UTF16Char *buffer, Index length) {
                            UTF32Char currentChar = 0;
                            uint32_t property;
                            UTF32Char decomposed[MAX_DECOMPOSED_LENGTH];
                            Index decompLength;
                            Index idx;
                            
                            if (length < 1) return 0;
                            
                            buffer += length;
                            while (length-- > 1) {
                                currentChar = *(--buffer);
                                if (UniCharIsSurrogateLowCharacter(currentChar)) {
                                    if ((length > 1) && UniCharIsSurrogateHighCharacter(*(buffer - 1))) {
                                        currentChar = UniCharGetLongCharacterForSurrogatePair(*(--buffer), currentChar);
                                        --length;
                                    } else {
                                        break;
                                    }
                                }
                                if (!UniCharIsMemberOf(currentChar, NonBaseCharacterSet)) break;
                                
                                property = UniCharGetCombiningPropertyForCharacter(currentChar, (const uint8_t *)UniCharGetUnicodePropertyDataForPlane(UniCharCombiningProperty, (currentChar >> 16) & 0xFF));
                                
                                if (property == 230) return false; // Above priority
                            }
                            if (length == 0) {
                                currentChar = *(--buffer);
                            } else if (UniCharIsSurrogateLowCharacter(currentChar) && UniCharIsSurrogateHighCharacter(*(--buffer))) {
                                currentChar = UniCharGetLongCharacterForSurrogatePair(*buffer, currentChar);
                            }
                            
                            decompLength = UnicodeDecoposition::DecomposeCharacter(currentChar, decomposed, MAX_DECOMPOSED_LENGTH);
                            currentChar = *decomposed;
                            
                            
                            for (idx = 1;idx < decompLength;idx++) {
                                currentChar = decomposed[idx];
                                property = UniCharGetCombiningPropertyForCharacter(currentChar, (const uint8_t *)UniCharGetUnicodePropertyDataForPlane(UniCharCombiningProperty, (currentChar >> 16) & 0xFF));
                                
                                if (property == 230) return false; // Above priority
                            }
                            return true;
                        }
                        
                        Private uint32_t UniCharGetConditionalCaseMappingFlags(UTF32Char theChar, UTF16Char *buffer, Index currentIndex, Index length, uint32_t type, const uint8_t *langCode, uint32_t lastFlags) {
                            if (theChar == 0x03A3) { // GREEK CAPITAL LETTER SIGMA
                                if ((type == UniCharToLowercase) && (currentIndex > 0)) {
                                    UTF16Char *start = buffer;
                                    UTF16Char *end = buffer + length;
                                    UTF32Char otherChar;
                                    
                                    // First check if we're after a cased character
                                    buffer += (currentIndex - 1);
                                    while (start <= buffer) {
                                        otherChar = *(buffer--);
                                        if (UniCharIsSurrogateLowCharacter(otherChar) && (start <= buffer) && UniCharIsSurrogateHighCharacter(*buffer)) {
                                            otherChar = UniCharGetLongCharacterForSurrogatePair(*(buffer--), otherChar);
                                        }
                                        if (!UniCharIsMemberOf(otherChar, CaseIgnorableCharacterSet)) {
                                            if (!UniCharIsMemberOf(otherChar, UppercaseLetterCharacterSet) && !UniCharIsMemberOf(otherChar, LowercaseLetterCharacterSet)) return 0; // Uppercase set contains titlecase
                                            break;
                                        }
                                    }
                                    
                                    // Next check if we're before a cased character
                                    buffer = start + currentIndex + 1;
                                    while (buffer < end) {
                                        otherChar = *(buffer++);
                                        if (UniCharIsSurrogateHighCharacter(otherChar) && (buffer < end) && UniCharIsSurrogateLowCharacter(*buffer)) {
                                            otherChar = UniCharGetLongCharacterForSurrogatePair(otherChar, *(buffer++));
                                        }
                                        if (!UniCharIsMemberOf(otherChar, CaseIgnorableCharacterSet)) {
                                            if (UniCharIsMemberOf(otherChar, UppercaseLetterCharacterSet) || UniCharIsMemberOf(otherChar, LowercaseLetterCharacterSet)) return 0; // Uppercase set contains titlecase
                                            break;
                                        }
                                    }
                                    return UniCharCaseMapFinalSigma;
                                }
                            } else if (langCode) {
                                if (*((const uint16_t *)langCode) == LITHUANIAN_LANG_CODE) {
                                    if ((theChar == 0x0307) && ((UniCharCaseMapAfter_i|UniCharCaseMapMoreAbove) & lastFlags) == (UniCharCaseMapAfter_i|UniCharCaseMapMoreAbove)) {
                                        return (__UniCharIsAfter_i(buffer, currentIndex) ? UniCharCaseMapAfter_i : 0);
                                    } else if (type == UniCharToLowercase) {
                                        if ((theChar == 0x0049) || (theChar == 0x004A) || (theChar == 0x012E)) {
                                            ++currentIndex;
                                            return (__UniCharIsMoreAbove(buffer + (currentIndex), length - currentIndex) ? UniCharCaseMapMoreAbove : 0);
                                        }
                                    } else if ((theChar == 'i') || (theChar == 'j')) {
                                        ++currentIndex;
                                        return (__UniCharIsMoreAbove(buffer + (currentIndex), length - currentIndex) ? (UniCharCaseMapAfter_i|UniCharCaseMapMoreAbove) : 0);
                                    }
                                } else if ((*((const uint16_t *)langCode) == TURKISH_LANG_CODE) || (*((const uint16_t *)langCode) == AZERI_LANG_CODE)) {
                                    if (type == UniCharToLowercase) {
                                        if (theChar == 0x0307) {
                                            return (UniCharCaseMapMoreAbove & lastFlags ? UniCharCaseMapAfter_i : 0);
                                        } else if (theChar == 0x0049) {
                                            return (((++currentIndex < length) && (buffer[currentIndex] == 0x0307)) ? UniCharCaseMapMoreAbove : 0);
                                        }
                                    }
                                } else if (*((const uint16_t *)langCode) == DUTCH_LANG_CODE) {
                                    if (UniCharCaseMapDutchDigraph & lastFlags) {
                                        return (((theChar == 0x006A) || (theChar == 0x004A)) ? UniCharCaseMapDutchDigraph : 0);
                                    } else {
                                        if ((type == UniCharToTitlecase) && ((theChar == 0x0069) || (theChar == 0x0049))) {
                                            return (((++currentIndex < length) && ((buffer[currentIndex] == 0x006A) || (buffer[currentIndex] == 0x004A))) ? UniCharCaseMapDutchDigraph : 0);
                                        }
                                    }
                                }
                                
                                if (UniCharCaseMapGreekTonos & lastFlags) { // still searching for tonos
                                    if (UniCharIsMemberOf(theChar, NonBaseCharacterSet)) {
                                        return UniCharCaseMapGreekTonos;
                                    }
                                }
                                if (((theChar >= 0x0370) && (theChar < 0x0400)) || ((theChar >= 0x1F00) && (theChar < 0x2000))) { // Greek/Coptic & Greek extended ranges
                                    if (((type == UniCharToUppercase) || (type == UniCharToTitlecase))&& (UniCharIsMemberOf(theChar, LetterCharacterSet))) return UniCharCaseMapGreekTonos;
                                }
                            }
                            return 0;
                        }
                        
                        // Unicode property database
                        static __UniCharBitmapData *__UniCharUnicodePropertyTable = nullptr;
                        static int __UniCharUnicodePropertyTableCount = 0;
                        
                        static SpinLock __UniCharPropTableLock;
                        
#if DEPLOYMENT_TARGET_MACOSX || DEPLOYMENT_TARGET_EMBEDDED || DEPLOYMENT_TARGET_EMBEDDED_MINI || DEPLOYMENT_TARGET_LINUX
#if USE_MACHO_SEGMENT
#define PROP_DB_FILE "__properties"
#else
#define PROP_DB_FILE "/UniCharPropertyDatabase.data"
#endif
#elif DEPLOYMENT_TARGET_WINDOWS
#if USE_MACHO_SEGMENT
#define PROP_DB_FILE "__properties"
#else
#define PROP_DB_FILE L"UniCharPropertyDatabase.data"
#endif
#else
#error Unknown or unspecified DEPLOYMENT_TARGET
#endif
                        
                        const void *UniCharGetUnicodePropertyDataForPlane(uint32_t propertyType, uint32_t plane) {

                            __UniCharPropTableLock.Acquire();
                            if (nullptr == __UniCharUnicodePropertyTable) {
                                __UniCharBitmapData *table;
                                const void *bytes;
                                const void *bodyBase;
                                const void *planeBase;
                                int headerSize;
                                int idx, count;
                                int planeIndex, planeCount;
                                int planeSize;
                                int64_t fileSize;
                                
                                if (!__UniCharLoadFile(PROP_DB_FILE, &bytes, &fileSize) || !__SimpleFileSizeVerification(bytes, fileSize)) {
                                    __UniCharPropTableLock.Release();
                                    return nullptr;
                                }
                                
#if defined (__cplusplus)
                                bytes = (uint8_t*)bytes + 4; // Skip Unicode version
                                headerSize = SwapInt32BigToHost(*((uint32_t *)bytes)); bytes = (uint8_t *)bytes + sizeof(uint32_t);
#else
                                bytes += 4; // Skip Unicode version
                                headerSize = SwapInt32BigToHost(*((uint32_t *)bytes)); bytes += sizeof(uint32_t);
#endif
                                
                                headerSize -= (sizeof(uint32_t) * 2);
                                bodyBase = (char *)bytes + headerSize;
                                
                                count = headerSize / sizeof(uint32_t);
                                __UniCharUnicodePropertyTableCount = count;
                                
                                table = Allocator<__UniCharBitmapData>::AllocatorSystemDefault.Allocate<__UniCharBitmapData>(count);
                                
                                for (idx = 0;idx < count;idx++) {
                                    planeCount = *((const uint8_t *)bodyBase);
                                    planeBase = (char *)bodyBase + planeCount + (planeCount % 4 ? 4 - (planeCount % 4) : 0);
                                    table[idx]._planes = Allocator<const uint8_t**>::AllocatorSystemDefault.Allocate<const uint8_t*>(planeCount);
                                    for (planeIndex = 0;planeIndex < planeCount;planeIndex++) {
                                        if ((planeSize = ((const uint8_t *)bodyBase)[planeIndex + 1])) {
                                            table[idx]._planes[planeIndex] = (const uint8_t *)planeBase;
#if defined (__cplusplus)
                                            planeBase = (char*)planeBase + (planeSize * 256);
#else
                                            planeBase += (planeSize * 256);
#endif
                                        } else {
                                            table[idx]._planes[planeIndex] = nullptr;
                                        }
                                    }
                                    
                                    table[idx]._numPlanes = planeCount;
#if defined (__cplusplus)
                                    bodyBase = (const uint8_t *)bodyBase + (SwapInt32BigToHost(*(uint32_t *)bytes));
                                    ((uint32_t *&)bytes) ++;
#else
                                    bodyBase += (SwapInt32BigToHost(*((uint32_t *)bytes++)));
#endif
                                }
                                
                                __UniCharUnicodePropertyTable = table;
                            }
                            
                            __UniCharPropTableLock.Release();
                            
                            return (plane < __UniCharUnicodePropertyTable[propertyType]._numPlanes ? __UniCharUnicodePropertyTable[propertyType]._planes[plane] : nullptr);
                        }
                        
                        Private uint32_t UniCharGetNumberOfPlanesForUnicodePropertyData(uint32_t propertyType) {
                            (void)UniCharGetUnicodePropertyDataForPlane(propertyType, 0);
                            return __UniCharUnicodePropertyTable[propertyType]._numPlanes;
                        }
                        
                        Private uint32_t UniCharGetUnicodeProperty(UTF32Char character, uint32_t propertyType) {
                            if (propertyType == UniCharCombiningProperty) {
                                return UniCharGetCombiningPropertyForCharacter(character, (const uint8_t *)UniCharGetUnicodePropertyDataForPlane(propertyType, (character >> 16) & 0xFF));
                            } else if (propertyType == UniCharBidiProperty) {
                                return UniCharGetBidiPropertyForCharacter(character, (const uint8_t *)UniCharGetUnicodePropertyDataForPlane(propertyType, (character >> 16) & 0xFF));
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
                        
                        bool UniCharFillDestinationBuffer(const UTF32Char *src, Index srcLength, void **dst, Index dstLength, Index *filledLength, UniCharEncodingFormat dstFormat)
                        {
                            UTF32Char currentChar;
                            Index usedLength = *filledLength;
                            
                            if (dstFormat == UniCharEncodingFormat::UTF16Format) {
                                UTF16Char *dstBuffer = (UTF16Char *)*dst;
                                
                                while (srcLength-- > 0) {
                                    currentChar = *(src++);
                                    
                                    if (currentChar > 0xFFFF) { // Non-BMP
                                        usedLength += 2;
                                        if (dstLength) {
                                            if (usedLength > dstLength) return false;
                                            currentChar -= 0x10000;
                                            *(dstBuffer++) = (UTF16Char)((currentChar >> 10) + 0xD800UL);
                                            *(dstBuffer++) = (UTF16Char)((currentChar & 0x3FF) + 0xDC00UL);
                                        }
                                    } else {
                                        ++usedLength;
                                        if (dstLength) {
                                            if (usedLength > dstLength) return false;
                                            *(dstBuffer++) = (UTF16Char)currentChar;
                                        }
                                    }
                                }
                                
                                *dst = dstBuffer;
                            } else if (dstFormat == UniCharEncodingFormat::UTF8Format) {
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
                                        if (usedLength > dstLength) return false;
                                        
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
                                        if (usedLength > dstLength) return false;
                                        *(dstBuffer++) = currentChar;
                                    }
                                }
                                
                                *dst = dstBuffer;
                            }
                            
                            *filledLength = usedLength;
                            
                            return true;
                        }
                        
#if DEPLOYMENT_TARGET_WINDOWS
                        void __UniCharCleanup(void)
                        {
                            int	idx;
                            
                            // cleanup memory allocated by __UniCharLoadBitmapData()
                            SpinLockLock(&__UniCharBitmapLock);
                            
                            if (__UniCharBitmapDataArray != nullptr) {
                                for (idx = 0; idx < (int)__UniCharNumberOfBitmaps; idx++) {
                                    AllocatorDeallocate(AllocatorSystemDefault, __UniCharBitmapDataArray[idx]._planes);
                                    __UniCharBitmapDataArray[idx]._planes = nullptr;
                                }
                                
                                AllocatorDeallocate(AllocatorSystemDefault, __UniCharBitmapDataArray);
                                __UniCharBitmapDataArray = nullptr;
                                __UniCharNumberOfBitmaps = 0;
                            }
                            
                            SpinLockUnlock(&__UniCharBitmapLock);
                            
                            // cleanup memory allocated by UniCharGetMappingData()
                            SpinLockLock(&__UniCharMappingTableLock);
                            
                            if (__UniCharMappingTables != nullptr) {
                                AllocatorDeallocate(AllocatorSystemDefault, __UniCharMappingTables);
                                __UniCharMappingTables = nullptr;
                            }
                            
                            // cleanup memory allocated by __UniCharLoadCaseMappingTable()
                            if (__UniCharCaseMappingTableCounts != nullptr) {
                                AllocatorDeallocate(AllocatorSystemDefault, __UniCharCaseMappingTableCounts);
                                __UniCharCaseMappingTableCounts = nullptr;
                                
                                __UniCharCaseMappingTable = nullptr;
                                __UniCharCaseMappingExtraTable = nullptr;
                            }
                            
                            SpinLockUnlock(&__UniCharMappingTableLock);
                            
                            // cleanup memory allocated by UniCharGetUnicodePropertyDataForPlane()
                            SpinLockLock(&__UniCharPropTableLock);
                            
                            if (__UniCharUnicodePropertyTable != nullptr) {
                                for (idx = 0; idx < __UniCharUnicodePropertyTableCount; idx++) {
                                    AllocatorDeallocate(AllocatorSystemDefault, __UniCharUnicodePropertyTable[idx]._planes);
                                    __UniCharUnicodePropertyTable[idx]._planes = nullptr;
                                }
                                
                                AllocatorDeallocate(AllocatorSystemDefault, __UniCharUnicodePropertyTable);
                                __UniCharUnicodePropertyTable = nullptr;
                                __UniCharUnicodePropertyTableCount = 0;
                            }
                            
                            SpinLockUnlock(&__UniCharPropTableLock);
                        }
#endif
                        
#undef USE_MACHO_SEGMENT
                    }
                }
            }