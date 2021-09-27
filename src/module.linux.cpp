#include <algorithm>
#include <dlfcn.h>
#include <link.h>
#include <module.hpp>

namespace lime
{
    struct module::impl
    {
        void *handle;
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
} // namespace lime