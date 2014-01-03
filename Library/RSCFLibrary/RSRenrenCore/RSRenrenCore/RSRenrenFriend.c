//
//  RSRenrenFriend.c
//  RSRenrenCore
//
//  Created by closure on 1/3/14.
//  Copyright (c) 2014 closure. All rights reserved.
//

#include "RSRenrenFriend.h"

#include <RSCoreFoundation/RSRuntime.h>

struct __RSRenrenFriend
{
    RSRuntimeBase _base;
    RSStringRef _name;
    RSURLRef _homePageURL;
    RSURLRef _imageURL;
    RSStringRef _schoolName;
    RSStringRef _account;
    RSUInteger _popularity;
};

static RSTypeRef __RSRenrenFriendClassCopy(RSAllocatorRef allocator, RSTypeRef rs, BOOL mutableCopy)
{
    return RSRetain(rs);
}

static void __RSRenrenFriendClassDeallocate(RSTypeRef rs)
{
    RSRenrenFriendRef friend = (RSRenrenFriendRef)rs;
    RSRelease(friend->_name);
    RSRelease(friend->_homePageURL);
    RSRelease(friend->_imageURL);
    RSRelease(friend->_schoolName);
    RSRelease(friend->_account);
    friend->_popularity = 0;
}

static BOOL __RSRenrenFriendClassEqual(RSTypeRef rs1, RSTypeRef rs2)
{
    RSRenrenFriendRef RSRenrenFriend1 = (RSRenrenFriendRef)rs1;
    RSRenrenFriendRef RSRenrenFriend2 = (RSRenrenFriendRef)rs2;
    BOOL result = NO;
    
    result = RSEqual(RSRenrenFriend1->_account, RSRenrenFriend2->_account);
    
    return result;
}

static RSHashCode __RSRenrenFriendClassHash(RSTypeRef rs)
{
    return RSHash(((RSRenrenFriendRef)rs)->_account);
}

static RSStringRef __RSRenrenFriendClassDescription(RSTypeRef rs)
{
    RSStringRef description = RSStringCreateWithFormat(RSAllocatorDefault, RSSTR("RSRenrenFriend %p"), rs);
    return description;
}

static RSRuntimeClass __RSRenrenFriendClass =
{
    _RSRuntimeScannedObject,
    "RSRenrenFriend",
    nil,
    __RSRenrenFriendClassCopy,
    __RSRenrenFriendClassDeallocate,
    __RSRenrenFriendClassEqual,
    __RSRenrenFriendClassHash,
    __RSRenrenFriendClassDescription,
    nil,
    nil
};

static RSTypeID _RSRenrenFriendTypeID = _RSRuntimeNotATypeID;
static RSSpinLock _RSRenrenFriendSpinLock = RSSpinLockInit;

static void __RSRenrenFriendInitialize()
{
    _RSRenrenFriendTypeID = __RSRuntimeRegisterClass(&__RSRenrenFriendClass);
    __RSRuntimeSetClassTypeID(&__RSRenrenFriendClass, _RSRenrenFriendTypeID);
}

RSExport RSTypeID RSRenrenFriendGetTypeID()
{
    RSSyncUpdateBlock(&_RSRenrenFriendSpinLock, ^{
        if (_RSRuntimeNotATypeID == _RSRenrenFriendTypeID)
            __RSRenrenFriendInitialize();
    });
    return _RSRenrenFriendTypeID;
}

static RSRenrenFriendRef __RSRenrenFriendCreateInstance(RSAllocatorRef allocator, RSTypeRef content)
{
    RSRenrenFriendRef instance = (RSRenrenFriendRef)__RSRuntimeCreateInstance(allocator, _RSRenrenFriendTypeID, sizeof(struct __RSRenrenFriend) - sizeof(RSRuntimeBase));
    
    return instance;
}

RSExport RSStringRef RSRenrenFriendGetName(RSRenrenFriendRef friendModel) {
    if (!friendModel) return nil;
    __RSGenericValidInstance(friendModel, _RSRenrenFriendTypeID);
    return friendModel->_name;
}

RSExport RSURLRef RSRenrenFriendGetHomePageURL(RSRenrenFriendRef friendModel) {
    if (!friendModel) return nil;
    __RSGenericValidInstance(friendModel, _RSRenrenFriendTypeID);
    return friendModel->_homePageURL;
}

RSExport RSURLRef RSRenrenFriendGetImageURL(RSRenrenFriendRef friendModel) {
    if (!friendModel) return nil;
    __RSGenericValidInstance(friendModel, _RSRenrenFriendTypeID);
    return friendModel->_imageURL;
}

RSExport RSStringRef RSRenrenFriendGetSchoolName(RSRenrenFriendRef friendModel) {
    if (!friendModel) return nil;
    __RSGenericValidInstance(friendModel, _RSRenrenFriendTypeID);
    return friendModel->_schoolName;
}

RSExport RSStringRef RSRenrenFriendGetAccountID(RSRenrenFriendRef friendModel) {
    if (!friendModel) return nil;
    __RSGenericValidInstance(friendModel, _RSRenrenFriendTypeID);
    return friendModel->_account;
}

RSExport RSUInteger RSRenrenFriendGetPopularity(RSRenrenFriendRef friendModel) {
    if (!friendModel) return 0;
    __RSGenericValidInstance(friendModel, _RSRenrenFriendTypeID);
    return friendModel->_popularity;
}

RSExport void RSRenrenFriendSetName(RSRenrenFriendRef friendModel, RSStringRef name) {
    if (!friendModel) return ;
    __RSGenericValidInstance(friendModel, _RSRenrenFriendTypeID);
    RSRelease(friendModel->_name);
    friendModel->_name = RSRetain(name);
}

RSExport void RSRenrenFriendSetHomePageURL(RSRenrenFriendRef friendModel, RSURLRef homePageURL) {
    if (!friendModel) return ;
    __RSGenericValidInstance(friendModel, _RSRenrenFriendTypeID);
    RSRelease(friendModel->_homePageURL);
    friendModel->_homePageURL = RSRetain(homePageURL);
}

RSExport void RSRenrenFriendSetImageURL(RSRenrenFriendRef friendModel, RSURLRef imageURL) {
    if (!friendModel) return ;
    __RSGenericValidInstance(friendModel, _RSRenrenFriendTypeID);
    RSRelease(friendModel->_imageURL);
    friendModel->_imageURL = RSRetain(imageURL);
}

RSExport void RSRenrenFriendSetSchoolName(RSRenrenFriendRef friendModel, RSStringRef schoolName) {
    if (!friendModel) return ;
    __RSGenericValidInstance(friendModel, _RSRenrenFriendTypeID);
    RSRelease(friendModel->_schoolName);
    friendModel->_schoolName = RSRetain(schoolName);
}

RSExport void RSRenrenFriendSetAccountID(RSRenrenFriendRef friendModel, RSStringRef accountID) {
    if (!friendModel) return ;
    __RSGenericValidInstance(friendModel, _RSRenrenFriendTypeID);
    RSRelease(friendModel->_account);
    friendModel->_account = RSRetain(accountID);
}
RSExport void RSRenrenFriendSetPopularity(RSRenrenFriendRef friendModel, RSUInteger popularity) {
    if (!friendModel) return ;
    __RSGenericValidInstance(friendModel, _RSRenrenFriendTypeID);
    friendModel->_popularity = popularity;
}
