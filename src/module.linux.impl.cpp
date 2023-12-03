#include "module.linux.impl.hpp"

namespace lime
{
    // Thanks to Andrey Belykh (https://stackoverflow.com/a/57099317)
    std::uint32_t module::impl::gnu_symbol_count(ElfW(Addr) address)
    {
        struct header_t
        {
            std::uint32_t n_buckets;
            std::uint32_t sym_offset;
            std::uint32_t bloom_size;
            std::uint32_t bloom_shift;
        };

        auto *header               = reinterpret_cast<header_t *>(address);
        const auto buckets_address = address + sizeof(header_t) + sizeof(std::uintptr_t) * header->bloom_size;

        auto last_symbol     = 0u;
        auto *bucket_address = reinterpret_cast<std::uint32_t *>(buckets_address);

        for (auto i = 0u; header->n_buckets > i; i++)
        {
            auto bucket = *bucket_address;

            if (last_symbol < bucket)
            {
                last_symbol = bucket;
            }

            bucket_address++;
        }

        if (last_symbol < header->sym_offset)
        {
            return header->sym_offset;
        }

        auto chain_base = buckets_address + sizeof(std::uint32_t) * header->n_buckets;

        while (true)
        {
            auto *entry = reinterpret_cast<std::uint32_t *>(chain_base +
                                                            (last_symbol - header->sym_offset) * sizeof(std::uint32_t));
            last_symbol++;

            if (*entry & 1)
            {
                break;
            }
        }

        return last_symbol;
    }

    void module::impl::iterate_symbols(const std::function<bool(const std::string &)> &callback) const
    {
        for (auto i = 0u; info.dlpi_phnum > i; i++)
        {
            if (info.dlpi_phdr[i].p_type != PT_DYNAMIC)
            {
                continue;
            }

            auto *dyn = reinterpret_cast<ElfW(Dyn) *>(info.dlpi_addr + info.dlpi_phdr[i].p_vaddr);

            ElfW(Sym) * symbols{};
            ElfW(Word) * hash_ptr{};

            char *string_table{};
            std::size_t symbol_count{};

            while (dyn->d_tag != DT_NULL)
            {
                if (dyn->d_tag == DT_HASH)
                {
                    hash_ptr     = reinterpret_cast<decltype(hash_ptr)>(dyn->d_un.d_ptr);
                    symbol_count = hash_ptr[1];
                }

                if (!symbol_count && dyn->d_tag == DT_GNU_HASH)
                {
                    symbol_count = gnu_symbol_count(dyn->d_un.d_ptr);
                }

                if (dyn->d_tag == DT_STRTAB)
                {
                    string_table = reinterpret_cast<char *>(dyn->d_un.d_ptr);
                }

                if (dyn->d_tag == DT_SYMTAB)
                {
                    symbols = reinterpret_cast<decltype(symbols)>(dyn->d_un.d_ptr);

                    for (auto j = 0u; symbol_count > j; j++)
                    {
                        if (!symbols[j].st_name)
                        {
                            continue;
                        }

                        if (symbols[j].st_other != 0)
                        {
                            continue;
                        }

                        if (callback(&string_table[symbols[j].st_name]))
                        {
                            return;
                        }
                    }
                }

                dyn++;
            }
        }
    }
} // namespace lime
