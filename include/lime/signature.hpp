#pragma once

#include "lib.hpp"
#include "page.hpp"

#include <cstdint>

#include <string>
#include <string_view>

#include <vector>
#include <optional>

namespace lime
{
    class signature
    {
        template <auto>
        struct result;

      private:
        std::string m_mask;
        std::string m_pattern;

      public:
        enum class results : std::uint8_t
        {
            first,
            all,
        };

      public:
        template <results T>
        using result_t = result<T>::type;

      public:
        signature(std::string_view ida_pattern);
        signature(const char *, std::string mask);

      public:
        template <results T = results::first>
        [[nodiscard]] result_t<T> find(protection = protection::read) const;

        template <results T = results::first>
        [[nodiscard]] result_t<T> find(const lib &, protection = protection::read) const;

        template <results T = results::first>
        [[nodiscard]] result_t<T> find(const page &, protection = protection::read) const;
    };

    template <>
    struct signature::result<signature::results::first>
    {
        using type = std::optional<std::uintptr_t>;
    };

    template <>
    struct signature::result<signature::results::all>
    {
        using type = std::vector<std::uintptr_t>;
    };
} // namespace lime
