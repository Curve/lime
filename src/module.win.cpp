#include "module.hpp"

#include <windows.h>
#include <psapi.h>

#include <functional>
#include <algorithm>

namespace lime
{
    struct module::impl
    {
        using callback_t = std::function<bool(std::string_view)>;

      public:
        std::string name;

      public:
        HMODULE handle;
        MODULEINFO info;

      public:
        void iterate_symbols(const callback_t &) const;

      public:
        static std::optional<module> get(HMODULE module);
        static std::string lower(std::string_view string);
    };

    module::module() :m_impl(std::make_unique<impl>()) {}

    module::~module() = default;

    module::module(const module &other) :m_impl(std::make_unique<impl>())
    {
        *m_impl = *other.m_impl;
    }

    module::module(module &&other) noexcept :m_impl(std::move(other.m_impl)) {}

    std::string_view module::name() const
    {
        return m_impl->name;
    }

    std::size_t module::size() const
    {
        return m_impl->info.SizeOfImage;
    }

    std::uintptr_t module::end() const
    {
        return start() + size();
    }

    std::uintptr_t module::start() const
    {
        return reinterpret_cast<std::uintptr_t>(m_impl->info.lpBaseOfDll);
    }

    std::vector<lime::symbol> module::symbols() const
    {
        std::vector<lime::symbol> rtn;

        auto fn = [&](auto name)
        {
            auto sym = symbol(name);

            rtn.emplace_back(lime::symbol{
                .name    = std::string{name},
                .address = sym,
            });

            return false;
        };

        m_impl->iterate_symbols(fn);

        return rtn;
    }

    std::uintptr_t module::symbol(std::string_view name) const
    {
        return reinterpret_cast<std::uintptr_t>(GetProcAddress(m_impl->handle, name.data()));
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

        std::vector<HMODULE> modules{32};
        const auto length = modules.size() * sizeof(HMODULE);

        DWORD required{};

        if (!EnumProcessModules(GetCurrentProcess(), modules.data(), length, &required))
        {
            return rtn;
        }

        modules.resize(required / sizeof(HMODULE));

        if (!EnumProcessModules(GetCurrentProcess(), modules.data(), required, &required))
        {
            return rtn;
        }

        for (const auto &handle : modules)
        {
            auto module = impl::get(handle);

            if (!module)
            {
                continue;
            }

            rtn.emplace_back(std::move(module.value()));
        }

        return rtn;
    }

    std::optional<module> module::get(std::string_view name)
    {
        auto *module = GetModuleHandleA(name.data());

        if (!module)
        {
            return std::nullopt;
        }

        return impl::get(module);
    }

    std::optional<module> module::load(std::string_view name)
    {
        auto *module = LoadLibraryA(name.data());

        if (!module)
        {
            return std::nullopt;
        }

        return impl::get(module);
    }

    std::optional<module> module::find(std::string_view name)
    {
        const auto lower = impl::lower(name);
        auto all         = modules();

        auto pred = [&](const auto &item)
        {
            return item.name().find(lower) != std::string_view::npos;
        };

        auto rtn = std::ranges::find_if(all, pred);

        if (rtn == all.end())
        {
            return std::nullopt;
        }

        return std::move(*rtn);
    }

    void module::impl::iterate_symbols(const module::impl::callback_t &callback) const
    {
        const auto base = reinterpret_cast<std::uintptr_t>(info.lpBaseOfDll);

        auto *header    = reinterpret_cast<IMAGE_DOS_HEADER *>(base);
        auto *nt_header = reinterpret_cast<IMAGE_NT_HEADERS *>(base + header->e_lfanew);

        auto *exports = reinterpret_cast<IMAGE_EXPORT_DIRECTORY *>(
            base + nt_header->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].VirtualAddress);

        auto *names = reinterpret_cast<std::uint32_t *>(base + exports->AddressOfNames);

        for (auto i = 0u; i < exports->NumberOfNames; i++)
        {
            const auto *current = reinterpret_cast<const char *>(base + names[i]);

            if (!callback(current))
            {
                continue;
            }

            break;
        }
    }

    std::optional<module> module::impl::get(HMODULE module)
    {
        char name[MAX_PATH];

        if (!GetModuleBaseNameA(GetCurrentProcess(), module, name, MAX_PATH))
        {
            return std::nullopt;
        }

        MODULEINFO info;

        if (!GetModuleInformation(GetCurrentProcess(), module, &info, sizeof(info)))
        {
            return std::nullopt;
        }

        lime::module item;

        item.m_impl->info   = info;
        item.m_impl->handle = module;
        item.m_impl->name   = lower(name);

        return item;
    }

    std::string module::impl::lower(std::string_view string)
    {
        std::string rtn{string};
        std::ranges::transform(rtn, rtn.begin(), [](unsigned char c) { return std::tolower(c); });

        return rtn;
    };
} // namespace lime
