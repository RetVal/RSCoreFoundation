//
//  RSPriveteBuffer.c
//  RSCoreFoundation
//
//  Created by RetVal on 10/30/12.
//  Copyright (c) 2012 RetVal. All rights reserved.
//

#include <stdio.h>
#include "RSPrivateBuffer.h"

static unsigned char utf8_look_for_table[] =
{
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
    2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
    3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3,
    4, 4, 4, 4, 4, 4, 4, 4, 5, 5, 5, 5, 6, 6, 1, 1
};

#define UTFLEN(x)  utf8_look_for_table[(x)]

//计算str字符数目
RSInline RSIndex __RSUTF8StepMove(RSUBuffer str)
{
    return UTFLEN(*str);
}

RSPrivate RSIndex __RSGetUTF8Length(RSCUBuffer str)
{
    RSIndex clen = strlen((char *)str);
    RSIndex len = 0;
    
    for(const unsigned char *ptr = str;
        *ptr!=0&&len<clen;
        len++, ptr+=UTFLEN((unsigned char)*ptr));
    
    return len;
}

//get子串
static char* subUtfString(const unsigned char *str, RSIndex start, RSIndex end)
{
    RSIndex len = __RSGetUTF8Length(str);
    
    if(start >= len) return nil;
    if(end > len) end = len;
    
    const unsigned char *sptr = str;
    for(RSIndex i = 0; i < start; ++i,sptr+=UTFLEN((unsigned char)*sptr));
    
    const unsigned char *eptr = sptr;
    for(RSIndex i = start; i < end; ++i,eptr += UTFLEN((unsigned char)*eptr));
    
    size_t retLen = eptr - sptr;
    char *retStr = (char*)RSAllocatorAllocate(RSAllocatorSystemDefault, retLen+1);
    memcpy(retStr, sptr, retLen);
    retStr[retLen] = 0;
    
    return retStr;
}

void __RSBufferMakeUpper(RSBuffer buffer, RSIndex length)
{
    RSIndex idx = 0;
    while (idx < length)
    {
        if (buffer[idx] <= 'z' && buffer[idx] >= 'a')
            buffer[idx] &= ~0x20;
        idx++;
    }
}

void __RSBufferMakeLower(RSBuffer buffer, RSIndex length)
{
    RSIndex idx = 0;
    while (idx < length)
    {
        if (buffer[idx] <= 'Z' && buffer[idx] >= 'A')
            buffer[idx] |= 0x20;
        idx++;
    }
}

void __RSBufferMakeCapital(RSBuffer buffer, RSIndex length)
{
    RSIndex idx = 0;
    unsigned flag = !(buffer[idx] & 0x20);
    char c = 0;
    const unsigned signal = 0x1;
    while (idx < length)
    {
        c = buffer[idx];
        if ((flag & signal) && (buffer[idx] <= 'z' && buffer[idx] >= 'a'))
        {
            buffer[idx] &= ~0x20;
            flag &= ~signal;
        }
        else if (!isalpha(buffer[idx]))
        {
            flag |= signal;
        }
        else if (!(buffer[idx] & 0x20)) {
            flag &= ~signal;
        }
        idx++;
    }
}

void __RSBufferReverse(RSBuffer buffer, RSIndex start, RSIndex end)
{
    RSIndex i ,j,temp;
    for(i = start,j = end; i < j; i++,j--)
    {
        temp = buffer[i];
        buffer[i] = buffer[j];
        buffer[j] = temp;
    }
}

void ___RSBufferReverse(RSBuffer buffer, RSIndex start, RSIndex end)
{
    RSUBuffer ps = (RSUBuffer)buffer, pe = nil;
    for (RSIndex i = 0; i < start; ++i) ps += UTFLEN(*ps);
    pe = ps;
    for (RSIndex i = start; i < end; ++i) pe += UTFLEN((unsigned char)*pe);
    RSUBlock cache[7] = {0};
    RSIndex len = 0;

    RSIndex i = 0, j = 0;
    for (i = start, j = end; i < j; i++, j--) {
        len = __RSUTF8StepMove(ps);
        __builtin_memcpy(cache, ps, len);
        ps += len;
        
    }
}


