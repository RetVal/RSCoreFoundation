//
//  RSBase.c
//  RSCoreFoundation
//
//  Created by RetVal on 10/14/12.
//  Copyright (c) 2012 RetVal. All rights reserved.
//

#include <RSCoreFoundation/RSBase.h>
#include <RSCoreFoundation/RSNil.h>
#include <RSCoreFoundation/RSRuntime.h>
#include <RSCoreFoundation/RSDate.h>
#include <RSCoreFoundation/RSNumber.h>
#include <RSCoreFoundation/RSTimeZone.h>

#include "../BaseStructure/RSPrivate/RSPrivateOperatingSystem/RSPrivateTask.h"

RSInline RSTypeID __RSGetTypeID(RSTypeRef obj)
{
    RSTypeID typeID = 0;
    if (RS_IS_TAGGED_OBJ(obj))
    {
        switch (typeID = RS_TAGGED_OBJ_TYPE(obj))
        {
            case RSTaggedObjectID_Integer:
                typeID = RSNumberGetTypeID();
                break;
            default:
                HALTWithError(RSGenericException, "invalid object");
                break;
        }
        return typeID;
    }
    else typeID = ((RSRuntimeBase*)obj)->_rsinfo._objId;
    return typeID;
}

RSInline BOOL   __RSCheckInstanceISCustomReferenceType(RSTypeRef obj) {
    RSTypeID typeID = __RSGetTypeID(obj);
    if (typeID == _RSRuntimeNotATypeID) {
        __RSCLog(RSLogLevelDebug, "object is <%p>\n", obj);
        HALTWithError(RSInvalidArgumentException, "the type of the object is not a registered class in runtime");
    }
    const RSRuntimeClass *cls = __RSRuntimeGetClassWithTypeID(typeID);
    if (cls && cls->refcount) return YES;
    return NO;
}

RSExport RSTypeRef RSRetain(RSTypeRef obj)
{
    if (obj == nil || RS_IS_TAGGED_OBJ(obj)) return obj;
    __RSRuntimeCheckInstanceAvailable(obj);
    if (__RSCheckInstanceISCustomReferenceType(obj))
    {
        __RSRuntimeInstanceRetain(obj, NO);  // not try
#if __RSRuntimeDebugPreference
        if (___RSDebugLogPreference._RSRuntimeInstanceRefWatcher) {
#if __LP64__
            __RSCLog(RSLogLevelDebug, "%s - retain <%p> - count : %lld\n", __RSRuntimeGetClassWithTypeID(RSGetTypeID(obj))->className, obj, RSGetRetainCount(obj));
#else
            __RSCLog(RSLogLevelDebug, "%s - retain <%p> - count : %ld\n", __RSRuntimeGetClassWithTypeID(RSGetTypeID(obj))->className, obj, RSGetRetainCount(obj));
#endif
        }
#endif
        return obj;
    }
    else
    {
        __RSRuntimeRetain(obj, NO);
#if __RSRuntimeDebugPreference
        if (___RSDebugLogPreference._RSRuntimeInstanceRefWatcher) {
#if __LP64__
            __RSCLog(RSLogLevelDebug, "%s - retain <%p> - count : %lld\n", __RSRuntimeGetClassWithTypeID(RSGetTypeID(obj))->className, obj, RSGetRetainCount(obj));
#else
            __RSCLog(RSLogLevelDebug, "%s - retain <%p> - count : %ld\n", __RSRuntimeGetClassWithTypeID(RSGetTypeID(obj))->className, obj, RSGetRetainCount(obj));
#endif
        }
#endif

        return obj;
    }

    return nil;
}


RSPrivate void __RSRelease(RSTypeRef obj)
{
    if (__RSCheckInstanceISCustomReferenceType(obj))
    {
        __RSRuntimeInstanceRelease(obj, NO);  // not try
        obj = nil;
        return;
    }
    else
    {
        __RSRuntimeRelease(obj, NO);
        obj = nil;
        return ;
    }
}

