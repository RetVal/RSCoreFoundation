//
//  RSBasicObject.cpp
//  RSCoreFoundation
//
//  Created by closure on 5/26/15.
//  Copyright (c) 2015 RetVal. All rights reserved.
//

#include "Object.h"
#include <iostream>

namespace RSFoundation {
    namespace Basic {
        NotCopyable::NotCopyable() {
        }
        
        NotCopyable::NotCopyable(const NotCopyable &) {
        }
        
        NotCopyable& NotCopyable::operator=(const NotCopyable &) {
            return *this;
        }
        
        
        std::string string_format(const std::string fmt_str, ...) {
            int final_n, n = ((int)fmt_str.size()) * 2; /* Reserve two times as much as the length of the fmt_str */
            std::string str;
            std::unique_ptr<char[]> formatted;
            va_list ap;
            while(1) {
                formatted.reset(new char[n]); /* Wrap the plain char array into the unique_ptr */
                strcpy(&formatted[0], fmt_str.c_str());
                va_start(ap, fmt_str);
                final_n = vsnprintf(&formatted[0], n, fmt_str.c_str(), ap);
                va_end(ap);
                if (final_n < 0 || final_n >= n)
                    n += abs(final_n - n + 1);
                else
                    break;
            }
            return std::string(formatted.get());
        }
        
//        void Object::GetClassName(std::string &className) const {
//            char *output_buffer = nullptr;
//            size_t len = 0;
//            int status = 0;
//            const std::type_info& typeinfo = typeid(*this);
//            __cxa_demangle(typeinfo.name(), nullptr, &len, &status);
//            if (len) {
//                output_buffer = new char[len + 1];
//                assert(output_buffer != nullptr && "尼玛");
//                __cxa_demangle(typeinfo.name(), output_buffer, &len, &status);
//                assert(status == 0 && "尼玛");
//                className = std::string(output_buffer);
//                delete[] output_buffer;
//            } else {
//                className = std::string(typeinfo.name());
//            }
//        }
//        
//        void Object::Description(std::string &desc, unsigned int indent, bool debug) const {
//            if (debug) {
//                std::string cn;
//                GetClassName(cn);
//                for (unsigned int i = 0; i < indent; ++i) {
//                    desc.append("\t");
//                }
//                desc += std::move(string_format("<%s %p>", cn.c_str(), this));
//            }
//        }
//        
//        void Object::Dump() const {
//            std::string desc;
//            Description(desc, 0, true);
//            desc.append("\t");
//            std::cout << desc << std::endl;
//        }
        Object::Object() {
            
        }
        
        Object::~Object() {
        }
    }
}
