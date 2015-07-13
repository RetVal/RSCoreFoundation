//
//  Lock.h
//  RSCoreFoundation
//
//  Created by closure on 6/2/15.
//  Copyright (c) 2015 RetVal. All rights reserved.
//

#ifndef __RSCoreFoundation__Lock__
#define __RSCoreFoundation__Lock__

#include <RSFoundation/BasicTypeDefine.hpp>
#include <RSFoundation/Object.hpp>

#if DEPLOYMENT_TARGET_MACOSX || DEPLOYMENT_TARGET_IPHONEOS
#include <libkern/OSAtomic.h>
#endif

#if DEPLOYMENT_TARGET_MACOSX || DEPLOYMENT_TARGET_IPHONEOS || DEPLOYMENT_TARGET_LINUX
#include <pthread/pthread.h>
#endif

namespace RSFoundation {
    namespace Basic {
        class Lock : public Object {
        public:
            Lock() {} ;
            virtual ~Lock() {};
        public:
            virtual void Acquire() {};
            virtual void Release() {};
        };
        
        class SpinLock : public Lock {
        private:
#if DEPLOYMENT_TARGET_MACOSX || DEPLOYMENT_TARGET_IPHONEOS
            
#undef LOCK_IMPLMENT
#undef LOCK_INIT
#undef LOCK_LOCK
#undef LOCK_UNLOCK
            
#define LOCK_IMPLMENT OSSpinLock
#define LOCK_INIT OS_SPINLOCK_INIT
#define LOCK_LOCK(x) OSSpinLockLock(&(x))
#define LOCK_UNLOCK(x) OSSpinLockUnlock(&(x))
#else
#define LOCK_IMPLMENT OSSpinLock
#define LOCK_INIT 0
#endif
        public:
            SpinLock() : Lock() , _lock(LOCK_INIT) {
            }
            
        public:
            virtual void Acquire() override {
                LOCK_LOCK(_lock);
            }
            
            virtual void Release() override {
                LOCK_UNLOCK(_lock);
            }
            
        private:
            volatile LOCK_IMPLMENT _lock;
        };
        
        class MutexLock : public Lock {
#if DEPLOYMENT_TARGET_MACOSX || DEPLOYMENT_TARGET_IPHONEOS || DEPLOYMENT_TARGET_LINUX
            
#undef LOCK_IMPLMENT
#undef LOCK_INIT
#undef LOCK_LOCK
#undef LOCK_UNLOCK            
            
#define LOCK_IMPLMENT pthread_mutex_t
#define LOCK_INIT PTHREAD_MUTEX_INITIALIZER
#define LOCK_LOCK(x) pthread_mutex_lock(&(x))
#define LOCK_UNLOCK(x) pthread_mutex_unlock(&(x))
#else
#define LOCK_IMPLMENT pthread_mutex_t
#define LOCK_INIT PTHREAD_MUTEX_INITIALIZER
#endif
        public:
            MutexLock() : Lock(), _lock(LOCK_INIT) {
            }
            
            virtual void Acquire() override {
                LOCK_LOCK(_lock);
            }
            
            virtual void Release() override {
                LOCK_UNLOCK(_lock);
            }
            
        private:
            LOCK_IMPLMENT _lock;
        };
        
        class LockRAII {
        public:
            LockRAII(Lock &lock) : _lock(lock) {
                _lock.Acquire();
            }
            
            ~LockRAII() {
                _lock.Release();
            }
            
        private:
            Lock &_lock;
        };
        
    }
}

#endif /* defined(__RSCoreFoundation__Lock__) */
