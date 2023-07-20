#include "page.hpp"
#include "utils/signature.hpp"

#include <cstring>
#include <sstream>

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

            auto &current = *reinterpret_cast<char *>(address + j);

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
        if (m_impl->required != protection::none && !(page.prot() & m_impl->required))
        {
            return std::nullopt;
        }

        if (!(page.prot() & protection::read))
        {
            return std::nullopt;
        }

        auto end = page.end() - m_impl->mask.size();

        for (auto i = page.start(); end > i; i++)
        {
            if (m_impl->matches(i))
            {
                return i;
            }
        }

        return std::nullopt;
    }

    std::optional<std::uintptr_t> signature::find(const module &module) const
    {
        auto end = module.end() - m_impl->mask.size();

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

            rtn.insert(rtn.end(), found.begin(), found.end());
        }

        return rtn;
    }

    std::vector<std::uintptr_t> signature::find_all(const page &page) const
    {
        std::vector<std::uintptr_t> rtn;

        if (m_impl->required != protection::none && !(page.prot() & m_impl->required))
        {
            return rtn;
        }

        if (!(page.prot() & protection::read))
        {
            return rtn;
        }

        auto end = page.end() - m_impl->mask.size();

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

        auto end = module.end() - m_impl->mask.size();

        for (auto i = module.start(); end > i; i++)
        {
            auto page = page::at(i);

            if (!page)
            {
                continue;
            }

            auto found = find_all(page.value());

            if (!found.empty())
            {
                rtn.insert(rtn.end(), found.begin(), found.end());
            }

            i += page->size() - 1;
        }

        return rtn;
    }

    signature signature::from(const std::string &ida_pattern, protection required)
    {
        signature rtn;
        rtn.m_impl->required = required;

        auto pattern = std::stringstream{ida_pattern};
        std::string current;

        while (std::getline(pattern, current, ' '))
        {
            if (current.find('?') != std::string::npos)
            {
                rtn.m_impl->mask.push_back('?');
                rtn.m_impl->pattern.push_back(0);

                continue;
            }

            auto hex = std::stoul(current, nullptr, 16);

            rtn.m_impl->mask.push_back('x');
            rtn.m_impl->pattern.push_back(static_cast<char>(hex));
        }

        return rtn;
    }

    signature signature::from(const char *pattern, std::string mask, protection required)
    {
        signature rtn;
        rtn.m_impl->required = required;

        rtn.m_impl->pattern = std::string{pattern, mask.size()};
        rtn.m_impl->mask = std::move(mask);

        return rtn;
    }
} // namespace lime