//
//  RSArchiver.c
//  RSCoreFoundation
//
//  Created by RetVal on 9/2/13.
//  Copyright (c) 2013 RetVal. All rights reserved.
//

#include "RSArchiver.h"

#include <RSCoreFoundation/RSRuntime.h>
#include <RSCoreFoundation/RSDictionary.h>
#include <RSCoreFoundation/RSDictionary+Extension.h>
#include <RSCoreFoundation/RSError.h>
#include <RSCoreFoundation/RSNumber.h>
#include <RSCoreFoundation/RSNumber+Extension.h>
#include <RSCoreFoundation/RSCalendar.h>
#include <RSCoreFoundation/RSTimeZone.h>
#include <RSCoreFoundation/RSBinaryPropertyList.h>
#include <RSCoreFoundation/RSPropertyList.h>
#include <RSCoreFoundation/RSNil.h>
#include <RSCoreFoundation/RSNotification.h>
#include <RSCoreFoundation/RSError+Extension.h>
#include <RSCoreFoundation/RSData+Extension.h>

extern RSTypeRef __RSBPLCreateObjectWithData(RSDataRef data);
extern RSDataRef __RSBPLCreateDataForObject(RSTypeRef obj);
extern RSMutableDataRef __RSPropertyListCreateWithError(RSTypeRef root, __autorelease RSErrorRef* error);

#pragma mark -
#pragma mark RSUnarchiver Class System
// dynamic register class will case the TypeID of class changed, use class-name transform.

static RSArrayRef __RSUnarchiverCopyClassNamesFromClassTypeIDs(RSArrayRef classTypeIDs) {
    RSMutableArrayRef classNames = RSArrayCreateMutable(RSAllocatorSystemDefault, RSArrayGetCount(classTypeIDs));
    RSArrayApplyBlock(classTypeIDs, RSMakeRange(0, RSArrayGetCount(classTypeIDs)), ^(const void *value, RSUInteger idx, BOOL *isStop) {
        RSNumberRef classTypeID = (RSNumberRef)value;
        RSTypeID typeID = _RSRuntimeNotATypeID;
        if (RSNumberGetValue(classTypeID, &typeID)) {
            RSStringRef className = RSStringCreateWithCString(RSAllocatorSystemDefault, __RSRuntimeGetClassNameWithTypeID(typeID), RSStringEncodingUTF8);
            RSArrayAddObject(classNames, className);
            RSRelease(className);
        }
    });
    return classNames;
}

static RSArrayRef __RSUnarchiverCopyClassTypeIDsFromClassNames(RSArrayRef classNames) {
    RSMutableArrayRef classTypeIDs = RSArrayCreateMutable(RSAllocatorSystemDefault, RSArrayGetCount(classNames));
    RSArrayApplyBlock(classNames, RSMakeRange(0, RSArrayGetCount(classNames)), ^(const void *value, RSUInteger idx, BOOL *isStop) {
        RSTypeID classTypeID = __RSRuntimeGetClassTypeIDWithName(RSStringGetUTF8String((RSStringRef)value));
        if (classTypeID != _RSRuntimeNotATypeID) {
            RSArrayAddObject(classTypeIDs,
#if __LP64__
                             RSNumberWithUnsignedLonglong(classTypeID)
#else
                             RSNumberWithUnsignedLong(classTypeID)
#endif
                             );
        }
    });
    return classTypeIDs;
}

RS_PUBLIC_CONST_STRING_DECL(__RSArchiverContextData, "context");    // RSDictionary / RSData
RS_PUBLIC_CONST_STRING_DECL(__RSArchiverContextOrder, "order");     // RSArray
RS_PUBLIC_CONST_STRING_DECL(__RSArchiverContextEncoded, "encoded"); // RSNumber (BOOL)
RS_PUBLIC_CONST_STRING_DECL(RSKeyedArchiveRootObjectKey, "root");

#pragma mark -
#pragma mark RSArchiver Call Backs

static RSSpinLock _RSArchiverCallbacksLock = RSSpinLockInit;
static RSMutableDictionaryRef _RSArchiverCallbacks = nil;       // class-name(C-String) : isa(archiver-callbacks)

static RSMutableDictionaryRef _RSArchiverCache __unused = nil;           // Archiver-name(RSString) : RSArchiver

extern const RSDictionaryContext* __RSStringConstantTableContext;
extern RSTypeRef ___RSISAPayloadCreateWithISACopy(RSAllocatorRef allocator, ISA isa, RSIndex size);
extern ISA ___RSISAPayloadGetISA(RSTypeRef _isa);

static void __RSArchiverCallBacksInitialize() {
    RSSpinLockLock(&_RSArchiverCallbacksLock);
    if (!_RSArchiverCallbacks) {
        _RSArchiverCallbacks = RSDictionaryCreateMutable(RSAllocatorSystemDefault, 0, __RSStringConstantTableContext);
        __RSRuntimeSetInstanceSpecial(_RSArchiverCallbacks, YES);
    }
    RSSpinLockUnlock(&_RSArchiverCallbacksLock);
}

RSInline RSIndex __RSArchiverGetVersion(const RSArchiverCallBacks *callbacks) {
    return callbacks->version;
}

RSInline RSArchiverSerializeCallBack __RSArchiverGetSerializeCallBack(const RSArchiverCallBacks *callbacks) {
    return callbacks->serialize;
}

RSInline RSArchiverDeserializeCallBack __RSArchiverGetDeserializeCallBack(const RSArchiverCallBacks *callbacks) {
    return callbacks->deserialize;
}

RSInline RSTypeID __RSArchiverGetClassID(const RSArchiverCallBacks *callbacks) {
    return callbacks->classID;
}

RSInline BOOL __RSArchiverCallBacksValid(const RSArchiverCallBacks *callbacks)
{
    if (callbacks && __RSArchiverGetVersion(callbacks) == 0 && __RSArchiverGetSerializeCallBack(callbacks) && __RSArchiverGetDeserializeCallBack(callbacks) && __RSRuntimeGetClassWithTypeID(__RSArchiverGetClassID(callbacks))) return YES;
    return NO;
}

static BOOL __RSArchiverRegisterCallbacks(const RSArchiverCallBacks *callbacks) {
    if (__RSArchiverCallBacksValid(callbacks)) {
        RSTypeRef isa = ___RSISAPayloadCreateWithISACopy(RSAllocatorSystemDefault, (ISA)callbacks, sizeof(RSArchiverCallBacks));
        if (!isa) return NO;
        BOOL result = NO;
        RSSpinLockLock(&_RSArchiverCallbacksLock);
        if (nil == RSDictionaryGetValue(_RSArchiverCallbacks, __RSRuntimeGetClassNameWithTypeID(__RSArchiverGetClassID(callbacks)))) {
            RSDictionarySetValue(_RSArchiverCallbacks, __RSRuntimeGetClassNameWithTypeID(__RSArchiverGetClassID(callbacks)), (isa));
            result = YES;
        }
        else
            __RSCLog(RSLogLevelWarning, "Archiver type conflict");
        RSSpinLockUnlock(&_RSArchiverCallbacksLock);
        RSRelease(isa);
        return result;
    }
    return NO;
}

static void __RSArchiverUnregisterCallbacks(RSTypeID clsId) {
    if (clsId > 0) {
        const void *key = __RSRuntimeGetClassNameWithTypeID(clsId);
        if (!key) return;
        RSSpinLockLock(&_RSArchiverCallbacksLock);
        RSDictionaryRemoveValue(_RSArchiverCallbacks, key);
        RSSpinLockUnlock(&_RSArchiverCallbacksLock);
    }
}

RSExport BOOL RSArchiverRegisterCallbacks(const RSArchiverCallBacks * callbacks ) {
    return __RSArchiverRegisterCallbacks(callbacks);
}

RSExport void RSArchiverUnregisterCallbacks(RSTypeID classId) {
    return __RSArchiverUnregisterCallbacks(classId);
}

static RSTypeRef __RSArchiverGetCallBacksISAForID(RSTypeID ID) {
    RSTypeRef isa = nil;
    RSSpinLockLock(&_RSArchiverCallbacksLock);
    isa = RSDictionaryGetValue(_RSArchiverCallbacks, __RSRuntimeGetClassNameWithTypeID(ID));
    RSSpinLockUnlock(&_RSArchiverCallbacksLock);
    return isa;
}

