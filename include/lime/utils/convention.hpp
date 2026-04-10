#pragma once

#include <cstdint>

namespace lime::utils
{
    enum class calling_convention : std::uint8_t
    {
        cc_generic,
        cc_cdecl,
        cc_stdcall,
        cc_fastcall,
        cc_thiscall,
        cc_vectorcall,
    };

    template <typename, auto>
    struct convention_traits;
} // namespace lime::utils

#include "convention.inl"
