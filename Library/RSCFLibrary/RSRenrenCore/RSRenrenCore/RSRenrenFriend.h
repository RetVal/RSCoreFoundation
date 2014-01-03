//
//  RSRenrenFriend.h
//  RSRenrenCore
//
//  Created by closure on 1/3/14.
//  Copyright (c) 2014 closure. All rights reserved.
//

#ifndef RSRenrenCore_RSRenrenFriend
#define RSRenrenCore_RSRenrenFriend

#include <RSCoreFoundation/RSCoreFoundation.h>

RS_EXTERN_C_BEGIN

typedef struct __RSRenrenFriend *RSRenrenFriendRef;

RSExport RSTypeID RSRenrenFriendGetTypeID();
RSExport RSStringRef RSRenrenFriendGetName(RSRenrenFriendRef friendModel);
RSExport RSURLRef RSRenrenFriendGetHomePageURL(RSRenrenFriendRef friendModel);
RSExport RSURLRef RSRenrenFriendGetImageURL(RSRenrenFriendRef friendModel);
RSExport RSStringRef RSRenrenFriendGetSchoolName(RSRenrenFriendRef friendModel);
RSExport RSStringRef RSRenrenFriendGetAccountID(RSRenrenFriendRef friendModel);
RSExport RSUInteger RSRenrenFriendGetPopularity(RSRenrenFriendRef friendModel);

RSExport void RSRenrenFriendSetName(RSRenrenFriendRef friendModel, RSStringRef name);
RSExport void RSRenrenFriendSetHomePageURL(RSRenrenFriendRef friendModel, RSURLRef homePageURL);
RSExport void RSRenrenFriendSetImageURL(RSRenrenFriendRef friendModel, RSURLRef imageURL);
RSExport void RSRenrenFriendSetSchoolName(RSRenrenFriendRef friendModel, RSStringRef schoolName);
RSExport void RSRenrenFriendSetAccountID(RSRenrenFriendRef friendModel, RSStringRef accountID);
RSExport void RSRenrenFriendSetPopularity(RSRenrenFriendRef friendModel, RSUInteger popularity);
RS_EXTERN_C_END
#endif 
