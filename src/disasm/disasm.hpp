#pragma once
#include <cstdint>
#include <optional>
#include <vector>

namespace lime
{
    namespace disasm
    {
        bool is_far_relocateable(const std::vector<std::uint8_t> &);
        std::size_t get_required_prologue_length(const std::uintptr_t &, const std::size_t &);

        std::size_t get_estimated_size(const std::vector<std::uint8_t> &, const bool &);
        std::optional<std::vector<std::uint8_t>> build_code(const std::vector<std::uint8_t> &, const std::uintptr_t &, const std::uintptr_t &,
                                                            const std::uintptr_t &);
    } // namespace disasm
} // namespace lime