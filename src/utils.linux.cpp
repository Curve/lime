#include "utils.linux.hpp"

#include <algorithm>

namespace lime
{
    std::uint32_t utils::gnu_symbol_count(ElfW(Addr) address)
    {
        // Thanks to Andrey Belykh (https://stackoverflow.com/a/57099317)

        struct header_t
        {
            std::uint32_t n_buckets;
            std::uint32_t sym_offset;
            std::uint32_t bloom_size;
            std::uint32_t bloom_shift;
        };

        const auto *const header   = reinterpret_cast<header_t *>(address);
        const auto buckets_address = address + sizeof(header_t) + (sizeof(std::uintptr_t) * header->bloom_size);

        auto last_sym              = 0uz;
        const auto *bucket_address = reinterpret_cast<std::uint32_t *>(buckets_address);

        for (auto i = 0uz; header->n_buckets > i; ++i, ++bucket_address)
        {
            const auto bucket = *bucket_address;
            last_sym          = std::max<std::size_t>(last_sym, bucket);
        }

        if (last_sym < header->sym_offset)
        {
            return header->sym_offset;
        }

        const auto chain_base = buckets_address + (sizeof(std::uint32_t) * header->n_buckets);
        const auto get_entry  = [chain_base, header](auto sym)
        {
            return reinterpret_cast<std::uint32_t *>(chain_base + ((sym - header->sym_offset) * sizeof(std::uint32_t)));
        };

        for (const auto *entry = get_entry(last_sym); *entry & 1; entry = get_entry(++last_sym))
        {
        }

        return last_sym;
    }

    const char *utils::iter_sym(dl_phdr_info info, const lib::sym_predicate &pred)
    {
        for (auto i = 0uz; info.dlpi_phnum > i; ++i)
        {
            if (info.dlpi_phdr[i].p_type != PT_DYNAMIC)
            {
                continue;
            }

            const auto *dyn      = reinterpret_cast<ElfW(Dyn) *>(info.dlpi_addr + info.dlpi_phdr[i].p_vaddr);
            const auto *symbols  = std::add_pointer_t<ElfW(Sym)>{};
            const auto *hash_ptr = std::add_pointer_t<ElfW(Word)>{};

            auto sym_count           = 0uz;
            const auto *string_table = std::add_pointer_t<char>{};

            for (; dyn->d_tag != DT_NULL; ++dyn)
            {
                if (dyn->d_tag == DT_HASH)
                {
                    hash_ptr  = reinterpret_cast<decltype(hash_ptr)>(dyn->d_un.d_ptr);
                    sym_count = hash_ptr[1];
                }

                if (!sym_count && dyn->d_tag == DT_GNU_HASH)
                {
                    sym_count = gnu_symbol_count(dyn->d_un.d_ptr);
                }

                if (dyn->d_tag == DT_STRTAB)
                {
                    string_table = reinterpret_cast<char *>(dyn->d_un.d_ptr);
                }

                if (dyn->d_tag != DT_SYMTAB)
                {
                    continue;
                }

                symbols = reinterpret_cast<decltype(symbols)>(dyn->d_un.d_ptr);

                for (auto j = 0uz; sym_count > j; j++)
                {
                    if (!symbols[j].st_name)
                    {
                        continue;
                    }

                    if (symbols[j].st_other != 0)
                    {
                        continue;
                    }

                    const auto *const name = &string_table[symbols[j].st_name];

                    if (!pred(name))
                    {
                        continue;
                    }

                    return name;
                }
            }
        }

        return nullptr;
    }
} // namespace lime
