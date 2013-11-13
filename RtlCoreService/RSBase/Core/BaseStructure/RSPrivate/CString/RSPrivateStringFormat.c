//
//  RSPrivateStringFormat.c
//  RSCoreFoundation
//
//  Created by RetVal on 10/24/12.
//  Copyright (c) 2012 RetVal. All rights reserved.
//

#include "RSPrivateStringFormat.h"
#include "RSPrivateBuffer.h"
#include "RSString.h"

//#include <objc/runtime.h>
//#include <objc/message.h>
static RSIndex __RSFormatAddress(RSBuffer* buffer, RSBuffer format, va_list ap);
static RSIndex __RSFormatCString(RSBuffer* buffer, RSBuffer format, va_list ap);

static const char __RSFormatKeys[] = "-+ 0#";
static const char __RSFormatAccuracyKey = '.';
static const char __RSFormatLengthKeys[] = "hlL";
static const char __RSFormatControllerKeys[] = "nfrtx"; // \n \f \r \t \xhh

/*
 Format String format :
 % [flags] [field_width] [.precision] [length_modifier] conversion_character
 
 */




static RSIndex __RSFormatNumber(RSBuffer* buffer, RSBuffer format, va_list ap)
{
    //register RSIndex width, pad;
    register RSIndex pc = 0;
    //char cache_buffer[32] = "";
    
    return pc;
}


static void __RSBufferPutChar(RSBuffer* buffer, RSCBlock c)
{
    if (buffer) {
        **buffer = c;
        (*buffer) ++;
    }
}
#define PAD_RIGHT 1
#define PAD_ZERO 2
#define __RSFormatSize(size,cs) if ((++(cs)) > (size)) { \
realloc(*bufRef,size*2); \
size = (((size)+(16-1)) &~ (16-1));\
}
#define ISDIGIT(c)  ((0 <= ((c)-'0')) && (((c)-'0')  <= 9))

#define COPY_VA_INT \
do { \
const int value = abs (va_arg (ap, int)); \
char buf[32]; \
ptr++; /* Go past the asterisk.  */ \
*sptr = '\0'; /* nil terminate sptr.  */ \
sprintf(buf, "%d", value); \
strcat(sptr, buf); \
while (*sptr) sptr++; \
} while (0)

#define PRINT_CHAR(CHAR) \
do { \
__RSBufferPutChar(&base, CHAR); \
ptr++; \
pc++; \
continue; \
} while (0)

