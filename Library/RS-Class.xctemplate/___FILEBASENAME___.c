//
//  ___FILENAME___
//  ___PROJECTNAME___
//
//  Created by ___FULLUSERNAME___ on ___DATE___.
//___COPYRIGHT___
//

#include "___FILEBASENAME___.h"

#include <RSCoreFoundation/RSRuntime.h>

struct _____FILEBASENAME___ {
    RSRuntimeBase _base;
    <#Object properties#>;
};

static void _____FILEBASENAME___ClassInit(RSTypeRef rs) {
    <#do your init operation#>
}

static RSTypeRef _____FILEBASENAME___ClassCopy(RSAllocatorRef allocator, RSTypeRef rs, BOOL mutableCopy) {
    return RSRetain(rs);
}

static void _____FILEBASENAME___ClassDeallocate(RSTypeRef rs) {
    <#do your dealloc operation#>
}

static BOOL _____FILEBASENAME___ClassEqual(RSTypeRef rs1, RSTypeRef rs2) {
    ___FILEBASENAME___Ref ___FILEBASENAME___1 = (___FILEBASENAME___Ref)rs1;
    ___FILEBASENAME___Ref ___FILEBASENAME___2 = (___FILEBASENAME___Ref)rs2;
    BOOL result = NO;
    
    result = <#do your compare operation#>
    
    return result;
}

static RSHashCode _____FILEBASENAME___ClassHash(RSTypeRef rs) {
    <#do your hash operation#>
}

static RSStringRef _____FILEBASENAME___ClassDescription(RSTypeRef rs) {
    RSStringRef description = RSStringCreateWithFormat(RSAllocatorDefault, RSSTR("___FILEBASENAME___ %p"), rs);
    return description;
}

static RSRuntimeClass _____FILEBASENAME___Class = {
    _RSRuntimeScannedObject,
    0,
    "___FILEBASENAME___",
    _____FILEBASENAME___ClassInit,
    _____FILEBASENAME___ClassCopy,
    _____FILEBASENAME___ClassDeallocate,
    _____FILEBASENAME___ClassEqual,
    _____FILEBASENAME___ClassHash,
    _____FILEBASENAME___ClassDescription,
    nil,
    nil
};

static RSTypeID ____FILEBASENAME___TypeID = _RSRuntimeNotATypeID;
static void __RSMultidimensionalDictionaryInitialize();

RSExport RSTypeID RSMultidimensionalDictionaryGetTypeID()
{
    static dispatch_once_t onceToken;
    dispatch_once(&onceToken, ^{
        __RSMultidimensionalDictionaryInitialize();
    });
    return _RSMultidimensionalDictionaryTypeID;
}

static void _____FILEBASENAME___Initialize() {
    ____FILEBASENAME___TypeID = __RSRuntimeRegisterClass(&_____FILEBASENAME___Class);
    __RSRuntimeSetClassTypeID(&_____FILEBASENAME___Class, ____FILEBASENAME___TypeID);
}

RSExport RSTypeID ___FILEBASENAME___GetTypeID() {
//    static dispatch_once_t onceToken;
//    dispatch_once(&onceToken, ^{
//        _____FILEBASENAME___Initialize();
//    });
    return ____FILEBASENAME___TypeID;
}

//RSPrivate void _____FILEBASENAME___Deallocate() {
//    <#do your finalize operation#>
//}

static ___FILEBASENAME___Ref _____FILEBASENAME___CreateInstance(RSAllocatorRef allocator, <#arguments...#>) {
    ___FILEBASENAME___Ref instance = (___FILEBASENAME___Ref)__RSRuntimeCreateInstance(allocator, ____FILEBASENAME___TypeID /*___FILEBASENAME___GetTypeID()*/, sizeof(struct _____FILEBASENAME___) - sizeof(RSRuntimeBase));
    
    <#do your other setting for the instance#>
    
    return instance;
}