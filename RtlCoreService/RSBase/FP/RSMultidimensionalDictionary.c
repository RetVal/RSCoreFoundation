//
//  RSMultidimensionalDictionary.c
//  RSCoreFoundation
//
//  Created by closure on 2/2/14.
//  Copyright (c) 2014 RetVal. All rights reserved.
//

#include "RSMultidimensionalDictionary.h"

#include <RSCoreFoundation/RSRuntime.h>
RS_CONST_STRING_DECL(__RSMultidimensionalDictionayKeyPathSeparatingStrings, ".");

struct __RSMultidimensionalDictionary
{
    RSRuntimeBase _base;
    RSUInteger _dimensions;
    RSMutableDictionaryRef _dict;
};

static void __RSMultidimensionalDictionaryClassInit(RSTypeRef rs)
{
    RSMultidimensionalDictionaryRef md = (RSMultidimensionalDictionaryRef)rs;
    md->_dict = RSDictionaryCreateMutable(RSAllocatorSystemDefault, 0, RSDictionaryRSTypeContext);
}

static RSTypeID _RSMultidimensionalDictionaryTypeID = _RSRuntimeNotATypeID;

static RSTypeRef __RSMultidimensionalDictionaryClassCopy(RSAllocatorRef allocator, RSTypeRef rs, BOOL mutableCopy)
{
//    if (!mutableCopy) return RSRetain(rs);
    RSMultidimensionalDictionaryRef md = (RSMultidimensionalDictionaryRef)rs;
    RSMultidimensionalDictionaryRef copy = (RSMultidimensionalDictionaryRef)__RSRuntimeCreateInstance(allocator, _RSMultidimensionalDictionaryTypeID, sizeof(struct __RSMultidimensionalDictionary) -  sizeof(struct __RSRuntimeBase));
    copy->_dimensions = md->_dimensions;
    copy->_dict = RSMutableCopy(allocator, md->_dict);
    return copy;
}

static void __RSMultidimensionalDictionaryClassDeallocate(RSTypeRef rs)
{
    RSMultidimensionalDictionaryRef md = (RSMultidimensionalDictionaryRef)rs;
    RSRelease(md->_dict);
}

static BOOL __RSMultidimensionalDictionaryClassEqual(RSTypeRef rs1, RSTypeRef rs2)
{
    RSMultidimensionalDictionaryRef RSMultidimensionalDictionary1 = (RSMultidimensionalDictionaryRef)rs1;
    RSMultidimensionalDictionaryRef RSMultidimensionalDictionary2 = (RSMultidimensionalDictionaryRef)rs2;
    BOOL result = NO;
    
    result = RSMultidimensionalDictionary1->_dimensions == RSMultidimensionalDictionary2->_dimensions && RSEqual(RSMultidimensionalDictionary1->_dict, RSMultidimensionalDictionary2->_dict);
    
    return result;
}

static RSStringRef __RSMultidimensionalDictionaryClassDescription(RSTypeRef rs)
{
    RSMultidimensionalDictionaryRef dict = (RSMultidimensionalDictionaryRef)rs;
    RSStringRef description = RSStringCreateWithFormat(RSAllocatorDefault, RSSTR("RSMultidimensionalDictionary %p <%d Dimension>\n%r"), dict, dict->_dimensions, dict->_dict);
    return description;
}

static RSRuntimeClass __RSMultidimensionalDictionaryClass =
{
    _RSRuntimeScannedObject,
    0,
    "RSMultidimensionalDictionary",
    __RSMultidimensionalDictionaryClassInit,
    __RSMultidimensionalDictionaryClassCopy,
    __RSMultidimensionalDictionaryClassDeallocate,
    __RSMultidimensionalDictionaryClassEqual,
    nil,
    __RSMultidimensionalDictionaryClassDescription,
    nil,
    nil
};

static void __RSMultidimensionalDictionaryInitialize();