RSIndex __RSFormatPrint(RSBuffer* buf, RSIndex* retBufferSize, RSCUBuffer format, va_list ap)
{
    RSIndex result = -1;
    if (buf == nil) return result;
    RSCUBuffer ptr = format;
    RSBuffer base = *buf;
    register RSIndex pc = 0;        // the offset of the base;
    register RSIndex length = 0;    // the block unit need size;
    register RSIndex size = 0;      // the buffer capacity;
                                    // the current buffer size is (size - pc)
    char specifier[128] = "";
    char cacheForNumber[128] = "";
    if (*buf == nil)
    {
        *buf = RSAllocatorAllocate(RSAllocatorSystemDefault, 64);
        size = RSAllocatorSize(RSAllocatorSystemDefault, 64);
        base = *buf;
    }
    //RSPrintValue printValue[RSFormatBuffer] = {0};
    while (*ptr != '\0')
    {
        if (*ptr == '%')
        {
            //++ptr;
            // check the flags
            //width = pad = 0;
            char * sptr = specifier;
            int wide_width = 0, short_width = 0;
            //RSPrintValue* printValue = nil;
            *sptr++ = *ptr++; // Copy the % and move forward.
            if (*ptr == '\0') break; // if the last character is %, here should return.
            if (*ptr != '%')
            {
                
                char* __addr = strchr (__RSFormatKeys, *ptr);
                while (__addr && *__addr != '\0') /* Move past flags.  */
                {
                    *sptr++ = *ptr++;
                    __addr = strchr (__RSFormatKeys, *ptr);
                }
                if (*ptr == '*')
                    COPY_VA_INT;
                else
                    while (ISDIGIT(*ptr)) /* Handle explicit numeric value.  */
                        *sptr++ = *ptr++;
                if (*ptr == '.')
                {
                    *sptr++ = *ptr++; /* Copy and go past the period.  */
                    if (*ptr == '*')
                        COPY_VA_INT;
                    else
                        while (ISDIGIT(*ptr)) /* Handle explicit numeric value.  */
                            *sptr++ = *ptr++;
                }
                __addr = strchr (__RSFormatLengthKeys, *ptr);
                while (__addr && *__addr != '\0')
                {
                    switch (*ptr)
                    {
                        case 'h':
                            short_width = 1;
                            break;
                        case 'l':
                            wide_width++;
                            break;
                        case 'L':
                            wide_width = 2;
                            break;
                        default:
                            abort();
                    }
                    *sptr++ = *ptr++;
                    __addr = strchr (__RSFormatLengthKeys, *ptr);
                }
                RSStringRef des = nil;
                char* cStr = nil;
                switch (*ptr)
                {
                    case 'd':
                    case 'i':
                    case 'o':
                    case 'u':
                    case 'x':
                    case 'X':
                    case 'c':
                    {
                        *sptr++ = *ptr++; /* Copy the type specifier.  */
                        *sptr = '\0'; /* nil terminate sptr.  */
                        if (short_width)
                        {
                            //PRINT_TYPE(int);
                        RETRY_B1:
                            //length = vsnprintf(base, size - pc, specifier, ap);
                            length = vsnprintf(cacheForNumber, sizeof(cacheForNumber), specifier, ap);
                            if (length < (size - pc))
                            {
                                strcat(base, cacheForNumber);
                                base += length;
                                pc += length;
                            }
                            else
                            {
                                RSIndex tmp = 0;
                                if (size > length) tmp = size*2;
                                else tmp = size + length + 1;
                                *buf = RSAllocatorReallocate(RSAllocatorSystemDefault, *buf, tmp);
                                base = *buf; base += pc;
                                size = tmp;
                                strcat(base, cacheForNumber);
                                base += length;
                                pc += length;
                                //goto RETRY_B1;
                            }
                            memset(cacheForNumber, 0, length);
                            //                            printValue[idxOfArray].value.int64Value = va_arg(ap, int);
                            //                            printValue[idxOfArray].type = RSFormatLongType;
                            //                            printValue[idxOfArray].size = RSFormatSizeLong;
                            
                            
                        }
                        else
                        {
                            
                            switch (wide_width)
                            {
                                case 0:
                                    //PRINT_TYPE(int);
                                RETRY_B2:
                                    //length = vsnprintf(base, size - pc, specifier, ap);
                                    length = vsnprintf(cacheForNumber, sizeof(cacheForNumber), specifier, ap);
                                    if (length < (size - pc))
                                    {
                                        strcat(base, cacheForNumber);
                                        base  += length;
                                        pc += length;
                                    }
                                    else
                                    {
                                        RSIndex tmp = 0;
                                        if (size > length) tmp = size*2;
                                        else tmp = size + length + 1;
                                        *buf = RSAllocatorReallocate(RSAllocatorSystemDefault, *buf, tmp);
                                        base = *buf; base += pc;
                                        size = tmp;
                                        strcat(base, cacheForNumber);
                                        base += length;
                                        pc += length;
                                        //goto RETRY_B2;
                                    }
                                    memset(cacheForNumber, 0, length);
                                    //                                    printValue[idxOfArray].value.int64Value = va_arg(ap, int);
                                    //                                    printValue[idxOfArray].type = RSFormatLongType;
                                    //                                    printValue[idxOfArray].size = RSFormatSizeLong;
                                    break;
                                case 1:
                                RETRY_B3:
                                    //PRINT_TYPE(long);
                                    //length = vsnprintf(base, size - pc, specifier, ap);
                                    length = vsnprintf(cacheForNumber, sizeof(cacheForNumber), specifier, ap);
                                    if (length < (size - pc))
                                    {
                                        strcat(base, cacheForNumber);
                                        base  += length;
                                        pc += length;
                                    }
                                    else
                                    {
                                        RSIndex tmp = 0;
                                        if (size > length) tmp = size*2;
                                        else tmp = size + length + 1;
                                        *buf = RSAllocatorReallocate(RSAllocatorSystemDefault, *buf, tmp);
                                        base = *buf; base += pc;
                                        size = tmp;
                                        strcat(base, cacheForNumber);
                                        base += length;
                                        pc += length;
                                        //goto RETRY_B3;
                                    }
                                    memset(cacheForNumber, 0, length);
                                    //                                    printValue[idxOfArray].value.int64Value = va_arg(ap, long);
                                    //                                    printValue[idxOfArray].type = RSFormatLongType;
                                    //                                    printValue[idxOfArray].size = RSFormatSizeLong;
                                    break;
                                case 2:
                                default:
                                RETRY_B4:
#if defined(__GNUC__) || defined(HAVE_LONG_LONG)
                                    //PRINT_TYPE(long long);
                                    length = vsnprintf(cacheForNumber, sizeof(cacheForNumber), specifier, ap);
                                    if (length < (size - pc))
                                    {
                                        strcat(base, cacheForNumber);
                                        base += length;
                                        pc += length;
                                    }
                                    else
                                    {
                                        RSIndex tmp = 0;
                                        if (size > length) tmp = size*2;
                                        else tmp = size + length + 1;
                                        *buf = RSAllocatorReallocate(RSAllocatorSystemDefault, *buf, tmp);
                                        base = *buf; base += pc;
                                        size = tmp;
                                        strcat(base, cacheForNumber);
                                        base += length;
                                        pc += length;
                                        //goto RETRY_B4;
                                    }
                                    memset(cacheForNumber, 0, length);
                                    
                                    //                                    printValue[idxOfArray].value.int64Value = va_arg(ap, long long);
                                    //                                    printValue[idxOfArray].type = RSFormatLongType;
                                    //                                    printValue[idxOfArray].size = RSFormatSizeLong;
#else
                                    //PRINT_TYPE(long); /* Fake it and hope for the best.  */
#endif
                                    break;
                            }
                        }
                    }
                        // number end.
                        break;
                    case 'f':
                    case 'e':
                    case 'E':
                    case 'g':
                    case 'G':
                    {
                        *sptr++ = *ptr++; /* Copy the type specifier.  */
                        *sptr = '\0'; /* nil terminate sptr.  */
                        if (wide_width == 0)
                        {
                            //RSIndex result;
                            //                            *sptr++ = *ptr++; /* Copy the type specifier.  */
                            //                            *sptr = '\0'; /* nil terminate sptr.  */
                        RETRY_B5:
                            length = vsnprintf(cacheForNumber, sizeof(cacheForNumber), specifier, ap);
                            if (length < (size - pc))
                            {
                                strcat(base, cacheForNumber);
                                base += length;
                                pc += length;
                            }
                            else
                            {
                                RSIndex tmp = 0;
                                if (size > length) tmp = size*2;
                                else tmp = size + length + 1;
                                *buf = RSAllocatorReallocate(RSAllocatorSystemDefault, *buf, tmp);
                                base = *buf; base += pc;
                                size = tmp;
                                strcat(base, cacheForNumber);
                                base += length;
                                pc += length;
                                //goto RETRY_B5;
                            }
                            memset(cacheForNumber, 0, length);
                            //                            printValue[idxOfArray].value.doubleValue = va_arg(ap, double);
                            //                            printValue[idxOfArray].type = RSFormatDoubleType;
                            //                            printValue[idxOfArray].size = RSFormatSize8;
                            
                        }// PRINT_TYPE(double);
                        else
                        {
#if defined(__GNUC__) || defined(HAVE_LONG_DOUBLE)
                            //PRINT_TYPE(long double);
                            //                            *sptr++ = *ptr++; /* Copy the type specifier.  */
                            //                            *sptr = '\0'; /* nil terminate sptr.  */
                        RETRY_B6:
                            length = vsnprintf(cacheForNumber, sizeof(cacheForNumber), specifier, ap);
                            if (length < (size - pc))
                            {
                                strcat(base, cacheForNumber);
                                base += length;
                                pc += length;
                            }
                            else
                            {
                                RSIndex tmp = 0;
                                if (size > length) tmp = size*2;
                                else tmp = size + length + 1;
                                *buf = RSAllocatorReallocate(RSAllocatorSystemDefault, *buf, tmp);
                                base = *buf; base += pc;
                                size = tmp;
                                strcat(base, cacheForNumber);
                                base += length;
                                pc += length;
                                //goto RETRY_B6;
                            }
                            memset(cacheForNumber, 0, length);
                            //                            printValue[idxOfArray].value.doubleValue = va_arg(ap, double);
                            //                            printValue[idxOfArray].type = RSFormatDoubleType;
                            //                            printValue[idxOfArray].size = RSFormatSize8;
#else
                            //PRINT_TYPE(double); /* Fake it and hope for the best.  */
                        RETRY_B6:
                            length = vsnprintf(cacheForNumber, sizeof(cacheForNumber), specifier, ap);
                            if (length < (size - pc))
                            {
                                strcat(base, cacheForNumber);
                                base += length;
                                pc += length;
                            }
                            else
                            {
                                RSIndex tmp = 0;
                                if (size > length) tmp = size*2;
                                else tmp = size + length + 1;
                                *buf = RSAllocatorReallocate(RSAllocatorSystemDefault, *buf, tmp);
                                base = *buf; base += pc;
                                size = tmp;
                                strcat(base, cacheForNumber);
                                base += length;
                                pc += length;
                                //goto RETRY_B6;
                            }
                            memset(cacheForNumber, 0, length);
#endif
                        }
                    }
                        break;
                    case 's':
                        //PRINT_TYPE(char *);
                        *sptr++ = *ptr++; /* Copy the type specifier.  */
                        *sptr = '\0'; /* nil terminate sptr.  */
                        cStr = va_arg(ap, char*);
                    RETRY_B7:
                        if ((length = strlen(cStr)) < (size - pc))
                        {
                            strcat(base, cStr);
                            base += length;
                            pc += length;
                        }
                        else
                        {
                            RSIndex tmp = 0;
                            if (size > length) tmp = size*2;
                            else tmp = size + length + 1;
                            *buf = RSAllocatorReallocate(RSAllocatorSystemDefault, *buf, tmp);
                            base = *buf; base += pc;
                            size = tmp;
                            goto RETRY_B7;
                        }
                        //                        printValue[idxOfArray].value.pointerValue = va_arg(ap, char *);
                        //                        printValue[idxOfArray].type = RSFormatPointerType;
                        //                        printValue[idxOfArray].size = RSFormatSizePointer;
                        
                        //pc += __RSFormatCString(base, sptr, ap);
                        break;
                    case 'R':
                    case 'r':
                        *sptr++ = *ptr++; /* Copy the type specifier.  */
                        *sptr = '\0'; /* nil terminate sptr.  */
                        des = RSDescription(va_arg(ap, RSTypeRef));
                        length = RSStringGetLength(des);
                        
                        //length = vsnprintf(base, size, specifier, ap);
                    RETRY_B8:
                        if (length < (size - pc))
                        {
                            strcat(base, RSStringGetCStringPtr(des, RSStringEncodingUTF8));
                            RSRelease(des);
                            base += length;
                            pc += length;
                        }
                        else
                        {
                            RSIndex tmp = 0;
                            if (size > length) tmp = size*2;
                            else tmp = size + length + 1;
                            *buf = RSAllocatorReallocate(RSAllocatorSystemDefault, *buf, tmp);
                            base = *buf; base += pc;
                            size = tmp;
                            goto RETRY_B8;
                        }
                        
                        break;
                    case '@':   // reserved for Objective-C do not use right now. 2013-6-1 active
//                        *sptr++ = *ptr++;
//                        *sptr = '\0';
//                        id instance = nil;
//                        const char *cls_name = class_getName(object_getClass(instance = objc_msgSend(va_arg(ap, id), @selector(description))));
//                        if (0 == strncmp(cls_name, "KSString", 8) ||
//                            0 == strncmp(cls_name, "KSMutableString", 15))
//                        {
//                            des = (RSStringRef)RSRetain(objc_msgSend(instance, @selector(_RSString)));
//                            length = RSStringGetLength(des);
//                        RETRY_BOC:
//                            if (length < (size - pc))
//                            {
//                                strcat(base, RSStringGetCStringPtr(des));
//                                RSRelease(des);
//                                base += length;
//                                pc += length;
////                                objc_msgSend(instance, @selector(release));
//                            }
//                            else
//                            {
//                                RSIndex tmp = 0;
//                                if (size > length) tmp = size*2;
//                                else tmp = size + length + 1;
//                                *buf = RSAllocatorReallocate(RSAllocatorSystemDefault, *buf, tmp);
//                                base = *buf; base += pc;
//                                size = tmp;
//                                goto RETRY_BOC;
//                            }
//                        }
                        break;
                    case 'p':
                        //PRINT_TYPE(void *);
                        *sptr++ = *ptr++; /* Copy the type specifier.  */
                        *sptr = '\0'; /* nil terminate sptr.  */
                    RETRY_B9:
                        length = vsnprintf(cacheForNumber, sizeof(cacheForNumber), specifier, ap);
                        //length = vsnprintf(base, (size - pc), specifier, ap);
                        if (length < (size - pc))
                        {
                            strcat(base, cacheForNumber);
                            base += length;
                            pc += length;
                        }
                        else
                        {
                            RSIndex tmp = 0;
                            if (size > length) tmp = size*2;
                            else tmp = size + length + 1;
                            *buf = RSAllocatorReallocate(RSAllocatorSystemDefault, *buf, tmp);
                            base = *buf; base += pc;
                            size = tmp;
                            strcat(base, cacheForNumber);
                            base += length;
                            pc += length;
                            //goto RETRY_B9;
                        }
                        memset(cacheForNumber, 0, length);
                        //                        printValue[idxOfArray].value.pointerValue = va_arg(ap, void *);
                        //                        printValue[idxOfArray].type = RSFormatPointerType;
                        //                        printValue[idxOfArray].size = RSFormatSizePointer;
                        //pc += __RSFormatAddress(base, sptr, ap);
                        break;
                    case '%':
                        PRINT_CHAR('%');
                        break;
                }
            }
        }// if (*ptr == '%')
        else
        {
        normal:
        RETRY_B10:
            if ((size - pc) > 1) {
                __RSBufferPutChar(&base, *ptr);++pc;++ptr;
            }else
            {
                RSIndex tmp = 0;
                if (size > length) tmp = size*2;
                else tmp = size + length + 1;
                *buf = RSAllocatorReallocate(RSAllocatorSystemDefault, *buf, tmp);
                base = *buf; base += pc;
                size = tmp;
                goto RETRY_B10;
             }
              // buffer add itself automatically.
        }
    }// end while (*ptr != '\0')
    result = 1;
end:
    va_end(ap);
    if (retBufferSize) *retBufferSize = size;
    return (result == -1) ? -1 : pc;
}

