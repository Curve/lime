#include <Windows.h>
#include <functional>
#include <module.hpp>

#include <ImageHlp.h>
#include <psapi.h>

namespace lime
{
    struct module::impl
    {
        HMODULE handle;
        void iterate_symbols(const std::function<bool(const std::string &)> &) const;
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
            return reinterpret_cast<std::uintptr_t>(GetProcAddress(m_impl->handle, name.c_str()));
        }

        return 0;
    }

    std::vector<module> module::get_modules()
    {
        std::vector<module> rtn;

        HMODULE modules[1024];
        DWORD modules_size = 0;

        if (EnumProcessModules(GetCurrentProcess(), modules, sizeof(modules), &modules_size))
        {
            for (auto i = 0ul; i < (modules_size / sizeof(HMODULE)); i++)
            {
                TCHAR name[MAX_PATH];

                if (!GetModuleBaseName(GetCurrentProcess(), modules[i], name, sizeof(name) / sizeof(TCHAR)))
                    continue;

                MODULEINFO info;
                if (!GetModuleInformation(GetCurrentProcess(), modules[i], &info, sizeof(info)))
                    continue;

                module module;
                module.m_name = name;
                module.m_size = info.SizeOfImage;
                module.m_impl->handle = modules[i];
                module.m_start = reinterpret_cast<std::uintptr_t>(info.lpBaseOfDll);

                rtn.emplace_back(std::move(module));
            }
        }

        return rtn;
    }

    std::optional<module> module::get(const std::string &name)
    {
        HMODULE handle = nullptr;

        if (name.empty())
            handle = GetModuleHandle(nullptr);
        else
            handle = GetModuleHandle(name.c_str());

        if (handle)
        {
            MODULEINFO info;
            if (GetModuleInformation(GetCurrentProcess(), handle, &info, sizeof(info)))
            {
                module rtn;
                rtn.m_name = name;
                rtn.m_impl->handle = handle;
                rtn.m_size = info.SizeOfImage;
                rtn.m_start = reinterpret_cast<std::uintptr_t>(info.lpBaseOfDll);

                return rtn;
            }
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

    void module::impl::iterate_symbols(const std::function<bool(const std::string &)> &callback) const
    {
        TCHAR path[MAX_PATH];
        GetModuleFileNameA(handle, path, sizeof(path));

        _LOADED_IMAGE image;
        if (MapAndLoad(path, nullptr, &image, TRUE, TRUE))
        {
            ULONG size{};
            const auto *export_directory =
                reinterpret_cast<_IMAGE_EXPORT_DIRECTORY *>(ImageDirectoryEntryToData(image.MappedAddress, false, IMAGE_DIRECTORY_ENTRY_EXPORT, &size));

            if (export_directory)
            {
                const auto *rvas =
                    reinterpret_cast<DWORD *>(ImageRvaToVa(image.FileHeader, image.MappedAddress, export_directory->AddressOfNames, nullptr));

                for (size_t i = 0; i < export_directory->NumberOfNames; i++)
                {
                    const auto *name = reinterpret_cast<char *>(ImageRvaToVa(image.FileHeader, image.MappedAddress, rvas[i], nullptr));
                    if (callback(name))
                        return;
                }
            }

            UnMapAndLoad(&image);
        }
    }

    std::vector<std::pair<std::string, std::uintptr_t>> module::get_symbols() const
    {
        if (m_impl)
        {
            std::vector<std::pair<std::string, std::uintptr_t>> rtn;
            m_impl->iterate_symbols([&](const std::string &name) {
                rtn.emplace_back(std::make_pair(name, get_symbol(name)));
                return false;
            });

            return rtn;
        }

        return {};
    }

    std::optional<std::uintptr_t> module::find_symbol(const std::string &name) const
    {
        if (m_impl)
        {
            std::uintptr_t rtn{};
            m_impl->iterate_symbols([&](const std::string &module_name) {
                if (module_name.find(name) != std::string::npos)
                {
                    rtn = get_symbol(module_name);
                    return true;
                }

                return false;
            });

            return rtn;
        }

        return std::nullopt;
    }
} // namespace lime