void __RSBufferTrunLeft(RSBuffer buffer,RSIndex sizeToMove,RSIndex length)
{
    RSIndex left = sizeToMove % length;
    if (left == 0) left = length;
    if(left == 0)
        return ;
    __RSBufferReverse(buffer,0,left-1);
    __RSBufferReverse(buffer,left,length-1);
    __RSBufferReverse(buffer,0,length-1);
    return ;
}

void __RSBufferTrunRight(RSBuffer buffer,RSIndex sizeToMove,RSIndex length)
{
    __RSBufferTrunLeft(buffer, length - sizeToMove, length);
    return;
}

BOOL __RSBufferDeleteSubBuffer(RSBuffer str,RSCBuffer substr )
{
    RSBuffer s = nil;
    RSCBuffer t;
    BOOL status = NO;
    RSIndex num = 0;
    
    for ( s = str, t = substr; *s != '\0'; ) {
        if ( *s != *t ) {
            t = substr;
            status = NO;
        }
        if ( *s == *t ) {
            status = YES;
            s++, t++;
        }
        if ( status == NO ) {                  /* 一直不等才s++    */
            s++;
        }
        if ( *t == '\0' && status == YES ) {     /* 发现子串即删除   */
            num++;                                /* 发现子串次数计数 */
            do {
                *( s - ( t - substr) ) = *s;
            } while ( *s++ != '\0' );
            s = str, t = substr;       /*  复位 */
        }
    }
    
    if ( num == 0 )
        return NO;
    else
        return YES;
}

BOOL __RSBufferDeleteRange(RSBuffer str, RSIndex length,  RSRange range)
{
    if (range.length == 0) return YES;
    RSIndex idx = 0;
    length -= range.location + range.length;
    if (length < 0) return NO;
    for (idx = 0; idx < length; idx++) {
        str[range.location + idx] = str[range.location + range.length + idx];
        //__RSCLog(RSLogLevelDebug, "%s\n",str);
    }
    memset(str + range.location + length + 0, '\0', range.length);
    return YES;
}

static void __RSStringMoveBufferUnit(RSBuffer buffer, RSIndex n, BOOL left)
{
    if (left) {
        __RSBufferDeleteRange(buffer,  strlen(buffer), RSMakeRange(0, n));
    }
    else {
        __RSBufferTrunRight(buffer, n, strlen(buffer));
    }
}
BOOL __RSBufferMove(RSBuffer aMutableString, RSIndex length, RSRange rangeFrom, RSRange rangeTo)
{
//    if(rangeFrom.location + rangeFrom.length >= length) return NO;
//    if(rangeTo.location + rangeTo.length >= length) return NO;
//    if(rangeTo.length != rangeFrom.length) return NO;
//    
//    RSIndex idx = 0;
//    
//    if(rangeTo.location > rangeFrom.location)
//    {
//        for(idx = rangeTo.length; idx >= 0; --idx)
//        {
//            aMutableString[idx + rangeTo.location] = aMutableString[idx + rangeFrom.location];
//        }
//    }
//    else
//    {
//        for(idx = 0; idx < rangeTo.length; ++idx)
//        {
//            aMutableString[idx + rangeTo.location] = aMutableString[idx + rangeFrom.location];
//        }
//    }
    
    return YES;
}

// KMP compare function
static void __KMPGetNextValue(RSCBuffer ptrn, RSUInteger plen, RSIndex* nextval)
{
    RSIndex i = 0;
    nextval[i] = -1;
    RSIndex j = -1;
    while( i < plen-1 )
    {
        if( j == -1 || ptrn[i] == ptrn[j] )   //循环的if部分
        {
            ++i;
            ++j;
            //修正的地方就发生下面这4行
            if( ptrn[i] != ptrn[j] ) //++i，++j之后，再次判断ptrn[i]与ptrn[j]的关系
                nextval[i] = j;      //之前的错误解法就在于整个判断只有这一句。
            else
                nextval[i] = nextval[j];
        }
        else                                 //循环的else部分
            j = nextval[j];
    }
}

