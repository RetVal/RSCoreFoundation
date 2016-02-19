//
//  RSRuntimeDebugSupport.h
//  RSCoreFoundation
//
//  Created by closure on 9/29/15.
//  Copyright © 2015 RetVal. All rights reserved.
//

#ifndef DEBUG_PERFERENCE
#define DEBUG_PERFERENCE(Prefix, Name, BitWidth, Default, Comment)
#endif

#ifndef DEBUG_LOG_PERFERENCE
#define DEBUG_LOG_PERFERENCE(Name, BitWidth, Default, Comment) DEBUG_PERFERENCE(_RSLog, Name, BitWidth, Default, Comment)
#endif

#ifndef DEBUG_RUNTIME_PERFERENCE
#define DEBUG_RUNTIME_PERFERENCE(Name, BitWidth, Default, Comment) DEBUG_PERFERENCE(_RSRuntime, Name, BitWidth, Default, Comment)
#endif

#ifndef DEBUG_STRING_PERFERENCE
#define DEBUG_STRING_PERFERENCE(Name, BitWidth, Default, Comment) DEBUG_PERFERENCE(_RSString, Name, BitWidth, Default, Comment)
#endif

#ifndef DEBUG_PLIST_PERFERENCE
#define DEBUG_PLIST_PERFERENCE(Name, BitWidth, Default, Comment) DEBUG_PERFERENCE(_RSPropertyList, Name, BitWidth, Default, Comment)
#endif



DEBUG_RUNTIME_PERFERENCE(InstanceBZeroBeforeDie, 1, 1, "")
DEBUG_RUNTIME_PERFERENCE(ISABaseOnEmptyField, 1, 0, "")
DEBUG_RUNTIME_PERFERENCE(InstanceManageWatcher, 1, 0, "")
DEBUG_RUNTIME_PERFERENCE(InstanceRefWatcher, 1, 0, "")
DEBUG_RUNTIME_PERFERENCE(InstanceAllocFreeWatcher, 1, 0, "")
DEBUG_RUNTIME_PERFERENCE(InstanceARC, 1, 0, "")
DEBUG_RUNTIME_PERFERENCE(CheckAutoreleaseFlag, 1, 0, "")
DEBUG_RUNTIME_PERFERENCE(LogSave, 1, 0, "")

DEBUG_STRING_PERFERENCE(NoticeWhenConstantStringAddToTable, 1, 0, "")

DEBUG_PLIST_PERFERENCE(WarningWhenParseNullKey, 1, 0, "")
DEBUG_PLIST_PERFERENCE(WarningWhenParseNullValue, 1, 0, "")

DEBUG_LOG_PERFERENCE(DebugLevelShouldFallThrough, 1, 0, "")

#undef DEBUG_PERFERENCE