RSExport void RSRelease(RSTypeRef RS_RELEASES_ARGUMENT obj)
{
    if (obj == nil || RS_IS_TAGGED_OBJ(obj)) return;
    
#if __RSRuntimeDebugPreference
    if (___RSDebugLogPreference._RSRuntimeInstanceRefWatcher) {
        RSIndex rc = RSGetRetainCount(obj);
#if __LP64__
        __RSCLog(RSLogLevelDebug, "%s - release <%p> - count : %lld\n", __RSRuntimeGetClassWithTypeID(RSGetTypeID(obj))->className, obj, rc - 1);
#else
        __RSCLog(RSLogLevelDebug, "%s - release <%p> - count : %ld\n", __RSRuntimeGetClassWithTypeID(RSGetTypeID(obj))->className, obj, rc - 1);
#endif
    }
#endif
    
#if __RSRuntimeInstanceARC & 0
    if (__RSIsAutorelease(obj))
    {
        void* p = __builtin_return_address(1);
        if (likely(p == __RSAutoreleasePoolReleaseUntil))
        {
            // pool drain
        }
        RSIndex rc = RSGetRetainCount(obj);
        if (rc == 1)
        {
            __RSSetUnAutorelease(obj);
            
        }
    }
#endif
#if __RSRuntimeDebugPreference
    if (___RSDebugLogPreference._RSRuntimeCheckAutoreleaseFlag) {
        RSIndex rc = RSGetRetainCount(obj);
        if (1 == rc && __RSIsAutorelease(obj)) {
            __RSCLog(RSLogLevelWarning, "%s want to release autoreleased object <%p, %lld>\n", __FUNCTION__, obj, RSGetTypeID(obj));
        }
    }
#endif
    __RSRelease(obj);
}

RSExport RSMutableTypeRef RSAutorelease(RSTypeRef RS_RELEASES_ARGUMENT obj)
{
    if (obj == nil || RS_IS_TAGGED_OBJ(obj) || __RSRuntimeInstanceIsClass(obj) || __RSRuntimeInstanceIsStackValue(obj) || __RSRuntimeInstanceIsSpecial(obj))
        return (RSMutableTypeRef)obj;
    __RSRuntimeCheckInstanceAvailable(obj);
    
    if (__RSGetTypeID(obj) == RSAutoreleasePoolGetTypeID())
        HALTWithError(RSInvalidArgumentException, "RSAutoreleasePool can not add itself to the pool!");
    if (__RSRuntimeInstanceIsStackValue(obj) || __RSRuntimeInstanceIsSpecial(obj)) return (RSMutableTypeRef)obj;
#if __RSRuntimeDebugPreference 
    if (___RSDebugLogPreference._RSRuntimeCheckAutoreleaseFlag) {
        if (__RSIsAutorelease(obj) && RSGetRetainCount(obj) == 1) {
            __RSCLog(RSLogLevelWarning, "%s want to add autoreleased object. refuse adding it<%p, %s>\n", __FUNCTION__, obj, __RSRuntimeGetClassNameWithInstance(obj));
            return (RSMutableTypeRef)__RSAutorelease(obj);
        }
        __RSSetAutorelease(obj);
    }
#endif
    return (RSMutableTypeRef)__RSAutorelease(obj);
}

RSExport RSIndex RSGetRetainCount(RSTypeRef obj)
{
    if (obj == nil) return 0;
    if (RS_IS_TAGGED_OBJ(obj) || __RSRuntimeInstanceIsSpecial(obj) || __RSRuntimeInstanceIsClass(obj)) return 1;
    __RSRuntimeCheckInstanceAvailable(obj);
    RSIndex ref = 0;
    if (__RSCheckInstanceISCustomReferenceType(obj)) {
        // the object self support the reference function.
        // call the class method to do the reference retain.
        ref = __RSRuntimeInstanceGetRetainCount(obj);
    }
    else ref = __RSRuntimeGetRetainCount(obj);
    return (ref < RSIndexMax) ? (ref) : (RSIndexMax);
}

RSExport RSTypeID RSGetTypeID(RSTypeRef obj)
{
    if (obj == nil) obj = RSNil;
    __RSRuntimeCheckInstanceAvailable(obj);
    return __RSGetTypeID(obj);
}

RSExport BOOL RSEqual(RSTypeRef obj1, RSTypeRef obj2)
{
    if (obj1 == obj2) return YES;
    if (obj1 == nil || obj2 == nil) return NO;
    __RSRuntimeCheckInstanceAvailable(obj1);
    __RSRuntimeCheckInstanceAvailable(obj2);
    RSTypeID ID1 = __RSGetTypeID(obj1);
    RSTypeID ID2 = __RSGetTypeID(obj2);
    if (ID1 != ID2) { return NO; }
    
    if (__RSRuntimeInstanceIsClass(obj1) || __RSRuntimeInstanceIsClass(obj2))
        return NO;
    
    RSRuntimeClassEqual equal = __RSRuntimeGetClassWithTypeID(ID1)->equal;
    if (equal) {
        return equal(obj1,obj2);
    }
    return NO;
}

