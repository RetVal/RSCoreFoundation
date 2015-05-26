//
//  RSTypeTraits.h
//  RSCoreFoundation
//
//  Created by closure on 5/26/15.
//  Copyright (c) 2015 RetVal. All rights reserved.
//

#ifndef __RSCoreFoundation__RSTypeTraits__
#define __RSCoreFoundation__RSTypeTraits__

namespace RSFoundation {
    namespace Basic {
        template<typename T>
        struct RemoveReference {
            typedef T			Type;
        };
        
        template<typename T>
        struct RemoveReference<T&> {
            typedef T			Type;
        };
        
        template<typename T>
        struct RemoveReference<T&&> {
            typedef T			Type;
        };
        
        template<typename T>
        struct RemoveConst {
            typedef T			Type;
        };
        
        template<typename T>
        struct RemoveConst<const T> {
            typedef T			Type;
        };
        
        template<typename T>
        struct RemoveVolatile {
            typedef T			Type;
        };
        
        template<typename T>
        struct RemoveVolatile<volatile T> {
            typedef T			Type;
        };
        
        template<typename T>
        struct RemoveCVR {
            typedef T								Type;
        };
        
        template<typename T>
        struct RemoveCVR<T&> {
            typedef typename RemoveCVR<T>::Type		Type;
        };
        
        template<typename T>
        struct RemoveCVR<T&&> {
            typedef typename RemoveCVR<T>::Type		Type;
        };
        
        template<typename T>
        struct RemoveCVR<const T> {
            typedef typename RemoveCVR<T>::Type		Type;
        };
        
        template<typename T>
        struct RemoveCVR<volatile T> {
            typedef typename RemoveCVR<T>::Type		Type;
        };
        
        template<typename T>
        typename RemoveReference<T>::Type&& MoveValue(T&& value) {
            return (typename RemoveReference<T>::Type&&)value;
        }
        
        template<typename T>
        T&& ForwardValue(typename RemoveReference<T>::Type&& value) {
            return (T&&)value;
        }
        
        template<typename T>
        T&& ForwardValue(typename RemoveReference<T>::Type& value) {
            return (T&&)value;
        }
        
        template<typename ...TArgs>
        struct TypeTuple {
        };
    }
}

#endif /* defined(__RSCoreFoundation__RSTypeTraits__) */
