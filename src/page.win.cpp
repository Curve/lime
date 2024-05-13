#include "page.hpp"
#include "module.hpp"
#include "constants.hpp"

#include <limits>
#include <windows.h>
#include <memoryapi.h>

namespace lime
{
    struct page::impl
    {
        protection prot;
        DWORD original_prot;

      public:
        std::uintptr_t end;
        std::uintptr_t start;

      public:
        static protection translate(DWORD);
        static DWORD translate(protection);

      public:
        static std::shared_ptr<page> from(LPVOID, std::size_t, protection);
    };

    protection page::impl::translate(DWORD prot)
    {
        protection rtn{protection::none};

        if (prot & PAGE_READONLY)
        {
            rtn = protection::read;
        }
        else if (prot & PAGE_READWRITE)
        {
            rtn = protection::read | protection::write;
        }
        else if (prot & (PAGE_EXECUTE_READ | PAGE_EXECUTE_READWRITE | PAGE_EXECUTE_WRITECOPY))
        {
            rtn = protection::read | protection::write | protection::execute;
        }

        return rtn;
    }

    DWORD page::impl::translate(protection prot)
    {
        DWORD rtn{0};

        if (prot & protection::read)
        {
            rtn = PAGE_READONLY;
        }
        if (prot & protection::write)
        {
            rtn = PAGE_READWRITE;
        }
        if (prot & protection::execute)
        {
            rtn = PAGE_EXECUTE_READWRITE;
        }

        return rtn;
    }

    std::shared_ptr<page> page::impl::from(LPVOID address, std::size_t size, protection protection)
    {
        auto *rtn = new page;

        rtn->m_impl->start = reinterpret_cast<std::uintptr_t>(address);
        rtn->m_impl->end   = rtn->m_impl->start + size;

        rtn->m_impl->original_prot = translate(protection);
        rtn->m_impl->prot          = protection;

        auto deleter = [](page *page)
        {
            VirtualFree(reinterpret_cast<LPVOID>(page->start()), page->size(), MEM_RELEASE);
            delete page;
        };

        return {rtn, deleter};
    }

    page::page() : m_impl(std::make_unique<impl>()) {}

    page::~page() = default;

    page::page(const page &other) : m_impl(std::make_unique<impl>())
    {
        *m_impl = *other.m_impl;
    }

    page::page(page &&other) noexcept : m_impl(std::move(other.m_impl)) {}

    page &page::operator=(page &&other) noexcept
    {
        m_impl = std::move(other.m_impl);
        return *this;
    }

    protection page::prot() const
    {
        return m_impl->prot;
    }

    std::size_t page::size() const
    {
        return m_impl->end - m_impl->start;
    }

    std::uintptr_t page::end() const
    {
        return m_impl->end;
    }

    std::uintptr_t page::start() const
    {
        return m_impl->start;
    }

    bool page::restore()
    {
        [[maybe_unused]] DWORD old{};

        if (VirtualProtect(reinterpret_cast<LPVOID>(start()), size(), m_impl->original_prot, &old) == 0)
        {
            return false;
        }

        m_impl->prot = impl::translate(m_impl->original_prot);

        return true;
    }

    bool page::protect(protection prot)
    {
        [[maybe_unused]] DWORD old{};

        if (VirtualProtect(reinterpret_cast<LPVOID>(start()), size(), impl::translate(prot), &old) == 0)
        {
            return false;
        }

        m_impl->prot = prot;

        return true;
    }

    std::vector<page> page::pages()
    {
        std::vector<page> rtn;

        void *address{};
        MEMORY_BASIC_INFORMATION info;

        while (VirtualQuery(address, &info, sizeof(info)))
        {
            auto base = reinterpret_cast<std::uintptr_t>(info.BaseAddress);
            rtn.emplace_back(unsafe(base));

            address = reinterpret_cast<void *>(base + info.RegionSize);
        }

        return rtn;
    }

    page page::unsafe(std::uintptr_t address)
    {
        // NOLINTNEXTLINE(*-optional-access)
        return at(address).value();
    }

    std::optional<page> page::at(std::uintptr_t address)
    {
        MEMORY_BASIC_INFORMATION info;

        if (VirtualQuery(reinterpret_cast<LPVOID>(address), &info, sizeof(info)) == 0)
        {
            return std::nullopt;
        }

        const auto base = reinterpret_cast<std::uintptr_t>(info.BaseAddress);

        page rtn;

        rtn.m_impl->start         = base;
        rtn.m_impl->end           = base + info.RegionSize;
        rtn.m_impl->original_prot = info.Protect;
        rtn.m_impl->prot          = impl::translate(info.Protect);

        return rtn;
    }

    std::shared_ptr<page> page::allocate(std::size_t size, protection prot)
    {
        return allocate<alloc_policy::exact>(reinterpret_cast<std::uintptr_t>(nullptr), size, prot);
    }

    template <>
    std::shared_ptr<page> page::allocate<alloc_policy::exact>(std::uintptr_t where, std::size_t size,
                                                              protection protection)
    {
        auto *addr      = reinterpret_cast<LPVOID>(where);
        const auto prot = impl::translate(protection);
        auto *alloc     = VirtualAlloc(addr, size, MEM_COMMIT | MEM_RESERVE, prot);

        if (!alloc)
        {
            return nullptr;
        }

        return impl::from(alloc, size, protection);
    }

    template <>
    std::shared_ptr<page> page::allocate<alloc_policy::nearby>(std::uintptr_t where, std::size_t size,
                                                               protection protection)
    {
#ifdef LIME_DISABLE_ALLOC2
        return nullptr;
#else

        if constexpr (lime::arch == lime::architecture::x86)
        {
            return allocate(size, protection);
        }

        MEM_EXTENDED_PARAMETER param{};
        MEM_ADDRESS_REQUIREMENTS requirements{};

        SYSTEM_INFO si{};
        GetSystemInfo(&si);

        const auto granularity = si.dwAllocationGranularity;

        auto min = where - std::numeric_limits<std::int32_t>::max();
        auto max = where + std::numeric_limits<std::int32_t>::max();

        min += (granularity - (min % granularity));
        max -= (max % granularity) + 1;

        requirements.Alignment             = 0;
        requirements.HighestEndingAddress  = reinterpret_cast<LPVOID>(max);
        requirements.LowestStartingAddress = min < 0 ? nullptr : reinterpret_cast<LPVOID>(min);

        param.Pointer = &requirements;
        param.Type    = MemExtendedParameterAddressRequirements;

        static const auto VirtualAlloc2 = []()
        {
            auto kernel32 = module::load("kernel32.dll");
            return reinterpret_cast<decltype(::VirtualAlloc2) *>(kernel32->symbol("VirtualAlloc2"));
        }();

        auto *alloc = VirtualAlloc2(GetCurrentProcess(), nullptr, size, MEM_COMMIT | MEM_RESERVE,
                                    impl::translate(protection), &param, 1);

        if (!alloc)
        {
            return nullptr;
        }

        return impl::from(alloc, size, protection);
#endif
    }
} // namespace lime
