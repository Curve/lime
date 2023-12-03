#pragma once
#include "module.hpp"

#include <elf.h>
#include <link.h>
#include <functional>

namespace lime
{
    struct module::impl
    {
        void *handle;
        dl_phdr_info info;

      public:
        void iterate_symbols(const std::function<bool(const std::string &)> &) const;

      public:
        static std::uint32_t gnu_symbol_count(ElfW(Addr));
    };
} // namespace lime
