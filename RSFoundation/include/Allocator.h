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

#include <malloc/malloc.h>

namespace RSFoundation {
    namespace Basic {
        template<typename T>
        class Allocator : public Object {
        public:
            Allocator() {
                zone = malloc_default_zone();
            }
            static Allocator<T> AllocatorSystemDefault;
            static Allocator<T> AllocatorDefault;
            
        public:
            template<typename ...Args>
            T *Allocate(Args... args, size_t size = sizeof(T)) {
                void *ptr = malloc_zone_malloc(zone, size);
                T *t = new (ptr) T(args...);
                return t;
            }
            
            void Deallocate(void *ptr) {
                malloc_zone_free(zone, ptr);
            }
            
            void Deallocate(T *t) {
                t->T::~T();
                malloc_zone_free(zone, static_cast<void*>(t));
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
            malloc_zone_t *zone;
        };
        
        template<typename T>
        Allocator<T> Allocator<T>::AllocatorSystemDefault = Allocator<T>();
        
        template<typename T>
        Allocator<T> Allocator<T>::AllocatorDefault = Allocator<T>();
    }
}

#endif /* defined(__RSCoreFoundation__Allocator__) */