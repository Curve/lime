#include "lib.hpp"

#include <ranges>
#include <generator>

namespace lime
{
    enum class kind : std::uint8_t
    {
        regex,
        plain,
    };

    struct token
    {
        std::string value;
        lime::kind kind;
    };

    std::generator<token> parse(std::string_view str)
    {
        auto buffer = std::string{};
        auto kind   = lime::kind::plain;

        static const auto emit = [](token what) -> std::generator<token>
        {
            if (what.value.empty())
            {
                co_return;
            }

            co_yield what;
        };

        for (auto c : str)
        {
            const auto last    = buffer.empty() ? '\0' : buffer.back();
            const auto escaped = last == '\\';

            if (!escaped && c == '<')
            {
                co_yield std::ranges::elements_of(emit({
                    .value = std::exchange(buffer, std::string{}),
                    .kind  = std::exchange(kind, lime::kind::regex),
                }));

                continue;
            }
            else if (c == '<')
            {
                buffer.pop_back();
            }

            if (!escaped && c == '>')
            {
                co_yield std::ranges::elements_of(emit({
                    .value = std::exchange(buffer, std::string{}),
                    .kind  = std::exchange(kind, lime::kind::plain),
                }));

                continue;
            }
            else if (c == '>')
            {
                buffer.pop_back();
            }

            buffer.push_back(c);
        }

        co_yield std::ranges::elements_of(emit({.value = buffer, .kind = kind}));
    }

    std::string regex_escape(const std::string &value)
    {
        static const auto meta = std::regex{R"([\.\^\$\+\(\)\[\]\{\}\|\?\*])"};
        return std::regex_replace(value, meta, "\\$&");
    }

    pattern literals::operator""_re(const char *str, std::size_t len)
    {
        static const auto escape = [](const token &item)
        {
            if (item.kind == kind::regex)
            {
                return item.value;
            }

            return regex_escape(item.value);
        };

        auto string = parse(std::string_view{str, len}) //
                      | std::views::transform(escape)   //
                      | std::views::join                //
                      | std::ranges::to<std::string>();

        return {.regex = std::regex{string}, .raw = string};
    }

    std::uintptr_t lib::end() const
    {
        return start() + size();
    }

    std::optional<std::uintptr_t> lib::symbol(const pattern &re) const
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
