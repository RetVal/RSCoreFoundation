//
//  RSTypeTraits.h
//  RSCoreFoundation
//
//  Created by closure on 5/26/15.
//  Copyright (c) 2015 RetVal. All rights reserved.
//

#ifndef __RSCoreFoundation__RSTypeTraits__
#define __RSCoreFoundation__RSTypeTraits__

#include <RSFoundation/BasicTypeDefine.h>

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
        
        template<typename T> struct POD {
            static const bool Result=false;
        };
        
        template<>struct POD<bool>{static const bool Result=true;};
        template<>struct POD<Int8>{static const bool Result=true;};
        template<>struct POD<UInt8>{static const bool Result=true;};
        template<>struct POD<Int16>{static const bool Result=true;};
        template<>struct POD<UInt16>{static const bool Result=true;};
        template<>struct POD<Int32>{static const bool Result=true;};
        template<>struct POD<UInt32>{static const bool Result=true;};
        template<>struct POD<Int64>{static const bool Result=true;};
        template<>struct POD<UInt64>{static const bool Result=true;};
        template<>struct POD<char>{static const bool Result=true;};
        template<>struct POD<wchar_t>{static const bool Result=true;};
        template<typename T>struct POD<T*>{static const bool Result=true;};
        template<typename T>struct POD<T&>{static const bool Result=true;};
        template<typename T, typename C>struct POD<T C::*>{static const bool Result=true;};
        template<typename T, Int32 _Size>struct POD<T[_Size]>{static const bool Result=POD<T>::Result;};
        template<typename T>struct POD<const T>{static const bool Result=POD<T>::Result;};
        template<typename T>struct POD<volatile T>{static const bool Result=POD<T>::Result;};
        template<typename T>struct POD<const volatile T>{static const bool Result=POD<T>::Result;};
        
        struct YesType {};
        struct NoType {};
        
        template<typename T, typename YesOrNo>
        struct AcceptType{};
        
        template<typename T>
        struct AcceptType<T, YesType> {
            typedef T Type;
        };
        
        template<typename YesOrNo>
        struct AcceptValue {
            static const bool Result = false;
        };
        
        template<>
        struct AcceptValue<YesType> {
            static const bool Result = true;
        };
        
        template<typename TFrom, typename TTo>
        struct RequiresConvertable {
            static YesType Test(TTo *value);
            static NoType Test(void *value);
            typedef decltype(Test((TFrom *)0)) YesNoType;
        };
        
        
    }
}

#endif /* defined(__RSCoreFoundation__RSTypeTraits__) */