static RSArchiverCallBacks *__RSArchiverGetCallBacksForID(RSTypeID ID) {
    RSTypeRef isa = __RSArchiverGetCallBacksISAForID(ID);
    return isa ? ___RSISAPayloadGetISA(isa) : nil;
}

#pragma mark -
#pragma mark RSArchiver Context

struct __RSArchiverContext {
    RSIndex _deserializeIndex;          // Current Unarchive Object index
    RSIndex _serializeIndex;            // Current Archive Object index
    RSMutableArrayRef _archiveOrder;    // RSNumber(tagged-object)
    RSMutableDictionaryRef _archiveData;// RSString(key) : RSData(Archive Data)
    RSMutableDataRef _data;             // Caller setup(weak)
};

typedef struct __RSArchiverContext __RSArchiverContext;
typedef struct __RSArchiverContext __RSUnarchiverContext;

static BOOL __RSArchiverContextInit(__RSArchiverContext *context) {
    context->_serializeIndex = 0;
    context->_archiveData = RSDictionaryCreateMutable(RSAllocatorSystemDefault, 0, RSDictionaryRSTypeContext);
    context->_archiveOrder = RSArrayCreateMutable(RSAllocatorSystemDefault, 0);
    return context->_archiveData && context->_archiveOrder;
}

static void __RSArchiverContextRelease(__RSArchiverContext *context) {
    if (context->_archiveData) {
        RSRelease(context->_archiveData);
        context->_archiveData = nil;
    }
    
    if (context->_archiveOrder) {
        RSRelease(context->_archiveOrder);
        context->_archiveData = nil;
    }
}

RSInline RSIndex __RSArchiverContextGetSerializeIndex(const __RSArchiverContext *context)
{
    return context->_serializeIndex;
}

RSInline RSIndex __RSArchiverContextGetDeserializeIndex(const __RSArchiverContext *context)
{
    return context->_deserializeIndex;
}

RSInline void __RSArchiverContextAddSerializeIndex(__RSArchiverContext *context, RSIndex idx)
{
    context->_serializeIndex += idx;
}

RSInline void __RSArchiverContextAddDeserializeIndex(__RSArchiverContext *context, RSIndex idx)
{
    context->_deserializeIndex += idx;
}

RSInline RSIndex __RSArchiverContextGetIndexAndIncrement(__RSArchiverContext *context)
{
    RSIndex ret = __RSArchiverContextGetSerializeIndex(context);
    __RSArchiverContextAddSerializeIndex(context, 1);
    return ret;
}

RSInline RSMutableArrayRef __RSArchiverContextGetOrder(const __RSArchiverContext *context)
{
    return context->_archiveOrder;
}

static void __RSArchiverContextPushTypeID(const __RSArchiverContext *context, RSTypeID ID)
{
    RSArrayAddObject(__RSArchiverContextGetOrder(context), RSNumberWithUnsignedLonglong(ID));
    __RSArchiverContextAddSerializeIndex((__RSArchiverContext *)context, 1);
}

RSInline RSTypeID __RSArchiverContextPopTypeID(const __RSArchiverContext *context)
{
    RSTypeID ID = 0;
    if (RSNumberGetValue(RSArrayObjectAtIndex(__RSArchiverContextGetOrder(context), __RSArchiverContextGetDeserializeIndex(context)), &ID))
        __RSArchiverContextAddDeserializeIndex((__RSArchiverContext*)context, 1);
    return ID;
}

RSInline RSMutableDictionaryRef __RSArchiverContextGetDataMap(const __RSArchiverContext *context)
{
    return context->_archiveData;
}

RSInline RSTypeRef __RSArchiverContextGetData(const __RSArchiverContext *context)
{
    return context->_data;
}

RSInline void __RSArchiverContextSetData(RSArchiverContextRef context, RSMutableDataRef data)
{
    context->_data = data;
}

RSInline const __RSArchiverContext *__RSArchiverGetContext(const RSArchiverRef archiver);
RSInline RSStringRef __RSArchiverGetCurrentKey(const RSArchiverRef archiver);
RSInline void __RSArchiverSetCurrentKey(RSArchiverRef archiver, RSStringRef key);
static const RSArchiverCallBacks *__RSArchiverGetCallBacks(const RSArchiverRef archiver, RSTypeID ID);


RSInline RSDataRef __RSArchiverContextSerializeEx(const RSArchiverRef archiver, RSTypeRef object, BOOL push)
{
    RSTypeID ID = RSGetTypeID(object);
    if (push) __RSArchiverContextPushTypeID(__RSArchiverGetContext(archiver), ID);
    RSDataRef data = __RSArchiverGetSerializeCallBack(__RSArchiverGetCallBacks((archiver), ID))(archiver, object);
    return data;
}

RSInline RSDataRef __RSArchiverContextSerialize(const RSArchiverRef archiver, RSTypeRef object)
{
    return __RSArchiverContextSerializeEx(archiver, object, YES);
}

RSInline BOOL __RSArchiverContextSerializeObjectForKey(const RSArchiverRef archiver, RSStringRef key, RSTypeRef object)
{
    if (!key || !object) return NO;
    __RSArchiverSetCurrentKey(archiver, key);
    RSDataRef data = __RSArchiverContextSerialize(archiver, object);
    if (data)
    {
        RSDictionarySetValue(__RSArchiverContextGetDataMap(__RSArchiverGetContext(archiver)), key, data);
        RSRelease(data);
        return YES;
    }
    return NO;
}

#pragma mark -
#pragma mark RSArchiver

struct __RSArchiver {
    RSRuntimeBase _base;
    RSMutableDictionaryRef _cache;      // class-name : isa (get callbacks from register table and add to cache)
    RSStringRef _name;
    RSStringRef _ck; // __weak
    RSDataRef (*arch_copy_ptr)(RSArchiverRef archiver);
    RSDataRef (*unarch_copy_ptr)(RSDataRef);
    __RSArchiverContext _context;
};

RSInline void __RSArchiverSetSerializeMode(RSArchiverRef archiver, BOOL serialized)
{
    __RSBitfieldSetValue(RSRuntimeClassBaseFiled(archiver), 2, 1, serialized);
}

RSInline BOOL __RSArchiverGetSerializeMode(RSArchiverRef archiver)
{
    return __RSBitfieldGetValue(RSRuntimeClassBaseFiled(archiver), 2, 1);
}

RSInline void __RSArchiverSetSerializing(RSArchiverRef archiver, BOOL serializing)
{
    __RSBitfieldSetValue(RSRuntimeClassBaseFiled(archiver), 3, 2, serializing);
}

RSInline BOOL __RSArchiverIsSerializing(RSArchiverRef archiver)
{
    return __RSBitfieldGetValue(RSRuntimeClassBaseFiled(archiver), 3, 2);
}

RSInline void __RSArchiverSetCallBacks(const RSArchiverRef archiver, RSTypeID ID, RSTypeRef isa)
{
    RSDictionarySetValue(archiver->_cache, __RSRuntimeGetClassNameWithTypeID(ID), isa);
}

static const RSArchiverCallBacks *__RSArchiverGetCallBacks(const RSArchiverRef archiver, RSTypeID ID) {
    const RSArchiverCallBacks *cb = ___RSISAPayloadGetISA(RSDictionaryGetValue(archiver->_cache, __RSRuntimeGetClassNameWithTypeID(ID)));
    if (cb) return cb;
    RSTypeRef isa = __RSArchiverGetCallBacksISAForID(ID);    // need spin lock
    if (isa)
    {
        __RSArchiverSetCallBacks(archiver, ID, isa);
        cb = ___RSISAPayloadGetISA(isa);
    }
    return cb;
}

static void __RSArchiverClassInit(RSTypeRef rs)
{
    RSArchiverRef archiver = (RSArchiverRef)rs;
    archiver->_name = RSSTR("RSArchiver");
    archiver->_cache = RSDictionaryCreateMutable(RSAllocatorSystemDefault, 0, __RSStringConstantTableContext);
}

static RSTypeRef __RSArchiverClassCopy(RSAllocatorRef allocator, RSTypeRef rs, BOOL mutableCopy)
{
    return RSRetain(rs);
}

static void __RSArchiverClassDeallocate(RSTypeRef rs)
{
    RSArchiverRef archiver = (RSArchiverRef)rs;
    RSRelease(archiver->_name);
    RSRelease(archiver->_cache);
    __RSArchiverContextRelease(&archiver->_context);
}

