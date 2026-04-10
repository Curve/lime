#pragma once

#include "convention.hpp"

#include <utility>

#if defined(__clang__)
#define lime_cc(convention) __attribute__((convention))
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunknown-pragmas"
#pragma clang diagnostic ignored "-Wignored-attributes"
#elif defined(__GNUC__)
#define lime_cc(convention) __attribute__((convention))
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wattributes"
#elif defined(_MSC_VER)
#define lime_cc(convention) __##convention
#endif

namespace lime::utils
{
    template <typename R, typename... Ts>
    struct convention_traits<R(Ts...), calling_convention::cc_generic>
    {
        using pointer = R (*)(Ts...);

      public:
        template <auto Fn>
        static R wrapper(Ts... params)
        {
            return Fn(std::forward<Ts>(params)...);
        }
    };

    template <typename R, typename... Ts>
    struct convention_traits<R(Ts...), calling_convention::cc_cdecl>
    {
        using pointer = R(lime_cc(cdecl) *)(Ts...);

      public:
        template <auto Fn>
        static R lime_cc(cdecl) wrapper(Ts... params)
        {
            return Fn(std::forward<Ts>(params)...);
        }
    };

    template <typename R, typename... Ts>
    struct convention_traits<R(Ts...), calling_convention::cc_stdcall>
    {
        using pointer = R(lime_cc(stdcall) *)(Ts...);

      public:
        template <auto Fn>
        static R lime_cc(stdcall) wrapper(Ts... params)
        {
            return Fn(std::forward<Ts>(params)...);
        }
    };

    template <typename R, typename... Ts>
    struct convention_traits<R(Ts...), calling_convention::cc_fastcall>
    {
        using pointer = R(lime_cc(fastcall) *)(Ts...);

      public:
        template <auto Fn>
        static R lime_cc(fastcall) wrapper(Ts... params)
        {
            return Fn(std::forward<Ts>(params)...);
        }
    };

    template <typename R, typename... Ts>
    struct convention_traits<R(Ts...), calling_convention::cc_thiscall>
    {
        using pointer = R(lime_cc(thiscall) *)(Ts...);

      public:
        template <auto Fn>
        static R lime_cc(thiscall) wrapper(Ts... params)
        {
            return Fn(std::forward<Ts>(params)...);
        }
    };

    template <typename R, typename... Ts>
    struct convention_traits<R(Ts...), calling_convention::cc_vectorcall>
    {
        using pointer = R(lime_cc(vectorcall) *)(Ts...);

      public:
        template <auto Fn>
        static R lime_cc(vectorcall) wrapper(Ts... params)
        {
            return Fn(std::forward<Ts>(params)...);
        }
    };
} // namespace lime::utils

#if defined(__clang__)
#pragma clang diagnostic pop
#elif defined(__GNUC__)
#pragma GCC diagnostic pop
#endif

#if defined(lime_cc)
#undef lime_cc
#endif
