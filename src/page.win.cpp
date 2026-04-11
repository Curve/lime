#include "page.hpp"

#include "lib.hpp"
#include "constants.hpp"

#include <limits>

#include <windows.h>
#include <memoryapi.h>

namespace lime
{
    struct page::impl
    {
        DWORD prot;

      public:
        std::uintptr_t start;
        std::uintptr_t end;

      public:
        std::optional<DWORD> original;
        std::shared_ptr<void> release;

      public:
        static protection translate(DWORD);
        static DWORD translate(protection);
    };

    protection page::impl::translate(DWORD prot)
    {
        using enum protection;

        if (prot & PAGE_READONLY)
        {
            return read;
        }
        else if (prot & PAGE_READWRITE)
        {
            return read | write;
        }
        else if (prot & PAGE_EXECUTE_READWRITE || prot & PAGE_EXECUTE_WRITECOPY)
        {
            return read | write | execute;
        }

        return {};
    }

    DWORD page::impl::translate(protection prot)
    {
        using enum protection;

        auto rtn = DWORD{};

        if (prot & read)
        {
            rtn = PAGE_READONLY;
        }
        if (prot & write)
        {
            rtn = PAGE_READWRITE;
        }
        if (prot & execute)
        {
            rtn = PAGE_EXECUTE_READWRITE;
        }

        return rtn;
    }

    page::page(impl data) : m_impl(std::make_unique<impl>(std::move(data))) {}

    page::page(const page &other) : page(*other.m_impl) {}

    page::page(page &&other) noexcept : m_impl(std::move(other.m_impl)) {}

    page::~page() = default;

    page &page::operator=(page other) noexcept
    {
        swap(*this, other);
        return *this;
    }

    void swap(page &first, page &second) noexcept
    {
        using std::swap;
        swap(first.m_impl, second.m_impl);
    }

    std::uintptr_t page::start() const
    {
        return m_impl->start;
    }

    std::uintptr_t page::end() const
    {
        return m_impl->end;
    }

    std::size_t page::size() const
    {
        return m_impl->end - m_impl->start;
    }

    protection page::prot() const
    {
        return impl::translate(m_impl->prot);
    }

    bool page::can(protection what) const
    {
        return prot() & what;
    }

    bool page::restore()
    {
        if (!m_impl->original.has_value())
        {
            return true;
        }

        const auto original       = *m_impl->original;
        auto *const start_address = reinterpret_cast<LPVOID>(start());

        [[maybe_unused]] auto old = DWORD{};

        if (VirtualProtect(start_address, size(), original, &old) == 0)
        {
            return false;
        }

        m_impl->prot     = original;
        m_impl->original = {};

        return true;
    }

    bool page::allow(protection what)
    {
        return protect(prot() & what);
    }

    bool page::protect(protection prot)
    {
        const auto protection     = impl::translate(prot);
        auto *const start_address = reinterpret_cast<LPVOID>(start());

        auto original = DWORD{};

        if (VirtualProtect(start_address, size(), protection, &original) == 0)
        {
            return false;
        }

        m_impl->original = original;
        m_impl->prot     = protection;

        return true;
    }

    template <>
    std::optional<page> page::allocate<page::policy::exact>(std::uintptr_t where, std::size_t size, protection protection)
    {
        const auto prot   = impl::translate(protection);
        auto *const alloc = VirtualAlloc(reinterpret_cast<LPVOID>(where), size, MEM_COMMIT | MEM_RESERVE, prot);

        if (!alloc)
        {
            return std::nullopt;
        }

        return page{{
            .prot    = prot,
            .start   = reinterpret_cast<std::uintptr_t>(alloc),
            .end     = reinterpret_cast<std::uintptr_t>(alloc) + size,
            .release = std::shared_ptr<void>{alloc, [](auto *alloc) { VirtualFree(alloc, 0, MEM_RELEASE); }},
        }};
    }

    template <>
    std::optional<page> page::allocate<page::policy::nearby>(std::uintptr_t where, std::size_t size, protection protection)
    {
        if constexpr (lime::arch == lime::architecture::x86)
        {
            return allocate(size, protection);
        }

        static const auto VirtualAlloc2 = []
        {
            const auto kernel_base = lib::load("kernelbase.dll");
            const auto address     = kernel_base->symbol("VirtualAlloc2").value();
            return reinterpret_cast<decltype(::VirtualAlloc2) *>(address);
        }();

        auto param        = MEM_EXTENDED_PARAMETER{};
        auto requirements = MEM_ADDRESS_REQUIREMENTS{};

        auto si = SYSTEM_INFO{};
        GetSystemInfo(&si);

        const auto granularity = si.dwAllocationGranularity;

        auto min = static_cast<std::intptr_t>(where) - std::numeric_limits<std::int32_t>::max();
        auto max = where + std::numeric_limits<std::int32_t>::max();

        min += (granularity - (min % granularity));
        max -= (max % granularity) + 1;

        requirements.Alignment             = 0;
        requirements.HighestEndingAddress  = reinterpret_cast<LPVOID>(max);
        requirements.LowestStartingAddress = min < 0 ? nullptr : reinterpret_cast<LPVOID>(min);

        param.Pointer = &requirements;
        param.Type    = MemExtendedParameterAddressRequirements;

        const auto prot   = impl::translate(protection);
        auto *const alloc = VirtualAlloc2(GetCurrentProcess(), nullptr, size, MEM_COMMIT | MEM_RESERVE, prot, &param, 1);

        if (!alloc)
        {
            return std::nullopt;
        }

        return page{{
            .prot    = prot,
            .start   = reinterpret_cast<std::uintptr_t>(alloc),
            .end     = reinterpret_cast<std::uintptr_t>(alloc) + size,
            .release = std::shared_ptr<void>{alloc, [](auto *alloc) { VirtualFree(alloc, 0, MEM_RELEASE); }},
        }};
    }

    std::optional<page> page::allocate(std::size_t size, protection prot)
    {
        return allocate<policy::exact>(reinterpret_cast<std::uintptr_t>(nullptr), size, prot);
    }

    std::vector<page> page::pages()
    {
        auto rtn = std::vector<page>{};

        const auto *address = LPVOID{};
        auto info           = MEMORY_BASIC_INFORMATION{};

        while (VirtualQuery(address, &info, sizeof(info)))
        {
            const auto start = reinterpret_cast<std::uintptr_t>(info.BaseAddress);
            const auto end   = start + info.RegionSize;

            rtn.emplace_back(page{{
                .prot  = info.Protect,
                .start = start,
                .end   = end,
            }});

            address = reinterpret_cast<LPVOID>(end);
        }

        return rtn;
    }

    std::optional<page> page::at(std::uintptr_t address)
    {
        const auto *addr = reinterpret_cast<LPVOID>(address);
        auto info        = MEMORY_BASIC_INFORMATION{};

        if (VirtualQuery(addr, &info, sizeof(info)) == 0)
        {
            return std::nullopt;
        }

        const auto start = reinterpret_cast<std::uintptr_t>(info.BaseAddress);
        const auto end   = start + info.RegionSize;

        return page{{
            .prot  = info.Protect,
            .start = start,
            .end   = end,
        }};
    }
} // namespace lime
