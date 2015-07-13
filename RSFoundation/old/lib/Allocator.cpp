//
//  Allocator.cpp
//  RSCoreFoundation
//
//  Created by closure on 5/27/15.
//  Copyright (c) 2015 RetVal. All rights reserved.
//

#include "Allocator.hpp"

#if defined(RS_FOUNDATION_GC)
#ifdef GC_NAME_CONFLICT
# define USE_GC GC_NS_QUALIFY(UseGC)
struct foo * GC;
#else
# define USE_GC GC_NS_QUALIFY(GC)
#endif

#include "gc_cpp.h"
#include "gc.h"
#include "gc_allocator.h"
#define GC

#else 
#include "debug_new.h"
#endif

namespace RSFoundation {
    namespace Basic {
        
        void *GCAllocator::Allocate(size_t size) {
#if defined(RS_FOUNDATION_GC)
            return GC_MALLOC(size);
#else 
            return nullptr;
#endif
        }
        
        void *GCAllocator::Reallocate(void *ptr, size_t size) {
#if defined(RS_FOUNDATION_GC)
            return GC_REALLOC(ptr, size);
#else
            return nullptr;
#endif
        }
        
        void GCAllocator::Free(void *ptr) {
#if defined(RS_FOUNDATION_GC)
            return GC_FREE(ptr);
#else
            return;
#endif
        }
        
        size_t GCAllocator::Size(void *ptr) {
#if defined(RS_FOUNDATION_GC)
            return GC_size(ptr);
#else
            return 0;
#endif
        }
        
        void *OSXAllocator::Allocate(size_t size) {
            return new char[size]();
//            return malloc_zone_malloc(malloc_default_zone(), size);
        }
        
        void *OSXAllocator::Reallocate(void *ptr, size_t size) {
            return malloc_zone_realloc(malloc_default_zone(), ptr, size);
        }
        
        void OSXAllocator::Free(void *ptr) {
            delete []ptr;
//            return malloc_zone_free(malloc_default_zone(), ptr);
        }
        
        size_t OSXAllocator::Size(void *ptr) {
            return malloc_size(const_cast<void *>(ptr));
        }
        
        extern "C" void *_malloc(malloc_zone_t *zone, size_t size) {
//            return GC_MALLOC(size);
            return malloc_zone_malloc(zone, size);
        }
        
        extern "C" void *_realloc(malloc_zone_t *zone, void *ptr, size_t size) {
//            return GC_REALLOC(ptr, size);
            return malloc_zone_realloc(zone, ptr, size);
        }
        
        extern "C" void _free(malloc_zone_t *zone, void *ptr) {
//            return GC_FREE(ptr);
            malloc_zone_free(zone, ptr);
        }
        
#if RS_FOUNDATION_GC
        
        class GCObject : public GC_NS_QUALIFY(gc) {
        public:
            GCObject() {
                
            }
            
            ~GCObject() {
                
            }
        };
        
#endif
        
        __RS_FOUNDATION_INIT_ROUTINE(1000) void Initialize() {
#if RS_FOUNDATION_GC
            GC_INIT();
#endif
        }
        
        __RS_FOUNDATION_FINAL_ROUTINE(1000)void Finalize() {
#if defined(RS_FOUNDATION_GC)
            while (GC_collect_a_little()) { }
            for (int i = 0; i < 1; i++) {
                GC_gcollect();
                std::cout << "Completed " << GC_get_gc_no() << " collections" <<std::endl;
                std::cout << "Heap size is " << GC_get_heap_size() << std::endl;
            }
            GC_dump();
            std::cout << "Completed " << GC_get_gc_no() << " collections" <<std::endl;
            std::cout << "Heap size is " << GC_get_heap_size() << std::endl;
            printf("\n***************************************************************\n");
#endif
        }
    }
}
