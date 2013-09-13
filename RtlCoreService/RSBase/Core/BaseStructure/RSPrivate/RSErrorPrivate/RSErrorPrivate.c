//
//  RSErrorPrivate.c
//  RSCoreFoundation
//
//  Created by RetVal on 02/6/02.
//  Copyright (c) 2002 RetVal. All rights reserved.
//

#include <stdio.h>
#include "RSErrorPrivate.h"

static const struct __RSErrorPrivateFormatTable __RSErrorPrivateDescriptionFormatTable[] = {
    {"The operation success.", 0},
    {"The operation can not be completed.", 0},
    {"File can not be opened.", 0},
    {"File can not be read.", 0},
    {"File can not be wrote.", 0},
    {"File can not be executed.", 0},
    {"File can not be deleted.", 0},
    {"File or directory is not found.", 0},
    {"Privilege is too low to do the operation.", 0},
    {"Reserved.", 0},
    {"Get the file end pointer.", 0},
    {"The target is missed.", 0},
    {"Directory can not be created.", 0},
    {"OS has not enough memory for use.", 0},
    {"Reserved.", 0}, //kErrMmNode,
    {"Memory buffer is too small to hold content.", 0},
    {"Memory buffer is too large to use(waste some resource).", 0},
    {"Do not have the privilege for do the memory operation.", 0},
    {"The pointer(%P) is nil.", 0},
    {"The pointer(%P) is not nil.", 0},
    {"Reserved.", 0},
    {"The stack is full.", 0},
    {"The operation is not available.", 0},
    {"Format is not available.", 0},
    {"The operation is not be verifyed.", 0},
//    {"", 0},
//    {"", 0},
//    {"", 0},
//    {"", 0},
//    {"", 0},
//    {"", 0},
//    {"", 0},
//    {"", 0},
//    {"", 0},
//    {"", 0},
//    {"", 0},
//    {"", 0},
//    {"", 0},
};

static const RSUInteger __RSErrorPrivateDescriptionFormatCount = sizeof(__RSErrorPrivateDescriptionFormatTable) / sizeof(struct __RSErrorPrivateFormatTable);

struct __RSErrorPrivateFormatTable __RSErrorDomainRSCoreFoundationGetCStringWithCode(RSIndex code)
{
    if (code > kOperatorFail)
        code -= kOperatorFail;
    if (0 < code  && code < __RSErrorPrivateDescriptionFormatCount)
    {
        return __RSErrorPrivateDescriptionFormatTable[code];
    }
    static struct __RSErrorPrivateFormatTable undefinedErrorCode = {"The code is undefined.",0};
    return undefinedErrorCode;
}