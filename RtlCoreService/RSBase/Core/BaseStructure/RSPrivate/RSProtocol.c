//
//  RSProtocol.c
//  RSCoreFoundation
//
//  Created by RetVal on 6/27/13.
//  Copyright (c) 2013 RetVal. All rights reserved.
//

#include "RSProtocol.h"
#include <RSCoreFoundation/RSString+Extension.h>
#include <RSCoreFoundation/RSNotificationCenter.h>
#include <RSCoreFoundation/RSRuntime.h>

struct __RSProtocol
{
    RSRuntimeBase _base;
    RSMutableArrayRef _superProtocols;   // strong!
    RSStringRef _name;
    RSMutableDictionaryRef _table;
};

static void __RSProtocolClassInit(RSTypeRef rs)
{
//    RSProtocolRef protocol = (RSProtocolRef)rs;
//    protocol->_table = RSDictionaryCreateMutable(RSAllocatorSystemDefault, 0, RSDictionaryNilValueContext);
}

static RSTypeRef __RSProtocolClassCopy(RSAllocatorRef allocator, RSTypeRef rs, BOOL mutableCopy)
{
    return RSRetain(rs);
}

static void __RSProtocolClassDeallocate(RSTypeRef rs)
{
    RSProtocolRef protocol = (RSProtocolRef)rs;
    if (protocol->_name) RSRelease(protocol->_name);
    if (protocol->_table) RSRelease(protocol->_table);
}

static BOOL __RSProtocolClassEqual(RSTypeRef rs1, RSTypeRef rs2)
{
    RSProtocolRef RSProtocol1 = (RSProtocolRef)rs1;
    RSProtocolRef RSProtocol2 = (RSProtocolRef)rs2;
    BOOL result = NO;
    
    result = RSEqual(RSProtocol1->_name, RSProtocol2->_name);
    if (result) result = RSEqual(RSProtocol1->_table, RSProtocol2->_table);
    
    return result;
}

static RSHashCode __RSProtocolClassHash(RSTypeRef rs)
{
    return RSHash(((RSProtocolRef)rs)->_name);
}

static RSStringRef __RSProtocolClassDescription(RSTypeRef rs)
{
    RSStringRef description = RSStringCreateWithFormat(RSAllocatorSystemDefault, RSSTR("RSProtocol <%r>"), ((RSProtocolRef)rs)->_name);
    return description;
}

static RSRuntimeClass __RSProtocolClass =
{
    _RSRuntimeScannedObject,
    "RSProtocol",
    __RSProtocolClassInit,
    __RSProtocolClassCopy,
    __RSProtocolClassDeallocate,
    __RSProtocolClassEqual,
    __RSProtocolClassHash,
    __RSProtocolClassDescription,
    nil,
    nil
};

static RSTypeID _RSProtocolTypeID = _RSRuntimeNotATypeID;

RSExport RSTypeID RSProtocolGetTypeID()
{
    return _RSProtocolTypeID;
}

static __unused RSMutableDictionaryRef __RSProtocolCache = nil;  // RSProtocolRef included
static __unused RSSpinLock __RSProtocolCacheLock = RSSpinLockInit;

static __unused BOOL __RSProtocolCacheAddProtocol(RSProtocolRef protocol)
{
    if (!protocol) return NO;
    RSStringRef name = RSProtocolGetName(protocol);
    RSSpinLockLock(&__RSProtocolCacheLock);
    if (RSDictionaryGetValue(__RSProtocolCache, name))
    {
        RSSpinLockUnlock(&__RSProtocolCacheLock);
        return YES;
    }
    RSDictionarySetValue(__RSProtocolCache, name, protocol);
    RSSpinLockUnlock(&__RSProtocolCacheLock);
    return YES;
}

static __unused BOOL __RSProtocolCacheRemoveProtocol(RSProtocolRef protocol)
{
    if (!protocol) return NO;
    __RSGenericValidInstance(protocol, _RSProtocolTypeID);
    RSStringRef name = RSProtocolGetName(protocol);
    RSSpinLockLock(&__RSProtocolCacheLock);
    RSTypeRef value = nil;
    if (!(value = RSDictionaryGetValue(__RSProtocolCache, name)))
    {
        RSSpinLockUnlock(&__RSProtocolCacheLock);
        return NO;
    }
    __RSRuntimeSetInstanceSpecial(value, NO);
    RSDictionaryRemoveValue(__RSProtocolCache, name);
    RSSpinLockUnlock(&__RSProtocolCacheLock);
    return YES;
}

static __unused RSProtocolRef __RSProtocolCacheLookup(RSStringRef name)
{
    if (!name) return nil;
    RSProtocolRef protocol = nil;
    RSSpinLockLock(&__RSProtocolCacheLock);
    
    if ((protocol = (RSProtocolRef)RSDictionaryGetValue(__RSProtocolCache, name)))
    {
        RSSpinLockUnlock(&__RSProtocolCacheLock);
        return protocol;
    }
    RSSpinLockUnlock(&__RSProtocolCacheLock);
    return protocol;
}

static void __RSProtocolDeallocate()
{
    __RSRuntimeSetInstanceSpecial(__RSProtocolCache, NO);
    if (__RSProtocolCache) {
        RSSpinLockLock(&__RSProtocolCacheLock);
        RSRelease(__RSProtocolCache);
        RSSpinLockUnlock(&__RSProtocolCacheLock);
    }
    __RSProtocolCache = nil;
}