RSIndex __RSFormatConvertHex(RSBuffer buffer, RSIndex n, BOOL captial)
{
    RSIndex i = 0;
    RSIndex pc = 0;
    char a[sizeof(void*)*2 + 3] = "";
    for (i = 0; n; n /= 16)
    {
        RSIndex b = n % 16;
        if (b > 9)
            a[i++] = b + 'a' - 10;
        else
            a[i++] = b + '0';
    }
//    (buffer)[0] = '0';
//    (buffer)[1] = 'x';
//    pc = 2 + i;
    for (RSIndex j = 2; i > 0;j++, --i)
    {
        //printf ("%c", a[i - 1]);
        (buffer)[j] = a[i - 1];
    }
    return pc;
}

static RSIndex __RSFormatAddress(RSBuffer* buffer, RSBuffer format, va_list ap)
{
    register RSIndex pc = 0;
    register RSIndex len = 0;
    char cache_buffer[sizeof(void*)*2 + 3] = "";
    cache_buffer[0] = '0';
    cache_buffer[1] = (format[strlen(format)] == 'P') ? ('X'):('x');
    RSIndex address = (RSIndex)(RSIndex*)va_arg(ap, void*); //get the address
    //strncat(cache_buffer, "0x", len = strlen("0x"));
    //pc += len;
    __RSFormatConvertHex(cache_buffer, address, (format[strlen(format)] == 'P'));
    strncpy(*buffer, cache_buffer, len = strlen(cache_buffer));
    pc += len;
    *buffer += pc;
    return pc;
}

