//
//  Object.hpp
//  RSCoreFoundation
//
//  Created by closure on 7/6/15.
//  Copyright © 2015 RetVal. All rights reserved.
//

#ifndef Object_cpp
#define Object_cpp

#include <cstddef>
#include <RSCoreFoundation/RSCoreFoundation.h>

namespace RSCF {
    class String;
    
    template <class Derived>
    class RefCountedBase {
        mutable unsigned ref_cnt;
        
    public:
        RefCountedBase() : ref_cnt(0) {}
        RefCountedBase(const RefCountedBase &) : ref_cnt(0) {}
        
        void retain() const { ++ref_cnt; }
        void release() const {
            assert (ref_cnt > 0 && "Reference count is already zero.");
            if (--ref_cnt == 0) delete static_cast<const Derived*>(this);
        }
        
        unsigned getRetainCount() const { return ref_cnt; }
    };
    
    template <typename T> struct IntrusiveRefCntPtrInfo {
        static void retain(T *obj) { obj->retain(); }
        static void release(T *obj) { obj->release(); }
        static unsigned getRetainCount(T *obj) { return obj->getRetainCount(); }
    };
    
    template <typename T>
    class IntrusiveRefCntPtr {
        T* Obj;
        
    public:
        typedef T element_type;
        
        explicit IntrusiveRefCntPtr() : Obj(nullptr) {}
        
        IntrusiveRefCntPtr(T* obj) : Obj(obj) {
            retain();
        }
        
        IntrusiveRefCntPtr(const IntrusiveRefCntPtr& S) : Obj(S.Obj) {
            retain();
        }
        
        IntrusiveRefCntPtr(IntrusiveRefCntPtr&& S) : Obj(S.Obj) {
            S.Obj = nullptr;
        }
        
        template <class X>
        IntrusiveRefCntPtr(IntrusiveRefCntPtr<X>&& S) : Obj(S.get()) {
            S.Obj = 0;
        }
        
        template <class X>
        IntrusiveRefCntPtr(const IntrusiveRefCntPtr<X>& S)
        : Obj(S.get()) {
            retain();
        }
        
        IntrusiveRefCntPtr& operator=(IntrusiveRefCntPtr S) {
            swap(S);
            return *this;
        }
        
        ~IntrusiveRefCntPtr() { release(); }
        
        T& operator*() const { return *Obj; }
        
        T* operator->() const { return Obj; }
        
        T* get() const { return Obj; }
        
        explicit operator bool() const { return Obj; }
        
        void swap(IntrusiveRefCntPtr& other) {
            T* tmp = other.Obj;
            other.Obj = Obj;
            Obj = tmp;
        }
        
        void reset() {
            release();
            Obj = nullptr;
        }
        
        void resetWithoutRelease() {
            Obj = 0;
        }
        
    private:
        void retain() { if (Obj) IntrusiveRefCntPtrInfo<T>::retain(Obj); }
        void release() { if (Obj) IntrusiveRefCntPtrInfo<T>::release(Obj); }
        
        template <typename X>
        friend class IntrusiveRefCntPtr;
    };
    
    template<class T, class U>
    inline bool operator==(const IntrusiveRefCntPtr<T>& A,
                           const IntrusiveRefCntPtr<U>& B)
    {
        return A.get() == B.get();
    }
    
    template<class T, class U>
    inline bool operator!=(const IntrusiveRefCntPtr<T>& A,
                           const IntrusiveRefCntPtr<U>& B)
    {
        return A.get() != B.get();
    }
    
    template<class T, class U>
    inline bool operator==(const IntrusiveRefCntPtr<T>& A,
                           U* B)
    {
        return A.get() == B;
    }
    
    template<class T, class U>
    inline bool operator!=(const IntrusiveRefCntPtr<T>& A,
                           U* B)
    {
        return A.get() != B;
    }
    
    template<class T, class U>
    inline bool operator==(T* A,
                           const IntrusiveRefCntPtr<U>& B)
    {
        return A == B.get();
    }
    
    template<class T, class U>
    inline bool operator!=(T* A,
                           const IntrusiveRefCntPtr<U>& B)
    {
        return A != B.get();
    }
    
    template <class T>
    bool operator==(std::nullptr_t A, const IntrusiveRefCntPtr<T> &B) {
        return !B;
    }
    
    template <class T>
    bool operator==(const IntrusiveRefCntPtr<T> &A, std::nullptr_t B) {
        return B == A;
    }
    
    template <class T>
    bool operator!=(std::nullptr_t A, const IntrusiveRefCntPtr<T> &B) {
        return !(A == B);
    }
    
    template <class T>
    bool operator!=(const IntrusiveRefCntPtr<T> &A, std::nullptr_t B) {
        return !(A == B);
    }
    
    class Object {
    public:
        Object() {}
        virtual ~Object() {}
    };
    
    class RefObject : public Object, public RefCountedBase<RefObject> {
    public:
        
    };
}

#endif /* Object_cpp */
