//
//  RSRegularExpression.h.c
//  RSCoreFoundation
//
//  Created by closure on 12/26/13.
//  Copyright (c) 2013 RetVal. All rights reserved.
//

#include "RSRegularExpression.h"

#include <RSCoreFoundation/RSRuntime.h>
#include "uregex.h"

typedef void * RSRegularExpression;
struct __RSRegularExpression
{
    RSRuntimeBase _base;
    RSStringRef _pattern;
    RSRegularExpression _regex;
};

static void __RSRegularExpressionClassDeallocate(RSTypeRef rs)
{   RSCoreFoundationCrash("");
    struct __RSRegularExpression *expression = (struct __RSRegularExpression *)rs;
    if (expression->_pattern) RSRelease(expression->_pattern);
    if (expression->_regex) uregex_close(expression->_regex);
}

static RSStringRef __RSRegularExpressionClassDescription(RSTypeRef rs)
{
    RSStringRef description = RSStringCreateWithFormat(RSAllocatorDefault, RSSTR("%r"), ((RSRegularExpressionRef)rs)->_pattern);
    return description;
}

static RSRuntimeClass __RSRegularExpressionClass =
{
    _RSRuntimeScannedObject,
    0,
    "RSRegularExpression",
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
    const UChar *ptr = RSStringGetCharactersPtr(expression);
    BOOL needFree = NO;
    if (!ptr) {
        ptr = RSAllocatorAllocate(RSAllocatorSystemDefault, RSStringGetMaximumSizeForEncoding(RSStringGetLength(expression), RSStringEncodingUnicode));
        if (!ptr) {
            return nil;
        }
        UniChar *unichar = (UniChar *)ptr;
        RSStringGetCharacters(expression, RSStringGetRange(expression), unichar);
        needFree = YES;
    }
    
    RSRegularExpression _regex = nil;
    UParseError error = {0};
    UErrorCode errorCode = 0;
    _regex = uregex_open(ptr, -1, 0, &error, &errorCode);
    if (errorCode == U_ZERO_ERROR && _regex != nil) {
        struct __RSRegularExpression* instance = (struct __RSRegularExpression *)__RSRuntimeCreateInstance(allocator, _RSRegularExpressionTypeID, sizeof(struct __RSRegularExpression) - sizeof(RSRuntimeBase));
        instance->_regex = _regex;
        instance->_pattern = RSRetain(expression);
        if (needFree) RSAllocatorDeallocate(RSAllocatorSystemDefault, ptr);
        return instance;
    }
    if (needFree) RSAllocatorDeallocate(RSAllocatorSystemDefault, ptr);
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
    const UChar *ptr = RSStringGetCharactersPtr(str);
    BOOL needFree = NO;
    if (!ptr) {
        ptr = RSAllocatorAllocate(RSAllocatorSystemDefault, RSStringGetMaximumSizeForEncoding(RSStringGetLength(str), RSStringEncodingUnicode));
        if (!ptr) {
            return nil;
        }
        UniChar *unichar = (UniChar *)ptr;
        RSStringGetCharacters(str, RSStringGetRange(str), unichar);
        needFree = YES;
    }
    RSMutableArrayRef results = nil;
    UErrorCode errorCode = U_ZERO_ERROR;
    uregex_setText(regexExpression->_regex, ptr, -1, &errorCode);
    RSIndex offset = 0;
    BOOL find = uregex_find64(regexExpression->_regex, offset, &errorCode);
    if (find) {
        results = RSArrayCreateMutable(RSAllocatorSystemDefault, 0);
    }
    while (find) {
        RSRange range = RSMakeRange(uregex_start64(regexExpression->_regex, 0, &errorCode), uregex_end64(regexExpression->_regex, 0, &errorCode));
        range.length = range.length - range.location;
        RSArrayAddObject(results, RSNumberWithRange(range));
        find = uregex_findNext(regexExpression->_regex, &errorCode);
    }
    if (needFree) {
        RSAllocatorDeallocate(RSAllocatorSystemDefault, ptr);
        ptr = nil;
    }
    return RSAutorelease(results);
}
