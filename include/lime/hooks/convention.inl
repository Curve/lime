#pragma once

#include "convention.hpp"

#include <utility>
#include <cstdint>

namespace lime::detail
{
    template <typename Ret, typename... Args>
    struct calling_convention<Ret(Args...), convention::automatic>
    {
        using add = Ret(Args...);

        template <auto Function>
        static Ret wrapper(Args... args)
        {
            return Function(std::forward<Args>(args)...);
        }
    };

#if INTPTR_MAX == INT32_MAX

#ifdef _MSC_VER
#define LIME_CDECL    __cdecl
#define LIME_STDCALL  __stdcall
#define LIME_FASTCALL __fastcall
#define LIME_THISCALL __thiscall
#else
#define LIME_CDECL    __attribute__((cdecl))
#define LIME_STDCALL  __attribute__((stdcall))
#define LIME_FASTCALL __attribute__((fastcall))
#define LIME_THISCALL __attribute__((thiscall))
#endif

#ifdef __GNUC__
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wattributes"
#endif

    template <typename Ret, typename... Args>
    struct calling_convention<Ret(Args...), convention::c_cdecl>
    {
        using add = Ret LIME_CDECL(Args...);

        template <auto Function>
        static Ret LIME_CDECL wrapper(Args... args)
        {
            return Function(std::forward<Args>(args)...);
        }
    };

    template <typename Ret, typename... Args>
    struct calling_convention<Ret(Args...), convention::c_stdcall>
    {
        using add = Ret LIME_STDCALL(Args...);

        template <auto Function>
        static Ret LIME_STDCALL wrapper(Args... args)
        {
            return Function(std::forward<Args>(args)...);
        }
    };

    template <typename Ret, typename... Args>
    struct calling_convention<Ret(Args...), convention::c_fastcall>
    {
        using add = Ret LIME_FASTCALL(Args...);

        template <auto Function>
        static Ret LIME_FASTCALL wrapper(Args... args)
        {
            return Function(std::forward<Args>(args)...);
        }
    };

    template <typename Ret, typename... Args>
    struct calling_convention<Ret(Args...), convention::c_thiscall>
    {
        using add = Ret LIME_THISCALL(Args...);

        template <auto Function>
        static Ret LIME_THISCALL wrapper(Args... args)
        {
            return Function(std::forward<Args>(args)...);
        }
    };

#ifdef __GNUC__
#pragma GCC diagnostic pop
#endif

#endif

#undef LIME_CDECL
#undef LIME_STDCALL
#undef LIME_FASTCALL
#undef LIME_THISCALL
} // namespace lime::detail
