#pragma once

#include "convention.hpp"

#include <utility>
#include <cstdint>

namespace lime
{
    namespace detail
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

        template <typename Ret, typename... Args>
        struct calling_convention<Ret(Args...), convention::cdecl>
        {
            using add = Ret LIME_CDECL(Args...);

            template <auto Function>
            static Ret LIME_CDECL wrapper(Args... args)
            {
                return Function(std::forward<Args>(args)...);
            }
        };

        template <typename Ret, typename... Args>
        struct calling_convention<Ret(Args...), convention::stdcall>
        {
            using add = Ret LIME_STDCALL(Args...);

            template <auto Function>
            static Ret LIME_STDCALL wrapper(Args... args)
            {
                return Function(std::forward<Args>(args)...);
            }
        };

        template <typename Ret, typename... Args>
        struct calling_convention<Ret(Args...), convention::fastcall>
        {
            using add = Ret LIME_FASTCALL(Args...);

            template <auto Function>
            static Ret LIME_FASTCALL wrapper(Args... args)
            {
                return Function(std::forward<Args>(args)...);
            }
        };

        template <typename Ret, typename... Args>
        struct calling_convention<Ret(Args...), convention::thiscall>
        {
            using add = Ret LIME_THISCALL(Args...);

            template <auto Function>
            static Ret LIME_THISCALL wrapper(Args... args)
            {
                return Function(std::forward<Args>(args)...);
            }
        };
    }; // namespace detail

#undef LIME_CDECL
#undef LIME_STDCALL
#undef LIME_FASTCALL
#undef LIME_THISCALL
#endif
} // namespace lime