static RSIndex __KMPSearch(RSCBuffer src, RSIndex slen, RSCBuffer patn, RSIndex plen, RSIndex* nextval, RSUInteger pos)
{
    RSIndex i = pos;
    RSIndex j = 0;
    while ( i < slen && j < plen )
    {
        if( j == -1 || src[i] == patn[j] )
        {
            ++i;
            ++j;
        }
        else
        {
            j = nextval[j];
            //当匹配失败的时候直接用p[j_next]与s[i]比较，
            //下面阐述怎么求这个值，即匹配失效后下一次匹配的位置
        }
    }
    if( j >= plen )
        return i-plen;
    else
        return RSNotFound;
}
RSIndex __RSBufferFindSubString(RSCBuffer str1, RSCBuffer str2)
{
    if (str1 == nil || str2 == nil) return -1;
    RSIndex* next = nil;
    RSIndex length = strlen(str2);
    
    next = RSAllocatorAllocate(RSAllocatorSystemDefault, (length+1)*sizeof(RSIndex));
    __KMPGetNextValue(str2, length, next);
    RSIndex result = __KMPSearch(str1, strlen(str1), str2, length, next, 0);
    RSAllocatorDeallocate(RSAllocatorSystemDefault, next);
    return result;
}

RSInline RSRange *__RSRangesResize(RSAllocatorRef allocator, RSRange *rgs, RSIndex n, RSIndex nn)
{
    if (nn > n)
    {
        rgs = RSAllocatorReallocate(allocator, rgs, nn * sizeof(RSRange));
    }
    return rgs;
}

RSRange *__RSBufferFindSubStrings(RSCBuffer cStr, RSCBuffer strToFind, RSIndex *numberOfRanges)
{
    if (cStr == nil || strToFind == nil || numberOfRanges == nil) return nil;
    RSIndex *next = nil;
    RSIndex length = strlen(strToFind);
    RSIndex strLength = strlen(cStr);
    
    next = RSAllocatorAllocate(RSAllocatorSystemDefault, (length + 1) * sizeof(RSIndex));
    __KMPGetNextValue(strToFind, length, next);
    RSRange *rgs = nil;
    RSIndex cnt = 0, pos = 0;
    
    RSIndex findResult = __KMPSearch(cStr, strLength, strToFind, length, next, 0);
    while (findResult != RSNotFound)
    {
        ++cnt;
        if (rgs == nil) rgs = RSAllocatorAllocate(RSAllocatorDefault, sizeof(RSRange) * cnt);
        else rgs = __RSRangesResize(RSAllocatorDefault, rgs, cnt, cnt + 1);
        rgs[cnt - 1].location = findResult;
        rgs[cnt - 1].length = length;
        pos = rgs[cnt - 1].location + rgs[cnt - 1].length;
        findResult = __KMPSearch(cStr, strLength, strToFind, length, next, pos);
    }
    RSAllocatorDeallocate(RSAllocatorSystemDefault, next);
    
    *numberOfRanges = cnt;
    return rgs;
}

struct ___RSBPContext
{
    RSBuffer* result;
    RSIndex cnt;
    RSIndex limit;
    RSUInteger size;
    int dumpAddrFd;
};

RSInline BOOL ___RSBPDump(struct ___RSBPContext* context)
{
    if (context->cnt > context->limit && context->dumpAddrFd != -1)
    {
        write(context->dumpAddrFd, *context->result, context->size);
        
        memset(*context->result, 0, context->size);
        context->size = 0;
        context->cnt = 0;
        return YES;
    }
    return NO;
}

