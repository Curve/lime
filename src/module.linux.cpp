#include "module.hpp"
#include "module.linux.impl.hpp"

#include <link.h>
#include <dlfcn.h>

#include <algorithm>

namespace lime
{
    module::module() :m_impl(std::make_unique<impl>()) {}

    module::~module() = default;

    module::module(const module &other) :m_impl(std::make_unique<impl>())
    {
        *m_impl = *other.m_impl;
    }

    module::module(module &&other) noexcept :m_impl(std::move(other.m_impl)) {}

    std::string module::name() const
    {
        return m_impl->info.dlpi_name;
    }

    std::size_t module::size() const
    {
        std::size_t rtn{};

        for (auto i = 0; m_impl->info.dlpi_phnum > i; i++)
        {
            rtn += m_impl->info.dlpi_phdr[i].p_memsz;
        }

        return rtn;
    }

    std::uintptr_t module::end() const
    {
        return start() + size();
    }

    std::uintptr_t module::start() const
    {
        return m_impl->info.dlpi_addr + m_impl->info.dlpi_phdr->p_vaddr;
    }

    std::vector<lime::symbol> module::symbols() const
    {
        std::vector<lime::symbol> rtn;

        m_impl->iterate_symbols([&](const std::string &name) {
            rtn.emplace_back(lime::symbol{.name = name, .address = symbol(name)});
            return false;
        });

        return rtn;
    }

    std::uintptr_t module::symbol(const std::string &name) const
    {
        if (!m_impl->handle)
        {
            return 0;
        }

        auto *addr = dlsym(m_impl->handle, name.c_str());
        return reinterpret_cast<std::uintptr_t>(addr);
    }

    std::optional<std::uintptr_t> module::find_symbol(const std::string &name) const
    {
        std::uintptr_t rtn{0};

        m_impl->iterate_symbols([&](const std::string &item) {
            if (item.find(name) == std::string::npos)
            {
                return false;
            }

            rtn = symbol(item);
            return true;
        });

        if (!rtn)
        {
            return std::nullopt;
        }

        return rtn;
    }

    std::vector<module> module::modules()
    {
        std::vector<module> rtn;

        auto callback = [](dl_phdr_info *info, std::size_t, void *data) {
            auto &res = *reinterpret_cast<decltype(rtn) *>(data);

            if (!info)
            {
                return 0;
            }

            module current;

            current.m_impl->info = *info;
            current.m_impl->handle = dlopen(current.name().c_str(), RTLD_NOLOAD | RTLD_LAZY);

            res.emplace_back(std::move(current));

            return 0;
        };

        dl_iterate_phdr(callback, &rtn);
        return rtn;
    }

    std::optional<module> module::get(const std::string &name)
    {
        auto all = modules();
        auto module = std::find_if(all.begin(), all.end(), [&](auto &item) { return item.name() == name; });

        if (module == all.end())
        {
            return std::nullopt;
        }

        return std::move(*module);
    }

    std::optional<module> module::find(const std::string &name)
    {
        auto all = modules();

        constexpr auto npos = std::string::npos;
        auto module = std::find_if(all.begin(), all.end(), [&](auto &item) { return item.name().find(name) != npos; });

        if (module == all.end())
        {
            return std::nullopt;
        }

        return std::move(*module);
    }
} // namespace lime