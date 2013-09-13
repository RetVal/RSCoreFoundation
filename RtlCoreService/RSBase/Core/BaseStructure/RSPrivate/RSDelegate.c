//
//  RSDelegate.c
//  RSCoreFoundation
//
//  Created by RetVal on 6/26/13.
//  Copyright (c) 2013 RetVal. All rights reserved.
//

#include "RSDelegate.h"

#include <RSCoreFoundation/RSRuntime.h>

struct __RSDelegate
{
    RSRuntimeBase _base;
    RSStringRef _name;
    RSMutableDictionaryRef _protocols;
};

static RSTypeRef __RSDelegateClassCopy(RSAllocatorRef allocator, RSTypeRef rs, BOOL mutableCopy)
{
    RSDelegateRef delegate = (RSDelegateRef)rs;
    
    return RSRetain(delegate);
}

static void __RSDelegateClassDeallocate(RSTypeRef rs)
{
    RSDelegateRef delegate = (RSDelegateRef)rs;
    if (delegate->_name) RSRelease(delegate->_name);
    if (delegate->_protocols) RSRelease(delegate->_protocols);
    
}

static BOOL __RSDelegateClassEqual(RSTypeRef rs1, RSTypeRef rs2)
{
    RSDelegateRef RSDelegate1 = (RSDelegateRef)rs1;
    RSDelegateRef RSDelegate2 = (RSDelegateRef)rs2;
    BOOL result = NO;
    
    result = RSEqual(RSDelegate1->_protocols, RSDelegate2->_protocols) && RSEqual(RSDelegate1->_name, RSDelegate2->_name);
    
    return result;
}

static RSStringRef __RSDelegateClassDescription(RSTypeRef rs)
{
    RSDelegateRef delegate = (RSDelegateRef)rs;
    RSStringRef description = RSStringCreateWithFormat(RSAllocatorSystemDefault, RSSTR("RSDelegate<%r>"), delegate->_name);
    return description;
}

static RSRuntimeClass __RSDelegateClass =
{
    _RSRuntimeScannedObject,
    "RSDelegate",
    nil,
    __RSDelegateClassCopy,
    __RSDelegateClassDeallocate,
    __RSDelegateClassEqual,
    nil,
    __RSDelegateClassDescription,
    nil,
    nil
};

static RSTypeID _RSDelegateTypeID = _RSRuntimeNotATypeID;

RSExport RSTypeID RSDelegateGetTypeID()
{
    return _RSDelegateTypeID;
}

RSPrivate void __RSDelegateInitialize()
{
    _RSDelegateTypeID = __RSRuntimeRegisterClass(&__RSDelegateClass);
    __RSRuntimeSetClassTypeID(&__RSDelegateClass, _RSDelegateTypeID);
}

static RSDelegateRef __RSDelegateCreateInstance(RSAllocatorRef allocator, RSStringRef name, RSArrayRef protocols)
{
    RSDelegateRef instance = (RSDelegateRef)__RSRuntimeCreateInstance(allocator, _RSDelegateTypeID, sizeof(struct __RSDelegate) - sizeof(RSRuntimeBase));
    
    instance->_name = RSRetain(name);
    RSMutableDictionaryRef dict = RSDictionaryCreateMutable(allocator, 0, RSDictionaryRSTypeContext);
    RSUInteger cnt = RSArrayGetCount(protocols);
    RSProtocolRef protocol = nil;
    for (RSUInteger idx = 0; idx < cnt; ++idx) {
        protocol = (RSProtocolRef)RSArrayObjectAtIndex(protocols, idx);
        RSDictionarySetValue(dict, RSProtocolGetName(protocol), protocol);
    }
    instance->_protocols = dict;
    return instance;
}

RSExport RSDelegateRef RSDelegateCreateWithProtocols(RSAllocatorRef allocator, RSStringRef delegateName, RSArrayRef protocols)
{
    if (!delegateName || !protocols) return nil;
    if (RSStringGetLength(delegateName) == 0) return nil;
    return __RSDelegateCreateInstance(allocator, delegateName, protocols);
}

RSExport RSStringRef RSDelegateGetName(RSDelegateRef delegate)
{
    __RSGenericValidInstance(delegate, _RSDelegateTypeID);
    return delegate->_name;
}

static RSProtocolRef __RSProtocolLookUpWithName(restrict RSProtocolRef entry, restrict RSStringRef name)
{
    if (RSEqual(RSProtocolGetName(entry), name)) return entry;
    RSArrayRef supers = RSProtocolGetSupers(entry);
    if (supers)
    {
        RSUInteger cnt = RSArrayGetCount(supers);
        for (RSUInteger idx = 0; idx < cnt; ++idx) {
            entry = (RSProtocolRef)RSArrayObjectAtIndex(supers, idx);
            entry = __RSProtocolLookUpWithName(entry, name);
            if (entry) return entry;
        }
    }
    return nil;
}

static RSProtocolRef __RSDelegateLookupProtocolWithName(restrict RSDelegateRef delegate, restrict RSStringRef name)
{
    restrict RSProtocolRef protocol = (ISA)RSDictionaryGetValue(delegate->_protocols, name);
    if (protocol) return protocol;
    RSArrayRef allProtocols = RSDictionaryAllValues(delegate->_protocols);
    
    RSUInteger idx = 0, cnt = RSArrayGetCount(allProtocols);
    while (idx < cnt) {
        protocol = (RSProtocolRef)RSArrayObjectAtIndex(allProtocols, idx++);
        protocol = __RSProtocolLookUpWithName(protocol, name);
        if (protocol) break;
    }
    
    RSRelease(allProtocols);
    return protocol;
}


RSExport RSProtocolRef RSDelegateGetProtocolWithName(RSDelegateRef delegate, RSStringRef name)
{
    __RSGenericValidInstance(delegate, _RSDelegateTypeID);
    RSProtocolRef protocol = nil;
    RSArrayRef protocols = nil;
    if (delegate->_protocols)
    {
        protocol = (RSProtocolRef)RSDictionaryGetValue(delegate->_protocols, name);
        if (protocol) return protocol;
        if (!RSProtocolIsRoot(protocol))
        {
            protocols = RSProtocolGetSupers(protocol);
            RSUInteger cnt = RSArrayGetCount(protocols);
            for (RSUInteger idx = 0; idx < cnt; ++idx) {
                protocol = (RSProtocolRef)RSArrayObjectAtIndex(protocols, idx);
                if (protocol && RSEqual(RSProtocolGetName(protocol), name)) return protocol;
            }
        }
    }
    return nil;
}

RSExport BOOL RSDelegateConfirmProtocol(RSDelegateRef delegate, RSStringRef protocol)
{
    return (nil != RSDelegateGetProtocolWithName(delegate, protocol));
}