static void ___RSBufferPermutation(RSBuffer* result, RSUInteger* used, RSUInteger* capacity, RSBuffer pStr, RSBuffer pBegin, struct ___RSBPContext* context)
{
    if(!pStr || !pBegin)
        return;
    BOOL dump = NO;
    if(*pBegin == '\0')
    {
        if (result != nil)
        {
            RSUInteger length = strlen(pStr) + 1 + 1;   // 1 = '\0', 1 = '\n';
            if (*result == nil)
            {
                *result = RSAllocatorAllocate(RSAllocatorSystemDefault, length);
                *capacity = RSAllocatorSize(RSAllocatorSystemDefault, length);
                *used = 0;
                
                if (*used + length < *capacity)
                {
                    memcpy(*result + *used, pStr, length - 1);
                    *used += length - 1;
                    memcpy(*result + *used, "\n", 1);
                    *used += 1;
                    context->size += length;
                    context->cnt ++;
                    
                    dump = ___RSBPDump(context);
                    if (dump == YES)
                    {
                        *used = 0;
                    }
                }
            }
            else
            {
                // the result is used.
            COPY:
                if (*used + length < *capacity)
                {
                    memcpy(*result + *used, pStr, length - 1);
                    *used += length - 1;
                    memcpy(*result + *used, "\n", 1);
                    *used += 1;
                    
                    context->size += length;
                    context->cnt ++;
                    
                    dump = ___RSBPDump(context);
                    if (dump == YES)
                    {
                        *used = 0;
                    }
                }
                else
                {
                    *result = RSAllocatorReallocate(RSAllocatorSystemDefault, *result, *capacity + length*2);
                    *capacity = RSAllocatorSize(RSAllocatorSystemDefault, *capacity + length);
                    goto COPY;
                }
            }
        }
        else
            __RSCLog(RSLogLevelNotice, "%s\n", pStr);   // add to result here.
    }
    else
    {
        for(char* pCh = pBegin; *pCh != '\0'; ++ pCh)
        {
            // swap pCh and pBegin
            char temp = *pCh;
            *pCh = *pBegin;
            *pBegin = temp;
            
            ___RSBufferPermutation(result, used, capacity, pStr, pBegin + 1, context);
            // restore pCh and pBegin
            temp = *pCh;
            *pCh = *pBegin;
            *pBegin = temp;
        }
    }
}
#include <fcntl.h>
void __RSBufferPermutation(RSCBuffer dumpPath, RSBuffer* result, RSBuffer pStr)
{
    RSUInteger used = 0, capacity = 0;
    struct ___RSBPContext context;
    context.result = result;
    context.limit = 1000;
    context.cnt = 0;
    context.size = 0;
    context.dumpAddrFd = open(dumpPath, O_RDWR | O_CREAT, 0777);
    ___RSBufferPermutation(result, &used, &capacity, pStr, pStr, &context);
    if (context.dumpAddrFd) {
        close(context.dumpAddrFd);
    }
}

BOOL __RSBufferIsPalindrome(RSCBuffer s, RSInteger n)
{
    if (s == 0 || n < 1) return NO; // invalid string
    RSBuffer first, second;
    RSInteger m = ((n>>1) - 1) >= 0 ? (n>>1) - 1 : 0; // m is themiddle point of s
    first = (RSBuffer)s + m;
    second = (RSBuffer)s + n - 1 - m;
    while (first >= s)
        if (*first-- != *second++) return NO; // not equal, so it's not apalindrome
    return YES; // check over, it's a palindrome
}

static int __char10ToInt(char c)
{
    int num;
    num = 0;//
    switch (c)
    {
        case '0':
            num = 0;
            break;
        case '1':
            num = 1;
            break;
        case '2':
            num = 2;
            break;
        case '3':
            num = 3;
            break;
        case '4':
            num = 4;
            break;
        case '5':
            num = 5;
            break;
        case '6':
            num = 6;
            break;
        case '7':
            num = 7;
            break;
        case '8':
            num = 8;
            break;
        case '9':
            num = 9;
            break;
        default:
            break;
    }
    return num;
}

static int __char16ToInt(char c)
{
    int num;
    num = 0;//
    switch (c)
    {
        case '0':
            num = 0;
            break;
        case '1':
            num = 1;
            break;
        case '2':
            num = 2;
            break;
        case '3':
            num = 3;
            break;
        case '4':
            num = 4;
            break;
        case '5':
            num = 5;
            break;
        case '6':
            num = 6;
            break;
        case '7':
            num = 7;
            break;
        case '8':
            num = 8;
            break;
        case '9':
            num = 9;
            break;
        case 'a':
        case 'A':
            num = 10;
            break;
        case 'b':
        case 'B':
            num = 11;
            break;
        case 'c':
        case 'C':
            num = 12;
            break;
        case 'd':
        case 'D':
            num = 13;
            break;
        case 'e':
        case 'E':
            num = 14;
            break;
        case 'f':
        case 'F':
            num = 15;
            break;
        default:
            break;
    }
    return num;
}



