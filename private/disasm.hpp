#pragma once

#include <vector>
#include <cstdint>
#include <optional>

namespace lime
{
    struct imm;
    struct disp;
} // namespace lime

namespace lime::disasm
{
    bool valid(std::uintptr_t);
    bool relative(std::uintptr_t);
    bool branching(std::uintptr_t);

    std::size_t size(std::uintptr_t);
    std::size_t mnemonic(std::uintptr_t);

    disp displacement(std::uintptr_t);
    std::vector<imm> immediates(std::uintptr_t);
    std::optional<std::uintptr_t> follow(std::uintptr_t);
} // namespace lime::disasm