static BOOL __RSArchiverClassEqual(RSTypeRef rs1, RSTypeRef rs2)
{
    RSArchiverRef RSArchiver1 = (RSArchiverRef)rs1;
    RSArchiverRef RSArchiver2 = (RSArchiverRef)rs2;
    BOOL result = RSEqual(RSArchiver1->_name, RSArchiver2->_name);
    return result;
}

static RSStringRef __RSArchiverClassDescription(RSTypeRef rs)
{
    RSArchiverRef archiver = (RSArchiverRef)rs;
    RSStringRef description = RSStringCreateWithFormat(RSAllocatorDefault, RSSTR("RSArchiver (%r)<%p>\n%r\n%r\n"), archiver->_name, archiver, __RSArchiverContextGetDataMap(__RSArchiverGetContext(archiver)), archiver->_context._archiveOrder);
    return description;
}

static RSRuntimeClass __RSArchiverClass =
{
    _RSRuntimeScannedObject,
    0,
    "RSArchiver",
    __RSArchiverClassInit,
    __RSArchiverClassCopy,
    __RSArchiverClassDeallocate,
    __RSArchiverClassEqual,
    nil,
    __RSArchiverClassDescription,
    nil,
    nil
};

static RSTypeID _RSArchiverTypeID = _RSRuntimeNotATypeID;

RSExport RSTypeID RSArchiverGetTypeID() {
    return _RSArchiverTypeID;
}

#pragma mark -
#pragma mark RSUnarchiver

struct __RSUnarchiver {
    RSRuntimeBase _base;
    RSMutableDictionaryRef _cache;      // class-name : isa (get callbacks from register table and add to cache)
    RSStringRef _name;
    RSStringRef _ck; // __weak
    RSDataRef (*arch_copy_ptr)(RSArchiverRef archiver);
    RSDataRef (*unarch_copy_ptr)(RSDataRef);
    __RSUnarchiverContext _context;
};

RSInline void __RSUnarchiverSetSerializeMode(RSUnarchiverRef unarchiver, BOOL serialized)
{
    __RSBitfieldSetValue(RSRuntimeClassBaseFiled(unarchiver), 2, 1, serialized);
}

RSInline BOOL __RSUnarchiverGetSerializeMode(RSUnarchiverRef unarchiver)
{
    return __RSBitfieldGetValue(RSRuntimeClassBaseFiled(unarchiver), 2, 1);
}

RSInline void __RSUnarchiverSetSerializing(RSUnarchiverRef unarchiver, BOOL serializing)
{
    __RSBitfieldSetValue(RSRuntimeClassBaseFiled(unarchiver), 3, 2, serializing);
}

RSInline BOOL __RSUnarchiverIsSerializing(RSUnarchiverRef unarchiver)
{
    return __RSBitfieldGetValue(RSRuntimeClassBaseFiled(unarchiver), 3, 2);
}

RSInline void __RSUnarchiverSetCallBacks(const RSUnarchiverRef unarchiver, RSTypeID ID, RSTypeRef isa)
{
    RSDictionarySetValue(unarchiver->_cache, __RSRuntimeGetClassNameWithTypeID(ID), isa);
}

static const RSArchiverCallBacks *__RSUnarchiverGetCallBacks(const RSUnarchiverRef unarchiver, RSTypeID ID)
{
    const RSArchiverCallBacks *cb = ___RSISAPayloadGetISA(RSDictionaryGetValue(unarchiver->_cache, __RSRuntimeGetClassNameWithTypeID(ID)));
    if (cb) return cb;
    RSTypeRef isa = __RSArchiverGetCallBacksISAForID(ID);    // need spin lock
    if (isa)
    {
        __RSUnarchiverSetCallBacks(unarchiver, ID, isa);
        cb = ___RSISAPayloadGetISA(isa);
    }
    return cb;
}

RSInline RSTypeRef __RSUnarchiverContextDeserializeObjectEx(const RSUnarchiverRef archiver, RSDataRef data, RSTypeID typeID) {
    if (!data) return nil;
    RSTypeID ID = typeID;
    return __RSArchiverGetDeserializeCallBack(__RSUnarchiverGetCallBacks((archiver), ID))(archiver, data);
}


RSInline const __RSUnarchiverContext *__RSUnarchiverGetContext(RSUnarchiverRef unarchiver) {
    return &unarchiver->_context;
}
// equal to RSUnarchiverDecodeObject
RSInline RSTypeRef __RSUnarchiverContextDeserializeObject(const RSUnarchiverRef unarchiver, RSDataRef data) {
    if (!data) return nil;
    RSTypeID ID = 0;
    if ((ID = __RSArchiverContextPopTypeID(__RSUnarchiverGetContext(unarchiver))))
        return __RSUnarchiverContextDeserializeObjectEx(unarchiver, data, ID);
    return nil;
}


static void __RSUnarchiverClassInit(RSTypeRef rs)
{
    RSUnarchiverRef unarchiver = (RSUnarchiverRef)rs;
    unarchiver->_name = RSSTR("RSUnarchiver");
    unarchiver->_cache = RSDictionaryCreateMutable(RSAllocatorSystemDefault, 0, __RSStringConstantTableContext);
}

static RSTypeRef __RSUnarchiverClassCopy(RSAllocatorRef allocator, RSTypeRef rs, BOOL mutableCopy)
{
    return RSRetain(rs);
}

static void __RSUnarchiverClassDeallocate(RSTypeRef rs)
{
    RSArchiverRef archiver = (RSArchiverRef)rs;
    RSRelease(archiver->_name);
    RSRelease(archiver->_cache);
    __RSArchiverContextRelease(&archiver->_context);
}

static BOOL __RSUnarchiverClassEqual(RSTypeRef rs1, RSTypeRef rs2)
{
    RSArchiverRef RSArchiver1 = (RSArchiverRef)rs1;
    RSArchiverRef RSArchiver2 = (RSArchiverRef)rs2;
    BOOL result = RSEqual(RSArchiver1->_name, RSArchiver2->_name);
    return result;
}

static RSStringRef __RSUnarchiverClassDescription(RSTypeRef rs)
{
    RSArchiverRef archiver = (RSArchiverRef)rs;
    RSStringRef description = RSStringCreateWithFormat(RSAllocatorDefault, RSSTR("RSArchiver (%r)<%p>\n%r\n%r\n"), archiver->_name, archiver, __RSArchiverContextGetDataMap(__RSArchiverGetContext(archiver)), archiver->_context._archiveOrder);
    return description;
}

static RSRuntimeClass __RSUnarchiverClass =
{
    _RSRuntimeScannedObject,
    0,
    "RSUnarchiver",
    __RSUnarchiverClassInit,
    __RSUnarchiverClassCopy,
    __RSUnarchiverClassDeallocate,
    __RSUnarchiverClassEqual,
    nil,
    __RSUnarchiverClassDescription,
    nil,
    nil
};

static RSTypeID _RSUnarchiverTypeID = _RSRuntimeNotATypeID;

RSExport RSTypeID RSUnarchiverGetTypeID() {
    return _RSUnarchiverTypeID;
}


RSExport RSDataRef RSArchiverContextMakeDataPacket(const RSArchiverRef archiver, RSIndex count, RSDataRef data1, ...)
{
    if (archiver == nil || count == 0 || data1 == nil) return nil;
    __RSGenericValidInstance(archiver, _RSArchiverTypeID);
    RSDataRef data = nil;
    va_list ap;
    va_start(ap, data1);
    RSTypeRef _data = data1;
    RSMutableArrayRef array = RSArrayCreateMutable(RSAllocatorSystemDefault, count);
    for (RSUInteger idx = 0; idx < count; idx++)
    {
        if (_data)
        {
            RSArrayAddObject(array, _data);
            _data = va_arg(ap, RSTypeRef);
        }
        else
        {
            __RSCLog(RSLogLevelCritical, "Count make error");
        }
    }
    
    if (RSArrayGetCount(array))
    {
        data = __RSArchiverContextSerializeEx(archiver, array, NO);
    }
    RSRelease(array);
    
    va_end(ap);
    return data;
}

RSExport RSArrayRef RSArchiverContextUnarchivePacket(const RSUnarchiverRef unarchiver, RSDataRef data, RSTypeID ID)
{
    if (unarchiver == nil || data == nil) return nil;
    __RSGenericValidInstance(unarchiver, _RSArchiverTypeID);
    return __RSUnarchiverContextDeserializeObjectEx(unarchiver, data, ID);
}

