//
//  RSTypeTraits.h
//  RSCoreFoundation
//
//  Created by closure on 5/26/15.
//  Copyright (c) 2015 RetVal. All rights reserved.
//

#ifndef __RSCoreFoundation__RSTypeTraits__
#define __RSCoreFoundation__RSTypeTraits__

#include <RSFoundation/BasicTypeDefine.hpp>

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
        
        template <class _Tp, _Tp __v>
        struct integral_constant {
            static constexpr const _Tp      value = __v;
            typedef _Tp               value_type;
            typedef integral_constant type;
            
            inline constexpr operator value_type() const noexcept {return value;}
            inline constexpr value_type operator ()() const noexcept {return value;}
        };
        
        template <class _Tp, _Tp __v>
        constexpr const _Tp integral_constant<_Tp, __v>::value;
        
        typedef integral_constant<bool, true>  true_type;
        typedef integral_constant<bool, false> false_type;
        
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
        

        namespace __is_base_of_impl {
            template <typename B, typename D>
            struct Host
            {
                operator B*() const;
                operator D*();
            };
        }
        
        template <typename B, typename D>
        struct is_base_of {
            typedef char (&yes)[1];
            typedef char (&no)[2];
            
            template <typename T>
            static yes check(D*, T);
            static no check(B*, int);
            
            static const bool value = sizeof(check(__is_base_of_impl::Host<B,D>(), int())) == sizeof(yes);
        };
    }
}

#endif /* defined(__RSCoreFoundation__RSTypeTraits__) */
