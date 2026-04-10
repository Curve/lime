#pragma once

#include "lib.hpp"

#include <elf.h>
#include <link.h>

namespace lime::utils
{
    [[nodiscard]] std::uint32_t gnu_symbol_count(ElfW(Addr));
    const char *iter_sym(dl_phdr_info info, const lib::sym_predicate &);
} // namespace lime::utils
