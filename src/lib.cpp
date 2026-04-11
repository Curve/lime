#include "lib.hpp"

namespace lime
{
    regex literals::operator""_re(const char *str, std::size_t len)
    {
        auto string = std::string{str, len};
        return {.regex = std::regex{string}, .pattern = string};
    }

    std::uintptr_t lib::end() const
    {
        return start() + size();
    }

    std::optional<std::uintptr_t> lib::symbol(const regex &re) const
    {
        return symbol([&](std::string_view name) { return std::regex_search(name.begin(), name.end(), re.regex); });
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