RSExport RSTypeID RSMultidimensionalDictionaryGetTypeID()
{
    static dispatch_once_t onceToken;
    dispatch_once(&onceToken, ^{
        __RSMultidimensionalDictionaryInitialize();
    });
    return _RSMultidimensionalDictionaryTypeID;
}

static void __RSMultidimensionalDictionaryInitialize()
{
    _RSMultidimensionalDictionaryTypeID = __RSRuntimeRegisterClass(&__RSMultidimensionalDictionaryClass);
    __RSRuntimeSetClassTypeID(&__RSMultidimensionalDictionaryClass, _RSMultidimensionalDictionaryTypeID);
}

static RSMultidimensionalDictionaryRef __RSMultidimensionalDictionaryCreateInstance(RSAllocatorRef allocator, RSUInteger dimensions, RSDictionaryRef dict)
{
    RSMultidimensionalDictionaryRef instance = (RSMultidimensionalDictionaryRef)__RSRuntimeCreateInstance(allocator, RSMultidimensionalDictionaryGetTypeID(), sizeof(struct __RSMultidimensionalDictionary) - sizeof(RSRuntimeBase));
    instance->_dimensions = dimensions;
    RSDictionaryApplyBlock(dict, ^(const void *key, const void *value, BOOL *stop) {
        RSDictionarySetValue(instance->_dict, key, value);
    });
    return instance;
}

RSExport RSMultidimensionalDictionaryRef RSMultidimensionalDictionaryCreate(RSAllocatorRef allocator, RSUInteger dimension) {
    return __RSMultidimensionalDictionaryCreateInstance(allocator, dimension, nil);
}

RSExport RSUInteger RSMultidimensionalDictionaryGetDimension(RSMultidimensionalDictionaryRef dict) {
    if (!dict) return 0;
    __RSGenericValidInstance(dict, _RSMultidimensionalDictionaryTypeID);
    return dict->_dimensions;
}

RSExport RSDictionaryRef RSMultidimensionalDictionaryGetDimensionEntry(RSMultidimensionalDictionaryRef dict) {
    if (!dict) return nil;
    __RSGenericValidInstance(dict, _RSMultidimensionalDictionaryTypeID);
    return dict->_dict;
}

static RSTypeRef __RSMultidimensionalDictionaryGetValueWithKeys(RSMultidimensionalDictionaryRef dict, RSArrayRef keys) {
    RSTypeRef key = nil;
    if (dict->_dimensions < RSArrayGetCount(keys)) return nil; // keys is not invalid
    RSUInteger dimensions = min(dict->_dimensions, RSArrayGetCount(keys));
    RSMutableDictionaryRef currentDimension = dict->_dict;
    for (RSUInteger idx = 0; idx < dimensions - 1; idx++) {
        key = RSArrayObjectAtIndex(keys, idx);
        RSMutableDictionaryRef dimension = (RSMutableDictionaryRef)RSDictionaryGetValue(currentDimension, key);
        if (!dimension) return nil;
        currentDimension = dimension;
    }
    key = RSArrayLastObject(keys);
    return RSDictionaryGetValue(currentDimension, key);
}

static RSTypeRef __RSMultidimensionalDictionaryGetValue(RSMultidimensionalDictionaryRef dict, va_list arguments) {
    RSArrayRef keys = __RSArrayCreateWithArguments(arguments, dict->_dimensions);
    if (!keys || !RSArrayGetCount(keys)) {
        RSRelease(keys);
        return nil;
    }
    RSTypeRef value = __RSMultidimensionalDictionaryGetValueWithKeys(dict, keys);
    RSRelease(keys);
    return value;
}

RSExport RSTypeRef RSMultidimensionalDictionaryGetValue(RSMultidimensionalDictionaryRef dict, ...) {
    if (!dict) return nil;
    __RSGenericValidInstance(dict, _RSMultidimensionalDictionaryTypeID);
    va_list ap;
    va_start(ap, dict);
    RSTypeRef value = __RSMultidimensionalDictionaryGetValue(dict, ap);
    va_end(ap);
    return value;
}

