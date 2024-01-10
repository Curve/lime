#pragma once

#include <cstddef>
#include <cstdint>

namespace lime
{
    enum class architecture
    {
        x86,
        x64,
    };

    static constexpr inline architecture arch = (sizeof(std::uintptr_t)) == 8 ? architecture::x64 : architecture::x86;
    static constexpr inline auto max_instruction_size = 0x15;

    enum size : std::size_t
    {
        jmp_near = sizeof(std::int32_t) + 1,
        jmp_far  = arch == architecture::x86 ? jmp_near : sizeof(std::uintptr_t) + 6,
    };
} // namespace lime
