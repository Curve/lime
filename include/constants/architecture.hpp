#pragma once
#include <cstdint>

namespace lime
{
    enum class architecture
    {
        x86,
        x64
    };

    constexpr architecture arch = (sizeof(std::uintptr_t)) == 8 ? architecture::x64 : architecture::x86;
} // namespace lime