#include "signature.hpp"

#include <ranges>
#include <charconv>

namespace lime
{
    char htob(std::string_view hex)
    {
        std::uint8_t parsed = '\x00';
        std::from_chars(hex.data(), hex.data() + hex.size(), parsed, 16);
        return static_cast<char>(parsed);
    }

    bool matches(std::uintptr_t address, std::string_view mask, std::string_view pattern)
    {
        for (auto i = 0uz; mask.size() > i; ++i)
        {
            if (mask[i] == '?')
            {
                continue;
            }

            if (*reinterpret_cast<const char *>(address + i) == pattern[i])
            {
                continue;
            }

            return false;
        }

        return true;
    }

    signature::signature(std::string_view ida_pattern)
    {
        static constexpr auto transform = [](auto &&item)
        {
            return item.contains('?') ? '?' : 'x';
        };

        static constexpr auto upper = [](char c)
        {
            return std::toupper(c);
        };

        const auto split = ida_pattern                    //
                           | std::views::transform(upper) //
                           | std::views::split(' ')       //
                           | std::ranges::to<std::vector<std::string>>();

        m_pattern = split | std::views::transform(htob) | std::ranges::to<std::string>();
        m_mask    = split | std::views::transform(transform) | std::ranges::to<std::string>();
    }

    signature::signature(const char *pattern, std::string mask) : m_mask{std::move(mask)}, m_pattern{pattern, m_mask.size()} {}

    template <>
    signature::result_t<signature::results::first> signature::find<signature::results::first>(const page &, protection) const;

    template <>
    signature::result_t<signature::results::first> signature::find<signature::results::first>(protection prot) const
    {
        for (const auto &page : lime::page::pages())
        {
            auto result = find<results::first>(page, prot);

            if (!result.has_value())
            {
                continue;
            }

            return result;
        }

        return std::nullopt;
    }

    template <>
    signature::result_t<signature::results::first> signature::find<signature::results::first>(const lib &library, protection prot) const
    {
        const auto end = library.end() - m_mask.size();

        for (auto i = library.start(); end > i;)
        {
            const auto page = lime::page::at(i);

            if (!page.has_value())
            {
                ++i;
                continue;
            }

            if (auto res = find<results::first>(*page, prot); res.has_value())
            {
                return res;
            }

            i += page->size();
        }

        return std::nullopt;
    }

    template <>
    signature::result_t<signature::results::first> signature::find<signature::results::first>(const page &page, protection prot) const
    {
        if (!page.can(prot))
        {
            return std::nullopt;
        }

        const auto end = page.end() - m_mask.size();

        for (auto i = page.start(); end > i; ++i)
        {
            if (!matches(i, m_mask, m_pattern))
            {
                continue;
            }

            return i;
        }

        return std::nullopt;
    }

    template <>
    signature::result_t<signature::results::all> signature::find<signature::results::all>(const page &, protection) const;

    template <>
    signature::result_t<signature::results::all> signature::find<signature::results::all>(protection prot) const
    {
        auto rtn = std::vector<std::uintptr_t>{};

        for (const auto &page : lime::page::pages())
        {
            rtn.insert_range(rtn.end(), find<results::all>(page, prot));
        }

        return rtn;
    }

    template <>
    signature::result_t<signature::results::all> signature::find<signature::results::all>(const lib &library, protection prot) const
    {
        auto rtn       = std::vector<std::uintptr_t>{};
        const auto end = library.end() - m_mask.size();

        for (auto i = library.start(); end > i;)
        {
            const auto page = lime::page::at(i);

            if (!page.has_value())
            {
                ++i;
                continue;
            }

            rtn.insert_range(rtn.end(), find<results::all>(*page, prot));
            i += page->size();
        }

        return rtn;
    }

    template <>
    signature::result_t<signature::results::all> signature::find<signature::results::all>(const page &page, protection prot) const
    {
        if (!page.can(prot))
        {
            return {};
        }

        auto rtn       = std::vector<std::uintptr_t>{};
        const auto end = page.end() - m_mask.size();

        for (auto i = page.start(); end > i; ++i)
        {
            if (!matches(i, m_mask, m_pattern))
            {
                continue;
            }

            rtn.emplace_back(i);
        }

        return rtn;
    }
} // namespace lime