RSExport BOOL RSNotEqual(RSTypeRef obj1, RSTypeRef obj2) {
    return !RSEqual(obj1, obj2);
}

#define ELF_STEP(B) T1 = (H << 4) + B; T2 = T1 & 0xF0000000; if (T2) T1 ^= (T2 >> 24); T1 &= (~T2); H = T1;

RSHashCode RSHashBytes(RSBitU8* bytes, RSBit32 length)
{
    /* The ELF hash algorithm, used in the ELF object file format */
    RSBitU32 H = 0, T1, T2;
    RSBit32 rem = (RSBit32)length;
    while (3 < rem)
    {
        ELF_STEP(bytes[length - rem]);
        ELF_STEP(bytes[length - rem + 1]);
        ELF_STEP(bytes[length - rem + 2]);
        ELF_STEP(bytes[length - rem + 3]);
        rem -= 4;
    }
    switch (rem) {
        case 3:  ELF_STEP(bytes[length - 3]);
        case 2:  ELF_STEP(bytes[length - 2]);
        case 1:  ELF_STEP(bytes[length - 1]);
        case 0:  ;
    }
    return H;
}

#undef ELF_STEP

RSExport RSHashCode RSHash(RSTypeRef obj)
{
    if (obj == nil) return 0;
//    if (RS_IS_TAGGED_OBJ(obj)) return (RSHashCode)obj;
    __RSRuntimeCheckInstanceAvailable(obj);
    
    if (__RSRuntimeInstanceIsClass(obj)) return (RSHashCode)obj;
    
    RSTypeID id = __RSGetTypeID(obj);
    RSRuntimeClassHash hash = __RSRuntimeGetClassWithTypeID(id)->hash;
    if (hash) {
        return hash(obj);
    }
    return (RSHashCode)obj;
}
void RSDeallocateInstance(RSTypeRef obj)
{
    if (nil == obj) HALTWithError(RSInvalidArgumentException, "the object is nil");
    if (__RSRuntimeInstanceIsClass(obj)) return;
    if (unlikely(YES == __RSRuntimeInstanceIsStackValue(obj))) return;
    RSTypeID id = _RSRuntimeNotATypeID;
    if ((id = RSGetTypeID(obj)) == _RSRuntimeNotATypeID) HALTWithError(RSInvalidArgumentException, "the object is not available");
    RSRuntimeClass* cls = (RSRuntimeClass*)__RSRuntimeGetClassWithTypeID(id);
    if (___RSDebugLogPreference._RSRuntimeInstanceManageWatcher)
        __RSCLog(RSLogLevelDebug, "%s dealloc - <%p>\n",cls->className, obj);

    if (cls->deallocate) __RSRuntimeInstanceDeallocate(obj);
    return __RSRuntimeDeallocate(obj);
    HALTWithError(RSInvalidArgumentException, "overflow here");
}

RSExport RSTypeRef RSCopy(RSAllocatorRef allocator, RSTypeRef obj)
{
    if (nil == obj) return nil; //HALTWithError(RSInvalidArgumentException, "the object is nil");
    
    if (RS_IS_TAGGED_OBJ(obj) || __RSRuntimeInstanceIsClass(obj)) return obj;
    RSIndex id = _RSRuntimeNotATypeID;
    if ((id = RSGetTypeID(obj)) == _RSRuntimeNotATypeID) HALTWithError(RSInvalidArgumentException, "the object is not available");
    RSRuntimeClass* cls = (RSRuntimeClass*)__RSRuntimeGetClassWithTypeID(id);
    if (cls->copy) {
        // deep copy, return a new create object with retian count is 1.
        // if the class not support the deep copy, just return nil.
        return cls->copy(allocator, obj, NO);
    }
    // shallow copy, just retain the object and return itself because of the class is not support copy method.
    return RSRetain(obj);
}

RSExport RSMutableTypeRef RSMutableCopy(RSAllocatorRef allocator, RSTypeRef obj)
{
    if (nil == obj) return nil; //HALTWithError(RSInvalidArgumentException, "the object is nil");
    
    if (RS_IS_TAGGED_OBJ(obj) || __RSRuntimeInstanceIsClass(obj)) return (RSMutableTypeRef)obj;
    
    RSIndex id = _RSRuntimeNotATypeID;
    if ((id = RSGetTypeID(obj)) == _RSRuntimeNotATypeID) HALTWithError(RSInvalidArgumentException, "the object is not available");
    RSRuntimeClass* cls = (RSRuntimeClass*)__RSRuntimeGetClassWithTypeID(id);
    if (cls->copy) {
        // deep copy, return a new create object with retian count is 1.
        // if the class not support the deep copy, just return nil.
        return (RSMutableTypeRef)cls->copy(allocator, obj, YES);
    }
    // here we do not do the shallow copy because the caller want to change something about the orginal object.
    return nil;

}