#pragma mark -
#pragma mark RSArchiver Built-in rouitne initializer
static void __RSArchiverInitialize_Allocator();
static void __RSArchiverInitialize_Array();
static void __RSArchiverInitialize_Calendar();
static void __RSArchiverInitialize_Data();
static void __RSArchiverInitialize_Date();
static void __RSArchiverInitialize_Dictionary();
static void __RSArchiverInitialize_Error();
static void __RSArchiverInitialize_Exception();
static void __RSArchiverInitialize_Nil();
static void __RSArchiverInitialize_Notification();
static void __RSArchiverInitialize_Number();
static void __RSArchiverInitialize_Plist();
static void __RSArchiverInitialize_Queue();
static void __RSArchiverInitialize_Set();
static void __RSArchiverInitialize_String();
static void __RSArchiverInitialize_Timezone();
static void __RSArchiverInitialize_UUID();

RSPrivate void __RSArchiverInitialize()
{
    _RSArchiverTypeID = __RSRuntimeRegisterClass(&__RSArchiverClass);
    __RSRuntimeSetClassTypeID(&__RSArchiverClass, _RSArchiverTypeID);
    _RSUnarchiverTypeID = __RSRuntimeRegisterClass(&__RSUnarchiverClass);
    __RSRuntimeSetClassTypeID(&__RSUnarchiverClass, _RSUnarchiverTypeID);
    
    __RSArchiverCallBacksInitialize();
    __RSArchiverInitialize_Allocator();
    __RSArchiverInitialize_Array();
    __RSArchiverInitialize_Calendar();
    __RSArchiverInitialize_Data();
    __RSArchiverInitialize_Date();
    __RSArchiverInitialize_Dictionary();
    __RSArchiverInitialize_Error();
    __RSArchiverInitialize_Exception();
    __RSArchiverInitialize_Nil();
    __RSArchiverInitialize_Notification();
    __RSArchiverInitialize_Number();
    __RSArchiverInitialize_Plist();
    __RSArchiverInitialize_Queue();
    __RSArchiverInitialize_Set();
    __RSArchiverInitialize_String();
    __RSArchiverInitialize_Timezone();
    __RSArchiverInitialize_UUID();
}

RSPrivate void __RSArchiverDeallocate()
{
    RSSpinLockLock(&_RSArchiverCallbacksLock);
    if (_RSArchiverCallbacks)
    {
        __RSRuntimeSetInstanceSpecial(_RSArchiverCallbacks, NO);
        RSRelease(_RSArchiverCallbacks);
        _RSArchiverCallbacks = nil;
    }
    RSSpinLockUnlock(&_RSArchiverCallbacksLock);
}
static RSDataRef __RSArchiverNormalCopyDataPtr(RSArchiverRef archiver);
static RSDataRef __RSArchiverKeyedCopyDataPtr(RSArchiverRef archiver);
static RSDataRef __RSArchiverKeyedCopyEncodeRoutine(RSDataRef data);
static RSDataRef __RSUnarchiverKeyedCopyDecodeRoutine(RSDataRef data);

static RSArchiverRef __RSArchiverCreateInstance(RSAllocatorRef allocator)
{
    RSArchiverRef instance = (RSArchiverRef)__RSRuntimeCreateInstance(allocator, _RSArchiverTypeID, sizeof(struct __RSArchiver) - sizeof(RSRuntimeBase));
    if (!__RSArchiverContextInit(&instance->_context))
        __RSCLog(RSLogLevelError, "archiver can not be init");
    instance->arch_copy_ptr = __RSArchiverKeyedCopyDataPtr;
    instance->unarch_copy_ptr = __RSUnarchiverKeyedCopyDecodeRoutine;
    
    instance->arch_copy_ptr = __RSArchiverNormalCopyDataPtr;
    instance->unarch_copy_ptr = __RSUnarchiverKeyedCopyDecodeRoutine;
    
    return instance;
}

static BOOL __RSUnarchiverRestoreContext(RSUnarchiverRef, RSDataRef);

static RSUnarchiverRef __RSUnarchiverCreateInstance(RSAllocatorRef allocator, RSDataRef data)
{
    RSUnarchiverRef instance = (RSUnarchiverRef)__RSRuntimeCreateInstance(allocator, _RSUnarchiverTypeID, sizeof(struct __RSUnarchiver) - sizeof(RSRuntimeBase));
    instance->_name = RSSTR("RSUnarchiver");
    if (!__RSUnarchiverRestoreContext(instance, data)) {
        RSExceptionCreateAndRaise(allocator, RSInvalidArgumentException, RSSTR("data is invalid archiver data"), nil);
        RSRelease(instance);
        instance = nil;
    }
    return instance;
}

RSInline const __RSArchiverContext *__RSArchiverGetContext(RSArchiverRef archiver)
{
    return &archiver->_context;
}

RSInline void __RSArchiverSetCurrentKey(RSArchiverRef archiver, RSStringRef key)
{
    archiver->_ck = key;
}

RSInline RSStringRef __RSArchiverGetCurrentKey(const RSArchiverRef archiver)
{
    return archiver->_ck;
}

RSExport RSArchiverRef RSArchiverCreate(RSAllocatorRef allocator)
{
    return __RSArchiverCreateInstance(allocator);
}

RSExport RSDataRef RSArchiverEncodeObject(RSArchiverRef archiver, RSTypeRef object)
{
    if (!archiver || !object) return nil;
    __RSGenericValidInstance(archiver, _RSArchiverTypeID);
    RSDataRef data = __RSArchiverContextSerialize(archiver, object);
    return data;
}

RSExport RSTypeRef RSUnarchiverDecodeObject(RSUnarchiverRef unarchiver, RSDataRef data) {
    if (!unarchiver || !data) return nil;
    __RSGenericValidInstance(unarchiver, _RSUnarchiverTypeID);
    RSTypeRef object = __RSUnarchiverContextDeserializeObject(unarchiver, data);
    return object;
}

RSExport BOOL RSArchiverEncodeObjectForKey(RSArchiverRef archiver, RSStringRef key, RSTypeRef object)
{
    if (!archiver || !key) return NO;
    __RSGenericValidInstance(archiver, _RSArchiverTypeID);
    return __RSArchiverContextSerializeObjectForKey(archiver, key, object);
}

RSExport RSStringRef RSArchiverGetCurrentKey(RSArchiverRef archiver)
{
    __RSGenericValidInstance(archiver, _RSArchiverTypeID);
    return __RSArchiverGetCurrentKey(archiver);
}

RSExport RSTypeRef RSUnarchiverDecodeObjectForKey(RSUnarchiverRef unarchiver, RSStringRef key)
{
    __RSGenericValidInstance(unarchiver, _RSUnarchiverTypeID);
    return __RSUnarchiverContextDeserializeObject(unarchiver, RSDictionaryGetValue(__RSArchiverContextGetDataMap(__RSUnarchiverGetContext(unarchiver)), key));
}

static RSDataRef __RSArchiverNormalCopyDataPtr(RSArchiverRef archiver)
{
    RSMutableDictionaryRef dict = RSDictionaryCreateMutable(RSAllocatorSystemDefault, 0, RSDictionaryRSTypeContext);
    RSArrayRef classNames = __RSUnarchiverCopyClassNamesFromClassTypeIDs(__RSArchiverContextGetOrder(__RSArchiverGetContext(archiver)));
    RSDictionarySetValue(dict, __RSArchiverContextOrder, classNames);
    RSRelease(classNames);
    RSDictionarySetValue(dict, __RSArchiverContextData, __RSArchiverContextGetDataMap(__RSArchiverGetContext(archiver)));
    RSDataRef data = RSPropertyListCreateXMLData(RSAllocatorSystemDefault, dict);
    RSRelease(dict);
    return data;
}

static RSDataRef __RSArchiverKeyedCopyDataPtr(RSArchiverRef archiver)
{
    RSMutableDictionaryRef dict = RSDictionaryCreateMutable(RSAllocatorSystemDefault, 0, RSDictionaryRSTypeContext);
    RSArrayRef classNames = __RSUnarchiverCopyClassNamesFromClassTypeIDs(__RSArchiverContextGetOrder(__RSArchiverGetContext(archiver)));
    RSDictionarySetValue(dict, __RSArchiverContextOrder, classNames);
    RSRelease(classNames);
    RSDictionarySetValue(dict, __RSArchiverContextData, __RSArchiverContextGetDataMap(__RSArchiverGetContext(archiver)));
    RSDataRef data = RSBinaryPropertyListCreateXMLData(RSAllocatorSystemDefault, dict);
    RSRelease(dict);
    return data;
}

