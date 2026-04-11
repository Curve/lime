#include "lib.hpp"

namespace lime
{
    std::uintptr_t lib::end() const
    {
        return start() + size();
    }

    std::optional<std::uintptr_t> lib::symbol(const std::regex &regex) const
    {
        return symbol([&](std::string_view name) { return std::regex_search(name.begin(), name.end(), regex); });
    }

    std::optional<lib> lib::find(const std::regex &regex)
    {
        return find(
            [&](const auto &item)
            {
                const auto name = item.name();
                return std::regex_search(name.begin(), name.end(), regex);
            });
    }

    std::optional<lib> lib::find(const lib_predicate &pred)
    {
        auto libraries = lib::libraries();
        auto it        = std::ranges::find_if(libraries, pred);

        if (it == libraries.end())
        {
            return std::nullopt;
        }

        return *it;
    }
} // namespace lime
