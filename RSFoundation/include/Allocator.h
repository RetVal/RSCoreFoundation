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

namespace RSFoundation {
    namespace Basic {
        template<typename T>
        class Allocator : public Object, private Counter<UInt32> {
        public:
            static Allocator<T> AllocatorSystemDefault;
            static Allocator<T> AllocatorDefault;
            
        public:
            template<typename ...Args>
            T *Allocate(Args... args) {
                void *ptr = malloc_zone_malloc(zone, sizeof(T));
                T *t = new (ptr) T(args...);
                Inc();
                return t;
            }
            
            template<typename T2>
            T2 *Allocate(size_t size) {
                if (POD<T2>::Result) {
                    return static_cast<T2*>(malloc_zone_malloc(zone, size * sizeof(T2)));
                }
                return new T2[size];
            }
            
            template<typename T2>
            T2 *Reallocate(void *p, size_t size) {
                static_assert(POD<T2>::Result, "");
                T2 *ptr = static_cast<T2*>(malloc_zone_realloc(zone, p, size * sizeof(T2)));
                return ptr;
            }
            
            void Deallocate(void *ptr) {
                malloc_zone_free(zone, ptr);
            }
            
            void Deallocate(T *t) {
                t->T::~T();
                malloc_zone_free(zone, static_cast<void*>(t));
                Dec();
            }

            void Deallocate(Nullable<T> &obj) {
                if (obj) {
                    Nullable<T> &temp = MoveValue(obj);
                    Deallocate(temp.GetValue());
                }
            }
            
            void Deallocate(nullptr_t nilPtr) {
                
            }
            
        private:
            Allocator() {
                zone = malloc_default_zone();
                name = typeid(T).name();
            }
            
        private:
            malloc_zone_t *zone;
            const char *name;
        };
        
        template<typename T>
        Allocator<T> Allocator<T>::AllocatorSystemDefault = Allocator<T>();
        
        template<typename T>
        Allocator<T> Allocator<T>::AllocatorDefault = Allocator<T>();
    }
}

#endif /* defined(__RSCoreFoundation__Allocator__) */