RSPrivate void __RSProtocolInitialize()
{
    _RSProtocolTypeID = __RSRuntimeRegisterClass(&__RSProtocolClass);
    __RSRuntimeSetClassTypeID(&__RSProtocolClass, _RSProtocolTypeID);
    
    __RSProtocolCache = RSDictionaryCreateMutable(RSAllocatorSystemDefault, 0, RSDictionaryRSTypeContext);
    __RSRuntimeSetInstanceSpecial(__RSProtocolCache, YES);
    
    RSObserverRef observer = RSObserverCreate(RSAllocatorSystemDefault, RSCoreFoundationWillDeallocateNotification, __RSProtocolDeallocate, nil);
    RSNotificationCenterAddObserver(RSNotificationCenterGetDefault(), observer);
    RSRelease(observer);
}

static RSDictionaryContext ___RSProtocolDictionayContext = {
    0,
    &___RSConstantCStringKeyContext,
    &___RSDictionaryNilValueContext,
};

static BOOL __RSProtocolVerifyPCE(restrict PCE pce)
{
    BOOL result = NO;
    if (!pce) return result;
    RSUInteger length = strlen(pce);
    if (!length) return result;
    if (isnumber(pce[0])) return NO;
    for (RSUInteger idx = 1; idx < length; ++idx) {
        if (isnumber(pce[idx]) || isalpha(pce[idx]) || pce[idx] == '_') continue;
        else return NO;
    }
    return YES;
}

static RSProtocolRef __RSProtocolCreateInstance(RSAllocatorRef allocator, RSStringRef name, RSDictionaryRef pces, RSArrayRef supers)
{
    RSProtocolRef instance = (RSProtocolRef)__RSRuntimeCreateInstance(allocator, _RSProtocolTypeID, sizeof(struct __RSProtocol) - sizeof(RSRuntimeBase));
    
    instance->_name = RSRetain(name);
    instance->_table = (RSMutableDictionaryRef)RSRetain(pces);
    if (supers) instance->_superProtocols = RSMutableCopy(allocator, supers);
    __RSRuntimeSetInstanceSpecial(instance, YES);
    __RSProtocolCacheAddProtocol(instance);
    return instance;
}

static RSProtocolRef __RSProtocolCreateWithContext(RSAllocatorRef allocator, RSProtocolContext *context, RSArrayRef supers)
{
    if (!context || !context->delegateName) return nil;
    RSProtocolRef protocol = __RSProtocolCacheLookup(__RSStringMakeConstantString(context->delegateName));
    if (protocol) {
        return (RSProtocolRef)RSRetain(protocol);
    }
    RSMutableDictionaryRef table = RSDictionaryCreateMutable(allocator, 0, &___RSProtocolDictionayContext);
    RSProtocolItem *item = nil;
    for (RSUInteger idx = 0; idx < context->countOfItems; ++idx) {
        item = &context->items[idx];
        if (!item->sel || !__RSProtocolVerifyPCE(item->sel))
        {
            RSExceptionCreateAndRaise(RSAllocatorSystemDefault, RSInvalidArgumentException, RSStringWithFormat(RSSTR("RSProtocol<%s> sel field is nil"), context->delegateName), nil);
            RSRelease(table);
            return nil;
        }
        
        if (!item->imp)
        {
            RSExceptionCreateAndRaise(RSAllocatorSystemDefault, RSInvalidArgumentException, RSStringWithFormat(RSSTR("RSProtocol<%s> imp field is nil for sel<%s>"), context->delegateName, item->sel), nil);
            RSRelease(table);
            return nil;
        }
        
        RSDictionarySetValue(table, item->sel, item->imp);
    }
    protocol = __RSProtocolCreateInstance(allocator, RSStringWithFormat(RSSTR("%s"), context->delegateName), table, supers);
    RSRelease(table);
    return protocol;
}

RSExport RSStringRef RSProtocolGetName(RSProtocolRef protocol)
{
    __RSGenericValidInstance(protocol, _RSProtocolTypeID);
    return protocol->_name;
}

RSExport RSArrayRef RSProtocolGetSupers(RSProtocolRef protocol)
{
    __RSGenericValidInstance(protocol, _RSProtocolTypeID);
    return protocol->_superProtocols;
}

RSExport BOOL RSProtocolIsRoot(RSProtocolRef protocol)
{
    RSArrayRef protocols = RSProtocolGetSupers(protocol);
    if (!protocols) return YES;
    return (RSArrayGetCount(protocols) == 0);
}

static ISA __RSProtocolLookupMethodWithName(restrict RSProtocolRef protocol, restrict PCE pce)
{
    restrict ISA imp = (ISA)RSDictionaryGetValue(protocol->_table, pce);
    if (imp) return imp;
    if (protocol->_superProtocols)
    {
        RSUInteger idx = 0, cnt = RSArrayGetCount(protocol->_superProtocols);
        while (protocol && idx < cnt) {
            protocol = (RSProtocolRef)RSArrayObjectAtIndex(protocol->_superProtocols, idx++);
            imp = __RSProtocolLookupMethodWithName(protocol, pce);  // Depth-first recursion
            if (imp) break;
        }
    }
    return imp;
}

RSExport ISA RSProtocolGetImplementationWithPCE(RSProtocolRef protocol, PCE pce)
{
    if (pce) return __RSProtocolLookupMethodWithName(protocol, pce);
    return nil;
}

RSExport BOOL RSProtocolConfirmImplementation(RSProtocolRef protocol, PCE pce)
{
    if (RSProtocolGetImplementationWithPCE(protocol, pce)) return YES;
    return NO;
}

RSExport RSProtocolRef RSProtocolCreateWithContext(RSAllocatorRef allocator, RSProtocolContext *context, RSArrayRef supers)
{
    return __RSProtocolCreateWithContext(allocator, context, supers);
}

RSExport RSProtocolRef RSProtocolCreateWithName(RSAllocatorRef allocator, RSStringRef name)
{
    if (!name) return nil;
    return (RSProtocolRef) RSRetain(__RSProtocolCacheLookup(name));
}
