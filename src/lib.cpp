#include "lib.impl.hpp"

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

            if (const auto open = (c == '<'); open && !escaped)
            {
                co_yield std::ranges::elements_of(emit({
                    .value = std::exchange(buffer, std::string{}),
                    .kind  = std::exchange(kind, lime::kind::regex),
                }));

                continue;
            }
            else if (open)
            {
                buffer.pop_back();
            }

            if (const auto close = (c == '>'); close && !escaped)
            {
                co_yield std::ranges::elements_of(emit({
                    .value = std::exchange(buffer, std::string{}),
                    .kind  = std::exchange(kind, lime::kind::plain),
                }));

                continue;
            }
            else if (close)
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

    pattern::pattern(impl data) : m_impl(std::make_unique<impl>(std::move(data))) {}

    pattern::pattern(const pattern &other) : pattern(*other.m_impl) {}

    pattern::pattern(pattern &&other) noexcept : m_impl(std::move(other.m_impl)) {}

    pattern::~pattern() = default;

    pattern &pattern::operator=(pattern other) noexcept
    {
        swap(*this, other);
        return *this;
    }

    void swap(pattern &first, pattern &second) noexcept
    {
        using std::swap;
        swap(first.m_impl, second.m_impl);
    }

    pattern pattern::icase() const
    {
        return impl{
            .raw   = m_impl->raw,
            .regex = std::regex{m_impl->raw, m_impl->regex.flags() | std::regex::icase},
        };
    }

    bool pattern::match(std::string_view value) const
    {
        return std::regex_search(value.begin(), value.end(), m_impl->regex);
    }

    pattern pattern::from(std::string_view value)
    {
        static const auto escape = [](const token &item)
        {
            if (item.kind == kind::regex)
            {
                return item.value;
            }

            return regex_escape(item.value);
        };

        auto string = parse(value)                    //
                      | std::views::transform(escape) //
                      | std::views::join              //
                      | std::ranges::to<std::string>();

        return impl{.raw = string, .regex = std::regex{string}};
    }

    pattern literals::operator""_re(const char *str, std::size_t len)
    {
        return pattern::from(std::string_view{str, len});
    }

    std::uintptr_t lib::end() const
    {
        return start() + size();
    }

    std::optional<std::uintptr_t> lib::symbol(const pattern &re) const
    {
        return symbol([&](const char *name) { return re.match(name); });
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