static RSDataRef __RSArchiverKeyedCopyEncodeRoutine(RSDataRef data)
{
    return RSRetain(data);
}

static RSDataRef __RSUnarchiverKeyedCopyDecodeRoutine(RSDataRef data)
{
    return RSRetain(data);
}

static RSDataRef __RSArchiverKeyedEncodedCopyDataPtr(RSArchiverRef archiver)
{
    RSMutableDictionaryRef dict = RSDictionaryCreateMutable(RSAllocatorSystemDefault, 0, RSDictionaryRSTypeContext);
    RSDictionaryRef dataMap = __RSArchiverContextGetDataMap(__RSArchiverGetContext(archiver));
    RSErrorRef error = nil;
    RSDataRef mapdata = __RSPropertyListCreateWithError(dataMap, &error);
    RSDataRef data = nil;
    if (mapdata && !error)
    {
        RSDataRef encodedData = __RSArchiverKeyedCopyEncodeRoutine(mapdata);
        RSRelease(mapdata);
        
        RSDictionarySetValue(dict, __RSArchiverContextEncoded, RSBooleanTrue);
        RSDictionarySetValue(dict, __RSArchiverContextData, encodedData);
        RSRelease(encodedData);
        RSDictionarySetValue(dict, __RSArchiverContextOrder, __RSArchiverContextGetOrder(__RSArchiverGetContext(archiver)));
        
        data = RSBinaryPropertyListCreateXMLData(RSAllocatorSystemDefault, dict);
    }
    RSRelease(dict);
    return data;
}

static BOOL __RSUnarchiverRestoreContext(RSUnarchiverRef unarchiver, RSDataRef data)
{
    BOOL result = NO;
    bzero(&unarchiver->_context, sizeof(__RSArchiverContext));
    RSDictionaryRef raw = RSDictionaryCreateWithData(RSAllocatorSystemDefault, data);
    RSTypeRef encoded = RSDictionaryGetValue(raw, __RSArchiverContextEncoded);
    RSTypeRef contextData = RSDictionaryGetValue(raw, __RSArchiverContextData);
    unarchiver->_context._archiveOrder = (RSMutableArrayRef)__RSUnarchiverCopyClassTypeIDsFromClassNames(RSDictionaryGetValue(raw, __RSArchiverContextOrder));
    
    if (encoded && encoded == RSBooleanTrue && RSGetTypeID(contextData) == RSDataGetTypeID())
    {
        // decode (depend on __RSArchiverKeyedEncodedCopyDataPtr)
        RSDataRef decoded = __RSUnarchiverKeyedCopyDecodeRoutine(encoded);
        RSRelease(encoded);
        unarchiver->_context._archiveData = (RSMutableDictionaryRef)RSDictionaryCreateWithData(RSAllocatorSystemDefault, decoded);
        result = YES;
    }
    else
    {
        unarchiver->_context._archiveData = (RSMutableDictionaryRef)RSRetain(contextData);
        result = YES;
    }
    RSRelease(raw);
    return result;
}

RSExport RSDataRef RSArchiverCopyData(RSArchiverRef archiver)
{
    __RSGenericValidInstance(archiver, _RSArchiverTypeID);
    return archiver->arch_copy_ptr(archiver);
}

#pragma mark -
#pragma mark RSUnarchiver Built-in Unarchive Routine

RSExport RSUnarchiverRef RSUnarchiverCreate(RSAllocatorRef allocator, RSDataRef data)
{
    RSUnarchiverRef unarchiver = nil;
    unarchiver = __RSUnarchiverCreateInstance(allocator, data);
    return unarchiver;
}

RSExport RSUnarchiverRef RSUnarchiverCreateWithContentOfPath(RSAllocatorRef allocator, RSStringRef path) {
    return RSUnarchiverCreate(allocator, RSDataWithContentOfPath(path));
}

RSExport RSTypeRef RSUnarchiverObjectWithContentOfPath(RSStringRef path) {
    return RSAutorelease(RSUnarchiverCreateWithContentOfPath(RSAllocatorSystemDefault, path));
}

#pragma mark -
#pragma mark RSArchiver Built-in Archive Routine

RSInline const RSArchiverCallBacks* __RSArchiverMakeCallBack(RSArchiverCallBacks *cb, RSTypeID ID, RSArchiverSerializeCallBack scb, RSArchiverDeserializeCallBack dscb)
{
    cb->version = 0;
    cb->classID = ID;
    cb->serialize = scb;
    cb->deserialize = dscb;
    return cb;
}

static RSDataRef __RSAllocatorSerializeCallBack(RSArchiverRef archiver, RSTypeRef object)
{
    if (RSAllocatorGetTypeID() == RSGetTypeID(object))
    {
        RSBitU8 buf[2];
        RSBitU8 *bytes = buf;
        RSIndex size = sizeof(buf);
        
        if (object == RSAllocatorDefault)
        {
            bytes = (RSBitU8 *)0x03ab;
        }
        else if (object == RSAllocatorSystemDefault)
        {
            bytes = (RSBitU8 *)0x03ad;
        }
        else bytes = (RSBitU8 *)0x03af;
        
        RSDataRef data = RSDataCreate(object, bytes, size);
        return data;
    }
    return nil;
}

static RSTypeRef __RSAllocatorDeserializeCallBack(RSUnarchiverRef archiver, RSDataRef data)
{
    if (data)
    {
        const RSBitU8 *bytes = RSDataGetBytesPtr(data);
        if (RSDataGetLength(data) == 2 && bytes && bytes[0] == 0x03)
        {
            switch (bytes[1]) {
                case 0xab:
                    return RSAllocatorDefault;
                case 0xad:
                    return RSAllocatorSystemDefault;
                case 0xae:
                    return nil;
                default:
                    __RSCLog(RSLogLevelCritical, "RSAllocator Unarchiver failed");
            }
        }
    }
    return nil;
}

static void __RSArchiverInitialize_Allocator()
{
    RSArchiverCallBacks cb = {0};
    __RSArchiverRegisterCallbacks(__RSArchiverMakeCallBack(&cb,
                                                           RSAllocatorGetTypeID()/*2*/,
                                                           __RSAllocatorSerializeCallBack,
                                                           __RSAllocatorDeserializeCallBack));
}

#if !RS_BLOCKS_AVAILABLE
struct __arrayArchiveContext
{
    RSArchiverRef archiver;
    void *context;
};

static void __RSArrayApplySerializeCallBack(const void *value, void *context)
{
    struct __arrayArchiveContext *ctx = (struct __arrayArchiveContext*)context;
    RSDataRef subData = __RSArchiverContextSerialize(ctx->archiver, value);
    RSArrayAddObject(ctx->context, subData);
    RSRelease(subData);
}

static void __RSArrayApplyDeserializeCallBack(const void *value, void *context)
{
    struct __arrayArchiveContext *ctx = (struct __arrayArchiveContext*)context;
    RSTypeRef object = __RSArchiverContextDeserialize(ctx->archiver, value);
    RSArrayAddObject(ctx->context, object);
    RSRelease(object);
}
#endif

static RSDataRef __RSArraySerializeCallBack(RSArchiverRef archiver, RSTypeRef object)
{
    RSMutableDataRef data = __RSPropertyListCreateWithError(object, nil);
    if (data) return data; // Plist array
    RSMutableArrayRef array = RSArrayCreateMutable(RSAllocatorSystemDefault, 0); // RSData
#if RS_BLOCKS_AVAILABLE
    RSArrayApplyBlock(object, RSMakeRange(0, RSArrayGetCount(object)), ^(const void *value, RSUInteger idx, BOOL *isStop) {
        RSDataRef subData = __RSArchiverContextSerialize(archiver, value);
        RSArrayAddObject(array, subData);
        RSRelease(subData);
    });
#else
    struct __arrayArchiveContext ctx;
    ctx.archiver = archiver;
    ctx.context = array;
    RSArrayApplyFunction(object, RSMakeRange(0, RSArrayGetCount(object)), __RSArrayApplySerializeCallBack, &ctx);
#endif
    RSDictionaryRef dict = RSDictionaryCreateWithObjectsAndOKeys(RSAllocatorSystemDefault, array, RSSTR("array"), nil);
    RSRelease(array);
    data = __RSPropertyListCreateWithError(dict, nil);
    RSRelease(dict);
    return data;
}