RSExport RSStringRef RSDescription(RSTypeRef obj)
{
    if (nil == obj) obj = RSNil;
    __RSRuntimeCheckInstanceAvailable(obj);
    
    if (!RS_IS_TAGGED_OBJ(obj) && __RSRuntimeInstanceIsClass(obj))
        return RSStringCreateWithFormat(RSAllocatorSystemDefault, RSSTR("RSClass %s"), ((RSRuntimeClass *)obj)->className);
    
    RSRuntimeClass* cls = (RSRuntimeClass*)__RSRuntimeGetClassWithTypeID(__RSGetTypeID(obj));
    if (cls->description)
    {
        RSRuntimeClassDescription description = cls->description;
        return description(obj);
    }
    RSStringRef string = RSStringCreateWithFormat(RSAllocatorSystemDefault, RSSTR("<%s %p [%p]>"), __RSRuntimeGetClassNameWithInstance(obj), obj, RSGetAllocator(obj));
    return string;
}

RSExport void RSLog(RSStringRef format,...)
{
    RSRetain(format);
    va_list args ;
    va_start(args, format);
    
    __RSLogArgs(RSLogLevelNotice, format, args);
    
    va_end(args);
    RSRelease(format);
}

RSExport RSTypeRef RSShow(RSTypeRef obj)
{
    if (nil == obj) obj = RSNil;
    RSLog(RSSTR("%R"), obj);
    return obj;
}

RSExport RSStringRef RSStringFromRSRange(RSRange range)
{
    return RSAutorelease(RSStringCreateWithFormat(RSAllocatorSystemDefault, RSSTR("(%lld, %lld)"), range.location, range.length));
}

#pragma mark -
#pragma mark RSClassRef API
RSExport RSStringRef RSClassNameFromInstance(RSTypeRef id)
{
    return RSClassGetName(RSClassFromInstance(id));
}

RSExport RSStringRef RSClassGetName(RSClassRef cls) {
    if (!cls) return nil;
    return RSAutorelease(RSStringCreateWithCString(RSAllocatorSystemDefault, ((RSRuntimeClass *)cls)->className, RSStringEncodingUTF8));
}

RSExport RSTypeID RSClassGetTypeID(RSClassRef cls) {
    if (!cls) return _RSRuntimeNotATypeID;
    if (!__RSRuntimeInstanceIsClass(cls)) return _RSRuntimeNotATypeID;
    return __RSRuntimeGetClassTypeID(cls);
}

RSExport BOOL RSInstanceIsMemberOfClass(RSTypeRef id, RSClassRef cls) {
    return RSClassGetTypeID(cls) == RSGetTypeID(id);
}

RSExport RSClassRef RSClassFromInstance(RSTypeRef id) {
    if (id == nil) return nil;
    __RSRuntimeCheckInstanceAvailable(id);
    
    if (__RSRuntimeInstanceIsClass(id)) return id;
    
    return __RSRuntimeGetClassWithTypeID(__RSGetTypeID(id));
}

RSExport RSClassRef RSClassGetWithName(RSStringRef name) {
    return RSClassGetWithUTF8String(RSStringGetUTF8String(name));
}

RSExport RSClassRef RSClassGetWithUTF8String(const char *name) {
    if (!name) return nil;
    RSTypeID ID = __RSRuntimeGetClassTypeIDWithName(name);
    if (ID) return __RSRuntimeGetClassWithTypeID(ID);
    return nil;
}

RSExport BOOL RSInstanceIsMemberOfClassByUTF8String(RSTypeRef id, const char *name) {
    return RSInstanceIsMemberOfClass(id, RSClassGetWithUTF8String(name));
}

RSExport RSStringRef RSClassGetNameWithInstance(RSTypeRef id) {
    return RSClassGetName(RSClassFromInstance(id));
}

#if RS_BLOCKS_AVAILABLE
void RSSyncUpdateBlock(volatile RSSpinLock * lock, void (^block)(void))
{
    if (!lock) return;
    RSSpinLockLock(lock);
    block();
    if (!RSSpinLockTry(lock))
        RSSpinLockUnlock(lock);
    else
        __RSCLog(RSLogLevelWarning, "May be case an dead lock, %s can only be used updating your data synchronized", __func__);
}
#endif
