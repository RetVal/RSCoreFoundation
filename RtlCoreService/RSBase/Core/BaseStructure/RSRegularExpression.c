//
//  RSRegularExpression.h.c
//  RSCoreFoundation
//
//  Created by closure on 12/26/13.
//  Copyright (c) 2013 RetVal. All rights reserved.
//

#include "RSRegularExpression.h"

#include <RSCoreFoundation/RSRuntime.h>
#include <regex.h>

struct __RSRegularExpression
{
    RSRuntimeBase _base;
    regex_t _regex;
    
};

static void __RSRegularExpressionClassDeallocate(RSTypeRef rs)
{
    struct __RSRegularExpression *expression = (struct __RSRegularExpression *)rs;
    regfree(&expression->_regex);
}

static RSStringRef __RSRegularExpressionClassDescription(RSTypeRef rs)
{
    RSStringRef description = RSStringCreateWithFormat(RSAllocatorDefault, RSSTR("RSRegularExpression.h %p"), rs);
    return description;
}

static RSRuntimeClass __RSRegularExpressionClass =
{
    _RSRuntimeScannedObject,
    "RSRegularExpression.h",
    nil,
    nil,
    __RSRegularExpressionClassDeallocate,
    nil,
    nil,
    __RSRegularExpressionClassDescription,
    nil,
    nil
};

static RSTypeID _RSRegularExpressionTypeID = _RSRuntimeNotATypeID;

RSExport RSTypeID RSRegularExpressionGetTypeID() {
    return _RSRegularExpressionTypeID;
}

static void __RSRegularExpressionInitialize() {
    if (_RSRuntimeNotATypeID != _RSRegularExpressionTypeID)
        return;
    _RSRegularExpressionTypeID = __RSRuntimeRegisterClass(&__RSRegularExpressionClass);
    __RSRuntimeSetClassTypeID(&__RSRegularExpressionClass, _RSRegularExpressionTypeID);
}

static RSRegularExpressionRef __RSRegularExpressionCreateInstance(RSAllocatorRef allocator, RSStringRef expression, int flag) {
    __RSRegularExpressionInitialize();
    const char *ptr = RSStringCopyUTF8String(expression);
    if (!ptr) return nil;
    regex_t _regex;
    if (0 == regcomp(&_regex, ptr, REG_EXTENDED)) {
        struct __RSRegularExpression * instance = (struct __RSRegularExpression *)__RSRuntimeCreateInstance(allocator, _RSRegularExpressionTypeID, sizeof(struct __RSRegularExpression) - sizeof(RSRuntimeBase));
        __builtin_memcpy(&instance->_regex, &_regex, sizeof(regex_t));
        RSAllocatorDeallocate(RSAllocatorSystemDefault, ptr);
        return instance;
    }
    RSAllocatorDeallocate(RSAllocatorSystemDefault, ptr);
    return nil;
}

RSExport RSRegularExpressionRef RSRegularExpressionCreate(RSAllocatorRef allocator, RSStringRef expression) {
    if (!expression) return nil;
    return __RSRegularExpressionCreateInstance(allocator, expression, 0);
}

RSExport RSArrayRef RSRegularExpressionApplyOnString(RSRegularExpressionRef regexExpression, RSStringRef str, __autorelease RSErrorRef *error) {
    if (!regexExpression || !str) return nil;
    __RSGenericValidInstance(regexExpression, _RSRegularExpressionTypeID);
    if (!str) return nil;
    const char *utf8 = nil;
    RSIndex usedLength = 0, maxSize = RSStringGetMaximumSizeForEncoding(RSStringGetLength(str), RSStringEncodingUTF8);
    if (maxSize) {
        if ((utf8 = RSAllocatorAllocate(RSAllocatorSystemDefault, ++maxSize))) {
            usedLength = RSStringGetCString(str, (char *)utf8, maxSize, RSStringEncodingUTF8) + 1; // add end flag
            char *tmp = RSAllocatorAllocate(RSAllocatorSystemDefault, usedLength);
            if (tmp) {
                __builtin_memcpy(tmp, utf8, usedLength);
                RSAllocatorDeallocate(RSAllocatorSystemDefault, utf8);
                utf8 = tmp;
            }
        }
    }
    if (!utf8) return nil;
    RSMutableArrayRef results = nil;
    regmatch_t match = {0};
    size_t nmatch = 1;
    int result = regnexec(&regexExpression->_regex, utf8, usedLength, nmatch, &match, 1);
    if (REG_NOMATCH == result) {
        __RSCLog(RSLogLevelNotice, "regex expression no match!");
    } else if (0 == result) {
        for (RSUInteger offset = match.rm_so; result == 0 && offset < usedLength && match.rm_so != -1; result = regnexec(&regexExpression->_regex, utf8 + offset, usedLength - offset, nmatch, &match, 1)) {
            RSShow(RSStringWithSubstring(str, RSMakeRange(offset, match.rm_eo - match.rm_so)));
            offset += match.rm_eo - match.rm_so;
        }
    } else if (!result && error) {
    }
    RSAllocatorDeallocate(RSAllocatorSystemDefault, utf8);
    return results;
}