static BOOL __isValidateStr16(const char *str)
{
    size_t len,i;
    if (nil == str)
    {
        return NO;
    }
    len = strlen(str);
    for (i = 0; i < len; i++)
    {
        if (!((str[i] >= '0' && str[i] <= '9') || (str[i] >= 'A' && str[i] <= 'F')|| (str[i] >= 'a' && str[i] <= 'f')))
            //满足条件之一0~9或者a~z或者A~Z都是合法的十六进制字符
            return NO;
    }
    return YES;
}

RSIndex __RSBufferToNumber16(RSCBuffer str, RSIndex length)
{
    RSIndex len,i,num;
    num = 0;//使用数据必须初始化否则产生不确定值
    len = length;
    for (i = 0; i < len; i++)
    {
        num = num*16 + __char16ToInt(str[i]);/*十六进制字符串与10进制的对应数据*/
    }
    return num;
}

RSIndex __RSBufferToNumber10(RSCBuffer str, RSIndex length)
{
    RSIndex len,i,num;
    num = 0;//使用数据必须初始化否则产生不确定值
    len = length;
    for (i = 0; i < len; i++)
    {
        num = num*10 + __char10ToInt(str[i]);/*十六进制字符串与10进制的对应数据*/
    }
    return num;
}

BOOL __RSBufferContainString_low_hash(RSCBuffer str1, RSCBuffer str2)
{
    char hash[26] = {0};
    while (*str1) {
        hash[*str1-'a'] = 1;
        ++str1;
    }
    while (*str2)
    {
        if (0 == hash[*str2 - 'a']) {
            return NO;
        }
        ++str2;
    }   return YES;
}

BOOL __RSBufferContainString_high_hash(RSCBuffer str1, RSCBuffer str2)
{
    char hash[26] = {0};
    while (*str1) {
        hash[*str1-'A'] = 1;
        ++str1;
    }
    while (*str2)
    {
        if (0 == hash[*str2 - 'A']) {
            return NO;
        }
        ++str2;
    }
    return YES;
}


BOOL __RSBufferContainString_low_bitmap(RSCBuffer str1, RSCBuffer str2)
{
    char hash[4] = {0};
    /*
     4*8 = 32 > 26
     */
    while (*str1) {
        hash[(*str1-'a')/8] |= 1 << (*str1-'a') % 8;
        ++str1;
    }
    while (*str2)
    {
        if (NO == (1 << ((*str2-'a') % 8) & hash[(*str2-'a')/8]))
        {
            return NO;
        }
        ++str2;
    }
    return YES;

}

BOOL __RSBufferContainString_high_bitmap(RSCBuffer str1, RSCBuffer str2)
{
    char hash[4] = {0};
    /*
     4*8 = 32 > 26
     */
    while (*str1) {
        hash[(*str1-'A')/8] |= 1 << (*str1-'A') % 8;
        ++str1;
    }
    while (*str2)
    {
        if (NO == (1 << ((*str2-'A') % 8) & hash[(*str2-'A')/8]))
        {
            return NO;
        }
        ++str2;
    }
    return YES;
}

BOOL __RSBufferMatchString_low(RSCBuffer str1, RSCBuffer str2)
{
    RSIndex l1 = strlen(str1), l2 = strlen(str2);
    if (l1 != l2) return NO;
    int hash[26] = {0};
    for (RSIndex idx = 0; idx < l1; idx++)
    {
        ++hash[*(str1 + idx) - 'a'];
    }
    for (RSIndex idx = 0; idx < l2; idx++)
    {
        if (hash[*(str2 + idx) - 'a'] > 0) {
            --hash[*(str2 + idx)];
        }
        else return NO;
    }
    return YES;
}

BOOL __RSBufferMatchString_high(RSCBuffer str1, RSCBuffer str2)
{
    RSIndex l1 = strlen(str1), l2 = strlen(str2);
    if (l1 != l2) return NO;
    int hash[26] = {0};
    for (RSIndex idx = 0; idx < l1; idx++)
    {
        ++hash[*(str1 + idx) - 'A'];
    }
    for (RSIndex idx = 0; idx < l2; idx++)
    {
        if (hash[*(str2 + idx) - 'A'] > 0) {
            --hash[*(str2 + idx)];
        }
        else return NO;
    }
    return YES;
}
