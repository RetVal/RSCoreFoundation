//
//  Allocator.h
//  RSCoreFoundation
//
//  Created by closure on 5/27/15.
//  Copyright (c) 2015 RetVal. All rights reserved.
//

#ifndef __RSCoreFoundation__Allocator__
#define __RSCoreFoundation__Allocator__

#include <RSFoundation/Object.h>
#include <RSFoundation/Nullable.h>

#include <typeinfo>

#include <malloc/malloc.h>
#include <new>
#include <iostream>

namespace RSFoundation {
    namespace Collection {
        class String;
    }
    namespace Basic {
        class GCAllocator : public Object {
        private:
            template <typename T>
            friend class Allocator;
            GCAllocator() {};
            ~GCAllocator() {};
            static void *Allocate(size_t size);
            static void *Reallocate(void* ptr, size_t size);
            static void Free(void *ptr);
        };
        
        template <typename T>
        class Allocator : public Object, private Counter<UInt32> {
        public:
            static Allocator<T> AllocatorSystemDefault;
            static Allocator<T> AllocatorDefault;
            
        public:
            template<typename ...Args>
            T *Allocate(Args... args) {
                void *ptr = GCAllocator::Allocate(sizeof(T));
                T *t = new (ptr) T(args...);
                Inc();
                std::cout << Self << " inc " << name << " " << Val() << "\n";
                return t;
            }
            
            template<typename T2>
            T2 *Allocate(size_t size) {
                return _AllocateImpl<T2, std::is_pod<T2>::value>::_Allocate(zone, size);
            }
            
        private:
            friend class RSFoundation::Collection::String;
            template<typename T2, bool isPod>
            class _AllocateImpl {
                friend class Allocator;
                friend class RSFoundation::Collection::String;
                static T2 *_Allocate(malloc_zone_t *zone, size_t size) {
                    return nullptr;
                }
            };
            
            template<typename T2>
            class _AllocateImpl<T2, false> {
                friend class Allocator;
                friend class RSFoundation::Collection::String;
                static T2 *_Allocate(malloc_zone_t *zone, size_t size) {
                    size = 8 + size; // this +
                    void *ptr = GCAllocator::Allocate(size);
                    T2 *t = new (ptr) T2;
                    Allocator<T2> *allocator = &Allocator<T2>::AllocatorSystemDefault;
                    allocator->Inc();
                    std::cout << allocator->Self << " inc " << allocator->name << " " << allocator->Val() << "\n";
                    return t;
                }
            };
            
            template<typename T2>
            class _AllocateImpl<T2, true> {
                friend class Allocator;
                friend class RSFoundation::Collection::String;
                static T2 *_Allocate(malloc_zone_t *zone, size_t size) {
                    return static_cast<T2*>(GCAllocator::Allocate(size * sizeof(T2)));;
                }
            };
        public:
            
            template<typename T2>
            T2 *Reallocate(void *p, size_t size) {
                static_assert(POD<T2>::Result, "");
                T2 *ptr = static_cast<T2*>(GCAllocator::Reallocate(p, size * sizeof(T2)));
                return ptr;
            }
            
            void Deallocate(void *ptr) {
                GCAllocator::Free(ptr);
            }
            
            void Deallocate(T *t) {
                t->T::~T();
                GCAllocator::Free(static_cast<void*>(t));
                Dec();
                std::cout << Self << " dec " << name << " " << Val() << "\n";
            }
            
            void Deallocate(const T *t) {
                t->T::~T();
                GCAllocator::Free((void*)(t));
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
            
            bool IsGC() const {
                return false;
            }
        private:
            Allocator() {
                zone = malloc_default_zone();
                Object obj;
                obj.template GetClassName<T>(name);
                obj.template GetClassName<decltype(this)>(Self);
                
//                std::cout << Self << " init with " << name << "\n";
            }
            
            ~Allocator() {
//                std::cout << Self << " dealloc " << name << "\n";
            }
            
        private:
            malloc_zone_t *zone;
            std::string name;
            std::string Self;
        };

        
        template<typename T>
        Allocator<T> Allocator<T>::AllocatorSystemDefault = Allocator<T>();
        
        template<typename T>
        Allocator<T> Allocator<T>::AllocatorDefault = Allocator<T>();
    }
}

#endif /* defined(__RSCoreFoundation__Allocator__) */