//
//  Allocator.h
//  RSCoreFoundation
//
//  Created by closure on 5/27/15.
//  Copyright (c) 2015 RetVal. All rights reserved.
//

#ifndef __RSCoreFoundation__Allocator__
#define __RSCoreFoundation__Allocator__

#include <RSFoundation/Object.hpp>
#include <RSFoundation/Nullable.hpp>

#include <typeinfo>

#include <malloc/malloc.h>
#include <new>
#include <iostream>

namespace RSFoundation {
    class Equal;
    namespace Basic {
        class Hash;
    }
    
    namespace Collection {
        class String;
    }
    namespace Basic {
        class AllocatorProtocol : public virtual Protocol {
        public:
            AllocatorProtocol() {};
            ~AllocatorProtocol() {};
            static void *Allocate(size_t size) {return nullptr;}
            static void *Reallocate(void* ptr, size_t size) {return nullptr;}
            static void Free(void *ptr) {return;}
            static size_t Size(void *ptr) {return 0;}
        };
        
        class GCAllocator : public virtual AllocatorProtocol {
        public:
            template <typename T, typename U>
            friend class Allocator;
            GCAllocator() {};
            ~GCAllocator() {};
            static void *Allocate(size_t size);
            static void *Reallocate(void* ptr, size_t size);
            static void Free(void *ptr);
            static size_t Size(void *ptr);
        };
        
        class OSXAllocator : public virtual AllocatorProtocol {
        public:
            template <typename T, typename U>
            friend class Allocator;
            OSXAllocator() {};
            ~OSXAllocator() {};
            static void *Allocate(size_t size);
            static void *Reallocate(void* ptr, size_t size);
            static void Free(void *ptr);
            static size_t Size(void *ptr);
        private:
        };
        
        template <typename T, typename IMPL =
#if defined(RS_FOUNDATION_GC)
        GCAllocator
#else
        OSXAllocator
#endif
        >
        class Allocator : public Object, private Counter<UInt32> {
            static_assert(std::is_base_of<AllocatorProtocol, IMPL>::value, "IMPL should inherit from AllocatorProtocol");
        public:
            static Allocator<T, IMPL> SystemDefault;
            static Allocator<T, IMPL> Default;
            
        public:
            template<typename ...Args>
            T *Allocate(Args... args) {
                void *ptr = IMPL::Allocate(sizeof(T));
                T *t = new (ptr) T(args...);
                Inc();
                std::cout << Self << " inc " << name << " " << Val() << "\n";
                return t;
            }
            
            template<typename T2>
            T2 *Allocate(size_t size) {
                return _AllocateImpl<T2, std::is_pod<T2>::value>::_Allocate(size);
            }
            
            template<void*>
            void *Allocate(size_t size) {
                return IMPL::Allocate(size);
            }
            
        private:
            friend class RSFoundation::Collection::String;
            friend class RSFoundation::Basic::Hash;
            
            template<typename T2, bool isPod>
            class _AllocateImpl {
                friend class Allocator;
                friend class RSFoundation::Collection::String;
                friend class RSFoundation::Basic::Hash;
                
                static T2 *_Allocate(size_t size) {
                    return nullptr;
                }
            };
            
            template<typename T2>
            class _AllocateImpl<T2, false> {
                friend class Allocator;
                friend class RSFoundation::Collection::String;
                friend class RSFoundation::Basic::Hash;
                
                static T2 *_Allocate(size_t size) {
                    size = 8 + size; // this +
                    void *ptr = IMPL::Allocate(size);
                    T2 *t = new (ptr) T2;
                    Allocator<T2> *allocator = &Allocator<T2>::SystemDefault;
                    allocator->Inc();
                    std::cout << allocator->Self << " inc " << allocator->name << " " << allocator->Val() << "\n";
                    return t;
                }
            };
            
            template<typename T2>
            class _AllocateImpl<T2, true> {
                friend class Allocator;
                friend class RSFoundation::Collection::String;
                friend class RSFoundation::Basic::Hash;
                
                static T2 *_Allocate(size_t size) {
                    return static_cast<T2*>(IMPL::Allocate(size * sizeof(T2)));;
                }
            };
        public:
            
            template<typename T2>
            T2 *Reallocate(void *p, size_t size) {
                static_assert(POD<T2>::Result, "");
                T2 *ptr = static_cast<T2*>(IMPL::Reallocate(p, size * sizeof(T2)));
                return ptr;
            }
            
            void Deallocate(void *ptr) {
                IMPL::Free(ptr);
            }
            
            void Deallocate(T *t) {
                t->T::~T();
                IMPL::Free(static_cast<void*>(t));
                Dec();
                std::cout << Self << " dec " << name << " " << Val() << "\n";
            }
            
            void Deallocate(const T *t) {
                t->T::~T();
                IMPL::Free((void*)(t));
                Dec();
                std::cout << Self << " dec " << name << " " << Val() << "\n";
            }

            void Deallocate(Nullable<T> &obj) {
                if (obj) {
                    Nullable<T> &temp = MoveValue(obj);
                    Deallocate(temp.GetValue());
                }
            }
            
            void Deallocate(nullptr_t nilPtr) {
                
            }
            
            size_t GetSize(void *ptr) {
                return IMPL::Size(ptr);
            }
            
            bool IsGC() const {
                return is_base_of<GCAllocator, IMPL>::value ? true : false;
            }
        private:
            Allocator() {
                Object obj;
                obj.template GetClassName<T>(name);
                obj.template GetClassName<decltype(this)>(Self);
            }
            
            ~Allocator() {
            }
            
        private:
            std::string name;
            std::string Self;
        };

        
        template<typename T, typename IMPL>
        Allocator<T, IMPL> Allocator<T, IMPL>::SystemDefault;
        
        template<typename T, typename IMPL>
        Allocator<T, IMPL> Allocator<T, IMPL>::Default;
    }
}

#endif /* defined(__RSCoreFoundation__Allocator__) */