RSExport RSTypeRef RSMultidimensionalDictionaryGetValueWithKeyPaths(RSMultidimensionalDictionaryRef dict, RSStringRef keyPath) {
    if (!dict) return nil;
    __RSGenericValidInstance(dict, _RSMultidimensionalDictionaryTypeID);
    RSArrayRef keys = RSStringCreateComponentsSeparatedByStrings(RSAllocatorSystemDefault, keyPath, __RSMultidimensionalDictionayKeyPathSeparatingStrings);
    RSTypeRef value = __RSMultidimensionalDictionaryGetValueWithKeys(dict, keys);
    RSRelease(keys);
    return value;
}

RSExport RSTypeRef RSMultidimensionalDictionaryGetValueWithKeys(RSMultidimensionalDictionaryRef dict, RSArrayRef keys) {
    if (!dict) return nil;
    __RSGenericValidInstance(dict, _RSMultidimensionalDictionaryTypeID);
    RSTypeRef value = __RSMultidimensionalDictionaryGetValueWithKeys(dict, keys);
    return value;
}

static void __RSMultidimensionalDictionarySetValueWithKeys(RSMultidimensionalDictionaryRef dict, RSTypeRef value, RSArrayRef keys) {
    RSTypeRef key = RSArrayObjectAtIndex(keys, 0);
    RSMutableDictionaryRef currentDimension = dict->_dict;
    RSUInteger idx = 1;
    
    for (; key && idx < dict->_dimensions; idx++) {
        RSMutableDictionaryRef dimension = (RSMutableDictionaryRef)RSDictionaryGetValue(currentDimension, key);
        if (!dimension) {
            dimension = RSDictionaryCreateMutable(RSAllocatorSystemDefault, 0, RSDictionaryRSTypeContext);
            RSDictionarySetValue(currentDimension, key, dimension);
            RSRelease(dimension);
        }
        currentDimension = dimension;
        key = RSArrayObjectAtIndex(keys, idx);
    }
    if (idx == dict->_dimensions)
        RSDictionarySetValue(currentDimension, key, value);
 }

static void __RSMultidimensionalDictionarySetValue(RSMultidimensionalDictionaryRef dict, RSTypeRef value, va_list arguments) {
    RSArrayRef keys = __RSArrayCreateWithArguments(arguments, dict->_dimensions);
    __RSMultidimensionalDictionarySetValueWithKeys(dict, value, keys);
    RSRelease(keys);
}

RSExport void RSMultidimensionalDictionarySetValue(RSMultidimensionalDictionaryRef dict, RSTypeRef value, ...) {
    if (!dict) return;
    __RSGenericValidInstance(dict, _RSMultidimensionalDictionaryTypeID);
    va_list ap;
    va_start(ap, value);
    __RSMultidimensionalDictionarySetValue(dict, value, ap);
    va_end(ap);
}

RSExport void RSMultidimensionalDictionarySetValueWithKeyPaths(RSMultidimensionalDictionaryRef dict, RSTypeRef value, RSStringRef keyPath) {
    if (!dict) return;
    __RSGenericValidInstance(dict, _RSMultidimensionalDictionaryTypeID);
    RSArrayRef keys = RSStringCreateComponentsSeparatedByStrings(RSAllocatorSystemDefault, keyPath, __RSMultidimensionalDictionayKeyPathSeparatingStrings);
    __RSMultidimensionalDictionarySetValueWithKeys(dict, value, keys);
    RSRelease(keys);
}

RSExport void RSMultidimensionalDictionarySetValueWithKeys(RSMultidimensionalDictionaryRef dict, RSTypeRef value, RSArrayRef keys) {
    if (!dict) return;
    __RSGenericValidInstance(dict, _RSMultidimensionalDictionaryTypeID);
    __RSMultidimensionalDictionarySetValueWithKeys(dict, value, keys);
}

