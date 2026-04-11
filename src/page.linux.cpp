#include "page.hpp"
#include "constants.hpp"

#include <cassert>

#include <limits>
#include <ranges>

#include <fstream>
#include <algorithm>

#include <sys/mman.h>
#include <scn/scan.h>

namespace lime
{
    struct page::impl
    {
        protection prot;

      public:
        std::uintptr_t start;
        std::uintptr_t end;

      public:
        std::optional<protection> original;
        std::shared_ptr<void> release;

      public:
        static std::optional<page> parse_page(std::string_view);
    };

    std::optional<page> page::impl::parse_page(std::string_view line)
    {
        const auto result = scn::scan<std::uintptr_t, std::uintptr_t, std::string>(line, "{:x}-{:x} {}");

        if (!result)
        {
            assert(false && "Failed to parse page");
            return std::nullopt;
        }

        auto [start, end, permissions] = result->values();
        auto prot                      = protection{};

        if (permissions.size() < 3) [[unlikely]]
        {
            permissions = std::format("{}{}", permissions, std::string(3 - permissions.size(), '-'));
        }

        if (permissions[0] == 'r')
        {
            prot |= protection::read;
        }
        if (permissions[1] == 'w')
        {
            prot |= protection::write;
        }
        if (permissions[2] == 'x')
        {
            prot |= protection::execute;
        }

        return page{{
            .prot  = prot,
            .start = start,
            .end   = end,
        }};
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
        return m_impl->prot;
    }

    bool page::can(protection what) const
    {
        return (m_impl->prot & what) == what;
    }

    bool page::restore()
    {
        if (!m_impl->original.has_value())
        {
            return true;
        }

        const auto original = *m_impl->original;
        const auto prot     = static_cast<int>(original);

        if (mprotect(reinterpret_cast<void *>(start()), size(), prot) != 0)
        {
            return false;
        }

        m_impl->prot     = original;
        m_impl->original = {};

        return true;
    }

    bool page::allow(protection what)
    {
        return protect(m_impl->prot | what);
    }

    bool page::protect(protection prot)
    {
        if (mprotect(reinterpret_cast<void *>(start()), size(), static_cast<int>(prot)) != 0)
        {
            return false;
        }

        m_impl->original = m_impl->prot;
        m_impl->prot     = prot;

        return true;
    }

    template <>
    std::optional<page> page::allocate<page::policy::exact>(std::uintptr_t where, std::size_t size, protection protection)
    {
        const auto prot   = static_cast<int>(protection);
        auto *const alloc = mmap(reinterpret_cast<void *>(where), size, prot, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);

        if (alloc == MAP_FAILED)
        {
            return std::nullopt;
        }

        return page{{
            .prot    = protection,
            .start   = reinterpret_cast<std::uintptr_t>(alloc),
            .end     = reinterpret_cast<std::uintptr_t>(alloc) + size,
            .release = std::shared_ptr<void>{alloc, [size](auto *alloc) { munmap(alloc, size); }},
        }};
    }

    template <>
    std::optional<page> page::allocate<page::policy::nearby>(std::uintptr_t where, std::size_t size, protection protection)
    {
        if constexpr (lime::arch == lime::architecture::x86)
        {
            return allocate(size, protection);
        }

        static constexpr auto is_aligned = [](auto what)
        {
            return what & (getpagesize() - 1);
        };

        static constexpr auto align = [](auto what)
        {
            return (what + getpagesize()) & ~(getpagesize() - 1);
        };

        auto *alloc  = MAP_FAILED;
        auto aligned = is_aligned(where) ? where : align(where);

        const auto flags = MAP_PRIVATE | MAP_ANON | MAP_FIXED_NOREPLACE;
        const auto prot  = static_cast<int>(protection);

        while (alloc == MAP_FAILED)
        {
            static constexpr auto max_dist = std::numeric_limits<std::int32_t>().max();
            const auto dist                = aligned - where;

            if (dist >= max_dist)
            {
                return std::nullopt;
            }

            alloc   = mmap(reinterpret_cast<void *>(aligned), size, prot, flags, -1, 0);
            aligned = align(aligned);
        }

        return page{{
            .prot    = protection,
            .start   = reinterpret_cast<std::uintptr_t>(alloc),
            .end     = reinterpret_cast<std::uintptr_t>(alloc) + size,
            .release = std::shared_ptr<void>{alloc, [size](auto *alloc) { munmap(alloc, size); }},
        }};
    }

    std::optional<page> page::allocate(std::size_t size, protection prot)
    {
        return allocate<policy::exact>(reinterpret_cast<std::uintptr_t>(nullptr), size, prot);
    }

    std::vector<page> page::pages()
    {
        auto maps = std::ifstream{"/proc/self/maps"};

        if (maps.fail())
        {
            assert(false && "Failed to open /proc/self/maps");
            return {};
        }

        auto rtn = std::vector<page>{};

        for (std::string line; std::getline(maps, line);)
        {
            auto parsed = impl::parse_page(line);

            if (!parsed.has_value())
            {
                continue;
            }

            rtn.emplace_back(std::move(*parsed));
        }

        return rtn;
    }

    std::optional<page> page::at(std::uintptr_t address)
    {
        auto pages    = page::pages();
        auto reversed = std::views::reverse(pages);

        auto it = std::ranges::find_if(reversed,
                                       [address](const auto &page)
                                       { // 
                                           return address >= page.start() && address <= page.end();
                                       });

        if (it == reversed.end())
        {
            return std::nullopt;
        }

        return *it;
    }
} // namespace lime
