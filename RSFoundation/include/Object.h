//
//  RSBasicObject.h
//  RSCoreFoundation
//
//  Created by closure on 5/26/15.
//  Copyright (c) 2015 RetVal. All rights reserved.
//

#ifndef __RSCoreFoundation__RSBasicObject__
#define __RSCoreFoundation__RSBasicObject__

#include <RSFoundation/BasicTypeDefine.h>
#include <RSFoundation/TypeTraits.h>
#include <string>
#include <cxxabi.h>

namespace RSFoundation {
    using namespace __cxxabiv1;
    namespace Basic {
        class NotCopyable {
        private:
            NotCopyable(const NotCopyable &);
            NotCopyable& operator=(const NotCopyable &);
        public:
            NotCopyable();
        };
        
        template<typename T, size_t _Size = 1>
        class Counter {
            static_assert(POD<T>::Result, "T must be POD");
        private:
            T __elems_[_Size > 0 ? _Size : 1];
            
        public:
            Counter() = default;
            
            void Inc(size_t x = 0) { ++__elems_[x]; }
            void Dec(size_t x = 0) { --__elems_[x]; }
            
            const T& Val(size_t x = 0) const { return __elems_[x]; }
            const T& Val(size_t x = 0) { return __elems_[x]; }
        };
        
        class Object {
        public:
            Object();
            virtual ~Object();
            template <typename T>
            void GetClassName(std::string &className) {
                char *output_buffer = nullptr;
                size_t len = 0;
                int status = 0;
                const std::type_info& typeinfo = typeid(T);
                output_buffer = __cxa_demangle(typeinfo.name(), nullptr, &len, &status);
                if (output_buffer != nullptr) {
                    assert(status == 0 && "尼玛");
                    className = std::string(output_buffer);
                    free(output_buffer);
                } else {
                    className = std::string(typeinfo.name());
                }
            }
            
//            virtual void GetClassName(std::string &className) const;
//            virtual void Description(std::string &desc, unsigned int indent = 0, bool debug = false) const;
//            virtual void Dump() const;
        };
    }
}

#endif /* defined(__RSCoreFoundation__RSBasicObject__) */
