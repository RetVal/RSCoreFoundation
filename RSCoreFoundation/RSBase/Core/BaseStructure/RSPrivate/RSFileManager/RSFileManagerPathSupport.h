//
//  RSFileManagerPathSupport.h
//  RSCoreFoundation
//
//  Created by RetVal on 3/1/13.
//  Copyright (c) 2013 RetVal. All rights reserved.
//

#ifndef RSCoreFoundation_RSFileManagerPathSupport_h
#define RSCoreFoundation_RSFileManagerPathSupport_h
#include <RSCoreFoundation/RSFileManager.h>
RSPrivate BOOL _RSStripTrailingPathSlashes(UniChar *unichars, RSIndex *length);
RSPrivate BOOL _RSTransmutePathSlashes(UniChar *unichars, RSIndex *length, UniChar replSlash);
RSPrivate RSIndex _RSStartOfLastPathComponent(UniChar *unichars, RSIndex length);
RSPrivate BOOL _RSAppendPathExtension(UniChar* unichars, RSIndex *length, RSIndex maxLength, UniChar* extension, RSIndex extensionLength);

RSPrivate RSIndex _RSLengthAfterDeletingLastPathComponent(UniChar *unichars, RSIndex length);
RSPrivate RSIndex _RSStartOfPathExtension(UniChar *unichars, RSIndex length);
RSPrivate RSIndex _RSLengthAfterDeletingPathExtension(UniChar *unichars, RSIndex length);
#endif