static RSTypeRef __RSArrayDeserializeCallBack(RSUnarchiverRef unarchiver, RSDataRef data)
{
    RSPropertyListRef plist = RSPropertyListCreateWithXMLData(RSAllocatorSystemDefault, data);
    if (!plist) return nil; // should I halt the Foundation?
    RSArrayRef array = RSPropertyListGetContent(plist);
    RSMutableArrayRef retArray = (RSMutableArrayRef)array;
    RSDictionaryRef dict = (RSDictionaryRef)array;
    RSTypeID ID = RSGetTypeID(array);
    if (RSArrayGetTypeID() != ID && RSDictionaryGetTypeID() != ID)
    {
        RSRelease(plist);
        __RSCLog(RSLogLevelCritical, "%s %s %s", __FILE__, __func__, "data did not contain the array");
    }
    if (ID == RSDictionaryGetTypeID())
    {
        retArray = RSArrayCreateMutable(RSAllocatorSystemDefault, RSDictionaryGetCount(dict));
        array = RSDictionaryGetValue(dict, RSSTR("array"));
#if RS_BLOCKS_AVAILABLE
        RSArrayApplyBlock(array, RSMakeRange(0, RSArrayGetCount(array)), ^(const void *value, RSUInteger idx, BOOL *isStop) {
            RSTypeRef object = __RSUnarchiverContextDeserializeObject(unarchiver, value);
            RSArrayAddObject(retArray, object);
            RSRelease(object);
        });
#else
        struct __arrayArchiveContext ctx;
        ctx.archiver = archiver;
        ctx.context = retArray;
        RSArrayApplyFunction(array, RSMakeRange(0, RSArrayGetCount(array)), __RSArrayApplyDeserializeCallBack, &ctx);
#endif
    }
    else if (ID == RSArrayGetTypeID())
    {
        RSRetain(retArray);
    }
    RSRelease(plist);
    return retArray;
}

static void __RSArchiverInitialize_Array()
{
    RSArchiverCallBacks cb = {0};
    __RSArchiverRegisterCallbacks(__RSArchiverMakeCallBack(&cb, RSArrayGetTypeID(), __RSArraySerializeCallBack, __RSArrayDeserializeCallBack));
}

static RSDataRef __RSCalendarSerializeCallBack(RSArchiverRef archiver, RSTypeRef object)
{
    RSCalendarRef calendar = (RSCalendarRef)object;
    RSStringRef identifier = RSCalendarGetIdentifier(calendar);
    RSTimeZoneRef tz = RSCalendarGetTimeZone(calendar);
    RSDataRef data1 = __RSArchiverContextSerialize(archiver, identifier);
    RSDataRef data2 = __RSArchiverContextSerialize(archiver, tz);
    if (data1 && data2)
    {
        RSMutableArrayRef array = RSArrayCreateMutable(RSAllocatorSystemDefault, 2);
        RSArrayAddObject(array, data1);
        RSArrayAddObject(array, data2);
        RSRelease(data1);
        RSRelease(data2);
        RSDataRef data = RSArchiverContextMakeDataPacket(archiver, 2, data1, data2);
        RSRelease(array);
        return data;
    }
    RSRelease(data1);
    RSRelease(data2);
    return nil;
}

static RSTypeRef __RSCalendarDeserializeCallBack(RSUnarchiverRef unarchiver, RSDataRef data)
{
//    RSArrayRef array = __RSArchiverContextDeserializeObjectEx(archiver, data, RSArrayGetTypeID());
    RSArrayRef array = RSArchiverContextUnarchivePacket(unarchiver, data, RSArrayGetTypeID());
    RSDataRef data1 = RSArrayObjectAtIndex(array, 0);
    RSDataRef data2 = RSArrayObjectAtIndex(array, 1);
    RSStringRef identifier = __RSUnarchiverContextDeserializeObject(unarchiver, data1);
    RSStringRef tz = __RSUnarchiverContextDeserializeObject(unarchiver, data2);
    RSRelease(array);
    
    RSCalendarRef calendar = RSCalendarCreateWithIndentifier(RSAllocatorSystemDefault, identifier);
    RSRelease(identifier);
    RSTimeZoneRef timezone = RSTimeZoneCreateWithName(RSAllocatorSystemDefault, tz);
    RSRelease(tz);
    RSCalendarSetTimeZone(calendar, timezone);
    RSRelease(timezone);
    
    return calendar;
}

static void __RSArchiverInitialize_Calendar()
{
    RSArchiverCallBacks cb = {0};
    __RSArchiverRegisterCallbacks(__RSArchiverMakeCallBack(&cb, RSCalendarGetTypeID(), __RSCalendarSerializeCallBack, __RSCalendarDeserializeCallBack));
}

static RSDataRef __RSDataSerializeCallBack(RSArchiverRef archiver, RSTypeRef object)
{
    return RSCopy(RSAllocatorSystemDefault, object);
}

static RSTypeRef __RSDataDeserializeCallBack(RSUnarchiverRef unarchiver, RSDataRef data)
{
    return RSRetain(data);
}

static void __RSArchiverInitialize_Data()
{
    RSArchiverCallBacks cb = {0};
    __RSArchiverRegisterCallbacks(__RSArchiverMakeCallBack(&cb, RSDataGetTypeID(), __RSDataSerializeCallBack, __RSDataDeserializeCallBack));
}

static RSDataRef __RSDateSerializeCallBack(RSArchiverRef archiver, RSTypeRef object)
{
    return __RSBPLCreateDataForObject(object);
}

static RSTypeRef __RSDateDeserializeCallBack(RSUnarchiverRef unarchiver, RSDataRef data)
{
    return __RSBPLCreateObjectWithData(data);
}

static void __RSArchiverInitialize_Date()
{
    RSArchiverCallBacks cb = {0};
    __RSArchiverRegisterCallbacks(__RSArchiverMakeCallBack(&cb, RSDateGetTypeID(), __RSDateSerializeCallBack, __RSDateDeserializeCallBack));
}

static RSDataRef __RSDictionarySerializeCallBack(RSArchiverRef archiver, RSTypeRef object)
{
    RSMutableDataRef data = __RSPropertyListCreateWithError(object, nil);
    if (data) return data; // Plist dictionary
    RSIndex count = RSDictionaryGetCount(object);
    RSTypeRef _contain[256*3] = {0};
    RSTypeRef* keyValuePairs = count <= 256 ? &_contain[0] : RSAllocatorAllocate(RSAllocatorSystemDefault, count * 3);
    RSDictionaryGetKeysAndValues(object, &keyValuePairs[0], &keyValuePairs[count]);
    for (RSUInteger idx = 0; idx < count; idx++)
    {
//        keyValuePairs[count * 2 + idx] = __RSArchiverContextSerialize(archiver, keyValuePairs[count * 0 + idx]);
        keyValuePairs[count * 2 + idx] = __RSArchiverContextSerialize(archiver, keyValuePairs[count * 1 + idx]);
    }
    
    RSDictionaryRef dict = RSDictionaryCreate(RSAllocatorSystemDefault, &keyValuePairs[count * 0], &keyValuePairs[count * 2], count, RSDictionaryRSTypeContext);
    
    for (RSUInteger idx = 0; idx < count; idx++) {
        RSRelease(keyValuePairs[count * 2 + idx]);
    }
    
    if (keyValuePairs != &_contain[0])
    {
        RSAllocatorDeallocate(RSAllocatorSystemDefault, keyValuePairs);
    }
    
    RSArrayRef array = RSArrayCreateWithObject(RSAllocatorSystemDefault, dict);
    RSRelease(dict);
    data = __RSPropertyListCreateWithError(array, nil);
    RSRelease(array);
    return data;
}

