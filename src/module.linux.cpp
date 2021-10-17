#include <algorithm>
#include <dlfcn.h>
#include <link.h>
#include <module.hpp>

namespace lime
{
    struct module::impl
    {
        void *handle;
        dl_phdr_info info;
    };

    module::~module() = default;
    module::module(module &&) noexcept = default;
    module::module() : m_impl(std::make_unique<impl>()), m_size(0), m_start(0) {}

    std::string module::get_name() const
    {
        return m_name;
    }
    std::size_t module::get_size() const
    {
        return m_size;
    }
    std::uintptr_t module::get_start() const
    {
        return m_start;
    }
    std::uintptr_t module::get_symbol(const std::string &name) const
    {
        if (m_impl->handle)
        {
            const auto *sym = dlsym(m_impl->handle, name.c_str());
            if (sym)
            {
                return reinterpret_cast<std::uintptr_t>(sym);
            }
        }

        return 0;
    }

    std::vector<module> module::get_modules()
    {
        std::vector<module> rtn;
        dl_iterate_phdr(
            [](dl_phdr_info *info, std::size_t, void *data) {
                auto &rtn = *reinterpret_cast<std::vector<module> *>(data);
                if (info)
                {
                    module module;
                    module.m_name = info->dlpi_name;
                    module.m_start = info->dlpi_addr + info->dlpi_phdr->p_vaddr;

                    module.m_impl->info = *info;
                    module.m_impl->handle = dlopen(module.m_name.c_str(), RTLD_NOLOAD | RTLD_LAZY);

                    for (auto i = 0; info->dlpi_phnum > i; i++)
                    {
                        module.m_size += info->dlpi_phdr[i].p_memsz;
                    }

                    rtn.emplace_back(std::move(module));
                }
                return 0;
            },
            &rtn);

        return rtn;
    }

    std::optional<module> module::get(const std::string &name)
    {
        //? dlinfo doesn't supply us with enough information, that's why we have to rely on dl_iterate_phdr
        auto modules = get_modules();
        auto module = std::find_if(modules.begin(), modules.end(), [&name](const auto &item) { return item.m_name == name; });

        if (module != modules.end())
        {
            return std::move(*module);
        }

        return std::nullopt;
    }

    std::optional<module> module::find(const std::string &name)
    {
        for (auto &module : get_modules())
        {
            if (module.get_name().find(name) != std::string::npos)
            {
                return std::move(module);
            }
        }

        return std::nullopt;
    }

    std::uint32_t GetNumberOfSymbolsFromGnuHash(Elf64_Addr gnuHashAddress)
    {
        using Header = struct
        {
            std::uint32_t nbuckets;
            std::uint32_t symoffset;
            std::uint32_t bloom_size;
            std::uint32_t bloom_shift;
        };

        auto *header = reinterpret_cast<Header *>(gnuHashAddress);
        auto bucketsAddress = reinterpret_cast<std::uintptr_t>(gnuHashAddress + sizeof(Header) + (sizeof(std::uint64_t) * header->bloom_size));

        std::uint32_t lastSymbol = 0;
        auto *bucketAddress = reinterpret_cast<std::uint32_t *>(bucketsAddress);

        for (std::uint32_t i = 0; i < header->nbuckets; ++i)
        {
            std::uint32_t bucket = *bucketAddress;
            if (lastSymbol < bucket)
            {
                lastSymbol = bucket;
            }
            bucketAddress++;
        }

        if (lastSymbol < header->symoffset)
        {
            return header->symoffset;
        }

        auto chainBaseAddress = reinterpret_cast<std::uintptr_t>(bucketAddress) + (sizeof(std::uint32_t) * header->nbuckets);

        while (true)
        {
            auto *chainEntry = reinterpret_cast<std::uint32_t *>((chainBaseAddress + (lastSymbol - header->symoffset) * sizeof(std::uint32_t)));

            lastSymbol++;

            if (*chainEntry & 1)
            {
                break;
            }
        }

        return lastSymbol;
    }

    std::vector<std::pair<std::string, std::uintptr_t>> module::get_symbols() const
    {
        std::vector<std::pair<std::string, std::uintptr_t>> rtn;

        for (auto i = 0u; m_impl->info.dlpi_phnum > i; i++)
        {
            if (m_impl->info.dlpi_phdr[i].p_type == PT_DYNAMIC)
            {
                auto *dyn = reinterpret_cast<ElfW(Dyn) *>(m_impl->info.dlpi_addr + m_impl->info.dlpi_phdr[i].p_vaddr);

                char *string_table{};
                ElfW(Sym *) sym_entry{};
                ElfW(Word *) hash_ptr{};
                auto symbol_count = 0u;

                while (dyn->d_tag != DT_NULL)
                {
                    if (dyn->d_tag == DT_HASH)
                    {
                        hash_ptr = reinterpret_cast<ElfW(Word *)>(dyn->d_un.d_ptr);
                        symbol_count = hash_ptr[1];
                    }
                    else if (dyn->d_tag == DT_GNU_HASH && symbol_count == 0)
                    {
                        symbol_count = GetNumberOfSymbolsFromGnuHash(dyn->d_un.d_ptr);
                    }
                    else if (dyn->d_tag == DT_STRTAB)
                    {
                        string_table = reinterpret_cast<char *>(dyn->d_un.d_ptr);
                    }
                    else if (dyn->d_tag == DT_SYMTAB)
                    {
                        sym_entry = reinterpret_cast<ElfW(Sym *)>(dyn->d_un.d_ptr);

                        for (auto sym_index = 0u; sym_index < symbol_count; sym_index++)
                        {
                            std::pair<std::string, std::uintptr_t> entry;
                            entry.first = std::string(&string_table[sym_entry[sym_index].st_name]);
                            entry.second = reinterpret_cast<std::uintptr_t>(dlsym(m_impl->handle, entry.first.c_str()));

                            rtn.emplace_back(entry);
                        }
                    }

                    dyn++;
                }
            }
        }

        return rtn;
    }

    std::optional<std::uintptr_t> module::find_symbol(const std::string &name) const
    {
        auto symbols = get_symbols();
        auto symbol = std::find_if(symbols.begin(), symbols.end(), [&name](const auto &item) { return item.first.find(name) != std::string::npos; });

        if (symbol != symbols.end())
        {
            return symbol->second;
        }

        return std::nullopt;
    }
} // namespace lime