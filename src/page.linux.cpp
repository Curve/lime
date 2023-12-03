#include "page.hpp"

#include <cassert>
#include <cinttypes>
#include <sys/mman.h>

#include <array>
#include <limits>
#include <fstream>
#include <algorithm>
#include <filesystem>

namespace lime
{
    namespace fs = std::filesystem;

    struct page::impl
    {
        protection prot;
        int original_prot;

      public:
        std::uintptr_t end;
        std::uintptr_t start;

      public:
        static std::shared_ptr<page> from(void *, std::size_t, protection);
    };

    std::shared_ptr<page> page::impl::from(void *address, std::size_t size, protection protection)
    {
        auto *rtn = new page;

        rtn->m_impl->start = reinterpret_cast<std::uintptr_t>(address);
        rtn->m_impl->end   = rtn->m_impl->start + size;

        rtn->m_impl->original_prot = static_cast<int>(protection);
        rtn->m_impl->prot          = protection;

        auto deleter = [](page *page)
        {
            munmap(reinterpret_cast<void *>(page->start()), page->size());
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
        if (mprotect(reinterpret_cast<void *>(start()), size(), m_impl->original_prot) != 0)
        {
            return false;
        }

        m_impl->prot = static_cast<protection>(m_impl->original_prot);

        return true;
    }

    bool page::protect(protection prot)
    {
        if (mprotect(reinterpret_cast<void *>(start()), size(), static_cast<int>(prot)) != 0)
        {
            return false;
        }

        m_impl->prot = prot;

        return true;
    }

    std::vector<page> page::pages()
    {
        auto path = fs::path("/proc") / "self" / "maps";
        auto maps = std::ifstream{path};

        if (maps.fail())
        {
            assert(false && "Failed to open /proc/self/maps");
            return {};
        }

        std::vector<page> rtn;

        std::string line;
        while (std::getline(maps, line))
        {
            std::uintptr_t end{};
            std::uintptr_t start{};
            std::array<char, 4> permissions{};

            // NOLINTNEXTLINE(cert-err34-c)
            auto read = sscanf(line.c_str(), "%" PRIxPTR "-%" PRIxPTR " %4c", &start, &end, permissions.data());

            if (read != 3)
            {
                assert(false && "Failed to read line");
                continue;
            }

            page item;
            item.m_impl->end   = end;
            item.m_impl->start = start;

            if (permissions[0] == 'r')
            {
                item.m_impl->prot |= protection::read;
            }
            if (permissions[1] == 'w')
            {
                item.m_impl->prot |= protection::write;
            }
            if (permissions[2] == 'x')
            {
                item.m_impl->prot |= protection::execute;
            }

            item.m_impl->original_prot = static_cast<int>(item.m_impl->prot);

            rtn.emplace_back(std::move(item));
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
        auto pages = page::pages();

        auto it = std::find_if(pages.begin(), pages.end(),
                               [&](const auto &page) { return address >= page.start() && address <= page.end(); });

        if (it == pages.end())
        {
            return std::nullopt;
        }

        return *it;
    }

    std::shared_ptr<page> page::allocate(std::size_t size, protection prot)
    {
        return allocate<alloc_policy::exact>(reinterpret_cast<std::uintptr_t>(nullptr), size, prot);
    }

    template <>
    std::shared_ptr<page> page::allocate<alloc_policy::exact>(std::uintptr_t where, std::size_t size,
                                                              protection protection)
    {
        auto *addr      = reinterpret_cast<void *>(where);
        const auto prot = static_cast<int>(protection);
        auto *alloc     = mmap(addr, size, prot, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);

        if (alloc == MAP_FAILED)
        {
            return nullptr;
        }

        return impl::from(alloc, size, protection);
    }

    template <>
    std::shared_ptr<page> page::allocate<alloc_policy::nearby>(std::uintptr_t where, std::size_t size,
                                                               protection protection)
    {
        auto aligned = where & (getpagesize() - 1) ? (where + getpagesize()) & ~(getpagesize() - 1) : where;
        auto *alloc  = MAP_FAILED;

        const auto flags = MAP_PRIVATE | MAP_ANON | MAP_FIXED_NOREPLACE;
        const auto prot  = static_cast<int>(protection);

        while (alloc == MAP_FAILED)
        {
            const auto diff = aligned - where;

            if (diff >= std::numeric_limits<std::int32_t>().max())
            {
                return nullptr;
            }

            alloc   = mmap(reinterpret_cast<void *>(aligned), size, prot, flags, -1, 0);
            aligned = (aligned + getpagesize()) & ~(getpagesize() - 1);
        }

        return impl::from(alloc, size, protection);
    }
} // namespace lime
