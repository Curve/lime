#include "utils/signature.hpp"

#include "page.hpp"

#include <cassert>
#include <charconv>

#include <string>
#include <ranges>

namespace lime
{
    struct signature::impl
    {
        std::string mask;
        std::string pattern;

      public:
        protection required;

      public:
        [[nodiscard]] bool matches(std::uintptr_t) const;
    };

    signature::signature() : m_impl(std::make_unique<impl>()) {}

    signature::~signature() = default;

    signature::signature(const signature &other) : m_impl(std::make_unique<impl>())
    {
        *m_impl = *other.m_impl;
    }

    signature::signature(signature &&other) noexcept : m_impl(std::move(other.m_impl)) {}

    bool signature::impl::matches(std::uintptr_t address) const
    {
        for (auto j = 0u; mask.size() > j; j++)
        {
            if (mask[j] == '?')
            {
                continue;
            }

            const auto &current = *reinterpret_cast<char *>(address + j);

            if (current == pattern[j])
            {
                continue;
            }

            return false;
        }

        return true;
    }

    std::optional<std::uintptr_t> signature::find() const
    {
        for (const auto &page : page::pages())
        {
            auto rtn = find(page);

            if (!rtn)
            {
                continue;
            }

            return rtn;
        }

        return std::nullopt;
    }

    std::optional<std::uintptr_t> signature::find(const page &page) const
    {
        if (!(page.prot() & m_impl->required))
        {
            return std::nullopt;
        }

        const auto end = page.end() - m_impl->mask.size();

        for (auto i = page.start(); end > i; i++)
        {
            if (!m_impl->matches(i))
            {
                continue;
            }

            return i;
        }

        return std::nullopt;
    }

    std::optional<std::uintptr_t> signature::find(const module &module) const
    {
        const auto end = module.end() - m_impl->mask.size();

        for (auto i = module.start(); end > i; i++)
        {
            auto page = page::at(i);

            if (!page)
            {
                continue;
            }

            auto res = find(page.value());

            if (res)
            {
                return res.value();
            }

            i += page->size() - 1;
        }

        return std::nullopt;
    }

    std::vector<std::uintptr_t> signature::find_all() const
    {
        std::vector<std::uintptr_t> rtn;

        for (const auto &page : page::pages())
        {
            auto found = find_all(page);

            if (found.empty())
            {
                continue;
            }

            std::ranges::move(found, std::back_inserter(rtn));
        }

        return rtn;
    }

    std::vector<std::uintptr_t> signature::find_all(const page &page) const
    {
        std::vector<std::uintptr_t> rtn;

        if (!(page.prot() & m_impl->required))
        {
            return rtn;
        }

        const auto end = page.end() - m_impl->mask.size();

        for (auto i = page.start(); end > i; i++)
        {
            if (!m_impl->matches(i))
            {
                continue;
            }

            rtn.emplace_back(i);
        }

        return rtn;
    }

    std::vector<std::uintptr_t> signature::find_all(const module &module) const
    {
        std::vector<std::uintptr_t> rtn;

        const auto end = module.end() - m_impl->mask.size();

        for (auto i = module.start(); end > i; i++)
        {
            const auto page = page::at(i);

            if (!page)
            {
                continue;
            }

            auto found = find_all(page.value());

            if (!found.empty())
            {
                std::ranges::move(found, std::back_inserter(rtn));
            }

            i += page->size() - 1;
        }

        return rtn;
    }

    signature signature::from(std::string_view ida_pattern, protection required)
    {
        auto split = std::views::split(ida_pattern, ' ');

        std::string mask;
        std::string pattern;

        for (const auto &r : split)
        {
            std::string_view current{r.begin(), r.end()};

            if (current.starts_with('?'))
            {
                mask.push_back('?');
                pattern.push_back(0);

                continue;
            }

            int hex{};

            [[maybe_unused]] auto result = std::from_chars(current.begin(), current.end(), hex, 16);
            assert((result.ec == std::errc{}) && "Failed to convert given character");

            mask.push_back('x');
            pattern.push_back(static_cast<char>(hex));
        }

        return from(pattern.c_str(), mask, required);
    }

    signature signature::from(const char *pattern, std::string mask, protection required)
    {
        signature rtn;

        rtn.m_impl->pattern  = {pattern, mask.size()};
        rtn.m_impl->mask     = std::move(mask);
        rtn.m_impl->required = required | protection::read;

        return rtn;
    }
} // namespace lime
