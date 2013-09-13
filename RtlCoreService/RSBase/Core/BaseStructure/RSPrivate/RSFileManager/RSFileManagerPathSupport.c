//
//  RSFileManagerPathSupport.c
//  RSCoreFoundation
//
//  Created by RetVal on 3/1/13.
//  Copyright (c) 2013 RetVal. All rights reserved.
//

#include <stdio.h>
#include <RSCoreFoundation/RSFileManager.h>
#if DEPLOYMENT_TARGET_WINDOWS
#define WINDOWS_PATH_SEMANTICS
#else
#define UNIX_PATH_SEMANTICS
#endif

#if defined(WINDOWS_PATH_SEMANTICS)
#define RSPreferredSlash	((UniChar)'\\')
#elif defined(UNIX_PATH_SEMANTICS)
#define RSPreferredSlash	((UniChar)'/')
#elif defined(HFS_PATH_SEMANTICS)
#define RSPreferredSlash	((UniChar)':')
#else
#error Cannot define NSPreferredSlash on this platform
#endif

#if defined(HFS_PATH_SEMANTICS)
#define HAS_DRIVE(S) (NO)
#define HAS_NET(S) (NO)
#else
#define HAS_DRIVE(S) ((S)[1] == ':' && (('A' <= (S)[0] && (S)[0] <= 'Z') || ('a' <= (S)[0] && (S)[0] <= 'z')))
#define HAS_NET(S) ((S)[0] == '\\' && (S)[1] == '\\')
#endif

#if defined(WINDOWS_PATH_SEMANTICS)
#define IS_SLASH(C)	((C) == '\\' || (C) == '/')
#elif defined(UNIX_PATH_SEMANTICS)
#define IS_SLASH(C)	((C) == '/')
#elif defined(HFS_PATH_SEMANTICS)
#define IS_SLASH(C)	((C) == ':')
#endif

RSPrivate BOOL _RSStripTrailingPathSlashes(UniChar *unichars, RSIndex *length)
{
    BOOL destHasDrive = (1 < *length) && HAS_DRIVE(unichars);
    RSIndex oldLength = *length;
    while (((destHasDrive && 3 < *length) || (!destHasDrive && 1 < *length)) && IS_SLASH(unichars[*length - 1])) {
        (*length)--;
    }
    return (oldLength != *length);
}

RSPrivate BOOL _RSTransmutePathSlashes(UniChar *unichars, RSIndex *length, UniChar replSlash)
{
    RSIndex didx, sidx, scnt = *length;
    sidx = (1 < *length && HAS_NET(unichars)) ? 2 : 0;
    didx = sidx;
    while (sidx < scnt)
    {
        if (IS_SLASH(unichars[sidx]))
        {
            unichars[didx++] = replSlash;
            for (sidx++; sidx < scnt && IS_SLASH(unichars[sidx]); sidx++);
        }
        else
        {
            unichars[didx++] = unichars[sidx++];
        }
    }
    *length = didx;
    return (scnt != didx);
}


RSPrivate RSIndex _RSStartOfLastPathComponent(UniChar *unichars, RSIndex length)
{
    RSIndex idx;
    if (length < 2)
    {
        return 0;
    }
    for (idx = length - 1; idx; idx--)
    {
        if (IS_SLASH(unichars[idx - 1]))
        {
            return idx;
        }
    }
    if ((2 < length) && HAS_DRIVE(unichars))
    {
        return 2;
    }
    return 0;
}


RSPrivate BOOL _RSAppendPathExtension(UniChar* unichars, RSIndex *length, RSIndex maxLength, UniChar* extension, RSIndex extensionLength)
{
    if (maxLength < *length + 1 + extensionLength)
    {
        return NO;
    }
    if ((0 < extensionLength && IS_SLASH(extension[0])) || (1 < extensionLength && HAS_DRIVE(extension))) {
        return NO;
    }
    _RSStripTrailingPathSlashes(unichars, length);
    switch (*length) {
        case 0:
            return NO;
        case 1:
            if (IS_SLASH(unichars[0]) || unichars[0] == '~') {
                return NO;
            }
            break;
        case 2:
            if (HAS_DRIVE(unichars) || HAS_NET(unichars)) {
                return NO;
            }
            break;
        case 3:
            if (IS_SLASH(unichars[2]) && HAS_DRIVE(unichars)) {
                return NO;
            }
            break;
    }
    if (0 < *length && unichars[0] == '~') {
        RSIndex idx;
        BOOL hasSlash = NO;
        for (idx = 1; idx < *length; idx++)
        {
            if (IS_SLASH(unichars[idx])) {
                hasSlash = YES;
                break;
            }
        }
        if (!hasSlash) {
            return NO;
        }
    }
    unichars[(*length)++] = '.';
    memmove(unichars + *length, extension, extensionLength * sizeof(UniChar));
    *length += extensionLength;
    return YES;
}

RSPrivate RSIndex _RSLengthAfterDeletingLastPathComponent(UniChar *unichars, RSIndex length)
{
    RSIndex idx;
    if (length < 2)
    {
        return 0;
    }
    for (idx = length - 1; idx; idx--)
    {
        if (IS_SLASH(unichars[idx - 1]))
        {
            if ((idx != 1) && (!HAS_DRIVE(unichars) || idx != 3))
            {
                return idx - 1;
            }
            return idx;
        }
    }
    if ((2 < length) && HAS_DRIVE(unichars))
    {
        return 2;
    }
    return 0;
}

RSPrivate RSIndex _RSStartOfPathExtension(UniChar *unichars, RSIndex length)
{
    RSIndex idx;
    if (length < 2)
    {
        return 0;
    }
    for (idx = length - 1; idx; idx--)
    {
        if (IS_SLASH(unichars[idx - 1]))
        {
            return 0;
        }
        if (unichars[idx] != '.')
        {
            continue;
        }
        if (idx == 2 && HAS_DRIVE(unichars))
        {
            return 0;
        }
        return idx;
    }
    return 0;
}

RSPrivate RSIndex _RSLengthAfterDeletingPathExtension(UniChar *unichars, RSIndex length)
{
    RSIndex start = _RSStartOfPathExtension(unichars, length);
    return ((0 < start) ? start : length);
}

