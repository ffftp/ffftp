//
// Copyright (C) Microsoft Corporation
// All rights reserved.
//
// This file is for internal WRL usage only
//
// Backport to Windows 7 SDK by Kurata Sayuri.

#ifndef _WRL_INTERNAL_H_
#define _WRL_INTERNAL_H_

#ifdef _MSC_VER
#pragma once
#endif  // _MSC_VER

#ifndef __WRL_ASSERT__
// Take CRT assert as default
#include <crtdbg.h>
#define __WRL_ASSERT__(cond)    _ASSERTE(cond)
#endif // __WRL_ASSERT__

// Include common headers
#include <windows.h>

// Helper types
namespace Microsoft {
namespace WRL {
namespace Details {

    struct BoolStruct
    {
        int Member;
    };

    typedef int BoolStruct::* BoolType;

    inline void __declspec(noreturn) RaiseException(HRESULT hr, DWORD dwExceptionFlags = EXCEPTION_NONCONTINUABLE) throw()
    {
        ::RaiseException(static_cast<DWORD>(hr), dwExceptionFlags, 0, NULL);
    }

    template <bool b, typename T = void>
    struct EnableIf
    {
    };

    template <typename T>
    struct EnableIf<true, T>
    {
        typedef T type;
    };

    template <typename T1, typename T2>
    struct IsSame
    {
        static const bool value = false;
    };

    template <typename T1>
    struct IsSame<T1, T1>
    {
        static const bool value = true;
    };

    template<class T>
    struct RemoveReference
    {
        typedef T Type;
    };

    template<class T>
    struct RemoveReference<T&>
    {
        typedef T Type;
    };

    template<class T>
    struct RemoveReference<T&&>
    {
        typedef T Type;
    };

    template<class T>
    inline typename RemoveReference<T>::Type&& Move(_Inout_ T&& arg) throw()
    {
        return ((typename RemoveReference<T>::Type&&)arg);
    }

    template<class T>
    inline void Swap(_Inout_ T& left, _Inout_ T& right) throw()
    {
        T tmp = Move(left);
        left = Move(right);
        right = Move(tmp);
    }

    // Disables template argument deduction from Forward helper
    template<class T>
    struct Identity
    {
        // Map T to type unchanged
        typedef T Type;
    };

    template<class T> 
    inline T&& Forward(typename Identity<T>::Type& arg) throw()
    {
        // Forward arg, given explicitly specified Type parameter
        return (T&&)arg;
    }

    template <typename Base, typename Derived>
    struct IsBaseOfStrict
    {
        static const bool value = __is_base_of(Base, Derived);
    };

    template <typename Base>
    struct IsBaseOfStrict<Base, Base>
    {
        static const bool value = false;
    };

}}} // namespace Microsoft::WRL::Details

#endif // _WRL_INTERNAL_H_
