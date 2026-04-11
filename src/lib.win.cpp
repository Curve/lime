#include "lib.hpp"

#include <windows.h>
#include <psapi.h>

namespace lime
{
    struct lib::impl
    {
        HMODULE handle;
        MODULEINFO info;

      public:
        std::string name;

      public:
        static std::optional<lib> from(HMODULE);
        static const char *iter_sym(MODULEINFO, const lib::sym_predicate &);
    };

    lib::lib(impl data) : m_impl(std::make_unique<impl>(std::move(data))) {}

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
        return reinterpret_cast<std::uintptr_t>(m_impl->info.lpBaseOfDll);
    }

    std::string_view lib::name() const
    {
        return m_impl->name;
    }

    std::size_t lib::size() const
    {
        return m_impl->info.SizeOfImage;
    }

    std::vector<lime::symbol> lib::symbols() const
    {
        auto rtn = std::vector<lime::symbol>{};

        impl::iter_sym(m_impl->info,
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
        auto *const addr = GetProcAddress(m_impl->handle, name);

        if (!addr)
        {
            return std::nullopt;
        }

        return reinterpret_cast<std::uintptr_t>(addr);
    }

    std::optional<std::uintptr_t> lib::symbol(const sym_predicate &pred) const
    {
        const auto *const name = impl::iter_sym(m_impl->info, pred);

        if (!name)
        {
            return std::nullopt;
        }

        return symbol(name);
    }

    std::vector<lib> lib::libraries()
    {
        auto rtn     = std::vector<lib>{};
        auto modules = std::vector<HMODULE>{32};

        auto required     = DWORD{};
        const auto length = modules.size() * sizeof(HMODULE);

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
            auto module = impl::from(handle);

            if (!module.has_value())
            {
                continue;
            }

            rtn.emplace_back(std::move(*module));
        }

        return rtn;
    }

    std::optional<lib> lib::load(const fs::path &path)
    {
        auto *const module = LoadLibraryW(path.wstring().c_str());

        if (!module)
        {
            return std::nullopt;
        }

        return impl::from(module);
    }

    std::optional<lib> lib::find(const regex &re)
    {
        const auto regex = std::regex{re.pattern, std::regex::icase};

        return find(
            [&](const auto &item)
            {
                const auto name = item.name();
                return std::regex_search(name.begin(), name.end(), regex);
            });
    }

    std::optional<lib> lib::find(const fs::path &name)
    {
        auto *const module = GetModuleHandleW(name.wstring().c_str());

        if (!module)
        {
            return std::nullopt;
        }

        return impl::from(module);
    }

    std::optional<lib> lib::impl::from(HMODULE module)
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

        return lib{{.handle = module, .info = info, .name = name}};
    }

    const char *lib::impl::iter_sym(MODULEINFO info, const lib::sym_predicate &pred)
    {
        const auto base = reinterpret_cast<std::uintptr_t>(info.lpBaseOfDll);

        const auto *const header    = reinterpret_cast<IMAGE_DOS_HEADER *>(base);
        const auto *const nt_header = reinterpret_cast<IMAGE_NT_HEADERS *>(base + header->e_lfanew);

        const auto export_address = base + nt_header->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].VirtualAddress;

        const auto *const exports = reinterpret_cast<IMAGE_EXPORT_DIRECTORY *>(export_address);
        const auto *const names   = reinterpret_cast<std::uint32_t *>(base + exports->AddressOfNames);

        for (auto i = 0uz; i < exports->NumberOfNames; ++i)
        {
            const auto *const current = reinterpret_cast<const char *>(base + names[i]);

            if (!pred(current))
            {
                continue;
            }

            return current;
        }

        return nullptr;
    }
} // namespace lime
