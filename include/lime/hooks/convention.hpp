#pragma once

namespace lime
{
    enum class convention
    {
        automatic,
        cdecl,
        stdcall,
        fastcall,
        thiscall,
    };

    namespace detail
    {
        template <typename T, convention Convention>
        struct calling_convention;
    };
} // namespace lime

#include "convention.inl"
