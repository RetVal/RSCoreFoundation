//
//  RSBasicObjectBox.h
//  RSCoreFoundation
//
//  Created by closure on 5/27/15.
//  Copyright (c) 2015 RetVal. All rights reserved.
//

#ifndef __RSCoreFoundation__RSBasicObjectBox__
#define __RSCoreFoundation__RSBasicObjectBox__

#include <RSFoundation/Object.hpp>

namespace RSFoundation {
    namespace Basic {

        template<typename T>
        class ObjectBox : public Object {
        private:
            T object;
        public:
            ObjectBox(const T& _obj) : object(_obj) {
            }
            
            ObjectBox(T&& _obj) : object(MoveValue(_obj)) {
            }
            
            ObjectBox(const ObjectBox<T> &_obj) : object(_obj.object) {
            }
            
            ObjectBox(ObjectBox<T>&& _obj) : object(MoveValue(_obj.object)) {
            }
            
            ObjectBox<T>& operator=(const ObjectBox<T>& value) {
                object = value.object;
                return *this;
            }
            
            ObjectBox<T>& operator=(ObjectBox<T>&& value) {
                object = MoveValue(value.object);
                return *this;
            }
            
            const T& Unbox() const {
                return object;
            }
        };
    }
}

#endif /* defined(__RSCoreFoundation__RSBasicObjectBox__) */
