//
//  RSRuntimeAtomic.h
//  RSCoreFoundation
//
//  Created by RetVal on 10/16/12.
//  Copyright (c) 2012 RetVal. All rights reserved.
//

#ifndef RSCoreFoundation_RSRuntimeAtomic_h
#define RSCoreFoundation_RSRuntimeAtomic_h
#include <RSCoreFoundation/RSBase.h>
#include <RSCoreFoundation/RSBaseMacro.h>
#include <libkern/OSAtomic.h>
#if __LP64__
//typedef bool (* __RSRuntimeAtomicAdd) (int64_t __oldValue, int64_t __newValue, volatile int64_t *__theValue);
//static __RSRuntimeAtomicAdd RSRuntimeAtomicAdd = OSAtomicCompareAndSwap64Barrier;
#define RSRuntimeAtomicAdd OSAtomicCompareAndSwap64Barrier
#else
//typedef bool (* __RSRuntimeAtomicAdd) (int32_t __oldValue, int32_t __newValue, volatile int32_t *__theValue);
//static __RSRuntimeAtomicAdd RSRuntimeAtomicAdd = OSAtomicCompareAndSwap32Barrier;
#define RSRuntimeAtomicAdd OSAtomicCompareAndSwap32Barrier
#endif
#endif
