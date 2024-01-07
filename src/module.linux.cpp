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

    std::string_view module::name() const
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

        auto fn = [&](auto name)
        {
            auto sym = symbol(name);
            auto str = std::string{name};

            rtn.emplace_back(lime::symbol{
                .name    = std::move(str),
                .address = sym,
            });

            return false;
        };

        m_impl->iterate_symbols(fn);

        return rtn;
    }

    std::uintptr_t module::symbol(std::string_view name) const
    {
        auto *addr = dlsym(m_impl->handle, name.data());
        return reinterpret_cast<std::uintptr_t>(addr);
    }

    std::optional<std::uintptr_t> module::find_symbol(std::string_view name) const
    {
        std::uintptr_t rtn{0};

        auto fn = [&](auto item)
        {
            if (item.find(name) == std::string_view::npos)
            {
                return false;
            }

            rtn = symbol(item);

            return true;
        };

        m_impl->iterate_symbols(fn);

        if (!rtn)
        {
            return std::nullopt;
        }

        return rtn;
    }

    std::vector<module> module::modules()
    {
        std::vector<module> rtn;

        auto callback = [](dl_phdr_info *info, std::size_t, void *data)
        {
            auto &res = *reinterpret_cast<decltype(rtn) *>(data);

            if (!info)
            {
                return 0;
            }

            module current;

            current.m_impl->info   = *info;
            current.m_impl->handle = dlopen(current.name().data(), RTLD_NOLOAD | RTLD_LAZY);

            res.emplace_back(std::move(current));

            return 0;
        };

        dl_iterate_phdr(callback, &rtn);

        return rtn;
    }

    std::optional<module> module::get(std::string_view name)
    {
        auto all    = modules();
        auto module = std::find_if(all.begin(), all.end(), [&](auto &item) { return item.name() == name; });

        if (module == all.end())
        {
            return std::nullopt;
        }

        return std::move(*module);
    }

    std::optional<module> module::load(std::string_view name)
    {
        if (dlopen(name.data(), RTLD_NOW) == nullptr)
        {
            return std::nullopt;
        }

        return get(name);
    }

    std::optional<module> module::find(std::string_view name)
    {
        auto all = modules();

        auto fn = [&](const auto &item)
        {
            return item.name().find(name) != std::string_view::npos;
        };

        auto rtn = std::ranges::find_if(all, fn);

        if (rtn == all.end())
        {
            return std::nullopt;
        }

        return std::move(*rtn);
    }
} // namespace lime