static RSTypeRef __RSDictionaryDeserializeCallBack(RSUnarchiverRef unarchiver, RSDataRef data)
{
    RSPropertyListRef plist = RSPropertyListCreateWithXMLData(RSAllocatorSystemDefault, data);
    if (!plist) return nil; // should I halt the Foundation?
    RSDictionaryRef dict = RSPropertyListGetContent(plist);
    RSMutableDictionaryRef retDict = (RSMutableDictionaryRef)dict;
    RSArrayRef array = (RSArrayRef)dict;
    RSTypeID ID = RSGetTypeID(dict);
    if (ID == RSArrayGetTypeID())
    {
        retDict = nil;
        dict = RSArrayObjectAtIndex(array, 0);
        RSIndex count = RSDictionaryGetCount(dict);
        RSTypeRef _contain[256*3] = {0};
        RSTypeRef* keyValuePairs = count <= 256 ? &_contain[0] : RSAllocatorAllocate(RSAllocatorSystemDefault, count * 3);
        RSDictionaryGetKeysAndValues(dict, &keyValuePairs[0], &keyValuePairs[count]);
        for (RSUInteger idx = 0; idx < count; idx++)
        {
            keyValuePairs[count * 2 + idx] = __RSUnarchiverContextDeserializeObject(unarchiver, keyValuePairs[count * 1 + idx]);
        }
        
        retDict = RSDictionaryCreateMutable(RSAllocatorSystemDefault, 0, RSDictionaryRSTypeContext);
        for (RSUInteger idx = 0; idx < count; idx++)
        {
            RSDictionarySetValue(retDict, keyValuePairs[0 + idx], keyValuePairs[count * 2 + idx]);
            RSRelease(keyValuePairs[count * 2 + idx]);
        }
    }
    else if (RSDictionaryGetTypeID() == ID)
    {
        // good job
        RSRetain(retDict);
    }
    else
    {
        __RSCLog(RSLogLevelCritical, "%s %s %s", __FILE__, __func__, "data did not contain the array");
    }
    RSRelease(plist);
    return retDict;
}

static void __RSArchiverInitialize_Dictionary()
{
    RSArchiverCallBacks cb = {0};
    __RSArchiverRegisterCallbacks(__RSArchiverMakeCallBack(&cb, RSDictionaryGetTypeID(), &__RSDictionarySerializeCallBack, &__RSDictionaryDeserializeCallBack));
}

static RSDataRef __RSErrorSerializeCallBack(RSArchiverRef archiver, RSTypeRef object)
{
    RSDataRef data = nil;
    RSErrorRef error = (RSErrorRef)object;
    RSStringRef domain = RSErrorGetDomain(error);
    RSNumberRef errorCode = RSNumberWithInteger(RSErrorGetCode(error));
    RSDictionaryRef userInfo = RSErrorCopyUserInfo(error);
    
    RSDataRef data1 = nil, data2 = nil, data3 = nil;
    data1 = __RSArchiverContextSerialize(archiver, domain);
    data2 = __RSArchiverContextSerialize(archiver, errorCode);
    data3 = __RSArchiverContextSerialize(archiver, userInfo);
    data = RSArchiverContextMakeDataPacket(archiver, 3, data1, data2, data3);
    RSRelease(data1); RSRelease(data2); RSRelease(data3);
    return data;
}

static RSTypeRef __RSErrorDeserializeCallBack(RSUnarchiverRef unarchiver, RSDataRef data)
{
    RSErrorRef error = nil;
    RSArrayRef array = RSArchiverContextUnarchivePacket(unarchiver, data, RSArrayGetTypeID());
    RSStringRef domain = __RSUnarchiverContextDeserializeObject(unarchiver, RSArrayObjectAtIndex(array, 0));
    RSNumberRef errorCode = __RSUnarchiverContextDeserializeObject(unarchiver, RSArrayObjectAtIndex(array, 1));
    RSDictionaryRef userInfo = __RSUnarchiverContextDeserializeObject(unarchiver, RSArrayObjectAtIndex(array, 2));
    RSRelease(array);
    RSIndex ec = 0;
    RSNumberGetValue(errorCode, &ec);
    error = RSErrorCreate(RSAllocatorSystemDefault, domain, ec, userInfo);
    RSRelease(domain);
    RSRelease(errorCode);
    RSRelease(userInfo);
    return error;
}

static void __RSArchiverInitialize_Error()
{
    RSArchiverCallBacks cb = {0};
    __RSArchiverRegisterCallbacks(__RSArchiverMakeCallBack(&cb, RSErrorGetTypeID(), __RSErrorSerializeCallBack, __RSErrorDeserializeCallBack));
}

static void __RSArchiverInitialize_Exception()
{
    RSArchiverCallBacks cb = {0};
    __RSArchiverRegisterCallbacks(&cb);
}

static RSDataRef __RSNilSerializeCallBack(RSArchiverRef archiver, RSTypeRef object)
{
    RSBitU8 null_byte = 0x00;
    return RSDataCreate(RSAllocatorSystemDefault, &null_byte, sizeof(null_byte));
}

static RSTypeRef __RSNilDeserializeCallBack(RSUnarchiverRef unarchiver, RSDataRef data)
{
    return nil; // RSNull
}

static void __RSArchiverInitialize_Nil()
{
    RSArchiverCallBacks cb = {0};
    __RSArchiverRegisterCallbacks(__RSArchiverMakeCallBack(&cb, RSNilGetTypeID(), __RSNilSerializeCallBack, __RSNilDeserializeCallBack));
}

static RSDataRef __RSNotificationSerializeCallBack(RSArchiverRef archiver, RSTypeRef object)
{
    RSNotificationRef notification = (RSNotificationRef)object;
    RSStringRef name = RSNotificationGetName(notification);
    RSTypeRef filter = RSNotificationGetObject(notification) ? : (RSTypeRef)RSNull;
    RSDictionaryRef userInfo = RSNotificationGetUserInfo(notification) ? : (RSDictionaryRef)RSNull;
    
    RSDataRef data1 = __RSArchiverContextSerialize(archiver, name);
    RSDataRef data2 = __RSArchiverContextSerialize(archiver, filter);
    RSDataRef data3 = __RSArchiverContextSerialize(archiver, userInfo);
    
    RSDataRef data = RSArchiverContextMakeDataPacket(archiver, 3, data1, data2, data3);
    
    RSRelease(data1);
    RSRelease(data2);
    RSRelease(data3);
    return data;
}

static RSTypeRef __RSNotificationDeserializeCallBack(RSUnarchiverRef unarchiver, RSDataRef data)
{
    RSArrayRef array = RSArchiverContextUnarchivePacket(unarchiver, data, RSArrayGetTypeID());
    RSStringRef name = __RSUnarchiverContextDeserializeObject(unarchiver, RSArrayObjectAtIndex(array, 0));
    RSTypeRef filter = __RSUnarchiverContextDeserializeObject(unarchiver, RSArrayObjectAtIndex(array, 1));
    RSDictionaryRef userInfo = __RSUnarchiverContextDeserializeObject(unarchiver, RSArrayObjectAtIndex(array, 2));
    RSRelease(array);
    
    RSNotificationRef notificaiton = RSNotificationCreate(RSAllocatorSystemDefault, name, filter, userInfo);
    
    RSRelease(name);
    RSRelease(filter);
    RSRelease(userInfo);
    return notificaiton;
}

static void __RSArchiverInitialize_Notification()
{
    RSArchiverCallBacks cb = {0};
    __RSArchiverRegisterCallbacks(__RSArchiverMakeCallBack(&cb, RSNotificationGetTypeID(), __RSNotificationSerializeCallBack, __RSNotificationDeserializeCallBack));
}

static RSDataRef __RSNumberSerializeCallBack(RSArchiverRef archiver, RSTypeRef object)
{
    return __RSBPLCreateDataForObject(object);
}

static RSTypeRef __RSNumberDeserializeCallBack(RSUnarchiverRef unarchiver, RSDataRef data)
{
    return __RSBPLCreateObjectWithData(data);
}
static void __RSArchiverInitialize_Number()
{
    RSArchiverCallBacks cb = {0};
    __RSArchiverRegisterCallbacks(__RSArchiverMakeCallBack(&cb, RSNumberGetTypeID(), __RSNumberSerializeCallBack, __RSNumberDeserializeCallBack));
}

static RSDataRef __RSPlistSerializeCallBack(RSArchiverRef archiver, RSTypeRef object)
{
    RSTypeRef content = RSPropertyListGetContent((RSPropertyListRef)object);
    RSDataRef data = __RSArchiverContextSerialize(archiver, content);
    return data;
}

static RSTypeRef __RSPlistDeserializeCallBack(RSUnarchiverRef unarchiver, RSDataRef data)
{
    RSTypeRef content = __RSUnarchiverContextDeserializeObject(unarchiver, data);
    RSPropertyListRef plist = RSPropertyListCreateWithContent(RSAllocatorSystemDefault, content);
    RSRelease(content);
    return plist;
}

static void __RSArchiverInitialize_Plist()
{
    RSArchiverCallBacks cb = {0};
    __RSArchiverRegisterCallbacks(__RSArchiverMakeCallBack(&cb, RSPropertyListGetTypeID(), __RSPlistSerializeCallBack, __RSPlistDeserializeCallBack));
}

