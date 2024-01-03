#include "module.hpp"

#include <windows.h>
#include <imagehlp.h>
#include <psapi.h>

#include <functional>
#include <algorithm>

namespace lime
{
    struct module::impl
    {
        using callback_t = std::function<bool(const std::string &)>;

      public:
        HMODULE handle;

      public:
        MODULEINFO info;
        std::string name;

      public:
        void iterate_symbols(const callback_t &) const;

      public:
        static std::optional<module> get(HMODULE module);
        static std::string lower(const std::string &string);
    };

    module::module() :m_impl(std::make_unique<impl>()) {}

    module::~module() = default;

    module::module(const module &other) :m_impl(std::make_unique<impl>())
    {
        *m_impl = *other.m_impl;
    }

    module::module(module &&other) noexcept :m_impl(std::move(other.m_impl)) {}

    std::string module::name() const
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

        auto fn = [&](std::string name)
        {
            auto sym = symbol(name);
            rtn.emplace_back(lime::symbol{.name = std::move(name), .address = sym});

            return false;
        };

        m_impl->iterate_symbols(fn);

        return rtn;
    }

    std::uintptr_t module::symbol(const std::string &name) const
    {
        return reinterpret_cast<std::uintptr_t>(GetProcAddress(m_impl->handle, name.c_str()));
    }

    std::optional<std::uintptr_t> module::find_symbol(const std::string &name) const
    {
        std::uintptr_t rtn{0};

        auto fn = [&](const std::string &item)
        {
            if (item.find(name) == std::string::npos)
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

        DWORD modules_size{};
        HMODULE modules[1024];

        if (!EnumProcessModules(GetCurrentProcess(), modules, sizeof(modules), &modules_size))
        {
            return rtn;
        }

        for (auto i = 0u; (modules_size / sizeof(HMODULE)) > i; i++)
        {
            auto module = impl::get(modules[i]);

            if (!module)
            {
                continue;
            }

            rtn.emplace_back(std::move(module.value()));
        }

        return rtn;
    }

    std::optional<module> module::get(const std::string &name)
    {
        auto *module = GetModuleHandleA(name.c_str());

        if (!module)
        {
            return std::nullopt;
        }

        return impl::get(module);
    }

    std::optional<module> module::load(const std::string &name)
    {
        auto *module = LoadLibraryA(name.c_str());

        if (!module)
        {
            return std::nullopt;
        }

        return impl::get(module);
    }

    std::optional<module> module::find(const std::string &name)
    {
        static constexpr auto npos = std::string::npos;
        const auto lower           = impl::lower(name);
        auto all                   = modules();

        auto it = std::find_if(all.begin(), all.end(), [&](auto &item) { return item.name().find(lower) != npos; });

        if (it == all.end())
        {
            return std::nullopt;
        }

        return std::move(*it);
    }

    void module::impl::iterate_symbols(const std::function<bool(const std::string &)> &callback) const
    {
        CHAR path[MAX_PATH];
        GetModuleFileNameA(handle, path, MAX_PATH);

        _LOADED_IMAGE image;

        if (!MapAndLoad(path, nullptr, &image, TRUE, TRUE))
        {
            return;
        }

        ULONG size{};
        const auto *export_directory = reinterpret_cast<_IMAGE_EXPORT_DIRECTORY *>(
            ImageDirectoryEntryToData(image.MappedAddress, false, IMAGE_DIRECTORY_ENTRY_EXPORT, &size));

        if (!export_directory)
        {
            UnMapAndLoad(&image);
            return;
        }

        const auto *rvas = reinterpret_cast<DWORD *>(
            ImageRvaToVa(image.FileHeader, image.MappedAddress, export_directory->AddressOfNames, nullptr));

        for (size_t i = 0; export_directory->NumberOfNames > i; i++)
        {
            const auto *name =
                reinterpret_cast<char *>(ImageRvaToVa(image.FileHeader, image.MappedAddress, rvas[i], nullptr));

            if (callback(name))
            {
                break;
            }
        }

        UnMapAndLoad(&image);
    }

    std::optional<module> module::impl::get(HMODULE module)
    {
        CHAR name[MAX_PATH];

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

    std::string module::impl::lower(const std::string &string)
    {
        auto rtn{string};
        std::transform(rtn.begin(), rtn.end(), rtn.begin(), [](unsigned char c) { return std::tolower(c); });

        return rtn;
    };
} // namespace lime
