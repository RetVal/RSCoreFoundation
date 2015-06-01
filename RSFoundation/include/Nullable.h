//
//  RSBasicNullable.h
//  RSCoreFoundation
//
//  Created by closure on 5/27/15.
//  Copyright (c) 2015 RetVal. All rights reserved.
//

#ifndef __RSCoreFoundation__RSBasicNullable__
#define __RSCoreFoundation__RSBasicNullable__

#include <RSFoundation/Object.h>

namespace RSFoundation {
    namespace Basic {
        template<typename T>
        class Nullable {
        private:
            typedef T* TRef;
            TRef object;
        public:
            Nullable() : object(nullptr) {
            }
            
            Nullable(const T& value) : object(new T(value)) {
            }
            
            Nullable(T&& value) : object(new T(MoveValue(value))) {
            }
            
            Nullable(const Nullable<T>& value) : object(value.object ? new T(*value.object) : nullptr) {
            }
            
            Nullable(Nullable<T>&& value) : object(value.object) {
                value.object = nullptr; // move
            }
            
            ~Nullable() {
                if (object) {
                    delete object;
                    object = nullptr;
                }
            }
            
            Nullable<T>& operator=(const T& value) {
                if (object) {
                    delete object;
                    object = nullptr;
                }
                object = new T(value);
                return *this;
            }
            
            Nullable<T>& operator=(const Nullable<T>& value) {
                if (this != &value) {
                    if (object) {
                        delete object;
                        object = nullptr;
                    }
                    if (value.object) {
                        object = new T(*value.object);
                    }
                }
                return *this;
            }
            
            Nullable<T>& operator=(Nullable<T>&& value) {
                if (this != &value) {
                    if (object) {
                        delete object;
                        object = nullptr;
                    }
                    object = value.object;
                    value.object = nullptr;
                }
                return *this;
            }
            
            static bool IsEqualTo(const Nullable<T>& a, const Nullable<T>& b) {
                return a.object
                ? b.object
                    ? *a.object == *b.object
                    : false
                :b.object
                    ? false
                    :true;
            }
            
            static ComparisonResult Compare(const Nullable<T>& a, const Nullable<T>& b) {
                return a.object
                ? b.object
                    ? (*a.object == *b.object ? EqualTo : *a.object < *b.object ? LessThan : GreaterThan)
                : GreaterThan
                : b.object
                ? LessThan
                : EqualTo;
            }
            
            bool operator==(const Nullable<T>& value) const {
                return IsEqualTo(*this, value);
            }
            
            bool operator!=(const Nullable<T>& value) const {
                return !IsEqualTo(*this, value);
            }
            
            
            
            bool operator>(const Nullable<T>& value) const {
                return Compare(*this, value) == GreaterThan;
            }
            
            bool operator>=(const Nullable<T>& value) const {
                return Compare(*this, value) != LessThan;
            }
            
            bool operator<(const Nullable<T>& value) const {
                return Compare(*this, value) == LessThan;
            }
            
            bool operator<=(const Nullable<T>& value) const {
                return Compare(*this, value) != GreaterThan;
            }
            
            operator bool() const {
                return object != nullptr;
            }
            
            const T& GetValue() const {
                return *object;
            }
        };
    }
}

#endif /* defined(__RSCoreFoundation__RSBasicNullable__) */
