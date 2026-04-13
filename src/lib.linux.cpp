#include "lib.impl.hpp"
#include "utils.linux.hpp"

#include <link.h>
#include <dlfcn.h>

#include <algorithm>

namespace lime
{
    struct lib::impl
    {
        void *handle;
        dl_phdr_info info;
    };

    lib::lib(impl data) : m_impl(std::make_unique<impl>(data)) {}

    lib::lib(const lib &other) : lib(*other.m_impl) {}

    lib::lib(lib &&other) noexcept : m_impl(std::move(other.m_impl)) {}

    lib::~lib() = default;

    lib &lib::operator=(lib other) noexcept
    {
        swap(*this, other);
        return *this;
    }

    void swap(lib &first, lib &second) noexcept
    {
        using std::swap;
        swap(first.m_impl, second.m_impl);
    }

    std::uintptr_t lib::start() const
    {
        return m_impl->info.dlpi_addr + m_impl->info.dlpi_phdr->p_vaddr;
    }

    std::string_view lib::name() const
    {
        return m_impl->info.dlpi_name;
    }

    std::size_t lib::size() const
    {
        auto rtn = std::size_t{};

        for (auto i = 0uz; m_impl->info.dlpi_phnum > i; ++i)
        {
            rtn += m_impl->info.dlpi_phdr[i].p_memsz;
        }

        return rtn;
    }

    std::vector<lime::symbol> lib::symbols() const
    {
        auto rtn = std::vector<lime::symbol>{};

        utils::iter_sym(m_impl->info,
                        [&](const char *name)
                        {
                            if (auto addr = symbol(name); addr.has_value())
                            {
                                rtn.emplace_back(lime::symbol{
                                    .name    = name,
                                    .address = *addr,
                                });
                            }

                            return false;
                        });

        return rtn;
    }

    std::optional<std::uintptr_t> lib::symbol(const char *name) const
    {
        auto *const addr = dlsym(m_impl->handle, name);

        if (!addr)
        {
            return std::nullopt;
        }

        return reinterpret_cast<std::uintptr_t>(addr);
    }

    std::optional<std::uintptr_t> lib::symbol(const sym_predicate &pred) const
    {
        const auto *const name = utils::iter_sym(m_impl->info, pred);

        if (!name)
        {
            return std::nullopt;
        }

        return symbol(name);
    }

    std::vector<lib> lib::libraries()
    {
        auto rtn      = std::vector<lib>{};
        auto callback = [](dl_phdr_info *info, std::size_t, void *data)
        {
            if (info)
            {
                reinterpret_cast<decltype(rtn) *>(data)->emplace_back(lib{{
                    .handle = dlopen(info->dlpi_name, RTLD_NOLOAD | RTLD_LAZY),
                    .info   = *info,
                }});
            }

            return 0;
        };

        dl_iterate_phdr(callback, &rtn);

        return rtn;
    }

    std::optional<lib> lib::load(const fs::path &path)
    {
        if (dlopen(path.c_str(), RTLD_NOW) == nullptr)
        {
            return std::nullopt;
        }

        return find(path.c_str());
    }

    std::optional<lib> lib::find(const pattern &re)
    {
        return find(
            [&](const auto &item)
            {
                const auto name = item.name();
                return std::regex_search(name.begin(), name.end(), re.m_impl->regex);
            });
    }

    std::optional<lib> lib::find(const fs::path &name)
    {
        return find([name](const auto &item) { return item.name() == name; });
    }
} // namespace lime