//static RSDataRef __RS SerializeCallBack(RSArchiverRef archiver, RSTypeRef object)
//static RSTypeRef __RS DeserializeCallBack(RSUnarchiverRef unarchiver, RSDataRef data)

static void __RSArchiverInitialize_Queue()
{
    RSArchiverCallBacks cb = {0};
    __RSArchiverRegisterCallbacks(&cb);
}

//static RSDataRef __RS SerializeCallBack(RSArchiverRef archiver, RSTypeRef object)
//static RSTypeRef __RS DeserializeCallBack(RSUnarchiverRef unarchiver, RSDataRef data)

static void __RSArchiverInitialize_Set()
{
    RSArchiverCallBacks cb = {0};
    __RSArchiverRegisterCallbacks(&cb);
}

static RSDataRef __RSStringSerializeCallBack(RSArchiverRef archiver, RSTypeRef object)
{
    return __RSBPLCreateDataForObject(object);
//    RSDataRef data = RSStringCreateExternalRepresentation(RSAllocatorSystemDefault, object, RSStringEncodingUTF8, 0);
//    return data;
}

static RSTypeRef __RSStringDeserializeCallBack(RSUnarchiverRef archiver, RSDataRef data)
{
    return __RSBPLCreateObjectWithData(data);
//    RSStringRef string = RSStringCreateWithData(RSAllocatorSystemDefault, data, RSStringEncodingUTF8);
//    return string;
}

static void __RSArchiverInitialize_String()
{
    RSArchiverCallBacks cb = {0};
#if DEBUG
    RSAssert(RSStringGetTypeID() == 7, 3, "TypeID of RSString should be 7");
#endif
    __RSArchiverRegisterCallbacks(__RSArchiverMakeCallBack(&cb, 7, __RSStringSerializeCallBack, __RSStringDeserializeCallBack));
}

static RSDataRef __RSTimeZoneSerializeCallBack(RSArchiverRef archiver, RSTypeRef object)
{
    RSTimeZoneRef tz = (RSTimeZoneRef)object;
    RSStringRef id = RSTimeZoneGetName(tz);
    RSDataRef data = __RSArchiverContextSerialize(archiver, id);
    return data;
}

static RSTypeRef __RSTimeZoneDeserializeCallBack(RSUnarchiverRef archiver, RSDataRef data)
{
    RSStringRef id = __RSUnarchiverContextDeserializeObject(archiver, data);
    RSTimeZoneRef tz = RSTimeZoneCreateWithName(RSAllocatorSystemDefault, id);
    RSRelease(id);
    return tz;
}

static void __RSArchiverInitialize_Timezone()
{
    RSArchiverCallBacks cb = {0};
    __RSArchiverRegisterCallbacks(__RSArchiverMakeCallBack(&cb, RSTimeZoneGetTypeID(), __RSTimeZoneSerializeCallBack, __RSTimeZoneDeserializeCallBack));
}

static RSDataRef __RSUUIDSerializeCallBack(RSArchiverRef archiver, RSTypeRef object)
{
    return __RSBPLCreateDataForObject(object);
}

static RSTypeRef __RSUUIDDeserializeCallBack(RSUnarchiverRef archiver, RSDataRef data)
{
    return __RSBPLCreateObjectWithData(data);
}

static void __RSArchiverInitialize_UUID()
{
    RSArchiverCallBacks cb = {0};
    __RSArchiverRegisterCallbacks(__RSArchiverMakeCallBack(&cb, RSUUIDGetTypeID(), __RSUUIDSerializeCallBack, __RSUUIDDeserializeCallBack));
}

#if DEBUG

#pragma mark -
#pragma mark RSArchiver test routine
RSExport void __RSArchiverTestMain()
{
    return;
//    RSArchiverRef archiverEX = RSArchiverCreate(RSAllocatorSystemDefault);
//    RSMutableArrayRef array = RSArrayCreateMutable(RSAllocatorSystemDefault, 0);
//    RSArrayAddObject(array, RSTimeZoneCopySystem());
//    RSMutableDictionaryRef dict = RSDictionaryCreateMutable(RSAllocatorSystemDefault, 0, RSDictionaryRSTypeContext);
//    
//    RSDictionarySetValue(dict, RSSTR("k1"), RSNumberWithInt(1));
//    RSDictionarySetValue(dict, RSSTR("k3"), RSSTR("v3"));
//    RSDictionarySetValue(dict, RSSTR("k2"), RSSTR("v2"));
//    RSDictionarySetValue(dict, RSSTR("k4"), RSSTR("v4"));
//    RSDictionarySetValue(dict, RSSTR("k5"), RSTimeZoneCopySystem());
//    RSDictionarySetValue(dict, RSSTR("k6"), array);
//    RSDictionarySetValue(dict, RSSTR("k7"), RSDateGetCurrent(RSAllocatorSystemDefault));
//    RSDictionarySetValue(dict, RSSTR("k8"), RSErrorWithDomainCodeAndUserInfo(RSErrorDomainRSCoreFoundation, kErrVerify, nil));
//    RSRelease(array);
//    
//    RSArchiverEncodeObjectForKey(archiverEX, RSSTR("dict"), dict);
//    RSDataWriteToFile(RSAutorelease(RSArchiverCopyData(archiverEX)), RSFileManagerStandardizingPath(RSSTR("~/Desktop/dict.plist")), RSWriteFileAutomatically);
//    RSRelease(archiverEX);
//    
//    RSUnarchiverRef unarchiver = RSUnarchiverCreate(RSAllocatorSystemDefault, RSDataWithContentOfPath(RSFileManagerStandardizingPath(RSSTR("~/Desktop/dict.plist"))));
//    dict = (RSMutableDictionaryRef)RSUnarchiverDecodeObjectForKey(unarchiver, RSSTR("dict"));
//    RSShow(dict);
//    RSRelease(dict);
//    RSRelease(unarchiver);
//    return;
//    RSArchiverRef archiver = RSArchiverCreate(RSAllocatorSystemDefault);
//    RSArchiverEncodeObjectForKey(archiver, RSSTR("date"), RSDateGetCurrent(RSAllocatorSystemDefault));
//    RSArchiverEncodeObjectForKey(archiver, RSSTR("test"), RSSTR("value"));
//    
//    RSTimeZoneRef tz = RSTimeZoneCopySystem();
//    RSArchiverEncodeObjectForKey(archiver, RSSTR("tz"), tz);
//    RSRelease(tz);
//    
//    RSDataRef data = RSDataCreateWithString(RSAllocatorSystemDefault, RSSTR("test-data"), RSStringEncodingUTF8);
//    RSArchiverEncodeObjectForKey(archiver, RSSTR("data"), data);
//    RSRelease(data);
//    
//    RSCalendarRef calendar = RSCalendarCreateWithIndentifier(RSAllocatorSystemDefault, RSGregorianCalendar);
//    RSArchiverEncodeObjectForKey(archiver, RSSTR("cal"), calendar);
//    RSRelease(calendar);
//    
//    RSShow((archiver));
//    RSDataWriteToFile(RSAutorelease(RSArchiverCopyData(archiver)), RSFileManagerStandardizingPath(RSSTR("~/Desktop/archiver.plist")), RSWriteFileAutomatically);
//    RSRelease(archiver);
//    
//    unarchiver = RSUnarchiverCreate(RSAllocatorSystemDefault, RSDataWithContentOfPath(RSFileManagerStandardizingPath(RSSTR("~/Desktop/archiver.plist"))));
//    RSDateRef date = RSUnarchiverDecodeObjectForKey(unarchiver, RSSTR("date"));
//    RSShow(date);
//    RSRelease(date);
//    
//    RSStringRef value = RSUnarchiverDecodeObjectForKey(unarchiver, RSSTR("test"));
//    RSShow(value);
//    RSRelease(value);
//    
//    tz = RSUnarchiverDecodeObjectForKey(unarchiver, RSSTR("tz"));
//    RSShow(tz);
//    RSRelease(tz);
//    
//    data = RSUnarchiverDecodeObjectForKey(unarchiver, RSSTR("data"));
//    RSShow(data);
//    RSRelease(data);
//    
//    calendar = (RSCalendarRef)RSUnarchiverDecodeObjectForKey(unarchiver, RSSTR("cal"));
//    RSShow(calendar);
//    RSRelease(calendar);
//
//    RSRelease(archiver);
}
#endif
