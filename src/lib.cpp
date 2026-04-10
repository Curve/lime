#include "lib.hpp"

namespace lime
{
    std::regex literals::operator""_re(const char *str, std::size_t len)
    {
        // TODO: Windows should have this as icase!
        return std::regex{std::string{str, len}};
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
} // namespace lime
