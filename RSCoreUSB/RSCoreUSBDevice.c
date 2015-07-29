//
//  RSCoreUSBDevice.c
//  RSCoreFoundation
//
//  Created by closure on 7/23/15.
//  Copyright (c) 2015 RetVal. All rights reserved.
//

#include "RSCoreUSBDevice.h"

#include <RSCoreFoundation/RSRuntime.h>

struct __RSCoreUSBDevice {
    RSRuntimeBase _base;
    RSStringRef _deviceName;
};

static void __RSCoreUSBDeviceClassInit(RSTypeRef rs) {
    
}

static RSTypeRef __RSCoreUSBDeviceClassCopy(RSAllocatorRef allocator, RSTypeRef rs, BOOL mutableCopy) {
    return RSRetain(rs);
}

static void __RSCoreUSBDeviceClassDeallocate(RSTypeRef rs) {
    RSCoreUSBDeviceRef device = (RSCoreUSBDeviceRef)rs;
    RSRelease(device->_deviceName);
}

static BOOL __RSCoreUSBDeviceClassEqual(RSTypeRef rs1, RSTypeRef rs2) {
    RSCoreUSBDeviceRef RSCoreUSBDevice1 = (RSCoreUSBDeviceRef)rs1;
    RSCoreUSBDeviceRef RSCoreUSBDevice2 = (RSCoreUSBDeviceRef)rs2;
    BOOL result = NO;
    
    result = RSEqual(RSCoreUSBDevice1->_deviceName, RSCoreUSBDevice2->_deviceName);
    
    return result;
}

static RSHashCode __RSCoreUSBDeviceClassHash(RSTypeRef rs) {
    RSCoreUSBDeviceRef device = (RSCoreUSBDeviceRef)rs;
    return RSHash(device->_deviceName);
}

static RSStringRef __RSCoreUSBDeviceClassDescription(RSTypeRef rs) {
    RSStringRef description = RSStringCreateWithFormat(RSAllocatorDefault, RSSTR("RSCoreUSBDevice %p"), rs);
    return description;
}

static RSRuntimeClass __RSCoreUSBDeviceClass = {
    _RSRuntimeScannedObject,
    0,
    "RSCoreUSBDevice",
    __RSCoreUSBDeviceClassInit,
    __RSCoreUSBDeviceClassCopy,
    __RSCoreUSBDeviceClassDeallocate,
    __RSCoreUSBDeviceClassEqual,
    __RSCoreUSBDeviceClassHash,
    __RSCoreUSBDeviceClassDescription,
    nil,
    nil
};

static RSTypeID _RSCoreUSBDeviceTypeID = _RSRuntimeNotATypeID;

static void __RSCoreUSBDeviceInitialize() {
    _RSCoreUSBDeviceTypeID = __RSRuntimeRegisterClass(&__RSCoreUSBDeviceClass);
    __RSRuntimeSetClassTypeID(&__RSCoreUSBDeviceClass, _RSCoreUSBDeviceTypeID);
}

RSExport RSTypeID RSCoreUSBDeviceGetTypeID() {
//    static dispatch_once_t onceToken;
//    dispatch_once(&onceToken, ^{
//        __RSCoreUSBDeviceInitialize();
//    });
    return _RSCoreUSBDeviceTypeID;
}

//RSPrivate void __RSCoreUSBDeviceDeallocate() {
//    <#do your finalize operation#>
//}

static RSCoreUSBDeviceRef __RSCoreUSBDeviceCreateInstance(RSAllocatorRef allocator, RSStringRef uuid) {
    RSCoreUSBDeviceRef instance = (RSCoreUSBDeviceRef)__RSRuntimeCreateInstance(allocator, _RSCoreUSBDeviceTypeID /*RSCoreUSBDeviceGetTypeID()*/, sizeof(struct __RSCoreUSBDevice) - sizeof(RSRuntimeBase));
    
    
    
    return instance;
}
