#pragma once

namespace lime
{
    enum class convention
    {
        automatic,
        c_cdecl,
        c_stdcall,
        c_fastcall,
        c_thiscall,
    };

    namespace detail
    {
        template <typename T, convention Convention>
        struct calling_convention;
    };
} // namespace lime

#include "convention.inl"
